#include "RNM_WorldSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "ResourceNodeMarker.h" // logging category
#include "RNM_NodeScanner.h"
#include "RNM_MapMarkerService.h"
#include "ResourceNodeMarker_ConfigStruct.h"

void URNM_WorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UWorld* World = GetWorld();
    if (!World) return;

    // Only run in actual gameplay level
    /*if (World->GetName() != "Persistent_Level") return;*/
    if (World->WorldType != EWorldType::Game)

    UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: Subsystem running in gameplay world"));

    World->OnWorldBeginPlay.AddUObject(this, &URNM_WorldSubsystem::InitializeConfig);
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

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return;

    APawn* PlayerPawn = PC->GetPawn();
    if (!PlayerPawn) return;

    const FVector PlayerLocation = PlayerPawn->GetActorLocation();

    for (FResourceNodeInfo& NodeInfo : ResourceNodes)
    {
        if (!NodeInfo.NodeActor || ScannedNodes.Contains(NodeInfo.NodeActor))
            continue;

        if (bConfigLoaded)
        {
            const bool bPurityEnabled =
                (NodeInfo.Purity == RP_Pure && ConfigData.bMarkPure) ||
                (NodeInfo.Purity == RP_Normal && ConfigData.bMarkNormal) ||
                (NodeInfo.Purity == RP_Inpure && ConfigData.bMarkImpure);

            if (!bPurityEnabled)
            {
                UE_LOG(LogResourceNodeMarker, Warning,
                    TEXT("RNM: Skipping %s - purity %d filtered by config"),
                    *NodeInfo.ResourceName.ToString(), (int32)NodeInfo.Purity);
                continue;
            }
        }

        const float Distance = FVector::DistSquared(PlayerLocation, NodeInfo.Location);

        if (Distance <= PlayerProximityThresholdSq)
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

void URNM_WorldSubsystem::InitializeConfig()
{
    ConfigData = FResourceNodeMarker_ConfigStruct::GetActiveConfig(GetWorld());
    bConfigLoaded = true;

    UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: Config loaded"));
    UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: --- Purity Settings ---"));
    UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: Mark Pure:   %s"), ConfigData.bMarkPure ? TEXT("YES") : TEXT("NO"));
    UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: Mark Normal: %s"), ConfigData.bMarkNormal ? TEXT("YES") : TEXT("NO"));
    UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: Mark Impure: %s"), ConfigData.bMarkImpure ? TEXT("YES") : TEXT("NO"));
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