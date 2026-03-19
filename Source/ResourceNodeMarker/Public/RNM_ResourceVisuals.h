#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RNM_ResourceVisuals.generated.h"

USTRUCT(BlueprintType)
struct FResourceVisual
{
    GENERATED_BODY()

    // Icon ID (integer from MapManager's icon list)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    int32 IconID = 656; // default ore icon

    // Colors for each purity level
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FLinearColor PureColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FLinearColor NormalColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    FLinearColor ImpureColor = FLinearColor::White;

};

// Icon presets
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

UCLASS(Blueprintable)
class RESOURCENODEMARKER_API URNM_ResourceVisuals : public UObject
{

    GENERATED_BODY()

private:
    TMap<FName, int32> IconMap;

public:
    URNM_ResourceVisuals();

    // Map resource name -> visual info
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
    TMap<FName, FResourceVisual> ResourceVisualMap;

    // Helper to get a visual by resource name
    UFUNCTION(BlueprintPure, Category = "Resources")
    FResourceVisual GetResourceVisual(const FName& ResourceName, bool bUseIcons) const;

    /**
     * Generates Pure/Normal/Impure shades from a single base color.
     * Pure   ? more saturated, darker (rich/vivid)
     * Normal ? base color
     * Impure ? less saturated, brighter (washed out)
     */
    static void GeneratePurityShades(
        const FLinearColor& BaseColor,
        FLinearColor& OutPure,
        FLinearColor& OutNormal,
        FLinearColor& OutImpure);
};