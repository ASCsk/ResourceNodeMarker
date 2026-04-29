#pragma once
#include "CoreMinimal.h"
#include "RNM_ResourceNodeInfo.h"
#include "RNM_ResourceVisuals.h"
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
     * Must be called before any other method.
     */
    void Initialize(
        const TArray<FResourceNodeInfo>& InResourceNodes,
        const TMap<FIntVector, TArray<int32>>& InSpatialGrid,
        URNM_ResourceVisuals* InResourceVisuals,
        const FResourceNodeMarker_ConfigStruct& InConfig);

    /**
     * Reads existing RNM:: markers from the map and rebuilds cluster state from them.
     * Does not create any new markers.
     */
    void RebuildClustersFromExistingMarkers(UWorld* World);

    /**
     * Deletes RNM map markers (legacy and RNM::#class-id categories).
     */
    void DeleteAllRNMMarkers(UWorld* World);

    /**
     * Creates one map marker per cluster (or one per node when bClusterNodes is false).
     * Should be called after DeleteAllRNMMarkers.
     */
    /** @return Number of markers created successfully. */
    int32 CreateAllClusterMarkers(UWorld* World);

    /**
     * Called when the player discovers a new node.
     * When clustering is enabled: adds to a neighbor cluster or merges; when disabled, always a solo marker.
     */
    void OnNodeDiscovered(UWorld* World, int32 NodeIndex);

    /** Returns true if the node at the given index has already been discovered. */
    bool IsNodeDiscovered(int32 NodeIndex) const;

    void MarkNodeDiscovered(int32 NodeIndex);

    /**
     * Called when an extractor is placed on a node.
     * Marks node as discovered, removes it from its cluster,
     * deletes the cluster marker and recreates it if nodes remain.
     */
     void OnExtractorPlaced(UWorld* World, const FVector& NodeLocation);

private:
    void MergeClusters(int32 TargetIndex, int32 SourceIndex);
    void ApplySoloClusteringLayoutIfNeeded();
    int32 FindResourceNodeIndex(const FResourceNodeInfo& Info) const;

    float ClusterRadiusSq = 0.0f;
    float ClusterHeightTolerance = 0.0f;

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