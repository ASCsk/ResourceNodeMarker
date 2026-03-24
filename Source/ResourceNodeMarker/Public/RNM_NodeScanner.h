#pragma once
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FGResourceNode.h"
#include "RNM_ResourceNodeInfo.h"

class RNM_NodeScanner
{
public:
    RNM_NodeScanner() = default;
    ~RNM_NodeScanner() = default;

    /**
     * Scan the world for extractable unoccupied resource nodes.
     * @param World - The world to scan.
     * @param OutNodes - Array to store found nodes.
     */
    static void ScanNodes(UWorld* World, TArray<FResourceNodeInfo>& OutNodes);

    /**
     * Builds a spatial grid from scanned nodes for efficient proximity lookups.
     * @param Nodes - The scanned nodes to index.
     * @param OutGrid - Map of grid cell to node indices.
     */
    static void BuildSpatialGrid(const TArray<FResourceNodeInfo>& Nodes, TMap<FIntVector, TArray<int32>>& OutGrid);

    /**
     * Returns the grid cell for a given world location.
     */
    static FIntVector GetGridCell(const FVector& Location);

    /**
     * Returns all node indices in the given cell and its 8 neighbors.
     */
    static TArray<int32> GetNeighborCellIndices(const TMap<FIntVector, TArray<int32>>& Grid, const FIntVector& Cell);

    // Cluster radius — max distance between two nodes to be considered neighbors
    static constexpr float CLUSTER_RADIUS_Z_HEIGHT = 10000.0f; // 100m in cm
    static constexpr float CLUSTER_RADIUS = 40000.0f; // 400m in cm
    static constexpr float CLUSTER_RADIUS_SQ = CLUSTER_RADIUS * CLUSTER_RADIUS;
};