#include "RNM_MapMarkerService.h"
#include "ResourceNodeMarker.h"
#include "FGItemDescriptor.h"
#include "FGMapManager.h"
#include "FGResourceDescriptor.h"
#include "FGResourceNode.h"

namespace
{
bool IsLegacyRNMCategoryWithoutClassId(const FString& Category)
{
    return Category == TEXT("RNM::Ore") || Category == TEXT("RNM::Fluid");
}

FString FormatLocalizedPurityCount(const int32 Count, const EResourcePurity Purity)
{
    if (Count <= 0) return FString();

    FString Label;
    if (const UEnum* PurityEnum = StaticEnum<EResourcePurity>())
    {
        const int64 EnumValue = static_cast<int64>(Purity);
        if (PurityEnum->IsValidEnumValue(EnumValue))
            Label = PurityEnum->GetDisplayNameTextByValue(EnumValue).ToString();
    }

    if (Label.IsEmpty())
    {
        switch (Purity)
        {
        case RP_Pure:   Label = TEXT("Pure"); break;
        case RP_Normal: Label = TEXT("Normal"); break;
        case RP_Inpure: Label = TEXT("Impure"); break;
        default:        return FString();
        }
    }

    return FString::Printf(TEXT("%d %s"), Count, *Label);
}
}

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

    const bool bIsUpdate = Cluster.CurrentMarkerGUID.IsValid();
    const FGuid PreviousMarkerGUID = Cluster.CurrentMarkerGUID;
    if (!bIsUpdate && !MapManager->CanAddNewMapMarker())
    {
        UE_LOG(LogResourceNodeMarker, Warning,
            TEXT("RNM_MapMarkerService: Map marker limit reached (%d), skipping marker for %s"),
            MapManager->GetMaxNumMapMarkers(),
            *Cluster.ResourceName.ToString());
        return false;
    }

    TSubclassOf<UFGResourceDescriptor> ResClass = nullptr;
    FName LegacyDisplayKey = NAME_None;
    for (const FResourceNodeInfo& N : Cluster.Nodes)
    {
        if (!ResClass && N.ResourceDescriptorClass)
            ResClass = N.ResourceDescriptorClass;
        if (!ResClass && N.NodeActor)
            ResClass = N.NodeActor->GetResourceClass();
        if (LegacyDisplayKey == NAME_None && N.NodeActor)
            LegacyDisplayKey = FName(*N.NodeActor->GetResourceName().ToString());
    }

    const FName VisualClassKey = ResClass ? ResClass->GetFName() : Cluster.ResourceName;
    FResourceVisual Visual = ResourceVisuals->GetResourceVisual(
        VisualClassKey, ResClass, Config.bUseIcons, LegacyDisplayKey);
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
    Marker.CategoryName = BuildCategoryName(bIsFluid, VisualClassKey);
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

    // Previous marker removed only after add succeeds; brief overlap stays under the 250 cap.
    if (PreviousMarkerGUID.IsValid() && PreviousMarkerGUID != OutGUID)
    {
        MapManager->Authority_RemoveMapMarkerByID(PreviousMarkerGUID);
        UE_LOG(LogResourceNodeMarker, Log,
            TEXT("RNM_MapMarkerService: Removed existing cluster marker for %s"),
            *Cluster.ResourceName.ToString());
    }

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
        PurityStr += FormatLocalizedPurityCount(PureCount, RP_Pure);

    if (NormalCount > 0)
    {
        if (!PurityStr.IsEmpty()) PurityStr += TEXT(", ");
        PurityStr += FormatLocalizedPurityCount(NormalCount, RP_Normal);
    }

    if (ImpureCount > 0)
    {
        if (!PurityStr.IsEmpty()) PurityStr += TEXT(", ");
        PurityStr += FormatLocalizedPurityCount(ImpureCount, RP_Inpure);
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

FString RNM_MapMarkerService::GetRNMCategoryBase(const FString& Category)
{
    const int32 HashIdx = Category.Find(TEXT("#"));
    return HashIdx == INDEX_NONE ? Category : Category.Left(HashIdx);
}

bool RNM_MapMarkerService::CategoryMatchesRNMFilter(
    const FString& FilterCategory,
    const FString& MarkerCategory)
{
    if (FilterCategory == MarkerCategory)
        return true;

    if (GetRNMCategoryBase(FilterCategory) != GetRNMCategoryBase(MarkerCategory))
        return false;

    // Legacy RNM::Ore / RNM::Fluid filters include stable RNM::Ore#Desc_* markers.
    if (!FilterCategory.Contains(TEXT("#")))
        return true;

    FName FilterClassId;
    FName MarkerClassId;
    const bool bFilterHasClassId = TryParseClassIdFromCategory(FilterCategory, FilterClassId);
    const bool bMarkerHasClassId = TryParseClassIdFromCategory(MarkerCategory, MarkerClassId);
    if (bFilterHasClassId && bMarkerHasClassId)
        return FilterClassId == MarkerClassId;

    return !bFilterHasClassId || !bMarkerHasClassId;
}

bool RNM_MapMarkerService::TryParseClassIdFromCategory(const FString& Category, FName& OutClassFName)
{
    OutClassFName = NAME_None;
    if (IsLegacyRNMCategoryWithoutClassId(Category))
        return false;

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

FString RNM_MapMarkerService::BuildCategoryName(const bool bIsFluid, const FName StableClassId)
{
    const FString Base = bIsFluid ? TEXT("RNM::Fluid") : TEXT("RNM::Ore");
    if (StableClassId != NAME_None)
        return FString::Printf(TEXT("%s#%s"), *Base, *StableClassId.ToString());
    return Base;
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