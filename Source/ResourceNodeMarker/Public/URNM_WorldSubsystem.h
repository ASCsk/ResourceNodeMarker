#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FGResourceNode.h"
#include "TimerManager.h"

#include "URNM_WorldSubsystem.generated.h"

#define PLAYER_PROXIMITY 2000.0f // cm

USTRUCT()
struct FNodeInfo
{
    GENERATED_BODY()

    FVector Location;
    FString ResourceName;
    EResourcePurity Purity;
    AFGResourceNode* NodeActor;
};

UCLASS()
class RESOURCENODEMARKER_API URNM_WorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    void ScanResourceNodes();
    void CheckPlayerProximity();
    void SpawnMapMarker(const FNodeInfo& NodeInfo);
    FString GetPurityString(EResourcePurity Purity) const;

private:
    TArray<FNodeInfo> ResourceNodes;
    TSet<AFGResourceNode*> ScannedNodes;
    FTimerHandle ProximityTimerHandle;
    float PlayerProximityThreshold = PLAYER_PROXIMITY; 
};