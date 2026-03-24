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
     * Migrates markers from old categories (Resources, Ore) to RNM:: categories.
     * Should only run once per installation via migration flag.
     */
    void MigrateOldMarkers(UWorld* World);

    /**
     * Reads existing RNM:: markers from the map and rebuilds cluster state from them.
     * Does not create any new markers.
     */
    void RebuildClustersFromExistingMarkers(UWorld* World);

    /**
     * Deletes all markers under RNM::Ore and RNM::Fluid categories.
     */
    void DeleteAllRNMMarkers(UWorld* World);

    /**
     * Creates one marker per discovered cluster.
     * Should be called after DeleteAllRNMMarkers.
     */
    void CreateAllClusterMarkers(UWorld* World);

    /**
     * Called when the player discovers a new node.
     * Handles solo cluster creation, adding to existing cluster, or merging clusters.
     */
    void OnNodeDiscovered(UWorld* World, int32 NodeIndex);

    /** Returns true if the node at the given index has already been discovered. */
    bool IsNodeDiscovered(int32 NodeIndex) const;

private:
    int32 FindOrCreateCluster(int32 NodeIndex);
    void MergeClusters(int32 TargetIndex, int32 SourceIndex);

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