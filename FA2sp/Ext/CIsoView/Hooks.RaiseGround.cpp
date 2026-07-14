#include "Body.h"
#include "../../FA2sp.h"

#include <unordered_map>
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
#include "../../Miscs/TheaterInfo.h"

UINT nFlags;
DEFINE_HOOK(4612F0, CIsoView_OnLButtonDown_Update_nFlags, 5)
{
	nFlags = R->Stack<UINT>(0x4);
	return 0;
}
// already TakeSnapshot()
DEFINE_HOOK(46404B, CIsoView_OnLButtonDown_ACTIONMODE_HEIGHTEN, 7)
{
	GET(int, X, EDI);
	GET(int, Y, ESI);

	if (!CMapDataExt::IsCoordInFullMap(X, Y)) return 0x464AE4;

	if (CIsoViewExt::UsingNewRaiseGround)
	{
		CMapDataExt::RaiseVertices(X, Y, true, (nFlags & MK_SHIFT) && (nFlags & MK_CONTROL), (nFlags & MK_SHIFT));
	}
	else
	{	
		CMapDataExt::RaiseGrounds(X, Y, true, nFlags);
	}

	::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
	return 0x46686A;
}
// already TakeSnapshot()
DEFINE_HOOK(464B01, CIsoView_OnLButtonDown_ACTIONMODE_LOWER, 7)
{
	GET(int, X, EDI);
	GET(int, Y, ESI);
	if (!CMapDataExt::IsCoordInFullMap(X, Y)) return 0x46555F;

	if (CIsoViewExt::UsingNewRaiseGround)
	{
		CMapDataExt::RaiseVertices(X, Y, false, (nFlags & MK_SHIFT) && (nFlags & MK_CONTROL), (nFlags & MK_SHIFT));
	}
	else 
	{
		CMapDataExt::RaiseGrounds(X, Y, false, nFlags);
	}

	::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
	return 0x46686A;
}
// already TakeSnapshot()
DEFINE_HOOK(46557C, CIsoView_OnLButtonDown_ACTIONMODE_HEIGHTENTILE, 6)
{
	auto pIsoView = CIsoViewExt::GetExtension();
	auto point = pIsoView->GetCurrentMapCoord(pIsoView->MouseCurrentPosition);
	auto& mapData = CMapData::Instance();
	int X = point.X;
	int Y = point.Y;
	std::vector<int> ignoreList;

	for (auto& cell : CMapDataExt::CellDataExts)
	{
		cell.Adjusted = false;
		cell.CreateSlope = false;
	}

	int f, n;
	for (f = -pIsoView->BrushSizeX / 2; f < pIsoView->BrushSizeX / 2 + 1; f++)
	{
		for (n = -pIsoView->BrushSizeY / 2; n < pIsoView->BrushSizeY / 2 + 1; n++)
		{
			if (!CMapDataExt::IsCoordInFullMap(X + f, Y + n)) continue;
			int pos = X + f + (Y + n) * mapData.MapWidthPlusHeight;
			ignoreList.push_back(pos);
			CMapDataExt::AdjustHeightAt(X + f, Y + n, 1);
			CMapDataExt::CellDataExts[pos].Adjusted = true;
		}
	}
	if (nFlags & MK_CONTROL)
	{
		CMapDataExt::CheckCellRise(false, 0, false, &ignoreList);
		for (int i = 1; i < CMapDataExt::CellDataExts.size(); i++) // skip 0
		{
			if (CMapDataExt::CellDataExts[i].Adjusted)
			{
				int thisX = mapData.GetXFromCoordIndex(i);
				int thisY = mapData.GetYFromCoordIndex(i);
				int loops[3] = { 0, -1, 1 };
				for (int i : loops)
					for (int e : loops)
					{
						int newX = thisX + i;
						int newY = thisY + e;
						int pos = CMapData::Instance->GetCoordIndex(newX, newY);
						if (!CMapDataExt::IsCoordInFullMap(pos)) continue;
						CMapDataExt::CellDataExts[pos].CreateSlope = true;
					}
			}
		}
		for (int i = 1; i < CMapDataExt::CellDataExts.size(); i++) // skip 0
		{
			if (CMapDataExt::CellDataExts[i].CreateSlope)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), i) == ignoreList.end())
				{
					int thisX = mapData.GetXFromCoordIndex(i);
					int thisY = mapData.GetYFromCoordIndex(i);
					CMapDataExt::CreateSlopeAt(thisX, thisY, false);
				}
			}
		}
	}
	for (int i = 0; i < CMapDataExt::CellDataExts.size(); i++)
	{
		if (CMapDataExt::CellDataExts[i].CreateSlope || CMapDataExt::CellDataExts[i].Adjusted)
		{
			int thisX = mapData.GetXFromCoordIndex(i);
			int thisY = mapData.GetYFromCoordIndex(i);
			CMapData::Instance->UpdateMapPreviewAt(thisX, thisY);
		}
	}

	::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
	return 0x46686A;
	//return 0x465CAA;
}
// already TakeSnapshot()
DEFINE_HOOK(465CC7, CIsoView_OnLButtonDown_ACTIONMODE_LOWERTILE, 6)
{
	auto pIsoView = CIsoViewExt::GetExtension();
	auto point = pIsoView->GetCurrentMapCoord(pIsoView->MouseCurrentPosition);
	auto& mapData = CMapData::Instance();
	int X = point.X;
	int Y = point.Y;
	std::vector<int> ignoreList;

	for (auto& cell : CMapDataExt::CellDataExts)
	{
		cell.Adjusted = false;
		cell.CreateSlope = false;
	}

	int f, n;
	for (f = -pIsoView->BrushSizeX / 2; f < pIsoView->BrushSizeX / 2 + 1; f++)
	{
		for (n = -pIsoView->BrushSizeY / 2; n < pIsoView->BrushSizeY / 2 + 1; n++)
		{
			if (!CMapDataExt::IsCoordInFullMap(X + f, Y + n)) continue;
			int pos = X + f + (Y + n) * mapData.MapWidthPlusHeight;
			ignoreList.push_back(pos);
			CMapDataExt::AdjustHeightAt(X + f, Y + n, -1);
			CMapDataExt::CellDataExts[pos].Adjusted = true;
		}
	}
	if (nFlags & MK_CONTROL)
	{
		for (int i = 1; i < CMapDataExt::CellDataExts.size(); i++) // for lower, we need one more square
		{
			if (CMapDataExt::CellDataExts[i].Adjusted)
			{
				int thisX = mapData.GetXFromCoordIndex(i);
				int thisY = mapData.GetYFromCoordIndex(i);
				int loops[3] = { 0, -1, 1 };
				for (int i : loops)
					for (int e : loops)
					{
						int newX = thisX + i;
						int newY = thisY + e;
						int pos = CMapData::Instance->GetCoordIndex(newX, newY);
						if (!CMapDataExt::IsCoordInFullMap(pos)) continue;
						CMapDataExt::CellDataExts[pos].CreateSlope = true;
					}
			}
		}
		for (int i = 1; i < CMapDataExt::CellDataExts.size(); i++) // for lower, we need one more square
		{
			if (CMapDataExt::CellDataExts[i].CreateSlope)
			{
				CMapDataExt::CellDataExts[i].CreateSlope = false;
				if (CMapDataExt::CellDataExts[i].Adjusted) continue;

				CMapDataExt::CellDataExts[i].Adjusted = true;
				int thisX = mapData.GetXFromCoordIndex(i);
				int thisY = mapData.GetYFromCoordIndex(i);
				CMapDataExt::AdjustHeightAt(thisX, thisY, -1);
			}
		}
		CMapDataExt::CheckCellLow(false, 0, false, &ignoreList);
		for (int i = 1; i < CMapDataExt::CellDataExts.size(); i++) // skip 0
		{
			if (CMapDataExt::CellDataExts[i].Adjusted)
			{
				int thisX = mapData.GetXFromCoordIndex(i);
				int thisY = mapData.GetYFromCoordIndex(i);
				int loops[3] = { 0, -1, 1 };
				for (int i : loops)
					for (int e : loops)
					{
						int newX = thisX + i;
						int newY = thisY + e;
						int pos = CMapData::Instance->GetCoordIndex(newX, newY);
						if (!CMapDataExt::IsCoordInFullMap(pos)) continue;
						CMapDataExt::CellDataExts[pos].CreateSlope = true;
					}
			}
		}
		for (int i = 1; i < CMapDataExt::CellDataExts.size(); i++) // skip 0
		{
			if (CMapDataExt::CellDataExts[i].CreateSlope)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), i) == ignoreList.end())
				{
					int thisX = mapData.GetXFromCoordIndex(i);
					int thisY = mapData.GetYFromCoordIndex(i);
					CMapDataExt::CreateSlopeAt(thisX, thisY, false);
				}
			}
		}
	}
	for (int i = 0; i < CMapDataExt::CellDataExts.size(); i++)
	{
		if (CMapDataExt::CellDataExts[i].CreateSlope || CMapDataExt::CellDataExts[i].Adjusted)
		{
			int thisX = mapData.GetXFromCoordIndex(i);
			int thisY = mapData.GetYFromCoordIndex(i);
			CMapData::Instance->UpdateMapPreviewAt(thisX, thisY);
		}
	}

	::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
	return 0x46686A;
	//return 0x4663D3;
}

DEFINE_HOOK(45B523, CIsoView_OnMouseMove_SkipShift, 7)
{
	if (CIsoView::CurrentCommand->Command == 11 || CIsoView::CurrentCommand->Command == 12) {
		CIsoViewExt::SetStatusBarText(Translations::TranslateOrDefault("HeightenAndLowerMessage",
				"Ctrl: Raise/lower same tileset, Shift: Steep slope, Ctrl+Shift:  Ignore non-morphable tiles"));
		return 0x45B567;
	}
	return 0;
}

DEFINE_HOOK(456DA0, CIsoView_OnMouseMove_Update_nFlags, 8)
{
	CIsoViewExt::nFlagsMove = R->Stack<UINT>(0x4);
	return 0;
}

DEFINE_HOOK(45B5B6, CIsoView_OnMouseMove_FLATTENGROUND, 9)
{
	if (CIsoView::CurrentCommand->Command != 15)
		return 0x45BF33;

	if ((CIsoViewExt::nFlagsMove != MK_LBUTTON) && CIsoViewExt::nFlagsMove != (MK_LBUTTON | MK_SHIFT) && (CIsoViewExt::nFlagsMove != (MK_LBUTTON | MK_CONTROL | MK_SHIFT)))
		return 0x45BF33;

	if (CIsoViewExt::nFlagsMove == (MK_LBUTTON | MK_SHIFT) || CIsoViewExt::nFlagsMove == (MK_LBUTTON | MK_CONTROL | MK_SHIFT)) // reduce lag
	{
		static int reduceFrequency = 0;
		reduceFrequency++;
		if (reduceFrequency == 5)
		{
			reduceFrequency = 0;
			return 0x45BF33;
		}
	}

	auto pIsoView = CIsoViewExt::GetExtension();
	int height = pIsoView->FlattenHeight;
	auto point = pIsoView->GetCurrentMapCoord(pIsoView->MouseCurrentPosition);
	int X = point.X;
	int Y = point.Y;

	if (!CMapDataExt::IsCoordInFullMap(X, Y)) return 0x45BF33;

	auto& mapData = CMapData::Instance();
	int posClick = X + Y * mapData.MapWidthPlusHeight;
	auto cellClick = mapData.GetCellAt(X, Y);
	int groundClick = CMapDataExt::GetSafeTileIndex(mapData.GetCellAt(X, Y)->TileIndex);
	auto tiledataClick = CMapDataExt::TileData[groundClick];

	auto IgnoreMorphable = CIsoViewExt::nFlagsMove == (MK_LBUTTON | MK_CONTROL | MK_SHIFT);

	if (CIsoViewExt::UsingNewRaiseGround)
	{
		if (tiledataClick.Morphable || IgnoreMorphable)
		{
			int loopStartX = -pIsoView->BrushSizeX / 2;
			int loopStartY = -pIsoView->BrushSizeY / 2;
			int loopEndX = pIsoView->BrushSizeX / 2 + 1;
			int loopEndY = pIsoView->BrushSizeY / 2 + 1;
		
			// using pathfinding algorithm to enable brushes to be separated by non-Morphable tiles
			// first = map pos, second = should be processed
			std::unordered_map<int, bool> process; 
		
			if (!tiledataClick.Morphable) // skip top half for flattening from cliffs
			{
				loopStartX = 0;
				loopStartY = 0;
			}
			int f, n;
			if (!IgnoreMorphable)
			{
				for (f = loopStartX; f < loopEndX; f++)
				{
					for (n = loopStartY; n < loopEndY; n++)
					{
						int pos = X + f + (Y + n) * mapData.MapWidthPlusHeight;
						process[pos] = false;
					}
				}
				CMapDataExt::FindConnectedTiles(process, X, Y);
			}
			std::set<VertexHeight> vertices;
			for (f = loopStartX; f < loopEndX; f++)
			{
				for (n = loopStartY; n < loopEndY; n++)
				{
					auto cells = VertexHeight::GetCellsFromVertices({{X + f, Y + n}});
					bool isAllValid = true;
					int unprocessedCellCount = 0;
					int nonmorphableCellCount = 0;
					for (auto coord : cells)
					{	
						if (!CMapDataExt::IsCoordInFullMap(coord.X, coord.Y)) 
						{
							isAllValid = false;
							unprocessedCellCount = 4;
							nonmorphableCellCount = 4;
							break;
						}
						if (!IgnoreMorphable && !process[mapData.GetCoordIndex(coord.X, coord.Y)])
						{
							unprocessedCellCount++;						
						}
						auto cell = mapData.GetCellAt(coord.X, coord.Y);
						int ground = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
					
						if (!CMapDataExt::TileData[ground].Morphable && !IgnoreMorphable)
						{
							nonmorphableCellCount++;
						}
					}
					if (!isAllValid) continue;
					if (!IgnoreMorphable && nonmorphableCellCount == 4) continue;
					if (!IgnoreMorphable && unprocessedCellCount == 4) continue;
					vertices.insert({X + f, Y + n, height});
				}
			}

			auto smoothedVertices = CMapDataExt::GetSmoothedVertexHeight(
				vertices, (CIsoViewExt::nFlagsMove & MK_SHIFT) && !IgnoreMorphable,
				 IgnoreMorphable, true);
			VertexHeight::ApplyRamps(smoothedVertices, nullptr, true, IgnoreMorphable);
			for (auto& vh : smoothedVertices)
			{
				int pos = vh.X + vh.Y * mapData.MapWidthPlusHeight;
				if (pos < 0 || pos >= CMapDataExt::CellDataExts.size()) continue;
				CMapData::Instance->UpdateMapPreviewAt(vh.X, vh.Y);
			}
		}
	}
	else
	{
		for (auto& cell : CMapDataExt::CellDataExts)
		{
			cell.Adjusted = false;
			cell.CreateSlope = false;
		}
	
		int loopStartX = -pIsoView->BrushSizeX / 2;
		int loopStartY = -pIsoView->BrushSizeY / 2;
		int loopEndX = pIsoView->BrushSizeX / 2 + 1;
		int loopEndY = pIsoView->BrushSizeY / 2 + 1;
	
		// using pathfinding algorithm to enable brushes to be separated by non-Morphable tiles
		// first = map pos, second = should be processed
		std::unordered_map<int, bool> process; 
	
		if (!tiledataClick.Morphable) // skip top half for flattening from cliffs
		{
			loopStartX = 0;
			loopStartY = 0;
		}
		int f, n;
		if (!IgnoreMorphable)
		{
			for (f = loopStartX; f < loopEndX; f++)
			{
				for (n = loopStartY; n < loopEndY; n++)
				{
					int pos = X + f + (Y + n) * mapData.MapWidthPlusHeight;
					process[pos] = false;
				}
			}
			CMapDataExt::FindConnectedTiles(process, X, Y);
		}
		for (f = loopStartX; f < loopEndX; f++)
		{
			for (n = loopStartY; n < loopEndY; n++)
			{
				if (!CMapDataExt::IsCoordInFullMap(X + f, Y + n)) continue;
				int pos = X + f + (Y + n) * mapData.MapWidthPlusHeight;
				if (!IgnoreMorphable)
					if (!process[pos]) continue;
	
				auto cell = mapData.GetCellAt(X + f, Y + n);
				int ground = CMapDataExt::GetSafeTileIndex(mapData.GetCellAt(X + f, Y + n)->TileIndex);
	
				if ((CMapDataExt::TileData[ground].Morphable || IgnoreMorphable))
				{
					CMapDataExt::GetExtension()->SetHeightAt(X + f, Y + n, height);
					CMapDataExt::CellDataExts[pos].Adjusted = true;
				}
			}
		}
		CMapDataExt::CheckCellRise((CIsoViewExt::nFlagsMove & MK_SHIFT) && !IgnoreMorphable, 0, IgnoreMorphable);
		CMapDataExt::CheckCellLow((CIsoViewExt::nFlagsMove & MK_SHIFT) && !IgnoreMorphable, 0, IgnoreMorphable);
		for (int i = 1; i < CMapDataExt::CellDataExts.size(); i++) // skip 0
		{
			if (CMapDataExt::CellDataExts[i].Adjusted)
			{
				int thisX = mapData.GetXFromCoordIndex(i);
				int thisY = mapData.GetYFromCoordIndex(i);
				int loops[3] = { 0, -1, 1 };
				for (int i : loops)
					for (int e : loops)
					{
						int newX = thisX + i;
						int newY = thisY + e;
						int pos = CMapData::Instance->GetCoordIndex(newX, newY);
						if (!CMapDataExt::IsCoordInFullMap(pos)) continue;
						CMapDataExt::CellDataExts[pos].CreateSlope = true;
						CMapDataExt::ReplaceRampWithFlat(newX, newY);
					}
			}
		}
		for (int i = 1; i < CMapDataExt::CellDataExts.size(); i++) // skip 0
		{
			if (CMapDataExt::CellDataExts[i].CreateSlope)
			{
				int thisX = mapData.GetXFromCoordIndex(i);
				int thisY = mapData.GetYFromCoordIndex(i);	
				CMapDataExt::CreateSlopeAt(thisX, thisY, IgnoreMorphable, true);
			}
		}
		for (int i = 0; i < CMapDataExt::CellDataExts.size(); i++)
		{
			if (CMapDataExt::CellDataExts[i].CreateSlope || CMapDataExt::CellDataExts[i].Adjusted)
			{
				int thisX = mapData.GetXFromCoordIndex(i);
				int thisY = mapData.GetYFromCoordIndex(i);
				CMapData::Instance->UpdateMapPreviewAt(thisX, thisY);
			}
		}
	}

	::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
	
	return 0x45BF33;
}

DEFINE_HOOK(46640E, CIsoView_OnLButtonDown_FlattenHeight, 5)
{
	if (CIsoViewExt::UsingNewRaiseGround)
	{
		GET(int, X, EDI);
		GET(int, Y, ESI);
	
		VertexHeight vhClick = {X, Y};
		vhClick.GetVertexHeight(false, true);
		CIsoViewExt::GetExtension()->FlattenHeight = vhClick.Height;
	}

	return 0x464AC1;
}
