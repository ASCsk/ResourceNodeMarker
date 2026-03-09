#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EngineUtils.h"
#include "FGResourceNode.h"
#include "URNM_WorldSubsystem.generated.h"

/**
 * World subsystem for Resource Node Marker (RNM)
 * Counts all resource nodes once the world has begun play
 */
UCLASS()
class RESOURCENODEMARKER_API URNM_WorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    /** Scans all resource nodes in the world after BeginPlay */
    void ScanResourceNodes();
};