#pragma once

#include <CMapData.h>

#include <unordered_map>
#include <vector>
#include "../../ExtraWindow/CNewTrigger/CNewTrigger.h"
#include "../../Miscs/Palettes.h"
#include "../../Helpers/FString.h"

struct TerrainGeneratorOverlay
{
    WORD Overlay;
    std::vector<int> AvailableOverlayData;
};

struct TerrainGeneratorGroup
{
    double Chance = 0.0;
    std::vector<FString> Items;
    std::vector<int> AvailableTiles;
    bool HasExtraIndex = false;
    std::vector<TerrainGeneratorOverlay> Overlays;
    std::vector<FString> OverlayItems;
};

struct BuildingPowers
{
    int TotalPower;
    int Output;
    int Drain;
};

struct OverlayTypeData
{
    bool Rock;
    bool Wall;
    bool TerrainRock;
    bool RailRoad;
    FString CustomPaletteName;
    RGBClass RadarColor;
};

struct TubeData
{
    MapCoord StartCoord;
    int StartFacing;
    MapCoord EndCoord;
    std::vector<int> Facings;
    std::vector<MapCoord> PathCoords;
    bool PositiveFacing;
    FString key;
};

struct ExtraImageInfo
{
    int TileIndex;
    int TileSubIndex;
    int AltType;

    bool operator==(const ExtraImageInfo& other) const {
        return TileIndex == other.TileIndex &&
            TileSubIndex == other.TileSubIndex &&
            AltType == other.AltType;
    }

    bool operator<(const ExtraImageInfo& other) const {
        if (TileIndex != other.TileIndex)
            return TileIndex < other.TileIndex;
        if (TileSubIndex != other.TileSubIndex)
            return TileSubIndex < other.TileSubIndex;
        return AltType < other.AltType;
    }
};

struct BuildingDataExt
{
    ~BuildingDataExt()
    {
        if (Foundations)
            delete Foundations;
        if (LinesToDraw)
            delete LinesToDraw;
    }

    bool IsCustomFoundation() const
    {
        return Foundations != nullptr;
    }

    int Width{ 0 };
    int Height{ 0 };
    std::vector<MapCoord>* Foundations{ nullptr };
    std::vector<std::pair<MapCoord, MapCoord>>* LinesToDraw{ nullptr };
};

struct BuildingRenderData
{
    unsigned int HouseColor;
    FString ID;
    short X;
    short Y;
    short Facing;
    short Strength;
    unsigned char PowerUpCount;
    FString PowerUp1;
    FString PowerUp2;
    FString PowerUp3;
    bool poweredOn;
};

struct LightingSource
{
    float CenterX;
    float CenterY;
    int LightVisibility;
    float LightIntensity;
    float LightRedTint;
    float LightGreenTint;
    float LightBlueTint;
};

struct LightingSourcePosition
{
    int X;
    int Y;
    FString BuildingType;
    bool operator==(const LightingSourcePosition& another) const
    {
        return
            X == another.X &&
            Y == another.Y &&
            BuildingType == another.BuildingType;
    }
};

struct BaseNodeDataExt
{
    int BuildingID;
    int BasenodeID;
    FString House;
    int X;
    int Y;
    FString ID;

    bool operator==(const BaseNodeDataExt& another) const
    {
        return
            BuildingID == another.BuildingID &&
            BasenodeID == another.BasenodeID &&
            X == another.X &&
            Y == another.Y &&
            House == another.House &&
            ID == another.ID;
    }

};

struct TileAnimation
{
    int TileIndex;
    int AttachedSubTile;
    int XOffset;
    int YOffset;
    int ZAdjust;
    FString AnimName;
    FString ImageName;
};

struct CellDataExt
{
    WORD X;
    WORD Y;

    WORD NewOverlay = 0xFFFF;

    // for preview
    bool AroundPlayerLocation = false;
    bool AroundHighBridge = false;

    // for locate cell
    bool drawCell = false;

    // for smooth water
    bool IsWater = false;
    bool Processed = false;

    // for raise ground
    bool Adjusted = false;
    bool CreateSlope = false;

    // for create shore
    bool ShoreProcessed = false;
    bool ShoreLATNeeded = false;

    // for terrain generation
    bool AddRandomTile = false;

    // for line tool
    bool LineToolProcessed = false;

    // for lighting preview
    LightingSourceTint Lighting = { 0.0f , 0.0f , 0.0f , 0.0f };

    std::vector<BaseNodeDataExt> BaseNodes;
    // first = index of StructureIndexMap, second = index in GetBuildingTypeID 
    std::unordered_map<short, short> Structures;

    // first = index, second = type 
    std::unordered_map<short, short> Terrains;
    std::unordered_map<short, short> Smudges;

    bool HasAnim = false;
    bool HasAnnotation = false;

    int RecordMinimapUpdateIndex[3] = { -1 } ;
};

class HistoryRecord {
public:
    virtual ~HistoryRecord() = default;
};

class TerrainRecord : public HistoryRecord {
public:
    int left;
    int top;
    int bottom;
    int right;

    std::unique_ptr<BOOL[]> bRedrawTerrain;
    std::unique_ptr<WORD[]> overlay;
    std::unique_ptr<BYTE[]> overlaydata;
    std::unique_ptr<WORD[]> wGround;
    std::unique_ptr<WORD[]> bMapData;
    std::unique_ptr<BYTE[]> bSubTile;
    std::unique_ptr<BYTE[]> bHeight;
    std::unique_ptr<BYTE[]> bMapData2;
    std::unique_ptr<BYTE[]> bRNDData;

    void record(int left, int top, int right, int bottom);
    void recover();
};

class ObjectRecord : public HistoryRecord {
public:
    enum RecordType : int
    {
        Building = 0x00000001,
        Unit = 0x00000002,
        Aircraft = 0x00000004,
        Infantry = 0x00000008,
        Terrain = 0x00000010,
        Smudge = 0x00000020,
        Basenode = 0x00000040,
        Tunnel = 0x00000080,
        Waypoint = 0x00000100,
        Celltag = 0x00000200,
        Annotation = 0x00000400
    };
    int recordFlags = 0;
    int recordedFlages = 0;
    std::vector<FString> BuildingList;
    std::vector<FString> UnitList;
    std::vector<FString> AircraftList;
    std::vector<FString> InfantryList;
    std::map<FString, FString> TerrainList;
    std::vector<FString> SmudgeList;
    std::map<FString, std::vector<FString>> BasenodeList;
    std::vector<FString> TunnelList;
    std::map<FString, FString> WaypointList;
    std::map<FString, FString> CelltagList;
    std::map<FString, FString> AnnotationList;

    void record(int recordType);
    void appendRecord(int recordType);
    void recover();

    static ObjectRecord* ObjectRecord_HoldingPtr;
};

class MixedRecord : public HistoryRecord {
public:
    TerrainRecord terrain;
    ObjectRecord object;

    void record(int left, int top, int right, int bottom, int recordType);
    void recover();
};

class HistoryList {
public:
    void add(std::unique_ptr<HistoryRecord> rec) {
        records.emplace_back(std::move(rec));
    }

    void add(int recordType) {
        auto data = std::make_unique<ObjectRecord>();
        data->record(recordType);
        records.emplace_back(std::move(data));
    }

    void add(int left, int top, int right, int bottom, int recordType) {
        auto data = std::make_unique<MixedRecord>();
        data->record(left, top, right, bottom, recordType);
        records.emplace_back(std::move(data));
    }

    void insert(size_t index, std::unique_ptr<HistoryRecord> rec) {
        if (index > records.size()) {
            index = records.size();
        }
        records.insert(records.begin() + index, std::move(rec));
    }

    void insert(size_t index, int recordType) {
        auto data = std::make_unique<ObjectRecord>();
        data->record(recordType);
        if (index > records.size()) {
            index = records.size();
        }
        records.insert(records.begin() + index, std::move(data));
    }

    void insert(size_t index, int left, int top, int right, int bottom, int recordType) {
        auto data = std::make_unique<MixedRecord>();
        data->record(left, top, right, bottom, recordType);
        if (index > records.size()) {
            index = records.size();
        }
        records.insert(records.begin() + index, std::move(data));
    }

    void erase(size_t index) {
        if (index < records.size()) {
            records.erase(records.begin() + index);
        }
    }

    void resize(size_t newSize) {
        records.resize(newSize);
    }

    void clear() {
        records.clear();
    }

    HistoryRecord* get(size_t index) {
        if (index >= records.size()) return nullptr;
        return records[index].get();
    }

    size_t size() const { return records.size(); }

private:
    std::vector<std::unique_ptr<HistoryRecord>> records;
};

class CMapDataExt : public CMapData
{
public:
    static CMapDataExt* GetExtension()
    {
        return reinterpret_cast<CMapDataExt*>(&CMapData::Instance());
    }
    enum OverlayCreditsType
    {
        OverlayCredits_Riparius = 0,
        OverlayCredits_Cruentus = 1,
        OverlayCredits_Vinifera = 2,
        OverlayCredits_Aboreus = 3,
        OverlayCredits_NumOf
    };

    void PackExt(bool UpdatePreview, bool Description);
    // just alter CellData size for lua.restore_snapshot
    bool ResizeMapExt(MapRect* const pRect);
    
    enum OreType { Riparius = 0, Cruentus, Vinifera, Aboreus };
    int GetOreValue(unsigned short nOverlay, unsigned char nOverlayData);
    int GetOreValueAt(CellData& cell);
    void InitOreValue();
    static bool IsOre(unsigned short nOverlay);

    bool IsTileIntact(int x, int y, int startX = -1, int startY = -1, int right = -1, int bottom = -1);
    std::vector<MapCoord> GetIntactTileCoords(int x, int y, bool oriIntact);
    LandType GetAltLandType(int tileIndex, int TileSubIndex);
    static inline LandType GetLandType(int tileIndex, int TileSubIndex)
    {
        if (tileIndex == 0xFFFF)
            tileIndex = 0;

        return CMapDataExt::TileData[tileIndex].TileBlockDatas[TileSubIndex].TerrainType;
    }
    void PlaceTileAt(int X, int Y, int index, int callType = -1);
    void SetHeightAt(int X, int Y, int height);

    //void InitializeBuildingTypesExt(const char* ID);
    static void InitializeAllHdmEdition(bool updateMinimap = true, bool reloadCellDataExt = true);
    static void UpdateTriggers();
    static FString AddTrigger(std::shared_ptr<Trigger> trigger);
    static FString AddTrigger(FString id);
    static std::shared_ptr<Trigger> GetTrigger(FString id);
    static void DeleteTrigger(FString id);
    static void CreateRandomGround(int TopX, int TopY, int BottomX, int BottomY, int scale, std::vector<std::pair<std::vector<int>, float>> tiles, bool override, bool multiSelection, bool onlyClear = false);
    static void CreateRandomOverlay(int TopX, int TopY, int BottomX, int BottomY, std::vector<std::pair<std::vector<TerrainGeneratorOverlay>, float>> overlays, bool override, bool multiSelection, bool onlyClear = false);
    static void CreateRandomTerrain(int TopX, int TopY, int BottomX, int BottomY, std::vector<std::pair<std::vector<FString>, float>> terrains, bool override, bool multiSelection, bool onlyClear = false);
    static void CreateRandomSmudge(int TopX, int TopY, int BottomX, int BottomY, std::vector<std::pair<std::vector<FString>, float>> smudges, bool override, bool multiSelection, bool onlyClear = false);

    static unsigned short CurrentRenderBuildingStrength;
    static std::unordered_map<int, BuildingDataExt> BuildingDataExts;
    static std::vector<BuildingRenderData> BuildingRenderDatasFix;
    static std::vector<OverlayTypeData> OverlayTypeDatas;
    static void UpdateFieldStructureData_Optimized();
    static void UpdateFieldStructureData_Index(int iniIndex, ppmfc::CString value = "");
    static void SmoothAll();
    static void SmoothTileAt(int X, int Y, bool gameLAT = false);
    static void CreateSlopeAt(int x, int y, bool IgnoreMorphable = false);
    static void SmoothWater();
    static BuildingPowers GetStructurePower(CBuildingData object);
    static BuildingPowers GetStructurePower(ppmfc::CString value);
    static void GetBuildingDataByIniID(int bldID, CBuildingData& data);
    static inline int GetSafeTileIndex(int idx)
    {
        if (idx == 0xFFFF)
            idx = 0;
        return idx;
    }
    // damageStage = -1 means read the target cell overlayData to determine
    static void PlaceWallAt(int dwPos, int overlay, int damageStage = -1, bool firstRun = true);
    static int GetInfantryAt(int dwPos, int dwSubPos = -1);
    static std::vector<int> GetStructureSize(ppmfc::CString structure);
    static ppmfc::CString GetFacing(MapCoord oldMapCoord, MapCoord newMapCoord, ppmfc::CString currentFacing, int numFacings = 8);
    static int GetFacing(MapCoord oldMapCoord, MapCoord newMapCoord, int numFacings = 8);
    static int GetFacing4(MapCoord oldMapCoord, MapCoord newMapCoord);
    static bool IsValidTileSet(int tileset);
    static ppmfc::CString GetAvailableIndex();
    static void UpdateMapSectionIndicies(const ppmfc::CString& lpSection);
    inline static bool HasAnnotation(int pos)
    {
        return CMapDataExt::CellDataExts[pos].HasAnnotation;
    }
    static void UpdateAnnotation();
    inline static bool IsCoordInFullMap(int X, int Y)
        {
            return X >= 0 && Y >= 0 &&
                X < CMapData::Instance->MapWidthPlusHeight &&
                Y < CMapData::Instance->MapWidthPlusHeight;
        };
    inline static bool IsCoordInFullMap(int CoordIndex)
    {
        return IsCoordInFullMap(CMapData::Instance->GetXFromCoordIndex(CoordIndex), CMapData::Instance->GetYFromCoordIndex(CoordIndex));
    }
    static CellData ExtTempCellData;
    inline static CellData* TryGetCellAt(int X, int Y)
    {
        if (IsCoordInFullMap(X, Y))
            return CMapData::Instance->GetCellAt(X, Y);
        else
        {
            ExtTempCellData.Infantry[0] = -1;
            ExtTempCellData.Infantry[1] = -1;
            ExtTempCellData.Infantry[2] = -1;
            ExtTempCellData.Unit = -1;
            ExtTempCellData.Aircraft = -1;
            ExtTempCellData.Structure = -1;
            ExtTempCellData.BaseNode.BasenodeID = -1;
            ExtTempCellData.BaseNode.BuildingID = -1;
            ExtTempCellData.Terrain = -1;
            ExtTempCellData.Smudge = -1;
            ExtTempCellData.Height = 0;
            ExtTempCellData.TileIndex = 0;
            ExtTempCellData.TileSubIndex = 0;
            ExtTempCellData.Flag.NotAValidCell = 1;
            return &ExtTempCellData;
        }
    }
    inline static CellData* TryGetCellAt(int nIndex)
    {
        TryGetCellAt(CMapData::Instance->GetXFromCoordIndex(nIndex), CMapData::Instance->GetYFromCoordIndex(nIndex));
    }

    std::string convertToExtendedOverlayPack(const std::string& input);
    std::string convertFromExtendedOverlayPack(const std::string& input);

    void SetNewOverlayAt(int x, int y, WORD ovr);
    void SetNewOverlayAt(int pos, WORD ovr);
    WORD GetOverlayAt(int x, int y);
    WORD GetOverlayAt(int pos);
    static OverlayTypeData GetOverlayTypeData(WORD index);
    static void AssignCellData(CellData& dst, const CellData& src);
    std::unique_ptr<TerrainRecord> MakeTerrainRecord(int left, int top, int right, int bottom);
    static ObjectRecord* MakeObjectRecord(int recordType, bool recordOnce = false);
    static void MakeMixedRecord(int left, int top, int right, int bottom, int recordType);

    static void UpdateFieldStructureData_RedrawMinimap();
    static void UpdateFieldUnitData_RedrawMinimap();
    static void UpdateFieldInfantryData_RedrawMinimap();
    static void UpdateFieldAircraftData_RedrawMinimap();

    static int OreValue[4];
    static std::vector<std::vector<int>> Tile_to_lat;
    static std::vector<int> TileSet_starts;

    static CellDataExt CellDataExt_FindCell;
    static std::vector<CellDataExt> CellDataExts;
    //static MapCoord CurrentMapCoord;
    static MapCoord CurrentMapCoordPaste;

    static CTileTypeClass* TileData;
    static int TileDataCount;
    static int CurrentTheaterIndex;

    static int PaveTile;
    static int GreenTile;
    static int MiscPaveTile;
    static int Medians;
    static int PavedRoads;
    static int ShorePieces;
    static int WaterBridge;
    static int BridgeSet;
    static int WoodBridgeSet;
    static int HeightBase;
    static Palette Palette_ISO;
    static Palette Palette_ISO_NoTint;
    static Palette Palette_Shadow;
    static Palette Palette_AlphaImage;
    static std::vector<std::pair<LightingSourcePosition, LightingSource>> LightingSources;
    static int AutoShore_ShoreTileSet;
    static int AutoShore_GreenTileSet;
    static std::unordered_set<int> ShoreTileSets;
    static std::unordered_map<int, bool> SoftTileSets; // soft = affected by shore logic
    static FString BitmapImporterTheater;
    static float ConditionYellow;
    static float ConditionRed;
    static bool DeleteBuildingByIniID;
    static std::unordered_map<FString, std::shared_ptr<Trigger>> Triggers;
    static std::vector<short> StructureIndexMap;
    static std::vector<TubeData> Tubes;
    static std::unordered_map<int, TileAnimation> TileAnimations;
    // 0 = tem, 1 = sno, 2 = urban, 3 = newurban, 4 = lunar, 5 = desert
    static std::unordered_map<int, FString> TileSetOriginSetNames[6];
    static std::unordered_set<FString> TerrainPaletteBuildings;
    static std::unordered_set<FString> DamagedAsRubbleBuildings;
    static std::unordered_set<int> RedrawExtraTileSets;
    static std::unordered_map<int, Palette*> TileSetPalettes;
    static int NewINIFormat;
    static WORD NewOverlay[0x40000];
    static HistoryList UndoRedoDatas;
    static int UndoRedoDataIndex;
    static bool IsLoadingMapFile;
    static std::vector<FString> MapIniSectionSorting;
};