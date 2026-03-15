#include "RNM_NodeScanner.h"

void RNM_NodeScanner::ScanNodes(UWorld* World, TArray<FResourceNodeInfo>& OutNodes)
{
    if (!World) return;

    OutNodes.Reset();

    for (TActorIterator<AFGResourceNode> It(World); It; ++It)
    {
        AFGResourceNode* Node = *It;
        if (!Node) continue;

        // Only extractable nodes
        if (!Node->CanPlaceResourceExtractor())
            continue;

        FResourceNodeInfo Info;
        Info.NodeActor = Node;
        Info.Location = Node->GetActorLocation();
        Info.ResourceName = FName(*Node->GetResourceName().ToString());
        Info.Purity = Node->GetResoucePurity();

        OutNodes.Add(Info);
    }

    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM_NodeScanner: Found %d extractable nodes"), OutNodes.Num());
}