#pragma once

#include "../FA2sp.h"

#include <CPalette.h>
#include "../Ext/CIsoView/Body.h"
#include <map>
#include <list>

// References from ccmaps-net
// In fact the lighting should just be integers (from YR)
// R = Lighting.R * 100.0 + 0.1
// G = Lighting.G * 100.0 + 0.1
// B = Lighting.B * 100.0 + 0.1
// Ambient = Lighting.Ambient * 100.0 + 0.1
// Ground = Lighting.Ground * 1000.0 + 0.1
// Level = Lighting.Level * 1000.0 + 0.1
// I just choose to take a reference from ccmaps-net code - secsome

struct LightingSourceTint
{
    float RedTint;
    float GreenTint;
    float BlueTint;
    float AmbientTint;
public:
    static LightingSourceTint ApplyLamp(int X, int Y);
    static void CalculateMapLamps();
    static bool IsLamp(ppmfc::CString ID);

};

struct LightingKey
{
    const Palette* origin;

    uint16_t rmult;
    uint16_t gmult;
    uint16_t bmult;

    uint8_t isObject;

    uint8_t colorR;
    uint8_t colorG;
    uint8_t colorB;

    bool operator==(const LightingKey& o) const noexcept
    {
        return origin == o.origin &&
            rmult == o.rmult &&
            gmult == o.gmult &&
            bmult == o.bmult &&
            isObject == o.isObject &&
            colorR == o.colorR &&
            colorG == o.colorG &&
            colorB == o.colorB;
    }
};

struct LightingKeyHash
{
    size_t operator()(const LightingKey& k) const noexcept
    {
        size_t h = 0;

        auto combine = [](size_t& seed, size_t v)
        {
            seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };

        combine(h, std::hash<const void*>()(k.origin));
        combine(h, k.rmult);
        combine(h, k.gmult);
        combine(h, k.bmult);
        combine(h, k.isObject);
        combine(h, k.colorR);
        combine(h, k.colorG);
        combine(h, k.colorB);

        return h;
    }
};

struct LightingStruct
{
    float Red;
    float Green;
    float Blue;
    float Ground;
    float Ambient;
    float Level;

    bool operator==(const LightingStruct& another) const
    {
        return
            Red == another.Red &&
            Green == another.Green &&
            Blue == another.Blue &&
            Ground == another.Ground &&
            Ambient == another.Ambient &&
            Level == another.Level;
    }
    bool operator!=(const LightingStruct& another) const
    {
        return !(*this == another);
    }
    bool operator<(const LightingStruct& another) const
    {
        return
            std::tie(Red, Green, Blue, Ground, Ambient, Level) <
            std::tie(another.Red, another.Green, another.Blue, another.Ground, another.Ambient, another.Level);
    }
public:
    static LightingStruct GetCurrentLighting();
    static LightingStruct CurrentLighting;

    static const LightingStruct NoLighting;
};

class PaletteCache
{
public:
    using CacheMap = std::unordered_map<LightingKey, Palette, LightingKeyHash>;

    static CacheMap Cache;

    static Palette* GetOrCreate(
        Palette* origin, Palette* remapped,
        float rMult, float gMult, float bMult, float ambient,
        bool isObject);
};

class LightingPalette
{
private:
    Palette* OriginPalette;
public:
    float RedMult;
    float GreenMult;
    float BlueMult;
    float AmbientMult;
    Palette Colors;
    LightingPalette(Palette& originPal);
    // objectType : -1 = others, 0 = unit, 1 = inf, 2 = air, 3 = building, 
    // 4 = building rubble & TerrainPalette building, 5 = custom-palette terrains
    // 6 = tiberium tree
    void AdjustLighting(LightingStruct& lighting, Cell3DLocation location = { 0 }, bool tint = true, int objectType = -1);
    void ResetColors();
    void RemapColors(BGRStruct color);
    void TintColors(bool isObject = false);
    Palette* GetPalette();
    
};

class PalettesManager
{
    static FMap<Palette*> OriginPaletteFiles;
    static std::map<Palette*, std::map<std::pair<BGRStruct, LightingStruct>, LightingPalette>> CalculatedPaletteFiles;
    static std::map<Palette*, std::map<std::pair<BGRStruct, LightingStruct>, LightingPalette>> CalculatedDimmedPaletteFiles;
    static std::map<Palette*, std::map<LightingStruct, LightingPalette>> CalculatedPaletteFilesNoRemap;
    static Palette* CurrentIso;

public:
    static void Release();

    static bool NeedReloadLighting;
    static std::list<LightingPalette> CalculatedObjectPaletteFiles;
    static std::vector<Palette*> CalculatedMixedPalettes;

    static Palette* GetCurrentIso();
    static Palette* LoadPalette(FString palname);
    static Palette* LoadTiberiumCellAnimPalette(BGRStruct& color, FString palname);
    static Palette* GetPalette(Palette* pPal, BGRStruct& color, bool remap = true, Cell3DLocation location = {0});
    static Palette* GetTileSetBrowserViewPalette(Palette* pPal, BGRStruct& color, bool remap = true, Cell3DLocation location = {0});
    static Palette* GetObjectPalette(Palette* pPal, BGRStruct& color, bool remap, Cell3DLocation location, bool isopal = false, int extraLightType = -1);
    static Palette* GetOverlayPalette(Palette* pPal, Cell3DLocation location, int overlay);
};


