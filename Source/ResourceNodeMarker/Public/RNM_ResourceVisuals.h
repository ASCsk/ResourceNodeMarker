#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/NoExportTypes.h"
#include "RNM_ResourceVisuals.generated.h"

class UFGResourceDescriptor;

USTRUCT(BlueprintType)
struct FResourceVisual
{
    GENERATED_BODY()

    /** Map manager icon id (stamp or in-game resource icon depending on config). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    int32 IconID = 656;

    /** Marker tint when the cluster's dominant purity is Pure. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FLinearColor PureColor = FLinearColor::White;

    /** Marker tint when the cluster's dominant purity is Normal. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FLinearColor NormalColor = FLinearColor::White;

    /** Marker tint when the cluster's dominant purity is Impure. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FLinearColor ImpureColor = FLinearColor::White;
};

/** Default AFGMapManager stamp icon ids (rock, fluid, etc.). */
struct FStampPreset
{
    int Rock = 660;
    int Fluids = 657;
    int Radiation = 659;
    int Fruit = 662;
    int Slug = 663;
    int Crash = 652;
    int Creature = 654;
    int QuestionMark = 656;
};

/** Per-resource in-game map icon ids when bUseIcons is enabled. */
struct FIconsPreset
{
    int Copper = 198;
    int Iron = 193;
    int Limestone = 204;
    int Caterium = 200;
    int Coal = 205;
    int Sulfur = 203;
    int Bauxite = 199;
    int Quartz = 206;
    int Uranium = 201;
    int Sam = 280;
};

/** Lookup table for resource icon ids and purity tint colors. */
UCLASS(Blueprintable)
class RESOURCENODEMARKER_API URNM_ResourceVisuals : public UObject
{

    GENERATED_BODY()

private:
    TMap<FName, int32> IconMap;

public:
    URNM_ResourceVisuals();

    /**
     * Editor/overridable map of resource keys to icon and purity colors.
     * Keys include UClass FNames (Desc_OreIron_C) and legacy English display names.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
    TMap<FName, FResourceVisual> ResourceVisualMap;

    /**
     * Resolves icon and purity colors for a resource.
     * Prefer GetResourceVisual(..., ResClass, ...) when a descriptor class is available.
     * @param ResourceName - UClass FName or legacy display key.
     * @param bUseIcons - When false, uses stock rock/fluid stamps; when true, per-resource icons.
     * @return Visual data applied to new map markers.
     */
    UFUNCTION(BlueprintPure, Category = "Resources")
    FResourceVisual GetResourceVisual(const FName& ResourceName, bool bUseIcons) const;

    /**
     * Resolves icon and purity colors for a resource.
     * Lookup order: UClass FName, class-name pattern, then localized display names.
     * @param ResourceClassFName - Stable descriptor UClass FName (e.g. Desc_OreIron_C).
     * @param ResClass - Optional descriptor class for form detection and extra keys.
     * @param bUseIcons - When false, uses stock rock/fluid stamps; when true, per-resource icons.
     * @param OptionalLegacyDisplayKey - Localized GetResourceName() for old save compatibility.
     * @return Visual data applied to new map markers.
     */
    FResourceVisual GetResourceVisual(
        const FName& ResourceClassFName,
        TSubclassOf<UFGResourceDescriptor> ResClass,
        bool bUseIcons,
        FName OptionalLegacyDisplayKey = NAME_None) const;

    /**
     * Derives Pure, Normal, and Impure tints from a single base color.
     * Pure is more saturated and darker; Impure is desaturated and brighter.
     * @param BaseColor - Normal-purity reference color.
     * @param OutPure - Output tint for pure nodes.
     * @param OutNormal - Output tint for normal nodes (same as BaseColor).
     * @param OutImpure - Output tint for impure nodes.
     */
    static void GeneratePurityShades(
        const FLinearColor& BaseColor,
        FLinearColor& OutPure,
        FLinearColor& OutNormal,
        FLinearColor& OutImpure);
};