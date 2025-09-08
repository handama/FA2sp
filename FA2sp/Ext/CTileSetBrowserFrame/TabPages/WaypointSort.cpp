#include "WaypointSort.h"

#include "../../../FA2sp.h"
#include "../../../Helpers/STDHelpers.h"

#include <CFinalSunDlg.h>

#include "TaskForceSort.h"
#include "ScriptSort.h"
#include <CLoading.h>
#include "../../../ExtraWindow/CObjectSearch/CObjectSearch.h"
#include "../../../Helpers/Translations.h"
#include "../../CMapData/Body.h"
#include "../../../ExtraWindow/CNewTeamTypes/CNewTeamTypes.h"
#include "../../../ExtraWindow/CNewTaskforce/CNewTaskforce.h"
#include "../../../ExtraWindow/CNewScript/CNewScript.h"
#include "../../../Miscs/DialogStyle.h"

WaypointSort WaypointSort::Instance;

void WaypointSort::LoadAllTriggers()
{
    ExtConfigs::InitializeMap = false;
    this->Clear();
    // TODO : 
    // Optimisze the efficiency
    if (auto pSection = CINI::CurrentDocument->GetSection("Waypoints"))
    {
        for (auto& pair : pSection->GetEntities())
        {
            auto second = atoi(pair.second);
            if (second >= 0)
            {
                this->AddTrigger(pair.first, second % 1000, second / 1000);
            }
        }
    }
    ExtConfigs::InitializeMap = true;
}

void WaypointSort::Clear()
{
    TreeViewHelper::ClearTreeView(this->GetHwnd());
}

BOOL WaypointSort::OnNotify(LPNMTREEVIEW lpNmTreeView)
{
    switch (lpNmTreeView->hdr.code)
    {
    case TVN_SELCHANGED:
        if (auto data = TreeViewHelper::GetTreeItemData(this->GetHwnd(), lpNmTreeView->itemNew.hItem))
        {
            auto& pID = data->param;
            bool Success = false;
            if (strlen(pID) && ExtConfigs::InitializeMap)
            {
                auto pSection = CINI::CurrentDocument->GetSection("Waypoints");
                if (pSection->GetEntities().find(pID) != pSection->GetEntities().end())
                {
                    if (auto pCord = CINI::CurrentDocument->GetString("Waypoints", pID))
                    {
                        auto second = atoi(pCord);
                        if (second > 0)
                        {
                            CObjectSearch::MoveToMapCoord(second / 1000, second % 1000);
                        }
                    }
                }
                if (IsWindowVisible(CNewTrigger::GetHandle()))
                {
                    FString pStr = CINI::CurrentDocument->GetString("Triggers", pID);
                    auto results = FString::SplitString(pStr);
                    if (results.size() > 3)
                    {
                        pStr = results[2];
                        FString tmp = pStr;
                        pStr.Format("%s (%s)", pID, tmp);
                        auto idx = SendMessage(CNewTrigger::hSelectedTrigger, CB_FINDSTRINGEXACT, 0, (LPARAM)pStr);
                        if (idx != CB_ERR)
                        {
                            SendMessage(CNewTrigger::hSelectedTrigger, CB_SETCURSEL, idx, NULL);
                            CNewTrigger::OnSelchangeTrigger();
                            Success = true;
                        }
                    }
                }
                if (IsWindowVisible(CNewScript::GetHandle()))
                {
                    FString pStr = CINI::CurrentDocument->GetString(pID, "Name");
                    FString space1 = " (";
                    FString space2 = ")";

                    int idx = SendMessage(CNewScript::hSelectedScript, CB_FINDSTRINGEXACT, 0, (LPARAM)(pID + space1 + pStr + space2));
                    if (idx != CB_ERR)
                    {
                        SendMessage(CNewScript::hSelectedScript, CB_SETCURSEL, idx, NULL);
                        CNewScript::OnSelchangeScript();
                        Success = true;
                    }
                }
                if (IsWindowVisible(CNewTeamTypes::GetHandle()))
                {
                    FString pStr = CINI::CurrentDocument->GetString(pID, "Name");
                    FString space1 = " (";
                    FString space2 = ")";

                    int idx = SendMessage(CNewTeamTypes::hSelectedTeam, CB_FINDSTRINGEXACT, 0, (LPARAM)(pID + space1 + pStr + space2));
                    if (idx != CB_ERR)
                    {
                        SendMessage(CNewTeamTypes::hSelectedTeam, CB_SETCURSEL, idx, NULL);
                        CNewTeamTypes::OnSelchangeTeamtypes();
                        Success = true;
                    }

                }

            }
            return Success;
        }
        break;
    default:
        break;
    }

    return FALSE;
}

BOOL WaypointSort::OnMessage(PMSG pMsg)
{
    switch (pMsg->message)
    {
    case WM_RBUTTONDOWN:
        this->ShowMenu(pMsg->pt);
        return TRUE;
    default:
        break;
    }

    return FALSE;
}

void WaypointSort::Create(HWND hParent)
{
    RECT rect;
    ::GetClientRect(hParent, &rect);

    this->m_hWnd = CreateWindowEx(NULL, "SysTreeView32", nullptr,
        WS_CHILD | WS_BORDER | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL |
        TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hParent,
        NULL, static_cast<HINSTANCE>(FA2sp::hInstance), nullptr);

    if (ExtConfigs::EnableDarkMode && this->m_hWnd)
    {
        ::SendMessage(this->m_hWnd, TVM_SETBKCOLOR, 0, RGB(32, 32, 32));
        ::SendMessage(this->m_hWnd, TVM_SETTEXTCOLOR, 0, RGB(220, 220, 220));
    }
}

void WaypointSort::OnSize() const
{
    RECT rect;
    ::GetClientRect(::GetParent(this->GetHwnd()), &rect);
    ::MoveWindow(this->GetHwnd(), 2, 29, rect.right - rect.left - 6, rect.bottom - rect.top - 35, FALSE);
}

void WaypointSort::ShowWindow(bool bShow) const
{
    ::ShowWindow(this->GetHwnd(), bShow ? SW_SHOW : SW_HIDE);
}

void WaypointSort::ShowWindow() const
{
    this->ShowWindow(true);
}

void WaypointSort::HideWindow() const
{
    this->ShowWindow(false);
}

void WaypointSort::ShowMenu(POINT pt) const
{
    HMENU hPopupMenu = ::CreatePopupMenu();
    ::AppendMenu(hPopupMenu, MF_STRING, (UINT_PTR)MenuItem::Refresh, Translations::TranslateOrDefault("Refresh", "Refresh"));
    ::TrackPopupMenu(hPopupMenu, TPM_VERTICAL | TPM_HORIZONTAL, pt.x, pt.y, NULL, this->GetHwnd(), nullptr);
}

bool WaypointSort::IsValid() const
{
    return this->GetHwnd() != NULL;
}

bool WaypointSort::IsVisible() const
{
    return this->IsValid() && ::IsWindowVisible(this->m_hWnd);
}

const FString& WaypointSort::GetCurrentPrefix() const
{
    return this->m_strPrefix;
}

HWND WaypointSort::GetHwnd() const
{
    return this->m_hWnd;
}

WaypointSort::operator HWND() const
{
    return this->GetHwnd();
}

HTREEITEM WaypointSort::FindLabel(HTREEITEM hItemParent, LPCSTR pszLabel) const
{
    TVITEM tvi;
    char chLabel[0x200];

    for (tvi.hItem = TreeView_GetChild(this->GetHwnd(), hItemParent); tvi.hItem;
        tvi.hItem = TreeView_GetNextSibling(this->GetHwnd(), tvi.hItem))
    {
        tvi.mask = TVIF_TEXT | TVIF_CHILDREN;
        tvi.pszText = chLabel;
        tvi.cchTextMax = _countof(chLabel);
        if (TreeView_GetItem(this->GetHwnd(), &tvi))
        {
            if (strcmp(tvi.pszText, pszLabel) == 0)
                return tvi.hItem;
            if (tvi.cChildren)
            {
                HTREEITEM hChildSearch = this->FindLabel(tvi.hItem, pszLabel);
                if (hChildSearch) 
                    return hChildSearch;
            }
        }
    }
    return NULL;
}

std::vector<FString> WaypointSort::GetGroup(FString triggerId, FString& name) const
{
    FString pSrc = CINI::CurrentDocument->GetString(triggerId, "Name", "");

    auto ret = std::vector<FString>{};
    int nStart = pSrc.Find('[');
    int nEnd = pSrc.Find(']');
    if (nStart < nEnd && nStart == 0)
    {
        name = pSrc.Mid(nEnd + 1);
        pSrc = pSrc.Mid(nStart + 1, nEnd - nStart - 1);
        ret = FString::SplitString(pSrc, ".");
        return ret;
    }
    else
        name = pSrc;
    
    ret.clear();
    return ret;
}

void WaypointSort::AddTrigger(FString triggerId, int x, int y) const
{
    if (this->IsVisible())
    {
        auto name2 = atoi(triggerId);
        FString pSrc;
        pSrc.Format("%03d (%d, %d)", name2, x, y);
        

        auto hParent = TVI_ROOT;
        TreeViewHelper::InsertTreeItem(this->GetHwnd(), pSrc, triggerId, hParent);

        if (HTREEITEM hNode = this->FindLabel(hParent, pSrc))
        {
            hParent = hNode;
            FString pWP;
            pWP.Format("%d", name2);
            auto process = [](const char* s)
                {
                    int n = 0;
                    int len = strlen(s);
                    for (int i = len - 1, j = 1; i >= 0; i--, j *= 26)
                    {
                        int c = toupper(s[i]);
                        if (c < 'A' || c > 'Z') return 0;
                        n += ((int)c - 64) * j;
                    }
                    if (n <= 0)
                        return -1;
                    return n - 1;
                };

            for (auto& triggerPair : CMapDataExt::Triggers)
            {
                auto& trigger = triggerPair.second;
                bool addEvent = false;
                bool addAction = false;
                for (auto& thisEvent : trigger->Events)
                {
                    auto eventInfos = FString::SplitString(CINI::FAData->GetString("EventsRA2", thisEvent.EventNum, "MISSING,0,0,0,0,MISSING,0,1,0"), 8);
                    FString paramType[2];
                    paramType[0] = eventInfos[1];
                    paramType[1] = eventInfos[2];
                    std::vector<FString> pParamTypes[2];
                    pParamTypes[0] = FString::SplitString(CINI::FAData->GetString("ParamTypes", paramType[0], "MISSING,0"));
                    pParamTypes[1] = FString::SplitString(CINI::FAData->GetString("ParamTypes", paramType[1], "MISSING,0"));
                    FString thisWp = "-1";
                    if (thisEvent.Params[0] == "2")
                    {
                        if (pParamTypes[0][1] == "1")// waypoint
                        {
                            thisWp = thisEvent.Params[1];
                            if (thisWp == pWP) addEvent = true;
                        }
                        if (pParamTypes[1][1] == "1")// waypoint
                        {
                            thisWp = thisEvent.Params[2];
                            if (thisWp == pWP) addEvent = true;
                        }
                    }
                    else
                    {
                        if (pParamTypes[1][1] == "1")// waypoint
                        {
                            thisWp = thisEvent.Params[1];
                            if (thisWp == pWP) addEvent = true;
                        }
                    }
                }
                if (addEvent)
                {
                    FString text;
                    text.Format(Translations::TranslateOrDefault("ObjectInfo.Waypoint.Event",
                        "Event: %s (%s)"), trigger->Name, trigger->ID);

                    TreeViewHelper::InsertTreeItem(this->GetHwnd(), text, trigger->ID, hParent);
                }

                for (auto& thisAction : trigger->Actions)
                {
                    auto actionInfos = FString::SplitString(CINI::FAData->GetString("ActionsRA2", thisAction.ActionNum, "MISSING,0,0,0,0,0,0,0,0,0,MISSING,0,1,0"), 13);
                    FString thisWp = "-1";
                    FString paramType[7];
                    for (int i = 0; i < 7; i++)
                        paramType[i] = actionInfos[i + 1];

                    std::vector<FString> pParamTypes[6];
                    for (int i = 0; i < 6; i++)
                        pParamTypes[i] = FString::SplitString(CINI::FAData->GetString("ParamTypes", paramType[i], "MISSING,0"));

                    thisAction.Param7isWP = true;
                    for (auto& pair : CINI::FAData->GetSection("DontSaveAsWP")->GetEntities())
                    {
                        if (atoi(pair.second) == -atoi(paramType[0]))
                            thisAction.Param7isWP = false;
                    }

                    for (int i = 0; i < 6; i++)
                    {
                        auto& param = pParamTypes[i];
                        if (param[1] == "1")// waypoint
                        {
                            thisWp = thisAction.Params[i];
                            if (thisWp == pWP) addAction = true;
                        }
                    }
                    if (atoi(paramType[6]) > 0 && thisAction.Param7isWP)
                    {
                        thisWp.Format("%d", process(thisAction.Params[6]));
                        if (thisWp == pWP) addAction = true;
                    }
                }
                if (addAction)
                {
                    FString text;
                    text.Format(Translations::TranslateOrDefault("ObjectInfo.Waypoint.Action",
                        "Action: %s (%s)"), trigger->Name, trigger->ID);

                    TreeViewHelper::InsertTreeItem(this->GetHwnd(), text, trigger->ID, hParent);
                }
            }

            if (auto pSection = CINI::CurrentDocument->GetSection("ScriptTypes"))
            {
                for (auto& pair : pSection->GetEntities())
                {
                    bool add = false;

                    for (int i = 0; i < 50; i++)
                    {
                        char id[10];
                        _itoa(i, id, 10);
                        auto line = CINI::CurrentDocument->GetString(pair.second, id);
                        if (line == "")
                            continue;

                        auto app = FString::SplitString(line);
                        if (app.size() != 2)
                            continue;

                        int actionType = atoi(app[0]);
                        switch (actionType)
                        {
                        case 1: if (app[1] == triggerId) add = true; break;
                        case 3: if (app[1] == triggerId) add = true; break;
                        case 15: if (app[1] == triggerId) add = true; break;
                        case 16: if (app[1] == triggerId) add = true; break;
                        case 59: if (app[1] == triggerId) add = true; break;
                        default: break;
                        }
                    }
                    if (add)
                    {
                        FString text;
                        text.Format(Translations::TranslateOrDefault("ObjectInfo.Waypoint.Script",
                            "Script: %s (%s)"), CINI::CurrentDocument->GetString(pair.second, "Name"), pair.second);
                        TreeViewHelper::InsertTreeItem(this->GetHwnd(), text, pair.second, hParent);
                    }
                }
            }
            if (auto pSection = CINI::CurrentDocument->GetSection("TeamTypes"))
            {
                auto process = [](const char* s)
                    {
                        int n = 0;
                        int len = strlen(s);
                        for (int i = len - 1, j = 1; i >= 0; i--, j *= 26)
                        {
                            int c = toupper(s[i]);
                            if (c < 'A' || c > 'Z') return 0;
                            n += ((int)c - 64) * j;
                        }
                        if (n <= 0)
                            return -1;
                        return n - 1;
                    };
                for (auto& pair : pSection->GetEntities())
                {
                    auto wp = CINI::CurrentDocument->GetString(pair.second, "Waypoint");

                    if (process(wp) == atoi(triggerId))
                    {
                        std::vector<FString> skiplist;
                        bool add = true;
                        if (ExtConfigs::Waypoint_SkipCheckList)
                        {
                            skiplist = FString::SplitStringTrimmed(ExtConfigs::Waypoint_SkipCheckList);
                        }
                        if (skiplist.size() > 0)
                        {
                            for (auto& wp : skiplist)
                            {
                                if (triggerId == wp)
                                    add = false;
                            }
                        }

                        if (add)
                        {
                            FString text;
                            text.Format(Translations::TranslateOrDefault("ObjectInfo.Waypoint.Team",
                                "Team: %s (%s)"), CINI::CurrentDocument->GetString(pair.second, "Name"), pair.second);
                            TreeViewHelper::InsertTreeItem(this->GetHwnd(), text, pair.second, hParent);
                        }
                    }
                }
            }
        }
    }
}


