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
                return R;
            }
        }
    }

    FResourceVisual Default;
    FStampPreset Preset;
    Default.IconID = Preset.QuestionMark;
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
