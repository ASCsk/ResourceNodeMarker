#include "RNM_ResourceVisuals.h"

#include "FGResourceDescriptor.h"
#include "FGItemDescriptor.h"


URNM_ResourceVisuals::URNM_ResourceVisuals()
{
    FIconPreset Icons;

    auto MakeColor = [](const FString& Hex)
        {
            return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
        };

    auto AddSolid = [&](const FString& Name, const FString& Impure, const FString& Normal, const FString& Pure)
        {
            FResourceVisual Visual;
            Visual.IconID = Icons.Rock;
            Visual.ImpureColor = MakeColor(Impure);
            Visual.NormalColor = MakeColor(Normal);
            Visual.PureColor = MakeColor(Pure);

            ResourceVisualMap.Add(Name, Visual);
        };

    auto AddLiquid = [&](const FString& Name, const FString& Impure, const FString& Normal, const FString& Pure)
        {
            FResourceVisual Visual;
            Visual.IconID = Icons.Fluids;
            Visual.ImpureColor = MakeColor(Impure);
            Visual.NormalColor = MakeColor(Normal);
            Visual.PureColor = MakeColor(Pure);

            ResourceVisualMap.Add(Name, Visual);
        };

    /*
    ROCK RESOURCES
    */

    AddSolid(TEXT("Copper Ore"), "B34E00", "D16619", "F28233"); 
    AddSolid(TEXT("Iron Ore"), "5E626B", "93959E", "C4C4C4"); 
    AddSolid(TEXT("Limestone"), "A68F53", "D1B97B", "FFEAB5");
    AddSolid(TEXT("Caterium Ore"), "EBBC09", "FFD324", "FFDB4D");
    AddSolid(TEXT("Coal"), "1F1B1B", "403B3B", "5E5E5E");
    AddSolid(TEXT("Sulfur"), "C9C900", "E6E615", "FFFF69");
    AddSolid(TEXT("Bauxite"), "FFA636", "FFB65E", "FFC785");
    AddSolid(TEXT("Raw Quartz"), "FEB5FF", "FED4FF", "FFE5FF");
    AddSolid(TEXT("Uranium"), "5BD900", "85FF2E", "B2FF78");
    AddSolid(TEXT("SAM Ore"), "4F0063", "A332C9", "C258E8");

    /*
    FLUID RESOURCES
    */

    AddLiquid(TEXT("Water"), "00CCB8", "17E3CE", "4FFFEC");
    AddLiquid(TEXT("Crude Oil"), "101010", "1F1F1F", "474747");
    AddLiquid(TEXT("Nitrogen"), "828282", "A6A6A6", "D4D2D2");
}

FResourceVisual URNM_ResourceVisuals::GetResourceVisual(const FString& ResourceName) const
{
    if (const FResourceVisual* Found = ResourceVisualMap.Find(ResourceName))
    {
        return *Found;
    }

    return FResourceVisual(); // fallback
}