#include "FA2sp.h"
#include "FA2sp.Constants.h"
#include "Logger.h"

#include "Helpers/MutexHelper.h"
#include "Helpers/InstructionSet.h"
#include "Helpers/STDHelpers.h"
#include "Miscs/Palettes.h"
#include "Miscs/VoxelDrawer.h"
#include "Miscs/Exception.h"

#include "Ext/CFinalSunApp/Body.h"

#include <CINI.h>

#include <clocale>
#include <algorithm>
#include <bit>
#include "Helpers/Translations.h"

HANDLE FA2sp::hInstance;
ULONG_PTR FA2sp::ulCookie;
ULONG_PTR FA2sp::ulCookieEx;
std::string FA2sp::STDBuffer;
ppmfc::CString FA2sp::Buffer;
void* FA2sp::pExceptionHandler = nullptr;
bool FA2sp::g_VEH_Enabled = true;

bool ExtConfigs::IsQuitingProgram = false;
bool ExtConfigs::BrowserRedraw;
int	 ExtConfigs::ObjectBrowser_GuessMode;
int	 ExtConfigs::ObjectBrowser_GuessMax;
bool ExtConfigs::ObjectBrowser_CleanUp;
bool ExtConfigs::ObjectBrowser_SafeHouses;
bool ExtConfigs::ObjectBrowser_Foundation;
bool ExtConfigs::LoadLunarWater;
bool ExtConfigs::LoadCivilianStringtable;
bool ExtConfigs::PasteShowOutlineDefault;
bool ExtConfigs::AllowIncludes;
bool ExtConfigs::AllowInherits;
bool ExtConfigs::AllowPlusEqual;
bool ExtConfigs::TutorialTexts_Hide;
bool ExtConfigs::TutorialTexts_Fix;
bool ExtConfigs::TutorialTexts_Viewer;
bool ExtConfigs::SkipTipsOfTheDay;
bool ExtConfigs::SortByTriggerName;
bool ExtConfigs::SortByLabelName;
bool ExtConfigs::SortByLabelName_AITrigger;
bool ExtConfigs::SortByLabelName_Trigger;
bool ExtConfigs::SortByLabelName_Team;
bool ExtConfigs::SortByLabelName_Taskforce;
bool ExtConfigs::SortByLabelName_Script;
bool ExtConfigs::NewTriggerPlusID;
bool ExtConfigs::DisplayTriggerID;
bool ExtConfigs::AdjustDropdownWidth;
int ExtConfigs::AdjustDropdownWidth_Factor;
int ExtConfigs::AdjustDropdownWidth_Max;
int ExtConfigs::CopySelectionBound_Color;
int ExtConfigs::CursorSelectionBound_Color;
int ExtConfigs::DistanceRuler_Color;
int ExtConfigs::DeathWeaponRangeBound_Color;
int ExtConfigs::WeaponRangeBound_Color;
int ExtConfigs::WeaponRangeMinimumBound_Color;
int ExtConfigs::SecondaryWeaponRangeBound_Color;
int ExtConfigs::SecondaryWeaponRangeMinimumBound_Color;
bool ExtConfigs::WeaponRangeBound_SubjectToElevation;
int ExtConfigs::GapRangeBound_Color;
int ExtConfigs::SensorsRangeBound_Color;
int ExtConfigs::CloakRangeBound_Color;
int ExtConfigs::PsychicRangeBound_Color;
int ExtConfigs::GuardRangeBound_Color;
int ExtConfigs::SightRangeBound_Color;
int ExtConfigs::CursorSelectionBound_HeightColor;
int ExtConfigs::Waypoint_Color;
bool ExtConfigs::Waypoint_Background;
int ExtConfigs::Waypoint_Background_Color;
CPoint ExtConfigs::Waypoint_Text_ExtraOffset;
int ExtConfigs::BaseNodeIndex_Color;
bool ExtConfigs::BaseNodeIndex_Background;
bool ExtConfigs::BaseNodeIndex;
int ExtConfigs::BaseNodeIndex_Background_Color;
bool ExtConfigs::ExtWaypoints;
bool ExtConfigs::ExtFacings;
bool ExtConfigs::ExtFacings_Drag;
bool ExtConfigs::ExtFacings_DragPreview;
int ExtConfigs::UndoRedoLimit;
bool ExtConfigs::UndoRedo_ShiftPlaceTile;
bool ExtConfigs::UndoRedo_RecordObjects;
bool ExtConfigs::UndoRedo_HoldPlaceOverlay;
bool ExtConfigs::UseRGBHouseColor;
bool ExtConfigs::SaveMap_AutoSave;
int ExtConfigs::SaveMap_AutoSave_Interval;
int ExtConfigs::SaveMap_AutoSave_Interval_Real;
int ExtConfigs::SaveMap_AutoSave_MaxCount;
bool ExtConfigs::SaveMap_OnlySaveMAP;
//bool ExtConfigs::SaveMap_MultiPlayOnlySaveYRM;
//bool ExtConfigs::SaveMap_SinglePlayOnlySaveMAP;
int ExtConfigs::SaveMap_DefaultPreviewOptionMP;
int ExtConfigs::SaveMap_DefaultPreviewOptionSP;
bool ExtConfigs::SaveMap_FileEncodingComment;
bool ExtConfigs::VerticalLayout;
int ExtConfigs::RecentFileLimit;
int ExtConfigs::MultiSelectionColor;
int ExtConfigs::TerrainGeneratorColor;
bool ExtConfigs::MultiSelectionShiftDeselect;
bool ExtConfigs::RandomTerrainObjects;
unsigned int ExtConfigs::MaxVoxelFacing;
bool ExtConfigs::DDrawInVideoMem;
bool ExtConfigs::DDrawEmulation;
bool ExtConfigs::NoHouseNameTranslation;
bool ExtConfigs::BetterHouseNameTranslation;
bool ExtConfigs::EnableMultiSelection;
bool ExtConfigs::StrictExceptionFilter;
bool ExtConfigs::ExtendedValidationNoError;
bool ExtConfigs::HideNoRubbleBuilding;
bool ExtConfigs::ModernObjectBrowser;
bool ExtConfigs::PlayerAtXForTechnos;
bool ExtConfigs::PlayerAtXForTriggers;
bool ExtConfigs::FileWatcher;
bool ExtConfigs::LoadRA2MixFilesOnly;
bool ExtConfigs::ExtVariables;
bool ExtConfigs::TestNotLoaded;
bool ExtConfigs::CloneWithOrderedID;
bool ExtConfigs::InfantrySubCell_GameDefault;
bool ExtConfigs::InfantrySubCell_Edit;
bool ExtConfigs::InfantrySubCell_Edit_Single;
bool ExtConfigs::InfantrySubCell_Edit_Drag;
bool ExtConfigs::InfantrySubCell_Edit_Place;
bool ExtConfigs::InfantrySubCell_Edit_FixCenter;
bool ExtConfigs::InfantrySubCell_OccupationBits;
bool ExtConfigs::PlaceStructureOverlappingCheck;
bool ExtConfigs::PlaceStructureUpgrades;
bool ExtConfigs::PlaceStructureUpgradeStrength;
bool ExtConfigs::PlaceTileSkipHide;
bool ExtConfigs::InitializeMap;
bool ExtConfigs::ReloadGameFromMapFolder;
bool ExtConfigs::ArtImageSwap;
bool ExtConfigs::ExtraRaiseGroundTerrainSupport;
bool ExtConfigs::ExtendedValidationAres;
bool ExtConfigs::AIRepairDefaultYes;
bool ExtConfigs::AISellableDefaultYes;
bool ExtConfigs::SaveMaps_BetterMapPreview;
bool ExtConfigs::ShowMapBoundInMiniMap;
bool ExtConfigs::CursorSelectionBound_AutoColor;
bool ExtConfigs::MultiSelect_ConsiderLAT;
bool ExtConfigs::FillArea_ConsiderLAT;
bool ExtConfigs::FillArea_ConsiderWater;
bool ExtConfigs::DPIAware;
bool ExtConfigs::SkipBrushSizeChangeOnTools;
bool ExtConfigs::INIEditor_IgnoreTeams;
bool ExtConfigs::StringBufferStackAllocation = true;
int ExtConfigs::RangeBound_MaxRange;
int ExtConfigs::SearchCombobox_MaxCount;
bool ExtConfigs::SearchCombobox_Waypoint;
bool ExtConfigs::NewTheaterType;
bool ExtConfigs::IncludeType;
bool ExtConfigs::InheritType;
int ExtConfigs::TreeViewCameo_Size;
bool ExtConfigs::TreeViewCameo_Display;
float ExtConfigs::LightingSource[3];
bool ExtConfigs::UseStrictNewTheater;
bool ExtConfigs::InGameDisplay_Shadow;
bool ExtConfigs::InGameDisplay_Deploy;
bool ExtConfigs::InGameDisplay_Water;
bool ExtConfigs::InGameDisplay_Damage;
bool ExtConfigs::InGameDisplay_Hover;
bool ExtConfigs::InGameDisplay_AlphaImage;
bool ExtConfigs::InGameDisplay_Bridge;
bool ExtConfigs::InGameDisplay_AnimAdjust;
bool ExtConfigs::FlatToGroundHideExtra;
bool ExtConfigs::LightingPreview_MultUnitColor;
bool ExtConfigs::LightingPreview_TintTileSetBrowserView;
bool ExtConfigs::DDrawScalingBilinear;
bool ExtConfigs::DDrawScalingBilinear_OnlyShrink;
bool ExtConfigs::LoadImageDataFromServer;
bool ExtConfigs::UseNewToolBarCameo;
bool ExtConfigs::EnableVisualStyle;
bool ExtConfigs::DisableDirectoryCheck;
bool ExtConfigs::ExtOverlays;
bool ExtConfigs::SaveMap_PreserveINISorting;
bool ExtConfigs::ExtMixLoader;
int ExtConfigs::DisplayTextSize;
int ExtConfigs::DistanceRuler_Records;
bool ExtConfigs::DisplayObjectsOutside;
bool ExtConfigs::AVX2_Support;
bool ExtConfigs::EnableDarkMode;
bool ExtConfigs::EnableDarkMode_DimMap;
bool ExtConfigs::ShrinkTilesInTileSetBrowser;
ppmfc::CString ExtConfigs::CloneWithOrderedID_Digits;
ppmfc::CString ExtConfigs::NewTriggerPlusID_Digits;
ppmfc::CString ExtConfigs::Waypoint_SkipCheckList;

CInfantryData ExtConfigs::DefaultInfantryProperty;
CUnitData ExtConfigs::DefaultUnitProperty;
CAircraftData ExtConfigs::DefaultAircraftProperty;
CBuildingData ExtConfigs::DefaultBuildingProperty;

std::vector<ExtConfigs::DynamicOptions> ExtConfigs::Options;

MultimapHelper Variables::RulesMap = { &CINI::Rules(), &CINI::CurrentDocument() };
MultimapHelper Variables::Rules = { &CINI::Rules() };
MultimapHelper Variables::FAData = { &CINI::FAData() };
MultimapHelper Variables::Rules_FAData = { &CINI::Rules(), &CINI::CurrentDocument(), &CINI::FAData() };

void FA2sp::ExtConfigsInitialize()
{	
	ExtConfigs::BrowserRedraw = CINI::FAData->GetBool("ExtConfigs", "BrowserRedraw");
	ExtConfigs::ModernObjectBrowser = CINI::FAData->GetBool("ExtConfigs", "ModernObjectBrowser");
	ExtConfigs::ObjectBrowser_GuessMode = CINI::FAData->GetInteger("ExtConfigs", "ObjectBrowser.GuessMode", 0);
	ExtConfigs::ObjectBrowser_GuessMax = CINI::FAData->GetInteger("ExtConfigs", "ObjectBrowser.GuessMax", 15);
	ExtConfigs::ObjectBrowser_CleanUp = CINI::FAData->GetBool("ExtConfigs", "ObjectBrowser.CleanUp");
	ExtConfigs::ObjectBrowser_SafeHouses = CINI::FAData->GetBool("ExtConfigs", "ObjectBrowser.SafeHouses");
	ExtConfigs::ObjectBrowser_Foundation = CINI::FAData->GetBool("ExtConfigs", "ObjectBrowser.Foundation");
	ExtConfigs::LoadLunarWater = CINI::FAData->GetBool("ExtConfigs", "LoadLunarWater");
	ExtConfigs::LoadCivilianStringtable = CINI::FAData->GetBool("ExtConfigs", "LoadCivilianStringtable");
	ExtConfigs::PasteShowOutlineDefault = CINI::FAData->GetBool("ExtConfigs", "PasteShowOutline");
	
	ExtConfigs::AllowIncludes = CINI::FAData->GetBool("ExtConfigs", "AllowIncludes");
	ExtConfigs::AllowInherits = CINI::FAData->GetBool("ExtConfigs", "AllowInherits");
	ExtConfigs::AllowPlusEqual = CINI::FAData->GetBool("ExtConfigs", "AllowPlusEqual");

	ExtConfigs::TutorialTexts_Hide = CINI::FAData->GetBool("ExtConfigs", "TutorialTexts.Hide");
	ExtConfigs::TutorialTexts_Fix = CINI::FAData->GetBool("ExtConfigs", "TutorialTexts.Fix");
	ExtConfigs::TutorialTexts_Viewer = CINI::FAData->GetBool("ExtConfigs", "TutorialTexts.Viewer");

	ExtConfigs::SkipTipsOfTheDay = CINI::FAData->GetBool("ExtConfigs", "SkipTipsOfTheDay");
	
	ExtConfigs::SortByTriggerName = CINI::FAData->GetBool("ExtConfigs", "SortByTriggerName");
	ExtConfigs::SortByLabelName = CINI::FAData->GetBool("ExtConfigs", "SortByLabelName");
	ExtConfigs::SortByLabelName_AITrigger = CINI::FAData->GetBool("ExtConfigs", "SortByLabelName.AITrigger");
	ExtConfigs::SortByLabelName_Trigger = CINI::FAData->GetBool("ExtConfigs", "SortByLabelName.Trigger");
	ExtConfigs::SortByLabelName_Team = CINI::FAData->GetBool("ExtConfigs", "SortByLabelName.Team");
	ExtConfigs::SortByLabelName_Taskforce = CINI::FAData->GetBool("ExtConfigs", "SortByLabelName.Taskforce");
	ExtConfigs::SortByLabelName_Script = CINI::FAData->GetBool("ExtConfigs", "SortByLabelName.Script");

	ExtConfigs::NewTriggerPlusID = CINI::FAData->GetBool("ExtConfigs", "NewTriggerPlusID");
	ExtConfigs::DisplayTriggerID = CINI::FAData->GetBool("ExtConfigs", "DisplayTriggerID");
	ExtConfigs::CloneWithOrderedID = CINI::FAData->GetBool("ExtConfigs", "CloneWithOrderedID");
	auto temp = CINI::FAData->GetString("ExtConfigs", "CloneWithOrderedID.Digits");
	temp.TrimRight();
	ExtConfigs::CloneWithOrderedID_Digits = temp;

	auto temp2 = CINI::FAData->GetString("ExtConfigs", "NewTriggerPlusID.Digits");
	temp2.TrimRight();
	ExtConfigs::NewTriggerPlusID_Digits = temp2;

	ExtConfigs::AdjustDropdownWidth = CINI::FAData->GetBool("ExtConfigs", "AdjustDropdownWidth");
	ExtConfigs::AdjustDropdownWidth_Factor = CINI::FAData->GetInteger("ExtConfigs", "AdjustDropdownWidth.Factor", 8);
	ExtConfigs::AdjustDropdownWidth_Max = CINI::FAData->GetInteger("ExtConfigs", "AdjustDropdownWidth.Max", 360);

	ExtConfigs::CopySelectionBound_Color = 
		CINI::FAData->GetColor("ExtConfigs", "CopySelectionBound.Color", 0x0000FF);
	ExtConfigs::CursorSelectionBound_Color =
		CINI::FAData->GetColor("ExtConfigs", "CursorSelectionBound.Color", 0x3CA03C);
	ExtConfigs::CursorSelectionBound_HeightColor = 
		CINI::FAData->GetColor("ExtConfigs", "CursorSelectionBound.HeightIndicatorColor", 0x3C3C3C);
	ExtConfigs::CursorSelectionBound_AutoColor = CINI::FAData->GetBool("ExtConfigs", "CursorSelectionBound.AutoHeightColor");
	ExtConfigs::MultiSelect_ConsiderLAT = CINI::FAData->GetBool("ExtConfigs", "MultiSelect.ConsiderLAT", true);
	ExtConfigs::FillArea_ConsiderLAT = CINI::FAData->GetBool("ExtConfigs", "FillArea.ConsiderLAT", true);
	ExtConfigs::FillArea_ConsiderWater = CINI::FAData->GetBool("ExtConfigs", "FillArea.ConsiderWater", true);

	ExtConfigs::DPIAware = CINI::FAData->GetBool("ExtConfigs", "DPIAware");

	ExtConfigs::Waypoint_Color = CINI::FAData->GetColor("ExtConfigs", "Waypoint.Color", 0xFF0000);
	ExtConfigs::Waypoint_Background = CINI::FAData->GetBool("ExtConfigs", "Waypoint.Background");
	ExtConfigs::Waypoint_Background_Color = CINI::FAData->GetColor("ExtConfigs", "Waypoint.Background.Color", 0xFFFFFF);

	ExtConfigs::DeathWeaponRangeBound_Color = CINI::FAData->GetColor("ExtConfigs", "DeathWeaponRangeBound.Color", 0x277FFF);
	ExtConfigs::WeaponRangeBound_Color = CINI::FAData->GetColor("ExtConfigs", "WeaponRangeBound.Color", 0xFFFF00);
	ExtConfigs::WeaponRangeMinimumBound_Color = CINI::FAData->GetColor("ExtConfigs", "WeaponRangeMinimumBound.Color", 0xC8C800);
	ExtConfigs::SecondaryWeaponRangeBound_Color = CINI::FAData->GetColor("ExtConfigs", "SecondaryWeaponRangeBound.Color", 0x82FF00);
	ExtConfigs::SecondaryWeaponRangeMinimumBound_Color = CINI::FAData->GetColor("ExtConfigs", "SecondaryWeaponRangeMinimumBound.Color", 0x64C800);

	ExtConfigs::GapRangeBound_Color = CINI::FAData->GetColor("ExtConfigs", "GapRangeBound.Color", 0xFF0000);
	ExtConfigs::SensorsRangeBound_Color = CINI::FAData->GetColor("ExtConfigs", "SensorsRangeBound.Color", 0xFF00FF);
	ExtConfigs::CloakRangeBound_Color = CINI::FAData->GetColor("ExtConfigs", "CloakRangeBound.Color", 0x0000FF);
	ExtConfigs::PsychicRangeBound_Color = CINI::FAData->GetColor("ExtConfigs", "PsychicRangeBound.Color", 0x00FFFF);
	ExtConfigs::GuardRangeBound_Color = CINI::FAData->GetColor("ExtConfigs", "GuardRangeBound.Color", 0x00FF00);
	ExtConfigs::SightRangeBound_Color = CINI::FAData->GetColor("ExtConfigs", "SightRangeBound.Color", 0xFFFFFF);

	ExtConfigs::WeaponRangeBound_SubjectToElevation = CINI::FAData->GetBool("ExtConfigs", "WeaponRangeBound.SubjectToElevation");

	ExtConfigs::Waypoint_Color = CINI::FAData->GetColor("ExtConfigs", "Waypoint.Color", 0x0000FF);
	ExtConfigs::Waypoint_Background = CINI::FAData->GetBool("ExtConfigs", "Waypoint.Background");
	ExtConfigs::Waypoint_Background_Color = CINI::FAData->GetColor("ExtConfigs", "Waypoint.Background.Color", 0xFFFFFF);

	ExtConfigs::BaseNodeIndex_Color = CINI::FAData->GetColor("ExtConfigs", "BaseNodeIndex.Color", 0x00FFFF);
	ExtConfigs::BaseNodeIndex_Background = CINI::FAData->GetBool("ExtConfigs", "BaseNodeIndex.Background");
	ExtConfigs::BaseNodeIndex = CINI::FAData->GetBool("ExtConfigs", "BaseNodeIndex");
	ExtConfigs::BaseNodeIndex_Background_Color = CINI::FAData->GetColor("ExtConfigs", "BaseNodeIndex.Background.Color", 0x3C3C3C);

	ExtConfigs::Waypoint_Text_ExtraOffset = CINI::FAData->GetPoint("ExtConfigs", "Waypoint.Text.ExtraOffset");
	ExtConfigs::Waypoint_SkipCheckList = CINI::FAData->GetString("ExtConfigs", "Waypoint.SkipCheckList");

	ExtConfigs::ExtWaypoints = CINI::FAData->GetBool("ExtConfigs", "ExtWaypoints");
	ExtConfigs::ExtFacings = CINI::FAData->GetBool("ExtConfigs", "ExtFacings");
	ExtConfigs::ExtFacings_Drag = CINI::FAData->GetBool("ExtConfigs", "ExtFacings.Drag");
	ExtConfigs::ExtFacings_DragPreview = CINI::FAData->GetBool("ExtConfigs", "ExtFacings.DragPreview", true);
	ExtConfigs::ExtVariables = CINI::FAData->GetBool("ExtConfigs", "ExtVariables");
	ExtConfigs::AIRepairDefaultYes = CINI::FAData->GetBool("ExtConfigs", "AIRepairDefaultYes");
	ExtConfigs::AISellableDefaultYes = CINI::FAData->GetBool("ExtConfigs", "AISellableDefaultYes");

	ExtConfigs::ShrinkTilesInTileSetBrowser = CINI::FAData->GetBool("ExtConfigs", "ShrinkTilesInTileSetBrowser");
	ExtConfigs::EnableDarkMode = CINI::FAData->GetBool("ExtConfigs", "EnableDarkMode");
	ExtConfigs::EnableDarkMode_DimMap = CINI::FAData->GetBool("ExtConfigs", "EnableDarkMode.DimMap");
	ExtConfigs::DisplayObjectsOutside = CINI::FAData->GetBool("ExtConfigs", "DisplayObjectsOutside");
	ExtConfigs::DDrawScalingBilinear = CINI::FAData->GetBool("ExtConfigs", "DDrawScalingBilinear", true);
	ExtConfigs::DDrawScalingBilinear_OnlyShrink = CINI::FAData->GetBool("ExtConfigs", "DDrawScalingBilinear.OnlyShrink", true);
	ExtConfigs::LoadImageDataFromServer = CINI::FAData->GetBool("ExtConfigs", "LoadImageDataFromServer");

	ExtConfigs::LightingPreview_MultUnitColor = CINI::FAData->GetBool("ExtConfigs", "LightingPreview.MultUnitColor");
	ExtConfigs::LightingPreview_TintTileSetBrowserView = CINI::FAData->GetBool("ExtConfigs", "LightingPreview.TintTileSetBrowserView");
	ExtConfigs::UseStrictNewTheater = CINI::FAData->GetBool("ExtConfigs", "UseStrictNewTheater");
	ExtConfigs::DisableDirectoryCheck = CINI::FAData->GetBool("ExtConfigs", "DisableDirectoryCheck");
	ExtConfigs::UseNewToolBarCameo = CINI::FAData->GetBool("ExtConfigs", "UseNewToolBarCameo", true);
	ExtConfigs::EnableVisualStyle = CINI::FAData->GetBool("ExtConfigs", "EnableVisualStyle", true);
	ExtConfigs::InGameDisplay_Shadow = CINI::FAData->GetBool("ExtConfigs", "InGameDisplay.Shadow", true);
	ExtConfigs::InGameDisplay_Deploy = CINI::FAData->GetBool("ExtConfigs", "InGameDisplay.Deploy", true);
	ExtConfigs::InGameDisplay_Water = CINI::FAData->GetBool("ExtConfigs", "InGameDisplay.Water", true);
	ExtConfigs::InGameDisplay_Damage = CINI::FAData->GetBool("ExtConfigs", "InGameDisplay.Damage", true);
	ExtConfigs::InGameDisplay_Hover = CINI::FAData->GetBool("ExtConfigs", "InGameDisplay.Hover", true);
	ExtConfigs::InGameDisplay_AlphaImage = CINI::FAData->GetBool("ExtConfigs", "InGameDisplay.AlphaImage", true);
	ExtConfigs::InGameDisplay_Bridge = CINI::FAData->GetBool("ExtConfigs", "InGameDisplay.Bridge", true);
	ExtConfigs::InGameDisplay_AnimAdjust = CINI::FAData->GetBool("ExtConfigs", "InGameDisplay.AnimAdjust", true);
	ExtConfigs::FlatToGroundHideExtra = CINI::FAData->GetBool("ExtConfigs", "FlatToGroundHideExtra");
	ExtConfigs::ExtOverlays = CINI::FAData->GetBool("ExtConfigs", "ExtOverlays");

	ExtConfigs::DistanceRuler_Records = CINI::FAData->GetInteger("ExtConfigs", "DistanceRuler.Records", 5);
	ExtConfigs::DisplayTextSize = CINI::FAData->GetInteger("ExtConfigs", "DisplayTextSize", 18);
	ExtConfigs::TreeViewCameo_Size = CINI::FAData->GetInteger("ExtConfigs", "TreeViewCameo.Size", 32);
	if (ExtConfigs::TreeViewCameo_Size > 64)
		ExtConfigs::TreeViewCameo_Size = 64;
	if (ExtConfigs::TreeViewCameo_Size < 16)
		ExtConfigs::TreeViewCameo_Size = 16;

	ExtConfigs::NewTheaterType = CINI::FAData->GetBool("ExtConfigs", "NewTheaterType", true);
	ExtConfigs::InheritType = CINI::FAData->GetBool("ExtConfigs", "InheritType");
	ExtConfigs::IncludeType = CINI::FAData->GetBool("ExtConfigs", "IncludeType");
	ExtConfigs::SearchCombobox_Waypoint = CINI::FAData->GetBool("ExtConfigs", "SearchCombobox.Waypoint");
	ExtConfigs::SearchCombobox_MaxCount = CINI::FAData->GetInteger("ExtConfigs", "SearchCombobox.MaxCount", 1000);
	if (ExtConfigs::SearchCombobox_MaxCount < 0)
		ExtConfigs::SearchCombobox_MaxCount = INT_MAX;
	ExtConfigs::RangeBound_MaxRange = CINI::FAData->GetInteger("ExtConfigs", "RangeBound.MaxRange", 50);
	ExtConfigs::UndoRedoLimit = CINI::FAData->GetInteger("ExtConfigs", "UndoRedoLimit", 64);
	ExtConfigs::UndoRedo_ShiftPlaceTile = CINI::FAData->GetBool("ExtConfigs", "UndoRedo.ShiftPlaceTile", true);
	ExtConfigs::UndoRedo_HoldPlaceOverlay = CINI::FAData->GetBool("ExtConfigs", "UndoRedo.HoldPlaceOverlay", true);
	ExtConfigs::UndoRedo_RecordObjects = CINI::FAData->GetBool("ExtConfigs", "UndoRedo.RecordObjects", true);

	ExtConfigs::UseRGBHouseColor = CINI::FAData->GetBool("ExtConfigs", "UseRGBHouseColor");
	ExtConfigs::INIEditor_IgnoreTeams = CINI::FAData->GetBool("ExtConfigs", "INIEditor.IgnoreTeams");
	ExtConfigs::StringBufferStackAllocation = CINI::FAData->GetBool("ExtConfigs", "StringBufferStackAllocation", true);

	if (ExtConfigs::SaveMap_AutoSave = CINI::FAData->GetBool("ExtConfigs", "SaveMap.AutoSave"))
	{
		ExtConfigs::SaveMap_AutoSave_Interval = CINI::FAData->GetInteger("ExtConfigs", "SaveMap.AutoSave.Interval", 300);
		ExtConfigs::SaveMap_AutoSave_Interval_Real = CINI::FAData->GetInteger("ExtConfigs", "SaveMap.AutoSave.Interval", 300);
		ExtConfigs::SaveMap_AutoSave_MaxCount = CINI::FAData->GetInteger("ExtConfigs", "SaveMap.AutoSave.MaxCount", 10);
		if (ExtConfigs::SaveMap_AutoSave_Interval < 30)
		{
			ExtConfigs::SaveMap_AutoSave_Interval_Real = 30;
			ExtConfigs::SaveMap_AutoSave_Interval = 30;
		}
	}
	else
	{
		ExtConfigs::SaveMap_AutoSave_Interval_Real = CINI::FAData->GetInteger("ExtConfigs", "SaveMap.AutoSave.Interval", 300);
		if (ExtConfigs::SaveMap_AutoSave_Interval_Real < 30)
		{
			ExtConfigs::SaveMap_AutoSave_Interval_Real = 30;
		}
		ExtConfigs::SaveMap_AutoSave_Interval = -1;
	}
	ExtConfigs::SaveMap_FileEncodingComment = CINI::FAData->GetBool("ExtConfigs", "SaveMap.FileEncodingComment");
	ExtConfigs::SaveMap_OnlySaveMAP = CINI::FAData->GetBool("ExtConfigs", "SaveMap.OnlySaveMAP");
	ExtConfigs::SaveMap_PreserveINISorting = CINI::FAData->GetBool("ExtConfigs", "SaveMap.PreserveINISorting");
	//ExtConfigs::SaveMap_MultiPlayOnlySaveYRM = CINI::FAData->GetBool("ExtConfigs", "SaveMap.OnlySaveYRM.MultiPlay");
	//ExtConfigs::SaveMap_SinglePlayOnlySaveMAP = CINI::FAData->GetBool("ExtConfigs", "SaveMap.OnlySaveMAP.SinglePlay");
	ExtConfigs::SaveMap_DefaultPreviewOptionMP = CINI::FAData->GetInteger("ExtConfigs", "SaveMap.DefaultPreviewOptionMP", 0);
	ExtConfigs::SaveMap_DefaultPreviewOptionSP = CINI::FAData->GetInteger("ExtConfigs", "SaveMap.DefaultPreviewOptionSP", 1);

	ExtConfigs::VerticalLayout = CINI::FAData->GetBool("ExtConfigs", "VerticalLayout");

	ExtConfigs::RecentFileLimit = std::clamp(CINI::FAData->GetInteger("ExtConfigs", "RecentFileLimit"), 4, 9);

	ExtConfigs::MultiSelectionColor = CINI::FAData->GetColor("ExtConfigs", "MultiSelectionColor", 0x00FF00);
	ExtConfigs::TerrainGeneratorColor = CINI::FAData->GetColor("ExtConfigs", "TerrainGeneratorColor", 0x00FFFF);
	ExtConfigs::DistanceRuler_Color = CINI::FAData->GetColor("ExtConfigs", "DistanceRuler.Color", 0x0000FF);
	ExtConfigs::MultiSelectionShiftDeselect = CINI::FAData->GetBool("ExtConfigs", "MultiSelectionShiftDeselect");
	ExtConfigs::ExtMixLoader = CINI::FAData->GetBool("ExtConfigs", "ExtMixLoader", true);

	ExtConfigs::RandomTerrainObjects = CINI::FAData->GetBool("ExtConfigs", "RandomTerrainObjects");

	ExtConfigs::MaxVoxelFacing = CINI::FAData->GetInteger("ExtConfigs", "MaxVoxelFacing", 8);
	ExtConfigs::MaxVoxelFacing = std::clamp(
		1 << (std::bit_width(ExtConfigs::MaxVoxelFacing) - 1), 8, 256
	);
	// Disable it for now
	ExtConfigs::MaxVoxelFacing = 8;

	ExtConfigs::DDrawInVideoMem = CINI::FAData->GetBool("ExtConfigs", "DDrawInVideoMem", true);
	ExtConfigs::DDrawEmulation = CINI::FAData->GetBool("ExtConfigs", "DDrawEmulation");

	ExtConfigs::NoHouseNameTranslation = CINI::FAData->GetBool("ExtConfigs", "NoHouseNameTranslation");
	ExtConfigs::BetterHouseNameTranslation = CINI::FAData->GetBool("ExtConfigs", "BetterHouseNameTranslation");

	ExtConfigs::EnableMultiSelection = true; // CINI::FAData->GetBool("ExtConfigs", "EnableMultiSelection");

	ExtConfigs::StrictExceptionFilter = CINI::FAData->GetBool("ExtConfigs", "StrictExceptionFilter");
	ExtConfigs::ExtendedValidationNoError = CINI::FAData->GetBool("ExtConfigs", "ExtendedValidationNoError");
	ExtConfigs::HideNoRubbleBuilding = CINI::FAData->GetBool("ExtConfigs", "HideNoRubbleBuilding");

	ExtConfigs::PlayerAtXForTechnos = CINI::FAData->GetBool("ExtConfigs", "PlayerAtXForTechnos");
	ExtConfigs::PlayerAtXForTriggers = CINI::FAData->GetBool("ExtConfigs", "PlayerAtXForTriggers");
	ExtConfigs::FileWatcher = CINI::FAData->GetBool("ExtConfigs", "FileWatcher", true);
	ExtConfigs::LoadRA2MixFilesOnly = CINI::FAData->GetBool("ExtConfigs", "LoadRA2MixFilesOnly");
	ExtConfigs::InfantrySubCell_GameDefault = CINI::FAData->GetBool("ExtConfigs", "InfantrySubCell.GameDefault");
	ExtConfigs::InfantrySubCell_Edit = CINI::FAData->GetBool("ExtConfigs", "InfantrySubCell.Edit");
	ExtConfigs::InfantrySubCell_Edit_Single = CINI::FAData->GetBool("ExtConfigs", "InfantrySubCell.Edit.Single");
	ExtConfigs::InfantrySubCell_Edit_Drag = CINI::FAData->GetBool("ExtConfigs", "InfantrySubCell.Edit.Drag");
	ExtConfigs::InfantrySubCell_Edit_Place = CINI::FAData->GetBool("ExtConfigs", "InfantrySubCell.Edit.Place");
	ExtConfigs::InfantrySubCell_Edit_FixCenter = CINI::FAData->GetBool("ExtConfigs", "InfantrySubCell.Edit.FixCenter");
	ExtConfigs::InfantrySubCell_OccupationBits = CINI::FAData->GetBool("ExtConfigs", "InfantrySubCell.OccupationBits");
	ExtConfigs::PlaceStructureOverlappingCheck = CINI::FAData->GetBool("ExtConfigs", "PlaceStructure.OverlappingCheck");
	ExtConfigs::PlaceStructureUpgrades = CINI::FAData->GetBool("ExtConfigs", "PlaceStructure.AutoUpgrade");
	ExtConfigs::PlaceStructureUpgradeStrength = CINI::FAData->GetBool("ExtConfigs", "PlaceStructure.UpgradeStrength");
	ExtConfigs::PlaceTileSkipHide = CINI::FAData->GetBool("ExtConfigs", "PlaceTileSkipHide");
	ExtConfigs::ReloadGameFromMapFolder = CINI::FAData->GetBool("ExtConfigs", "ReloadGameFromMapFolder");
	ExtConfigs::ArtImageSwap = CINI::FAData->GetBool("ExtConfigs", "ArtImageSwap");
	ExtConfigs::ExtraRaiseGroundTerrainSupport = CINI::FAData->GetBool("ExtConfigs", "ExtraRaiseGroundTerrainSupport");
	ExtConfigs::ExtendedValidationAres = CINI::FAData->GetBool("ExtConfigs", "ExtendedValidationAres");
	ExtConfigs::SaveMaps_BetterMapPreview = CINI::FAData->GetBool("ExtConfigs", "SaveMap.BetterMapPreview");
	ExtConfigs::ShowMapBoundInMiniMap = CINI::FAData->GetBool("ExtConfigs", "ShowMapBoundInMiniMap");

	ExtConfigs::SkipBrushSizeChangeOnTools = CINI::FAData->GetBool("ExtConfigs", "SkipBrushSizeChangeOnTools");
	CIsoViewExt::ScaledMax = CINI::FAData->GetDouble("ExtConfigs", "DDrawScalingMaximum", 1.5);
	if (CIsoViewExt::ScaledMax < 1.0)
		CIsoViewExt::ScaledMax = 1.0;

	auto ls = STDHelpers::SplitString(CINI::FAData->GetString("ExtConfigs", "LightingSource", "0.05,1,0.2"), 2);
	ExtConfigs::LightingSource[0] = atof(ls[0]);
	ExtConfigs::LightingSource[1] = atof(ls[1]);
	ExtConfigs::LightingSource[2] = atof(ls[2]);

	auto infantry = STDHelpers::SplitString(CINI::FAData->GetString("ExtConfigs", "DefaultInfantryProperty",
		"House,ID,256,X,Y,Subcell,Guard,64,None,0,-1,0,0,0"), 13);
	ExtConfigs::DefaultInfantryProperty.Health = infantry[2];
	ExtConfigs::DefaultInfantryProperty.Status = infantry[6];
	ExtConfigs::DefaultInfantryProperty.Facing = infantry[7];
	ExtConfigs::DefaultInfantryProperty.Tag = infantry[8];
	ExtConfigs::DefaultInfantryProperty.VeterancyPercentage = infantry[9];
	ExtConfigs::DefaultInfantryProperty.Group = infantry[10];
	ExtConfigs::DefaultInfantryProperty.IsAboveGround = infantry[11];
	ExtConfigs::DefaultInfantryProperty.AutoNORecruitType = infantry[12];
	ExtConfigs::DefaultInfantryProperty.AutoYESRecruitType = infantry[13];

	auto unit = STDHelpers::SplitString(CINI::FAData->GetString("ExtConfigs", "DefaultUnitProperty",
		"House,ID,256,X,Y,64,Guard,None,0,-1,0,-1,0,0"), 13);
	ExtConfigs::DefaultUnitProperty.Health = unit[2];
	ExtConfigs::DefaultUnitProperty.Facing = unit[5];
	ExtConfigs::DefaultUnitProperty.Status = unit[6];
	ExtConfigs::DefaultUnitProperty.Tag = unit[7];
	ExtConfigs::DefaultUnitProperty.VeterancyPercentage = unit[8];
	ExtConfigs::DefaultUnitProperty.Group = unit[9];
	ExtConfigs::DefaultUnitProperty.IsAboveGround = unit[10];
	ExtConfigs::DefaultUnitProperty.FollowsIndex = unit[11];
	ExtConfigs::DefaultUnitProperty.AutoNORecruitType = unit[12];
	ExtConfigs::DefaultUnitProperty.AutoYESRecruitType = unit[13];

	auto aircraft = STDHelpers::SplitString(CINI::FAData->GetString("ExtConfigs", "DefaultAircraftProperty",
		"House,ID,256,X,Y,64,Guard,None,0,-1,0,0"), 11);
	ExtConfigs::DefaultAircraftProperty.Health = aircraft[2];
	ExtConfigs::DefaultAircraftProperty.Facing = aircraft[5];
	ExtConfigs::DefaultAircraftProperty.Status = aircraft[6];
	ExtConfigs::DefaultAircraftProperty.Tag = aircraft[7];
	ExtConfigs::DefaultAircraftProperty.VeterancyPercentage = aircraft[8];
	ExtConfigs::DefaultAircraftProperty.Group = aircraft[9];
	ExtConfigs::DefaultAircraftProperty.AutoNORecruitType = aircraft[10];
	ExtConfigs::DefaultAircraftProperty.AutoYESRecruitType = aircraft[11];

	auto building = STDHelpers::SplitString(CINI::FAData->GetString("ExtConfigs", "DefaultBuildingProperty",
		"House,ID,256,X,Y,0,None,0,0,1,0,0,None,None,None,1,0"), 16);
	ExtConfigs::DefaultBuildingProperty.Health = building[2];
	ExtConfigs::DefaultBuildingProperty.Facing = building[5];
	ExtConfigs::DefaultBuildingProperty.Tag = building[6];
	ExtConfigs::DefaultBuildingProperty.AISellable = building[7];
	ExtConfigs::DefaultBuildingProperty.AIRebuildable = building[8];
	ExtConfigs::DefaultBuildingProperty.PoweredOn = building[9];
	ExtConfigs::DefaultBuildingProperty.Upgrades = building[10];
	ExtConfigs::DefaultBuildingProperty.SpotLight = building[11];
	ExtConfigs::DefaultBuildingProperty.Upgrade1 = building[12];
	ExtConfigs::DefaultBuildingProperty.Upgrade2 = building[13];
	ExtConfigs::DefaultBuildingProperty.Upgrade3 = building[14];
	ExtConfigs::DefaultBuildingProperty.AIRepairable = building[15];
	ExtConfigs::DefaultBuildingProperty.Nominal = building[16];

	ExtConfigs::InitializeMap = false;

	ExtConfigs::TestNotLoaded = false;

	// Editor Interface and Behavior
	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.TutorialTexts.Viewer", "Open CSF Viewer when editing CSF params in Trigger editor"),
		.IniKey = "TutorialTexts.Viewer",
		.Value = &ExtConfigs::TutorialTexts_Viewer,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.CloneWithOrderedID", "Clone triggers (teams) with increasing number instead of 'Clone'"),
		.IniKey = "CloneWithOrderedID",
		.Value = &ExtConfigs::CloneWithOrderedID,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.AdjustDropdownWidth", "Auto-adjust label width for editors"),
		.IniKey = "AdjustDropdownWidth",
		.Value = &ExtConfigs::AdjustDropdownWidth,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.SortByLabelName", "Sort selected labels in editors by name"),
		.IniKey = "SortByLabelName",
		.Value = &ExtConfigs::SortByLabelName,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.SortByLabelName.AITrigger", "Sort AI triggers by label name, override global setting"),
		.IniKey = "SortByLabelName.AITrigger",
		.Value = &ExtConfigs::SortByLabelName_AITrigger,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.SortByLabelName.Trigger", "Sort triggers by label name, override global setting"),
		.IniKey = "SortByLabelName.Trigger",
		.Value = &ExtConfigs::SortByLabelName_Trigger,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.SortByLabelName.Team", "Sort teams by label name, override global setting"),
		.IniKey = "SortByLabelName.Team",
		.Value = &ExtConfigs::SortByLabelName_Team,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.SortByLabelName.Taskforce", "Sort taskforces by label name, override global setting"),
		.IniKey = "SortByLabelName.Taskforce",
		.Value = &ExtConfigs::SortByLabelName_Taskforce,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.SortByLabelName.Script", "Sort scripts by label name, override global setting"),
		.IniKey = "SortByLabelName.Script",
		.Value = &ExtConfigs::SortByLabelName_Script,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.SearchCombobox.Waypoint", "Popup dropdown when editing waypoint params"),
		.IniKey = "SearchCombobox.Waypoint",
		.Value = &ExtConfigs::SearchCombobox_Waypoint,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.VerticalLayout", "Move tile browser to right"),
		.IniKey = "VerticalLayout",
		.Value = &ExtConfigs::VerticalLayout,
		.Type = ExtConfigs::SpecialOptionType::Restart
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.EnableVisualStyle", "Enable visual style (may cause Chinese input unavailable)"),
		.IniKey = "EnableVisualStyle",
		.Value = &ExtConfigs::EnableVisualStyle,
		.Type = ExtConfigs::SpecialOptionType::Restart
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.EnableDarkMode", "Enable dark mode"),
		.IniKey = "EnableDarkMode",
		.Value = &ExtConfigs::EnableDarkMode,
		.Type = ExtConfigs::SpecialOptionType::Restart
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.EnableDarkMode.DimMap", "make map view dim in drak mode"),
		.IniKey = "EnableDarkMode.DimMap",
		.Value = &ExtConfigs::EnableDarkMode_DimMap,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.UseNewToolBarCameo", "Use new tool bar cameo"),
		.IniKey = "UseNewToolBarCameo",
		.Value = &ExtConfigs::UseNewToolBarCameo,
		.Type = ExtConfigs::SpecialOptionType::Restart
		});

	// Object Browser Settings
	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.ObjectBrowser.SafeHouses", "Block invalid houses"),
		.IniKey = "ObjectBrowser.SafeHouses",
		.Value = &ExtConfigs::ObjectBrowser_SafeHouses,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.ObjectBrowser.Foundation", "Group buildings by foundation"),
		.IniKey = "ObjectBrowser.Foundation",
		.Value = &ExtConfigs::ObjectBrowser_Foundation,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.LoadLunarWater", "Show lunar water"),
		.IniKey = "LoadLunarWater",
		.Value = &ExtConfigs::LoadLunarWater,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	// Map Display and Rendering
	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.InGameDisplay.Shadow", "Load and show shadows"),
		.IniKey = "InGameDisplay.Shadow",
		.Value = &ExtConfigs::InGameDisplay_Shadow,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.InGameDisplay.Deploy", "Load and show deploy-swap images"),
		.IniKey = "InGameDisplay.Deploy",
		.Value = &ExtConfigs::InGameDisplay_Deploy,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.InGameDisplay.Water", "Load and show water-swap images"),
		.IniKey = "InGameDisplay.Water",
		.Value = &ExtConfigs::InGameDisplay_Water,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.InGameDisplay.Damage", "Load and show damage-swap images (Phobos B47+)"),
		.IniKey = "InGameDisplay.Damage",
		.Value = &ExtConfigs::InGameDisplay_Damage,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.InGameDisplay.AlphaImage", "Load and show AlphaImage"),
		.IniKey = "InGameDisplay.AlphaImage",
		.Value = &ExtConfigs::InGameDisplay_AlphaImage,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.InGameDisplay.Hover", "Display hovering units higher"),
		.IniKey = "InGameDisplay.Hover",
		.Value = &ExtConfigs::InGameDisplay_Hover,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.InGameDisplay.Bridge", "Display on-bridge units higher"),
		.IniKey = "InGameDisplay.Bridge",
		.Value = &ExtConfigs::InGameDisplay_Bridge,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.InGameDisplay.AnimAdjust", "Adjust building animation layer, may not be consistent with in-game"),
		.IniKey = "InGameDisplay.AnimAdjust",
		.Value = &ExtConfigs::InGameDisplay_AnimAdjust,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.FlatToGroundHideExtra", "Hide extra image when flat-to-ground is enabled"),
		.IniKey = "FlatToGroundHideExtra",
		.Value = &ExtConfigs::FlatToGroundHideExtra,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.DisplayObjectsOutside", "Display objects outside map"),
		.IniKey = "DisplayObjectsOutside",
		.Value = &ExtConfigs::DisplayObjectsOutside,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.ShowMapBoundInMiniMap", "Show map bound in mini-map"),
		.IniKey = "ShowMapBoundInMiniMap",
		.Value = &ExtConfigs::ShowMapBoundInMiniMap,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.Waypoint.Background", "Draw background for waypoint texts"),
		.IniKey = "Waypoint.Background",
		.Value = &ExtConfigs::Waypoint_Background,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.BaseNodeIndex.Background", "Draw background for base node texts"),
		.IniKey = "BaseNodeIndex.Background",
		.Value = &ExtConfigs::BaseNodeIndex_Background,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.RandomTerrainObjects", "Show all terrain objects in random tree dialog"),
		.IniKey = "RandomTerrainObjects",
		.Value = &ExtConfigs::RandomTerrainObjects,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.LightingPreview.MultUnitColor", "Mult unit color when using 'Normal' lighting"),
		.IniKey = "LightingPreview.MultUnitColor",
		.Value = &ExtConfigs::LightingPreview_MultUnitColor,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.LightingPreview.TintTileSetBrowserView", "Mult tile set browser images when changing lighting"),
		.IniKey = "LightingPreview.TintTileSetBrowserView",
		.Value = &ExtConfigs::LightingPreview_TintTileSetBrowserView,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.ShrinkTilesInTileSetBrowser", "Shink tile images in tile set browser"),
		.IniKey = "ShrinkTilesInTileSetBrowser",
		.Value = &ExtConfigs::ShrinkTilesInTileSetBrowser,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.CursorSelectionBound.AutoHeightColor", "Adjust cursor color by height"),
		.IniKey = "CursorSelectionBound.AutoHeightColor",
		.Value = &ExtConfigs::CursorSelectionBound_AutoColor,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	// Map Saving and File Management
	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.SaveMap.AutoSave", "Auto-save map"),
		.IniKey = "SaveMap.AutoSave",
		.Value = &ExtConfigs::SaveMap_AutoSave,
		.Type = ExtConfigs::SpecialOptionType::SaveMap_Timer
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.SaveMap.OnlySaveMAP", "Only save map in .map format"),
		.IniKey = "SaveMap.OnlySaveMAP",
		.Value = &ExtConfigs::SaveMap_OnlySaveMAP,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.SaveMap.BetterMapPreview", "Generate better map preview for MP maps"),
		.IniKey = "SaveMap.BetterMapPreview",
		.Value = &ExtConfigs::SaveMaps_BetterMapPreview,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.SaveMap.FileEncodingComment", "Add file encoding comment in the beginning of map"),
		.IniKey = "SaveMap.FileEncodingComment",
		.Value = &ExtConfigs::SaveMap_FileEncodingComment,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.SaveMap.PreserveINISorting", "Preserve existing INI section sorting when saving"),
		.IniKey = "SaveMap.PreserveINISorting",
		.Value = &ExtConfigs::SaveMap_PreserveINISorting,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.FileWatcher", "Enable file watcher"),
		.IniKey = "FileWatcher",
		.Value = &ExtConfigs::FileWatcher,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.ReloadGameFromMapFolder", "Reload game resources from map folder"),
		.IniKey = "ReloadGameFromMapFolder",
		.Value = &ExtConfigs::ReloadGameFromMapFolder,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.SkipTipsOfTheDay", "Skip tips of the day"),
		.IniKey = "SkipTipsOfTheDay",
		.Value = &ExtConfigs::SkipTipsOfTheDay,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.SkipBrushSizeChangeOnTools", "Skip brush size change when changing tools"),
		.IniKey = "SkipBrushSizeChangeOnTools",
		.Value = &ExtConfigs::SkipBrushSizeChangeOnTools,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.PasteShowOutline", "The default value of showing outline when pasting"),
		.IniKey = "PasteShowOutline",
		.Value = &ExtConfigs::PasteShowOutlineDefault,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.PlaceTileSkipHide", "Skip hidden tiles when placing tiles"),
		.IniKey = "PlaceTileSkipHide",
		.Value = &ExtConfigs::PlaceTileSkipHide,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.INIEditor.IgnoreTeams", "Ignore team sections in INI editor"),
		.IniKey = "INIEditor.IgnoreTeams",
		.Value = &ExtConfigs::INIEditor_IgnoreTeams,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.FillArea.ConsiderLAT", "Also fill LAT tiles when ctrl-filling areas"),
		.IniKey = "FillArea.ConsiderLAT",
		.Value = &ExtConfigs::FillArea_ConsiderLAT,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.FillArea.ConsiderWater", "Consider all water tiles the same when ctrl-filling areas"),
		.IniKey = "FillArea.ConsiderWater",
		.Value = &ExtConfigs::FillArea_ConsiderWater,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.UndoRedo.ShiftPlaceTile", "Only record one history when shift placing tiles"),
		.IniKey = "UndoRedo.ShiftPlaceTile",
		.Value = &ExtConfigs::UndoRedo_ShiftPlaceTile,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.UndoRedo.HoldPlaceOverlay", "Only record one history when shift placing overlays"),
		.IniKey = "UndoRedo.HoldPlaceOverlay",
		.Value = &ExtConfigs::UndoRedo_HoldPlaceOverlay,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.UndoRedo.RecordObjects", "Record editing history of objects"),
		.IniKey = "UndoRedo.RecordObjects",
		.Value = &ExtConfigs::UndoRedo_RecordObjects,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	// Game Logic and Validation
	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.LoadCivilianStringtable", "Load extra civilian stringtable"),
		.IniKey = "LoadCivilianStringtable",
		.Value = &ExtConfigs::LoadCivilianStringtable,
		.Type = ExtConfigs::SpecialOptionType::Restart
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.NoHouseNameTranslation", "Not to translate house names"),
		.IniKey = "NoHouseNameTranslation",
		.Value = &ExtConfigs::NoHouseNameTranslation,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.BetterHouseNameTranslation", "Translate house names in a better way"),
		.IniKey = "BetterHouseNameTranslation",
		.Value = &ExtConfigs::BetterHouseNameTranslation,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.ArtImageSwap", "Use Image= in art(md).ini (Phobos B25+)"),
		.IniKey = "ArtImageSwap",
		.Value = &ExtConfigs::ArtImageSwap,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.AllowIncludes", "Load include INIs (Ares/Phobos B34+)"),
		.IniKey = "AllowIncludes",
		.Value = &ExtConfigs::AllowIncludes,
		.Type = ExtConfigs::SpecialOptionType::Restart
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.AllowInherits", "Load inherited INI sections (Ares/Phobos B34+)"),
		.IniKey = "AllowInherits",
		.Value = &ExtConfigs::AllowInherits,
		.Type = ExtConfigs::SpecialOptionType::Restart
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.AllowPlusEqual", "Load += registries (Ares)"),
		.IniKey = "AllowPlusEqual",
		.Value = &ExtConfigs::AllowPlusEqual,
		.Type = ExtConfigs::SpecialOptionType::Restart
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.IncludeType", "Set include type (Y = Phobos, N = Ares)"),
		.IniKey = "IncludeType",
		.Value = &ExtConfigs::IncludeType,
		.Type = ExtConfigs::SpecialOptionType::Restart
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.InheritType", "Set inherit type (Y = Phobos, N = Ares)"),
		.IniKey = "InheritType",
		.Value = &ExtConfigs::InheritType,
		.Type = ExtConfigs::SpecialOptionType::Restart
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.ExtWaypoints", "Enable infinite waypoints (Phobos B16+)"),
		.IniKey = "ExtWaypoints",
		.Value = &ExtConfigs::ExtWaypoints,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.ExtVariables", "Enable infinite local variables (Phobos B22+)"),
		.IniKey = "ExtVariables",
		.Value = &ExtConfigs::ExtVariables,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.ExtOverlays", "Enable infinite overlay support (Phobos B48+)"),
		.IniKey = "ExtOverlays",
		.Value = &ExtConfigs::ExtOverlays,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.ExtFacings", "Enable 32 facings support"),
		.IniKey = "ExtFacings",
		.Value = &ExtConfigs::ExtFacings,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.ExtFacings.Drag", "Allow drag units into 32 facings"),
		.IniKey = "ExtFacings.Drag",
		.Value = &ExtConfigs::ExtFacings_Drag,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.ExtFacings.DragPreview", "Preview facing while draging"),
		.IniKey = "ExtFacings.DragPreview",
		.Value = &ExtConfigs::ExtFacings_DragPreview,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.ExtMixLoader", "Enable new mix loader"),
		.IniKey = "ExtMixLoader",
		.Value = &ExtConfigs::ExtMixLoader,
		.Type = ExtConfigs::SpecialOptionType::Restart
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.ExtendedValidationNoError", "Extended map validators only show warning instead of error"),
		.IniKey = "ExtendedValidationNoError",
		.Value = &ExtConfigs::ExtendedValidationNoError,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.ExtendedValidationAres", "When checking INI length, use 512 instead of 128 (Ares)"),
		.IniKey = "ExtendedValidationAres",
		.Value = &ExtConfigs::ExtendedValidationAres,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.UseStrictNewTheater", "Use strict NewTheater rule when loading images"),
		.IniKey = "UseStrictNewTheater",
		.Value = &ExtConfigs::UseStrictNewTheater,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.NewTheaterType", "Set NewTheater type (Y = Ares, N = YR)"),
		.IniKey = "NewTheaterType",
		.Value = &ExtConfigs::NewTheaterType,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.PlayerAtXForTechnos", "Show <Player @ X> options in unit property dialog (Phobos B37+)"),
		.IniKey = "PlayerAtXForTechnos",
		.Value = &ExtConfigs::PlayerAtXForTechnos,
		.Type = ExtConfigs::SpecialOptionType::ReloadMap
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.PlayerAtXForTriggers", "Show <Player @ X> options in trigger editor (Phobos B49+)"),
		.IniKey = "PlayerAtXForTriggers",
		.Value = &ExtConfigs::PlayerAtXForTriggers,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.PlaceStructure.OverlappingCheck", "Cannot place overlapped buildings"),
		.IniKey = "PlaceStructure.OverlappingCheck",
		.Value = &ExtConfigs::PlaceStructureOverlappingCheck,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.PlaceStructure.AutoUpgrade", "Auto calculate upgrade count"),
		.IniKey = "PlaceStructure.AutoUpgrade",
		.Value = &ExtConfigs::PlaceStructureUpgrades,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.PlaceStructure.UpgradeStrength", "Set buildings with upgrades to full health (for its appearance in game)"),
		.IniKey = "PlaceStructure.UpgradeStrength",
		.Value = &ExtConfigs::PlaceStructureUpgradeStrength,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.WeaponRangeBound.SubjectToElevation", "Consider effect of cliff when showing weapon range"),
		.IniKey = "WeaponRangeBound.SubjectToElevation",
		.Value = &ExtConfigs::WeaponRangeBound_SubjectToElevation,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.HideNoRubbleBuilding", "Hide buildings with zero health and LeaveRubble=no"),
		.IniKey = "HideNoRubbleBuilding",
		.Value = &ExtConfigs::HideNoRubbleBuilding,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	// Infantry Placement
	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.InfantrySubCell.GameDefault", "Always place the middle infantry in bottom like in-game"),
		.IniKey = "InfantrySubCell.GameDefault",
		.Value = &ExtConfigs::InfantrySubCell_GameDefault,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.InfantrySubCell.Edit", "Edit infantry based on sub-position of mouse"),
		.IniKey = "InfantrySubCell.Edit",
		.Value = &ExtConfigs::InfantrySubCell_Edit,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.InfantrySubCell.Edit.Single", "Edit infantry based on sub-position of mouse in one-infantry-cells"),
		.IniKey = "InfantrySubCell.Edit.Single",
		.Value = &ExtConfigs::InfantrySubCell_Edit_Single,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.InfantrySubCell.Edit.Drag", "Drag infantry based on sub-position of mouse"),
		.IniKey = "InfantrySubCell.Edit.Drag",
		.Value = &ExtConfigs::InfantrySubCell_Edit_Drag,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.InfantrySubCell.Edit.Place", "Place infantry based on sub-position of mouse"),
		.IniKey = "InfantrySubCell.Edit.Place",
		.Value = &ExtConfigs::InfantrySubCell_Edit_Place,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.InfantrySubCell.Edit.FixCenter", "Fix middle infantry with InfantrySubCell.GameDefault"),
		.IniKey = "InfantrySubCell.Edit.FixCenter",
		.Value = &ExtConfigs::InfantrySubCell_Edit_FixCenter,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.InfantrySubCell.OccupationBits", "Consider terrain objects when placing infantry"),
		.IniKey = "InfantrySubCell.OccupationBits",
		.Value = &ExtConfigs::InfantrySubCell_OccupationBits,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	// Performance and System Settings
	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.DDrawInVideoMem", "Arrange DirectDraw surface in video memory"),
		.IniKey = "DDrawInVideoMem",
		.Value = &ExtConfigs::DDrawInVideoMem,
		.Type = ExtConfigs::SpecialOptionType::Restart
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.DDrawEmulation", "Use emulation mode in DirectDrawCreate"),
		.IniKey = "DDrawEmulation",
		.Value = &ExtConfigs::DDrawEmulation,
		.Type = ExtConfigs::SpecialOptionType::Restart
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.DDrawScalingBilinear", "Use bilinear scaling (smoother, but slower)"),
		.IniKey = "DDrawScalingBilinear",
		.Value = &ExtConfigs::DDrawScalingBilinear,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.DDrawScalingBilinear.OnlyShrink", "Use bilinear scaling only in shrinking"),
		.IniKey = "DDrawScalingBilinear.OnlyShrink",
		.Value = &ExtConfigs::DDrawScalingBilinear_OnlyShrink,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.LoadImageDataFromServer", "Load images from independent server (not rec., unless memory shortage)"),
		.IniKey = "LoadImageDataFromServer",
		.Value = &ExtConfigs::LoadImageDataFromServer,
		.Type = ExtConfigs::SpecialOptionType::Restart
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.StringBufferStackAllocation", "Always allocate CString memory in stack"),
		.IniKey = "StringBufferStackAllocation",
		.Value = &ExtConfigs::StringBufferStackAllocation,
		.Type = ExtConfigs::SpecialOptionType::None
		});

	ExtConfigs::Options.push_back(ExtConfigs::DynamicOptions{
		.DisplayName = Translations::TranslateOrDefault("Options.StrictExceptionFilter", "Use strict exception filter (catch C++ EH exceptions)"),
		.IniKey = "StrictExceptionFilter",
		.Value = &ExtConfigs::StrictExceptionFilter,
		.Type = ExtConfigs::SpecialOptionType::Restart
		});


	CINI fa2;
	std::string path;
	path = CFinalSunApp::ExePath;
	path += "\\FinalAlert.ini";
	fa2.ClearAndLoad(path.c_str());

	for (const auto& opt : ExtConfigs::Options)
	{
		*opt.Value = fa2.GetBool("Options", opt.IniKey, *opt.Value);
	}

	CIsoViewExt::PasteShowOutline = ExtConfigs::PasteShowOutlineDefault;

}

// DllMain
BOOL APIENTRY DllMain(HANDLE hInstance, DWORD dwReason, LPVOID v)
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		FA2sp::hInstance = hInstance;
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

bool DetachFromDebugger();
DEFINE_HOOK(537129, ExeRun, 9)
{
	Logger::Initialize();
	Logger::Info(APPLY_INFO);
	Logger::Wrap(1);

	Logger::Raw("==============================\nCPU Report:\n%s==============================\n", 
		InstructionSet::Report().c_str());

	ExtConfigs::AVX2_Support = InstructionSet::AVX2();

	if (DetachFromDebugger())
		Logger::Info("Syringe detached!\n");
	else
		Logger::Warn("Failed to detach Syringe!\n");

#ifndef NDEBUG
	MessageBox(NULL, APPLY_INFO, PRODUCT_NAME, MB_OK);
	
#endif
	bool bMutexResult = MutexHelper::Attach(MUTEX_HASH_VAL);
	if (!bMutexResult)
	{
		if (MessageBox(nullptr, MUTEX_INIT_ERROR_MSG, MUTEX_INIT_ERROR_TIT, MB_YESNO | MB_ICONQUESTION) != IDYES)
			ExitProcess(114514);
	}
	
	FA2Expand::ExeRun();

	return 0;
}

#include <CLoading.h>

DEFINE_HOOK(47FACE, CLoading_OnInitDialog_ProgramInfo, 7)
{
	GET(CLoading*, pThis, ESI);

	pThis->CSCVersion.SetWindowText(LOADING_VERSION);
	pThis->CSCBuiltby.SetWindowText(LOADING_AUTHOR);
	pThis->SetDlgItemText(1300, LOADING_WEBSITE);

	return 0;
}

#include "Ext/CFinalSunDlg/Body.h"

DEFINE_HOOK(537208, ExeTerminate, 9)
{
	MutexHelper::Detach();
	Logger::Info("FA2sp Terminating...\n");
	Logger::Close();
	VoxelDrawer::Finalize();

	// Destruct static ppmfc stuffs here
	CViewObjectsExt::OnExeTerminate();

	if (ExtConfigs::EnableVisualStyle)
	{
		::DeactivateActCtx(NULL, FA2sp::ulCookie);
		::DeactivateActCtx(NULL, FA2sp::ulCookieEx);
	}

	GET(UINT, result, EAX);
	ExitProcess(result);
}

#ifdef _DEBUG
// Just for test, lol
//DEFINE_HOOK(43273B, ExitMessageBox, 8)
//{
//	R->EAX(MB_OK);
//	return 0x432743;
//}
#endif

#include <tlhelp32.h>
// https://stackoverflow.com/questions/70458828
bool DetachFromDebugger()
{
	using NTSTATUS = LONG;

	auto GetDebuggerProcessId = [](DWORD dwSelfProcessId) -> DWORD
	{
		DWORD dwParentProcessId = -1;
		HANDLE hSnapshot = CreateToolhelp32Snapshot(2, 0);
		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);
		Process32First(hSnapshot, &pe32);
		do
		{
			if (pe32.th32ProcessID == dwSelfProcessId)
			{
				dwParentProcessId = pe32.th32ParentProcessID;
				break;
			}
		} while (Process32Next(hSnapshot, &pe32));
		CloseHandle(hSnapshot);
		return dwParentProcessId;
	};

	HMODULE hModule = LoadLibrary("ntdll.dll");
	if (hModule != NULL)
	{
		auto const NtRemoveProcessDebug =
			(NTSTATUS(__stdcall*)(HANDLE, HANDLE))GetProcAddress(hModule, "NtRemoveProcessDebug");
		auto const NtSetInformationDebugObject =
			(NTSTATUS(__stdcall*)(HANDLE, ULONG, PVOID, ULONG, PULONG))GetProcAddress(hModule, "NtSetInformationDebugObject");
		auto const NtQueryInformationProcess =
			(NTSTATUS(__stdcall*)(HANDLE, ULONG, PVOID, ULONG, PULONG))GetProcAddress(hModule, "NtQueryInformationProcess");
		auto const NtClose =
			(NTSTATUS(__stdcall*)(HANDLE))GetProcAddress(hModule, "NtClose");

		HANDLE hDebug;
		HANDLE hCurrentProcess = GetCurrentProcess();
		NTSTATUS status = NtQueryInformationProcess(hCurrentProcess, 30, &hDebug, sizeof(HANDLE), 0);
		if (0 <= status)
		{
			ULONG killProcessOnExit = FALSE;
			status = NtSetInformationDebugObject(
				hDebug,
				1,
				&killProcessOnExit,
				sizeof(ULONG),
				NULL
			);
			if (0 <= status)
			{
				const auto pid = GetDebuggerProcessId(GetProcessId(hCurrentProcess));
				status = NtRemoveProcessDebug(hCurrentProcess, hDebug);
				if (0 <= status)
				{
					HANDLE hDbgProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
					if (INVALID_HANDLE_VALUE != hDbgProcess)
					{
						BOOL ret = TerminateProcess(hDbgProcess, EXIT_SUCCESS);
						CloseHandle(hDbgProcess);
						return ret;
					}
				}
			}
			NtClose(hDebug);
		}
		FreeLibrary(hModule);
	}

	return false;
}