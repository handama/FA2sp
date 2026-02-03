#include "Common.h"
#include "CNewTrigger/CNewTrigger.h"
#include "../Helpers/STDHelpers.h"
#include "CNewScript/CNewScript.h"
#include "../Helpers/Translations.h"
#include "CObjectSearch/CObjectSearch.h"
#include "../Helpers/FString.h"
#include <regex>
#include <string>
#include <vector>
#include <algorithm>
#include <CFinalSunApp.h>
#include "../Miscs/StringtableLoader.h"
#include "../Ext/CMapData/Body.h"
#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include "Lexilla.h"

CINI& ExtraWindow::map = CINI::CurrentDocument;
CINI& ExtraWindow::fadata = CINI::FAData;
MultimapHelper& ExtraWindow::rules = Variables::RulesMap;
std::vector<DropTarget> ExtraWindow::g_DropTargets;

bool ExtraWindow::bComboLBoxSelected = false;
bool ExtraWindow::bEnterSearch = false;

FString ExtraWindow::GetTeamDisplayName(const char* id)
{
    FString name;
    name.Format("%s (%s)", id, map.GetString(id, "Name"));
    return name;
}
FString ExtraWindow::GetAITriggerDisplayName(const char* id)
{
    FString name = "";
    auto atoms = FString::SplitString(map.GetString("AITriggerTypes", id));
    if (atoms.size() < 1)
        return name;

    name.Format("%s (%s)", id, atoms[0]);
    return name;
}
void ExtraWindow::SetEditControlFontSize(HWND hWnd, float nFontSizeMultiplier, bool richEdit, const char* newFont)
{
    if (richEdit)
    {
        CHARFORMAT cf;
        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(cf);
        SendMessage(hWnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

        int currentSize = cf.yHeight / 10;
        int newSize = (int)(currentSize * nFontSizeMultiplier);

        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_SIZE | CFM_FACE;
        cf.yHeight = newSize * 10;
        if (strcmp(newFont, "") != 0)
            lstrcpy(cf.szFaceName, newFont);
        SendMessage(hWnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    }
    else
    {
        HFONT hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
        LOGFONT logfont = { 0 };

        if (hFont) {
            GetObject(hFont, sizeof(LOGFONT), &logfont);
        }
        logfont.lfHeight *= nFontSizeMultiplier;        
        if (strcmp(newFont, "") != 0)
            lstrcpy(logfont.lfFaceName, newFont);


        HFONT hNewFont = CreateFontIndirect(&logfont);
        SendMessage(hWnd, WM_SETFONT, (WPARAM)hNewFont, TRUE);

    }
}

FString ExtraWindow::FormatTriggerDisplayName(const char* id, const char* name)
{
    FString ret;
    ret.Format("%s (%s)", id, name);
    return ret;
}

FString ExtraWindow::GetTriggerDisplayName(const char* id)
{
    FString name;
    if (strcmp(id, "<none>") == 0)
        return id;
    auto atoms = FString::SplitString(map.GetString("Triggers", id, "Americans,<none>,MISSING,0,1,1,1,0"), 2);
    name.Format("%s (%s)", id, atoms[2]);
    return name;
}

FString ExtraWindow::GetTriggerName(const char* id)
{
    FString name;
    if (strcmp(id, "<none>") == 0)
        return id;
    auto atoms = FString::SplitString(map.GetString("Triggers", id, "Americans,<none>,MISSING,0,1,1,1,0"), 2);
    return atoms[2];
}

FString ExtraWindow::GetAITriggerName(const char* id)
{
    FString name;
    if (strcmp(id, "<none>") == 0)
        return id;
    auto atoms = FString::SplitString(map.GetString("AITriggerTypes", id, "MISSING"), size_t(0));
    return atoms[0];
}

FString ExtraWindow::GetTagName(const char* id)
{
    if (strcmp(id, "<none>") == 0)
        return id;
    auto atoms = FString::SplitString(map.GetString("Tags", id, "0,MISSING,01000000"), 1);
    return atoms[1];
}

FString ExtraWindow::GetTagDisplayName(const char* id)
{
    FString name;
    if (strcmp(id, "<none>") == 0)
        return id;
    auto atoms = FString::SplitString(map.GetString("Tags", id, "0,MISSING,01000000"), 1);
    name.Format("%s (%s)", id, atoms[1]);
    return name;
}

FString ExtraWindow::GetEventDisplayName(const char* id, int index, bool addIndex)
{
    FString name;
    FString name2;
    FString atom = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("EventsRA2"), id, "MISSING"), size_t(0))[0];
    atom = FString::ReplaceSpeicalString(atom);
    name.Format("%s %s", id, atom);
    if (index >= 0 && addIndex)
        name2.Format("[%d] %s", index, name);
    else name2 = name;
    return name2;
}

FString ExtraWindow::GetActionDisplayName(const char* id, int index, bool addIndex)
{
    FString name;
    FString name2;
    FString atom = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("ActionsRA2"), id, "MISSING"), size_t(0))[0];
    atom = FString::ReplaceSpeicalString(atom);
    name.Format("%s %s", id, atom);
    if (index >= 0 && addIndex)
        name2.Format("[%d] %s", index, name);
    else name2 = name;
    return name2;
}

FString ExtraWindow::GetTranslatedSectionName(const char* section)
{
    auto transed = FinalAlertConfig::Language + "-" + section;
    if (!CINI::FAData->SectionExists(transed))
        transed = section;
    return transed;
}

int ExtraWindow::FindCBStringExactStart(HWND hComboBox, const char* searchText)
{
    int itemCount = SendMessage(hComboBox, CB_GETCOUNT, 0, 0);
    for (int i = 0; i < itemCount; ++i) {
        char buffer[256]{ 0 };
        SendMessage(hComboBox, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(buffer));

        if (strncmp(buffer, searchText, strlen(searchText)) == 0) {
            return i;
        }
    }
    return -1;
}

void ExtraWindow::AdjustDropdownWidth(HWND hWnd)
{
    if (ExtConfigs::AdjustDropdownWidth)
    {
        int itemCount = (int)SendMessage(hWnd, CB_GETCOUNT, 0, 0);
        int maxWidth = 120;
        for (int i = 0; i < itemCount; ++i)
        {
            TCHAR buffer[512];
            SendMessage(hWnd, CB_GETLBTEXT, i, (LPARAM)buffer);

            int thisSize = strlen(buffer) * ExtConfigs::AdjustDropdownWidth_Factor;
            if (thisSize > maxWidth)
                maxWidth = thisSize;
            if (maxWidth > ExtConfigs::AdjustDropdownWidth_Max)
            {
                maxWidth = ExtConfigs::AdjustDropdownWidth_Max;
                break;
            }
        }
        SendMessage(hWnd, CB_SETDROPPEDWIDTH, maxWidth, 0);
    }
}

void ExtraWindow::SyncComboBoxContent(HWND hSource, HWND hTarget, bool addNone)
{
    char buffer[512]{ 0 };
    GetWindowText(hTarget, buffer, 511);

    SendMessage(hTarget, CB_RESETCONTENT, 0, 0);
    if (addNone)
        SendMessage(hTarget, CB_INSERTSTRING, 0, (LPARAM)(LPCSTR)"<none>");

    int count = (int)SendMessage(hSource, CB_GETCOUNT, 0, 0);
    for (int i = 0; i < count; i++)
    {
        char buffer[512]{ 0 };
        SendMessage(hSource, CB_GETLBTEXT, i, (LPARAM)buffer);
        if (addNone)
            SendMessage(hTarget, CB_INSERTSTRING, i + 1, (LPARAM)buffer);
        else
            SendMessage(hTarget, CB_INSERTSTRING, i, (LPARAM)buffer);
    }

    int index = SendMessage(hTarget, CB_FINDSTRINGEXACT, 0, (LPARAM)(LPCSTR)buffer);
    if (index != CB_ERR)
        SendMessage(hTarget, CB_SETCURSEL, index, NULL);
}

void ExtraWindow::LoadParams(HWND& hWnd, FString idx, CNewTrigger* instance)
{
    FString addonN1 = "-1 - ";
    FString addonN2 = "-2 - ";
    FString addonN3 = "-3 - ";

    while (SendMessage(hWnd, CB_DELETESTRING, 0, NULL) != CB_ERR);
    switch (atoi(idx)) {
    case 1:
        LoadParam_Waypoints(hWnd);
        break;
    case 2:
        LoadParam_ActionList(hWnd);
        break;
    case 3:
        LoadParam_CountryList(hWnd);
        LoadParam_HouseAddon_Multi(hWnd);

        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN1 + Translations::TranslateOrDefault("NonNeutralrandomhouse", "Non-Neutral random house")).c_str());
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN2 + Translations::TranslateOrDefault("FirstNeutralhouse", "First Neutral house")).c_str());
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN3 + Translations::TranslateOrDefault("RandomHumanplayer", "Random Human player")).c_str());
        break;
    case 4:
        LoadParam_CountryList(hWnd);
        LoadParam_HouseAddon_Multi(hWnd);
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN1 + Translations::TranslateOrDefault("Anyhouse", "Any house")).c_str());
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN2 + Translations::TranslateOrDefault("Triggerhouse", "Trigger house")).c_str());
        break;
    case 5:
        LoadParam_CountryList(hWnd);
        LoadParam_HouseAddon_Multi(hWnd);
        break;
    case 6:
        LoadParam_CountryList(hWnd);
        LoadParam_HouseAddon_MultiAres(hWnd);
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN1 + Translations::TranslateOrDefault("Anyhouse", "Any house")).c_str());
        break;
    case 7:
        LoadParam_CountryList(hWnd);
        LoadParam_HouseAddon_MultiAres(hWnd);
        break;
    case 8:
        LoadParam_TechnoTypes(hWnd);
        break;
    case 9:
        LoadParam_Triggers(hWnd, instance);
        break;
    case 10:
        if (!ExtConfigs::TutorialTexts_Viewer)
            LoadParam_Stringtables(hWnd);
        break;
    case 11:
        LoadParam_Tags(hWnd);
        break;
    case 12: // float
        if (instance == &CNewTrigger::Instance[0]) CNewTrigger::Instance[0].ActionParamUsesFloat = true;
        else if (instance == &CNewTrigger::Instance[1]) CNewTrigger::Instance[1].ActionParamUsesFloat = true;
        break;
    case 13:
        LoadParam_CountryList(hWnd);
        LoadParam_HouseAddon_Multi(hWnd);
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN1 + Translations::TranslateOrDefault("Allhouse", "All house")).c_str());
        break;
    case 14:
        LoadParam_CountryList(hWnd);
        LoadParam_HouseAddon_Multi(hWnd);
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN1 + Translations::TranslateOrDefault("CancelForceEnemy", "Cancel force enemy")).c_str());
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)(addonN2 + Translations::TranslateOrDefault("ForceNoEnemy", "Force no enemy")).c_str());
        break;
    case 15:
        LoadParam_Teamtypes(hWnd);
        break;
    default:
        if (atoi(idx) >= 500)
        {
            if (auto pSectionNewParamTypes = fadata.GetSection("NewParamTypes"))
            {
                auto atoms3 = FString::SplitString(fadata.GetString("NewParamTypes", idx), 4);
                auto sectionName = atoms3[0];
                auto& loadFrom = atoms3[1];
                auto& strictOrder = atoms3[2];
                auto& showUIName = atoms3[3];
                auto& useValue = atoms3[4];
                MultimapHelper mmh;
                
                LoadFrom(mmh, loadFrom);

                if (loadFrom == "0" || loadFrom == "fadata")
                    sectionName = ExtraWindow::GetTranslatedSectionName(sectionName);

                if (useValue == "1")
                {
                    int i = 0;
                    for (auto& kvp : mmh.GetSection(sectionName))
                    {
                        FString output;
                        output.Format("%s", kvp.second);
                        if (showUIName == "1")
                        {
                            FString uiname = CViewObjectsExt::QueryUIName(kvp.second, true);
                            if (uiname != kvp.second && uiname != "")
                            {
                                FString tmp = output;
                                output.Format("%s - %s", tmp, uiname);
                            }
                        }
                        SendMessage(hWnd, CB_INSERTSTRING, i, output);
                        i++;
                    }
                }
                else
                {
                    if (strictOrder == "1")
                    {
                        for (auto pINI : mmh.GetINIData())
                        {
                            if (pINI == &CINI::CurrentDocument)
                            {
                                // refresh indicies
                                CMapDataExt::UpdateMapSectionIndicies(sectionName);
                            }
                        }
                        auto&& entries = mmh.ParseIndicies(sectionName, true);
                        for (size_t i = 0, sz = entries.size(); i < sz; i++)
                        {
                            FString output;
                            output.Format("%d - %s", i, entries[i]);
                            if (showUIName == "1")
                            {
                                FString uiname = CViewObjectsExt::QueryUIName(entries[i], true);
                                if (uiname != entries[i] && uiname != "")
                                {
                                    FString tmp = output;
                                    output.Format("%s - %s", tmp, uiname);
                                }
                            }
                            SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)output.c_str());
                        }
                    }
                    else
                    {
                        int i = 0;
                        for (auto& kvp : mmh.GetUnorderedUnionSection(sectionName.c_str()))
                        {
                            FString output;
                            output.Format("%s - %s", kvp.first, kvp.second);
                            if (showUIName == "1")
                            {
                                FString uiname = CViewObjectsExt::QueryUIName(kvp.second, true);
                                if (uiname != kvp.second && uiname != "")
                                {
                                    FString tmp = output;
                                    output.Format("%s - %s", tmp, uiname);
                                }
                            }
                            SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)output.c_str());
                            i++;
                        }
                    }
                }
            }
        }
        break;
    }
}

void ExtraWindow::LoadParam_Waypoints(HWND& hWnd)
{
    int i = 0;
    if (auto pSection = map.GetSection("Waypoints")) 
    {
        for (auto& kvp : pSection->GetEntities())
        {
            FString output;
            int point = atoi(kvp.second);
            int x = point % 1000;
            int y = point / 1000;

            output.Format("%s - (%d, %d)", kvp.first, x, y);
            SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)output.c_str());
            i++;
        }
    }

}

void ExtraWindow::LoadParam_ActionList(HWND& hWnd)
{
    FString key;
    FString text;
    for (int i = 0; i < 50; i++)
    {
        key.Format("%d", i);
        auto value = map.GetString(CNewScript::CurrentScriptID, key.c_str());
        auto atoms = FString::SplitString(value, 1);
        key.Format("%d", i + 1);
        if (value != "")
        {
            if (CNewScript::ActionHasExtraParam[atoms[0]])
            {
                int param = atoi(atoms[1]);
                int high = param / 0x10000;
                int low = param % 0x10000;
                text.Format("%s - [%s - (%d, %d)]", key, atoms[0], low, high);
            }
            else
                text.Format("%s - [%s - %s]", key, atoms[0], atoms[1]);

            SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)text.c_str());
        }
    }
}

void ExtraWindow::LoadParam_CountryList(HWND& hWnd)
{
    MultimapHelper mmh;
    mmh.AddINI(&CINI::Rules);
    mmh.AddINI(&CINI::CurrentDocument);

    int idx = 0;
    int rIdx = 0;
    const auto& indicies = Variables::RulesMap.ParseIndicies("Countries", true);
    for (auto& value : indicies)
    {
        if (value == "Nod" || value == "GDI") {
            rIdx++;
            continue;
        }
        FString output;
            output.Format("%d - %s", rIdx, value);
            FString uiname = CViewObjectsExt::QueryUIName(value, true);
            if (uiname != value && uiname != "")
            {
                FString tmp = output;
                output.Format("%s - %s", tmp, uiname);
            }

        SendMessage(hWnd, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)output.c_str());
        rIdx++;
    }
}

void ExtraWindow::LoadParam_TechnoTypes(HWND& hWnd, int specificType, int style, bool sort)
{
    int idx = 0;

    auto addValueList = [&](const char* secName)
        {
            const auto& indicies = Variables::RulesMap.ParseIndicies(secName, true);
            for (auto& value : indicies)
            {
                FString output;
                output.Format("%s", value);
                FString uiname = CViewObjectsExt::QueryUIName(value, true);
                switch (style)
                {
                case 0:
                {
                    FString tmp = output;
                    output.Format("%s - %s", tmp, uiname);
                }
                break;
                case 1:
                {
                    FString tmp = output;
                    output.Format("%s - %s", tmp, uiname);
                }
                break;
                default:
                    break;
                }

                if (sort)
                    SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)output.c_str());
                else
                    SendMessage(hWnd, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)output.c_str());
            }
        };

    switch (specificType)
    {
    case 0:
        addValueList("BuildingTypes");
        break;
    case 1:
        addValueList("AircraftTypes");
        break;
    case 2:
        addValueList("InfantryTypes");
        break;
    case 3:
        addValueList("VehicleTypes");
        break;
    case 4:
        addValueList("AircraftTypes");
        addValueList("InfantryTypes");
        addValueList("VehicleTypes");
        break;
    default:        
        addValueList("BuildingTypes");
        addValueList("AircraftTypes");
        addValueList("InfantryTypes");
        addValueList("VehicleTypes");
        break;
    }
}

void ExtraWindow::LoadParam_HouseAddon_Multi(HWND& hWnd)
{
    if (CMapData::Instance->IsMultiOnly())
    {
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4475 - <Player @ A>");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4476 - <Player @ B>");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4477 - <Player @ C>");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4478 - <Player @ D>");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4479 - <Player @ E>");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4480 - <Player @ F>");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4481 - <Player @ G>");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4482 - <Player @ H>");
    }
}

void ExtraWindow::LoadParam_HouseAddon_MultiAres(HWND& hWnd)
{
    if (CMapData::Instance->IsMultiOnly())
    {
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4475 - <Player @ A> (Ares0.A+)");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4476 - <Player @ B> (Ares0.A+)");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4477 - <Player @ C> (Ares0.A+)");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4478 - <Player @ D> (Ares0.A+)");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4479 - <Player @ E> (Ares0.A+)");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4480 - <Player @ F> (Ares0.A+)");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4481 - <Player @ G> (Ares0.A+)");
        SendMessage(hWnd, CB_INSERTSTRING, SendMessage(hWnd, CB_GETCOUNT, NULL, NULL), (LPARAM)(LPCSTR)"4482 - <Player @ H> (Ares0.A+)");
    }
}

void ExtraWindow::LoadParam_Triggers(HWND& hWnd, CNewTrigger* instance)
{
    if (instance)
        ExtraWindow::SyncComboBoxContent(instance->hSelectedTrigger, hWnd);
    else
        ExtraWindow::SyncComboBoxContent(CNewTrigger::Instance[0].hSelectedTrigger, hWnd);
}

void ExtraWindow::LoadParam_Tags(HWND& hWnd)
{
    if (auto pSection = CINI::CurrentDocument().GetSection("Tags"))
    {
        int idx = 0;
        for (auto& kvp : pSection->GetEntities())
        {
            auto tagAtoms = FString::SplitString(kvp.second);
            if (tagAtoms.size() < 3) continue;
            FString text;
            text.Format("%s - %s", kvp.first, tagAtoms[1]);
            SendMessage(hWnd, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)text.c_str());
        }
    }
}

void ExtraWindow::LoadParam_Teamtypes(HWND& hWnd)
{
    if (auto pSection = CINI::CurrentDocument->GetSection("TeamTypes"))
    {
        int idx = 0;
        for (auto& [key, value] : pSection->GetEntities())
        {
            auto name = GetTeamDisplayName(value);
            SendMessage(hWnd, CB_INSERTSTRING, idx++, name);
        }
    }
}

#define MAX_COMBOBOX_STRING_LENGTH 192

void ExtraWindow::LoadParam_Stringtables(HWND& hWnd)
{
    for (auto& x : StringtableLoader::CSFFiles_Stringtable)
    {
        char buffer[MAX_COMBOBOX_STRING_LENGTH + 1];
        _tcsncpy(buffer, x.first + " - " + x.second, MAX_COMBOBOX_STRING_LENGTH);
        buffer[MAX_COMBOBOX_STRING_LENGTH] = _T('\0');
        SendMessage(hWnd, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)buffer);
    }

}

bool ExtraWindow::OnCloseupCComboBox(HWND& hWnd, std::map<int, FString>& labels, bool isComboboxSelectOnly)
{
    if (!labels.empty())
    {
        char buffer[512]{ 0 };
        GetWindowText(hWnd, buffer, 511);
        FString text(buffer);
        SendMessage(hWnd, CB_GETLBTEXT, SendMessage(hWnd, CB_GETCURSEL, NULL, NULL), (LPARAM)buffer);
        while (SendMessage(hWnd, CB_DELETESTRING, 0, NULL) != CB_ERR);
        for (auto& pair : labels)
            SendMessage(hWnd, CB_INSERTSTRING, pair.first, (LPARAM)(LPCSTR)pair.second.c_str());
        labels.clear();
        int idx = SendMessage(hWnd, CB_FINDSTRINGEXACT, 0, (LPARAM)buffer);
        if (idx == CB_ERR)
        {
            if (isComboboxSelectOnly)
                SendMessage(hWnd, CB_SETCURSEL, 0, NULL);
            else
                SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)text.c_str());
        }
        else
            SendMessage(hWnd, CB_SETCURSEL, idx, NULL);

        if (!ExtraWindow::bComboLBoxSelected || isComboboxSelectOnly)
        {
            ExtraWindow::bComboLBoxSelected = false;
            return false;
        }
    }
    ExtraWindow::bComboLBoxSelected = false;
    return true;
}

void ExtraWindow::OnEditCComboBox(HWND& hWnd, std::map<int, FString>& labels)
{
    if ((SendMessage(hWnd, CB_GETCOUNT, NULL, NULL) > ExtConfigs::SearchCombobox_MaxCount 
        || labels.size() > ExtConfigs::SearchCombobox_MaxCount) 
        && !bEnterSearch)
    {
        return;
    }

    ExtraWindow::bComboLBoxSelected = false;

    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (!labels.empty())
    {
        while (SendMessage(hWnd, CB_DELETESTRING, 0, NULL) != CB_ERR);
        for (auto& pair : labels)
        {
            SendMessage(hWnd, CB_INSERTSTRING, pair.first, (LPARAM)(LPCSTR)pair.second.c_str());
        }
        labels.clear();
    }

    GetWindowText(hWnd, buffer, 511);
    SendMessage(hWnd, CB_SHOWDROPDOWN, TRUE, NULL);

    std::vector<int> deletedLabels;
    for (int idx = SendMessage(hWnd, CB_GETCOUNT, NULL, NULL) - 1; idx >= 0; idx--)
    {
        SendMessage(hWnd, CB_GETLBTEXT, idx, (LPARAM)buffer2);
        bool del = false;
        FString tmp(buffer2);
        if (!(IsLabelMatch(buffer2, buffer) || strcmp(buffer, "")   == 0))
        {
            deletedLabels.push_back(idx);
        }
        labels[idx] = tmp;
    }
    for (int idx : deletedLabels)
    {
        SendMessage(hWnd, CB_DELETESTRING, idx, NULL);
    }
    if (strlen(buffer) == 1)
    {
        SetWindowText(hWnd, (LPCSTR)buffer);
        SendMessage(hWnd, CB_SETEDITSEL, 0, MAKELPARAM(1, 1));
    }
    HCURSOR hCursor = LoadCursor(NULL, IDC_ARROW);
    SetCursor(hCursor);
}

bool ExtraWindow::SortLabels(FString a, FString b)
{
    if (!ExtConfigs::SortByLabelName) {
        return a < b;
    }
    FString::TrimIndexElse(a);
    FString::TrimIndexElse(b);
    a = a.Mid(1, a.GetLength() - 2);
    b = b.Mid(1, b.GetLength() - 2);

    auto sa = std::string(a);
    auto sb = std::string(b);

    std::regex re("(\\D*)(\\d+)");
    std::sregex_iterator itA(sa.begin(), sa.end(), re);
    std::sregex_iterator itB(sb.begin(), sb.end(), re);
    std::sregex_iterator end;
    VEHGuard guard(false);

    while (itA != end && itB != end) {

        std::string prefixA = (*itA)[1].str();
        std::string prefixB = (*itB)[1].str();
        if (prefixA != prefixB) return prefixA < prefixB;

        int numA = INT_MAX;
        int numB = INT_MAX;
        try {
            numA = std::stoi((*itA)[2].str());
        }
        catch (const std::out_of_range& e)
        {
            UNREFERENCED_PARAMETER(e);
        }
        try {
            numB = std::stoi((*itB)[2].str());
        }
        catch (const std::out_of_range& e)
        {
            UNREFERENCED_PARAMETER(e);
        }
        if (numA != numB) return numA < numB;

        if (numA == INT_MAX) {
            std::string suffixA = (*itA)[2].str();
            std::string suffixB = (*itB)[2].str();
            if (suffixA != suffixB) return suffixA < suffixB;
        }
        ++itA;
        ++itB;
    }

    return sa < sb;
}

bool ExtraWindow::SortRawStrings(std::string sa, std::string sb)
{
    std::regex re("(\\D*)(\\d+)");
    std::sregex_iterator itA(sa.begin(), sa.end(), re);
    std::sregex_iterator itB(sb.begin(), sb.end(), re);
    std::sregex_iterator end;
    VEHGuard guard(false);

    while (itA != end && itB != end) {

        std::string prefixA = (*itA)[1].str();
        std::string prefixB = (*itB)[1].str();
        if (prefixA != prefixB) return prefixA < prefixB;

        int numA = INT_MAX;
        int numB = INT_MAX;
        try {
            numA = std::stoi((*itA)[2].str());
        }
        catch (const std::out_of_range& e)
        {
            UNREFERENCED_PARAMETER(e);
        }
        try {
            numB = std::stoi((*itB)[2].str());
        }
        catch (const std::out_of_range& e)
        {
            UNREFERENCED_PARAMETER(e);
        }
        if (numA != numB) return numA < numB;

        if (numA == INT_MAX) {
            std::string suffixA = (*itA)[2].str();
            std::string suffixB = (*itB)[2].str();
            if (suffixA != suffixB) return suffixA < suffixB;
        }
        ++itA;
        ++itB;
    }

    return sa < sb;
}

void ExtraWindow::SortTeams(HWND& hWnd, FString section, int& selectedIndex, FString id)
{
    while (SendMessage(hWnd, CB_DELETESTRING, 0, NULL) != CB_ERR);
    std::vector<FString> labels;
    if (auto pSection = map.GetSection(section)) {
        for (auto& pair : pSection->GetEntities()) {
            labels.push_back(ExtraWindow::GetTeamDisplayName(pair.second));
        }
    }

    bool tmp = ExtConfigs::SortByLabelName;
    
    if (section == "ScriptTypes")
        ExtConfigs::SortByLabelName = ExtConfigs::SortByLabelName_Script;
    else if (section == "TaskForces")
        ExtConfigs::SortByLabelName = ExtConfigs::SortByLabelName_Taskforce;
    else if (section == "TeamTypes")
        ExtConfigs::SortByLabelName = ExtConfigs::SortByLabelName_Team;

    std::sort(labels.begin(), labels.end(), ExtraWindow::SortLabels);

    ExtConfigs::SortByLabelName = tmp;

    for (size_t i = 0; i < labels.size(); ++i) {
        SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)labels[i].c_str());
    }
    if (id != "") {
        selectedIndex = SendMessage(hWnd, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetTeamDisplayName(id).c_str());
        SendMessage(hWnd, CB_SETCURSEL, selectedIndex, NULL);
    }
}

void ExtraWindow::SortAITriggers(HWND& hWnd, int& selectedIndex, FString id)
{
    while (SendMessage(hWnd, CB_DELETESTRING, 0, NULL) != CB_ERR);
    std::vector<FString> labels;
    if (auto pSection = map.GetSection("AITriggerTypes")) {
        for (auto& pair : pSection->GetEntities()) {
            labels.push_back(ExtraWindow::GetAITriggerDisplayName(pair.first));
        }
    }

    bool tmp = ExtConfigs::SortByLabelName;
    ExtConfigs::SortByLabelName = ExtConfigs::SortByLabelName_AITrigger;

    std::sort(labels.begin(), labels.end(), ExtraWindow::SortLabels);

    ExtConfigs::SortByLabelName = tmp;

    for (size_t i = 0; i < labels.size(); ++i) {
        SendMessage(hWnd, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)labels[i].c_str());
    }
    if (id != "") {
        selectedIndex = SendMessage(hWnd, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetAITriggerDisplayName(id).c_str());
        SendMessage(hWnd, CB_SETCURSEL, selectedIndex, NULL);
    }
}

bool ExtraWindow::IsLabelMatch(const char* target, const char* source, bool exactMatch)
{
    std::string simple_target = target;
    std::string simple_source = source;

    if (!exactMatch)
    {
        simple_target = STDHelpers::ToUpperCase(simple_target);
        simple_source = STDHelpers::ToUpperCase(simple_source);
        simple_target = STDHelpers::ChineseTraditional_ToSimple((std::string)simple_target);
        simple_source = STDHelpers::ChineseTraditional_ToSimple((std::string)simple_source);
    }

    ppmfc::CString divide = simple_source.c_str();
    STDHelpers::TrimString(divide);
    auto splits = STDHelpers::SplitString(divide, "|");
    for (auto& split : splits)
    {
        auto atoms = STDHelpers::SplitString(split, "*");
        if (atoms.size() > 1)
        {
            ppmfc::CString tmp = simple_target.c_str();
            char* pCompare = tmp.m_pchData;
            bool match = true;
            for (int i = 0; i < atoms.size(); i++)
            {
                auto& atom = atoms[i];
                if (i == 0 && exactMatch)
                {
                    if (atom == "")
                        continue;
                    else
                    {
                        if (auto pSub = (char*)_mbsstr((unsigned char*)pCompare, (unsigned char*)atom.m_pchData))
                            if (strcmp(pCompare, pSub) != 0)
                            {
                                match = false;
                                break;
                            }
                    }
                }
                else if (i == atoms.size() - 1 && exactMatch)
                {
                    if (atom == "")
                        continue;
                    else
                    {
                        if (strcmp(pCompare, atom) != 0)
                        {
                            match = false;
                            break;
                        }
                    }
                }

                if (atom == "")
                    continue;
                auto pSub = (char*)_mbsstr((unsigned char*)pCompare, (unsigned char*)atom.m_pchData);
                if (pSub != NULL)
                {
                    pCompare = pSub;
                }
                else
                {
                    match = false;
                    break;
                }
            }
            if (match)
                return true;
        }
        else
        {
            if (exactMatch)
            {
                if (strcmp(simple_target.c_str(), split) == 0)
                    return true;
            }
            else
            {
                if ((char*)_mbsstr((unsigned char*)simple_target.c_str(), (unsigned char*)split.m_pchData) != NULL)
                    return true;
            }

        }
    }
    return false;
}

FString ExtraWindow::GetCloneName(FString oriName)
{
    FString newName = oriName;
    if (ExtConfigs::CloneWithOrderedID)
    {
        std::string input(oriName);
        size_t pos = input.size();
        while (pos > 0 && std::isdigit(input[pos - 1])) {
            --pos;
        }

        if (pos == input.size()) {
            return newName + " 02";
        }

        std::string prefix = input.substr(0, pos);
        std::string numberStr = input.substr(pos);

        int number = INT_MAX;
        VEHGuard guard(false);
        try {
            number = std::stoi(numberStr);
        }
        catch (const std::out_of_range& e) {
            UNREFERENCED_PARAMETER(e);
        }
        if (number < INT_MAX) {
            ++number;
        }
        else {
            return newName + " 02";
        }
        
        std::string newNumberStr = std::to_string(number);
        while (newNumberStr.size() < numberStr.size()) {
            newNumberStr = "0" + newNumberStr;
        }

        newName = prefix.c_str();
        newName += newNumberStr.c_str();
    }
    else
        newName = oriName + " Clone";

    return newName;
}

void ExtraWindow::LoadFrom(MultimapHelper& mmh, FString loadfrom)
{
    if (loadfrom == "0" || loadfrom == "fadata")
        mmh.AddINI(&CINI::FAData);
    else if (loadfrom == "1" || loadfrom == "rules")
    {
        mmh = Variables::Rules;
    }
    else if (loadfrom == "2" || loadfrom == "rules+map")
    {
        mmh = Variables::RulesMap;
    }
    else if (loadfrom == "3" || loadfrom == "map")
        mmh.AddINI(&CINI::CurrentDocument);
    else if (loadfrom == "4" || loadfrom == "art")
        mmh.AddINI(&CINI::Art);
    else if (loadfrom == "5" || loadfrom == "sound")
        mmh.AddINI(&CINI::Sound);
    else if (loadfrom == "6" || loadfrom == "theme")
        mmh.AddINI(&CINI::Theme);
    else if (loadfrom == "7" || loadfrom == "ai+map")
    {
        mmh.AddINI(&CINI::Ai);
        mmh.AddINI(&CINI::CurrentDocument);
    }
    else if (loadfrom == "8" || loadfrom == "eva")
        mmh.AddINI(&CINI::Eva);
    else if (loadfrom == "9" || loadfrom == "theater")
        mmh.AddINI(CINI::CurrentTheater);
    else if (loadfrom == "10" || loadfrom == "ai")
        mmh.AddINI(&CINI::Ai);

}

void ExtraWindow::TrimStringIndex(FString& str) {
    str.Trim();
    int spaceIndex = str.Find(" - ");
    if (spaceIndex > 0) {
        str = str.Mid(0, spaceIndex);
    }
    str.Trim();
}

void ExtraWindow::RegisterDropTarget(HWND hWnd, DropType type, CNewTrigger* trigger)
{
    RECT rc;
    GetWindowRect(hWnd, &rc);

    g_DropTargets.push_back({ hWnd, rc, type, trigger, GetAncestor(hWnd, GA_ROOT) });
}

struct UnregisterCtx
{
    HWND hParent;
};

static void UpdateDropTargetRectChild(HWND hWnd)
{
    for (auto& t : ExtraWindow::g_DropTargets)
    {
        if (t.hWnd == hWnd)
        {
            GetWindowRect(hWnd, &t.screenRect);
            return;
        }
    }
}

static BOOL CALLBACK EnumChildProcUpdate(HWND hWnd, LPARAM lParam)
{
    auto* ctx = reinterpret_cast<UnregisterCtx*>(lParam);
    UpdateDropTargetRectChild(hWnd);
    return TRUE;
}

void ExtraWindow::UpdateDropTargetRect(HWND hWnd)
{
    UpdateDropTargetRectChild(hWnd);
    UnregisterCtx ctx{ hWnd };
    EnumChildWindows(hWnd, EnumChildProcUpdate, (LPARAM)&ctx);
}

bool IsWindowAbove(HWND a, HWND b)
{
    for (HWND h = a; h; h = GetWindow(h, GW_HWNDNEXT))
    {
        if (h == b)
            return true;
    }
    return false;
}

DropTarget ExtraWindow::FindDropTarget(POINT screenPt)
{
    std::vector<DropTarget*> sorted;
    for (auto& t : g_DropTargets)
        sorted.push_back(&t);

    std::sort(sorted.begin(), sorted.end(),
        [](DropTarget* a, DropTarget* b)
    {
        return IsWindowAbove(a->hRoot, b->hRoot);
    });

    for (auto* t : sorted)
    {
        if (PtInRect(&t->screenRect, screenPt))
            return *t;
    }
    return { nullptr, {0,0}, DropType::Unknown, nullptr, nullptr };
}

void ExtraWindow::UnregisterDropTarget(HWND hWnd)
{
    g_DropTargets.erase(
        std::remove_if(
            g_DropTargets.begin(),
            g_DropTargets.end(),
            [hWnd](const DropTarget& t)
    {
        return t.hWnd == hWnd;
    }
        ),
        g_DropTargets.end()
    );
}

static BOOL CALLBACK EnumChildProcUnregister(HWND hWnd, LPARAM lParam)
{
    auto* ctx = reinterpret_cast<UnregisterCtx*>(lParam);
    ExtraWindow::UnregisterDropTarget(hWnd);
    return TRUE;
}

void ExtraWindow::UnregisterDropTargetsOfWindow(HWND hMainWnd)
{
    UnregisterDropTarget(hMainWnd);

    UnregisterCtx ctx{ hMainWnd };
    EnumChildWindows(hMainWnd, EnumChildProcUnregister, (LPARAM)&ctx);
}

bool ExtraWindow::IsPointOnIsoViewAndNotCovered(POINT ptScreen)
{
    auto hIsoView = CIsoView::GetInstance()->GetSafeHwnd();
    RECT rc;
    GetWindowRect(hIsoView, &rc);
    if (!PtInRect(&rc, ptScreen))
        return false;

    HWND hTop = WindowFromPoint(ptScreen);

    while (hTop && hTop != hIsoView)
    {
        hTop = GetParent(hTop);
    }

    return hTop == hIsoView;
}

FString ExtraWindow::GetScintillaText(HWND hScintilla)
{
    if (!hScintilla) return {};

    size_t len = ::SendMessage(hScintilla, SCI_GETTEXTLENGTH, 0, 0);
    FString text(len + 1, '\0');
    ::SendMessage(hScintilla, SCI_GETTEXT, len + 1, (LPARAM)text.data());
    text.resize(len);
    text.toANSI();

    return text;
}

void ExtraWindow::SetScintillaText(HWND hScintilla, FString& text)
{
    if (!hScintilla) return;

    text.toUTF8();
    SendMessage(hScintilla, SCI_SETTEXT, 0, text);

    return;
}

bool ExtraWindow::HitTestListView(
    HWND hListView,
    POINT ptScreen,
    ListViewHitResult& out
)
{
    POINT ptClient = ptScreen;
    ScreenToClient(hListView, &ptClient);

    LVHITTESTINFO hti = {};
    hti.pt = ptClient;

    int item = ListView_SubItemHitTest(hListView, &hti);
    if (item < 0)
        return false;

    out.item = item;
    out.subItem = hti.iSubItem;
    out.flags = hti.flags;
    return true;
}