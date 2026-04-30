#pragma once
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "FGMapManager.h"
#include "FGMapMarker.h"
#include "RNM_ResourceNodeInfo.h"
#include "RNM_ResourceVisuals.h"
#include "ResourceNodeMarker_ConfigStruct.h"

class RNM_MapMarkerService
{
public:
    RNM_MapMarkerService() = default;
    ~RNM_MapMarkerService() = default;

    /**
     * Creates or updates a cluster marker.
     * If the cluster has an existing marker (CurrentMarkerGUID is valid), it will be deleted first.
     * @param World - The game world.
     * @param Cluster - The cluster to create a marker for.
     * @param ResourceVisuals - Visual lookup helper.
     * @param Config - Active mod configuration.
     * @param OutGUID - The GUID of the created marker.
     * @return true if marker creation succeeded.
     */
    static bool CreateOrUpdateClusterMarker(
        UWorld* World,
        FResourceNodeCluster& Cluster,
        URNM_ResourceVisuals* ResourceVisuals,
        const FResourceNodeMarker_ConfigStruct& Config,
        FGuid& OutGUID);

    static ECompassViewDistance ParseCompassViewDistance(int32 Value);

    /** RNM markers (legacy RNM::Ore / RNM::Fluid or v2 RNM::Ore#ClassFName). */
    static bool IsRNMMapMarkerCategory(const FString& Category);

    /** Legacy: category held RNM::Ore#ClassFName. Still parsed for old saves. */
    static bool TryParseClassIdFromCategory(const FString& Category, FName& OutClassFName);

    /** Current: stable id suffix on marker Name: " ... #RNM:Desc_OreIron_C" */
    static bool TryParseClassIdFromMarkerName(const FString& MarkerName, FName& OutClassFName);

    /** Short UI category only: RNM::Ore or RNM::Fluid */
    static FString BuildCategoryName(bool bIsFluid);

    static constexpr float MARKER_LOCATION_TOLERANCE_SQ = 100.0f * 100.0f;

private:
    static FString BuildClusterMarkerName(const FResourceNodeCluster& Cluster);
};