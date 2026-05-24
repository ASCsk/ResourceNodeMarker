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

    /** Live world actor; may be null after reload if not rescanned. */
    UPROPERTY()
    AFGResourceNode* NodeActor = nullptr;

    /** World location at scan time (cm). */
    FVector Location;

    /** UClass FName of UFGResourceDescriptor — language-invariant, safe for save/rebuild and visuals. */
    FName ResourceName;

    /** Cached GetResourceClass() at scan time; used when NodeActor is unavailable. */
    UPROPERTY()
    TSubclassOf<UFGResourceDescriptor> ResourceDescriptorClass = nullptr;

    /** Node purity at scan time. */
    EResourcePurity Purity;
};

USTRUCT()
struct FResourceNodeCluster
{
    GENERATED_BODY()

    /** Nodes grouped under one map marker. */
    TArray<FResourceNodeInfo> Nodes;

    /** Centroid of Nodes; used as marker location. */
    FVector AverageLocation = FVector::ZeroVector;

    /** UFGResourceDescriptor UClass FName, same as FResourceNodeInfo::ResourceName. */
    FName ResourceName;

    /** Purity with the highest node count; drives marker color. */
    EResourcePurity DominantPurity = RP_Normal;

    /** Active map marker guid; invalidated when the marker is removed. */
    FGuid CurrentMarkerGUID;

    /** Recomputes AverageLocation from Nodes. */
    void RecalculateCenter();

    /** Recomputes DominantPurity from node purity counts. */
    void RecalculateDominantPurity();

    /** Appends Other.Nodes and recalculates center and dominant purity. */
    void MergeWith(const FResourceNodeCluster& Other);

    /**
     * Map marker scale based on node count in the cluster.
     * @return Scale factor passed to FMapMarker::Scale.
     */
    float GetMarkerScale() const;
};