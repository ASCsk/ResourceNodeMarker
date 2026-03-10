#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FGResourceNode.h"
#include "RNM_ResourceNodeInfo.h"

/**
 * Helper class to scan the world for resource nodes
 */
class RNM_NodeScanner
{
public:
    RNM_NodeScanner() = default;
    ~RNM_NodeScanner() = default;

    /**
     * Scan the world for extractable resource nodes.
     * @param World - The world to scan.
     * @param OutNodes - Array to store found nodes.
     */
    static void ScanNodes(UWorld* World, TArray<FResourceNodeInfo>& OutNodes);
};