#pragma once

#include <CFinalSunDlg.h>
#include "../FA2Expand.h"

#include <array>
#include <set>
#include <map>
#include <memory>

#include <CPropertyBuilding.h>
#include <CPropertyAircraft.h>
#include <CPropertyInfantry.h>
#include <CPropertyUnit.h>

#include <CObjectDatas.h>
#include <CMapData.h>
#include <unordered_set>
#include <unordered_map>

class NOVTABLE CFinalSunDlgExt : CFinalSunDlg
{
public:
    static void ProgramStartupInit();

    static int CurrentLighting;
    static int SearchObjectType;
    static std::pair<FString, int> SearchObjectIndex;

    static bool CheckProperty_Vehicle(CUnitData data);
    static bool CheckProperty_Aircraft(CAircraftData data);
    static bool CheckProperty_Building(CBuildingData data);
    static bool CheckProperty_Infantry(CInfantryData data);


    BOOL PreTranslateMessageExt(MSG* pMsg);
    BOOL OnCommandExt(WPARAM wParam, LPARAM lParam);
};

class ConnectedTiles
{
    public:
        MapCoord ConnectionPoint0; // in its image
        MapCoord ConnectionPoint1;
        bool Side0; //0 for front, 1 for back
        bool Side1; //0 for front, 1 for back
        int Direction0; // 0 for north (top-right), and clockwise
        int Direction1; // 0 for north (top-right), and clockwise

        int AdditionalOffset;
        int HeightAdjust;
        std::vector<int> TileIndices;
        int Index;

        bool Opposite;

        bool GetNextSide(bool opposite)
        {
            if (opposite)
                return Side0;
            else
                return Side1;
        }
        bool GetNextSide()
        {
            if (Opposite)
                return Side0;
            else
                return Side1;
        }
        int GetNextDirection(bool opposite)
        {
            if (opposite)
                return Direction0;
            else
                return Direction1;
        }
        int GetThisDirection(bool opposite)
        {
            if (opposite)
                return Direction1;
            else
                return Direction0;
        }
        int GetNextDirection()
        {
            if (Opposite)
                return Direction0;
            else
                return Direction1;
        }
        MapCoord GetNextConnectionPoint(bool opposite)
        {
            if (opposite)
                return ConnectionPoint0;
            else
                return ConnectionPoint1;
        }
        int GetNextHeightOffset(bool opposite)
        {
            if (opposite)
                return -HeightAdjust;
            else
                return HeightAdjust;
        }
        int GetNextHeightOffset()
        {
            if (Opposite)
                return -HeightAdjust;
            else
                return HeightAdjust;
        }
};
class ConnectedTileSet
{
public:
    int StartTile;
    bool Allowed;
    std::vector<ConnectedTiles> ConnectedTile;
    FString Name;
    FString SetName;
    int Type;
    int SpecialType;
    bool WaterCliff;
    bool IsTXCityCliff;
    int ToSetPress[10];
};
struct ConnectedTileInfo
{
    int Index;
    bool Front;
};
struct MoveBaseNode
{
    FString House;
    FString Key;
    FString ID;
    int X = -1;
    int Y = -1;
};

class CViewObjectsExt : public CViewObjects
{
public:
    enum {
        Root_Nothing = 0, Root_Ground, Root_Owner, Root_Infantry, Root_Vehicle,
        Root_Aircraft, Root_Building, Root_Terrain, Root_Smudge, Root_Overlay,
        Root_Waypoint, Root_Celltag, Root_Basenode, Root_Tunnel, Root_PlayerLocation,
        Root_PropertyBrush, Root_Annotation, Root_InfantrySubCell, Root_View,
        Root_MultiSelection, Root_Cliff, Root_Delete, Root_Count
    };
    

    enum ConnectedTileSetTypes{
        Cliff = 0, CityCliff, IceCliff, DirtRoad, CityDirtRoad, Highway, Shore, PaveShore, RailRoad
    };
    enum ConnectedTileSetSpecialTypes{
        SnowSnow = 0, SnowStone, StoneStone, StoneSnow, SnowWater, StoneWater
    };

    enum {
        Set_Building = 0, Set_Infantry, Set_Vehicle, Set_Aircraft, Set_Count
    };
    enum{
        RandomRock = 1000, Wall = 2000, WallEnd = 3999, AddOre , ReduceOre
    };
    enum{
        RandomTechno = 9950
    };

    enum {
        RandomTree = 1000
    };
    
    enum {
        MoveUp = 0, MoveDown, Move
    };
    
    enum {
        AnnotationsAdd = 0, AnnotationsRemove
    };

    enum {
        i1_2_3 = 0, i1_3_2, i2_1_3, i2_3_1, i3_1_2, i3_2_1, i4_2_3, i4_3_2, i2_4_3, i2_3_4, i3_4_2, i3_2_4, changeOrder
    };

    enum randomCrater {
        random1x1crater = 100, randomBIGcrater, randomcrater, random1x1burn, randomBIGburn, randomburn, random1x1smudge, randomBIGsmudge, randomsmudge
    };


    enum {
        Add = 0, Delete, AllDelete, batchAdd, batchDelete, TileSetAdd, TileSetDelete,
        ConnectedAdd, ConnectedDelete, ReplaceHide, squareBatchAdd, squareBatchDelete
    };

    enum
    {
        Const_Infantry = 10000, Const_Building = 20000, Const_Aircraft = 30000,
        Const_Vehicle = 40000, Const_Terrain = 50000, Const_Overlay = 63000,
        Const_House = 70000, Const_Smudge = 80000, Const_PropertyBrush = 90000,
        Const_InfantrySubCell = 100000, Const_BaseNode = 110000, Const_ViewObjectInfo = 120000,
        Const_MultiSelection = 130000, Const_ConnectedTile = 140000, Const_Annotation = 150000,
    };
    static std::unordered_map<int, ConnectedTileInfo> TreeView_ConnectedTileMap;
    static int CurrentConnectedTileType;
    static int RedrawCalledCount;
    static bool IsOpeningAnnotationDlg;
    static int InsertingOverlayData;

private:
    static std::array<HTREEITEM, Root_Count> ExtNodes;
    static std::unordered_set<FString> IgnoreSet;
    static std::unordered_set<FString> ForceName;
    static std::unordered_map<FString, FString> RenameString;
    static std::unordered_set<FString> ExtSets[Set_Count];
    static std::unordered_map<FString, int[10]> KnownItem;
    static std::unordered_map<FString, int> Owners;
    static std::unordered_set<FString> AddOnceSet;
    static int AddedItemCount;
    static CImageList m_ImageList;
    static int InsertingTileIndex;
    static int InsertingOverlay;
    static bool InsertingSpecialBitmap;
    static FString InsertingObjectID;
    static CBitmap SpecialBitmap;

    HTREEITEM InsertString(const char* pString, DWORD dwItemData = 0, 
        HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST);
    HTREEITEM InsertTranslatedString(const char* pOriginString, DWORD dwItemData = 0,
        HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST);
    void Redraw_Initialize();
    void Redraw_MainList();
    void Redraw_Ground();
    void Redraw_Owner();
    void Redraw_Infantry();
    void Redraw_Vehicle();
    void Redraw_Aircraft();
    void Redraw_Building();
    void Redraw_Terrain();
    void Redraw_Smudge();
    void Redraw_Overlay();
    void Redraw_Waypoint();
    void Redraw_Celltag();
    void Redraw_Basenode();
    void Redraw_Tunnel();
    void Redraw_PlayerLocation();
    void Redraw_PropertyBrush();
    void Redraw_InfantrySubCell();

    void Redraw_ViewObjectInfo();
    void Redraw_MultiSelection();
    void Redraw_Annotation();

    bool DoPropertyBrush_Building();
    bool DoPropertyBrush_Infantry();
    bool DoPropertyBrush_Vehicle();
    bool DoPropertyBrush_Aircraft();

    int PropagateFirstNonZeroIcon(HTREEITEM hItem);
    void UpdateTreeIconsForSubtree(HTREEITEM hItem);

public:
    enum ObjectTerrainType
    {
        Infantry = 0,
        Building,
        Aircraft,
        Vehicle,
        BaseNode,
        Overlay,
        Celltag,
        Waypoints,
        Tile,
        Terrain,
        Smudge,
        Object,
        All,
        AllTerrain,
        House,
        WeaponRange,
        SecondaryWeaponRange,
        DeathWeaponRange,
        GapRange,
        SensorsRange,
        CloakRange,
        PsychicRange,
        GuardRange,
        SightRange,
        AllRange,
    };


    static MapCoord CliffConnectionCoord;
    static std::vector<MapCoord> CliffConnectionCoordRecords;
    static int CliffConnectionTile;
    static int CliffConnectionHeight;
    static int CliffConnectionHeightAdjust;
    static int LastSuccessfulIndex;
    static bool LastSuccessfulOpposite;
    static int LastSuccessfulHeightOffset;
    static bool IsUsingTXCliff;
    static bool HeightChanged;
    static bool IsInPlaceCliff_OnMouseMove;
    static bool PlaceConnectedTile_Start;

    static int NextCTHeightOffset;
    static void LowerNextCT()
    {
        NextCTHeightOffset--;
        if (NextCTHeightOffset < -1)
            NextCTHeightOffset = -1;
    }
    static void RaiseNextCT()
    {
        NextCTHeightOffset++;
        if (NextCTHeightOffset > 1)
            NextCTHeightOffset = 1;
    }

    static std::vector<ConnectedTileSet> ConnectedTileSets;
    static std::vector<ConnectedTiles> LastPlacedCTRecords;
    static ConnectedTiles LastPlacedCT;
    static ConnectedTiles ThisPlacedCT;
    static int LastTempPlacedCTIndex;
    static int LastTempFacing;

    static int LastCTTile;
    static std::vector<int> LastCTTileRecords;
    static std::vector<int> LastHeightRecords;

    //static CINI ConnectedTileDrawer;

    static bool BuildingBrushBools[14];
    static bool InfantryBrushBools[10];
    static bool VehicleBrushBools[11];
    static bool AircraftBrushBools[9];

    static bool BuildingBrushBoolsBF[14];
    static bool InfantryBrushBoolsF[10];
    static bool VehicleBrushBoolsF[11];
    static bool AircraftBrushBoolsF[9];
    static bool BuildingBrushBoolsBNF[14];

    static std::vector<FString> ObjectFilterB;
    static std::vector<FString> ObjectFilterI;
    static std::vector<FString> ObjectFilterA;
    static std::vector<FString> ObjectFilterV;
    static std::vector<FString> ObjectFilterBN;
    static std::vector<FString> ObjectFilterCT;

    static std::map<int, int> WallDamageStages;

    static bool InitPropertyDlgFromProperty;
    static int PlacingWall;
    static int PlacingRandomRock;
    static int PlacingRandomSmudge;
    static int PlacingRandomTerrain;
    static int PlacingRandomInfantry;
    static int PlacingRandomVehicle;
    static int PlacingRandomStructure;
    static int PlacingRandomAircraft;
    static bool PlacingRandomRandomFacing;
    static bool PlacingRandomStructureAIRepairs;
    static MoveBaseNode MoveBaseNode_SelectedObj;
    static bool NeedChangeTreeViewSelect;

    static std::unique_ptr<CPropertyBuilding> BuildingBrushDlg;
    static std::unique_ptr<CPropertyInfantry> InfantryBrushDlg;
    static std::unique_ptr<CPropertyUnit> VehicleBrushDlg;
    static std::unique_ptr<CPropertyAircraft> AircraftBrushDlg;

    static std::unique_ptr<CPropertyBuilding> BuildingBrushDlgBF;
    static std::unique_ptr<CPropertyInfantry> InfantryBrushDlgF;
    static std::unique_ptr<CPropertyUnit> VehicleBrushDlgF;
    static std::unique_ptr<CPropertyAircraft> AircraftBrushDlgF;
    static std::unique_ptr<CPropertyBuilding> BuildingBrushDlgBNF;

    void Redraw();
    bool UpdateEngine(int nData);
    static void OnExeTerminate();
    static void InitializeOnUpdateEngine();
    static void ApplyDragFacing(int X, int Y);
    static void ApplyChangeOwner(int X, int Y);
    static void ApplyPropertyBrush(int X, int Y);
    static void ApplyPropertyBrush_Building(int nIndex);
    static void ApplyPropertyBrush_Infantry(int nIndex);
    static void ApplyPropertyBrush_Aircraft(int nIndex);
    static void ApplyPropertyBrush_Vehicle(int nIndex);
    static void ApplyPropertyBrush_Building(CBuildingData& data);
    static void ApplyPropertyBrush_Infantry(CInfantryData& data);
    static void ApplyPropertyBrush_Aircraft(CAircraftData& data);
    static void ApplyPropertyBrush_Vehicle(CUnitData& data);
    static void ApplyInfantrySubCell(int X, int Y);
    static void PlaceConnectedTile_OnMouseMove(int X, int Y, bool place = false);
    static void PlaceConnectedTile_OnLButtonDown(int X, int Y);
    static void ConnectedTile_Initialize();
    static void MoveBaseNodeOrder(int X, int Y);
    static void MoveBaseNode(int X, int Y);
    static void ModifyOre(int X, int Y);
    static void AddAnnotation(int X, int Y);
    static void RemoveAnnotation(int X, int Y);
    static void DeleteTube(int X, int Y);
    static void BatchAddMultiSelection(int X, int Y, bool add);
    static void SquareBatchAddMultiSelection(int X, int Y, bool add);
    static void Redraw_ConnectedTile(CViewObjectsExt* pThis);
    
    static bool IsIgnored(const char* pItem);

    static FString QueryUIName(const char* pRegName, bool bOnlyOneLine = true);

public:
    /// <summary>
    /// Guess which type does the item belongs to.
    /// </summary>
    /// <param name="pRegName">Reg name of the given item.</param>
    /// <returns>
    /// The index of type guessed. -1 if cannot be guessed.
    /// 0 = Building, 1 = Infantry, 2 = Vehicle, 3 = Aircraft
    /// </returns>
    static int GuessType(const char* pRegName);
    /// <summary>
    /// Guess which side does the item belongs to.
    /// </summary>
    /// <param name="pRegName">Reg name of the given item.</param>
    /// <param name="nType">
    /// Which type does this item belongs to.
    /// 0 = Building, 1 = Infantry, 2 = Vehicle, 3 = Aircraft
    /// </param>
    /// <returns>The index of side guessed. -1 if cannot be guessed.</returns>
    static std::vector<int> GuessSide(const char* pRegName, int nType);
    /// <summary>
    /// Guess which side does the item belongs to.
    /// </summary>
    /// <param name="pRegName">Reg name of the given item.</param>
    /// <returns>The index of side guessed. -1 if cannot be guessed.</returns>
    static int GuessBuildingSide(const char* pRegName);
    /// <summary>
    /// Guess which side does the item belongs to.
    /// </summary>
    /// <param name="pRegName">Reg name of the given item.</param>
    /// <param name="nType">
    /// Which type does this item belongs to.
    /// 0 = Building, 1 = Infantry, 2 = Vehicle, 3 = Aircraft
    /// </param>
    /// <returns>The index of side guessed. -1 if cannot be guessed.</returns>
    static int GuessGenericSide(const char* pRegName, int nType);
};
