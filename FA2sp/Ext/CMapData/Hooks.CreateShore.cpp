#include "Body.h"
#include <Helpers/Macro.h>
#include "../../FA2sp.h"
#include "../../Helpers/TheaterHelpers.h"
#include "../../Helpers/STDHelpers.h"
#include <Miscs/Miscs.h>
#include "../CIsoView/Body.h"
#include "../CFinalSunDlg/Body.h"
#include "../../Miscs/TheaterInfo.h"

DEFINE_HOOK(4BC490, CMapData_CreateShore, 7)
{
	GET(CMapDataExt*, pThis, ECX);
	GET_STACK(int, startX, 0x4);
	GET_STACK(int, startY, 0x8);
	GET_STACK(int, right, 0xC);
	GET_STACK(int, bottom, 0x10);
	GET_STACK(BOOL, bRemoveUseless, 0x14);
	// be warn Width matchs Y

	auto cellDatas = pThis->CellDatas;
	auto& theater = CINI::CurrentTheater();

	int shorePieces = pThis->AutoShore_ShoreTileSet;
	int greenTiles = pThis->AutoShore_GreenTileSet;
	int waterSet = pThis->WaterSet;
	int waterBridge = pThis->WaterBridge;
	if (shorePieces < 0 || shorePieces >(int)CMapDataExt::TileSet_starts.size()
		|| greenTiles > (int)CMapDataExt::TileSet_starts.size()
		|| waterSet < 0 || waterSet >(int)CMapDataExt::TileSet_starts.size())
		return 0x4C2FC5;

	int tileStart = CMapDataExt::TileSet_starts[shorePieces];
	int tileEnd = CMapDataExt::TileSet_starts[shorePieces + 1] - 1;
	if (tileEnd - tileStart < 39 || tileEnd >= CMapDataExt::TileDataCount)
		return 0x4C2FC5;

	int waterSetStart = CMapDataExt::TileSet_starts[waterSet];
	int waterSetEnd = CMapDataExt::TileSet_starts[waterSet + 1] - 1;
	int greenTile = -1;
	if (greenTiles >= 0)
		greenTile = CMapDataExt::TileSet_starts[greenTiles];
	//  last two big shores and water bridges
	std::vector<int> SpecialShores;
	for (int i = tileStart; i <= tileEnd; i++)
	{
		const auto& tile = CMapDataExt::TileData[i];
		if (tile.Width + tile.Height > 6)
			SpecialShores.push_back(i);
	}

	if (waterBridge >= 0)
	{
		for (int i = 0; i < 2; i++)
			SpecialShores.push_back(i + CMapDataExt::TileSet_starts[waterBridge]);
	}

	std::map<int, std::map<int, bool>> hackedShores;
	if (CMapDataExt::AutoShore_ShoreTileSet != CMapDataExt::ShorePieces)
	{
		if (auto pSection = CINI::FAData->GetSection("ShoreTerrainRA2"))
		{
			for (const auto& [key, value] : pSection->GetEntities())
			{
				auto atoms = STDHelpers::SplitString(key, 1, "_");
				int tileIdx = atoi(atoms[0]) + tileStart;
				int tileSubIdx = atoi(atoms[1]);
				hackedShores[tileIdx][tileSubIdx] = atoi(value) > 0 ? true : false;
			}
		}
	}

	// a trick to avoid affecting other shorelines
	// ignore the working shore
	auto tileNameHasShore = [&](int setIdx)
		{
			if (setIdx == shorePieces)
				return false;
			auto& ret = CMapDataExt::ShoreTileSets;
			
			if (ret.find(setIdx) != ret.end())
				return true;
			return false;
		};

	auto getAltLandType = [&](int tileIndex, int TileSubIndex)
		{
			if (tileIndex == 0xFFFF)
				tileIndex = 0;

			if (hackedShores.find(tileIndex) != hackedShores.end())
			{
				auto& tiles = hackedShores[tileIndex];
				if (tiles.find(TileSubIndex) != tiles.end())
				{
					if (tiles[TileSubIndex])
					{
						return LandType::Water;
					}
					else
					{
						return LandType::Rough;
					}
				}
			}
			return CMapDataExt::TileData[tileIndex].TileBlockDatas[TileSubIndex].TerrainTypeAlt;
		};

	auto tileIsClear = [&](int tileIdx, int tileSubIdx)
		{
			if (tileIdx == 0xFFFF)
				tileIdx = 0;
			auto ttype = getAltLandType(tileIdx, tileSubIdx);
			if (ttype == LandType::Water)
				return false;
			auto& ret = CMapDataExt::SoftTileSets;
			if (ret.find(CMapDataExt::TileData[tileIdx].TileSet) != ret.end())
				return ret[CMapDataExt::TileData[tileIdx].TileSet];

			if (ttype == LandType::Clear13 
				|| ttype == LandType::Clear0
				|| ttype == LandType::Clear1)
				return true;

			return false;
		};

	std::vector<int> tiles_2x3;
	std::vector<int> tiles_3x2;
	std::vector<int> tiles_2x2;
	std::vector<int> tiles_2x2Corners;
	std::vector<int> tiles_1x2;
	std::vector<int> tiles_2x1;

	// 1 means rough (or land), 2 means beach (or water)
	int shoreMatch_2x3[2][3] = { 0 };
	int shoreMatch_3x2[3][2] = { 0 };
	int shoreMatch_2x2[2][2] = { 0 };
	int shoreMatch_1x2[1][2] = { 0 };
	int shoreMatch_2x1[2][1] = { 0 };

	for (int i = tileStart; i <= tileEnd; i++)
	{
		auto const& tile = CMapDataExt::TileData[i];
		
		if (tile.Width == 2 && tile.Height == 3)
			tiles_2x3.push_back(i);
		else if (tile.Width == 3 && tile.Height == 2)
			tiles_3x2.push_back(i);
		else if (tile.Width == 2 && tile.Height == 2)
		{
			int beachCount = 0;
			for (int m = 0; m < 2; m++)
			{
				for (int n = 0; n < 2; n++)
				{
					int subTileidx = n * 2 + m;
					auto ttype = getAltLandType(i, subTileidx);
					if (ttype == LandType::Water)
						beachCount++;
				}
			}
			if (beachCount == 1 || beachCount == 3)
			{
				tiles_2x2Corners.push_back(i);
			}
			else
				tiles_2x2.push_back(i);
		}

		else if (tile.Width == 1 && tile.Height == 2)
			tiles_1x2.push_back(i);
		else if (tile.Width == 2 && tile.Height == 1)
			tiles_2x1.push_back(i);
	}

	for (int x = startX - 2; x < right + 2; x++)
	{
		for (int y = startY - 2; y < bottom + 2; y++)
		{
			if (!pThis->IsCoordInMap(x, y))
				continue;

			int pos = pThis->GetCoordIndex(x, y);
			pThis->CellDataExts[pos].ShoreProcessed = false;
			pThis->CellDataExts[pos].ShoreLATNeeded = false;

			auto cell = pThis->GetCellAt(x, y);
			if (ExtConfigs::PlaceTileSkipHide && cell->IsHidden())
				pThis->CellDataExts[pos].ShoreProcessed = true;
		}
	}

	for (int x = startX; x < right; x++)
	{
		for (int y = startY; y < bottom; y++)
		{
			if (!pThis->IsCoordInMap(x, y))
				continue;

			auto cell = pThis->GetCellAt(x, y);

			if (ExtConfigs::PlaceTileSkipHide && cell->IsHidden())
				continue;

			int tileIndex = cell->TileIndex;
			if (tileIndex == 0xFFFF)
				tileIndex = 0;

			// remove broken beaches
			if ((tileIndex >= tileStart && tileIndex <= tileEnd) && !pThis->IsTileIntact(x, y))
			{
				auto ttype = getAltLandType(tileIndex, cell->TileSubIndex);
				if (tileIsClear(tileIndex, cell->TileSubIndex))
				{
					int replaceTile = greenTile >= 0 ? greenTile : 0;
					cell->TileIndex = replaceTile;
					cell->TileSubIndex = 0;
					cell->Flag.AltIndex = STDHelpers::RandomSelectInt(0, CMapDataExt::TileData[replaceTile].AltTypeCount + 1);
				}
				else if (ttype == LandType::Water)
				{
					cell->TileIndex = STDHelpers::RandomSelectInt(TheaterInfo::CurrentSmallWaters);
					cell->TileSubIndex = 0;
					cell->Flag.AltIndex = 0;
				}
			}
			// keep special shores
			else if (std::find(SpecialShores.begin(), SpecialShores.end(), tileIndex) != SpecialShores.end() && pThis->IsTileIntact(x, y))
			{
				for (auto& mc : pThis->GetIntactTileCoords(x, y, true))
				{
					if (!pThis->IsCoordInMap(mc.X, mc.Y))
						continue;
					int pos = pThis->GetCoordIndex(mc.X, mc.Y);
					auto& cellExt = pThis->CellDataExts[pos];

					cellExt.ShoreProcessed = true;
				}
			}
		}
	}


	// remove 1x1 land and water
	// only used in bmp2map, not necessary
	if (bRemoveUseless)
	{

	}

	auto process = [&](int w, int h, std::vector<int>tiles, int* shoreMatch)
		{
			for (int x = startX; x < right; x++)
			{
				for (int y = startY; y < bottom; y++)
				{
					if (!pThis->IsCoordInMap(x, y))
						continue;

					int pos = pThis->GetCoordIndex(x, y);
					auto cell = pThis->GetCellAt(pos);

					if (ExtConfigs::PlaceTileSkipHide && cell->IsHidden())
						continue;

					auto& cellExt = pThis->CellDataExts[pos];

					if (cellExt.ShoreProcessed) continue;

					std::vector<int> targetBeachTiles;

					bool breakCheck = false;
					int oriHeight = cell->Height;
					for (int m = 0; m < w; m++)
					{
						if (breakCheck) break;
						for (int n = 0; n < h; n++)
						{
							if (breakCheck) break;
							if (!pThis->IsCoordInMap(x + n, y + m))
							{
								shoreMatch[m * h + n] = 0;
								continue;
							}

							int pos2 = pThis->GetCoordIndex(x + n, y + m);
							auto cell2 = pThis->GetCellAt(pos2);

							auto& cellExt2 = pThis->CellDataExts[pos2];
							int tileIndex = cell2->TileIndex;
							if (tileIndex == 0xFFFF)
								tileIndex = 0;

							// skip intact tiles on the edges
							if (x + n < startX + 1 || y + m < startY + 1 || x + n >= right - 1 || y + m >= bottom - 1)
							{
								if (pThis->IsTileIntact(x + n, y + m) && !pThis->IsTileIntact(x + n, y + m, startX, startY, right, bottom))
								{
									for (auto& mc : pThis->GetIntactTileCoords(x + n, y + m, true))
									{
										int pos3 = pThis->GetCoordIndex(mc.X, mc.Y);
										auto& cellExt3 = pThis->CellDataExts[pos3];
										cellExt3.ShoreProcessed = true;
									}
								}
							}

							if (CMapDataExt::TileData[tileIndex].TileBlockDatas[cell2->TileSubIndex].RampType != 0)
								cellExt2.ShoreProcessed = true;

							if (cell2->Height != oriHeight)
								cellExt2.ShoreProcessed = true;
	
							if (cellExt2.ShoreProcessed)
								breakCheck = true;

							auto ttype = getAltLandType(tileIndex, cell2->TileSubIndex);

							if (tileIsClear(tileIndex, cell2->TileSubIndex))
							{
								auto tile = CMapDataExt::TileData[tileIndex];
								bool skip = false;
								// check cliffs with beachs
								for (int m = 0; m < tile.Width; m++)
								{
									for (int n = 0; n < tile.Height; n++)
									{
										int subIdx = n * tile.Width + m;

										auto ttype2 = getAltLandType(tileIndex, subIdx);
										if (ttype2 == LandType::Cliff)
											skip = true;
									}
								}
								if (tileNameHasShore(tile.TileSet))
									skip = true;
								if (!skip)
									shoreMatch[m * h + n] = 1;
								else
									shoreMatch[m * h + n] = 0;
							}
							else if ((tileIndex >= tileStart && tileIndex <= tileEnd && ttype == LandType::Water) 
								|| (tileIndex >= waterSetStart && tileIndex <= waterSetEnd && ttype == LandType::Water))
								shoreMatch[m * h + n] = 2;
							else 
								shoreMatch[m * h + n] = 0;

							if (ExtConfigs::PlaceTileSkipHide && cell2->IsHidden())
								shoreMatch[m * h + n] = 0;
						}
					}
					if (breakCheck) continue;
					for (auto index : tiles)
					{
						bool match = true;
						for (int m = 0; m < w; m++)
						{
							for (int n = 0; n < h; n++)
							{
								int subTileidx = n * w + m;
								auto ttype = getAltLandType(index, subTileidx);
								int thisType = -1;
								if (tileIsClear(index, subTileidx))
									thisType = 1;
								if (ttype == LandType::Water)
									thisType = 2;

								if (shoreMatch[m * h + n] != thisType)
									match = false;
							}
						}
						if (match)
							targetBeachTiles.push_back(index);
					}
					if (!targetBeachTiles.empty())
					{
						int targetBeachTile = STDHelpers::RandomSelectInt(targetBeachTiles);
						for (int m = 0; m < w; m++)
						{
							for (int n = 0; n < h; n++)
							{
								if (!pThis->IsCoordInMap(x + n, y + m))
									continue;

								int pos2 = pThis->GetCoordIndex(x + n, y + m);
								auto cell2 = pThis->GetCellAt(pos2);

								if (ExtConfigs::PlaceTileSkipHide && cell2->IsHidden())
									continue;

								auto& cellExt2 = pThis->CellDataExts[pos2];

								cell2->TileIndex = targetBeachTile;
								cell2->TileSubIndex = n * w + m;
								cell->Flag.AltIndex = 0;
								cellExt2.ShoreProcessed = true;
							}
						}
					}

				}
			}
		};

	process(2, 3, tiles_2x3, &shoreMatch_2x3[0][0]);
	process(3, 2, tiles_3x2, &shoreMatch_3x2[0][0]);
	process(2, 2, tiles_2x2Corners, &shoreMatch_2x2[0][0]);
	process(2, 2, tiles_2x2, &shoreMatch_2x2[0][0]);
	process(1, 2, tiles_1x2, &shoreMatch_1x2[0][0]);
	process(2, 1, tiles_2x1, &shoreMatch_2x1[0][0]);

	// now add green tile around beaches
	for (int x = startX - 1; x < right + 1; x++)
	{
		for (int y = startY - 1; y < bottom + 1; y++)
		{
			if (!pThis->IsCoordInMap(x, y))
				continue;

			int pos = pThis->GetCoordIndex(x, y);
			auto cell = pThis->GetCellAt(pos);

			if (ExtConfigs::PlaceTileSkipHide && cell->IsHidden())
				continue;

			auto& cellExt = pThis->CellDataExts[pos];
			int tileIndex = cell->TileIndex;
			if (tileIndex == 0xFFFF)
				tileIndex = 0;

			if (CMapDataExt::TileData[tileIndex].TileBlockDatas[cell->TileSubIndex].RampType != 0)
				continue;

			auto tile = CMapDataExt::TileData[tileIndex];
			auto ttype = getAltLandType(tileIndex, cell->TileSubIndex);

			if ((tileIndex < tileStart || tileIndex > tileEnd)
				&& tileIsClear(tileIndex, cell->TileSubIndex))
			{
				bool skip = false;
				// check cliffs with beachs
				for (int m = 0; m < tile.Width; m++)
				{
					for (int n = 0; n < tile.Height; n++)
					{
						int subIdx = n * tile.Width + m;

						if (CMapDataExt::TileData[tileIndex].TileBlockDatas[subIdx].ImageData == NULL) {
							continue;
						}

						auto ttype2 = getAltLandType(tileIndex, subIdx);
						if (ttype2 == LandType::Cliff)
							skip = true;
					}
				}
				if (tileNameHasShore(tile.TileSet))
					skip = true;

				if (!skip)
				{
					const int loop[4][2] = { {0, -1},{0, 1},{1, 0},{-1, 0} };
					for (auto pair : loop)
					{
						int newX = pair[0] + x;
						int newY = pair[1] + y;

						if (!pThis->IsCoordInMap(newX, newY))
							continue;

						int pos2 = pThis->GetCoordIndex(newX, newY);
						auto cell2 = pThis->GetCellAt(pos2);
						auto& cellExt2 = pThis->CellDataExts[pos2];
						int tileIndex2 = cell2->TileIndex;
						if (tileIndex2 == 0xFFFF)
							tileIndex2 = 0;
						auto ttype2 = getAltLandType(tileIndex2, cell2->TileSubIndex);
						if (tileIndex2 >= tileStart && tileIndex2 <= tileEnd
							&& tileIsClear(tileIndex2, cell2->TileSubIndex)
							&& cellExt2.ShoreProcessed)
						{
							if (greenTile >= 0)
							{
								cell->TileIndex = greenTile;
								cell->TileSubIndex = 0;
								cell->Flag.AltIndex = STDHelpers::RandomSelectInt(0, CMapDataExt::TileData[greenTile].AltTypeCount + 1);
							}
							cellExt.ShoreLATNeeded = true;
							break;
						}
					}
				}
			}
		}
	}

	// LAT
	if (!CFinalSunApp::Instance->DisableAutoLat)
	{
		for (int x = startX - 1; x < right + 1; x++)
		{
			for (int y = startY - 1; y < bottom + 1; y++)
			{
				if (!pThis->IsCoordInMap(x, y))
					continue;

				auto cell = pThis->GetCellAt(x, y);
				if (ExtConfigs::PlaceTileSkipHide && cell->IsHidden())
					continue;

				bool next = false;
				for (int i = -1; i < 2; i++) {
					if (next) break;
					for (int j = -1; j < 2; j++) {
						if (next) break;
						int pos = pThis->GetCoordIndex(x + i, y + j);
						auto& cellExt = pThis->CellDataExts[pos];
						if (cellExt.ShoreLATNeeded) {
							next = true;
							pThis->SmoothTileAt(x, y, true);
						}
					}
				}
			}
		}
	}

	return 0x4C2FC5;
}


