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
    int32 ExtractorMarkerBehavior = 0; // 0 = Keep, 1 = Remove

    UPROPERTY(BlueprintReadWrite)
    bool bUseIcons = true;

    /* Retrieves active configuration value and returns object of this struct containing it */
    static FResourceNodeMarker_ConfigStruct GetActiveConfig(UObject* WorldContext) {
        FResourceNodeMarker_ConfigStruct ConfigStruct{};
        FConfigId ConfigId{"ResourceNodeMarker", ""};
        if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull)) {
            UConfigManager* ConfigManager = World->GetGameInstance()->GetSubsystem<UConfigManager>();
            ConfigManager->FillConfigurationStruct(ConfigId, FDynamicStructInfo{FResourceNodeMarker_ConfigStruct::StaticStruct(), &ConfigStruct});
        }
        return ConfigStruct;
    }
};

