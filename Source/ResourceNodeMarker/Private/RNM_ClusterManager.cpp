#include "RNM_ClusterManager.h"
#include "RNM_NodeScanner.h"
#include "RNM_MapMarkerService.h"
#include "ResourceNodeMarker.h"
#include "FGMapManager.h"

void URNM_ClusterManager::Initialize(
    const TArray<FResourceNodeInfo>& InResourceNodes,
    const TMap<FIntVector, TArray<int32>>& InSpatialGrid,
    URNM_ResourceVisuals* InResourceVisuals,
    const FResourceNodeMarker_ConfigStruct& InConfig)
{
    ResourceNodes = &InResourceNodes;
    SpatialGrid = &InSpatialGrid;
    ResourceVisuals = InResourceVisuals;
    Config = InConfig;

    DiscoveredClusters.Reset();
    NodeToClusterMap.Reset();
    DiscoveredNodeIndices.Reset();
}

void URNM_ClusterManager::MigrateOldMarkers(UWorld* World)
{
    if (!World) return;

    AFGMapManager* MapManager = AFGMapManager::Get(World);
    if (!MapManager) return;

    TArray<FMapMarker> AllMarkers;
    MapManager->GetMapMarkers(AllMarkers);

    TArray<FMapMarker> MarkersToMigrate;
    for (const FMapMarker& Marker : AllMarkers)
    {
        if (Marker.CategoryName == TEXT("Resources") ||
            Marker.CategoryName == TEXT("Ore"))
        {
            MarkersToMigrate.Add(Marker);
        }
    }

    for (FMapMarker& Marker : MarkersToMigrate)
    {
        Marker.CategoryName = TEXT("RNM::Ore");
        MapManager->UpdateMapMarker(Marker);
    }

    UE_LOG(LogResourceNodeMarker, Log,
        TEXT("RNM_ClusterManager: Migrated %d old markers to RNM:: category"),
        MarkersToMigrate.Num());
}

void URNM_ClusterManager::RebuildClustersFromExistingMarkers(UWorld* World)
{
    if (!World || !ResourceNodes || !SpatialGrid) return;

    AFGMapManager* MapManager = AFGMapManager::Get(World);
    if (!MapManager) return;

    TArray<FMapMarker> ExistingMarkers;
    MapManager->GetMapMarkers(ExistingMarkers);

    for (const FMapMarker& Marker : ExistingMarkers)
    {
        if (Marker.CategoryName != TEXT("RNM::Ore") &&
            Marker.CategoryName != TEXT("RNM::Fluid"))
            continue;

        FIntVector Cell = RNM_NodeScanner::GetGridCell(Marker.Location);
        TArray<int32> CandidateIndices = RNM_NodeScanner::GetNeighborCellIndices(*SpatialGrid, Cell);

        TArray<int32> ClusterNodeIndices;
        for (int32 NodeIndex : CandidateIndices)
        {
            if (DiscoveredNodeIndices.Contains(NodeIndex))
                continue;

            const FResourceNodeInfo& Node = (*ResourceNodes)[NodeIndex];

            if (Node.ResourceName != FName(*Marker.Name.Left(Marker.Name.Find(TEXT(" (")))))
                continue;

            if (FVector::DistSquared(Node.Location, Marker.Location) <= RNM_NodeScanner::CLUSTER_RADIUS_SQ)
                ClusterNodeIndices.Add(NodeIndex);
        }

        if (ClusterNodeIndices.Num() == 0)
            continue;

        FResourceNodeCluster Cluster;
        for (int32 NodeIndex : ClusterNodeIndices)
        {
            Cluster.Nodes.Add((*ResourceNodes)[NodeIndex]);
            Cluster.ResourceName = (*ResourceNodes)[NodeIndex].ResourceName;
            DiscoveredNodeIndices.Add(NodeIndex);
        }

        Cluster.RecalculateCenter();
        Cluster.RecalculateDominantPurity();

        int32 ClusterIndex = DiscoveredClusters.Add(Cluster);

        for (int32 NodeIndex : ClusterNodeIndices)
            NodeToClusterMap.Add(NodeIndex, ClusterIndex);

        UE_LOG(LogResourceNodeMarker, Log,
            TEXT("RNM_ClusterManager: Rebuilt cluster for %s with %d nodes"),
            *Cluster.ResourceName.ToString(),
            Cluster.Nodes.Num());
    }

    UE_LOG(LogResourceNodeMarker, Log,
        TEXT("RNM_ClusterManager: Rebuilt %d clusters from existing markers"),
        DiscoveredClusters.Num());
}

void URNM_ClusterManager::DeleteAllRNMMarkers(UWorld* World)
{
    if (!World) return;

    AFGMapManager* MapManager = AFGMapManager::Get(World);
    if (!MapManager) return;

    TArray<FMapMarker> AllMarkers;
    MapManager->GetMapMarkers(AllMarkers);

    TArray<FMapMarker> MarkersToRemove;
    for (const FMapMarker& Marker : AllMarkers)
    {
        if (Marker.CategoryName == TEXT("RNM::Ore") ||
            Marker.CategoryName == TEXT("RNM::Fluid"))
        {
            MarkersToRemove.Add(Marker);
        }
    }

    for (const FMapMarker& Marker : MarkersToRemove)
        MapManager->RemoveMapMarker(Marker);

    UE_LOG(LogResourceNodeMarker, Log,
        TEXT("RNM_ClusterManager: Deleted %d existing RNM markers"),
        MarkersToRemove.Num());
}

void URNM_ClusterManager::CreateAllClusterMarkers(UWorld* World)
{
    if (!World) return;

    int32 CreatedCount = 0;
    for (FResourceNodeCluster& Cluster : DiscoveredClusters)
    {
        if (Cluster.Nodes.Num() == 0) continue;

        FGuid NewGUID;
        if (RNM_MapMarkerService::CreateOrUpdateClusterMarker(
            World, Cluster, ResourceVisuals, Config, NewGUID))
        {
            Cluster.CurrentMarkerGUID = NewGUID;
            CreatedCount++;
        }
    }

    UE_LOG(LogResourceNodeMarker, Log,
        TEXT("RNM_ClusterManager: Created %d cluster markers"), CreatedCount);
}

bool URNM_ClusterManager::IsNodeDiscovered(int32 NodeIndex) const
{
    return DiscoveredNodeIndices.Contains(NodeIndex);
}

void URNM_ClusterManager::MergeClusters(int32 TargetIndex, int32 SourceIndex)
{
    // Remap all nodes from source to target
    for (auto& Pair : NodeToClusterMap)
    {
        if (Pair.Value == SourceIndex)
            Pair.Value = TargetIndex;
    }

    DiscoveredClusters[TargetIndex].MergeWith(DiscoveredClusters[SourceIndex]);

    // Invalidate source
    DiscoveredClusters[SourceIndex].Nodes.Reset();
    DiscoveredClusters[SourceIndex].CurrentMarkerGUID.Invalidate();
}

void URNM_ClusterManager::OnNodeDiscovered(UWorld* World, int32 NodeIndex)
{
    if (!ResourceNodes || !SpatialGrid) return;

    const FResourceNodeInfo& NewNode = (*ResourceNodes)[NodeIndex];

    DiscoveredNodeIndices.Add(NodeIndex);

    FIntVector Cell = RNM_NodeScanner::GetGridCell(NewNode.Location);
    TArray<int32> CandidateIndices = RNM_NodeScanner::GetNeighborCellIndices(*SpatialGrid, Cell);

    TSet<int32> NeighborClusterIndices;
    for (int32 CandidateIndex : CandidateIndices)
    {
        if (CandidateIndex == NodeIndex) continue;
        if (!DiscoveredNodeIndices.Contains(CandidateIndex)) continue;

        const FResourceNodeInfo& Candidate = (*ResourceNodes)[CandidateIndex];

        if (Candidate.ResourceName != NewNode.ResourceName) continue;
        if (FVector::DistSquared(NewNode.Location, Candidate.Location) > RNM_NodeScanner::CLUSTER_RADIUS_SQ) continue;
        if (FMath::Abs(NewNode.Location.Z - Candidate.Location.Z) > RNM_NodeScanner::CLUSTER_RADIUS_Z_HEIGHT) continue;

        if (const int32* ClusterIdx = NodeToClusterMap.Find(CandidateIndex))
            NeighborClusterIndices.Add(*ClusterIdx);
    }

    if (NeighborClusterIndices.Num() == 0)
    {
        FResourceNodeCluster NewCluster;
        NewCluster.ResourceName = NewNode.ResourceName;
        NewCluster.Nodes.Add(NewNode);
        NewCluster.RecalculateCenter();
        NewCluster.RecalculateDominantPurity();

        int32 ClusterIndex = DiscoveredClusters.Add(NewCluster);
        NodeToClusterMap.Add(NodeIndex, ClusterIndex);

        FGuid NewGUID;
        if (RNM_MapMarkerService::CreateOrUpdateClusterMarker(
            World, DiscoveredClusters[ClusterIndex], ResourceVisuals, Config, NewGUID))
        {
            DiscoveredClusters[ClusterIndex].CurrentMarkerGUID = NewGUID;
        }

        UE_LOG(LogResourceNodeMarker, Log,
            TEXT("RNM_ClusterManager: New solo cluster for %s"),
            *NewNode.ResourceName.ToString());
    }
    else if (NeighborClusterIndices.Num() == 1)
    {
        int32 ClusterIndex = *NeighborClusterIndices.CreateConstIterator();
        DiscoveredClusters[ClusterIndex].Nodes.Add(NewNode);
        DiscoveredClusters[ClusterIndex].RecalculateCenter();
        DiscoveredClusters[ClusterIndex].RecalculateDominantPurity();
        NodeToClusterMap.Add(NodeIndex, ClusterIndex);

        FGuid NewGUID;
        if (RNM_MapMarkerService::CreateOrUpdateClusterMarker(
            World, DiscoveredClusters[ClusterIndex], ResourceVisuals, Config, NewGUID))
        {
            DiscoveredClusters[ClusterIndex].CurrentMarkerGUID = NewGUID;
        }

        UE_LOG(LogResourceNodeMarker, Log,
            TEXT("RNM_ClusterManager: Added node to cluster for %s (%d nodes)"),
            *NewNode.ResourceName.ToString(),
            DiscoveredClusters[ClusterIndex].Nodes.Num());
    }
    else
    {
        TArray<int32> ClusterIndices = NeighborClusterIndices.Array();
        int32 TargetClusterIndex = ClusterIndices[0];

        DiscoveredClusters[TargetClusterIndex].Nodes.Add(NewNode);
        NodeToClusterMap.Add(NodeIndex, TargetClusterIndex);

        for (int32 i = 1; i < ClusterIndices.Num(); i++)
            MergeClusters(TargetClusterIndex, ClusterIndices[i]);

        DiscoveredClusters[TargetClusterIndex].RecalculateCenter();
        DiscoveredClusters[TargetClusterIndex].RecalculateDominantPurity();

        FGuid NewGUID;
        if (RNM_MapMarkerService::CreateOrUpdateClusterMarker(
            World, DiscoveredClusters[TargetClusterIndex], ResourceVisuals, Config, NewGUID))
        {
            DiscoveredClusters[TargetClusterIndex].CurrentMarkerGUID = NewGUID;
        }

        UE_LOG(LogResourceNodeMarker, Log,
            TEXT("RNM_ClusterManager: Merged %d clusters for %s (%d nodes)"),
            ClusterIndices.Num(),
            *NewNode.ResourceName.ToString(),
            DiscoveredClusters[TargetClusterIndex].Nodes.Num());
    }
}