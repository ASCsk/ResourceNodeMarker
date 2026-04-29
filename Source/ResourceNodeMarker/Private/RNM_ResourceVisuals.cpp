#include "RNM_ResourceVisuals.h"

namespace
{
const FName TryStripBlueprintC(const FName& N)
{
    const FString S = N.ToString();
    if (S.EndsWith(TEXT("_C"), ESearchCase::CaseSensitive) && S.Len() > 2)
        return FName(*S.LeftChop(2));
    return NAME_None;
}
}

URNM_ResourceVisuals::URNM_ResourceVisuals() = default;

FResourceVisual URNM_ResourceVisuals::GetResourceVisual(
    const FName& ResourceName,
    const bool bUseIcons) const
{
    if (const FResourceVisual* Found = ResourceVisualMap.Find(ResourceName))
    {
        FResourceVisual R = *Found;
        if (bUseIcons)
        {
            if (const int32* Icon = IconMap.Find(ResourceName)) R.IconID = *Icon;
        }
        return R;
    }

    const FName Chopped = TryStripBlueprintC(ResourceName);
    if (Chopped != NAME_None)
    {
        if (const FResourceVisual* Found = ResourceVisualMap.Find(Chopped))
        {
            FResourceVisual R = *Found;
            if (bUseIcons)
            {
                if (const int32* Icon = IconMap.Find(Chopped)) R.IconID = *Icon;
            }
            return R;
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
