#pragma once
#include "CoreMinimal.h"
#include "Configuration/ConfigManager.h"
#include "Engine/Engine.h"
#include "ResourceNodeMarker_ConfigStruct.generated.h"

/* Struct generated from Mod Configuration Asset '/ResourceNodeMarker/ResourceNodeMarker_Config' */
USTRUCT(BlueprintType)
struct FResourceNodeMarker_ConfigStruct {
    GENERATED_BODY()
public:

    // Purity
    UPROPERTY(BlueprintReadWrite)
    bool bMarkPure = true;

    UPROPERTY(BlueprintReadWrite)
    bool bMarkNormal = true;

    UPROPERTY(BlueprintReadWrite)
    bool bMarkImpure = true;

    UPROPERTY(BlueprintReadWrite)
    float ProximityRadius = 160.0f; //meters, converted to cm in code

    UPROPERTY(BlueprintReadWrite)
    int32 CompassViewDistance = 2; // 2 = Mid

    UPROPERTY(BlueprintReadWrite)
    int32 ExtractorMarkerBehavior = 0; // 0 = Keep, 1 = Remove; legacy 2 (Highlight) normalized to 1 on load

    /**
     * When false: map markers use stock stamps only — rock for solid resources, fluid/drop stamp for liquids and gases.
     * When true: use per-resource icons from ResourceVisualMap / IconMap (in-game style icons).
     */
    UPROPERTY(BlueprintReadWrite)
    bool bUseIcons = false;

    UPROPERTY(BlueprintReadWrite)
    float ClusterRadius = 250.0f; // meters, converted to cm in code

    UPROPERTY(BlueprintReadWrite)
    float ClusterHeightTolerance = 100.0f; // meters, converted to cm in code

    /**
     * When true (default), nearby nodes of the same resource share one map marker.
     * When false, each node gets its own marker.
     * UE config: BP_ConfigPropertyBool checkbox (same pattern as bMarkPure / bUseIcons).
     * SML reads config once at world begin play — reload required to apply changes.
     */
    UPROPERTY(BlueprintReadWrite)
    bool bClusterNodes = true;

    static float GetClusterRadiusCm(const FResourceNodeMarker_ConfigStruct& Config)
    {
        return FMath::Max(Config.ClusterRadius * 100.0f, 1.0f);
    }

    static float GetClusterHeightToleranceCm(const FResourceNodeMarker_ConfigStruct& Config)
    {
        return FMath::Max(Config.ClusterHeightTolerance * 100.0f, 0.0f);
    }

    static bool IsClusteringEnabled(const FResourceNodeMarker_ConfigStruct& Config)
    {
        return Config.bClusterNodes;
    }

    /** Maps legacy config values after load (e.g. Highlight → Remove). */
    static void NormalizeLegacyValues(FResourceNodeMarker_ConfigStruct& Config);

    static FResourceNodeMarker_ConfigStruct GetActiveConfig(UObject* WorldContext);
};

