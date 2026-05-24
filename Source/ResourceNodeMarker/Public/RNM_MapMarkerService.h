#pragma once
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "FGMapManager.h"
#include "FGMapMarker.h"
#include "RNM_ResourceNodeInfo.h"
#include "RNM_ResourceVisuals.h"
#include "ResourceNodeMarker_ConfigStruct.h"

/** Stateless helpers for creating, naming, and identifying RNM map markers. */
class RNM_MapMarkerService
{
public:
    RNM_MapMarkerService() = default;
    ~RNM_MapMarkerService() = default;

    /**
     * Creates or updates a cluster marker.
     * When updating, adds the new marker first and removes the previous one only on success.
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

    /**
     * Maps config integer (0–4) to Satisfactory compass view distance.
     * @param Value - Config CompassViewDistance; invalid values log a warning and default to Mid.
     * @return Matching ECompassViewDistance for the map marker.
     */
    static ECompassViewDistance ParseCompassViewDistance(int32 Value);

    /**
     * Returns true if Category is an RNM-owned marker category.
     * Accepts legacy RNM::Ore / RNM::Fluid and stable RNM::Ore#ClassFName forms.
     * @param Category - FMapMarker::CategoryName from the map manager.
     */
    static bool IsRNMMapMarkerCategory(const FString& Category);

    /**
     * Parses a stable resource class id embedded in CategoryName.
     * @param Category - Expected form RNM::Ore#Desc_OreIron_C or RNM::Fluid#Desc_Water_C.
     * @param OutClassFName - UClass FName when parsing succeeds.
     * @return true if a # suffix was present and recognized.
     */
    static bool TryParseClassIdFromCategory(const FString& Category, FName& OutClassFName);

    /**
     * Parses a stable class id from a legacy marker name suffix.
     * @param MarkerName - Name that may end with " #RNM:ClassName" (older markers only).
     * @param OutClassFName - UClass FName when parsing succeeds.
     * @return true if the suffix was found.
     */
    static bool TryParseClassIdFromMarkerName(const FString& MarkerName, FName& OutClassFName);

    /**
     * Builds the map filter category string for an RNM marker.
     * @param bIsFluid - true for liquids/gases (RNM::Fluid), false for solids (RNM::Ore).
     * @param StableClassId - Optional UClass FName appended after # for locale-safe rebuild.
     * @return Category string stored on FMapMarker::CategoryName.
     */
    static FString BuildCategoryName(bool bIsFluid, FName StableClassId = NAME_None);

    /** Squared distance (uu) used to match extractor placement to a scanned node location. */
    static constexpr float MARKER_LOCATION_TOLERANCE_SQ = 100.0f * 100.0f;

private:
    /** Localized display name plus purity counts, e.g. "Iron Ore (2 Pure, 1 Normal)". */
    static FString BuildClusterMarkerName(const FResourceNodeCluster& Cluster);
};