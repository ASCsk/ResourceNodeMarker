#pragma once
#include "CoreMinimal.h"
#include "FGResourceNode.h"
#include "RNM_ResourceNodeInfo.generated.h"

USTRUCT()
struct FResourceNodeInfo
{
    GENERATED_BODY()

    AFGResourceNode* NodeActor = nullptr;
    FVector Location;
    FName ResourceName;
    EResourcePurity Purity;
};

USTRUCT()
struct FResourceNodeCluster
{
    GENERATED_BODY()

    TArray<FResourceNodeInfo> Nodes;
    FVector AverageLocation = FVector::ZeroVector;
    FName ResourceName;
    EResourcePurity DominantPurity = RP_Normal;
    FGuid CurrentMarkerGUID;        // tracks the active marker so it can be deleted on update

    void RecalculateCenter();
    void RecalculateDominantPurity();
    void MergeWith(const FResourceNodeCluster& Other);
    float GetMarkerScale() const;
};