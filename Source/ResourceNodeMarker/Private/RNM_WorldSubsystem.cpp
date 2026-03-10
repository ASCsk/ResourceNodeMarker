#include "RNM_WorldSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "ResourceNodeMarker.h" // logging category
#include "RNM_NodeScanner.h"
#include "RNM_MapMarkerService.h"

void URNM_WorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UWorld* World = GetWorld();
    if (!World) return;

    // Only run in actual gameplay level
    if (World->GetName() != "Persistent_Level") return;

    UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: Subsystem running in gameplay world"));

    // Scan all nodes once the world begins play
    World->OnWorldBeginPlay.AddUObject(this, &URNM_WorldSubsystem::ScanAllNodes);

    // Start proximity check timer
    World->GetTimerManager().SetTimer(
        ProximityTimerHandle,
        this,
        &URNM_WorldSubsystem::CheckPlayerProximity,
        0.25f, // every 250 ms
        true
    );

    // Create visuals object
    ResourceVisuals = NewObject<URNM_ResourceVisuals>(this);
}

void URNM_WorldSubsystem::ScanAllNodes()
{
    RNM_NodeScanner::ScanNodes(GetWorld(), ResourceNodes);
    UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: Cached %d resource nodes"), ResourceNodes.Num());
}

void URNM_WorldSubsystem::CheckPlayerProximity()
{
    UWorld* World = GetWorld();
    if (!World) return;

    APawn* PlayerPawn = World->GetFirstPlayerController()->GetPawn();
    if (!PlayerPawn) return;

    const FVector PlayerLocation = PlayerPawn->GetActorLocation();

    for (FResourceNodeInfo& NodeInfo : ResourceNodes)
    {
        if (!NodeInfo.NodeActor || ScannedNodes.Contains(NodeInfo.NodeActor))
            continue;

        const float Distance = FVector::Dist(PlayerLocation, NodeInfo.Location);

        if (Distance <= PlayerProximityThreshold)
        {
            ScannedNodes.Add(NodeInfo.NodeActor);

            bool bCreated = RNM_MapMarkerService::CreateMarker(World, NodeInfo, ResourceVisuals);
            if (bCreated)
            {
                UE_LOG(LogResourceNodeMarker, Warning,
                    TEXT("RNM: Player near resource node, marker created for %s"), *NodeInfo.ResourceName.ToString());
            }
        }
    }
}

void URNM_WorldSubsystem::Deinitialize()
{
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(ProximityTimerHandle);
    }

    Super::Deinitialize();

    UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM World Subsystem Shutdown"));
}