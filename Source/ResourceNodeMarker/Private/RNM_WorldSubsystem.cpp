#include "RNM_WorldSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "ResourceNodeMarker.h"
#include "RNM_NodeScanner.h"
#include "RNM_MapMarkerService.h"
#include "ResourceNodeMarker_ConfigStruct.h"

void URNM_WorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UWorld* World = GetWorld();

    if (!World) 
        return;

    if (World->WorldType != EWorldType::Game)
        return;

    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Subsystem running in gameplay world"));

    World->OnWorldBeginPlay.AddUObject(this, &URNM_WorldSubsystem::InitializeConfig);
    World->OnWorldBeginPlay.AddUObject(this, &URNM_WorldSubsystem::ScanAllNodes);
    World->OnWorldBeginPlay.AddUObject(this, &URNM_WorldSubsystem::BindBuildableDelegate);

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
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Cached %d resource nodes"), ResourceNodes.Num());
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
        if (!IsValid(NodeInfo.NodeActor) || ScannedNodes.Contains(NodeInfo.NodeActor))
            continue;

        if (bConfigLoaded)
        {
            const bool bPurityEnabled =
                (NodeInfo.Purity == RP_Pure && ConfigData.bMarkPure) ||
                (NodeInfo.Purity == RP_Normal && ConfigData.bMarkNormal) ||
                (NodeInfo.Purity == RP_Inpure && ConfigData.bMarkImpure);

            if (!bPurityEnabled)
            {
                UE_LOG(LogResourceNodeMarker, Log,
                    TEXT("RNM: Skipping %s - purity %d filtered by config"),
                    *NodeInfo.ResourceName.ToString(), (int32)NodeInfo.Purity);
                continue;
            }
        }

        const float Distance = FVector::DistSquared(PlayerLocation, NodeInfo.Location);

        if (Distance <= PlayerProximityThresholdSq)
        {
            ScannedNodes.Add(NodeInfo.NodeActor);

            bool bCreated = RNM_MapMarkerService::CreateMarker(World, NodeInfo, ResourceVisuals, ConfigData);
            if (bCreated)
            {
                UE_LOG(LogResourceNodeMarker, Log,
                    TEXT("RNM: Player near resource node, marker created for %s"), *NodeInfo.ResourceName.ToString());
            }
        }
    }
}

void URNM_WorldSubsystem::InitializeConfig()
{
    ConfigData = FResourceNodeMarker_ConfigStruct::GetActiveConfig(GetWorld());
    bConfigLoaded = true;

    PlayerProximityThresholdSq = (ConfigData.ProximityRadius * 100.0f) * (ConfigData.ProximityRadius * 100.0f);

    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Config loaded"));
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: --- Purity Settings ---"));
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Mark Pure:   %s"), ConfigData.bMarkPure ? TEXT("YES") : TEXT("NO"));
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Mark Normal: %s"), ConfigData.bMarkNormal ? TEXT("YES") : TEXT("NO"));
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Mark Impure: %s"), ConfigData.bMarkImpure ? TEXT("YES") : TEXT("NO"));
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: --- Compass Settings ---"));
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Compass View Distance: %d (%s)"),
        ConfigData.CompassViewDistance,
        ConfigData.CompassViewDistance == 0 ? TEXT("Off") :
        ConfigData.CompassViewDistance == 1 ? TEXT("Near") :
        ConfigData.CompassViewDistance == 2 ? TEXT("Mid") :
        ConfigData.CompassViewDistance == 3 ? TEXT("Far") :
        ConfigData.CompassViewDistance == 4 ? TEXT("Always") : TEXT("Invalid"));
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: --- Proximity Settings ---"));
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Proximity Radius: %.0fm (%.0fcm)"),
        ConfigData.ProximityRadius,
        ConfigData.ProximityRadius * 100.0f);
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: --- Extractor Settings ---"));
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Extractor Marker Behavior: %d (%s)"),
        ConfigData.ExtractorMarkerBehavior,
        ConfigData.ExtractorMarkerBehavior == 0 ? TEXT("Keep") :
        ConfigData.ExtractorMarkerBehavior == 1 ? TEXT("Remove") :
        ConfigData.ExtractorMarkerBehavior == 2 ? TEXT("Highlight") : TEXT("Invalid"));
}

void URNM_WorldSubsystem::BindBuildableDelegate()
{
    UWorld* World = GetWorld();
    if (!World) return;

    AFGBuildableSubsystem* BuildableSubsystem = AFGBuildableSubsystem::Get(World);
    if (!BuildableSubsystem)
    {
        UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: BuildableSubsystem not found, miner placement detection disabled"));
        return;
    }

    BuildableSubsystem->mBuildableAddedDelegate.AddDynamic(
        this,
        &URNM_WorldSubsystem::OnBuildableConstructed
    );

    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Bound to BuildableConstructedGlobalDelegate"));
}

void URNM_WorldSubsystem::OnBuildableConstructed(AFGBuildable* Buildable)
{
    if (!Buildable)
    {
        UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: No Buildable"));
        return;
    }

    // Only care about resource extractors
    AFGBuildableResourceExtractor* Extractor = Cast<AFGBuildableResourceExtractor>(Buildable);
    if (!Extractor)
    {
        UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: Extractor not found"));
        return;
    }

    // 0 = Keep, nothing to do
    if (ConfigData.ExtractorMarkerBehavior == 0)
    {
        UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: Config 0 = Keep, nothing to do"));
        return;
    }

    TScriptInterface<IFGExtractableResourceInterface> ExtractableResource = Extractor->GetExtractableResource();
    if (!ExtractableResource)
    {
        UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: ExtractableResource not found"));
        return;
    }
    

    UObject* ResourceObject = ExtractableResource.GetObject();
    if (!ResourceObject)
    {
        UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: ResourceObject not found"));
        return;
    }

    AActor* ResourceActor = Cast<AActor>(ResourceObject);
    if (!ResourceActor)
    {
        UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: ResourceActor not found"));
        return;
    }

    const FVector NodeLocation = ResourceActor->GetActorLocation();
    UE_LOG(LogResourceNodeMarker, Log,
        TEXT("RNM: Extractor placed at resource location X=%.0f Y=%.0f Z=%.0f"),
        NodeLocation.X, NodeLocation.Y, NodeLocation.Z);

    UWorld* World = GetWorld();
    if (!World) return;

    AFGMapManager* MapManager = AFGMapManager::Get(World);
    if (!MapManager) return;

    TArray<FMapMarker> AllMarkers;
    MapManager->GetMapMarkers(AllMarkers);

    for (FMapMarker& Marker : AllMarkers)
    {
        if (FVector::DistSquared(Marker.Location, NodeLocation) <= RNM_MapMarkerService::MARKER_LOCATION_TOLERANCE_SQ)
        {
            // 1 = Remove
            if (ConfigData.ExtractorMarkerBehavior == 1)
            {
                MapManager->RemoveMapMarker(Marker);
                UE_LOG(LogResourceNodeMarker, Log,
                    TEXT("RNM: Removed marker at extractor placement location %s"),
                    *NodeLocation.ToString());
            }
            return;
        }
    }
}

void URNM_WorldSubsystem::Deinitialize()
{
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(ProximityTimerHandle);

        AFGBuildableSubsystem* BuildableSubsystem = AFGBuildableSubsystem::Get(World);
        if (BuildableSubsystem)
        {
            BuildableSubsystem->mBuildableAddedDelegate.RemoveDynamic(
                this,
                &URNM_WorldSubsystem::OnBuildableConstructed
            );
        }
    }

    Super::Deinitialize();

    UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM World Subsystem Shutdown"));
}