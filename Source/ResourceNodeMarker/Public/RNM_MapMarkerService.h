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
     * Accepts legacy RNM::Ore / RNM::Fluid / RNM::Ore#ClassFName and localized display categories.
     * @param Category - FMapMarker::CategoryName from the map manager.
     * @param KnownDisplayCategories - Required for localized categories; pass from CollectKnownDisplayCategories.
     */
    static bool IsRNMMapMarkerCategory(
        const FString& Category,
        const TSet<FString>* KnownDisplayCategories);

    /** Removes Satisfactory rich-text markup (e.g. &lt;Bold&gt;) from map marker strings. */
    static FString StripRichTextMarkup(const FString& Text);

    /** Localized resource display labels for all scanned node types in this session. */
    static TSet<FString> CollectKnownDisplayCategories(const TArray<FResourceNodeInfo>& Nodes);

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
     * Builds the map filter category string shown in the map UI dropdown.
     * @param LocalizedDisplayLabel - Localized resource name (marker title resource part).
     * @return Category string stored on FMapMarker::CategoryName.
     */
    static FString BuildCategoryName(const FString& LocalizedDisplayLabel);

    /** Squared distance (uu) used to match extractor placement to a scanned node location. */
    static constexpr float MARKER_LOCATION_TOLERANCE_SQ = 100.0f * 100.0f;

private:
    /** Localized display name plus purity counts, e.g. "Iron Ore (2 Pure, 1 Normal)". */
    static FString BuildClusterMarkerName(
        const FResourceNodeCluster& Cluster,
        const FString& ResourceLabel);
};