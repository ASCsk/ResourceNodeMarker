#include "RNM_MapMarkerService.h"
#include "ResourceNodeMarker.h"
#include "FGItemDescriptor.h"
#include "FGMapManager.h"
#include "FGResourceDescriptor.h"

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

    TSubclassOf<UFGResourceDescriptor> ResClass = nullptr;
    FName LegacyDisplayKey = NAME_None;
    for (const FResourceNodeInfo& N : Cluster.Nodes)
    {
        if (!ResClass && N.ResourceDescriptorClass)
            ResClass = N.ResourceDescriptorClass;
        if (LegacyDisplayKey == NAME_None && N.NodeActor)
            LegacyDisplayKey = FName(*N.NodeActor->GetResourceName().ToString());
    }

    FResourceVisual Visual = ResourceVisuals->GetResourceVisual(
        Cluster.ResourceName, ResClass, Config.bUseIcons, LegacyDisplayKey);
    FStampPreset Icons;
    const bool bIsFluid = ResClass
        ? (UFGItemDescriptor::GetForm(ResClass) == EResourceForm::RF_LIQUID
            || UFGItemDescriptor::GetForm(ResClass) == EResourceForm::RF_GAS)
        : (Visual.IconID == Icons.Fluids);

    FMapMarker Marker;
    Marker.MarkerGUID = FGuid::NewGuid();
    Marker.CreatedByPlayerID = MapManager->GetLocalPlayerID();
    Marker.Location = Cluster.AverageLocation;
    Marker.MapMarkerType = ERepresentationType::RT_MapMarker;
    Marker.IconID = Visual.IconID;
    Marker.Scale = Cluster.GetMarkerScale();
    Marker.Name = BuildClusterMarkerName(Cluster);
    Marker.CategoryName = BuildCategoryName(bIsFluid);
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

    FString ResourceLabel;
    for (const FResourceNodeInfo& N : Cluster.Nodes)
    {
        if (N.NodeActor)
        {
            ResourceLabel = N.NodeActor->GetResourceName().ToString();
            break;
        }
    }
    if (ResourceLabel.IsEmpty())
        ResourceLabel = Cluster.ResourceName.ToString();

    return FString::Printf(TEXT("%s (%s)"), *ResourceLabel, *PurityStr);
}

bool RNM_MapMarkerService::IsRNMMapMarkerCategory(const FString& Category)
{
    return Category == TEXT("RNM::Ore") || Category == TEXT("RNM::Fluid")
        || Category.StartsWith(TEXT("RNM::Ore#")) || Category.StartsWith(TEXT("RNM::Fluid#"));
}

bool RNM_MapMarkerService::TryParseClassIdFromCategory(const FString& Category, FName& OutClassFName)
{
    OutClassFName = NAME_None;
    const int32 HashIdx = Category.Find(TEXT("#"));
    if (HashIdx == INDEX_NONE) return false;

    const FString Prefix = Category.Left(HashIdx);
    if (Prefix != TEXT("RNM::Ore") && Prefix != TEXT("RNM::Fluid")) return false;

    const FString ClassStr = Category.Mid(HashIdx + 1);
    if (ClassStr.IsEmpty()) return false;

    OutClassFName = FName(*ClassStr);
    return true;
}

bool RNM_MapMarkerService::TryParseClassIdFromMarkerName(const FString& MarkerName, FName& OutClassFName)
{
    OutClassFName = NAME_None;
    static const FString Tag(TEXT(" #RNM:"));
    const int32 Idx = MarkerName.Find(Tag);
    if (Idx == INDEX_NONE) return false;

    FString ClassStr = MarkerName.Mid(Idx + Tag.Len()).TrimStartAndEnd();
    if (ClassStr.IsEmpty()) return false;

    OutClassFName = FName(*ClassStr);
    return true;
}

FString RNM_MapMarkerService::BuildCategoryName(const bool bIsFluid)
{
    return bIsFluid ? TEXT("RNM::Fluid") : TEXT("RNM::Ore");
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