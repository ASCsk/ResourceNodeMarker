#pragma once
#include "CoreMinimal.h"
#include "RNM_ResourceNodeInfo.h"
#include "RNM_ResourceVisuals.h"
#include "RNM_NodeScanner.h"
#include "ResourceNodeMarker_ConfigStruct.h"
#include "FGMapManager.h"
#include "RNM_ClusterManager.generated.h"

UCLASS()
class RESOURCENODEMARKER_API URNM_ClusterManager : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Initializes the cluster manager with required dependencies.
     * Must be called before any other method; resets all discovery state.
     * @param InResourceNodes - Scanned nodes (owned by URNM_WorldSubsystem).
     * @param InSpatialGrid - Cell index built from InResourceNodes.
     * @param InResourceVisuals - Icon and purity color lookup.
     * @param InConfig - Active mod configuration (radius, clustering, icons).
     */
    void Initialize(
        const TArray<FResourceNodeInfo>& InResourceNodes,
        const TMap<FIntVector, TArray<int32>>& InSpatialGrid,
        URNM_ResourceVisuals* InResourceVisuals,
        const FResourceNodeMarker_ConfigStruct& InConfig);

    /**
     * Rebuilds in-memory cluster state from existing RNM map markers.
     * Does not create or delete markers; used before a full marker refresh on load.
     * @param World - The game world (reads markers via AFGMapManager).
     */
    void RebuildClustersFromExistingMarkers(UWorld* World);

    /**
     * Removes every map marker whose CategoryName is RNM-owned.
     * @param World - The game world.
     */
    void DeleteAllRNMMarkers(UWorld* World);

    /**
     * Creates one map marker per cluster (or one per node when clustering is disabled).
     * Call after DeleteAllRNMMarkers when replacing all markers on load.
     * @param World - The game world.
     * @return Number of markers created successfully.
     */
    int32 CreateAllClusterMarkers(UWorld* World);

    /**
     * Handles proximity discovery for a single node index.
     * Clusters or merges neighbors when enabled; rolls back state if marker create fails.
     * @param World - The game world.
     * @param NodeIndex - Index into the scanned ResourceNodes array.
     */
    void OnNodeDiscovered(UWorld* World, int32 NodeIndex);

    /**
     * Returns whether the node has been discovered (marker placed or extractor handled).
     * @param NodeIndex - Index into the scanned ResourceNodes array.
     */
    bool IsNodeDiscovered(int32 NodeIndex) const;

    /**
     * Updates cluster state when an extractor is placed on a node.
     * Removes the node from its cluster and recreates or deletes the cluster marker.
     * Rolls back the node removal if marker update fails.
     * @param World - The game world.
     * @param NodeLocation - World location of the resource node under the extractor.
     */
    void OnExtractorPlaced(UWorld* World, const FVector& NodeLocation);

private:
    bool MergeClusters(UWorld* World, int32 TargetIndex, int32 SourceIndex, bool bRemoveSourceMarker = true);
    bool TryRemoveClusterMapMarker(UWorld* World, FResourceNodeCluster& Cluster);
    void ApplySoloClusteringLayoutIfNeeded();
    int32 FindResourceNodeIndex(const FResourceNodeInfo& Info) const;

    float ClusterRadiusSq = 0.0f;
    float ClusterHeightTolerance = 0.0f;
    float GridCellSizeCm = RNM_NodeScanner::DEFAULT_CLUSTER_RADIUS_CM;

private:
    // References to subsystem-owned data
    const TArray<FResourceNodeInfo>* ResourceNodes = nullptr;
    const TMap<FIntVector, TArray<int32>>* SpatialGrid = nullptr;

    UPROPERTY()
    URNM_ResourceVisuals* ResourceVisuals = nullptr;

    FResourceNodeMarker_ConfigStruct Config;

    // Runtime cluster state
    TArray<FResourceNodeCluster> DiscoveredClusters;
    TMap<int32, int32> NodeToClusterMap;
    TSet<int32> DiscoveredNodeIndices;
};