#include "CObjectSearch.h"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/MultimapHelper.h"
#include "../Common.h"

#include <CLoading.h>
#include <CFinalSunDlg.h>
#include <CObjectDatas.h>
#include <CMapData.h>
#include <CIsoView.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CMapData/Body.h"
#include "../CNewTeamTypes/CNewTeamTypes.h"
#include "../CNewTaskforce/CNewTaskforce.h"
#include "../CNewScript/CNewScript.h"
#include "../CNewTrigger/CNewTrigger.h"
#include "../CNewAITrigger/CNewAITrigger.h"
#include "../../Helpers/FString.h"
#include "../../Miscs/StringtableLoader.h"

HWND CObjectSearch::m_hwnd;
CTileSetBrowserFrame* CObjectSearch::m_parent;
std::vector<std::pair<std::string, std::regex>> CObjectSearch::Nodes;
std::map<int, FString> CObjectSearch::Datas;
int CObjectSearch::ListBoxIndex;
std::vector<TVITEMA> CObjectSearch::ListBox_TreeView;
std::vector<int> CObjectSearch::ListBox_Tile;
std::vector<std::pair<int, int>> CObjectSearch::ListBox_MapCoord;
std::vector<FString> CObjectSearch::ListBoxTexts;
bool CObjectSearch::bExactMatch;
bool CObjectSearch::bTreeView;
bool CObjectSearch::bMap;
bool CObjectSearch::bTileSet;
bool CObjectSearch::bListBoxButton;
bool CObjectSearch::bWaypoints;
bool CObjectSearch::bMapCoords;
bool CObjectSearch::bPropertyBushFilter;
bool CObjectSearch::ToggleWindowSize_once;
bool CObjectSearch::bTrigger;
bool CObjectSearch::bAttachedTrigger;
bool CObjectSearch::bEvent_Action;
bool CObjectSearch::bTriggerParam;
bool CObjectSearch::bTeamType;
bool CObjectSearch::bTaskForce;
bool CObjectSearch::bScript;
bool CObjectSearch::bTag;
bool CObjectSearch::bAITrigger;

void CObjectSearch::Create(CTileSetBrowserFrame* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(302),
        pWnd->DialogBar.GetSafeHwnd(),
        CObjectSearch::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CObjectSearch.\n");
        m_parent = NULL;
        return;
    }
    //ExtraWindow::CenterWindowPos(m_parent->GetSafeHwnd(), m_hwnd);

}

void CObjectSearch::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("GlobalSearchTitle", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](const char* pLabelName, int nIDDlgItem)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };

    ToggleListBoxRangeVisibility(hWnd, false);

    Translate("GlobalSearch.ExactMatch", Controls::ExactMatch);
    Translate("GlobalSearch.Search", Controls::Search);
    Translate("GlobalSearch.Range", Controls::Range);
    Translate("GlobalSearch.TreeView", Controls::TreeView);
    Translate("GlobalSearch.Map", Controls::Map);
    Translate("GlobalSearch.TileSet", Controls::TileSet);
    Translate("GlobalSearch.ListBoxButton", Controls::ListBoxButton);
    Translate("GlobalSearch.Waypoints", Controls::Waypoints);
    Translate("GlobalSearch.MapCoords", Controls::MapCoords);
    Translate("GlobalSearch.PropertyBushFilter", Controls::PropertyBushFilter);

    Translate("GlobalSearch.Trigger", Controls::Trigger);
    Translate("GlobalSearch.AttachedTrigger", Controls::AttachedTrigger);
    Translate("GlobalSearch.Event_Action", Controls::Event_Action);
    Translate("GlobalSearch.TriggerParam", Controls::TriggerParam);
    Translate("GlobalSearch.ListBox_Range", Controls::ListBox_Range);
    Translate("GlobalSearch.TeamType", Controls::TeamType);
    Translate("GlobalSearch.TaskForce", Controls::TaskForce);
    Translate("GlobalSearch.Script", Controls::Script);
    Translate("GlobalSearch.Tag", Controls::Tag);
    Translate("GlobalSearch.AITrigger", Controls::AITrigger);


    CObjectSearch::bExactMatch = false;
    SendMessage(GetDlgItem(hWnd, Controls::TreeView), BM_SETCHECK, 1, 0);
    HWND hFilter = GetDlgItem(hWnd, Controls::PropertyBushFilter);
    EnableWindow(hFilter, FALSE);
    CObjectSearch::bTreeView = true;
    CObjectSearch::bMap = false;
    CObjectSearch::bTileSet = false;
    CObjectSearch::bListBoxButton = false;
    CObjectSearch::bWaypoints = false;
    CObjectSearch::bMapCoords = false;
    CObjectSearch::bPropertyBushFilter = false;
    CObjectSearch::ToggleWindowSize_once = false;
    UpdateTypes(hWnd);
}

void CObjectSearch::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CObjectSearch::m_hwnd = NULL;
    CObjectSearch::m_parent = NULL;
    CObjectSearch::Nodes.clear();
    CObjectSearch::Datas.clear();
    CObjectSearch::ListBoxIndex = 0;
    CObjectSearch::ListBox_TreeView.clear();
    CObjectSearch::ListBox_Tile.clear();
    CObjectSearch::ListBoxTexts.clear();
}

BOOL CALLBACK CObjectSearch::DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    HWND hListBox = GetDlgItem(hwnd, Controls::ListBox);
    HWND hFilter = GetDlgItem(hwnd, Controls::PropertyBushFilter);
    FString buffer;

    auto Translate = [&hwnd, &buffer](const char* pLabelName, int nIDDlgItem)
        {
            HWND hTarget = GetDlgItem(hwnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };

    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CObjectSearch::Initialize(hwnd);
        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::Search:
            if (CODE == BN_CLICKED)
                CObjectSearch::OnSearchButtonUp(hwnd); 
            break;
        case Controls::ListBox:
            CObjectSearch::ListBoxProc(hwnd, CODE, lParam);
            break;
        case Controls::ExactMatch: 
            CObjectSearch::bExactMatch = SendMessage(GetDlgItem(hwnd, Controls::ExactMatch), BM_GETCHECK, 0, 0);
            break;
        case Controls::PropertyBushFilter:
            CObjectSearch::bPropertyBushFilter = SendMessage(GetDlgItem(hwnd, Controls::PropertyBushFilter), BM_GETCHECK, 0, 0);
            break;
        case Controls::TreeView:
            OnRangeChanged(hwnd);
            Translate("GlobalSearch.Search", Controls::Search);
            CObjectSearch::bTreeView = SendMessage(GetDlgItem(hwnd, Controls::TreeView), BM_GETCHECK, 0, 0);
            ToggleListBoxRangeVisibility(hwnd, false);
            ToggleWindowSize(hwnd);
            break;
        case Controls::Map:
            OnRangeChanged(hwnd);
            Translate("GlobalSearch.Search", Controls::Search);
            CObjectSearch::bMap = SendMessage(GetDlgItem(hwnd, Controls::Map), BM_GETCHECK, 0, 0);
            ToggleListBoxRangeVisibility(hwnd, false);
            ToggleWindowSize(hwnd);
            EnableWindow(hFilter, TRUE);
            break;
        case Controls::TileSet:
            OnRangeChanged(hwnd);
            Translate("GlobalSearch.Search", Controls::Search);
            CObjectSearch::bTileSet = SendMessage(GetDlgItem(hwnd, Controls::TileSet), BM_GETCHECK, 0, 0);
            ToggleListBoxRangeVisibility(hwnd, false);
            ToggleWindowSize(hwnd);
            break;
        case Controls::ListBoxButton:
            OnRangeChanged(hwnd);
            Translate("GlobalSearch.Search", Controls::Search);
            CObjectSearch::bListBoxButton = SendMessage(GetDlgItem(hwnd, Controls::ListBoxButton), BM_GETCHECK, 0, 0);
            ToggleListBoxRangeVisibility(hwnd, true);
            ToggleWindowSize(hwnd);
            break;
        case Controls::Waypoints:
            OnRangeChanged(hwnd);
            Translate("GlobalSearch.SearchWP", Controls::Search);
            CObjectSearch::bWaypoints = SendMessage(GetDlgItem(hwnd, Controls::Waypoints), BM_GETCHECK, 0, 0);
            ToggleListBoxRangeVisibility(hwnd, false);
            ToggleWindowSize(hwnd);
            if (CObjectSearch::bWaypoints)
                CObjectSearch::UpdateDetailsWaypoint(hwnd);
            break;
        case Controls::MapCoords:
            OnRangeChanged(hwnd);
            Translate("GlobalSearch.SearchWP", Controls::Search);
            CObjectSearch::bMapCoords = SendMessage(GetDlgItem(hwnd, Controls::MapCoords), BM_GETCHECK, 0, 0);
            ToggleListBoxRangeVisibility(hwnd, false);
            ToggleWindowSize(hwnd);
            if (CObjectSearch::bMapCoords)
            {
                buffer = "Format: X,Y";
                Translations::GetTranslationItem("GlobalSearch.SearchMCHint", buffer);
                SendMessage(
                    hListBox,
                    LB_SETITEMDATA,
                    SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)buffer),
                    0
                );
            }
            break;
        case Controls::Trigger:
            CObjectSearch::OnListBoxRangeChanged(hwnd);
            CObjectSearch::bTrigger = SendMessage(GetDlgItem(hwnd, Controls::Trigger), BM_GETCHECK, 0, 0);
            break;
        case Controls::AttachedTrigger:
            CObjectSearch::OnListBoxRangeChanged(hwnd);
            CObjectSearch::bAttachedTrigger = SendMessage(GetDlgItem(hwnd, Controls::AttachedTrigger), BM_GETCHECK, 0, 0);
            break;
        case Controls::Event_Action:
            CObjectSearch::OnListBoxRangeChanged(hwnd);
            CObjectSearch::bEvent_Action = SendMessage(GetDlgItem(hwnd, Controls::Event_Action), BM_GETCHECK, 0, 0);
            break;
        case Controls::TriggerParam:
            CObjectSearch::OnListBoxRangeChanged(hwnd);
            CObjectSearch::bTriggerParam = SendMessage(GetDlgItem(hwnd, Controls::TriggerParam), BM_GETCHECK, 0, 0);
            break;
        case Controls::TeamType:
            CObjectSearch::OnListBoxRangeChanged(hwnd);
            CObjectSearch::bTeamType = SendMessage(GetDlgItem(hwnd, Controls::TeamType), BM_GETCHECK, 0, 0);
            break;
        case Controls::TaskForce:
            CObjectSearch::OnListBoxRangeChanged(hwnd);
            CObjectSearch::bTaskForce = SendMessage(GetDlgItem(hwnd, Controls::TaskForce), BM_GETCHECK, 0, 0);
            break;
        case Controls::Script:
            CObjectSearch::OnListBoxRangeChanged(hwnd);
            CObjectSearch::bScript = SendMessage(GetDlgItem(hwnd, Controls::Script), BM_GETCHECK, 0, 0);
            break;
        case Controls::Tag:
            CObjectSearch::OnListBoxRangeChanged(hwnd);
            CObjectSearch::bTag = SendMessage(GetDlgItem(hwnd, Controls::Tag), BM_GETCHECK, 0, 0);
            break;
        case Controls::AITrigger:
            CObjectSearch::OnListBoxRangeChanged(hwnd);
            CObjectSearch::bAITrigger = SendMessage(GetDlgItem(hwnd, Controls::AITrigger), BM_GETCHECK, 0, 0);
            break;
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CObjectSearch::Close(hwnd);
        return TRUE;
    }
    case 114514:
        CObjectSearch::OnSearchButtonUp(hwnd);
        break;

    }

    // Process this message through default handler
    return FALSE;
}

void CObjectSearch::ListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam)
{
    HWND hListBox = reinterpret_cast<HWND>(lParam);
    auto cViewObjects = CFinalSunDlg::Instance->MyViewFrame.pViewObjects;
    if (SendMessage(hListBox, LB_GETCOUNT, NULL, NULL) <= 0)
        return;

    if (CObjectSearch::bTreeView)
    {
        switch (nCode)
        {
        case LBN_SELCHANGE:
        case LBN_DBLCLK:
            TreeView_SelectItem(cViewObjects->m_hWnd, CObjectSearch::ListBox_TreeView[SendMessage(hListBox, LB_GETCURSEL, NULL, NULL)].hItem);
            break;
        default:
            break;
        }
    }
    else if (CObjectSearch::bTileSet)
    {
        HWND hParent = m_parent->DialogBar.GetSafeHwnd();
        HWND hTileComboBox = GetDlgItem(hParent, 1366);

        switch (nCode)
        {
        case LBN_SELCHANGE:
        case LBN_DBLCLK:
            SendMessage(
                hTileComboBox,
                CB_SETCURSEL,
                CObjectSearch::ListBox_Tile[SendMessage(hListBox, LB_GETCURSEL, NULL, NULL)],
                NULL
            );
            SendMessage(hParent, WM_COMMAND, MAKEWPARAM(1366, CBN_SELCHANGE), (LPARAM)hTileComboBox);
            break;
        default:
            break;
        }
    }
    else if (CObjectSearch::bMap)
    {
        int index = SendMessage(hListBox, LB_GETCURSEL, NULL, NULL);
        if (index >= 0 && index < CObjectSearch::ListBox_MapCoord.size())
        {
            auto& location = CObjectSearch::ListBox_MapCoord[index];
            switch (nCode)
            {
            case LBN_SELCHANGE:
            case LBN_DBLCLK:
                MoveToMapCoord(location.first, location.second);
                break;
            default:
                break;
            }
        }
    }
    else if (CObjectSearch::bListBoxButton)
    {
        switch (nCode)
        {
        case LBN_SELCHANGE:
        case LBN_DBLCLK:
            if (CObjectSearch::bTrigger)
            {
                if (IsWindowVisible(CNewTrigger::GetHandle()))
                {
                    auto dlg = GetDlgItem(CNewTrigger::GetHandle(), CNewTrigger::Controls::SelectedTrigger);
                    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)CObjectSearch::ListBoxTexts[SendMessage(hListBox, LB_GETCURSEL, NULL, NULL)]);
                    if (idx == CB_ERR)
                        break;
                    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
                    CNewTrigger::OnSelchangeTrigger();
                }
            }
            else if (CObjectSearch::bAttachedTrigger)
            {
                if (IsWindowVisible(CNewTrigger::GetHandle()))
                {
                    auto dlg = GetDlgItem(CNewTrigger::GetHandle(), CNewTrigger::Controls::Attachedtrigger);
                    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)CObjectSearch::ListBoxTexts[SendMessage(hListBox, LB_GETCURSEL, NULL, NULL)]);
                    if (idx == CB_ERR)
                        break;
                    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
                    CNewTrigger::OnSelchangeAttachedTrigger();
                }
            }
            else if (CObjectSearch::bEvent_Action)
            {
                if (IsWindowVisible(CFinalSunDlg::Instance->TriggerFrame.m_hWnd) && CFinalSunDlg::Instance->TriggerFrame.TabCtrl.GetCurSel() == 2)
                {
                    auto idx = CFinalSunDlg::Instance->TriggerFrame.TriggerAction.CCBActionType.FindStringExact(0, CObjectSearch::ListBoxTexts[SendMessage(hListBox, LB_GETCURSEL, NULL, NULL)]);
                    if (idx == CB_ERR)
                        break;
                    CFinalSunDlg::Instance->TriggerFrame.TriggerAction.CCBActionType.SetCurSel(idx);
                    CFinalSunDlg::Instance->TriggerFrame.TriggerAction.OnCBActionTypeEditChanged();
                }
                else if (IsWindowVisible(CFinalSunDlg::Instance->TriggerFrame.m_hWnd) && CFinalSunDlg::Instance->TriggerFrame.TabCtrl.GetCurSel() == 1)
                {
                    auto idx = CFinalSunDlg::Instance->TriggerFrame.TriggerEvent.CCBEventType.FindStringExact(0, CObjectSearch::ListBoxTexts[SendMessage(hListBox, LB_GETCURSEL, NULL, NULL)]);
                    if (idx == CB_ERR)
                        break;
                    CFinalSunDlg::Instance->TriggerFrame.TriggerEvent.CCBEventType.SetCurSel(idx);
                    CFinalSunDlg::Instance->TriggerFrame.TriggerEvent.OnCBEventTypeEditChanged();
                }
            }
            else if (CObjectSearch::bTriggerParam)
            {
                if (IsWindowVisible(CFinalSunDlg::Instance->TriggerFrame.m_hWnd) && CFinalSunDlg::Instance->TriggerFrame.TabCtrl.GetCurSel() == 2)
                {
                    auto idx = CFinalSunDlg::Instance->TriggerFrame.TriggerAction.CCBParameters.FindStringExact(0, CObjectSearch::ListBoxTexts[SendMessage(hListBox, LB_GETCURSEL, NULL, NULL)]);
                    if (idx == CB_ERR)
                        break;
                    CFinalSunDlg::Instance->TriggerFrame.TriggerAction.CCBParameters.SetCurSel(idx);
                    CFinalSunDlg::Instance->TriggerFrame.TriggerAction.OnCBParamValueEditChanged();
                }
                else if (IsWindowVisible(CFinalSunDlg::Instance->TriggerFrame.m_hWnd) && CFinalSunDlg::Instance->TriggerFrame.TabCtrl.GetCurSel() == 1)
                {
                    auto idx = CFinalSunDlg::Instance->TriggerFrame.TriggerEvent.CCBParameters.FindStringExact(0, CObjectSearch::ListBoxTexts[SendMessage(hListBox, LB_GETCURSEL, NULL, NULL)]);
                    if (idx == CB_ERR)
                        break;
                    CFinalSunDlg::Instance->TriggerFrame.TriggerEvent.CCBParameters.SetCurSel(idx);
                    CFinalSunDlg::Instance->TriggerFrame.TriggerEvent.OnCBParamValueEditChanged();
                }
            }
            else if (CObjectSearch::bTeamType)
            {
                if (IsWindowVisible(CNewTeamTypes::GetHandle()))
                {
                    auto dlg = GetDlgItem(CNewTeamTypes::GetHandle(), CNewTeamTypes::Controls::SelectedTeam);
                    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)CObjectSearch::ListBoxTexts[SendMessage(hListBox, LB_GETCURSEL, NULL, NULL)]);
                    if (idx == CB_ERR)
                        break;
                    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
                    CNewTeamTypes::OnSelchangeTeamtypes();
                }
            }
            else if (CObjectSearch::bTaskForce)
            {
                if (IsWindowVisible(CNewTaskforce::GetHandle()))
                {
                    auto dlg = GetDlgItem(CNewTaskforce::GetHandle(), CNewTaskforce::Controls::SelectedTaskforce);
                    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)CObjectSearch::ListBoxTexts[SendMessage(hListBox, LB_GETCURSEL, NULL, NULL)]);
                    if (idx == CB_ERR)
                        break;
                    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
                    CNewTaskforce::OnSelchangeTaskforce();
                }
                else if (IsWindowVisible(CNewTeamTypes::GetHandle()))
                {
                    auto dlg = GetDlgItem(CNewTeamTypes::GetHandle(), CNewTeamTypes::Controls::Taskforce);
                    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)CObjectSearch::ListBoxTexts[SendMessage(hListBox, LB_GETCURSEL, NULL, NULL)]);
                    if (idx == CB_ERR)
                        break;
                    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
                    CNewTeamTypes::OnSelchangeTaskForce();
                }
            }
            else if (CObjectSearch::bScript)
            {
                if (IsWindowVisible(CNewScript::GetHandle()))
                {
                    auto dlg = GetDlgItem(CNewScript::GetHandle(), CNewScript::Controls::SelectedScript);
                    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)CObjectSearch::ListBoxTexts[SendMessage(hListBox, LB_GETCURSEL, NULL, NULL)]);
                    if (idx == CB_ERR)
                        break;
                    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
                    CNewScript::OnSelchangeScript();
                }
                else if (IsWindowVisible(CNewTeamTypes::GetHandle()))
                {
                    auto dlg = GetDlgItem(CNewTeamTypes::GetHandle(), CNewTeamTypes::Controls::Script);
                    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)CObjectSearch::ListBoxTexts[SendMessage(hListBox, LB_GETCURSEL, NULL, NULL)]);
                    if (idx == CB_ERR)
                        break;
                    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
                    CNewTeamTypes::OnSelchangeScript();
                }
            }
            else if (CObjectSearch::bAITrigger)
            {
                if (IsWindowVisible(CNewAITrigger::GetHandle()))
                {
                    auto dlg = GetDlgItem(CNewAITrigger::GetHandle(), CNewAITrigger::Controls::SelectedAITrigger);
                    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)CObjectSearch::ListBoxTexts[SendMessage(hListBox, LB_GETCURSEL, NULL, NULL)]);
                    if (idx == CB_ERR)
                        return;
                    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
                    CNewAITrigger::OnSelchangeAITrigger();
                }

            }
            else if (CObjectSearch::bTag)
            {
                bool refresh = false;
                bool refresh2 = false;
                auto propertyDlg = FindWindow(NULL, Translations::TranslateOrDefault("StructCap", "StructCap"));
                if (!propertyDlg)
                    propertyDlg = FindWindow(NULL, Translations::TranslateOrDefault("UnitCap", "UnitCap"));
                if (!propertyDlg)
                    propertyDlg = FindWindow(NULL, Translations::TranslateOrDefault("InfCap", "InfCap"));
                if (!propertyDlg)
                    propertyDlg = FindWindow(NULL, Translations::TranslateOrDefault("AirCap", "AirCap"));
                if (!propertyDlg)
                {
                    propertyDlg = CFinalSunDlg::Instance->Tags;
                    if (propertyDlg)
                        refresh2 = true;
                }
                if (!IsWindowVisible(propertyDlg))
                {
                    propertyDlg = CNewTeamTypes::GetHandle();
                    if (propertyDlg)
                        refresh = true;
                }
                if (!IsWindowVisible(propertyDlg))
                    return;

                auto hTag = GetDlgItem(propertyDlg, 1083);
                CComboBox& cTrigger = *(CComboBox*)CWnd::FromHandle(hTag);
                auto idx = cTrigger.FindStringExact(0, CObjectSearch::ListBoxTexts[SendMessage(hListBox, LB_GETCURSEL, NULL, NULL)]);
                if (idx == CB_ERR)
                    break;
                cTrigger.SetCurSel(idx);
                if (refresh) CNewTeamTypes::OnSelchangeTag();
                if (refresh2) CFinalSunDlg::Instance->Tags.OnCBCurrentTagSelectedChanged();
            }

            break;
        default:
            break;
        }
    }
    else if (CObjectSearch::bWaypoints)
    {
        auto& wp = CObjectSearch::ListBox_MapCoord[SendMessage(hListBox, LB_GETCURSEL, NULL, NULL)];
        switch (nCode)
        {
        case LBN_SELCHANGE:
        case LBN_DBLCLK:
            MoveToMapCoord(wp.first, wp.second);
            break;
        default:
            break;
        }
    }
}

void CObjectSearch::OnSearchButtonUp(HWND hWnd)
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    CObjectSearch::ListBoxIndex = 0;
    CObjectSearch::ListBox_TreeView.clear();
    CObjectSearch::ListBox_Tile.clear();
    CObjectSearch::ListBox_MapCoord.clear();
    CObjectSearch::ListBoxTexts.clear();
    HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
    if (!CObjectSearch::bMapCoords && !CObjectSearch::bWaypoints)
        while (SendMessage(hListBox, LB_DELETESTRING, 0, NULL) != LB_ERR);

    char buffer[256];
    GetDlgItemText(m_hwnd, Input, buffer, 256);


    if (CObjectSearch::bTreeView)
    {
        auto cViewObjects = CFinalSunDlg::Instance->MyViewFrame.pViewObjects;
        CObjectSearch::FindLabel(hWnd, TVI_ROOT, buffer);

        if (SendMessage(hListBox, LB_GETCOUNT, NULL, NULL) == 1)
            SendMessage(hListBox, LB_SETCURSEL, 0, NULL);

        if (CObjectSearch::ListBox_TreeView.size() == 1)
            TreeView_SelectItem(cViewObjects->m_hWnd, CObjectSearch::ListBox_TreeView[0].hItem);
    }
    else if (CObjectSearch::bTileSet)
    {
        for (auto& tile : CObjectSearch::Datas)
        {
            if (CObjectSearch::IsLabelMatch(tile.second, buffer))
            {
                CObjectSearch::UpdateDetailsTile(hWnd, tile.first);
            }
        }
        if (SendMessage(hListBox, LB_GETCOUNT, NULL, NULL) == 1)
            SendMessage(hListBox, LB_SETCURSEL, 0, NULL);

        if (CObjectSearch::ListBox_Tile.size() == 1)
        {
            HWND hParent = m_parent->DialogBar.GetSafeHwnd();
            HWND hTileComboBox = GetDlgItem(hParent, 1366);
            SendMessage(
                hTileComboBox,
                CB_SETCURSEL,
                CObjectSearch::ListBox_Tile[0],
                NULL
            );
            SendMessage(hParent, WM_COMMAND, MAKEWPARAM(1366, CBN_SELCHANGE), (LPARAM)hTileComboBox);
        }
    }
    else if (CObjectSearch::bMap)
    {
        CObjectSearch::SearchObjects(hWnd, buffer);
    }
    else if (CObjectSearch::bListBoxButton)
    {
        if (CObjectSearch::bTrigger)
        {
            CObjectSearch::SearchTriggers(hWnd, buffer);

            if (SendMessage(hListBox, LB_GETCOUNT, NULL, NULL) == 1)
                SendMessage(hListBox, LB_SETCURSEL, 0, NULL);
            if (CObjectSearch::ListBoxTexts.size() == 1)
                if (IsWindowVisible(CNewTrigger::GetHandle()))
                {
                    auto dlg = GetDlgItem(CNewTrigger::GetHandle(), CNewTrigger::Controls::SelectedTrigger);
                    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)CObjectSearch::ListBoxTexts[0]);
                    if (idx == CB_ERR)
                        return;
                    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
                    CNewTrigger::OnSelchangeTrigger();
                }
        }
        else if (CObjectSearch::bAttachedTrigger)
        {
            CObjectSearch::SearchAttachedTriggers(hWnd, buffer);
            if (SendMessage(hListBox, LB_GETCOUNT, NULL, NULL) == 1)
                SendMessage(hListBox, LB_SETCURSEL, 0, NULL);
            if (CObjectSearch::ListBoxTexts.size() == 1)
                if (IsWindowVisible(CNewTrigger::GetHandle()))
                {
                    auto dlg = GetDlgItem(CNewTrigger::GetHandle(), CNewTrigger::Controls::Attachedtrigger);
                    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)CObjectSearch::ListBoxTexts[0]);
                    if (idx == CB_ERR)
                        return;
                    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
                    CNewTrigger::OnSelchangeAttachedTrigger();
                }
        }
        else if (CObjectSearch::bEvent_Action)
        {
            CObjectSearch::SearchEvent_Action(hWnd, buffer);
            if (SendMessage(hListBox, LB_GETCOUNT, NULL, NULL) == 1)
                SendMessage(hListBox, LB_SETCURSEL, 0, NULL);
            if (CObjectSearch::ListBoxTexts.size() == 1)
            {
                if (IsWindowVisible(CFinalSunDlg::Instance->TriggerFrame.m_hWnd) && CFinalSunDlg::Instance->TriggerFrame.TabCtrl.GetCurSel() == 2)
                {
                    auto idx = CFinalSunDlg::Instance->TriggerFrame.TriggerAction.CCBActionType.FindStringExact(0, CObjectSearch::ListBoxTexts[0]);
                    if (idx == CB_ERR)
                        return;
                    CFinalSunDlg::Instance->TriggerFrame.TriggerAction.CCBActionType.SetCurSel(idx);
                    CFinalSunDlg::Instance->TriggerFrame.TriggerAction.OnCBActionTypeEditChanged();
                }
                else if (IsWindowVisible(CFinalSunDlg::Instance->TriggerFrame.m_hWnd) && CFinalSunDlg::Instance->TriggerFrame.TabCtrl.GetCurSel() == 1)
                {
                    auto idx = CFinalSunDlg::Instance->TriggerFrame.TriggerEvent.CCBEventType.FindStringExact(0, CObjectSearch::ListBoxTexts[0]);
                    if (idx == CB_ERR)
                        return;
                    CFinalSunDlg::Instance->TriggerFrame.TriggerEvent.CCBEventType.SetCurSel(idx);
                    CFinalSunDlg::Instance->TriggerFrame.TriggerEvent.OnCBEventTypeEditChanged();
                }
            }

        }
        else if (CObjectSearch::bTriggerParam)
        {
            CObjectSearch::SearchTriggerParam(hWnd, buffer);
            if (SendMessage(hListBox, LB_GETCOUNT, NULL, NULL) == 1)
                SendMessage(hListBox, LB_SETCURSEL, 0, NULL);
            if (CObjectSearch::ListBoxTexts.size() == 1)
            {
                if (IsWindowVisible(CFinalSunDlg::Instance->TriggerFrame.m_hWnd) && CFinalSunDlg::Instance->TriggerFrame.TabCtrl.GetCurSel() == 2)
                {
                    auto idx = CFinalSunDlg::Instance->TriggerFrame.TriggerAction.CCBParameters.FindStringExact(0, CObjectSearch::ListBoxTexts[0]);
                    if (idx == CB_ERR)
                        return;
                    CFinalSunDlg::Instance->TriggerFrame.TriggerAction.CCBParameters.SetCurSel(idx);
                    CFinalSunDlg::Instance->TriggerFrame.TriggerAction.OnCBParamValueEditChanged();
                }
                else if (IsWindowVisible(CFinalSunDlg::Instance->TriggerFrame.m_hWnd) && CFinalSunDlg::Instance->TriggerFrame.TabCtrl.GetCurSel() == 1)
                {
                    auto idx = CFinalSunDlg::Instance->TriggerFrame.TriggerEvent.CCBParameters.FindStringExact(0, CObjectSearch::ListBoxTexts[0]);
                    if (idx == CB_ERR)
                        return;
                    CFinalSunDlg::Instance->TriggerFrame.TriggerEvent.CCBParameters.SetCurSel(idx);
                    CFinalSunDlg::Instance->TriggerFrame.TriggerEvent.OnCBParamValueEditChanged();
                }
            }
        }
        else if (CObjectSearch::bTeamType)
        {
            CObjectSearch::SearchTeamType(hWnd, buffer);
            if (SendMessage(hListBox, LB_GETCOUNT, NULL, NULL) == 1)
                SendMessage(hListBox, LB_SETCURSEL, 0, NULL);
            if (CObjectSearch::ListBoxTexts.size() == 1)
            {
                if (IsWindowVisible(CNewTeamTypes::GetHandle()))
                {
                    auto dlg = GetDlgItem(CNewTeamTypes::GetHandle(), CNewTeamTypes::Controls::SelectedTeam);
                    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)CObjectSearch::ListBoxTexts[0]);
                    if (idx == CB_ERR)
                        return;
                    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
                    CNewTeamTypes::OnSelchangeTeamtypes();
                }
            }
        }
        else if (CObjectSearch::bTaskForce)
        {
            CObjectSearch::SearchTaskForce(hWnd, buffer);
            if (SendMessage(hListBox, LB_GETCOUNT, NULL, NULL) == 1)
                SendMessage(hListBox, LB_SETCURSEL, 0, NULL);
            if (CObjectSearch::ListBoxTexts.size() == 1)
            {
                if (IsWindowVisible(CNewTaskforce::GetHandle()))
                {
                    auto dlg = GetDlgItem(CNewTaskforce::GetHandle(), CNewTaskforce::Controls::SelectedTaskforce);
                    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)CObjectSearch::ListBoxTexts[0]);
                    if (idx == CB_ERR)
                        return;
                    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
                    CNewTaskforce::OnSelchangeTaskforce();
                }
                else if (IsWindowVisible(CNewTeamTypes::GetHandle()))
                {
                    auto dlg = GetDlgItem(CNewTeamTypes::GetHandle(), CNewTeamTypes::Controls::Taskforce);
                    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)CObjectSearch::ListBoxTexts[0]);
                    if (idx == CB_ERR)
                        return;
                    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
                    CNewTeamTypes::OnSelchangeTaskForce();
                }
            }
        }
        else if (CObjectSearch::bScript)
        {
            CObjectSearch::SearchScript(hWnd, buffer);
            if (SendMessage(hListBox, LB_GETCOUNT, NULL, NULL) == 1)
                SendMessage(hListBox, LB_SETCURSEL, 0, NULL);
            if (CObjectSearch::ListBoxTexts.size() == 1)
            {
                if (IsWindowVisible(CNewScript::GetHandle()))
                {
                    auto dlg = GetDlgItem(CNewScript::GetHandle(), CNewScript::Controls::SelectedScript);
                    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)CObjectSearch::ListBoxTexts[0]);
                    if (idx == CB_ERR)
                        return;
                    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
                    CNewScript::OnSelchangeScript();
                }
                else if (IsWindowVisible(CNewTeamTypes::GetHandle()))
                {
                    auto dlg = GetDlgItem(CNewTeamTypes::GetHandle(), CNewTeamTypes::Controls::Script);
                    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)CObjectSearch::ListBoxTexts[0]);
                    if (idx == CB_ERR)
                        return;
                    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
                    CNewTeamTypes::OnSelchangeScript();
                }
            }
        }
        else if (CObjectSearch::bAITrigger)
        {
            CObjectSearch::SearchAITrigger(hWnd, buffer);
            if (SendMessage(hListBox, LB_GETCOUNT, NULL, NULL) == 1)
                SendMessage(hListBox, LB_SETCURSEL, 0, NULL);
            if (CObjectSearch::ListBoxTexts.size() == 1)
            {
                if (IsWindowVisible(CNewAITrigger::GetHandle()))
                {
                    auto dlg = GetDlgItem(CNewAITrigger::GetHandle(), CNewAITrigger::Controls::SelectedAITrigger);
                    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)CObjectSearch::ListBoxTexts[0]);
                    if (idx == CB_ERR)
                        return;
                    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
                    CNewAITrigger::OnSelchangeAITrigger();
                }
            }
        }
        else if (CObjectSearch::bTag)
        {
            CObjectSearch::SearchInfoTag(hWnd, buffer);
            if (SendMessage(hListBox, LB_GETCOUNT, NULL, NULL) == 1)
                SendMessage(hListBox, LB_SETCURSEL, 0, NULL);
            if (CObjectSearch::ListBoxTexts.size() == 1)
            {
                bool refresh = false;
                bool refresh2 = false;
                auto propertyDlg = FindWindow(NULL, Translations::TranslateOrDefault("StructCap", "StructCap"));
                if (!propertyDlg)
                    propertyDlg = FindWindow(NULL, Translations::TranslateOrDefault("UnitCap", "UnitCap"));
                if (!propertyDlg)
                    propertyDlg = FindWindow(NULL, Translations::TranslateOrDefault("InfCap", "InfCap"));
                if (!propertyDlg)
                    propertyDlg = FindWindow(NULL, Translations::TranslateOrDefault("AirCap", "AirCap"));
                if (!propertyDlg)
                {
                    propertyDlg = CFinalSunDlg::Instance->Tags;
                    if (propertyDlg)
                        refresh2 = true;
                }
                if (!IsWindowVisible(propertyDlg))
                {
                    propertyDlg = CNewTeamTypes::GetHandle();
                    if (propertyDlg)
                        refresh = true;
                }
                if (!IsWindowVisible(propertyDlg))
                    return;

                auto hTag = GetDlgItem(propertyDlg, 1083);
                CComboBox& cTrigger = *(CComboBox*)CWnd::FromHandle(hTag);
                auto idx = cTrigger.FindStringExact(0, CObjectSearch::ListBoxTexts[0]);
                if (idx == CB_ERR)
                    return;
                cTrigger.SetCurSel(idx);
                if (refresh) CNewTeamTypes::OnSelchangeTag();
                if (refresh2) CFinalSunDlg::Instance->Tags.OnCBCurrentTagSelectedChanged();
            }
        }
    }
    else if (CObjectSearch::bWaypoints)
    {
        std::string wp(buffer);
        int start = 0, end = wp.size();
        while (start < end && std::isspace(wp[start])) {
            start++;
        }
        while (start < end && wp[start] == '0') {
            start++;
        }
        while (end > start && std::isspace(wp[end - 1])) {
            end--;
        }
        if (start == end) {
            wp = "0";
        }
        wp = wp.substr(start, end - start);
        auto pWP = CINI::CurrentDocument->GetString("Waypoints", wp.c_str(), "-1");
        auto second = atoi(pWP);
        if (second >= 0)
        {
            int y = second % 1000;
            int x = second / 1000;
            MoveToMapCoord(x, y);
        }
        else
        {
            const FString invalid_coord = Translations::TranslateOrDefault(
                "NavigateCoordInvalidWaypoint", "Invalid waypoint!"
            );
            const FString invalid_title = Translations::TranslateOrDefault(
                "NavigateCoordInvalidTitle", "Error!"
            );
            ::MessageBox(CFinalSunDlg::Instance->m_hWnd, invalid_coord, invalid_title, MB_OK | MB_ICONWARNING);
        }
    }
    else if (CObjectSearch::bMapCoords)
    {
        auto location = FString::SplitStringTrimmed(buffer);
        if (location.size() == 2)
        {
            int y = atoi(location[0]);
            int x = atoi(location[1]);
            MoveToMapCoord(x, y);
        }
        else
        {
            const FString invalid_coord = Translations::TranslateOrDefault(
                "NavigateCoordInvalidCoord", "Invalid coordinate!"
            );
            const FString invalid_title = Translations::TranslateOrDefault(
                "NavigateCoordInvalidTitle", "Error!"
            );
            ::MessageBox(CFinalSunDlg::Instance->m_hWnd, invalid_coord, invalid_title, MB_OK | MB_ICONWARNING);
        }
    }

    int maxWidth = 0;
    HDC hdc = GetDC(hListBox);
    HFONT hFont = (HFONT)SendMessage(hListBox, WM_GETFONT, 0, 0);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    int count = (int)SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    for (int i = 0; i < count; ++i) {
        TCHAR buf[512];
        SendMessage(hListBox, LB_GETTEXT, i, (LPARAM)buf);
        SIZE sz;
        GetTextExtentPoint32(hdc, buf, lstrlen(buf), &sz);
        if (sz.cx > maxWidth)
            maxWidth = sz.cx;
    }

    SelectObject(hdc, hOldFont);
    ReleaseDC(hListBox, hdc);
    SendMessage(hListBox, LB_SETHORIZONTALEXTENT, maxWidth + 10, 0);
}
void CObjectSearch::SearchTriggers(HWND hWnd, const char* source)
{
    //if (IsWindowVisible(CFinalSunDlg::Instance->TriggerFrame.m_hWnd))
    //{
    //    auto& cTrigger = CFinalSunDlg::Instance->TriggerFrame.CCBCurrentTrigger;
    //    for (int i = 0; i < cTrigger.GetCount(); i++)
    //    {
    //        FString strItem;
    //        cTrigger.GetLBText(i, strItem);
    //        if (IsLabelMatch(strItem, source))
    //        {
    //
    //            HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
    //            SendMessage(
    //                hListBox,
    //                LB_SETITEMDATA,
    //                SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)strItem),
    //                0
    //            );
    //
    //            CObjectSearch::ListBoxTexts.push_back(strItem);
    //            CObjectSearch::ListBoxIndex++;
    //        }
    //    }
    //}
    if (IsWindowVisible(CNewTrigger::GetHandle()))
    {
        auto dlg = GetDlgItem(CNewTrigger::GetHandle(), CNewTrigger::Controls::SelectedTrigger);
        char buffer[512]{ 0 };

        for (int i = 0; i < SendMessage(dlg, CB_GETCOUNT, 0, 0); i++)
        {
            FString strItem;
            SendMessage(dlg, CB_GETLBTEXT, i, (LPARAM)buffer);
            strItem = buffer;
            if (IsLabelMatch(strItem, source))
            {

                HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
                SendMessage(
                    hListBox,
                    LB_SETITEMDATA,
                    SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)strItem),
                    0
                );

                CObjectSearch::ListBoxTexts.push_back(strItem);
                CObjectSearch::ListBoxIndex++;
            }
        }
    }

    return;
}

void CObjectSearch::SearchAttachedTriggers(HWND hWnd, const char* source)
{
    //if (IsWindowVisible(CFinalSunDlg::Instance->TriggerFrame.m_hWnd) && CFinalSunDlg::Instance->TriggerFrame.TabCtrl.GetCurSel() == 0)
    //{
    //    auto& cTrigger = CFinalSunDlg::Instance->TriggerFrame.TriggerOption.CCBAttachedTag;
    //    for (int i = 0; i < cTrigger.GetCount(); i++)
    //    {
    //        FString strItem;
    //        cTrigger.GetLBText(i, strItem);
    //        if (IsLabelMatch(strItem, source))
    //        {
    //
    //            HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
    //            SendMessage(
    //                hListBox,
    //                LB_SETITEMDATA,
    //                SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)strItem),
    //                0
    //            );
    //
    //            CObjectSearch::ListBoxTexts.push_back(strItem);
    //            CObjectSearch::ListBoxIndex++;
    //        }
    //    }
    //}
    if (IsWindowVisible(CNewTrigger::GetHandle()))
    {
        auto dlg = GetDlgItem(CNewTrigger::GetHandle(), CNewTrigger::Controls::Attachedtrigger);
        char buffer[512]{ 0 };

        for (int i = 0; i < SendMessage(dlg, CB_GETCOUNT, 0, 0); i++)
        {
            FString strItem;
            SendMessage(dlg, CB_GETLBTEXT, i, (LPARAM)buffer);
            strItem = buffer;
            if (IsLabelMatch(strItem, source))
            {

                HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
                SendMessage(
                    hListBox,
                    LB_SETITEMDATA,
                    SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)strItem),
                    0
                );

                CObjectSearch::ListBoxTexts.push_back(strItem);
                CObjectSearch::ListBoxIndex++;
            }
        }
    }
    return;
}

void CObjectSearch::SearchEvent_Action(HWND hWnd, const char* source)
{
    if (IsWindowVisible(CFinalSunDlg::Instance->TriggerFrame.m_hWnd) && CFinalSunDlg::Instance->TriggerFrame.TabCtrl.GetCurSel() == 2)
    {
        auto& cTrigger = CFinalSunDlg::Instance->TriggerFrame.TriggerAction.CCBActionType;
        for (int i = 0; i < cTrigger.GetCount(); i++)
        {
            ppmfc::CString strItem;
            cTrigger.GetLBText(i, strItem);
            if (IsLabelMatch(strItem, source))
            {

                HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
                SendMessage(
                    hListBox,
                    LB_SETITEMDATA,
                    SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)strItem),
                    0
                );

                CObjectSearch::ListBoxTexts.push_back(strItem);
                CObjectSearch::ListBoxIndex++;
            }
        }
    }
    else if (IsWindowVisible(CFinalSunDlg::Instance->TriggerFrame.m_hWnd) && CFinalSunDlg::Instance->TriggerFrame.TabCtrl.GetCurSel() == 1)
    {
        auto& cTrigger = CFinalSunDlg::Instance->TriggerFrame.TriggerEvent.CCBEventType;
        for (int i = 0; i < cTrigger.GetCount(); i++)
        {
            ppmfc::CString strItem;
            cTrigger.GetLBText(i, strItem);
            if (IsLabelMatch(strItem, source))
            {

                HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
                SendMessage(
                    hListBox,
                    LB_SETITEMDATA,
                    SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)strItem),
                    0
                );

                CObjectSearch::ListBoxTexts.push_back(strItem);
                CObjectSearch::ListBoxIndex++;
            }
        }
    }

    return;
}

void CObjectSearch::SearchTriggerParam(HWND hWnd, const char* source)
{
    if (IsWindowVisible(CFinalSunDlg::Instance->TriggerFrame.m_hWnd) && CFinalSunDlg::Instance->TriggerFrame.TabCtrl.GetCurSel() == 2)
    {
        auto& cTrigger = CFinalSunDlg::Instance->TriggerFrame.TriggerAction.CCBParameters;
        for (int i = 0; i < cTrigger.GetCount(); i++)
        {
            ppmfc::CString strItem;
            cTrigger.GetLBText(i, strItem);
            if (IsLabelMatch(strItem, source))
            {

                HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
                SendMessage(
                    hListBox,
                    LB_SETITEMDATA,
                    SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)strItem),
                    0
                );

                CObjectSearch::ListBoxTexts.push_back(strItem);
                CObjectSearch::ListBoxIndex++;
            }
        }
    }
    else if (IsWindowVisible(CFinalSunDlg::Instance->TriggerFrame.m_hWnd) && CFinalSunDlg::Instance->TriggerFrame.TabCtrl.GetCurSel() == 1)
    {
        auto& cTrigger = CFinalSunDlg::Instance->TriggerFrame.TriggerEvent.CCBParameters;
        for (int i = 0; i < cTrigger.GetCount(); i++)
        {
            ppmfc::CString strItem;
            cTrigger.GetLBText(i, strItem);
            if (IsLabelMatch(strItem, source))
            {

                HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
                SendMessage(
                    hListBox,
                    LB_SETITEMDATA,
                    SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)strItem),
                    0
                );

                CObjectSearch::ListBoxTexts.push_back(strItem);
                CObjectSearch::ListBoxIndex++;
            }
        }
    }

    return;
}

void CObjectSearch::SearchTeamType(HWND hWnd, const char* source)
{
    if (IsWindowVisible(CNewTeamTypes::GetHandle()))
    {
        auto dlg = GetDlgItem(CNewTeamTypes::GetHandle(), CNewTeamTypes::Controls::SelectedTeam);
        char buffer[512]{ 0 };

        for (int i = 0; i < SendMessage(dlg, CB_GETCOUNT, 0, 0); i++)
        {
            FString strItem;
            SendMessage(dlg, CB_GETLBTEXT, i, (LPARAM)buffer);
            strItem = buffer;
            if (IsLabelMatch(strItem, source))
            {

                HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
                SendMessage(
                    hListBox,
                    LB_SETITEMDATA,
                    SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)strItem),
                    0
                );

                CObjectSearch::ListBoxTexts.push_back(strItem);
                CObjectSearch::ListBoxIndex++;
            }
        }
    }

    return;
}

void CObjectSearch::SearchTaskForce(HWND hWnd, const char* source)
{
    if (IsWindowVisible(CNewTaskforce::GetHandle()))
    {
        auto dlg = GetDlgItem(CNewTaskforce::GetHandle(), CNewTaskforce::Controls::SelectedTaskforce);
        char buffer[512]{ 0 };

        for (int i = 0; i < SendMessage(dlg, CB_GETCOUNT, 0, 0); i++)
        {
            FString strItem;
            SendMessage(dlg, CB_GETLBTEXT, i, (LPARAM)buffer);
            strItem = buffer;
            if (IsLabelMatch(strItem, source))
            {

                HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
                SendMessage(
                    hListBox,
                    LB_SETITEMDATA,
                    SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)strItem),
                    0
                );

                CObjectSearch::ListBoxTexts.push_back(strItem);
                CObjectSearch::ListBoxIndex++;
            }
        }
    }
    else if (IsWindowVisible(CNewTeamTypes::GetHandle()))
    {
        auto dlg = GetDlgItem(CNewTeamTypes::GetHandle(), CNewTeamTypes::Controls::Taskforce);
        char buffer[512]{ 0 };

        for (int i = 0; i < SendMessage(dlg, CB_GETCOUNT, 0, 0); i++)
        {
            FString strItem;
            SendMessage(dlg, CB_GETLBTEXT, i, (LPARAM)buffer);
            strItem = buffer;
            if (IsLabelMatch(strItem, source))
            {

                HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
                SendMessage(
                    hListBox,
                    LB_SETITEMDATA,
                    SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)strItem),
                    0
                );

                CObjectSearch::ListBoxTexts.push_back(strItem);
                CObjectSearch::ListBoxIndex++;
            }
        }
    }
    return;
}

void CObjectSearch::SearchScript(HWND hWnd, const char* source)
{
    if (IsWindowVisible(CNewScript::GetHandle()))
    {
        auto dlg = GetDlgItem(CNewScript::GetHandle(), CNewScript::Controls::SelectedScript);
        char buffer[512]{ 0 };

        for (int i = 0; i < SendMessage(dlg, CB_GETCOUNT, 0, 0); i++)
        {
            FString strItem;
            SendMessage(dlg, CB_GETLBTEXT, i, (LPARAM)buffer);
            strItem = buffer;
            if (IsLabelMatch(strItem, source))
            {

                HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
                SendMessage(
                    hListBox,
                    LB_SETITEMDATA,
                    SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)strItem),
                    0
                );

                CObjectSearch::ListBoxTexts.push_back(strItem);
                CObjectSearch::ListBoxIndex++;
            }
        }
    }
    else if (IsWindowVisible(CNewTeamTypes::GetHandle()))
    {
        auto dlg = GetDlgItem(CNewTeamTypes::GetHandle(), CNewTeamTypes::Controls::Script);
        char buffer[512]{ 0 };

        for (int i = 0; i < SendMessage(dlg, CB_GETCOUNT, 0, 0); i++)
        {
            FString strItem;
            SendMessage(dlg, CB_GETLBTEXT, i, (LPARAM)buffer);
            strItem = buffer;
            if (IsLabelMatch(strItem, source))
            {

                HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
                SendMessage(
                    hListBox,
                    LB_SETITEMDATA,
                    SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)strItem),
                    0
                );

                CObjectSearch::ListBoxTexts.push_back(strItem);
                CObjectSearch::ListBoxIndex++;
            }
        }
    }

    return;
}

void CObjectSearch::SearchInfoTag(HWND hWnd, const char* source)
{
    auto propertyDlg = FindWindow(NULL, Translations::TranslateOrDefault("StructCap", "StructCap"));
    if (!propertyDlg)
        propertyDlg = FindWindow(NULL, Translations::TranslateOrDefault("UnitCap", "UnitCap"));
    if (!propertyDlg)
        propertyDlg = FindWindow(NULL, Translations::TranslateOrDefault("InfCap", "InfCap"));
    if (!propertyDlg)
        propertyDlg = FindWindow(NULL, Translations::TranslateOrDefault("AirCap", "AirCap"));
    if (!propertyDlg)
        propertyDlg = CFinalSunDlg::Instance->Tags;
    if (!IsWindowVisible(propertyDlg))
        propertyDlg = CNewTeamTypes::GetHandle();
    if (!IsWindowVisible(propertyDlg))
        return;

    char buffer[512]{ 0 };
    auto hTag = GetDlgItem(propertyDlg, 1083);
    CComboBox& cTrigger = *(CComboBox*)CWnd::FromHandle(hTag);
    for (int i = 0; i < cTrigger.GetCount(); i++)
    {
        FString strItem;
        
        cTrigger.GetLBText(i, buffer);
        strItem = buffer;
        if (IsLabelMatch(strItem, source))
        {
            HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
            SendMessage(
                hListBox,
                LB_SETITEMDATA,
                SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)strItem),
                0
            );

            CObjectSearch::ListBoxTexts.push_back(strItem);
            CObjectSearch::ListBoxIndex++;
        }
    }

    return;
}

void CObjectSearch::SearchAITrigger(HWND hWnd, const char* source)
{
    if (IsWindowVisible(CNewAITrigger::GetHandle()))
    {
        auto dlg = GetDlgItem(CNewAITrigger::GetHandle(), CNewAITrigger::Controls::SelectedAITrigger);
        char buffer[512]{ 0 };

        for (int i = 0; i < SendMessage(dlg, CB_GETCOUNT, 0, 0); i++)
        {
            FString strItem;
            SendMessage(dlg, CB_GETLBTEXT, i, (LPARAM)buffer);
            strItem = buffer;
            if (IsLabelMatch(strItem, source))
            {

                HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
                SendMessage(
                    hListBox,
                    LB_SETITEMDATA,
                    SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)strItem),
                    0
                );

                CObjectSearch::ListBoxTexts.push_back(strItem);
                CObjectSearch::ListBoxIndex++;
            }
        }
    }


    return;
}


void CObjectSearch::SearchObjects(HWND hWnd, const char* source)
{
    auto SearchSection = [&](int SearchObjectType)
        {
            FString section;
            int tagIndex = 0;
            switch (SearchObjectType)
            {
            case FindType::Aircraft:
                section = "Aircraft";
                tagIndex = 7;
                break;
            case FindType::Infantry:
                section = "Infantry";
                tagIndex = 8;
                break;
            case FindType::Structure:
                section = "Structures";
                tagIndex = 6;
                break;
            case FindType::Unit:
                section = "Units";
                tagIndex = 7;
                break;
            default:
                break;
            }

            std::pair<int, int> location;  //x, y
            if (auto pSection = CMapData::Instance->INI.GetSection(section))
            {
                int index = -1;
                for (auto& pair : pSection->GetEntities())
                {
                    index++;
                    auto atoms = FString::SplitString(pair.second);
                    if (atoms.size() <= tagIndex)
                        continue;
                    auto& pID = atoms[1];
                    auto pTag = atoms[tagIndex];
                    if (pTag != "None")
                    {
                        pTag.Format("%s %s", atoms[tagIndex], ExtraWindow::GetTagName(atoms[tagIndex]));
                    }
                    
                    bool met = false;
                    bool tagMet = false;

                    FString name;
                    
                    name = StringtableLoader::QueryUIName(pID);
                    if (name == "MISSING")
                        name = Variables::RulesMap.GetString(pID, "Name", pID);
                    if (name != pID) {
                        FString tmp = name;
                        name.Format("%s (%s)", tmp, pID);
                    }

                    if (IsLabelMatch(name, source)) {
                        met = true;
                    }
                    else if (pTag != "None" && IsLabelMatch(pTag, source)) {
                        tagMet = true;
                        met = true;
                    }
                    else {
                        continue;
                    }

                    if (CObjectSearch::bPropertyBushFilter)
                    {
                        met = false;
                        CAircraftData airData;
                        CInfantryData infData;
                        CBuildingData buiData;
                        CUnitData unitData;
                        switch (SearchObjectType) {
                        case FindType::Aircraft:
                            CMapData::Instance->GetAircraftData(index, airData);
                            if (CFinalSunDlgExt::CheckProperty_Aircraft(airData))
                                met = true;
                            break;
                        case FindType::Infantry:
                            CMapData::Instance->GetInfantryData(index, infData);
                            if (CFinalSunDlgExt::CheckProperty_Infantry(infData))
                                met = true;
                            break;
                        case FindType::Structure:
                            CMapDataExt::GetBuildingDataByIniID(index, buiData);
                            if (CFinalSunDlgExt::CheckProperty_Building(buiData))
                                met = true;
                            break;
                        case FindType::Unit:
                            CMapData::Instance->GetUnitData(index, unitData);
                            if (CFinalSunDlgExt::CheckProperty_Vehicle(unitData))
                                met = true;
                            break;
                        default: break;
                        }
                    }

                    if (met)
                    {
                        location.second = atoi(atoms[3]);
                        location.first = atoi(atoms[4]);
                        FString house = atoms[0];
                        auto& countries = CINI::Rules->GetSection("Countries")->GetEntities();
                        FString translated;
    
                        for (auto& pair : countries)
                        {
                            if (ExtConfigs::BetterHouseNameTranslation)
                                translated = StringtableLoader::QueryUIName(pair.second) + "(" + pair.second + ")";
                            else
                                translated = StringtableLoader::QueryUIName(pair.second);

                            house.Replace(pair.second, translated);
                        }
  

                        HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
                        FString tmp = name;
                        if (!tagMet)
                            name.Format("%s (%d, %d) (%s)", tmp, location.second, location.first, house);
                        else
                            name.Format("%s (%d, %d) (%s) (%s)", tmp, location.second, location.first, house, pTag);
                        SendMessage(
                            hListBox,
                            LB_SETITEMDATA,
                            SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)name),
                            0
                        );

                        CObjectSearch::ListBox_MapCoord.push_back(location);
                        CObjectSearch::ListBoxIndex++;
                    }

                }
            }
        };

    SearchSection(FindType::Aircraft);
    SearchSection(FindType::Structure);
    SearchSection(FindType::Infantry);
    SearchSection(FindType::Unit);
}

HTREEITEM CObjectSearch::FindLabel(HWND hWnd, HTREEITEM hItemParent, LPCSTR pszLabel)
{
    TVITEM tvi;
    char chLabel[0x200] = { 0 };
    auto cViewObjects = CFinalSunDlg::Instance->MyViewFrame.pViewObjects->m_hWnd;

    for (tvi.hItem = TreeView_GetChild(cViewObjects, hItemParent); tvi.hItem;
        tvi.hItem = TreeView_GetNextSibling(cViewObjects, tvi.hItem))
    {
        tvi.mask = TVIF_TEXT | TVIF_CHILDREN;
        tvi.pszText = chLabel;
        tvi.cchTextMax = _countof(chLabel);
        if (TreeView_GetItem(cViewObjects, &tvi))
        {
            if (tvi.cChildren)
            {
                CObjectSearch::FindLabel(hWnd, tvi.hItem, pszLabel);
            }
            else
            {
                if (CObjectSearch::IsLabelMatch(tvi.pszText, pszLabel))
                    CObjectSearch::UpdateDetailsTreeView(hWnd, tvi);
            }
        }
    }
    return NULL;
}

void CObjectSearch::UpdateDetailsTreeView(HWND hWnd, TVITEMA tvi)
{
    for (auto& listed : CObjectSearch::ListBoxTexts)
    {
        if (strcmp(listed, tvi.pszText) == 0)
        {
            return;
        }

    }

    HWND hDetails = GetDlgItem(hWnd, Controls::ListBox);

    FString text = tvi.pszText;

    SendMessage(
        hDetails,
        LB_SETITEMDATA,
        SendMessage(hDetails, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)text),
        0
    );

    CObjectSearch::ListBoxTexts.push_back(text);
    CObjectSearch::ListBox_TreeView.push_back(tvi);
    CObjectSearch::ListBoxIndex++;

    return;
}
void CObjectSearch::UpdateDetailsTile(HWND hWnd, int index)
{

    HWND hDetails = GetDlgItem(hWnd, Controls::ListBox);

    FString text = CObjectSearch::Datas[index];

    SendMessage(
        hDetails,
        LB_SETITEMDATA,
        SendMessage(hDetails, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)text),
        0
    );


    CObjectSearch::ListBox_Tile.push_back(index);
    CObjectSearch::ListBoxIndex++;

    return;
}

void CObjectSearch::UpdateDetailsWaypoint(HWND hWnd)
{
    CObjectSearch::ListBox_MapCoord.clear();
    if (auto pSection = CINI::CurrentDocument->GetSection("Waypoints"))
    {
        for (auto& pair : pSection->GetEntities())
        {
            auto second = atoi(pair.second);

            if (second >= 0)
            {
                std::pair<int, int> wp;
                wp.second = second % 1000;
                wp.first = second / 1000;
                
                HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
                FString text;
                text.Format("%03d (%d, %d)", atoi(pair.first), wp.second, wp.first);
                SendMessage(
                    hListBox,
                    LB_SETITEMDATA,
                    SendMessage(hListBox, LB_INSERTSTRING, CObjectSearch::ListBoxIndex, (LPARAM)(LPCSTR)text),
                    0
                );

                CObjectSearch::ListBox_MapCoord.push_back(wp);
                CObjectSearch::ListBoxIndex++;
            }
        }
    }
}


void CObjectSearch::UpdateTypes(HWND hWnd)
{
    HWND hParent = m_parent->DialogBar.GetSafeHwnd();
    HWND hTileComboBox = GetDlgItem(hParent, 1366);
    int nTileCount = SendMessage(hTileComboBox, CB_GETCOUNT, NULL, NULL);
    if (nTileCount <= 0)
        return;

    FString tile;
    for (int idx = 0; idx < nTileCount; ++idx)
    {
        int nTile = SendMessage(hTileComboBox, CB_GETITEMDATA, idx, NULL);
        tile.Format("TileSet%04d", nTile);
        tile = CINI::CurrentTheater->GetString(tile, "SetName", "NO NAME");
        Translations::GetTranslationItem(tile, tile);
        FString tmp = tile;
        tile.Format("(%04d) %s", nTile, tmp);
        CObjectSearch::Datas[idx] = tile;
    }
}

void CObjectSearch::OnRangeChanged(HWND hWnd)
{
    CObjectSearch::bMap = false;
    CObjectSearch::bTileSet = false;
    CObjectSearch::bListBoxButton = false;
    CObjectSearch::bTreeView = false;
    CObjectSearch::bWaypoints = false;
    CObjectSearch::bMapCoords = false;
    HWND hFilter = GetDlgItem(hWnd, Controls::PropertyBushFilter);
    EnableWindow(hFilter, FALSE);
    while (SendMessage(GetDlgItem(hWnd, Controls::ListBox), LB_DELETESTRING, 0, NULL) != LB_ERR);
    CObjectSearch::ListBoxIndex = 0;
    CObjectSearch::ListBox_TreeView.clear();
    CObjectSearch::ListBox_Tile.clear();
    CObjectSearch::ListBox_MapCoord.clear();
    CObjectSearch::ListBoxTexts.clear();
}
void CObjectSearch::OnListBoxRangeChanged(HWND hWnd)
{
    CObjectSearch::bTrigger = false;
    CObjectSearch::bAttachedTrigger = false;
    CObjectSearch::bEvent_Action = false;
    CObjectSearch::bTriggerParam = false;
    CObjectSearch::bTeamType = false;
    CObjectSearch::bTaskForce = false;
    CObjectSearch::bScript = false;
    CObjectSearch::bTag = false;
    CObjectSearch::bAITrigger = false;
    while (SendMessage(GetDlgItem(hWnd, Controls::ListBox), LB_DELETESTRING, 0, NULL) != LB_ERR);

}

void CObjectSearch::MoveToMapCoord(int x, int y)
{
    if (!CMapData::Instance->IsCoordInMap(x, y))
    {
        const FString invalid_coord = Translations::TranslateOrDefault(
            "NavigateCoordInvalidCoord", "Invalid coordinate!"
        );
        const FString invalid_title = Translations::TranslateOrDefault(
            "NavigateCoordInvalidTitle", "Error!"
        );
        ::MessageBox(CFinalSunDlg::Instance->m_hWnd, invalid_coord, invalid_title, MB_OK | MB_ICONWARNING);
        return;
    }
    CMapDataExt::CellDataExt_FindCell.X = x;
    CMapDataExt::CellDataExt_FindCell.Y = y;
    CMapDataExt::CellDataExt_FindCell.drawCell = true;

    CIsoViewExt::MoveToMapCoord(y, x);

    CMapDataExt::CellDataExt_FindCell.drawCell = false;
}
void CObjectSearch::ToggleListBoxRangeVisibility(HWND hWnd, bool show)
{
    HWND hListBox_Range = GetDlgItem(hWnd, Controls::ListBox_Range);
    HWND hTrigger = GetDlgItem(hWnd, Controls::Trigger);
    HWND hAttachedTrigger = GetDlgItem(hWnd, Controls::AttachedTrigger);
    HWND hEvent_Action = GetDlgItem(hWnd, Controls::Event_Action);
    HWND hTriggerParam = GetDlgItem(hWnd, Controls::TriggerParam);
    HWND hTeamType = GetDlgItem(hWnd, Controls::TeamType);
    HWND hTaskForvce = GetDlgItem(hWnd, Controls::TaskForce);
    HWND hScript = GetDlgItem(hWnd, Controls::Script);
    HWND hTag = GetDlgItem(hWnd, Controls::Tag);
    HWND hAITrigger = GetDlgItem(hWnd, Controls::AITrigger);
    HWND hText = GetDlgItem(hWnd, Controls::Input);
    HWND hButton = GetDlgItem(hWnd, Controls::Search);
    HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);
    if (show)
    {
        ShowWindow(hListBox_Range, SW_SHOW);
        ShowWindow(hTrigger, SW_SHOW);
        ShowWindow(hAttachedTrigger, SW_SHOW);
        ShowWindow(hEvent_Action, SW_SHOW);
        ShowWindow(hTriggerParam, SW_SHOW);
        ShowWindow(hTeamType, SW_SHOW);
        ShowWindow(hTaskForvce, SW_SHOW);
        ShowWindow(hScript, SW_SHOW);
        ShowWindow(hTag, SW_SHOW);
        ShowWindow(hAITrigger, SW_SHOW);
    }
    else
    {
        ShowWindow(hListBox_Range, SW_HIDE);
        ShowWindow(hTrigger, SW_HIDE);
        ShowWindow(hAttachedTrigger, SW_HIDE);
        ShowWindow(hEvent_Action, SW_HIDE);
        ShowWindow(hTriggerParam, SW_HIDE);
        ShowWindow(hTeamType, SW_HIDE);
        ShowWindow(hTaskForvce, SW_HIDE);
        ShowWindow(hScript, SW_HIDE);
        ShowWindow(hTag, SW_HIDE);
        ShowWindow(hAITrigger, SW_HIDE);
    }


}
void CObjectSearch::ToggleWindowSize(HWND hWnd)
{
    HWND hText = GetDlgItem(hWnd, Controls::Input);
    HWND hButton = GetDlgItem(hWnd, Controls::Search);
    HWND hListBox = GetDlgItem(hWnd, Controls::ListBox);

    int TriggerMove = 110;
    RECT r;
    POINT p;

    if (CObjectSearch::bListBoxButton && !CObjectSearch::ToggleWindowSize_once)
    {

        GetWindowRect(m_hwnd, &r);
        MoveWindow(m_hwnd, r.left, r.top, r.right - r.left + TriggerMove, r.bottom - r.top, true);

        GetWindowRect(hListBox, &r);
        p.x = r.left; p.y = r.top;
        ScreenToClient(m_hwnd, &p);
        MoveWindow(hListBox, p.x + TriggerMove, p.y, r.right - r.left, r.bottom - r.top, true);

        GetWindowRect(hText, &r);
        p.x = r.left; p.y = r.top;
        ScreenToClient(m_hwnd, &p);
        MoveWindow(hText, p.x + TriggerMove, p.y, r.right - r.left, r.bottom - r.top, true);

        GetWindowRect(hButton, &r);
        p.x = r.left; p.y = r.top;
        ScreenToClient(m_hwnd, &p);
        MoveWindow(hButton, p.x + TriggerMove, p.y, r.right - r.left, r.bottom - r.top, true);

        CObjectSearch::ToggleWindowSize_once = true;

    }
    else if (!CObjectSearch::bListBoxButton && CObjectSearch::ToggleWindowSize_once)
    {
        GetWindowRect(m_hwnd, &r);
        MoveWindow(m_hwnd, r.left, r.top, r.right - r.left - TriggerMove, r.bottom - r.top, true);

        GetWindowRect(hListBox, &r);
        p.x = r.left; p.y = r.top;
        ScreenToClient(m_hwnd, &p);
        MoveWindow(hListBox, p.x - TriggerMove, p.y, r.right - r.left, r.bottom - r.top, true);

        GetWindowRect(hText, &r);
        p.x = r.left; p.y = r.top;
        ScreenToClient(m_hwnd, &p);
        MoveWindow(hText, p.x - TriggerMove, p.y, r.right - r.left, r.bottom - r.top, true);

        GetWindowRect(hButton, &r);
        p.x = r.left; p.y = r.top;
        ScreenToClient(m_hwnd, &p);
        MoveWindow(hButton, p.x - TriggerMove, p.y, r.right - r.left, r.bottom - r.top, true);

        CObjectSearch::ToggleWindowSize_once = false;
    }
}

bool CObjectSearch::IsLabelMatch(const char* target, const char* source)
{
    return ExtraWindow::IsLabelMatch(target, source, bExactMatch);
}