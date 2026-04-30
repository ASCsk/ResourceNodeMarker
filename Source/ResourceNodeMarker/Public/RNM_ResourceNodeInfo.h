#pragma once
#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "FGResourceNode.h"
#include "FGResourceDescriptor.h"
#include "RNM_ResourceNodeInfo.generated.h"

USTRUCT()
struct FResourceNodeInfo
{
    GENERATED_BODY()

    UPROPERTY()
    AFGResourceNode* NodeActor = nullptr;

    FVector Location;
    /** UClass FName of UFGResourceDescriptor — language-invariant, safe for save/rebuild and visuals. */
    FName ResourceName;
    /** Same as GetResourceClass() at scan time; kept so markers still resolve visuals if NodeActor is gone. */
    UPROPERTY()
    TSubclassOf<UFGResourceDescriptor> ResourceDescriptorClass = nullptr;
    EResourcePurity Purity;
};

USTRUCT()
struct FResourceNodeCluster
{
    GENERATED_BODY()

    TArray<FResourceNodeInfo> Nodes;
    FVector AverageLocation = FVector::ZeroVector;
    /** UFGResourceDescriptor UClass FName, same as FResourceNodeInfo::ResourceName. */
    FName ResourceName;
    EResourcePurity DominantPurity = RP_Normal;
    FGuid CurrentMarkerGUID; // track the active marker so it can be deleted on update

    void RecalculateCenter();
    void RecalculateDominantPurity();
    void MergeWith(const FResourceNodeCluster& Other);
    float GetMarkerScale() const;
};