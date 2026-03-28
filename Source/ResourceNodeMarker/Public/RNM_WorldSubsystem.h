#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ResourceNodeMarker_ConfigStruct.h"
#include "RNM_ResourceNodeInfo.h"
#include "RNM_ResourceVisuals.h"
#include "RNM_ClusterManager.h"
#include "Buildables/FGBuildableResourceExtractor.h"
#include "FGBuildableSubsystem.h"
#include "RNM_WorldSubsystem.generated.h"

UCLASS()
class RESOURCENODEMARKER_API URNM_WorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    void InitializeConfig();
    void ScanAllNodes();
    void CheckPlayerProximity();
    void BindBuildableDelegate();
    bool HasMigrationRun() const;
    void MarkMigrationComplete();

    UFUNCTION()
    void OnBuildableConstructed(AFGBuildable* Buildable);

private:
    TArray<FResourceNodeInfo> ResourceNodes;
    TMap<FIntVector, TArray<int32>> SpatialGrid;
    FTimerHandle ProximityTimerHandle;

    // 160m default in cm squared, overwritten by config on world begin play
    float PlayerProximityThresholdSq = (160.0f * 100.0f) * (160.0f * 100.0f);

    UPROPERTY()
    URNM_ResourceVisuals* ResourceVisuals = nullptr;

    UPROPERTY()
    URNM_ClusterManager* ClusterManager = nullptr;

    FResourceNodeMarker_ConfigStruct ConfigData;
    bool bConfigLoaded = false;
};