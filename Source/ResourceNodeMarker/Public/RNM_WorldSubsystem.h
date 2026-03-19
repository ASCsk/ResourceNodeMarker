#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ResourceNodeMarker_ConfigStruct.h"
#include "RNM_ResourceNodeInfo.h"
#include "RNM_ResourceVisuals.h"
#include "Buildables/FGBuildableResourceExtractor.h"
#include "FGBuildableSubsystem.h"

#include "RNM_WorldSubsystem.generated.h"

#define PLAYER_PROXIMITY 160.0f // m

UCLASS()
class RESOURCENODEMARKER_API URNM_WorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    void CheckPlayerProximity();
    void ScanAllNodes();
    void InitializeConfig();
    void BindBuildableDelegate();

    UFUNCTION()
    void OnBuildableConstructed(AFGBuildable* Buildable);

private:
    TArray<FResourceNodeInfo> ResourceNodes;
    TSet<TWeakObjectPtr<AFGResourceNode>> ScannedNodes;
    FTimerHandle ProximityTimerHandle;
    float PlayerProximityThresholdSq = (PLAYER_PROXIMITY * 100.0f) * (PLAYER_PROXIMITY * 100.0f); // 160m default, stored as cm squared

    UPROPERTY()
    URNM_ResourceVisuals* ResourceVisuals;

    FResourceNodeMarker_ConfigStruct ConfigData;
    bool bConfigLoaded = false;
};