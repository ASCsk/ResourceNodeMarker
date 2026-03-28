#include "RNM_MapMarkerService.h"
#include "ResourceNodeMarker.h"
#include "FGMapManager.h"

static const TCHAR* CATEGORY_ORE = TEXT("RNM::Ore");
static const TCHAR* CATEGORY_FLUID = TEXT("RNM::Fluid");

bool RNM_MapMarkerService::CreateOrUpdateClusterMarker(
    UWorld* World,
    FResourceNodeCluster& Cluster,
    URNM_ResourceVisuals* ResourceVisuals,
    const FResourceNodeMarker_ConfigStruct& Config,
    FGuid& OutGUID)
{
    if (!World || !ResourceVisuals) return false;

    AFGMapManager* MapManager = AFGMapManager::Get(World);
    if (!MapManager) return false;

    // Delete existing marker if this cluster already has one
    if (Cluster.CurrentMarkerGUID.IsValid())
    {
        MapManager->Authority_RemoveMapMarkerByID(Cluster.CurrentMarkerGUID);
        Cluster.CurrentMarkerGUID.Invalidate();
        UE_LOG(LogResourceNodeMarker, Log,
            TEXT("RNM_MapMarkerService: Removed existing cluster marker for %s"),
            *Cluster.ResourceName.ToString());
    }

    FResourceVisual Visual = ResourceVisuals->GetResourceVisual(Cluster.ResourceName, Config.bUseIcons);
    FStampPreset Icons;

    FMapMarker Marker;
    Marker.MarkerGUID = FGuid::NewGuid();
    Marker.CreatedByPlayerID = MapManager->GetLocalPlayerID();
    Marker.Location = Cluster.AverageLocation;
    Marker.MapMarkerType = ERepresentationType::RT_MapMarker;
    Marker.IconID = Visual.IconID;
    Marker.Scale = Cluster.GetMarkerScale();
    Marker.Name = BuildClusterMarkerName(Cluster);
    Marker.CategoryName = (Visual.IconID == Icons.Fluids) ? CATEGORY_FLUID : CATEGORY_ORE;
    Marker.CompassViewDistance = ParseCompassViewDistance(Config.CompassViewDistance);

    switch (Cluster.DominantPurity)
    {
    case RP_Pure:   Marker.Color = Visual.PureColor;   break;
    case RP_Normal: Marker.Color = Visual.NormalColor; break;
    case RP_Inpure: Marker.Color = Visual.ImpureColor; break;
    default:        Marker.Color = FLinearColor::White; break;
    }

    FMapMarker CreatedMarker;
    bool bSuccess = MapManager->AddNewMapMarker(Marker, CreatedMarker);

    if (!bSuccess)
    {
        UE_LOG(LogResourceNodeMarker, Warning,
            TEXT("RNM_MapMarkerService: Failed to create cluster marker for %s"),
            *Cluster.ResourceName.ToString());
        return false;
    }

    OutGUID = CreatedMarker.MarkerGUID;

    UE_LOG(LogResourceNodeMarker, Log,
        TEXT("RNM_MapMarkerService: Cluster marker created for %s (%d nodes)"),
        *Cluster.ResourceName.ToString(),
        Cluster.Nodes.Num());

    return true;
}

FString RNM_MapMarkerService::BuildClusterMarkerName(const FResourceNodeCluster& Cluster)
{
    int32 PureCount = 0;
    int32 NormalCount = 0;
    int32 ImpureCount = 0;

    for (const FResourceNodeInfo& Node : Cluster.Nodes)
    {
        switch (Node.Purity)
        {
        case RP_Pure:   PureCount++;   break;
        case RP_Normal: NormalCount++; break;
        case RP_Inpure: ImpureCount++; break;
        }
    }

    FString PurityStr;

    if (PureCount > 0)
        PurityStr += FString::Printf(TEXT("%d Pure"), PureCount);

    if (NormalCount > 0)
    {
        if (!PurityStr.IsEmpty()) PurityStr += TEXT(", ");
        PurityStr += FString::Printf(TEXT("%d Normal"), NormalCount);
    }

    if (ImpureCount > 0)
    {
        if (!PurityStr.IsEmpty()) PurityStr += TEXT(", ");
        PurityStr += FString::Printf(TEXT("%d Impure"), ImpureCount);
    }

    return FString::Printf(TEXT("%s (%s)"), *Cluster.ResourceName.ToString(), *PurityStr);
}

FString RNM_MapMarkerService::GetPurityString(EResourcePurity Purity)
{
    switch (Purity)
    {
    case RP_Inpure: return TEXT("Impure");
    case RP_Normal: return TEXT("Normal");
    case RP_Pure:   return TEXT("Pure");
    default:        return TEXT("Unknown");
    }
}

ECompassViewDistance RNM_MapMarkerService::ParseCompassViewDistance(int32 Value)
{
    switch (Value)
    {
    case 0:  return ECompassViewDistance::CVD_Off;
    case 1:  return ECompassViewDistance::CVD_Near;
    case 2:  return ECompassViewDistance::CVD_Mid;
    case 3:  return ECompassViewDistance::CVD_Far;
    case 4:  return ECompassViewDistance::CVD_Always;
    default:
        UE_LOG(LogResourceNodeMarker, Warning,
            TEXT("RNM_MapMarkerService: Invalid CompassViewDistance value '%d', defaulting to Mid"),
            Value);
        return ECompassViewDistance::CVD_Mid;
    }
}