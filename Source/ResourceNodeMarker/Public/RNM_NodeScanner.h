#pragma once
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FGResourceNode.h"
#include "RNM_ResourceNodeInfo.h"

/** World scan and 2D spatial indexing for resource node proximity queries. */
class RNM_NodeScanner
{
public:
    RNM_NodeScanner() = default;
    ~RNM_NodeScanner() = default;

    /**
     * Scans the world for extractable, unoccupied resource nodes.
     * @param World - The world to scan.
     * @param OutNodes - Filled with node actor, location, class FName, and purity.
     */
    static void ScanNodes(UWorld* World, TArray<FResourceNodeInfo>& OutNodes);

    /**
     * Builds a 2D spatial grid from scanned nodes for neighbor lookups.
     * @param Nodes - The scanned nodes to index.
     * @param OutGrid - Map of grid cell to node indices in Nodes.
     * @param CellSizeCm - Cell edge length in cm; should match configured cluster radius.
     */
    static void BuildSpatialGrid(
        const TArray<FResourceNodeInfo>& Nodes,
        TMap<FIntVector, TArray<int32>>& OutGrid,
        float CellSizeCm);

    /**
     * Returns the grid cell containing a world location (Z ignored).
     * @param Location - World position in cm.
     * @param CellSizeCm - Cell edge length in cm; should match configured cluster radius.
     */
    static FIntVector GetGridCell(const FVector& Location, float CellSizeCm);

    /**
     * Returns node indices in the given cell and its eight neighbors.
     * @param Grid - Spatial grid produced by BuildSpatialGrid.
     * @param Cell - Center cell from GetGridCell.
     * @return Flat list of indices into the original Nodes array.
     */
    static TArray<int32> GetNeighborCellIndices(const TMap<FIntVector, TArray<int32>>& Grid, const FIntVector& Cell);

    /** Default cluster radius when config is unavailable (250 m, in cm). */
    static constexpr float DEFAULT_CLUSTER_RADIUS_CM = 25000.0f;
};
