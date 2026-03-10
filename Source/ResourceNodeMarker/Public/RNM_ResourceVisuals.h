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
struct FIconPreset
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

UCLASS(Blueprintable)
class RESOURCENODEMARKER_API URNM_ResourceVisuals : public UObject
{
    GENERATED_BODY()
public:
    URNM_ResourceVisuals();

    // Map resource name -> visual info
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
    TMap<FString, FResourceVisual> ResourceVisualMap;

    // Helper to get a visual by resource name
    UFUNCTION(BlueprintPure, Category = "Resources")
    FResourceVisual GetResourceVisual(const FString& ResourceName) const;
};