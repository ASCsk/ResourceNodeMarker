#pragma once

#include "CoreMinimal.h"
#include "FGResourceNode.h"
#include "RNM_ResourceNodeInfo.generated.h"

USTRUCT()
struct FResourceNodeInfo
{
    GENERATED_BODY()

    UPROPERTY()
    AFGResourceNode* NodeActor = nullptr;

    FVector Location;
    FName ResourceName;
    EResourcePurity Purity;
    
};