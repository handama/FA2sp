#include "ControlHelpers.h"
#include "Translations.h"
#include "STDHelpers.h"
#include "MultimapHelper.h"

#include "../FA2sp.h"
#include "../Miscs/Hooks.INI.h"

#include <CMapData.h>

namespace ControlHelpers
{
    void ComboBox::LoadHouses(ppmfc::CComboBox& combobox, bool bShowIndex)
    {
        combobox.DeleteAllStrings();

        auto&& entries = Variables::Rules.ParseIndicies("Houses", true);
        CString buffer;
        for (size_t i = 0, sz = entries.size(); i < sz; ++i)
        {
            if (bShowIndex)
                buffer.Format("%u - %s", i, entries[i]);
            else
                buffer = entries[i];
            combobox.SetItemData(combobox.AddString(buffer), i);
        }

    }

    void ComboBox::LoadCountries(ppmfc::CComboBox& combobox, bool bShowIndex)
    {
        combobox.DeleteAllStrings();
        auto& doc = CINIExt::CurrentDocument();

        MultimapHelper mmh;

        mmh.AddINI(&CINI::Rules());
        mmh.AddINI(&CINIExt::CurrentDocument());

        auto&& entries = mmh.ParseIndicies("Countries", true);

        CString buffer;
        for (size_t i = 0, sz = entries.size(); i < sz; ++i)
        {
            if (bShowIndex)
                buffer.Format("%u - ", i);

            ppmfc::CString temp = CMapData::GetUIName(entries[i]);
            ppmfc::CString temp2 = mmh.GetString(entries[i], "Name");
            if (temp != "MISSING" && temp != temp2)
            {
                std::string uiname = std::string(CMapData::GetUIName(entries[i]));

                size_t index = uiname.find('(');
                uiname = uiname.substr(0, index);

                buffer += (uiname + " - ").c_str();
            }


            buffer += entries[i];

            combobox.SetItemData(combobox.AddString(buffer), i);
        }
    }

    void ComboBox::LoadGenericList(ppmfc::CComboBox& combobox, const char* pSection, bool bShowRegName, bool bShowIndex, bool bRegNameFirst)
    {
        combobox.DeleteAllStrings();

        auto&& entries = Variables::Rules.ParseIndicies(pSection, true);
        CString buffer;
        for (size_t i = 0, sz = entries.size(); i < sz; ++i)
        {
            if (!bRegNameFirst)
            {
                if (bShowIndex)
                    buffer.Format("%u - %s", i, CMapData::GetUIName(entries[i]));
                else
                    buffer = CMapData::GetUIName(entries[i]);
            }
            else
            {
                if (bShowIndex)
                    buffer.Format("%u - %s", i, entries[i]);
                else
                    buffer = entries[i];
            }
            if (bShowRegName)
                buffer += (" - " + entries[i]);
            combobox.SetItemData(combobox.AddString(buffer), i);
        }

    }

    bool ComboBox::LoadSectionList(ppmfc::CComboBox& combobox, const char* pSection, int pLoadFrom, bool bShowIndex, bool bShowUIName, bool bUseID, bool bUseStrictOrder)
    {
        combobox.DeleteAllStrings();
		bool ret = false;
        MultimapHelper mmh;
		switch (pLoadFrom)
		{
		default :
		case 0:
			mmh.AddINI(&CINI::FAData());
			break;
		case 1:
			mmh.AddINI(&CINI::Rules());
			break;
		case 2:
	        mmh.AddINI(&CINI::Rules());
			mmh.AddINI(&CINIExt::CurrentDocument());
			break;
		case 3:
			mmh.AddINI(&CINIExt::CurrentDocument());
			break;
		case 4:
			mmh.AddINI(&CINI::Art());
			break;
		case 5:
            mmh.AddINI(&CINI::FAData());
            mmh.AddINI(&CINI::Rules());
            mmh.AddINI(&CINIExt::CurrentDocument());
			break;
		}
        if (bUseStrictOrder)
        {
            auto&& entries = mmh.ParseIndicies(pSection, true);
            if (entries.size() > 0)
            {

                for (size_t i = 0, sz = entries.size(); i < sz; ++i)
                {
                    CString buffer;
                    if (bShowIndex)
                        buffer.Format("%u - ", i);

                    if (bShowUIName)
                    {
                        ppmfc::CString temp = CMapData::GetUIName(entries[i]);
                        ppmfc::CString temp2 = mmh.GetString(entries[i], "Name");
                        if (temp != "MISSING" && temp != temp2)
                        {
                            std::string uiname = std::string(CMapData::GetUIName(entries[i]));

                            size_t index = uiname.find('(');
                            if (index != std::string::npos && strcmp(pSection, "Countries") == 0) {
                                uiname = uiname.substr(0, index);
                            }

                            buffer += (uiname + " - ").c_str();
                        }
                    }


                    buffer += entries[i];


                    if (!bUseID)
                        combobox.SetItemData(combobox.AddString(buffer), i);
                    else
                        combobox.SetItemData(combobox.AddString(entries[i]), i);


                }
                ret = true;
            }
        }
        else
        {
            auto entries = mmh.GetUnorderedSection(pSection);
            if (entries.size() > 0)
            {

                for (auto& entry : entries)
                {
                    CString buffer;
                    if (bShowIndex)
                        buffer.Format("%s - ", entry.first);

                    if (bShowUIName)
                    {
                        ppmfc::CString temp = CMapData::GetUIName(entry.second);
                        ppmfc::CString temp2 = mmh.GetString(entry.second, "Name");
                        if (temp != "MISSING" && temp != temp2)
                        {
                            std::string uiname = std::string(CMapData::GetUIName(entry.second));

                            size_t index = uiname.find('(');
                            if (index != std::string::npos && strcmp(pSection, "Countries") == 0) {
                                uiname = uiname.substr(0, index);
                            }

                            buffer += (uiname + " - ").c_str();
                        }
                    }


                    buffer += entry.second;


                    if (!bUseID)
                        combobox.SetItemData(combobox.AddString(buffer), atoi(entry.first));
                    else
                        combobox.SetItemData(combobox.AddString(entry.second), atoi(entry.first));


                }
                ret = true;
            }
        }


        if (CMapData::Instance->IsMultiOnly() && strcmp(pSection, "Countries") == 0)
        {
            if (bShowIndex)
            {
                combobox.SetItemData(combobox.AddString("4475 - <Player @ A>"), 4475);
                combobox.SetItemData(combobox.AddString("4476 - <Player @ B>"), 4476);
                combobox.SetItemData(combobox.AddString("4477 - <Player @ C>"), 4477);
                combobox.SetItemData(combobox.AddString("4478 - <Player @ D>"), 4478);
                combobox.SetItemData(combobox.AddString("4479 - <Player @ E>"), 4479);
                combobox.SetItemData(combobox.AddString("4480 - <Player @ F>"), 4480);
                combobox.SetItemData(combobox.AddString("4481 - <Player @ G>"), 4481);
                combobox.SetItemData(combobox.AddString("4482 - <Player @ H>"), 4482);
            }
            else
            {
                combobox.SetItemData(combobox.AddString("<Player @ A>"), 4475);
                combobox.SetItemData(combobox.AddString("<Player @ B>"), 4476);
                combobox.SetItemData(combobox.AddString("<Player @ C>"), 4477);
                combobox.SetItemData(combobox.AddString("<Player @ D>"), 4478);
                combobox.SetItemData(combobox.AddString("<Player @ E>"), 4479);
                combobox.SetItemData(combobox.AddString("<Player @ F>"), 4480);
                combobox.SetItemData(combobox.AddString("<Player @ G>"), 4481);
                combobox.SetItemData(combobox.AddString("<Player @ H>"), 4482);
            }
        }
		return ret;
    }

    void ComboBox::LoadHousesAddon(ppmfc::CComboBox& combobox, bool bShowIndex)
    {
        if (bShowIndex)
        {
            combobox.SetItemData(combobox.AddString("-1 - 随机非中立所属方"), -1);
            combobox.SetItemData(combobox.AddString("-2 - 第一个中立所属方"), -2);
            combobox.SetItemData(combobox.AddString("-3 - 随机人类玩家"), -3);
        }
        else
        {
            combobox.SetItemData(combobox.AddString("随机非中立所属方"), -1);
            combobox.SetItemData(combobox.AddString("第一个中立所属方"), -2);
            combobox.SetItemData(combobox.AddString("随机人类玩家"), -3);
        }
	}
    void ComboBox::LoadHousesAddon2(ppmfc::CComboBox& combobox, bool bShowIndex)
    {
        if (bShowIndex)
        {
            combobox.SetItemData(combobox.AddString("-1 - 任意所属方"), -1);
            combobox.SetItemData(combobox.AddString("-2 - 触发所属方"), -2);
        }
        else
        {
            combobox.SetItemData(combobox.AddString("任意所属方"), -1);
            combobox.SetItemData(combobox.AddString("触发所属方"), -2);
        }
    }
    void ComboBox::LoadHousesAddon3(ppmfc::CComboBox& combobox, bool bShowIndex)
    {
        if (bShowIndex)
        {
            combobox.SetItemData(combobox.AddString("-1 - 任意所属方"), -1);
        }
        else
        {
            combobox.SetItemData(combobox.AddString("任意所属方"), -1);
        }
    } 
    void ComboBox::LoadHousesAddonMulti(ppmfc::CComboBox& combobox, bool bShowIndex)
    {
        if (CMapData::Instance->IsMultiOnly())
        {
            if (bShowIndex)
            {
                combobox.SetItemData(combobox.AddString("4475 - <Player @ A>"), 4475);
                combobox.SetItemData(combobox.AddString("4476 - <Player @ B>"), 4476);
                combobox.SetItemData(combobox.AddString("4477 - <Player @ C>"), 4477);
                combobox.SetItemData(combobox.AddString("4478 - <Player @ D>"), 4478);
                combobox.SetItemData(combobox.AddString("4479 - <Player @ E>"), 4479);
                combobox.SetItemData(combobox.AddString("4480 - <Player @ F>"), 4480);
                combobox.SetItemData(combobox.AddString("4481 - <Player @ G>"), 4481);
                combobox.SetItemData(combobox.AddString("4482 - <Player @ H>"), 4482);
            }
            else
            {
                combobox.SetItemData(combobox.AddString("<Player @ A>"), 4475);
                combobox.SetItemData(combobox.AddString("<Player @ B>"), 4476);
                combobox.SetItemData(combobox.AddString("<Player @ C>"), 4477);
                combobox.SetItemData(combobox.AddString("<Player @ D>"), 4478);
                combobox.SetItemData(combobox.AddString("<Player @ E>"), 4479);
                combobox.SetItemData(combobox.AddString("<Player @ F>"), 4480);
                combobox.SetItemData(combobox.AddString("<Player @ G>"), 4481);
                combobox.SetItemData(combobox.AddString("<Player @ H>"), 4482);
            }
        }
    }
    void ComboBox::LoadHousesAddonMultiAres(ppmfc::CComboBox& combobox, bool bShowIndex)
    {
        if (CMapData::Instance->IsMultiOnly())
        {
            if (bShowIndex)
            {
                combobox.SetItemData(combobox.AddString("4475 - <Player @ A> (Ares 0.A+)"), 4475);
                combobox.SetItemData(combobox.AddString("4476 - <Player @ B> (Ares 0.A+)"), 4476);
                combobox.SetItemData(combobox.AddString("4477 - <Player @ C> (Ares 0.A+)"), 4477);
                combobox.SetItemData(combobox.AddString("4478 - <Player @ D> (Ares 0.A+)"), 4478);
                combobox.SetItemData(combobox.AddString("4479 - <Player @ E> (Ares 0.A+)"), 4479);
                combobox.SetItemData(combobox.AddString("4480 - <Player @ F> (Ares 0.A+)"), 4480);
                combobox.SetItemData(combobox.AddString("4481 - <Player @ G> (Ares 0.A+)"), 4481);
                combobox.SetItemData(combobox.AddString("4482 - <Player @ H> (Ares 0.A+)"), 4482);
            }
            else
            {
                combobox.SetItemData(combobox.AddString("<Player @ A> (Ares 0.A+)"), 4475);
                combobox.SetItemData(combobox.AddString("<Player @ B> (Ares 0.A+)"), 4476);
                combobox.SetItemData(combobox.AddString("<Player @ C> (Ares 0.A+)"), 4477);
                combobox.SetItemData(combobox.AddString("<Player @ D> (Ares 0.A+)"), 4478);
                combobox.SetItemData(combobox.AddString("<Player @ E> (Ares 0.A+)"), 4479);
                combobox.SetItemData(combobox.AddString("<Player @ F> (Ares 0.A+)"), 4480);
                combobox.SetItemData(combobox.AddString("<Player @ G> (Ares 0.A+)"), 4481);
                combobox.SetItemData(combobox.AddString("<Player @ H> (Ares 0.A+)"), 4482);
            }
        }
    }
    void ComboBox::LoadTriggerParamType(ppmfc::CComboBox& combobox, int nCode)
    {
		auto& ini = CINI::FAData();
		ppmfc::CString currentCode;
		currentCode.Format("%d", nCode);
		ppmfc::CString entry = ini.GetString("TriggerParamTypes", currentCode);
		if (entry != "")
		{
			ppmfc::CString paramSection;
			int loadFrom = 0;
			bool bShowIndex = true;
            bool bUseID = false;
            bool bShowUIName = false;
            bool bUseStrictOrder = true;
			auto splits = STDHelpers::SplitString(entry);
			if (splits.size() >= 1)
			{
				paramSection = splits[0];
				STDHelpers::TrimString(paramSection);
				if (splits.size() >= 2)
					loadFrom = atoi(splits[1]);
				if (splits.size() >= 3 && atoi(splits[2]) == 0)
					bShowIndex = false;
                if (splits.size() >= 3 && atoi(splits[2]) == 2)
                {
                    bUseID = false;
                    bShowIndex = false;
                }
                   
                if (splits.size() >= 4 && atoi(splits[3]) != 0)
                    bShowUIName = true;                
                
                if (splits.size() >= 5 && atoi(splits[4]) != 0)
                    bUseStrictOrder = false;

				LoadSectionList(combobox, paramSection, loadFrom, bShowIndex, bShowUIName, bUseID, bUseStrictOrder);
				return;
			}
		}
        combobox.DeleteAllStrings();
	}
}