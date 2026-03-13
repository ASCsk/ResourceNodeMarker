#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ResourceNodeMarker_ConfigStruct.h"
#include "RNM_ResourceNodeInfo.h"
#include "RNM_ResourceVisuals.h"

#include "RNM_WorldSubsystem.generated.h"

#define PLAYER_PROXIMITY 20000.0f // cm

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

private:
    TArray<FResourceNodeInfo> ResourceNodes;
    TSet<TWeakObjectPtr<AFGResourceNode>> ScannedNodes;
    FTimerHandle ProximityTimerHandle;
    float PlayerProximityThreshold = PLAYER_PROXIMITY;
    float PlayerProximityThresholdSq = PlayerProximityThreshold * PlayerProximityThreshold;

    UPROPERTY()
    URNM_ResourceVisuals* ResourceVisuals;

    FResourceNodeMarker_ConfigStruct ConfigData;
    bool bConfigLoaded = false;
};