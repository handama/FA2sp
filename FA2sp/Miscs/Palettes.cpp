#include "Palettes.h"

#include <CPalette.h>
#include <Drawing.h>
#include <CMixFile.h>
#include <CLoading.h>

#include <algorithm>

#include "../Ext/CFinalSunDlg/Body.h"
#include "../Ext/CMapData/Body.h"
#include "MultiSelection.h"

const LightingStruct LightingStruct::NoLighting = { -1,-1,-1,-1,-1,-1 };
LightingStruct LightingStruct::CurrentLighting;

std::map<FString, Palette*> PalettesManager::OriginPaletteFiles;
std::map<Palette*, std::map<std::pair<BGRStruct, LightingStruct>, LightingPalette>> PalettesManager::CalculatedPaletteFiles;
std::map<Palette*, std::map<std::pair<BGRStruct, LightingStruct>, LightingPalette>> PalettesManager::CalculatedDimmedPaletteFiles;
std::map<Palette*, std::map<LightingStruct, LightingPalette>> PalettesManager::CalculatedPaletteFilesNoRemap;
std::vector<LightingPalette> PalettesManager::CalculatedObjectPaletteFiles;
Palette* PalettesManager::CurrentIso;
bool PalettesManager::NeedReloadLighting = false;

void PalettesManager::Release()
{
    Logger::Debug("PalettesManager: Releasing palettes.\n");
    for (auto& pair : PalettesManager::OriginPaletteFiles)
        if (pair.second && pair.second != Palette::PALETTE_UNIT &&
            pair.second != Palette::PALETTE_ISO &&
            pair.second != Palette::PALETTE_THEATER &&
            pair.second != Palette::PALETTE_LIB)
            GameDelete(pair.second);

    PalettesManager::OriginPaletteFiles.clear();
    PalettesManager::CalculatedPaletteFiles.clear();
    PalettesManager::CalculatedPaletteFilesNoRemap.clear();
    PalettesManager::CalculatedDimmedPaletteFiles.clear();
    PalettesManager::CalculatedObjectPaletteFiles.clear();
    PalettesManager::NeedReloadLighting = true;
}

Palette* PalettesManager::GetCurrentIso()
{
    return PalettesManager::CurrentIso;
}

Palette* PalettesManager::LoadPalette(FString palname)
{
    auto itr = PalettesManager::OriginPaletteFiles.find(palname);
    if (itr != PalettesManager::OriginPaletteFiles.end())
        return itr->second;
    
    auto palToLoad = palname;
    palToLoad.Replace("iso\233NotAutoTinted", "iso");
    palToLoad.Replace("iso\233AutoTinted", "iso");

    if (auto pBuffer = (BytePalette*)CLoading::Instance->ReadWholeFile(palToLoad))
    {
        auto pPalette = GameCreate<Palette>();
        for (int i = 0; i < 256; ++i)
        {
            pPalette->Data[i].R = pBuffer->Data[i].red << 2;
            pPalette->Data[i].G = pBuffer->Data[i].green << 2;
            pPalette->Data[i].B = pBuffer->Data[i].blue << 2;
        }
        GameDelete(pBuffer);
        PalettesManager::OriginPaletteFiles[palname] = pPalette;
        return pPalette;
    }

    return nullptr;
}

Palette* PalettesManager::GetPalette(Palette* pPal, BGRStruct& color, bool remap, Cell3DLocation location)
{
    if (remap)
    {
        auto itr = PalettesManager::CalculatedPaletteFiles[pPal].find(std::make_pair(color, LightingStruct::CurrentLighting));
        if (itr != PalettesManager::CalculatedPaletteFiles[pPal].end())
            return itr->second.GetPalette();
    }
    else
    {
        auto itr = PalettesManager::CalculatedPaletteFilesNoRemap[pPal].find(LightingStruct::CurrentLighting);
        if (itr != PalettesManager::CalculatedPaletteFilesNoRemap[pPal].end())
            return itr->second.GetPalette();
    }


    auto& p = remap ? PalettesManager::CalculatedPaletteFiles[pPal].emplace(
        std::make_pair(std::make_pair(color, LightingStruct::CurrentLighting), LightingPalette(*pPal))
    ).first->second : PalettesManager::CalculatedPaletteFilesNoRemap[pPal].emplace(
        std::make_pair(LightingStruct::CurrentLighting, LightingPalette(*pPal))
    ).first->second;
    if (remap)
        p.RemapColors(color);
    else
        p.ResetColors();

    if (LightingStruct::CurrentLighting != LightingStruct::NoLighting)
    {
        p.AdjustLighting(LightingStruct::CurrentLighting, location);
        p.TintColors();
    }
    return p.GetPalette();

}

Palette* PalettesManager::GetTileSetBrowserViewPalette(Palette* pPal, BGRStruct& color, bool remap, Cell3DLocation location)
{
    if (ExtConfigs::EnableDarkMode && ExtConfigs::EnableDarkMode_DimMap)
    {
        auto pal = ExtConfigs::LightingPreview_TintTileSetBrowserView ? GetPalette(pPal, color, remap, location) : pPal;
        auto itr = PalettesManager::CalculatedDimmedPaletteFiles[pal].find(std::make_pair(color, LightingStruct::CurrentLighting));
        if (itr != PalettesManager::CalculatedDimmedPaletteFiles[pal].end())
            return itr->second.GetPalette();

        auto& p = PalettesManager::CalculatedDimmedPaletteFiles[pal].emplace(
            std::make_pair(std::make_pair(color, LightingStruct::CurrentLighting), LightingPalette(*pal))
        ).first->second;
        p.ResetColors();
        p.AmbientMult = 0.7f;
        p.RedMult = 1.0f;
        p.GreenMult = 1.0f;
        p.BlueMult = 1.0f;
        p.TintColors();
        return p.GetPalette();
    }

    return ExtConfigs::LightingPreview_TintTileSetBrowserView ? GetPalette(pPal, color, remap, location) : pPal; 
}

Palette* PalettesManager::GetObjectPalette(Palette* pPal, BGRStruct& color, bool remap, Cell3DLocation location, bool isopal, int extraLightType)
{
    auto& p = PalettesManager::CalculatedObjectPaletteFiles.emplace_back(LightingPalette(*pPal));
    if (remap)
        p.RemapColors(color);
    else
        p.ResetColors();

    bool tintRGB = true;
    // normal lighting won't tint unit RGB
    if (remap && !ExtConfigs::LightingPreview_MultUnitColor && CFinalSunDlgExt::CurrentLighting == 31001 && extraLightType != 4)
        tintRGB = false;

    if (LightingStruct::CurrentLighting != LightingStruct::NoLighting)
    {
        if (!isopal)
        {
            p.AdjustLighting(LightingStruct::CurrentLighting, location, tintRGB, extraLightType);
            p.TintColors();
        }
        else // isopal already tinted, so only apply level color
        {
            auto& ret = LightingStruct::CurrentLighting;
            for (int i = 0; i < 256; ++i)
            {
                float ambLevel = ret.Ambient - ret.Ground + ret.Level * location.Height;
                ambLevel = std::clamp(ambLevel, 0.0f, 2.0f);
                float amb = ret.Ambient - ret.Ground;
                amb = std::clamp(amb, 0.0f, 2.0f);
                p.Colors[i].R = (unsigned char)std::min(p.Colors[i].R / amb * ambLevel, 255.0f);
                p.Colors[i].G = (unsigned char)std::min(p.Colors[i].G / amb * ambLevel, 255.0f);
                p.Colors[i].B = (unsigned char)std::min(p.Colors[i].B / amb * ambLevel, 255.0f);
            }
        }
    }
    return p.GetPalette();
}

Palette* PalettesManager::GetOverlayPalette(Palette* pPal, Cell3DLocation location, int overlay)
{
    auto& p = PalettesManager::CalculatedObjectPaletteFiles.emplace_back(LightingPalette(*pPal));
    p.ResetColors();

    auto& ret = LightingStruct::CurrentLighting;
    auto safeColorBtye = [](int x)
        {
            if (x > 255)
                x = 255;
            if (x < 0)
                x = 0;
            return (byte)x;
        };

    if (LightingStruct::CurrentLighting != LightingStruct::NoLighting)
    {
        const auto overlay = CMapDataExt::GetExtension()->GetOverlayAt(
            CMapData::Instance->GetCoordIndex(
                CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y));
        if (!CMapDataExt::IsOre(overlay))
        {
            p.AdjustLighting(LightingStruct::CurrentLighting, location);
            p.TintColors();
        }   
    }
    if (MultiSelection::IsSelected(CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y))
    {
        for (int i = 0; i < 256; ++i)
        {
            p.Colors[i].R = (p.Colors[i].R * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->R) / 3;
            p.Colors[i].G = (p.Colors[i].G * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->G) / 3;
            p.Colors[i].B = (p.Colors[i].B * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->B) / 3;
        }
    }

    return p.GetPalette();
}

LightingStruct LightingStruct::GetCurrentLighting()
{
    switch (CFinalSunDlgExt::CurrentLighting)
    {
    case 31001:
        CurrentLighting.Red = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "Red", 1.0));
        CurrentLighting.Green = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "Green", 1.0));
        CurrentLighting.Blue = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "Blue", 0.5));
        CurrentLighting.Ambient = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "Ambient", 1.0));
        CurrentLighting.Ground = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "Ground", 0.008));
        CurrentLighting.Level = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "Level", 0.087));
        return CurrentLighting;
    case 31002:
        CurrentLighting.Red = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "IonRed", 1.0));
        CurrentLighting.Green = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "IonGreen", 1.0));
        CurrentLighting.Blue = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "IonBlue", 0.5));
        CurrentLighting.Ambient = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "IonAmbient", 1.0));
        CurrentLighting.Ground = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "IonGround", 0.008));
        CurrentLighting.Level = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "IonLevel", 0.087));
        return CurrentLighting;
    case 31003:
        CurrentLighting.Red = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "DominatorRed", 1.0));
        CurrentLighting.Green = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "DominatorGreen", 1.0));
        CurrentLighting.Blue = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "DominatorBlue", 0.5));
        CurrentLighting.Ambient = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "DominatorAmbient", 1.0));
        CurrentLighting.Ground = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "DominatorGround", 0.008));
        CurrentLighting.Level = static_cast<float>(CINI::CurrentDocument->GetDouble("Lighting", "DominatorLevel", 0.087));
        return CurrentLighting;
    }
    CurrentLighting = LightingStruct::NoLighting;
    // 31000
    return CurrentLighting;
}

LightingPalette::LightingPalette(Palette& originPal)
{
    this->OriginPalette = &originPal;
    this->ResetColors();
}

void LightingPalette::AdjustLighting(LightingStruct& lighting, Cell3DLocation location, bool tint, int extraLightType)
{
    const auto lamp = LightingSourceTint::ApplyLamp(location.X, location.Y);

    this->AmbientMult = lighting.Ambient + lamp.AmbientTint - lighting.Ground + lighting.Level * location.Height;
    if (extraLightType == 0)
    {
        this->AmbientMult += Variables::RulesMap.GetSingle("AudioVisual", "ExtraUnitLight", 0.2f);
    }
    else if (extraLightType == 1)
    {
        this->AmbientMult += Variables::RulesMap.GetSingle("AudioVisual", "ExtraInfantryLight", 0.2f);
    }
    else if (extraLightType == 2)
    {
        this->AmbientMult += Variables::RulesMap.GetSingle("AudioVisual", "ExtraAircraftLight", 0.2f);
    }

    if (tint)
    {
        if (extraLightType >= 0 && extraLightType != 4)
        {
            // lamp won't tint unit RGB
            this->RedMult = lighting.Red;
            this->GreenMult = lighting.Green;
            this->BlueMult = lighting.Blue;
        }
        else
        {
            this->RedMult = lighting.Red + lamp.RedTint;
            this->GreenMult = lighting.Green + lamp.GreenTint;
            this->BlueMult = lighting.Blue + lamp.BlueTint;
        }
    }
    else
    {
        this->RedMult = 1.0f;
        this->GreenMult = 1.0f;
        this->BlueMult = 1.0f;
    }
}

void LightingPalette::ResetColors()
{
    this->Colors = *this->OriginPalette;
}

void LightingPalette::RemapColors(BGRStruct color)
{
    this->ResetColors();
    for (int i = 16; i <= 31; ++i)
    {
        int ii = i - 16;
        double cosval = ii * 0.08144869842640204 + 0.3490658503988659;
        double sinval = ii * 0.04654211338651545 + 0.8726646259971648;
        if (!ii)
            cosval = 0.1963495408493621;

        RGBClass rgb_remap{ color.R,color.G,color.B };
        HSVClass hsv_remap = rgb_remap;
        hsv_remap.H = hsv_remap.H;
        hsv_remap.S = (unsigned char)(std::sin(sinval) * hsv_remap.S);
        hsv_remap.V = (unsigned char)(std::cos(cosval) * hsv_remap.V);
        RGBClass result = hsv_remap;

        this->Colors[i] = { result.B,result.G,result.R };
    }
}

void LightingPalette::TintColors(bool isObject)
{
    this->RedMult = std::clamp(this->RedMult, 0.0f, 2.0f);
    this->GreenMult = std::clamp(this->GreenMult, 0.0f, 2.0f);
    this->BlueMult = std::clamp(this->BlueMult, 0.0f, 2.0f);
    this->AmbientMult = std::clamp(this->AmbientMult, 0.0f, 2.0f);

    auto rmult = this->AmbientMult * this->RedMult;
    auto gmult = this->AmbientMult * this->GreenMult;
    auto bmult = this->AmbientMult * this->BlueMult;

    for (int i = 0; i < 240; ++i)
    {
        this->Colors[i].R = (unsigned char)std::min(this->Colors[i].R * rmult, 255.0f);
        this->Colors[i].G = (unsigned char)std::min(this->Colors[i].G * gmult, 255.0f);
        this->Colors[i].B = (unsigned char)std::min(this->Colors[i].B * bmult, 255.0f);
    }
    if (!isObject)
    {
        for (int i = 240; i < 255; ++i)
        {
            this->Colors[i].R = (unsigned char)std::min(this->Colors[i].R * rmult, 255.0f);
            this->Colors[i].G = (unsigned char)std::min(this->Colors[i].G * gmult, 255.0f);
            this->Colors[i].B = (unsigned char)std::min(this->Colors[i].B * bmult, 255.0f);
        }
    }
    this->Colors[255].R = (unsigned char)std::min(this->Colors[255].R * rmult, 255.0f);
    this->Colors[255].G = (unsigned char)std::min(this->Colors[255].G * gmult, 255.0f);
    this->Colors[255].B = (unsigned char)std::min(this->Colors[255].B * bmult, 255.0f);
}

Palette* LightingPalette::GetPalette()
{
    return &this->Colors;
}

LightingSourceTint LightingSourceTint::ApplyLamp(int X, int Y)
{
    if (CMapDataExt::Instance->IsCoordInMap(X, Y))
        return CMapDataExt::CellDataExts[CMapData::Instance->GetCoordIndex(X, Y)].Lighting;
    return { 0.0f, 0.0f, 0.0f, 0.0f };
}

void LightingSourceTint::CalculateMapLamps()
{
    if (LightingStruct::CurrentLighting == LightingStruct::NoLighting)
        return;

    CMapDataExt::LightingSources.clear();
    const float TOLERANCE = 0.001f;
    if (auto pSection = CINI::CurrentDocument->GetSection("Structures"))
    {
        for (const auto& [key,value] : pSection->GetEntities())
        {
            const auto atoms = STDHelpers::SplitString(value, 9);
            // powered on
            if (atoms[9] != "0")
            {
                const auto& ID = atoms[1];
                LightingSource ls{};
                ls.LightIntensity = Variables::RulesMap.GetSingle(ID, "LightIntensity", 0.0f);
                if (abs(ls.LightIntensity) > TOLERANCE)
                {
                    ls.LightVisibility = Variables::RulesMap.GetInteger(ID, "LightVisibility", 5000);
                    ls.LightRedTint = Variables::RulesMap.GetSingle(ID, "LightRedTint", 1.0f);
                    ls.LightGreenTint = Variables::RulesMap.GetSingle(ID, "LightGreenTint", 1.0f);
                    ls.LightBlueTint = Variables::RulesMap.GetSingle(ID, "LightBlueTint", 1.0f);
                    const int Index = CMapData::Instance->GetBuildingTypeID(ID);
                    const int Y = atoi(atoms[3]);
                    const int X = atoi(atoms[4]);
                    const auto& DataExt = CMapDataExt::BuildingDataExts[Index];

                    ls.CenterX = X + DataExt.Height / 2.0f;
                    ls.CenterY = Y + DataExt.Width / 2.0f;
                    LightingSourcePosition lsp;
                    lsp.X = X;
                    lsp.Y = Y;
                    lsp.BuildingType = ID;
                    CMapDataExt::LightingSources.push_back(std::make_pair(lsp, ls));
                }
            }
        }
    }
    for (int x = 0; x < CMapData::Instance->MapWidthPlusHeight; ++x)
    {
        for (int y = 0; y < CMapData::Instance->MapWidthPlusHeight; ++y)
        {
            auto& ret = CMapDataExt::CellDataExts[CMapData::Instance->GetCoordIndex(x, y)].Lighting;
            ret.RedTint = 0.0f;
            ret.GreenTint = 0.0f;
            ret.BlueTint = 0.0f;
            ret.AmbientTint = 0.0f;
            for (const auto& [idx, ls] : CMapDataExt::LightingSources)
            {
                float sqX = (x - ls.CenterX) * (x - ls.CenterX);
                float sqY = (y - ls.CenterY) * (y - ls.CenterY);
                float distanceSquare = sqX + sqY;

                if ((0 < ls.LightVisibility) && (distanceSquare < ls.LightVisibility * ls.LightVisibility / 65536))
                {
                    float lsEffect = (ls.LightVisibility - 256 * sqrt(distanceSquare)) / ls.LightVisibility;
                    switch (CFinalSunDlgExt::CurrentLighting)
                    {
                        // color doesn't apply in superweapons
                    case 31002:
                    case 31003:
                    {
                        float maxTint = 0.0f;
                        if (ls.LightRedTint > maxTint) maxTint = ls.LightRedTint;
                        if (ls.LightBlueTint > maxTint) maxTint = ls.LightBlueTint;
                        if (ls.LightGreenTint > maxTint) maxTint = ls.LightGreenTint;
                        ret.AmbientTint += lsEffect * (maxTint * 0.5 + ls.LightIntensity);
                        break;
                    }
                    default:
                        ret.AmbientTint += lsEffect * ls.LightIntensity;
                        ret.RedTint += lsEffect * ls.LightRedTint;
                        ret.BlueTint += lsEffect * ls.LightBlueTint;
                        ret.GreenTint += lsEffect * ls.LightGreenTint;
                        break;
                    }
                }
            }
        }
    }
}

bool LightingSourceTint::IsLamp(ppmfc::CString ID)
{
    const float TOLERANCE = 0.001f;
    return abs(Variables::RulesMap.GetSingle(ID, "LightIntensity", 0.0f)) > TOLERANCE;
}