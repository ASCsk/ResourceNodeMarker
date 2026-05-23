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
    static void BuildSpatialGrid(
        const TArray<FResourceNodeInfo>& Nodes,
        TMap<FIntVector, TArray<int32>>& OutGrid,
        float CellSizeCm);

    /** Returns the grid cell for a given world location. CellSizeCm should match Config.ClusterRadius * 100. */
    static FIntVector GetGridCell(const FVector& Location, float CellSizeCm);

    /**
     * Returns all node indices in the given cell and its 8 neighbors.
     */
    static TArray<int32> GetNeighborCellIndices(const TMap<FIntVector, TArray<int32>>& Grid, const FIntVector& Cell);

    /** Default cluster radius when config is unavailable (250m in cm). */
    static constexpr float DEFAULT_CLUSTER_RADIUS_CM = 25000.0f;
};
