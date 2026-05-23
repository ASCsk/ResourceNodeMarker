#include "RNM_WorldSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "ResourceNodeMarker.h"
#include "RNM_NodeScanner.h"
#include "RNM_MapMarkerService.h"

void URNM_WorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UWorld* World = GetWorld();
    if (!World) return;

    if (World->WorldType != EWorldType::Game) return;

    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Subsystem running in gameplay world"));

    World->OnWorldBeginPlay.AddUObject(this, &URNM_WorldSubsystem::InitializeConfig);
    World->OnWorldBeginPlay.AddUObject(this, &URNM_WorldSubsystem::BindBuildableDelegate);

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
        ConfigData.ExtractorMarkerBehavior == 1 ? TEXT("Remove") : TEXT("Invalid"));
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: --- Icon Settings ---"));
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Use Icons: %s"), ConfigData.bUseIcons ? TEXT("YES") : TEXT("NO"));
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: --- Clustering ---"));
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Cluster Radius: %.0fm"), ConfigData.ClusterRadius);
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Cluster Height Tolerance: %.0fm"), ConfigData.ClusterHeightTolerance);
    UE_LOG(LogResourceNodeMarker, Log, TEXT("RNM: Cluster nodes: %s"),
        FResourceNodeMarker_ConfigStruct::IsClusteringEnabled(ConfigData)
            ? TEXT("YES (shared markers)") : TEXT("NO (one marker per node)"));

    ScanAllNodes();

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            ProximityTimerHandle,
            this,
            &URNM_WorldSubsystem::CheckPlayerProximity,
            0.25f,
            true
        );
    }
}

void URNM_WorldSubsystem::ScanAllNodes()
{
    UWorld* World = GetWorld();
    if (!World || !ClusterManager || !ResourceVisuals) return;

    RNM_NodeScanner::ScanNodes(World, ResourceNodes);
    const float GridCellSizeCm = FResourceNodeMarker_ConfigStruct::GetClusterRadiusCm(ConfigData);
    RNM_NodeScanner::BuildSpatialGrid(ResourceNodes, SpatialGrid, GridCellSizeCm);

    ClusterManager->Initialize(ResourceNodes, SpatialGrid, ResourceVisuals, ConfigData);
    ClusterManager->RebuildClustersFromExistingMarkers(World);
    ClusterManager->DeleteAllRNMMarkers(World);
    const int32 Created = ClusterManager->CreateAllClusterMarkers(World);

    UE_LOG(LogResourceNodeMarker, Log,
        TEXT("RNM: Scanned %d resource nodes, created %d cluster markers from discovery state"),
        ResourceNodes.Num(), Created);
}

void URNM_WorldSubsystem::CheckPlayerProximity()
{
    if (!bConfigLoaded || !ClusterManager) return;

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
        AFGResourceNode* Node = NodeInfo.NodeActor;
        if (!IsValid(Node)) continue;
        if (Node->IsOccupied()) continue;

        const bool bPurityEnabled =
            (NodeInfo.Purity == RP_Pure && ConfigData.bMarkPure) ||
            (NodeInfo.Purity == RP_Normal && ConfigData.bMarkNormal) ||
            (NodeInfo.Purity == RP_Inpure && ConfigData.bMarkImpure);

        if (!bPurityEnabled) continue;

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
    if (!Buildable || !bConfigLoaded || !ClusterManager) return;

    AFGBuildableResourceExtractor* Extractor = Cast<AFGBuildableResourceExtractor>(Buildable);
    if (!Extractor) return;

    if (ConfigData.ExtractorMarkerBehavior != 1) return;

    TScriptInterface<IFGExtractableResourceInterface> ExtractableResource = Extractor->GetExtractableResource();
    if (!ExtractableResource) return;

    UObject* ResourceObject = ExtractableResource.GetObject();
    if (!ResourceObject) return;

    AActor* ResourceActor = Cast<AActor>(ResourceObject);
    if (!ResourceActor) return;

    const FVector NodeLocation = ResourceActor->GetActorLocation();

    UWorld* World = GetWorld();
    if (!World) return;

    ClusterManager->OnExtractorPlaced(World, NodeLocation);
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