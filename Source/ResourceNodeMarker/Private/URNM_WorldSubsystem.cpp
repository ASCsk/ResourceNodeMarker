


#include "URNM_WorldSubsystem.h"
#include "Subsystems/SubsystemCollection.h" // needed for FSubsystemCollectionBase

void URNM_WorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM World Subsystem Initialized"));
}

void URNM_WorldSubsystem::Deinitialize()
{
    Super::Deinitialize();

    UE_LOG(LogResourceNodeMarker, Warning, TEXT("RNM World Subsystem Shutdown"));
}
