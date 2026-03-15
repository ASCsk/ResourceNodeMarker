#include "ResourceNodeMarker.h"

#define LOCTEXT_NAMESPACE "FResourceNodeMarkerModule"

DEFINE_LOG_CATEGORY(LogResourceNodeMarker);

void FResourceNodeMarkerModule::StartupModule()
{
    UE_LOG(LogResourceNodeMarker, Log, TEXT("ResourceNodeMarker module loaded"));
}

void FResourceNodeMarkerModule::ShutdownModule()
{
    // cleanup
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FResourceNodeMarkerModule, ResourceNodeMarker)