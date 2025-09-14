#pragma once

#include "Ext/FA2Expand.h"
#include "Helpers/MultimapHelper.h"

#include <Helpers/Macro.h>
#include <MFC/ppmfc_cstring.h>

#include <map>
#include <unordered_map>
#include <optional>
#include <CObjectDatas.h>

typedef unsigned char byte;

#define RIPARIUS_BEGIN 102
#define RIPARIUS_END 121
#define CRUENTUS_BEGIN 27
#define CRUENTUS_END 38
#define VINIFERA_BEGIN 127
#define VINIFERA_END 146
#define ABOREUS_BEGIN 147
#define ABOREUS_END 166

class FA2sp
{
public:
    static HANDLE hInstance;
    static std::string STDBuffer;
    static ppmfc::CString Buffer;
    static void* pExceptionHandler;
    static ULONG_PTR ulCookie;
    static ULONG_PTR ulCookieEx;
    static bool g_VEH_Enabled;

    static void ExtConfigsInitialize();
};

class ExtConfigs
{
public:
    static bool IsQuitingProgram;
    static bool BrowserRedraw;
    static int ObjectBrowser_GuessMode;
    static int ObjectBrowser_GuessMax;
    static bool ObjectBrowser_CleanUp;
    static bool ObjectBrowser_SafeHouses;
    static bool ObjectBrowser_Foundation;
    static bool LoadLunarWater;
    static bool LoadCivilianStringtable;
    static bool PasteShowOutlineDefault;
    static bool AllowIncludes;
    static bool AllowInherits;
    static bool AllowPlusEqual;
    static bool TutorialTexts_Hide;
    static bool TutorialTexts_Fix;
    static bool TutorialTexts_Viewer;
    static bool SkipTipsOfTheDay;
    static bool SortByTriggerName;
    static bool SortByLabelName;
    static bool SortByLabelName_AITrigger;
    static bool SortByLabelName_Trigger;
    static bool SortByLabelName_Team;
    static bool SortByLabelName_Taskforce;
    static bool SortByLabelName_Script;
    static bool DisplayTriggerID;
    static bool NewTriggerPlusID;
    static bool AdjustDropdownWidth;
    static int AdjustDropdownWidth_Factor;
    static int AdjustDropdownWidth_Max;
    static int CopySelectionBound_Color;
    static int CursorSelectionBound_Color;
    static int DistanceRuler_Color;
    static int DeathWeaponRangeBound_Color;
    static int WeaponRangeBound_Color;
    static int WeaponRangeMinimumBound_Color;
    static int SecondaryWeaponRangeBound_Color;
    static int SecondaryWeaponRangeMinimumBound_Color;
    static int GapRangeBound_Color;
    static int SensorsRangeBound_Color;
    static int CloakRangeBound_Color;
    static int PsychicRangeBound_Color;
    static int GuardRangeBound_Color;
    static int SightRangeBound_Color;
    static bool WeaponRangeBound_SubjectToElevation;
    static int CursorSelectionBound_HeightColor;
    static int Waypoint_Color;
    static bool Waypoint_Background;
    static int Waypoint_Background_Color;
    static int BaseNodeIndex_Color;
    static bool BaseNodeIndex_Background;
    static bool BaseNodeIndex;
    static int BaseNodeIndex_Background_Color;
    static CPoint Waypoint_Text_ExtraOffset;
    static bool ExtWaypoints;
    static bool ExtFacings;
    static bool ExtFacings_Drag;
    static bool ExtFacings_DragPreview;
    static int UndoRedoLimit;
    static bool UndoRedo_RecordObjects;
    static bool UndoRedo_ShiftPlaceTile;
    static bool UndoRedo_HoldPlaceOverlay;
    static bool UseRGBHouseColor;
    static bool SaveMap_AutoSave;
    static int SaveMap_AutoSave_Interval;
    static int SaveMap_AutoSave_Interval_Real;
    static int SaveMap_AutoSave_MaxCount;
    static bool SaveMap_OnlySaveMAP;
    //static bool SaveMap_MultiPlayOnlySaveYRM;
    //static bool SaveMap_SinglePlayOnlySaveMAP;
    static int SaveMap_DefaultPreviewOptionMP;
    static int SaveMap_DefaultPreviewOptionSP;
    static bool SaveMap_FileEncodingComment;
    static bool VerticalLayout;
    static int RecentFileLimit;
    static int MultiSelectionColor;
    static int TerrainGeneratorColor;
    static bool MultiSelectionShiftDeselect;
    static bool RandomTerrainObjects;
    static unsigned int MaxVoxelFacing;
    static bool DDrawInVideoMem;
    static bool DDrawEmulation;
    static bool NoHouseNameTranslation;
    static bool BetterHouseNameTranslation;
    static bool EnableMultiSelection;
    static bool StrictExceptionFilter;
    static bool ExtendedValidationNoError;
    static bool HideNoRubbleBuilding;
    static bool ModernObjectBrowser;
    static bool PlayerAtXForTechnos;
    static bool PlayerAtXForTriggers;
    static bool FileWatcher;
    static bool LoadRA2MixFilesOnly;
    static bool ExtVariables;
    static bool TestNotLoaded;
    static bool CloneWithOrderedID;
    static bool InfantrySubCell_GameDefault;
    static bool InfantrySubCell_Edit;
    static bool InfantrySubCell_Edit_Single;
    static bool InfantrySubCell_Edit_Drag;
    static bool InfantrySubCell_Edit_Place;
    static bool InfantrySubCell_Edit_FixCenter;
    static bool InfantrySubCell_OccupationBits;
    static bool PlaceStructureOverlappingCheck;
    static bool PlaceStructureUpgrades;
    static bool PlaceStructureUpgradeStrength;
    static bool PlaceTileSkipHide;
    static bool InitializeMap;
    static bool ReloadGameFromMapFolder;
    static bool ArtImageSwap;
    static bool ExtraRaiseGroundTerrainSupport;
    static bool ExtendedValidationAres;
    static bool AIRepairDefaultYes;
    static bool AISellableDefaultYes;
    static bool SaveMaps_BetterMapPreview;
    static bool ShowMapBoundInMiniMap;
    static bool CursorSelectionBound_AutoColor;
    static bool MultiSelect_ConsiderLAT;
    static bool FillArea_ConsiderLAT;
    static bool FillArea_ConsiderWater;
    static bool DPIAware;
    static bool SkipBrushSizeChangeOnTools;
    static bool INIEditor_IgnoreTeams;
    static bool StringBufferStackAllocation;
    static int RangeBound_MaxRange;
    static int SearchCombobox_MaxCount;
    static bool SearchCombobox_Waypoint;
    static bool NewTheaterType;
    static bool IncludeType;
    static bool InheritType;
    static int TreeViewCameo_Size;
    static bool TreeViewCameo_Display;
    static float LightingSource[3];
    static bool UseStrictNewTheater;
    static bool InGameDisplay_Shadow;
    static bool InGameDisplay_Deploy;
    static bool InGameDisplay_Water;
    static bool InGameDisplay_Damage;
    static bool InGameDisplay_Hover;
    static bool InGameDisplay_AlphaImage;
    static bool InGameDisplay_Bridge;
    static bool InGameDisplay_AnimAdjust;
    static bool FlatToGroundHideExtra;
    static bool LightingPreview_MultUnitColor;
    static bool LightingPreview_TintTileSetBrowserView;
    static bool DDrawScalingBilinear;
    static bool DDrawScalingBilinear_OnlyShrink;
    static bool LoadImageDataFromServer;
    static int DisplayTextSize;
    static int DistanceRuler_Records;
    static bool DisplayObjectsOutside;
    static bool UseNewToolBarCameo;
    static bool EnableVisualStyle;
    static bool DisableDirectoryCheck;
    static bool ExtOverlays;
    static bool SaveMap_PreserveINISorting;
    static bool ExtMixLoader;
    static bool AVX2_Support;
    static bool EnableDarkMode;
    static bool EnableDarkMode_DimMap;
    static bool ShrinkTilesInTileSetBrowser;
    static ppmfc::CString CloneWithOrderedID_Digits;
    static ppmfc::CString NewTriggerPlusID_Digits;
    static ppmfc::CString Waypoint_SkipCheckList;
    static CInfantryData DefaultInfantryProperty;
    static CUnitData DefaultUnitProperty;
    static CAircraftData DefaultAircraftProperty;
    static CBuildingData DefaultBuildingProperty;

    enum SpecialOptionType : char
    {
        None = 0,
        Restart = 1,
        ReloadMap = 2,
        SaveMap_Timer = 3,
    };

    struct DynamicOptions
    {
        FString DisplayName;
        FString IniKey;
        bool* Value;
        SpecialOptionType Type = SpecialOptionType::None;
    };
    static std::vector<DynamicOptions> Options;
};

namespace std {
    template<>
    struct hash<ppmfc::CString> {
        size_t operator()(const ppmfc::CString& str) const {
            return hash<string_view>()(string_view(str, str.GetLength()));
        }
    };
    template<>
    struct hash<CString> {
        size_t operator()(const CString& str) const {
            return hash<string_view>()(string_view(str, str.GetLength()));
        }
    };
}

class Variables
{
public:
    static MultimapHelper RulesMap;
    static MultimapHelper Rules;
    static MultimapHelper FAData;
    static MultimapHelper Rules_FAData;
};

class VEHGuard {
    bool oldState;
public:
    VEHGuard(bool enable) {
        oldState = FA2sp::g_VEH_Enabled;
        FA2sp::g_VEH_Enabled = enable;
    }
    ~VEHGuard() {
        FA2sp::g_VEH_Enabled = oldState;
    }
};