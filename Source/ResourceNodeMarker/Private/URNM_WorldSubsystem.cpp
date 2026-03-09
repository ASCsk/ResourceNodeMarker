#include "URNM_WorldSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FGResourceNode.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "ResourceNodeMarker.h" // for logging category

void URNM_WorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UWorld* World = GetWorld();
    if (!World) return;

    // Only run in actual gameplay level
    if (World->GetName() != "Persistent_Level") return;

    UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: Subsystem running in gameplay world"));

    // Scan nodes after world begins play
    World->OnWorldBeginPlay.AddUObject(this, &URNM_WorldSubsystem::ScanResourceNodes);

    // Start proximity check timer
    World->GetTimerManager().SetTimer(
        ProximityTimerHandle,
        this,
        &URNM_WorldSubsystem::CheckPlayerProximity,
        0.25f, // every 250 ms
        true
    );
}

void URNM_WorldSubsystem::ScanResourceNodes()
{
    UWorld* World = GetWorld();
    if (!World) return;

    ResourceNodes.Empty();

    int32 ResourceNodeCount = 0;

    for (TActorIterator<AFGResourceNode> It(World); It; ++It)
    {
        AFGResourceNode* Node = *It;

        if (!Node) continue;

        // Filter: miner nodes only
        if (Node->GetResourceNodeType() != EResourceNodeType::Node)
        {
            continue;
        }

        FNodeInfo Info;

        Info.NodeActor = Node;
        Info.Location = Node->GetActorLocation();
        Info.ResourceName = Node->GetResourceName().ToString();
        Info.Purity = Node->GetResoucePurity();

        ResourceNodes.Add(Info);
        ResourceNodeCount++;
    }

    UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: Cached %d miner nodes"), ResourceNodeCount);
}


void URNM_WorldSubsystem::CheckPlayerProximity()
{
    UWorld* World = GetWorld();
    if (!World) return;

    APawn* PlayerPawn = World->GetFirstPlayerController()->GetPawn();
    if (!PlayerPawn) return;

    const FVector PlayerLocation = PlayerPawn->GetActorLocation();

    for (FNodeInfo& NodeInfo : ResourceNodes)
    {
        if (!NodeInfo.NodeActor || ScannedNodes.Contains(NodeInfo.NodeActor))
            continue;

        const float Distance = FVector::Dist(PlayerLocation, NodeInfo.Location);

        if (Distance <= PlayerProximityThreshold)
        {
            ScannedNodes.Add(NodeInfo.NodeActor);

            FString PurityString;

            switch (NodeInfo.Purity)
            {
            case RP_Inpure:
                PurityString = TEXT("Impure");
                break;

            case RP_Normal:
                PurityString = TEXT("Normal");
                break;

            case RP_Pure:
                PurityString = TEXT("Pure");
                break;

            default:
                PurityString = TEXT("Unknown");
                break;
            }

            UE_LOG(LogResourceNodeMarker, Warning,
                TEXT("Player near resource node: %s | Resource: %s | Purity: %s | Distance: %.2f m"),
                *NodeInfo.NodeActor->GetName(),
                *NodeInfo.ResourceName,
                *PurityString,
                Distance / 100.0f
            );
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