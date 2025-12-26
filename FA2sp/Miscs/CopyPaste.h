#pragma once

#include <CMapData.h>
#include "..\Ext\CMapData\Body.h"
#include <set>
#include <concepts>

#pragma pack(push, 1)
struct StringField {
    uint32_t Offset;
    uint16_t Length;
};

struct MyClipboardData
{
    int X;
    int Y;
    unsigned short Overlay;
    unsigned char OverlayData;
    unsigned short TileIndex;
    unsigned short TileIndexHiPart;
    unsigned char TileSubIndex;
    unsigned char Height;
    unsigned char IceGrowth;
    unsigned short TileSet;
    unsigned short TileSetSubIndex;
    CellData::CellDataFlag Flag;

    StringField TerrainData;
    StringField SmudgeData;
    StringField BuildingData;
    StringField AircraftData;
    StringField InfantryData_1;
    StringField InfantryData_2;
    StringField InfantryData_3;
    StringField UnitData;
};
#pragma pack(pop)

struct TileRule {
    std::vector<int> sourceTiles;
    std::vector<int> destinationTiles;
    bool isRandom = false;
    bool hasHeightOverride = false;
    int heightOverride = -1; 
    bool hasSubIndexOverride = false;
    int subIndexOverride = -1; 
    bool reverse = false;
};

class CopyPaste
{
public:
    static std::set<MapCoord> PastedCoords;
    static bool CopyWholeMap;
    static bool OnLButtonDownPasted;
    static bool IsCutting;
    static std::vector<TileRule> TileConvertRules;
    static void Copy(const std::set<MapCoord>& coords);
    static void Paste(int X, int Y, int nBaseHeight, MyClipboardData* data, size_t length, int recordType,
        std::set<MapCoord>* validCoords = nullptr);
    static void PasteArea(int X, int Y, int nBaseHeight, MyClipboardData* data, size_t length, int recordType);
    static void LoadTileConvertRule(char sourceTheater);
    static void ConvertTile(CellData& cell);
private:
    static const char* GetString(const MyClipboardData& cell, const StringField& field, MyClipboardData* pBufferBase);
};