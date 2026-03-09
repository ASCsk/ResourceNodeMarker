#include "URNM_WorldSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FGResourceNode.h"

void URNM_WorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UWorld* World = GetWorld();
    if (!World) return;

    // Only run in the actual gameplay level
    if (World->GetName() != "Persistent_Level") return;

    UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: Subsystem running in gameplay world"));

    // Bind a call to ScanResourceNodes when world begins play
    World->OnWorldBeginPlay.AddUObject(this, &URNM_WorldSubsystem::ScanResourceNodes);
}

void URNM_WorldSubsystem::ScanResourceNodes()
{
    UWorld* World = GetWorld();
    if (!World) return;

    int32 ResourceNodeCount = 0;

    for (TActorIterator<AFGResourceNode> It(World); It; ++It)
    {
        ResourceNodeCount++;
    }

    UE_LOG(LogResourceNodeMarker, Warning,
        TEXT("RNM: Resource node count (BeginPlay): %d"),
        ResourceNodeCount);
}

void URNM_WorldSubsystem::Deinitialize()
{
    Super::Deinitialize();
    UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM World Subsystem Shutdown"));
}