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

static void AdjustHeightAt(int x, int y, int offset)
{
	if (CMapDataExt::IsCoordInFullMap(x, y))
	{
		auto& cell = CMapData::Instance->CellDatas[x + y * CMapData::Instance->MapWidthPlusHeight];
		if (ExtConfigs::PlaceTileSkipHide && cell.IsHidden())
			return;
		int height = cell.Height + offset;
		if (height < 0) height = 0;
		if (height > 14) height = 14;
		cell.Height = height;
	}
}

static void SetHeightForSameTileSet(int x, int y, int height, std::vector<int> tilesets)
{
	if (CMapDataExt::IsCoordInFullMap(x, y))
	{
		int pos = x + y * CMapData::Instance->MapWidthPlusHeight;
		int loops[3] = { 0, -1, 1 };
		for (int i : loops)
			for (int e : loops)
			{
				if (!CMapDataExt::IsCoordInFullMap(x + i, y + e))
					continue;
				int thisPos = x + i + (y + e) * CMapData::Instance->MapWidthPlusHeight;
				if (CMapDataExt::CellDataExts[thisPos].Adjusted)
					continue;

				auto cell = CMapData::Instance->GetCellAt(thisPos);
				if (ExtConfigs::PlaceTileSkipHide && cell->IsHidden())
					continue;

				int ground = cell->TileIndex;
				if (ground == 0xFFFF) ground = 0;
				int tileSet = CMapDataExt::TileData[ground].TileSet;
				int heightOffset = CMapDataExt::TileData[ground].TileBlockDatas[cell->TileSubIndex].Height;
				for (auto set: tilesets)
					if (tileSet == set)
					{
						int newHeight = height + heightOffset;
						if (newHeight < 0) newHeight = 0;
						if (newHeight > 14) newHeight = 14;
						CMapData::Instance->CellDatas[thisPos].Height = newHeight;
						CMapDataExt::CellDataExts[thisPos].Adjusted = true;
						SetHeightForSameTileSet(x + i, y + e, height, tilesets);
						break;
					}
			}

	}
}

static void SetHeightForSameTileIndex(int x, int y, int height, int tileindex)
{
	if (CMapDataExt::IsCoordInFullMap(x, y))
	{
		auto oriCell = CMapData::Instance->GetCellAt(x, y);
		int oriGround = oriCell->TileIndex;
		if (oriGround == 0xFFFF) oriGround = 0;
		const int loops[3] = { 0, -1, 1 };
		for (int i : loops)
			for (int e : loops)
			{
				if (!CMapDataExt::IsCoordInFullMap(x + i, y + e))
					continue;
				int thisPos = x + i + (y + e) * CMapData::Instance->MapWidthPlusHeight;
				if (CMapDataExt::CellDataExts[thisPos].Adjusted)
					continue;

				auto cell = CMapData::Instance->GetCellAt(thisPos);
				if (ExtConfigs::PlaceTileSkipHide && cell->IsHidden())
					continue;
				int ground = cell->TileIndex;
				if (ground == 0xFFFF) ground = 0;
				int heightOffset = CMapDataExt::TileData[ground].TileBlockDatas[cell->TileSubIndex].Height;
				if (ground == tileindex)
				{
					int of = oriCell->TileSubIndex;
					int f = cell->TileSubIndex;
					int width = CMapDataExt::TileData[oriGround].Width;
					int ox = of / width;
					int oy = of % width;
					int nx = f / width;
					int ny = f % width;

					if (nx - ox != i || ny - oy != e)
						continue;

					int newheight = height + heightOffset;
					if (newheight < 0) newheight = 0;
					if (newheight > 14) newheight = 14;
					CMapData::Instance->CellDatas[thisPos].Height = newheight;
					CMapDataExt::CellDataExts[thisPos].Adjusted = true;
					SetHeightForSameTileIndex(x + i, y + e, height, tileindex);
					break;
				}
			}

	}
}

static void FindConnectedTiles(std::unordered_map<int, bool>& process, int startX, int startY)
{
	const int loop[5][2] = { {0, 0},{0, -1},{0, 1},{1, 0},{-1, 0} }; // do not calculate 4 corners
	for (auto pair : loop)
	{
		int newX = pair[0] + startX;
		int newY = pair[1] + startY;
		if (!CMapDataExt::IsCoordInFullMap(newX, newY)) continue;
		int pos = newX + newY * CMapData::Instance->MapWidthPlusHeight;
		if (process.find(pos) == process.end())
			continue;
		if (process[pos])
			continue;
		auto cell = CMapData::Instance->GetCellAt(pos);
		if (ExtConfigs::PlaceTileSkipHide && cell->IsHidden())
			continue;
		int ground = cell->TileIndex;
		if (ground == 0xFFFF) ground = 0;
		if (CMapDataExt::TileData[ground].Morphable)
		{
			process[pos] = true;
			FindConnectedTiles(process, newX, newY);
		}
	}
}

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
	auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
	auto& mapData = CMapData::Instance();
	int posClick = X + Y * mapData.MapWidthPlusHeight;
	auto cellClick = mapData.GetCellAt(X, Y);
	int groundClick = mapData.GetCellAt(X, Y)->TileIndex;
	if (groundClick == 0xFFFF) groundClick = 0;
	auto tiledataClick = CMapDataExt::TileData[groundClick];

	bool IgnoreMorphable = (nFlags & MK_SHIFT) && (nFlags & MK_CONTROL);

	for (auto& cell : CMapDataExt::CellDataExts)
	{
		cell.Adjusted = false;
		cell.CreateSlope = false;
	}

	if (((nFlags & MK_CONTROL) || !tiledataClick.Morphable) && !IgnoreMorphable) // raise same tiles, ignore brush size
	{
		int heightClick = cellClick->Height;
		if (CMapDataExt::TileData[groundClick].Morphable)
		{
			CMapDataExt::GetExtension()->SetHeightAt(X, Y, heightClick + 1);
		}
		else
		{
			auto getNW = [](int x, int y)
				{
					return CMapDataExt::TryGetCellAt(x, y - 1);
				};
			auto getSE = [](int x, int y)
				{
					return CMapDataExt::TryGetCellAt(x, y + 1);
				};
			auto getNE = [](int x, int y)
				{
					return CMapDataExt::TryGetCellAt(x - 1, y);
				};
			auto getSW = [](int x, int y)
				{
					return CMapDataExt::TryGetCellAt(x + 1, y);
				};
			int tileSet = CMapDataExt::TileData[groundClick].TileSet;
			int heightOffset = CMapDataExt::TileData[groundClick].TileBlockDatas[cellClick->TileSubIndex].Height;

			std::vector<int> matchTiles;
			matchTiles.push_back(tileSet);
			for (int latidx = 0; latidx < CMapDataExt::Tile_to_lat.size(); ++latidx)
			{
				int iSmoothSet = CMapDataExt::Tile_to_lat[latidx][0];
				int iLatSet = CMapDataExt::Tile_to_lat[latidx][1];

				if (tileSet == iSmoothSet) matchTiles.push_back(iLatSet);
				if (tileSet == iLatSet) matchTiles.push_back(iSmoothSet);
			}
			ppmfc::CString tmp;
			tmp.Format("%d, %d", X, Y);
			if (nFlags & MK_CONTROL)
			{
				SetHeightForSameTileSet(X, Y, heightClick + 1 - heightOffset, matchTiles);
				CMapDataExt::CheckCellRise(false, 0, false);
			}
			else
				SetHeightForSameTileIndex(X, Y, heightClick + 1 - heightOffset, groundClick);
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
					int thisX = mapData.GetXFromCoordIndex(i);
					int thisY = mapData.GetYFromCoordIndex(i);
					CMapDataExt::CreateSlopeAt(thisX, thisY, IgnoreMorphable);
				}
			}
		}

	}
	else if (tiledataClick.Morphable || IgnoreMorphable)
	{
		int f, n;
		int heightClick = cellClick->Height;
		for (f = -pIsoView->BrushSizeX / 2; f < pIsoView->BrushSizeX / 2 + 1; f++)
		{
			for (n = -pIsoView->BrushSizeY / 2; n < pIsoView->BrushSizeY / 2 + 1; n++)
			{
				if (!CMapDataExt::IsCoordInFullMap(X + f, Y + n)) continue;

				int pos = X + f + (Y + n) * mapData.MapWidthPlusHeight;
				auto cell = mapData.GetCellAt(X + f, Y + n);
				int ground = mapData.GetCellAt(X + f, Y + n)->TileIndex;
				if (ground == 0xFFFF) ground = 0;

				if ((CMapDataExt::TileData[ground].Morphable || IgnoreMorphable) && cell->Height == heightClick)
				{
					CMapDataExt::GetExtension()->SetHeightAt(X + f, Y + n, heightClick + 1);
					CMapDataExt::CellDataExts[pos].Adjusted = true;
				}
			}
		}
		CMapDataExt::CheckCellRise((nFlags & MK_SHIFT) && !IgnoreMorphable, 0, IgnoreMorphable);
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
				int thisX = mapData.GetXFromCoordIndex(i);
				int thisY = mapData.GetYFromCoordIndex(i);
				CMapDataExt::CreateSlopeAt(thisX, thisY, IgnoreMorphable);
			}
		}
	}
	else
	{
		CMapDataExt::GetExtension()->SetHeightAt(X, Y, cellClick->Height + 1);
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
	//return 0x464AE4;
}
// already TakeSnapshot()
DEFINE_HOOK(464B01, CIsoView_OnLButtonDown_ACTIONMODE_LOWER, 7)
{
	GET(int, X, EDI);
	GET(int, Y, ESI);
	if (!CMapDataExt::IsCoordInFullMap(X, Y)) return 0x46555F;
	auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
	auto& mapData = CMapData::Instance();
	int posClick = X + Y * mapData.MapWidthPlusHeight;
	auto cellClick = mapData.GetCellAt(X, Y);
	int groundClick = mapData.GetCellAt(X, Y)->TileIndex;
	if (groundClick == 0xFFFF) groundClick = 0;
	auto tiledataClick = CMapDataExt::TileData[groundClick];

	bool IgnoreMorphable = (nFlags & MK_SHIFT) && (nFlags & MK_CONTROL);

	for (auto& cell : CMapDataExt::CellDataExts)
	{
		cell.Adjusted = false;
		cell.CreateSlope = false;
	}

	int brushx = pIsoView->BrushSizeX;
	int brushy = pIsoView->BrushSizeY;
	int loopStartX = -pIsoView->BrushSizeX / 2;
	int loopStartY = -pIsoView->BrushSizeY / 2;
	int loopEndX = pIsoView->BrushSizeX / 2 + 1;
	int loopEndY = pIsoView->BrushSizeY / 2 + 1;

	if (((nFlags & MK_CONTROL) || !tiledataClick.Morphable) && !IgnoreMorphable) // raise same tiles, ignore brush size
	{
		int heightClick = cellClick->Height;
		if (CMapDataExt::TileData[groundClick].Morphable)
		{
			CMapDataExt::GetExtension()->SetHeightAt(X, Y, heightClick - 1);
		}
		else
		{
			auto getNW = [](int x, int y)
				{
					return CMapDataExt::TryGetCellAt(x, y - 1);
				};
			auto getSE = [](int x, int y)
				{
					return CMapDataExt::TryGetCellAt(x, y + 1);
				};
			auto getNE = [](int x, int y)
				{
					return CMapDataExt::TryGetCellAt(x - 1, y);
				};
			auto getSW = [](int x, int y)
				{
					return CMapDataExt::TryGetCellAt(x + 1, y);
				};
			int tileSet = CMapDataExt::TileData[groundClick].TileSet;
			int heightOffset = CMapDataExt::TileData[groundClick].TileBlockDatas[cellClick->TileSubIndex].Height;

			std::vector<int> matchTiles;
			matchTiles.push_back(tileSet);
			for (int latidx = 0; latidx < CMapDataExt::Tile_to_lat.size(); ++latidx)
			{
				int iSmoothSet = CMapDataExt::Tile_to_lat[latidx][0];
				int iLatSet = CMapDataExt::Tile_to_lat[latidx][1];

				if (tileSet == iSmoothSet) matchTiles.push_back(iLatSet);
				if (tileSet == iLatSet) matchTiles.push_back(iSmoothSet);
			}
			if (nFlags & MK_CONTROL)
			{
				SetHeightForSameTileSet(X, Y, heightClick - 1 - heightOffset, matchTiles);

				// for lower, we need one more square
				// but ignore non-norphable tiles
				for (int i = 1; i < CMapDataExt::CellDataExts.size(); i++)
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

								auto cellCheckNonMorphable = mapData.GetCellAt(pos);
								int groundCheckNonMorphable = cellCheckNonMorphable->TileIndex;
								if (groundCheckNonMorphable == 0xFFFF) groundCheckNonMorphable = 0;
								auto tiledataCheckNonMorphable = CMapDataExt::TileData[groundCheckNonMorphable];
								if (!tiledataCheckNonMorphable.Morphable) continue;
								if (cellCheckNonMorphable->Height <= heightClick - 1) continue;

								CMapDataExt::CellDataExts[pos].CreateSlope = true;
							}
					}
				}
				for (int i = 1; i < CMapDataExt::CellDataExts.size(); i++)
				{
					if (CMapDataExt::CellDataExts[i].CreateSlope)
					{
						CMapDataExt::CellDataExts[i].CreateSlope = false;
						if (CMapDataExt::CellDataExts[i].Adjusted) continue;

						CMapDataExt::CellDataExts[i].Adjusted = true;
						int thisX = mapData.GetXFromCoordIndex(i);
						int thisY = mapData.GetYFromCoordIndex(i);
						CMapDataExt::GetExtension()->SetHeightAt(thisX, thisY, heightClick - 1);
					}
				}
				CMapDataExt::CheckCellLow(false, 0, false);
			}
			else
				SetHeightForSameTileIndex(X, Y, heightClick - 1 - heightOffset, groundClick);

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
					int thisX = mapData.GetXFromCoordIndex(i);
					int thisY = mapData.GetYFromCoordIndex(i);
					CMapDataExt::CreateSlopeAt(thisX, thisY, IgnoreMorphable);
				}
			}		

		}

	}
	else if (tiledataClick.Morphable || IgnoreMorphable)
	{
		int f, n;
		int heightClick = cellClick->Height;
		for (f = loopStartX; f < loopEndX; f++)
		{
			for (n = loopStartY; n < loopEndY; n++)
			{
				if (!CMapDataExt::IsCoordInFullMap(X + f, Y + n)) continue;

				int pos = X + f + (Y + n) * mapData.MapWidthPlusHeight;
				auto cell = mapData.GetCellAt(X + f, Y + n);
				int ground = mapData.GetCellAt(X + f, Y + n)->TileIndex;
				if (ground == 0xFFFF) ground = 0;

				if ((CMapDataExt::TileData[ground].Morphable || IgnoreMorphable) && cell->Height == heightClick)
				{
					CMapDataExt::GetExtension()->SetHeightAt(X + f, Y + n, heightClick - 1);
					CMapDataExt::CellDataExts[pos].Adjusted = true;
				}
			}
		}
		CMapDataExt::CheckCellLow((nFlags & MK_SHIFT) && !IgnoreMorphable, 0, IgnoreMorphable);
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
				int thisX = mapData.GetXFromCoordIndex(i);
				int thisY = mapData.GetYFromCoordIndex(i);
				CMapDataExt::CreateSlopeAt(thisX, thisY, IgnoreMorphable);
			}
		}
	}
	else
	{
		CMapDataExt::GetExtension()->SetHeightAt(X, Y, cellClick->Height - 1);
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
	//return 0x46555F;
}
// already TakeSnapshot()
DEFINE_HOOK(46557C, CIsoView_OnLButtonDown_ACTIONMODE_HEIGHTENTILE, 6)
{
	auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
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
			AdjustHeightAt(X + f, Y + n, 1);
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
	auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
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
			AdjustHeightAt(X + f, Y + n, -1);
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
				AdjustHeightAt(thisX, thisY, -1);
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

	auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
	int height = pIsoView->FlattenHeight;
	auto point = pIsoView->GetCurrentMapCoord(pIsoView->MouseCurrentPosition);
	int X = point.X;
	int Y = point.Y;

	if (!CMapDataExt::IsCoordInFullMap(X, Y)) return 0x45BF33;

	auto& mapData = CMapData::Instance();
	int posClick = X + Y * mapData.MapWidthPlusHeight;
	auto cellClick = mapData.GetCellAt(X, Y);
	int groundClick = mapData.GetCellAt(X, Y)->TileIndex;
	if (groundClick == 0xFFFF) groundClick = 0;
	auto tiledataClick = CMapDataExt::TileData[groundClick];

	auto IgnoreMorphable = CIsoViewExt::nFlagsMove == (MK_LBUTTON | MK_CONTROL | MK_SHIFT);


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
	// first = map pos, second = should be precessed
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
		FindConnectedTiles(process, X, Y);
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
			int ground = mapData.GetCellAt(X + f, Y + n)->TileIndex;
			if (ground == 0xFFFF) ground = 0;

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
				}
		}
	}
	for (int i = 1; i < CMapDataExt::CellDataExts.size(); i++) // skip 0
	{
		if (CMapDataExt::CellDataExts[i].CreateSlope)
		{
			int thisX = mapData.GetXFromCoordIndex(i);
			int thisY = mapData.GetYFromCoordIndex(i);
			CMapDataExt::CreateSlopeAt(thisX, thisY, IgnoreMorphable);
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
	
	return 0x45BF33;
}

