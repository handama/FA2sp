#include "Body.h"

#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Ext/CMapData/Body.h"
#include "../../FA2sp.h"

#include <CMapData.h>
#include "../../ExtraWindow/Common.h"

std::unordered_set<std::string> CMapValidatorExt::StructureOverlappingIgnores;
std::vector<ppmfc::CString> CMapValidatorExt::AttachedTriggers;
std::vector<ppmfc::CString> CMapValidatorExt::LoopedTriggers;

void CMapValidatorExt::ValidateOverlayLimit(BOOL& result)
{
	bool needNewIniFormat = CINI::CurrentDocument->GetInteger("Basic", "NewINIFormat", 4) > 4;
	if (!needNewIniFormat)
	{
		for (const auto& cellExt : CMapDataExt::CellDataExts)
		{
			if (cellExt.NewOverlay != 0xffff && cellExt.NewOverlay >= 0xff)
			{
				needNewIniFormat = true;
			}
		}
	}
	if (needNewIniFormat)
	{
		this->InsertString(Translations::TranslateOrDefault("MV_OverlayLimit", "Overlay index exceeds 255, map will be saved with NewINIFormat = 5."), true);
	}
}

void CMapValidatorExt::ValidateStructureOverlapping(BOOL& result)
{
	std::vector<std::vector<std::string>> Occupied;
	int length = CMapData::Instance->MapWidthPlusHeight;
	length *= length;
	Occupied.resize(length);

	if (auto pSection = CMapData::Instance->INI.GetSection("Structures"))
	{
		for (const auto& [_, Data] : pSection->GetEntities())
		{
			const auto splits = STDHelpers::SplitString(Data, 4);

			// In the list, ignore it.
			if (StructureOverlappingIgnores.count(splits[1].m_pchData))
				continue;

			const int Index = CMapDataExt::GetBuildingTypeIndex(splits[1]);
			const int Y = atoi(splits[3]);
			const int X = atoi(splits[4]);
			const auto& DataExt = CMapDataExt::BuildingDataExts[Index];

			if (!DataExt.IsCustomFoundation())
			{
				for (int dx = 0; dx < DataExt.Height; ++dx)
				{
					for (int dy = 0; dy < DataExt.Width; ++dy)
					{
						MapCoord coord = { X + dx, Y + dy };
						if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
							Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)].emplace_back(splits[1].m_pchData);
					}
				}
			}
			else
			{
				for (const auto& block : *DataExt.Foundations)
				{
					MapCoord coord = { X + block.Y, Y + block.X };
					if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
						Occupied[CMapData::Instance->GetCoordIndex(coord.X, coord.Y)].emplace_back(splits[1].m_pchData);
				}
			}
		}
	}

	ppmfc::CString Format = this->FetchLanguageString(
		"MV_OverlapStructures", "%1 structures overlap at (%2, %3): ");
	for (int i = 0; i < length; ++i)
	{
		if (Occupied[i].size() > 1)
		{
//			if (!ExtConfigs::ExtendedValidationNoError)
//				result = FALSE;
			auto buffer = Format;
			buffer.ReplaceNumString(1, Occupied[i].size());
			buffer.ReplaceNumString(2, CMapData::Instance->GetYFromCoordIndex(i));
			buffer.ReplaceNumString(3, CMapData::Instance->GetXFromCoordIndex(i));
			for (size_t k = 0; k < Occupied[i].size() - 1; ++k)
			{
				buffer += Occupied[i][k].c_str();
				buffer += ", ";
			}
			buffer += Occupied[i].back().c_str();
			this->InsertString(buffer, true);
		}
	}
}

void CMapValidatorExt::ValidateMissingParams(BOOL& result)
{
	auto ValidateSection = [&](const char* section)
	{
		if (auto pSection = CMapData::Instance->INI.GetSection(section))
		{
			ppmfc::CString Format = this->FetchLanguageString(
				"MV_LogicMissingParams", "%1 - %2 may have a missing param! Please check.");
			if (section=="Triggers")
				Format.ReplaceNumString(1, Translations::TranslateOrDefault("Trigger", "Trigger"));
			else if (section == "Actions")
				Format.ReplaceNumString(1, Translations::TranslateOrDefault("Action", "Action"));
			else if (section == "Events")
				Format.ReplaceNumString(1, Translations::TranslateOrDefault("Event", "Event"));
			else if (section == "Tags")
				Format.ReplaceNumString(1, Translations::TranslateOrDefault("Tag", "Tag"));

			for (const auto& [key, value] : pSection->GetEntities())
			{
				if (value.Find(",,") != -1) // has missing param!
				{
//					if (!ExtConfigs::ExtendedValidationNoError)
//						result = FALSE;
					auto tmp = Format;
					tmp.ReplaceNumString(2, key);
					InsertStringAsError(tmp);
				}

			}
		}
	};

	ValidateSection("Triggers");
	ValidateSection("Actions");
	ValidateSection("Events");
	ValidateSection("Tags");

	if (auto pSection = CMapData::Instance->INI.GetSection("Actions"))
	{
		ppmfc::CString Format = this->FetchLanguageString(
			"MV_LogicActionWrong", "Action - %1 has wrong params! Please check.");

		for (const auto& [key, value] : pSection->GetEntities())
		{
			auto atoms = STDHelpers::SplitString(value);
			if (atoms.size() < 1)
			{
				auto tmp = Format;
				tmp.ReplaceNumString(1, key);
				InsertStringAsError(tmp);
				continue;
			}
			int number = atoi(atoms[0]);
			if (atoms.size() != number * 8 + 1)
			{
				auto tmp = Format;
				tmp.ReplaceNumString(1, key);
				InsertStringAsError(tmp);
			}

		}
	}
	if (auto pSection = CMapData::Instance->INI.GetSection("Events"))
	{
		ppmfc::CString Format = this->FetchLanguageString(
			"MV_LogicEventWrong", "Event - %1 has wrong params! Please check.");

		
		for (const auto& [key, value] : pSection->GetEntities())
		{
			bool added = false;
			auto atoms = STDHelpers::SplitString(value);
			auto tmp = Format;
			tmp.ReplaceNumString(1, key);
			if (atoms.size() < 1)
			{
				InsertStringAsError(tmp);
				continue;
			}
			int number = atoi(atoms[0]);
			int pointer = 1;
			for (int i = 0; i < number; i++)
			{
				if (atoms.size() < pointer + 3)
				{
					InsertStringAsError(tmp);
					added = true;
					break;
				}
				auto param0 = atoms[pointer + 1];
				int eventLength = 3;
				if (param0 == "2")
					eventLength = 4;
				pointer += eventLength;
			}
			if (atoms.size() != pointer && !added)
				InsertStringAsError(tmp);
		}
	}
}

void CMapValidatorExt::ValidateRepeatingTaskforce(BOOL& result)
{
	ppmfc::CString Format1 = this->FetchLanguageString(
		"MV_ValidateRepeatingTaskforce_over5", "Taskforce - %1 has index over 5! They will be ignored by the game.");
	ppmfc::CString Format2 = this->FetchLanguageString(
		"MV_ValidateRepeatingTaskforce_repeat", "Taskforce - %1 has duplicate units! This will cause crash in game.");
	if (auto pSection = CMapData::Instance->INI.GetSection("TaskForces"))
	{
		for (auto& pair : pSection->GetEntities())
		{
			if (auto pTaskforce = CMapData::Instance->INI.GetSection(pair.second))
			{
				for (auto& pair2 : pTaskforce->GetEntities())
				{
					if (atoi(pair2.first) > 5)
					{
						auto tmp = Format1;
						tmp.ReplaceNumString(1, pair.second);
						InsertString(tmp, true);
						break;
					}
				}
				std::vector<ppmfc::CString> taskforces;
				bool end = false;
				for (int i = 0; i < 6; i++)
				{
					
					char buffer[10];
					_itoa(i, buffer, 10);
					auto pID = CMapData::Instance->INI.GetString(pair.second, buffer);
					if (pID != "")
					{
						taskforces.push_back(pID);
						for (size_t j = 0; j < taskforces.size() - 1; ++j) 
						{
							if (taskforces[j] == pID)
							{
								auto tmp = Format2;
								tmp.ReplaceNumString(1, pair.second);
								InsertStringAsError(tmp);
								end = true;
								break;
							}
						}
					}
					if (end)
						break;
				}
			}

		}
	}
}

void CMapValidatorExt::ValidateLoopTrigger_loop(ppmfc::CString attachedTrigger)
{

	auto trigger2 = CMapData::Instance->INI.GetString("Triggers", attachedTrigger);
	auto atoms2 = STDHelpers::SplitString(trigger2);
	if (atoms2.size() < 3)
		return;
	auto& attachedTrigger2 = atoms2[1];
	if (attachedTrigger2 == "<none>")
		return;
	for (auto& tri : AttachedTriggers)
	{
		if (tri == attachedTrigger2)
		{
			for (auto& tri2 : AttachedTriggers)
			{
				LoopedTriggers.push_back(tri2);
			}
			
			return;
		}
	}
	AttachedTriggers.push_back(attachedTrigger2);

	ValidateLoopTrigger_loop(attachedTrigger2);
}

void CMapValidatorExt::ValidateLoopTrigger(BOOL& result)
{
	ppmfc::CString Format1 = this->FetchLanguageString(
		"MV_ValidateLoopTrigger", "Trigger - %1 is in a loop of attached trigger£¡This will make game stuck.");

	if (auto pSection = CMapData::Instance->INI.GetSection("Triggers"))
	{
		for (auto& pair : pSection->GetEntities())
		{
			AttachedTriggers.clear();
			auto atoms = STDHelpers::SplitString(pair.second);
			if (atoms.size() < 3)
				continue;
			auto& attachedTrigger = atoms[1];

			if (attachedTrigger == "<none>")
				continue;

			AttachedTriggers.push_back(attachedTrigger);
			ValidateLoopTrigger_loop(attachedTrigger);
		}
	}
	if (LoopedTriggers.size() > 0)
	{
		std::sort(LoopedTriggers.begin(), LoopedTriggers.end());
		auto last = std::unique(LoopedTriggers.begin(), LoopedTriggers.end());
		LoopedTriggers.erase(last, LoopedTriggers.end());

		for (auto& loop : LoopedTriggers)
		{
			auto tmp = Format1;
			tmp.ReplaceNumString(1, loop);
			InsertStringAsError(tmp);
		}
	}

}


void CMapValidatorExt::ValidateBaseNode(BOOL& result)
{
	ppmfc::CString Format1 = this->FetchLanguageString(
		"MV_ValidateBaseNode", "House - %1 has wrong base nodes! Please check.");

	if (auto pSection = CMapData::Instance->INI.GetSection("Houses"))
	{
		for (auto& pair : pSection->GetEntities())
		{
			bool end = false;
			int nodeCount = CMapData::Instance->INI.GetInteger(pair.second, "NodeCount", 0);
			if (nodeCount > 0)
			{
				for (int i = 0; i < nodeCount; i++)
				{
					char key[10];
					sprintf(key, "%03d", i);
					auto value = CMapData::Instance->INI.GetString(pair.second, key, "");
					if (value == "")
					{
						end = true;
						break;
					}
					auto atoms = STDHelpers::SplitString(value);
					if (atoms.size() < 3)
					{
						end = true;
						break;
					}
				}
			}
			if (end)
			{
				auto tmp = Format1;
				tmp.ReplaceNumString(1, pair.second);
				InsertStringAsError(tmp);
			}
		}
	}
}

void CMapValidatorExt::ValidateValueLength(BOOL& result)
{
	ppmfc::CString Format1 = this->FetchLanguageString(
		"MV_ValidateValueLength128", "[%1] - Length of [%2] is over 127! It only works with ares enabled.");
	ppmfc::CString Format2 = this->FetchLanguageString(
		"MV_ValidateValueLength512", "[%1] - Length of [%2] is over 511! The excess part cannot be read!");

	for (auto& section : CMapData::Instance->INI.Dict)
	{
		int Length = 128;
		if (ExtConfigs::ExtendedValidationAres)
			Length = 512;

		if (!strcmp(section.first, "Actions") 
			|| !strcmp(section.first, "Events") 
			|| !strcmp(section.first, "AITriggerTypes")
			|| !strcmp(section.first, "Tubes"))
			Length = 512; 

		if (!strcmp(section.first, "Annotations"))
			continue;

		for (auto& pair : section.second.GetEntities())
		{
			auto line = pair.first + "=" + pair.second;
			if (line.GetLength() >= Length)
			{
				ppmfc::CString tmp;
				if (Length == 128)
					tmp = Format1;
				else
					tmp = Format2;

				tmp.ReplaceNumString(1, section.first);
				tmp.ReplaceNumString(2, pair.first);
				if (Length == 128)
					InsertString(tmp, true);
				else
					InsertStringAsError(tmp);
			}
		}

	}
}

void CMapValidatorExt::ValidateEmptyTeamTrigger(BOOL& result)
{
	ppmfc::CString Format1 = this->FetchLanguageString(
		"MV_ValidateEmptyTeamTriggerEvent", "Event - %1 has wrong Team params! Please check.");
	ppmfc::CString Format2 = this->FetchLanguageString(
		"MV_ValidateEmptyTeamTriggerAction", "Action - %1 has wrong Team params! Please check.");

	auto getLoadSectionName = [](ppmfc::CString index) {
			ppmfc::CString result = "";
			if (atoi(index) < 500)
				return result;
			if (auto pSectionNewParamTypes = CINI::FAData().GetSection("NewParamTypes"))
			{
				auto atoms3 = STDHelpers::SplitString(CINI::FAData().GetString("NewParamTypes", index), 4);
				result = atoms3[0];
			}
			return result;
		};
	auto teamExists = [](ppmfc::CString team) {
			if (auto pSection = CINI::CurrentDocument->GetSection("TeamTypes")) {
				for (auto& pair : pSection->GetEntities()) {
					if (pair.second == team)
						return true;
				}
			}
			return false;
		};

	for (auto& triggerPair : CMapDataExt::Triggers)
	{
		auto& trigger = triggerPair.second;
		bool addEvent = false;
		bool addAction = false;
		for (auto& thisEvent : trigger->Events)
		{
			auto eventInfos = FString::SplitString(CINI::FAData->GetString(ExtraWindow::GetTranslatedSectionName("EventsRA2"), thisEvent.EventNum, "MISSING,0,0,0,0,MISSING,0,1,0"), 8);
			FString paramType[2];
			paramType[0] = eventInfos[1];
			paramType[1] = eventInfos[2];
			std::vector<FString> pParamTypes[2];
			pParamTypes[0] = FString::SplitString(CINI::FAData->GetString(ExtraWindow::GetTranslatedSectionName("ParamTypes"), paramType[0], "MISSING,0"));
			pParamTypes[1] = FString::SplitString(CINI::FAData->GetString(ExtraWindow::GetTranslatedSectionName("ParamTypes"), paramType[1], "MISSING,0"));
			FString thisTeam = "-1";
			if (thisEvent.Params[0] == "2")
			{
				if (getLoadSectionName(pParamTypes[0][1]) == "TeamTypes")
				{
					thisTeam = thisEvent.Params[1];
					if (!teamExists(thisTeam)) addEvent = true;
				}
				if (getLoadSectionName(pParamTypes[1][1]) == "TeamTypes")
				{
					thisTeam = thisEvent.Params[2];
					if (!teamExists(thisTeam)) addEvent = true;
				}
			}
			else
			{
				if (getLoadSectionName(pParamTypes[1][1]) == "TeamTypes")
				{
					thisTeam = thisEvent.Params[1];
					if (!teamExists(thisTeam)) addEvent = true;
				}
			}
		}
		if (addEvent)
		{
			FString tmp = Format1;
			FString tmp2 = trigger->ID;
			tmp.ReplaceNumString(1, tmp2);
			InsertStringAsError(tmp);
		}

		for (auto& thisAction : trigger->Actions)
		{
			auto actionInfos = FString::SplitString(CINI::FAData->GetString(ExtraWindow::GetTranslatedSectionName("ActionsRA2"), thisAction.ActionNum, "MISSING,0,0,0,0,0,0,0,0,0,MISSING,0,1,0"), 13);
			FString thisTeam = "-1";
			FString paramType[7];
			for (int i = 0; i < 7; i++)
				paramType[i] = actionInfos[i + 1];

			std::vector<FString> pParamTypes[6];
			for (int i = 0; i < 6; i++)
				pParamTypes[i] = FString::SplitString(CINI::FAData->GetString(ExtraWindow::GetTranslatedSectionName("ParamTypes"), paramType[i], "MISSING,0"));

			for (int i = 0; i < 6; i++)
			{
				auto& param = pParamTypes[i];
				if (getLoadSectionName(param[1]) == "TeamTypes")
				{
					thisTeam = thisAction.Params[i];
					if (!teamExists(thisTeam)) addAction = true;
				}
			}
		}
		if (addAction)
		{
			FString tmp = Format2;
			FString tmp2 = trigger->ID;
			tmp.ReplaceNumString(1, tmp2);
			InsertStringAsError(tmp);
		}
	}
}

void CMapValidatorExt::ValidateTubes(BOOL& result)
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

	ppmfc::CString Format1 = this->FetchLanguageString(
		"MV_ValidateTubeTooLong", "Tunnel - %1 is longer than 100! Please check.");

	ppmfc::CString Format2 = this->FetchLanguageString(
		"MV_ValidateTubeUnidirection", "Tunnel - %1 is unidirectional! Please check.");

	ppmfc::CString Format3 = this->FetchLanguageString(
		"MV_ValidateTubeMultiShare", "Multiple tunnels share endpoints at %1. Please check.");

	auto getTubeName = [](const TubeData& tube)
		{
			ppmfc::CString ret;
			ret.Format("%s (%d, %d)->(%d, %d)", tube.key, tube.StartCoord.Y, tube.StartCoord.X, tube.EndCoord.Y, tube.EndCoord.X);
			return ret;
		};

	std::map<MapCoord, char> start_end_coords;
	std::set<ppmfc::CString> warnedTubes;
	for (const auto& tube : CMapDataExt::Tubes)
	{
		if (tube.PathCoords.size() > 100)
		{
			ppmfc::CString tmp = Format1;
			tmp.ReplaceNumString(1, getTubeName(tube));
			InsertStringAsError(tmp);
		}
		start_end_coords[tube.StartCoord] += 1;
		start_end_coords[tube.EndCoord] += 2;
	}
	for (auto& [coord, sum] : start_end_coords)
	{
		if (sum < 3)
		{
			for (const auto& tube : CMapDataExt::Tubes)
			{
				if (tube.StartCoord == coord || tube.EndCoord == coord)
				{
					if (warnedTubes.find(tube.key) == warnedTubes.end())
					{
						ppmfc::CString tmp = Format2;
						tmp.ReplaceNumString(1, getTubeName(tube));
						InsertStringAsError(tmp);
						warnedTubes.insert(tube.key);
						break;
					}
				}
			}
		}
		else if (sum > 3)
		{
			ppmfc::CString tmp = Format3;
			ppmfc::CString tmp2;
			tmp2.Format("(%d, %d)", coord.Y, coord.X);
			tmp.ReplaceNumString(1, tmp2);
			InsertStringAsError(tmp);
		}
	}
}

ppmfc::CString CMapValidatorExt::FetchLanguageString(const char* Key, const char* def)
{
	ppmfc::CString buffer;

	if (!Translations::GetTranslationItem(Key, buffer))
		buffer = def;

	return buffer;
}

void CMapValidatorExt::InsertStringAsError(const char* String)
{
	CLCResults.InsertItem(LVIF_TEXT | LVIF_IMAGE, CLCResults.GetItemCount(), String, NULL, NULL, ExtConfigs::ExtendedValidationNoError, NULL);
}

void CMapValidatorExt::InsertString(const char* String, bool IsWarning)
{
	CLCResults.InsertItem(LVIF_TEXT | LVIF_IMAGE, CLCResults.GetItemCount(), String, NULL, NULL, IsWarning, NULL);
}