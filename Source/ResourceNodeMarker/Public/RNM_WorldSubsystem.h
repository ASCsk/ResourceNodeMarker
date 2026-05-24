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

/**
 * Game-world orchestrator: config load, node scan, proximity discovery, extractor handling.
 * One instance per gameplay UWorld; owns scanned nodes, spatial grid, and cluster state.
 */
UCLASS()
class RESOURCENODEMARKER_API URNM_WorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    /** Registers OnWorldBeginPlay hooks and creates ResourceVisuals / ClusterManager. */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** Clears proximity timer and buildable delegate bindings. */
    virtual void Deinitialize() override;

private:
    /** Loads config, scans nodes, rebuilds markers, and starts the proximity timer. */
    void InitializeConfig();

    /** Rescans nodes, rebuilds cluster state, and recreates all RNM map markers. */
    void ScanAllNodes();

    /** Timer callback: discovers nodes within ProximityRadius of the local player. */
    void CheckPlayerProximity();

    /** Subscribes to AFGBuildableSubsystem::mBuildableAddedDelegate for extractor detection. */
    void BindBuildableDelegate();

    /** Handles extractor placement when ExtractorMarkerBehavior is Remove (1). */
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