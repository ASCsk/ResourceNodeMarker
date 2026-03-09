// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Subsystems/WorldSubsystem.h"
#include "FGResourceNode.h"
#include "URNM_WorldSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogResourceNodeMarker, Log, All);

class FResourceNodeMarkerModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
