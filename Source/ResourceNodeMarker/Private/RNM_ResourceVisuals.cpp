#include "RNM_ResourceVisuals.h"
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

URNM_ResourceVisuals::URNM_ResourceVisuals() = default;

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
    return Default;
}

void URNM_ResourceVisuals::GeneratePurityShades(
    const FLinearColor& BaseColor,
    FLinearColor& OutPure,
    FLinearColor& OutNormal,
    FLinearColor& OutImpure)
{
    OutNormal = BaseColor;

    FLinearColor Hsv = BaseColor.LinearRGBToHSV();

    FLinearColor PureH = Hsv;
    PureH.G = FMath::Clamp(Hsv.G * 1.12f, 0.0f, 1.0f);
    OutPure = PureH.HSVToLinearRGB();

    FLinearColor ImpH = Hsv;
    ImpH.G = FMath::Clamp(Hsv.G * 0.78f, 0.0f, 1.0f);
    OutImpure = ImpH.HSVToLinearRGB();
}
