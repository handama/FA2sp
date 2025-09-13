#include "CNewScript.h"
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
#include <Miscs/Miscs.h>
#include "../CObjectSearch/CObjectSearch.h"
#include "../../Ext/CTileSetBrowserFrame/TabPages/ScriptSort.h"
#include "../CNewTrigger/CNewTrigger.h"
#include "../CSearhReference/CSearhReference.h"
#include <numeric>

HWND CNewScript::m_hwnd;
CFinalSunDlg* CNewScript::m_parent;
CINI& CNewScript::map = CINI::CurrentDocument;
CINI& CNewScript::fadata = CINI::FAData;
MultimapHelper& CNewScript::rules = Variables::RulesMap;

HWND CNewScript::hSelectedScript;
HWND CNewScript::hNewScript;
HWND CNewScript::hDelScript;
HWND CNewScript::hCloScript;
HWND CNewScript::hAddAction;
HWND CNewScript::hDeleteAction;
HWND CNewScript::hCloneAction;
HWND CNewScript::hName;
HWND CNewScript::hActionsListBox;
HWND CNewScript::hActionType;
HWND CNewScript::hActionParam;
HWND CNewScript::hActionExtraParam;
HWND CNewScript::hDescription;
HWND CNewScript::hMoveUp;
HWND CNewScript::hMoveDown;
HWND CNewScript::hActionParamDes;
HWND CNewScript::hActionExtraParamDes;
HWND CNewScript::hInsert;
HWND CNewScript::hSearchReference;

int CNewScript::SelectedScriptIndex = -1;
FString CNewScript::CurrentScriptID;
std::map<int, FString> CNewScript::ScriptLabels;
std::map<int, FString> CNewScript::ActionTypeLabels;
std::map<int, FString> CNewScript::ActionParamLabels;
std::map<int, FString> CNewScript::ActionExtraParamLabels;
std::map<FString, bool> CNewScript::ActionHasExtraParam;
bool CNewScript::Autodrop;
bool CNewScript::ParamAutodrop[2];
bool CNewScript::DropNeedUpdate;
bool CNewScript::bInsert;
WNDPROC CNewScript::OriginalListBoxProc;

void CNewScript::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(306),
        pWnd->GetSafeHwnd(),
        CNewScript::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CNewScript.\n");
        m_parent = NULL;
        return;
    }
}

void CNewScript::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("ScriptTypesTitle", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };

    Translate(50700, "ScriptTypesDesc");
    Translate(50701, "ScriptTypesSelectedScript");
    Translate(50702, "ScriptTypesName");
    Translate(50703, "ScriptTypesActions");
    Translate(50704, "ScriptTypesActionType");
    Translate(50705, "ScriptTypesActionDesc");
    Translate(1154, "ScriptTypesAddScript");
    Translate(1066, "ScriptTypesDelScript");
    Translate(6300, "ScriptTypesCloScript");
    Translate(1173, "ScriptTypesAddAction");
    Translate(1174, "ScriptTypesDelAction");
    Translate(1198, "ScriptTypesActionParam");
    Translate(6301, "ScriptTypesCloAction");
    Translate(6302, "ScriptTypesInsertMode");
    Translate(6303, "ScriptTypesExtraParam");
    Translate(6305, "ScriptTypesMoveUp");
    Translate(6306, "ScriptTypesMoveDown");
    Translate(1999, "SearchReferenceTitle");

    hSelectedScript = GetDlgItem(hWnd, Controls::SelectedScript);
    hNewScript = GetDlgItem(hWnd, Controls::NewScript);
    hDelScript = GetDlgItem(hWnd, Controls::DelScript);
    hCloScript = GetDlgItem(hWnd, Controls::CloScript);
    hAddAction = GetDlgItem(hWnd, Controls::AddAction);
    hDeleteAction = GetDlgItem(hWnd, Controls::DeleteAction);
    hCloneAction = GetDlgItem(hWnd, Controls::CloneAction);
    hName = GetDlgItem(hWnd, Controls::Name);
    hActionsListBox = GetDlgItem(hWnd, Controls::ActionsListBox);
    hActionType = GetDlgItem(hWnd, Controls::ActionType);
    hActionParam = GetDlgItem(hWnd, Controls::ActionParam);
    hActionExtraParam = GetDlgItem(hWnd, Controls::ActionExtraParam);
    hDescription = GetDlgItem(hWnd, Controls::Description);
    hMoveUp = GetDlgItem(hWnd, Controls::MoveUp);
    hMoveDown = GetDlgItem(hWnd, Controls::MoveDown);
    hActionParamDes = GetDlgItem(hWnd, Controls::ActionParamDes);
    hActionExtraParamDes = GetDlgItem(hWnd, Controls::ActionExtraParamDes);
    hInsert = GetDlgItem(hWnd, Controls::Insert);
    hSearchReference = GetDlgItem(hWnd, Controls::SearchReference);
    bInsert = false;

    ExtraWindow::SetEditControlFontSize(hDescription, 1.3f);
    int tabstops[2] = { 80, 100 };
    SendMessage(hActionsListBox, LB_SETTABSTOPS, 2, (LPARAM)&tabstops);

    if (hActionsListBox)
        OriginalListBoxProc = (WNDPROC)SetWindowLongPtr(hActionsListBox, GWLP_WNDPROC, (LONG_PTR)ListBoxSubclassProc);

    Update(hWnd);
}

void CNewScript::Update(HWND& hWnd)
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    if (ScriptSort::Instance.IsVisible())
        ScriptSort::Instance.LoadAllTriggers();

    DropNeedUpdate = false;

    int idx = 0;
    
    ExtraWindow::SortTeams(hSelectedScript, "ScriptTypes", SelectedScriptIndex);

    int count = SendMessage(hSelectedScript, CB_GETCOUNT, NULL, NULL);
    if (SelectedScriptIndex < 0)
        SelectedScriptIndex = 0;
    if (SelectedScriptIndex > count - 1)
        SelectedScriptIndex = count - 1;
    SendMessage(hSelectedScript, CB_SETCURSEL, SelectedScriptIndex, NULL);
    
    idx = 0;
    while (SendMessage(hActionType, CB_DELETESTRING, 0, NULL) != CB_ERR);
    if (auto pSection = fadata.GetSection("ScriptsRA2"))
    {
        for (auto& pair : pSection->GetEntities())
        {
            auto atoms = FString::SplitString(pair.second, 4);
            if (atoms[2] == "0")
                SendMessage(hActionType, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)atoms[0]);
        }
    }

    if (auto pSection = fadata.GetSection("ScriptsRA2"))
    {
        for (auto& kvp : pSection->GetEntities())
        {
            auto atoms2 = FString::SplitString(kvp.second, 4);
            FString name = atoms2[0];
            FString::TrimIndex(name);
            auto& paramIdx = atoms2[1];
            auto& disable = atoms2[2];
            auto& hasParam = atoms2[3];
            auto& description = atoms2[4];
            if (hasParam == "1")
            {
                if (auto pSectionParam = fadata.GetSection("ScriptParams"))
                {
                    auto param = FString::SplitString(fadata.GetString("ScriptParams", paramIdx));
                    if (param.size() == 4)
                    {
                        ActionHasExtraParam[name] = true;
                        continue;
                    }
                }
            }
            ActionHasExtraParam[name] = false;
        }
    }

    Autodrop = false;
    OnSelchangeScript();
}

void CNewScript::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CNewScript::m_hwnd = NULL;
    CNewScript::m_parent = NULL;

}

LRESULT CALLBACK CNewScript::ListBoxSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
                OnSelchangeActionListbox();

            }
            else {
                SendMessage(hWnd, LB_SETCURSEL, 0, 0);
            }
            return TRUE;
        }
    }
    return CallWindowProc(OriginalListBoxProc, hWnd, message, wParam, lParam);
}

BOOL CALLBACK CNewScript::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CNewScript::Initialize(hWnd);
        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::ActionsListBox:
            ListBoxProc(hWnd, CODE, lParam);
            break;
        case Controls::NewScript:
            if (CODE == BN_CLICKED)
                OnClickNewScript();
            break;
        case Controls::DelScript:
            if (CODE == BN_CLICKED)
                OnClickDelScript(hWnd);
            break;
        case Controls::CloScript:
            if (CODE == BN_CLICKED)
                OnClickCloScript(hWnd);
            break;
        case Controls::AddAction:
            if (CODE == BN_CLICKED)
                OnClickAddAction(hWnd);
            break;
        case Controls::CloneAction:
            if (CODE == BN_CLICKED)
                OnClickCloneAction(hWnd);
            break;
        case Controls::DeleteAction:
            if (CODE == BN_CLICKED)
                OnClickDeleteAction(hWnd);
            break;
        case Controls::MoveUp:
            if (CODE == BN_CLICKED)
                OnClickMoveupAction(hWnd, false);
            break;
        case Controls::MoveDown:
            if (CODE == BN_CLICKED)
                OnClickMoveupAction(hWnd, true);
            break;
        case Controls::SearchReference:
            if (CODE == BN_CLICKED)
                OnClickSearchReference(hWnd);
            break;
        case Controls::SelectedScript:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeScript();
            else if (CODE == CBN_DROPDOWN)
                OnSeldropdownScript(hWnd);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeScript(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupScript();
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::Name:
            if (CODE == EN_CHANGE)
            {
                if (SelectedScriptIndex < 0)
                    break;
                char buffer[512]{ 0 };
                GetWindowText(hName, buffer, 511);
                map.WriteString(CurrentScriptID, "Name", buffer);

                DropNeedUpdate = true;

                FString name;
                name.Format("%s (%s)", CurrentScriptID, buffer);
                SendMessage(hSelectedScript, CB_DELETESTRING, SelectedScriptIndex, NULL);
                SendMessage(hSelectedScript, CB_INSERTSTRING, SelectedScriptIndex, (LPARAM)(LPCSTR)name);
                SendMessage(hSelectedScript, CB_SETCURSEL, SelectedScriptIndex, NULL);
            }
            break;
        case Controls::ActionType:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeActionType();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeActionType(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupActionType();
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::ActionParam:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeActionParam();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeActionParam(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupActionParam();
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::ActionExtraParam:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeActionExtraParam();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeActionExtraParam(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupActionExtraParam();
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::Insert:
            bInsert = SendMessage(hInsert, BM_GETCHECK, 0, 0);
            break;
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CNewScript::Close(hWnd);
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

void CNewScript::ListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam)
{
    if (SendMessage(hActionsListBox, LB_GETCOUNT, NULL, NULL) <= 0)
        return;

    switch (nCode)
    {
    case LBN_SELCHANGE:
    case LBN_DBLCLK:
        OnSelchangeActionListbox();
        break;
    default:
        break;
    }

}

void CNewScript::OnSelchangeActionListbox()
{
    if (SelectedScriptIndex < 0 || SendMessage(hActionsListBox, LB_GETCURSEL, NULL, NULL) < 0 || SendMessage(hActionsListBox, LB_GETCOUNT, NULL, NULL) <= 0)
    {
        SendMessage(hActionType, CB_SETCURSEL, -1, NULL);
        SendMessage(hActionParam, CB_SETCURSEL, -1, NULL);
        SendMessage(hActionExtraParam, CB_SETCURSEL, -1, NULL);
        SendMessage(hDescription, WM_SETTEXT, 0, (LPARAM)"");
        return;
    }

    int idx = SendMessage(hActionsListBox, LB_GETCURSEL, 0, NULL);
    if (idx < 0)
        idx = 0;
    if (idx > 49)
        idx = 49;

    UpdateActionAndParam(-1, idx);
}

void CNewScript::OnSelchangeActionExtraParam(bool edited)
{
    if (SelectedScriptIndex < 0 || SendMessage(hActionsListBox, LB_GETCURSEL, NULL, NULL) < 0)
        return;
    int idx = SendMessage(hActionsListBox, LB_GETCURSEL, 0, NULL);
    FString key;
    key.Format("%d", idx);
    auto value = map.GetString(CurrentScriptID, key);
    auto atoms = FString::SplitString(value, 1);
    if (!ActionHasExtraParam[atoms[0]])
        return;

    int curSel = SendMessage(hActionExtraParam, CB_GETCURSEL, NULL, NULL);

    FString text;
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hActionExtraParam, CB_GETCOUNT, NULL, NULL) > 0 || !ActionExtraParamLabels.empty())
        && CNewScript::ParamAutodrop[1])
    {
        ExtraWindow::OnEditCComboBox(hActionExtraParam, ActionExtraParamLabels);
    }

    if (curSel >= 0 && curSel < SendMessage(hActionExtraParam, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hActionExtraParam, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hActionExtraParam, buffer, 511);
        text = buffer;
    }

    if (!text)
        return;

    FString::TrimIndex(text);
    if (text == "")
        text = "0";

    text.Replace(",", "");

    int extraParam = atoi(text);

    int ActionParam = atoi(atoms[1]);
    int low = LOWORD(ActionParam);
    WORD high = extraParam;
    int newParam = MAKELONG(low, high);
    value.Format("%s,%d", atoms[0], newParam);
    map.WriteString(CurrentScriptID, key, value);
    text.Format("[%s] : %s - (%d, %d)", key, atoms[0], low, high);

    FString actionName = FString::SplitString(fadata.GetString("ScriptsRA2", atoms[0], atoms[0] + " - MISSING,0,1,0,MISSING"))[0];
    FString::TrimIndexElse(actionName);
    FString::TrimIndexElse(actionName);

    FString tmp = text;
    text.Format("%s\t%s", tmp, actionName);

    SendMessage(hActionsListBox, LB_DELETESTRING, idx, NULL);
    SendMessage(hActionsListBox, LB_INSERTSTRING, idx, (LPARAM)(LPCSTR)text);
    SendMessage(hActionsListBox, LB_SETCURSEL, idx, NULL);

}

void CNewScript::OnCloseupActionExtraParam()
{
    if (!ExtraWindow::OnCloseupCComboBox(hActionExtraParam, ActionExtraParamLabels))
    {
        OnSelchangeActionListbox();
    }
}

void CNewScript::OnSelchangeActionParam(bool edited)
{
    if (SelectedScriptIndex < 0 || SendMessage(hActionsListBox, LB_GETCURSEL, NULL, NULL) < 0)
        return;
    int curSel = SendMessage(hActionParam, CB_GETCURSEL, NULL, NULL);

    FString text;
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hActionParam, CB_GETCOUNT, NULL, NULL) > 0 || !ActionParamLabels.empty())
        && CNewScript::ParamAutodrop[0])
    {
        ExtraWindow::OnEditCComboBox(hActionParam, ActionParamLabels);
    }

    if (curSel >= 0 && curSel < SendMessage(hActionParam, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hActionParam, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hActionParam, buffer, 511);
        text = buffer;
    }

    if (!text)
        return;

    FString::TrimIndex(text);
    if (text == "")
        text = "0";

    text.Replace(",", "");

    int idx = SendMessage(hActionsListBox, LB_GETCURSEL, 0, NULL);
    FString key;
    key.Format("%d", idx);
    auto value = map.GetString(CurrentScriptID, key);
    auto atoms = FString::SplitString(value, 1);
    int param = atoi(text);
    if (ActionHasExtraParam[atoms[0]])
    {
        int actionParam = atoi(atoms[1]);
        WORD low = param;
        int high = HIWORD(actionParam);
        int newParam = MAKELONG(low, high);
        value.Format("%s,%d", atoms[0], newParam);
        map.WriteString(CurrentScriptID, key, value);
        text.Format("[%s] : %s - (%d, %d)", key, atoms[0], low, high);
    }
    else
    {
        value.Format("%s,%d", atoms[0], param);
        map.WriteString(CurrentScriptID, key, value);
        text.Format("[%s] : %s - %d", key, atoms[0], param);
    }
    FString actionName = FString::SplitString(fadata.GetString("ScriptsRA2", atoms[0], atoms[0] + " - MISSING,0,1,0,MISSING"))[0];
    FString::TrimIndexElse(actionName);
    FString::TrimIndexElse(actionName);
    FString tmp = text;
    text.Format("%s\t%s", tmp, actionName);

    SendMessage(hActionsListBox, LB_DELETESTRING, idx, NULL);
    SendMessage(hActionsListBox, LB_INSERTSTRING, idx, (LPARAM)(LPCSTR)text);
    SendMessage(hActionsListBox, LB_SETCURSEL, idx, NULL);

}

void CNewScript::OnCloseupActionParam()
{
    if (!ExtraWindow::OnCloseupCComboBox(hActionParam, ActionParamLabels))
    {
        OnSelchangeActionListbox();
    }
}

void CNewScript::OnSelchangeActionType(bool edited)
{
    if (SelectedScriptIndex < 0 || SendMessage(hActionsListBox, LB_GETCURSEL, NULL, NULL) < 0)
        return;
    int curSel = SendMessage(hActionType, CB_GETCURSEL, NULL, NULL);

    FString text;
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hActionType, CB_GETCOUNT, NULL, NULL) > 0 || !ActionTypeLabels.empty()))
    {
        ExtraWindow::OnEditCComboBox(hActionType, ActionTypeLabels);
    }

    if (curSel >= 0 && curSel < SendMessage(hActionType, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hActionType, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hActionType, buffer, 511);
        text = buffer;
    }

    if (!text)
        return;

    FString::TrimIndex(text);
    if (text == "")
        text = "0";

    text.Replace(",", "");

    int idx = SendMessage(hActionsListBox, LB_GETCURSEL, 0, NULL);
    FString key;
    key.Format("%d", idx);
    int actionIdx = atoi(text);
    auto value = map.GetString(CurrentScriptID, key);
    auto atoms = FString::SplitString(value, 1);
    value.Format("%s,%s", text, atoms[1]);

    map.WriteString(CurrentScriptID, key, value);

    value = map.GetString(CurrentScriptID, key);
    atoms = FString::SplitString(value, 1);
    if (ActionHasExtraParam[atoms[0]])
    {
        int actionParam = atoi(atoms[1]);
        int low = LOWORD(actionParam);
        int high = HIWORD(actionParam);
        text.Format("[%s] : %s - (%d, %d)", key, atoms[0], low, high);
    }
    else
    {
        text.Format("[%s] : %s - %s", key, atoms[0], atoms[1]);
    }
    FString actionName = FString::SplitString(fadata.GetString("ScriptsRA2", atoms[0], atoms[0] + " - MISSING,0,1,0,MISSING"))[0];
    FString::TrimIndexElse(actionName);
    FString::TrimIndexElse(actionName);
    FString tmp = text;
    text.Format("%s\t%s", tmp, actionName);

    SendMessage(hActionsListBox, LB_DELETESTRING, idx, NULL);
    SendMessage(hActionsListBox, LB_INSERTSTRING, idx, (LPARAM)(LPCSTR)text);
    SendMessage(hActionsListBox, LB_SETCURSEL, idx, NULL);

    UpdateActionAndParam(actionIdx, -1, false);
}

void CNewScript::OnCloseupActionType()
{
    if (!ExtraWindow::OnCloseupCComboBox(hActionType, ActionTypeLabels))
    {
        OnSelchangeActionListbox();
    }
}

void CNewScript::OnSeldropdownScript(HWND& hWnd)
{
    if (Autodrop)
    {
        Autodrop = false;
        return;
    } 
    if (!DropNeedUpdate)
        return;

    DropNeedUpdate = false;

    ExtraWindow::SortTeams(hSelectedScript, "ScriptTypes", SelectedScriptIndex, CurrentScriptID);

}

void CNewScript::OnSelchangeScript(bool edited, int specificIdx)
{
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hSelectedScript, CB_GETCOUNT, NULL, NULL) > 0 || !ScriptLabels.empty()))
    {
        Autodrop = true;
        ExtraWindow::OnEditCComboBox(hSelectedScript, ScriptLabels);
        return;
    }

    SelectedScriptIndex = SendMessage(hSelectedScript, CB_GETCURSEL, NULL, NULL);
    if (SelectedScriptIndex < 0 || SelectedScriptIndex >= SendMessage(hSelectedScript, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hActionType, CB_SETCURSEL, -1, NULL);
        SendMessage(hActionParam, CB_SETCURSEL, -1, NULL);
        SendMessage(hActionExtraParam, CB_SETCURSEL, -1, NULL);
        SendMessage(hDescription, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)"");
        while (SendMessage(hActionsListBox, LB_DELETESTRING, 0, NULL) != CB_ERR);
        return;
    }

    FString pID;
    SendMessage(hSelectedScript, CB_GETLBTEXT, SelectedScriptIndex, (LPARAM)buffer);
    pID = buffer;
    FString::TrimIndex(pID);

    CurrentScriptID = pID;
    while (SendMessage(hActionsListBox, LB_DELETESTRING, 0, NULL) != CB_ERR);
    if (auto pScript = map.GetSection(pID))
    {
        auto name = map.GetString(pID, "Name");
        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)name.m_pchData);

        std::vector<FString> sortedList;
        for (int i = 0; i < 50; i++)
        {
            FString key;
            key.Format("%d", i);
            auto value = map.GetString(pID, key);
            if (value != "")
            {
                if (FString::SplitString(value).size() == 2)
                    sortedList.push_back(value);
            }
            map.DeleteKey(pID, key);
        }
        int i = 0;
        for (auto& value : sortedList)
        {
            auto atoms = FString::SplitString(value, 1);
            FString text;
            FString key;
            key.Format("%d", i);
            if (ActionHasExtraParam[atoms[0]])
            {
                int param = atoi(atoms[1]);
                int low = LOWORD(param);
                int high = HIWORD(param);
                text.Format("[%s] : %s - (%d, %d)", key, atoms[0], low, high);
            }
            else
                text.Format("[%s] : %s - %s", key, atoms[0], atoms[1]);

            FString actionName = FString::SplitString(fadata.GetString("ScriptsRA2", atoms[0], atoms[0] + " - MISSING,0,1,0,MISSING"))[0];
            FString::TrimIndexElse(actionName);
            FString::TrimIndexElse(actionName);
            FString tmp = text;
            text.Format("%s\t%s", tmp, actionName);
            
            SendMessage(hActionsListBox, LB_ADDSTRING, 0, (LPARAM)(LPCSTR)text);
            map.WriteString(pID, key, value);
            i++;
        }
    }
    if (specificIdx > -1)
    {
        while (specificIdx >= SendMessage(hActionsListBox, LB_GETCOUNT, NULL, NULL))
            specificIdx--;
        SendMessage(hActionsListBox, LB_SETCURSEL, specificIdx, NULL);
    }
    else
        if (SendMessage(hActionsListBox, LB_GETCOUNT, NULL, NULL) > 0)
            SendMessage(hActionsListBox, LB_SETCURSEL, 0, NULL);

    OnSelchangeActionListbox();
    DropNeedUpdate = false;
}

void CNewScript::OnCloseupScript()
{
    if (!ExtraWindow::OnCloseupCComboBox(hSelectedScript, ScriptLabels, true))
    {
        OnSelchangeScript();
    }
}

void CNewScript::OnClickNewScript()
{
    FString key = CINI::GetAvailableKey("ScriptTypes");
    FString value = CMapDataExt::GetAvailableIndex();
    FString buffer2;

    FString newName = "";
    if (ScriptSort::CreateFromScriptSort)
        newName = ScriptSort::Instance.GetCurrentPrefix();
    newName += "New script";
    map.WriteString("ScriptTypes", key, value);
    map.WriteString(value, "Name", newName);

    ExtraWindow::SortTeams(hSelectedScript, "ScriptTypes", SelectedScriptIndex, value);

    OnSelchangeScript();
}

void CNewScript::OnClickDelScript(HWND& hWnd)
{
    if (SelectedScriptIndex < 0)
        return;
    int result = MessageBox(hWnd,
        Translations::TranslateOrDefault("ScriptDelWarn", "Are you sure to delete this ScriptType? Don't forget to delete any references to this ScriptType"),
        Translations::TranslateOrDefault("ScriptDelTitle", "Delete ScriptType"), MB_YESNO);

    if (result == IDNO)
        return;

    map.DeleteSection(CurrentScriptID);
    std::vector<FString> deteleKeys;
    if (auto pSection = map.GetSection("ScriptTypes"))
    {
        for (auto& pair : pSection->GetEntities())
        {
            if (CurrentScriptID == pair.second)
                deteleKeys.push_back(pair.first);
        }
    }
    for (auto& key : deteleKeys)
        map.DeleteKey("ScriptTypes", key);

    int idx = SelectedScriptIndex;
    SendMessage(hSelectedScript, CB_DELETESTRING, idx, NULL);
    if (idx >= SendMessage(hSelectedScript, CB_GETCOUNT, NULL, NULL))
        idx--;
    if (idx < 0)
        idx = 0;
    SendMessage(hSelectedScript, CB_SETCURSEL, idx, NULL);
    OnSelchangeScript();
}

void CNewScript::OnClickCloScript(HWND& hWnd)
{
    if (SelectedScriptIndex < 0)
        return;
    if (SendMessage(hSelectedScript, CB_GETCOUNT, NULL, NULL) > 0 && SelectedScriptIndex >= 0)
    {
        FString key = CINI::GetAvailableKey("ScriptTypes");
        FString value = CMapDataExt::GetAvailableIndex();

        CINI::CurrentDocument->WriteString("ScriptTypes", key, value);

        auto oldname = CINI::CurrentDocument->GetString(CurrentScriptID, "Name", "New script");
        FString newName = ExtraWindow::GetCloneName(oldname);

        CINI::CurrentDocument->WriteString(value, "Name", newName);

        auto copyitem = [&value](FString key)
            {
                if (auto ppStr = map.TryGetString(CurrentScriptID, key)) {
                    FString str = *ppStr;
                    str.Trim();
                    map.WriteString(value, key, str);
                }
            };
        FString buffer;
        for (int i = 0; i < 50; i++)
        {
            buffer.Format("%d", i);
            copyitem(buffer);
        }

        ExtraWindow::SortTeams(hSelectedScript, "ScriptTypes", SelectedScriptIndex, value);

        OnSelchangeScript();
    }
}

void CNewScript::OnClickAddAction(HWND& hWnd)
{
    if (SelectedScriptIndex < 0)
        return;
    int count = SendMessage(hActionsListBox, LB_GETCOUNT, 0, NULL);
    if (count >= 50)
        return;

    char buffer[512]{ 0 };
    FString text;
    FString keyThis;
    if (bInsert)
    {
        int idx = SendMessage(hActionsListBox, LB_GETCURSEL, 0, NULL);
        if (idx >= 0)
        {
            std::vector<FString> sortedList;
            for (int i = 0; i < 50; i++)
            {
                FString key;
                key.Format("%d", i);
                auto value = map.GetString(CurrentScriptID, key);
                if (value != "")
                {
                    if (FString::SplitString(value).size() == 2)
                        sortedList.push_back(value);
                }
                map.DeleteKey(CurrentScriptID, key);
            }
            if (idx < sortedList.size())
            {
                auto it = sortedList.begin() + idx;
                sortedList.insert(it, "0,0");
                int i = 0;
                for (auto& value : sortedList)
                {
                    auto atoms = FString::SplitString(value, 1);
                    FString text;
                    FString key;
                    key.Format("%d", i);
                    map.WriteString(CurrentScriptID, key, value);
                    i++;
                }

                OnSelchangeScript(false, idx);
            }
        }    
    }
    else
    {
        keyThis.Format("%d", count);
        map.WriteString(CurrentScriptID, keyThis, "0,0");

        text.Format("[%s] : %s - %s", keyThis, "0", "0");
        FString actionName = FString::SplitString(fadata.GetString("ScriptsRA2", "0"))[0];
        FString::TrimIndexElse(actionName);
        FString::TrimIndexElse(actionName);
        FString tmp = text;
        text.Format("%s\t%s", tmp, actionName);

        SendMessage(hActionsListBox, LB_INSERTSTRING, count, (LPARAM)(LPCSTR)text);
        SendMessage(hActionsListBox, LB_SETCURSEL, count, NULL);
    }
    OnSelchangeActionListbox();
}

void CNewScript::OnClickCloneAction(HWND& hWnd)
{
    if (SelectedScriptIndex < 0 || SendMessage(hActionsListBox, LB_GETCURSEL, NULL, NULL) < 0)
        return;
    int count = SendMessage(hActionsListBox, LB_GETCOUNT, 0, NULL);
    if (count >= 50)
        return;

    int idx = SendMessage(hActionsListBox, LB_GETCURSEL, 0, NULL);
    if (idx < 0) return;

    char buffer[512]{ 0 };
    FString text;
    FString key;
    FString key2;
    key.Format("%d", count);
    key2.Format("%d", idx);

    if (bInsert)
    {
        auto copied = map.GetString(CurrentScriptID, key2, "0,0");
        std::vector<FString> sortedList;
        for (int i = 0; i < 50; i++)
        {
            FString key;
            key.Format("%d", i);
            auto value = map.GetString(CurrentScriptID, key);
            if (value != "")
            {
                if (FString::SplitString(value).size() == 2)
                    sortedList.push_back(value);
            }
            map.DeleteKey(CurrentScriptID, key);
        }           
        if (idx < sortedList.size())
        {
            auto it = sortedList.begin() + idx;
            sortedList.insert(it, copied);
            int i = 0;
            for (auto& value : sortedList)
            {
                auto atoms = FString::SplitString(value, 1);
                FString text;
                FString key;
                key.Format("%d", i);
                map.WriteString(CurrentScriptID, key, value);
                i++;
            }

            OnSelchangeScript(false, idx);
        }
    }
    else
    {
        map.WriteString(CurrentScriptID, key, map.GetString(CurrentScriptID, key2, "0,0"));

        SendMessage(hActionsListBox, LB_GETTEXT, idx, (LPARAM)buffer);
        auto atoms = FString::SplitString(buffer, " ");
        bool first = true;
        for (auto& atom : atoms)
        {
            if (first)
            {
                text.Format("[%s]", key);
                first = false;
            }
            else
                text += " " + atom;
        }

        SendMessage(hActionsListBox, LB_INSERTSTRING, count, (LPARAM)(LPCSTR)text);
        SendMessage(hActionsListBox, LB_SETCURSEL, count, NULL);
    }
    OnSelchangeActionListbox();
}

void CNewScript::OnClickDeleteAction(HWND& hWnd)
{
    if (SelectedScriptIndex < 0 || SendMessage(hActionsListBox, LB_GETCURSEL, NULL, NULL) < 0)
        return;
    int idx = SendMessage(hActionsListBox, LB_GETCURSEL, 0, NULL);
    SendMessage(hActionsListBox, LB_DELETESTRING, idx, NULL);
    FString key;
    key.Format("%d", idx);
    map.DeleteKey(CurrentScriptID, key);

    OnSelchangeScript(false, idx);
}

void CNewScript::OnClickMoveupAction(HWND& hWnd, bool reverse)
{
    if (SelectedScriptIndex < 0 || SendMessage(hActionsListBox, LB_GETCURSEL, NULL, NULL) < 0)
        return;
    int idx = SendMessage(hActionsListBox, LB_GETCURSEL, 0, NULL);    
    int count = SendMessage(hActionsListBox, LB_GETCOUNT, 0, NULL);
    
    if (idx <= 0 && !reverse)
        return;
    if (idx >= count - 1 && reverse)
        return;

    char buffer[512]{ 0 };
    FString text;
    FString text2;

    int idx2 = idx - 1;
    if (reverse)
        idx2 = idx + 1;

    FString key;
    key.Format("%d", idx);
    auto value = map.GetString(CurrentScriptID, key);
    FString key2;
    key2.Format("%d", idx2);
    auto value2 = map.GetString(CurrentScriptID, key2);
    map.WriteString(CurrentScriptID, key, value2);
    map.WriteString(CurrentScriptID, key2, value);

    SendMessage(hActionsListBox, LB_GETTEXT, idx, (LPARAM)buffer);
    auto atoms = FString::SplitString(buffer, " ");
    bool first = true;
    for (auto& atom : atoms)
    {
        if (first)
        {
            text.Format("[%s]", key2);
            first = false;
        }
        else
            text += " " + atom;
    }
    SendMessage(hActionsListBox, LB_DELETESTRING, idx2, NULL);
    SendMessage(hActionsListBox, LB_INSERTSTRING, idx2, (LPARAM)(LPCSTR)text);

    SendMessage(hActionsListBox, LB_GETTEXT, idx2, (LPARAM)buffer);
    atoms = FString::SplitString(buffer, " ");
    first = true;
    for (auto& atom : atoms)
    {
        if (first)
        {
            text.Format("[%s]", key);
            first = false;
        }
        else
            text += " " + atom;
    }
    SendMessage(hActionsListBox, LB_DELETESTRING, idx, NULL);
    SendMessage(hActionsListBox, LB_INSERTSTRING, idx, (LPARAM)(LPCSTR)text);

    OnSelchangeScript(false, idx2);
}

void CNewScript::UpdateActionAndParam(int actionChanged, int listBoxCurChanged, bool changeActionIdx)
{
    if (listBoxCurChanged < 0)
    {
        listBoxCurChanged = SendMessage(hActionsListBox, LB_GETCURSEL, 0, NULL);
    }
    if (listBoxCurChanged < 0)
        listBoxCurChanged = 0;
    if (listBoxCurChanged > 49)
        listBoxCurChanged = 49;

    FString key;
    FString buffer;
    key.Format("%d", listBoxCurChanged);
    auto value = map.GetString(CurrentScriptID, key);
    auto atoms = FString::SplitString(value, 1);
    if (auto pSection = fadata.GetSection("ScriptsRA2"))
    {
        FString action;
        action.Format("%d", actionChanged);
        if (actionChanged < 0)
        {
            action = atoms[0];
        }
        auto atoms2 = FString::SplitString(fadata.GetString("ScriptsRA2", action, action + " - MISSING,0,1,0,MISSING"), 4);
        FString name = atoms2[0];
        auto& paramIdx = atoms2[1];
        auto& disable = atoms2[2];
        auto& hasParam = atoms2[3];
        auto& description = atoms2[4];
        if (changeActionIdx)
        {
            int ActionIdx = SendMessage(hActionType, CB_FINDSTRINGEXACT, 0, (LPARAM)name);
            if (ActionIdx != CB_ERR)
            {
                SendMessage(hActionType, CB_SETCURSEL, ActionIdx, NULL);
            }
            else
            {
                SendMessage(hActionType, CB_SETCURSEL, -1, NULL);
                SendMessage(hActionType, WM_SETTEXT, 0, (LPARAM)name);
            }
        }

        SendMessage(hDescription, WM_SETTEXT, 0, (LPARAM)FString::ReplaceSpeicalString(description));
        FString::TrimIndex(name);
        if (hasParam == "1")
        {
            EnableWindow(hActionParam, TRUE);
            CNewScript::ParamAutodrop[0] = true;
            CNewScript::ParamAutodrop[1] = true;
            if (auto pSectionParam = fadata.GetSection("ScriptParams"))
            {
                auto param = FString::SplitString(fadata.GetString("ScriptParams", paramIdx));
                if (param.size() >= 2)
                {
                    SendMessage(hActionParamDes, WM_SETTEXT, 0, (LPARAM)param[0]);
                    ExtraWindow::LoadParams(hActionParam, param[1]); 
                    if (!ExtConfigs::SearchCombobox_Waypoint && param[1] == "1") // waypoints
                    {
                        CNewScript::ParamAutodrop[0] = false;
                    }
                }
                if (ActionHasExtraParam[name])
                {
                    EnableWindow(hActionExtraParam, TRUE);
                    SendMessage(hActionExtraParamDes, WM_SETTEXT, 0, (LPARAM)param[2]);
                    ExtraWindow::LoadParams(hActionExtraParam, param[3]);
                    if (!ExtConfigs::SearchCombobox_Waypoint && param[3] == "1") // waypoints
                    {
                        CNewScript::ParamAutodrop[1] = false;
                    }
                    int actionParam = atoi(atoms[1]);
                    int low = LOWORD(actionParam);
                    int high = HIWORD(actionParam);

                    buffer.Format("%d - ", low);
                    int idx = SendMessage(hActionParam, CB_FINDSTRING, 0, (LPARAM)buffer);
                    if (idx == CB_ERR)
                    {
                        buffer.Format("%d", low);
                        SendMessage(hActionParam, WM_SETTEXT, 0, (LPARAM)buffer);
                    }
                    else
                        SendMessage(hActionParam, CB_SETCURSEL, idx, NULL);
                    buffer.Format("%d - ", high);
                    idx = SendMessage(hActionParam, CB_FINDSTRING, 0, (LPARAM)buffer);
                    if (idx == CB_ERR)
                    {
                        buffer.Format("%d", high);
                        SendMessage(hActionExtraParam, WM_SETTEXT, 0, (LPARAM)buffer);
                    }
                    else
                        SendMessage(hActionExtraParam, CB_SETCURSEL, idx, NULL);
                }
                else
                {
                    Translations::GetTranslationItem("ScriptTypesExtraParam", buffer);
                    SendMessage(hActionExtraParamDes, WM_SETTEXT, 0, (LPARAM)buffer);
                    EnableWindow(hActionExtraParam, FALSE);
                    SendMessage(hActionExtraParam, WM_SETTEXT, 0, (LPARAM)"");
                    auto& actionParam = atoms[1];
                    FString buffer;
                    buffer.Format("%s - ", actionParam);
                    int idx = SendMessage(hActionParam, CB_FINDSTRING, 0, (LPARAM)buffer);
                    if (idx == CB_ERR)
                    {
                        SendMessage(hActionParam, WM_SETTEXT, 0, (LPARAM)actionParam);
                    }
                    else
                        SendMessage(hActionParam, CB_SETCURSEL, idx, NULL);
                }
            }
        }
        else
        {
            EnableWindow(hActionParam, FALSE);
            EnableWindow(hActionExtraParam, FALSE);
            SendMessage(hActionExtraParam, WM_SETTEXT, 0, (LPARAM)"");
            SendMessage(hActionParam, WM_SETTEXT, 0, (LPARAM)"");
            Translations::GetTranslationItem("ScriptTypesActionParam", buffer);
            SendMessage(hActionParamDes, WM_SETTEXT, 0, (LPARAM)buffer);
            Translations::GetTranslationItem("ScriptTypesExtraParam", buffer);
            SendMessage(hActionExtraParamDes, WM_SETTEXT, 0, (LPARAM)buffer);
        }

    }

}

void CNewScript::OnClickSearchReference(HWND& hWnd)
{
    if (SelectedScriptIndex < 0)
        return;

    CSearhReference::SetSearchType(2);
    CSearhReference::SetSearchID(CurrentScriptID);
    if (CSearhReference::GetHandle() == NULL)
    {
        CSearhReference::Create(m_parent);
    }
    else
    {
        ::SendMessage(CSearhReference::GetHandle(), 114514, 0, 0);
    }

}

bool CNewScript::OnEnterKeyDown(HWND& hWnd)
{
    if (hWnd == hSelectedScript)
        OnSelchangeScript(true);
    else if (hWnd == hActionExtraParam)
        OnSelchangeActionExtraParam(true);
    else if (hWnd == hActionParam)
        OnSelchangeActionParam(true);
    else if (hWnd == hActionType)
        OnSelchangeActionType(true);
    else
        return false;
    return true;
}