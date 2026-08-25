#include "Body.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <queue>
#include <CLoading.h>

namespace
{
	enum PathPassage : unsigned
	{
		Pass_None = 0,
		Pass_Land = 1 << 0,
		Pass_Sea = 1 << 1,
		Pass_Beach = 1 << 2,
		Pass_Rail = 1 << 3,
	};

	constexpr double Step_Straight = 1.0;
	constexpr double Step_Diagonal = 1.41421356237309504880;
	constexpr double Destroy_Penalty = 2.0;

	struct PathLevelInfo
	{
		unsigned Passage = Pass_None;
		int Height = 0;
	};

	struct PathCellInfo
	{
		PathLevelInfo Levels[2]{};
		int LevelCount = 1;
		bool Destructible = false;
		bool Crushable = false;
		unsigned DestroyedPassage = Pass_None;
	};

	unsigned TerrainToPassage(LandType landType)
	{
		switch (landType)
		{
		case Clear0:
		case Ice1:
		case Ice2:
		case Ice3:
		case Ice4:
		case Road11:
		case Road12:
		case Clear13:
		case Rough:
			return Pass_Land;
		case Railroad:
			return Pass_Land | Pass_Rail;
		case Water:
			return Pass_Sea;
		case Beach:
			return Pass_Beach;
		case Tunnel:
			return Pass_Land | Pass_Sea;
		default:
			return Pass_None;
		}
	}

	PathCellInfo BuildPathCellInfo(int X, int Y)
	{
		auto pThis = CMapDataExt::GetExtension();
		PathCellInfo info;
		CellData* cell = pThis->GetCellAt(X, Y);
		int tileIndex = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
		unsigned terrainPassage = TerrainToPassage(CMapDataExt::GetLandType(tileIndex, cell->TileSubIndex));
		int height = cell->Height;
		info.DestroyedPassage = terrainPassage;

		WORD overlay = pThis->GetOverlayAt(pThis->GetCoordIndex(X, Y));
		bool hasBridge = false;
		int bridgeHeight = 0;
		if (overlay != 0xFFFF)
		{
			const auto typeData = CMapDataExt::GetOverlayTypeData(overlay);
			if (typeData.Rock || typeData.TerrainRock)
			{
				terrainPassage = Pass_None;
			}
			else if (typeData.RailRoad)
			{
				terrainPassage = Pass_Land | Pass_Rail;
			}
			else if (typeData.Crushable && typeData.Wall)
			{
				info.Crushable = true;
				info.Destructible = true;
				terrainPassage = Pass_None;
			}
			else if (typeData.Wall)
			{
				info.Destructible = true;
				terrainPassage = Pass_None;
			}
			else if (typeData.Road)
			{
				info.Destructible = true;
				terrainPassage = Pass_Land;
			}
			else if (typeData.Overrides)
			{
				hasBridge = true;
				bridgeHeight = height + 4;
			}
		}

		if (!hasBridge)
		{
			constexpr int NWSE_Dirs[2][2] = { {-1, 0}, {1, 0} };
			constexpr int NESW_Dirs[2][2] = { {0, -1}, {0, 1} };

			for (const auto& dir : NWSE_Dirs)
			{
				int nx = X + dir[0];
				int ny = Y + dir[1];
				if (!pThis->IsCoordInMap(nx, ny))
					continue;
				WORD nOverlay = pThis->GetOverlayAt(pThis->GetCoordIndex(nx, ny));
				if (nOverlay == 0xFFFF)
					continue;
				if (!CMapDataExt::GetOverlayTypeData(nOverlay).Overrides)
					continue;
				CellData* nCell = pThis->GetCellAt(nx, ny);
				if (nCell->OverlayData <= 8) // NW-SE
				{
					hasBridge = true;
					bridgeHeight = nCell->Height + 4;
					break;
				}
			}

			if (!hasBridge)
			{
				for (const auto& dir : NESW_Dirs)
				{
					int nx = X + dir[0];
					int ny = Y + dir[1];
					if (!pThis->IsCoordInMap(nx, ny))
						continue;
					WORD nOverlay = pThis->GetOverlayAt(pThis->GetCoordIndex(nx, ny));
					if (nOverlay == 0xFFFF)
						continue;
					if (!CMapDataExt::GetOverlayTypeData(nOverlay).Overrides)
						continue;
					CellData* nCell = pThis->GetCellAt(nx, ny);
					if (nCell->OverlayData >= 9 && nCell->OverlayData <= 17) // NE-SW
					{
						hasBridge = true;
						bridgeHeight = nCell->Height + 4;
						break;
					}
				}
			}
		}

		if (hasBridge)
		{
			info.LevelCount = 2;
			info.Levels[0] = { terrainPassage, height };
			info.Levels[1] = { Pass_Land, bridgeHeight };
			return info;
		}

		info.LevelCount = 1;
		info.Levels[0] = { terrainPassage, height };
		return info;
	}

	bool IsCliffBackCell(CMapDataExt* pThis, int X, int Y)
	{
		CellData* cell = pThis->GetCellAt(X, Y);
		int tileIndex = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
		LandType landType = CMapDataExt::GetLandType(tileIndex, cell->TileSubIndex);
		if (landType != Clear0 && landType != Clear13 && landType != Water)
			return false;

		WORD overlay = pThis->GetOverlayAt(pThis->GetCoordIndex(X, Y));
		if (overlay != 0xFFFF)
		{
			const auto typeData = CMapDataExt::GetOverlayTypeData(overlay);
			if (typeData.Road || typeData.RailRoad)
				return false;
		}

		constexpr int Offsets[2] = { 1, 2 };
		for (int offset : Offsets)
		{
			int X2 = X + offset;
			int Y2 = Y + offset;
			if (!pThis->IsCoordInMap(X2, Y2))
				continue;

			CellData* cell2 = pThis->GetCellAt(X2, Y2);
			if (cell2->Height < cell->Height + 4)
				continue;

			int tileIndex2 = CMapDataExt::GetSafeTileIndex(cell2->TileIndex);
			LandType landType2 = CMapDataExt::GetLandType(tileIndex2, cell2->TileSubIndex);
			if (landType2 == Rock7 || landType2 == Rock8 || landType2 == CliffRock)
				return true;
		}
		return false;
	}
}

std::vector<MapCoord> CMapDataExt::FindPath(MapCoord from, MapCoord to, PathfindingMoveType type, PathfindingObjectType objectType,
	bool destroyOverlay, bool ignoreBuilding, bool ignoreTree, std::vector<unsigned char>* outLevels,
	bool noCliffBack, std::vector<unsigned char>* outHeights, bool* outReachable)
{
	std::vector<MapCoord> result;
	auto pThis = GetExtension();
	if (outReachable)
		*outReachable = false;
	if (!pThis->IsCoordInMap(from.X, from.Y) || !pThis->IsCoordInMap(to.X, to.Y))
		return result;

	unsigned required = 0;
	bool canDestroy = false;
	switch (type)
	{
	case PathfindingMoveType::NormalLand:
		required = Pass_Land;
		break;
	case PathfindingMoveType::DestructiveLand:
		required = Pass_Land;
		canDestroy = true;
		break;
	case PathfindingMoveType::NormalSea:
		required = Pass_Sea;
		break;
	case PathfindingMoveType::DestructiveSea:
		required = Pass_Sea;
		canDestroy = true;
		break;
	case PathfindingMoveType::NormalAmphibian:
		required = Pass_Land | Pass_Sea | Pass_Beach;
		break;
	case PathfindingMoveType::DestructiveAmphibian:
		required = Pass_Land | Pass_Sea | Pass_Beach;
		canDestroy = true;
		break;
	case PathfindingMoveType::Train:
		required = Pass_Rail;
		break;
	default:
		return result;
	}

	canDestroy = canDestroy || (destroyOverlay && type != PathfindingMoveType::Train);

	auto cellBlockedByObjects = [&](int X, int Y) -> bool
	{
		CellData* cell = pThis->GetCellAt(X, Y);
		if (!ignoreBuilding && cell->Structure != -1)
			return true;
		if (cell->Terrain > -1
			&& cell->Terrain < CMapData::Instance->TerrainDatas.size())
		{
			auto& name = CMapData::Instance->TerrainDatas[cell->Terrain].TypeID;
			ppmfc::CString key = CLoading::Instance->TheaterIdentifier == 'A' ?
				"SnowOccupationBits" : "TemperateOccupationBits";
			if (!ignoreTree && objectType == PathfindingObjectType::Vehicle)
				return true;
			if (!ignoreTree && objectType == PathfindingObjectType::Infantry
				&& Variables::RulesMap.GetInteger(name, key, 7) >= 7)
				return true;
			if (Variables::RulesMap.GetBool(name, "SpawnsTiberium"))
				return true;
		}
		if (noCliffBack && IsCliffBackCell(pThis, X, Y))
			return true;
		return false;
	};

	if (cellBlockedByObjects(from.X, from.Y))
		return result;

	{
		const PathCellInfo& startInfo = BuildPathCellInfo(from.X, from.Y);
		bool startPassable = false;
		for (int level = 0; level < startInfo.LevelCount; ++level)
		{
			if (startInfo.Levels[level].Passage & required)
			{
				startPassable = true;
				break;
			}
			if (startInfo.Crushable && objectType == PathfindingObjectType::Vehicle
				&& (startInfo.DestroyedPassage & required))
			{
				startPassable = true;
				break;
			}
			if (canDestroy && startInfo.Destructible && (startInfo.DestroyedPassage & required))
			{
				startPassable = true;
				break;
			}
		}
		if (!startPassable)
			return result;
	}

	result.push_back(from);
	if (outLevels)
		outLevels->push_back(0);
	if (outHeights)
		outHeights->push_back(pThis->GetCellAt(from.X, from.Y)->Height);
	if (from.X == to.X && from.Y == to.Y)
	{
		if (outReachable)
			*outReachable = true;
		return result;
	}
	result.clear();
	if (outLevels)
		outLevels->clear();
	if (outHeights)
		outHeights->clear();

	const int posCount = pThis->MapWidthPlusHeight * pThis->MapWidthPlusHeight;
	const int startPos = pThis->GetCoordIndex(from.X, from.Y);
	const int endPos = pThis->GetCoordIndex(to.X, to.Y);
	const double infinity = std::numeric_limits<double>::max();

	std::vector<PathCellInfo> cells(posCount);
	std::vector<char> cellReady(posCount, 0);
	std::vector<double> dist(posCount * 2, infinity);
	std::vector<int> prevState(posCount * 2, -1);
	std::vector<int> prevTube(posCount * 2, -1);

	auto getCellInfo = [&](int pos) -> PathCellInfo&
	{
		if (!cellReady[pos])
		{
			cells[pos] = BuildPathCellInfo(pThis->GetXFromCoordIndex(pos), pThis->GetYFromCoordIndex(pos));
			cellReady[pos] = 1;
		}
		return cells[pos];
	};

	auto cellGroundPassable = [&](int X, int Y, bool allowDestroy) -> bool
	{
		const PathCellInfo& info = getCellInfo(pThis->GetCoordIndex(X, Y));
		const PathLevelInfo& level = info.Levels[0];
		if (level.Passage & required)
			return true;
		if (allowDestroy && canDestroy && info.Destructible && (info.DestroyedPassage & required))
			return true;
		return false;
	};

	struct TunnelJump
	{
		int StartPos;
		int EndPos;
		int TubeIndex;
		bool Reversed;
		double Cost;
	};

	std::vector<TunnelJump> tunnelJumps;
	if (required != Pass_Rail)
	{
		const auto& tubes = CMapDataExt::Tubes;
		for (int i = 0; i < (int)tubes.size(); ++i)
		{
			const auto& tube = tubes[i];
			if (!pThis->IsCoordInMap(tube.StartCoord.X, tube.StartCoord.Y)
				|| !pThis->IsCoordInMap(tube.EndCoord.X, tube.EndCoord.Y))
				continue;
			if (tube.PathCoords.size() < 2)
				continue;

			auto posInMap = [&](const MapCoord& coord) -> bool
			{
				return pThis->IsCoordInMap(coord.X, coord.Y);
			};

			bool startPassable = posInMap(tube.StartCoord) && cellGroundPassable(tube.StartCoord.X, tube.StartCoord.Y, false);
			bool endPassable = posInMap(tube.EndCoord) && cellGroundPassable(tube.EndCoord.X, tube.EndCoord.Y, false);
			if (!startPassable || !endPassable)
				continue;

			double cost = 0.0;
			for (size_t k = 1; k < tube.PathCoords.size(); ++k)
			{
				int dx = tube.PathCoords[k].X - tube.PathCoords[k - 1].X;
				int dy = tube.PathCoords[k].Y - tube.PathCoords[k - 1].Y;
				cost += (dx != 0 && dy != 0) ? Step_Diagonal : Step_Straight;
			}

			tunnelJumps.push_back({ pThis->GetCoordIndex(tube.StartCoord.X, tube.StartCoord.Y),
				pThis->GetCoordIndex(tube.EndCoord.X, tube.EndCoord.Y), i, false, cost });
			tunnelJumps.push_back({ pThis->GetCoordIndex(tube.EndCoord.X, tube.EndCoord.Y),
				pThis->GetCoordIndex(tube.StartCoord.X, tube.StartCoord.Y), i, true, cost });
		}
	}

	using QueueEntry = std::pair<double, int>;
	std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> queue;

	dist[startPos * 2] = 0.0;
	queue.emplace(0.0, startPos * 2);

	constexpr int DirX[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };
	constexpr int DirY[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };

	int foundState = -1;
	while (!queue.empty())
	{
		const QueueEntry entry = queue.top();
		queue.pop();
		const double distCur = entry.first;
		const int stateCur = entry.second;
		if (distCur > dist[stateCur])
			continue;

		const int posCur = stateCur / 2;
		const int levelCur = stateCur % 2;
		if (posCur == endPos && levelCur == 0)
		{
			foundState = stateCur;
			break;
		}

		if (levelCur == 0)
		{
			for (const auto& jump : tunnelJumps)
			{
				if (jump.StartPos != posCur)
					continue;
				const int stateNext = jump.EndPos * 2;
				const double distNext = distCur + jump.Cost;
				if (distNext < dist[stateNext])
				{
					dist[stateNext] = distNext;
					prevState[stateNext] = stateCur;
					prevTube[stateNext] = jump.TubeIndex;
					queue.emplace(distNext, stateNext);
				}
			}
		}

		const PathCellInfo& curInfo = getCellInfo(posCur);
		const PathLevelInfo& curLevel = curInfo.Levels[levelCur];
		const int xCur = pThis->GetXFromCoordIndex(posCur);
		const int yCur = pThis->GetYFromCoordIndex(posCur);

		for (int i = 0; i < 8; ++i)
		{
			const int xNext = xCur + DirX[i];
			const int yNext = yCur + DirY[i];
			if (!pThis->IsCoordInMap(xNext, yNext))
				continue;
			const int posNext = pThis->GetCoordIndex(xNext, yNext);
			const PathCellInfo& nextInfo = getCellInfo(posNext);
			const double step = (DirX[i] != 0 && DirY[i] != 0) ? Step_Diagonal : Step_Straight;

			for (int levelNext = 0; levelNext < nextInfo.LevelCount; ++levelNext)
			{
				const PathLevelInfo& nextLevel = nextInfo.Levels[levelNext];
				const int dh = nextLevel.Height - curLevel.Height;
				if (dh < -1 || dh > 1)
					continue;

				if (levelNext == 0 && posNext != startPos)
				{
					if (cellBlockedByObjects(xNext, yNext))
						continue;
				}

				double moveCost;
				if (nextInfo.Crushable && objectType == PathfindingObjectType::Vehicle
					&& (nextInfo.DestroyedPassage & required))
					moveCost = step;
				else if (nextLevel.Passage & required)
					moveCost = step;
				else if (canDestroy && nextInfo.Destructible && (nextInfo.DestroyedPassage & required))
					moveCost = step + Destroy_Penalty;
				else
					continue;

				const int stateNext = posNext * 2 + levelNext;
				const double distNext = distCur + moveCost;
				if (distNext < dist[stateNext])
				{
					dist[stateNext] = distNext;
					prevState[stateNext] = stateCur;
					prevTube[stateNext] = -1;
					queue.emplace(distNext, stateNext);
				}
			}
		}
	}

	if (foundState >= 0)
	{
		if (outReachable)
			*outReachable = true;

		std::vector<int> levels;
		std::vector<int> heights;
		for (int s = foundState; s >= 0; s = prevState[s])
		{
			MapCoord coord;
			coord.X = pThis->GetXFromCoordIndex(s / 2);
			coord.Y = pThis->GetYFromCoordIndex(s / 2);
			if (result.empty() || result.back().X != coord.X || result.back().Y != coord.Y)
			{
				result.push_back(coord);
				levels.push_back(s % 2);
				CellData* cell = pThis->GetCellAt(coord.X, coord.Y);
				heights.push_back(cell->Height + (s % 2 == 1 ? 4 : 0));
			}
			if (prevTube[s] >= 0)
			{
				const auto& tube = CMapDataExt::Tubes[prevTube[s]];
				const int prevPos = prevState[s] / 2;
				const int prevX = pThis->GetXFromCoordIndex(prevPos);
				const int prevY = pThis->GetYFromCoordIndex(prevPos);
				const bool forward = tube.StartCoord.X == prevX && tube.StartCoord.Y == prevY;
				const int tunnelHeight = std::min(
					pThis->GetCellAt(tube.StartCoord.X, tube.StartCoord.Y)->Height,
					pThis->GetCellAt(tube.EndCoord.X, tube.EndCoord.Y)->Height);
				if (forward)
				{
					for (int k = (int)tube.PathCoords.size() - 2; k >= 1; --k)
					{
						MapCoord mc = tube.PathCoords[k];
						if (result.back().X != mc.X || result.back().Y != mc.Y)
						{
							result.push_back(mc);
							levels.push_back(0);
							heights.push_back(tunnelHeight);
						}
					}
				}
				else
				{
					for (int k = 1; k <= (int)tube.PathCoords.size() - 2; ++k)
					{
						MapCoord mc = tube.PathCoords[k];
						if (result.back().X != mc.X || result.back().Y != mc.Y)
						{
							result.push_back(mc);
							levels.push_back(0);
							heights.push_back(tunnelHeight);
						}
					}
				}
			}
		}
		std::reverse(result.begin(), result.end());
		std::reverse(levels.begin(), levels.end());
		std::reverse(heights.begin(), heights.end());
		if (outLevels)
			*outLevels = std::vector<unsigned char>(levels.begin(), levels.end());
		if (outHeights)
			*outHeights = std::vector<unsigned char>(heights.begin(), heights.end());
	}
	return result;
}
