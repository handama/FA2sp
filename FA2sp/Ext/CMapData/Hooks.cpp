#include "Body.h"

#include <Helpers/Macro.h>

#include "../../FA2sp.h"
#include "../../Helpers/TheaterHelpers.h"
#include "../../Helpers/STDHelpers.h"

#include <Miscs/Miscs.h>
#include "../CIsoView/Body.h"
#include "../CFinalSunDlg/Body.h"
#include "../../Miscs/MultiSelection.h"
#include "../CTileSetBrowserFrame/TabPages/TriggerSort.h"
#include "../CTileSetBrowserFrame/TabPages/TeamSort.h"
#include "../CTileSetBrowserFrame/TabPages/WaypointSort.h"
#include "../CTileSetBrowserFrame/TabPages/TaskForceSort.h"
#include "../CTileSetBrowserFrame/TabPages/ScriptSort.h"
#include "../../Miscs/Palettes.h"
#include "../CLoading/Body.h"
#include "../../ExtraWindow/CLuaConsole/CLuaConsole.h"

DEFINE_HOOK(4C3E20, CMapData_CalculateMoneyCount, 7)
{
    auto pExt = CMapDataExt::GetExtension();

    int nCount = 0;

    for (int i = 0; i < pExt->CellDataCount; ++i)
        nCount += pExt->GetOreValue(pExt->CellDataExts[i].NewOverlay, pExt->CellDatas[i].OverlayData);

    R->EAX(nCount);

    return 0x4C4460;
}

DEFINE_HOOK(4A1DB0, CMapData_AddTiberium, 6)
{
    auto pExt = CMapDataExt::GetExtension();

    GET_STACK(unsigned char, nOverlay, 0x4);
    GET_STACK(unsigned char, nOverlayData, 0x8);
    
    pExt->MoneyCount += pExt->GetOreValue(nOverlay, nOverlayData);

    return 0x4A238E;
}

DEFINE_HOOK(4A17C0, CMapData_DeleteTiberium, 6)
{
    auto pExt = CMapDataExt::GetExtension();

    GET_STACK(unsigned char, nOverlay, 0x4);
    GET_STACK(unsigned char, nOverlayData, 0x8);

    pExt->MoneyCount -= pExt->GetOreValue(nOverlay, nOverlayData);

    return 0x4A1D9E;
}

DEFINE_HOOK(4BB04A, CMapData_AddTube_IgnoreUselessNegativeOne, 7)
{
    GET(CTubeData*, pTubeData, ESI);
    REF_STACK(ppmfc::CString, lpBuffer, STACK_OFFS(0x134, 0x124));

    for (int i = 0; i < 100; ++i)
    {
        if (pTubeData->Directions[i] == -1)
        {
            lpBuffer += ",-1";
            break;
        }

        lpBuffer += ',';
        lpBuffer += pTubeData->Directions[i] + '0';
    }

    return 0x4BB083;
}

DEFINE_HOOK(49DFB4, CMapData_LoadMap_InvalidTheater, 6)
{
    GET(const char*, lpTheaterName, EDI);

    for (const auto& lpStr : TheaterHelpers::GetEnabledTheaterNames()) 
    {
        if (_strcmpi(lpStr, lpTheaterName) == 0)
            return 0;
    }

    return 0x49EDD9;
}

#define DEFINE_THEATER_NAME_FIX(addr, name, reg) \
DEFINE_HOOK(addr, CMapData_LoadMap_SupportLowerCaseTheaterName_##name, 5) \
{R->EAX(_strcmpi(R->reg<const char*>(), #name)); return (0x##addr + 0xE);}

DEFINE_THEATER_NAME_FIX(49DFB4, TEMPERATE, EDI);
DEFINE_THEATER_NAME_FIX(49E1D9, SNOW, EDI);
DEFINE_THEATER_NAME_FIX(49E3FF, URBAN, EDI);
DEFINE_THEATER_NAME_FIX(49E631, NEWURBAN, EDI);
DEFINE_THEATER_NAME_FIX(49E863, LUNAR, EDI);
DEFINE_THEATER_NAME_FIX(49EAA4, DESERT, ESI);

#undef DEFINE_THEATER_NAME_FIX

DEFINE_HOOK(4A4A40, CMapData_UpdateFieldStructureData, 6)
{
	CMapDataExt::UpdateFieldStructureData_Optimized();
	return 0x4A583F;
}


//DEFINE_HOOK(4A4AA6, CMapData_UpdateMapFieldData_Structures_RenderData_Clear, 6)
//{
//    CMapDataExt::BuildingRenderDatasFix.clear();
//    return 0;
//}
//
//DEFINE_HOOK(4A4C8E, CMapData_UpdateMapFieldData_Structures_RenderData_Load, 6)
//{
//    GET(ppmfc::CString*, pString, EAX);
//
//    const auto& splits = STDHelpers::SplitString(*pString);
//    
//    BuildingRenderData data;
//    data.HouseColor = Miscs::GetColorRef(splits[0]);
//    data.ID = splits[1];
//    data.Strength = std::clamp(atoi(splits[2]), 0, 256);
//    data.Y = atoi(splits[3]);
//    data.X = atoi(splits[4]);
//    data.Facing = atoi(splits[5]);
//    data.PowerUpCount = atoi(splits[10]);
//    data.PowerUp1 = splits[12];
//    data.PowerUp2 = splits[13];
//    data.PowerUp3 = splits[14];
//	int size = CMapDataExt::BuildingRenderDatasFix.size();
//    CMapDataExt::BuildingRenderDatasFix[size] = data;
//
//    R->ESP(R->ESP() + 0xC);
//    return 0x4A4F6D;
//}
//
//DEFINE_HOOK(4A4F7A, CMapData_UpdateMapFieldData_Structures_SKIPCOPY, 9)
//{
//    return 0x4A4F83;
//}
//
//DEFINE_HOOK(4A57CD, CMapData_UpdateMapFieldData_Structures_SKIPDTOR, F)
//{
//    return 0x4A5817;
//}

DEFINE_HOOK(4C3C20, CMapData_GetStructureRenderData, 6)
{
    GET_STACK(int, ID, 0x4);
    GET_STACK(CBuildingRenderData*, pRet, 0x8);

    if (ID >= 0 && ID < CMapDataExt::BuildingRenderDatasFix.size())
    {
        const auto& data = CMapDataExt::BuildingRenderDatasFix[ID];
        *reinterpret_cast<unsigned int*>(&pRet->HouseColor) = data.HouseColor;
        pRet->ID = data.ID.c_str();
        pRet->Strength = static_cast<unsigned char>(data.Strength);
        pRet->X = data.X;
        pRet->Y = data.Y;
        pRet->Facing = data.Facing;
        pRet->PowerUpCount = data.PowerUpCount;
        pRet->PowerUp1 = data.PowerUp1.c_str();
        pRet->PowerUp2 = data.PowerUp2.c_str();
        pRet->PowerUp3 = data.PowerUp3.c_str();

        CMapDataExt::CurrentRenderBuildingStrength = data.Strength;
    }

    return 0x4C3CCC;
}


DEFINE_HOOK(4A2AB0, CMapData_UpdateInfantry, 7)
{
	GET_STACK(BOOL, bMapToINI, 0x4);
	auto Map = &CMapData::Instance();
	auto& m_infantry = Map->InfantryDatas;
	auto& m_mapfile = Map->INI;
	auto& fielddata_size = Map->CellDataCount;
	auto& fielddata = Map->CellDatas;

	if (bMapToINI == FALSE)
	{
		m_infantry.clear();
		m_infantry.reserve(100);

		int i;
		for (i = 0; i < Map->CellDataCount; i++)
		{
			int e;
			for (e = 0; e < 3; e++)
				fielddata[i].Infantry[e] = -1;
		}

		if (auto pSection = m_mapfile.GetSection("Infantry"))
		{
			std::vector<FString> values;
			for (const auto& [key, value] : pSection->GetEntities())
			{
				values.push_back(value);
				auto atoms = FString::SplitString(value, 13);

				int x = atoi(atoms[4]);
				int y = atoi(atoms[3]);
				int sp = atoi(atoms[5]);
				int pos = x + y * Map->MapWidthPlusHeight;

				CInfantryData id;
				id.House = atoms[0];
				id.TypeID = atoms[1];
				id.Health = atoms[2];
				id.Y = atoms[3];
				id.X = atoms[4];
				id.SubCell = atoms[5];
				id.Status = atoms[6];
				id.Facing = atoms[7];
				id.Tag = atoms[8];
				id.VeterancyPercentage = atoms[9];
				id.Group = atoms[10];
				id.IsAboveGround = atoms[11];
				id.AutoNORecruitType = atoms[12];
				id.AutoYESRecruitType = atoms[13];

				m_infantry.push_back(id);

				int spp = 10;
				switch (sp)
				{
				case 0:
				case 1:
				case 4:
					spp = 0;
					break;
				case 2:
					spp = 1;
					break;
				case 3:
					spp = 2;
					break;
				default:
					break;
				}

				if (spp < 3 && pos < fielddata_size)
					fielddata[pos].Infantry[spp] = (short)m_infantry.size() - 1;

				CMapData::Instance->UpdateMapPreviewAt(x, y);
			}
			// resort section to match the order of InfantryDatas
			m_mapfile.DeleteSection("Infantry");
			auto pSectionInf = m_mapfile.AddSection("Infantry");
			for (int i = 0; i < values.size(); ++i)
			{
				m_mapfile.WriteString(pSectionInf, STDHelpers::IntToString(i), values[i]);
			}
		}
	}
	else
	{
		m_mapfile.DeleteSection("Infantry");
		auto pSectionInf = m_mapfile.AddSection("Infantry");
		int count = 0;
		for (int i = 0; i < m_infantry.size(); ++i)
		{
			auto& infantry = m_infantry[i];
			if (!infantry.Flag)
			{
				FString value;
				value.Format("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s"
					, infantry.House
					, infantry.TypeID
					, infantry.Health
					, infantry.Y
					, infantry.X
					, infantry.SubCell
					, infantry.Status
					, infantry.Facing
					, infantry.Tag
					, infantry.VeterancyPercentage
					, infantry.Group
					, infantry.IsAboveGround
					, infantry.AutoNORecruitType
					, infantry.AutoYESRecruitType);

				m_mapfile.WriteString(pSectionInf, STDHelpers::IntToString(count++), value);
			}
		}
	}
	return 0x4A39B9;
}

DEFINE_HOOK(4A7C74, CMapData_DeleteInfantry_InfantrySubCell, 5)
{
	GET(int, pos, EAX);
	if (pos > 0)
		pos--;
	if (pos == 3)
		pos = 0;
	R->EAX(pos);

	return 0x4A7C79;
}

DEFINE_HOOK(4A7C3D, CMapData_DeleteInfantry_UpdateIni, 6)
{
	GET(int, index, EDI);
	CMapData::Instance->INI.DeleteKey("Infantry", STDHelpers::IntToString(index / 60));
	return 0;
}

DEFINE_HOOK(4AC210, CMapData_AddInfantry, 7)
{
	GET_STACK(CInfantryData*, lpInfantry, 0x4);
	GET_STACK(LPCTSTR, lpType, 0x8);
	GET_STACK(LPCTSTR, lpHouse, 0xC);
	GET_STACK(DWORD, dwPos, 0x10);
	GET_STACK(int, suggestedIndex, 0x14);

	auto Map = &CMapData::Instance();
	auto& m_infantry = Map->InfantryDatas;
	auto& fielddata_size = Map->CellDataCount;
	auto& fielddata = Map->CellDatas;
	CInfantryData infantry;

	if (lpInfantry != NULL)
	{
		infantry = *lpInfantry;
		dwPos = atoi(infantry.X) + atoi(infantry.Y) * Map->MapWidthPlusHeight;

		// MW Bugfix: not checking if infantry.SubCell does already exist caused crashes with user scripts!
		if (CMapDataExt::GetInfantryAt(dwPos, atoi(infantry.SubCell)) >= 0)
			infantry.SubCell = "-1";
	}
	else
	{
		infantry.House = lpHouse;
		infantry.TypeID = lpType;
		infantry.X.Format("%d", dwPos % Map->MapWidthPlusHeight);
		infantry.Y.Format("%d", dwPos / Map->MapWidthPlusHeight);
		infantry.SubCell = "-1";
		infantry.Status = ExtConfigs::DefaultInfantryProperty.Status;
		infantry.Tag = ExtConfigs::DefaultInfantryProperty.Tag;
		infantry.Facing = ExtConfigs::DefaultInfantryProperty.Facing;
		infantry.VeterancyPercentage = ExtConfigs::DefaultInfantryProperty.VeterancyPercentage;
		infantry.Group = ExtConfigs::DefaultInfantryProperty.Group;
		infantry.IsAboveGround = ExtConfigs::DefaultInfantryProperty.IsAboveGround;
		infantry.AutoNORecruitType = ExtConfigs::DefaultInfantryProperty.AutoNORecruitType;
		infantry.AutoYESRecruitType = ExtConfigs::DefaultInfantryProperty.AutoYESRecruitType;
		infantry.Health = ExtConfigs::DefaultInfantryProperty.Health;
	}
	if (CIsoViewExt::AutoPropertyBrush[2]
		&& !(CIsoView::CurrentCommand->Command == 1 && CIsoView::CurrentCommand->Type == 7)
		&& !CIsoView::GetInstance()->Drag
		&& CIsoView::CurrentCommand->Command != 21)
	{
		CViewObjectsExt::ApplyPropertyBrush_Infantry(infantry);
	}

	if (infantry.SubCell == "-1")
	{
		int subpos = -1;
		int i;

		if (Map->GetInfantryCountAt(dwPos) == 0)
		{
			if (ExtConfigs::InfantrySubCell_GameDefault)
				subpos = 4;
			else
				subpos = 0;
		}

		else
		{
			int oldInf = Map->GetInfantryAt_pos(dwPos, 0);
			if (oldInf > -1)
			{
				CInfantryData inf;
				Map->GetInfantryData(oldInf, inf);
				if (inf.SubCell == "0")
					for (i = 1; i < 3; i++)
					{
						if (Map->GetInfantryAt_pos(dwPos, i) == -1)
						{
							char c[50];
							_itoa(i, c, 10);
							inf.SubCell = c;
							Map->DeleteInfantryData(oldInf);
							Map->SetInfantryData(&inf, NULL, NULL, 0, -1);
							break;
						}
					}
			}

			for (i = 0; i < 3; i++)
			{
				if (Map->GetInfantryAt_pos(dwPos, i) == -1)
				{
					subpos = i + 1;
					if (ExtConfigs::InfantrySubCell_GameDefault && i == 0)
						subpos = 4;
					break;
				}
			}
		}

		if (subpos < 0) return 0x4ACA49;
		char c[50];
		_itoa(subpos, c, 10);

		infantry.SubCell = c;
	}

	infantry.Flag = 0;

	if (ExtConfigs::InfantrySubCell_OccupationBits)
	{
		auto cell = Map->TryGetCellAt(atoi(infantry.X), atoi(infantry.Y));
		if (cell->Terrain > -1)
		{
			std::vector<int> availableSubcells;
			std::vector<int> availableAndEmptySubcells;
			ppmfc::CString name;
			ppmfc::CString key = "TemperateOccupationBits";
			auto theaterIg = CINI::CurrentDocument->GetString("Map", "Theater");
			if (theaterIg == "SNOW")
				key = "SnowOccupationBits";

			if (auto pTerrain = CINI::Rules().GetSection("TerrainTypes"))
				name = *pTerrain->GetValueAt(cell->TerrainType);
			int bits = (CINI::Rules().GetInteger(name, key, 7) % 8 + 8) % 8;
			switch (bits)
			{
			case 0:
				availableSubcells.push_back(2);
				availableSubcells.push_back(3);
				availableSubcells.push_back(4);
				break;
			case 1:
				availableSubcells.push_back(3);
				availableSubcells.push_back(4);
				break;
			case 2:
				availableSubcells.push_back(2);
				availableSubcells.push_back(4);
				break;
			case 3:
				availableSubcells.push_back(4);
				break;
			case 4:
				availableSubcells.push_back(2);
				availableSubcells.push_back(3);
				break;
			case 5:
				availableSubcells.push_back(3);
				break;
			case 6:
				availableSubcells.push_back(2);
				break;
			case 7:
				break;
			default:
				break;
			}

			if (availableSubcells.empty())
				return 0x4ACA49;

			for (int subcell : availableSubcells)
			{
				bool available = true;
				for (int i = 0; i < 3; i++)
				{
					int pos = CMapData::Instance->CellDatas[dwPos].Infantry[i];
					if (pos > -1)
					{
						CInfantryData inf;
						Map->GetInfantryData(pos, inf);
						if (atoi(inf.SubCell) == subcell)
							available = false;
					}
				}
				if (available)
					availableAndEmptySubcells.push_back(subcell);
			}

			if (availableAndEmptySubcells.empty())
				return 0x4ACA49;

			int decidedSC = availableAndEmptySubcells[0];
			for (auto subcell : availableAndEmptySubcells)
			{
				if (atoi(infantry.SubCell) == subcell)
				{
					decidedSC = subcell;
					break;
				}
			}

			infantry.SubCell.Format("%d", decidedSC);
		}
	}

	int sp = atoi(infantry.SubCell);
	if (sp > 0) sp--;
	if (sp == 3)
		sp = 0;

	int i;
	int realIndex = -1;
	if (suggestedIndex >= 0 && suggestedIndex < m_infantry.size())
	{
		if (m_infantry[suggestedIndex].Flag)
		{
			m_infantry[suggestedIndex] = infantry;
			if (dwPos < fielddata_size) fielddata[dwPos].Infantry[sp] = suggestedIndex;
			realIndex = suggestedIndex;
		}
	}

	if (realIndex < 0)
		for (i = 0; i < m_infantry.size(); i++)
		{
			if (m_infantry[i].Flag) // yep, found one, replace it
			{
				m_infantry[i] = infantry;
				if (dwPos < fielddata_size) fielddata[dwPos].Infantry[sp] = i;
				realIndex = i;
				break;
			}
		}

	if (realIndex < 0)
	{
		m_infantry.push_back(infantry);
		realIndex = m_infantry.size() - 1;
		if (dwPos < fielddata_size) fielddata[dwPos].Infantry[sp] = (short)realIndex;
	}

	FString key;
	FString value;
	key.Format("%d", realIndex);
	value.Format("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s"
		,infantry.House
		,infantry.TypeID
		,infantry.Health
		,infantry.Y
		,infantry.X
		,infantry.SubCell
		,infantry.Status
		,infantry.Facing
		,infantry.Tag
		,infantry.VeterancyPercentage
		,infantry.Group
		,infantry.IsAboveGround
		,infantry.AutoNORecruitType
		,infantry.AutoYESRecruitType);
	
	Map->INI.WriteString("Infantry", key, value);

	CMapData::Instance->UpdateMapPreviewAt(atoi(infantry.X), atoi(infantry.Y));

	return 0x4ACA49;
}

DEFINE_HOOK(4ACB60, CMapData_Update_AddBuilding, 7)
{
	GET_STACK(CBuildingData*, lpStructure, 0x4);
	GET_STACK(LPCTSTR, lpType, 0x8);
	GET_STACK(LPCTSTR, lpHouse, 0xC);
	GET_STACK(DWORD, dwPos, 0x10);
	GET_STACK(ppmfc::CString, suggestedID, 0x14);


	auto Map = &CMapData::Instance();
	auto& fielddata_size = Map->CellDataCount;
	auto& fielddata = Map->CellDatas;
	CMapData::Instance->UpdateCurrentDocument();
	auto m_mapfile = Map->GetMapDocument();

	bool skipOverlapping = false;
	bool overlappingWarning = false;

	CBuildingData structure;
	if (lpStructure != NULL)
	{
		structure = *lpStructure;
		skipOverlapping = true;
		
	}
	else
	{
		char cx[10], cy[10];
		_itoa(dwPos % Map->MapWidthPlusHeight, cx, 10);
		_itoa(dwPos / Map->MapWidthPlusHeight, cy, 10);

		structure.Health = ExtConfigs::DefaultBuildingProperty.Health;
		structure.Facing = ExtConfigs::DefaultBuildingProperty.Facing;
		structure.Tag = ExtConfigs::DefaultBuildingProperty.Tag;
		structure.AISellable = ExtConfigs::DefaultBuildingProperty.AISellable;
		structure.AIRebuildable = ExtConfigs::DefaultBuildingProperty.AIRebuildable;
		structure.PoweredOn = ExtConfigs::DefaultBuildingProperty.PoweredOn;
		structure.Upgrades = ExtConfigs::DefaultBuildingProperty.Upgrades;
		structure.SpotLight = ExtConfigs::DefaultBuildingProperty.SpotLight;
		structure.Upgrade1 = ExtConfigs::DefaultBuildingProperty.Upgrade1;
		structure.Upgrade2 = ExtConfigs::DefaultBuildingProperty.Upgrade2;
		structure.Upgrade3 = ExtConfigs::DefaultBuildingProperty.Upgrade3;
		structure.AIRepairable = ExtConfigs::DefaultBuildingProperty.AIRepairable;
		structure.Nominal = ExtConfigs::DefaultBuildingProperty.Nominal;
		structure.House = lpHouse;
		structure.TypeID = lpType;
		structure.X = cx;
		structure.Y = cy;
		
	}
	if (ExtConfigs::PlaceStructureUpgrades) 
	{
		int upgradeCount = 0;
		if (structure.Upgrade1 != "None")
			upgradeCount++;
		if (structure.Upgrade2 != "None")
			upgradeCount++;
		if (structure.Upgrade3 != "None")
			upgradeCount++;
		structure.Upgrades = STDHelpers::IntToString(upgradeCount);
	}
	if (ExtConfigs::PlaceStructureUpgradeStrength)
	{
		int upgradeCount = 0;
		if (structure.Upgrade1 != "None")
			upgradeCount++;
		if (structure.Upgrade2 != "None")
			upgradeCount++;
		if (structure.Upgrade3 != "None")
			upgradeCount++;
		if (upgradeCount > 0)
		{
			structure.Health = "256";
		}
	}

	if (CIsoViewExt::AutoPropertyBrush[1]
		&& !(CIsoView::CurrentCommand->Command == 1 && CIsoView::CurrentCommand->Type == 7)
		&& !CIsoView::GetInstance()->Drag
		&& CIsoView::CurrentCommand->Command != 21)
	{
		CViewObjectsExt::ApplyPropertyBrush_Building(structure);
	}

	auto pSection = m_mapfile->AddOrGetSection("Structures");
	if (!CopyPaste::OnLButtonDownPasted && !CMapDataExt::SkipBuildingOverlappingCheck)
	{
		// check overlap
		auto& ignoreList = CINI::FAData->GetSection("StructureOverlappingCheckIgnores")->GetEntities();
		bool skipCheck = false;
		for (const auto& pair : ignoreList)
		{
			if (pair.second == structure.TypeID)
				skipCheck = true;

		}

		if (!skipCheck && ExtConfigs::PlaceStructureOverlappingCheck)
		{
			std::map<int, int> Occupied;
			int length = CMapData::Instance->MapWidthPlusHeight;
			length *= length;

			{
				const int Index = CMapDataExt::GetBuildingTypeIndex(structure.TypeID);
				const int Y = atoi(structure.Y);
				const int X = atoi(structure.X);
				const auto& DataExt = CMapDataExt::BuildingDataExts[Index];

				if (!DataExt.IsCustomFoundation())
				{
					for (int dx = 0; dx < DataExt.Height; ++dx)
					{
						for (int dy = 0; dy < DataExt.Width; ++dy)
						{
							MapCoord coord = { X + dx, Y + dy };
							if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
							{
								Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] = 114514;
							}

						}
					}
				}
				else
				{
					for (const auto& block : *DataExt.Foundations)
					{
						MapCoord coord = { X + block.Y, Y + block.X };
						if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
						{
							Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] = 114514;
						}

					}
				}
			}

			for (const auto& pair : pSection->GetEntities())
			{
				bool skipThis = false;
				auto values = STDHelpers::SplitString(pair.second, 4);
				auto& type = values[1];

				for (const auto& pair2 : ignoreList)
					if (pair2.second == type)
						skipThis = true;
				if (skipThis)
					continue;

				const int Index = CMapDataExt::GetBuildingTypeIndex(type);
				const int Y = atoi(values[3]);
				const int X = atoi(values[4]);
				const auto& DataExt = CMapDataExt::BuildingDataExts[Index];

				if (!DataExt.IsCustomFoundation())
				{
					for (int dx = 0; dx < DataExt.Height; ++dx)
					{
						for (int dy = 0; dy < DataExt.Width; ++dy)
						{
							MapCoord coord = { X + dx, Y + dy };
							if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
								if (Occupied.find(CMapData::Instance->GetCoordIndex(coord.X, coord.Y)) != Occupied.end())
									if (Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] >= 114514)
										Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] += 1;
						}
					}
				}
				else
				{
					for (const auto& block : *DataExt.Foundations)
					{
						MapCoord coord = { X + block.Y, Y + block.X };
						if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
							if (Occupied.find(CMapData::Instance->GetCoordIndex(coord.X, coord.Y)) != Occupied.end())
								if (Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] >= 114514)
									Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)] += 1;
					}
				}
			}
			for (auto& o : Occupied)
			{
				if (o.second > 114514)
					if (!skipOverlapping)
						return 0x4AD921;
					else
						overlappingWarning = true;
			}
		}

		if (overlappingWarning)
		{
			FString pMessage2;
			FString pMessage = Translations::TranslateOrDefault("PlaceStructureOverlapMessage",
				"The current operating structure %s overlaps with other structures.\nDelete or undo is strongly recommended!");
			pMessage2.Format(pMessage, structure.TypeID);

			MessageBox(CFinalSunDlg::Instance->m_hWnd, pMessage2, Translations::TranslateOrDefault("PlaceStructureOverlapTitle",
				"Structure Overlap"), MB_OK | MB_ICONEXCLAMATION);
		}
	}
		
	ppmfc::CString id = "";
	
	if (suggestedID != "" && STDHelpers::IsNumber(suggestedID))
	{
		if (pSection->GetEntities().find(suggestedID) == pSection->GetEntities().end())
			id = suggestedID;
	}
	if (id == "")
		id = CINI::GetAvailableKey("Structures");

	FString value;
	value = structure.House + "," + structure.TypeID + "," + structure.Health + "," + structure.Y +
		"," + structure.X + "," + structure.Facing + "," + structure.Tag + "," + structure.AISellable + "," +
		structure.AIRebuildable + "," + structure.PoweredOn + "," + structure.Upgrades + "," + structure.SpotLight + ","
		+ structure.Upgrade1 + "," + structure.Upgrade2 + "," + structure.Upgrade3 + "," + structure.AIRepairable + "," + structure.Nominal;

	m_mapfile->WriteString("Structures", id, value);

	int realid = 0;
	for (const auto& pair : pSection->GetEntities())
	{
		if (pair.first == id)
			break;
		realid++;
	}
	CMapDataExt::UpdateFieldStructureData_Index(realid, value);
	bool isLamp = LightingSourceTint::IsLamp(structure.TypeID);

	if (isLamp)
		LightingSourceTint::CalculateMapLamps();

	return 0x4AD921;
}

DEFINE_HOOK(466FE0, CIsoView_OnLButtonUp_Drag_Building, 5)
{
	GET_STACK(UINT, nFlags, 0x218);
	GET_STACK(int, screenCoordX, 0x21C);
	GET_STACK(int, screenCoordY, 0x220);

	auto Map = &CMapData::Instance();
	auto isoView = CIsoView::GetInstance();
	auto m_id = isoView->CurrentCellObjectIndex;
	
	int X = screenCoordX;
	int	Y = screenCoordY;
	CPoint point(X, Y);

	auto mapCoord = isoView->GetCurrentMapCoord(point);

	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Building);

	CBuildingData structure;
	Map->GetBuildingData(m_id, structure);
	structure.X = std::to_string(mapCoord.X).c_str();
	structure.Y = std::to_string(mapCoord.Y).c_str();

	if ((nFlags != MK_SHIFT))
	{
		Map->DeleteBuildingData(m_id);
	}
	Map->SetBuildingData(&structure, NULL, NULL, 0, "");


	return 0x467682;
}

DEFINE_HOOK(4A8FB0, CMapData_DeleteStructure, 7)
{
	GET_STACK(unsigned int, cellIndex, 0x4);

	auto& mapdata = CMapData::Instance();
	auto& ini = CINI::CurrentDocument;
	if (!ini->SectionExists("Structures"))
		return 0x4A98AC;

	if (cellIndex >= CMapDataExt::StructureIndexMap.size())
		return 0x4A98AC;
	int iniIndex = CMapDataExt::StructureIndexMap[cellIndex];
	if (CMapDataExt::DeleteBuildingByIniID)
	{
		iniIndex = cellIndex;
		bool found = false;
		for (int i = 0; i < CMapDataExt::StructureIndexMap.size(); ++i)
		{
			if (CMapDataExt::StructureIndexMap[i] == iniIndex)
			{
				cellIndex = i;
				found = true;
				break;
			}
		}
		if (!found)
			return 0x4A98AC;
	}
	if (iniIndex >= ini->GetKeyCount("Structures"))
		return 0x4A98AC;

	ppmfc::CString key = ini->GetKeyAt("Structures", iniIndex);
	ppmfc::CString value = ini->GetValueAt("Structures", iniIndex);
	auto splits =STDHelpers::SplitString(value, 4);
	bool isLamp = LightingSourceTint::IsLamp(splits[1]);
	int Index = CMapDataExt::GetBuildingTypeIndex(splits[1]);
	int Y = atoi(splits[3]);
	int X = atoi(splits[4]);
	auto& DataExt = CMapDataExt::BuildingDataExts[Index];

	if (!DataExt.IsCustomFoundation())
	{
		for (int dy = 0; dy < DataExt.Width; ++dy)
		{
			for (int dx = 0; dx < DataExt.Height; ++dx)
			{
				const int x = X + dx;
				const int y = Y + dy;
				const int pos = CMapData::Instance->GetCoordIndex(x, y);
				if (pos < CMapData::Instance->CellDataCount)
				{
					auto pCell = CMapData::Instance->GetCellAt(pos);
					auto& cellExt = CMapDataExt::CellDataExts[pos];
					cellExt.Structures.erase(cellIndex);
					if (cellExt.Structures.empty())
					{
						pCell->Structure = -1;
						pCell->TypeListIndex = -1;
					}
					else
					{
						pCell->Structure = cellExt.Structures.begin()->first;
						pCell->TypeListIndex = cellExt.Structures.begin()->second;
					}
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
			const int pos = CMapData::Instance->GetCoordIndex(x, y);
			if (pos < CMapData::Instance->CellDataCount)
			{
				auto pCell = CMapData::Instance->GetCellAt(pos);
				auto& cellExt = CMapDataExt::CellDataExts[pos];
				cellExt.Structures.erase(cellIndex);
				if (cellExt.Structures.empty())
				{
					pCell->Structure = -1;
					pCell->TypeListIndex = -1;
				}
				else
				{
					pCell->Structure = cellExt.Structures.begin()->first;
					pCell->TypeListIndex = cellExt.Structures.begin()->second;
				}
				CMapData::Instance->UpdateMapPreviewAt(x, y);
			}
		}
	}
	CMapDataExt::StructureIndexMap[cellIndex] = -1;
	ini->DeleteKey("Structures", key);
	CMapDataExt::BuildingRenderDatasFix.erase(CMapDataExt::BuildingRenderDatasFix.begin() + iniIndex);
	for (int i = 0; i < CMapDataExt::StructureIndexMap.size(); ++i)
	{
		if (CMapDataExt::StructureIndexMap[i] > iniIndex)
			CMapDataExt::StructureIndexMap[i]--;
	}

	if (isLamp)
	{
		LightingSourceTint::CalculateMapLamps();
	}

	return 0x4A98AC;
}

DEFINE_HOOK(456DA0, CIsoView_OnMouseMove_UpdateStructureData, 8)
{
	if (CMapDataExt::StructureIndexMap.size() > 65500)
	{
		CMapDataExt::UpdateFieldStructureData_Optimized();
	}
	return 0;
}

DEFINE_HOOK(4AB68D, CMapData_GetStdStructureData, 7)
{
	GET_STACK(int, cellIndex, STACK_OFFS(0x74, -0x4));
	R->Stack(STACK_OFFS(0x74, -0x4), CMapDataExt::StructureIndexMap[cellIndex]);
	return 0;
}

DEFINE_HOOK(4AAE98, CMapData_GetStructureData, A)
{
	GET_STACK(int, cellIndex, STACK_OFFS(0x90, -0x4));
	R->Stack(STACK_OFFS(0x90, -0x4), CMapDataExt::StructureIndexMap[cellIndex]);
	return 0;
}

DEFINE_HOOK(4B4996, CMapData_UpdateMapFieldData_NoRndForBridge, 6)
{
	GET(DWORD, dwID6, EAX);

	int dwID = dwID6 >> 6;
	if (dwID < CMapDataExt::TileDataCount)
		if (CMapDataExt::TileData[dwID].TileSet == CMapDataExt::WoodBridgeSet)
			return 0x4B4B7E;

	return 0x4B499C;
}

DEFINE_HOOK(49ED34, CMapData_LoadMap_InitializeMapDataExt, 5)
{
	Logger::Debug("CMapData::LoadMap(): About to call InitializeAllHdmEdition()\n");
	CMapDataExt::InitializeAllHdmEdition();
	return 0;
}

DEFINE_HOOK(4B9E38, CMapData_CreateMap_InitializeMapDataExt, 5)
{
	Logger::Debug("CMapData::CreateMap(): About to call InitializeAllHdmEdition()\n");
	CMapDataExt::InitializeAllHdmEdition();
	return 0;
}

DEFINE_HOOK(4B9CB5, CMapData_CreateMap_ClearOverlay, 5)
{
	std::fill(std::begin(CMapDataExt::NewOverlay), std::end(CMapDataExt::NewOverlay), 0xFFFF);
	CMapDataExt::NewINIFormat = 4;
	return 0;
}

DEFINE_HOOK(4B8AD2, CMapData_CreateMap_FixLocalSize, 5)
{
	GET_BASE(char*, lpBuffer, -0x38);
	GET_BASE(int, dwWidth, 0x8);
	GET_BASE(int, dwHeight, 0xC);

	FString localSize;
	localSize.Format("%d,%d,%d,%d", 3, 5, dwWidth - 6, dwHeight - 11);

	memcpy(lpBuffer, localSize.c_str(), std::min(localSize.GetLength(), 40));
	lpBuffer[std::min(localSize.GetLength(), 40)] = '\0';

	return 0;
}

DEFINE_HOOK(4C5E1E, CMapData_ResizeMap_FixLocalSize, 7)
{
	REF_STACK(ppmfc::CString, lpBuffer, STACK_OFFS(0x1C4, 0x178));
	GET_STACK(int, dwWidth, STACK_OFFS(0x1C4, -0xC));
	GET(int, dwHeight, EDI);

	lpBuffer.Format("%d,%d,%d,%d", 3, 5, dwWidth - 6, dwHeight - 11);
	CMapDataExt::SkipBuildingOverlappingCheck = true;

	return 0x4C5E64;
}

DEFINE_HOOK(4C55F7, CMapData_ResizeMap_DeleteBuilding, 7)
{
	CMapDataExt::DeleteBuildingByIniID = true;
	return 0;
}

DEFINE_HOOK(4C560D, CMapData_ResizeMap_DeleteBuilding_2, 6)
{
	CMapDataExt::DeleteBuildingByIniID = false;
	return 0;
}

DEFINE_HOOK(4C536C, CMapData_ResizeMap_GetBuildingData, 7)
{
	GET(int, index, EDI);
	REF_STACK(CBuildingData, pBld, STACK_OFFS(0x1C4, 0x130));

	CMapDataExt::GetBuildingDataByIniID(index, pBld);
	return 0;
}

DEFINE_HOOK(4C6456, CMapData_ResizeMap_ResizeCellDataExts, 8)
{
	CMapDataExt::UndoRedoDatas.clear();
	CMapDataExt::UndoRedoDataIndex = -1;
	CMapDataExt::CellDataExts.clear();
	CMapDataExt::CellDataExts.resize(CMapData::Instance->CellDataCount);
	return 0;
}

DEFINE_HOOK(4C76C6, CMapData_ResizeMap_Overlay, 5)
{
	GET_STACK(int, x_move, STACK_OFFS(0x1C4, 0x19C));
	GET_STACK(int, y_move, STACK_OFFS(0x1C4, 0x194));

	auto pThis = CMapDataExt::GetExtension();
	auto original = new WORD[0x40000];
	std::copy(std::begin(pThis->NewOverlay), std::end(pThis->NewOverlay), original);
	std::fill(std::begin(pThis->NewOverlay), std::end(pThis->NewOverlay), 0xFFFF);

	for (int y = 0; y < 512; ++y)
	{
		for (int x = 0; x < 512; ++x)
		{
			int new_x = x + x_move;
			int new_y = y + y_move;

			if (new_x >= 0 && new_x < 512 && new_y >= 0 && new_y < 512)
			{
				pThis->NewOverlay[new_y * 512 + new_x] = original[y * 512 + x];
			}
		}
	}
	delete[] original;

	int u, v;
	for (u = 0; u < pThis->MapWidthPlusHeight; u++)
	{
		for (v = 0; v < pThis->MapWidthPlusHeight; v++)
		{
			pThis->CellDataExts[u + v * pThis->MapWidthPlusHeight].NewOverlay = pThis->NewOverlay[v + u * 512];
		}
	}

	return 0;
}

DEFINE_HOOK(4C7DAF, CMapData_ResizeMap_InitializeMapDataExt, 7)
{
	// load objects to avoid weird palette issue
	CIsoView::GetInstance()->PrimarySurfaceLost();
	CMapDataExt::SkipBuildingOverlappingCheck = false;

	for (int i = 0; i < CMapData::Instance->MapWidthPlusHeight; i++) {
		for (int j = 0; j < CMapData::Instance->MapWidthPlusHeight; j++) {
			CMapData::Instance->UpdateMapPreviewAt(i, j);
		}
	}

	return 0;
}

DEFINE_HOOK(4A6040, CMapData_UpdateUnits, 6)
{
	GET(CMapData*, pThis, ECX);
	GET_STACK(BOOL, bSave, 0x4);
	if (bSave == FALSE)
	{
		int i;
		for (i = 0; i < pThis->CellDataCount; i++)
		{
			pThis->CellDatas[i].Unit = -1;
		}

		if (auto pSection = CINI::CurrentDocument->GetSection("Units"))
		{
			int i = 0;
			for (const auto& data : pSection->GetEntities())
			{
				auto atoms = STDHelpers::SplitString(data.second, 4);
				int x = atoi(atoms[4]);
				int y = atoi(atoms[3]);
				int pos = pThis->GetCoordIndex(x, y);
				if (pos < pThis->CellDataCount) 
					pThis->CellDatas[pos].Unit = i;

				pThis->UpdateMapPreviewAt(x, y);
				i++;
			}
		}
	}
	return 0x4A67CD;
}

DEFINE_HOOK(4A87A0, CMapData_DeleteUnit, 7)
{
	GET(CMapData*, pThis, ECX);
	GET_STACK(int, index, 0x4);
	if (auto pSection = CINI::CurrentDocument->GetSection("Units"))
	{
		if (index < pSection->GetEntities().size())
		{
			auto& value = *pSection->GetValueAt(index);
			auto atoms = STDHelpers::SplitString(value, 4);
			int x = atoi(atoms[4]);
			int y = atoi(atoms[3]);
			CINI::CurrentDocument->DeleteKey(pSection, *pSection->GetKeyAt(index));
			pThis->UpdateFieldUnitData(false);
			pThis->UpdateMapPreviewAt(x, y);
		}
	}
	return 0x4A8F9F;
}

DEFINE_HOOK(4A4270, CMapData_UpdateAircraft, 6)
{
	GET(CMapData*, pThis, ECX);
	GET_STACK(BOOL, bSave, 0x4);
	if (bSave == FALSE)
	{
		int i;
		for (i = 0; i < pThis->CellDataCount; i++)
		{
			pThis->CellDatas[i].Aircraft = -1;
		}

		if (auto pSection = CINI::CurrentDocument->GetSection("Aircraft"))
		{
			int i = 0;
			for (const auto& data : pSection->GetEntities())
			{
				auto atoms = STDHelpers::SplitString(data.second, 4);
				int x = atoi(atoms[4]);
				int y = atoi(atoms[3]);
				int pos = pThis->GetCoordIndex(x, y);
				if (pos < pThis->CellDataCount)
					pThis->CellDatas[pos].Aircraft = i;

				pThis->UpdateMapPreviewAt(x, y);
				i++;
			}
		}
	}
	return 0x4A4A09;
}

DEFINE_HOOK(4A98B0, CMapData_DeleteAircraft, 7)
{
	GET(CMapData*, pThis, ECX);
	GET_STACK(int, index, 0x4);
	if (auto pSection = CINI::CurrentDocument->GetSection("Aircraft"))
	{
		if (index < pSection->GetEntities().size())
		{
			auto& value = *pSection->GetValueAt(index);
			auto atoms = STDHelpers::SplitString(value, 4);
			int x = atoi(atoms[4]);
			int y = atoi(atoms[3]);
			CINI::CurrentDocument->DeleteKey(pSection, *pSection->GetKeyAt(index));
			pThis->UpdateFieldAircraftData(false);
			pThis->UpdateMapPreviewAt(x, y);
		}
	}
	return 0x4AA0AF;
}

DEFINE_HOOK(4A67D0, CMapData_UpdateWaypoints, 6)
{
	GET(CMapData*, pThis, ECX);
	GET_STACK(BOOL, bSave, 0x4);
	if (bSave == FALSE)
	{
		int i;
		for (i = 0; i < pThis->CellDataCount; i++)
		{
			pThis->CellDatas[i].Waypoint = -1;
		}

		if (auto pSection = CINI::CurrentDocument->GetSection("Waypoints"))
		{
			int i = 0;
			for (const auto& data : pSection->GetEntities())
			{
				int x =  atoi(data.second) / 1000;
				int y =  atoi(data.second) % 1000;
				int pos = pThis->GetCoordIndex(x, y);
				if (pos < pThis->CellDataCount) 
					pThis->CellDatas[pos].Waypoint = i;

				if (pThis->IsMultiOnly())
				{
					int k, l;
					for (k = -1; k < 2; k++)
						for (l = -1; l < 2; l++)
							pThis->UpdateMapPreviewAt(x + k, y + l);
				}
				i++;
			}
		}
	}
	return 0x4A6FA3;
}

DEFINE_HOOK(4A7CB0, CMapData_DeleteWaypoints, 7)
{
	GET(CMapData*, pThis, ECX);
	GET_STACK(int, index, 0x4);
	if (auto pSection = CINI::CurrentDocument->GetSection("Waypoints"))
	{
		if (index < pSection->GetEntities().size())
		{
			auto& value = *pSection->GetValueAt(index);
			int x = atoi(value) / 1000;
			int y = atoi(value) % 1000;
			CINI::CurrentDocument->DeleteKey(pSection, *pSection->GetKeyAt(index));
			pThis->UpdateFieldWaypointData(false);

			if (pThis->IsMultiOnly())
			{
				int k, l;
				for (k = -1; k < 2; k++)
					for (l = -1; l < 2; l++)
						pThis->UpdateMapPreviewAt(x + k, y + l);
			}
		}
	}
	
	return 0x4A847F;
}

DEFINE_HOOK(4A6FB0, CMapData_UpdateFieldBasenodeData, 6)
{
	GET_STACK(BOOL, bSave, 0x4);
	if (bSave == FALSE)
	{
		auto& mapData = CMapData::Instance;
		for (int i = 0; i < mapData->CellDataCount; i++)
		{
			mapData->CellDatas[i].BaseNode.BuildingID = -1;
			mapData->CellDatas[i].BaseNode.BasenodeID = -1;
			mapData->CellDatas[i].BaseNode.House = "";
			CMapDataExt::CellDataExts[i].BaseNodes.clear();
		}

		if (auto pSection = CINI::CurrentDocument->GetSection("Houses"))
		{
			for (const auto& [key, house] : pSection->GetEntities())
			{
				if (auto pHouse = CINI::CurrentDocument->GetSection(house))
				{
					for (int j = 0; j < pHouse->GetInteger("NodeCount"); ++j)
					{
						char key[10];
						sprintf(key, "%03d", j);
						auto&& atoms = STDHelpers::SplitString(pHouse->GetString(key), 2);
						const auto& ID = atoms[0];
						int bnX = atoi(atoms[2]);
						int bnY = atoi(atoms[1]);

						const int BuildingIndex = CMapDataExt::GetBuildingTypeIndex(ID);
						const auto& DataExt = CMapDataExt::BuildingDataExts[BuildingIndex];
						if (!DataExt.IsCustomFoundation())
						{
							for (int dy = 0; dy < DataExt.Width; ++dy)
							{
								for (int dx = 0; dx < DataExt.Height; ++dx)
								{
									if (!CMapDataExt::IsCoordInFullMap(bnX + dx, bnY + dy))
										continue;
									int pos = mapData->GetCoordIndex(bnX + dx, bnY + dy);
									auto& cellExt = CMapDataExt::CellDataExts[pos];
									auto cell = mapData->TryGetCellAt(pos);
									cell->BaseNode.BuildingID = BuildingIndex;
									cell->BaseNode.BasenodeID = j;
									cell->BaseNode.House = house;
									cellExt.BaseNodes.push_back({ BuildingIndex , j, house, bnX, bnY, ID });
								}
							}
						}
						else
						{
							for (const auto& block : *DataExt.Foundations)
							{
								if (!CMapDataExt::IsCoordInFullMap(bnX + block.X, bnY + block.Y))
									continue;
								int pos = mapData->GetCoordIndex(bnX + block.X, bnY + block.Y);
								auto& cellExt = CMapDataExt::CellDataExts[pos];
								auto cell = mapData->TryGetCellAt(pos);
								cell->BaseNode.BuildingID = BuildingIndex;
								cell->BaseNode.BasenodeID = j;
								cell->BaseNode.House = house;
								cellExt.BaseNodes.push_back({ BuildingIndex , j, house, bnX, bnY, ID });
							}
						}
					}
				}
			}
		}
	}
	return 0x4A781E;
}

DEFINE_HOOK(4A2872, CMapData_UpdateMapPreviewAt_OverlayColor, 7)
{
	GET_STACK(RGBTRIPLE*, Color_r, STACK_OFFS(0x78, 0x68));
	GET(RGBTRIPLE*, Color, EBP);
	GET_STACK(int, pos, STACK_OFFS(0x78, -0x4));
	auto setColor = [&](unsigned char R, unsigned char G, unsigned char B) {
		Color->rgbtBlue = B;
		Color->rgbtGreen = G;
		Color->rgbtRed = R;
		Color_r->rgbtBlue = B;
		Color_r->rgbtGreen = G;
		Color_r->rgbtRed = R;
		};
	auto overlay = CMapDataExt::GetExtension()->GetOverlayAt(pos);
	if (overlay != 0xFFFF)
	{
		const auto color = CMapDataExt::GetOverlayTypeData(overlay).RadarColor;
		setColor(color.R, color.G, color.B);
	}

	return 0;
}

DEFINE_HOOK(4A28B1, CMapData_UpdateMapPreviewAt_Terrain, 7)
{
	GET_STACK(RGBTRIPLE*, Color_r, STACK_OFFS(0x78, 0x68));
	GET(RGBTRIPLE*, Color, EBP);
	REF_STACK(CellData, cell, STACK_OFFS(0x78, 0x4C));
	auto setColor = [&](unsigned char R, unsigned char G, unsigned char B) {
		Color->rgbtBlue = B;
		Color->rgbtGreen = G;
		Color->rgbtRed = R;
		Color_r->rgbtBlue = B;
		Color_r->rgbtGreen = G;
		Color_r->rgbtRed = R;
		};
	auto safeColorBtye = [](int x)
		{
			if (x > 255)
				x = 255;
			if (x < 0)
				x = 0;
			return (byte)x;
		};

	RGBTRIPLE result;
	auto& ret = LightingStruct::CurrentLighting;
	bool realColor = true;
	switch (CFinalSunDlgExt::CurrentLighting)
	{
	case 31001:
	case 31002:
	case 31003:
		break;
	default:
		result.rgbtRed = safeColorBtye(Color->rgbtRed + cell.Height * 2);
		result.rgbtGreen = safeColorBtye(Color->rgbtGreen + cell.Height * 2);
		result.rgbtBlue = safeColorBtye(Color->rgbtBlue + cell.Height * 2);
		realColor = false;
		break;
	}
	if (realColor) {
		result.rgbtRed = safeColorBtye((Color->rgbtRed * (ret.Ambient - ret.Ground + ret.Level * cell.Height)) * ret.Red);
		result.rgbtGreen = safeColorBtye((Color->rgbtGreen * (ret.Ambient - ret.Ground + ret.Level * cell.Height)) * ret.Green);
		result.rgbtBlue = safeColorBtye((Color->rgbtBlue * (ret.Ambient - ret.Ground + ret.Level * cell.Height)) * ret.Blue);
	}

	setColor(result.rgbtRed, result.rgbtGreen, result.rgbtBlue);

	return CMapData::Instance->IsMultiOnly() ? 0x4A28C0 : 0x4A2968;
}

DEFINE_HOOK(4C9EFB, CMapData_AddSmudge, 6)
{
	GET(int, pos, EBP);
	GET(uintptr_t, smudgeType, EDI);
	pos >>= 6;
	uint32_t value = *reinterpret_cast<uint32_t*>(smudgeType + 16);

	auto& cellExt = CMapDataExt::CellDataExts[pos];
	cellExt.Smudges[CMapData::Instance->CellDatas[pos].Smudge] = value;

	return 0;
}

DEFINE_HOOK(4CA1B4, CMapData_UpdateSmudge_Clear, 5)
{
	for (auto& cellExt : CMapDataExt::CellDataExts)
	{
		cellExt.Smudges.clear();
	}
	return 0;
}

DEFINE_HOOK(4CA41E, CMapData_UpdateSmudge, 8)
{
	GET(int, pos, EBP);
	GET(int, smudgeType, ECX);
	pos >>= 6;

	auto& cellExt = CMapDataExt::CellDataExts[pos];
	cellExt.Smudges[CMapData::Instance->CellDatas[pos].Smudge] = smudgeType;

	return 0;
}

DEFINE_HOOK(4C9F78, CMapData_DeleteSmudge, 6)
{
	GET(int, pos, EAX);
	auto& cellExt = CMapDataExt::CellDataExts[pos];
	auto cell = CMapData::Instance->GetCellAt(pos);
	cellExt.Smudges.erase(cell->Smudge);
	if (cellExt.Smudges.empty())
	{
		cell->Smudge = -1;
		cell->SmudgeType = -1;
	}
	else
	{
		cell->Smudge = cellExt.Smudges.begin()->first;
		cell->SmudgeType = cellExt.Smudges.begin()->second;
	}

	return 0x4C9F93;
}

DEFINE_HOOK(4B1B1F, CMapData_AddTerrain, 8)
{
	GET(int, pos, EDI);
	pos >>= 6;
	auto cell = CMapData::Instance->GetCellAt(pos);
	if (cell->Terrain > -1)
	{
		auto& cellExt = CMapDataExt::CellDataExts[pos];
		cellExt.Terrains[cell->Terrain] = cell->TerrainType;
	}

	return 0;
}

DEFINE_HOOK(4A5A72, CMapData_UpdateTerrain_Clear, 5)
{
	for (auto& cellExt: CMapDataExt::CellDataExts)
	{
		cellExt.Terrains.clear();
	}
	return 0;
}

DEFINE_HOOK(4A5C63, CMapData_UpdateTerrain, 8)
{
	GET(int, pos, EDI);
	GET(int, terrainType, ECX);
	pos >>= 6;

	auto& cellExt = CMapDataExt::CellDataExts[pos];
	cellExt.Terrains[CMapData::Instance->CellDatas[pos].Terrain] = terrainType;

	return 0;
}

DEFINE_HOOK(4AA111, CMapData_DeleteTerrain, 6)
{
	GET(int, pos, EAX);

	auto& cellExt = CMapDataExt::CellDataExts[pos];
	auto cell = CMapData::Instance->GetCellAt(pos);
	cellExt.Terrains.erase(cell->Terrain);
	if (cellExt.Terrains.empty())
	{
		cell->Terrain = -1;
		cell->TerrainType = -1;
	}
	else
	{
		cell->Terrain = cellExt.Terrains.begin()->first;
		cell->TerrainType = cellExt.Terrains.begin()->second;
	}

	return 0x4C9F93;
}

DEFINE_HOOK(4BAEE0, CMapData_UpdateFieldTubeData, 7)
{
	CMapDataExt::Tubes.clear();
	if (auto pSection = CINI::CurrentDocument->GetSection("Tubes"))
	{
		for (const auto& [key, value] : pSection->GetEntities())
		{
			auto atom = STDHelpers::SplitString(value, 5);
			auto& tube = CMapDataExt::Tubes.emplace_back();
			tube.StartCoord = { atoi(atom[1]),atoi(atom[0]) };
			tube.EndCoord = { atoi(atom[4]),atoi(atom[3]) };
			tube.StartFacing = atoi(atom[2]);
			for (int i = 5; i < atom.size(); ++i)
			{
				int facing = atoi(atom[i]);
				if (facing < 0) break;
				tube.Facings.push_back(facing);
			}
			tube.PathCoords = CIsoViewExt::GetPathFromDirections(tube.StartCoord.X, tube.StartCoord.Y, tube.Facings);

			int pos_start = tube.StartCoord.X * 1000 + tube.StartCoord.Y;
			int pos_end = tube.EndCoord.X * 1000 + tube.EndCoord.Y;
			tube.PositiveFacing = pos_end > pos_start;
			tube.key = key;
		}
	}
	return 0;
}

DEFINE_HOOK(4A7830, CMapData_UpdateFieldOverlayData, 5)
{
	GET(CMapDataExt*, pThis, ECX);
	GET_STACK(BOOL, bSave, 0x4);

	if (bSave == FALSE)
	{
		int u, v;
		for (u = 0; u < pThis->MapWidthPlusHeight; u++)
		{
			for (v = 0; v < pThis->MapWidthPlusHeight; v++)
			{
				pThis->CellDataExts[u + v * pThis->MapWidthPlusHeight].NewOverlay = pThis->NewOverlay[v + u * 512];
				pThis->CellDatas[u + v * pThis->MapWidthPlusHeight].Overlay = std::min((WORD)0xff, pThis->NewOverlay[v + u * 512]);
				pThis->CellDatas[u + v * pThis->MapWidthPlusHeight].OverlayData = pThis->OverlayData[v + u * 512];

				pThis->UpdateMapPreviewAt(u, v);
			}
		}
	}
	else
	{
		int u, v;
		for (u = 0; u < pThis->MapWidthPlusHeight; u++)
		{
			for (v = 0; v < pThis->MapWidthPlusHeight; v++)
			{
				pThis->NewOverlay[v + u * 512] = pThis->CellDataExts[u + v * pThis->MapWidthPlusHeight].NewOverlay;
				pThis->Overlay[v + u * 512] = std::min((WORD)0xff, pThis->CellDataExts[u + v * pThis->MapWidthPlusHeight].NewOverlay);
				pThis->OverlayData[v + u * 512] = pThis->CellDatas[u + v * pThis->MapWidthPlusHeight].OverlayData;
			}
		}
	}

	pThis->MoneyCount = 0;
	for (int i = 0; i < pThis->CellDataCount; ++i)
		pThis->MoneyCount += pThis->GetOreValue(pThis->CellDataExts[i].NewOverlay, pThis->CellDatas[i].OverlayData);

	return 0;
}

DEFINE_HOOK(4A29E0, CMapData_GetOverlayAt, A)
{
	GET(CMapDataExt*, pThis, ECX);
	GET_STACK(int, dwPos, 0x4);
	if (dwPos >= pThis->CellDataCount || dwPos < 0)
		R->EAX(0);
	else
		R->EAX(pThis->CellDataExts[dwPos].NewOverlay);

	return 0x4A2A00;
}

DEFINE_HOOK(469557, CMapData_GetOverlayDirection_skip, 5)
{
	return 0x46955C;
}
