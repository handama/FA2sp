#include "Body.h"
#include "../../FA2sp.h"

#include <Drawing.h>
#include <CINI.h>
#include <CMapData.h>

#include "../../Miscs/MultiSelection.h"
#include "../../Helpers/STDHelpers.h"

#include "../CLoading/Body.h"
#include "../CMapData/Body.h"

#include "../../Source/CIsoView.h"
#include "../../Helpers/Translations.h"
#include "../CFinalSunDlg/Body.h"
#include <Miscs/Miscs.h>
#include "../../ExtraWindow/CTerrainGenerator/CTerrainGenerator.h"
#include "../../Miscs/Palettes.h"
#include <CShpFile.h>
#include <CMixFile.h>
#include "../../Miscs/TheaterInfo.h"

namespace CIsoViewDrawTemp
{
	int BuildingIndex;
}

/*
* FinalAlert 2 coordinate system just reversed the game's one
* Which means game's (X, Y) is (Y, X) in FinalAlert 2.
* Therefore we just replace the display function here but
* keep use FinalAlert's coordinate system in our codes.
*/
DEFINE_HOOK(45AEFF, CIsoView_OnMouseMove_UpdateCoordinateYXToXY, B)
{
	GET_STACK(int, nPointX, 0x30);
	GET_STACK(int, nPointY, 0x38);
	REF_STACK(ppmfc::CString, lpBuffer, STACK_OFFS(0x3D540, 0x3D4FC));

	lpBuffer.Format("%d / %d - ", nPointX, nPointY);

	R->EBX(nPointX);
	R->EDI(nPointY);

	return 0x45AF76;
}

DEFINE_HOOK(46CB96, CIsoView_DrawMouseAttachedStuff_OverlayAutoConnectionFix, 5)
{
	GET_STACK(CIsoView*, pThis, STACK_OFFS(0x94, 0x84));

	pThis->AutoConnectOverlayAt(R->EBP(), R->EBX());

	return 0x46CC86;
}

DEFINE_HOOK(469A69, CIsoView_CalculateOverlayConnection_OverlayAutoConnectionFix, 8)
{
	GET(unsigned char, nOverlayIndex, ESI);
	GET(bool, bConnectAsWall, ECX);

	enum { CanConnect = 0x469A71, NotAWall = 0x469B07 };

	if (bConnectAsWall)
		return CanConnect;

	if (nOverlayIndex >= 0 && nOverlayIndex < 255)
	{
		char lpKey[4];
		_itoa(nOverlayIndex, lpKey, 10);
		auto pRegName = CINI::Rules->GetString("OverlayTypes", lpKey, "");
		bool bWall = CINI::Rules->GetBool(pRegName, "Wall", false);
		if (bWall)
			return CanConnect;
	}

	return NotAWall;
}

DEFINE_HOOK(459F4F, CIsoView_Draw_CopySelectionBoundColor, 6)
{
	R->Stack<COLORREF>(0x0, ExtConfigs::CopySelectionBound_Color);
	return 0;
}

DEFINE_HOOK(470194, CIsoView_Draw_LayerVisible_Overlay, 8)
{
	return CIsoViewExt::DrawOverlays ? 0 : 0x470772;
}

DEFINE_HOOK(46BDFA, CIsoView_DrawMouseAttachedStuff_Structure, 5)
{
	GET_STACK(const int, X, STACK_OFFS(0x94, -0x4));
	GET_STACK(const int, Y, STACK_OFFS(0x94, -0x8));
	
	const int nMapCoord = CMapData::Instance->GetCoordIndex(X, Y);
	const auto& cell = CMapData::Instance->CellDatas[nMapCoord];
	if (cell.Structure < 0)
		CMapData::Instance->SetBuildingData(nullptr, CIsoView::CurrentCommand->ObjectID, CIsoView::CurrentHouse(), nMapCoord, "");

	return 0x46BF98;
}

DEFINE_HOOK(470502, CIsoView_Draw_OverlayOffset, 5)
{
	REF_STACK(const CellData, cell, STACK_OFFS(0xD18, 0xC60));
	GET(int, nOffset, EAX);

	const int nOverlay = cell.Overlay;
	const unsigned char nOverlayData = cell.OverlayData;

	if (nOverlay == 0xA7)
		nOffset -= 45;
	else if (
		nOverlay != 0x18 && nOverlay != 0x19 && // BRIDGE1, BRIDGE2
		nOverlay != 0x3B && nOverlay != 0x3C && // RAILBRDG1, RAILBRDG2
		nOverlay != 0xED && nOverlay != 0xEE // BRIDGEB1, BRIDGEB2
		)
	{
		if (nOverlay >= 0x27 && nOverlay <= 0x36) // Tracks
			nOffset += 15;
		else if (nOverlay >= 0x4A && nOverlay <= 0x65) // LOBRDG 1-28
			nOffset += 15;
		else if (nOverlay >= 0xCD && nOverlay <= 0xEC) // LOBRDGB 1-4
			nOffset += 15;
		else if (nOverlay < CMapDataExt::OverlayTypeDatas.size())
		{
			if (CMapDataExt::OverlayTypeDatas[nOverlay].Rock 
				//|| CMapDataExt::OverlayTypeDatas[nOverlay].TerrainRock // for compatibility of blockages
				|| CMapDataExt::OverlayTypeDatas[nOverlay].RailRoad)
				nOffset += 15;
		}
	}
	else
	{
		if (nOverlayData >= 0x9 && nOverlayData <= 0x11)
			nOffset -= 16;
		else
			nOffset -= 1;
	}

	R->EAX(nOffset);

	return 0x470574;
}

DEFINE_HOOK(457E9D, CIsoView_OnMouseMove_AutoLAT, 6)
{
    CMapDataExt::SmoothAll();
    return 0x459AA8;
}

DEFINE_HOOK(46242B, CIsoView_OnLBButtonDown_AutoLAT, 6)
{
    CMapDataExt::SmoothAll();
    return 0x463F19;
}

DEFINE_HOOK(45EC07, CIsoView_OnCommand_Skip_wParamCheck, 9)
{
	return 0x45EC10;
}

DEFINE_HOOK(45CDB8, CIsoView_OnMouseMove_SkipOverlayUndoRedo1, 6)
{
	if (!ExtConfigs::UndoRedo_HoldPlaceOverlay)
		return 0;
	return 0x45CDCC;
}

DEFINE_HOOK(45CD4F, CIsoView_OnMouseMove_SkipOverlayUndoRedo2, 6)
{
	if (!ExtConfigs::UndoRedo_HoldPlaceOverlay)
		return 0;
	return 0x45CD6D;
}

DEFINE_HOOK(46684D, CIsoView_OnLButtonDown_OverlayUndoRedo, 6)
{
	if (!ExtConfigs::UndoRedo_HoldPlaceOverlay)
		return 0;

	if (CIsoView::CurrentCommand->Command == 0x1 && CIsoView::CurrentCommand->Type == 6)
	{
		CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);
	}

	return 0;
}

bool IsPlacingTiles = false;
DEFINE_HOOK(46855D, CIsoView_OnLButtonUp_OverlayUndoRedo, 5)
{
	IsPlacingTiles = false;

	if (!ExtConfigs::UndoRedo_HoldPlaceOverlay)
		return 0;

	return 0;
}

DEFINE_HOOK(45B545, CIsoView_OnMouseMove_SkipPlaceTileUndoRedo_Notify, 7)
{
	if (!IsPlacingTiles || !ExtConfigs::UndoRedo_ShiftPlaceTile)
		CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);
	IsPlacingTiles = true;

	return 0;
}

DEFINE_HOOK(461A37, CIsoView_OnLButtonDown_SkipPlaceTileUndoRedo1, 7)
{
	if (!IsPlacingTiles || !ExtConfigs::UndoRedo_ShiftPlaceTile)
		return 0;
	return 0x461A5B;
}

DEFINE_HOOK(46D620, CIsoView_FillArea, 9)
{
	GET_STACK(int, dwX, 0x4);
	GET_STACK(int, dwY, 0x8);
	GET_STACK(int, dwID, 0xC);
	GET_STACK(byte, bSubTile, 0x10);

	CIsoViewExt::FillArea(dwX, dwY, dwID, bSubTile, dwX, dwY);

	return 0x46D808;
}

DEFINE_HOOK(461A37, CIsoView_PlaceTile_FixUndo, 7)
{
	GET(CIsoView*, pIsoView, EBX);
	GET(int, x, EDI);
	GET(int, ym6, ECX);
	int y = ym6 + 6;
	if (CIsoView::CurrentCommand->Type >= CMapDataExt::TileDataCount || CIsoView::CurrentCommand->Type < 0) {
		return 0;
	}
	auto tiledata = CMapDataExt::TileData[CIsoView::CurrentCommand->Type];

	CMapData::Instance->SaveUndoRedoData(true,
		x - tiledata.Height - 4,
		y - tiledata.Width - 4,
		x - tiledata.Height + pIsoView->BrushSizeX * tiledata.Height + 7,
		y - tiledata.Width + pIsoView->BrushSizeY * tiledata.Width + 7);
	return 0x461A5B;
}

DEFINE_HOOK(461C3E, CIsoView_OnLButtonDown_PlaceTile_SkipHide, 6)
{
	if (!ExtConfigs::PlaceTileSkipHide)
		return 0;

	GET_BASE(int, X, -0x64);
	GET(int, Y, EDX);
	const auto cell = CMapData::Instance->TryGetCellAt(X, Y);
	return cell->IsHidden() ? 0x4622F2 : 0;
}

DEFINE_HOOK(457648, CIsoView_OnMouseMove_PlaceTile_SkipHide, B)
{
	if (!ExtConfigs::PlaceTileSkipHide)
		return 0;

	GET_STACK(int, X, STACK_OFFS(0x3D528, 0x3D4E0));
	GET(int, Y, EAX);
	const auto cell = CMapData::Instance->TryGetCellAt(X, Y);
	return cell->IsHidden() ? 0x457D11 : 0;
}

DEFINE_HOOK(469410, CIsoView_ReInitializeDDraw_ReloadFA2SPHESettings, 6)
{
	auto currentLighting = CFinalSunDlgExt::CurrentLighting;
	Logger::Debug("CIsoView::ReInitializeDDraw(): About to call InitializeAllHdmEdition()\n");
	CMapDataExt::InitializeAllHdmEdition(false, false);
	CViewObjectsExt::Redraw_ConnectedTile(nullptr);
	CFinalSunDlgExt::CurrentLighting = currentLighting;

	if (CFinalSunDlgExt::CurrentLighting != 31000)
	{
		CheckMenuRadioItem(*CFinalSunDlg::Instance->GetMenu(), 31000, 31003, CFinalSunDlgExt::CurrentLighting, MF_UNCHECKED);
		LightingSourceTint::CalculateMapLamps();

		CFinalSunDlg::Instance->MyViewFrame.pIsoView->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		auto tmp = CIsoView::CurrentCommand->Command;
		if (CFinalSunDlg::Instance->MyViewFrame.pTileSetBrowserFrame->View.CurrentMode == 1) {
			HWND hParent = CFinalSunDlg::Instance->MyViewFrame.pTileSetBrowserFrame->DialogBar.GetSafeHwnd();
			HWND hTileComboBox = ::GetDlgItem(hParent, 1366);
			::SendMessage(hParent, WM_COMMAND, MAKEWPARAM(1366, CBN_SELCHANGE), (LPARAM)hTileComboBox);
			CIsoView::CurrentCommand->Command = tmp;
		}
	}

	return 0;
}

DEFINE_HOOK(46A362, CIsoView_UpdateStatusBar_BuildingID, 6)
{
	R->ESI(CMapDataExt::StructureIndexMap[R->ESI()]);
	return 0;
}

DEFINE_HOOK(461766, CIsoView_OnLButtonDown_DragObjects, 5)
{
	auto pThis = CIsoView::GetInstance();
	if (CIsoView::CurrentCommand->Command != 0 || pThis->LeftButtonDoubleClick_8C != FALSE || pThis->Drag == TRUE)
	{
		return 0x461964;
	}
	int pos = CMapData::Instance->GetCoordIndex(pThis->StartCell.X, pThis->StartCell.Y);
	auto cell = CMapData::Instance->TryGetCellAt(pos);
	pThis->CurrentCellObjectIndex = -1;
	pThis->CurrentCellObjectType = -1;
	if (CIsoViewExt::DrawInfantries)
	{
		const auto& filter = CIsoViewExt::VisibleInfantries;
		if (!ExtConfigs::InfantrySubCell_Edit)
		{
			pThis->CurrentCellObjectIndex = CMapDataExt::GetInfantryAt(pos);
		}
		else
		{
			pThis->CurrentCellObjectIndex = CIsoViewExt::GetSelectedSubcellInfantryIdx(pThis->StartCell.X, pThis->StartCell.Y);
		}
		if (CIsoViewExt::DrawInfantriesFilter && filter.find(pThis->CurrentCellObjectIndex) == filter.end())
			pThis->CurrentCellObjectIndex = -1;
		pThis->CurrentCellObjectType = 0;
	}
	if (CIsoViewExt::DrawAircrafts && pThis->CurrentCellObjectIndex < 0)
	{
		pThis->CurrentCellObjectIndex = cell->Aircraft;
		const auto& filter = CIsoViewExt::VisibleAircrafts;
		if (CIsoViewExt::DrawAircraftsFilter && filter.find(pThis->CurrentCellObjectIndex) == filter.end())
			pThis->CurrentCellObjectIndex = -1;
		pThis->CurrentCellObjectType = 2;
	}
	if (CIsoViewExt::DrawUnits && pThis->CurrentCellObjectIndex < 0)
	{
		pThis->CurrentCellObjectIndex = cell->Unit;
		const auto& filter = CIsoViewExt::VisibleUnits;
		if (CIsoViewExt::DrawUnitsFilter && filter.find(pThis->CurrentCellObjectIndex) == filter.end())
			pThis->CurrentCellObjectIndex = -1;
		pThis->CurrentCellObjectType = 3;
	}
	if (CIsoViewExt::DrawStructures && pThis->CurrentCellObjectIndex < 0)
	{
		pThis->CurrentCellObjectIndex = cell->Structure;
		pThis->CurrentCellObjectType = 1;
		if (cell->Structure < CMapDataExt::StructureIndexMap.size())
		{
			auto StrINIIndex = CMapDataExt::StructureIndexMap[cell->Structure];
			if (StrINIIndex != -1)
			{
				const auto& filter = CIsoViewExt::VisibleStructures;
				if (CIsoViewExt::DrawStructuresFilter && filter.find(StrINIIndex) == filter.end())
					pThis->CurrentCellObjectIndex = -1;
				else
				{
					const auto& objRender = CMapDataExt::BuildingRenderDatasFix[StrINIIndex];
					pThis->StartCell.X = objRender.X;
					pThis->StartCell.Y = objRender.Y;
				}
			}
			else
			{
				pThis->CurrentCellObjectIndex = -1;
			}
		}
		else
		{
			pThis->CurrentCellObjectIndex = -1;
		}
	}
	if (CIsoViewExt::DrawTerrains && pThis->CurrentCellObjectIndex < 0)
	{
		pThis->CurrentCellObjectIndex = cell->Terrain;
		pThis->CurrentCellObjectType = 4;
	}
	if (CIsoViewExt::DrawWaypoints && pThis->CurrentCellObjectIndex < 0)
	{
		pThis->CurrentCellObjectIndex = cell->Waypoint;
		pThis->CurrentCellObjectType = 6;
	}
	if (CIsoViewExt::DrawCelltags && pThis->CurrentCellObjectIndex < 0)
	{
		pThis->CurrentCellObjectIndex = cell->CellTag;
		pThis->CurrentCellObjectType = 5;
	}
	if (CIsoViewExt::DrawBasenodes && pThis->CurrentCellObjectIndex < 0)
	{
		if (cell->BaseNode.BasenodeID > -1)
		{
			auto& cellExt = CMapDataExt::CellDataExts[pos];
			const auto& node = cellExt.BaseNodes[0];
			pThis->CurrentCellObjectIndex = cell->BaseNode.BasenodeID;
			CIsoViewExt::CurrentCellObjectHouse = cell->BaseNode.House;
			pThis->StartCell.X = node.X;
			pThis->StartCell.Y = node.Y;
			pThis->CurrentCellObjectType = 8;
		}
	}
	if (CIsoViewExt::DrawSmudges && pThis->CurrentCellObjectIndex < 0)
	{
		pThis->CurrentCellObjectIndex = cell->Smudge;
		pThis->CurrentCellObjectType = 9;
	}
	if (CIsoViewExt::DrawAnnotations && pThis->CurrentCellObjectIndex < 0)
	{
		if (CMapDataExt::HasAnnotation(pos))
		{
			pThis->CurrentCellObjectIndex = pos;
			pThis->CurrentCellObjectType = 7;
		}
	}
	if (pThis->CurrentCellObjectIndex < 0)
	{
		return 0x461964;
	}
	pThis->Drag = TRUE;
	pThis->Moved = FALSE;
	return 0x46686A;
}

static UINT nLButtonUpFlags = 0;
DEFINE_HOOK(466970, CIsoView_OnLButtonUp_GetnFlags, 6)
{
	nLButtonUpFlags = R->Stack<UINT>(0x4);
	return 0;
}

DEFINE_HOOK(466DDE, CIsoView_OnLButtonUp_DragOthers, 7)
{
	auto isoView = CIsoView::GetInstance();
	auto& m_id = isoView->CurrentCellObjectIndex;
	auto& m_type = isoView->CurrentCellObjectType;

	//annotation
	if (m_type == 7)
	{
		CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Annotation);
		int X = R->EBX();
		int	Y = R->EDI();

		int oldX = CMapData::Instance->GetXFromCoordIndex(m_id);
		int oldY = CMapData::Instance->GetYFromCoordIndex(m_id);
		ppmfc::CString key;
		key.Format("%d", oldX * 1000 + oldY);
		if (CINI::CurrentDocument->KeyExists("Annotations", key))
		{
			auto value = CINI::CurrentDocument->GetString("Annotations", key);
			if (nLButtonUpFlags != MK_SHIFT)
			{
				auto& cellExt = CMapDataExt::CellDataExts[CMapData::Instance->GetCoordIndex(oldX, oldY)];
				cellExt.HasAnnotation = false;
				CINI::CurrentDocument->DeleteKey("Annotations", key);
			}
			key.Format("%d", X * 1000 + Y);
			CINI::CurrentDocument->WriteString("Annotations", key, value);
			auto& cellExt = CMapDataExt::CellDataExts[CMapData::Instance->GetCoordIndex(X, Y)];
			cellExt.HasAnnotation = true;
		}
		m_id = -1;
		m_type = -1;
	}
	//base nodes
	if (m_type == 8)
	{
		CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Basenode);
		int X = R->EBX();
		int	Y = R->EDI();
		char key[10];
		sprintf(key, "%03d", m_id);
		if (CINI::CurrentDocument->KeyExists(CIsoViewExt::CurrentCellObjectHouse, key))
		{
			auto atoms = STDHelpers::SplitString(CINI::CurrentDocument->GetString(CIsoViewExt::CurrentCellObjectHouse, key), 2);
			ppmfc::CString value;
			value.Format("%s,%d,%d", atoms[0], Y, X);
			if (nLButtonUpFlags == MK_SHIFT)
			{
				int nodeCount = CINI::CurrentDocument->GetInteger(CIsoViewExt::CurrentCellObjectHouse, "NodeCount");
				if (nodeCount < 1000)
				{
					for (int i = 0; i < 1000; ++i)
					{
						sprintf(key, "%03d", i);
						if (!CINI::CurrentDocument->KeyExists(CIsoViewExt::CurrentCellObjectHouse, key))
						{
							break;
						}
					}
					char count[10];
					sprintf(count, "%d", nodeCount + 1);
					CINI::CurrentDocument->WriteString(CIsoViewExt::CurrentCellObjectHouse, "NodeCount", count);
				}
			}
			CINI::CurrentDocument->WriteString(CIsoViewExt::CurrentCellObjectHouse, key, value);
			CMapData::Instance->UpdateFieldBasenodeData(false);
		}
		m_id = -1;
		m_type = -1;
	}
	//smudges
	if (m_type == 9)
	{
		CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Smudge);
		int X = R->EBX();
		int	Y = R->EDI();
		auto smudge = CMapData::Instance->SmudgeDatas[m_id];
		smudge.X = Y;
		smudge.Y = X;
		if (nLButtonUpFlags != MK_SHIFT)
			CMapData::Instance->DeleteSmudgeData(m_id);
		CMapData::Instance->SetSmudgeData(&smudge);
		m_id = -1;
		m_type = -1;
	}
	return 0;
}

DEFINE_HOOK(466E00, CIsoView_OnLButtonUp_DragFacing, 7)
{
	if (nLButtonUpFlags == MK_CONTROL)
	{
		auto Map = &CMapData::Instance();
		auto isoView = CIsoView::GetInstance();
		auto m_id = isoView->CurrentCellObjectIndex;
		auto m_type = isoView->CurrentCellObjectType;

		int X = R->EBX();
		int	Y = R->EDI();
		MapCoord newMapCoord = { X,Y };

		//order: inf unit air str
		if (m_type == 0)
		{
			CInfantryData infantry;
			CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Infantry);
			Map->GetInfantryData(m_id, infantry);
			auto oldMapCoord = MapCoord{ atoi(infantry.X), atoi(infantry.Y) };
			infantry.Facing = CMapDataExt::GetFacing(oldMapCoord, newMapCoord, infantry.Facing, ExtConfigs::ExtFacings_Drag ? 32 : 8);
			Map->DeleteInfantryData(m_id);
			Map->SetInfantryData(&infantry, NULL, NULL, 0, -1);
		}
		else if (m_type == 3)
		{
			CUnitData unit;
			CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Unit);
			Map->GetUnitData(m_id, unit);
			auto oldMapCoord = MapCoord{ atoi(unit.X), atoi(unit.Y) };
			unit.Facing = CMapDataExt::GetFacing(oldMapCoord, newMapCoord, unit.Facing, ExtConfigs::ExtFacings_Drag ? 32 : 8);
			Map->DeleteUnitData(m_id);
			Map->SetUnitData(&unit, NULL, NULL, 0, "");
		}
		else if (m_type == 2)
		{
			CAircraftData aircraft;
			CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Aircraft);
			Map->GetAircraftData(m_id, aircraft);
			auto oldMapCoord = MapCoord{ atoi(aircraft.X), atoi(aircraft.Y) };
			aircraft.Facing = CMapDataExt::GetFacing(oldMapCoord, newMapCoord, aircraft.Facing, ExtConfigs::ExtFacings_Drag ? 32 : 8);
			Map->DeleteAircraftData(m_id);
			Map->SetAircraftData(&aircraft, NULL, NULL, 0, "");
		}
		else if (m_type == 1)
		{
			CBuildingData structure;
			CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Building);
			Map->GetBuildingData(m_id, structure);
			auto oldMapCoord = MapCoord{ atoi(structure.X), atoi(structure.Y) };
			structure.Facing = CMapDataExt::GetFacing(oldMapCoord, newMapCoord, structure.Facing, ExtConfigs::ExtFacings_Drag ? 32 : 8);
			Map->DeleteBuildingData(m_id);
			Map->SetBuildingData(&structure, NULL, NULL, 0, "");
		}

		return 0x467682;
	}

	return 0;
}

DEFINE_HOOK(4576C6, CIsoView_OnMouseMove_NoRndForBridge, 6)
{
	GET_STACK(DWORD, dwID, STACK_OFFS(0x3D528, 0x3D450));

	if (dwID < CMapDataExt::TileDataCount)
		if (CMapDataExt::TileData[dwID].TileSet == CMapDataExt::WoodBridgeSet)
			return 0x4577F7;

	return 0x4576CC;
}

DEFINE_HOOK(461CDB, CIsoView_OnLButtonDown_NoRndForBridge, 6)
{
	GET(DWORD, dwID6, EDI);
	int dwID = dwID6 >> 6;
	if (dwID < CMapDataExt::TileDataCount)
		if (CMapDataExt::TileData[dwID].TileSet == CMapDataExt::WoodBridgeSet)
			return 0x461DEE;

	return 0x461CE1;
}

DEFINE_HOOK(45EBE0, CIsoView_OnCommand_ConfirmTube, 7)
{
	if (CIsoViewExt::IsPressingTube)
	{
		if (CIsoViewExt::TubeNodes.front() == CIsoViewExt::TubeNodes.back())
			return 0;

		((CIsoViewExt*)CIsoView::GetInstance())->ConfirmTube(CIsoView::CurrentCommand->Type == 0);

		CIsoViewExt::IsPressingTube = false;
		::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		return 0x45EDB6;
	}
	return 0;
}

DEFINE_HOOK(45C850, CIsoView_OnMouseMove_Delete, 5)
{
	auto pIsoView = (CIsoViewExt*)CIsoView::GetInstance();
	auto point = pIsoView->GetCurrentMapCoord(pIsoView->MouseCurrentPosition);

	auto makeOrAppendRecord = [](int recordType)
		{
			if (!ObjectRecord::ObjectRecord_HoldingPtr)
				ObjectRecord::ObjectRecord_HoldingPtr = CMapDataExt::MakeObjectRecord(recordType, true);
			else
				ObjectRecord::ObjectRecord_HoldingPtr->appendRecord(recordType);
		};

	for (int gx = point.X - pIsoView->BrushSizeX / 2; gx <= point.X + pIsoView->BrushSizeX / 2; gx++)
	{
		for (int gy = point.Y - pIsoView->BrushSizeY / 2; gy <= point.Y + pIsoView->BrushSizeY / 2; gy++)
		{
			if (!CMapDataExt::IsCoordInFullMap(gx, gy))
				continue;

			int nIndex = CMapData::Instance->GetCoordIndex(gx, gy);
			const auto& CellData = CMapData::Instance->CellDatas[nIndex];

			if (CellData.Structure != -1)
			{
				makeOrAppendRecord(ObjectRecord::RecordType::Building);
				CMapData::Instance->DeleteBuildingData(CellData.Structure);
			}

			if (ExtConfigs::InfantrySubCell_Edit &&
				pIsoView->BrushSizeX == 1 && pIsoView->BrushSizeY == 1)
			{
				int idx = CIsoViewExt::GetSelectedSubcellInfantryIdx(point.X, point.Y);
				if (idx != -1)
				{
					makeOrAppendRecord(ObjectRecord::RecordType::Infantry);
					CMapData::Instance->DeleteInfantryData(idx);
				}
			}
			else
			{
				if (CellData.Infantry[0] != -1)
				{
					makeOrAppendRecord(ObjectRecord::RecordType::Infantry);
					CMapData::Instance->DeleteInfantryData(CellData.Infantry[0]);
				}
				if (CellData.Infantry[1] != -1)
				{
					makeOrAppendRecord(ObjectRecord::RecordType::Infantry);
					CMapData::Instance->DeleteInfantryData(CellData.Infantry[1]);
				}
				if (CellData.Infantry[2] != -1)
				{
					makeOrAppendRecord(ObjectRecord::RecordType::Infantry);
					CMapData::Instance->DeleteInfantryData(CellData.Infantry[2]);
				}
			}
			if (CellData.Unit != -1)
			{
				makeOrAppendRecord(ObjectRecord::RecordType::Unit);
				CMapData::Instance->DeleteUnitData(CellData.Unit);
			}

			if (CellData.Aircraft != -1)
			{
				makeOrAppendRecord(ObjectRecord::RecordType::Aircraft);
				CMapData::Instance->DeleteAircraftData(CellData.Aircraft);
			}
			
			if (CellData.Terrain != -1)
			{
				makeOrAppendRecord(ObjectRecord::RecordType::Terrain);
				CMapData::Instance->DeleteTerrainData(CellData.Terrain);
			}
			
			if (CellData.Smudge != -1)
			{
				makeOrAppendRecord(ObjectRecord::RecordType::Smudge);
				CMapData::Instance->DeleteSmudgeData(CellData.Smudge);
			}
		}
	}
	::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);

	return 0x45C961;
}

DEFINE_HOOK(45EC1A, CIsoView_OnCommand_HandleProperty, A)
{
	auto pIsoView = CIsoView::GetInstance();
	auto& coord = pIsoView->StartCell;
	auto cell = CMapData::Instance->TryGetCellAt(coord.X, coord.Y);
	int pos = CMapData::Instance->GetCoordIndex(coord.X, coord.Y);

	int index = -1;
	int type = -1;
	if (CIsoViewExt::DrawInfantries)
	{
		const auto& filter = CIsoViewExt::VisibleInfantries;
		if (!ExtConfigs::InfantrySubCell_Edit)
		{
			index = CMapDataExt::GetInfantryAt(pos);
		}
		else
		{
			index = CIsoViewExt::GetSelectedSubcellInfantryIdx(pIsoView->StartCell.X, pIsoView->StartCell.Y);
		}
		if (CIsoViewExt::DrawInfantriesFilter && filter.find(index) == filter.end())
			index = -1;
		type = 0;
	}
	if (CIsoViewExt::DrawAircrafts && index < 0)
	{
		index = cell->Aircraft;
		const auto& filter = CIsoViewExt::VisibleAircrafts;
		if (CIsoViewExt::DrawAircraftsFilter && filter.find(index) == filter.end())
			index = -1;
		type = 2;
	}
	if (CIsoViewExt::DrawUnits && index < 0)
	{
		index = cell->Unit;
		const auto& filter = CIsoViewExt::VisibleUnits;
		if (CIsoViewExt::DrawUnitsFilter && filter.find(index) == filter.end())
			index = -1;
		type = 3;
	}
	if (CIsoViewExt::DrawStructures && index < 0)
	{
		index = cell->Structure;
		type = 1;
		if (cell->Structure < CMapDataExt::StructureIndexMap.size())
		{
			auto StrINIIndex = CMapDataExt::StructureIndexMap[cell->Structure];
			if (StrINIIndex != -1)
			{
				const auto& filter = CIsoViewExt::VisibleStructures;
				if (CIsoViewExt::DrawStructuresFilter && filter.find(StrINIIndex) == filter.end())
					index = -1;
				else
				{
					const auto& objRender = CMapDataExt::BuildingRenderDatasFix[StrINIIndex];
					pIsoView->StartCell.X = objRender.X;
					pIsoView->StartCell.Y = objRender.Y;
				}
			}
			else
			{
				index = -1;
			}
		}
		else
		{
			index = -1;
		}
	}
	if (index > -1 && type > -1)
	{
		if (type == 0 && !ExtConfigs::InfantrySubCell_Edit)
		{
			for (int i = 0; i < 3; ++i)
			{
				if (cell->Infantry[i] > -1)
					pIsoView->HandleProperties(cell->Infantry[i], 0);
			}
		}
		else
			pIsoView->HandleProperties(index, type);
	}

	if (pos < CMapData::Instance->CellDataCount)
	{
		ppmfc::CString key;
		key.Format("%d", coord.X * 1000 + coord.Y);
		if (CINI::CurrentDocument->KeyExists("Annotations", key))
		{
			auto atoms = STDHelpers::SplitString(CINI::CurrentDocument->GetString("Annotations", key), 6);
			ppmfc::CString value;
			bool folded = STDHelpers::IsTrue(atoms[2]);
			for (int i = 0; i < atoms.size(); ++i)
			{
				if (i != 0)
					value += ",";
				if (i != 2)
					value += atoms[i];
				else
				{
					if (folded)
						value += "no";
					else
						value += "yes";
				}
			}
			CINI::CurrentDocument->WriteString("Annotations", key, value);
			::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		}
	}

	return 0x45ED9E;
}

DEFINE_HOOK(46CB77, CIsoView_DrawMouseAttachedStuff_Overlay_1, 6)
{
	GET(int, pos, ESI);
	CMapDataExt::GetExtension()->SetNewOverlayAt(pos, CIsoView::CurrentCommand->Overlay);
	return 0x46CB89;
}

DEFINE_HOOK(46CC03, CIsoView_DrawMouseAttachedStuff_Overlay_2, 5)
{
	GET(int, pos, ESI);
	GET(int, i, EBP);
	GET(int, e, EDI);
	CMapDataExt::GetExtension()->SetNewOverlayAt(pos + i + e * CMapData::Instance->MapWidthPlusHeight, CIsoView::CurrentCommand->Overlay);
	return 0x46CC21;
}

DEFINE_HOOK(46CC5C, CIsoView_DrawMouseAttachedStuff_Overlay_3, 5)
{
	GET(int, pos, ESI);
	CMapDataExt::GetExtension()->SetNewOverlayAt(pos, CIsoView::CurrentCommand->Overlay);
	return 0x46CC86;
}

DEFINE_HOOK(46C38B, CIsoView_DrawMouseAttachedStuff_Ore, 9)
{
	GET(int, param, EAX);
	GET(int, dwPos, ESI);

	constexpr int GEM_DEAFULT = 30;
	constexpr int ORE_DEAFULT = 112;
	constexpr int ORE2_DEAFULT = VINIFERA_BEGIN;
	constexpr int ORE3_DEAFULT = ABOREUS_BEGIN;
	constexpr std::array<int, RIPARIUS_END - RIPARIUS_BEGIN> ORES = [] {
		std::array<int, RIPARIUS_END - RIPARIUS_BEGIN> v{};
		for (int i = 0; i < RIPARIUS_END - RIPARIUS_BEGIN; ++i)
			v[i] = RIPARIUS_BEGIN + i;
		return v;
	}();
	constexpr std::array<int, CRUENTUS_END - CRUENTUS_BEGIN> GEMS = [] {
		std::array<int, CRUENTUS_END - CRUENTUS_BEGIN> v{};
		for (int i = 0; i < CRUENTUS_END - CRUENTUS_BEGIN; ++i)
			v[i] = CRUENTUS_BEGIN + i;
		return v;
	}();
	constexpr std::array<int, VINIFERA_END - VINIFERA_BEGIN> ORE2S = [] {
		std::array<int, VINIFERA_END - VINIFERA_BEGIN> v{};
		for (int i = 0; i < VINIFERA_END - VINIFERA_BEGIN; ++i)
			v[i] = VINIFERA_BEGIN + i;
		return v;
	}();
	constexpr std::array<int, ABOREUS_END - ABOREUS_BEGIN> ORE3S = [] {
		std::array<int, ABOREUS_END - ABOREUS_BEGIN> v{};
		for (int i = 0; i < ABOREUS_END - ABOREUS_BEGIN; ++i)
			v[i] = ABOREUS_BEGIN + i;
		return v;
	}();

	auto pIsoView = (CIsoViewExt*)CIsoView::GetInstance();
	auto pMapData = CMapDataExt::GetExtension();
	int X = CMapData::Instance->GetXFromCoordIndex(dwPos);
	int Y = CMapData::Instance->GetYFromCoordIndex(dwPos);
	// ore
	if (param == 2 || param == 3 || param == 6 || param == 7)
	{
		int i, e;
		for (i = 0; i < pIsoView->BrushSizeX; i++)
		{
			for (e = 0; e < pIsoView->BrushSizeY; e++)
			{
				if (!pMapData->IsCoordInMap(i + X, e + Y))
					continue;

				int curPos = dwPos + i + e * pMapData->MapWidthPlusHeight;
				int curground = pMapData->GetSafeTileIndex(pMapData->GetCellAt(curPos)->TileIndex);

				if (pMapData->TileData[curground].AllowTiberium)
				{
					int targetOre;
					if (param == 2)
					{
						targetOre = ExtConfigs::ObjectBrowser_Ore_RandomPlacement ?
							STDHelpers::RandomSelectArray(ORES) : ORE_DEAFULT;
					}
					else if (param == 3)
					{
						targetOre = ExtConfigs::ObjectBrowser_Ore_RandomPlacement ?
							STDHelpers::RandomSelectArray(GEMS) : GEM_DEAFULT;
					}
					else if (param == 6)
					{
						targetOre = ExtConfigs::ObjectBrowser_Ore_RandomPlacement ?
							STDHelpers::RandomSelectArray(ORE2S) : ORE2_DEAFULT;
					}
					else if (param == 7)
					{
						targetOre = ExtConfigs::ObjectBrowser_Ore_RandomPlacement ?
							STDHelpers::RandomSelectArray(ORE3S) : ORE3_DEAFULT;
					}
					
					pMapData->SetNewOverlayAt(curPos, targetOre);
					pMapData->SetOverlayDataAt(curPos, 5);
				}
			}
		}
	}

	return 0x46CA82;
}


DEFINE_HOOK(45A081, CIsoView_OnMouseMove_Place_Enter_Overlay, 9)
{
	GET(int, command_param, EAX);
	switch (command_param)
	{
	case 3:
	case 6:
	case 7:
		return 0x45A08A;
	default:
		break;
	}
	return 0x45ACCA;
}

DEFINE_HOOK(45A08A, CIsoView_OnMouseMove_Place, 5)
{
	auto Map = CMapDataExt::GetExtension();
	auto pIsoView = (CIsoViewExt*)CIsoView::GetInstance();
	auto point = pIsoView->GetCurrentMapCoord(pIsoView->MouseCurrentPosition);
	const int& x = point.X;
	const int& y = point.Y;

	constexpr int MapBlockSize = 32;
	constexpr int Padding = 4;
	constexpr int TotalSize = MapBlockSize + Padding;

	std::unique_ptr<CellData[]> oldData = std::make_unique<CellData[]>(TotalSize * TotalSize);
	WORD oldNewOverlay[TotalSize][TotalSize];
	int i, e;

	for (i = -Padding; i < MapBlockSize; i++)
	{
		for (e = -Padding; e < MapBlockSize; e++)
		{
			int ix = i + Padding;
			int ex = e + Padding;

			DWORD dwPos = i + x + (e + y) * Map->MapWidthPlusHeight;
			if (dwPos >= Map->CellDataCount || dwPos < 0)
				continue;

			auto& cur_fieldExt = Map->CellDataExts[dwPos];
			oldData[ix * TotalSize + ex] = *Map->GetCellAt(dwPos);
			oldNewOverlay[ix][ex] = cur_fieldExt.NewOverlay;
		}
	}
	int money = Map->MoneyCount;

	if (CViewObjectsExt::PlacingWall >= 0)
	{
		int overlay = CViewObjectsExt::PlacingWall / 5;
		int damageLevel = CViewObjectsExt::PlacingWall % 5 - 1;
		if (damageLevel == 3)
			damageLevel = -2;
		if (damageLevel == -1)
			damageLevel = 0;

		for (int i = 0; i < pIsoView->BrushSizeX; i++)
			for (int j = 0; j < pIsoView->BrushSizeY; j++)
				CMapDataExt::PlaceWallAt(Map->GetCoordIndex(x + i, y + j), overlay, damageLevel);
	}
	else
	{
		pIsoView->DrawMouseAttachedStuff(x, y);
	}
	::RedrawWindow(pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);

	auto RestoreCellRegion = [&](int i, int e)
	{
		int ix = i + Padding;
		int ex = e + Padding;

		DWORD dwPos = i + x + (e + y) * Map->MapWidthPlusHeight;
		if (dwPos >= Map->CellDataCount || dwPos < 0)
			return;

		auto cur_field = Map->GetCellAt(dwPos);
		auto& cur_fieldExt = Map->CellDataExts[dwPos];

		if (cur_field->Aircraft != oldData[ix * TotalSize + ex].Aircraft)
			Map->DeleteAircraftData(cur_field->Aircraft);

		for (int z = 0; z < 3; z++)
			if (cur_field->Infantry[z] != oldData[ix * TotalSize + ex].Infantry[z])
				Map->DeleteInfantryData(cur_field->Infantry[z]);

		if (cur_field->Structure != oldData[ix * TotalSize + ex].Structure)
			Map->DeleteBuildingData(cur_field->Structure);

		if (cur_field->Terrain != oldData[ix * TotalSize + ex].Terrain)
			Map->DeleteTerrainData(cur_field->Terrain);

		if (cur_field->Smudge != oldData[ix * TotalSize + ex].Smudge)
			Map->DeleteSmudgeData(cur_field->Smudge);

		if (cur_field->Unit != oldData[ix * TotalSize + ex].Unit)
			Map->DeleteUnitData(cur_field->Unit);

		if (cur_fieldExt.NewOverlay != oldNewOverlay[ix][ex])
			Map->SetNewOverlayAt(dwPos, oldNewOverlay[ix][ex]);

		if (cur_field->OverlayData != oldData[ix * TotalSize + ex].OverlayData)
			Map->SetOverlayDataAt(dwPos, oldData[ix * TotalSize + ex].OverlayData);

		Map->DeleteTiberium(std::min(cur_fieldExt.NewOverlay, (word)0xFF), cur_field->OverlayData);
		Map->AssignCellData(Map->CellDatas[dwPos], oldData[ix * TotalSize + ex]);
		cur_fieldExt.NewOverlay = oldNewOverlay[ix][ex];
		Map->AddTiberium(std::min(cur_fieldExt.NewOverlay, (word)0xFF), cur_field->OverlayData);
	};

	RestoreCellRegion(0, 0);
	for (i = -Padding; i < MapBlockSize; i++)
	{
		for (e = -Padding; e < MapBlockSize; e++)
		{
			if (i == 0 && e == 0)
				continue;

			RestoreCellRegion(i, e);
		}
	}
	Map->MoneyCount = money;

	return 0x45AEF6;
}

DEFINE_HOOK(457573, CIsoView_OnMouseMove_CustomWater, 5)
{
	R->Stack(STACK_OFFS(0x3D528, 0x3D450), STDHelpers::RandomSelectInt(TheaterInfo::CurrentBigWaters));
	return 0x4575A4;
}

DEFINE_HOOK(461B8E, CIsoView_PlaceTile_CustomWater, 5)
{
	R->EAX(STDHelpers::RandomSelectInt(TheaterInfo::CurrentBigWaters));
	return 0x461BB4;
}

DEFINE_HOOK(4C4480, CIsoView_SmoothTiberium, 5)
{
	GET_STACK(int, dwPos, 0x4);

	static int _adj[9] = { 0,1,3,4,6,7,8,10,11 };
	auto Map = CMapDataExt::GetExtension();
	auto pIsoView = (CIsoViewExt*)CIsoView::GetInstance();

	auto& ovrl = Map->CellDataExts[dwPos].NewOverlay;
	auto& ovrld = Map->CellDatas[dwPos].OverlayData;

	if (!(ovrl >= RIPARIUS_BEGIN && ovrl <= RIPARIUS_END) &&
		!(ovrl >= CRUENTUS_BEGIN && ovrl <= CRUENTUS_END) &&
		!(ovrl >= VINIFERA_BEGIN && ovrl <= VINIFERA_END) &&
		!(ovrl >= ABOREUS_BEGIN && ovrl <= ABOREUS_END))
		return 0x4C45E9;

	Map->DeleteTiberium(std::min(ovrl, (word)0xFF), ovrld);

	int i, e;
	int x, y;
	x = dwPos % Map->MapWidthPlusHeight;
	y = dwPos / Map->MapWidthPlusHeight;
	int count = 0;

	for (i = -1; i < 2; i++)
	{
		for (e = -1; e < 2; e++)
		{
			int xx = x + i;
			int yy = y + e;

			if (xx < 0 || xx >= Map->MapWidthPlusHeight || yy < 0 || yy >= Map->MapWidthPlusHeight) continue;

			int pos = Map->GetCoordIndex(xx, yy);
			auto& ovrl = Map->CellDataExts[pos].NewOverlay;

			if (ovrl >= RIPARIUS_BEGIN && ovrl <= RIPARIUS_END)
			{
				count++;
			}
			if (ovrl >= CRUENTUS_BEGIN && ovrl <= CRUENTUS_END)
			{
				count++;
			}
			if (ovrl >= VINIFERA_BEGIN && ovrl <= VINIFERA_END)
			{
				count++;
			}
			if (ovrl >= ABOREUS_BEGIN && ovrl <= ABOREUS_END)
			{
				count++;
			}
		}
	}
	if (count > 0)
	{
		Map->CellDatas[dwPos].OverlayData = _adj[count - 1];
		Map->OverlayData[y + x * 512] = _adj[count - 1];

		Map->AddTiberium(std::min(ovrl, (word)0xFF), _adj[count - 1]);
	}
	else
	{
		Map->AddTiberium(std::min(ovrl, (word)0xFF), ovrld);
	}
	return 0x4C45E9;
}

static inline bool commandHasBorderRange(int command, int x = 1919, int y = 810)
{
	switch (command)
	{
	case 20:
	case 21:
	case 0x1F:
	case 0x1E:
		return (x == 1919 && y == 810) ? true : CMapDataExt::IsCoordInFullMap(x, y);
	case 0x1B:
	case 0x2:
	case 0:
		if (ExtConfigs::DisplayObjectsOutside && !CIsoView::GetInstance()->Drag)
			return (x == 1919 && y == 810) ? true : CMapDataExt::IsCoordInFullMap(x, y);
		break;
	case 1:
		// delete overlays
		if (ExtConfigs::DisplayObjectsOutside && CIsoView::CurrentCommand->Type == 6 && CIsoView::CurrentCommand->Param == 1)
			return (x == 1919 && y == 810) ? true : CMapDataExt::IsCoordInFullMap(x, y);
		break;
	case 5:
		// delete nodes
		if (ExtConfigs::DisplayObjectsOutside && CIsoView::CurrentCommand->Type == 2)
			return (x == 1919 && y == 810) ? true : CMapDataExt::IsCoordInFullMap(x, y);
		break;
	case 3:
		// delete waypoint
		if (ExtConfigs::DisplayObjectsOutside && CIsoView::CurrentCommand->Type == 1)
			return (x == 1919 && y == 810) ? true : CMapDataExt::IsCoordInFullMap(x, y);
		break;
	case 4:
		// delete celltag
		if (ExtConfigs::DisplayObjectsOutside && CIsoView::CurrentCommand->Type == 1)
			return (x == 1919 && y == 810) ? true : CMapDataExt::IsCoordInFullMap(x, y);
		break;
	case 6:
		// delete tube
		if (ExtConfigs::DisplayObjectsOutside)
			return (x == 1919 && y == 810) ? true : CMapDataExt::IsCoordInFullMap(x, y);
		break;
	case 0x1D:
		// add/delete multi-selection
		if (ExtConfigs::DisplayObjectsOutside && 
			(CIsoView::CurrentCommand->Type == 3 || CIsoView::CurrentCommand->Type == 4 
				|| CIsoView::CurrentCommand->Type == 10 || CIsoView::CurrentCommand->Type == 11))
			return (x == 1919 && y == 810) ? true : CMapDataExt::IsCoordInFullMap(x, y);
		break;
	case 0x21:
		// delete annotation
		if (ExtConfigs::DisplayObjectsOutside && CIsoView::CurrentCommand->Type != 0)
			return (x == 1919 && y == 810) ? true : CMapDataExt::IsCoordInFullMap(x, y);
		break;
	default:
		break;
	}
	return false;
}

#pragma comment(lib, "imm32.lib") 
DEFINE_HOOK(457223, CIsoView_OnMouseMove_MouseRange_1_DisableIME, 9)
{
	auto pIsoView = (CIsoViewExt*)CIsoView::GetInstance();
	HIMC hIMC = ::ImmGetContext(pIsoView->GetSafeHwnd());
	if (hIMC)
	{
		::ImmAssociateContext(pIsoView->GetSafeHwnd(), NULL);
		::ImmReleaseContext(pIsoView->GetSafeHwnd(), hIMC);
	}

	GET(int, command, EDX);
	if (commandHasBorderRange(command))
		return 0x4572B9;
	return 0x457235;
}

DEFINE_HOOK(4572B9, CIsoView_OnMouseMove_MouseRange_2, 5)
{
	GET(int, X, EBX);
	GET(int, Y, ESI);
	if (CMapDataExt::IsCoordInFullMap(X, Y))
		return 0x4572E1;

	return 0x456EB6;
}

DEFINE_HOOK(4615F0, CIsoView_OnLButtonDown_MouseRange, 5)
{
	GET(int, command, ECX);
	GET(int, x, EDI);
	GET(int, y, ESI);
	if (commandHasBorderRange(command, x, y))
		return 0x461651;
	return 0x4615FA;
}

//DEFINE_HOOK(466C8E, CIsoView_OnLButtonUp_MouseRange, 5)
//{
//	GET(int, command, EDX);
//	if (commandHasBorderRange(command))
//		return 0x466CEC;
//	return 0x466C98;
//}

DEFINE_HOOK(459D50, CIsoView_OnMouseMove_CliffBack_Alt_1, 6)
{
	if (TheaterInfo::CurrentInfoHasCliff2 && CIsoView::GetInstance()->KeyboardAMode)
		CIsoViewExt::CliffBackAlt = true;
	return 0;
}

DEFINE_HOOK(459D7B, CIsoView_OnMouseMove_CliffBack_Alt__2, 7)
{
	CIsoViewExt::CliffBackAlt = false;
	return 0;
}

DEFINE_HOOK(459BFA, CIsoView_OnMouseMove_CliffFront_Alt__1, 7)
{
	if (TheaterInfo::CurrentInfoHasCliff2 && CIsoView::GetInstance()->KeyboardAMode)
		CIsoViewExt::CliffBackAlt = true;
	return 0;
}

DEFINE_HOOK(459C28, CIsoView_OnMouseMove_CliffFront_Alt_2, 7)
{
	CIsoViewExt::CliffBackAlt = false;
	return 0;
}

DEFINE_HOOK(41B250, CIsoView_DrawCliff_NewUrban, 7)
{
	if (CIsoViewExt::CliffBackAlt)
		R->Stack(0x14, TRUE);
	return 0;
}

DEFINE_HOOK(45CD6D, CIsoView_OnMouseMove_StatusBar, 8)
{
	if (CIsoView::CurrentCommand->Command == 15) {
		SendMessage(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd, 0x401, 0,
			(LPARAM)Translations::TranslateOrDefault("FlattenGroundMessage",
				"Shift: Steep slope, Ctrl+Shift:  Ignore non-morphable tiles"));
		::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
		::UpdateWindow(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd);
		return 0x45CD82;
	}
	else if (CIsoView::CurrentCommand->Command == 13 || CIsoView::CurrentCommand->Command == 14) {
		SendMessage(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd, 0x401, 0,
			(LPARAM)Translations::TranslateOrDefault("HeightenAndLowerTileMessage",
				"Ctrl: Create slope on the edges"));
		::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
		::UpdateWindow(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd);
		return 0x45CD82;
	}
	else if (TheaterInfo::CurrentInfoHasCliff2 && (CIsoView::CurrentCommand->Command == 18 || CIsoView::CurrentCommand->Command == 19)) {
		SendMessage(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd, 0x401, 0,
			(LPARAM)(Translations::TranslateOrDefault("PressAToSwitchCliff", "Press key 'A' to switch cliff type")));
		::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
		::UpdateWindow(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd);
		return 0x45CD82;
	}
	else if (CIsoView::CurrentCommand->Command == 0x22) {
		SendMessage(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd, 0x401, 0,
			(LPARAM)(Translations::TranslateOrDefault("DrawTunnelMessage",
				"Click to draw the tunnel, double-click to set the endpoint and finish editing. The length of the tunnel cannot exceed 100.")));
		::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
		::UpdateWindow(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd);
		return 0x45CD82;
	}
	else if (CIsoView::CurrentCommand->Command == 0x1E) {
		ppmfc::CString text = "";
		ppmfc::CString buffer;
		for (int i = 0; i < 10; ++i)
		{
			int ctIndex = CIsoView::CurrentCommand->Type;
			auto& info = CViewObjectsExt::TreeView_ConnectedTileMap[ctIndex];
			auto& tileSet = CViewObjectsExt::ConnectedTileSets[info.Index];
			if (tileSet.ToSetPress[i] > -1)
			{
				for (auto& [ctIndex2, info2] : CViewObjectsExt::TreeView_ConnectedTileMap)
				{
					if (info2.Index == tileSet.ToSetPress[i] && info2.Front == info.Front)
					{
						auto& tileSet2 = CViewObjectsExt::ConnectedTileSets[info2.Index];
						buffer.Format(Translations::TranslateOrDefault("PressNumberToSwitchConnectedType", "Press number key %d to switch to %s")
							, i, tileSet2.Name);
						if (tileSet2.WaterCliff)
						{
							buffer += " ";
							buffer += Translations::TranslateOrDefault("SwitchConnectedTypeIsWaterCliff", "(Water)");
						}
						buffer += ", ";
						text += buffer;
					}
				}
			}
		}
		if (text != "")
		{
			text.Delete(text.GetLength() - 2, 2);
			SendMessage(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd, 0x401, 0, (LPARAM)text.m_pchData);
			::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
			::UpdateWindow(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd);
			return 0x45CD82;
		}
	}
	return 0;
}

DEFINE_HOOK(45F261, CIsoView_HandleProperties_Infantry, 7)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Infantry);
	return 0;
}

DEFINE_HOOK(45FA44, CIsoView_HandleProperties_Building, 7)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Building);
	return 0;
}

DEFINE_HOOK(4600B1, CIsoView_HandleProperties_Aircraft, 7)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Aircraft);
	return 0;
}

DEFINE_HOOK(460742, CIsoView_HandleProperties_Unit, 7)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Unit);
	return 0;
}

DEFINE_HOOK(466E50, CIsoView_OnLButtonUp_Drag_Infantry, 5)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Infantry);
	return 0;
}

// building in CMapData/Hooks.cpp
//DEFINE_HOOK(467035, CIsoView_OnLButtonUp_Drag_Building, 5)
//{
//	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Building);
//	return 0;
//}

DEFINE_HOOK(467212, CIsoView_OnLButtonUp_Drag_Aircraft, 5)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Aircraft);
	return 0;
}

DEFINE_HOOK(4673A2, CIsoView_OnLButtonUp_Drag_Unit, 5)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Unit);
	return 0;
}

DEFINE_HOOK(467521, CIsoView_OnLButtonUp_Drag_Terrain, 5)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Terrain);
	return 0;
}

DEFINE_HOOK(46759C, CIsoView_OnLButtonUp_Drag_Celltag, 5)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Celltag);
	return 0;
}

DEFINE_HOOK(467615, CIsoView_OnLButtonUp_Drag_Waypoint, 5)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Waypoint);
	return 0;
}

DEFINE_HOOK(45BFA6, CIsoView_OnMouseMove_Waypoint_Delete, 6)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Waypoint, true);
	return 0;
}

DEFINE_HOOK(45BFE1, CIsoView_OnMouseMove_Waypoint_Add, 8)
{
	GET(int, pos, EAX);

	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Waypoint, true);
	if (!CMapData::Instance->IsMultiOnly())
	{
		CMapData::Instance->SetWaypointData("", pos);
	}
	else
	{
		int i = 8;
		ppmfc::CString key = "8";
		while (CINI::CurrentDocument->KeyExists("Waypoints", key))
		{
			i++;
			key.Format("%d", i);
		}
		CMapData::Instance->SetWaypointData(key, pos);
	}
	return 0x45C1A6;
}

DEFINE_HOOK(523A23, CWaypointID_GetFreeWaypoint, 6)
{
	if (CMapData::Instance->IsMultiOnly())
		R->ECX(8);
	return 0;
}

DEFINE_HOOK(466562, CIsoView_OnLButtonDown_Waypoint_Add, 6)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Waypoint);
	return 0;
}

DEFINE_HOOK(45C0CF, CIsoView_OnMouseMove_Waypoint_AddPlayerLocation, 6)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Waypoint, true);
	auto deleteWaypoint = [](ppmfc::CString key)
		{
			if (auto pSection = CINI::CurrentDocument->GetSection("Waypoints"))
			{
				if (CINI::CurrentDocument->KeyExists("Waypoints", key))
				{
					auto&& value = pSection->GetString(key);
					int x = atoi(value) / 1000;
					int y = atoi(value) % 1000;
					CINI::CurrentDocument->DeleteKey(pSection, key);
					CMapData::Instance->UpdateFieldWaypointData(false);

					if (CMapData::Instance->IsMultiOnly())
					{
						int k, l;
						for (k = -1; k < 2; k++)
							for (l = -1; l < 2; l++)
								CMapData::Instance->UpdateMapPreviewAt(x + k, y + l);
					}
				}
			}
		};


	if (CMapData::Instance->IsMultiOnly())
	{
		GET_STACK(int, waypoint, STACK_OFFS(0x3D528, 0x3D518));
		ppmfc::CString key;
		key.Format("%d", waypoint);
		deleteWaypoint(key);
	}
	else
	{
		deleteWaypoint("98");
		deleteWaypoint("99");
	}
	return 0;
}

DEFINE_HOOK(45C1C7, CIsoView_OnMouseMove_Celltag_Add, 5)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Celltag, true);
	return 0;
}

DEFINE_HOOK(45C19B, CIsoView_OnMouseMove_Celltag_Delete, 6)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Celltag, true);
	return 0;
}

DEFINE_HOOK(46661C, CIsoView_OnLButtonDown_Celltag_Add, 5)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Celltag);
	return 0;
}

DEFINE_HOOK(4667E8, CIsoView_OnLButtonDown_Celltag_Modify, 6)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Celltag);
	return 0;
}

DEFINE_HOOK(466970, CIsoView_OnLButtonUp_ResetRecordStatus, 6)
{
	CIsoViewExt::HistoryRecord_IsHoldingLButton = false;
	ObjectRecord::ObjectRecord_HoldingPtr = nullptr;
	return 0;
}

DEFINE_HOOK(45C37D, CIsoView_OnMouseMove_Basenode_Add_Del_Building, 6)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Basenode | ObjectRecord::RecordType::Building, true);
	return 0;
}

DEFINE_HOOK(45C813, CIsoView_OnMouseMove_Basenode_Delete, 6)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Basenode, true);
	return 0;
}

DEFINE_HOOK(45C6D2, CIsoView_OnMouseMove_Basenode_Add, 6)
{
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Basenode, true);
	return 0;
}

DEFINE_HOOK(456E90, CIsoView_OnMouseMove_ScrollInRendering, 6)
{
	if (CIsoViewExt::RenderingMap)
		return 0x456EC0;

	return 0;
}

DEFINE_HOOK(469520, CIsoView_GetOverlayDirection, 6)
{
	GET_STACK(int, x, 0x4);
	GET_STACK(int, y, 0x8);

	auto Map = CMapDataExt::GetExtension();
	const auto& dwIsoSize = Map->MapWidthPlusHeight;
	int p = -1;
	auto overlay = Map->GetOverlayAt(x + y * dwIsoSize);

	bool isWall = CViewObjectsExt::WallDamageStages.find(overlay) != CViewObjectsExt::WallDamageStages.end();

	auto isTrack = [](int type)
	{
		return(type >= OVRL_TRACK_BEGIN && type <= OVRL_TRACK_END);
	};

	if (isTrack(overlay))
	{
		p = 0;

		if (isTrack(Map->GetOverlayAt((x - 1) + (y - 1) * dwIsoSize)) && isTrack(Map->GetOverlayAt((x + 1) + (y + 1) * dwIsoSize)))
			p = 0;
		else if (isTrack(Map->GetOverlayAt((x + 1) + (y - 1) * dwIsoSize)) && isTrack(Map->GetOverlayAt((x - 1) + (y + 1) * dwIsoSize)))
			p = 1;
		else if (isTrack(Map->GetOverlayAt((x - 1) + (y - 0) * dwIsoSize)) && isTrack(Map->GetOverlayAt((x + 1) + (y + 0) * dwIsoSize)))
			p = 2;
		else if (isTrack(Map->GetOverlayAt((x - 0) + (y - 1) * dwIsoSize)) && isTrack(Map->GetOverlayAt((x + 0) + (y + 1) * dwIsoSize)))
			p = 3;
		else if (isTrack(Map->GetOverlayAt((x - 1) + (y - 0) * dwIsoSize)) && isTrack(Map->GetOverlayAt((x + 1) + (y + 1) * dwIsoSize)))
			p = 4;
		else if (isTrack(Map->GetOverlayAt((x - 0) + (y - 1) * dwIsoSize)) && isTrack(Map->GetOverlayAt((x + 1) + (y + 1) * dwIsoSize)))
			p = 5;
		else if (isTrack(Map->GetOverlayAt((x - 1) + (y - 1) * dwIsoSize)) && isTrack(Map->GetOverlayAt((x - 0) + (y + 1) * dwIsoSize)))
			p = 6;
		else if (isTrack(Map->GetOverlayAt((x - 1) + (y - 1) * dwIsoSize)) && isTrack(Map->GetOverlayAt((x + 1) + (y + 0) * dwIsoSize)))
			p = 7;
		else if (isTrack(Map->GetOverlayAt((x - 1) + (y - 0) * dwIsoSize)) && isTrack(Map->GetOverlayAt((x + 1) + (y - 1) * dwIsoSize)))
			p = 8;
		else if (isTrack(Map->GetOverlayAt((x + 0) + (y + 1) * dwIsoSize)) && isTrack(Map->GetOverlayAt((x + 1) + (y - 1) * dwIsoSize)))
			p = 9;
		else if (isTrack(Map->GetOverlayAt((x - 0) + (y - 1) * dwIsoSize)) && isTrack(Map->GetOverlayAt((x - 1) + (y + 1) * dwIsoSize)))
			p = 10;
		else if (isTrack(Map->GetOverlayAt((x - 1) + (y + 1) * dwIsoSize)) && isTrack(Map->GetOverlayAt((x + 1) + (y + 0) * dwIsoSize)))
			p = 11;
		else if (isTrack(Map->GetOverlayAt((x - 1) + (y - 1) * dwIsoSize)))
			p = 0;
		else if (isTrack(Map->GetOverlayAt((x - 1) + (y + 1) * dwIsoSize)))
			p = 1;
		else if (isTrack(Map->GetOverlayAt((x - 1) + (y - 0) * dwIsoSize)))
			p = 2;
		else if (isTrack(Map->GetOverlayAt((x - 0) + (y - 1) * dwIsoSize)))
			p = 3;
		else if (isTrack(Map->GetOverlayAt((x + 1) + (y + 1) * dwIsoSize)))
			p = 0;
		else if (isTrack(Map->GetOverlayAt((x + 1) + (y - 1) * dwIsoSize)))
			p = 1;
		else if (isTrack(Map->GetOverlayAt((x + 1) + (y + 0) * dwIsoSize)))
			p = 2;
		else if (isTrack(Map->GetOverlayAt((x + 0) + (y + 1) * dwIsoSize)))
			p = 3;
	}
	else if (isWall)
	{
		p = 0;
		auto overlayData = Map->GetOverlayDataAt(x + y * dwIsoSize);
		auto damageStage = overlayData / 16;
		damageStage = std::min(damageStage, CViewObjectsExt::WallDamageStages[overlay] - 1);

		if (Map->GetOverlayAt(x - 1 + (y - 0) * dwIsoSize) == overlay) p |= 1 + damageStage * 16;
		if (Map->GetOverlayAt(x + (y + 1) * dwIsoSize) == overlay) p |= 2 + damageStage * 16;
		if (Map->GetOverlayAt(x + 1 + (y + 0) * dwIsoSize) == overlay) p |= 4 + damageStage * 16;
		if (Map->GetOverlayAt(x + (y - 1) * dwIsoSize) == overlay) p |= 8 + damageStage * 16;
	}

	R->EAX(p);

	return 0x4695E6;
}

DEFINE_HOOK(469B71, CIsoView_HandleTrail_Range, 8)
{
	R->Stack<int>(STACK_OFFS(0x14, -0x8), R->EBP() - 2);
	R->EBP(R->EBP() + 2);
	return 0x469B79;
}

DEFINE_HOOK(469BED, CIsoView_HandleTrail_Range_2, 8)
{
	R->Stack<int>(STACK_OFFS(0x14, -0x8), R->EBP() - 2);
	R->EBP(R->EBP() + 2);
	return 0x469BF5;
}