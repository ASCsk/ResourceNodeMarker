#include "RNM_ResourceVisuals.h"
#include "FGResourceDescriptor.h"
#include "FGItemDescriptor.h"

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

URNM_ResourceVisuals::URNM_ResourceVisuals()
{
    FIconPreset Icons;

    auto MakeColor = [](const FString& Hex)
        {
            return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
        };

    auto AddSolid = [&](const FName& Name, const FString& BaseHex)
        {
            FResourceVisual Visual;
            Visual.IconID = Icons.Rock;
            GeneratePurityShades(
                MakeColor(BaseHex),
                Visual.PureColor,
                Visual.NormalColor,
                Visual.ImpureColor);

            ResourceVisualMap.Add(Name, Visual);
        };

    auto AddLiquid = [&](const FName& Name, const FString& BaseHex)
        {
            FResourceVisual Visual;
            Visual.IconID = Icons.Fluids;
            GeneratePurityShades(
                MakeColor(BaseHex),
                Visual.PureColor,
                Visual.NormalColor,
                Visual.ImpureColor);

            ResourceVisualMap.Add(Name, Visual);
        };

    // ROCK RESOURCES — base color is the Normal shade from before
    AddSolid(TEXT("Copper Ore"), TEXT("CF4100"));
    AddSolid(TEXT("Iron Ore"), TEXT("93959E"));
    AddSolid(TEXT("Limestone"), TEXT("D1B97B"));
    AddSolid(TEXT("Caterium Ore"), TEXT("FFCB00"));
    AddSolid(TEXT("Coal"), TEXT("403B3B"));
    AddSolid(TEXT("Sulfur"), TEXT("E6E615"));
    AddSolid(TEXT("Bauxite"), TEXT("FFB65E"));
    AddSolid(TEXT("Raw Quartz"), TEXT("FED4FF"));
    AddSolid(TEXT("Uranium"), TEXT("85FF2E"));
    AddSolid(TEXT("SAM Ore"), TEXT("A332C9"));

    // FLUID RESOURCES
    AddLiquid(TEXT("Water"), TEXT("17E3CE"));
    AddLiquid(TEXT("Crude Oil"), TEXT("1F1F1F"));
    AddLiquid(TEXT("Nitrogen Gas"), TEXT("ADADAD"));
    AddLiquid(TEXT("Geyser"), TEXT("00FAB3"));
}

FResourceVisual URNM_ResourceVisuals::GetResourceVisual(const FName& ResourceName) const
{
    if (const FResourceVisual* Found = ResourceVisualMap.Find(ResourceName))
    {
        return *Found;
    }

    UE_LOG(LogResourceNodeMarker, Warning,
        TEXT("RNM_ResourceVisuals: No visual found for '%s', using fallback"),
        *ResourceName.ToString());

    return FResourceVisual(); // fallback
}