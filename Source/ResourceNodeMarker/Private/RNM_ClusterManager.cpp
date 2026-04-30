#include "RNM_ClusterManager.h"
#include "RNM_NodeScanner.h"
#include "RNM_MapMarkerService.h"
#include "ResourceNodeMarker.h"
#include "FGItemDescriptor.h"
#include "FGMapManager.h"
#include "FGResourceDescriptor.h"

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
    ClusterRadiusSq = (InConfig.ClusterRadius * 100.0f) * (InConfig.ClusterRadius * 100.0f);
    ClusterHeightTolerance = InConfig.ClusterHeightTolerance * 100.0f;

    DiscoveredClusters.Reset();
    NodeToClusterMap.Reset();
    DiscoveredNodeIndices.Reset();
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
        if (!RNM_MapMarkerService::IsRNMMapMarkerCategory(Marker.CategoryName))
            continue;

        FName StableClassId;
        bool bHasStableId = RNM_MapMarkerService::TryParseClassIdFromCategory(
            Marker.CategoryName, StableClassId);
        if (!bHasStableId)
            bHasStableId = RNM_MapMarkerService::TryParseClassIdFromMarkerName(Marker.Name, StableClassId);

        FIntVector Cell = RNM_NodeScanner::GetGridCell(Marker.Location);
        TArray<int32> CandidateIndices = RNM_NodeScanner::GetNeighborCellIndices(*SpatialGrid, Cell);

        FString NameForLegacy = Marker.Name;
        {
            static const FString RnmTag(TEXT(" #RNM:"));
            const int32 TagIdx = NameForLegacy.Find(RnmTag);
            if (TagIdx != INDEX_NONE)
                NameForLegacy = NameForLegacy.Left(TagIdx);
        }
        const int32 ParenIdx = NameForLegacy.Find(TEXT(" ("));
        const FString LegacyPrefix = (ParenIdx != INDEX_NONE) ? NameForLegacy.Left(ParenIdx) : NameForLegacy;

        TArray<int32> ClusterNodeIndices;
        for (int32 NodeIndex : CandidateIndices)
        {
            if (DiscoveredNodeIndices.Contains(NodeIndex)) continue;

            const FResourceNodeInfo& Node = (*ResourceNodes)[NodeIndex];
            if (FVector::DistSquared(Node.Location, Marker.Location) > RNM_NodeScanner::CLUSTER_RADIUS_SQ) continue;

            if (bHasStableId)
            {
                if (Node.ResourceName != StableClassId) continue;
            }
            else
            {
                if (!Node.NodeActor) continue;
                TSubclassOf<UFGResourceDescriptor> Cls = Node.NodeActor->GetResourceClass();
                if (!Cls) continue;
                const FName LegacyName(
                    *UFGItemDescriptor::GetItemName(Cls).ToString());
                const FName LegacyName2(
                    *Node.NodeActor->GetResourceName().ToString());
                const FName Pfx = FName(*LegacyPrefix);
                if (Pfx != LegacyName && Pfx != LegacyName2) continue;
            }
            ClusterNodeIndices.Add(NodeIndex);
        }

        if (ClusterNodeIndices.Num() == 0 && !bHasStableId)
        {
            TSet<FName> ClassIdsInRange;
            for (int32 NodeIndex : CandidateIndices)
            {
                if (DiscoveredNodeIndices.Contains(NodeIndex)) continue;
                const FResourceNodeInfo& Node = (*ResourceNodes)[NodeIndex];
                if (FVector::DistSquared(Node.Location, Marker.Location) > RNM_NodeScanner::CLUSTER_RADIUS_SQ) continue;
                ClassIdsInRange.Add(Node.ResourceName);
            }
            if (ClassIdsInRange.Num() == 1)
            {
                for (int32 NodeIndex : CandidateIndices)
                {
                    if (DiscoveredNodeIndices.Contains(NodeIndex)) continue;
                    const FResourceNodeInfo& Node = (*ResourceNodes)[NodeIndex];
                    if (FVector::DistSquared(Node.Location, Marker.Location) > RNM_NodeScanner::CLUSTER_RADIUS_SQ) continue;
                    ClusterNodeIndices.AddUnique(NodeIndex);
                }
            }
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

    ApplySoloClusteringLayoutIfNeeded();

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
        if (RNM_MapMarkerService::IsRNMMapMarkerCategory(Marker.CategoryName))
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

int32 URNM_ClusterManager::CreateAllClusterMarkers(UWorld* World)
{
    if (!World) return 0;

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
    return CreatedCount;
}

bool URNM_ClusterManager::IsNodeDiscovered(int32 NodeIndex) const
{
    return DiscoveredNodeIndices.Contains(NodeIndex);
}

int32 URNM_ClusterManager::FindResourceNodeIndex(const FResourceNodeInfo& Info) const
{
    if (!ResourceNodes) return INDEX_NONE;

    for (int32 i = 0; i < ResourceNodes->Num(); i++)
    {
        const FResourceNodeInfo& A = (*ResourceNodes)[i];
        if (Info.NodeActor && A.NodeActor == Info.NodeActor) return i;
    }

    static constexpr float MatchEpsilonSq = 1.0f; // 1 uu; nodes are from same scan, float noise only
    for (int32 i = 0; i < ResourceNodes->Num(); i++)
    {
        const FResourceNodeInfo& A = (*ResourceNodes)[i];
        if (A.ResourceName == Info.ResourceName
            && FVector::DistSquared(A.Location, Info.Location) <= MatchEpsilonSq)
        {
            return i;
        }
    }

    return INDEX_NONE;
}

void URNM_ClusterManager::ApplySoloClusteringLayoutIfNeeded()
{
    if (Config.bClusterNodes) return;

    TArray<FResourceNodeCluster> Split;
    TMap<int32, int32> NewNodeToCluster;
    NewNodeToCluster.Reserve(DiscoveredNodeIndices.Num());

    for (int32 OldIdx = 0; OldIdx < DiscoveredClusters.Num(); ++OldIdx)
    {
        FResourceNodeCluster& C = DiscoveredClusters[OldIdx];
        if (C.Nodes.Num() == 0) continue;

        if (C.Nodes.Num() == 1)
        {
            const int32 NewIdx = Split.Add(C);
            if (const int32 NodeIdx = FindResourceNodeIndex(C.Nodes[0]);
                NodeIdx != INDEX_NONE)
            {
                NewNodeToCluster.Add(NodeIdx, NewIdx);
            }
        }
        else
        {
            for (const FResourceNodeInfo& N : C.Nodes)
            {
                FResourceNodeCluster One;
                One.ResourceName = N.ResourceName;
                One.Nodes.Add(N);
                One.RecalculateCenter();
                One.RecalculateDominantPurity();

                const int32 NewIdx = Split.Add(One);
                if (const int32 NodeIdx = FindResourceNodeIndex(N);
                    NodeIdx != INDEX_NONE)
                {
                    NewNodeToCluster.Add(NodeIdx, NewIdx);
                }
            }
        }
    }

    DiscoveredClusters = MoveTemp(Split);
    NodeToClusterMap = MoveTemp(NewNodeToCluster);
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

    if (!Config.bClusterNodes)
    {
        FResourceNodeCluster NewCluster;
        NewCluster.ResourceName = NewNode.ResourceName;
        NewCluster.Nodes.Add(NewNode);
        NewCluster.RecalculateCenter();
        NewCluster.RecalculateDominantPurity();

        const int32 ClusterIndex = DiscoveredClusters.Add(NewCluster);
        NodeToClusterMap.Add(NodeIndex, ClusterIndex);

        FGuid NewGUID;
        if (RNM_MapMarkerService::CreateOrUpdateClusterMarker(
            World, DiscoveredClusters[ClusterIndex], ResourceVisuals, Config, NewGUID))
        {
            DiscoveredClusters[ClusterIndex].CurrentMarkerGUID = NewGUID;
        }

        UE_LOG(LogResourceNodeMarker, Log,
            TEXT("RNM_ClusterManager: Per-node marker (clustering off) for %s"),
            *NewNode.ResourceName.ToString());
        return;
    }

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
        if (FMath::Abs(NewNode.Location.Z - Candidate.Location.Z) > ClusterHeightTolerance) continue;

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

void URNM_ClusterManager::MarkNodeDiscovered(int32 NodeIndex)
{
    DiscoveredNodeIndices.Add(NodeIndex);
}

void URNM_ClusterManager::OnExtractorPlaced(UWorld* World, const FVector& NodeLocation)
{
    if (!ResourceNodes) return;

    // Find node index by location
    int32 FoundNodeIndex = -1;
    for (int32 i = 0; i < ResourceNodes->Num(); i++)
    {
        if (FVector::DistSquared((*ResourceNodes)[i].Location, NodeLocation) <= RNM_MapMarkerService::MARKER_LOCATION_TOLERANCE_SQ)
        {
            FoundNodeIndex = i;
            break;
        }
    }

    if (FoundNodeIndex == -1)
    {
        UE_LOG(LogResourceNodeMarker, Warning,
            TEXT("RNM_ClusterManager: Could not find node at extractor location %s"),
            *NodeLocation.ToString());
        return;
    }

    // Mark as discovered so proximity check never triggers for it again
    DiscoveredNodeIndices.Add(FoundNodeIndex);

    // Find which cluster this node belongs to
    const int32* ClusterIdxPtr = NodeToClusterMap.Find(FoundNodeIndex);
    if (!ClusterIdxPtr)
    {
        // Node was never discovered/clustered, nothing to update
        UE_LOG(LogResourceNodeMarker, Log,
            TEXT("RNM_ClusterManager: Extractor placed on undiscovered node, marked as discovered"));
        return;
    }

    int32 ClusterIndex = *ClusterIdxPtr;
    FResourceNodeCluster& Cluster = DiscoveredClusters[ClusterIndex];

    // Delete existing cluster marker
    if (Cluster.CurrentMarkerGUID.IsValid())
    {
        AFGMapManager* MapManager = AFGMapManager::Get(World);
        if (MapManager)
        {
            MapManager->Authority_RemoveMapMarkerByID(Cluster.CurrentMarkerGUID);
            Cluster.CurrentMarkerGUID.Invalidate();

            UE_LOG(LogResourceNodeMarker, Log,
                TEXT("RNM_ClusterManager: Removed cluster marker for %s due to extractor placement"),
                *Cluster.ResourceName.ToString());
        }
    }

    // Remove the tapped node from the cluster
    Cluster.Nodes.RemoveAll([&](const FResourceNodeInfo& Node)
        {
            return FVector::DistSquared(Node.Location, NodeLocation) <= RNM_MapMarkerService::MARKER_LOCATION_TOLERANCE_SQ;
        });

    // If cluster still has nodes recreate marker
    if (Cluster.Nodes.Num() > 0)
    {
        Cluster.RecalculateCenter();
        Cluster.RecalculateDominantPurity();

        FGuid NewGUID;
        if (RNM_MapMarkerService::CreateOrUpdateClusterMarker(
            World, Cluster, ResourceVisuals, Config, NewGUID))
        {
            Cluster.CurrentMarkerGUID = NewGUID;

            UE_LOG(LogResourceNodeMarker, Log,
                TEXT("RNM_ClusterManager: Recreated cluster marker for %s (%d nodes remaining)"),
                *Cluster.ResourceName.ToString(),
                Cluster.Nodes.Num());
        }
    }
    else
    {
        UE_LOG(LogResourceNodeMarker, Log,
            TEXT("RNM_ClusterManager: Cluster for %s is now empty, marker deleted"),
            *Cluster.ResourceName.ToString());
    }
}