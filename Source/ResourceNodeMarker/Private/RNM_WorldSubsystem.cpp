#include "RNM_WorldSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ResourceNodeMarker.h"
#include "RNM_NodeScanner.h"
#include "RNM_MapMarkerService.h"

static FString GetMigrationFlagPath()
{
    return FPaths::ProjectSavedDir() / TEXT("ResourceNodeMarker/migration_v1.flag");
}

bool URNM_WorldSubsystem::HasMigrationRun() const
{
    return FPaths::FileExists(GetMigrationFlagPath());
}

void URNM_WorldSubsystem::MarkMigrationComplete()
{
    FFileHelper::SaveStringToFile(TEXT("migration_v1"), *GetMigrationFlagPath());
}

void URNM_WorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UWorld* World = GetWorld();
    if (!World) return;

    if (World->WorldType != EWorldType::Game) return;

    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Subsystem running in gameplay world"));

    World->OnWorldBeginPlay.AddUObject(this, &URNM_WorldSubsystem::InitializeConfig);
    World->OnWorldBeginPlay.AddUObject(this, &URNM_WorldSubsystem::BindBuildableDelegate);

    World->GetTimerManager().SetTimer(
        ProximityTimerHandle,
        this,
        &URNM_WorldSubsystem::CheckPlayerProximity,
        0.25f,
        true
    );

    ResourceVisuals = NewObject<URNM_ResourceVisuals>(this);
    ClusterManager = NewObject<URNM_ClusterManager>(this);
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
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: --- Icon Settings ---"));
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Use Icons: %s"), ConfigData.bUseIcons ? TEXT("YES") : TEXT("NO"));

    ScanAllNodes();
}

void URNM_WorldSubsystem::ScanAllNodes()
{
    UWorld* World = GetWorld();

    RNM_NodeScanner::ScanNodes(World, ResourceNodes);
    RNM_NodeScanner::BuildSpatialGrid(ResourceNodes, SpatialGrid);

    ClusterManager->Initialize(ResourceNodes, SpatialGrid, ResourceVisuals, ConfigData);

    if (ConfigData.bResetMigration)
    {
        UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Migration reset requested, deleting flag"));
        IFileManager::Get().Delete(*GetMigrationFlagPath());

        // Auto reset config back to false
        // We do this by marking the config dirty so it saves false back to disk
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (UConfigManager* ConfigManager = GI->GetSubsystem<UConfigManager>())
            {
                ConfigManager->MarkConfigurationDirty(FConfigId{ TEXT("ResourceNodeMarker"), TEXT("") });
            }
        }
    }

    if (!HasMigrationRun())
    {
        UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Running first time migration"));
        ClusterManager->MigrateOldMarkers(World);
        MarkMigrationComplete();
    }

    ClusterManager->RebuildClustersFromExistingMarkers(World);
    ClusterManager->DeleteAllRNMMarkers(World);
    ClusterManager->CreateAllClusterMarkers(World);

    UE_LOG(LogResourceNodeMarker, Log,
        TEXT("RNM: Scanned %d nodes, created %d cluster markers"),
        ResourceNodes.Num(), 0);
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

    for (int32 i = 0; i < ResourceNodes.Num(); i++)
    {
        if (ClusterManager->IsNodeDiscovered(i))
            continue;

        const FResourceNodeInfo& NodeInfo = ResourceNodes[i];
        if (!NodeInfo.NodeActor) continue;

        if (bConfigLoaded)
        {
            const bool bPurityEnabled =
                (NodeInfo.Purity == RP_Pure && ConfigData.bMarkPure) ||
                (NodeInfo.Purity == RP_Normal && ConfigData.bMarkNormal) ||
                (NodeInfo.Purity == RP_Inpure && ConfigData.bMarkImpure);

            if (!bPurityEnabled) continue;
        }

        if (FVector::DistSquared(PlayerLocation, NodeInfo.Location) <= PlayerProximityThresholdSq)
        {
            ClusterManager->OnNodeDiscovered(World, i);
        }
    }
}

void URNM_WorldSubsystem::BindBuildableDelegate()
{
    UWorld* World = GetWorld();
    if (!World) return;

    AFGBuildableSubsystem* BuildableSubsystem = AFGBuildableSubsystem::Get(World);
    if (!BuildableSubsystem)
    {
        UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: BuildableSubsystem not found, extractor detection disabled"));
        return;
    }

    BuildableSubsystem->mBuildableAddedDelegate.AddDynamic(
        this,
        &URNM_WorldSubsystem::OnBuildableConstructed
    );

    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Bound to mBuildableAddedDelegate"));
}

void URNM_WorldSubsystem::OnBuildableConstructed(AFGBuildable* Buildable)
{
    if (!Buildable) return;

    AFGBuildableResourceExtractor* Extractor = Cast<AFGBuildableResourceExtractor>(Buildable);
    if (!Extractor) return;

    if (ConfigData.ExtractorMarkerBehavior == 0) return;

    TScriptInterface<IFGExtractableResourceInterface> ExtractableResource = Extractor->GetExtractableResource();
    if (!ExtractableResource) return;

    UObject* ResourceObject = ExtractableResource.GetObject();
    if (!ResourceObject) return;

    AActor* ResourceActor = Cast<AActor>(ResourceObject);
    if (!ResourceActor) return;

    const FVector NodeLocation = ResourceActor->GetActorLocation();

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
            if (ConfigData.ExtractorMarkerBehavior == 1)
            {
                MapManager->RemoveMapMarker(Marker);
                UE_LOG(LogResourceNodeMarker, Log,
                    TEXT("RNM: Removed marker at extractor location %s"),
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

    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: World Subsystem Shutdown"));
}