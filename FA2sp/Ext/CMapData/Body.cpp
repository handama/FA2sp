#include "Body.h"

#include "../../Miscs/SaveMap.h"

#include <CFinalSunApp.h>
#include <CFinalSunDlg.h>
#include <corecrt_math_defines.h>
#include <algorithm>
#include <vector>
#include <format>
#include "../../Helpers/STDHelpers.h"
#include <Miscs/Miscs.h>
#include <CIsoView.h>
#include "../CFinalSunDlg/Body.h"
#include "../../ExtraWindow/CNewTeamTypes/CNewTeamTypes.h"
#include "../../ExtraWindow/CNewTaskforce/CNewTaskforce.h"
#include "../../ExtraWindow/CNewScript/CNewScript.h"
#include "../../ExtraWindow/CNewTrigger/CNewTrigger.h"
#include "../../ExtraWindow/CNewINIEditor/CNewINIEditor.h"
#include "../../ExtraWindow/CSearhReference/CSearhReference.h"
#include "../../ExtraWindow/CCsfEditor/CCsfEditor.h"
#include "../../ExtraWindow/CNewAITrigger/CNewAITrigger.h"
#include "../../ExtraWindow/CLuaConsole/CLuaConsole.h"
#include "../../ExtraWindow/CNewLocalVariables/CNewLocalVariables.h"
#include "../CTileSetBrowserFrame/TabPages/TriggerSort.h"
#include "../CTileSetBrowserFrame/TabPages/TeamSort.h"
#include "../CTileSetBrowserFrame/TabPages/WaypointSort.h"
#include "../CTileSetBrowserFrame/TabPages/TaskForceSort.h"
#include "../CTileSetBrowserFrame/TabPages/ScriptSort.h"
#include "../../Miscs/MultiSelection.h"
#include "../../Helpers/Translations.h"
#include "../CIsoView/Body.h"
#include "../CTileSetBrowserFrame/TabPages/TagSort.h"
#include "../../Miscs/Palettes.h"
#include "../CLoading/Body.h"
#include "../../Miscs/Hooks.INI.h"
#include <unordered_set>
#include "../../Miscs/TheaterInfo.h"
#include "../../ExtraWindow/CTriggerAnnotation/CTriggerAnnotation.h"
#include <random>

int CMapDataExt::OreValue[4] { -1,-1,-1,-1 };
unsigned short CMapDataExt::CurrentRenderBuildingStrength;
std::vector<BuildingRenderData> CMapDataExt::BuildingRenderDatasFix;
std::vector<OverlayTypeData> CMapDataExt::OverlayTypeDatas;
CellDataExt CMapDataExt::CellDataExt_FindCell;
std::vector<CellDataExt> CMapDataExt::CellDataExts;
CellData CMapDataExt::ExtTempCellData;
//MapCoord CMapDataExt::CurrentMapCoord;
MapCoord CMapDataExt::CurrentMapCoordPaste;
std::unordered_map<int, BuildingDataExt> CMapDataExt::BuildingDataExts;
std::unordered_map<FString, int> CMapDataExt::BuildingTypes;
std::map<int, MapCoord> CMapDataExt::BuildingCenterCoords;
CTileTypeClass* CMapDataExt::TileData = nullptr;
int CMapDataExt::TileDataCount = 0;
int CMapDataExt::CurrentTheaterIndex;
int CMapDataExt::PaveTile;
int CMapDataExt::GreenTile;
int CMapDataExt::MiscPaveTile;
int CMapDataExt::Medians;
int CMapDataExt::PavedRoads;
int CMapDataExt::ShorePieces;
int CMapDataExt::WaterBridge;
int CMapDataExt::BridgeSet;
int CMapDataExt::WoodBridgeSet;
int CMapDataExt::HeightBase;
int CMapDataExt::AutoShore_ShoreTileSet;
int CMapDataExt::AutoShore_GreenTileSet;
float CMapDataExt::ConditionYellow = 0.5f;
float CMapDataExt::ConditionRed = 0.25f;
bool CMapDataExt::DeleteBuildingByIniID = false;
std::unordered_set<int> CMapDataExt::ShoreTileSets;
std::unordered_map<int, bool> CMapDataExt::SoftTileSets;
FString CMapDataExt::BitmapImporterTheater;
Palette CMapDataExt::Palette_ISO;
Palette CMapDataExt::Palette_ISO_NoTint;
Palette CMapDataExt::Palette_Shadow;
Palette CMapDataExt::Palette_AlphaImage;
std::vector<std::pair<LightingSourcePosition, LightingSource>> CMapDataExt::LightingSources;
std::vector<std::vector<int>> CMapDataExt::Tile_to_lat;
std::vector<int> CMapDataExt::TileSet_starts;
std::unordered_map<FString, std::shared_ptr<Trigger>> CMapDataExt::Triggers;
std::vector<short> CMapDataExt::StructureIndexMap;
std::vector<TubeData> CMapDataExt::Tubes;
std::unordered_map<int, TileAnimation> CMapDataExt::TileAnimations;
std::unordered_map<int, FString> CMapDataExt::TileSetOriginSetNames[6];
std::unordered_set<FString> CMapDataExt::TerrainPaletteBuildings;
std::unordered_set<FString> CMapDataExt::DamagedAsRubbleBuildings;
std::unordered_set<int> CMapDataExt::RedrawExtraTileSets;
std::unordered_map<int, Palette*> CMapDataExt::TileSetPalettes;
int CMapDataExt::NewINIFormat = 4;
WORD CMapDataExt::NewOverlay[0x40000] = {0xFFFF};
HistoryList CMapDataExt::UndoRedoDatas;
HistoryList CMapDataExt::PreviewHistoryData;
bool CMapDataExt::RecordingPreviewHistory = false;
int CMapDataExt::UndoRedoDataIndex;
bool CMapDataExt::IsLoadingMapFile = false;
bool CMapDataExt::IsMMXFile = false;
bool CMapDataExt::IsUTF8File = false;
bool CMapDataExt::SkipBuildingOverlappingCheck = false;
std::vector<FString> CMapDataExt::MapIniSectionSorting;
std::map<FString, std::set<FString>> CMapDataExt::PowersUpBuildings;
std::set<FString> CMapDataExt::PowersUpBuildingSet;
std::map<int, std::vector<CustomTile>> CMapDataExt::CustomTiles;
bool CMapDataExt::PlaceStructure_Preview = false;
std::map<int, BuildingRenderData> CMapDataExt::PlaceStructure_OldData;
std::map<FString, COLORREF> CMapDataExt::CustomWaypointColors;
std::map<FString, COLORREF> CMapDataExt::CustomCelltagColors;
ObjectRecord* ObjectRecord::ObjectRecord_HoldingPtr = nullptr;
std::map<FString, std::vector<TechnoAttachment>> CMapDataExt::TechnoAttachments;
std::map<FString, std::map<FString, FString>> CMapDataExt::MapInlineComments;
std::map<FString, std::map<FString, FString>> CMapDataExt::MapFrontlineComments;
std::map<FString, FString> CMapDataExt::MapInsectionComments;
std::map<FString, FString> CMapDataExt::MapFrontsectionComments;
bool CMapDataExt::IsNewMap;

static inline int DistSqrByIndex(int a, int b)
{
	auto& Map = CMapData::Instance;
	int dx = Map->GetXFromCoordIndex(a) - Map->GetXFromCoordIndex(b);
	int dy = Map->GetYFromCoordIndex(a) - Map->GetYFromCoordIndex(b);
	return dx * dx + dy * dy;
}

void CMapDataExt::RemapableOverlay_RefreshBuildingIndices()
{
	if (!ExtConfigs::InGameDisplay_RemapableOverlay)
		return;

	auto& Map = CMapData::Instance;
	const int MAP_W = Map->MapWidthPlusHeight;
	const int MAP_H = Map->MapWidthPlusHeight;
	const int MAP_SIZE = MAP_W * MAP_H;
	const int INF = INT_MAX;

	for (auto& cell : CellDataExts) {
		cell.CenterBuildingIndex = -1;
		cell.NearestCenterCellIndex = -1;
	}

	for (const auto& [buildingIndex, coord] : BuildingCenterCoords) {
		int centerIdx = Map->GetCoordIndex(coord.X, coord.Y);
		if (centerIdx < 0 || centerIdx >= MAP_SIZE)
			continue;

		auto& cell = CellDataExts[centerIdx];
		cell.CenterBuildingIndex = buildingIndex;
		cell.NearestCenterCellIndex = centerIdx;
	}

	for (int y = 0; y < MAP_H; ++y) {
		for (int x = 0; x < MAP_W; ++x) {
			int idx = y * MAP_W + x;
			if (idx >= MAP_SIZE)
				continue;

			auto& cur = CellDataExts[idx];
			if (cur.NearestCenterCellIndex != -1)
				continue;

			int bestCenter = -1;
			int bestDist = INF;

			if (x > 0)
				RemapableOverlay_CheckNeighbor(idx, idx - 1, bestCenter, bestDist);
			if (y > 0)
				RemapableOverlay_CheckNeighbor(idx, idx - MAP_W, bestCenter, bestDist);

			if (bestCenter != -1) {
				cur.NearestCenterCellIndex = bestCenter;
				cur.CenterBuildingIndex =
					CellDataExts[bestCenter].CenterBuildingIndex;
			}
		}
	}

	for (int y = MAP_H - 1; y >= 0; --y) {
		for (int x = MAP_W - 1; x >= 0; --x) {
			int idx = y * MAP_W + x;
			if (idx >= MAP_SIZE)
				continue;

			auto& cur = CellDataExts[idx];

			int curDist = (cur.NearestCenterCellIndex == -1)
				? INF
				: DistSqrByIndex(idx, cur.NearestCenterCellIndex);

			int bestCenter = cur.NearestCenterCellIndex;
			int bestDist = curDist;

			if (x + 1 < MAP_W)
				RemapableOverlay_CheckNeighbor(idx, idx + 1, bestCenter, bestDist);
			if (y + 1 < MAP_H)
				RemapableOverlay_CheckNeighbor(idx, idx + MAP_W, bestCenter, bestDist);

			if (bestCenter != cur.NearestCenterCellIndex) {
				cur.NearestCenterCellIndex = bestCenter;
				cur.CenterBuildingIndex =
					CellDataExts[bestCenter].CenterBuildingIndex;
			}
		}
	}

	for (auto& cell : CellDataExts) {
		if (cell.CenterBuildingIndex == -1)
			continue;

		int strINI = StructureIndexMap[cell.CenterBuildingIndex];
		if (strINI >= 0) {
			cell.RemapableColor =
				BuildingRenderDatasFix[strINI].HouseColor;
		}
	}
}

void CMapDataExt::RemapableOverlay_CheckNeighbor(
	int currentIdx,
	int neighborIdx,
	int& bestCenterCellIdx,
	int& bestDistSqr)
{
	if (neighborIdx < 0 || neighborIdx >= (int)CellDataExts.size())
		return;

	const auto& neighbor = CellDataExts[neighborIdx];
	if (neighbor.NearestCenterCellIndex == -1)
		return;

	int dist = DistSqrByIndex(currentIdx, neighbor.NearestCenterCellIndex);
	if (dist < bestDistSqr) {
		bestDistSqr = dist;
		bestCenterCellIdx = neighbor.NearestCenterCellIndex;
	}
}

void CMapDataExt::RemapableOverlay_AddBuilding(int buildingIndex, const MapCoord& center)
{
	BuildingCenterCoords[buildingIndex] = center;
	RemapableOverlay_RefreshBuildingIndices();
}

void CMapDataExt::RemapableOverlay_RemoveBuilding(int buildingIndex)
{
	auto it = BuildingCenterCoords.find(buildingIndex);
	if (it != BuildingCenterCoords.end()) {
		BuildingCenterCoords.erase(it);
		RemapableOverlay_RefreshBuildingIndices();
	}
}

int CMapDataExt::GetOreValue(unsigned short nOverlay, unsigned char nOverlayData)
{
    if (nOverlay >= RIPARIUS_BEGIN && nOverlay <= RIPARIUS_END)
        return nOverlayData * OreValue[OreType::Riparius];
    else if (nOverlay >= CRUENTUS_BEGIN && nOverlay <= CRUENTUS_END)
        return nOverlayData * OreValue[OreType::Cruentus];
    else if (nOverlay >= VINIFERA_BEGIN && nOverlay <= VINIFERA_END)
        return nOverlayData * OreValue[OreType::Vinifera];
    else if (nOverlay >= ABOREUS_BEGIN && nOverlay <= ABOREUS_END)
        return nOverlayData * OreValue[OreType::Aboreus];
    else
        return 0;
}

bool CMapDataExt::IsOre(unsigned short nOverlay)
{
	if (nOverlay >= RIPARIUS_BEGIN && nOverlay <= RIPARIUS_END)
		return true;
    else if (nOverlay >= CRUENTUS_BEGIN && nOverlay <= CRUENTUS_END)
        return true;
    else if (nOverlay >= VINIFERA_BEGIN && nOverlay <= VINIFERA_END)
        return true;
    else if (nOverlay >= ABOREUS_BEGIN && nOverlay <= ABOREUS_END)
        return true;

    return false;
}

int CMapDataExt::GetOreValueAt(CellData& cell)
{
    return GetOreValue(cell.Overlay, cell.OverlayData);
}

void CMapDataExt::ProcessBuildingType(const char* ID)
{
	int idx = this->GetBuildingTypeID(ID);
	auto& DataExt = this->BuildingDataExts[idx];
	this->BuildingTypes[ID] = idx;
	DataExt.BottomCoords.clear();
	DataExt.DamageFireOffsets.clear();

	ppmfc::CString ImageID = Variables::RulesMap.GetString(ID, "Image", ID);
	auto foundation = CINI::Art->GetString(ImageID, "Foundation", "1x1");

	// https://modenc.renegadeprojects.com/Foundation
	// This flag is read from art(md).ini twice: 
	// from section you specified in rules as [object]б·Image, 
	// and from simply [object] , 
	// and the second one overrules the first if present. 
	// however, ares's custom foundation doesn't have this bug.
	if (_strcmpi(foundation, "Custom"))
	{
		foundation = CINI::Art->GetString(ID, "Foundation", foundation);
		ImageID = ID;
	}

	if (_strcmpi(foundation, "Custom") && _strcmpi(foundation, "3x3REFINERY"))
	{
		auto sizes = STDHelpers::SplitStringMultiSplit(foundation, "x|X");
		if (sizes.size() >= 2)
		{
			DataExt.Width = atoi(sizes[0]);
			DataExt.Height = atoi(sizes[1]);
		}
		else
		{
			DataExt.Width = 1;
			DataExt.Height = 1;
		}
		if (DataExt.Width == 0)
			DataExt.Width = 1;
		if (DataExt.Height == 0)
			DataExt.Height = 1;
	}
	else
	{
		auto ParsePoint = [](const char* str)
		{
			int x = 0, y = 0;
			switch (sscanf_s(str, "%d,%d", &x, &y))
			{
			case 0:
				x = 0;
				y = 0;
				break;
			case 1:
				y = 0;
				break;
			case 2:
				break;
			default:
				__assume(0);
			}
			return MapCoord{ x,y };
		};

		if (_strcmpi(foundation, "3x3REFINERY"))
		{
			// Custom, code reference Ares
			DataExt.Width = CINI::Art->GetInteger(ImageID, "Foundation.X", 0);
			DataExt.Height = CINI::Art->GetInteger(ImageID, "Foundation.Y", 0);
			if (DataExt.Width == 0)
				DataExt.Width = 1;
			if (DataExt.Height == 0)
				DataExt.Height = 1;
			DataExt.Foundations = new std::vector<MapCoord>;
			for (int i = 0; i < DataExt.Width * DataExt.Height; ++i)
			{
				ppmfc::CString key;
				key.Format("Foundation.%d", i);
				if (auto pPoint = CINI::Art->TryGetString(ImageID, key)) {
					DataExt.Foundations->push_back(ParsePoint(*pPoint));
				}
				else
					break;
			}
		}
		else
		{
			DataExt.Width = 3;
			DataExt.Height = 3;
			DataExt.Foundations = new std::vector<MapCoord>;
			DataExt.Foundations->push_back({ 0,0 });
			DataExt.Foundations->push_back({ 1,0 });
			DataExt.Foundations->push_back({ 2,0 });
			DataExt.Foundations->push_back({ 0,1 });
			DataExt.Foundations->push_back({ 1,1 });
			DataExt.Foundations->push_back({ 0,2 });
			DataExt.Foundations->push_back({ 1,2 });
			DataExt.Foundations->push_back({ 2,2 });
		}

		// Build outline draw data
		DataExt.LinesToDraw = new std::vector<std::pair<MapCoord, MapCoord>>;
		std::vector<std::vector<BOOL>> LinesX, LinesY;

		LinesX.resize(DataExt.Width);
		for (auto& l : LinesX)
			l.resize(DataExt.Height + 1);
		LinesY.resize(DataExt.Width + 1);
		for (auto& l : LinesY)
			l.resize(DataExt.Height);

		for (const auto& block : *DataExt.Foundations)
		{
			LinesX[block.X][block.Y] = !LinesX[block.X][block.Y];
			LinesX[block.X][block.Y + 1] = !LinesX[block.X][block.Y + 1];
			LinesY[block.X][block.Y] = !LinesY[block.X][block.Y];
			LinesY[block.X + 1][block.Y] = !LinesY[block.X + 1][block.Y];
		}

		for (size_t y = 0; y < DataExt.Height + 1; ++y)
		{
			size_t length = 0;
			for (size_t x = 0; x < DataExt.Width; ++x)
			{
				if (LinesX[x][y])
					++length;
				else
				{
					if (!length)
						continue;
					MapCoord start, end;
					start.X = ((x - length) - y) * 30;
					start.Y = ((x - length) + y) * 15;
					end.X = (x - y) * 30 + 2;
					end.Y = (x + y) * 15 + 1;
					DataExt.LinesToDraw->push_back(std::make_pair(start, end));
					length = 0;
				}
			}
			if (length)
			{
				MapCoord start, end;
				start.X = ((DataExt.Width - length) - y) * 30;
				start.Y = ((DataExt.Width - length) + y) * 15;
				end.X = (DataExt.Width - y) * 30 + 2;
				end.Y = (DataExt.Width + y) * 15 + 1;
				DataExt.LinesToDraw->push_back(std::make_pair(start, end));
			}
		}

		for (size_t x = 0; x < DataExt.Width + 1; ++x)
		{
			size_t length = 0;
			for (size_t y = 0; y < DataExt.Height; ++y)
			{
				if (LinesY[x][y])
					++length;
				else
				{
					if (!length)
						continue;
					MapCoord start, end;
					start.X = (x - (y - length)) * 30;
					start.Y = (x + (y - length)) * 15;
					end.X = (x - y) * 30;
					end.Y = (x + y) * 15;
					DataExt.LinesToDraw->push_back(std::make_pair(start, end));
					length = 0;
				}
			}
			if (length)
			{
				MapCoord start, end;
				start.X = (x - (DataExt.Height - length)) * 30;
				start.Y = (x + (DataExt.Height - length)) * 15;
				end.X = (x - DataExt.Height) * 30;
				end.Y = (x + DataExt.Height) * 15;
				DataExt.LinesToDraw->push_back(std::make_pair(start, end));
			}
		}
	}

	// other art flags don't have save problem
	ImageID = Variables::RulesMap.GetString(ID, "Image", ID);
	for (int i = 0; i < 8; ++i)
	{
		FString key;
		key.Format("DamageFireOffset%d", i);
		if (CINI::Art->KeyExists(ImageID, key))
		{
			auto atoms = STDHelpers::SplitString(CINI::Art->GetString(ImageID, key, "0,0"), 1);
			DataExt.DamageFireOffsets.push_back({ atoi(atoms[0]),atoi(atoms[1]) });
		}
		else
		{
			break;
		}
	}

	DataExt.BottomCoords.reserve(std::max(1, DataExt.Width + DataExt.Height - 1));
	for (int x = 0; x < std::max(1, DataExt.Width + DataExt.Height - 1); ++x)
	{
		if (!DataExt.IsCustomFoundation())
		{
			if (x < DataExt.Width)
				DataExt.BottomCoords.emplace_back(DataExt.Height - 1, x);
			else
				DataExt.BottomCoords.emplace_back(DataExt.Height + DataExt.Width - 2 - x, DataExt.Width - 1);
		}
		else
		{
			auto& coords = *DataExt.Foundations;
			MapCoord coord = { 0,0 };
			if (x < DataExt.Width)
				coord = { DataExt.Height - 1, x };
			else
				coord = { DataExt.Height + DataExt.Width - 2 - x, DataExt.Width - 1 };

			int loopX = x;
			while (std::find(coords.begin(), coords.end(), MapCoord{ coord.Y, coord.X }) == coords.end())
			{
				if (coord.X <= 0 || coord.Y <= 0)
				{
					if (std::find(coords.begin(), coords.end(), MapCoord{ coord.Y, coord.X }) == coords.end())
					{
						if (loopX < DataExt.Width)
						{
							loopX++;
							coord = { DataExt.Height, loopX + 1 };
						}
						else if (loopX > DataExt.Width)
						{
							loopX--;
							coord = { DataExt.Height + DataExt.Width - 1 - loopX, DataExt.Width };
						}
						else
						{
							break;
						}
					}
					else
					{
						break;
					}
				}
				coord.X--;
				coord.Y--;
			}
			DataExt.BottomCoords.emplace_back(coord);
		}
	}
}

BuildingPowers CMapDataExt::GetStructurePower(CBuildingData object)
{
	BuildingPowers ret{};
	if (object.PoweredOn == "0")
		return ret;
	auto roundToPrecision = [](double value, int precision)
		{
			double multiplier = std::pow(10.0, precision);
			return std::round(value * multiplier) / multiplier;
		};
	int strength = (int)((double)Variables::RulesMap.GetInteger(object.TypeID, "Strength") * (double)(atoi(object.Health) / 256.0));
	if (strength == 0 && atoi(object.Health) > 0)
		strength = 1;

	int power1 = 0;
	int power2 = 0;
	int power3 = 0;
	int powerMain = Variables::RulesMap.GetInteger(object.TypeID, "Power");
	if (powerMain > 0)
		powerMain = ((double)powerMain) * roundToPrecision(((double)strength / (double)Variables::RulesMap.GetInteger(object.TypeID, "Strength")), 5);

	if (object.Upgrade1 != "None" && atoi(object.Upgrades) >= 1)
	{
		power1 = Variables::RulesMap.GetInteger(object.Upgrade1, "Power");
		if (power1 > 0)
			power1 = ((double)power1) * roundToPrecision(((double)strength / (double)Variables::RulesMap.GetInteger(object.TypeID, "Strength")), 5);
	}
	if (object.Upgrade2 != "None" && atoi(object.Upgrades) >= 2)
	{
		power2 = Variables::RulesMap.GetInteger(object.Upgrade2, "Power");
		if (power2 > 0)
			power2 = ((double)power2) * roundToPrecision(((double)strength / (double)Variables::RulesMap.GetInteger(object.TypeID, "Strength")), 5);
	}
	if (object.Upgrade3 != "None" && atoi(object.Upgrades) >= 3)
	{
		power3 = Variables::RulesMap.GetInteger(object.Upgrade3, "Power");
		if (power3 > 0)
			power3 = ((double)power3) * roundToPrecision(((double)strength / (double)Variables::RulesMap.GetInteger(object.TypeID, "Strength")), 5);
	}
	ret.TotalPower = powerMain + power1 + power2 + power3;
	ret.Output = (powerMain > 0 ? powerMain : 0) 
		+ (power1 > 0 ? power1 : 0)
		+ (power2 > 0 ? power2 : 0)
		+ (power3 > 0 ? power3 : 0);
	ret.Drain = (powerMain < 0 ? powerMain : 0)
		+ (power1 < 0 ? power1 : 0)
		+ (power2 < 0 ? power2 : 0)
		+ (power3 < 0 ? power3 : 0);
	return ret;
}

BuildingPowers CMapDataExt::GetStructurePower(ppmfc::CString value)
{
	auto atoms = STDHelpers::SplitString(value, 16);
	CBuildingData object;
	object.TypeID = atoms[1];
	object.Health = atoms[2];
	object.Upgrades = atoms[10];
	object.Upgrade1 = atoms[12];
	object.Upgrade2 = atoms[13];
	object.Upgrade3 = atoms[14];
	object.PoweredOn = atoms[9];
	return GetStructurePower(object);
}

void CMapDataExt::GetBuildingDataByIniID(int bldID, CBuildingData& data)
{
	auto atoms = STDHelpers::SplitString(CINI::CurrentDocument->GetValueAt("Structures", bldID), 16);
	data.House = atoms[0];
	data.TypeID = atoms[1];
	data.Health = atoms[2];
	data.Y = atoms[3];
	data.X = atoms[4];
	data.Facing = atoms[5];
	data.Tag = atoms[6];
	data.AISellable = atoms[7];
	data.AIRebuildable = atoms[8];
	data.PoweredOn = atoms[9];
	data.Upgrades = atoms[10];
	data.SpotLight = atoms[11];
	data.Upgrade1 = atoms[12];
	data.Upgrade2 = atoms[13];
	data.Upgrade3 = atoms[14];
	data.AIRepairable = atoms[15];
	data.Nominal = atoms[16];
}

int CMapDataExt::GetSafeTileIndex(int idx)
{
	if (idx >= CMapDataExt::TileDataCount)
		idx = 0;
	return idx;
}

void CMapDataExt::UpdateTriggers()
{
	CMapDataExt::Triggers.clear();
	std::map<FString, FString> TagMap;
	if (auto pSection = CINI::CurrentDocument().GetSection("Tags"))
	{
		for (auto& kvp : pSection->GetEntities())
		{
			auto tagAtoms = FString::SplitString(kvp.second);
			if (tagAtoms.size() < 3) continue;
			if (TagMap.find(tagAtoms[2]) == TagMap.end())
				TagMap[tagAtoms[2]] = kvp.first;
		}
	}
	if (auto pSection = CINI::CurrentDocument->GetSection("Triggers")) {
		for (const auto& pair : pSection->GetEntities()) {
			std::shared_ptr<Trigger> trigger(Trigger::create(pair.first, &TagMap));
			if (!trigger) {
				continue;
			}
			if (CMapDataExt::Triggers.find(pair.first) == CMapDataExt::Triggers.end()) {
				CMapDataExt::Triggers[pair.first] = std::move(trigger);
			}
		}
	}    
	if (TriggerSort::Instance.IsVisible())
	{
		TriggerSort::Instance.LoadAllTriggers();
	}
}

FString CMapDataExt::AddTrigger(std::shared_ptr<Trigger> trigger) {
	if (!trigger) {
		return "";
	}
	FString id = trigger->ID;
	CMapDataExt::Triggers[id] = std::move(trigger);
	return id;
}

FString CMapDataExt::AddTrigger(FString id) {
	std::shared_ptr<Trigger> trigger(Trigger::create(id));

	if (!trigger) {
		return "";
	}

	CMapDataExt::Triggers[id] = std::move(trigger);
	return id;
}

std::shared_ptr<Trigger> CMapDataExt::GetTrigger(FString id) {
	auto it = CMapDataExt::Triggers.find(id);
	if (it != CMapDataExt::Triggers.end()) {
		return std::shared_ptr<Trigger>(it->second.get(), [](Trigger*) {});
	}
	return nullptr;
}

void CMapDataExt::DeleteTrigger(FString id)
{
	auto it = CMapDataExt::Triggers.find(id);
	if (it != CMapDataExt::Triggers.end()) {
		CMapDataExt::Triggers.erase(it);
	}
}

bool CMapDataExt::IsTileIntact(int x, int y, int startX, int startY, int right, int bottom)
{
	if (!this->IsCoordInMap(x, y))
		return false;
	int pos = this->GetCoordIndex(x, y);
	auto cell = this->GetCellAt(pos);
	int tileIndex = cell->TileIndex;
	if (tileIndex == 0xFFFF)
		tileIndex = 0;

	int oriX = x - cell->TileSubIndex / CMapDataExt::TileData[tileIndex].Width;
	int oriY = y - cell->TileSubIndex % CMapDataExt::TileData[tileIndex].Width;

	int subIdx = 0;
	for (int m = 0; m < CMapDataExt::TileData[tileIndex].Height; m++)
	{
		for (int n = 0; n < CMapDataExt::TileData[tileIndex].Width; n++)
		{
			if (!this->IsCoordInMap(m + oriX, n + oriY))
				return false;
			if (startX >= 0)
				if (m + oriX < startX || n + oriY < startY || m + oriX >= right || n + oriY >= bottom)
					return false;

			auto cell2 = this->GetCellAt(m + oriX, n + oriY);
			int tileIndex2 = cell2->TileIndex;
			if (tileIndex2 == 0xFFFF)
				tileIndex2 = 0;

			if (CMapDataExt::TileData[tileIndex].TileBlockDatas[subIdx].ImageData != NULL)
			{
				if (tileIndex != tileIndex2)
					return false;

				if (cell2->TileSubIndex != subIdx)
					return false;
			}

			subIdx++;
		}
	}

	return true;

}

void CMapDataExt::SetHeightAt(int x, int y, int height)
{
	if (ExtConfigs::PlaceTileSkipHide)
	{
		const auto cell = this->TryGetCellAt(x, y);
		if (cell->IsHidden())
			return;
	}

	if (height < 0) height = 0;
	if (height > 14) height = 14;
	if (this->IsCoordInFullMap(x, y))
		this->CellDatas[x + y * this->MapWidthPlusHeight].Height = height;
}

void CMapDataExt::PlaceTileAt(int X, int Y, int index, int callType)
{
	if (!this->IsCoordInMap(X, Y))
		return;

	if (ExtConfigs::PlaceTileSkipHide && callType != 3) // 3 = cut
	{
		const auto cell = this->TryGetCellAt(X, Y);
		if (cell->IsHidden())
			return;
	}

	if (index < CUSTOM_TILE_START)
	{
		index = CMapDataExt::GetSafeTileIndex(index);
		if (index > CMapDataExt::TileDataCount) return;
		const auto& tileData = CMapDataExt::TileData[index];
		int width = CMapDataExt::TileData[index].Width;
		int height = CMapDataExt::TileData[index].Height;
		int startHeight = this->GetCellAt(X, Y)->Height;
		bool isBridge = (tileData.TileSet == BridgeSet || tileData.TileSet == WoodBridgeSet);

		int subIdx = 0;
		switch (callType)
		{
		case 1: // random terrain
		{
			for (int m = 0; m < height; m++)
			{
				for (int n = 0; n < width; n++)
				{
					if (!this->IsCoordInMap(m + X, n + Y))
						continue;
					if (tileData.TileBlockDatas[subIdx].ImageData != NULL)
					{
						auto& cellExt = CMapDataExt::CellDataExts[this->GetCoordIndex(m + X, n + Y)];
						if (cellExt.AddRandomTile) return;
					}
					subIdx++;
				}
			}
		}
		break;
		default:
			break;
		}

		subIdx = 0;
		for (int m = 0; m < height; m++)
		{
			for (int n = 0; n < width; n++)
			{
				if (tileData.TileBlockDatas[subIdx].ImageData != NULL && this->IsCoordInMap(m + X, n + Y))
				{
					auto cell = this->GetCellAt(m + X, n + Y);
					cell->TileIndex = index;
					cell->TileSubIndex = subIdx;
					cell->Flag.AltIndex = isBridge ? 0 : STDHelpers::RandomSelectInt(0, tileData.AltTypeCount + 1);
					SetHeightAt(m + X, n + Y, startHeight + tileData.TileBlockDatas[subIdx].Height);
					CMapData::Instance->UpdateMapPreviewAt(m + X, n + Y);

					auto& cellExt = CMapDataExt::CellDataExts[this->GetCoordIndex(m + X, n + Y)];
					switch (callType)
					{
					case 1: // random terrain
					{
						cellExt.AddRandomTile = true;
						break;
					}
					case 2: // line tile
					{
						cellExt.LineToolProcessed = true;
						break;
					}
					break;
					default:
						break;
					}
				}
				subIdx++;
			}
		}
	}
	else
	{
		auto customTileData = CMapDataExt::GetCustomTile(index);
		int width = customTileData->Width;
		int height = customTileData->Height;
		int startHeight = this->GetCellAt(X, Y)->Height;

		int subIdx = 0;
		switch (callType)
		{
		case 1: // random terrain
		{
			for (int m = 0; m < height; m++)
			{
				for (int n = 0; n < width; n++)
				{
					if (!this->IsCoordInMap(m + X, n + Y))
						continue;
					if (customTileData->TileBlockDatas[subIdx].TileBlock 
						&& customTileData->TileBlockDatas[subIdx].TileBlock->ImageData)
					{
						auto& cellExt = CMapDataExt::CellDataExts[this->GetCoordIndex(m + X, n + Y)];
						if (cellExt.AddRandomTile) return;
					}
					subIdx++;
				}
			}
		}
		break;
		default:
			break;
		}

		subIdx = 0;
		for (int m = 0; m < height; m++)
		{
			for (int n = 0; n < width; n++)
			{
				auto& tile = customTileData->TileBlockDatas[subIdx];
				if (tile.TileBlock && tile.TileBlock->ImageData && this->IsCoordInMap(m + X, n + Y))
				{
					auto tileData = CMapDataExt::TileData[tile.TileIndex];
					auto tileSet = tileData.TileSet;
					bool isBridge = (tileSet == CMapDataExt::BridgeSet || tileSet == CMapDataExt::WoodBridgeSet);

					auto cell = this->GetCellAt(m + X, n + Y);
					cell->TileIndex = tile.TileIndex;
					cell->TileSubIndex = tile.SubTileIndex;
					cell->Flag.AltIndex = isBridge ? 0 : STDHelpers::RandomSelectInt(0, tileData.AltTypeCount + 1);
					SetHeightAt(m + X, n + Y, startHeight + tile.GetHeight());
					CMapData::Instance->UpdateMapPreviewAt(m + X, n + Y);

					auto& cellExt = CMapDataExt::CellDataExts[this->GetCoordIndex(m + X, n + Y)];
					switch (callType)
					{
					case 1: // random terrain
					{
						cellExt.AddRandomTile = true;
						break;
					}
					case 2: // line tile
					{
						cellExt.LineToolProcessed = true;
						break;
					}
					break;
					default:
						break;
					}
				}
				subIdx++;
			}
		}
	}
}

std::vector<MapCoord> CMapDataExt::GetIntactTileCoords(int x, int y, bool oriIntact)
{
	std::vector<MapCoord> ret;
	if (!oriIntact || IsTileIntact(x, y))
	{
		int pos = this->GetCoordIndex(x, y);
		auto cell = this->GetCellAt(pos);
		int tileIndex = cell->TileIndex;
		if (tileIndex == 0xFFFF)
			tileIndex = 0;

		int oriX = x - cell->TileSubIndex / CMapDataExt::TileData[tileIndex].Width;
		int oriY = y - cell->TileSubIndex % CMapDataExt::TileData[tileIndex].Width;

		int subIdx = 0;
		for (int m = 0; m < CMapDataExt::TileData[tileIndex].Height; m++)
		{
			for (int n = 0; n < CMapDataExt::TileData[tileIndex].Width; n++)
			{
				if (CMapDataExt::TileData[tileIndex].TileBlockDatas[subIdx].ImageData != NULL)
				{
					MapCoord mc;
					mc.X = m + oriX;
					mc.Y = n + oriY;
					ret.push_back(mc);
				}
				subIdx++;
			}
		}

		return ret;
	}
	return ret;
}

LandType CMapDataExt::GetAltLandType(int tileIndex, int TileSubIndex)
{
	if (tileIndex == 0xFFFF)
		tileIndex = 0;

	if (TileData[tileIndex].TileSet != ShorePieces && TileData[tileIndex].TileSet == AutoShore_ShoreTileSet)
	{
		int relativeIdx = tileIndex - TileSet_starts[TileData[tileIndex].TileSet];
		ppmfc::CString key;
		key.Format("%d_%d", relativeIdx, TileSubIndex);
		if (CINI::FAData->KeyExists("ShoreTerrainRA2", key))
		{
			if (CINI::FAData->GetInteger("ShoreTerrainRA2", key) >= 0)
			{
				return LandType::Water;
			}
			else
			{
				return LandType::Rough;
			}
		}
	}

	return TileData[tileIndex].TileBlockDatas[TileSubIndex].TerrainTypeAlt;
}


void CMapDataExt::PlaceWallAt(int dwPos, int overlay, int damageStage, bool firstRun)
{
	auto Map = CMapDataExt::GetExtension();
	if (!Map->IsCoordInMap(dwPos)) return;
	if (damageStage == -1)
		damageStage = Map->GetOverlayDataAt(dwPos) / 16;
	else if (damageStage == -2)
	{
		auto&& overlays = Variables::RulesMap.ParseIndicies("OverlayTypes", true);
		int damageLevel = CINI::Art().GetInteger(overlays[overlay], "DamageLevels", 1);
		std::vector<int> rnd;
		for (int i = 0; i < damageLevel; i++)
			rnd.push_back(i);

		damageStage = STDHelpers::RandomSelectInt(rnd);
	}

	int overlayData = 16 * damageStage;
	int X = Map->GetXFromCoordIndex(dwPos);
	int Y = Map->GetYFromCoordIndex(dwPos);
	if (firstRun)
		Map->SetNewOverlayAt(dwPos, overlay);
	else
		if (Map->GetOverlayAt(dwPos) != overlay)
			return;
	//                              8      2    4      1
	//                              NW     SE   SW     NE
	const int loopCheck[4][2] = { {0,-1},{0,1},{1,0},{-1,0} };

	if (Map->IsCoordInMap(X + loopCheck[0][0], Y + loopCheck[0][1]))
	{
		int thisPos = Map->GetCoordIndex(X + loopCheck[0][0], Y + loopCheck[0][1]);
		if (Map->GetOverlayAt(thisPos) == overlay)
			overlayData += 8;
	}
	if (Map->IsCoordInMap(X + loopCheck[1][0], Y + loopCheck[1][1]))
	{
		int thisPos = Map->GetCoordIndex(X + loopCheck[1][0], Y + loopCheck[1][1]);
		if (Map->GetOverlayAt(thisPos) == overlay)
			overlayData += 2;
	}
	if (Map->IsCoordInMap(X + loopCheck[2][0], Y + loopCheck[2][1]))
	{
		int thisPos = Map->GetCoordIndex(X + loopCheck[2][0], Y + loopCheck[2][1]);
		if (Map->GetOverlayAt(thisPos) == overlay)
			overlayData += 4;
	}
	if (Map->IsCoordInMap(X + loopCheck[3][0], Y + loopCheck[3][1]))
	{
		int thisPos = Map->GetCoordIndex(X + loopCheck[3][0], Y + loopCheck[3][1]);
		if (Map->GetOverlayAt(thisPos) == overlay)
			overlayData += 1;
	}
	Map->SetOverlayDataAt(dwPos, overlayData);

	if (firstRun)
	{
		PlaceWallAt(Map->GetCoordIndex(X - 1, Y), overlay, -1, false);
		PlaceWallAt(Map->GetCoordIndex(X + 1, Y), overlay, -1, false);
		PlaceWallAt(Map->GetCoordIndex(X, Y - 1), overlay, -1, false);
		PlaceWallAt(Map->GetCoordIndex(X, Y + 1), overlay, -1, false);
	}

}

int CMapDataExt::GetInfantryAt(int dwPos, int dwSubPos)
{
	if (dwSubPos < 0)
	{
		int i;
		for (i = 0; i < 3; i++)
			if (CMapData::Instance->CellDatas[dwPos].Infantry[i] != -1)
				return CMapData::Instance->CellDatas[dwPos].Infantry[i];
		return -1;
	}
	if (dwSubPos == 4)
		dwSubPos = 0;
	else if (dwSubPos == 3)
		dwSubPos = 2;
	else if (dwSubPos == 2)
		dwSubPos = 1;
	else if (dwSubPos == 1)
		dwSubPos = 0;
	return CMapData::Instance->CellDatas[dwPos].Infantry[dwSubPos];
}

void CMapDataExt::InitOreValue()
{
    OreValue[OreType::Aboreus] = Variables::RulesMap.GetInteger("Aboreus", "Value");
    OreValue[OreType::Cruentus] = Variables::RulesMap.GetInteger("Cruentus", "Value");
    OreValue[OreType::Riparius] = Variables::RulesMap.GetInteger("Riparius", "Value");
    OreValue[OreType::Vinifera] = Variables::RulesMap.GetInteger("Vinifera", "Value");
}

void CMapDataExt::SmoothAll()
{
	if (CFinalSunApp::Instance().DisableAutoLat)
		return;

	auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
	auto tileDataBrush = CMapDataExt::TileData[CIsoView::CurrentCommand->Type];
	auto point = CIsoView::GetInstance()->GetCurrentMapCoord(CIsoView::GetInstance()->MouseCurrentPosition);

	for (int x = -tileDataBrush.Height; x < (pIsoView->BrushSizeX - 1) * tileDataBrush.Height + 2; x++)
	{
		for (int y = -tileDataBrush.Width; y < (pIsoView->BrushSizeY - 1) * tileDataBrush.Width + 2; y++)
		{
			SmoothTileAt(x + point.X, y + point.Y);
		}
	}
}

void CMapDataExt::SmoothWater()
{
	int side = CMapData::Instance->MapWidthPlusHeight;
	auto CellDatas = CMapData::Instance->CellDatas;

	// first check all the water tiles
	for (int i = 0; i < CMapData::Instance->CellDataCount; i++)
	{
		auto& cellExt = CMapDataExt::CellDataExts[i];
		auto& cell = CMapData::Instance->CellDatas[i];
		cellExt.Processed = false;
		cellExt.IsWater = false;
		for (auto idx : TheaterInfo::CurrentBigWaters)
			if (cell.TileIndex == idx)
				cellExt.IsWater = true;
		for (auto idx : TheaterInfo::CurrentSmallWaters)
			if (cell.TileIndex == idx)
				cellExt.IsWater = true;
	}

	// then replace all 2x2 water
	for (int i = 0; i < CMapData::Instance->CellDataCount; i++)
	{
		int Y = i / side;
		int X = i % side;
		if (!CMapData::Instance->IsCoordInMap(X, Y))
			continue;

		auto& cell = CMapDataExt::CellDataExts[i];
		if (cell.IsWater && !cell.Processed)
		{
			bool replaceBig = true;
			for (int d = 0; d < 2; d++)
			{
				for (int e = 0; e < 2; e++)
				{
					int dwPos = X + d + (Y + e) * side;
					auto& newCell = CMapDataExt::CellDataExts[dwPos];
					if (!newCell.IsWater || newCell.Processed)
						replaceBig = false;
				}
			}
			if (replaceBig)
			{
				int random = STDHelpers::RandomSelectInt(TheaterInfo::CurrentBigWaters);
				int subIdx = 0;
				for (int d = 0; d < 2; d++)
				{
					for (int e = 0; e < 2; e++)
					{
						int dwPos = X + d + (Y + e) * side;
						CellDatas[dwPos].TileIndex = random;
						CellDatas[dwPos].TileSubIndex = subIdx++;
						CellDatas[dwPos].Flag.AltIndex = 0;
						CMapDataExt::CellDataExts[dwPos].Processed = true;
					}
				}
			}
		}
	}

	// last fill remained 1x1 water
	for (int i = 0; i < CMapData::Instance->CellDataCount; i++)
	{
		int Y = i / side;
		int X = i % side;
		if (! CMapData::Instance->IsCoordInMap(X, Y))
			continue;

		auto& cell = CMapDataExt::CellDataExts[i];
		if (cell.IsWater && !cell.Processed)
		{
			int random = STDHelpers::RandomSelectInt(TheaterInfo::CurrentSmallWaters);
			int dwPos = X + Y  * side;
			CellDatas[dwPos].TileIndex = random;
			CellDatas[dwPos].TileSubIndex = 0;
			CellDatas[dwPos].Flag.AltIndex = 0;
			CMapDataExt::CellDataExts[dwPos].Processed = true;
		}
	}
}

void CMapDataExt::SmoothTileAt(int X, int Y, bool gameLAT)
{
	if (!CMapData::Instance->IsCoordInMap(X, Y))
		return;

	auto& mapData = CMapData::Instance();
	auto cellDatas = mapData.CellDatas;
	auto& ini = CINI::CurrentTheater();
	auto& fadata = CINI::FAData();

	auto cell = CMapData::Instance().TryGetCellAt(X, Y);
	if (ExtConfigs::PlaceTileSkipHide && cell->IsHidden())
		return ;

	if (cell->TileIndex == 0xFFFF) cell->TileIndex = 0;
	int dwPos = X + Y * mapData.MapWidthPlusHeight;
			
	int loopLimit = CMapDataExt::Tile_to_lat.size();
	if (gameLAT)
		loopLimit = 7;
	for (int latidx = 0; latidx < loopLimit; ++latidx)
	{
		int iSmoothSet = CMapDataExt::Tile_to_lat[latidx][0];
		int iLatSet = CMapDataExt::Tile_to_lat[latidx][1];

		if (iLatSet >= 0 && iSmoothSet >= 0 && iSmoothSet < CMapDataExt::TileSet_starts.size() && iLatSet < CMapDataExt::TileSet_starts.size() &&//iTargetSet >= 0 &&
			(CMapDataExt::TileData[GetSafeTileIndex(cell->TileIndex)].TileSet == iSmoothSet ||
				CMapDataExt::TileData[GetSafeTileIndex(cell->TileIndex)].TileSet == iLatSet ))
				// || CMapDataExt::TileData[cell->TileIndex].TileSet == iTargetSet))
		{
			std::vector<int> SmoothLatList;
			for (int slIdx = CMapDataExt::TileSet_starts[iLatSet]; slIdx < CMapDataExt::TileSet_starts[iLatSet + 1]; slIdx++)
				SmoothLatList.push_back(slIdx);
			for (int slIdx = CMapDataExt::TileSet_starts[iSmoothSet]; slIdx < CMapDataExt::TileSet_starts[iSmoothSet + 1]; slIdx++)
				SmoothLatList.push_back(slIdx);
			//PaveTile	-	MiscPaveTile	
			//PaveTile	-	Medians	
			//PaveTile	-	PavedRoads	
			//CrystalTile	-	CrystalCliff	(Firestorm only)
			//GreenTile	-	ShorePieces	(Red Alert 2/Yuri's Revenge only)
			//GreenTile	-	WaterBridge	(Red Alert 2/Yuri's Revenge only)
			if (iSmoothSet == PaveTile)
			{
				if (MiscPaveTile >= 0)
				for (int slIdx = CMapDataExt::TileSet_starts[MiscPaveTile]; slIdx < CMapDataExt::TileSet_starts[MiscPaveTile + 1]; slIdx++)
					SmoothLatList.push_back(slIdx);
				if (Medians >= 0)
				for (int slIdx = CMapDataExt::TileSet_starts[Medians]; slIdx < CMapDataExt::TileSet_starts[Medians + 1]; slIdx++)
					SmoothLatList.push_back(slIdx);
				if (PavedRoads >= 0)
				for (int slIdx = CMapDataExt::TileSet_starts[PavedRoads]; slIdx < CMapDataExt::TileSet_starts[PavedRoads + 1]; slIdx++)
					SmoothLatList.push_back(slIdx);
			}
			if (iSmoothSet == GreenTile)
			{
				if (ShorePieces >= 0)
				for (int slIdx = CMapDataExt::TileSet_starts[ShorePieces]; slIdx < CMapDataExt::TileSet_starts[ShorePieces + 1]; slIdx++)
					SmoothLatList.push_back(slIdx);
				if (WaterBridge >= 0)
				for (int slIdx = CMapDataExt::TileSet_starts[WaterBridge]; slIdx < CMapDataExt::TileSet_starts[WaterBridge + 1]; slIdx++)
					SmoothLatList.push_back(slIdx);
			}
			
			if (!gameLAT)
				if (CMapDataExt::Tile_to_lat[latidx].size() >= 3)
				{
					for (int i = 2; i < CMapDataExt::Tile_to_lat[latidx].size(); ++i)
					{
						SmoothLatList.push_back(CMapDataExt::Tile_to_lat[latidx][i]);
					}
				}



			if (CMapDataExt::TileData[GetSafeTileIndex(cell->TileIndex)].TileSet != iSmoothSet && CMapDataExt::TileData[GetSafeTileIndex(cell->TileIndex)].TileSet != iLatSet) break;

			int ts[3][3];  // terrain info
			for (int i = 0; i < 3; i++)
			{
				for (int e = 0; e < 3; e++)
				{
					if ( CMapData::Instance->IsCoordInMap(X + i - 1, Y + e - 1))
					{
						auto cell2 = CMapData::Instance().TryGetCellAt(X + i - 1, Y + e - 1);
						auto it = std::find(SmoothLatList.begin(), SmoothLatList.end(), GetSafeTileIndex(cell2->TileIndex));

						if (it != SmoothLatList.end())
							ts[i][e] = 1;
						else
							ts[i][e] = 0;
					}
					else
						ts[i][e] = 0;
				}
			}

			int needed = -1;
			int ils = 1;

			if (ts[1][1] == ils)
			{
				// single lat
				if (ts[0][1] != ils && ts[1][0] != ils
					&& ts[1][2] != ils && ts[2][1] != ils)
					needed = 16;
				else if (ts[0][1] == ils && ts[1][0] == ils
					&& ts[1][2] == ils && ts[2][1] == ils)
					needed = 0;
				else if (ts[0][1] == ils && ts[2][1] == ils &&
					ts[1][0] != ils && ts[1][2] != ils)
					needed = 11;
				else if (ts[1][0] == ils && ts[1][2] == ils &&
					ts[0][1] != ils && ts[2][1] != ils)
					needed = 6;
				else if (ts[1][0] != ils && ts[0][1] == ils &&
					ts[2][1] == ils)
					needed = 9;
				else if (ts[2][1] != ils && ts[1][0] == ils &&
					ts[1][2] == ils)
					needed = 5;
				else if (ts[1][2] != ils && ts[0][1] == ils &&
					ts[2][1] == ils)
					needed = 3;
				else if (ts[0][1] != ils && ts[1][0] == ils &&
					ts[1][2] == ils)
					needed = 2;
				else if (ts[0][1] == ils && ts[1][0] != ils &&
					ts[1][2] != ils && ts[2][1] != ils)
					needed = 15;
				else if (ts[1][2] == ils && ts[1][0] != ils &&
					ts[0][1] != ils && ts[2][1] != ils)
					needed = 14;
				else if (ts[2][1] == ils && ts[1][0] != ils &&
					ts[0][1] != ils && ts[1][2] != ils)
					needed = 12;
				else if (ts[1][0] == ils && ts[0][1] != ils &&
					ts[1][2] != ils && ts[2][1] != ils)
					needed = 8;
				else if (ts[1][0] != ils && ts[2][1] != ils)
					needed = 13;
				else if (ts[1][0] != ils && ts[0][1] != ils)
					needed = 10;
				else if (ts[2][1] != ils && ts[1][2] != ils)
					needed = 7;
				else if (ts[0][1] != ils && ts[1][2] != ils)
					needed = 4;


			}

			needed -= 1;
			int i = 0;
			if (needed >= 0)
			{
				i = CMapDataExt::TileSet_starts[iLatSet];

				// i is first lat tile
				int e;
				for (e = 0; e < needed; e++)
				{
					i += CMapDataExt::TileData[i].TileBlockCount;
				}


				cellDatas[dwPos].TileIndex = i;
				cellDatas[dwPos].TileSubIndex = 0;
				cellDatas[dwPos].Flag.AltIndex = STDHelpers::RandomSelectInt(0, CMapDataExt::TileData[i].AltTypeCount + 1);
				//SetTileAt(dwPos, i, 0);
			}
			else if (needed == -1)
			{
				i = CMapDataExt::TileSet_starts[iSmoothSet];

				cellDatas[dwPos].TileIndex = i;
				cellDatas[dwPos].TileSubIndex = 0;
				cellDatas[dwPos].Flag.AltIndex = STDHelpers::RandomSelectInt(0, CMapDataExt::TileData[i].AltTypeCount + 1);
			}
		}
	}
}

void CMapDataExt::CreateSlopeAt(int x, int y, bool IgnoreMorphable)
{
	std::vector<std::pair<int, int>> morphable;
	for (int nMorphable = 0; nMorphable < TheaterInfo::CurrentInfo.size(); nMorphable++)
		morphable.push_back(std::make_pair(TheaterInfo::CurrentInfo[nMorphable].Morphable, TheaterInfo::CurrentInfo[nMorphable].Ramp));

	auto cell = CMapData::Instance->TryGetCellAt(x, y);
	int groundClick = cell->TileIndex;
	if (groundClick == 0xFFFF) groundClick = 0;
	if (!IgnoreMorphable && !CMapDataExt::TileData[groundClick].Morphable) return;

	// default use clear
	int startTile = CMapDataExt::TileSet_starts[CINI::CurrentTheater->GetInteger("General", "RampBase", 9)];
	int flatTile = CMapDataExt::TileSet_starts[0];
	for (auto& pair : morphable)
	{
		if (CMapDataExt::TileData[groundClick].TileSet == pair.first || CMapDataExt::TileData[groundClick].TileSet == pair.second)
		{
			startTile = CMapDataExt::TileSet_starts[pair.second];
			flatTile = CMapDataExt::TileSet_starts[pair.first];
			break;
		}
	}
	// take LAT into consideration
	for (int latidx = 0; latidx < CMapDataExt::Tile_to_lat.size(); ++latidx)
	{
		int iSmoothSet = CMapDataExt::Tile_to_lat[latidx][0];
		int iLatSet = CMapDataExt::Tile_to_lat[latidx][1];

		if (CMapDataExt::TileData[groundClick].TileSet == iLatSet)
		{
			for (auto& pair : morphable)
			{
				if (iSmoothSet == pair.first)
				{
					startTile = CMapDataExt::TileSet_starts[pair.second];
					flatTile = CMapDataExt::TileSet_starts[pair.first];
					break;
				}
			}

		}
	}

	static std::vector<std::pair<int, int>> directions = {
	{1, 0},   // 0бу
	{1, 1},   // 45бу
	{0, 1},   // 90бу
	{-1, 1},  // 135бу
	{-1, 0},  // 180бу
	{-1, -1}, // 225бу
	{0, -1},  // 270бу
	{1, -1}   // 315бу
	};

	int height = cell->Height;
	auto isMorphable = [IgnoreMorphable](CellData* cell)
		{
			if (!cell) return 0;
			if (ExtConfigs::PlaceTileSkipHide && cell->IsHidden())
				return 0;
			if (IgnoreMorphable) return 1;
			int groundClick = cell->TileIndex;
			if (groundClick == 0xFFFF) groundClick = 0;
			return CMapDataExt::TileData[groundClick].Morphable;
		};
	auto getIndex = [startTile](int idx)
		{
			return startTile + idx;
		};
	auto getNW = [height, x, y, isMorphable]()
		{
			if (!CMapData::Instance->IsCoordInMap(x, y - 1))
				return 0;
			auto cellthis = CMapData::Instance->GetCellAt(x, y - 1);
			if (!isMorphable(cellthis))
				if (cellthis->Height - height <= 1)
					return cellthis->Height - height;
				else
					return 0;
			return cellthis->Height - height;
		};
	auto getSE = [height, x, y, isMorphable]()
		{			
			if (!CMapData::Instance->IsCoordInMap(x, y + 1))
				return 0;
			auto cellthis = CMapData::Instance->GetCellAt(x, y + 1);
			if (!isMorphable(cellthis))
				if (cellthis->Height - height <= 1)
					return cellthis->Height - height;
				else
					return 0;
			return cellthis->Height - height;
		};
	auto getNE = [height, x, y, isMorphable]()
		{			
			if (!CMapData::Instance->IsCoordInMap(x - 1, y))
				return 0;
			auto cellthis = CMapData::Instance->GetCellAt(x - 1, y);
			if (!isMorphable(cellthis))
				if (cellthis->Height - height <= 1)
					return cellthis->Height - height;
				else
					return 0;
			return cellthis->Height - height;
		};
	auto getSW = [height, x, y, isMorphable]()
		{
			if (!CMapData::Instance->IsCoordInMap(x + 1, y))
				return 0;
			auto cellthis = CMapData::Instance->GetCellAt(x + 1, y);
			if (!isMorphable(cellthis))
				if (cellthis->Height - height <= 1)
					return cellthis->Height - height;
				else
					return 0;
			return cellthis->Height - height;
		};
	auto getN = [height, x, y, isMorphable]()
		{
			if (!CMapData::Instance->IsCoordInMap(x - 1, y - 1))
				return 0;
			auto cellthis = CMapData::Instance->GetCellAt(x - 1, y - 1);
			if (!isMorphable(cellthis))
				if (cellthis->Height - height <= 2)
					return cellthis->Height - height;
				else
					return 0;
			return cellthis->Height - height;
		};
	auto getE = [height, x, y, isMorphable]()
		{
			if (!CMapData::Instance->IsCoordInMap(x - 1, y + 1))
				return 0;
			auto cellthis = CMapData::Instance->GetCellAt(x - 1, y + 1);
			if (!isMorphable(cellthis))
				if (cellthis->Height - height <= 2)
					return cellthis->Height - height;
				else
					return 0;
			return cellthis->Height - height;
		};
	auto getS = [height, x, y, isMorphable]()
		{
			if (!CMapData::Instance->IsCoordInMap(x + 1, y + 1))
				return 0;
			auto cellthis = CMapData::Instance->GetCellAt(x + 1, y + 1);
			if (!isMorphable(cellthis))
				if (cellthis->Height - height <= 2)
					return cellthis->Height - height;
				else
					return 0;
			return cellthis->Height - height;
		};
	auto getW = [height, x, y, isMorphable]()
		{
			if (!CMapData::Instance->IsCoordInMap(x + 1, y - 1))
				return 0;
			auto cellthis = CMapData::Instance->GetCellAt(x + 1, y - 1);
			if (!isMorphable(cellthis))
				if (cellthis->Height - height <= 2)
					return cellthis->Height - height;
				else
					return 0;
			return cellthis->Height - height;
		};

	auto getTileIndex = [&]()
		{
			if (getNE() > 1 || getNW() > 1 || getSE() > 1 || getSW() > 1
				|| getE() > 2 || getW() > 2 || getS() > 2 || getN() > 2)
				return flatTile; // if too high, skip slope
			if (getS() > 1) return getIndex(12);
			if (getW() > 1) return getIndex(13);
			if (getN() > 1) return getIndex(14);
			if (getE() > 1) return getIndex(15);
			if (getN() > 0 && getS() > 0 && getNE() == 0 && getNW() == 0 && getSE() == 0 && getSW() == 0) return getIndex(17);
			if (getE() > 0 && getW() > 0 && getNE() == 0 && getNW() == 0 && getSE() == 0 && getSW() == 0) return getIndex(16);
			if (getSW() > 0 && getSE() > 0) return getIndex(8);
			if (getNW() > 0 && getSW() > 0) return getIndex(9);
			if (getNW() > 0 && getNE() > 0) return getIndex(10);
			if (getNE() > 0 && getSE() > 0) return getIndex(11);

			for (int i = 0; i < 8; ++i)
			{
				int iPlus3 = (i + 3) % 8;
				auto& coordA = directions[i];
				auto& coordB = directions[iPlus3];
				if (CMapData::Instance->IsCoordInMap(x + coordA.first, y + coordA.second)
					&& CMapData::Instance->IsCoordInMap(x + coordB.first, y + coordB.second))
				{
					auto cellA = CMapData::Instance->GetCellAt(x + coordA.first, y + coordA.second);
					auto cellB = CMapData::Instance->GetCellAt(x + coordB.first, y + coordB.second);
					if (cellA->Height > height && cellB->Height > height)
					{
						if (getSW() > 0 && getE() > 0) return getIndex(8);
						if (getSE() > 0 && getW() > 0) return getIndex(8);

						if (getNW() > 0 && getS() > 0) return getIndex(9);
						if (getSW() > 0 && getN() > 0) return getIndex(9);

						if (getNE() > 0 && getW() > 0) return getIndex(10);
						if (getNW() > 0 && getE() > 0) return getIndex(10);

						if (getNE() > 0 && getS() > 0) return getIndex(11);
						if (getSE() > 0 && getN() > 0) return getIndex(11);
					}
				}
			}

			if (getSE() > 0) return getIndex(0);
			if (getSW() > 0) return getIndex(1);
			if (getNW() > 0) return getIndex(2);
			if (getNE() > 0) return getIndex(3);
			if (getS() > 0) return getIndex(4);
			if (getW() > 0) return getIndex(5);
			if (getN() > 0) return getIndex(6);
			if (getE() > 0) return getIndex(7);
			return flatTile;
		};

	int tileIndex = getTileIndex();
	if (tileIndex != flatTile || CMapDataExt::TileData[groundClick].TileBlockDatas[cell->TileSubIndex].RampType != 0)
	{
		if (ExtConfigs::PlaceTileSkipHide && cell->IsHidden())
			return;
		cell->TileIndex = tileIndex;
		cell->TileSubIndex = 0;
		cell->Flag.AltIndex = STDHelpers::RandomSelectInt(0, CMapDataExt::TileData[tileIndex].AltTypeCount + 1);
	}
}

void CMapDataExt::UpdateFieldStructureData_Index(int iniIndex, ppmfc::CString value, bool refreshCenter)
{
	if (value == "")
		value = CINI::CurrentDocument->GetValueAt("Structures", iniIndex);

	int cellIndex = StructureIndexMap.size();
	if (cellIndex > SHRT_MAX && !CIsoView::IsMouseMoving)
	{
		UpdateFieldStructureData_Optimized();
	}
	else
	{
		for (int i = 0; i < cellIndex; ++i)
		{
			if (CMapDataExt::StructureIndexMap[i] >= iniIndex)
				CMapDataExt::StructureIndexMap[i]++;
		}
		StructureIndexMap.push_back(iniIndex);

		const auto splits = FString::SplitString(value, 16);

		BuildingRenderData data;
		data.HouseColor = Miscs::GetColorRef(splits[0]);
		data.ID = splits[1];
		data.Strength = std::clamp(atoi(splits[2]), 0, 256);
		data.Y = atoi(splits[3]);
		data.X = atoi(splits[4]);
		data.Facing = atoi(splits[5]);
		data.PowerUpCount = atoi(splits[10]);
		data.PowerUp1 = splits[12];
		data.PowerUp2 = splits[13];
		data.PowerUp3 = splits[14];
		data.poweredOn = splits[9] != "0";
		CMapDataExt::BuildingRenderDatasFix.insert(CMapDataExt::BuildingRenderDatasFix.begin() + iniIndex, data);

		int X = atoi(splits[4]);
		int Y = atoi(splits[3]);
		const int BuildingIndex = CMapDataExt::GetBuildingTypeIndex(splits[1]);
		const auto& DataExt = CMapDataExt::BuildingDataExts[BuildingIndex];
		if (!DataExt.IsCustomFoundation())
		{
			for (int dy = 0; dy < DataExt.Width; ++dy)
			{
				for (int dx = 0; dx < DataExt.Height; ++dx)
				{
					const int x = X + dx;
					const int y = Y + dy;
					int coord = CMapData::Instance->GetCoordIndex(x, y);
					if (coord < CMapData::Instance->CellDataCount)
					{
						auto pCell = CMapData::Instance->GetCellAt(coord);
						CMapDataExt::CellDataExts[coord].Structures[cellIndex] = BuildingIndex;
						pCell->Structure = cellIndex;
						pCell->TypeListIndex = BuildingIndex;
						CMapData::Instance->UpdateMapPreviewAt(x, y);
					}
				}
			}
		}
		else
		{
			for (const auto& block : *DataExt.Foundations)
			{
				const int x = X + block.Y;
				const int y = Y + block.X;
				int coord = CMapData::Instance->GetCoordIndex(x, y);
				if (coord < CMapData::Instance->CellDataCount)
				{
					auto pCell = CMapData::Instance->GetCellAt(coord);
					CMapDataExt::CellDataExts[coord].Structures[cellIndex] = BuildingIndex;
					pCell->Structure = cellIndex;
					pCell->TypeListIndex = BuildingIndex;
					CMapData::Instance->UpdateMapPreviewAt(x, y);
				}
			}
		}

		if (ExtConfigs::InGameDisplay_RemapableOverlay)
		{
			if (refreshCenter)
			{
				// wall buildings can't affect overlays
				if (!Variables::RulesMap.GetBool(data.ID, "Wall"))
					CMapDataExt::RemapableOverlay_AddBuilding(cellIndex, { X + DataExt.Height / 2, Y + DataExt.Width / 2 });
			}
			else
			{
				if (!Variables::RulesMap.GetBool(data.ID, "Wall"))
					BuildingCenterCoords[cellIndex] = { X + DataExt.Height / 2, Y + DataExt.Width / 2 };
			}
		}
	}
}

void CMapDataExt::UpdateFieldStructureData_Optimized()
{
	auto Map = &CMapData::Instance();
	auto& fielddata_size = Map->CellDataCount;
	auto& fielddata = Map->CellDatas;
	BuildingCenterCoords.clear();

	int i = 0;
	for (i = 0; i < fielddata_size; i++)
	{
		CMapDataExt::CellDataExts[i].Structures.clear();
		fielddata[i].Structure = -1;
		fielddata[i].TypeListIndex = -1;
	}
	StructureIndexMap.clear();
	BuildingRenderDatasFix.clear();
	if (auto sec = CINI::CurrentDocument->GetSection("Structures"))
	{
		i = 0;
		for (const auto& data : sec->GetEntities())
		{
			UpdateFieldStructureData_Index(i, data.second, false);
			i++;
		}
		if (ExtConfigs::InGameDisplay_RemapableOverlay)
		{
			RemapableOverlay_RefreshBuildingIndices();
		}
	}
}

std::vector<int> CMapDataExt::GetStructureSize(ppmfc::CString structure)
{
	std::vector<int> result;
	MultimapHelper mmh;
	mmh.AddINI(&CINI::Rules());
	mmh.AddINI(&CINI::CurrentDocument());
	auto art = &CINI::Art();
	auto image = mmh.GetString(structure, "Image", structure);
	std::string foundation = std::string(art->GetString(image, "Foundation", "1X1"));
	if (foundation == "")
		foundation = "1X1";
	std::transform(foundation.begin(), foundation.end(), foundation.begin(), (int(*)(int))tolower);

	if (foundation == "custom")
	{
		std::string x = std::string(art->GetString(image, "Foundation.X", "5"));
		std::string y = std::string(art->GetString(image, "Foundation.Y", "5"));
		foundation = x + "x" + y;
	}
	auto found = STDHelpers::SplitString(foundation.c_str(), "x");
	result.push_back(atoi(found[1]));
	result.push_back(atoi(found[0]));
	return result;
}

ppmfc::CString CMapDataExt::GetFacing(MapCoord oldMapCoord, MapCoord newMapCoord, ppmfc::CString currentFacing, int numFacings)
{
	if (oldMapCoord == newMapCoord)
		return currentFacing;
	return STDHelpers::IntToString(GetFacing(oldMapCoord, newMapCoord, numFacings) * 256 / numFacings);
}

int CMapDataExt::GetFacing(MapCoord oldMapCoord, MapCoord newMapCoord, int numFacings)
{
	if (oldMapCoord == newMapCoord)
		return 0;

	double dx = (double)(newMapCoord.X - oldMapCoord.X);
	double dy = (double)(oldMapCoord.Y - newMapCoord.Y); 

	double angle = atan2(dy, dx);

	angle += M_PI;

	if (angle < 0)
		angle += 2 * M_PI;
	else if (angle >= 2 * M_PI)
		angle -= 2 * M_PI;

	double sector = (2 * M_PI) / numFacings;

	int facing = (int)floor(angle / sector + 0.5);
	if (facing >= numFacings)
		facing -= numFacings;

	return facing;
}

int CMapDataExt::GetFacing4(MapCoord oldMapCoord, MapCoord newMapCoord)
{
	if (oldMapCoord == newMapCoord)
		return 0;
	if (oldMapCoord.X == newMapCoord.X)
	{
		if (newMapCoord.Y >= oldMapCoord.Y)
			return 2;
		else
			return 6;
	}
	else
	{
		auto Tan = (double)(newMapCoord.Y - oldMapCoord.Y) / (double)(newMapCoord.X - oldMapCoord.X);
		auto radian = (atan(Tan) / M_PI) * 180.0;
		if (newMapCoord.X >= oldMapCoord.X)
		{
			if (radian >= 45)
				return 2;
			else if (radian >= -45)
				return 4;
			else
				return 6;
		}
		else
		{
			if (radian >= 45)
				return 6;
			else if (radian >= -45)
				return 0;
			else
				return 2;
		}
	}
	return 0;
}

bool CMapDataExt::IsValidTileSet(int tileset, bool allowToPlace)
{
	FString buffer;
	buffer.Format("TileSet%04d", tileset);

	auto exist = CINI::CurrentTheater->GetBool(buffer, "AllowToPlace", true);
	if (!allowToPlace)
		exist = true;
	auto exist2 = CINI::CurrentTheater->GetString(buffer, "FileName", "");
	auto exist3 = CINI::CurrentTheater->GetInteger(buffer, "TilesInSet");
	if (!exist || strcmp(exist2, "") == 0 || exist3 < 1)
		return false;
	return true;
}

ppmfc::CString CMapDataExt::GetAvailableIndex()
{
	auto v = VEHGuard(false);
	auto& ini = CINI::CurrentDocument;
	int initNumber = 1000000;

	std::unordered_set<std::string> usedIDs;
	int maxID = 0;

	auto parseID = [&](const std::string& s) {
		try {
			return std::stoi(s);
		}
		catch (...) {
			return -1;
		}
	};

	for (const auto& sec : { "ScriptTypes", "TaskForces", "TeamTypes" }) {
		if (auto pSection = ini->GetSection(sec)) {
			for (const auto& [k, v] : pSection->GetEntities()) {
				std::string id = v.m_pchData;
				usedIDs.insert(id);
				int val = parseID(id);
				if (val >= 0) maxID = std::max(maxID, val);
			}
		}
	}

	for (const auto& sec : { "Triggers", "Events", "Tags", "Actions", "AITriggerTypes" }) {
		if (auto pSection = ini->GetSection(sec)) {
			for (const auto& [k, v] : pSection->GetEntities()) {
				std::string id = k.m_pchData;
				usedIDs.insert(id);
				int val = parseID(id);
				if (val >= 0) maxID = std::max(maxID, val);
			}
		}
	}

	if (ExtConfigs::UseSequentialIndexing) {
		if (maxID < initNumber)
			maxID = initNumber;
		int nextID = maxID + 1;
		char idBuffer[9];
		std::sprintf(idBuffer, "%08d", nextID);
		return idBuffer;
	}

	char idBuffer[9];
	while (true) {
		std::sprintf(idBuffer, "%08d", initNumber);
		std::string id(idBuffer);

		if (usedIDs.find(id) == usedIDs.end() && !ini->SectionExists(id.c_str())) {
			return id.c_str();
		}

		initNumber++;
	}

	return "";
}

void CMapDataExt::UpdateMapSectionIndicies(const ppmfc::CString& lpSection)
{
	if (auto pSection = CINI::CurrentDocument->GetSection(lpSection))
	{
		std::vector<std::pair<FString, FString>> buffer;
		for (auto& [key, value] : pSection->GetEntities())
		{
			buffer.push_back(std::make_pair(key, value));
		}
		CINI::CurrentDocument->DeleteSection(lpSection);
		pSection = CINI::CurrentDocument->AddSection(lpSection);
		int index = 0;
		for (auto& [key, value] : buffer)
		{
			CINI::CurrentDocument->WriteString(pSection, key, value);
			std::pair<ppmfc::CString, int> ins =
				std::make_pair((ppmfc::CString)key, index++);
			std::pair<INIIndiceDict::iterator, bool> ret;
			reinterpret_cast<FAINIIndicesMap*>(&pSection->GetIndices())->insert(&ret, &ins);
		}
		Variables::RulesMap.ClearMap(lpSection);
	}
}

void CMapDataExt::UpdateAnnotation()
{
	for (auto& cellExt : CMapDataExt::CellDataExts)
	{
		cellExt.HasAnnotation = false;
	}
	if (auto pSection = CINI::CurrentDocument->GetSection("Annotations"))
	{
		for (const auto& [key, value] : pSection->GetEntities())
		{
			auto pos = atoi(key);
			int x = pos / 1000;
			int y = pos % 1000;

			auto& cellExt = CMapDataExt::CellDataExts[CMapData::Instance->GetCoordIndex(x, y)];
			cellExt.HasAnnotation = true;
		}
	}
}

void CMapDataExt::SetNewOverlayAt(int x, int y, WORD ovr)
{
	if (ovr >= 0xFF && !ExtConfigs::ExtOverlays && NewINIFormat < 5)
		ovr = 0xFFFF;

	auto pExt = GetExtension();
	int dwPos = pExt->GetCoordIndex(x, y);
	int olyPos = y + x * 512;

	if (olyPos > 262144 || dwPos > pExt->CellDataCount) return;

	auto& ovrl = pExt->CellDataExts[dwPos].NewOverlay;
	auto& ovrld = pExt->CellDatas[dwPos].OverlayData;

	pExt->DeleteTiberium(std::min(ovrl, (word)0xFF), ovrld);

	pExt->NewOverlay[olyPos] = ovr;
	pExt->Overlay[olyPos] = std::min(ovr, (WORD)0xff);
	pExt->CellDataExts[dwPos].NewOverlay = ovr;
	pExt->CellDatas[dwPos].Overlay = std::min(ovr, (WORD)0xff);

	pExt->OverlayData[olyPos] = 0;
	pExt->CellDatas[dwPos].OverlayData = 0;

	auto& ovrl2 = pExt->CellDataExts[dwPos].NewOverlay;
	auto& ovrld2 = pExt->CellDatas[dwPos].OverlayData;
	pExt->AddTiberium(std::min(ovrl2, (word)0xFF), ovrld2);

	int i, e;
	for (i = -1; i < 2; i++)
		for (e = -1; e < 2; e++)
			if (pExt->IsCoordInMap(x + i, y + e))
				pExt->SmoothTiberium(pExt->GetCoordIndex(x + i, y + e));


	pExt->UpdateMapPreviewAt(x, y);
}

void CMapDataExt::SetNewOverlayAt(int pos, WORD ovr)
{
	auto pExt = GetExtension();
	int x = pExt->GetXFromCoordIndex(pos);
	int y = pExt->GetYFromCoordIndex(pos);
	SetNewOverlayAt(x, y, ovr);
}

WORD CMapDataExt::GetOverlayAt(int x, int y)
{
	return GetOverlayAt(GetCoordIndex(x, y));
}

WORD CMapDataExt::GetOverlayAt(int pos)
{
	if (pos >= CellDataCount || pos < 0)
		return 0xffff;
	return CellDataExts[pos].NewOverlay;
}

OverlayTypeData CMapDataExt::GetOverlayTypeData(WORD index)
{
	if (index < CMapDataExt::OverlayTypeDatas.size())
	{
		return CMapDataExt::OverlayTypeDatas[index];
	}
	OverlayTypeData ret;
	ret.Rock = false;
	ret.Crate = false;
	ret.Wall = false;
	ret.Veins = false;
	ret.Rubble = false;
	ret.Tiberium = false;
	ret.CustomPaletteName = "";
	ret.TerrainRock = false;
	ret.RadarColor.R = 0;
	ret.RadarColor.G = 0;
	ret.RadarColor.B = 0;

	return ret;
}

int CMapDataExt::GetBuildingTypeIndex(const FString& ID)
{
	auto itr = BuildingTypes.find(ID);
	if (itr == BuildingTypes.end())
	{
		return -1;
	}
	return itr->second;
}

int CMapDataExt::GetPlayerLocationCountAtCell(int x, int y)
{
	auto pThis = GetExtension();
	if (!pThis->IsMultiOnly())
		return 0;
	int count = 0;
	for (int i = 0; i < 8; ++i)
	{
		FString key;
		key.Format("%d", i);
		if (CINI::CurrentDocument->KeyExists("Waypoints", key))
		{
			auto value = CINI::CurrentDocument->GetInteger("Waypoints", key);
			if (value >= 0)
			{
				int wx = value / 1000;
				int wy = value % 1000;
				for (int lx = wx - 1; lx < wx + 3; ++lx)
				{
					for (int ly = wy - 1; ly < wy + 3; ++ly)
					{
						if (lx == x && ly == y)
						{
							count++;
						}
					}
				}
			}
		}
	}
	return count;
}

void CMapDataExt::AssignCellData(CellData& dst, const CellData& src) 
{
	dst.Unit = src.Unit;
	dst.Infantry[0] = src.Infantry[0];
	dst.Infantry[1] = src.Infantry[1];
	dst.Infantry[2] = src.Infantry[2];
	dst.Aircraft = src.Aircraft;
	dst.Structure = src.Structure;
	dst.TypeListIndex = src.TypeListIndex;
	dst.Terrain = src.Terrain;
	dst.TerrainType = src.TerrainType;
	dst.Smudge = src.Smudge;
	dst.SmudgeType = src.SmudgeType;
	dst.Waypoint = src.Waypoint;

	dst.BaseNode.BuildingID = src.BaseNode.BuildingID;
	dst.BaseNode.BasenodeID = src.BaseNode.BasenodeID;
	dst.BaseNode.House = src.BaseNode.House;

	dst.Overlay = src.Overlay;
	dst.OverlayData = src.OverlayData;
	dst.TileIndex = src.TileIndex;
	dst.TileIndexHiPart = src.TileIndexHiPart;
	dst.TileSubIndex = src.TileSubIndex;
	dst.Height = src.Height;
	dst.IceGrowth = src.IceGrowth;
	dst.CellTag = src.CellTag;
	dst.Tube = src.Tube;
	dst.TubeDataIndex = src.TubeDataIndex;
	dst.StatusFlag = src.StatusFlag;

	dst.Flag = src.Flag;
}

std::unique_ptr<TerrainRecord> CMapDataExt::MakeTerrainRecord(int left, int top, int right, int bottom)
{
	auto data = std::make_unique<TerrainRecord>();
	data->record(left, top, right, bottom);
	return std::move(data);
}

void TerrainRecord::record(int left, int top, int right, int bottom)
{
	auto pThis = CMapDataExt::GetExtension();

	if (left < 0) left = 0;
	if (top < 0) top = 0;
	if (right > pThis->MapWidthPlusHeight) right = pThis->MapWidthPlusHeight;
	if (bottom > pThis->MapWidthPlusHeight) bottom = pThis->MapWidthPlusHeight;

	if (right == 0) right = pThis->MapWidthPlusHeight;
	if (bottom == 0) bottom = pThis->MapWidthPlusHeight;

	int width, height;
	width = right - left;
	height = bottom - top;

	int size = width * height;
	this->left = left;
	this->top = top;
	this->right = right;
	this->bottom = bottom;
	this->bHeight = std::make_unique<BYTE[]>(size);
	this->bMapData = std::make_unique<WORD[]>(size);
	this->bSubTile = std::make_unique<BYTE[]>(size);
	this->bMapData2 = std::make_unique<BYTE[]>(size);
	this->wGround = std::make_unique<WORD[]>(size);
	this->overlay = std::make_unique<WORD[]>(size);
	this->overlaydata = std::make_unique<BYTE[]>(size);
	this->bRedrawTerrain = std::make_unique<BOOL[]>(size);
	this->bRNDData = std::make_unique<BYTE[]>(size);

	int i, e;
	for (i = 0; i < width; i++)
	{
		for (e = 0; e < height; e++)
		{
			int pos_w, pos_r;
			pos_w = i + e * width;
			pos_r = left + i + (top + e) * pThis->MapWidthPlusHeight;
			auto cell = pThis->GetCellAt(pos_r);
			auto& cellExt = pThis->CellDataExts[pos_r];
			this->bHeight[pos_w] = cell->Height;
			this->bMapData[pos_w] = cell->TileIndexHiPart;
			this->bSubTile[pos_w] = cell->TileSubIndex;
			this->bMapData2[pos_w] = cell->IceGrowth;
			this->wGround[pos_w] = cell->TileIndex;
			this->overlay[pos_w] = cellExt.NewOverlay;
			this->overlaydata[pos_w] = cell->OverlayData;
			this->bRedrawTerrain[pos_w] = cell->Flag.RedrawTerrain;
			this->bRNDData[pos_w] = cell->Flag.AltIndex;
		}
	}
}

void TerrainRecord::recover()
{
	auto pThis = CMapDataExt::GetExtension();
	int left, top, width, height;
	left = this->left;
	top = this->top;
	width = this->right - left;
	height = this->bottom - top;

	int i, e;
	for (i = 0; i < width; i++)
	{
		for (e = 0; e < height; e++)
		{
			int pos_w, pos_r;
			pos_r = i + e * width;
			pos_w = left + i + (top + e) * pThis->MapWidthPlusHeight;
			auto cell = pThis->GetCellAt(pos_w);
			auto& cellExt = pThis->CellDataExts[pos_w];

			cell->Height = this->bHeight[pos_r];
			cell->TileIndexHiPart = this->bMapData[pos_r];
			cell->TileSubIndex = this->bSubTile[pos_r];
			cell->IceGrowth = this->bMapData2[pos_r];
			cell->TileIndex = this->wGround[pos_r];

			pThis->DeleteTiberium(std::min(cellExt.NewOverlay, (word)0xFF), cell->OverlayData);
			cellExt.NewOverlay = this->overlay[pos_r];
			cell->Overlay = std::min(this->overlay[pos_r], (word)0xFF);
			cell->OverlayData = this->overlaydata[pos_r];
			CMapDataExt::NewOverlay[e + i * 512] = this->overlay[pos_r];
			pThis->AddTiberium(std::min(cellExt.NewOverlay, (word)0xFF), cell->OverlayData);

			cell->Flag.RedrawTerrain = this->bRedrawTerrain[pos_r];
			cell->Flag.AltIndex = this->bRNDData[pos_r];

			pThis->UpdateMapPreviewAt(left + i, top + e);
		}
	}
}

void ObjectRecord::record(int recordType)
{
	if (!ExtConfigs::UndoRedo_RecordObjects)
		return;
	recordFlags = recordType;
	auto& ini = CMapData::Instance->INI;
	auto recordIniValue = [&ini](const char* lpName, std::vector<FString>& list)
		{
			if (auto pSection = ini.GetSection(lpName))
			{
				for (const auto& [key, value] : pSection->GetEntities())
				{
					list.push_back(value);
				}
			}
		};
	auto recordIniMap = [&ini](const char* lpName, std::map<FString, FString>& list)
		{
			if (auto pSection = ini.GetSection(lpName))
			{
				for (const auto& [key, value] : pSection->GetEntities())
				{
					list[key] = value;
				}
			}
		};
	if (recordType & RecordType::Building)
	{
		recordIniValue("Structures", BuildingList);
		recordedFlages |= RecordType::Building;
	}
	if (recordType & RecordType::Unit)
	{
		recordIniValue("Units", UnitList);
		recordedFlages |= RecordType::Unit;
	}
	if (recordType & RecordType::Aircraft)
	{
		recordIniValue("Aircraft", AircraftList);
		recordedFlages |= RecordType::Aircraft;
	}
	if (recordType & RecordType::Infantry)
	{
		recordIniValue("Infantry", InfantryList);
		recordedFlages |= RecordType::Infantry;
	}
	if (recordType & RecordType::Terrain)
	{
		recordedFlages |= RecordType::Terrain;
		FString key;
		for (const auto& terrain : CMapData::Instance->TerrainDatas)
		{
			if (!terrain.Flag)
			{
				key.Format("%04d", terrain.Y * 1000 + terrain.X);
				TerrainList[key] = terrain.TypeID;
			}	
		}
	}
	if (recordType & RecordType::Smudge)
	{
		recordedFlages |= RecordType::Smudge;
		FString value;
		for (const auto& smudge : CMapData::Instance->SmudgeDatas)
		{
			if (!smudge.Flag)
			{
				value.Format("%s,%d,%d,0", smudge.TypeID, smudge.X, smudge.Y);
				SmudgeList.push_back(value);
			}
		}
	}
	if (recordType & RecordType::Tunnel)
	{
		recordedFlages |= RecordType::Tunnel;
		recordIniValue("Tubes", TunnelList);
	}
	if (recordType & RecordType::Waypoint)
	{
		recordedFlages |= RecordType::Waypoint;
		recordIniMap("Waypoints", WaypointList);
	}
	if (recordType & RecordType::Celltag)
	{
		recordedFlages |= RecordType::Celltag;
		recordIniMap("CellTags", CelltagList);
	}
	if (recordType & RecordType::Annotation)
	{
		recordedFlages |= RecordType::Annotation;
		recordIniMap("Annotations", AnnotationList);
	}
	if (recordType & RecordType::Basenode)
	{
		recordedFlages |= RecordType::Basenode;
		if (auto pSection = ini.GetSection("Houses"))
		{
			for (const auto& [_, house] : pSection->GetEntities())
			{
				int nodeCount = ini.GetInteger(house, "NodeCount", 0);
				if (nodeCount > 0)
				{
					FString key;
					for (int i = 0; i < nodeCount; i++)
					{
						key.Format("%03d", i);
						BasenodeList[house].push_back(ini.GetString(house, key, ""));
					}
				}
				else
					BasenodeList[house];
			}
		}
	}
}

void ObjectRecord::appendRecord(int recordType)
{
	if (!ExtConfigs::UndoRedo_RecordObjects)
		return;
	recordFlags |= recordType;
	auto& ini = CMapData::Instance->INI;
	auto recordIniValue = [&ini](const char* lpName, std::vector<FString>& list)
		{
			if (auto pSection = ini.GetSection(lpName))
			{
				for (const auto& [key, value] : pSection->GetEntities())
				{
					list.push_back(value);
				}
			}
		};
	auto recordIniMap = [&ini](const char* lpName, std::map<FString, FString>& list)
		{
			if (auto pSection = ini.GetSection(lpName))
			{
				for (const auto& [key, value] : pSection->GetEntities())
				{
					list[key] = value;
				}
			}
		};
	if (recordType & RecordType::Building && !(recordedFlages & RecordType::Building))
	{
		recordIniValue("Structures", BuildingList);
		recordedFlages |= RecordType::Building;
	}
	if (recordType & RecordType::Unit && !(recordedFlages & RecordType::Unit))
	{
		recordIniValue("Units", UnitList);
		recordedFlages |= RecordType::Unit;
	}
	if (recordType & RecordType::Aircraft && !(recordedFlages & RecordType::Aircraft))
	{
		recordIniValue("Aircraft", AircraftList);
		recordedFlages |= RecordType::Aircraft;
	}
	if (recordType & RecordType::Infantry && !(recordedFlages & RecordType::Infantry))
	{
		recordIniValue("Infantry", InfantryList);
		recordedFlages |= RecordType::Infantry;
	}
	if (recordType & RecordType::Terrain && !(recordedFlages & RecordType::Terrain))
	{
		recordedFlages |= RecordType::Terrain;
		FString key;
		for (const auto& terrain : CMapData::Instance->TerrainDatas)
		{
			if (!terrain.Flag)
			{
				key.Format("%04d", terrain.Y * 1000 + terrain.X);
				TerrainList[key] = terrain.TypeID;
			}
		}
	}
	if (recordType & RecordType::Smudge && !(recordedFlages & RecordType::Smudge))
	{
		recordedFlages |= RecordType::Smudge;
		FString value;
		for (const auto& smudge : CMapData::Instance->SmudgeDatas)
		{
			if (!smudge.Flag)
			{
				value.Format("%s,%d,%d,0", smudge.TypeID, smudge.X, smudge.Y);
				SmudgeList.push_back(value);
			}
		}
	}
	if (recordType & RecordType::Tunnel && !(recordedFlages & RecordType::Tunnel))
	{
		recordedFlages |= RecordType::Tunnel;
		recordIniValue("Tubes", TunnelList);
	}
	if (recordType & RecordType::Waypoint && !(recordedFlages & RecordType::Waypoint))
	{
		recordedFlages |= RecordType::Waypoint;
		recordIniMap("Waypoints", WaypointList);
	}
	if (recordType & RecordType::Celltag && !(recordedFlages & RecordType::Celltag))
	{
		recordedFlages |= RecordType::Celltag;
		recordIniMap("CellTags", CelltagList);
	}
	if (recordType & RecordType::Annotation && !(recordedFlages & RecordType::Annotation))
	{
		recordedFlages |= RecordType::Annotation;
		recordIniMap("Annotations", AnnotationList);
	}
	if (recordType & RecordType::Basenode && !(recordedFlages & RecordType::Basenode))
	{
		recordedFlages |= RecordType::Basenode;
		if (auto pSection = ini.GetSection("Houses"))
		{
			for (const auto& [_, house] : pSection->GetEntities())
			{
				int nodeCount = ini.GetInteger(house, "NodeCount", 0);
				if (nodeCount > 0)
				{
					FString key;
					for (int i = 0; i < nodeCount; i++)
					{
						key.Format("%03d", i);
						BasenodeList[house].push_back(ini.GetString(house, key, ""));
					}
				}
				else
					BasenodeList[house];
			}
		}
	}
}

void ObjectRecord::recover()
{
	if (!ExtConfigs::UndoRedo_RecordObjects)
		return;

	auto& map = CMapData::Instance;
	auto& ini = map->INI;
	auto recoverIniValue = [&ini](const char* lpName, std::vector<FString>& list)
		{
			ini.DeleteSection(lpName);
			if (list.empty())
				return;
			auto pSection = ini.AddSection(lpName);
			int index = 0;
			FString key;
			for (const auto& value : list)
			{
				key.Format("%d", index++);
				ini.WriteString(pSection, key, value);
			}
		};
	auto recoverIniMap = [&ini](const char* lpName, std::map<FString, FString>& list)
		{
			ini.DeleteSection(lpName);
			if (list.empty())
				return;
			auto pSection = ini.AddSection(lpName);
			for (const auto& [key, value] : list)
			{
				ini.WriteString(pSection, key, value);
			}
		};
	if (recordFlags & RecordType::Building)
	{
		recoverIniValue("Structures", BuildingList);
		CMapDataExt::UpdateFieldStructureData_RedrawMinimap();
	}
	if (recordFlags & RecordType::Unit)
	{
		recoverIniValue("Units", UnitList);
		CMapDataExt::UpdateFieldUnitData_RedrawMinimap();
	}
	if (recordFlags & RecordType::Aircraft)
	{
		recoverIniValue("Aircraft", AircraftList);
		CMapDataExt::UpdateFieldAircraftData_RedrawMinimap();
	}
	if (recordFlags & RecordType::Infantry)
	{
		recoverIniValue("Infantry", InfantryList);
		CMapDataExt::UpdateFieldInfantryData_RedrawMinimap();
	}
	if (recordFlags & RecordType::Terrain)
	{
		if (TerrainList.empty())
		{		
			for (int i = 0; i < map->TerrainDatas.size(); ++i)
			{
				map->DeleteTerrainData(i);
			}
		}
		recoverIniMap("Terrain", TerrainList);
		map->UpdateFieldTerrainData(false);
	}
	if (recordFlags & RecordType::Smudge)
	{
		if (SmudgeList.empty())
		{
			for (int i = 0; i < map->SmudgeDatas.size(); ++i)
			{
				map->DeleteSmudgeData(i);
			}
		}
		recoverIniValue("Smudge", SmudgeList);
		map->UpdateFieldSmudgeData(false);
	}
	if (recordFlags & RecordType::Tunnel)
	{
		recoverIniValue("Tubes", TunnelList);
		map->UpdateFieldTubeData(false);
	}
	if (recordFlags & RecordType::Waypoint)
	{
		std::map<int, FString> oriWps;
		if (map->IsMultiOnly())
			if (auto pSection = CINI::CurrentDocument->GetSection("Waypoints"))
				for (const auto& data : pSection->GetEntities())
					oriWps[atoi(data.first)] = data.second;
		recoverIniMap("Waypoints", WaypointList);
		if (map->IsMultiOnly())
		{
			for (int i = 0; i < map->CellDataCount; ++i)
				map->CellDatas[i].Waypoint = -1;
			for (const auto& data : oriWps)
			{
				if (data.first < 8)
				{
					int x = atoi(data.second) / 1000;
					int y = atoi(data.second) % 1000;
					int k, l;
					for (k = -1; k < 2; k++)
						for (l = -1; l < 2; l++)
							map->UpdateMapPreviewAt(x + k, y + l);
				}
			}
		}
		map->UpdateFieldWaypointData(false);
	}
	if (recordFlags & RecordType::Celltag)
	{
		recoverIniMap("CellTags", CelltagList);
		map->UpdateFieldCelltagData(false);
	}
	if (recordFlags & RecordType::Annotation)
	{
		recoverIniMap("Annotations", AnnotationList);
		CMapDataExt::UpdateAnnotation();
	}
	if (recordFlags & RecordType::Basenode)
	{
		if (auto pSection = ini.GetSection("Houses"))
		{
			for (const auto& [_, house] : pSection->GetEntities())
			{
				auto houseMap = BasenodeList.find(house);
				if (houseMap != BasenodeList.end())
				{
					if (auto pHouseSection = ini.GetSection(house))
					{
						auto& nodes = houseMap->second;
						int nodeCount = std::max(ini.GetInteger(pHouseSection, "NodeCount", 0), (int)nodes.size());
						ini.WriteString(pHouseSection, "NodeCount", STDHelpers::IntToString(nodes.size()));
						if (nodeCount > 0)
						{
							FString key;
							for (int i = 0; i < nodeCount; i++)
							{
								key.Format("%03d", i);
								if (i < nodes.size())
								{
									ini.WriteString(pHouseSection, key, nodes[i]);
								}
								else
								{
									ini.DeleteKey(pHouseSection, key);
								}
							}
						}
					}		
				}
			}
		}
		map->UpdateFieldBasenodeData(false);
	}

	CFinalSunDlg::Instance->MyViewFrame.Minimap.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

void MixedRecord::record(int left, int top, int right, int bottom, int recordType)
{
	this->terrain.record(left, top, right, bottom);
	this->object.record(recordType);
}

void MixedRecord::recover()
{
	this->terrain.recover();
	this->object.recover();
}

ObjectRecord* CMapDataExt::MakeObjectRecord(int recordType, bool recordOnce)
{
	if (!ExtConfigs::UndoRedo_RecordObjects)
		return nullptr;
	if (recordOnce && !CIsoViewExt::HistoryRecord_IsHoldingLButton)
	{
		CIsoViewExt::HistoryRecord_IsHoldingLButton = true;
	}	
	else if (recordOnce && CIsoViewExt::HistoryRecord_IsHoldingLButton)
	{
		return nullptr;
	}
	if (recordType > 0)
	{
		auto pThis = GetExtension();
		// always erase following
		pThis->UndoRedoDatas.resize(pThis->UndoRedoDataIndex + 1);

		if (pThis->UndoRedoDatas.size() + 1 > ExtConfigs::UndoRedoLimit)
		{
			pThis->UndoRedoDatas.erase(0);
		}

		pThis->UndoRedoDataIndex = pThis->UndoRedoDatas.size();
		pThis->UndoRedoDatas.add(recordType);
		return dynamic_cast<ObjectRecord*>(pThis->UndoRedoDatas.get(pThis->UndoRedoDatas.size() - 1));
	}
	return nullptr;
}

void CMapDataExt::MakeMixedRecord(int left, int top, int right, int bottom, int recordType)
{
	auto pThis = GetExtension();
	// always erase following
	pThis->UndoRedoDatas.resize(pThis->UndoRedoDataIndex + 1);

	if (pThis->UndoRedoDatas.size() + 1 > ExtConfigs::UndoRedoLimit)
	{
		pThis->UndoRedoDatas.erase(0);
	}

	pThis->UndoRedoDataIndex = pThis->UndoRedoDatas.size();
	pThis->UndoRedoDatas.add(left, top, right, bottom, recordType);
}

void CMapDataExt::MakePreviewRecord(int left, int top, int right, int bottom)
{
	auto pThis = GetExtension();
	pThis->PreviewHistoryData.clear();
	pThis->PreviewHistoryData.add(left, top, right, bottom, 0);
}

void CMapDataExt::RestorePreviewRecord()
{
	auto pThis = GetExtension();
	if (auto data = pThis->PreviewHistoryData.get(0))
		if (auto* mr = dynamic_cast<MixedRecord*>(data))
			mr->recover();
}

void CMapDataExt::UpdateFieldStructureData_RedrawMinimap()
{
	int i = 0;
	auto pThis = GetExtension();
	for (i = 0; i < pThis->CellDataCount; i++)
	{
		pThis->CellDataExts[i].RecordMinimapUpdateIndex[0] = pThis->CellDatas[i].Structure;
	}

	pThis->UpdateFieldStructureData_Optimized();

	for (i = 0; i < pThis->CellDataCount; i++)
	{
		if (pThis->CellDataExts[i].RecordMinimapUpdateIndex[0] != -1 && pThis->CellDatas[i].Structure == -1)
		{
			int x = pThis->GetXFromCoordIndex(i);
			int y = pThis->GetYFromCoordIndex(i);
			pThis->UpdateMapPreviewAt(x, y);
		}
	}
}

void CMapDataExt::UpdateFieldUnitData_RedrawMinimap()
{
	int i = 0;
	auto pThis = GetExtension();
	for (i = 0; i < pThis->CellDataCount; i++)
	{
		pThis->CellDataExts[i].RecordMinimapUpdateIndex[0] = pThis->CellDatas[i].Unit;
	}

	pThis->UpdateFieldUnitData(false);

	for (i = 0; i < pThis->CellDataCount; i++)
	{
		if (pThis->CellDataExts[i].RecordMinimapUpdateIndex[0] != -1 && pThis->CellDatas[i].Unit == -1)
		{
			int x = pThis->GetXFromCoordIndex(i);
			int y = pThis->GetYFromCoordIndex(i);
			pThis->UpdateMapPreviewAt(x, y);
		}
	}
}

void CMapDataExt::UpdateFieldInfantryData_RedrawMinimap()
{
	int i = 0;
	auto pThis = GetExtension();
	for (i = 0; i < pThis->CellDataCount; i++)
	{
		for (int j = 0; j < 3; ++j)
			pThis->CellDataExts[i].RecordMinimapUpdateIndex[j] = pThis->CellDatas[i].Infantry[j];
	}

	pThis->UpdateFieldInfantryData(false);

	for (i = 0; i < pThis->CellDataCount; i++)
	{
		bool redraw = false;
		for (int j = 0; j < 3; ++j)
			if (pThis->CellDataExts[i].RecordMinimapUpdateIndex[j] != -1 && pThis->CellDatas[i].Infantry[j] == -1)
			{
				redraw = true;
				break;
			}
		if (redraw)
		{
			int x = pThis->GetXFromCoordIndex(i);
			int y = pThis->GetYFromCoordIndex(i);
			pThis->UpdateMapPreviewAt(x, y);
		}
	}
}

void CMapDataExt::UpdateFieldAircraftData_RedrawMinimap()
{
	int i = 0;
	auto pThis = GetExtension();
	for (i = 0; i < pThis->CellDataCount; i++)
	{
		pThis->CellDataExts[i].RecordMinimapUpdateIndex[0] = pThis->CellDatas[i].Aircraft;
	}

	pThis->UpdateFieldAircraftData(false);

	for (i = 0; i < pThis->CellDataCount; i++)
	{
		if (pThis->CellDataExts[i].RecordMinimapUpdateIndex[0] != -1 && pThis->CellDatas[i].Aircraft == -1)
		{
			int x = pThis->GetXFromCoordIndex(i);
			int y = pThis->GetYFromCoordIndex(i);
			pThis->UpdateMapPreviewAt(x, y);
		}
	}
}

void CMapDataExt::InitializeTileData()
{
	if (CMapDataExt::TileData)
		delete[] CMapDataExt::TileData;
	CMapDataExt::TileData = nullptr;

	auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");
	thisTheater.MakeUpper();
	if (thisTheater == "TEMPERATE" && CTileTypeInfo::Temperate().Datas)
	{
		CurrentTheaterIndex = 0;
		TileDataCount = CTileTypeInfo::Temperate().Count;
		TileData = new CTileTypeClass[CMapDataExt::TileDataCount];
		memcpy(TileData, CTileTypeInfo::Temperate().Datas, TileDataCount * sizeof(CTileTypeClass));
	}
	else if (thisTheater == "SNOW" && CTileTypeInfo::Snow().Datas)
	{
		CurrentTheaterIndex = 1;
		TileDataCount = CTileTypeInfo::Snow().Count;
		TileData = new CTileTypeClass[CMapDataExt::TileDataCount];
		memcpy(TileData, CTileTypeInfo::Snow().Datas, TileDataCount * sizeof(CTileTypeClass));
	}
	else if (thisTheater == "URBAN" && CTileTypeInfo::Urban().Datas)
	{
		CurrentTheaterIndex = 2;
		TileDataCount = CTileTypeInfo::Urban().Count;
		TileData = new CTileTypeClass[CMapDataExt::TileDataCount];
		memcpy(TileData, CTileTypeInfo::Urban().Datas, TileDataCount * sizeof(CTileTypeClass));
	}
	else if (thisTheater == "NEWURBAN" && CTileTypeInfo::NewUrban().Datas)
	{
		CurrentTheaterIndex = 3;
		TileDataCount = CTileTypeInfo::NewUrban().Count;
		TileData = new CTileTypeClass[CMapDataExt::TileDataCount];
		memcpy(TileData, CTileTypeInfo::NewUrban().Datas, TileDataCount * sizeof(CTileTypeClass));
	}
	else if (thisTheater == "LUNAR" && CTileTypeInfo::Lunar().Datas)
	{
		CurrentTheaterIndex = 4;
		TileDataCount = CTileTypeInfo::Lunar().Count;
		TileData = new CTileTypeClass[CMapDataExt::TileDataCount];
		memcpy(TileData, CTileTypeInfo::Lunar().Datas, TileDataCount * sizeof(CTileTypeClass));
	}
	else if (thisTheater == "DESERT" && CTileTypeInfo::Desert().Datas)
	{
		CurrentTheaterIndex = 5;
		TileDataCount = CTileTypeInfo::Desert().Count;
		TileData = new CTileTypeClass[CMapDataExt::TileDataCount];
		memcpy(TileData, CTileTypeInfo::Desert().Datas, TileDataCount * sizeof(CTileTypeClass));
	}

	if (!CMapDataExt::TileData)
	{
		Logger::Error("CMapDataExt::InitializeTileData() cannot initialize tile data!\n");
	}
}

void CustomTileBlock::SetTileBlock(int tile, int subtile, int height)
{
	Height = height;
	TileIndex = tile;
	SubTileIndex = subtile;
	if (CMapDataExt::TileDataCount > TileIndex
		&& CMapDataExt::TileData[TileIndex].TileBlockCount > SubTileIndex
		&& CMapDataExt::TileData[TileIndex].TileBlockDatas[SubTileIndex].ImageData)
		TileBlock = &CMapDataExt::TileData[TileIndex].TileBlockDatas[SubTileIndex];
	else
		TileBlock = nullptr;

	FrameTileIndex = TileIndex;
	if (CMapDataExt::TileData[TileIndex].FrameModeIndex != 0xFFFF)
		FrameTileIndex = CMapDataExt::TileData[TileIndex].FrameModeIndex;

	if (CMapDataExt::TileDataCount > FrameTileIndex
		&& CMapDataExt::TileData[FrameTileIndex].TileBlockCount > SubTileIndex
		&& CMapDataExt::TileData[FrameTileIndex].TileBlockDatas[SubTileIndex].ImageData)
		FrameTileBlock = &CMapDataExt::TileData[FrameTileIndex].TileBlockDatas[SubTileIndex];
	else
		FrameTileBlock = TileBlock;
}

int CustomTileBlock::GetHeight() const
{
	int height = 0;
	if (TileBlock)
		height = Height;// +TileBlock->Height;
	if (height > 14) height = 14;
	return height;
}

CTileBlockClass* CustomTileBlock::GetDisplayTileBlock()
{
	if (CFinalSunApp::Instance->FrameMode && FrameTileBlock)
	{
		return FrameTileBlock;
	}
	return TileBlock;
}

int CustomTileBlock::GetDisplayTileIndex() const
{
	if (CFinalSunApp::Instance->FrameMode && FrameTileBlock)
	{
		return FrameTileIndex;
	}
	return TileIndex;
}

void CustomTile::Initialize(int witdh, int height)
{
	Width = witdh;
	Height = height;
	TileBlockDatas = std::make_unique<CustomTileBlock[]>(Width * Height);
}

CustomTile* CMapDataExt::GetCustomTile(int tileIndex)
{
	tileIndex -= CUSTOM_TILE_START;
	for (auto& [tileset, ct] : CMapDataExt::CustomTiles)
	{
		if (tileIndex < 0)
			return nullptr;
		if (tileIndex < ct.size())
		{
			return &ct[tileIndex];
		}
		else
		{
			tileIndex -= ct.size();
			continue;
		}
	}
	return nullptr;
}

int CMapDataExt::GetCustomTileSet(int tileIndex)
{
	tileIndex -= CUSTOM_TILE_START;
	for (auto& [tileset, ct] : CMapDataExt::CustomTiles)
	{
		if (tileIndex < 0)
			return 0;
		if (tileIndex < ct.size())
		{
			return tileset;
		}
		else
		{
			tileIndex -= ct.size();
			continue;
		}
	}
	return 0;
}

int CMapDataExt::GetCustomTileIndex(int tileSet, int tileIndex)
{
	int ret = CUSTOM_TILE_START;
	for (auto& [tileset, ct] : CMapDataExt::CustomTiles)
	{
		if (tileset < tileSet)
		{
			ret += ct.size();
		}
		else if (tileset == tileSet)
		{
			ret += tileIndex;
			return ret;
		}
	}
	return 0;
}

std::vector<TechnoAttachment>* CMapDataExt::GetTechnoAttachmentInfo(const FString& ID)
{
	auto itr = TechnoAttachments.find(ID);
	if (itr != TechnoAttachments.end())
		return &itr->second;
	return nullptr;
}

void CMapDataExt::CheckCellLow(bool steep, int loopCount, bool IgnoreMorphable, std::vector<int>* ignoreList)
{
	auto isMorphable = [IgnoreMorphable](CellData* cell)
	{
		if (!cell) return false;
		if (ExtConfigs::PlaceTileSkipHide && cell->IsHidden())
			return false;
		if (IgnoreMorphable) return true;
		int groundClick = cell->TileIndex;
		if (groundClick == 0xFFFF) groundClick = 0;
		return CMapDataExt::TileData[groundClick].Morphable != 0;
	};

	auto getIndex = [](int x, int y)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return -1;
		return x + y * CMapData::Instance->MapWidthPlusHeight;
	};
	// additional check: lower tile must has 2x2 square
	if (loopCount == 0)
	{
		int adjustedCount = 0;
		int index = -1;
		for (int i = 1; i < CMapDataExt::CellDataExts.size(); i++)
		{
			if (CMapDataExt::CellDataExts[i].Adjusted)
			{
				adjustedCount++;
				index = i;
			}
		}
		if (adjustedCount == 1)
		{
			int x = CMapData::Instance->GetXFromCoordIndex(index);
			int y = CMapData::Instance->GetYFromCoordIndex(index);
			auto cellOri = CMapData::Instance->GetCellAt(index);
			MapCoord loops[3] = { {0,-1},{-1,0},{-1,-1} };
			for (auto& coord : loops)
			{
				if (CMapDataExt::IsCoordInFullMap(x + coord.X, y + coord.Y))
				{
					int pos = CMapData::Instance->GetCoordIndex(x + coord.X, y + coord.Y);
					auto cell = CMapData::Instance->GetCellAt(pos);
					// make sure adjust for lower
					if (cell->Height > cellOri->Height && isMorphable(cell))
					{
						cell->Height = cellOri->Height;
						CMapDataExt::CellDataExts[pos].Adjusted = true;
					}
				}
			}
		}
	}

	loopCount++;
	if (loopCount > 15)
		return;

	auto getNW = [](int x, int y) { return CMapDataExt::TryGetCellAt(x, y - 1); };
	auto getSE = [](int x, int y) { return CMapDataExt::TryGetCellAt(x, y + 1); };
	auto getNE = [](int x, int y) { return CMapDataExt::TryGetCellAt(x - 1, y); };
	auto getSW = [](int x, int y) { return CMapDataExt::TryGetCellAt(x + 1, y); };
	auto getN = [](int x, int y) { return CMapDataExt::TryGetCellAt(x - 1, y - 1); };
	auto getE = [](int x, int y) { return CMapDataExt::TryGetCellAt(x - 1, y + 1); };
	auto getS = [](int x, int y) { return CMapDataExt::TryGetCellAt(x + 1, y + 1); };
	auto getW = [](int x, int y) { return CMapDataExt::TryGetCellAt(x + 1, y - 1); };

	bool loop = false;

	for (int i = 1; i < CMapDataExt::CellDataExts.size(); i++)
	{
		if (!CMapDataExt::CellDataExts[i].Adjusted)
			continue;

		int height = std::clamp((int)CMapData::Instance->CellDatas[i].Height, 0, 14);
		int thisX = CMapData::Instance->GetXFromCoordIndex(i);
		int thisY = CMapData::Instance->GetYFromCoordIndex(i);

		auto checkAndLower = [&](CellData* neighbor, int nx, int ny, int limitDiff)
		{
			if (!neighbor) return false;
			if (!isMorphable(neighbor)) return false;
			int index = getIndex(nx, ny);
			if (index < 0) return false;
			if (ignoreList && std::find(ignoreList->begin(), ignoreList->end(), index) != ignoreList->end())
				return false;

			int diff = neighbor->Height - height;
			if (diff > limitDiff)
			{
				neighbor->Height = height + limitDiff;
				CMapDataExt::CellDataExts[index].Adjusted = true;
				return true;
			}
			return false;
		};

		auto nw = getNW(thisX, thisY);
		auto ne = getNE(thisX, thisY);
		auto sw = getSW(thisX, thisY);
		auto se = getSE(thisX, thisY);
		auto n = getN(thisX, thisY);
		auto s = getS(thisX, thisY);
		auto e = getE(thisX, thisY);
		auto w = getW(thisX, thisY);

		loop |= checkAndLower(nw, thisX, thisY - 1, 1);
		loop |= checkAndLower(se, thisX, thisY + 1, 1);
		loop |= checkAndLower(sw, thisX + 1, thisY, 1);
		loop |= checkAndLower(ne, thisX - 1, thisY, 1);

		if (!steep)
		{
			loop |= checkAndLower(n, thisX - 1, thisY - 1, 1);
			loop |= checkAndLower(s, thisX + 1, thisY + 1, 1);
			loop |= checkAndLower(w, thisX + 1, thisY - 1, 1);
			loop |= checkAndLower(e, thisX - 1, thisY + 1, 1);

		}
	}

	if (loop)
		CheckCellLow(steep, loopCount, IgnoreMorphable, ignoreList);
}

void CMapDataExt::CheckCellRise(bool steep, int loopCount, bool IgnoreMorphable, std::vector<int>* ignoreList)
{
	loopCount++;
	if (loopCount > 15)
		return;

	bool loop = false;

	auto getIndex = [](int x, int y)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return 0;
		return x + y * CMapData::Instance->MapWidthPlusHeight;
	};

	auto isMorphable = [IgnoreMorphable](CellData* cell)
	{
		if (!cell) return 0;
		if (ExtConfigs::PlaceTileSkipHide && cell->IsHidden())
			return 0;
		if (IgnoreMorphable) return 1;
		int groundClick = cell->TileIndex;
		if (groundClick == 0xFFFF) groundClick = 0;
		return CMapDataExt::TileData[groundClick].Morphable;
	};

	auto getNW = [](int x, int y) { return CMapDataExt::TryGetCellAt(x, y - 1); };
	auto getSE = [](int x, int y) { return CMapDataExt::TryGetCellAt(x, y + 1); };
	auto getNE = [](int x, int y) { return CMapDataExt::TryGetCellAt(x - 1, y); };
	auto getSW = [](int x, int y) { return CMapDataExt::TryGetCellAt(x + 1, y); };
	auto getN = [](int x, int y) { return CMapDataExt::TryGetCellAt(x - 1, y - 1); };
	auto getE = [](int x, int y) { return CMapDataExt::TryGetCellAt(x - 1, y + 1); };
	auto getS = [](int x, int y) { return CMapDataExt::TryGetCellAt(x + 1, y + 1); };
	auto getW = [](int x, int y) { return CMapDataExt::TryGetCellAt(x + 1, y - 1); };

	for (int i = 1; i < (int)CMapDataExt::CellDataExts.size(); i++) // skip 0
	{
		if (CMapDataExt::CellDataExts[i].Adjusted)
		{
			int height = CMapData::Instance->CellDatas[i].Height;
			if (height < 0)  height = 0;
			if (height > 14) height = 14;

			int thisX = CMapData::Instance->GetXFromCoordIndex(i);
			int thisY = CMapData::Instance->GetYFromCoordIndex(i);

			auto onw = getNW(thisX, thisY);
			auto one = getNE(thisX, thisY);
			auto osw = getSW(thisX, thisY);
			auto ose = getSE(thisX, thisY);
			auto on = getN(thisX, thisY);
			auto os = getS(thisX, thisY);
			auto oe = getE(thisX, thisY);
			auto ow = getW(thisX, thisY);

			if (isMorphable(onw) && height - onw->Height > 1)
			{
				int idx2 = getIndex(thisX, thisY - 1);
				if (!ignoreList || std::find(ignoreList->begin(), ignoreList->end(), idx2) == ignoreList->end())
				{
					onw->Height = height - 1;
					CMapDataExt::CellDataExts[idx2].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(ose) && height - ose->Height > 1)
			{
				int idx2 = getIndex(thisX, thisY + 1);
				if (!ignoreList || std::find(ignoreList->begin(), ignoreList->end(), idx2) == ignoreList->end())
				{
					ose->Height = height - 1;
					CMapDataExt::CellDataExts[idx2].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(osw) && height - osw->Height > 1)
			{
				int idx2 = getIndex(thisX + 1, thisY);
				if (!ignoreList || std::find(ignoreList->begin(), ignoreList->end(), idx2) == ignoreList->end())
				{
					osw->Height = height - 1;
					CMapDataExt::CellDataExts[idx2].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(one) && height - one->Height > 1)
			{
				int idx2 = getIndex(thisX - 1, thisY);
				if (!ignoreList || std::find(ignoreList->begin(), ignoreList->end(), idx2) == ignoreList->end())
				{
					one->Height = height - 1;
					CMapDataExt::CellDataExts[idx2].Adjusted = true;
					loop = true;
				}
			}
			if (!steep)
			{
				if (isMorphable(on) && height - on->Height > 1)
				{
					int idx2 = getIndex(thisX - 1, thisY - 1);
					if (!ignoreList || std::find(ignoreList->begin(), ignoreList->end(), idx2) == ignoreList->end())
					{
						on->Height = height - 1;
						CMapDataExt::CellDataExts[idx2].Adjusted = true;
						loop = true;
					}
				}
				if (isMorphable(os) && height - os->Height > 1)
				{
					int idx2 = getIndex(thisX + 1, thisY + 1);
					if (!ignoreList || std::find(ignoreList->begin(), ignoreList->end(), idx2) == ignoreList->end())
					{
						os->Height = height - 1;
						CMapDataExt::CellDataExts[idx2].Adjusted = true;
						loop = true;
					}
				}
				if (isMorphable(ow) && height - ow->Height > 1)
				{
					int idx2 = getIndex(thisX + 1, thisY - 1);
					if (!ignoreList || std::find(ignoreList->begin(), ignoreList->end(), idx2) == ignoreList->end())
					{
						ow->Height = height - 1;
						CMapDataExt::CellDataExts[idx2].Adjusted = true;
						loop = true;
					}
				}
				if (isMorphable(oe) && height - oe->Height > 1)
				{
					int idx2 = getIndex(thisX - 1, thisY + 1);
					if (!ignoreList || std::find(ignoreList->begin(), ignoreList->end(), idx2) == ignoreList->end())
					{
						oe->Height = height - 1;
						CMapDataExt::CellDataExts[idx2].Adjusted = true;
						loop = true;
					}
				}
			}

			int loops[3] = { 0, -1, 1 };
			for (int dx : loops)
				for (int dy : loops)
				{
					int newX = thisX + dx;
					int newY = thisY + dy;
					if (!CMapData::Instance->IsCoordInMap(newX, newY)) continue;

					auto cell = CMapDataExt::TryGetCellAt(newX, newY);
					if (!cell) continue;

					int ground = cell->TileIndex;
					if (ground == 0xFFFF) ground = 0;
					if (!CMapDataExt::TileData[ground].Morphable) continue;

					if (height - cell->Height <= 0) continue;

					auto nw = getNW(newX, newY);
					auto ne = getNE(newX, newY);
					auto sw = getSW(newX, newY);
					auto se = getSE(newX, newY);
					auto n = getN(newX, newY);
					auto s = getS(newX, newY);
					auto e = getE(newX, newY);
					auto w = getW(newX, newY);

					bool trigger =
						(nw && se && nw->Height - cell->Height > 0 && se->Height - cell->Height > 0)
						|| (sw && ne && sw->Height - cell->Height > 0 && ne->Height - cell->Height > 0)
						|| (s && ne && se && s->Height - cell->Height > 0 && se->Height - cell->Height <= 0 && ne->Height - cell->Height > 0)
						|| (s && nw && sw && s->Height - cell->Height > 0 && sw->Height - cell->Height <= 0 && nw->Height - cell->Height > 0)
						|| (n && se && ne && n->Height - cell->Height > 0 && ne->Height - cell->Height <= 0 && se->Height - cell->Height > 0)
						|| (n && sw && nw && n->Height - cell->Height > 0 && nw->Height - cell->Height <= 0 && sw->Height - cell->Height > 0)
						|| (e && nw && ne && e->Height - cell->Height > 0 && ne->Height - cell->Height <= 0 && nw->Height - cell->Height > 0)
						|| (e && sw && se && e->Height - cell->Height > 0 && se->Height - cell->Height <= 0 && sw->Height - cell->Height > 0)
						|| (w && ne && nw && w->Height - cell->Height > 0 && nw->Height - cell->Height <= 0 && ne->Height - cell->Height > 0)
						|| (w && se && sw && w->Height - cell->Height > 0 && sw->Height - cell->Height <= 0 && se->Height - cell->Height > 0);

					int tallerCount = 0;
					if (n && (n->Height - cell->Height > 0)) tallerCount++;
					if (s && (s->Height - cell->Height > 0)) tallerCount++;
					if (w && (w->Height - cell->Height > 0)) tallerCount++;
					if (e && (e->Height - cell->Height > 0)) tallerCount++;
					if (nw && (nw->Height - cell->Height > 0)) tallerCount++;
					if (ne && (ne->Height - cell->Height > 0)) tallerCount++;
					if (sw && (sw->Height - cell->Height > 0)) tallerCount++;
					if (se && (se->Height - cell->Height > 0)) tallerCount++;

					if (!trigger && tallerCount < 6) continue;

					int idx2 = getIndex(newX, newY);
					if (ignoreList && std::find(ignoreList->begin(), ignoreList->end(), idx2) != ignoreList->end())
						continue;

					if (!isMorphable(cell)) continue;

					const int diagLimit = 1;
					const int cardLimit = steep ? 2 : 1;

					int cap = 14;
					auto relax = [&](CellData* nb, int limit) {
						if (nb) cap = std::min(cap, (isMorphable(nb) ? nb->Height : height) + limit);
					};

					relax(nw, diagLimit);
					relax(ne, diagLimit);
					relax(sw, diagLimit);
					relax(se, diagLimit);

					relax(n, cardLimit);
					relax(s, cardLimit);
					relax(e, cardLimit);
					relax(w, cardLimit);

					int target = std::min(height, cap);
					target = std::max(target, (int)cell->Height);
					if (target > cell->Height)
					{
						cell->Height = target;
						CMapDataExt::CellDataExts[idx2].Adjusted = true;
						loop = true;
					}
				}
		}
	}

	if (loop)
		CheckCellRise(steep, loopCount, IgnoreMorphable, ignoreList);
}

void CMapDataExt::InitializeAllHdmEdition(bool updateMinimap, bool reloadCellDataExt, bool reloadImages)
{
	Logger::Debug("CMapDataExt::InitializeAllHdmEdition() Called with parameter %d %d %d.\n", updateMinimap, reloadCellDataExt, reloadImages);
	
	if (updateMinimap && reloadCellDataExt && reloadImages)
		CTileSetBrowserFrameExt::TerrainDlgLoaded = false;

	CLoadingExt::g_cache[0].clear();
	CLoadingExt::g_cache[1].clear();
	CLoadingExt::g_cacheTime[0].clear();
	CLoadingExt::g_cacheTime[1].clear();
	CLoadingExt::g_lastCleanup = 0;
	
	CIsoView::CurrentCommand->Type = 0;
	CIsoView::CurrentCommand->Command = 0;
	FA2sp::g_VEH_Enabled = true;

	CIsoView::GetInstance()->CurrentCellObjectIndex = -1;
	CIsoView::GetInstance()->CurrentCellObjectType = -1;
	CIsoView::GetInstance()->Drag = FALSE;
	CIsoViewExt::InitAlphaTable();

	Variables::RulesMap.ClearMap();
	Variables::Rules.ClearMap();
	Variables::Rules_FAData.ClearMap();

	if (reloadCellDataExt)
	{
		CMapDataExt::CellDataExts.clear();
		CMapDataExt::CellDataExts.resize(CMapData::Instance->CellDataCount);
		UndoRedoDatas.clear();
		UndoRedoDataIndex = -1;
	}

	int ovrIdx = 0;
	CMapDataExt::OverlayTypeDatas.clear();
	const auto& overlays = Variables::RulesMap.ParseIndicies("OverlayTypes", true);
	for (const auto& ol : overlays)
	{
		auto& item = CMapDataExt::OverlayTypeDatas.emplace_back();
		item.Rock = Variables::RulesMap.GetBool(ol, "IsARock");
		item.Wall = Variables::RulesMap.GetBool(ol, "Wall");
		item.Crate = Variables::RulesMap.GetBool(ol, "Crate");
		item.Veins = Variables::RulesMap.GetBool(ol, "IsVeins");
		item.Rubble = Variables::RulesMap.GetBool(ol, "IsRubble");
		item.Tiberium = Variables::RulesMap.GetBool(ol, "Tiberium");
		item.CustomPaletteName = CINI::Art->GetString(ol, "Palette");
		item.TerrainRock = Variables::RulesMap.GetString(ol, "Land", "") == "Rock";
		auto name = Variables::RulesMap.GetString(ol, "Name", "");
		name.MakeLower();
		item.RailRoad = Variables::RulesMap.GetString(ol, "Land", "") == "Railroad" 
			|| name.Find("track") > -1 || name.Find("rail") > -1;
		std::vector<FString> colors;

		if (RIPARIUS_BEGIN <= ovrIdx && ovrIdx <= RIPARIUS_END && Variables::RulesMap.KeyExists("Riparius", "MinimapColor"))
		{
			colors = FString::SplitString(Variables::RulesMap.GetString("Riparius", "MinimapColor", "0,0,0"), 2);
		}
		else if (CRUENTUS_BEGIN <= ovrIdx && ovrIdx <= CRUENTUS_END && Variables::RulesMap.KeyExists("Cruentus", "MinimapColor"))
		{
			colors = FString::SplitString(Variables::RulesMap.GetString("Cruentus", "MinimapColor", "0,0,0"), 2);
		}
		else if (VINIFERA_BEGIN <= ovrIdx && ovrIdx <= VINIFERA_END && Variables::RulesMap.KeyExists("Vinifera", "MinimapColor"))
		{
			colors = FString::SplitString(Variables::RulesMap.GetString("Vinifera", "MinimapColor", "0,0,0"), 2);
		}
		else if (ABOREUS_BEGIN <= ovrIdx && ovrIdx <= ABOREUS_END && Variables::RulesMap.KeyExists("Aboreus", "MinimapColor"))
		{
			colors = FString::SplitString(Variables::RulesMap.GetString("Aboreus", "MinimapColor", "0,0,0"), 2);
		}
		else
		{
			colors = FString::SplitString(Variables::RulesMap.GetString(ol, "RadarColor", "0,0,0"), 2);
		}
		item.RadarColor.R = atoi(colors[0]);
		item.RadarColor.G = atoi(colors[1]);
		item.RadarColor.B = atoi(colors[2]);
		ovrIdx++;
	}

	if (reloadImages)
	{
		if (CNewTeamTypes::GetHandle())
			::SendMessage(CNewTeamTypes::GetHandle(), 114514, 0, 0);

		if (CNewTaskforce::GetHandle())
			::SendMessage(CNewTaskforce::GetHandle(), 114514, 0, 0);

		if (CNewScript::GetHandle())
			::SendMessage(CNewScript::GetHandle(), 114514, 0, 0);

		if (CNewTrigger::GetHandle())
			::SendMessage(CNewTrigger::GetHandle(), 114514, 0, 0);
		else
			CMapDataExt::UpdateTriggers();

		if (CNewINIEditor::GetHandle())
			::SendMessage(CNewINIEditor::GetHandle(), 114514, 0, 0);

		if (CNewAITrigger::GetHandle())
			::SendMessage(CNewAITrigger::GetHandle(), 114514, 0, 0);

		if (CLuaConsole::GetHandle())
			::SendMessage(CLuaConsole::GetHandle(), 114514, 0, 0);

		if (CNewLocalVariables::GetHandle())
			::SendMessage(CNewLocalVariables::GetHandle(), 114514, 0, 0);

		if (IsWindowVisible(CCsfEditor::GetHandle()))
		{
			::SendMessage(CCsfEditor::GetHandle(), 114514, 0, 0);
		}
		if (CSearhReference::GetHandle())
		{
			CSearhReference::SetSearchID("");
			::SendMessage(CSearhReference::GetHandle(), WM_CLOSE, 0, 0);
		}
		if (CTriggerAnnotation::GetHandle())
		{
			CTriggerAnnotation::ID = "";
			::SendMessage(CSearhReference::GetHandle(), 114515, 0, 0);
		}
	}

	auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");
	thisTheater.MakeUpper();

	CFinalSunDlgExt::CurrentLighting = 31000;
	CheckMenuRadioItem(*CFinalSunDlg::Instance->GetMenu(), 31000, 31003, 31000, MF_UNCHECKED);
	if (reloadImages)
	{
		PalettesManager::Release();
	}
	FString theaterIg = thisTheater;
	if (theaterIg == "NEWURBAN")
		theaterIg = "UBN";
	FString theaterSuffix = theaterIg.Mid(0, 3);
	FString isoPal;
	isoPal.Format("iso%s.pal", theaterSuffix);
	isoPal.MakeUpper();
	auto pal = PalettesManager::LoadPalette(isoPal);
	CMapDataExt::Palette_ISO = *pal;
	CMapDataExt::Palette_ISO_NoTint = *pal;

	Palette_Shadow.Data[0].R = 255;
	Palette_Shadow.Data[0].G = 255;
	Palette_Shadow.Data[0].B = 255;
	Palette_Shadow.Data[0].Zero = 0;
	for (int i = 1; i < 256; i++)
	{
		Palette_Shadow.Data[i].R = 0;
		Palette_Shadow.Data[i].G = 0;
		Palette_Shadow.Data[i].B = 0;
		Palette_Shadow.Data[i].Zero = 0;
	}

	PaveTile = CINI::CurrentTheater->GetInteger("General", "PaveTile", -10);
	GreenTile = CINI::CurrentTheater->GetInteger("General", "GreenTile", -10);
	MiscPaveTile = CINI::CurrentTheater->GetInteger("General", "MiscPaveTile", -10);
	Medians = CINI::CurrentTheater->GetInteger("General", "Medians", -10);
	PavedRoads = CINI::CurrentTheater->GetInteger("General", "PavedRoads", -10);
	ShorePieces = CINI::CurrentTheater->GetInteger("General", "ShorePieces", -10);
	WaterBridge = CINI::CurrentTheater->GetInteger("General", "WaterBridge", -10);
	BridgeSet = CINI::CurrentTheater->GetInteger("General", "BridgeSet", -10);
	WoodBridgeSet = CINI::CurrentTheater->GetInteger("General", "WoodBridgeSet", -10);
	HeightBase = CINI::CurrentTheater->GetInteger("General", "HeightBase", -10);
	ConditionYellow = Variables::RulesMap.GetSingle("AudioVisual", "ConditionYellow", 0.5f);
	ConditionRed = Variables::RulesMap.GetSingle("AudioVisual", "ConditionRed", 0.25f);

	AutoShore_ShoreTileSet = ShorePieces;
	AutoShore_GreenTileSet = GreenTile;

	CMapDataExt::TileSet_starts.clear();
	CMapDataExt::ShoreTileSets.clear();
	CMapDataExt::SoftTileSets.clear();
	CMapDataExt::RedrawExtraTileSets.clear();
	CMapDataExt::TileSetPalettes.clear();

	if (auto theater = CINI::CurrentTheater())
	{
		int totalIndex = 0;
		FString sName = "";
		CMapDataExt::TileSet_starts.push_back(0);
		for (int index = 0; index < 10000; index++)
		{
			sName.Format("TileSet%04d", index);

			if (theater->SectionExists(sName))
			{
				totalIndex += theater->GetInteger(sName, "TilesInSet");
				CMapDataExt::TileSet_starts.push_back(totalIndex);

				auto setName = theater->GetString(sName, "SetName");
				setName.MakeLower();
				if (setName.Find("shore") != -1)
					ShoreTileSets.insert(index);

				if (theater->KeyExists(sName, "CustomPalette"))
				{
					Palette* pal = &CMapDataExt::Palette_ISO;
					FString custom = CINI::CurrentTheater->GetString(sName, "CustomPalette");
					((CLoadingExt*)CLoading::Instance())->GetFullPaletteName(custom);
					if (auto pPal = PalettesManager::LoadPalette(custom))
						pal = pPal;
					CMapDataExt::TileSetPalettes[index] = pal;
				}
				else
				{
					CMapDataExt::TileSetPalettes[index] = &CMapDataExt::Palette_ISO;
				}
			}
			else break;
		}
		FString theaterSoft = "SoftTileSets";
		if (auto pSection = CINI::FAData->GetSection(theaterSoft))
		{
			for (const auto& [key, value] : pSection->GetEntities())
			{
				int tileSet = CINI::CurrentTheater->GetInteger("General", STDHelpers::GetTrimString(key), -1);
				if (atoi(value) > 0)
				{
					SoftTileSets[tileSet] = true;
				}
				else
				{
					SoftTileSets[tileSet] = false;
				}
			}
		}
		theaterSoft += theaterSuffix;
		if (auto pSection = CINI::FAData->GetSection(theaterSoft))
		{
			for (const auto& [key, value] : pSection->GetEntities())
			{
				int tileSet = atoi(STDHelpers::GetTrimString(key));
				if (atoi(value) > 0)
				{
					SoftTileSets[tileSet] = true;
				}
				else
				{
					SoftTileSets[tileSet] = false;
				}
			}
		}
	}
	FString redrawExtra = "RedrawExtraTileSets";
	redrawExtra += theaterSuffix;
	if (auto pSection = CINI::FAData->GetSection(redrawExtra))
	{
		for (const auto& [_, value] : pSection->GetEntities())
		{
			RedrawExtraTileSets.insert(atoi(value));
		}
	}

	if (auto pSection = CINI::FAData->GetSection("AutoShoreTypes"))
	{
		auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");
		for (const auto& type : pSection->GetEntities())
		{
			auto atoms = STDHelpers::SplitString(type.second, 3);
			if (atoms[0] == thisTheater)
			{
				int shore = -1;
				int green = -1;
				if (!STDHelpers::IsNumber(atoms[2]))
				{
					shore = CINI::CurrentTheater->GetInteger("General", atoms[2], -1);
				}
				else
				{
					shore = atoi(atoms[2]);
				}
				if (!STDHelpers::IsNumber(atoms[3]))
				{
					green = CINI::CurrentTheater->GetInteger("General", atoms[3], -1);
				}
				else
				{
					green = atoi(atoms[3]);
				}
				if (shore >= 0 && CMapDataExt::IsValidTileSet(shore))
				{
					AutoShore_ShoreTileSet = shore;
					AutoShore_GreenTileSet = green;
					break;
				}
			}
		}
	}

	CViewObjectsExt::ConnectedTile_Initialize();

	CMapDataExt::Tile_to_lat.clear();

	std::vector<std::vector<FString>> lats = {
	{"SandTile", "ClearToSandLat"},
	{"GreenTile", "ClearToGreenLat"},
	{"RoughTile", "ClearToRoughLat"},
	{"PaveTile", "ClearToPaveLat"},
	{"BlueMoldTile", "ClearToBlueMoldLat"},
	{"CrystalTile", "ClearToCrystalLat"},
	{"SwampTile", "WaterToSwampLat"}
	};

	if (auto pLAT = CINI::FAData().GetSection("LATGroups"))
	{
		for (auto& pair : pLAT->GetEntities())
		{
			auto atoms = FString::SplitString(pair.second);
			if (atoms.size() >= 3)
			{
				std::vector<FString> group;
				if (CINI::CurrentDocument().GetString("Map", "Theater") != atoms[0])
					continue;
				group.push_back(atoms[1]);
				group.push_back(atoms[2]);
				if (atoms.size() >= 4)
					group.push_back(atoms[3]);
				lats.push_back(group);
			}
		}
	}

	for (int latidx = 0; latidx < lats.size(); ++latidx)
	{
		int iSmoothSet = CINI::FAData->GetInteger("LATSettings", lats[latidx][0], -1);
		int iLatSet = CINI::FAData->GetInteger("LATSettings", lats[latidx][1], -1);
		iSmoothSet = CINI::CurrentTheater->GetInteger("General", lats[latidx][0], iSmoothSet);
		iLatSet = CINI::CurrentTheater->GetInteger("General", lats[latidx][1], iLatSet);
		auto& lat = CMapDataExt::Tile_to_lat.emplace_back();
		lat.push_back(iSmoothSet);
		lat.push_back(iLatSet);

		if (lats[latidx].size() == 3)
		{
			auto noLatTiles = CINI::FAData->GetString("LATSettings", lats[latidx][2]);
			if (noLatTiles != "")
			{
				for (auto& noLatTile : STDHelpers::SplitString(noLatTiles))
				{
					int noLatTileIdx = atoi(noLatTile);
					if (noLatTileIdx < CMapDataExt::TileSet_starts.size() - 1)
						for (int slIdx = CMapDataExt::TileSet_starts[noLatTileIdx]; slIdx < CMapDataExt::TileSet_starts[noLatTileIdx + 1]; slIdx++)
							lat.push_back(slIdx);
				}
			}
		}
	}

	ppmfc::CString pInfoSection = TheaterInfo::GetInfoSection();
	// Forward compatibility
	if (CINI::FAData->SectionExists(pInfoSection + "2"))
		pInfoSection += "2";
	TheaterInfo::CurrentBigWaters.clear();
	TheaterInfo::CurrentSmallWaters.clear();
	if (CINI::FAData->KeyExists(pInfoSection, "BigWaterIndices"))
	{
		auto buffer = CINI::FAData->GetString(pInfoSection, "BigWaterIndices");
		auto buffers = STDHelpers::SplitString(buffer);
		for (auto& str : buffers)
		{
			TheaterInfo::CurrentBigWaters.insert(atoi(str));
		}
	}
	if (CINI::FAData->KeyExists(pInfoSection, "SmallWaterIndices"))
	{
		auto buffer = CINI::FAData->GetString(pInfoSection, "SmallWaterIndices");
		auto buffers = STDHelpers::SplitString(buffer);
		for (auto& str : buffers)
		{
			TheaterInfo::CurrentSmallWaters.insert(atoi(str));
		}
	}
	if (TheaterInfo::CurrentBigWaters.empty())
	{
		int waterSet = CINI::CurrentTheater->GetInteger("General", "WaterSet", 21);
		for (int i = 0; i < 6; ++i)
		{
			TheaterInfo::CurrentBigWaters.insert(CMapDataExt::TileSet_starts[waterSet] + i);
		}
	}
	if (TheaterInfo::CurrentSmallWaters.empty())
	{
		int waterSet = CINI::CurrentTheater->GetInteger("General", "WaterSet", 21);
		for (int i = 8; i < 13; ++i)
		{
			TheaterInfo::CurrentSmallWaters.insert(CMapDataExt::TileSet_starts[waterSet] + i);
		}
	}

	CMapDataExt::GetExtension()->InitOreValue();

	CTerrainGenerator::RangeFirstCell.X = -1;
	CTerrainGenerator::RangeFirstCell.Y = -1;
	CTerrainGenerator::RangeSecondCell.X = -1;
	CTerrainGenerator::RangeSecondCell.Y = -1;
	CTerrainGenerator::UseMultiSelection = false;

	if (updateMinimap)
	{
		// just update coords with overlays to show correct color
		for (int i = 0; i < CMapData::Instance->MapWidthPlusHeight; i++) {
			for (int j = 0; j < CMapData::Instance->MapWidthPlusHeight; j++) {
				CMapDataExt::CellDataExts[i + j * CMapData::Instance->MapWidthPlusHeight].NewOverlay = CMapDataExt::NewOverlay[j + i * 512];
				if (CMapDataExt::GetExtension()->GetOverlayAt(CMapData::Instance->GetCoordIndex(i, j)) != 0xFFFF) {
					CMapData::Instance->UpdateMapPreviewAt(i, j);
				}
			}
		}
	}
	CIsoViewExt::IsPressingTube = false;
	CIsoViewExt::TubeNodes.clear();

	if (reloadImages)
	{
		if (TagSort::Instance.IsVisible())
		{
			TagSort::Instance.LoadAllTriggers();
		}
		if (TeamSort::Instance.IsVisible())
		{
			TeamSort::Instance.LoadAllTriggers();
		}
		if (WaypointSort::Instance.IsVisible())
		{
			WaypointSort::Instance.LoadAllTriggers();
		}
		if (TaskforceSort::Instance.IsVisible())
		{
			TaskforceSort::Instance.LoadAllTriggers();
		}
		if (ScriptSort::Instance.IsVisible())
		{
			ScriptSort::Instance.LoadAllTriggers();
		}
		if (WaypointSort::Instance.IsVisible())
		{
			WaypointSort::Instance.LoadAllTriggers();
		}

		BuildingDataExts.clear();

		BuildingDataExt tempBuildingData;
		tempBuildingData.Width = 1;
		tempBuildingData.Height = 1;
		tempBuildingData.BottomCoords = { {0,0} };
		CMapDataExt::BuildingDataExts[-1] = tempBuildingData;
		const auto Types = Variables::RulesMap.GetSection("BuildingTypes");
		for (auto& Type : Types)
			CMapDataExt::GetExtension()->ProcessBuildingType(Type.second);

		TileAnimations.clear();
		for (auto& [index, setName] : CMapDataExt::TileSetOriginSetNames[CLoadingExt::GetITheaterIndex()])
		{
			if (CINI::CurrentTheater->SectionExists(setName) && index + 1 < TileSet_starts.size())
			{
				for (int i = TileSet_starts[index]; i < TileSet_starts[index + 1]; ++i)
				{
					int relativeIndex = i - TileSet_starts[index] + 1;
					auto& anim = TileAnimations[i];
					anim.TileIndex = i;
					FString Anim;
					FString XOffset;
					FString YOffset;
					FString AttachesTo;
					FString ZAdjust;
					Anim.Format("Tile%02dAnim", relativeIndex);
					XOffset.Format("Tile%02dXOffset", relativeIndex);
					YOffset.Format("Tile%02dYOffset", relativeIndex);
					AttachesTo.Format("Tile%02dAttachesTo", relativeIndex);
					ZAdjust.Format("Tile%02dZAdjust", relativeIndex);
					anim.AnimName = CINI::CurrentTheater->GetString(setName, Anim);
					anim.XOffset = CINI::CurrentTheater->GetInteger(setName, XOffset);
					anim.YOffset = CINI::CurrentTheater->GetInteger(setName, YOffset);
					anim.AttachedSubTile = CINI::CurrentTheater->GetInteger(setName, AttachesTo);
					anim.ZAdjust = CINI::CurrentTheater->GetInteger(setName, ZAdjust);
					FString imageName;
					imageName.Format("TileAnim%s\233%d%d", anim.AnimName, index, CLoadingExt::GetITheaterIndex());
					FString sectionName;
					sectionName.Format("TileSet%04d", index);
					auto customPal = CINI::CurrentTheater->GetString(sectionName, "CustomPalette", "iso");
					if (customPal == "iso")
						CLoadingExt::LoadShp(imageName, anim.AnimName + CLoading::Instance->GetFileExtension(), &Palette_ISO_NoTint, 0);
					else
						CLoadingExt::LoadShp(imageName, anim.AnimName + CLoading::Instance->GetFileExtension(), customPal, 0);
					anim.ImageName = imageName;
				}
			}
		}
		const char* InsigniaVeteran = "FA2spInsigniaVeteran";
		const char* InsigniaElite = "FA2spInsigniaElite";
		const char* DefaultInsigniaFile = "pips.shp";
		const char* PaletteName = "palette.pal";
		CLoadingExt::LoadShp(InsigniaVeteran, "pips.shp", PaletteName, 14);
		CLoadingExt::LoadShp(InsigniaElite, "pips.shp", PaletteName, 15);
		CLoadingExt::DamageFires.clear();
		std::random_device rd;
		CLoadingExt::RandomFireSeed = rd();
		auto fires = STDHelpers::SplitString(Variables::RulesMap.GetString("General", "DamageFireTypes"));
		for (const auto& fire : fires)
		{
			CLoadingExt::LoadFires(fire + ".shp");
		}
		TechnoAttachments.clear();
		int loopCount = Variables::RulesMap.ParseIndicies("AttachmentTypes").size();
		std::vector<FString> Type(loopCount);
		std::vector<FString> TechnoType(loopCount);
		std::vector<FString> FLH(loopCount);
		std::vector<FString> IsOnTurret(loopCount);
		std::vector<FString> RotationAdjust(loopCount);
		for (int i = 0; i < loopCount; ++i)
		{
			Type[i].Format("Attachment%d.Type", i);
			TechnoType[i].Format("Attachment%d.TechnoType", i);
			FLH[i].Format("Attachment%d.FLH", i);
			IsOnTurret[i].Format("Attachment%d.IsOnTurret", i);
			RotationAdjust[i].Format("Attachment%d.RotationAdjust", i);
		}
		auto GetFLH = [](int& F, int& L, int& H, const ppmfc::CString value)
		{
			int s_count = sscanf_s(value, "%d,%d,%d", &F, &L, &H);
			if (s_count == 0) F = L = H = 0;
			else if (s_count == 1) L = H = 0;
			else if (s_count == 2) H = 0;
		};
		auto GetTechnoAttachments = [&](const ppmfc::CString& ID)
		{
			for (int i = 0; i < loopCount; ++i)
			{
				if (Variables::RulesMap.KeyExists(ID, TechnoType[i]))
				{
					auto taType = Variables::RulesMap.GetString(ID, Type[i]);
					if (!Variables::RulesMap.GetBool(taType, "RespawnAtCreation", true))
						continue;

					auto& ret = TechnoAttachments[ID];
					auto& ta = ret.emplace_back();
					ta.ID = Variables::RulesMap.GetString(ID, TechnoType[i]);
					auto ySort = Variables::RulesMap.GetString(taType, "YSortPosition");
					if (ySort == "underparent")
						ta.YSortPosition = TechnoAttachment::YSortPosition::Bottom;
					else if (ySort == "overparent")
						ta.YSortPosition = TechnoAttachment::YSortPosition::Top;
					else
						ta.YSortPosition = TechnoAttachment::YSortPosition::Default;

					GetFLH(ta.F, ta.L, ta.H, Variables::RulesMap.GetString(ID, FLH[i], "0,0,0"));

					if (Variables::RulesMap.GetBool(ID, IsOnTurret[i]))
					{
						ta.IsOnTurret = true;
						FString ArtID = CLoadingExt::GetArtID(ID);
						int F, L, H;
						GetFLH(F, L, H, CINI::Art->GetString(ArtID, "TurretOffset", "0,0,0"));
						ta.F += F;
						ta.L += L;
						ta.H += H;
						ta.DeltaX = Variables::RulesMap.GetInteger(ID, "TurretAnimX");
						ta.DeltaY = Variables::RulesMap.GetInteger(ID, "TurretAnimY");
					}

					ta.RotationAdjust = Variables::RulesMap.GetInteger(ID, RotationAdjust[i]);
				}
			}

			auto aeTypes = STDHelpers::SplitString(Variables::RulesMap.GetString(ID, "AttachEffectTypes"));
			for (auto& ae : aeTypes)
			{
				if (!Variables::RulesMap.TryGetString(ae, "Stand.Type"))
					continue;

				auto& ret = TechnoAttachments[ID];
				auto& ta = ret.emplace_back();
				ta.ID = Variables::RulesMap.GetString(ae, "Stand.Type");
				ta.YSortPosition = Variables::RulesMap.GetInteger(ae, "Stand.ZOffset") < 0 ? 
					TechnoAttachment::YSortPosition::Bottom: TechnoAttachment::YSortPosition::Top;
				GetFLH(ta.F, ta.L, ta.H, Variables::RulesMap.GetString(ae, "Stand.Offset", "0,0,0"));

				if (Variables::RulesMap.GetBool(ae, "Stand.IsOnTurret"))
				{
					ta.IsOnTurret = true;
					FString ArtID = CLoadingExt::GetArtID(ID);
					int F, L, H;
					GetFLH(F, L, H, CINI::Art->GetString(ArtID, "TurretOffset", "0,0,0"));
					ta.F += F;
					ta.L += L;
					ta.H += H;
					ta.DeltaX = Variables::RulesMap.GetInteger(ID, "TurretAnimX");
					ta.DeltaY = Variables::RulesMap.GetInteger(ID, "TurretAnimY");
				}
				ta.RotationAdjust = Variables::RulesMap.GetInteger(ae, "Stand.Direction") * 16;
			}

			auto extraUnits = STDHelpers::SplitString(Variables::RulesMap.GetString(ID, "ExtraUnit.Definations"));
			for (auto& ex : extraUnits)
			{
				if (!Variables::RulesMap.TryGetString(ex, "ExtraUnit.Type"))
					continue;

				auto& ret = TechnoAttachments[ID];
				auto& ta = ret.emplace_back();
				ta.ID = Variables::RulesMap.GetString(ex, "ExtraUnit.Type");

				auto getLayer = [](const ppmfc::CString& value)
				{
					if (value == "air")
						return 1;
					if (value == "top")
						return 2;
					return 0;
				};
				int mainLayer = getLayer(Variables::RulesMap.GetString(ID, "Render.ForceLayer"));
				int exLayer = getLayer(Variables::RulesMap.GetString(ex, "Render.ForceLayer"));

				if (exLayer >= mainLayer)
					ta.YSortPosition = TechnoAttachment::YSortPosition::Top;
				else
					ta.YSortPosition = TechnoAttachment::YSortPosition::Bottom;
				GetFLH(ta.F, ta.L, ta.H, Variables::RulesMap.GetString(ex, "ExtraUnit.Position", "0,0,0"));
				if (Variables::RulesMap.GetBool(ex, "ExtraUnit.BindTurret"))
				{
					ta.IsOnTurret = true;
					FString ArtID = CLoadingExt::GetArtID(ID);
					int F, L, H;
					GetFLH(F, L, H, CINI::Art->GetString(ArtID, "TurretOffset", "0,0,0"));
					ta.F += F;
					ta.L += L;
					ta.H += H;
					ta.DeltaX = Variables::RulesMap.GetInteger(ID, "TurretAnimX");
					ta.DeltaY = Variables::RulesMap.GetInteger(ID, "TurretAnimY");
				}
				ta.RotationAdjust = Variables::RulesMap.GetInteger(ex, "ExtraUnit.FacingAngleAdjust") * 256 / 360;
			}

			auto virtualUnits = STDHelpers::SplitString(Variables::RulesMap.GetString(ID, "VirtualUnits"));
			for (auto& vr : virtualUnits)
			{
				if (!Variables::RulesMap.TryGetString(vr, "Techno"))
					continue;

				auto& ret = TechnoAttachments[ID];
				auto& ta = ret.emplace_back();
				ta.ID = Variables::RulesMap.GetString(vr, "Techno");
				if (Variables::RulesMap.GetBool(vr, "DisplayUpon", true))
					ta.YSortPosition = TechnoAttachment::YSortPosition::Top;
				else
					ta.YSortPosition = TechnoAttachment::YSortPosition::Bottom;
				GetFLH(ta.F, ta.L, ta.H, Variables::RulesMap.GetString(vr, "FLH", "0,0,0"));
				if (Variables::RulesMap.GetBool(vr, "OnTurret"))
				{
					ta.IsOnTurret = true;
					FString ArtID = CLoadingExt::GetArtID(ID);
					int F, L, H;
					GetFLH(F, L, H, CINI::Art->GetString(ArtID, "TurretOffset", "0,0,0"));
					ta.F += F;
					ta.L += L;
					ta.H += H;
					ta.DeltaX = Variables::RulesMap.GetInteger(ID, "TurretAnimX");
					ta.DeltaY = Variables::RulesMap.GetInteger(ID, "TurretAnimY");
				}
				ta.RotationAdjust = Variables::RulesMap.GetInteger(vr, "Rotation");
			}
		};

		for (auto& [_, ID] : Variables::RulesMap.GetSection("InfantryTypes"))
			GetTechnoAttachments(ID);
		for (auto& [_, ID] : Variables::RulesMap.GetSection("VehicleTypes"))
			GetTechnoAttachments(ID);
		for (auto& [_, ID] : Variables::RulesMap.GetSection("BuildingTypes"))
			GetTechnoAttachments(ID);
		for (auto& [_, ID] : Variables::RulesMap.GetSection("AircraftTypes"))
			GetTechnoAttachments(ID);
	}

	for (auto& [_, ID] : Variables::RulesMap.GetSection("InfantryTypes"))
	{
		auto ArtID = CLoadingExt::GetArtID(ID);
		auto ImageID = CLoadingExt::GetExtension()->GetInfantryFileID(ID);

		auto sequenceName = CINI::Art->GetString(ImageID, "Sequence");
		bool deployable = Variables::RulesMap.GetBool(ID, "Deployer") && CINI::Art->KeyExists(sequenceName, "Deployed");
		bool waterable = Variables::RulesMap.GetString(ID, "MovementZone") == "AmphibiousDestroyer"
			&& CINI::Art->KeyExists(sequenceName, "Swim");
		if (ExtConfigs::InGameDisplay_Water && waterable)
		{
			CLoadingExt::SwimableInfantries.insert(ID);
		}
	}
	CNewTrigger::ActionParamAffectedParams.clear();
	CNewTrigger::EventParamAffectedParams.clear();
	if (auto pSection = CINI::FAData->GetSection("ParamAffectedParams"))
	{
		for (const auto& [_, value] : pSection->GetEntities())
		{
			auto atoms = FString::SplitString(value, 5);
			auto& list = atoms[0] == "Event" ? CNewTrigger::EventParamAffectedParams : CNewTrigger::ActionParamAffectedParams;
			auto& target = list.emplace_back();
			target.Index = atoi(atoms[1]);
			target.SourceParam = atoi(atoms[2]);
			target.AffectedParam = atoi(atoms[3]);
			for (int i = 4; i < atoms.size() - 1; i = i + 2)
			{
				target.ParamMap[atoms[i]] = atoms[i + 1];
			}
		}
	}
	UpdateAnnotation();
	CIsoViewExt::DistanceRuler.clear();
	CMapDataExt::PowersUpBuildings.clear();
	CMapDataExt::PowersUpBuildingSet.clear();
	auto buildings = Variables::RulesMap.ParseIndicies("BuildingTypes", true);
	for (const auto& building : buildings)
	{
		auto parent = Variables::RulesMap.GetString(building, "PowersUpBuilding");
		if (!parent.IsEmpty())
		{
			CMapDataExt::PowersUpBuildings[parent].insert(building);
			CMapDataExt::PowersUpBuildingSet.insert(building);
		}
		auto parents = Variables::RulesMap.GetString(building, "PowersUp.Buildings");
		if (!parents.IsEmpty())
		{
			auto atoms = STDHelpers::SplitString(parents);
			for (auto& p : atoms)
			{
				CMapDataExt::PowersUpBuildings[p].insert(building);
				CMapDataExt::PowersUpBuildingSet.insert(building);
			}
		}
	}

	CustomWaypointColors.clear();
	CustomCelltagColors.clear();
	if (auto pColors = CINI::CurrentDocument->GetSection("FA2spColors"))
	{
		if (auto pSection = CINI::CurrentDocument->GetSection("Waypoints"))
		{
			for (const auto& [key, value] : pSection->GetEntities())
			{
				ppmfc::CString colorkey = "Wp";
				colorkey += key;
				auto color = CINI::CurrentDocument->GetColor(pColors, colorkey, ExtConfigs::DisplayColor_Waypoint);
				CMapDataExt::CustomWaypointColors[key] = color;
			}
		}
		if (auto pSection = CINI::CurrentDocument->GetSection("CellTags"))
		{
			for (const auto& [key, value] : pSection->GetEntities())
			{
				ppmfc::CString colorkey = "Tag";
				colorkey += value;
				auto color = CINI::CurrentDocument->GetColor(pColors, colorkey, ExtConfigs::DisplayColor_Celltag);
				CMapDataExt::CustomCelltagColors[value] = color;
			}
		}
	}
	CLoadingExt::DrawTurretShadow = Variables::RulesMap.GetBool("AudioVisual", "DrawTurretShadow");
}