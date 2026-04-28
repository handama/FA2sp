#include "CNewScript.h"
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
#include <Miscs/Miscs.h>
#include "../CObjectSearch/CObjectSearch.h"
#include "../../Ext/CTileSetBrowserFrame/TabPages/ScriptSort.h"
#include "../CNewTrigger/CNewTrigger.h"
#include "../CSearhReference/CSearhReference.h"
#include <numeric>
#include "../CTriggerAnnotation/CTriggerAnnotation.h"
#include "../CNewTeamTypes/CNewTeamTypes.h"

HWND CNewScript::m_hwnd;
CFinalSunDlg* CNewScript::m_parent;
CINI& CNewScript::map = CINI::CurrentDocument;
CINI& CNewScript::fadata = CINI::FAData;
MultimapHelper& CNewScript::rules = Variables::RulesMap;

VirtualComboBoxEx CNewScript::vcbSelectedScript;
VirtualComboBoxEx CNewScript::vcbActionType;
VirtualComboBoxEx CNewScript::vcbActionParam;
VirtualComboBoxEx CNewScript::vcbActionExtraParam;

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
HWND CNewScript::hRenderPath;
HWND CNewScript::hSearchReference;

int CNewScript::SelectedScriptIndex = -1;
FString CNewScript::CurrentScriptID;
FMap<bool> CNewScript::ActionHasExtraParam;
FMap<bool> CNewScript::ActionIsStringParam;
bool CNewScript::ParamAutodrop[2];
bool CNewScript::bInsert;
WNDPROC CNewScript::OriginalListBoxProc;
HWND CNewScript::hDragPoint;
WNDPROC CNewScript::OrigDragDotProc;
WNDPROC CNewScript::OrigDragingDotProc;
bool CNewScript::m_dragging = false;
POINT CNewScript::m_dragOffset{};
HWND CNewScript::m_hDragGhost = nullptr;
TargetHighlighter CNewScript::hl;

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
    Translate(6307, "ScriptTypesRenderPath");
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
    hRenderPath = GetDlgItem(hWnd, Controls::RenderPath);
    hSearchReference = GetDlgItem(hWnd, Controls::SearchReference);
    hDragPoint = GetDlgItem(hWnd, Controls::DragPoint);
    bInsert = false;
    CIsoViewExt::DrawScriptPath = false;

    ExtraWindow::SetEditControlFontSize(hDescription, 1.3f);
    int tabstops[2] = { 80, 100 };
    SendMessage(hActionsListBox, LB_SETTABSTOPS, 2, (LPARAM)&tabstops);

    if (hActionsListBox)
        OriginalListBoxProc = (WNDPROC)SetWindowLongPtr(hActionsListBox, GWLP_WNDPROC, (LONG_PTR)ListBoxSubclassProc);

    if (hDragPoint)
    {
        OrigDragDotProc = (WNDPROC)SetWindowLongPtr(hDragPoint, GWLP_WNDPROC, (LONG_PTR)DragDotProc);
    }

    vcbSelectedScript.Attach(hSelectedScript, &ExtConfigs::SortByLabelName_Script, false);
    vcbActionType.Attach(hActionType);
    vcbActionType.SetAutoSearchRestriction(&ExtConfigs::SearchCombobox_AllowNonParams);
    vcbActionParam.Attach(hActionParam);
    vcbActionParam.SetAutoSearchRestriction(&ParamAutodrop[0]);
    vcbActionExtraParam.Attach(hActionExtraParam);
    vcbActionExtraParam.SetAutoSearchRestriction(&ParamAutodrop[1]);

    hl.SetBorderColor(ExtConfigs::EnableDarkMode ? RGB(0, 90, 0) : RGB(0, 180, 0));
    hl.SetBorderThickness(3);
    hl.SetBorderRadius(0);

    Update(hWnd);
}

void CNewScript::Update(HWND& hWnd)
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    if (ScriptSort::Instance.IsVisible())
        ScriptSort::Instance.LoadAllTriggers();

    int idx = 0;
    
    ExtraWindow::SortTeams(vcbSelectedScript, "ScriptTypes", SelectedScriptIndex);

    int count = SendMessage(hSelectedScript, CB_GETCOUNT, NULL, NULL);
    if (SelectedScriptIndex < 0)
        SelectedScriptIndex = 0;
    if (SelectedScriptIndex > count - 1)
        SelectedScriptIndex = count - 1;
    SendMessage(hSelectedScript, CB_SETCURSEL, SelectedScriptIndex, NULL);
    
    idx = 0;
    ExtraWindow::ClearComboKeepText(hActionType);
    if (auto pSection = fadata.GetSection(ExtraWindow::GetTranslatedSectionName("ScriptsRA2")))
    {
        ComboBoxBatchUpdater cbb(hActionType, pSection->GetEntities().size(), false, 256, false);
        for (auto& pair : pSection->GetEntities())
        {
            auto atoms = FString::SplitString(pair.second, 4);
            if (atoms[2] == "0")
                SendMessage(hActionType, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)FString::ReplaceSpeicalString(atoms[0]));
        }
    }

    ActionIsStringParam.clear();
    if (auto pSection = fadata.GetSection("StringScripts"))
    {
        for (auto& pair : pSection->GetEntities())
        {
            ActionIsStringParam[pair.second] = true;
        }
    }

    if (auto pSection = fadata.GetSection(ExtraWindow::GetTranslatedSectionName("ScriptsRA2")))
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
                if (auto pSectionParam = fadata.GetSection(ExtraWindow::GetTranslatedSectionName("ScriptParams")))
                {
                    auto param = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("ScriptParams"), paramIdx));
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

    OnSelchangeScript();
}

void CNewScript::Close(HWND& hWnd)
{
    if (CIsoViewExt::DrawScriptPath)
    {
        CIsoViewExt::DrawScriptPath = false;
        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->GetSafeHwnd(), 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
    }

    EndDialog(hWnd, NULL);

    CNewScript::m_hwnd = NULL;
    CNewScript::m_parent = NULL;
}

LRESULT CALLBACK CNewScript::DragDotProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SETCURSOR:
    {
        if (CurrentScriptID != "")
        {
            SetCursor(LoadCursor(nullptr, IDC_HAND));
            return TRUE;
        }
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        HBRUSH hBrush = CreateSolidBrush(CurrentScriptID != "" ? RGB(0, 200, 0) : RGB(200, 0, 0));
        FillRect(hdc, &ps.rcPaint, hBrush);
        DeleteObject(hBrush);

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        if (CurrentScriptID != "")
        {
            m_dragging = true;
            SetCapture(hWnd);

            POINT pt;
            GetCursorPos(&pt);
            if (m_hDragGhost)
            {
                DestroyWindow(m_hDragGhost);
                m_hDragGhost = nullptr;
            }
            m_hDragGhost = CreateWindowEx(
                WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
                "STATIC",
                nullptr,
                WS_POPUP,
                pt.x - 6, pt.y - 6,
                12, 12,
                nullptr, nullptr,
                static_cast<HINSTANCE>(FA2sp::hInstance),
                nullptr
            );

            if (m_hDragGhost)
            {
                OrigDragingDotProc = (WNDPROC)SetWindowLongPtr(m_hDragGhost, GWLP_WNDPROC, (LONG_PTR)DragingDotProc);
                SetLayeredWindowAttributes(m_hDragGhost, 0, 200, LWA_ALPHA);
                ShowWindow(m_hDragGhost, SW_SHOW);
            }
            return 0;
        }
        break;
    }
    case WM_MOUSEMOVE:
    {
        if (!m_dragging) break;

        POINT pt;
        GetCursorPos(&pt);

        SetWindowPos(
            m_hDragGhost,
            nullptr,
            pt.x - 6, pt.y - 6,
            0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
        );

        auto target = ExtraWindow::FindDropTarget(pt);
        if (target.hWnd && IsWindowEnabled(target.hWnd) && target.type == DropType::TeamEditorScript
            )
        {
            if (!hl.IsActive())
            {
                hl.Attach(target);
            }
            else if (!hl.IsSameTarget(target))
            {
                hl.Detach();
                hl.Attach(target);
            }
        }
        else if (hl.IsActive())
        {
            hl.Detach();
        }

        return 0;
    }
    case WM_CAPTURECHANGED:
    {
        if (m_dragging)
        {
            m_dragging = false;
            ReleaseCapture();

            SetWindowLongPtr(
                m_hDragGhost,
                GWLP_WNDPROC,
                (LONG_PTR)OrigDragingDotProc
            );
            SetWindowLongPtr(m_hDragGhost, GWLP_USERDATA, 0);

            DestroyWindow(m_hDragGhost);
            m_hDragGhost = nullptr;
            hl.Detach();
        }
        break;
    }
    case WM_LBUTTONUP:
    {
        if (!m_dragging) break;

        m_dragging = false;
        ReleaseCapture();

        POINT pt;
        GetCursorPos(&pt);

        SetWindowLongPtr(
            m_hDragGhost,
            GWLP_WNDPROC,
            (LONG_PTR)OrigDragingDotProc
        );
        SetWindowLongPtr(m_hDragGhost, GWLP_USERDATA, 0);

        DestroyWindow(m_hDragGhost);
        m_hDragGhost = nullptr;
        hl.Detach();

        auto target = ExtraWindow::FindDropTarget(pt);
        if (target.hWnd)
        {
            auto scriptID = CurrentScriptID + " ";
            switch (target.type)
            {
            case DropType::TeamEditorScript:
                if (CNewTeamTypes::CurrentTeamID != "")
                {
                    auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, scriptID);
                    if (idx == CB_ERR)
                    {
                        CNewTeamTypes::OnDropdownScript();
                        idx = ExtraWindow::FindCBStringExactStart(target.hWnd, scriptID);
                    }
                    if (idx != CB_ERR)
                    {
                        SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                        CNewTeamTypes::OnSelchangeScript();
                    }
                }
                break;
            case DropType::Unknown:
            default:
                break;
            }
        }

        ReleaseCapture();
        return 0;
    }
    }

    return DefWindowProc(
        hWnd, message, wParam, lParam
    );
}

LRESULT CALLBACK CNewScript::DragingDotProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        HBRUSH hBrush = CreateSolidBrush(RGB(0, 200, 0));
        FillRect(hdc, &ps.rcPaint, hBrush);
        DeleteObject(hBrush);

        EndPaint(hWnd, &ps);
        return 0;
    }
    }

    return DefWindowProc(
        hWnd, message, wParam, lParam
    );
}

LRESULT CALLBACK CNewScript::ListBoxSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_MOUSEWHEEL:
    {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);

        RECT rc;
        GetClientRect(hWnd, &rc);

        if (pt.x >= rc.right - GetSystemMetrics(SM_CXVSCROLL))
        {
            return CallWindowProc(OriginalListBoxProc, hWnd, message, wParam, lParam);
        }

        int delta = (short)HIWORD(wParam);

        WPARAM keyParam = 0;
        if (delta > 0)
            keyParam = VK_UP;
        else if (delta < 0)
            keyParam = VK_DOWN;

        if (keyParam != 0)
        {
            SendMessage(hWnd, WM_KEYDOWN, keyParam, 0x00000001);
            SendMessage(hWnd, WM_KEYUP, keyParam, 0xC0000001);
        }

        return TRUE;
    }
    case WM_LBUTTONDOWN:
    {
        POINT pt = { (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam) };
        int index = SendMessage(hWnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));

        if (HIWORD(index))
        {
            return 0;
        }
        break;
    }
    }
    return CallWindowProc(OriginalListBoxProc, hWnd, message, wParam, lParam);
}

BOOL CALLBACK CNewScript::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_ACTIVATE:
    {
        if (SelectedScriptIndex >= 0 && SelectedScriptIndex < SendMessage(hSelectedScript, CB_GETCOUNT, NULL, NULL))
        {
            CTriggerAnnotation::Type = AnnoScript;
            CTriggerAnnotation::ID = CurrentScriptID;
            ::SendMessage(CTriggerAnnotation::GetHandle(), 114515, 0, 0);
        }
        return TRUE;
    }
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
            break;
        case Controls::Name:
            if (CODE == EN_CHANGE)
            {
                if (SelectedScriptIndex < 0)
                    break;
                CNewTeamTypes::ScriptListChanged = true;
                char buffer[512]{ 0 };
                GetWindowText(hName, buffer, 511);
                map.WriteString(CurrentScriptID, "Name", buffer);

                FString name = ExtraWindow::FormatTriggerDisplayName(CurrentScriptID, buffer);

                vcbSelectedScript.ReplaceString(SelectedScriptIndex, name);
                vcbSelectedScript.SetCurSel(SelectedScriptIndex);
            }
            break;
        case Controls::ActionType:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeActionType();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeActionType(true);
            break;
        case Controls::ActionParam:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeActionParam();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeActionParam(true);
            break;
        case Controls::ActionExtraParam:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeActionExtraParam();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeActionExtraParam(true);
            break;
        case Controls::Insert:
            bInsert = SendMessage(hInsert, BM_GETCHECK, 0, 0);
            break;
        case Controls::RenderPath:
            CIsoViewExt::DrawScriptPath = SendMessage(hRenderPath, BM_GETCHECK, 0, 0);
            if (CIsoViewExt::DrawScriptPath)
                UpdateScriptPath();
            else
                ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->GetSafeHwnd(), 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);           
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
    case WM_MEASUREITEM:
    {
        VirtualComboBoxEx::SetWindowHeight(hWnd, lParam);
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

FString CNewScript::GetOneBasedIndex(const FString& key)
{
    int i = atoi(key) + 1;
    FString ret;
    ret.Format("%d", i);
    return ret;
}

void CNewScript::OnSelchangeActionListbox()
{
    if (SelectedScriptIndex < 0 || SendMessage(hActionsListBox, LB_GETCARETINDEX, NULL, NULL) < 0 || SendMessage(hActionsListBox, LB_GETCOUNT, NULL, NULL) <= 0)
    {
        SendMessage(hActionType, CB_SETCURSEL, -1, NULL);
        SendMessage(hActionParam, CB_SETCURSEL, -1, NULL);
        SendMessage(hActionExtraParam, CB_SETCURSEL, -1, NULL);
        SendMessage(hDescription, WM_SETTEXT, 0, (LPARAM)"");
        return;
    }

    int idx = SendMessage(hActionsListBox, LB_GETCARETINDEX, 0, NULL);
    if (idx < 0)
        idx = 0;
    if (idx > 49)
        idx = 49;

    UpdateActionAndParam(-1, idx);
}

void CNewScript::OnSelchangeActionExtraParam(bool edited)
{
    if (SelectedScriptIndex < 0 || SendMessage(hActionsListBox, LB_GETCARETINDEX, NULL, NULL) < 0)
        return;
    int idx = SendMessage(hActionsListBox, LB_GETCARETINDEX, 0, NULL);
    FString key;
    key.Format("%d", idx);
    auto value = map.GetString(CurrentScriptID, key);
    auto atoms = FString::SplitString(value, 1);
    if (!ActionHasExtraParam[atoms[0]])
        return;

    FString text = vcbActionExtraParam.GetSelectedText(edited);
    if (text.empty())
        return;

    if (ActionIsStringParam[atoms[0]])
        ExtraWindow::TrimStringIndex(text);
    else
        FString::TrimIndex(text);
    if (text == "")
        text = "0";

    text.Replace(",", "");

    int ActionParam = atoi(atoms[1]);

    if (ActionIsStringParam[atoms[0]])
    {
        auto atoms = FString::SplitString(value, 2);
        auto& param = atoms[1];
        auto extraParam = text;

        value.Format("%s,%s,%s", atoms[0], param, extraParam);
        map.WriteString(CurrentScriptID, key, value);
        text.Format("[%s] : %s - (%s, %s)", GetOneBasedIndex(key), atoms[0], param, extraParam);
    }
    else
    {
        int extraParam = atoi(text);

        int low = LOWORD(ActionParam);
        WORD high = extraParam;
        int newParam = MAKELONG(low, high);
        value.Format("%s,%d", atoms[0], newParam);
        map.WriteString(CurrentScriptID, key, value);
        text.Format("[%s] : %s - (%d, %d)", GetOneBasedIndex(key), atoms[0], low, high);
    }

    FString actionName = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("ScriptsRA2"), atoms[0], atoms[0] + " - MISSING,0,1,0,MISSING"))[0];
    FString::TrimIndexElse(actionName);
    FString::TrimIndexElse(actionName);

    FString tmp = text;
    text.Format("%s\t%s", tmp, actionName);

    SendMessage(hActionsListBox, LB_DELETESTRING, idx, NULL);
    SendMessage(hActionsListBox, LB_INSERTSTRING, idx, (LPARAM)(LPCSTR)text); 
    SetListBoxSel(idx);

    UpdateScriptPath();
}

void CNewScript::OnSelchangeActionParam(bool edited)
{
    if (SelectedScriptIndex < 0 || SendMessage(hActionsListBox, LB_GETCARETINDEX, NULL, NULL) < 0)
        return;

    FString text = vcbActionParam.GetSelectedText(edited);
    if (text.empty())
        return;

    int idx = SendMessage(hActionsListBox, LB_GETCARETINDEX, 0, NULL);
    FString key;
    key.Format("%d", idx);
    auto value = map.GetString(CurrentScriptID, key);
    auto atoms = FString::SplitString(value, 1);

    if (ActionIsStringParam[atoms[0]])
        ExtraWindow::TrimStringIndex(text);
    else
        FString::TrimIndex(text);
    if (text == "")
        text = "0";

    text.Replace(",", "");

    if (ActionIsStringParam[atoms[0]])
    {
        auto param = text;
        if (ActionHasExtraParam[atoms[0]])
        {
            auto atoms = FString::SplitString(value, 2);
            value.Format("%s,%s,%s", atoms[0], param, atoms[2]);
            map.WriteString(CurrentScriptID, key, value);
            text.Format("[%s] : %s - (%s, %s)", GetOneBasedIndex(key), atoms[0], param, atoms[2]);
        }
        else
        {
            value.Format("%s,%s", atoms[0], param);
            map.WriteString(CurrentScriptID, key, value);
            text.Format("[%s] : %s - %s", GetOneBasedIndex(key), atoms[0], param);
        }
    }
    else
    {
        int param = atoi(text);
        if (ActionHasExtraParam[atoms[0]])
        {
            int actionParam = atoi(atoms[1]);
            WORD low = param;
            int high = HIWORD(actionParam);
            int newParam = MAKELONG(low, high);
            value.Format("%s,%d", atoms[0], newParam);
            map.WriteString(CurrentScriptID, key, value);
            text.Format("[%s] : %s - (%d, %d)", GetOneBasedIndex(key), atoms[0], low, high);
        }
        else
        {
            value.Format("%s,%d", atoms[0], param);
            map.WriteString(CurrentScriptID, key, value);
            text.Format("[%s] : %s - %d", GetOneBasedIndex(key), atoms[0], param);
        }
    }

    FString actionName = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("ScriptsRA2"), atoms[0], atoms[0] + " - MISSING,0,1,0,MISSING"))[0];
    FString::TrimIndexElse(actionName);
    FString::TrimIndexElse(actionName);
    FString tmp = text;
    text.Format("%s\t%s", tmp, actionName);

    SendMessage(hActionsListBox, LB_DELETESTRING, idx, NULL);
    SendMessage(hActionsListBox, LB_INSERTSTRING, idx, (LPARAM)(LPCSTR)text);
    SetListBoxSel(idx);

    UpdateScriptPath();
}

void CNewScript::OnSelchangeActionType(bool edited)
{
    if (SelectedScriptIndex < 0 || SendMessage(hActionsListBox, LB_GETCARETINDEX, NULL, NULL) < 0)
        return;

    FString text = vcbActionType.GetSelectedText(edited);
    if (text.empty())
        return;

    FString::TrimIndex(text);
    if (text == "")
        text = "0";

    text.Replace(",", "");

    int idx = SendMessage(hActionsListBox, LB_GETCARETINDEX, 0, NULL);
    FString key;
    key.Format("%d", idx);
    int actionIdx = atoi(text);
    auto value = map.GetString(CurrentScriptID, key);
    auto atoms = FString::SplitString(value, 1);
    value.Format("%s,%s", text, atoms[1]);

    map.WriteString(CurrentScriptID, key, value);

    value = map.GetString(CurrentScriptID, key);
    atoms = FString::SplitString(value, 1);

    if (ActionIsStringParam[atoms[0]])
    {
        if (ActionHasExtraParam[atoms[0]])
        {
            auto atoms = FString::SplitString(value, 2);
            text.Format("[%s] : %s - (%s, %s)", GetOneBasedIndex(key), atoms[0], atoms[1], atoms[2]);
        }
        else
        {
            text.Format("[%s] : %s - %s", GetOneBasedIndex(key), atoms[0], atoms[1]);
        }
    }
    else
    {
        if (ActionHasExtraParam[atoms[0]])
        {
            int actionParam = atoi(atoms[1]);
            int low = LOWORD(actionParam);
            int high = HIWORD(actionParam);
            text.Format("[%s] : %s - (%d, %d)", GetOneBasedIndex(key), atoms[0], low, high);
        }
        else
        {
            text.Format("[%s] : %s - %s", GetOneBasedIndex(key), atoms[0], atoms[1]);
        }
    }

    FString actionName = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("ScriptsRA2"), atoms[0], atoms[0] + " - MISSING,0,1,0,MISSING"))[0];
    FString::TrimIndexElse(actionName);
    FString::TrimIndexElse(actionName);
    FString tmp = text;
    text.Format("%s\t%s", tmp, actionName);

    SendMessage(hActionsListBox, LB_DELETESTRING, idx, NULL);
    SendMessage(hActionsListBox, LB_INSERTSTRING, idx, (LPARAM)(LPCSTR)text);
    SetListBoxSel(idx);

    UpdateActionAndParam(actionIdx, -1, false);
    UpdateScriptPath();
}

void CNewScript::OnSelchangeScript(bool edited, int specificIdx)
{
    char buffer[512]{ 0 };

    auto clear = []()
    {
        SendMessage(hActionType, CB_SETCURSEL, -1, NULL);
        SendMessage(hActionParam, CB_SETCURSEL, -1, NULL);
        SendMessage(hActionExtraParam, CB_SETCURSEL, -1, NULL);
        SendMessage(hDescription, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)"");
        while (SendMessage(hActionsListBox, LB_DELETESTRING, 0, NULL) != CB_ERR);
    };

    SelectedScriptIndex = SendMessage(hSelectedScript, CB_GETCURSEL, NULL, NULL);
    if (SelectedScriptIndex < 0 || SelectedScriptIndex >= SendMessage(hSelectedScript, CB_GETCOUNT, NULL, NULL))
    {
        clear();
        CurrentScriptID = "";
        InvalidateRect(hDragPoint, nullptr, TRUE);
        return;
    }

    FString pID;
    SendMessage(hSelectedScript, CB_GETLBTEXT, SelectedScriptIndex, (LPARAM)buffer);
    pID = buffer;
    FString::TrimIndex(pID);

    CurrentScriptID = pID;
    InvalidateRect(hDragPoint, nullptr, TRUE);

    CTriggerAnnotation::Type = AnnoScript;
    CTriggerAnnotation::ID = CurrentScriptID;
    ::SendMessage(CTriggerAnnotation::GetHandle(), 114515, 0, 0);

    while (SendMessage(hActionsListBox, LB_DELETESTRING, 0, NULL) != CB_ERR);
    if (auto pScript = map.GetSection(pID))
    {
        auto name = map.GetString(pID, "Name");
        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)name.GetString());

        std::vector<FString> sortedList;
        for (int i = 0; i < 50; i++)
        {
            FString key;
            key.Format("%d", i);
            auto value = map.GetString(pID, key);
            if (value != "")
            {
                if (FString::SplitString(value).size() >= 2)
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

            if (ActionIsStringParam[atoms[0]])
            {
                if (ActionHasExtraParam[atoms[0]])
                {
                    auto atoms = FString::SplitString(value, 2);
                    text.Format("[%s] : %s - (%s, %s)", GetOneBasedIndex(key), atoms[0], atoms[1], atoms[2]);
                }
                else
                {
                    text.Format("[%s] : %s - %s", GetOneBasedIndex(key), atoms[0], atoms[1]);
                }
            }
            else
            {
                if (ActionHasExtraParam[atoms[0]])
                {
                    int param = atoi(atoms[1]);
                    int low = LOWORD(param);
                    int high = HIWORD(param);
                    text.Format("[%s] : %s - (%d, %d)", GetOneBasedIndex(key), atoms[0], low, high);
                }
                else
                {
                    text.Format("[%s] : %s - %s", GetOneBasedIndex(key), atoms[0], atoms[1]);
                }
            }


            FString actionName = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("ScriptsRA2"), atoms[0], atoms[0] + " - MISSING,0,1,0,MISSING"))[0];
            FString::TrimIndexElse(actionName);
            FString::TrimIndexElse(actionName);
            FString tmp = text;
            text.Format("%s\t%s", tmp, actionName);
            
            SendMessage(hActionsListBox, LB_ADDSTRING, 0, (LPARAM)(LPCSTR)text);
            map.WriteString(pID, key, value);
            i++;
        }
    }
    else
    {
        clear();
    }
    if (specificIdx > -1)
    {
        while (specificIdx >= SendMessage(hActionsListBox, LB_GETCOUNT, NULL, NULL))
            specificIdx--;
        SetListBoxSel(specificIdx);
    }
    else
        if (SendMessage(hActionsListBox, LB_GETCOUNT, NULL, NULL) > 0)
            SetListBoxSel(0);

    OnSelchangeActionListbox();

    UpdateScriptPath();
}

void CNewScript::OnClickNewScript()
{
    CNewTeamTypes::ScriptListChanged = true;
    FString key = CINI::GetAvailableKey("ScriptTypes");
    FString value = CMapDataExt::GetAvailableIndex(EIndexType::Script);
    FString buffer2;

    FString newName = "";
    if (ScriptSort::CreateFromScriptSort)
        newName = ScriptSort::Instance.GetCurrentPrefix();
    newName += "New script";
    map.WriteString("ScriptTypes", key, value);
    map.WriteString(value, "Name", newName);

    ExtraWindow::SortTeams(vcbSelectedScript, "ScriptTypes", SelectedScriptIndex, value);

    OnSelchangeScript();
}

void CNewScript::OnClickDelScript(HWND& hWnd)
{
    if (SelectedScriptIndex < 0)
        return;

    CNewTeamTypes::ScriptListChanged = true;
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
        FString value = CMapDataExt::GetAvailableIndex(EIndexType::Script);

        CINI::CurrentDocument->WriteString("ScriptTypes", key, value);

        auto oldname = CINI::CurrentDocument->GetString(CurrentScriptID, "Name", "New script");
        FString newName = ExtraWindow::GetCloneName(oldname);

        CINI::CurrentDocument->WriteString(value, "Name", newName);
        CNewTeamTypes::ScriptListChanged = true;

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

        ExtraWindow::SortTeams(vcbSelectedScript, "ScriptTypes", SelectedScriptIndex, value);

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
        int idx = SendMessage(hActionsListBox, LB_GETCARETINDEX, 0, NULL);
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

        text.Format("[%s] : %s - %s", GetOneBasedIndex(keyThis), "0", "0");
        FString actionName = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("ScriptsRA2"), "0"))[0];
        actionName = FString::ReplaceSpeicalString(actionName);
        FString::TrimIndexElse(actionName);
        FString::TrimIndexElse(actionName);
        FString tmp = text;
        text.Format("%s\t%s", tmp, actionName);

        SendMessage(hActionsListBox, LB_INSERTSTRING, count, (LPARAM)(LPCSTR)text);
        SetListBoxSel(count);
    }
    OnSelchangeActionListbox();
    UpdateScriptPath();
}

void CNewScript::OnClickCloneAction(HWND& hWnd)
{
    if (SelectedScriptIndex < 0 || SendMessage(hActionsListBox, LB_GETCARETINDEX, NULL, NULL) < 0)
        return;
    int count = SendMessage(hActionsListBox, LB_GETCOUNT, 0, NULL);
    if (count >= 50)
        return;

    char buffer[512]{ 0 };
    if (bInsert)
    {
        int idx = SendMessage(hActionsListBox, LB_GETCARETINDEX, 0, NULL);
        if (idx < 0) return;

        FString text;
        FString key2;
        key2.Format("%d", idx);

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
        std::vector<int> curSels;
        GetListBoxSels(curSels);
        FString value;
        FString key;
        FString key2;
        FString text;
        std::vector<int> addSels;
        for (int i = 0; i < curSels.size(); ++i)
        {
            if (count >= 50) break;
            key.Format("%d", curSels[i]);
            key2.Format("%d", count);
            value = map.GetString(CurrentScriptID, key, "0,0");
            map.WriteString(CurrentScriptID, key2, value);

            SendMessage(hActionsListBox, LB_GETTEXT, curSels[i], (LPARAM)buffer);
            auto atoms = FString::SplitString(buffer, " ");
            bool first = true;
            for (auto& atom : atoms)
            {
                if (first)
                {
                    text.Format("[%s]", GetOneBasedIndex(key2));
                    first = false;
                }
                else
                    text += " " + atom;
            }

            SendMessage(hActionsListBox, LB_INSERTSTRING, count, (LPARAM)(LPCSTR)text);
            addSels.push_back(count);
            count++;
        }
        SetListBoxSels(addSels);
    }
    OnSelchangeActionListbox();
    UpdateScriptPath();
}

void CNewScript::OnClickDeleteAction(HWND& hWnd)
{
    if (SelectedScriptIndex < 0 || SendMessage(hActionsListBox, LB_GETCARETINDEX, NULL, NULL) < 0)
        return;
    std::vector<int> curSels;
    GetListBoxSels(curSels);

    int index = 0;
    for (auto rit = curSels.rbegin(); rit != curSels.rend(); ++rit)
    {
        index = *rit;
        SendMessage(hActionsListBox, LB_DELETESTRING, index, NULL);
        FString key;
        key.Format("%d", index);
        map.DeleteKey(CurrentScriptID, key);
    }

    OnSelchangeScript(false, index);
    UpdateScriptPath();
}

void CNewScript::OnClickMoveupAction(HWND& hWnd, bool reverse)
{
    if (SelectedScriptIndex < 0)
        return;

    std::vector<int> sels;
    GetListBoxSels(sels);

    if (sels.empty())
        return;

    int count = SendMessage(hActionsListBox, LB_GETCOUNT, 0, 0);

    if (!reverse)
    {
        if (sels.front() == 0)
            return;
    }
    else
    {
        if (sels.back() == count - 1)
            return;
    }

    char buffer[512]{ 0 };

    if (!reverse)
    {
        for (size_t i = 0; i < sels.size(); ++i)
        {
            int idx = sels[i];
            int idx2 = idx - 1;

            FString key, key2;
            key.Format("%d", idx);
            key2.Format("%d", idx2);

            auto value = map.GetString(CurrentScriptID, key);
            auto value2 = map.GetString(CurrentScriptID, key2);

            map.WriteString(CurrentScriptID, key, value2);
            map.WriteString(CurrentScriptID, key2, value);

            FString text;

            SendMessage(hActionsListBox, LB_GETTEXT, idx, (LPARAM)buffer);
            auto atoms = FString::SplitString(buffer, " ");
            bool first = true;
            for (auto& atom : atoms)
            {
                if (first)
                {
                    text.Format("[%s]", GetOneBasedIndex(key2));
                    first = false;
                }
                else
                    text += " " + atom;
            }
            SendMessage(hActionsListBox, LB_DELETESTRING, idx2, 0);
            SendMessage(hActionsListBox, LB_INSERTSTRING, idx2, (LPARAM)(LPCSTR)text);

            SendMessage(hActionsListBox, LB_GETTEXT, idx2 + 1, (LPARAM)buffer);
            atoms = FString::SplitString(buffer, " ");
            first = true;
            for (auto& atom : atoms)
            {
                if (first)
                {
                    text.Format("[%s]", GetOneBasedIndex(key));
                    first = false;
                }
                else
                    text += " " + atom;
            }
            SendMessage(hActionsListBox, LB_DELETESTRING, idx2 + 1, 0);
            SendMessage(hActionsListBox, LB_INSERTSTRING, idx2 + 1, (LPARAM)(LPCSTR)text);

            sels[i]--;
        }
    }
    else
    {
        for (int i = (int)sels.size() - 1; i >= 0; --i)
        {
            int idx = sels[i];
            int idx2 = idx + 1;

            FString key, key2;
            key.Format("%d", idx);
            key2.Format("%d", idx2);

            auto value = map.GetString(CurrentScriptID, key);
            auto value2 = map.GetString(CurrentScriptID, key2);

            map.WriteString(CurrentScriptID, key, value2);
            map.WriteString(CurrentScriptID, key2, value);

            FString text;

            SendMessage(hActionsListBox, LB_GETTEXT, idx, (LPARAM)buffer);
            auto atoms = FString::SplitString(buffer, " ");
            bool first = true;
            for (auto& atom : atoms)
            {
                if (first)
                {
                    text.Format("[%s]", GetOneBasedIndex(key2));
                    first = false;
                }
                else
                    text += " " + atom;
            }
            SendMessage(hActionsListBox, LB_DELETESTRING, idx2, 0);
            SendMessage(hActionsListBox, LB_INSERTSTRING, idx2, (LPARAM)(LPCSTR)text);

            SendMessage(hActionsListBox, LB_GETTEXT, idx, (LPARAM)buffer);
            atoms = FString::SplitString(buffer, " ");
            first = true;
            for (auto& atom : atoms)
            {
                if (first)
                {
                    text.Format("[%s]", GetOneBasedIndex(key));
                    first = false;
                }
                else
                    text += " " + atom;
            }
            SendMessage(hActionsListBox, LB_DELETESTRING, idx, 0);
            SendMessage(hActionsListBox, LB_INSERTSTRING, idx, (LPARAM)(LPCSTR)text);

            sels[i]++; 
        }
    }

    OnSelchangeScript(false, sels.front());
    SetListBoxSels(sels);

    UpdateScriptPath();
}

void CNewScript::UpdateActionAndParam(int actionChanged, int listBoxCurChanged, bool changeActionIdx)
{
    if (listBoxCurChanged < 0)
    {
        listBoxCurChanged = SendMessage(hActionsListBox, LB_GETCARETINDEX, 0, NULL);
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
    if (auto pSection = fadata.GetSection(ExtraWindow::GetTranslatedSectionName("ScriptsRA2")))
    {
        FString action;
        action.Format("%d", actionChanged);
        if (actionChanged < 0)
        {
            action = atoms[0];
        }
        auto atoms2 = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("ScriptsRA2"), action, action + " - MISSING,0,1,0,MISSING"), 4);
        bool missing = !fadata.KeyExists(ExtraWindow::GetTranslatedSectionName("ScriptsRA2"), action);
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
            if (auto pSectionParam = fadata.GetSection(ExtraWindow::GetTranslatedSectionName("ScriptParams")))
            {
                auto param = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("ScriptParams"), paramIdx));
                if (param.size() >= 2)
                {
                    SendMessage(hActionParamDes, WM_SETTEXT, 0, (LPARAM)param[0]);
                    ExtraWindow::LoadParams(vcbActionParam, param[1]); 
                    if (!ExtConfigs::SearchCombobox_Waypoint && param[1] == "1") // waypoints
                    {
                        CNewScript::ParamAutodrop[0] = false;
                    }
                }
                if (ActionIsStringParam[atoms[0]])
                {
                    if (ActionHasExtraParam[name])
                    {
                        auto atoms = FString::SplitString(value, 2);
                        EnableWindow(hActionExtraParam, TRUE);
                        SendMessage(hActionExtraParamDes, WM_SETTEXT, 0, (LPARAM)param[2]);
                        ExtraWindow::LoadParams(vcbActionExtraParam, param[3]);
                        if (!ExtConfigs::SearchCombobox_Waypoint && param[3] == "1") // waypoints
                        {
                            CNewScript::ParamAutodrop[1] = false;
                        }
                        buffer.Format("%s - ", atoms[1]);
                        int idx = SendMessage(hActionParam, CB_FINDSTRING, 0, (LPARAM)buffer);
                        if (idx == CB_ERR)
                        {
                            buffer.Format("%s", atoms[1]);
                            SendMessage(hActionParam, WM_SETTEXT, 0, (LPARAM)buffer);
                        }
                        else
                            SendMessage(hActionParam, CB_SETCURSEL, idx, NULL);
                        buffer.Format("%s - ", atoms[2]);
                        idx = SendMessage(hActionExtraParam, CB_FINDSTRING, 0, (LPARAM)buffer);
                        if (idx == CB_ERR)
                        {
                            buffer.Format("%s", atoms[2]);
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
                else
                {
                    if (ActionHasExtraParam[name])
                    {
                        EnableWindow(hActionExtraParam, TRUE);
                        SendMessage(hActionExtraParamDes, WM_SETTEXT, 0, (LPARAM)param[2]);
                        ExtraWindow::LoadParams(vcbActionExtraParam, param[3]);
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
                        idx = SendMessage(hActionExtraParam, CB_FINDSTRING, 0, (LPARAM)buffer);
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
        }
        else if (missing)
        {
            EnableWindow(hActionParam, TRUE);
            EnableWindow(hActionExtraParam, FALSE);
            ExtraWindow::ClearComboKeepText(hActionParam);
            SendMessage(hActionExtraParam, WM_SETTEXT, 0, (LPARAM)"");
            SendMessage(hActionParam, WM_SETTEXT, 0, (LPARAM)atoms[1]);
            Translations::GetTranslationItem("ScriptTypesActionParam", buffer);
            SendMessage(hActionParamDes, WM_SETTEXT, 0, (LPARAM)buffer);
            Translations::GetTranslationItem("ScriptTypesExtraParam", buffer);
            SendMessage(hActionExtraParamDes, WM_SETTEXT, 0, (LPARAM)buffer);
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

void CNewScript::UpdateScriptPath()
{
    if (!CIsoViewExt::DrawScriptPath
        ||CurrentScriptID.IsEmpty() 
        || !CINI::CurrentDocument->SectionExists(CurrentScriptID))
        return;

    CIsoViewExt::ScriptPath.clear();
    std::set<int> jumpLines;
    for (int i = 0; i < 50; i++)
    {
        FString key;
        key.Format("%d", i);
        auto value = CINI::CurrentDocument->GetString(CurrentScriptID, key);
        auto atoms = FString::SplitString(value);
        auto& action = atoms[0];
        auto& actionParam = atoms[1];
        if (atoms.size() < 2) break;
        if (action == "6")
        {
            i = atoi(actionParam) - 2;
            if (jumpLines.contains(i))
                break;

            jumpLines.insert(i);
            continue;
        }

        if (auto pSection = CINI::FAData->GetSection(ExtraWindow::GetTranslatedSectionName("ScriptsRA2")))
        {
            auto pValue = CINI::FAData->TryGetString(
                ExtraWindow::GetTranslatedSectionName("ScriptsRA2"),
                action);
            if (!pValue)
                continue;
            auto atoms2 = FString::SplitString(*pValue, 4);
            FString name = atoms2[0];
            auto& paramIdx = atoms2[1];
            auto& disable = atoms2[2];
            auto& hasParam = atoms2[3];
            auto& description = atoms2[4];

            FString::TrimIndex(name);
            if (hasParam == "1")
            {
                if (auto pSectionParam = CINI::FAData->GetSection(ExtraWindow::GetTranslatedSectionName("ScriptParams")))
                {
                    auto param = FString::SplitString(CINI::FAData->GetString(ExtraWindow::GetTranslatedSectionName("ScriptParams"), paramIdx));
                    if (ActionHasExtraParam[name])
                    {
                        if (param[3] == "1") // waypoints
                        {
                            auto pos = CINI::CurrentDocument->GetInteger("Waypoints", actionParam);
                            int x = pos / 1000;
                            int y = pos % 1000;
                            CIsoViewExt::ScriptPath.push_back({ x,y });
                        }
                    }
                    else if (param.size() >= 2)
                    {
                        if (param[1] == "1") // waypoints
                        {
                            auto pos = CINI::CurrentDocument->GetInteger("Waypoints", actionParam);
                            int x = pos / 1000;
                            int y = pos % 1000;
                            CIsoViewExt::ScriptPath.push_back({ x,y });
                        }
                    }
                }
            }
        }
    }
    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->GetSafeHwnd(), 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
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
    return false;
}

void CNewScript::SetListBoxSel(int index)
{
    SendMessage(hActionsListBox, LB_SETSEL, FALSE, -1);
    if (index >= 0)
        SendMessage(hActionsListBox, LB_SETSEL, TRUE, index);
    SendMessage(hActionsListBox, LB_SETCURSEL, index, NULL);
    SendMessage(hActionsListBox, LB_SETCARETINDEX, index, TRUE);
}

void CNewScript::SetListBoxSels(std::vector<int>& indices)
{
    SendMessage(hActionsListBox, LB_SETSEL, FALSE, -1);
    for (int idx : indices)
    {
        if (idx >= 0 && idx < SendMessage(hActionsListBox, LB_GETCOUNT, 0, 0))
        {
            SendMessage(hActionsListBox, LB_SETSEL, TRUE, idx);
        }
    }
    if (!indices.empty())
    {
        SendMessage(hActionsListBox, LB_SETCARETINDEX, indices[0], TRUE);
    }
}

void CNewScript::GetListBoxSels(std::vector<int>& indices)
{
    int numSelected = SendMessage(hActionsListBox, LB_GETSELCOUNT, 0, 0);
    if (numSelected > 0)
    {
        indices.resize(numSelected);
        SendMessage(hActionsListBox, LB_GETSELITEMS, numSelected, (LPARAM)indices.data());
        std::sort(indices.begin(), indices.end());
    }
    else
    {
        indices.clear();
    }
}