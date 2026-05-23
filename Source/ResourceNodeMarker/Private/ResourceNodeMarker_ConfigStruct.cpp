#include "ResourceNodeMarker_ConfigStruct.h"
#include "ResourceNodeMarker.h"

void FResourceNodeMarker_ConfigStruct::NormalizeLegacyValues(FResourceNodeMarker_ConfigStruct& Config)
{
    // Legacy Highlight enum value — treat as Remove until UE asset drops the entry
    if (Config.ExtractorMarkerBehavior == 2)
        Config.ExtractorMarkerBehavior = 1;
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
