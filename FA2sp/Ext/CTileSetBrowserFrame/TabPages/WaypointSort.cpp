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

FString WaypointSort::sm_cachedEventsSection;
FString WaypointSort::sm_cachedActionsSection;
FString WaypointSort::sm_cachedParamTypesSection;
FString WaypointSort::sm_cachedScriptsSection;
FString WaypointSort::sm_cachedScriptParamsSection;
FHashMap<CachedEventType> WaypointSort::sm_cachedEvents;
FHashMap<CachedActionType> WaypointSort::sm_cachedActions;
FHashSet WaypointSort::sm_cachedScriptActions;
std::unordered_set<int> WaypointSort::sm_dontSaveAsWP;
bool WaypointSort::sm_cacheInitialized = false;


void WaypointSort::InitCache()
{
    if (sm_cacheInitialized)
        return;

    sm_cachedEventsSection = ExtraWindow::GetTranslatedSectionName("EventsRA2");
    sm_cachedActionsSection = ExtraWindow::GetTranslatedSectionName("ActionsRA2");
    sm_cachedParamTypesSection = ExtraWindow::GetTranslatedSectionName("ParamTypes");
    sm_cachedScriptsSection = ExtraWindow::GetTranslatedSectionName("ScriptsRA2");
    sm_cachedScriptParamsSection = ExtraWindow::GetTranslatedSectionName("ScriptParams");

    if (auto pEventsSection = CINI::FAData->GetSection(sm_cachedEventsSection))
    {
        for (auto& pair : pEventsSection->GetEntities())
        {
            auto eventInfos = FString::SplitString(pair.second, 8);
            if (eventInfos.size() < 3)
                continue;

            CachedEventType cached;
            auto paramTypes0 = FString::SplitString(
                CINI::FAData->GetString(sm_cachedParamTypesSection, eventInfos[1], "MISSING,0"), 1);
            auto paramTypes1 = FString::SplitString(
                CINI::FAData->GetString(sm_cachedParamTypesSection, eventInfos[2], "MISSING,0"), 1);

            cached.param0IsWP = (paramTypes0.size() > 1 && paramTypes0[1] == "1");
            cached.param1IsWP = (paramTypes1.size() > 1 && paramTypes1[1] == "1");

            sm_cachedEvents[pair.first] = cached;
        }
    }

    if (auto pActionsSection = CINI::FAData->GetSection(sm_cachedActionsSection))
    {
        for (auto& pair : pActionsSection->GetEntities())
        {
            auto actionInfos = FString::SplitString(pair.second, 13);
            if (actionInfos.size() < 8)
                continue;

            CachedActionType cached;
            for (int i = 0; i < 6; i++)
            {
                auto paramTypes = FString::SplitString(
                    CINI::FAData->GetString(sm_cachedParamTypesSection, actionInfos[i + 1], "MISSING,0"), 1);
                cached.paramsWP[i] = (paramTypes.size() > 1 && paramTypes[1] == "1");
            }

            cached.param7IsWP = (atoi(actionInfos[7]) > 0);
            if (cached.param7IsWP && actionInfos.size() > 1)
            {
                if (auto pDontSection = CINI::FAData->GetSection("DontSaveAsWP"))
                {
                    for (auto& dontPair : pDontSection->GetEntities())
                    {
                        if (atoi(dontPair.second) == -atoi(actionInfos[1]))
                        {
                            cached.param7IsWP = false;
                            break;
                        }
                    }
                }
            }

            sm_cachedActions[pair.first] = cached;
        }
    }

    if (auto pDontSection = CINI::FAData->GetSection("DontSaveAsWP"))
    {
        for (auto& pair : pDontSection->GetEntities())
            sm_dontSaveAsWP.insert(atoi(pair.second));
    }

    if (auto pScriptsSection = CINI::FAData->GetSection(sm_cachedScriptsSection))
    {
        for (auto& pair : pScriptsSection->GetEntities())
        {
            auto paramType = FString::SplitString(pair.second, 1);
            if (paramType.size() < 2)
                continue;
            auto scriptParamType = CINI::FAData->GetString(sm_cachedScriptParamsSection, paramType[1], "MISSING,0");
            auto types = FString::SplitString(scriptParamType, 1);
            bool hasExtra = types.size() >= 4;
            bool meetAtA = types.size() > 1 && types[1] == "1";
            bool meetAtB = hasExtra && types[3] == "1";
            if (meetAtA || meetAtB)
                sm_cachedScriptActions.insert(pair.first);
        }
    }

    sm_cacheInitialized = true;
}

int WaypointSort::ProcessWaypointLetter(const char* s)
{
    if (!s || !*s)
        return -1;

    int n = 0;
    int len = (int)strlen(s);
    for (int i = len - 1, j = 1; i >= 0; i--, j *= 26)
    {
        int c = toupper((unsigned char)s[i]);
        if (c < 'A' || c > 'Z')
            return 0; 
        n += (c - 64) * j;
    }
    if (n <= 0)
        return -1;
    return n - 1;
}

void WaypointSort::LoadAllTriggers()
{
    ExtConfigs::InitializeMap = false;
    this->Clear();

    InitCache();

    auto pSection = CINI::CurrentDocument->GetSection("Waypoints");
    if (!pSection)
    {
        ExtConfigs::InitializeMap = true;
        return;
    }

    HWND hWnd = this->GetHwnd();

    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> waypointRefs;

    for (auto& triggerPair : CMapDataExt::Triggers)
    {
        auto& trigger = triggerPair.second;

        std::unordered_set<std::string> eventWPs;
        std::unordered_set<std::string> actionWPs;

        for (auto& thisEvent : trigger->Events)
        {
            auto it = sm_cachedEvents.find(thisEvent.EventNum);
            if (it == sm_cachedEvents.end())
                continue;
            auto& cached = it->second;

            if (thisEvent.Params[0] == "2")  
            {
                if (cached.param0IsWP)
                    eventWPs.insert(std::string(thisEvent.Params[1]));
                if (cached.param1IsWP)
                    eventWPs.insert(std::string(thisEvent.Params[2]));
            }
            else 
            {
                if (cached.param1IsWP)
                    eventWPs.insert(std::string(thisEvent.Params[1]));
            }
        }

        for (auto& thisAction : trigger->Actions)
        {
            auto it = sm_cachedActions.find(thisAction.ActionNum);
            if (it == sm_cachedActions.end())
                continue;
            auto& cached = it->second;

            for (int i = 0; i < 6; i++)
            {
                if (cached.paramsWP[i])
                    actionWPs.insert(std::string(thisAction.Params[i]));
            }
            if (cached.param7IsWP)
            {
                int wpNum = ProcessWaypointLetter(thisAction.Params[6]);
                if (wpNum >= 0)
                {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%d", wpNum);
                    actionWPs.insert(buf);
                }
            }
        }

        if (!eventWPs.empty())
        {
            FString eventText;
            eventText.Format(Translations::TranslateOrDefault("ObjectInfo.Waypoint.Event",
                "Event: %s (%s)"), trigger->Name, trigger->ID);
            std::string textStr = eventText;
            std::string idStr = trigger->ID;
            for (auto& wp : eventWPs)
                waypointRefs[wp].emplace_back(textStr, idStr);
        }
        if (!actionWPs.empty())
        {
            FString actionText;
            actionText.Format(Translations::TranslateOrDefault("ObjectInfo.Waypoint.Action",
                "Action: %s (%s)"), trigger->Name, trigger->ID);
            std::string textStr = actionText;
            std::string idStr = trigger->ID;
            for (auto& wp : actionWPs)
                waypointRefs[wp].emplace_back(textStr, idStr);
        }
    }

    if (auto pScriptSection = CINI::CurrentDocument->GetSection("ScriptTypes"))
    {
        for (auto& pair : pScriptSection->GetEntities())
        {
            std::unordered_set<std::string> scriptWPs;

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

                if (sm_cachedScriptActions.find(app[0]) != sm_cachedScriptActions.end())
                    scriptWPs.insert(std::string(app[1]));
            }

            if (!scriptWPs.empty())
            {
                FString text;
                text.Format(Translations::TranslateOrDefault("ObjectInfo.Waypoint.Script",
                    "Script: %s (%s)"), CINI::CurrentDocument->GetString(pair.second, "Name"), pair.second);
                std::string textStr = text;
                std::string idStr = FString(pair.second);
                for (auto& wp : scriptWPs)
                    waypointRefs[wp].emplace_back(textStr, idStr);
            }
        }
    }

    if (auto pTeamSection = CINI::CurrentDocument->GetSection("TeamTypes"))
    {
        for (auto& pair : pTeamSection->GetEntities())
        {
            auto wp = CINI::CurrentDocument->GetString(pair.second, "Waypoint");
            auto wp2 = CINI::CurrentDocument->GetString(pair.second, "TransportWaypoint");
            int wpNum = ProcessWaypointLetter(wp);
            int wpNum2 = ProcessWaypointLetter(wp2);
            if (wpNum >= 0 || wpNum2 >= 0 && CINI::CurrentDocument->GetBool(pair.second, "UseTransportOrigin"))
            {
                FString text;
                text.Format(Translations::TranslateOrDefault("ObjectInfo.Waypoint.Team",
                    "Team: %s (%s)"), CINI::CurrentDocument->GetString(pair.second, "Name"), pair.second);
                std::string textStr = text;
                std::string idStr = FString(pair.second);

                if (wpNum == wpNum2)
                {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%d", wpNum);
                    waypointRefs[buf].emplace_back(textStr, idStr);
                }
                else{
                    if (wpNum >= 0)
                    {                  
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%d", wpNum);
                        waypointRefs[buf].emplace_back(textStr, idStr);
                    }
                    if (wpNum2 >= 0)
                    {                  
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%d", wpNum2);
                        waypointRefs[buf].emplace_back(textStr, idStr);
                    }
                }
            }
        }
    }

    SendMessage(hWnd, WM_SETREDRAW, FALSE, 0);

    for (auto& pair : pSection->GetEntities())
    {
        auto second = atoi(pair.second);
        if (second < 0)
            continue;

        int x = second % 1000;
        int y = second / 1000;
        auto name2 = atoi(pair.first);

        FString pSrc;
        pSrc.Format("%03d (%d, %d)", name2, x, y);

        HTREEITEM hItem = TreeViewHelper::InsertTreeItem(hWnd, pSrc, pair.first, TVI_ROOT);

        auto it = waypointRefs.find(std::string(FString(pair.first)));
        if (it != waypointRefs.end())
        {
            for (auto& ref : it->second)
            {
                TreeViewHelper::InsertTreeItem(hWnd, ref.first.c_str(), ref.second.c_str(), hItem);
            }
        }
    }

    SendMessage(hWnd, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hWnd, NULL, TRUE);

    ExtConfigs::InitializeMap = true;
}

void WaypointSort::Clear()
{
    TreeViewHelper::ClearTreeView(this->GetHwnd());
    this->IndexClear();
}

BOOL WaypointSort::OnNotify(LPNMTREEVIEW lpNmTreeView)
{
    switch (lpNmTreeView->hdr.code)
    {
    case TVN_SELCHANGED:
        if (auto data = TreeViewHelper::GetTreeItemData(this->GetHwnd(), lpNmTreeView->itemNew.hItem))
        {
            if (data->isParent) break;
            auto& pID = data->param;
            bool Success = false;
            if (strlen(pID) && ExtConfigs::InitializeMap)
            {
                if (auto pCord = CINI::CurrentDocument->TryGetString("Waypoints", pID))
                {
                    auto second = atoi(*pCord);
                    if (second > 0)
                    {
                        CObjectSearch::MoveToMapCoord(second / 1000, second % 1000);
                    }
                }
                if (IsWindowVisible(CNewTrigger::GetFirstValidInstance().GetHandle()))
                {
                    FString pStr = CINI::CurrentDocument->GetString("Triggers", pID);
                    auto results = FString::SplitString(pStr);
                    if (results.size() > 3)
                    {
                        pStr = results[2];
                        FString tmp = pStr;
                        pStr.Format("%s (%s)", pID, tmp);
                        auto idx = SendMessage(CNewTrigger::GetFirstValidInstance().hSelectedTrigger, CB_FINDSTRINGEXACT, 0, (LPARAM)pStr);
                        if (idx != CB_ERR)
                        {
                            SendMessage(CNewTrigger::GetFirstValidInstance().hSelectedTrigger, CB_SETCURSEL, idx, NULL);
                            CNewTrigger::GetFirstValidInstance().OnSelchangeTrigger();
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
    int tabPageheight = 20 * CFinalSunAppExt::ProgramScaleFactor;
    ::MoveWindow(this->GetHwnd(), 2, tabPageheight, rect.right - rect.left - 6, rect.bottom - rect.top - 6 - tabPageheight, FALSE);
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

std::string WaypointSort::MakeLabelKey(HTREEITEM hParent, LPCSTR pszLabel)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%p:", hParent);
    return std::string(buf) + pszLabel;
}

void WaypointSort::IndexAdd(HTREEITEM hParent, LPCSTR pszLabel, HTREEITEM hItem) const
{
    if (hParent && pszLabel && pszLabel[0])
        m_labelIndex[MakeLabelKey(hParent, pszLabel)] = hItem;
}

void WaypointSort::IndexRemove(HTREEITEM hParent, LPCSTR pszLabel) const
{
    if (hParent && pszLabel && pszLabel[0])
        m_labelIndex.erase(MakeLabelKey(hParent, pszLabel));
}

void WaypointSort::IndexClear() const
{
    m_labelIndex.clear();
}

HTREEITEM WaypointSort::FindLabel(HTREEITEM hItemParent, LPCSTR pszLabel) const
{
    auto it = m_labelIndex.find(MakeLabelKey(hItemParent, pszLabel));
    if (it != m_labelIndex.end())
        return it->second;
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
        auto hWpItem = TreeViewHelper::InsertTreeItem(this->GetHwnd(), pSrc, triggerId, hParent);
        this->IndexAdd(hParent, pSrc, hWpItem);

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
                    auto eventInfos = FString::SplitString(CINI::FAData->GetString(ExtraWindow::GetTranslatedSectionName("EventsRA2"), thisEvent.EventNum, "MISSING,0,0,0,0,MISSING,0,1,0"), 8);
                    FString paramType[2];
                    paramType[0] = eventInfos[1];
                    paramType[1] = eventInfos[2];
                    std::vector<FString> pParamTypes[2];
                    pParamTypes[0] = FString::SplitString(CINI::FAData->GetString(ExtraWindow::GetTranslatedSectionName("ParamTypes"), paramType[0], "MISSING,0"));
                    pParamTypes[1] = FString::SplitString(CINI::FAData->GetString(ExtraWindow::GetTranslatedSectionName("ParamTypes"), paramType[1], "MISSING,0"));
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
                    auto actionInfos = FString::SplitString(CINI::FAData->GetString(ExtraWindow::GetTranslatedSectionName("ActionsRA2"), thisAction.ActionNum, "MISSING,0,0,0,0,0,0,0,0,0,MISSING,0,1,0"), 13);
                    FString thisWp = "-1";
                    FString paramType[7];
                    for (int i = 0; i < 7; i++)
                        paramType[i] = actionInfos[i + 1];

                    std::vector<FString> pParamTypes[6];
                    for (int i = 0; i < 6; i++)
                        pParamTypes[i] = FString::SplitString(CINI::FAData->GetString(ExtraWindow::GetTranslatedSectionName("ParamTypes"), paramType[i], "MISSING,0"));

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

                        if (sm_cachedScriptActions.find(app[0]) != sm_cachedScriptActions.end())
                            add = true;
                        int actionType = atoi(app[0]);
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
                    auto wp2 = CINI::CurrentDocument->GetString(pair.second, "TransportWaypoint");

                    if (process(wp) == atoi(triggerId) 
                    || (process(wp2) == atoi(triggerId) && CINI::CurrentDocument->GetBool(pair.second, "UseTransportOrigin")))
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