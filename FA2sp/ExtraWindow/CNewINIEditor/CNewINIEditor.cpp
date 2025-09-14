#include "CNewINIEditor.h"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/MultimapHelper.h"
#include "../Common.h"

#include <CLoading.h>
#include <CFinalSunDlg.h>
#include <CObjectDatas.h>
#include <CMapData.h>
#include <CIsoView.h>
#include <richedit.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CMapData/Body.h"
#include <Miscs/Miscs.h>
#include "../CObjectSearch/CObjectSearch.h"
#include "../../Ext/CTileSetBrowserFrame/TabPages/TriggerSort.h"
#include "../CNewScript/CNewScript.h"
#include <numeric>
#include "../../Ext/CTriggerFrame/Body.h"
#define INI_BUFFER_SIZE 800000

HWND CNewINIEditor::m_hwnd;
CFinalSunDlg* CNewINIEditor::m_parent;
HWND CNewINIEditor::m_hwndImporter;
CINI& CNewINIEditor::map = CINI::CurrentDocument;
CINI& CNewINIEditor::fadata = CINI::FAData;
MultimapHelper& CNewINIEditor::rules = Variables::RulesMap;

HWND CNewINIEditor::hSearchText;
HWND CNewINIEditor::hSectionList;
HWND CNewINIEditor::hNewSectionName;
HWND CNewINIEditor::hINIEdit;
HWND CNewINIEditor::hNewButton;
HWND CNewINIEditor::hDeleteButton;
HWND CNewINIEditor::hImportButton;
HWND CNewINIEditor::hImportTextButton;
HWND CNewINIEditor::hImporterDesc;
HWND CNewINIEditor::hImporterOK;
HWND CNewINIEditor::hImporterText;
std::map<int, FString> CNewINIEditor::SectionLabels;
int CNewINIEditor::origWndWidth;
int CNewINIEditor::origWndHeight;
int CNewINIEditor::minWndWidth;
int CNewINIEditor::minWndHeight;
bool CNewINIEditor::minSizeSet;

WNDPROC CNewINIEditor::OriginalListBoxProc;
char INIBuffer[INI_BUFFER_SIZE]{ 0 };

void CNewINIEditor::Create(CFinalSunDlg* pWnd)
{
    HMODULE hModule = LoadLibrary(TEXT("Riched32.dll"));
    if (!hModule)
        MessageBox(NULL, Translations::TranslateOrDefault("FailedLoadRiched32DLL", "Could not Load Riched32.dll£¡"), Translations::TranslateOrDefault("Error", "Error"), MB_ICONERROR);

    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(308),
        pWnd->GetSafeHwnd(),
        CNewINIEditor::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CNewINIEditor.\n");
        m_parent = NULL;
        return;
    }
}

void CNewINIEditor::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("INIEditorTitle", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };

    Translate(1400, "INIEditorSearch");
    Translate(1401, "INIEditorNew");
    Translate(1402, "INIEditorDelete");
    Translate(1403, "INIEditorImport");
    Translate(1409, "INIEditorImportText");
    Translate(1408, "INIEditorDescription");


    hSearchText = GetDlgItem(hWnd, Controls::SearchText);
    hSectionList = GetDlgItem(hWnd, Controls::SectionList);
    hNewSectionName = GetDlgItem(hWnd, Controls::NewSectionName);
    hINIEdit = GetDlgItem(hWnd, Controls::INIEdit);
    hNewButton = GetDlgItem(hWnd, Controls::NewButton);
    hDeleteButton = GetDlgItem(hWnd, Controls::DeleteButton);
    hImportButton = GetDlgItem(hWnd, Controls::ImportButton);
    hImportTextButton = GetDlgItem(hWnd, Controls::ImportTextButton);

    ExtraWindow::SetEditControlFontSize(hINIEdit, 1.4f, true, "Consolas");
    ExtraWindow::SetEditControlFontSize(hSectionList, 1.2f, false, "Consolas");
    SendMessage(hINIEdit, EM_LIMITTEXT, (WPARAM)INI_BUFFER_SIZE, 0);
    SendMessage(hINIEdit, EM_SETEVENTMASK, 0, (LPARAM)(ENM_CHANGE));

    if (ExtConfigs::EnableDarkMode)
    {
        ::SendMessage(hINIEdit, EM_SETBKGNDCOLOR, (WPARAM)FALSE, (LPARAM)RGB(32, 32, 32));
        CHARFORMAT cf = { 0 };
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_COLOR;
        cf.crTextColor = RGB(220, 220, 220);
        ::SendMessage(hINIEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    }

    if (hSectionList)
        OriginalListBoxProc = (WNDPROC)SetWindowLongPtr(hSectionList, GWLP_WNDPROC, (LONG_PTR)ListBoxSubclassProc);

    Update(hWnd);
}

void CNewINIEditor::InitializeImporter(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("INIEditorImporterTitle", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };

    Translate(1410, "INIEditorImporterDesc");
    Translate(1412, "INIEditorImporterOK");

    hImporterDesc = GetDlgItem(hWnd, Controls::ImporterDesc);
    hImporterOK = GetDlgItem(hWnd, Controls::ImporterOK);
    hImporterText = GetDlgItem(hWnd, Controls::ImporterText);
    ExtraWindow::SetEditControlFontSize(hImporterText, 1.4f, true, "Consolas");
    SendMessage(hImporterText, EM_LIMITTEXT, (WPARAM)INI_BUFFER_SIZE, 0);
    SendMessage(hImporterText, EM_SETUNDOLIMIT, 0, 0);
}

void CNewINIEditor::Update(HWND& hWnd)
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    int currentIndex = SendMessage(hSectionList, LB_GETCURSEL, NULL, NULL);

    SectionLabels.clear();
    while (SendMessage(hSectionList, LB_DELETESTRING, 0, NULL) != CB_ERR);
    auto itr = map.Dict.begin();
    for (size_t i = 0, sz = map.Dict.size(); i < sz; ++i, ++itr)
    {
        auto& sectionName = itr->first;
        if (IsMapPack(sectionName)) continue;
        if (ExtConfigs::INIEditor_IgnoreTeams && IsTeam(sectionName)) continue;
        SendMessage(hSectionList, LB_ADDSTRING, 0, (LPARAM)(LPCSTR)sectionName.m_pchData);
    }
    char buffer[512]{ 0 };
    GetWindowText(hSearchText, buffer, 511);
    if (strcmp(buffer, "") != 0)
        OnEditchangeSearch();
    else
        OnSelchangeListbox(currentIndex);
}

void CNewINIEditor::CloseImporter(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CNewINIEditor::m_hwndImporter = NULL;

}
void CNewINIEditor::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CNewINIEditor::m_hwnd = NULL;
    CNewINIEditor::m_parent = NULL;

}

LRESULT CALLBACK CNewINIEditor::ListBoxSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_MOUSEWHEEL:

        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);
        RECT rc;
        GetClientRect(hWnd, &rc);

        if (pt.x >= rc.right)
        {
            return CallWindowProc(OriginalListBoxProc, hWnd, message, wParam, lParam);
        }
        else
        {
            int nCurSel = (int)SendMessage(hWnd, LB_GETCURSEL, 0, 0);
            int nCount = (int)SendMessage(hWnd, LB_GETCOUNT, 0, 0);

            if (nCurSel != LB_ERR && nCount > 0)
            {
                if ((short)HIWORD(wParam) > 0 && nCurSel > 0)
                {
                    SendMessage(hWnd, LB_SETCURSEL, nCurSel - 1, 0);
                }
                else if ((short)HIWORD(wParam) < 0 && nCurSel < nCount - 1)
                {
                    SendMessage(hWnd, LB_SETCURSEL, nCurSel + 1, 0);
                }
                OnSelchangeListbox();

            }
            else {
                SendMessage(hWnd, LB_SETCURSEL, 0, 0);
            }

            return TRUE;
        }
    }
    return CallWindowProc(OriginalListBoxProc, hWnd, message, wParam, lParam);
}

BOOL CALLBACK CNewINIEditor::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CNewINIEditor::Initialize(hWnd);
        RECT rect;
        GetClientRect(hWnd, &rect);
        origWndWidth = rect.right - rect.left;
        origWndHeight = rect.bottom - rect.top;
        minSizeSet = false;
        return TRUE;
    }
    case WM_GETMINMAXINFO: {
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
    case WM_SIZE: {
        int newWndWidth = LOWORD(lParam);
        int newWndHeight = HIWORD(lParam);

        RECT rect;
        GetWindowRect(hSectionList, &rect);

        POINT topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);

        int newWidth = rect.right - rect.left;
        int newHeight = rect.bottom - rect.top + newWndHeight - origWndHeight;
        MoveWindow(hSectionList, topLeft.x, topLeft.y, newWidth, newHeight, TRUE);

        GetWindowRect(hINIEdit, &rect);
        topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);
        newWidth = rect.right - rect.left + newWndWidth - origWndWidth;
        newHeight = rect.bottom - rect.top + newWndHeight - origWndHeight;
        MoveWindow(hINIEdit, topLeft.x, topLeft.y, newWidth, newHeight, TRUE);

        origWndWidth = newWndWidth;
        origWndHeight = newWndHeight;
        break;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::SectionList:
            ListBoxProc(hWnd, CODE, lParam);
            break;
        case Controls::ImportButton:
            if (CODE == BN_CLICKED)
            {
                
                CFinalSunDlg::Instance()->INIEditor.OnClickImportINI();
                UpdateAllGameObject();
                Update(hWnd);
            }
            break;
        case Controls::ImportTextButton:
            if (CODE == BN_CLICKED)
                OnClickImportText(hWnd);
            break;
        case Controls::NewButton:
            if (CODE == BN_CLICKED)
                OnClickNewSection();
            break;
        case Controls::DeleteButton:
            if (CODE == BN_CLICKED)
                OnClickDelSection(hWnd);
            break;
        case Controls::INIEdit:
            if (CODE == EN_CHANGE)
                OnEditchangeINIEdit();
            break;
        case Controls::SearchText:
            if (CODE == EN_CHANGE)
                OnEditchangeSearch();
            break;
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CNewINIEditor::Close(hWnd);
        return TRUE;
    }
    case 114514: // used for update
    {
        Update(hWnd);
        return TRUE;
    }
    }

    // Process this message through default handler
    return FALSE;
}

BOOL CALLBACK CNewINIEditor::DlgProcImporter(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CNewINIEditor::InitializeImporter(hWnd);
        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::ImporterOK:
            if (CODE == BN_CLICKED)
                OnClickImporterOK(hWnd);
            break;
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CNewINIEditor::CloseImporter(hWnd);
        return TRUE;
    }
    }

    // Process this message through default handler
    return FALSE;
}

void CNewINIEditor::ListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam)
{
    if (SendMessage(hSectionList, LB_GETCOUNT, NULL, NULL) <= 0)
        return;

    switch (nCode)
    {
    case LBN_SELCHANGE:
    case LBN_DBLCLK:
        OnSelchangeListbox();
        break;
    default:
        break;
    }

}

void CNewINIEditor::OnClickImportText(HWND& hWnd)
{
    m_hwndImporter = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(309),
        hWnd,
        CNewINIEditor::DlgProcImporter
    );

    if (m_hwndImporter)
        ShowWindow(m_hwndImporter, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CNewINIEditorImporter.\n");
    }
}

void CNewINIEditor::OnClickImporterOK(HWND& hWnd)
{
    GetWindowText(hImporterText, INIBuffer, INI_BUFFER_SIZE - 1);
    FString text(INIBuffer);
    auto lines = FString::SplitStringMultiSplit(text, "\n|\r");
    FString section;
    std::vector<FString> sections;
    
    
    for (auto& line : lines)
    {
        FString::TrimSemicolon(line);
        line.Trim();

        int nStart = line.Find('[');
        int nEnd = line.Find(']');
        if (nStart < nEnd && nStart == 0)
        {
            section = line.Mid(nStart + 1, nEnd - nStart - 1);
            sections.push_back(section);
            continue;
        }
        if (section == "")
            continue;

        auto pair = FString::SplitKeyValue(line);
        if (line != "" && pair.first != "" && pair.second != "")
        {
            map.WriteString(section, pair.first, pair.second);
        }
    }

    for (auto& sec : sections) {
        UpdateGameObject(sec);
    }

    SendMessage(m_hwnd, 114514, 0, 0);
    SendMessage(m_hwndImporter, WM_CLOSE, 0, 0);
}

void CNewINIEditor::OnClickNewSection()
{
    SendMessage(hSearchText, WM_SETTEXT, 0, (LPARAM)(LPCSTR)"");
    SectionLabels.clear();
    char section[512]{ 0 };
    GetWindowText(hNewSectionName, section, 511);
    if (strcmp(section, "") == 0) return;
    if (IsGameObject(section) || IsMapPack(section) || IsHouse(section)) return;
    if (ExtConfigs::INIEditor_IgnoreTeams && IsTeam(section)) return;

    int idx = FindLBTextCaseSensitive(hSectionList, section);
    if (map.SectionExists(section))
    {
        SendMessage(hSectionList, LB_SETCURSEL, idx, 0);
        OnSelchangeListbox();
        return;
    }
    else if (idx != LB_ERR)
    {
        map.AddSection(section);
        SendMessage(hSectionList, LB_SETCURSEL, idx, 0);
        OnSelchangeListbox();
        return;
    }
    else
    {
        SendMessage(hSectionList, LB_SETCURSEL, SendMessage(hSectionList, LB_ADDSTRING, 0, (LPARAM)section), 0);
        map.AddSection(section);
        OnSelchangeListbox();
    }
}

void CNewINIEditor::OnClickDelSection(HWND& hWnd)
{
    int idx = SendMessage(hSectionList, LB_GETCURSEL, NULL, NULL);
    char section[512]{ 0 };
    SendMessage(hSectionList, LB_GETTEXT, idx, (LPARAM)section);

    FString warn = "Are you sure you want to delete %section%? You should be really careful, you may not be able to use the map afterwards.";
    warn = Translations::TranslateOrDefault("INIEditorDelWarn", warn);
    warn.Replace("%section%", section);

    int result = MessageBox(hWnd,
        warn,
        Translations::TranslateOrDefault("INIEditorDelTitle", "Delete Section"), MB_YESNO);

    if (result == IDNO)
        return;

    map.DeleteSection(section);
    UpdateGameObject(section);

    SendMessage(hSectionList, LB_DELETESTRING, idx, NULL);

    if (idx >= SendMessage(hSectionList, LB_GETCOUNT, NULL, NULL))
        idx--;
    if (idx < 0)
        idx = 0;
    SendMessage(hSectionList, LB_SETCURSEL, idx, NULL);

    OnSelchangeListbox();
}

void CNewINIEditor::OnSelchangeListbox(int index)
{
    if (index >= 0)
    {
        SendMessage(hSectionList, LB_SETCURSEL, index, NULL);
    }
    if (SendMessage(hSectionList, LB_GETCURSEL, NULL, NULL) < 0 || SendMessage(hSectionList, LB_GETCOUNT, NULL, NULL) <= 0)
    {
        SendMessage(hINIEdit, WM_SETTEXT, 0, (LPARAM)(LPCSTR)"");
        return;
    }
    int idx = SendMessage(hSectionList, LB_GETCURSEL, 0, NULL);

    char section[512]{ 0 };
    SendMessage(hSectionList, LB_GETTEXT, idx, (LPARAM)section);
    FString text;
    if (auto pSection = map.GetSection(section))
    {
        for (auto& pair : pSection->GetEntities())
        {
            FString line;
            line.Format("%s=%s\r\n", pair.first, pair.second);
            text += line;
        }
        SendMessage(hINIEdit, WM_SETTEXT, 0, text);
    }
    else
        SendMessage(hINIEdit, WM_SETTEXT, 0, (LPARAM)(LPCSTR)"");

}

void CNewINIEditor::OnEditchangeINIEdit()
{
    if (SendMessage(hSectionList, LB_GETCURSEL, NULL, NULL) < 0 || SendMessage(hSectionList, LB_GETCOUNT, NULL, NULL) <= 0)
    {
        SendMessage(hINIEdit, WM_SETTEXT, 0, (LPARAM)(LPCSTR)"");
        return;
    }
    int idx = SendMessage(hSectionList, LB_GETCURSEL, 0, NULL);
    char section[512]{ 0 };
    SendMessage(hSectionList, LB_GETTEXT, idx, (LPARAM)section);
    GetWindowText(hINIEdit, INIBuffer, INI_BUFFER_SIZE - 1);
    FString text(INIBuffer);
    auto lines = FString::SplitStringMultiSplit(text, "\n|\r");

    map.DeleteSection(section);
    for (auto& line : lines)
    {
        const char* lb = strchr(line, '[');
        const char* rb = strchr(line, ']');
        const char* eq = strchr(line, '=');

        if (!lb || !rb || rb < lb || (eq && eq < lb)) {
            FString::TrimSemicolon(line);
            line.Trim();

            auto pair = FString::SplitKeyValue(line);
            if (line != "" && pair.first != "")// && pair.second != "") // allow empty value
                map.WriteString(section, pair.first, pair.second);
        }
    }

    UpdateGameObject(section);
}

void CNewINIEditor::OnEditchangeSearch()
{
    if ((SendMessage(hSectionList, LB_GETCOUNT, NULL, NULL) > ExtConfigs::SearchCombobox_MaxCount
        || SectionLabels.size() > ExtConfigs::SearchCombobox_MaxCount) && !ExtraWindow::bEnterSearch)
    {
        return;
    }
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (!SectionLabels.empty())
    {
        while (SendMessage(hSectionList, LB_DELETESTRING, 0, NULL) != LB_ERR);
        for (auto& pair : SectionLabels)
        {
            SendMessage(hSectionList, LB_INSERTSTRING, pair.first, (LPARAM)(LPCSTR)pair.second);
        }
        SectionLabels.clear();
    }

    GetWindowText(hSearchText, buffer, 511);

    std::vector<int> deletedLabels;
    for (int idx = SendMessage(hSectionList, LB_GETCOUNT, NULL, NULL) - 1; idx >= 0; idx--)
    {
        SendMessage(hSectionList, LB_GETTEXT, idx, (LPARAM)buffer2);
        bool del = false;
        FString tmp(buffer2);
        if (!(ExtraWindow::IsLabelMatch(buffer2, buffer) || strcmp(buffer, "") == 0))
        {
            deletedLabels.push_back(idx);
        }
        SectionLabels[idx] = tmp;
    }
    for (int idx : deletedLabels)
    {
        SendMessage(hSectionList, LB_DELETESTRING, idx, NULL);
    }
}

int CNewINIEditor::FindLBTextCaseSensitive(HWND hwndCtl, const char* searchString)
{
    int count = (int)SendMessage(hwndCtl, LB_GETCOUNT, 0, 0);
    for (int i = 0; i < count; i++) {
        char itemText[512]{ 0 };
        SendMessage(hwndCtl, LB_GETTEXT, i, (LPARAM)itemText);

        if (strcmp(itemText, searchString) == 0) {
            return i; 
        }
    }
    return LB_ERR; 
}

void CNewINIEditor::UpdateGameObject(const char* lpSectionName)
{
    if (!IsGameObject(lpSectionName) && !IsHouse(lpSectionName)) return;
    if (strcmp(lpSectionName, "Structures") == 0) {
        CMapDataExt::UpdateFieldStructureData_RedrawMinimap();
    }
    else if (strcmp(lpSectionName, "Terrain") == 0) {
        CMapData::Instance->UpdateFieldTerrainData(false);
    }
    else if (strcmp(lpSectionName, "Waypoints") == 0) {
         CMapData::Instance->UpdateFieldWaypointData(false);
    }
    else if (strcmp(lpSectionName, "Smudge") == 0) {
        CMapData::Instance->UpdateFieldSmudgeData(false);
    }
    else if (strcmp(lpSectionName, "Units") == 0) {
        CMapDataExt::UpdateFieldUnitData_RedrawMinimap();
    }
    else if (strcmp(lpSectionName, "CellTags") == 0) {
        CMapData::Instance->UpdateFieldCelltagData(false);
    }
    else if (strcmp(lpSectionName, "Aircraft") == 0) {
        CMapDataExt::UpdateFieldAircraftData_RedrawMinimap();
    }
    else if (strcmp(lpSectionName, "Infantry") == 0) {
        CMapDataExt::UpdateFieldInfantryData_RedrawMinimap();
    }
    else if (strcmp(lpSectionName, "Annotations") == 0) {
        CMapData::Instance->UpdateFieldInfantryData(false);
    }
    else if (strcmp(lpSectionName, "Countries") == 0) {
        CMapDataExt::UpdateMapSectionIndicies("Countries");
    }
    else if (IsHouse(lpSectionName)) {
        CMapData::Instance->UpdateFieldBasenodeData(false);
    }
    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.Minimap.m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

void CNewINIEditor::UpdateAllGameObject()
{
    CMapDataExt::UpdateFieldStructureData_RedrawMinimap();
    CMapDataExt::UpdateFieldUnitData_RedrawMinimap();
    CMapDataExt::UpdateFieldAircraftData_RedrawMinimap();
    CMapDataExt::UpdateFieldInfantryData_RedrawMinimap();
    CMapData::Instance->UpdateFieldTerrainData(false);
    CMapData::Instance->UpdateFieldWaypointData(false);
    CMapData::Instance->UpdateFieldSmudgeData(false);
    CMapData::Instance->UpdateFieldCelltagData(false);
    CMapData::Instance->UpdateFieldBasenodeData(false);
    CMapDataExt::UpdateAnnotation();
    CMapDataExt::UpdateMapSectionIndicies("Countries");

    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.Minimap.m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

bool CNewINIEditor::IsGameObject(const char* lpSectionName)
{
    FString str(lpSectionName);

    if (str == "Terrain" || str == "Waypoints" || str == "Smudge" ||
        str == "Structures" || str == "Units" || str == "CellTags" ||
        str == "Aircraft" || str == "Infantry" || str == "Annotations"
        || str == "Countries")
        return true;

    return false;
}

bool CNewINIEditor::IsMapPack(const char* lpSectionName)
{
    FString str(lpSectionName);

    if (str == "IsoMapPack5" || str == "OverlayPack" || str == "OverlayDataPack" ||
        str == "Preview" || str == "PreviewPack" || str == "Map")
        return true;

    return false;
}

bool CNewINIEditor::IsHouse(const char* lpSectionName)
{
    FString str(lpSectionName);

    if (auto pSection = map.GetSection("Houses")) {
        for (auto& pair : pSection->GetEntities()) {
            if (pair.second == lpSectionName)
                return true;
        }
    }
    return false;
}

bool CNewINIEditor::IsTeam(const char* lpSectionName)
{
    FString str(lpSectionName);

    if (auto pSection = map.GetSection("TeamTypes")) {
        for (auto& pair : pSection->GetEntities()) {
            if (pair.second == lpSectionName)
                return true;
        }
    }
    if (auto pSection = map.GetSection("TaskForces")) {
        for (auto& pair : pSection->GetEntities()) {
            if (pair.second == lpSectionName)
                return true;
        }
    }
    if (auto pSection = map.GetSection("ScriptTypes")) {
        for (auto& pair : pSection->GetEntities()) {
            if (pair.second == lpSectionName)
                return true;
        }
    }
    return false;
}

bool CNewINIEditor::OnEnterKeyDown(HWND& hWnd)
{
    if (hWnd == hSearchText)
        OnEditchangeSearch();
    else
        return false;
    return true;
}

DEFINE_HOOK(40B826, OnClickImportINI, 5)
{
    return 0x40B832;
}