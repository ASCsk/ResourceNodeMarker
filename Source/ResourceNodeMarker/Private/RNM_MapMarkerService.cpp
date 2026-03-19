#include "RNM_MapMarkerService.h"
#include "ResourceNodeMarker.h"
#include "FGMapManager.h"

static const TCHAR* CATEGORY_ORE = TEXT("RNM::Ore");
static const TCHAR* CATEGORY_FLUID = TEXT("RNM::Fluid");

bool RNM_MapMarkerService::CreateMarker(UWorld* World, const FResourceNodeInfo& NodeInfo, URNM_ResourceVisuals* ResourceVisuals, const FResourceNodeMarker_ConfigStruct& Config)
{
    if (!World || !ResourceVisuals) 
        return false;

    AFGMapManager* MapManager = AFGMapManager::Get(World);
    if (!MapManager) 
        return false;

    TArray<FMapMarker> ExistingMarkers;
    MapManager->GetMapMarkers(ExistingMarkers);

    for (const FMapMarker& Existing : ExistingMarkers)
    {
        if (FVector::DistSquared(Existing.Location, NodeInfo.Location) <= MARKER_LOCATION_TOLERANCE_SQ)
        {
            UE_LOG(LogResourceNodeMarker, Warning,
                TEXT("RNM_MapMarkerService: Marker already exists at %s, skipping"),
                *NodeInfo.ResourceName.ToString());
            return false;
        }
    }

    FResourceVisual Visual = ResourceVisuals->GetResourceVisual(NodeInfo.ResourceName, Config.bUseIcons);
    FStampPreset Icons;

    FMapMarker Marker;
    Marker.MarkerGUID = FGuid::NewGuid();
    Marker.Location = NodeInfo.Location;
    Marker.Name = FString::Printf(TEXT("%s (%s)"), *NodeInfo.ResourceName.ToString(), *GetPurityString(NodeInfo.Purity));
    Marker.MapMarkerType = ERepresentationType::RT_MapMarker;
    Marker.IconID = Visual.IconID;
    Marker.Color = FLinearColor::White;
    Marker.CategoryName = (Visual.IconID == Icons.Fluids) ? CATEGORY_FLUID : CATEGORY_ORE;
    Marker.Scale = 1.0f;
    Marker.CompassViewDistance = ParseCompassViewDistance(Config.CompassViewDistance);
    Marker.CreatedByPlayerID = MapManager->GetLocalPlayerID();

    switch (NodeInfo.Purity)
    {
    case RP_Pure:   Marker.Color = Visual.PureColor; break;
    case RP_Normal: Marker.Color = Visual.NormalColor; break;
    case RP_Inpure: Marker.Color = Visual.ImpureColor; break;
    default:        Marker.Color = FLinearColor::White; break;
    }

    FMapMarker CreatedMarker;
    bool bSuccess = MapManager->AddNewMapMarker(Marker, CreatedMarker);

    if (!bSuccess)
    {
        UE_LOG(LogResourceNodeMarker, Warning,
            TEXT("RNM_MapMarkerService: Failed to create marker for %s"),
            *NodeInfo.ResourceName.ToString());
        return false;
    }

    UE_LOG(LogResourceNodeMarker, Log,
        TEXT("RNM_MapMarkerService: Marker created for %s"),
        *NodeInfo.ResourceName.ToString());
    return true;
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