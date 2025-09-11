#include <Helpers/Macro.h>
#include <Drawing.h>
#include <CPalette.h>
#include <CLoading.h>
#include "../../Miscs/Palettes.h"
#include "../CFinalSunDlg/Body.h"
#include "../CIsoView/Body.h"
#include "../CLoading/Body.h"
#include "../CMapData/Body.h"
#include <Miscs/Miscs.h>
#include "../../Helpers/Translations.h"
#include "../../Miscs/Hooks.INI.h"
#include "../../Miscs/MultiSelection.h"
#include <codecvt>
#include "../../ExtraWindow/CTerrainGenerator/CTerrainGenerator.h"

static int Left, Right, Top, Bottom;
static CRect window;
static MapCoord VisibleCoordTL;
static MapCoord VisibleCoordBR;
static int HorizontalLoopIndex;
static ppmfc::CPoint ViewPosition;

std::unordered_set<short> CIsoViewExt::VisibleStructures;
std::unordered_set<short> CIsoViewExt::VisibleInfantries;
std::unordered_set<short> CIsoViewExt::VisibleUnits;
std::unordered_set<short> CIsoViewExt::VisibleAircrafts;

struct CellInfo {
	int X, Y;
	int screenX, screenY;
	int pos;
	bool isInMap;
	CellData* cell;
	CellDataExt* cellExt;
};

static std::vector<std::pair<MapCoord, FString>> WaypointsToDraw;
static std::vector<std::pair<MapCoord, FString>> OverlayTextsToDraw;
static std::vector<std::pair<MapCoord, DrawBuildings>> BuildingsToDraw;
static std::vector<std::pair<MapCoord, ImageDataClassSafe*>> AlphaImagesToDraw;
static std::vector<DrawVeterancy> DrawVeterancies;
static std::vector<CellInfo> visibleCells;
static std::unordered_set<short> DrawnBuildings;
static std::vector<BaseNodeDataExt> DrawnBaseNodes;

#define EXTRA_BORDER 15

inline static bool IsCoordInWindow(int X, int Y)
{
	return
		X + Y > VisibleCoordTL.X + VisibleCoordTL.Y - EXTRA_BORDER &&
		X + Y < VisibleCoordBR.X + VisibleCoordBR.Y + CIsoViewExt::EXTRA_BORDER_BOTTOM &&
		X > Y + VisibleCoordBR.X - VisibleCoordBR.Y - EXTRA_BORDER &&
		X < Y + VisibleCoordTL.X - VisibleCoordTL.Y + EXTRA_BORDER;
}

inline static void GetUnitImageID(FString& ImageID, const CUnitData& obj, const LandType& landType)
{
	if (ExtConfigs::InGameDisplay_Water)
	{
		if (landType == LandType::Water || landType == LandType::Beach)
		{
			ImageID = Variables::RulesMap.GetString(obj.TypeID, "WaterImage", obj.TypeID);
		}
	}
	if (ExtConfigs::InGameDisplay_Damage)
	{
		int HP = atoi(obj.Health);
		if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP)
		{
			ImageID = Variables::RulesMap.GetString(obj.TypeID, "Image.ConditionYellow", ImageID);
		}
		if (static_cast<int>((CMapDataExt::ConditionRed + 0.001f) * 256) > HP)
		{
			ImageID = Variables::RulesMap.GetString(obj.TypeID, "Image.ConditionRed", ImageID);
		}
		if (ExtConfigs::InGameDisplay_Water)
		{
			if (landType == LandType::Water || landType == LandType::Beach)
			{
				if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP)
				{
					ImageID = Variables::RulesMap.GetString(obj.TypeID, "WaterImage.ConditionYellow", ImageID);
				}
				if (static_cast<int>((CMapDataExt::ConditionRed + 0.001f) * 256) > HP)
				{
					ImageID = Variables::RulesMap.GetString(obj.TypeID, "WaterImage.ConditionRed", ImageID);
				}
			}
		}
	}
	// UnloadingClass is prior
	if (ExtConfigs::InGameDisplay_Deploy && obj.Status == "Unload")
	{
		ImageID = Variables::RulesMap.GetString(obj.TypeID, "UnloadingClass", ImageID);
	}
}

DEFINE_HOOK(46DE00, CIsoView_Draw_Begin, 7)
{
	auto pThis = CIsoView::GetInstance();
	PalettesManager::CalculatedObjectPaletteFiles.clear();
	CIsoViewExt::VisibleStructures.clear();
	CIsoViewExt::VisibleInfantries.clear();
	CIsoViewExt::VisibleUnits.clear();
	CIsoViewExt::VisibleAircrafts.clear();
	CLoadingExt::CurrentFrameImageDataMap.clear();
	CIsoViewExt::InitAlphaTable();

	WaypointsToDraw.clear();
	OverlayTextsToDraw.clear();
	BuildingsToDraw.clear();
	AlphaImagesToDraw.clear();
	DrawVeterancies.clear();
	visibleCells.clear();
	DrawnBuildings.clear();
	DrawnBaseNodes.clear();

	RECT rect;
	::GetClientRect(pThis->GetSafeHwnd(), &rect);
	POINT topLeft = { rect.left, rect.top };
	::ClientToScreen(pThis->GetSafeHwnd(), &topLeft);
	double offsetX = 0.016795436849483363 * topLeft.x - 4.664099013466316;
	double offsetY = 0.03362306232114938 * topLeft.y - 2.4360168849787662;
	ViewPosition = pThis->ViewPosition;
	pThis->ViewPosition.x += offsetX;
	pThis->ViewPosition.y += offsetY;

	if (PalettesManager::NeedReloadLighting)
	{
		LightingStruct::GetCurrentLighting();
		PalettesManager::NeedReloadLighting = false;
	}

	if (INIIncludes::MapINIWarn)
	{
		if (!CINI::CurrentDocument->GetBool("FA2spVersionControl", "MapIncludeWarned"))
		{
			int result = MessageBox(CIsoView::GetInstance()->GetSafeHwnd(),
				Translations::TranslateOrDefault("MapIncludeWarningMessage",
					"This map contains include INIs. All key value pairs in the INIs will not be saved to the map, nor will be saved to the INIs.\n"
					"If you click 'OK', this warning will no longer pop up in this map."),
				Translations::TranslateOrDefault("Warning", "Warning"),
				MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION);

			if (result == IDOK)
				CINI::CurrentDocument->WriteBool("FA2spVersionControl", "MapIncludeWarned", true);
		}
		INIIncludes::MapINIWarn = false;
	}

	return 0;
}

DEFINE_HOOK(46DF20, CIsoView_Draw_BackgroundColor, 6)
{
	R->Stack(STACK_OFFS(0xD20, 0x020), ExtConfigs::EnableDarkMode ? RGB(32, 32, 32) : RGB(255, 255, 255));
	return 0;
}

DEFINE_HOOK(46DF49, CIsoView_Draw_ScaledBorder, 6)
{
	GET_STACK(CRect, viewRect, STACK_OFFS(0xD18, 0xCCC));
	viewRect.right += viewRect.Width() * (CIsoViewExt::ScaledFactor - 1.0);
	viewRect.bottom += viewRect.Height() * (CIsoViewExt::ScaledFactor - 1.0);
	R->Stack(STACK_OFFS(0xD18, 0xCCC), viewRect);
	return 0;
}

DEFINE_HOOK(46E815, CIsoView_Draw_Optimize_GetBorder, 5)
{
	Left = R->Stack<int>(STACK_OFFS(0xD18, 0xC10)) - EXTRA_BORDER;
	Right = R->Stack<int>(STACK_OFFS(0xD18, 0xC64)) + EXTRA_BORDER;
	Top = R->Stack<int>(STACK_OFFS(0xD18, 0xCBC)) - EXTRA_BORDER;
	Bottom = R->Stack<int>(STACK_OFFS(0xD18, 0xC18)) + CIsoViewExt::EXTRA_BORDER_BOTTOM;
	auto pThis = CIsoView::GetInstance();
	pThis->GetWindowRect(&window);

	double scale = CIsoViewExt::ScaledFactor;
	if (scale < 0.9)
		scale += 0.1;
	if (scale < 0.7)
		scale += 0.1;
	if (scale < 0.5)
		scale += 0.1;
	window.right += window.Width() * (scale - 1.0);
	if (scale < 1.0)
		scale = 1.0;
	window.bottom += window.Height() * (scale - 1.0);

	VisibleCoordTL.X = window.left + pThis->ViewPosition.x;
	VisibleCoordTL.Y = window.top + pThis->ViewPosition.y;
	VisibleCoordBR.X = window.right + pThis->ViewPosition.x;
	VisibleCoordBR.Y = window.bottom + pThis->ViewPosition.y;
	pThis->ScreenCoord2MapCoord_Flat(VisibleCoordTL.X, VisibleCoordTL.Y);
	pThis->ScreenCoord2MapCoord_Flat(VisibleCoordBR.X, VisibleCoordBR.Y);
	if (VisibleCoordBR.X < 0 || VisibleCoordBR.Y < 0)
	{
		VisibleCoordBR.X = CMapData::Instance->Size.Width;
		VisibleCoordBR.Y = CMapData::Instance->MapWidthPlusHeight + 1;
	}
	if (VisibleCoordTL.X < 0 || VisibleCoordTL.Y < 0)
	{
		VisibleCoordTL.X = CMapData::Instance->Size.Width;
		VisibleCoordTL.Y = 0;
	}

	HorizontalLoopIndex = VisibleCoordTL.X + VisibleCoordTL.Y - EXTRA_BORDER;
	return 0;
}

DEFINE_HOOK(46EA64, CIsoView_Draw_MainLoop, 6)
{
	GET_STACK(float, DrawOffsetX, STACK_OFFS(0xD18, 0xCB0));
	GET_STACK(float, DrawOffsetY, STACK_OFFS(0xD18, 0xCB8));
	LEA_STACK(LPDDSURFACEDESC2, lpDesc, STACK_OFFS(0xD18, 0x92C));

	auto pThis = (CIsoViewExt*)CIsoView::GetInstance();
	if (lpDesc->lpSurface == NULL) 
	{
		pThis->PrimarySurfaceLost();
		return 0x474DB3;
	}

	DDBoundary boundary{ lpDesc->dwWidth, lpDesc->dwHeight, lpDesc->lPitch };
	CIsoViewExt::drawOffsetX = DrawOffsetX;
	CIsoViewExt::drawOffsetY = DrawOffsetY;

	HDC hDC;
	pThis->lpDDBackBufferSurface->GetDC(&hDC);

	if (CIsoViewExt::DrawInfantries && CIsoViewExt::DrawInfantriesFilter && CViewObjectsExt::InfantryBrushDlgF)
	{
		auto CheckValue = [&](int nCheckBoxIdx, const ppmfc::CString& src, const ppmfc::CString& dst)
			{
				if (CViewObjectsExt::InfantryBrushBoolsF[nCheckBoxIdx - 1300])
				{
					if (dst == src) return true;
					else return false;
				}
				return true;
			};
		short idx = 0;
		for (const auto& data : CMapData::Instance->InfantryDatas)
		{
			const auto& filter = CViewObjectsExt::ObjectFilterI;
			if (filter.empty() || std::find(filter.begin(), filter.end(), data.TypeID) != filter.end())
			{
				if (CheckValue(1300, CViewObjectsExt::InfantryBrushDlgF->CString_House, data.House) &&
					CheckValue(1301, CViewObjectsExt::InfantryBrushDlgF->CString_HealthPoint, data.Health) &&
					CheckValue(1302, CViewObjectsExt::InfantryBrushDlgF->CString_State, data.Status) &&
					CheckValue(1303, CViewObjectsExt::InfantryBrushDlgF->CString_Direction, data.Facing) &&
					CheckValue(1304, CViewObjectsExt::InfantryBrushDlgF->CString_VerteranStatus, data.VeterancyPercentage) &&
					CheckValue(1305, CViewObjectsExt::InfantryBrushDlgF->CString_Group, data.Group) &&
					CheckValue(1306, CViewObjectsExt::InfantryBrushDlgF->CString_OnBridge, data.IsAboveGround) &&
					CheckValue(1307, CViewObjectsExt::InfantryBrushDlgF->CString_AutoCreateNoRecruitable, data.AutoNORecruitType) &&
					CheckValue(1308, CViewObjectsExt::InfantryBrushDlgF->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType) &&
					CheckValue(1309, CViewObjectsExt::InfantryBrushDlgF->CString_Tag, data.Tag))
					CIsoViewExt::VisibleInfantries.insert(idx);
			}
			idx++;
		}
	}
	if (CIsoViewExt::DrawUnits && CIsoViewExt::DrawUnitsFilter && CViewObjectsExt::VehicleBrushDlgF)
	{
		auto CheckValue = [&](int nCheckBoxIdx, const ppmfc::CString& src, const ppmfc::CString& dst)
			{
				if (CViewObjectsExt::VehicleBrushBoolsF[nCheckBoxIdx - 1300])
				{
					if (dst == src) return true;
					else return false;
				}
				return true;
			};
		short idx = 0;
		for (short idx = 0; idx < CINI::CurrentDocument->GetKeyCount("Units"); idx++)
		{
			CUnitData data;
			CMapData::Instance->GetUnitData(idx, data);
			const auto& filter = CViewObjectsExt::ObjectFilterV;
			if (filter.empty() || std::find(filter.begin(), filter.end(), data.TypeID) != filter.end())
			{
				if (CheckValue(1300, CViewObjectsExt::VehicleBrushDlgF->CString_House, data.House) &&
					CheckValue(1301, CViewObjectsExt::VehicleBrushDlgF->CString_HealthPoint, data.Health) &&
					CheckValue(1302, CViewObjectsExt::VehicleBrushDlgF->CString_State, data.Status) &&
					CheckValue(1303, CViewObjectsExt::VehicleBrushDlgF->CString_Direction, data.Facing) &&
					CheckValue(1304, CViewObjectsExt::VehicleBrushDlgF->CString_VeteranLevel, data.VeterancyPercentage) &&
					CheckValue(1305, CViewObjectsExt::VehicleBrushDlgF->CString_Group, data.Group) &&
					CheckValue(1306, CViewObjectsExt::VehicleBrushDlgF->CString_OnBridge, data.IsAboveGround) &&
					CheckValue(1307, CViewObjectsExt::VehicleBrushDlgF->CString_FollowerID, data.FollowsIndex) &&
					CheckValue(1308, CViewObjectsExt::VehicleBrushDlgF->CString_AutoCreateNoRecruitable, data.AutoNORecruitType) &&
					CheckValue(1309, CViewObjectsExt::VehicleBrushDlgF->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType) &&
					CheckValue(1310, CViewObjectsExt::VehicleBrushDlgF->CString_Tag, data.Tag))
					CIsoViewExt::VisibleUnits.insert(idx);
			}
		}
	}
	if (CIsoViewExt::DrawAircrafts && CIsoViewExt::DrawAircraftsFilter && CViewObjectsExt::AircraftBrushDlgF)
	{
		auto CheckValue = [&](int nCheckBoxIdx, const ppmfc::CString& src, const ppmfc::CString& dst)
			{
				if (CViewObjectsExt::AircraftBrushBoolsF[nCheckBoxIdx - 1300])
				{
					if (dst == src) return true;
					else return false;
				}
				return true;
			};
		for (short idx = 0; idx < CINI::CurrentDocument->GetKeyCount("Aircraft"); idx++)
		{
			CAircraftData data;
			CMapData::Instance->GetAircraftData(idx, data);
			const auto& filter = CViewObjectsExt::ObjectFilterA;
			if (filter.empty() || std::find(filter.begin(), filter.end(), data.TypeID) != filter.end())
			{
				if (CheckValue(1300, CViewObjectsExt::AircraftBrushDlgF->CString_House, data.House) &&
					CheckValue(1301, CViewObjectsExt::AircraftBrushDlgF->CString_HealthPoint, data.Health) &&
					CheckValue(1302, CViewObjectsExt::AircraftBrushDlgF->CString_Direction, data.Facing) &&
					CheckValue(1303, CViewObjectsExt::AircraftBrushDlgF->CString_Status, data.Status) &&
					CheckValue(1304, CViewObjectsExt::AircraftBrushDlgF->CString_VeteranLevel, data.VeterancyPercentage) &&
					CheckValue(1305, CViewObjectsExt::AircraftBrushDlgF->CString_Group, data.Group) &&
					CheckValue(1306, CViewObjectsExt::AircraftBrushDlgF->CString_AutoCreateNoRecruitable, data.AutoNORecruitType) &&
					CheckValue(1307, CViewObjectsExt::AircraftBrushDlgF->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType) &&
					CheckValue(1308, CViewObjectsExt::AircraftBrushDlgF->CString_Tag, data.Tag))
					CIsoViewExt::VisibleAircrafts.insert(idx);
			}
		}
	}
	if (CIsoViewExt::DrawStructures && CIsoViewExt::DrawStructuresFilter && CViewObjectsExt::BuildingBrushDlgBF)
	{
		auto CheckValue = [&](int nCheckBoxIdx, const ppmfc::CString& src, const ppmfc::CString& dst)
			{
				if (CViewObjectsExt::BuildingBrushBoolsBF[nCheckBoxIdx - 1300])
				{
					if (dst == src) return true;
					else return false;
				}
				return true;
			};
		for (short idx = 0; idx < CINI::CurrentDocument->GetKeyCount("Structures"); idx++)
		{
			CBuildingData data;
			CMapDataExt::GetBuildingDataByIniID(idx, data);
			const auto& filter = CViewObjectsExt::ObjectFilterB;
			if (filter.empty() || std::find(filter.begin(), filter.end(), data.TypeID) != filter.end())
			{
				if (CheckValue(1300, CViewObjectsExt::BuildingBrushDlgBF->CString_House, data.House) &&
					CheckValue(1301, CViewObjectsExt::BuildingBrushDlgBF->CString_HealthPoint, data.Health) &&
					CheckValue(1302, CViewObjectsExt::BuildingBrushDlgBF->CString_Direction, data.Facing) &&
					CheckValue(1303, CViewObjectsExt::BuildingBrushDlgBF->CString_Sellable, data.AISellable) &&
					CheckValue(1304, CViewObjectsExt::BuildingBrushDlgBF->CString_Rebuildable, data.AIRebuildable) &&
					CheckValue(1305, CViewObjectsExt::BuildingBrushDlgBF->CString_EnergySupport, data.PoweredOn) &&
					CheckValue(1306, CViewObjectsExt::BuildingBrushDlgBF->CString_UpgradeCount, data.Upgrades) &&
					CheckValue(1307, CViewObjectsExt::BuildingBrushDlgBF->CString_Spotlight, data.SpotLight) &&
					CheckValue(1308, CViewObjectsExt::BuildingBrushDlgBF->CString_Upgrade1, data.Upgrade1) &&
					CheckValue(1309, CViewObjectsExt::BuildingBrushDlgBF->CString_Upgrade2, data.Upgrade2) &&
					CheckValue(1310, CViewObjectsExt::BuildingBrushDlgBF->CString_Upgrade3, data.Upgrade3) &&
					CheckValue(1311, CViewObjectsExt::BuildingBrushDlgBF->CString_AIRepairs, data.AIRepairable) &&
					CheckValue(1312, CViewObjectsExt::BuildingBrushDlgBF->CString_ShowName, data.Nominal) &&
					CheckValue(1313, CViewObjectsExt::BuildingBrushDlgBF->CString_Tag, data.Tag))
					CIsoViewExt::VisibleStructures.insert(idx);
			}
		}
	}

	auto isCoordInFullMap = [](int X, int Y)
	{
		if (!ExtConfigs::DisplayObjectsOutside)
			return CMapData::Instance->IsCoordInMap(X, Y);

		return X >= 0 && Y >= 0 &&
			X < CMapData::Instance->MapWidthPlusHeight &&
			Y < CMapData::Instance->MapWidthPlusHeight;
	};

	for (int XplusY = Left + Top; XplusY < Right + Bottom; XplusY++) {
		for (int X = 0; X < XplusY; X++) {
			int Y = XplusY - X;
			if (!IsCoordInWindow(X, Y) || !isCoordInFullMap(X, Y)) continue;
			int pos = CMapData::Instance->GetCoordIndex(X, Y);
			int screenX = X, screenY = Y;
			CIsoView::MapCoord2ScreenCoord(screenX, screenY);
			screenX -= DrawOffsetX;
			screenY -= DrawOffsetY;
			visibleCells.push_back({ X, Y, screenX, screenY, pos,
				CMapData::Instance->IsCoordInMap(X, Y), 
				CMapData::Instance->GetCellAt(pos), 
				&CMapDataExt::CellDataExts[pos] });
		}
	}

	//loop1: tiles
	std::vector<MapCoord> RedrawCoords;
	for (const auto& info : visibleCells)
	{
		if (!info.isInMap) continue;
		auto& X = info.X;
		auto& Y = info.Y;
		auto& cell = info.cell;
		auto& cellExt = info.cellExt;
		auto& screenX = info.screenX;
		auto& screenY = info.screenY;

		CIsoViewExt::CurrentDrawCellLocation.X = X;
		CIsoViewExt::CurrentDrawCellLocation.Y = Y;
		CIsoViewExt::CurrentDrawCellLocation.Height = cell->Height;

		int altImage = cell->Flag.AltIndex;
		int tileIndex = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
		int tileSubIndex = CMapDataExt::GetSafeTileIndex(cell->TileSubIndex);
		if (tileIndex >= CMapDataExt::TileDataCount)
			continue;

		cell->Flag.RedrawTerrain = false;
		for (int i = 1; i <= 2; i++)
		{
			if (CMapData::Instance->IsCoordInMap(X - i, Y - i))
			{
				auto blockedCell = CMapData::Instance->GetCellAt(X - i, Y - i);
				if (cell->Height - blockedCell->Height >= 2 * i
					|| i == 1 && blockedCell->Flag.RedrawTerrain && cell->Height > blockedCell->Height)
					cell->Flag.RedrawTerrain = true;
			}
		}

		if (!cell->Flag.RedrawTerrain)
		{
			if (CFinalSunApp::Instance->FrameMode)
			{
				if (CMapDataExt::TileData[tileIndex].FrameModeIndex != 0xFFFF)
				{
					tileIndex = CMapDataExt::TileData[tileIndex].FrameModeIndex;
				}
				else
				{
					tileIndex = CMapDataExt::TileSet_starts[CMapDataExt::HeightBase] + cell->Height;
					tileSubIndex = 0;
				}
			}

			CTileTypeClass tile = CMapDataExt::TileData[tileIndex];
			int tileSet = tile.TileSet;
			if (tile.AltTypeCount)
			{
				if (altImage > 0)
				{
					altImage = altImage < tile.AltTypeCount ? altImage : tile.AltTypeCount;
					tile = tile.AltTypes[altImage - 1];
				}
			}
			if (tileSubIndex < tile.TileBlockCount && tile.TileBlockDatas[tileSubIndex].ImageData != NULL)
			{
				auto& subTile = tile.TileBlockDatas[tileSubIndex];
				int x = screenX;
				int y = screenY;
				x -= 60;
				y -= 30;

				if (subTile.HasValidImage)
				{
					Palette* pal = CMapDataExt::TileSetPalettes[CMapDataExt::TileData[tileIndex].TileSet];
						
					CIsoViewExt::BlitTerrain(pThis, lpDesc->lpSurface, window, boundary,
						x + subTile.XMinusExX, y + subTile.YMinusExY, &subTile, pal,
						cell->IsHidden() ? 128 : 255);

					auto& cellExt = CMapDataExt::CellDataExts[CMapData::Instance->GetCoordIndex(X, Y)];
					cellExt.HasAnim = false;
					if (CMapDataExt::TileAnimations.find(tileIndex) != CMapDataExt::TileAnimations.end())
					{
						auto& tileAnim = CMapDataExt::TileAnimations[tileIndex];
						if (tileAnim.AttachedSubTile == tileSubIndex)
						{
							cellExt.HasAnim = true;
						}
					}
					if (CMapDataExt::RedrawExtraTileSets.find(tileSet) != CMapDataExt::RedrawExtraTileSets.end())
						RedrawCoords.push_back(MapCoord{ X,Y });
				}
			}
		}
	}
	for (const auto& coord : RedrawCoords)
	{
		const int& X = coord.X;
		const int& Y = coord.Y;
		const auto cell = CMapData::Instance->GetCellAt(X, Y);

		CIsoViewExt::CurrentDrawCellLocation.X = X;
		CIsoViewExt::CurrentDrawCellLocation.Y = Y;
		CIsoViewExt::CurrentDrawCellLocation.Height = cell->Height;

		int altImage = cell->Flag.AltIndex;
		int tileIndex = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
		int tileSubIndex = CMapDataExt::GetSafeTileIndex(cell->TileSubIndex);

		if (CFinalSunApp::Instance->FrameMode)
		{
			if (CMapDataExt::TileData[tileIndex].FrameModeIndex != 0xFFFF)
			{
				tileIndex = CMapDataExt::TileData[tileIndex].FrameModeIndex;
			}
			else
			{
				tileIndex = CMapDataExt::TileSet_starts[CMapDataExt::HeightBase] + cell->Height;
				tileSubIndex = 0;
			}
		}

		CTileTypeClass tile = CMapDataExt::TileData[tileIndex];
		if (tile.AltTypeCount)
		{
			if (altImage > 0)
			{
				altImage = altImage < tile.AltTypeCount ? altImage : tile.AltTypeCount;
				tile = tile.AltTypes[altImage - 1];
			}
		}
		if (tileSubIndex < tile.TileBlockCount && tile.TileBlockDatas[tileSubIndex].ImageData != NULL)
		{
			auto& subTile = tile.TileBlockDatas[tileSubIndex];
			int x = X;
			int y = Y;
			CIsoView::MapCoord2ScreenCoord(x, y);
			x -= DrawOffsetX;
			y -= DrawOffsetY;
			x -= 60;
			y -= 30;

			if (subTile.HasValidImage)
			{
				Palette* pal = CMapDataExt::TileSetPalettes[CMapDataExt::TileData[tileIndex].TileSet];

				FString extraImageID;
				extraImageID.Format("EXTRAIMAGE\233%d%d%d", tileIndex, tileSubIndex, altImage);
				auto pData = CLoadingExt::GetImageDataFromServer(extraImageID);
				if (pData->pImageBuffer)
				{
					CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
						x + subTile.XMinusExX + 30,
						y + subTile.YMinusExY + 30,
						pData, pal, cell->IsHidden() ? 128 : 255, -2, -10);
				}			
			}
		}
	}

	//loop2: shadows
	bool shadow = CIsoViewExt::DrawShadows && ExtConfigs::InGameDisplay_Shadow;
	int shadowMask_width = window.right - window.left;
	int shadowMask_height = window.bottom - window.top;
	int shadowMask_size = shadowMask_width * shadowMask_height;
	std::vector<char> shadowMask_Building_Infantry(shadowMask_size, 0);
	std::vector<char> shadowMask_Terrain(shadowMask_size, 0);
	std::vector<char> shadowMask_Overlay(shadowMask_size, 0);
	for (const auto& info : visibleCells)
	{
		if (!info.isInMap) continue;
		auto& X = info.X;
		auto& Y = info.Y;
		auto& pos = info.pos;
		auto& cell = info.cell;
		auto& cellExt = info.cellExt;
		auto& x = info.screenX;
		auto& y = info.screenY;

		CIsoViewExt::CurrentDrawCellLocation.X = X;
		CIsoViewExt::CurrentDrawCellLocation.Y = Y;
		CIsoViewExt::CurrentDrawCellLocation.Height = cell->Height;

		if (cell->Waypoint != -1)
		{
			WaypointsToDraw.push_back(std::make_pair(MapCoord{ X, Y },
				CINI::CurrentDocument->GetKeyAt("Waypoints", cell->Waypoint)));
		}
		if (cell->Structure > -1 && cell->Structure < CMapDataExt::StructureIndexMap.size())
		{
			auto StrINIIndex = CMapDataExt::StructureIndexMap[cell->Structure];
			if (StrINIIndex != -1)
			{
				const auto& filter = CIsoViewExt::VisibleStructures;
				if (!CIsoViewExt::DrawStructuresFilter
					|| filter.find(StrINIIndex) != filter.end())
				{
					const auto& objRender = CMapDataExt::BuildingRenderDatasFix[StrINIIndex];
					if (std::find(DrawnBuildings.begin(), DrawnBuildings.end(), StrINIIndex) == DrawnBuildings.end())
					{
						DrawnBuildings.insert(StrINIIndex);
						MapCoord objCenter;
						const int BuildingIndex = CMapData::Instance->GetBuildingTypeID(objRender.ID);
						const auto& DataExt = CMapDataExt::BuildingDataExts[BuildingIndex];
						objCenter.X = objRender.X + DataExt.Height / 2;
						objCenter.Y = objRender.Y + DataExt.Width / 2;
						if (!isCoordInFullMap(objCenter.X, objCenter.Y))
						{
							objCenter.X = objRender.X;
							objCenter.Y = objRender.Y;
						}
						// if objects overlapping with building, draw building earlier
						if (DataExt.IsCustomFoundation())
						{
							for (const auto& block : *DataExt.Foundations)
							{
								MapCoord coord = { X + block.Y, Y + block.X };
								if (!isCoordInFullMap(coord.X, coord.Y))
									continue;

								if (!isCoordInFullMap(objCenter.X, objCenter.Y))
								{
									objCenter.X = coord.X;
									objCenter.Y = coord.Y;
								}

								auto buildingCell = CMapData::Instance->GetCellAt(coord.X, coord.Y);
								if (buildingCell->Unit != -1 || buildingCell->Aircraft != -1
									|| buildingCell->Terrain != -1 || buildingCell->Infantry[0] != -1
									|| buildingCell->Infantry[1] != -1 || buildingCell->Infantry[2] != -1)
								{
									objCenter.X = objRender.X;
									objCenter.Y = objRender.Y;
									break;
								}
							}
						}
						else
						{
							for (int dx = 0; dx < DataExt.Height; ++dx)
							{
								for (int dy = 0; dy < DataExt.Width; ++dy)
								{
									MapCoord coord = { X + dx, Y + dy };
									if (!isCoordInFullMap(coord.X, coord.Y))
										continue;

									if (!isCoordInFullMap(objCenter.X, objCenter.Y))
									{
										objCenter.X = coord.X;
										objCenter.Y = coord.Y;
									}

									auto buildingCell = CMapData::Instance->GetCellAt(coord.X, coord.Y);
									if (buildingCell->Unit != -1 || buildingCell->Aircraft != -1
										|| buildingCell->Terrain != -1 || buildingCell->Infantry[0] != -1
										|| buildingCell->Infantry[1] != -1 || buildingCell->Infantry[2] != -1)
									{
										objCenter.X = objRender.X;
										objCenter.Y = objRender.Y;
										break;
									}
								}
							}
						}
						BuildingsToDraw.push_back(std::make_pair(MapCoord{ objRender.X, objRender.Y },
							DrawBuildings{ StrINIIndex , (short)objCenter.X, (short)objCenter.Y, (short)BuildingIndex }));

						if (shadow && CIsoViewExt::DrawStructures)
						{
							int x1 = objRender.X;
							int y1 = objRender.Y;
							CIsoView::MapCoord2ScreenCoord(x1, y1);
							x1 -= DrawOffsetX;
							y1 -= DrawOffsetY;

							int nFacing = 0;
							if (Variables::RulesMap.GetBool(objRender.ID, "Turret") && !Variables::RulesMap.GetBool(objRender.ID, "TurretAnimIsVoxel"))
							{
								int FacingCount = CLoadingExt::GetAvailableFacing(objRender.ID);
								nFacing = (FacingCount + 7 * FacingCount / 8 - (objRender.Facing * FacingCount / 256) % FacingCount) % FacingCount;
							}

							const int HP = objRender.Strength;
							int status = CLoadingExt::GBIN_NORMAL;
							if (HP == 0)
								status = CLoadingExt::GBIN_RUBBLE;
							else if (static_cast<int>((CMapDataExt::ConditionRed + 0.001f) * 256) > HP)
								status = CLoadingExt::GBIN_DAMAGED;
							else if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP
								&& !(Variables::RulesMap.GetInteger(objRender.ID, "TechLevel") < 0 && Variables::RulesMap.GetBool(objRender.ID, "CanOccupyFire")))
								status = CLoadingExt::GBIN_DAMAGED;
							const auto& imageName = CLoadingExt::GetBuildingImageName(objRender.ID, nFacing, status, true);

							if (!CLoadingExt::IsObjectLoaded(objRender.ID))
							{
								CLoading::Instance->LoadObjects(objRender.ID);
							}

							auto pData = CLoadingExt::GetImageDataFromServer(imageName);
							if (pData->pImageBuffer)
							{
								CIsoViewExt::MaskShadowPixels(window, 
									x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, shadowMask_Building_Infantry);
							}
						}
					}
				}
			}
		}
		for (int i = 0; i < 3 && shadow; i++)
		{
			if (cell->Infantry[i] != -1 && CIsoViewExt::DrawInfantries)
			{
				const auto& filter = CIsoViewExt::VisibleInfantries;
				if (!CIsoViewExt::DrawInfantriesFilter
					|| std::find(filter.begin(), filter.end(), cell->Infantry[i]) != filter.end())
				{
					CInfantryData obj;
					CMapData::Instance->GetInfantryData(cell->Infantry[i], obj);
					int nFacing = 7 - (atoi(obj.Facing) / 32) % 8;

					bool water = false;
					const auto& swim = CLoadingExt::SwimableInfantries;
					if (ExtConfigs::InGameDisplay_Water && std::find(swim.begin(), swim.end(), obj.TypeID) != swim.end())
					{
						auto landType = CMapDataExt::GetLandType(cell->TileIndex, cell->TileSubIndex);
						if (landType == LandType::Water || landType == LandType::Beach)
						{
							water = true;
						}
					}
					bool deploy = ExtConfigs::InGameDisplay_Deploy
						&& obj.Status == "Unload" && Variables::RulesMap.GetBool(obj.TypeID, "Deployer");

					const auto& imageName = CLoadingExt::GetImageName(obj.TypeID, nFacing, true, deploy && !water, water);

					if (!CLoadingExt::IsObjectLoaded(obj.TypeID))
					{
						CLoading::Instance->LoadObjects(obj.TypeID);
					}
					auto pData = CLoadingExt::GetImageDataFromServer(imageName);

					if (pData->pImageBuffer)
					{
						int x1 = x;
						int y1 = y;
						switch (atoi(obj.SubCell))
						{
						case 2:
							x1 += 15;
							y1 += 14;
							break;
						case 3:
							x1 -= 15;
							y1 += 14;
							break;
						case 4:
							y1 += 22;
							break;
						default:
							y1 += 15;
							break;
						}

						if (ExtConfigs::InGameDisplay_Bridge && obj.IsAboveGround == "1")
							y1 -= 60;

						CIsoViewExt::MaskShadowPixels(window,
							x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, shadowMask_Building_Infantry);
					}
				}
			}
		}
		if (shadow && cell->Unit != -1 && CIsoViewExt::DrawUnits)
		{
			const auto& filter = CIsoViewExt::VisibleUnits;
			if (!CIsoViewExt::DrawUnitsFilter
				|| std::find(filter.begin(), filter.end(), cell->Unit) != filter.end())
			{
				CUnitData obj;
				CMapData::Instance->GetUnitData(cell->Unit, obj);

				FString ImageID = obj.TypeID;
				GetUnitImageID(ImageID, obj, CMapDataExt::GetLandType(cell->TileIndex, cell->TileSubIndex));

				int facings = CLoadingExt::GetAvailableFacing(obj.TypeID);
				int nFacing = (atoi(obj.Facing) * facings / 256) % facings;

				const auto& imageName = CLoadingExt::GetImageName(ImageID, nFacing, true);

				if (!CLoadingExt::IsObjectLoaded(ImageID))
				{
					CLoading::Instance->LoadObjects(ImageID);
				}
				auto pData = CLoadingExt::GetImageDataFromServer(imageName);

				if (pData->pImageBuffer)
				{
					int x1 = x;
					int y1 = y;

					if (ExtConfigs::InGameDisplay_Bridge && obj.IsAboveGround == "1")
						y1 -= 60;

					// units are special, they overlap with each other
					CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
						x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2 + 15, pData, NULL, 128);
				}
			}
		}
		if (shadow && cell->Terrain != -1 && CIsoViewExt::DrawTerrains)
		{
			auto obj = Variables::RulesMap.GetValueAt("TerrainTypes", cell->TerrainType);
			const auto& imageName = CLoadingExt::GetImageName(obj, 0, true);

			if (!CLoadingExt::IsObjectLoaded(obj))
			{
				CLoading::Instance->LoadObjects(obj);
			}
			auto pData = CLoadingExt::GetImageDataFromServer(imageName);

			if (pData->pImageBuffer)
			{
				int x1 = x;
				int y1 = y;

				CIsoViewExt::MaskShadowPixels(window, 
					x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2 + (Variables::RulesMap.GetBool(obj, "SpawnsTiberium") ? 0 : 12),
					pData, shadowMask_Terrain);
			}
		}
		if (shadow && cellExt->NewOverlay != 0xFFFF && CIsoViewExt::DrawOverlays)
		{
			auto imageName = CLoadingExt::GetOverlayName(cellExt->NewOverlay, cell->OverlayData, true);
			auto pData = CLoadingExt::GetImageDataFromServer(imageName);

			if (!pData || !pData->pImageBuffer)
			{
				auto obj = Variables::RulesMap.GetValueAt("OverlayTypes", cellExt->NewOverlay);
				if (!CLoadingExt::IsOverlayLoaded(obj))
				{
					CLoadingExt::GetExtension()->LoadOverlay(obj, cellExt->NewOverlay);
					pData = CLoadingExt::GetImageDataFromServer(imageName);
				}
			}

			if (pData && pData->pImageBuffer)
			{
				int x1 = x;
				int y1 = y;
				if (cellExt->NewOverlay == 0xA7)
					y1 -= 45;
				else if (
					cellExt->NewOverlay != 0x18 && cellExt->NewOverlay != 0x19 && // BRIDGE1, BRIDGE2
					cellExt->NewOverlay != 0x3B && cellExt->NewOverlay != 0x3C && // RAILBRDG1, RAILBRDG2
					cellExt->NewOverlay != 0xED && cellExt->NewOverlay != 0xEE // BRIDGEB1, BRIDGEB2
					)
				{
					if (cellExt->NewOverlay >= 0x27 && cellExt->NewOverlay <= 0x36) // Tracks
						y1 += 15;
					else if (cellExt->NewOverlay >= 0x4A && cellExt->NewOverlay <= 0x65) // LOBRDG 1-28
						y1 += 15;
					else if (cellExt->NewOverlay >= 0xCD && cellExt->NewOverlay <= 0xEC) // LOBRDGB 1-4
						y1 += 15;
					else if (cellExt->NewOverlay < CMapDataExt::OverlayTypeDatas.size())
					{
						if (CMapDataExt::OverlayTypeDatas[cellExt->NewOverlay].Rock
							//|| CMapDataExt::OverlayTypeDatas[cellExt->NewOverlay].TerrainRock // for compatibility of blockages
							|| CMapDataExt::OverlayTypeDatas[cellExt->NewOverlay].RailRoad)
							y1 += 15;
					}
				}
				else
				{
					if (cell->OverlayData >= 0x9 && cell->OverlayData <= 0x11)
						y1 -= 16;
					else
						y1 -= 1;
				}

				CIsoViewExt::MaskShadowPixels(window, 
					x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, shadowMask_Overlay);
			}
		}
	}
	if (shadow)
	{
		std::vector<byte> shadowMask(shadowMask_size, 0);
		for (size_t i = 0; i < shadowMask_size; ++i) {
			shadowMask[i] += shadowMask_Building_Infantry[i];
			shadowMask[i] += shadowMask_Terrain[i];
			shadowMask[i] += shadowMask_Overlay[i];
		}
		CIsoViewExt::DrawShadowMask(lpDesc->lpSurface, boundary, window, shadowMask);
	}

	//loop3: objects
	for (const auto& info : visibleCells)
	{
		if (!info.isInMap) continue;
		auto& X = info.X;
		auto& Y = info.Y;
		auto& pos = info.pos;
		auto& cell = info.cell;
		auto& cellExt = info.cellExt;
		auto& x = info.screenX;
		auto& y = info.screenY;

		CIsoViewExt::CurrentDrawCellLocation.X = X;
		CIsoViewExt::CurrentDrawCellLocation.Y = Y;
		CIsoViewExt::CurrentDrawCellLocation.Height = cell->Height;

		// tiles
		if (info.isInMap)
		{
			int altImage = cell->Flag.AltIndex;
			int tileIndex = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
			int tileSubIndex = CMapDataExt::GetSafeTileIndex(cell->TileSubIndex);
			if (tileIndex < CMapDataExt::TileDataCount)
			{
				auto drawTerrainAnim = [&pThis, &lpDesc, &boundary, &cell](int tileIndex, int tileSubIndex, int x, int y)
					{
						if (CMapDataExt::TileAnimations.find(tileIndex) != CMapDataExt::TileAnimations.end())
						{
							auto& tileAnim = CMapDataExt::TileAnimations[tileIndex];
							if (tileAnim.AttachedSubTile == tileSubIndex)
							{
								auto pData = CLoadingExt::GetImageDataFromServer(tileAnim.ImageName);

								if (pData->pImageBuffer)
								{
									CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
										x - pData->FullWidth / 2 + tileAnim.XOffset,
										y - pData->FullHeight / 2 + tileAnim.YOffset + 15,
										pData, NULL, cell->IsHidden() ? 128 : 255, -2, -10);
								}
							}
						}
					};

				if (cell->Flag.RedrawTerrain)
				{
					if (CFinalSunApp::Instance->FrameMode)
					{
						if (CMapDataExt::TileData[tileIndex].FrameModeIndex != 0xFFFF)
						{
							tileIndex = CMapDataExt::TileData[tileIndex].FrameModeIndex;
						}
						else
						{
							tileIndex = CMapDataExt::TileSet_starts[CMapDataExt::HeightBase] + cell->Height;
							tileSubIndex = 0;
						}
					}

					CTileTypeClass tile = CMapDataExt::TileData[tileIndex];
					int tileSet = tile.TileSet;
					if (tile.AltTypeCount)
					{
						if (altImage > 0)
						{
							altImage = altImage < tile.AltTypeCount ? altImage : tile.AltTypeCount;
							tile = tile.AltTypes[altImage - 1];
						}
					}
					if (tileSubIndex < tile.TileBlockCount && tile.TileBlockDatas[tileSubIndex].ImageData != NULL)
					{
						auto& subTile = tile.TileBlockDatas[tileSubIndex];
						int x1 = x;
						int y1 = y;
						x1 -= 60;
						y1 -= 30;

						if (subTile.HasValidImage)
						{
							Palette* pal = CMapDataExt::TileSetPalettes[CMapDataExt::TileData[tileIndex].TileSet];

							CIsoViewExt::BlitTerrain(pThis, lpDesc->lpSurface, window, boundary,
								x1 + subTile.XMinusExX, y1 + subTile.YMinusExY, &subTile, pal,
								cell->IsHidden() ? 128 : 255);

							if (CMapDataExt::RedrawExtraTileSets.find(tileSet) != CMapDataExt::RedrawExtraTileSets.end())
							{
								FString extraImageID;
								extraImageID.Format("EXTRAIMAGE\233%d%d%d", tileIndex, tileSubIndex, altImage);
								auto pData = CLoadingExt::GetImageDataFromServer(extraImageID);
								if (pData->pImageBuffer)
								{
									CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
										x1 + subTile.XMinusExX + 30,
										y1 + subTile.YMinusExY + 30,
										pData, pal, cell->IsHidden() ? 128 : 255, -2, -10);
								}
							}
							drawTerrainAnim(tileIndex, tileSubIndex, x1 + 60, y1 + 30);
						}
					}
				}
				else
				{
					if (cellExt->HasAnim)
					{
						if (CFinalSunApp::Instance->FrameMode)
						{
							if (CMapDataExt::TileData[tileIndex].FrameModeIndex != 0xFFFF)
							{
								tileIndex = CMapDataExt::TileData[tileIndex].FrameModeIndex;
							}
							else
							{
								tileIndex = CMapDataExt::TileSet_starts[CMapDataExt::HeightBase] + cell->Height;
								tileSubIndex = 0;
							}
						}
						drawTerrainAnim(tileIndex, tileSubIndex, x, y);
					}
				}
			}
		}
			
		//smudges
		if (cell->Smudge != -1 && CIsoViewExt::DrawSmudges)
		{
			auto obj = Variables::RulesMap.GetValueAt("SmudgeTypes", cell->SmudgeType);
			const auto& imageName = CLoadingExt::GetImageName(obj, 0);
			if (!CLoadingExt::IsObjectLoaded(obj))
			{
				CLoading::Instance->LoadObjects(obj);
			}
			auto pData = CLoadingExt::GetImageDataFromServer(imageName);

			if (pData->pImageBuffer)
			{
				CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
					x - pData->FullWidth / 2, y - pData->FullHeight / 2, pData, NULL, 255, 0, -1, false);
			}
		}

		//overlays
		int nextPos = CMapData::Instance->GetCoordIndex(X + 1, Y + 1);
		if (nextPos >= CMapData::Instance->CellDataCount)
			nextPos = 0;
		auto cellNext = CMapData::Instance->GetCellAt(nextPos);
		auto& cellNextExt = CMapDataExt::CellDataExts[nextPos];
		if ((cellExt->NewOverlay != 0xFFFF || cellNextExt.NewOverlay != 0xFFFF) && CIsoViewExt::DrawOverlays)
		{
			if (
				cellNextExt.NewOverlay == 0x18 || cellNextExt.NewOverlay == 0x19 || // BRIDGE1, BRIDGE2
				cellNextExt.NewOverlay == 0x3B || cellNextExt.NewOverlay == 0x3C || // RAILBRDG1, RAILBRDG2
				cellNextExt.NewOverlay == 0xED || cellNextExt.NewOverlay == 0xEE || // BRIDGEB1, BRIDGEB2
				(cellNextExt.NewOverlay >= 0x4A && cellNextExt.NewOverlay <= 0x65) || // LOBRDG 1-28
				(cellNextExt.NewOverlay >= 0xCD && cellNextExt.NewOverlay <= 0xEC) // LOBRDGB 1-4
				)
			{
				auto imageName = CLoadingExt::GetOverlayName(cellNextExt.NewOverlay, cellNext->OverlayData);
				auto pData = CLoadingExt::GetImageDataFromServer(imageName);

				if (!pData || !pData->pImageBuffer)
				{
					auto obj = Variables::RulesMap.GetValueAt("OverlayTypes", cellNextExt.NewOverlay);
					if (!CLoadingExt::IsOverlayLoaded(obj))
					{
						CLoadingExt::GetExtension()->LoadOverlay(obj, cellNextExt.NewOverlay);
						pData = CLoadingExt::GetImageDataFromServer(imageName);
					}
					if (!pData || !pData->pImageBuffer)
					{
						if (!(cellNextExt.NewOverlay >= 0x4a && cellNextExt.NewOverlay <= 0x65) &&
							!(cellNextExt.NewOverlay >= 0xcd && cellNextExt.NewOverlay <= 0xec))
						{
							char cd[10];
							cd[0] = '0';
							cd[1] = 'x';
							_itoa(cellNextExt.NewOverlay, cd + 2, 16);
							OverlayTextsToDraw.push_back(std::make_pair(MapCoord{ X+1,Y+1 }, cd));
						}
					}
				}
				if (pData && pData->pImageBuffer)
				{
					int x1 = x;
					int y1 = y;
					if (cellNextExt.NewOverlay == 0xA7)
						y1 -= 45;
					else if (
						cellNextExt.NewOverlay != 0x18 && cellNextExt.NewOverlay != 0x19 && // BRIDGE1, BRIDGE2
						cellNextExt.NewOverlay != 0x3B && cellNextExt.NewOverlay != 0x3C && // RAILBRDG1, RAILBRDG2
						cellNextExt.NewOverlay != 0xED && cellNextExt.NewOverlay != 0xEE // BRIDGEB1, BRIDGEB2
						)
					{
						if (cellNextExt.NewOverlay >= 0x27 && cellNextExt.NewOverlay <= 0x36) // Tracks
							y1 += 15;
						else if (cellNextExt.NewOverlay >= 0x4A && cellNextExt.NewOverlay <= 0x65) // LOBRDG 1-28
							y1 += 15;
						else if (cellNextExt.NewOverlay >= 0xCD && cellNextExt.NewOverlay <= 0xEC) // LOBRDGB 1-4
							y1 += 15;
						else if (cellNextExt.NewOverlay < CMapDataExt::OverlayTypeDatas.size())
						{
							if (CMapDataExt::OverlayTypeDatas[cellNextExt.NewOverlay].Rock
								//|| CMapDataExt::OverlayTypeDatas[cellNextExt.NewOverlay].TerrainRock // for compatibility of blockages
								|| CMapDataExt::OverlayTypeDatas[cellNextExt.NewOverlay].RailRoad)
								y1 += 15;
						}
					}
					else
					{
						if (cellNext->OverlayData >= 0x9 && cellNext->OverlayData <= 0x11)
							y1 -= 16;
						else
							y1 -= 1;
					}
					y1 += 30;
					y1 -= (cellNext->Height - cell->Height) * 15;

					auto tmp = CIsoViewExt::CurrentDrawCellLocation;
					CIsoViewExt::CurrentDrawCellLocation.X++;
					CIsoViewExt::CurrentDrawCellLocation.Y++;
					CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
						x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, NULL, 255, 0, 500 + cellNextExt.NewOverlay, false);
					CIsoViewExt::CurrentDrawCellLocation = tmp;
				}
			}
			if (
				cellExt->NewOverlay != 0xFFFF &&
				cellExt->NewOverlay != 0x18 && cellExt->NewOverlay != 0x19 && // BRIDGE1, BRIDGE2
				cellExt->NewOverlay != 0x3B && cellExt->NewOverlay != 0x3C && // RAILBRDG1, RAILBRDG2
				cellExt->NewOverlay != 0xED && cellExt->NewOverlay != 0xEE && // BRIDGEB1, BRIDGEB2
				!(cellExt->NewOverlay >= 0x4A && cellExt->NewOverlay <= 0x65) && // LOBRDG 1-28
				!(cellExt->NewOverlay >= 0xCD && cellExt->NewOverlay <= 0xEC) // LOBRDGB 1-4
				)
			{
				auto imageName = CLoadingExt::GetOverlayName(cellExt->NewOverlay, cell->OverlayData);
				auto pData = CLoadingExt::GetImageDataFromServer(imageName);

				if (!pData || !pData->pImageBuffer)
				{
					auto obj = Variables::RulesMap.GetValueAt("OverlayTypes", cellExt->NewOverlay);
					if (!CLoadingExt::IsOverlayLoaded(obj))
					{
						CLoadingExt::GetExtension()->LoadOverlay(obj, cellExt->NewOverlay);
						pData = CLoadingExt::GetImageDataFromServer(imageName);
					}
					if (!pData || !pData->pImageBuffer)
					{
						if (!(cellExt->NewOverlay >= 0x4a && cellExt->NewOverlay <= 0x65) &&
							!(cellExt->NewOverlay >= 0xcd && cellExt->NewOverlay <= 0xec))
						{
							char cd[10];
							cd[0] = '0';
							cd[1] = 'x';
							_itoa(cellExt->NewOverlay, cd + 2, 16);
							OverlayTextsToDraw.push_back(std::make_pair(MapCoord{ X,Y }, cd));
						}
					}
				}
				if (pData && pData->pImageBuffer)
				{
					int x1 = x;
					int y1 = y;
					if (cellExt->NewOverlay == 0xA7)
						y1 -= 45;
					else if (
						cellExt->NewOverlay != 0x18 && cellExt->NewOverlay != 0x19 && // BRIDGE1, BRIDGE2
						cellExt->NewOverlay != 0x3B && cellExt->NewOverlay != 0x3C && // RAILBRDG1, RAILBRDG2
						cellExt->NewOverlay != 0xED && cellExt->NewOverlay != 0xEE // BRIDGEB1, BRIDGEB2
						)
					{
						if (cellExt->NewOverlay >= 0x27 && cellExt->NewOverlay <= 0x36) // Tracks
							y1 += 15;
						else if (cellExt->NewOverlay >= 0x4A && cellExt->NewOverlay <= 0x65) // LOBRDG 1-28
							y1 += 15;
						else if (cellExt->NewOverlay >= 0xCD && cellExt->NewOverlay <= 0xEC) // LOBRDGB 1-4
							y1 += 15;
						else if (cellExt->NewOverlay < CMapDataExt::OverlayTypeDatas.size())
						{
							if (CMapDataExt::OverlayTypeDatas[cellExt->NewOverlay].Rock
								//|| CMapDataExt::OverlayTypeDatas[cellExt->NewOverlay].TerrainRock // for compatibility of blockages
								|| CMapDataExt::OverlayTypeDatas[cellExt->NewOverlay].RailRoad)
								y1 += 15;
						}
					}
					else
					{
						if (cell->OverlayData >= 0x9 && cell->OverlayData <= 0x11)
							y1 -= 16;
						else
							y1 -= 1;
					}
					CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
						x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, NULL, 255, 0, 500 + cellExt->NewOverlay, false);
				}
			}
		}

		//terrains
		if (cell->Terrain != -1 && CIsoViewExt::DrawTerrains)
		{
			auto obj = Variables::RulesMap.GetValueAt("TerrainTypes", cell->TerrainType);
			const auto& imageName = CLoadingExt::GetImageName(obj, 0);

			if (!CLoadingExt::IsObjectLoaded(obj))
			{
				CLoading::Instance->LoadObjects(obj);
			}
			auto pData = CLoadingExt::GetImageDataFromServer(imageName);

			if (pData->pImageBuffer)
			{
				CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
					x - pData->FullWidth / 2, y - pData->FullHeight / 2 + (Variables::RulesMap.GetBool(obj, "SpawnsTiberium") ? 0 : 12),
					pData, NULL, 255, 0, -1, false);

				if (ExtConfigs::InGameDisplay_AlphaImage && CIsoViewExt::DrawAlphaImages)
				{
					if (auto pAIFile = Variables::RulesMap.TryGetString(obj, "AlphaImage"))
					{
						auto pAIData = CLoadingExt::GetImageDataFromServer(*pAIFile + "\233ALPHAIMAGE");
						if (pAIData && pAIData->pImageBuffer)
						{
							AlphaImagesToDraw.push_back(
								std::make_pair(MapCoord{ x - pAIData->FullWidth / 2,
									y - pAIData->FullHeight / 2 + (Variables::RulesMap.GetBool(obj, "SpawnsTiberium") ? 0 : 12) },
									pAIData));
						}
					}
				}
			}
		}

		//buildings
		for (const auto& [mc, draw] : BuildingsToDraw)
		{
			if ((draw.x == X && draw.y == Y))
			{
				int pos = CMapData::Instance->GetCoordIndex(X, Y);
				const auto& objRender = CMapDataExt::BuildingRenderDatasFix[draw.index];
				int x = mc.X;
				int y = mc.Y;
				CIsoView::MapCoord2ScreenCoord(x, y);
				x -= DrawOffsetX;
				y -= DrawOffsetY;

				if (CFinalSunApp::Instance->ShowBuildingCells)
				{
					const auto& DataExt = CMapDataExt::BuildingDataExts[draw.buildingIndex];
					if (DataExt.IsCustomFoundation())
						pThis->DrawLockedLines(*DataExt.LinesToDraw, x, y, objRender.HouseColor, false, false, lpDesc);
					else
						pThis->DrawLockedCellOutline(x, y, DataExt.Width, DataExt.Height, objRender.HouseColor, false, false, lpDesc);
				}

				if (CIsoViewExt::DrawStructures)
				{
					int nFacing = 0;
					if (Variables::RulesMap.GetBool(objRender.ID, "Turret"))
					{
						int FacingCount = CLoadingExt::GetAvailableFacing(objRender.ID);
						nFacing = (FacingCount + 7 * FacingCount / 8 - (objRender.Facing * FacingCount / 256) % FacingCount) % FacingCount;
					}

					const int HP = objRender.Strength;
					int status = CLoadingExt::GBIN_NORMAL;
					if (HP == 0)
						status = CLoadingExt::GBIN_RUBBLE;
					else if (static_cast<int>((CMapDataExt::ConditionRed + 0.001f) * 256) > HP)
						status = CLoadingExt::GBIN_DAMAGED;
					else if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP
						&& !(Variables::RulesMap.GetInteger(objRender.ID, "TechLevel") < 0 && Variables::RulesMap.GetBool(objRender.ID, "CanOccupyFire")))
						status = CLoadingExt::GBIN_DAMAGED;
					const auto& imageName = CLoadingExt::GetBuildingImageName(objRender.ID, nFacing, status);

					if (!CLoadingExt::IsObjectLoaded(objRender.ID))
					{
						CLoading::Instance->LoadObjects(objRender.ID);
					}
					auto pData = CLoadingExt::GetImageDataFromServer(imageName);

					if (pData->pImageBuffer)
					{
						auto& isoset = CMapDataExt::TerrainPaletteBuildings;
						auto& dam_rubble = CMapDataExt::DamagedAsRubbleBuildings;
						CIsoViewExt::BlitSHPTransparent_Building(pThis, lpDesc->lpSurface, window, boundary,
							x - pData->FullWidth / 2, y - pData->FullHeight / 2, pData, NULL, 255,
							objRender.HouseColor, -1, status == CLoadingExt::GBIN_RUBBLE &&
							dam_rubble.find(objRender.ID) == dam_rubble.end()
							&& imageName != CLoadingExt::GetBuildingImageName(objRender.ID, nFacing, CLoadingExt::GBIN_DAMAGED),
							isoset.find(objRender.ID) != isoset.end());

						for (int upgrade = 0; upgrade < objRender.PowerUpCount; ++upgrade)
						{
							const auto& upg = upgrade == 0 ? objRender.PowerUp1 : (upgrade == 1 ? objRender.PowerUp2 : objRender.PowerUp3);
							const auto& upgXX = upgrade == 0 ? "PowerUp1LocXX" : (upgrade == 1 ? "PowerUp2LocXX" : "PowerUp3LocXX");
							const auto& upgYY = upgrade == 0 ? "PowerUp1LocYY" : (upgrade == 1 ? "PowerUp2LocYY" : "PowerUp3LocYY");
							if (upg.GetLength() == 0)
								continue;

							auto pUpgData = CLoadingExt::GetImageDataFromServer(CLoadingExt::GetImageName(upg, 0));
							if ((!pUpgData || !pUpgData->pImageBuffer) && !CLoadingExt::IsObjectLoaded(upg))
							{
								CLoading::Instance->LoadObjects(upg);
							}
							if (pUpgData && pUpgData->pImageBuffer)
							{
								auto ArtID = CLoadingExt::GetArtID(objRender.ID);

								int x1 = x;
								int y1 = y;
								x1 += CINI::Art->GetInteger(ArtID, upgXX, 0);
								y1 += CINI::Art->GetInteger(ArtID, upgYY, 0);
								CIsoViewExt::BlitSHPTransparent_Building(pThis, lpDesc->lpSurface, window, boundary,
									x1 - pUpgData->FullWidth / 2, y1 - pUpgData->FullHeight / 2, pUpgData, NULL, 255,
									objRender.HouseColor, -1, false, isoset.find(objRender.ID) != isoset.end());
							}
						}
						if (ExtConfigs::InGameDisplay_AlphaImage && CIsoViewExt::DrawAlphaImages && objRender.poweredOn)
						{
							if (auto pAIFile = Variables::RulesMap.TryGetString(objRender.ID, "AlphaImage"))
							{
								auto pAIData = CLoadingExt::GetImageDataFromServer(*pAIFile + "\233ALPHAIMAGE");
								if (pAIData && pAIData->pImageBuffer)
								{
									AlphaImagesToDraw.push_back(
										std::make_pair(MapCoord{ x - pAIData->FullWidth / 2, y - pAIData->FullHeight / 2 + 15 },pAIData));
								}
							}
						}
					}
				}
				break;
			}
		}

		//nodes
		if (CIsoViewExt::DrawBasenodes)
		{
			if (!cellExt->BaseNodes.empty())
			{
				auto pThis = (CIsoViewExt*)CIsoView::GetInstance();
				DDBoundary boundary{ lpDesc->dwWidth, lpDesc->dwHeight, lpDesc->lPitch };

				for (const auto& node : cellExt->BaseNodes)
				{
					if (std::find(DrawnBaseNodes.begin(), DrawnBaseNodes.end(), node) == DrawnBaseNodes.end())
					{
						DrawnBaseNodes.push_back(node);

						int X = node.X;
						int Y = node.Y;
						CIsoView::MapCoord2ScreenCoord(X, Y);
						X -= DrawOffsetX;
						Y -= DrawOffsetY;

						if (CIsoViewExt::DrawBasenodesFilter && CViewObjectsExt::BuildingBrushDlgBNF)
						{
							const auto& filter = CViewObjectsExt::ObjectFilterBN;
							auto CheckValue = [&](int nCheckBoxIdx, ppmfc::CString& src, const ppmfc::CString& dst)
								{
									if (CViewObjectsExt::BuildingBrushBoolsBNF[nCheckBoxIdx - 1300])
									{
										if (dst == src) return true;
										else return false;
									}
									return true;
								};
							if (filter.empty() || std::find(filter.begin(), filter.end(), node.ID) != filter.end())
							{
								if (!CheckValue(1300, CViewObjectsExt::BuildingBrushDlgBNF->CString_House, node.House))
									continue;
							}
							else
							{
								continue;
							}
						}
						const auto& imageName = CLoadingExt::GetBuildingImageName(node.ID, 0, 0);
						auto color = Miscs::GetColorRef(node.House);

						if (!CLoadingExt::IsObjectLoaded(node.ID))
						{
							CLoading::Instance->LoadObjects(node.ID);
						}
						auto pData = CLoadingExt::GetImageDataFromServer(imageName);
						int cellStr = -1;
						if (cell->Structure > -1 && cell->Structure < CMapDataExt::StructureIndexMap.size())
							cellStr = CMapDataExt::StructureIndexMap[cell->Structure];
						if (CFinalSunApp::Instance->ShowBuildingCells || cellStr != -1)
						{
							const int BuildingIndex = CMapData::Instance->GetBuildingTypeID(node.ID);
							const auto& DataExt = CMapDataExt::BuildingDataExts[BuildingIndex];
							if (DataExt.IsCustomFoundation())
							{
								pThis->DrawLockedLines(*DataExt.LinesToDraw, X, Y, color, true, false, lpDesc);
								pThis->DrawLockedLines(*DataExt.LinesToDraw, X + 1, Y, color, true, false, lpDesc);
							}
							else
							{
								pThis->DrawLockedCellOutline(X, Y, DataExt.Width, DataExt.Height, color, true, false, lpDesc);
								pThis->DrawLockedCellOutline(X + 1, Y, DataExt.Width, DataExt.Height, color, true, false, lpDesc);
							}
						}

						if (pData->pImageBuffer)
						{
							auto& isoset = CMapDataExt::TerrainPaletteBuildings;
							CIsoViewExt::BlitSHPTransparent_Building(pThis, lpDesc->lpSurface, window, boundary,
								X - pData->FullWidth / 2, Y - pData->FullHeight / 2, pData, NULL, 128,
								color, -1, false, isoset.find(node.ID) != isoset.end());
						}
					}
				}
			}
		}

		//units
		if (cell->Unit != -1 && CIsoViewExt::DrawUnits)
		{
			const auto& filter = CIsoViewExt::VisibleUnits;
			if (!CIsoViewExt::DrawUnitsFilter
				|| std::find(filter.begin(), filter.end(), cell->Unit) != filter.end())
			{
				CUnitData obj;
				CMapData::Instance->GetUnitData(cell->Unit, obj);

				FString ImageID = obj.TypeID;
				GetUnitImageID(ImageID, obj, CMapDataExt::GetLandType(cell->TileIndex, cell->TileSubIndex));

				int facings = CLoadingExt::GetAvailableFacing(obj.TypeID);
				int nFacing = (atoi(obj.Facing) * facings / 256) % facings;

				const auto& imageName = CLoadingExt::GetImageName(ImageID, nFacing);

				if (!CLoadingExt::IsObjectLoaded(ImageID))
				{
					CLoading::Instance->LoadObjects(ImageID);
				}
				auto pData = CLoadingExt::GetImageDataFromServer(imageName);

				if (pData->pImageBuffer)
				{
					bool HoveringUnit = ExtConfigs::InGameDisplay_Hover && Variables::RulesMap.GetString(obj.TypeID, "SpeedType") == "Hover"
						&& (Variables::RulesMap.GetString(obj.TypeID, "Locomotor") == "Hover"
							|| Variables::RulesMap.GetString(obj.TypeID, "Locomotor") == "{4A582742-9839-11d1-B709-00A024DDAFD1}");

					auto color = Miscs::GetColorRef(obj.House);

					if (ExtConfigs::InGameDisplay_Bridge && obj.IsAboveGround == "1")
						pThis->DrawLine(x + 30, y + 15 - (HoveringUnit ? 10 : 0) - 60 - 30,
							x + 30, y + 15 - (HoveringUnit ? 10 : 0) - 30, ExtConfigs::CursorSelectionBound_HeightColor, false, false, lpDesc, true);

					CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
						x - pData->FullWidth / 2,
						y - pData->FullHeight / 2 + 15 - (HoveringUnit ? 10 : 0) - 
						(ExtConfigs::InGameDisplay_Bridge && obj.IsAboveGround == "1" ? 60 : 0),
						pData, NULL, 255, color, 0, true);

					auto& veter = DrawVeterancies.emplace_back();
					int	VP = atoi(obj.VeterancyPercentage);
					veter.X = x;
					veter.Y = y - (HoveringUnit ? 10 : 0) - (ExtConfigs::InGameDisplay_Bridge && obj.IsAboveGround == "1" ? 60 : 0);
					veter.VP = VP;
				}
			}
		}

		//aircrafts
		if (cell->Aircraft != -1 && CIsoViewExt::DrawAircrafts)
		{
			const auto& filter = CIsoViewExt::VisibleAircrafts;
			if (!CIsoViewExt::DrawAircraftsFilter
				|| std::find(filter.begin(), filter.end(), cell->Aircraft) != filter.end())
			{
				CAircraftData obj;
				CMapData::Instance->GetAircraftData(cell->Aircraft, obj);
				auto imageID = obj.TypeID;

				if (ExtConfigs::InGameDisplay_Damage)
				{
					int HP = atoi(obj.Health);
					if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP)
					{
						imageID = Variables::RulesMap.GetString(obj.TypeID, "Image.ConditionYellow", imageID);
					}
					if (static_cast<int>((CMapDataExt::ConditionRed + 0.001f) * 256) > HP)
					{
						imageID = Variables::RulesMap.GetString(obj.TypeID, "Image.ConditionRed", imageID);
					}
				}

				if (!CLoadingExt::IsObjectLoaded(imageID))
				{
					CLoading::Instance->LoadObjects(imageID);
				}

				int facings = CLoadingExt::GetAvailableFacing(obj.TypeID);
				int nFacing = (atoi(obj.Facing) * facings / 256) % facings;
				const auto& imageName = CLoadingExt::GetImageName(imageID, nFacing);
				auto pData = CLoadingExt::GetImageDataFromServer(imageName);

				if (pData->pImageBuffer)
				{
					auto color = Miscs::GetColorRef(obj.House);
					CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
						x - pData->FullWidth / 2, y - pData->FullHeight / 2 + 15, pData, NULL, 255, color, 2, true);

					auto& veter = DrawVeterancies.emplace_back();
					int	VP = atoi(obj.VeterancyPercentage);
					veter.X = x;
					veter.Y = y;
					veter.VP = VP;
				}
			}
		}

		//infantries
		for (int i = 2; i >= 0; --i)
		{
			if (cell->Infantry[i] != -1 && CIsoViewExt::DrawInfantries)
			{
				const auto& filter = CIsoViewExt::VisibleInfantries;
				if (!CIsoViewExt::DrawInfantriesFilter
					|| std::find(filter.begin(), filter.end(), cell->Infantry[i]) != filter.end())
				{
					CInfantryData obj;
					CMapData::Instance->GetInfantryData(cell->Infantry[i], obj);
					int nFacing = 7 - (atoi(obj.Facing) / 32) % 8;

					bool water = false;
					const auto& swim = CLoadingExt::SwimableInfantries;
					if (ExtConfigs::InGameDisplay_Water && std::find(swim.begin(), swim.end(), obj.TypeID) != swim.end())
					{
						auto landType = CMapDataExt::GetLandType(cell->TileIndex, cell->TileSubIndex);
						if (landType == LandType::Water || landType == LandType::Beach)
						{
							water = true;
						}
					}
					bool deploy = ExtConfigs::InGameDisplay_Deploy
						&& obj.Status == "Unload" && Variables::RulesMap.GetBool(obj.TypeID, "Deployer");

					const auto& imageName = CLoadingExt::GetImageName(obj.TypeID, nFacing, false, deploy && !water, water);

					if (!CLoadingExt::IsObjectLoaded(obj.TypeID))
					{
						CLoading::Instance->LoadObjects(obj.TypeID);
					}
					auto pData = CLoadingExt::GetImageDataFromServer(imageName);

					if (pData->pImageBuffer)
					{
						int x1 = x;
						int y1 = y;
						switch (atoi(obj.SubCell))
						{
						case 2:
							x1 += 15;
							y1 += 14;
							break;
						case 3:
							x1 -= 15;
							y1 += 14;
							break;
						case 4:
							y1 += 22;
							break;
						default:
							y1 += 15;
							break;
						}

						if (ExtConfigs::InGameDisplay_Bridge && obj.IsAboveGround == "1")
							y1 -= 60;

						if (ExtConfigs::InGameDisplay_Bridge && obj.IsAboveGround == "1")
							pThis->DrawLine(x1 + 30, y1 - 30,
								x1 + 30, y1 + 60 - 30, ExtConfigs::CursorSelectionBound_HeightColor, false, false, lpDesc, true);

						auto color = Miscs::GetColorRef(obj.House);
						CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
							x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, NULL, 255, color, 1, true);

						auto& veter = DrawVeterancies.emplace_back();
						int	VP = atoi(obj.VeterancyPercentage);
						veter.X = x1 - 5;
						veter.Y = y1 - 4 - 15;
						veter.VP = VP;
					}
				}
			}
		}
	}

	for (const auto& ai : AlphaImagesToDraw)
	{
		CIsoViewExt::BlitSHPTransparent_AlphaImage(pThis, lpDesc->lpSurface, window, boundary,
			ai.first.X, ai.first.Y, ai.second);
	}

	const char* InsigniaVeteran = "FA2spInsigniaVeteran";
	const char* InsigniaElite = "FA2spInsigniaElite";
	auto veteran = CLoadingExt::GetImageDataFromMap(InsigniaVeteran);
	auto elite = CLoadingExt::GetImageDataFromMap(InsigniaElite);

	for (auto& dv : DrawVeterancies)
	{
		if (dv.VP >= 200)
			CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
				dv.X - elite->FullWidth / 2 + 10, dv.Y + 21 - elite->FullHeight / 2, elite, 0, 255, 0, -100, false);
		else if (dv.VP >= 100)
			CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary,
				dv.X - veteran->FullWidth / 2 + 10, dv.Y + 21 - veteran->FullWidth / 2, veteran, 0, 255, 0, -100, false);
	}

	if (CIsoViewExt::DrawTubes)
	{
		for (const auto& tube : CMapDataExt::Tubes)
		{
			int color = tube.PositiveFacing ? RGB(255, 0, 0) : RGB(0, 0, 255);
			int height = std::min(CMapData::Instance->TryGetCellAt(tube.StartCoord.X, tube.StartCoord.Y)->Height,
				CMapData::Instance->TryGetCellAt(tube.EndCoord.X, tube.EndCoord.Y)->Height);
			height *= 15;
			if (CFinalSunApp::Instance->FlatToGround)
				height = 0;
			for (int i = 0; i < tube.PathCoords.size() - 1; ++i)
			{
				int x1, x2, y1, y2;
				x1 = tube.PathCoords[i].X;
				y1 = tube.PathCoords[i].Y;
				x2 = tube.PathCoords[i + 1].X;
				y2 = tube.PathCoords[i + 1].Y;
				CIsoView::MapCoord2ScreenCoord_Flat(x1, y1);
				CIsoView::MapCoord2ScreenCoord_Flat(x2, y2);
				x1 -= DrawOffsetX;
				y1 -= DrawOffsetY;
				x2 -= DrawOffsetX;
				y2 -= DrawOffsetY;
				if (tube.PositiveFacing)
				{
					x1 += 1;
					y1 += 1;
					x2 += 1;
					y2 += 1;
				}
				else
				{
					x1 -= 1;
					y1 -= 1;
					x2 -= 1;
					y2 -= 1;
				}
				pThis->DrawLine(x1 + 30, y1 - 15 - height, x2 + 30, y2 - 15 - height, color, false, false, lpDesc);
			}
			int x1, x2, y1, y2;
			x1 = tube.StartCoord.X;
			y1 = tube.StartCoord.Y;
			x2 = tube.EndCoord.X;
			y2 = tube.EndCoord.Y;
			CIsoView::MapCoord2ScreenCoord_Flat(x1, y1);
			CIsoView::MapCoord2ScreenCoord_Flat(x2, y2);
			x1 -= DrawOffsetX;
			y1 -= DrawOffsetY;
			x2 -= DrawOffsetX;
			y2 -= DrawOffsetY;
			if (tube.PositiveFacing)
			{
				x1 += 1;
				y1 += 1;
				x2 += 1;
				y2 += 1;
			}
			else
			{
				x1 -= 1;
				y1 -= 1;
				x2 -= 1;
				y2 -= 1;
			}
			pThis->DrawLockedCellOutline(x1, y1 - height, 1, 1, color, true, false, lpDesc);
			pThis->DrawLockedCellOutlineX(x2, y2 - height, 1, 1, color, color, false, false, lpDesc, true);
		}
	}
	if (CIsoViewExt::RockCells)
	{
		auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");
		auto Map = &CMapData::Instance();
		for (int i = 0; i < Map->CellDataCount; i++)
		{
			auto cell = &Map->CellDatas[i];
			auto& cellExt = CMapDataExt::CellDataExts[i];
			int x = i % Map->MapWidthPlusHeight;
			int y = i / Map->MapWidthPlusHeight;
			int tileIndex = cell->TileIndex;
			if (tileIndex == 65535)
				tileIndex = 0;


			if (CMapDataExt::TileData && tileIndex < int(CTileTypeClass::InstanceCount()) && cell->TileSubIndex < CMapDataExt::TileData[tileIndex].TileBlockCount)
			{
				auto ttype = CMapDataExt::TileData[tileIndex].TileBlockDatas[cell->TileSubIndex].TerrainType;
				if (ttype == 0x7 || ttype == 0x8 || ttype == 0xf ||
					(cellExt.NewOverlay == 0xFFFF ? false : CMapDataExt::GetOverlayTypeData(cellExt.NewOverlay).TerrainRock))
				{
					CIsoView::MapCoord2ScreenCoord(x, y);
					int drawX = x - DrawOffsetX;
					int drawY = y - DrawOffsetY;
					pThis->DrawLockedCellOutlineX(drawX, drawY, 1, 1, RGB(255, 0, 0), RGB(40, 0, 0), true, false, lpDesc);
				}
			}
		}
	}
	if (CTerrainGenerator::RangeFirstCell.X > -1 && CTerrainGenerator::RangeSecondCell.X > -1)
	{
		if (MultiSelection::SelectedCoords.empty())
		{
			int X = CTerrainGenerator::RangeFirstCell.X, Y = CTerrainGenerator::RangeFirstCell.Y;

			if (CMapData::Instance().IsCoordInMap(X, Y))
			{
				int XW = abs(CTerrainGenerator::RangeSecondCell.X - CTerrainGenerator::RangeFirstCell.X) + 1;
				int YW = abs(CTerrainGenerator::RangeSecondCell.Y - CTerrainGenerator::RangeFirstCell.Y) + 1;
				if (X > CTerrainGenerator::RangeSecondCell.X)
					X = CTerrainGenerator::RangeSecondCell.X;
				if (Y > CTerrainGenerator::RangeSecondCell.Y)
					Y = CTerrainGenerator::RangeSecondCell.Y;

				CIsoView::MapCoord2ScreenCoord(X, Y);

				int drawX = X - DrawOffsetX;
				int drawY = Y - DrawOffsetY;

				pThis->DrawLockedCellOutline(drawX, drawY, YW, XW, ExtConfigs::TerrainGeneratorColor, false, false, lpDesc);
			}
		}
		else
		{
			CTerrainGenerator::RangeFirstCell.X = -1;
			CTerrainGenerator::RangeFirstCell.Y = -1;
			CTerrainGenerator::RangeSecondCell.X = -1;
			CTerrainGenerator::RangeSecondCell.Y = -1;
		}
	}

	// celltag, waypoint, annotation
	for (const auto& info : visibleCells)
	{
		if (!info.isInMap) continue;
		auto& X = info.X;
		auto& Y = info.Y;
		auto& pos = info.pos;
		auto& cell = info.cell;
		auto& cellExt = info.cellExt;
		auto& x = info.screenX;
		auto& y = info.screenY;

		if (CIsoViewExt::DrawCelltags && cell->CellTag != -1)
		{
			if (CIsoViewExt::DrawCellTagsFilter && !CViewObjectsExt::ObjectFilterCT.empty())
			{
				ppmfc::CString id = "";
				if (CMapData::Instance().INI.SectionExists("CellTags"))
					id = CMapData::Instance().INI.GetStringAt("CellTags", cell->CellTag);

				if (id != "")
				{
					for (auto& name : CViewObjectsExt::ObjectFilterCT)
					{
						if (name == id)
						{
							pThis->DrawCelltag(x, y, lpDesc);
							break;
						}
						if (STDHelpers::IsNumber(name))
						{
							int n = atoi(name);
							if (n < 1000000)
							{
								FString buffer;
								buffer.Format("%08d", n + 1000000);
								if (buffer == id)
								{
									pThis->DrawCelltag(x, y, lpDesc);
									break;
								}
							}
						}
					}
				}
			}
			else
				pThis->DrawCelltag(x, y, lpDesc);
		}

		if (CIsoViewExt::DrawWaypoints && cell->Waypoint != -1)
			pThis->DrawWaypointFlag(x, y, lpDesc);

		if (CMapDataExt::HasAnnotation(pos))
		{
			pThis->DrawBitmap("annotation", x + 5, y - 2, lpDesc);
		}
	}

	auto& cellDataExt = CMapDataExt::CellDataExt_FindCell;
	if (cellDataExt.drawCell)
	{
		int x = cellDataExt.X;
		int y = cellDataExt.Y;

		CIsoView::MapCoord2ScreenCoord(x, y);

		int drawX = x - DrawOffsetX;
		int drawY = y - DrawOffsetY;

		pThis->DrawBitmap("target", drawX - 20, drawY - 11, lpDesc);
	}

	if (CIsoViewExt::DrawOverlays)
	{
		SetBkMode(hDC, TRANSPARENT);
		SetTextAlign(hDC, TA_CENTER);
		SetTextColor(hDC, RGB(0, 0, 0));
		for (const auto& [coord, index] : OverlayTextsToDraw)
		{
			if (IsCoordInWindow(coord.X, coord.Y))
			{
				MapCoord mc = coord;
				CIsoView::MapCoord2ScreenCoord(mc.X, mc.Y);
				int drawX = mc.X - DrawOffsetX + 30;
				int drawY = mc.Y - DrawOffsetY - 25;
				TextOut(hDC, drawX, drawY, index, strlen(index));
			}
		}
	}

	if (CIsoViewExt::DrawBaseNodeIndex)
	{
		SetTextColor(hDC, ExtConfigs::BaseNodeIndex_Color);
		if (ExtConfigs::BaseNodeIndex_Background)
		{
			SetBkMode(hDC, OPAQUE);
			SetBkColor(hDC, ExtConfigs::BaseNodeIndex_Background_Color);
		}
		else
			SetBkMode(hDC, TRANSPARENT);
		SetTextAlign(hDC, TA_CENTER);

		auto& ini = CMapData::Instance->INI;
		if (auto pSection = ini.GetSection("Houses"))
		{
			for (auto& pair : pSection->GetEntities())
			{
				int nodeCount = ini.GetInteger(pair.second, "NodeCount", 0);
				if (nodeCount > 0)
				{
					for (int i = 0; i < nodeCount; i++)
					{
						char key[10];
						sprintf(key, "%03d", i);
						auto value = ini.GetString(pair.second, key, "");
						if (value == "")
							continue;
						auto atoms = STDHelpers::SplitString(value);
						if (atoms.size() < 3)
							continue;

						int x = atoi(atoms[2]);
						int y = atoi(atoms[1]);

						if (IsCoordInWindow(x, y))
						{
							CIsoView::MapCoord2ScreenCoord(x, y);

							int ndrawX = x - DrawOffsetX + 30;
							int ndrawY = y - DrawOffsetY - 15;

							TextOut(hDC, ndrawX, ndrawY, key, strlen(key));
						}
					}
				}
			}
		}
	}

	if (CIsoViewExt::DrawWaypoints)
	{
		SetTextColor(hDC, ExtConfigs::Waypoint_Color);
		if (ExtConfigs::Waypoint_Background)
		{
			SetBkMode(hDC, OPAQUE);
			SetBkColor(hDC, ExtConfigs::Waypoint_Background_Color);
		}
		else
			SetBkMode(hDC, TRANSPARENT);
		SetTextAlign(hDC, TA_CENTER);


		for (const auto& [coord, index] : WaypointsToDraw)
		{
			if (IsCoordInWindow(coord.X, coord.Y))
			{
				MapCoord mc = coord;
				CIsoView::MapCoord2ScreenCoord(mc.X, mc.Y);
				int drawX = mc.X - DrawOffsetX + 30 + ExtConfigs::Waypoint_Text_ExtraOffset.x;
				int drawY = mc.Y - DrawOffsetY - 15 + ExtConfigs::Waypoint_Text_ExtraOffset.y;
				TextOut(hDC, drawX, drawY, index, strlen(index));
			}
		}
	}

	SetTextAlign(hDC, TA_LEFT);
	SetTextColor(hDC, RGB(0, 0, 0));

	if (auto pSection = CINI::CurrentDocument->GetSection("Annotations"))
	{
		for (const auto& [key, value] : pSection->GetEntities())
		{
			auto pos = atoi(key);
			int x = pos / 1000;
			int y = pos % 1000;
			if (!IsCoordInWindow(x, y) || (!ExtConfigs::DisplayObjectsOutside && !CMapData::Instance->IsCoordInMap(x, y)))
				continue;
			CIsoView::MapCoord2ScreenCoord(x, y);
			x -= DrawOffsetX;
			y -= DrawOffsetY;
			x += 23;
			y -= 15;
			auto atoms = FString::SplitString(value, 6);
			int fontSize = std::min(100, atoi(atoms[0]));
			fontSize = std::max(10, fontSize);
			bool bold = STDHelpers::IsTrue(atoms[1]);
			bool folded = STDHelpers::IsTrue(atoms[2]);
			auto textColor = STDHelpers::HexStringToColorRefRGB(atoms[3]);
			auto bgColor = STDHelpers::HexStringToColorRefRGB(atoms[4]);

			FString text = atoms[5];
			for (int i = 6; i < atoms.size() - 1; i++)
			{
				text += ",";
				text += atoms[i];
			}
			text.Replace("\\n", "\n");
			auto result = STDHelpers::StringToWString(text);

			if (folded)
			{
				int count = 3;
				if (count < result.length() - 1)
				{
					if (IS_HIGH_SURROGATE(result[count - 1]) && IS_LOW_SURROGATE(result[count])) {
						count--;
					}
					result = result.substr(0, count);
					wchar_t toRemove = L'\n';
					result.erase(std::remove(result.begin(), result.end(), toRemove), result.end());
					result += L"...";
				}
				if (fontSize > 18)
					fontSize = 18;
			}
			CIsoViewExt::BlitText(result, textColor, bgColor,
				pThis, lpDesc->lpSurface, window, boundary, x, y, fontSize, 128, folded ? false : bold);
		}
	}

	if (CIsoViewExt::PasteShowOutline && CIsoView::CurrentCommand->Command == 21 && !CopyPaste::PastedCoords.empty() && !CopyPaste::CopyWholeMap)
	{
		CIsoViewExt::DrawMultiMapCoordBorders(lpDesc, CopyPaste::PastedCoords, ExtConfigs::CopySelectionBound_Color);
	}
	// line tool
	auto& command = pThis->LastAltCommand;
	if ((GetKeyState(VK_MENU) & 0x8000) && command.isSame())
	{
		auto point = pThis->GetCurrentMapCoord(pThis->MouseCurrentPosition);
		auto mapCoords = pThis->GetLinePoints({ command.X, command.Y }, { point.X,point.Y });
		CIsoViewExt::DrawMultiMapCoordBorders(lpDesc, mapCoords, ExtConfigs::CursorSelectionBound_Color);
	}

	if (CIsoViewExt::DrawBounds)
	{
		auto& map = CINI::CurrentDocument();
		auto size = STDHelpers::SplitString(map.GetString("Map", "Size", "0,0,0,0"));
		auto lSize = STDHelpers::SplitString(map.GetString("Map", "LocalSize", "0,0,0,0"));

		int mapwidth = atoi(size[2]);
		int mapheight = atoi(size[3]);

		int mpL = atoi(lSize[0]);
		int mpT = atoi(lSize[1]);
		int mpW = atoi(lSize[2]);
		int mpH = atoi(lSize[3]);

		int y1 = mpT + mpL - 2 + 3;
		int x1 = mapwidth + mpT - mpL - 3 + 3;


		int y2 = mpT + mpL + mpW - 2 + 3;
		int x2 = mapwidth - mpL - mpW + mpT - 3 + 3;

		CIsoView::MapCoord2ScreenCoord_Flat(x1, y1);
		int drawX1 = x1 - DrawOffsetX;
		int drawY1 = y1 - DrawOffsetY;

		CIsoView::MapCoord2ScreenCoord_Flat(x2, y2);
		int drawX2 = x2 - DrawOffsetX;
		int drawY2 = y2 - DrawOffsetY;

		pThis->DrawLine(drawX1, drawY1 - 30, drawX2, drawY2 - 30, RGB(0, 0, 255), false, false, lpDesc);
	}

	pThis->lpDDBackBufferSurface->ReleaseDC(hDC);

	return 0x474DB3;
}

DEFINE_HOOK(474DDF, CIsoView_Draw_SkipBounds, 5)
{
	return CIsoViewExt::DrawBounds ? 0 : 0x4750B0;
}

DEFINE_HOOK(474FE0, CIsoView_Draw_SkipMoneyOnMap, 5)
{
	return 0x4750B0;
}

DEFINE_HOOK(475187, CIsoView_Draw_End, 7)
{
	CIsoView::GetInstance()->ViewPosition = ViewPosition;
	return 0;
}
