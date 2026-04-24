#include "Common.h"
#include "CNewTrigger/CNewTrigger.h"
#include "../Helpers/STDHelpers.h"
#include "../Helpers/WinVer.h"
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
#include "../Miscs/DialogStyle.h"
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

ATOM TargetHighlighter::window_class_atom_ = 0;
bool TargetHighlighter::class_registered_ = false;

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
    if (strcmp(id, "<none>") == 0)
        return id;
    auto name = FString::GetParam(map.GetString("Triggers", id, "Americans,<none>,MISSING,0,1,1,1,0"), 2);
    name.Format("%s (%s)", id, name);
    return name;
}

FString ExtraWindow::GetTriggerName(const char* id)
{
    if (strcmp(id, "<none>") == 0)
        return id;
    return FString::GetParam(map.GetString("Triggers", id, "Americans,<none>,MISSING,0,1,1,1,0"), 2);
}

FString ExtraWindow::GetAITriggerName(const char* id)
{
    if (strcmp(id, "<none>") == 0)
        return id;
    return FString::GetParam(map.GetString("AITriggerTypes", id, "MISSING"), 0);
}

FString ExtraWindow::GetTagName(const char* id)
{
    if (strcmp(id, "<none>") == 0)
        return id;
    return FString::GetParam(map.GetString("Tags", id, "0,MISSING,01000000"), 1);
}

FString ExtraWindow::GetTagDisplayName(const char* id)
{
    if (strcmp(id, "<none>") == 0)
        return id;
    FString name;
    name.Format("%s (%s)", id, FString::GetParam(map.GetString("Tags", id, "0,MISSING,01000000"), 1));
    return name;
}

FString ExtraWindow::GetEventDisplayName(const char* id, int index, bool addIndex)
{
    FString name;
    FString name2;
    FString atom = FString::GetParam(fadata.GetString(ExtraWindow::GetTranslatedSectionName("EventsRA2"), id, "MISSING"), 0);
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
    FString atom = FString::GetParam(fadata.GetString(ExtraWindow::GetTranslatedSectionName("ActionsRA2"), id, "MISSING"), 0);
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

    SendMessage(hTarget, WM_SETREDRAW, FALSE, 0);
    SendMessage(hTarget, CB_RESETCONTENT, 0, 0);

    int count = (int)SendMessage(hSource, CB_GETCOUNT, 0, 0);
    SendMessage(hTarget, CB_INITSTORAGE, count + (addNone ? 1 : 0), 512);

    int insertIndex = 0;

    if (addNone)
    {
        SendMessage(hTarget, CB_ADDSTRING, 0, (LPARAM)"<none>");
        insertIndex = 1;
    }

    for (int i = 0; i < count; i++)
    {
        char buffer2[512]{ 0 };
        SendMessage(hSource, CB_GETLBTEXT, i, (LPARAM)buffer2);
        SendMessage(hTarget, CB_ADDSTRING, 0, (LPARAM)buffer2);
    }

    int index = SendMessage(hTarget, CB_FINDSTRINGEXACT, 0, (LPARAM)buffer);
    if (index != CB_ERR)
        SendMessage(hTarget, CB_SETCURSEL, index, 0);

    SendMessage(hTarget, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hTarget, NULL, TRUE);
}

void ExtraWindow::LoadParams(VirtualComboBoxEx& vcb, FString idx, CNewTrigger* instance)
{
    FString addonN1 = "-1 - ";
    FString addonN2 = "-2 - ";
    FString addonN3 = "-3 - ";
    
    FString editText = vcb.GetEditText();
    vcb.Clear();
    vcb.SetEditText(editText);
    switch (atoi(idx)) {
    case 1:
        LoadParam_Waypoints(vcb);
        break;
    case 2:
        LoadParam_ActionList(vcb);
        break;
    case 3:
        LoadParam_CountryList(vcb);
        LoadParam_HouseAddon_Multi(vcb);
        vcb.AddString(addonN1 + Translations::TranslateOrDefault("NonNeutralrandomhouse", "Non-Neutral random house"));
        vcb.AddString(addonN2 + Translations::TranslateOrDefault("FirstNeutralhouse", "First Neutral house"));
        vcb.AddString(addonN3 + Translations::TranslateOrDefault("RandomHumanplayer", "Random Human player"));
        break;
    case 4:
        LoadParam_CountryList(vcb);
        LoadParam_HouseAddon_Multi(vcb);
        vcb.AddString(addonN1 + Translations::TranslateOrDefault("Anyhouse", "Any house"));
        vcb.AddString(addonN2 + Translations::TranslateOrDefault("Triggerhouse", "Trigger house"));
        break;
    case 5:
        LoadParam_CountryList(vcb);
        LoadParam_HouseAddon_Multi(vcb);
        break;
    case 6:
        LoadParam_CountryList(vcb);
        LoadParam_HouseAddon_MultiAres(vcb);
        vcb.AddString(addonN1 + Translations::TranslateOrDefault("Anyhouse", "Any house"));
        break;
    case 7:
        LoadParam_CountryList(vcb);
        LoadParam_HouseAddon_MultiAres(vcb);
        break;
    case 8:
        LoadParam_TechnoTypes(vcb);
        break;
    case 9:
        LoadParam_Triggers(vcb, instance);
        break;
    case 10:
        if (!ExtConfigs::TutorialTexts_Viewer)
            LoadParam_Stringtables(vcb);
        break;
    case 11:
        LoadParam_Tags(vcb);
        break;
    case 12: // float
        if (instance == &CNewTrigger::Instance[0]) CNewTrigger::Instance[0].ActionParamUsesFloat = true;
        else if (instance == &CNewTrigger::Instance[1]) CNewTrigger::Instance[1].ActionParamUsesFloat = true;
        break;
    case 13:
        LoadParam_CountryList(vcb);
        LoadParam_HouseAddon_Multi(vcb);
        vcb.AddString(addonN1 + Translations::TranslateOrDefault("Allhouse", "All house"));
        break;
    case 14:
        LoadParam_CountryList(vcb);
        LoadParam_HouseAddon_Multi(vcb);
        vcb.AddString(addonN1 + Translations::TranslateOrDefault("CancelForceEnemy", "Cancel force enemy"));
        vcb.AddString(addonN2 + Translations::TranslateOrDefault("ForceNoEnemy", "Force no enemy"));
        break;
    case 15:
        LoadParam_Teamtypes(vcb);
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
                    auto section = mmh.GetSection(sectionName);
                    for (auto& kvp : section)
                    {
                        FString output;
                        output.Format("%s", kvp.second);
                        if (showUIName == "1")
                        {
                            FString uiname = CViewObjectsExt::QueryUIName(kvp.second, true);
                            if (uiname != kvp.second && uiname != "")
                            {
                                output.Format("%s - %s", output, uiname);
                            }
                        }
                        vcb.AddString(output);
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
                                    output.Format("%s - %s", output, uiname);
                                }
                            }
                            vcb.AddString(output);
                        }
                    }
                    else
                    {
                        int i = 0;
                        auto section = mmh.GetUnorderedUnionSection(sectionName);
                        for (auto& kvp : section)
                        {
                            FString output;
                            output.Format("%s - %s", kvp.first, kvp.second);
                            if (showUIName == "1")
                            {
                                FString uiname = CViewObjectsExt::QueryUIName(kvp.second, true);
                                if (uiname != kvp.second && uiname != "")
                                {
                                    output.Format("%s - %s", output, uiname);
                                }
                            }
                            vcb.AddString(output);
                            i++;
                        }
                    }
                }
            }
        }
        break;
    }
}

void ExtraWindow::LoadParam_Waypoints(VirtualComboBoxEx& vcb)
{
    if (auto pSection = map.GetSection("Waypoints")) 
    {
        for (auto& kvp : pSection->GetEntities())
        {
            FString output;
            int point = atoi(kvp.second);
            int x = point % 1000;
            int y = point / 1000;

            output.Format("%s - (%d, %d)", kvp.first, x, y);
            vcb.AddString(output);
        }
    }
}

void ExtraWindow::LoadParam_ActionList(VirtualComboBoxEx& vcb)
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

            vcb.AddString(text);
        }
    }
}

void ExtraWindow::LoadParam_CountryList(VirtualComboBoxEx& vcb)
{
    MultimapHelper mmh;
    mmh.AddINI(&CINI::Rules);
    mmh.AddINI(&CINI::CurrentDocument);

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
            output.Format("%s - %s", output, uiname);
        }

        vcb.AddString(output);
        rIdx++;
    }
}

void ExtraWindow::LoadParam_TechnoTypes(VirtualComboBoxEx& vcb, int specificType, int style, bool sort)
{
    int idx = 0;
    std::vector<FString> technos;
    auto addValueList = [&](const char* secName)
    {
        for (auto& [key, value] : Variables::RulesMap.GetSection(secName))
        {
            technos.push_back(value);
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

    vcb.Clear();
    if (sort)
        SortRawStrings(technos);
    for (auto& value : technos)
    {
        FString uiname = CViewObjectsExt::QueryUIName(value, true);
        switch (style)
        {
        case 0:
        {
            value.Format("%s - %s", value, uiname);
        }
        break;
        case 1:
        {
            value.Format("%s - %s", value, uiname);
        }
        break;
        default:
            break;
        }
    }

    vcb.AddStrings(technos);
}

void ExtraWindow::LoadParam_HouseAddon_Multi(VirtualComboBoxEx& vcb)
{
    if (CMapData::Instance->IsMultiOnly())
    {
        vcb.AddString("4475 - <Player @ A>");
        vcb.AddString("4476 - <Player @ B>");
        vcb.AddString("4477 - <Player @ C>");
        vcb.AddString("4478 - <Player @ D>");
        vcb.AddString("4479 - <Player @ E>");
        vcb.AddString("4480 - <Player @ F>");
        vcb.AddString("4481 - <Player @ G>");
        vcb.AddString("4482 - <Player @ H>");
    }
}

void ExtraWindow::LoadParam_HouseAddon_MultiAres(VirtualComboBoxEx& vcb)
{
    if (CMapData::Instance->IsMultiOnly())
    {
        vcb.AddString("4475 - <Player @ A> (Ares0.A+)");
        vcb.AddString("4476 - <Player @ B> (Ares0.A+)");
        vcb.AddString("4477 - <Player @ C> (Ares0.A+)");
        vcb.AddString("4478 - <Player @ D> (Ares0.A+)");
        vcb.AddString("4479 - <Player @ E> (Ares0.A+)");
        vcb.AddString("4480 - <Player @ F> (Ares0.A+)");
        vcb.AddString("4481 - <Player @ G> (Ares0.A+)");
        vcb.AddString("4482 - <Player @ H> (Ares0.A+)");
    }
}

void ExtraWindow::LoadParam_Triggers(VirtualComboBoxEx& vcb, CNewTrigger* instance)
{
    if (instance)
        vcb.CopyFrom(instance->vcbSelectedTrigger);
    else
        vcb.CopyFrom(CNewTrigger::Instance[0].vcbSelectedTrigger);
}

void ExtraWindow::LoadParam_Tags(VirtualComboBoxEx& vcb)
{
    if (auto pSection = CINI::CurrentDocument().GetSection("Tags"))
    {
        FString text;
        for (auto& kvp : pSection->GetEntities())
        {
            auto tagAtoms = FString::SplitString(kvp.second);
            if (tagAtoms.size() < 3) continue;
            text.Format("%s - %s", kvp.first, tagAtoms[1]);
            vcb.AddString(text);
        }
    }
}

void ExtraWindow::LoadParam_Teamtypes(VirtualComboBoxEx& vcb)
{
    if (auto pSection = CINI::CurrentDocument->GetSection("TeamTypes"))
    { 
        for (auto& [key, value] : pSection->GetEntities())
        {
            auto name = GetTeamDisplayName(value);
            vcb.AddString(name);
        }
    }
}

#define MAX_COMBOBOX_STRING_LENGTH 128
void ExtraWindow::LoadParam_Stringtables(VirtualComboBoxEx& vcb)
{
    for (auto& x : StringtableLoader::CSFFiles_Stringtable)
    {
        char buffer[MAX_COMBOBOX_STRING_LENGTH + 1];
        _tcsncpy(buffer, x.first + " - " + x.second, MAX_COMBOBOX_STRING_LENGTH);
        buffer[MAX_COMBOBOX_STRING_LENGTH] = _T('\0');
        vcb.AddString(buffer);
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
            SendMessage(hWnd, CB_INSERTSTRING, pair.first, pair.second);
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
            SendMessage(hWnd, CB_INSERTSTRING, pair.first, pair.second);
        }
        labels.clear();
    }

    GetWindowText(hWnd, buffer, 511);
    SendMessage(hWnd, CB_SHOWDROPDOWN, TRUE, NULL);

    std::vector<int> deletedLabels;
    LabelMatcher matcher(buffer);
    for (int idx = SendMessage(hWnd, CB_GETCOUNT, NULL, NULL) - 1; idx >= 0; idx--)
    {
        SendMessage(hWnd, CB_GETLBTEXT, idx, (LPARAM)buffer2);
        bool del = false;
        FString tmp(buffer2);
        if (!(matcher.Match(buffer2) || strcmp(buffer, "")   == 0))
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

static SortLabelKey BuildKey(const FString& input)
{
    SortLabelKey key;

    FString s = input;
    FString::TrimIndexElse(s);
    s = s.Mid(1, s.GetLength() - 2);

    key.original = input;

    const char* str = s.c_str();
    std::string text;

    while (*str)
    {
        if (*str >= '0' && *str <= '9')
        {
            if (!text.empty())
            {
                key.parts.push_back({ text, "", false });
                text.clear();
            }

            const char* start = str;
            while (*str >= '0' && *str <= '9')
                ++str;

            key.parts.push_back({ "", std::string(start, str - start), true });
        }
        else
        {
            text += *str;
            ++str;
        }
    }

    if (!text.empty())
        key.parts.push_back({ text, "", false });

    return key;
}

static int CompareNumber(const std::string& a, const std::string& b)
{
    size_t i = 0; while (i < a.size() && a[i] == '0') ++i;
    size_t j = 0; while (j < b.size() && b[j] == '0') ++j;

    size_t lenA = a.size() - i;
    size_t lenB = b.size() - j;

    if (lenA != lenB)
        return (lenA < lenB) ? -1 : 1;

    int cmp = std::memcmp(a.data() + i, b.data() + j, lenA);
    if (cmp < 0) return -1;
    if (cmp > 0) return 1;

    return 0;
}

static bool CompareKey(const SortLabelKey& a, const SortLabelKey& b)
{
    const auto& pa = a.parts;
    const auto& pb = b.parts;

    size_t n = std::min(pa.size(), pb.size());

    for (size_t i = 0; i < n; ++i)
    {
        const auto& A = pa[i];
        const auto& B = pb[i];

        if (!A.isNumber && !B.isNumber)
        {
            if (A.text != B.text)
                return A.text < B.text;
        }
        else if (A.isNumber && B.isNumber)
        {
            int cmp = CompareNumber(A.number, B.number);
            if (cmp != 0)
                return cmp < 0;
        }
        else
        {
            return A.isNumber;
        }
    }

    if (pa.size() != pb.size())
        return pa.size() < pb.size();

    return a.original < b.original;
}

void ExtraWindow::SortLabels(std::vector<FString>& labels)
{
    if (!ExtConfigs::SortByLabelName)
    {
        std::sort(labels.begin(), labels.end());
        return;
    }

    std::vector<SortLabelKey> keys;
    keys.reserve(labels.size());

    for (const auto& l : labels)
        keys.push_back(BuildKey(l));

    std::sort(keys.begin(), keys.end(), CompareKey);

    for (size_t i = 0; i < labels.size(); ++i)
        labels[i] = std::move(keys[i].original);
}

void ExtraWindow::SortLabels(std::vector<std::pair<FString, FString>>& labels, bool first)
{
    if (!ExtConfigs::SortByLabelName)
    {
        std::sort(labels.begin(), labels.end(),
            [first](const auto& a, const auto& b)
        {
            return first ? (a.first < b.first) : (a.second < b.second);
        });
        return;
    }

    std::vector<SortKeyIndex> keys;
    keys.reserve(labels.size());

    for (size_t i = 0; i < labels.size(); ++i)
    {
        const FString& target = first ? labels[i].first : labels[i].second;
        keys.push_back({ BuildKey(target), i });
    }

    std::sort(keys.begin(), keys.end(),
        [](const SortKeyIndex& a, const SortKeyIndex& b)
    {
        return CompareKey(a.key, b.key);
    });

    std::vector<std::pair<FString, FString>> temp;
    temp.reserve(labels.size());

    for (auto& k : keys)
    {
        temp.push_back(std::move(labels[k.index]));
    }

    labels = std::move(temp);
}

static SortLabelKey BuildRawKey(const FString& input)
{
    SortLabelKey key;

    key.original = input;

    const char* str = input.c_str();
    std::string text;

    while (*str)
    {
        if (*str >= '0' && *str <= '9')
        {
            if (!text.empty())
            {
                key.parts.push_back({ text, "", false });
                text.clear();
            }

            const char* start = str;
            while (*str >= '0' && *str <= '9')
                ++str;

            key.parts.push_back({ "", std::string(start, str - start), true });
        }
        else
        {
            text += *str;
            ++str;
        }
    }

    if (!text.empty())
        key.parts.push_back({ text, "", false });

    return key;
}

void ExtraWindow::SortRawStrings(std::vector<FString>& labels)
{
    std::vector<SortLabelKey> keys;
    keys.reserve(labels.size());

    for (const auto& l : labels)
        keys.push_back(BuildRawKey(l));

    std::sort(keys.begin(), keys.end(), CompareKey);

    for (size_t i = 0; i < labels.size(); ++i)
        labels[i] = std::move(keys[i].original);
}

void ExtraWindow::SortRawStrings(std::vector<std::pair<FString, FString>>& labels, bool first)
{
    std::vector<SortKeyIndex> keys;
    keys.reserve(labels.size());

    for (size_t i = 0; i < labels.size(); ++i)
    {
        const FString& target = first ? labels[i].first : labels[i].second;
        keys.push_back({ BuildRawKey(target), i });
    }

    std::sort(keys.begin(), keys.end(),
        [](const SortKeyIndex& a, const SortKeyIndex& b)
    {
        return CompareKey(a.key, b.key);
    });

    std::vector<std::pair<FString, FString>> temp;
    temp.reserve(labels.size());

    for (auto& k : keys)
    {
        temp.push_back(std::move(labels[k.index]));
    }

    labels = std::move(temp);
}

void ExtraWindow::SortRawStrings(std::vector<std::pair<std::string, std::string>>& labels, bool first)
{
    std::vector<SortKeyIndex> keys;
    keys.reserve(labels.size());

    for (size_t i = 0; i < labels.size(); ++i)
    {
        const std::string& target = first ? labels[i].first : labels[i].second;
        keys.push_back({ BuildRawKey(target), i });
    }

    std::sort(keys.begin(), keys.end(),
        [](const SortKeyIndex& a, const SortKeyIndex& b)
    {
        return CompareKey(a.key, b.key);
    });

    std::vector<std::pair<std::string, std::string>> temp;
    temp.reserve(labels.size());

    for (auto& k : keys)
    {
        temp.push_back(std::move(labels[k.index]));
    }

    labels = std::move(temp);
}

void ExtraWindow::SortTeams(HWND& hWnd, FString section, int& selectedIndex, FString id)
{
    ExtraWindow::ClearComboKeepText(hWnd);
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

    ExtraWindow::SortLabels(labels);

    ExtConfigs::SortByLabelName = tmp;

    ComboBoxBatchUpdater t(hWnd, labels.size(), false, 512, false);
    for (size_t i = 0; i < labels.size(); ++i) {
        SendMessage(hWnd, CB_ADDSTRING, 0, labels[i]);
    }
    if (id != "") {
        selectedIndex = SendMessage(hWnd, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetTeamDisplayName(id).c_str());
        SendMessage(hWnd, CB_SETCURSEL, selectedIndex, NULL);
    }
}

void ExtraWindow::SortTeams(VirtualComboBoxEx& vcb, FString section, int& selectedIndex, FString id, bool clear)
{
    if (clear)
        vcb.Clear();
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

    ExtraWindow::SortLabels(labels);

    ExtConfigs::SortByLabelName = tmp;

    vcb.AddStrings(labels);
    if (id != "") {
        selectedIndex = vcb.FindStringExact(ExtraWindow::GetTeamDisplayName(id));
        vcb.SetCurSel(selectedIndex);
    }
}

bool ExtraWindow::IsLabelMatch(const char* target, const char* source, bool exactMatch)
{
    FString simple_target = target;
    FString simple_source = source;

    if (!exactMatch)
    {
        simple_target.MakeUpper();
        simple_source.MakeUpper();
        simple_target = STDHelpers::ChineseTraditional_ToSimple(simple_target);
        simple_source = STDHelpers::ChineseTraditional_ToSimple(simple_source);
    }

    simple_source.Trim();
    auto splits = FString::SplitString(simple_source, "|");
    for (auto& split : splits)
    {
        auto atoms = FString::SplitString(split, "*");
        if (atoms.size() > 1)
        {
            FString compare = simple_target;
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
                        if (auto pSub = (char*)_mbsstr((const unsigned char*)compare.c_str(), (const unsigned char*)atom.c_str()))
                            if (strcmp(compare, pSub) != 0)
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
                        if (strcmp(compare, atom) != 0)
                        {
                            match = false;
                            break;
                        }
                    }
                }

                if (atom == "")
                    continue;
                auto pSub = (char*)_mbsstr((const unsigned char*)compare.c_str(), (const unsigned char*)atom.c_str());
                if (pSub != NULL)
                {
                    compare = pSub;
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
                if ((char*)_mbsstr((unsigned char*)simple_target.c_str(), (unsigned char*)split.c_str()) != NULL)
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

void ExtraWindow::ClearComboKeepText(HWND hWnd)
{
    char buffer[512]{ 0 };
    GetWindowText(hWnd, buffer, 511);
    SendMessage(hWnd, CB_RESETCONTENT, 0, 0);
    SetWindowText(hWnd, buffer);
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
    ::SendMessage(hScintilla, SCI_EMPTYUNDOBUFFER, 0, 0);

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

void ExtraWindow::UpdateListBoxHScroll(HWND hListBox)
{
    HDC hdc = GetDC(hListBox);
    HFONT hFont = (HFONT)SendMessage(hListBox, WM_GETFONT, 0, 0);
    HFONT old = (HFONT)SelectObject(hdc, hFont);

    int count = (int)SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    int maxWidth = 0;

    for (int i = 0; i < count; ++i)
    {
        int len = (int)SendMessageW(hListBox, LB_GETTEXTLEN, i, 0);
        if (len <= 0) continue;

        std::wstring text(len + 1, L'\0');
        SendMessageW(hListBox, LB_GETTEXT, i, (LPARAM)text.data());

        SIZE sz{};
        GetTextExtentPoint32W(hdc, text.c_str(), len, &sz);
        maxWidth = MAX(maxWidth, (int)sz.cx);
    }

    SelectObject(hdc, old);
    ReleaseDC(hListBox, hdc);

    SendMessage(hListBox, LB_SETHORIZONTALEXTENT, maxWidth + 10, 0);
}

void HelpDlg::CreateHelpDlg(HWND& hParent, const FString& Title, const FString& Text)
{
    if (hDlg) return;

    hDlg = CreateDialogParam(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(334),
        hParent,
        HelpDlgProc,
        reinterpret_cast<LPARAM>(this)
    );

    if (hDlg)
    {
        ShowWindow(hDlg, SW_SHOW);
        hText = GetDlgItem(hDlg, 1000);
        SetWindowText(hDlg, Title);
        SetWindowText(hText, Text);
        ExtraWindow::SetEditControlFontSize(hText, 1.2f);

        RECT rect;
        GetClientRect(hDlg, &rect);
        origWndWidth = rect.right - rect.left;
        origWndHeight = rect.bottom - rect.top;
        minSizeSet = false;
    }
    else
    {
        Logger::Error("Failed to create HelpDlg.\n");
    }
}

BOOL CALLBACK HelpDlg::HelpDlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    HelpDlg* self = nullptr;

    if (Msg == WM_INITDIALOG)
    {
        self = reinterpret_cast<HelpDlg*>(lParam);
        if (self == nullptr) return FALSE;

        self->hDlg = hWnd;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)self);
        return TRUE;
    }

    self = reinterpret_cast<HelpDlg*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (self == nullptr)
        return FALSE;
    return self->HandleMsg(hWnd, Msg, wParam, lParam);
}

BOOL CALLBACK HelpDlg::HandleMsg(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        return TRUE;
    }
    case WM_GETMINMAXINFO: 
    {
        if (!minSizeSet) {
            int borderWidth = GetSystemMetrics(SM_CXBORDER);
            int borderHeight = GetSystemMetrics(SM_CYBORDER);
            int captionHeight = GetSystemMetrics(SM_CYCAPTION);
            minWndWidth = origWndWidth + 2 * borderWidth;
            minWndHeight = origWndHeight + captionHeight + 2 * borderHeight;
            minSizeSet = true;
        }
        MINMAXINFO* pMinMax = (MINMAXINFO*)lParam;
        pMinMax->ptMinTrackSize.x = minWndWidth;
        pMinMax->ptMinTrackSize.y = minWndHeight;
        return TRUE;
    }
    case WM_SIZE: 
    {
        int newWndWidth = LOWORD(lParam);
        int newWndHeight = HIWORD(lParam);

        RECT rect;
        GetWindowRect(hText, &rect);
        POINT topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);
        int newWidth = rect.right - rect.left + newWndWidth - origWndWidth;
        int newHeight = rect.bottom - rect.top + newWndHeight - origWndHeight;
        MoveWindow(hText, topLeft.x, topLeft.y, newWidth, newHeight, TRUE);

        origWndWidth = newWndWidth;
        origWndHeight = newWndHeight;
        break;
    }
    case WM_CLOSE:
    {
        CloseHelpDlg();
        return TRUE;
    }
    }
    return FALSE;
}

void HelpDlg::CloseHelpDlg()
{
    EndDialog(hDlg, NULL);
    hDlg = NULL;
    hText = NULL;
}

TargetHighlighter::TargetHighlighter() = default;

TargetHighlighter::~TargetHighlighter() {
    Detach();
}

bool TargetHighlighter::RegisterWindowClassIfNeeded() {
    if (class_registered_) return true;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"TargetHighlighterBorder";
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);

    window_class_atom_ = RegisterClassExW(&wc);
    if (window_class_atom_ == 0) {
        return false;
    }

    class_registered_ = true;
    return true;
}

HWND TargetHighlighter::CreateHighlightWindow(const RECT& rect) {
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    if (width < 4 || height < 4) return nullptr;

    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST |
        WS_EX_TOOLWINDOW |
        WS_EX_TRANSPARENT |
        WS_EX_NOACTIVATE,
        MAKEINTATOM(window_class_atom_),
        "",
        WS_POPUP,
        rect.left,
        rect.top,
        width,
        height,
        nullptr,
        nullptr,
        GetModuleHandle(nullptr),
        this
    );

    if (!hwnd) return nullptr;

    UpdateRegion(hwnd, width, height);

    ShowWindow(hwnd, SW_SHOWNA);
    return hwnd;
}

void TargetHighlighter::UpdateRegion(HWND hwnd, int width, int height)
{
    int t = border_thickness_;
    if (t <= 0) return;

    HRGN outer;
    HRGN inner;

    if (border_radius_ > 0) {
        outer = CreateRoundRectRgn(
            0, 0, width, height,
            border_radius_ * 2,
            border_radius_ * 2);

        inner = CreateRoundRectRgn(
            t, t, width - t, height - t,
            border_radius_ * 2,
            border_radius_ * 2);
    }
    else {
        outer = CreateRectRgn(0, 0, width, height);
        inner = CreateRectRgn(t, t, width - t, height - t);
    }

    CombineRgn(outer, outer, inner, RGN_DIFF);

    SetWindowRgn(hwnd, outer, TRUE);

    DeleteObject(inner);
}

void TargetHighlighter::Attach(DropTarget target) {
    Detach();

    auto hTarget = target.hWnd;
    if (!IsWindow(hTarget)) {
        return;
    }

    if (!RegisterWindowClassIfNeeded()) {
        return;
    }

    target_hwnd_ = hTarget;

    RECT rc{};

    if (target.type == DropType::BatchTriggerListView)
    {
        ListViewHitResult hit;
        POINT pt;
        GetCursorPos(&pt);
        if (ExtraWindow::HitTestListView(hTarget, pt, hit))
        {
            auto row = hit.item;
            auto col = hit.subItem;
            RECT sub{};
            sub.top = col;

            if (col == 0)
            {
                if (!ListView_GetItemRect(hTarget, row, &sub, LVIR_BOUNDS))
                {
                    target_hwnd_ = nullptr;
                    return;
                }

                int colWidth = ListView_GetColumnWidth(hTarget, 0);
                sub.right = sub.left + colWidth;
            }
            else
            {
                sub.top = col;

                if (!ListView_GetSubItemRect(hTarget, row, col, LVIR_BOUNDS, &sub))
                {
                    target_hwnd_ = nullptr;
                    return;
                }
            }

            POINT pt1{ sub.left, sub.top };
            POINT pt2{ sub.right, sub.bottom };

            ClientToScreen(hTarget, &pt1);
            ClientToScreen(hTarget, &pt2);

            rc.left = pt1.x;
            rc.top = pt1.y;
            rc.right = pt2.x;
            rc.bottom = pt2.y;

            col_ = col;
            row_ = row;
        }
        else
        {
            target_hwnd_ = nullptr;
            return;
        }
    }
    else
    {
        if (!GetWindowRect(target_hwnd_, &rc)) {
            target_hwnd_ = nullptr;
            return;
        }
    }

    highlight_hwnd_ = CreateHighlightWindow(rc);
    if (!highlight_hwnd_) {
        target_hwnd_ = nullptr;
    }
}

void TargetHighlighter::Detach() {
    DestroyHighlightWindow();
    target_hwnd_ = nullptr;
}

void TargetHighlighter::DestroyHighlightWindow() {
    if (highlight_hwnd_) {
        DestroyWindow(highlight_hwnd_);
        highlight_hwnd_ = nullptr;
    }
}

void TargetHighlighter::UpdatePosition() {
    if (!IsActive()) return;

    RECT rc{};
    if (!GetWindowRect(target_hwnd_, &rc)) {
        Detach();
        return;
    }

    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    SetWindowPos(
        highlight_hwnd_,
        HWND_TOPMOST,
        rc.left,
        rc.top,
        width,
        height,
        SWP_NOACTIVATE);

    UpdateRegion(highlight_hwnd_, width, height);
}

bool TargetHighlighter::IsSameTarget(DropTarget target)
{
    if (target.type == DropType::BatchTriggerListView)
    {
        ListViewHitResult hit;
        POINT pt;
        GetCursorPos(&pt);
        if (ExtraWindow::HitTestListView(target.hWnd, pt, hit))
        {
            auto row = hit.item;
            auto col = hit.subItem;
            return target.hWnd == target_hwnd_ && row_ == row && col_ == col;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return target.hWnd == target_hwnd_;
    }
}

LRESULT CALLBACK TargetHighlighter::WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    TargetHighlighter* self = nullptr;

    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<TargetHighlighter*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(self));
    }
    else {
        self = reinterpret_cast<TargetHighlighter*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    switch (msg) {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        HBRUSH brush = CreateSolidBrush(self->border_color_);
        FillRect(hdc, &rc, brush);
        DeleteObject(brush);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_NCHITTEST:
        return HTTRANSPARENT;

    case WM_ERASEBKGND:
        return 1;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool LabelMatcher::Match(const char* target) const
{
    FString simple_target = target;

    if (!m_exactMatch)
    {
        simple_target.MakeUpper();
        simple_target = STDHelpers::ChineseTraditional_ToSimple(simple_target);
    }

    for (const auto& pattern : m_patterns)
    {
        if (MatchPattern(simple_target, pattern))
            return true;
    }
    return false;
}

void LabelMatcher::Build(const char* source)
{
    FString simple_source = source;

    if (!m_exactMatch)
    {
        simple_source.MakeUpper();
        simple_source = STDHelpers::ChineseTraditional_ToSimple(simple_source);
    }

    simple_source.Trim();

    auto splits = FString::SplitString(simple_source, "|");

    for (auto& split : splits)
    {
        Pattern pattern;
        pattern.atoms = FString::SplitString(split, "*");
        m_patterns.emplace_back(std::move(pattern));
    }
}

bool LabelMatcher::MatchPattern(const FString& target, const Pattern& pattern) const
{
    const auto& atoms = pattern.atoms;

    if (atoms.size() == 1)
    {
        const FString& atom = atoms[0];

        if (m_exactMatch)
        {
            return strcmp(target.c_str(), atom.c_str()) == 0;
        }
        else
        {
            return _mbsstr(
                (const unsigned char*)target.c_str(),
                (const unsigned char*)atom.c_str()
            ) != nullptr;
        }
    }

    const char* compare = target.c_str();

    for (size_t i = 0; i < atoms.size(); ++i)
    {
        const FString& atom = atoms[i];

        if (atom.empty())
            continue;

        const char* pSub = (const char*)_mbsstr(
            (const unsigned char*)compare,
            (const unsigned char*)atom.c_str()
        );

        if (!pSub)
            return false;

        if (m_exactMatch)
        {
            if (i == 0 && pSub != compare)
                return false;

            if (i == atoms.size() - 1)
            {
                size_t remainLen = strlen(pSub);
                if (remainLen != atom.length())
                    return false;
            }
        }
        compare = pSub + atom.length();
    }
    return true;
}

#define VCB_TIMER_SELECT  1
#define VCB_TIMER_RESTORE  2
#define ITEM_HEIGHT  15

VirtualComboBoxEx::VirtualComboBoxEx() {}
VirtualComboBoxEx::~VirtualComboBoxEx() { Detach(); }

void VirtualComboBoxEx::Attach(HWND hwnd, bool* sortType, bool allowFreeText)
{
    hCombo = hwnd;

    COMBOBOXINFO cbi = { sizeof(cbi) };
    GetComboBoxInfo(hwnd, &cbi);

    hEdit = cbi.hwndItem;
    hList = cbi.hwndList;

    SetWindowLongPtr(hCombo, GWLP_USERDATA, (LONG_PTR)this);
    oldComboProc = (WNDPROC)SetWindowLongPtr(hCombo, GWLP_WNDPROC, (LONG_PTR)ComboProc);

    SetWindowLongPtr(hEdit, GWLP_USERDATA, (LONG_PTR)this);
    oldEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)EditProc);

    SetWindowLongPtr(hList, GWLP_USERDATA, (LONG_PTR)this);
    oldListProc = (WNDPROC)SetWindowLongPtr(hList, GWLP_WNDPROC, (LONG_PTR)ListProc);

    EnsureFilteredAll();
    SyncListCount();

    m_sortByLabelKey = sortType != nullptr;
    m_sortType = sortType;
    m_programmaticDropdown = false;
    m_allowFreeText = allowFreeText;
}

void VirtualComboBoxEx::SetAutoSearchRestriction(bool* restrict)
{
    m_allowFilter = restrict;
}

void VirtualComboBoxEx::SetWindowHeight(HWND hwnd, LPARAM lParam)
{
    LPMEASUREITEMSTRUCT mis = (LPMEASUREITEMSTRUCT)lParam;
    HDC hdc = GetDC(hwnd);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hwnd, hdc);
    mis->itemHeight = ITEM_HEIGHT * dpi / 96.0f;
}

void VirtualComboBoxEx::Detach()
{
    if (hCombo && oldComboProc)
        SetWindowLongPtr(hCombo, GWLP_WNDPROC, (LONG_PTR)oldComboProc);

    if (hEdit && oldEditProc)
        SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)oldEditProc);

    if (hList && oldListProc)
        SetWindowLongPtr(hList, GWLP_WNDPROC, (LONG_PTR)oldListProc);

    hCombo = hEdit = hList = nullptr;
}

void VirtualComboBoxEx::CopyFrom(const VirtualComboBoxEx& other,
    const std::vector<FString>* addToFront,
    const std::vector<FString>* addToEnd)
{
    if (this == &other)
        return;

    items.clear();

    size_t total =
        (addToFront ? addToFront->size() : 0) +
        other.items.size() +
        (addToEnd ? addToEnd->size() : 0);

    items.reserve(total);

    if (addToFront)
    {
        for (auto& s : *addToFront)
        {
            VCBItemEntry e;
            e.text = s; 
            if (m_sortByLabelKey)
                e.key = BuildKey(e.text); 

            items.emplace_back(std::move(e));
        }
    }

    items.insert(items.end(), other.items.begin(), other.items.end());

    if (addToEnd)
    {
        for (auto& s : *addToEnd)
        {
            VCBItemEntry e;
            e.text = s;
            if (m_sortByLabelKey)
                e.key = BuildKey(e.text);

            items.emplace_back(std::move(e));
        }
    }

    EnsureFilteredAll();
    m_cachedMaxWidth = other.m_cachedMaxWidth;
    SyncListCount();
}

void VirtualComboBoxEx::AddString(const char* str)
{
    VCBItemEntry e;
    e.text = str;
    if (m_sortByLabelKey)
        e.key = BuildKey(e.text);
    items.push_back(std::move(e));
    filtered.push_back((int)items.size() - 1);

    if (m_dropWidthMode == VirtualComboBoxEx::DropWidthMode::DropWidth_AutoMax)
    {
        int thisWidth = CalcItemWidth(items.size() - 1);
        if (m_cachedMaxWidth < thisWidth)
        {
            m_cachedMaxWidth = thisWidth;
            UpdateDropWidth();
        }
    }

    m_nextDropSort = true;
}

void VirtualComboBoxEx::AddStrings(const std::vector<FString>& ret, const char* oriText)
{
    if (ret.empty())
        return;

    int index = -1;

    items.reserve(items.size() + ret.size());
    filtered.reserve(filtered.size() + ret.size());

    int base = (int)items.size();

    if (oriText)
    {
        for (size_t i = 0; i < ret.size(); ++i)
        {
            if (ret[i] == oriText)
                index = i;

            VCBItemEntry e;
            e.text = ret[i];
            if (m_sortByLabelKey)
                e.key = BuildKey(e.text);

            items.push_back(std::move(e));
            filtered.push_back(base + (int)i);
        }
    }
    else
    {
        for (size_t i = 0; i < ret.size(); ++i)
        {
            VCBItemEntry e;
            e.text = ret[i];
            if (m_sortByLabelKey)
                e.key = BuildKey(e.text);

            items.push_back(std::move(e));
            filtered.push_back(base + (int)i);
        }
    }

    m_cachedMaxWidth = 0;
    SyncListCount();

    if(oriText)
        SendMessage(hCombo, CB_SETCURSEL, index, NULL);
}

int VirtualComboBoxEx::InsertString(int index, const char* str)
{
    if (!str)
        return -1;

    if (index < 0 || index >(int)items.size())
        index = (int)items.size();

    VCBItemEntry e;
    e.text = str;
    if (m_sortByLabelKey)
        e.key = BuildKey(e.text);

    items.insert(items.begin() + index, std::move(e));

    if (m_dropWidthMode == VirtualComboBoxEx::DropWidthMode::DropWidth_AutoMax)
    {
        int thisWidth = CalcItemWidth(items.size() - 1);
        if (m_cachedMaxWidth < thisWidth)
        {
            m_cachedMaxWidth = thisWidth;
            UpdateDropWidth();
        }
    }

    m_nextDropSort = true;

    return index;
}

int VirtualComboBoxEx::ReplaceString(int index, const char* str)
{
    if (!str)
        return -1;

    if (index < 0 || index >= (int)items.size())
        return -1;

    VCBItemEntry e;
    e.text = str;
    if (m_sortByLabelKey)
        e.key = BuildKey(e.text);

    items[index] = e;

    if (curSel == index)
    {
        SetWindowTextA(hEdit, items[index].text);
    }

    if (m_dropWidthMode == VirtualComboBoxEx::DropWidthMode::DropWidth_AutoMax)
    {
        int thisWidth = CalcItemWidth(items.size() - 1);
        if (m_cachedMaxWidth < thisWidth)
        {
            m_cachedMaxWidth = thisWidth;
            UpdateDropWidth();
        }
    }

    m_nextDropSort = true;

    return index;
}

void VirtualComboBoxEx::Clear()
{
    items.clear();
    filtered.clear();
    curSel = -1;
    m_cachedMaxWidth = 0;
    SyncListCount();
}

int VirtualComboBoxEx::GetCurSel() const
{
    return curSel;
}

void VirtualComboBoxEx::SetCurSel(int idx)
{
    SendMessage(hCombo, CB_SETCURSEL, idx, NULL);
}

void VirtualComboBoxEx::EnsureFilteredAll()
{
    filtered.resize(items.size());
    for (int i = 0; i < (int)items.size(); ++i)
        filtered[i] = i;
}

void VirtualComboBoxEx::SyncListCount()
{
    int topIndex = (int)SendMessage(hList, LB_GETTOPINDEX, 0, 0);

    SendMessage(hList, WM_SETREDRAW, FALSE, 0);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);

    SendMessage(hList, LB_INITSTORAGE, filtered.size(), filtered.size() * 32);

    for (size_t i = 0; i < filtered.size(); ++i)
    {
        SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)"");
    }

    SendMessage(hList, WM_SETREDRAW, TRUE, 0);

    InvalidateRect(hList, nullptr, TRUE);
    UpdateDropWidth();
}

void VirtualComboBoxEx::Filter(const char* text)
{
    if (m_allowFilter && !*m_allowFilter)
        return;

    m_filterActive = true;
    std::vector<int> newFiltered;

    if (!text || !*text)
    {
        m_filterActive = false;
        newFiltered.resize(items.size());
        for (int i = 0; i < (int)items.size(); ++i)
            newFiltered[i] = i;
    }
    else
    {
        LabelMatcher matcher(text);
        for (int i = 0; i < (int)items.size(); ++i)
        {
            if (matcher.Match(items[i].text))
                newFiltered.push_back(i);
        }
    }

    filtered.swap(newFiltered);

    SyncListCount();

    if (!filtered.empty())
    {
        m_programmaticDropdown = true;
        SendMessage(hCombo, CB_SHOWDROPDOWN, TRUE, 0);
        m_programmaticDropdown = false;
        HCURSOR hCursor = LoadCursor(NULL, IDC_ARROW);
        SetCursor(hCursor);
    }
}

int VirtualComboBoxEx::GetCount() const
{
    return (int)items.size();
}

int VirtualComboBoxEx::GetFilteredCount() const
{
    return (int)filtered.size();
}

const char* VirtualComboBoxEx::GetItemText(int index) const
{
    if (index < 0 || index >= (int)items.size())
        return nullptr;

    return items[index].text;
}

const char* VirtualComboBoxEx::GetEditText() const
{
    static char text[512];
    memset(text, 0, sizeof(text));
    GetWindowTextA(hEdit, text, sizeof(text));
    return text;
}

void VirtualComboBoxEx::SetEditText(const char* text) const
{
    SetWindowTextA(hEdit, text);
}

const char* VirtualComboBoxEx::GetFilteredText(int index) const
{
    if (index < 0 || index >= (int)filtered.size())
        return nullptr;

    return items[filtered[index]].text;
}

const char* VirtualComboBoxEx::GetSelectedText(bool allowEdit) const
{
    if (allowEdit)
    {
        return GetEditText();
    }

    if (curSel < 0 || curSel >= (int)items.size())
    {
        return "";
    }    

    return items[curSel].text;
}

int VirtualComboBoxEx::FindStringExact(const char* str) const
{
    if (!str) return -1;

    for (int i = 0; i < (int)items.size(); ++i)
    {
        if (strcmp(items[i].text, str) == 0)
            return i;
    }
    return -1;
}

int VirtualComboBoxEx::FindString(const char* str) const
{
    if (!str) return -1;

    LabelMatcher matcher(str);
    for (int i = 0; i < (int)items.size(); ++i)
    {
        if (matcher.Match(items[i].text))
            return i;
    }
    return -1;
}

int VirtualComboBoxEx::DeleteString(int index)
{
    if (index < 0 || index >= (int)items.size())
        return -1;

    items.erase(items.begin() + index);
    m_cachedMaxWidth = 0;

    EnsureFilteredAll();
    SyncListCount();

    return (int)items.size();
}

int VirtualComboBoxEx::CalcMaxItemWidth()
{
    HDC hdc = GetDC(hCombo);

    HFONT hFont = (HFONT)SendMessage(hCombo, WM_GETFONT, 0, 0);
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

    SIZE sz = {};
    int maxW = 0;

    for (const auto& s : items)
    {
        GetTextExtentPoint32A(hdc, s.text, (int)s.text.length(), &sz);
        if (sz.cx > maxW)
            maxW = sz.cx;
    }

    SelectObject(hdc, oldFont);
    ReleaseDC(hCombo, hdc);

    maxW += 12;
    maxW += GetSystemMetrics(SM_CXVSCROLL);

    maxW = std::min(maxW, ExtConfigs::AdjustDropdownWidth_Max);

    return maxW;
}

int VirtualComboBoxEx::CalcItemWidth(int index)
{
    if (index < 0 || index >= (int)items.size())
        return 0;

    HDC hdc = GetDC(hCombo);

    HFONT hFont = (HFONT)SendMessage(hCombo, WM_GETFONT, 0, 0);
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

    SIZE sz = {};
    GetTextExtentPoint32A(hdc, items[index].text, (int)items[index].text.length(), &sz);
    int maxW = sz.cx;

    SelectObject(hdc, oldFont);
    ReleaseDC(hCombo, hdc);

    maxW += 12;
    maxW += GetSystemMetrics(SM_CXVSCROLL);

    maxW = std::min(maxW, ExtConfigs::AdjustDropdownWidth_Max);

    return maxW;
}

void VirtualComboBoxEx::UpdateDropWidth()
{
    if (!hCombo) return;

    int width = 0;

    if (m_dropWidthMode == VirtualComboBoxEx::DropWidthMode::DropWidth_FollowCombo)
    {
        RECT rc;
        GetWindowRect(hCombo, &rc);
        width = rc.right - rc.left;
    }
    else
    {
        if (m_cachedMaxWidth == 0)
            m_cachedMaxWidth = CalcMaxItemWidth();

        width = m_cachedMaxWidth;
    }

    SendMessage(hCombo, CB_SETDROPPEDWIDTH, width, 0);
}

void VirtualComboBoxEx::SetDropWidthMode(DropWidthMode mode)
{
    m_dropWidthMode = mode;
    UpdateDropWidth();
}

void VirtualComboBoxEx::SortItems(int* pSelIndex)
{
    FString selectedText;
    bool hasSel = false;

    if (pSelIndex && *pSelIndex >= 0 && *pSelIndex < (int)items.size())
    {
        selectedText = items[*pSelIndex].text;
        hasSel = true;
    }

    if (m_sortType && *m_sortType)
    {
        std::sort(items.begin(), items.end(),
            [](const VCBItemEntry& a, const VCBItemEntry& b)
        {
            return CompareKey(a.key, b.key);
        });
    }
    else
    {
        std::sort(items.begin(), items.end(),
            [](const VCBItemEntry& a, const VCBItemEntry& b)
        {
            return a.text < b.text;
        });
    }

    EnsureFilteredAll();

    int newIndex = -1;

    if (hasSel)
    {
        for (int i = 0; i < (int)items.size(); ++i)
        {
            if (items[i].text == selectedText)
            {
                newIndex = i;
                break;
            }
        }
    }

    if (pSelIndex)
        *pSelIndex = newIndex;

    SyncListCount();
}

LRESULT VirtualComboBoxEx::OnComboMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case CB_GETCURSEL:
    {
        return curSel;
    }
    case CB_GETCOUNT:
    {
        return (LRESULT)items.size();
    }
    case CB_GETLBTEXTLEN:
    {
        int idx = (int)wParam;

        if (idx < 0 || idx >= (int)items.size())
            return CB_ERR;

        const auto& s = items[idx].text;

        return (LRESULT)s.length();
    }
    case CB_GETLBTEXT:
    {
        int idx = (int)wParam;
        char* buf = (char*)lParam;

        if (!buf || idx < 0 || idx >= (int)items.size())
            return CB_ERR;

        const auto& s = items[idx];

        strcpy(buf, s.text);

        return (LRESULT)s.text.length();
    }
    case CB_FINDSTRING:
    {
        const char* str = (const char*)lParam;
        return FindString(str);
    }
    case CB_FINDSTRINGEXACT:
    {
        const char* str = (const char*)lParam;
        return FindStringExact(str);
    }
    case CB_RESETCONTENT:
    {
        Clear();
        return 0;
    }
    case CB_ADDSTRING:
    {
        const char* str = (const char*)lParam;

        AddString(str);

        return items.size() - 1;
    }
    case CB_INSERTSTRING:
    {
        int idx = (int)wParam;
        const char* str = (const char*)lParam;

        return InsertString(idx, str);
    }
    case CB_DELETESTRING:
    {
        int idx = (int)wParam;
        return DeleteString(idx);
    }
    }

    return 0;
}

LRESULT CALLBACK VirtualComboBoxEx::ComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* pThis = (VirtualComboBoxEx*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg)
    {
    case CB_SETCURSEL:
    {
        int idx = (int)wParam;

        if (idx >= 0 && idx < (int)pThis->items.size())
        {
            pThis->curSel = idx;
            SetWindowTextA(pThis->hEdit, pThis->items[idx].text);
        }
        else
        {
            pThis->curSel = -1;
            SetWindowTextA(pThis->hEdit, "");
        }

        LRESULT ret = CallWindowProc(
            pThis->oldComboProc, hwnd, msg, wParam, lParam);

        return ret;
    }
    case CB_GETCURSEL:
    case CB_GETCOUNT:
    case CB_GETLBTEXT:
    case CB_GETLBTEXTLEN:
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_RESETCONTENT:
    case CB_ADDSTRING:
    case CB_INSERTSTRING:
    {
        return pThis->OnComboMessage(hwnd, msg, wParam, lParam);
    }
    }
    switch (msg)
    {
    case WM_COMMAND:
    {
        auto CODE = HIWORD(wParam);

        if (CODE == LBN_SELCHANGE)
        {
            HWND hList = (HWND)lParam;

            if (hList == pThis->hList)
            {
                int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);

                if (sel >= 0 && sel < (int)pThis->filtered.size())
                {
                    pThis->m_programmaticDropdown = true;
                    SendMessage(pThis->hCombo, CB_SHOWDROPDOWN, FALSE, 0);
                    pThis->m_programmaticDropdown = false;

                    int real = pThis->filtered[sel];
                    pThis->SetCurSel(real);

                    HWND hParent = GetParent(pThis->hCombo);
                    SendMessage(hParent,
                        WM_COMMAND,
                        MAKEWPARAM(GetDlgCtrlID(pThis->hCombo), CBN_SELCHANGE),
                        (LPARAM)pThis->hCombo);
                }
            }
            return 0;
        }
        break;
    }
    case WM_MOUSEWHEEL:
    {
        int curSel = pThis->curSel;
        if ((short)HIWORD(wParam) > 0)
        {
            curSel--;
        }
        else if ((short)HIWORD(wParam) < 0)
        {
            curSel++;
        }

        if (curSel >= 0 && curSel < pThis->items.size())
        {
            pThis->SetCurSel(curSel);

            HWND hParent = GetParent(pThis->hCombo);
            SendMessage(hParent,
                WM_COMMAND,
                MAKEWPARAM(GetDlgCtrlID(pThis->hCombo), CBN_SELCHANGE),
                (LPARAM)pThis->hCombo);
        }

        return 0;
    }
    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        auto* pThis = (VirtualComboBoxEx*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        if (!pThis) break;

        HDC hdc = dis->hDC;
        RECT rc = dis->rcItem;

        HFONT hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
        HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

        SetBkMode(hdc, TRANSPARENT);

        std::string text;

        if (dis->itemID == (UINT)-1)
        {
            if (pThis->curSel >= 0 && pThis->curSel < (int)pThis->items.size())
                text = pThis->items[pThis->curSel].text;
        }
        else if (dis->itemID < pThis->filtered.size())
        {
            text = pThis->items[pThis->filtered[dis->itemID]].text;
        }

        if (dis->itemState & ODS_SELECTED)
        {
            FillRect(hdc, &rc, (HBRUSH)(COLOR_HIGHLIGHT + 1));
            SetTextColor(hdc, ExtConfigs::EnableDarkMode ? 
                DarkTheme::MyGetSysColor(COLOR_HIGHLIGHTTEXT) :
                GetSysColor(COLOR_HIGHLIGHTTEXT));
        }
        else
        {
            FillRect(hdc, &rc, ExtConfigs::EnableDarkMode ?
                DarkTheme::MyGetSysColorBrush(COLOR_WINDOW) :
                (HBRUSH)(COLOR_WINDOW + 1));
            SetTextColor(hdc, ExtConfigs::EnableDarkMode ?
                DarkTheme::MyGetSysColor(COLOR_WINDOWTEXT) :
                GetSysColor(COLOR_WINDOWTEXT));
        }

        rc.left += 4;

        DrawTextA(hdc, text.c_str(), -1, &rc,
            DT_SINGLELINE | DT_VCENTER | DT_LEFT);

        SelectObject(hdc, oldFont);
        return TRUE;
    }
    case WM_MEASUREITEM:
    {
        LPMEASUREITEMSTRUCT mis = (LPMEASUREITEMSTRUCT)lParam;
        HDC hdc = GetDC(hwnd);
        int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(hwnd, hdc);
        mis->itemHeight = ITEM_HEIGHT * dpi / 96.0f;
        return TRUE;
    }
    }

    return CallWindowProc(pThis->oldComboProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK VirtualComboBoxEx::EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* pThis = (VirtualComboBoxEx*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_CHAR:
    {
        if (wParam == VK_RETURN || GetKeyState(VK_CONTROL) & 0x8000 || (GetKeyState(VK_MENU) & 0x8000))
            break;

        LRESULT ret = CallWindowProc(pThis->oldEditProc, hwnd, msg, wParam, lParam);

        char buf[512];
        GetWindowTextA(hwnd, buf, sizeof(buf));
        pThis->Filter(buf);

        return ret;
    }
    case WM_PASTE:
    {
        if ((GetKeyState(VK_MENU) & 0x8000))
            break;

        LRESULT ret = CallWindowProc(pThis->oldEditProc, hwnd, msg, wParam, lParam);

        char buf[512];
        GetWindowTextA(hwnd, buf, sizeof(buf));
        pThis->Filter(buf);

        return ret;
    }

    case WM_KEYDOWN:
    {
        if (wParam == VK_BACK || wParam == VK_DELETE)
        {
            LRESULT ret = CallWindowProc(pThis->oldEditProc, hwnd, msg, wParam, lParam);

            char buf[512];
            GetWindowTextA(hwnd, buf, sizeof(buf));
            pThis->Filter(buf);

            return ret;
        }
        else if (wParam == VK_DOWN || wParam == VK_UP)
        {
            int count = (int)pThis->filtered.size();
            if (count == 0)
                return 0;

            int sel = (int)SendMessage(pThis->hList, LB_GETCURSEL, 0, 0);

            if (sel == LB_ERR)
                sel = -1;

            if (wParam == VK_DOWN)
                sel = std::min(sel + 1, count - 1);
            else
                sel = std::max(sel - 1, 0);

            SendMessage(pThis->hList, LB_SETCURSEL, sel, 0);

            int real = pThis->filtered[sel];
            pThis->SetCurSel(real);

            HWND hParent = GetParent(pThis->hCombo);
            SendMessage(hParent,
                WM_COMMAND,
                MAKEWPARAM(GetDlgCtrlID(pThis->hCombo), CBN_SELCHANGE),
                (LPARAM)pThis->hCombo);

            SendMessage(pThis->hEdit, EM_SETSEL, 0, -1);

            return 0;
        }
        else if (wParam == VK_RETURN)
        {
            HWND hParent = GetParent(pThis->hCombo);
            int sel = (int)SendMessage(pThis->hList, LB_GETCURSEL, 0, 0);

            pThis->m_programmaticDropdown = true;
            SendMessage(pThis->hCombo, CB_SHOWDROPDOWN, FALSE, 0);
            pThis->m_programmaticDropdown = false;

            if (sel == -1 && !pThis->m_allowFreeText && pThis->items.size() > 0)
                sel = 0;

            if (sel != LB_ERR && sel < (int)pThis->filtered.size())
            {
                int real = pThis->filtered[sel];

                pThis->SetCurSel(real);

                SendMessage(pThis->hEdit, EM_SETSEL, 0, -1);

                SendMessage(hParent,
                    WM_COMMAND,
                    MAKEWPARAM(GetDlgCtrlID(pThis->hCombo), CBN_SELCHANGE),
                    (LPARAM)pThis->hCombo);
            }
            else
            {
                SendMessage(hParent,
                    WM_COMMAND,
                    MAKEWPARAM(GetDlgCtrlID(pThis->hCombo), CBN_EDITCHANGE),
                    (LPARAM)pThis->hCombo);
            }

            pThis->m_filterActive = false;

            return 0;
        }

        break;
    }
    }

    return CallWindowProc(pThis->oldEditProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK VirtualComboBoxEx::ListProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* pThis = (VirtualComboBoxEx*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (msg == LB_SETCURSEL && !FA2sp::WinInfo.IsWindowsVistaOrGreater() && pThis)
    {
        int newSel = (int)wParam;
        if (newSel == -1)
        {
            if (pThis->curSel >= 0)
            {
                return 0;
            }
        }
    }

    switch (msg)
    {
    case WM_WINDOWPOSCHANGED:
    {
        WINDOWPOS* wp = (WINDOWPOS*)lParam;

        if (wp->flags & SWP_SHOWWINDOW)
        {
            if (!pThis->m_programmaticDropdown &&
                !pThis->m_filterActive)
            {
                if (pThis->m_nextDropSort && pThis->m_sortByLabelKey)
                {
                    pThis->m_nextDropSort = false;
                    pThis->SortItems();
                }
                else
                {
                    pThis->EnsureFilteredAll();
                    pThis->SyncListCount();
                }

                char buf[512];
                GetWindowTextA(pThis->hEdit, buf, sizeof(buf));

                if (buf[0])
                {
                    int match = -1;
                    for (int i = 0; i < (int)pThis->items.size(); ++i)
                    {
                        if (strcmp(pThis->items[i].text, buf) == 0)
                        {
                            match = i;
                            break;
                        }
                    }

                    if (match >= 0)
                    {
                        pThis->pendingSelect = match;

                        SetTimer(hwnd, VCB_TIMER_SELECT, 0, NULL);
                    }
                }
            }
        }
        break;
    }
    case WM_SHOWWINDOW: 
    {
        if (!wParam)
        {
            SetTimer(hwnd, VCB_TIMER_RESTORE, 0, NULL);
            if (pThis->m_programmaticDropdown)
                pThis->m_programmaticPostDropdown = true;

        }
        break;
    }
    case WM_TIMER:
    {
        if (wParam == VCB_TIMER_SELECT)
        {
            KillTimer(hwnd, VCB_TIMER_SELECT);

            int match = pThis->pendingSelect;

            if (match >= 0 && match < (int)pThis->items.size())
            {
                SendMessage(hwnd, LB_SETCURSEL, match, 0);
                pThis->curSel = match;
            }
            return 0;
        }
        else if (wParam == VCB_TIMER_RESTORE)
        {
            KillTimer(hwnd, VCB_TIMER_RESTORE);

            pThis->EnsureFilteredAll();
            pThis->SyncListCount();

            pThis->m_filterActive = false;

            if (pThis->m_programmaticPostDropdown)
            {
                pThis->m_programmaticPostDropdown = false;
                break;
            }

            char buf[512];
            GetWindowTextA(pThis->hEdit, buf, sizeof(buf));

            int match = -1;

            if (buf[0])
            {
                for (int i = 0; i < (int)pThis->items.size(); ++i)
                {
                    const auto& s = pThis->items[i].text;

                    if (_strnicmp(s.c_str(), buf, strlen(buf)) == 0)
                    {
                        match = i;
                        break;
                    }
                }
            }

            HWND hParent = GetParent(pThis->hCombo);
            if (match == -1 && !pThis->m_allowFreeText && pThis->items.size() > 0)
                match = 0;

            if (match >= 0)
            {
                pThis->SetCurSel(match);
                SendMessage(hParent,
                    WM_COMMAND,
                    MAKEWPARAM(GetDlgCtrlID(pThis->hCombo), CBN_SELCHANGE),
                    (LPARAM)pThis->hCombo);
            }
            else
            {
                SendMessage(hParent,
                    WM_COMMAND,
                    MAKEWPARAM(GetDlgCtrlID(pThis->hCombo), CBN_EDITCHANGE),
                    (LPARAM)pThis->hCombo);
            }

            return 0;
        }
        break;
    }
    }

    return CallWindowProc(pThis->oldListProc, hwnd, msg, wParam, lParam);
}