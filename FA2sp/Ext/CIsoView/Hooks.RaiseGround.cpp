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

static void CheckCellLow(bool steep, int loopCount = 0, bool IgnoreMorphable = false, std::vector<int> ignoreList = {})
{
	loopCount++;
	if (loopCount > 15) // 15 should be enough for 0-14 layers
		return;
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
	auto getIndex = [](int x, int y)
		{
			if (!CMapData::Instance().IsCoordInMap(x, y))
				return 0;
			return x + y * CMapData::Instance->MapWidthPlusHeight;
		};
	auto getNW = [](int x, int y)
		{
			return CMapData::Instance->TryGetCellAt(x, y - 1);
		};
	auto getSE = [](int x, int y)
		{
			return CMapData::Instance->TryGetCellAt(x, y + 1);
		};
	auto getNE = [](int x, int y)
		{
			return CMapData::Instance->TryGetCellAt(x - 1, y);
		};
	auto getSW = [](int x, int y)
		{
			return CMapData::Instance->TryGetCellAt(x + 1, y);
		};
	auto getN = [](int x, int y)
		{
			return CMapData::Instance->TryGetCellAt(x - 1, y - 1);
		};
	auto getE = [](int x, int y)
		{
			return CMapData::Instance->TryGetCellAt(x - 1, y + 1);
		};
	auto getS = [](int x, int y)
		{
			return CMapData::Instance->TryGetCellAt(x + 1, y + 1);
		};
	auto getW = [](int x, int y)
		{
			return CMapData::Instance->TryGetCellAt(x + 1, y - 1);
		};
	bool loop = false;
	for (int i = 1; i < CMapDataExt::CellDataExts.size(); i++) // skip 0
	{
		if (CMapDataExt::CellDataExts[i].Adjusted)
		{
			int height = CMapData::Instance->CellDatas[i].Height;
			if (height < 0) height = 0;
			if (height > 14) height = 14;
			int thisX = CMapData::Instance->GetXFromCoordIndex(i);
			int thisY = CMapData::Instance->GetYFromCoordIndex(i);

			auto nw = getNW(thisX, thisY);
			auto ne = getNE(thisX, thisY);
			auto sw = getSW(thisX, thisY);
			auto se = getSE(thisX, thisY);
			auto n = getN(thisX, thisY);
			auto s = getS(thisX, thisY);
			auto e = getE(thisX, thisY);
			auto w = getW(thisX, thisY);
			if (isMorphable(nw) && nw->Height - height > 1)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX, thisY - 1)) == ignoreList.end())
				{
					nw->Height = height + 1;
					CMapDataExt::CellDataExts[getIndex(thisX, thisY - 1)].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(se) && se->Height - height > 1)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX, thisY + 1)) == ignoreList.end())
				{
					se->Height = height + 1;
					CMapDataExt::CellDataExts[getIndex(thisX, thisY + 1)].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(sw) && sw->Height - height > 1)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX + 1, thisY)) == ignoreList.end())
				{
					sw->Height = height + 1;
					CMapDataExt::CellDataExts[getIndex(thisX + 1, thisY)].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(ne) && ne->Height - height > 1)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX - 1, thisY)) == ignoreList.end())
				{
					ne->Height = height + 1;
					CMapDataExt::CellDataExts[getIndex(thisX - 1, thisY)].Adjusted = true;
					loop = true;
				}
			}
			if (!steep)
			{
				if (isMorphable(n) && n->Height - height > 1)
				{
					if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX - 1, thisY - 1)) == ignoreList.end())
					{
						n->Height = height + 1;
						CMapDataExt::CellDataExts[getIndex(thisX - 1, thisY - 1)].Adjusted = true;
						loop = true;
					}
				}
				if (isMorphable(s) && s->Height - height > 1)
				{
					if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX + 1, thisY + 1)) == ignoreList.end())
					{
						s->Height = height + 1;
						CMapDataExt::CellDataExts[getIndex(thisX + 1, thisY + 1)].Adjusted = true;
						loop = true;
					}
				}
				if (isMorphable(w) && w->Height - height > 1)
				{
					if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX + 1, thisY - 1)) == ignoreList.end())
					{
						w->Height = height + 1;
						CMapDataExt::CellDataExts[getIndex(thisX + 1, thisY - 1)].Adjusted = true;
						loop = true;
					}
				}
				if (isMorphable(e) && e->Height - height > 1)
				{
					if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX - 1, thisY + 1)) == ignoreList.end())
					{
						e->Height = height + 1;
						CMapDataExt::CellDataExts[getIndex(thisX - 1, thisY + 1)].Adjusted = true;
						loop = true;
					}
				}
			}


			if (isMorphable(nw) && nw->Height - height > 0 && se->Height - height > 0)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX, thisY - 1)) == ignoreList.end())
				{
					nw->Height = height;
					CMapDataExt::CellDataExts[getIndex(thisX, thisY - 1)].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(ne) && sw->Height - height > 0 && ne->Height - height > 0)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX - 1, thisY)) == ignoreList.end())
				{
					ne->Height = height;
					CMapDataExt::CellDataExts[getIndex(thisX - 1, thisY)].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(s) && s->Height - height > 0 && se->Height - height <= 0 && ne->Height - height > 0)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX + 1, thisY + 1)) == ignoreList.end())
				{
					s->Height = height;
					CMapDataExt::CellDataExts[getIndex(thisX + 1, thisY + 1)].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(s) && s->Height - height > 0 && sw->Height - height <= 0 && nw->Height - height > 0)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX + 1, thisY + 1)) == ignoreList.end())
				{
					s->Height = height;
					CMapDataExt::CellDataExts[getIndex(thisX + 1, thisY + 1)].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(n) && n->Height - height > 0 && ne->Height - height <= 0 && se->Height - height > 0)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX - 1, thisY - 1)) == ignoreList.end())
				{
					n->Height = height;
					CMapDataExt::CellDataExts[getIndex(thisX - 1, thisY - 1)].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(n) && n->Height - height > 0 && nw->Height - height <= 0 && sw->Height - height > 0)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX - 1, thisY - 1)) == ignoreList.end())
				{
					n->Height = height;
					CMapDataExt::CellDataExts[getIndex(thisX - 1, thisY - 1)].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(e) && e->Height - height > 0 && ne->Height - height <= 0 && nw->Height - height > 0)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX - 1, thisY + 1)) == ignoreList.end())
				{
					e->Height = height;
					CMapDataExt::CellDataExts[getIndex(thisX - 1, thisY + 1)].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(e) && e->Height - height > 0 && se->Height - height <= 0 && sw->Height - height > 0)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX - 1, thisY + 1)) == ignoreList.end())
				{
					e->Height = height;
					CMapDataExt::CellDataExts[getIndex(thisX - 1, thisY + 1)].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(e) && w->Height - height > 0 && nw->Height - height <= 0 && ne->Height - height > 0)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX + 1, thisY - 1)) == ignoreList.end())
				{
					e->Height = height;
					CMapDataExt::CellDataExts[getIndex(thisX + 1, thisY - 1)].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(e) && w->Height - height > 0 && sw->Height - height <= 0 && se->Height - height > 0)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX + 1, thisY - 1)) == ignoreList.end())
				{
					e->Height = height;
					CMapDataExt::CellDataExts[getIndex(thisX + 1, thisY - 1)].Adjusted = true;
					loop = true;
				}
			}
		}
	}
	if (loop)
		CheckCellLow(steep, loopCount, IgnoreMorphable, ignoreList);
}

static void CheckCellRise(bool steep, int loopCount = 0, bool IgnoreMorphable = false, std::vector<int> ignoreList = {})
{
	//                     *
	//        NW        N     N        NE
	//               N     *     N
	//            N     H     H     N
	//         *     *     C     *     *
	//            N     H     H     N
	//               N     *     N
	//        SW        N     N       SE
	//                     *
	loopCount++;
	if (loopCount > 15)
		return;
	bool loop = false;
	auto getIndex = [](int x, int y)
		{			
			if (!CMapData::Instance().IsCoordInMap(x, y))
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
	auto getNW = [](int x, int y)
		{
			return CMapData::Instance->TryGetCellAt(x, y - 1);
		};
	auto getSE = [](int x, int y)
		{
			return CMapData::Instance->TryGetCellAt(x, y + 1);
		};
	auto getNE = [](int x, int y)
		{
			return CMapData::Instance->TryGetCellAt(x - 1, y);
		};
	auto getSW = [](int x, int y)
		{
			return CMapData::Instance->TryGetCellAt(x + 1, y);
		};
	auto getN = [](int x, int y)
		{
			return CMapData::Instance->TryGetCellAt(x - 1, y - 1);
		};
	auto getE = [](int x, int y)
		{
			return CMapData::Instance->TryGetCellAt(x - 1, y + 1);
		};
	auto getS = [](int x, int y)
		{
			return CMapData::Instance->TryGetCellAt(x + 1, y + 1);
		};
	auto getW = [](int x, int y)
		{
			return CMapData::Instance->TryGetCellAt(x + 1, y - 1);
		};
	for (int i = 1; i < CMapDataExt::CellDataExts.size(); i++) // skip 0
	{
		if (CMapDataExt::CellDataExts[i].Adjusted)
		{
			int height = CMapData::Instance->CellDatas[i].Height;
			//if (onexone)
			//	height++;
			if (height < 0) height = 0;
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
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX, thisY - 1)) == ignoreList.end())
				{
					onw->Height = height - 1;
					CMapDataExt::CellDataExts[getIndex(thisX, thisY - 1)].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(ose) && height - ose->Height > 1)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX, thisY + 1)) == ignoreList.end())
				{
					ose->Height = height - 1;
					CMapDataExt::CellDataExts[getIndex(thisX, thisY + 1)].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(osw) && height - osw->Height > 1)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX + 1, thisY)) == ignoreList.end())
				{
					osw->Height = height - 1;
					CMapDataExt::CellDataExts[getIndex(thisX + 1, thisY)].Adjusted = true;
					loop = true;
				}
			}
			if (isMorphable(one) && height - one->Height > 1)
			{
				if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX - 1, thisY)) == ignoreList.end())
				{
					one->Height = height - 1;
					CMapDataExt::CellDataExts[getIndex(thisX - 1, thisY)].Adjusted = true;
					loop = true;
				}
			}
			if (!steep)
			{
				if (isMorphable(on) && height - on->Height > 1)
				{
					if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX - 1, thisY - 1)) == ignoreList.end())
					{
						on->Height = height - 1;
						CMapDataExt::CellDataExts[getIndex(thisX - 1, thisY - 1)].Adjusted = true;
						loop = true;
					}
				}
				if (isMorphable(os) && height - os->Height > 1)
				{
					if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX + 1, thisY + 1)) == ignoreList.end())
					{
						os->Height = height - 1;
						CMapDataExt::CellDataExts[getIndex(thisX + 1, thisY + 1)].Adjusted = true;
						loop = true;
					}
				}
				if (isMorphable(ow) && height - ow->Height > 1)
				{
					if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX + 1, thisY - 1)) == ignoreList.end())
					{
						ow->Height = height - 1;
						CMapDataExt::CellDataExts[getIndex(thisX + 1, thisY - 1)].Adjusted = true;
						loop = true;
					}
				}
				if (isMorphable(oe) && height - oe->Height > 1)
				{
					if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(thisX - 1, thisY + 1)) == ignoreList.end())
					{
						oe->Height = height - 1;
						CMapDataExt::CellDataExts[getIndex(thisX - 1, thisY + 1)].Adjusted = true;
						loop = true;
					}
				}
			}

			// center goes first
			int loops[3] = { 0, -1, 1 };
			for (int i : loops)
				for (int e : loops)
				{
					int newX = thisX + i;
					int newY = thisY + e;
					if (!CMapData::Instance->IsCoordInMap(newX, newY)) continue;
					auto cell = CMapData::Instance->TryGetCellAt(newX, newY);
					int ground = cell->TileIndex;
					if (ground == 0xFFFF) ground = 0;
					if (!CMapDataExt::TileData[ground].Morphable) continue;

					auto nw = getNW(newX, newY);
					auto ne = getNE(newX, newY);
					auto sw = getSW(newX, newY);
					auto se = getSE(newX, newY);
					auto n = getN(newX, newY);
					auto s = getS(newX, newY);
					auto e = getE(newX, newY);
					auto w = getW(newX, newY);

					if (height - cell->Height > 0)
					{
						if ((nw->Height - cell->Height > 0 && se->Height - cell->Height > 0)
							|| (sw->Height - cell->Height > 0 && ne->Height - cell->Height > 0)
							|| (s->Height - cell->Height > 0 && se->Height - cell->Height <= 0 && ne->Height - cell->Height > 0)
							|| (s->Height - cell->Height > 0 && sw->Height - cell->Height <= 0 && nw->Height - cell->Height > 0)
							|| (n->Height - cell->Height > 0 && ne->Height - cell->Height <= 0 && se->Height - cell->Height > 0)
							|| (n->Height - cell->Height > 0 && nw->Height - cell->Height <= 0 && sw->Height - cell->Height > 0)
							|| (e->Height - cell->Height > 0 && ne->Height - cell->Height <= 0 && nw->Height - cell->Height > 0)
							|| (e->Height - cell->Height > 0 && se->Height - cell->Height <= 0 && sw->Height - cell->Height > 0)
							|| (w->Height - cell->Height > 0 && nw->Height - cell->Height <= 0 && ne->Height - cell->Height > 0)
							|| (w->Height - cell->Height > 0 && sw->Height - cell->Height <= 0 && se->Height - cell->Height > 0)
							)
						{
							if (isMorphable(cell))
							{
								if (std::find(ignoreList.begin(), ignoreList.end(), getIndex(newX, newY)) == ignoreList.end())
								{
									cell->Height = height;
									CMapDataExt::CellDataExts[getIndex(newX, newY)].Adjusted = true;
									loop = true;
								}
							}
						}
					}
				}
		}
	}

	if (loop)
		CheckCellRise(steep, loopCount, IgnoreMorphable, ignoreList);
}

static void AdjustHeightAt(int x, int y, int offset)
{
	if (CMapData::Instance->IsCoordInMap(x, y))
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
	if (CMapData::Instance->IsCoordInMap(x, y))
	{
		int pos = x + y * CMapData::Instance->MapWidthPlusHeight;
		int loops[3] = { 0, -1, 1 };
		for (int i : loops)
			for (int e : loops)
			{
				if (!CMapData::Instance->IsCoordInMap(x + i, y + e))
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
	if (CMapData::Instance->IsCoordInMap(x, y))
	{
		auto oriCell = CMapData::Instance->GetCellAt(x, y);
		int oriGround = oriCell->TileIndex;
		if (oriGround == 0xFFFF) oriGround = 0;
		const int loops[3] = { 0, -1, 1 };
		for (int i : loops)
			for (int e : loops)
			{
				if (!CMapData::Instance->IsCoordInMap(x + i, y + e))
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
		if (!CMapData::Instance->IsCoordInMap(newX, newY)) continue;
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

	if (!CMapData::Instance().IsCoordInMap(X, Y)) return 0x464AE4;
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
					return CMapData::Instance->TryGetCellAt(x, y - 1);
				};
			auto getSE = [](int x, int y)
				{
					return CMapData::Instance->TryGetCellAt(x, y + 1);
				};
			auto getNE = [](int x, int y)
				{
					return CMapData::Instance->TryGetCellAt(x - 1, y);
				};
			auto getSW = [](int x, int y)
				{
					return CMapData::Instance->TryGetCellAt(x + 1, y);
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
				CheckCellRise(false, 0, false);
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
							if (!CMapData::Instance->IsCoordInMap(pos)) continue;
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
				if (!CMapData::Instance->IsCoordInMap(X + f, Y + n)) continue;

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
		CheckCellRise((nFlags & MK_SHIFT) && !IgnoreMorphable, 0, IgnoreMorphable);
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
						if (!CMapData::Instance->IsCoordInMap(pos)) continue;
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
	::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);

	CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);
	CMapData::Instance->DoUndo();
	return 0x46686A;
	//return 0x464AE4;
}
// already TakeSnapshot()
DEFINE_HOOK(464B01, CIsoView_OnLButtonDown_ACTIONMODE_LOWER, 7)
{
	GET(int, X, EDI);
	GET(int, Y, ESI);
	if (!CMapData::Instance().IsCoordInMap(X, Y)) return 0x46555F;
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
					return CMapData::Instance->TryGetCellAt(x, y - 1);
				};
			auto getSE = [](int x, int y)
				{
					return CMapData::Instance->TryGetCellAt(x, y + 1);
				};
			auto getNE = [](int x, int y)
				{
					return CMapData::Instance->TryGetCellAt(x - 1, y);
				};
			auto getSW = [](int x, int y)
				{
					return CMapData::Instance->TryGetCellAt(x + 1, y);
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
								if (!CMapData::Instance->IsCoordInMap(pos)) continue;

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
				CheckCellLow(false, 0, false);
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
							if (!CMapData::Instance->IsCoordInMap(pos)) continue;
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
				if (!CMapData::Instance->IsCoordInMap(X + f, Y + n)) continue;

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
		CheckCellLow((nFlags & MK_SHIFT) && !IgnoreMorphable, 0, IgnoreMorphable);
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
						if (!CMapData::Instance->IsCoordInMap(pos)) continue;
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

	::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
	CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);
	CMapData::Instance->DoUndo();
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
			if (!CMapData::Instance->IsCoordInMap(X + f, Y + n)) continue;
			int pos = X + f + (Y + n) * mapData.MapWidthPlusHeight;
			ignoreList.push_back(pos);
			AdjustHeightAt(X + f, Y + n, 1);
			CMapDataExt::CellDataExts[pos].Adjusted = true;
		}
	}
	if (nFlags & MK_CONTROL)
	{
		CheckCellRise(false, 0, false, ignoreList);
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
						if (!CMapData::Instance->IsCoordInMap(pos)) continue;
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


	::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
	CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);
	CMapData::Instance->DoUndo();
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
			if (!CMapData::Instance->IsCoordInMap(X + f, Y + n)) continue;
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
						if (!CMapData::Instance->IsCoordInMap(pos)) continue;
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
		CheckCellLow(false, 0, false, ignoreList);
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
						if (!CMapData::Instance->IsCoordInMap(pos)) continue;
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
	::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
	CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);
	CMapData::Instance->DoUndo();
	return 0x46686A;
	//return 0x4663D3;
}

DEFINE_HOOK(45B523, CIsoView_OnMouseMove_SkipShift, 7)
{
	if (CIsoView::CurrentCommand->Command == 11 || CIsoView::CurrentCommand->Command == 12) {
		SendMessage(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd, 0x401, 0,
			(LPARAM)Translations::TranslateOrDefault("HeightenAndLowerMessage",
				"Ctrl: Raise/lower same tileset, Shift: Steep slope, Ctrl+Shift:  Ignore non-morphable tiles"));
		::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
		::UpdateWindow(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd);
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
		std::vector<int> random = { 0,0,0,0,1 };
		if (STDHelpers::RandomSelectInt(random) == 0)
			return 0x45BF33;
	}

	auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
	int height = pIsoView->FlattenHeight;
	auto point = pIsoView->GetCurrentMapCoord(pIsoView->MouseCurrentPosition);
	int X = point.X;
	int Y = point.Y;

	if (!CMapData::Instance().IsCoordInMap(X, Y)) return 0x45BF33;

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
			if (!CMapData::Instance->IsCoordInMap(X + f, Y + n)) continue;
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
	CheckCellRise((CIsoViewExt::nFlagsMove & MK_SHIFT) && !IgnoreMorphable, 0, IgnoreMorphable);
	CheckCellLow((CIsoViewExt::nFlagsMove & MK_SHIFT) && !IgnoreMorphable, 0, IgnoreMorphable);
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
					if (!CMapData::Instance->IsCoordInMap(pos)) continue;
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


	::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
	
	return 0x45BF33;
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
	return 0;
}

