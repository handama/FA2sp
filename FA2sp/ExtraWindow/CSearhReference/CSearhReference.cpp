#include "CSearhReference.h"
#include "../CNewTeamTypes/CNewTeamTypes.h"
#include "../CNewTrigger/CNewTrigger.h"
#include "../CNewAITrigger/CNewAITrigger.h"
#include "../Common.h"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/MultimapHelper.h"

#include <CLoading.h>
#include <CFinalSunDlg.h>
#include <CObjectDatas.h>
#include <CMapData.h>
#include <CIsoView.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CMapData/Body.h"

HWND CSearhReference::m_hwnd;
CFinalSunDlg* CSearhReference::m_parent;
CINI& CSearhReference::map = CINI::CurrentDocument;
MultimapHelper& CSearhReference::rules = Variables::RulesMap;

HWND CSearhReference::hListbox;
HWND CSearhReference::hRefresh;
HWND CSearhReference::hObjectText;
FString CSearhReference::SearchID = "";
int CSearhReference::origWndWidth;
int CSearhReference::origWndHeight;
int CSearhReference::minWndWidth;
int CSearhReference::minWndHeight;
bool CSearhReference::minSizeSet;
bool CSearhReference::IsTeamType = false;
bool CSearhReference::IsTrigger = false;
bool CSearhReference::IsVariable = false;
std::map<int, ScriptParamPos> CSearhReference::LocalVariableScripts;
std::map<int, int> CSearhReference::LocalVariableEvents;
std::map<int, int> CSearhReference::LocalVariableActions;
std::map<int, std::map<int, FString>> CSearhReference::LocalVariableParamAffectedEvents;
std::map<int, std::map<int, FString>> CSearhReference::LocalVariableParamAffectedActions;

void CSearhReference::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(310),
        pWnd->GetSafeHwnd(),
        CSearhReference::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CSearhReference.\n");
        m_parent = NULL;
        return;
    }
}

void CSearhReference::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("SearchReferenceTitle", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };
    
	Translate(1001, "SearchReferenceRefresh");

    hListbox = GetDlgItem(hWnd, Controls::Listbox);
    hRefresh = GetDlgItem(hWnd, Controls::Refresh);
    hObjectText = GetDlgItem(hWnd, Controls::ObjectText);

    Update();
}

void CSearhReference::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CSearhReference::m_hwnd = NULL;
    CSearhReference::m_parent = NULL;
    CSearhReference::SearchID = "";

}

BOOL CALLBACK CSearhReference::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CSearhReference::Initialize(hWnd);
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
        POINT topLeft;

        GetWindowRect(hListbox, &rect);
        topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);
        int newWidth = rect.right - rect.left + newWndWidth - origWndWidth;
        int newHeight = rect.bottom - rect.top + newWndHeight - origWndHeight;
        MoveWindow(hListbox, topLeft.x, topLeft.y, newWidth, newHeight, TRUE);

        GetWindowRect(hRefresh, &rect);
        topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);
        newWidth = rect.right - rect.left;
        newHeight = rect.bottom - rect.top;
        MoveWindow(hRefresh, topLeft.x + newWndWidth - origWndWidth, topLeft.y + newWndHeight - origWndHeight, newWidth, newHeight, TRUE);

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
        case Controls::Listbox:
            ListBoxProc(hWnd, CODE, lParam);
            break;
        case Controls::Refresh:
            if (CODE == BN_CLICKED)
                Update();
            break;
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CSearhReference::Close(hWnd);
        return TRUE;
    }
    case 114514: // used for update
    {
        Update();
        return TRUE;
    }

    }

    // Process this message through default handler
    return FALSE;
}

void CSearhReference::ListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam)
{
    if (SendMessage(hListbox, LB_GETCOUNT, NULL, NULL) <= 0)
        return;

    switch (nCode)
    {
    case LBN_SELCHANGE:
    case LBN_DBLCLK:
        OnSelchangeListbox(hWnd);
        break;
    default:
        break;
    }

}
void CSearhReference::OnSelchangeListbox(HWND hWnd)
{
    if (SendMessage(hListbox, LB_GETCURSEL, NULL, NULL) < 0 || SendMessage(hListbox, LB_GETCOUNT, NULL, NULL) <= 0)
    {
        return;
    }

    int idx = SendMessage(hListbox, LB_GETCURSEL, 0, NULL);
    char buffer[512]{ 0 };
    SendMessage(hListbox, LB_GETTEXT, idx, (LPARAM)buffer);

    FString ID = buffer;
    int prefix = ID.Find(": ");
    ID = ID.Mid(prefix + 2);
    FString fullName = ID;
    FString::TrimIndex(ID);

    if (IsTeamType || IsTrigger || IsVariable)
    {
        int data = SendMessage(hListbox, LB_GETITEMDATA, idx, 0);
        if (data >= 100 && data < 300)
        {
            if (CNewTrigger::GetHandle() == NULL)
                CNewTrigger::Create(m_parent);

            auto dlg = GetDlgItem(CNewTrigger::GetHandle(), CNewTrigger::Controls::SelectedTrigger);
            auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, ExtraWindow::GetTriggerDisplayName(ID));
            if (idx == CB_ERR)
                return;
            SendMessage(dlg, CB_SETCURSEL, idx, NULL);
            CNewTrigger::OnSelchangeTrigger(false, data < 200 ? data - 100 : 0, data >= 200 ? data - 200 : 0);
            SetWindowPos(CNewTrigger::GetHandle(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        }
        else if (data == 2)
        {
            if (CNewAITrigger::GetHandle() == NULL)
                CNewAITrigger::Create(m_parent);

            auto dlg = GetDlgItem(CNewAITrigger::GetHandle(), CNewAITrigger::Controls::SelectedAITrigger);
            auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, ExtraWindow::GetAITriggerDisplayName(ID));
            if (idx == CB_ERR)
                return;
            SendMessage(dlg, CB_SETCURSEL, idx, NULL);
            CNewAITrigger::OnSelchangeAITrigger();
            SetWindowPos(CNewAITrigger::GetHandle(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        }
        else if (data >= 500 && data < 550)
        {
            if (CNewScript::GetHandle() == NULL)
                CNewScript::Create(m_parent);

            auto dlg = GetDlgItem(CNewScript::GetHandle(), CNewScript::Controls::SelectedScript);
            auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, ExtraWindow::GetTeamDisplayName(ID));
            if (idx == CB_ERR)
                return;
            SendMessage(dlg, CB_SETCURSEL, idx, NULL);
            CNewScript::OnSelchangeScript();
            auto dlg2 = GetDlgItem(CNewScript::GetHandle(), CNewScript::Controls::ActionsListBox);
            SendMessage(dlg2, LB_SETCURSEL, data - 500, NULL);
            CNewScript::OnSelchangeActionListbox();
            SetWindowPos(CNewScript::GetHandle(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        }
    }
    else
    {
        if (CNewTeamTypes::GetHandle() == NULL)
            CNewTeamTypes::Create(m_parent);

        auto dlg = GetDlgItem(CNewTeamTypes::GetHandle(), CNewTeamTypes::Controls::SelectedTeam);
        auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, fullName);
        if (idx == CB_ERR)
            return;
        SendMessage(dlg, CB_SETCURSEL, idx, NULL);
        CNewTeamTypes::OnSelchangeTeamtypes();
        SetWindowPos(CNewTeamTypes::GetHandle(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    }

}

void CSearhReference::Update()
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    while (SendMessage(hListbox, LB_DELETESTRING, 0, NULL) != CB_ERR);
    int idx = 0;
    FString tmp;
    if (IsTeamType || IsTrigger)
    {
        if (IsTeamType)
            SendMessage(hObjectText, WM_SETTEXT, 0, (LPARAM)ExtraWindow::GetTeamDisplayName(SearchID));
        else if (IsTrigger)
            SendMessage(hObjectText, WM_SETTEXT, 0, (LPARAM)ExtraWindow::GetTriggerDisplayName(SearchID));
        for (auto& triggerPair : CMapDataExt::Triggers)
        {
            auto& trigger = triggerPair.second;
            int index = 0;
            for (auto& e : trigger->Events)
            {
                for (auto& ep : e.Params)
                {
                    if (ep == SearchID)
                    {
                        FString text;
                        text.Format("%s%s %s[%d]", GetPrefix(1), ExtraWindow::GetTriggerDisplayName(trigger->ID),
                            Translations::TranslateOrDefault("Event", "Event"), index);
                        SendMessage(
                            hListbox,
                            LB_SETITEMDATA,
                            SendMessage(hListbox, LB_INSERTSTRING, idx++, text),
                            100 + index
                        );
                    }
                }
                index++;
            }
            index = 0;
            for (auto& a : trigger->Actions)
            {
                for (auto& ap : a.Params)
                {
                    if (ap == SearchID)
                    {
                        FString text;
                        text.Format("%s%s %s[%d]", GetPrefix(1), ExtraWindow::GetTriggerDisplayName(trigger->ID),
                            Translations::TranslateOrDefault("Action", "Action"), index);
                        SendMessage(
                            hListbox,
                            LB_SETITEMDATA,
                            SendMessage(hListbox, LB_INSERTSTRING, idx++, text),
                            200 + index
                        );
                    }
                }
                index++;
            }
            if (trigger->AttachedTrigger == SearchID)
            {
                FString text;
                text.Format("%s%s %s", GetPrefix(1), ExtraWindow::GetTriggerDisplayName(trigger->ID),
                    Translations::TranslateOrDefault("SearchReference.AttachedTrigger", "Attached"));
                SendMessage(
                    hListbox,
                    LB_SETITEMDATA,
                    SendMessage(hListbox, LB_INSERTSTRING, idx++, text),
                    100 
                );
            }      
            // 100+ means events, 200+ means actions
        }
        if (IsTeamType)
            if (auto pSection = map.GetSection("AITriggerTypes"))
            {
                for (auto& pair : pSection->GetEntities())
                {
                    //01000139=GM2-ORCA-10,01000138,GoodMid2,1,0,GAWEAT,0100000003000000000000000000000000000000000000000000000000000000,70.000000,30.000000,70.000000,1,0,3,0,<none>,1,1,1
                    auto atoms = FString::SplitString(map.GetString("AITriggerTypes", pair.first));
                    if (atoms.size() < 18) continue;
                    if (atoms[1] == SearchID || atoms[14] == SearchID)
                    {
                        auto text = GetPrefix(4) + ExtraWindow::GetAITriggerDisplayName(pair.first);
                        SendMessage(
                            hListbox,
                            LB_SETITEMDATA,
                            SendMessage(hListbox, LB_INSERTSTRING, idx++, text),
                            2
                        );
                        // 2 means aitrigger
                    }
                }
            }
    }
    else if (IsVariable)
    {
        FString text;
        text.Format("%s - %s", SearchID, map.GetString("VariableNames", SearchID));
        SendMessage(hObjectText, WM_SETTEXT, 0, (LPARAM)text);

        if (LocalVariableScripts.empty())
        {
            if (auto pSection = CINI::FAData->GetSection("ScriptsRA2"))
            {
                for (const auto& [key, value] : pSection->GetEntities())
                {
                    auto atoms = FString::SplitString(value, 1);
                    auto scriptParams = FString::SplitString(CINI::FAData->GetString("ScriptParams", atoms[1]));
                    if (scriptParams.size() >= 2)
                    {
                        auto newParamTypes = FString::SplitString(CINI::FAData->GetString("NewParamTypes", scriptParams[1]), 4);
                        auto& sectionName = newParamTypes[0];
                        auto& loadFrom = newParamTypes[1];

                        if (sectionName == "VariableNames" && (loadFrom == "3" || loadFrom == "map"))
                        {
                            LocalVariableScripts[atoi(key)] = scriptParams.size() == 4 ? ScriptParamPos::low : ScriptParamPos::normal;
                        }
                    }
                    if (scriptParams.size() >= 4)
                    {
                        auto newParamTypes = FString::SplitString(CINI::FAData->GetString("NewParamTypes", scriptParams[3]), 4);
                        auto& sectionName = newParamTypes[0];
                        auto& loadFrom = newParamTypes[1];

                        if (sectionName == "VariableNames" && (loadFrom == "3" || loadFrom == "map"))
                        {
                            LocalVariableScripts[atoi(key)] = ScriptParamPos::high;
                        }
                    }
                }
            }
        }
        if (LocalVariableEvents.empty())
        {
            LocalVariableParamAffectedEvents.clear();
            if (auto pSection = CINI::FAData->GetSection("EventsRA2"))
            {
                for (const auto& [key, value] : pSection->GetEntities())
                {
                    auto eventInfos = FString::SplitString(value, 8);
                    FString paramType[2];
                    paramType[0] = eventInfos[1];
                    paramType[1] = eventInfos[2];
                    std::vector<FString> pParamTypes[2];
                    pParamTypes[0] = FString::SplitString(CINI::FAData->GetString("ParamTypes", paramType[0], "MISSING,0"));
                    pParamTypes[1] = FString::SplitString(CINI::FAData->GetString("ParamTypes", paramType[1], "MISSING,0"));
                    FString code = "0";
                    if (pParamTypes[0].size() == 3) code = pParamTypes[0][2];

                    if (CINI::FAData->KeyExists("NewParamTypes", pParamTypes[0][1]))
                    {
                        auto newParamTypes = FString::SplitString(CINI::FAData->GetString("NewParamTypes", pParamTypes[0][1]), 4);
                        auto& sectionName = newParamTypes[0];
                        auto& loadFrom = newParamTypes[1];

                        if (sectionName == "VariableNames" && (loadFrom == "3" || loadFrom == "map"))
                        {
                            LocalVariableEvents[atoi(key)] = code == "2" ? 1 : 0;
                        }
                    }
                    if (CINI::FAData->KeyExists("NewParamTypes", pParamTypes[1][1]))
                    {
                        auto newParamTypes = FString::SplitString(CINI::FAData->GetString("NewParamTypes", pParamTypes[1][1]), 4);
                        auto& sectionName = newParamTypes[0];
                        auto& loadFrom = newParamTypes[1];

                        if (sectionName == "VariableNames" && (loadFrom == "3" || loadFrom == "map"))
                        {
                            LocalVariableEvents[atoi(key)] = code == "2" ? 2 : 1;
                        }
                    }
                }
            }
            
            auto& params = CNewTrigger::EventParamAffectedParams;
            for (const auto& param : params)
            {
                for (const auto& [value, paramType] : param.ParamMap)
                {
                    FString type = FString::SplitString(CINI::FAData->GetString("ParamTypes", paramType, "MISSING,0"))[1];
                    if (CINI::FAData->KeyExists("NewParamTypes", type))
                    {
                        auto newParamTypes = FString::SplitString(CINI::FAData->GetString("NewParamTypes", type), 4);
                        auto& sectionName = newParamTypes[0];
                        auto& loadFrom = newParamTypes[1];

                        if (sectionName == "VariableNames" && (loadFrom == "3" || loadFrom == "map"))
                        {
                            FString eventKey;
                            eventKey.Format("%d", param.Index);
                            auto eventInfos = FString::SplitString(CINI::FAData->GetString("EventsRA2", eventKey), 8);
                            FString paramType[2];
                            paramType[0] = eventInfos[1];
                            paramType[1] = eventInfos[2];
                            std::vector<FString> pParamTypes[2];
                            pParamTypes[0] = FString::SplitString(CINI::FAData->GetString("ParamTypes", paramType[0], "MISSING,0"));
                            pParamTypes[1] = FString::SplitString(CINI::FAData->GetString("ParamTypes", paramType[1], "MISSING,0"));
                            FString code = "0";
                            if (pParamTypes[0].size() == 3) code = pParamTypes[0][2];

                            LocalVariableEvents[param.Index] = param.AffectedParam + 10 + (code == "2" ? 1 : 0);
                            LocalVariableParamAffectedEvents[param.Index][param.SourceParam + (code == "2" ? 1 : 0)] =  value;
                            break;
                        }
                    }         
                }
            }
        }
        if (LocalVariableActions.empty())
        {
            LocalVariableParamAffectedActions.clear();
            if (auto pSection = CINI::FAData->GetSection("ActionsRA2"))
            {
                for (const auto& [key, value] : pSection->GetEntities())
                {
                    auto actionInfos = FString::SplitString(value, 13);
                    FString paramType[7];
                    for (int i = 0; i < 7; i++)
                        paramType[i] = actionInfos[i + 1];

                    std::vector<FString> pParamTypes[7];
                    for (int i = 0; i < 7; i++)
                        pParamTypes[i] = FString::SplitString(CINI::FAData->GetString("ParamTypes", paramType[i], "MISSING,0"));

                    for (int i = 0; i < 7; i++)
                    {
                        if (CINI::FAData->KeyExists("NewParamTypes", pParamTypes[i][1]))
                        {
                            auto newParamTypes = FString::SplitString(CINI::FAData->GetString("NewParamTypes", pParamTypes[i][1]), 4);
                            auto& sectionName = newParamTypes[0];
                            auto& loadFrom = newParamTypes[1];

                            if (sectionName == "VariableNames" && (loadFrom == "3" || loadFrom == "map"))
                            {
                                LocalVariableActions[atoi(key)] = i;
                                break;
                            }
                        }
                    }
                }
            }

            auto& params = CNewTrigger::ActionParamAffectedParams;
            for (const auto& param : params)
            {
                for (const auto& [value, paramType] : param.ParamMap)
                {
                    FString type = FString::SplitString(CINI::FAData->GetString("ParamTypes", paramType, "MISSING,0"))[1];
                    if (CINI::FAData->KeyExists("NewParamTypes", type))
                    {
                        auto newParamTypes = FString::SplitString(CINI::FAData->GetString("NewParamTypes", type), 4);
                        auto& sectionName = newParamTypes[0];
                        auto& loadFrom = newParamTypes[1];

                        if (sectionName == "VariableNames" && (loadFrom == "3" || loadFrom == "map"))
                        {
                            FString actionKey;
                            actionKey.Format("%d", param.Index);
                            auto actionInfos = FString::SplitString(CINI::FAData->GetString("ActionsRA2", actionKey), 13);
                            FString paramType[7];
                            for (int i = 0; i < 7; i++)
                                paramType[i] = actionInfos[i + 1];

                            int paramIdx[7];
                            for (int i = 0; i < 7; i++)
                                paramIdx[i] = atoi(paramType[i]);

                            std::vector<int> affectedParamIndex2RealIndex;
                            for (int i = 0; i < 7; i++)
                            {
                                if (paramIdx[i] > 0)
                                {
                                    affectedParamIndex2RealIndex.push_back(i);
                                }
                            }
                            if (affectedParamIndex2RealIndex.size() > param.AffectedParam && affectedParamIndex2RealIndex.size() > param.SourceParam)
                            {
                                LocalVariableActions[param.Index] = affectedParamIndex2RealIndex[param.AffectedParam] + 10;
                                LocalVariableParamAffectedEvents[param.Index][affectedParamIndex2RealIndex[param.SourceParam]] = value;
                            }
                            break;
                        }
                    }
                }
            }
        }
        if (auto pSection = map.GetSection("ScriptTypes"))
        {
            for (const auto& [_, id] : pSection->GetEntities())
            {
                if (auto pScript = map.GetSection(id))
                {
                    FString key;
                    for (int i = 0; i < 50; i++)
                    {
                        key.Format("%d", i);
                        if (map.KeyExists(id, key))
                        {
                            auto atoms = FString::SplitString(map.GetString(id, key), 1);
                            int action = atoi(atoms[0]);
                            int param = atoi(atoms[1]);
                            if (LocalVariableScripts.find(action) != LocalVariableScripts.end())
                            {
                                int targetParam = 0;
                                switch (LocalVariableScripts[action])
                                {
                                case ScriptParamPos::normal:
                                    targetParam = param;
                                    break;
                                case ScriptParamPos::low:
                                    targetParam = LOWORD(param);
                                    break;
                                case ScriptParamPos::high:
                                    targetParam = HIWORD(param);
                                    break;
                                default:
                                    continue;
                                    break;
                                }
                                if (atoi(SearchID) == targetParam)
                                {
                                    FString text;
                                    text.Format("%s%s %s[%d]", GetPrefix(3), ExtraWindow::GetTeamDisplayName(id), Translations::TranslateOrDefault("SearchReferenceScriptLine", "Line"), i);

                                    SendMessage(
                                        hListbox,
                                        LB_SETITEMDATA,
                                        SendMessage(hListbox, LB_INSERTSTRING, idx++, text),
                                        500 + i
                                    );
                                    // 500+ means script and line
                                }
                            }

                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
        }

        for (const auto& [id, trigger] : CMapDataExt::Triggers)
        {
            int i = 0;
            for (const auto& eve : trigger->Events)
            {
                if (LocalVariableEvents.find(atoi(eve.EventNum)) != LocalVariableEvents.end())
                {
                    int pos = LocalVariableEvents[atoi(eve.EventNum)];
                    if (pos > 10)
                    {
                        const auto& sourceParams = LocalVariableParamAffectedEvents[atoi(eve.EventNum)];
                        for (const auto& [s, v] : sourceParams)
                        {
                            if (eve.Params[s] == v)
                            {
                                pos -= 10;
                                break;
                            }
                        }
                    }
                    if (pos < 0 || pos >= 3)
                    {
                        i++;
                        continue;
                    }
                    int value = atoi(eve.Params[pos]);
                    if (atoi(SearchID) == value)
                    {
                        FString text;
                        text.Format("%s%s %s[%d]", GetPrefix(1), ExtraWindow::GetTriggerDisplayName(id), Translations::TranslateOrDefault("Event", "Event"), i);

                        SendMessage(
                            hListbox,
                            LB_SETITEMDATA,
                            SendMessage(hListbox, LB_INSERTSTRING, idx++, text),
                            100 + i
                        );
                    }
                }
                i++;
            }
            i = 0;
            for (const auto& act : trigger->Actions)
            {
                if (LocalVariableActions.find(atoi(act.ActionNum)) != LocalVariableActions.end())
                {
                    int pos = LocalVariableActions[atoi(act.ActionNum)];
                    if (pos > 10)
                    {
                        const auto& sourceParams = LocalVariableParamAffectedEvents[atoi(act.ActionNum)];
                        for (const auto& [s, v] : sourceParams)
                        {
                            if (act.Params[s] == v)
                            {
                                pos -= 10;
                                break;
                            }
                        }
                    }
                    if (pos < 0 || pos >= 7)
                    {
                        i++;
                        continue;
                    }
                    int value = atoi(act.Params[pos]);
                    if (atoi(SearchID) == value)
                    {
                        FString text;
                        text.Format("%s%s %s[%d]", GetPrefix(1), ExtraWindow::GetTriggerDisplayName(id), Translations::TranslateOrDefault("Action", "Action"), i);

                        SendMessage(
                            hListbox,
                            LB_SETITEMDATA,
                            SendMessage(hListbox, LB_INSERTSTRING, idx++, text),
                            200 + i
                        );
                    }
                }
                i++;
            }
        }
    }   
    else
    {
        SendMessage(hObjectText, WM_SETTEXT, 0, (LPARAM)ExtraWindow::GetTeamDisplayName(SearchID));
        if (auto pSection = map.GetSection("TeamTypes"))
        {
            for (auto& pair : pSection->GetEntities())
            {
                auto refTaskforce = map.GetString(pair.second, "TaskForce");
                auto refScript = map.GetString(pair.second, "Script");
                if (SearchID == refTaskforce|| SearchID == refScript)
                {
                    auto text = GetPrefix(0) + ExtraWindow::GetTeamDisplayName(pair.second);
                    SendMessage(hListbox, LB_INSERTSTRING, idx++, 
                        text);
                }
            }
        }
    }

    return;
}

FString CSearhReference::GetPrefix(int type)
{
    switch (type)
    {
    case 0:
        return FString(Translations::TranslateOrDefault("SearhReference.Team", "Team"))+ ": ";
    case 1:
        return FString(Translations::TranslateOrDefault("SearhReference.Trigger", "Trigger"))+ ": ";
    case 3:
        return FString(Translations::TranslateOrDefault("SearhReference.Script", "Script"))+ ": ";
    case 4:
        return FString(Translations::TranslateOrDefault("SearhReference.AITrigger", "AI Trigger"))+ ": ";
    default:
        break;
    }
    return {};
}
