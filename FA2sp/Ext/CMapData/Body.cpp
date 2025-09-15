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
int CMapDataExt::UndoRedoDataIndex;
bool CMapDataExt::IsLoadingMapFile = false;
std::vector<FString> CMapDataExt::MapIniSectionSorting;
ObjectRecord* ObjectRecord::ObjectRecord_HoldingPtr = nullptr;

int CMapDataExt::GetOreValue(unsigned short nOverlay, unsigned char nOverlayData)
{
    if (nOverlay >= 0x66 && nOverlay <= 0x79)
        return nOverlayData * OreValue[OreType::Riparius];
    else if (nOverlay >= 0x1B && nOverlay <= 0x26)
        return nOverlayData * OreValue[OreType::Cruentus];
    else if (nOverlay >= 0x7F && nOverlay <= 0x92)
        return nOverlayData * OreValue[OreType::Vinifera];
    else if (nOverlay >= 0x93 && nOverlay <= 0xA6)
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

void CMapDataExt::UpdateTriggers()
{
	CMapDataExt::Triggers.clear();
	if (auto pSection = CINI::CurrentDocument->GetSection("Triggers")) {
		for (const auto& pair : pSection->GetEntities()) {
			std::shared_ptr<Trigger> trigger(Trigger::create(pair.first));
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

	if (ExtConfigs::PlaceTileSkipHide)
	{
		const auto cell = this->TryGetCellAt(X, Y);
		if (cell->IsHidden())
			return;
	}

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
		MultimapHelper mmh;
		mmh.AddINI(&CINI::Rules());
		auto&& overlays = mmh.ParseIndicies("OverlayTypes", true);
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
	std::vector<int> BigWaterTiles;
	std::vector<int> SmallWaterTiles;

	int waterSet = CINI::CurrentTheater->GetInteger("General", "WaterSet", 21);
	if (waterSet < 0 || waterSet > CMapDataExt::TileSet_starts.size())
		return;

	for (int i = 0; i < 6; i++)
		BigWaterTiles.push_back(i + CMapDataExt::TileSet_starts[waterSet]);

	for (int i = 8; i < 13; i++)
		SmallWaterTiles.push_back(i + CMapDataExt::TileSet_starts[waterSet]);

	
	int side = CMapData::Instance->MapWidthPlusHeight;
	auto CellDatas = CMapData::Instance->CellDatas;


	// first check all the water tiles
	for (int i = 0; i < CMapData::Instance->CellDataCount; i++)
	{
		auto& cellExt = CMapDataExt::CellDataExts[i];
		auto& cell = CMapData::Instance->CellDatas[i];
		cellExt.Processed = false;
		cellExt.IsWater = false;
		for (auto idx : BigWaterTiles)
			if (cell.TileIndex == idx)
				cellExt.IsWater = true;
		for (auto idx : SmallWaterTiles)
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
				int random = STDHelpers::RandomSelectInt(BigWaterTiles);
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
			int random = STDHelpers::RandomSelectInt(SmallWaterTiles);
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

void CMapDataExt::UpdateFieldStructureData_Index(int iniIndex, ppmfc::CString value)
{
	if (value == "")
		value = CINI::CurrentDocument->GetValueAt("Structures", iniIndex);

	int cellIndex = StructureIndexMap.size();
	if (cellIndex > 65500 && !CIsoView::IsMouseMoving)
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
		const int BuildingIndex = CMapData::Instance->GetBuildingTypeID(splits[1]);
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
	}
}

void CMapDataExt::UpdateFieldStructureData_Optimized()
{
	auto Map = &CMapData::Instance();
	auto& fielddata_size = Map->CellDataCount;
	auto& fielddata = Map->CellDatas;

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
			UpdateFieldStructureData_Index(i, data.second);
			i++;
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

bool CMapDataExt::IsValidTileSet(int tileset) 
{
	FString buffer;
	buffer.Format("TileSet%04d", tileset);

	auto exist = CINI::CurrentTheater->GetBool(buffer, "AllowToPlace", true);
	auto exist2 = CINI::CurrentTheater->GetString(buffer, "FileName", "");
	auto exist3 = CINI::CurrentTheater->GetInteger(buffer, "TilesInSet");
	if (!exist || strcmp(exist2, "") == 0 || exist3 < 1)
		return false;
	return true;
}

ppmfc::CString CMapDataExt::GetAvailableIndex()
{
	auto& ini = CINI::CurrentDocument;
	int n = 1000000;

	std::unordered_set<std::string> usedIDs;

	for (const auto& sec : { "ScriptTypes", "TaskForces", "TeamTypes" }) {
		if (auto pSection = ini->GetSection(sec)) {
			for (const auto& [k, v] : pSection->GetEntities()) {
				usedIDs.insert(v.m_pchData);
			}
		}
	}
	for (const auto& sec : { "Triggers", "Events", "Tags", "Actions", "AITriggerTypes" }) {
		if (auto pSection = ini->GetSection(sec)) {
			for (const auto& [k, v] : pSection->GetEntities()) {
				usedIDs.insert(k.m_pchData);
			}
		}
	}

	char idBuffer[9];
	while (true)
	{
		std::sprintf(idBuffer, "%08d", n);
		std::string id(idBuffer);

		if (usedIDs.find(id) == usedIDs.end() && !ini->SectionExists(id.c_str())) {
			return id.c_str();
		}

		n++;
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
	if (pos >= CellDataCount)
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
	ret.Wall = false;
	ret.CustomPaletteName = "";
	ret.TerrainRock = false;
	ret.RadarColor.R = 0;
	ret.RadarColor.G = 0;
	ret.RadarColor.B = 0;

	return ret;
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
		recoverIniMap("Waypoints", WaypointList);
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

void CMapDataExt::InitializeAllHdmEdition(bool updateMinimap, bool reloadCellDataExt)
{
	Logger::Debug("CMapDataExt::InitializeAllHdmEdition() Called!\n");
	CIsoView::CurrentCommand->Type = 0;
	CIsoView::CurrentCommand->Command = 0;
	FA2sp::g_VEH_Enabled = true;

	CIsoView::GetInstance()->CurrentCellObjectIndex = -1;
	CIsoView::GetInstance()->CurrentCellObjectType = -1;
	CIsoView::GetInstance()->Drag = FALSE;

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

	auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");
	if (thisTheater == "TEMPERATE")
	{
		CurrentTheaterIndex = 0;
		CMapDataExt::TileData = CTileTypeInfo::Temperate().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::Temperate().Count;
	}
	if (thisTheater == "SNOW")
	{
		CurrentTheaterIndex = 1;
		CMapDataExt::TileData = CTileTypeInfo::Snow().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::Snow().Count;
	}
	if (thisTheater == "URBAN")
	{
		CurrentTheaterIndex = 2;
		CMapDataExt::TileData = CTileTypeInfo::Urban().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::Urban().Count;
	}
	if (thisTheater == "NEWURBAN")
	{
		CurrentTheaterIndex = 3;
		CMapDataExt::TileData = CTileTypeInfo::NewUrban().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::NewUrban().Count;
	}
	if (thisTheater == "LUNAR")
	{
		CurrentTheaterIndex = 4;
		CMapDataExt::TileData = CTileTypeInfo::Lunar().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::Lunar().Count;
	}
	if (thisTheater == "DESERT")
	{
		CurrentTheaterIndex = 5;
		CMapDataExt::TileData = CTileTypeInfo::Desert().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::Desert().Count;
	}

	CFinalSunDlgExt::CurrentLighting = 31000;
	CheckMenuRadioItem(*CFinalSunDlg::Instance->GetMenu(), 31000, 31003, 31000, MF_UNCHECKED);
	PalettesManager::Release();
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
					auto custom = CINI::CurrentTheater->GetString(sName, "CustomPalette");
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
				if (CMapDataExt::GetExtension()->GetOverlayAt(CMapData::Instance->GetCoordIndex(i, j)) != 0xFFFF) {
					CMapData::Instance->UpdateMapPreviewAt(i, j);
				}
			}
		}
	}
	CIsoViewExt::IsPressingTube = false;
	CIsoViewExt::TubeNodes.clear();

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
	CLoadingExt::LoadShp(InsigniaVeteran, "pips.shp", PaletteName, 14, false);
	CLoadingExt::LoadShp(InsigniaElite, "pips.shp", PaletteName, 15, false);

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
}