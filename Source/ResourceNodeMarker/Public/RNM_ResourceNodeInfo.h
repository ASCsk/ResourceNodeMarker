#pragma once

#include "CoreMinimal.h"
#include "FGResourceNode.h"
#include "RNM_ResourceNodeInfo.generated.h"

USTRUCT()
struct FResourceNodeInfo
{
    GENERATED_BODY()

    FVector Location;
    FName ResourceName;
    EResourcePurity Purity;
    AFGResourceNode* NodeActor = nullptr;
};