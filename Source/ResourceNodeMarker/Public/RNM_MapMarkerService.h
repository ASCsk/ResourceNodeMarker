#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "FGMapManager.h"
#include "FGMapMarker.h"
#include "RNM_ResourceNodeInfo.h"
#include "RNM_ResourceVisuals.h"
#include "ResourceNodeMarker_ConfigStruct.h"

/**
 * Helper class to create map markers for resource nodes
 */
class RNM_MapMarkerService
{
public:
    RNM_MapMarkerService() = default;
    ~RNM_MapMarkerService() = default;

    /**
     * Create a map marker for the given node.
     * @param World - The game world.
     * @param NodeInfo - Resource node data.
     * @param ResourceVisuals - Visual lookup helper.
     * @return true if marker creation succeeded.
     */
    static bool CreateMarker(UWorld* World, const FResourceNodeInfo& NodeInfo, URNM_ResourceVisuals* ResourceVisuals, const FResourceNodeMarker_ConfigStruct& Config);
    static ECompassViewDistance ParseCompassViewDistance(int32 Value);

private:
    // Helper to convert purity enum to string
    static FString GetPurityString(EResourcePurity Purity);
};