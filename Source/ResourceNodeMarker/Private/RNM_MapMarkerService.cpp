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

void ResolveClusterResourceContext(
    const FResourceNodeCluster& Cluster,
    TSubclassOf<UFGResourceDescriptor>& OutResClass,
    FName& OutLegacyDisplayKey)
{
    OutResClass = nullptr;
    OutLegacyDisplayKey = NAME_None;

    for (const FResourceNodeInfo& N : Cluster.Nodes)
    {
        if (!OutResClass && N.ResourceDescriptorClass)
            OutResClass = N.ResourceDescriptorClass;
        if (!OutResClass && N.NodeActor)
            OutResClass = N.NodeActor->GetResourceClass();
        if (OutLegacyDisplayKey == NAME_None && N.NodeActor)
        {
            const FText ResourceName = N.NodeActor->GetResourceName();
            if (!ResourceName.IsEmpty())
            {
                OutLegacyDisplayKey = FName(*RNM_MapMarkerService::StripRichTextMarkup(
                    ResourceName.ToString()));
            }
        }
    }
}

FString GetClusterResourceDisplayLabel(
    const FResourceNodeCluster& Cluster,
    TSubclassOf<UFGResourceDescriptor> ResClass)
{
    for (const FResourceNodeInfo& N : Cluster.Nodes)
    {
        if (N.NodeActor)
        {
            if (TSubclassOf<UFGResourceDescriptor> NodeClass = N.NodeActor->GetResourceClass())
            {
                const FText ItemName = UFGItemDescriptor::GetItemName(NodeClass);
                if (!ItemName.IsEmpty())
                {
                    return RNM_MapMarkerService::StripRichTextMarkup(ItemName.ToString());
                }
            }

            const FText ResourceName = N.NodeActor->GetResourceName();
            if (!ResourceName.IsEmpty())
            {
                return RNM_MapMarkerService::StripRichTextMarkup(ResourceName.ToString());
            }
        }
    }

    if (ResClass)
    {
        const FText ItemName = UFGItemDescriptor::GetItemName(ResClass);
        if (!ItemName.IsEmpty())
            return RNM_MapMarkerService::StripRichTextMarkup(ItemName.ToString());
    }

    return Cluster.ResourceName.ToString();
}

FString GetPurityDisplayLabel(const FResourceNodeCluster& Cluster, const EResourcePurity Purity)
{
    for (const FResourceNodeInfo& Node : Cluster.Nodes)
    {
        if (!Node.NodeActor || Node.Purity != Purity)
            continue;

        const FText GameLabel = Node.NodeActor->GetResoucePurityText();
        if (!GameLabel.IsEmpty())
            return RNM_MapMarkerService::StripRichTextMarkup(GameLabel.ToString());
        continue;
    }

    if (const UEnum* PurityEnum = StaticEnum<EResourcePurity>())
    {
        const int64 EnumValue = static_cast<int64>(Purity);
        if (PurityEnum->IsValidEnumValue(EnumValue))
        {
            const FText EnumLabel = PurityEnum->GetDisplayNameTextByValue(EnumValue);
            if (!EnumLabel.IsEmpty())
                return RNM_MapMarkerService::StripRichTextMarkup(EnumLabel.ToString());
        }
    }

    switch (Purity)
    {
    case RP_Pure:   return TEXT("Pure");
    case RP_Normal: return TEXT("Normal");
    case RP_Inpure: return TEXT("Impure");
    default:        return FString();
    }
}

FString FormatPurityCountSegment(
    const FResourceNodeCluster& Cluster,
    const int32 Count,
    const EResourcePurity Purity)
{
    if (Count <= 0) return FString();

    const FString Label = GetPurityDisplayLabel(Cluster, Purity);
    if (Label.IsEmpty()) return FString();

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
    ResolveClusterResourceContext(Cluster, ResClass, LegacyDisplayKey);

    // Cluster.ResourceName is descriptor UClass FName (Desc_*_C), not localized display text.
    const FName VisualClassKey = ResClass ? ResClass->GetFName() : Cluster.ResourceName;
    FResourceVisual Visual = ResourceVisuals->GetResourceVisual(
        VisualClassKey, ResClass, Config.bUseIcons, LegacyDisplayKey);

    const FString DisplayLabel = GetClusterResourceDisplayLabel(Cluster, ResClass);

    FMapMarker Marker;
    Marker.MarkerGUID = FGuid::NewGuid();
    Marker.CreatedByPlayerID = MapManager->GetLocalPlayerID();
    Marker.Location = Cluster.AverageLocation;
    Marker.MapMarkerType = ERepresentationType::RT_MapMarker;
    Marker.IconID = Visual.IconID;
    Marker.Scale = Cluster.GetMarkerScale();
    Marker.Name = BuildClusterMarkerName(Cluster, DisplayLabel);
    Marker.CategoryName = BuildCategoryName(DisplayLabel);
    Marker.CompassViewDistance = ParseCompassViewDistance(Config.CompassViewDistance);

    switch (Cluster.DominantPurity)
    {
    case RP_Pure:   Marker.Color = Visual.PureColor;   break;
    case RP_Normal: Marker.Color = Visual.NormalColor; break;
    case RP_Inpure: Marker.Color = Visual.ImpureColor; break;
    default:        Marker.Color = Visual.NormalColor; break;
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

FString RNM_MapMarkerService::BuildClusterMarkerName(
    const FResourceNodeCluster& Cluster,
    const FString& ResourceLabel)
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
        PurityStr += FormatPurityCountSegment(Cluster, PureCount, RP_Pure);

    if (NormalCount > 0)
    {
        if (!PurityStr.IsEmpty()) PurityStr += TEXT(", ");
        PurityStr += FormatPurityCountSegment(Cluster, NormalCount, RP_Normal);
    }

    if (ImpureCount > 0)
    {
        if (!PurityStr.IsEmpty()) PurityStr += TEXT(", ");
        PurityStr += FormatPurityCountSegment(Cluster, ImpureCount, RP_Inpure);
    }

    if (PurityStr.IsEmpty())
        return ResourceLabel;

    return FString::Printf(TEXT("%s (%s)"), *ResourceLabel, *PurityStr);
}

FString RNM_MapMarkerService::StripRichTextMarkup(const FString& Text)
{
    FString Plain = Text;
    for (;;)
    {
        const int32 Open = Plain.Find(TEXT("<"), ESearchCase::CaseSensitive);
        if (Open == INDEX_NONE)
            break;

        const int32 Close = Plain.Find(TEXT(">"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Open);
        if (Close == INDEX_NONE)
            break;

        Plain.RemoveAt(Open, Close - Open + 1);
    }
    return Plain.TrimStartAndEnd();
}

bool RNM_MapMarkerService::IsRNMMapMarkerCategory(
    const FString& Category,
    const TSet<FString>* KnownDisplayCategories)
{
    if (Category == TEXT("RNM::Ore") || Category == TEXT("RNM::Fluid")
        || Category.StartsWith(TEXT("RNM::Ore#")) || Category.StartsWith(TEXT("RNM::Fluid#")))
        return true;

    if (!KnownDisplayCategories)
        return false;

    const FString PlainCategory = StripRichTextMarkup(Category);
    return KnownDisplayCategories->Contains(PlainCategory);
}

TSet<FString> RNM_MapMarkerService::CollectKnownDisplayCategories(const TArray<FResourceNodeInfo>& Nodes)
{
    TSet<FString> Categories;
    for (const FResourceNodeInfo& Node : Nodes)
    {
        if (Node.ResourceDescriptorClass)
        {
            const FText ItemName = UFGItemDescriptor::GetItemName(Node.ResourceDescriptorClass);
            if (!ItemName.IsEmpty())
                Categories.Add(StripRichTextMarkup(ItemName.ToString()));
        }

        if (Node.NodeActor)
        {
            const FText ResourceName = Node.NodeActor->GetResourceName();
            if (!ResourceName.IsEmpty())
                Categories.Add(StripRichTextMarkup(ResourceName.ToString()));
        }
    }
    return Categories;
}

bool RNM_MapMarkerService::TryParseClassIdFromCategory(const FString& Category, FName& OutClassFName)
{
    // Legacy markers only (RNM::Ore#Desc_OreIron_C). New markers use localized CategoryName.
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

FString RNM_MapMarkerService::BuildCategoryName(const FString& LocalizedDisplayLabel)
{
    return StripRichTextMarkup(LocalizedDisplayLabel);
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