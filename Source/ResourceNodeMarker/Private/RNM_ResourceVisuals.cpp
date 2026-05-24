#include "RNM_ResourceVisuals.h"
#include "ResourceNodeMarker.h"
#include "FGItemDescriptor.h"
#include "FGResourceDescriptor.h"

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

/** In-game map icon ids (FIconsPreset). */
namespace InGameIcon
{
    constexpr int Copper = 198;
    constexpr int Iron = 193;
    constexpr int Limestone = 204;
    constexpr int Caterium = 200;
    constexpr int Coal = 205;
    constexpr int Sulfur = 203;
    constexpr int Bauxite = 199;
    constexpr int Quartz = 206;
    constexpr int Uranium = 201;
    constexpr int Sam = 280;
}

struct FResourceCatalogEntry
{
    /** Matched on stripped descriptor UClass name; longest match wins (see FindResourceCatalogEntry). */
    const TCHAR* ClassPattern;
    const TCHAR* Hex;
    bool bLiquid;
    int32 IconId;
    const TCHAR* const* MapKeys;
    int32 MapKeyCount;
};

static const TCHAR* CopperMapKeys[] = {
    TEXT("Copper Ore"), TEXT("Desc_OreCopper_C"), TEXT("Desc_OreCopper"), TEXT("BP_OreCopper_C") };
static const TCHAR* IronMapKeys[] = {
    TEXT("Iron Ore"), TEXT("Desc_OreIron_C"), TEXT("Desc_OreIron"), TEXT("BP_OreIron_C") };
static const TCHAR* LimestoneMapKeys[] = {
    TEXT("Limestone"), TEXT("Desc_Stone_C"), TEXT("Desc_Stone"), TEXT("Desc_Limestone_C"), TEXT("BP_Stone_C") };
static const TCHAR* CateriumMapKeys[] = {
    TEXT("Caterium Ore"), TEXT("Desc_OreCaterium_C"), TEXT("Desc_OreCaterium"), TEXT("BP_OreCaterium_C") };
static const TCHAR* CoalMapKeys[] = {
    TEXT("Coal"), TEXT("Desc_OreCoal_C"), TEXT("Desc_OreCoal"), TEXT("BP_OreCoal_C") };
static const TCHAR* SulfurMapKeys[] = {
    TEXT("Sulfur"), TEXT("Desc_OreSulfur_C"), TEXT("Desc_OreSulfur"), TEXT("BP_OreSulfur_C") };
static const TCHAR* BauxiteMapKeys[] = {
    TEXT("Bauxite"), TEXT("Desc_OreBauxite_C"), TEXT("Desc_OreBauxite"), TEXT("BP_OreBauxite_C") };
static const TCHAR* QuartzMapKeys[] = {
    TEXT("Raw Quartz"), TEXT("Desc_OreQuartz_C"), TEXT("Desc_OreQuartz"), TEXT("Desc_Crystal_C"), TEXT("BP_OreQuartz_C") };
static const TCHAR* UraniumMapKeys[] = {
    TEXT("Uranium"), TEXT("Desc_OreUranium_C"), TEXT("Desc_OreUranium"), TEXT("BP_OreUranium_C") };
static const TCHAR* SamMapKeys[] = {
    TEXT("SAM"), TEXT("Desc_SAM_C"), TEXT("Desc_SAM"), TEXT("Desc_OreSAM_C"), TEXT("BP_SAM_C") };
static const TCHAR* WaterMapKeys[] = {
    TEXT("Water"), TEXT("Desc_Water_C"), TEXT("Desc_Water"), TEXT("BP_Water_C") };
static const TCHAR* OilMapKeys[] = {
    TEXT("Crude Oil"), TEXT("Desc_LiquidOil_C"), TEXT("Desc_LiquidOil"), TEXT("BP_LiquidOil_C") };
static const TCHAR* NitrogenMapKeys[] = {
    TEXT("Nitrogen Gas"), TEXT("Desc_LiquidNitrogen_C"), TEXT("Desc_LiquidNitrogen"), TEXT("BP_LiquidNitrogen_C") };
static const TCHAR* GeyserMapKeys[] = {
    TEXT("Geyser"), TEXT("Desc_WaterGeyser_C"), TEXT("Desc_WaterGeyser"), TEXT("BP_WaterGeyser_C") };

/** Single source of truth for hex colors, icon ids, and ResourceVisualMap keys. */
static const FResourceCatalogEntry ResourceCatalog[] = {
    { TEXT("Desc_WaterGeyser"), TEXT("00FAB3"), true, 0, GeyserMapKeys, UE_ARRAY_COUNT(GeyserMapKeys) },
    { TEXT("Desc_LiquidNitrogen"), TEXT("ADADAD"), true, 0, NitrogenMapKeys, UE_ARRAY_COUNT(NitrogenMapKeys) },
    { TEXT("Desc_LiquidOil"), TEXT("1F1F1F"), true, 0, OilMapKeys, UE_ARRAY_COUNT(OilMapKeys) },
    { TEXT("Desc_OreCaterium"), TEXT("FFCB00"), false, InGameIcon::Caterium, CateriumMapKeys, UE_ARRAY_COUNT(CateriumMapKeys) },
    { TEXT("Desc_OreCopper"), TEXT("CF4100"), false, InGameIcon::Copper, CopperMapKeys, UE_ARRAY_COUNT(CopperMapKeys) },
    { TEXT("Desc_OreBauxite"), TEXT("FFB65E"), false, InGameIcon::Bauxite, BauxiteMapKeys, UE_ARRAY_COUNT(BauxiteMapKeys) },
    { TEXT("Desc_OreUranium"), TEXT("85FF2E"), false, InGameIcon::Uranium, UraniumMapKeys, UE_ARRAY_COUNT(UraniumMapKeys) },
    { TEXT("Desc_OreQuartz"), TEXT("FED4FF"), false, InGameIcon::Quartz, QuartzMapKeys, UE_ARRAY_COUNT(QuartzMapKeys) },
    { TEXT("Desc_OreSulfur"), TEXT("E6E615"), false, InGameIcon::Sulfur, SulfurMapKeys, UE_ARRAY_COUNT(SulfurMapKeys) },
    { TEXT("Desc_OreIron"), TEXT("93959E"), false, InGameIcon::Iron, IronMapKeys, UE_ARRAY_COUNT(IronMapKeys) },
    { TEXT("Desc_Stone"), TEXT("D1B97B"), false, InGameIcon::Limestone, LimestoneMapKeys, UE_ARRAY_COUNT(LimestoneMapKeys) },
    { TEXT("Desc_OreCoal"), TEXT("403B3B"), false, InGameIcon::Coal, CoalMapKeys, UE_ARRAY_COUNT(CoalMapKeys) },
    { TEXT("Desc_OreSAM"), TEXT("A332C9"), false, InGameIcon::Sam, SamMapKeys, UE_ARRAY_COUNT(SamMapKeys) },
    { TEXT("Desc_SAM"), TEXT("A332C9"), false, InGameIcon::Sam, SamMapKeys, UE_ARRAY_COUNT(SamMapKeys) },
    { TEXT("Desc_Water"), TEXT("17E3CE"), true, 0, WaterMapKeys, UE_ARRAY_COUNT(WaterMapKeys) },
    { TEXT("Desc_Crystal"), TEXT("FED4FF"), false, InGameIcon::Quartz, QuartzMapKeys, UE_ARRAY_COUNT(QuartzMapKeys) },
};

FString NormalizeDescriptorClassName(FString ClassName)
{
    if (ClassName.EndsWith(TEXT("_C"), ESearchCase::CaseSensitive) && ClassName.Len() > 2)
        return ClassName.Left(ClassName.Len() - 2);
    return ClassName;
}

bool ClassNameMatchesCatalogPattern(const FString& NormalizedClassName, const TCHAR* Pattern)
{
    const FString PatternStr(Pattern);
    if (NormalizedClassName == PatternStr)
        return true;

    if (!NormalizedClassName.StartsWith(PatternStr))
        return false;

    if (NormalizedClassName.Len() == PatternStr.Len())
        return true;

    const TCHAR Next = NormalizedClassName[PatternStr.Len()];
    return Next == TEXT('_');
}

const FResourceCatalogEntry* FindResourceCatalogEntry(const FString& ClassName)
{
    const FString Normalized = NormalizeDescriptorClassName(ClassName);
    const FResourceCatalogEntry* Best = nullptr;
    int32 BestPatternLen = 0;

    for (const FResourceCatalogEntry& Entry : ResourceCatalog)
    {
        if (!ClassNameMatchesCatalogPattern(Normalized, Entry.ClassPattern))
            continue;

        const int32 PatternLen = FCString::Strlen(Entry.ClassPattern);
        if (PatternLen > BestPatternLen)
        {
            BestPatternLen = PatternLen;
            Best = &Entry;
        }
    }

    return Best;
}

FLinearColor ColorFromHex(const FString& Hex)
{
    return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
}

int32 GetStampIconIdForOreOrFluid(TSubclassOf<UFGResourceDescriptor> ResClass)
{
    FStampPreset Sp;
    if (!ResClass) return Sp.Rock;

    const EResourceForm Form = UFGItemDescriptor::GetForm(ResClass);
    if (Form == EResourceForm::RF_LIQUID || Form == EResourceForm::RF_GAS)
        return Sp.Fluids;

    return Sp.Rock;
}

int32 ResolveInGameIconIdFromResourceClass(TSubclassOf<UFGResourceDescriptor> ResClass)
{
    FStampPreset Sp;
    if (!ResClass) return Sp.QuestionMark;

    const EResourceForm Form = UFGItemDescriptor::GetForm(ResClass);
    if (Form == EResourceForm::RF_LIQUID || Form == EResourceForm::RF_GAS)
        return Sp.Fluids;

    if (const FResourceCatalogEntry* Entry = FindResourceCatalogEntry(ResClass->GetName()))
        return Entry->IconId != 0 ? Entry->IconId : Sp.Rock;

    return Sp.Rock;
}

FResourceVisual BuildVisualFromCatalogEntry(
    const FResourceCatalogEntry& Entry,
    const bool bUseIcons,
    TSubclassOf<UFGResourceDescriptor> ResClass)
{
    FResourceVisual Visual;
    FStampPreset Stamp;
    Visual.IconID = Entry.bLiquid ? Stamp.Fluids : Stamp.Rock;
    URNM_ResourceVisuals::GeneratePurityShades(
        ColorFromHex(Entry.Hex),
        Visual.PureColor,
        Visual.NormalColor,
        Visual.ImpureColor);

    if (bUseIcons)
        Visual.IconID = ResClass ? ResolveInGameIconIdFromResourceClass(ResClass)
            : (Entry.IconId != 0 ? Entry.IconId : Stamp.Rock);

    return Visual;
}

bool ApplyCatalogColors(FResourceVisual& Visual, const FString& ClassName)
{
    if (const FResourceCatalogEntry* Entry = FindResourceCatalogEntry(ClassName))
    {
        const FResourceVisual Colors = BuildVisualFromCatalogEntry(
            *Entry, /*bUseIcons=*/false, nullptr);
        Visual.PureColor = Colors.PureColor;
        Visual.NormalColor = Colors.NormalColor;
        Visual.ImpureColor = Colors.ImpureColor;
        return true;
    }
    return false;
}

void LogUnknownVisualOnce(const FName& ResourceClassFName)
{
    static TSet<FName> Warned;
    if (Warned.Contains(ResourceClassFName))
        return;

    Warned.Add(ResourceClassFName);
    UE_LOG(LogResourceNodeMarker, Warning,
        TEXT("RNM_ResourceVisuals: No visual found for class keys around '%s', using fallback"),
        *ResourceClassFName.ToString());
}
}

URNM_ResourceVisuals::URNM_ResourceVisuals()
{
    FStampPreset Stamp;

    for (const FResourceCatalogEntry& Entry : ResourceCatalog)
    {
        FResourceVisual Visual;
        Visual.IconID = Entry.bLiquid ? Stamp.Fluids : Stamp.Rock;
        GeneratePurityShades(
            ColorFromHex(Entry.Hex),
            Visual.PureColor,
            Visual.NormalColor,
            Visual.ImpureColor);

        for (int32 i = 0; i < Entry.MapKeyCount; ++i)
        {
            ResourceVisualMap.Add(FName(Entry.MapKeys[i]), Visual);
            if (Entry.IconId != 0)
                IconMap.Add(FName(Entry.MapKeys[i]), Entry.IconId);
        }
    }
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

    auto TryMapKeys = [&](const TArray<FName>& Keys, const bool bIconMapOnly) -> TOptional<FResourceVisual>
    {
        if (!bIconMapOnly)
        {
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
        }

        if (!bUseIcons) return NullOpt;

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
        return NullOpt;
    };

    FString ClassNameForPattern = ResourceClassFName.ToString();
    if (ResClass)
        ClassNameForPattern = ResClass->GetName();

    // Phase 1: UClass FNames only — stable across game language.
    TArray<FName> ClassKeys;
    AppendUniqueKey(ClassKeys, ResourceClassFName);
    AppendUniqueKey(ClassKeys, TryStripBlueprintC(ResourceClassFName));
    if (ResClass)
    {
        AppendUniqueKey(ClassKeys, ResClass->GetFName());
        AppendUniqueKey(ClassKeys, TryStripBlueprintC(ResClass->GetFName()));
    }

    if (TOptional<FResourceVisual> Found = TryMapKeys(ClassKeys, /*bIconMapOnly=*/false))
        return Found.GetValue();

    // Phase 2: longest ClassPattern match on descriptor UClass name.
    if (const FResourceCatalogEntry* Entry = FindResourceCatalogEntry(ClassNameForPattern))
    {
        FResourceVisual R = BuildVisualFromCatalogEntry(*Entry, bUseIcons, ResClass);
        FinalizeIconId(R);
        return R;
    }

    // Phase 3: localized / legacy display names (English map keys, current locale GetItemName).
    TArray<FName> DisplayKeys;
    AppendUniqueKey(DisplayKeys, OptionalLegacyDisplayKey);
    if (ResClass)
    {
        const FText ItemName = UFGItemDescriptor::GetItemName(ResClass);
        if (!ItemName.IsEmpty())
            AppendUniqueKey(DisplayKeys, FName(*ItemName.ToString()));
    }

    if (TOptional<FResourceVisual> Found = TryMapKeys(DisplayKeys, /*bIconMapOnly=*/false))
        return Found.GetValue();

    if (bUseIcons)
    {
        if (TOptional<FResourceVisual> IconOnly = TryMapKeys(DisplayKeys, /*bIconMapOnly=*/true))
        {
            FResourceVisual R = IconOnly.GetValue();
            const bool bHasCatalogColors = ApplyCatalogColors(R, ClassNameForPattern);
            if (!bHasCatalogColors)
            {
                if (TOptional<FResourceVisual> ClassColors = TryMapKeys(ClassKeys, /*bIconMapOnly=*/false))
                {
                    R.PureColor = ClassColors->PureColor;
                    R.NormalColor = ClassColors->NormalColor;
                    R.ImpureColor = ClassColors->ImpureColor;
                }
            }
            FinalizeIconId(R);
            return R;
        }
    }

    FResourceVisual Default;
    Default.IconID = Preset.QuestionMark;
    if (!bUseIcons)
        Default.IconID = GetStampIconIdForOreOrFluid(ResClass);
    else if (ResClass)
        Default.IconID = ResolveInGameIconIdFromResourceClass(ResClass);

    const bool bHasCatalogColors = ApplyCatalogColors(Default, ClassNameForPattern);
    if (!bHasCatalogColors)
    {
        const FLinearColor NeutralBase(0.55f, 0.55f, 0.55f);
        GeneratePurityShades(
            NeutralBase,
            Default.PureColor,
            Default.NormalColor,
            Default.ImpureColor);
        LogUnknownVisualOnce(ResourceClassFName);
    }
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

    OutPure = FLinearColor(H,
        FMath::Clamp(S * 1.4f, 0.0f, 1.0f),
        FMath::Clamp(V * 0.75f, 0.0f, 1.0f),
        1.0f).HSVToLinearRGB();

    OutImpure = FLinearColor(H,
        FMath::Clamp(S * 0.7f, 0.0f, 1.0f),
        FMath::Clamp(V * 1.5f, 0.0f, 1.0f),
        1.0f).HSVToLinearRGB();
}
