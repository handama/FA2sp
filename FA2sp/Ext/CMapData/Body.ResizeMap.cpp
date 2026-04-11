#include "Body.h"
#include <CUpdateProgress.h>
#include "../../Helpers/Translations.h"
#include "../../ExtraWindow/CMeasurementToolbox/CMeasurementToolbox.h"

bool CMapDataExt::ResizeMapExt(MapRect* const pRect)
{	
	std::vector<FString> BuildingList;
	std::vector<FString> UnitList;
	std::vector<FString> AircraftList;
	std::vector<FString> InfantryList;
	std::map<FString, FString> WaypointList;
	std::map<FString, FString> CelltagList;
	CellData* oldCellDatas = CellDatas;

	int ow = Size.Width;
	int oh = Size.Height;
	int os = MapWidthPlusHeight;

	auto recordIniValue = [this](const char* lpName, std::vector<FString>& list)
	{
		if (auto pSection = INI.GetSection(lpName))
		{
			for (const auto& [key, value] : pSection->GetEntities())
			{
				list.push_back(value);
			}
		}
	};
	auto recordIniMap = [this](const char* lpName, std::map<FString, FString>& list)
	{
		if (auto pSection = INI.GetSection(lpName))
		{
			for (const auto& [key, value] : pSection->GetEntities())
			{
				list[key] = value;
			}
		}
	};

	recordIniValue("Structures", BuildingList);
	recordIniValue("Units", UnitList);
	recordIniValue("Aircraft", AircraftList);
	recordIniValue("Infantry", InfantryList);
	recordIniMap("Waypoints", WaypointList);
	recordIniMap("CellTags", CelltagList);

	CellDatas = NULL;
	CellDataCount = 0;
	CMapDataExt::UndoRedoDatas.clear();
	CMapDataExt::UndoRedoDataIndex = -1;

	ppmfc::CString mapSize;
	mapSize.Format("0,0,%d,%d", pRect->Width, pRect->Height);
	INI.WriteString("Map", "Size", mapSize);

	MapRect newLocalSize = LocalSize;
	newLocalSize.Width += pRect->Width - Size.Width;
	newLocalSize.Height += pRect->Height - Size.Height;

	mapSize.Format("%d,%d,%d,%d", newLocalSize.Left, newLocalSize.Top, newLocalSize.Width, newLocalSize.Height);
	INI.WriteString("Map", "LocalSize", mapSize);

	Size.Width = pRect->Width;
	Size.Height = pRect->Height;
	LocalSize = newLocalSize;
	MapWidthPlusHeight = Size.Width + Size.Height;

	CellDatas = GameCreateVector<CellData>((MapWidthPlusHeight + 1) * (MapWidthPlusHeight + 1));
	CellDataCount = (MapWidthPlusHeight + 1) * (MapWidthPlusHeight + 1);
	CellDataExts.clear();
	CellDataExts.resize(CellDataCount);

	if (IsoPackData != NULL) GameDeleteArray(IsoPackData, IsoPackDataCount);
	IsoPackData = NULL;
	IsoPackDataCount = 0;

	int x_move = 0;
	int y_move = 0;

	x_move += (pRect->Width - ow);
	x_move += pRect->Top;
	y_move += pRect->Top;
	x_move += -pRect->Left;
	y_move += pRect->Left;

	// copy tiles now
	for (int x = 0; x < os; x++)
	{
		for (int y = 0; y < os; y++)
		{
			int new_x = x + x_move;
			int new_y = y + y_move;

			if (!IsCoordInMap(new_x, new_y)) continue;

			CellData& fdd = CellDatas[new_x + new_y * MapWidthPlusHeight];
			CellData& fdo = oldCellDatas[x + y * os];

			AssignCellData(fdd, fdo);
		}
	}

	GameDeleteVector(oldCellDatas);
	oldCellDatas = NULL;

	// overlay
	auto oldOverlay = new WORD[0x40000];
	auto oldOverlayData = new BYTE[0x40000];
	std::copy(std::begin(NewOverlay), std::end(NewOverlay), oldOverlay);
	std::fill(std::begin(NewOverlay), std::end(NewOverlay), 0xFFFF);
	std::copy(std::begin(OverlayData), std::end(OverlayData), oldOverlayData);
	std::fill(std::begin(OverlayData), std::end(OverlayData), 0x0);

	for (int y = 0; y < 512; ++y)
	{
		for (int x = 0; x < 512; ++x)
		{
			int new_x = x + x_move;
			int new_y = y + y_move;

			if (IsCoordInMap(new_x, new_y))
			{
				NewOverlay[new_x * 512 + new_y] = oldOverlay[x * 512 + y];
				Overlay[new_x * 512 + new_y] = std::min((WORD)0xff, oldOverlay[x * 512 + y]);
				OverlayData[new_x * 512 + new_y] = oldOverlayData[x * 512 + y];
			}
		}
	}
	delete[] oldOverlay;
	delete[] oldOverlayData;

	FString buffer;
	auto recoverIniValue = [&x_move, &y_move, &buffer, this](const char* lpName, std::vector<FString>& list, int x_pos, int y_pos)
	{
		INI.DeleteSection(lpName);
		if (list.empty())
			return;
		auto pSection = INI.AddSection(lpName);
		int index = 0;
		FString key;
		for (auto& value : list)
		{
			key.Format("%d", index++);
			std::vector<FString> tokens = FString::SplitString(value);
			if (x_pos < tokens.size() && y_pos < tokens.size())
			{
				int x = atoi(tokens[x_pos]);
				int y = atoi(tokens[y_pos]);
				x += x_move;
				y += y_move;
				if (!IsCoordInMap(x, y))
					continue;
				buffer.Format("%d", x);
				tokens[x_pos] = buffer;
				buffer.Format("%d", y);
				tokens[y_pos] = buffer;
				INI.WriteString(pSection, key, FString::Join(tokens));
			}
		}
	};
	recoverIniValue("Structures", BuildingList, 4, 3);
	recoverIniValue("Units", UnitList, 4, 3);
	recoverIniValue("Aircraft", AircraftList, 4, 3);
	recoverIniValue("Infantry", InfantryList, 4, 3);

	UpdateFieldStructureData_Optimized();
	UpdateFieldUnitData(false);
	UpdateFieldAircraftData(false);
	UpdateFieldInfantryData(false);

	INI.DeleteSection("CellTags");
	if (auto pSection = INI.AddSection("CellTags"))
	{
		for (const auto& [key, value] : CelltagList)
		{
			int number = atoi(key);
			int x = number / 1000 + x_move;
			int y = number % 1000 + y_move;
			if (!IsCoordInMap(x, y))
				continue;
			int keyn = x * 1000 + y;
			INI.WriteString(pSection, STDHelpers::IntToString(keyn), value);
		}
	}
	UpdateFieldCelltagData(false);

	INI.DeleteSection("Waypoints");
	if (auto pSection = INI.AddSection("Waypoints"))
	{
		for (const auto& [key, value] : WaypointList)
		{
			int number = atoi(value);
			int x = number / 1000 + x_move;
			int y = number % 1000 + y_move;
			if (!IsCoordInMap(x, y))
				continue;
			int valuen = x * 1000 + y;
			INI.WriteString(pSection, key, STDHelpers::IntToString(valuen));
		}
	}
	UpdateFieldWaypointData(false);

	std::vector<std::tuple<FString, FString, int, int>> smudges;
	for (size_t i = 0; i < SmudgeDatas.size(); ++i)
	{
		const auto& data = SmudgeDatas[i];
		buffer.Format("%d", i);
		smudges.emplace_back(buffer, data.TypeID, data.X + y_move, data.Y + x_move);
	}
	INI.DeleteSection("Smudge");
	if (auto pSection = INI.AddSection("Smudge"))
	{
		for (const auto& [key, id, x, y] : smudges)
		{
			if (!IsCoordInMap(x, y))
				continue;

			buffer.Format("%s,%d,%d,0", id, x, y);
			INI.WriteString(pSection, key, buffer);
		}
	}
	UpdateFieldSmudgeData(false);

	std::vector<std::tuple<FString , int, int>> terrains;
	for (size_t i = 0; i < TerrainDatas.size(); ++i)
	{
		const auto& data = TerrainDatas[i];
		terrains.emplace_back(data.TypeID, data.Y + x_move, data.X + y_move);
	}
	INI.DeleteSection("Terrain");
	if (auto pSection = INI.AddSection("Terrain"))
	{
		for (const auto& [id, x, y] : terrains)
		{
			if (!IsCoordInMap(x, y))
				continue;
			int key = x * 1000 + y;
			INI.WriteString(pSection, STDHelpers::IntToString(key), id);
		}
	}
	UpdateFieldTerrainData(false);

	for (const auto& [_, house] : Variables::RulesMap.GetSection("Houses"))
	{
		if (auto pSection = INI.GetSection(house))
		{
			const int nodeCount = INI.GetInteger(pSection, "NodeCount");

			std::vector<std::tuple<FString, int, int>> nodes;
			for (int i = 0; i < nodeCount; ++i)
			{
				buffer.Format("%03d", i);
				const auto value = INI.GetString(pSection, buffer);
				const auto splits = FString::SplitString(value);
				if (splits.size() > 2)
					nodes.emplace_back(splits[0], atoi(splits[1]) + y_move, atoi(splits[2]) + x_move);

				INI.DeleteKey(pSection, buffer);
			}

			int i = 0;
			for (auto& [id, y, x] : nodes)
			{
				if (!IsCoordInMap(x, y))
					continue;

				FString key;
				buffer.Format("%s,%d,%d", id, y, x);
				key.Format("%03d", i);
				INI.WriteString(pSection, key, buffer);
				i++;
			}
			INI.WriteString(pSection, "NodeCount", STDHelpers::IntToString(i));
		}
	}
	UpdateFieldBasenodeData(false);

	if (auto pSection = CINI::CurrentDocument->GetSection("Tubes"))
	{
		for (const auto& [key, value] : pSection->GetEntities())
		{
			auto atoms = FString::SplitString(value, 5);

			MapCoord StartCoord = { atoi(atoms[1]),atoi(atoms[0]) };
			MapCoord EndCoord = { atoi(atoms[4]),atoi(atoms[3]) };
			StartCoord += {x_move, y_move};
			EndCoord += {x_move, y_move};
			if (!IsCoordInMap(StartCoord.X, StartCoord.Y)
				|| !IsCoordInMap(EndCoord.X, EndCoord.Y))
				continue;

			atoms[1].Format("%d", StartCoord.X);
			atoms[0].Format("%d", StartCoord.Y);
			atoms[4].Format("%d", EndCoord.X);
			atoms[3].Format("%d", EndCoord.Y);
			FString val;
			for (auto& atom : atoms)
			{
				val += atom;
				val += ",";
			}
			val.Delete(val.GetLength() - 1, 1);
			CINI::CurrentDocument->WriteString(pSection, key, val);
		}
	}
	UpdateFieldTubeData(false);

	if (auto pSection = CINI::CurrentDocument->GetSection("Annotations"))
	{
		std::vector<std::pair<FString, FString>> annotations;
		for (const auto& [key, value] : pSection->GetEntities())
		{
			auto pos = atoi(key);
			int x = pos / 1000 + x_move;
			int y = pos % 1000 + y_move;
			if (!IsCoordInMap(x, y))
				continue;
			buffer.Format("%d", y + x * 1000);
			annotations.push_back(std::make_pair(FString(buffer), FString(value)));
		}
		CINI::CurrentDocument->DeleteSection("Annotations");
		pSection = CINI::CurrentDocument->AddSection("Annotations");
		for (const auto& [key, value] : annotations)
		{
			CINI::CurrentDocument->WriteString(pSection, key, value);
		}
	}
	CMapDataExt::UpdateAnnotation();

	for (auto& twoPoints : CIsoViewExt::TwoPointDistance)
	{
		twoPoints.Point1.X += x_move;
		twoPoints.Point1.Y += y_move;
		twoPoints.Point2.X += x_move;
		twoPoints.Point2.Y += y_move;
	}
	for (auto& [mc, radius] : CIsoViewExt::Circles)
	{
		mc.X += x_move;
		mc.Y += y_move;
	}
	if (CIsoViewExt::AxialSymmetryLine[0] != MapCoord{ 0,0 })
	{
		CIsoViewExt::AxialSymmetryLine[0].X += x_move;
		CIsoViewExt::AxialSymmetryLine[0].Y += y_move;
		CIsoViewExt::AxialSymmetryLine[1].X += x_move;
		CIsoViewExt::AxialSymmetryLine[1].Y += y_move;
	}
	for (auto& [mc1, mc2] : CIsoViewExt::AxialSymmetricPoints)
	{
		mc1.X += x_move;
		mc1.Y += y_move;
		mc2.X += x_move;
		mc2.Y += y_move;
	}
	for (auto& [mc1, mc2] : CIsoViewExt::CentralSymmetricPoints)
	{
		mc1.X += x_move;
		mc1.Y += y_move;
		mc2.X += x_move;
		mc2.Y += y_move;
	}
	if (CIsoViewExt::CentralSymmetryCenter != MapCoord{ 0,0 })
	{
		CIsoViewExt::CentralSymmetryCenter.X += x_move;
		CIsoViewExt::CentralSymmetryCenter.Y += y_move;
	}

	InitMinimap();

	for (int y = 0; y < MapWidthPlusHeight; ++y)
	{
		for (int x = 0; x < MapWidthPlusHeight; ++x)
		{
			CellDataExts[x + y * MapWidthPlusHeight].NewOverlay = NewOverlay[x * 512 + y];
			UpdateMapPreviewAt(x, y);
		}
	}

	CIsoViewExt::MoveToMapCoord(MapWidthPlusHeight / 2, MapWidthPlusHeight / 2);

	return true;
}

bool CMapDataExt::ResizeMap_AllocCellData(MapRect* const pRect)
{
	GameDeleteVector(CellDatas);
	CellDatas = NULL;
	CellDataCount = 0;
	CMapDataExt::UndoRedoDatas.clear();
	CMapDataExt::UndoRedoDataIndex = -1;

	ppmfc::CString mapSize;
	mapSize.Format("0,0,%d,%d", pRect->Width, pRect->Height);
	INI.WriteString("Map", "Size", mapSize);

	MapRect newLocalSize = LocalSize;
	newLocalSize.Width += pRect->Width - Size.Width;
	newLocalSize.Height += pRect->Height - Size.Height;

	mapSize.Format("%d,%d,%d,%d", newLocalSize.Left, newLocalSize.Top, newLocalSize.Width, newLocalSize.Height);
	INI.WriteString("Map", "LocalSize", mapSize);

	Size.Width = pRect->Width;
	Size.Height = pRect->Height;
	LocalSize = newLocalSize;
	MapWidthPlusHeight = Size.Width + Size.Height;

	CellDatas = GameCreateVector<CellData>((MapWidthPlusHeight + 1) * (MapWidthPlusHeight + 1));
	CellDataCount = (MapWidthPlusHeight + 1) * (MapWidthPlusHeight + 1);
	CellDataExts.clear();
	CellDataExts.resize(CellDataCount);

	if (IsoPackData != NULL) GameDeleteArray(IsoPackData, IsoPackDataCount);
	IsoPackData = NULL;
	IsoPackDataCount = 0;

	CMeasurementToolbox::ClearStatus();

	return true;
}

DEFINE_HOOK(4C45F0, CMapData_ResizeMap, 6)
{
	GET(CMapDataExt*, pThis, ECX);
	GET_STACK(int, left, 0x4);
	GET_STACK(int, top, 0x8);
	GET_STACK(int, width, 0xC);
	GET_STACK(int, height, 0x10);

	MapRect rect{ left, top, width, height };
	pThis->ResizeMapExt(&rect);

	return 0x4C7DC7;
}
