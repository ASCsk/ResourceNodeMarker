#include "RNM_ResourceNodeInfo.h"

void FResourceNodeCluster::RecalculateCenter()
{
    if (Nodes.Num() == 0) return;

    FVector Sum = FVector::ZeroVector;
    for (const FResourceNodeInfo& Node : Nodes)
        Sum += Node.Location;

    AverageLocation = Sum / Nodes.Num();
}

void FResourceNodeCluster::RecalculateDominantPurity()
{
    int32 PureCount = 0;
    int32 NormalCount = 0;
    int32 ImpureCount = 0;

    for (const FResourceNodeInfo& Node : Nodes)
    {
        switch (Node.Purity)
        {
        case RP_Pure:   PureCount++;   break;
        case RP_Normal: NormalCount++; break;
        case RP_Inpure: ImpureCount++; break;
        }
    }

    if (PureCount >= NormalCount && PureCount >= ImpureCount)
        DominantPurity = RP_Pure;
    else if (NormalCount >= ImpureCount)
        DominantPurity = RP_Normal;
    else
        DominantPurity = RP_Inpure;
}

void FResourceNodeCluster::MergeWith(const FResourceNodeCluster& Other)
{
    for (const FResourceNodeInfo& Node : Other.Nodes)
        Nodes.Add(Node);

    RecalculateCenter();
    RecalculateDominantPurity();
}

float FResourceNodeCluster::GetMarkerScale() const
{
    const int32 NodeCount = Nodes.Num();

    if (NodeCount <= 1) return 1.0f;
    if (NodeCount <= 3) return 1.5f;
    if (NodeCount <= 6) return 2.0f;
    return 2.5f;
}