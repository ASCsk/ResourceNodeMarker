#include "ResourceNodeMarker_ConfigStruct.h"
#include "ResourceNodeMarker.h"

void FResourceNodeMarker_ConfigStruct::NormalizeLegacyValues(FResourceNodeMarker_ConfigStruct& Config)
{
    // Legacy Highlight enum value — treat as Remove until UE asset drops the entry
    if (Config.ExtractorMarkerBehavior == 2)
        Config.ExtractorMarkerBehavior = 1;

    // Validate radius values
    if (Config.ProximityRadius <= 0.0f)
    {
        UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: Invalid ProximityRadius %.0f, resetting to default 16000"), Config.ProximityRadius);
        Config.ProximityRadius = 16000.0f;
    }
    if (Config.ClusterRadius <= 0.0f)
    {
        UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: Invalid ClusterRadius %.0f, resetting to default 25000"), Config.ClusterRadius);
        Config.ClusterRadius = 25000.0f;
    }
    if (Config.ClusterHeightTolerance < 0.0f)
    {
        UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM: Invalid ClusterHeightTolerance %.0f, resetting to default 10000"), Config.ClusterHeightTolerance);
        Config.ClusterHeightTolerance = 10000.0f;
    }
}

FResourceNodeMarker_ConfigStruct FResourceNodeMarker_ConfigStruct::GetActiveConfig(UObject* WorldContext)
{
    FResourceNodeMarker_ConfigStruct ConfigStruct{};
    FConfigId ConfigId{"ResourceNodeMarker", ""};

    const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
    if (!World)
    {
        UE_LOG(LogResourceNodeMarker, Warning,
            TEXT("RNM: GetActiveConfig called without a valid world; using default config"));
        return ConfigStruct;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        UE_LOG(LogResourceNodeMarker, Warning,
            TEXT("RNM: GetActiveConfig: no GameInstance; using default config"));
        return ConfigStruct;
    }

    UConfigManager* ConfigManager = GameInstance->GetSubsystem<UConfigManager>();
    if (!ConfigManager)
    {
        UE_LOG(LogResourceNodeMarker, Warning,
            TEXT("RNM: GetActiveConfig: UConfigManager not found; using default config"));
        return ConfigStruct;
    }

    if (!ConfigManager->GetConfigurationById(ConfigId))
    {
        UE_LOG(LogResourceNodeMarker, Warning,
            TEXT("RNM: GetActiveConfig: config '%s' not registered; using default config"),
            *ConfigId.ModReference);
        return ConfigStruct;
    }

    ConfigManager->FillConfigurationStruct(ConfigId, FDynamicStructInfo{
        FResourceNodeMarker_ConfigStruct::StaticStruct(), &ConfigStruct});
    NormalizeLegacyValues(ConfigStruct);
    return ConfigStruct;
}
