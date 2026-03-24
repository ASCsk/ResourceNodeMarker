#include "RNM_NodeScanner.h"
#include "ResourceNodeMarker.h"

void RNM_NodeScanner::ScanNodes(UWorld* World, TArray<FResourceNodeInfo>& OutNodes)
{
    if (!World) return;

    OutNodes.Reset();

    for (TActorIterator<AFGResourceNode> It(World); It; ++It)
    {
        AFGResourceNode* Node = *It;
        if (!Node) continue;
        if (!Node->CanPlaceResourceExtractor()) continue;
        if (Node->IsOccupied()) continue;

        FResourceNodeInfo Info;
        Info.NodeActor = Node;
        Info.Location = Node->GetActorLocation();
        Info.ResourceName = FName(*Node->GetResourceName().ToString());
        Info.Purity = Node->GetResoucePurity();

        OutNodes.Add(Info);
    }

    UE_LOG(LogResourceNodeMarker, Log,
        TEXT("RNM_NodeScanner: Found %d extractable unoccupied nodes"), OutNodes.Num());
}

FIntVector RNM_NodeScanner::GetGridCell(const FVector& Location)
{
    return FIntVector(
        FMath::FloorToInt(Location.X / CLUSTER_RADIUS),
        FMath::FloorToInt(Location.Y / CLUSTER_RADIUS),
        0 // 2D grid, Z handled separately via distance check
    );
}

void RNM_NodeScanner::BuildSpatialGrid(const TArray<FResourceNodeInfo>& Nodes, TMap<FIntVector, TArray<int32>>& OutGrid)
{
    OutGrid.Reset();

    for (int32 i = 0; i < Nodes.Num(); i++)
    {
        FIntVector Cell = GetGridCell(Nodes[i].Location);
        OutGrid.FindOrAdd(Cell).Add(i);
    }

    UE_LOG(LogResourceNodeMarker, Log,
        TEXT("RNM_NodeScanner: Built spatial grid with %d cells for %d nodes"),
        OutGrid.Num(), Nodes.Num());
}

TArray<int32> RNM_NodeScanner::GetNeighborCellIndices(const TMap<FIntVector, TArray<int32>>& Grid, const FIntVector& Cell)
{
    TArray<int32> Result;

    for (int32 dx = -1; dx <= 1; dx++)
    {
        for (int32 dy = -1; dy <= 1; dy++)
        {
            FIntVector NeighborCell(Cell.X + dx, Cell.Y + dy, 0);
            if (const TArray<int32>* Indices = Grid.Find(NeighborCell))
            {
                Result.Append(*Indices);
            }
        }
    }

    return Result;
}