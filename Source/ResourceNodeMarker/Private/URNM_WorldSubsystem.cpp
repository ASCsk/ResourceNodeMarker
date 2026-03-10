#include "URNM_WorldSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FGResourceNode.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "ResourceNodeMarker.h" // for logging category
#include "FGMapManager.h"
#include "FGMapMarker.h"
#include "FGPlayerController.h"
#include "FGPlayerState.h"
#include "Kismet/GameplayStatics.h" //for player controller


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

        // Filter: extractable nodes only
        if (!Node->CanPlaceResourceExtractor())
        {
            continue; // skip deposits, geysers, etc
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

            FString PurityString = GetPurityString(NodeInfo.Purity);

            UE_LOG(LogResourceNodeMarker, Warning,
                TEXT("Player near resource node: %s | Resource: %s | Purity: %s | Distance: %.2f m | Location: X=%.0f Y=%.0f Z=%.0f"),
                *NodeInfo.NodeActor->GetName(),
                *NodeInfo.ResourceName,
                *PurityString,
                Distance / 100.0f,
                NodeInfo.Location.X,
                NodeInfo.Location.Y,
                NodeInfo.Location.Z
            );

            SpawnMapMarker(NodeInfo);
        }
    }
}

void URNM_WorldSubsystem::SpawnMapMarker(const FNodeInfo& NodeInfo)
{
    UWorld* World = GetWorld();
    if (!World) return;


    AFGMapManager* MapManager = AFGMapManager::Get(World);
    if (!MapManager)
    {
        UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: MapManager not found"));
        return;
    }

    if (!MapManager->CanAddNewMapMarker())
    {
        UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: Map marker limit reached"));
        return;
    }

    //TArray<FMapMarker> AllMarkers;
    //MapManager->GetMapMarkers(AllMarkers);

    //for (const FMapMarker& Marker : AllMarkers)
    //{
    //    UE_LOG(LogTemp, Warning,
    //        TEXT(
    //            "Marker: %s | GUID: %s | Type: %d | IconID: %d | Color: R=%.2f G=%.2f B=%.2f A=%.2f | Scale: %.2f | CompassView: %d | Category: %s | PlacedByAccountID: %s | CreatedByPlayerID: %s"
    //        ),
    //        *Marker.Name,
    //        *Marker.MarkerGUID.ToString(),
    //        (int)Marker.MapMarkerType,
    //        Marker.IconID,
    //        Marker.Color.R,
    //        Marker.Color.G,
    //        Marker.Color.B,
    //        Marker.Color.A,
    //        Marker.Scale,
    //        (int)Marker.CompassViewDistance,
    //        *Marker.CategoryName,
    //        *Marker.MarkerPlacedByAccountID,
    //        *Marker.CreatedByPlayerID.ToString()
    //    );
    //}

    FMapMarker Marker;

    Marker.MarkerGUID = FGuid::NewGuid();
    Marker.Location = NodeInfo.Location;

    FString PurityString = GetPurityString(NodeInfo.Purity);

    Marker.Name = FString::Printf(
        TEXT("%s (%s)"),
        *NodeInfo.ResourceName,
        *PurityString
    );
    Marker.MapMarkerType = static_cast<ERepresentationType>(16);
    Marker.IconID = 660;
    Marker.CategoryName = TEXT("");
    Marker.Scale = 1.0f;
    Marker.Color = FLinearColor::White;
    Marker.CompassViewDistance = ECompassViewDistance::CVD_Always;
    Marker.CreatedByPlayerID = MapManager->GetLocalPlayerID();
    Marker.MarkerPlacedByAccountID = MapManager->GetLocalPlayerID().ToString();

    switch (NodeInfo.Purity)
    {
        case RP_Inpure: Marker.Color = FLinearColor::Red; break;
        case RP_Normal: Marker.Color = FLinearColor::Yellow; break;
        case RP_Pure: Marker.Color = FLinearColor::Green; break;
        default: Marker.Color = FLinearColor::White; break;
    }

    FMapMarker CreatedMarker;

    bool bSuccess = MapManager->AddNewMapMarker(Marker, CreatedMarker);

    if (bSuccess)
    {
        UE_LOG(LogResourceNodeMarker, Warning, TEXT("SpawnMapMarker: MarkerCreatedByPlayerID (GUID) = %s"), *Marker.CreatedByPlayerID.ToString());
        UE_LOG(LogResourceNodeMarker, Warning,
            TEXT("RNM: Map marker created for %s"),
            *NodeInfo.ResourceName);
    }
    else
    {
        UE_LOG(LogResourceNodeMarker, Warning, TEXT("SpawnMapMarker: MarkerCreatedByPlayerID (GUID) = %s"), *Marker.CreatedByPlayerID.ToString());
        UE_LOG(LogResourceNodeMarker, Warning,
            TEXT("RNM: Failed to create marker for %s"),
            *NodeInfo.ResourceName);
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

FString URNM_WorldSubsystem::GetPurityString(EResourcePurity Purity) const
{
    switch (Purity)
    {
    case RP_Inpure:
        return TEXT("Impure");

    case RP_Normal:
        return TEXT("Normal");

    case RP_Pure:
        return TEXT("Pure");

    default:
        return TEXT("Unknown");
    }
}