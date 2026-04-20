#pragma once

#include "../FA2sp.h"

#include "Translations.h"
#include <MFC/ppmfc_cstring.h>
#include "STDHelpers.h"

#include <CLoading.h>
#include <CINI.h>

#include <set>
#include <vector>
#include <CMapData.h>

class TheaterHelpers
{
public:

	static ppmfc::CString GetTranslatedName(ppmfc::CString theaterName)
	{
		if (!IsAllowedTheaterName(theaterName))
			return theaterName;

		auto itemName = "TheaterName" + GetSuffix(theaterName);
		return Translations::GetTranslationItem(itemName, FA2sp::Buffer) ? FA2sp::Buffer : theaterName;
	}

	static ppmfc::CString GetSuffix(ppmfc::CString theaterName)
	{
		if (strcmp(theaterName, "TEMPERATE") == 0)
			return "Tem";
		else if (strcmp(theaterName, "SNOW") == 0)
			return "Sno";
		else if (strcmp(theaterName, "URBAN") == 0)
			return "Urb";
		else if (strcmp(theaterName, "NEWURBAN") == 0)
			return "Ubn";
		else if (strcmp(theaterName, "LUNAR") == 0)
			return "Lun";
		else if (strcmp(theaterName, "DESERT") == 0)
			return "Des";
		else
			return "";
	}

	static ppmfc::CString GetSuffix(char theaterIdentifier)
	{
		if (theaterIdentifier == 'T')
			return "Tem";
		else if (theaterIdentifier == 'A')
			return "Sno";
		else if (theaterIdentifier == 'U')
			return "Urb";
		else if (theaterIdentifier == 'N')
			return "Ubn";
		else if (theaterIdentifier == 'L')
			return "Lun";
		else if (theaterIdentifier == 'D')
			return "Des";
		else
			return "Tem";
	}

	static ppmfc::CString GetCurrentSuffix()
	{
		if (!CMapData::Instance->MapWidthPlusHeight)
			return "";

		return GetSuffix(CINI::CurrentDocument->GetString("Map", "Theater"));
	}

	static bool IsAllowedTheaterName(ppmfc::CString theaterName)
	{
		std::set<ppmfc::CString> allowedNames = { "TEMPERATE", "SNOW", "URBAN", "NEWURBAN", "LUNAR", "DESERT" };
		std::set<ppmfc::CString> mdTheaterNames = { "NEWURBAN", "LUNAR", "DESERT" };

		if (!CLoading::HasMdFile && mdTheaterNames.find(theaterName) != mdTheaterNames.end())
			return false;

		if (allowedNames.find(theaterName) != allowedNames.end())
			return true;

		return false;
	}

	static std::vector<ppmfc::CString> GetEnabledTheaterNames()
	{
		std::vector<ppmfc::CString> list;

		if (auto sides = CINI::FAData->GetSection("Theaters"))
		{
			for (auto& itr : sides->GetEntities())
			{
				if (STDHelpers::IsNoneOrEmpty(itr.second))
					continue;
				ppmfc::CString oriName = STDHelpers::SplitString(itr.second, 1)[0];


				oriName.MakeUpper();
				oriName.Trim();

				if (IsAllowedTheaterName(oriName))
					list.push_back(oriName);
			}
		}

		if (list.size() > 0)
			return list;
		else
			return { "TEMPERATE", "SNOW", "URBAN", "NEWURBAN", "LUNAR", "DESERT" };
	}

	static std::vector<std::pair<ppmfc::CString, ppmfc::CString>> GetIniTheaterNamePairs()
	{
		std::vector<std::pair<ppmfc::CString, ppmfc::CString>> list;

		if (auto sides = CINI::FAData->GetSection("Theaters"))
		{
			for (auto& itr : sides->GetEntities())
			{
				if (STDHelpers::IsNoneOrEmpty(itr.second))
					continue;

				ppmfc::CString oriName = STDHelpers::SplitString(itr.second, 2)[0];
				ppmfc::CString newName = STDHelpers::SplitString(itr.second, 2)[1];

				oriName.MakeUpper();
				oriName.Trim();
				newName.MakeUpper();
				newName.Trim();

				if (STDHelpers::IsNoneOrEmpty(oriName))
					continue;
				if (STDHelpers::IsNoneOrEmpty(newName))
					continue;

				if (IsAllowedTheaterName(oriName))
					list.push_back({ oriName, newName });
			}
		}

		if (list.size() > 0)
			return list;
		else
			return { {"TEMPERATE","TEMPERATE"}, {"SNOW","SNOW"}, {"URBAN","URBAN"},
				{"NEWURBAN","NEWURBAN"}, {"LUNAR","LUNAR"}, {"DESERT","DESERT"} };
	}

	static std::map<char, ppmfc::CString>& GetFileTheaterSuffix()
	{
		return TheaterSuffixes;
	}

	static std::map<char, char>& GetFileTheaterLetter()
	{
		return TheaterLetters;
	}

	static void InitTheaterSuffix()
	{
		TheaterSuffixes['T'] = "TEM";
		TheaterSuffixes['A'] = "SNO";
		TheaterSuffixes['U'] = "URB";
		TheaterSuffixes['N'] = "UBN";
		TheaterSuffixes['L'] = "LUN";
		TheaterSuffixes['D'] = "DES";
		TheaterLetters['T'] = 'T';
		TheaterLetters['A'] = 'A';
		TheaterLetters['U'] = 'U';
		TheaterLetters['N'] = 'N';
		TheaterLetters['L'] = 'L';
		TheaterLetters['D'] = 'D';
		if (auto pSection = CINI::FAData->GetSection("TheaterSuffixes"))
		{
			for (auto& itr : pSection->GetEntities())
			{
				ppmfc::CString key = itr.first;
				key.MakeUpper();
				auto values = STDHelpers::SplitString(itr.second, 2);
				values[0].MakeUpper();
				values[0].Mid(0, 3);
				values[1].MakeUpper();
				values[1].Mid(0, 1);

				if (!values[0].IsEmpty())
				{
					if (key == "TEMPERATE")
						TheaterSuffixes['T'] = values[0];
					else if (key == "SNOW")
						TheaterSuffixes['A'] = values[0];
					else if (key == "URBAN")
						TheaterSuffixes['U'] = values[0];
					else if (key == "NEWURBAN")
						TheaterSuffixes['N'] = values[0];
					else if (key == "LUNAR")
						TheaterSuffixes['L'] = values[0];
					else if (key == "DESERT")
						TheaterSuffixes['D'] = values[0];
				}
				if (!values[1].IsEmpty())
				{
					if (key == "TEMPERATE")
						TheaterLetters['T'] = values[1][0];
					else if (key == "SNOW")
						TheaterLetters['A'] = values[1][0];
					else if (key == "URBAN")
						TheaterLetters['U'] = values[1][0];
					else if (key == "NEWURBAN")
						TheaterLetters['N'] = values[1][0];
					else if (key == "LUNAR")
						TheaterLetters['L'] = values[1][0];
					else if (key == "DESERT")
						TheaterLetters['D'] = values[1][0];
				}
			}
		}

		RunTime::ResetStaticCharAt(0x5CEAA1, TheaterSuffixes['N']);
		RunTime::ResetStaticCharAt(0x5CEAA9, TheaterSuffixes['D']);
		RunTime::ResetStaticCharAt(0x5CEAB1, TheaterSuffixes['L']);
		RunTime::ResetStaticCharAt(0x5CEAB9, TheaterSuffixes['U']);
		RunTime::ResetStaticCharAt(0x5CEAC1, TheaterSuffixes['A']);
		RunTime::ResetStaticCharAt(0x5CEAC9, TheaterSuffixes['T']);
	}

private:
	static std::map<char, ppmfc::CString> TheaterSuffixes;
	static std::map<char, char> TheaterLetters;
};

