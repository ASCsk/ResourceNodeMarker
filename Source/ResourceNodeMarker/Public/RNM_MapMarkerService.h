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

    static constexpr float MARKER_LOCATION_TOLERANCE_SQ = 100.0f * 100.0f;

private:
    static FString BuildClusterMarkerName(const FResourceNodeCluster& Cluster);
    static FString GetPurityString(EResourcePurity Purity);
};