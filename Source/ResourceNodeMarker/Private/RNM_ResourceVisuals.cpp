#include "RNM_ResourceVisuals.h"
#include "ResourceNodeMarker.h"
#include "FGItemDescriptor.h"
#include "FGResourceDescriptor.h"

#include <initializer_list>

namespace
{
const FName TryStripBlueprintC(const FName& N)
{
    const FString S = N.ToString();
    if (S.EndsWith(TEXT("_C"), ESearchCase::CaseSensitive) && S.Len() > 2)
        return FName(*S.LeftChop(2));
    return NAME_None;
}

void AppendUniqueKey(TArray<FName>& Keys, const FName& N)
{
    if (N != NAME_None && !Keys.Contains(N))
        Keys.Add(N);
}

/** Stock stamps when bUseIcons is false: rock for solids, fluid/drop for liquids/gases. */
int32 GetStampIconIdForOreOrFluid(TSubclassOf<UFGResourceDescriptor> ResClass)
{
    FStampPreset Sp;
    if (!ResClass) return Sp.Rock;

    const EResourceForm Form = UFGItemDescriptor::GetForm(ResClass);
    if (Form == EResourceForm::RF_LIQUID || Form == EResourceForm::RF_GAS)
        return Sp.Fluids;

    return Sp.Rock;
}

/** When bUseIcons is true and visuals miss: map descriptor class names to in-game map icon IDs (FIconsPreset). */
int32 ResolveInGameIconIdFromResourceClass(TSubclassOf<UFGResourceDescriptor> ResClass)
{
    if (!ResClass) return 656;

    const EResourceForm Form = UFGItemDescriptor::GetForm(ResClass);
    if (Form == EResourceForm::RF_LIQUID || Form == EResourceForm::RF_GAS)
        return 657;

    const FString S = ResClass->GetName();
    FIconsPreset Ip;
    if (S.Contains(TEXT("Iron"))) return Ip.Iron;
    if (S.Contains(TEXT("Copper"))) return Ip.Copper;
    if (S.Contains(TEXT("Limestone"))) return Ip.Limestone;
    if (S.Contains(TEXT("Stone"))) return Ip.Limestone;
    if (S.Contains(TEXT("Coal"))) return Ip.Coal;
    if (S.Contains(TEXT("Sulfur"))) return Ip.Sulfur;
    if (S.Contains(TEXT("Bauxite"))) return Ip.Bauxite;
    if (S.Contains(TEXT("Quartz"))) return Ip.Quartz;
    if (S.Contains(TEXT("Uranium"))) return Ip.Uranium;
    if (S.Contains(TEXT("Caterium"))) return Ip.Caterium;
    if (S.Contains(TEXT("Sam"))) return Ip.Sam;

    FStampPreset Sp;
    return Sp.Rock;
}
}

URNM_ResourceVisuals::URNM_ResourceVisuals()
{
    FStampPreset Stamp;
    FIconsPreset Icons;

    auto MakeColor = [](const FString& Hex)
    {
        return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
    };

    /** Same purity shades + stamp icon for every lookup key (English display + UClass FNames). */
    auto AddResourceGroup = [&](const std::initializer_list<const TCHAR*> Names, const FString& BaseHex,
                                const bool bLiquid)
    {
        FResourceVisual Visual;
        Visual.IconID = bLiquid ? Stamp.Fluids : Stamp.Rock;
        GeneratePurityShades(
            MakeColor(BaseHex),
            Visual.PureColor,
            Visual.NormalColor,
            Visual.ImpureColor);
        for (const TCHAR* Name : Names)
            ResourceVisualMap.Add(FName(Name), Visual);
    };

    // Solid ores: legacy English keys (main) plus descriptor class FNames for language-independent scans.
    AddResourceGroup(
        {TEXT("Copper Ore"), TEXT("Desc_OreCopper_C"), TEXT("Desc_OreCopper"), TEXT("BP_OreCopper_C")},
        TEXT("CF4100"), false);
    AddResourceGroup(
        {TEXT("Iron Ore"), TEXT("Desc_OreIron_C"), TEXT("Desc_OreIron"), TEXT("BP_OreIron_C")},
        TEXT("93959E"), false);
    AddResourceGroup(
        {TEXT("Limestone"), TEXT("Desc_Stone_C"), TEXT("Desc_Stone"), TEXT("Desc_Limestone_C"), TEXT("BP_Stone_C")},
        TEXT("D1B97B"), false);
    AddResourceGroup(
        {TEXT("Caterium Ore"), TEXT("Desc_OreCaterium_C"), TEXT("Desc_OreCaterium"), TEXT("BP_OreCaterium_C")},
        TEXT("FFCB00"), false);
    AddResourceGroup(
        {TEXT("Coal"), TEXT("Desc_OreCoal_C"), TEXT("Desc_OreCoal"), TEXT("BP_OreCoal_C")},
        TEXT("403B3B"), false);
    AddResourceGroup(
        {TEXT("Sulfur"), TEXT("Desc_OreSulfur_C"), TEXT("Desc_OreSulfur"), TEXT("BP_OreSulfur_C")},
        TEXT("E6E615"), false);
    AddResourceGroup(
        {TEXT("Bauxite"), TEXT("Desc_OreBauxite_C"), TEXT("Desc_OreBauxite"), TEXT("BP_OreBauxite_C")},
        TEXT("FFB65E"), false);
    AddResourceGroup(
        {TEXT("Raw Quartz"), TEXT("Desc_OreQuartz_C"), TEXT("Desc_OreQuartz"), TEXT("Desc_Crystal_C"), TEXT("BP_OreQuartz_C")},
        TEXT("FED4FF"), false);
    AddResourceGroup(
        {TEXT("Uranium"), TEXT("Desc_OreUranium_C"), TEXT("Desc_OreUranium"), TEXT("BP_OreUranium_C")},
        TEXT("85FF2E"), false);
    AddResourceGroup(
        {TEXT("SAM"), TEXT("Desc_SAM_C"), TEXT("Desc_SAM"), TEXT("Desc_OreSAM_C"), TEXT("BP_SAM_C")},
        TEXT("A332C9"), false);

    AddResourceGroup(
        {TEXT("Water"), TEXT("Desc_Water_C"), TEXT("Desc_Water"), TEXT("BP_Water_C")},
        TEXT("17E3CE"), true);
    AddResourceGroup(
        {TEXT("Crude Oil"), TEXT("Desc_LiquidOil_C"), TEXT("Desc_LiquidOil"), TEXT("BP_LiquidOil_C")},
        TEXT("1F1F1F"), true);
    AddResourceGroup(
        {TEXT("Nitrogen Gas"), TEXT("Desc_LiquidNitrogen_C"), TEXT("Desc_LiquidNitrogen"), TEXT("BP_LiquidNitrogen_C")},
        TEXT("ADADAD"), true);
    AddResourceGroup(
        {TEXT("Geyser"), TEXT("Desc_WaterGeyser_C"), TEXT("Desc_WaterGeyser"), TEXT("BP_WaterGeyser_C")},
        TEXT("00FAB3"), true);

    auto AddIconPair = [&](const std::initializer_list<const TCHAR*> Names, const int32 IconId)
    {
        for (const TCHAR* N : Names)
            IconMap.Add(FName(N), IconId);
    };

    AddIconPair({TEXT("Copper Ore"), TEXT("Desc_OreCopper_C"), TEXT("Desc_OreCopper"), TEXT("BP_OreCopper_C")}, Icons.Copper);
    AddIconPair({TEXT("Iron Ore"), TEXT("Desc_OreIron_C"), TEXT("Desc_OreIron"), TEXT("BP_OreIron_C")}, Icons.Iron);
    AddIconPair({TEXT("Limestone"), TEXT("Desc_Stone_C"), TEXT("Desc_Stone"), TEXT("Desc_Limestone_C"), TEXT("BP_Stone_C")},
        Icons.Limestone);
    AddIconPair({TEXT("Caterium Ore"), TEXT("Desc_OreCaterium_C"), TEXT("Desc_OreCaterium"), TEXT("BP_OreCaterium_C")},
        Icons.Caterium);
    AddIconPair({TEXT("Coal"), TEXT("Desc_OreCoal_C"), TEXT("Desc_OreCoal"), TEXT("BP_OreCoal_C")}, Icons.Coal);
    AddIconPair({TEXT("Sulfur"), TEXT("Desc_OreSulfur_C"), TEXT("Desc_OreSulfur"), TEXT("BP_OreSulfur_C")}, Icons.Sulfur);
    AddIconPair({TEXT("Bauxite"), TEXT("Desc_OreBauxite_C"), TEXT("Desc_OreBauxite"), TEXT("BP_OreBauxite_C")}, Icons.Bauxite);
    AddIconPair({TEXT("Raw Quartz"), TEXT("Desc_OreQuartz_C"), TEXT("Desc_OreQuartz"), TEXT("Desc_Crystal_C"), TEXT("BP_OreQuartz_C")},
        Icons.Quartz);
    AddIconPair({TEXT("Uranium"), TEXT("Desc_OreUranium_C"), TEXT("Desc_OreUranium"), TEXT("BP_OreUranium_C")}, Icons.Uranium);
    AddIconPair({TEXT("SAM"), TEXT("Desc_SAM_C"), TEXT("Desc_SAM"), TEXT("Desc_OreSAM_C"), TEXT("BP_SAM_C")}, Icons.Sam);
}

FResourceVisual URNM_ResourceVisuals::GetResourceVisual(const FName& ResourceName, const bool bUseIcons) const
{
    return GetResourceVisual(ResourceName, nullptr, bUseIcons, NAME_None);
}

FResourceVisual URNM_ResourceVisuals::GetResourceVisual(
    const FName& ResourceClassFName,
    TSubclassOf<UFGResourceDescriptor> ResClass,
    const bool bUseIcons,
    const FName OptionalLegacyDisplayKey) const
{
    TArray<FName> Keys;
    AppendUniqueKey(Keys, ResourceClassFName);
    AppendUniqueKey(Keys, TryStripBlueprintC(ResourceClassFName));
    AppendUniqueKey(Keys, OptionalLegacyDisplayKey);

    if (ResClass)
    {
        AppendUniqueKey(Keys, ResClass->GetFName());
        AppendUniqueKey(Keys, TryStripBlueprintC(ResClass->GetFName()));
        // Legacy map keys: English (or current locale) display name from the item descriptor
        const FText ItemName = UFGItemDescriptor::GetItemName(ResClass);
        if (!ItemName.IsEmpty())
            AppendUniqueKey(Keys, FName(*ItemName.ToString()));
    }

    FStampPreset Preset;

    auto FinalizeIconId = [&](FResourceVisual& R)
    {
        if (!bUseIcons)
        {
            R.IconID = GetStampIconIdForOreOrFluid(ResClass);
            return;
        }
        if (!ResClass) return;
        if (R.IconID != Preset.QuestionMark && R.IconID != 0) return;
        R.IconID = ResolveInGameIconIdFromResourceClass(ResClass);
    };

    for (const FName& Key : Keys)
    {
        if (const FResourceVisual* Found = ResourceVisualMap.Find(Key))
        {
            FResourceVisual R = *Found;
            if (bUseIcons)
            {
                if (const int32* Icon = IconMap.Find(Key))
                    R.IconID = *Icon;
            }
            FinalizeIconId(R);
            return R;
        }
    }

    if (bUseIcons)
    {
        for (const FName& Key : Keys)
        {
            if (const int32* Icon = IconMap.Find(Key))
            {
                FResourceVisual R;
                R.IconID = *Icon;
                FinalizeIconId(R);
                return R;
            }
        }
    }

    FResourceVisual Default;
    Default.IconID = Preset.QuestionMark;
    if (!bUseIcons)
        Default.IconID = GetStampIconIdForOreOrFluid(ResClass);
    else if (ResClass)
        Default.IconID = ResolveInGameIconIdFromResourceClass(ResClass);

    UE_LOG(LogResourceNodeMarker, Warning,
        TEXT("RNM_ResourceVisuals: No visual found for keys around '%s', using fallback"),
        *ResourceClassFName.ToString());
    return Default;
}

void URNM_ResourceVisuals::GeneratePurityShades(
    const FLinearColor& BaseColor,
    FLinearColor& OutPure,
    FLinearColor& OutNormal,
    FLinearColor& OutImpure)
{
    FLinearColor HSV = BaseColor.LinearRGBToHSV();
    const float H = HSV.R;
    const float S = HSV.G;
    const float V = HSV.B;

    OutNormal = BaseColor;

    // Pure: more saturated, slightly darker
    OutPure = FLinearColor(H,
        FMath::Clamp(S * 1.4f, 0.0f, 1.0f),
        FMath::Clamp(V * 0.75f, 0.0f, 1.0f),
        1.0f).HSVToLinearRGB();

    // Impure: less saturated, brighter
    OutImpure = FLinearColor(H,
        FMath::Clamp(S * 0.7f, 0.0f, 1.0f),
        FMath::Clamp(V * 1.5f, 0.0f, 1.0f),
        1.0f).HSVToLinearRGB();
}
