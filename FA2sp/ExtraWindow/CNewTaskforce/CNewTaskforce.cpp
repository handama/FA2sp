#include "CNewTaskforce.h"
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
#include "../../Ext/CTileSetBrowserFrame/TabPages/TeamSort.h"
#include "../../Ext/CTileSetBrowserFrame/TabPages/TaskForceSort.h"
#include "../CNewScript/CNewScript.h"
#include <numeric>
#include "../CSearhReference/CSearhReference.h"
#include "../CTriggerAnnotation/CTriggerAnnotation.h"
#include "../CNewTeamTypes/CNewTeamTypes.h"

HWND CNewTaskforce::m_hwnd;
CFinalSunDlg* CNewTaskforce::m_parent;
CINI& CNewTaskforce::map = CINI::CurrentDocument;
MultimapHelper& CNewTaskforce::rules = Variables::RulesMap;


HWND CNewTaskforce::hSelectedTaskforce;
HWND CNewTaskforce::hNewTaskforce;
HWND CNewTaskforce::hDelTaskforce;
HWND CNewTaskforce::hCloTaskforce;
HWND CNewTaskforce::hAddUnit;
HWND CNewTaskforce::hDeleteUnit;
HWND CNewTaskforce::hName;
HWND CNewTaskforce::hGroup;
HWND CNewTaskforce::hUnitsListBox;
HWND CNewTaskforce::hNumber;
HWND CNewTaskforce::hUnitType;
HWND CNewTaskforce::hSearchReference;
HWND CNewTaskforce::hDragPoint;

int CNewTaskforce::SelectedTaskForceIndex = -1;
FString CNewTaskforce::CurrentTaskForceID;
VirtualComboBoxEx CNewTaskforce::vcbSelectedTaskforce;
VirtualComboBoxEx CNewTaskforce::vcbUnitType;
WNDPROC CNewTaskforce::OriginalListBoxProc;
WNDPROC CNewTaskforce::OrigDragDotProc;
WNDPROC CNewTaskforce::OrigDragingDotProc;
bool CNewTaskforce::m_dragging = false;
bool CNewTaskforce::m_programmaticEdit = false;
POINT CNewTaskforce::m_dragOffset{};
HWND CNewTaskforce::m_hDragGhost = nullptr;
TargetHighlighter CNewTaskforce::hl;

void CNewTaskforce::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(305),
        pWnd->GetSafeHwnd(),
        CNewTaskforce::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CNewTaskforce.\n");
        m_parent = NULL;
        return;
    }
}

void CNewTaskforce::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("TaskforceTitle", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };
    
    Translate(50800, "TaskforceList");
    Translate(50801, "TaskforceUnits");
    Translate(50802, "TaskforceGroup");
    Translate(50803, "TaskforceUnitNumber");
    Translate(50804, "TaskforceUnitType");
    Translate(50805, "TaskforceSelected");
    Translate(50806, "TaskforceName");
    Translate(1151, "TaskforceNewTask");
    Translate(1150, "TaskforceDelTask");
    Translate(50807, "TaskforceCloTask");
    Translate(1146, "TaskforceNewUnit");
    Translate(1147, "TaskforceDelUnit");
    Translate(1999, "SearchReferenceTitle");
    
    hSelectedTaskforce = GetDlgItem(hWnd, Controls::SelectedTaskforce);
    hNewTaskforce = GetDlgItem(hWnd, Controls::NewTaskforce);
    hDelTaskforce = GetDlgItem(hWnd, Controls::DelTaskforce);
    hCloTaskforce = GetDlgItem(hWnd, Controls::CloTaskforce);
    hAddUnit = GetDlgItem(hWnd, Controls::AddUnit);
    hDeleteUnit = GetDlgItem(hWnd, Controls::DeleteUnit);
    hName = GetDlgItem(hWnd, Controls::Name);
    hGroup = GetDlgItem(hWnd, Controls::Group);
    hUnitsListBox = GetDlgItem(hWnd, Controls::UnitsListBox);
    hNumber = GetDlgItem(hWnd, Controls::Number);
    hUnitType = GetDlgItem(hWnd, Controls::UnitType);
    hSearchReference = GetDlgItem(hWnd, Controls::SearchReference);
    hDragPoint = GetDlgItem(hWnd, Controls::DragPoint);

    if (hUnitsListBox)
        OriginalListBoxProc = (WNDPROC)SetWindowLongPtr(hUnitsListBox, GWLP_WNDPROC, (LONG_PTR)ListBoxSubclassProc);

    if (hDragPoint)
    {
        OrigDragDotProc = (WNDPROC)SetWindowLongPtr(hDragPoint, GWLP_WNDPROC, (LONG_PTR)DragDotProc);
    }
    hl.SetBorderColor(ExtConfigs::EnableDarkMode ? RGB(0, 90, 0) : RGB(0, 180, 0));
    hl.SetBorderThickness(3);
    hl.SetBorderRadius(0);

    vcbSelectedTaskforce.Attach(hSelectedTaskforce, &ExtConfigs::SortByLabelName_Taskforce, false);
    vcbUnitType.Attach(hUnitType);

    Update(hWnd);
}

void CNewTaskforce::Update(HWND& hWnd)
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    if (TaskforceSort::Instance.IsVisible())
        TaskforceSort::Instance.LoadAllTriggers();

    ExtraWindow::SortTeams(vcbSelectedTaskforce, "TaskForces", SelectedTaskForceIndex);

    int count = SendMessage(hSelectedTaskforce, CB_GETCOUNT, NULL, NULL);
    if (SelectedTaskForceIndex < 0)
        SelectedTaskForceIndex = 0;
    if (SelectedTaskForceIndex > count - 1)
        SelectedTaskForceIndex = count - 1;
    SendMessage(hSelectedTaskforce, CB_SETCURSEL, SelectedTaskForceIndex, NULL);

    ExtraWindow::LoadParam_TechnoTypes(vcbUnitType, 4, 1);

    OnSelchangeTaskforce();
}

void CNewTaskforce::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CNewTaskforce::m_hwnd = NULL;
    CNewTaskforce::m_parent = NULL;
}

LRESULT CALLBACK CNewTaskforce::DragDotProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SETCURSOR:
    {
        if (CurrentTaskForceID != "")
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

        HBRUSH hBrush = CreateSolidBrush(CurrentTaskForceID != "" ? RGB(0, 200, 0) : RGB(200, 0, 0));
        FillRect(hdc, &ps.rcPaint, hBrush);
        DeleteObject(hBrush);

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        if (CurrentTaskForceID != "")
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
        if (target.hWnd && IsWindowEnabled(target.hWnd) && target.type == DropType::TeamEditorTaskForce)
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
            auto taskforceID = CurrentTaskForceID + " ";
            switch (target.type)
            {
            case DropType::TeamEditorTaskForce:
                if (CNewTeamTypes::CurrentTeamID != "")
                {
                    auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, taskforceID);
                    if (idx == CB_ERR)
                    {
                        CNewTeamTypes::OnDropdownTaskForce();
                        idx = ExtraWindow::FindCBStringExactStart(target.hWnd, taskforceID);
                    }
                    if (idx != CB_ERR)
                    {
                        SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                        CNewTeamTypes::OnSelchangeTaskForce();
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

LRESULT CALLBACK CNewTaskforce::DragingDotProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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

LRESULT CALLBACK CNewTaskforce::ListBoxSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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

BOOL CALLBACK CNewTaskforce::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_ACTIVATE:
    {
        if (SelectedTaskForceIndex >= 0 && SelectedTaskForceIndex < SendMessage(hSelectedTaskforce, CB_GETCOUNT, NULL, NULL))
        {
            CTriggerAnnotation::Type = AnnoTaskforce;
            CTriggerAnnotation::ID = CurrentTaskForceID;
            ::SendMessage(CTriggerAnnotation::GetHandle(), 114515, 0, 0);
        }
        return TRUE;
    }
    case WM_INITDIALOG:
    {
        CNewTaskforce::Initialize(hWnd);
        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::UnitsListBox:
            ListBoxProc(hWnd, CODE, lParam);
            break;
        case Controls::NewTaskforce:
            if (CODE == BN_CLICKED)
                OnClickNewTaskforce();
            break;
        case Controls::DelTaskforce:
            if (CODE == BN_CLICKED)
                OnClickDelTaskforce(hWnd);
            break;
        case Controls::CloTaskforce:
            if (CODE == BN_CLICKED)
                OnClickCloTaskforce(hWnd);
            break;
        case Controls::AddUnit:
            if (CODE == BN_CLICKED)
                OnClickAddUnit(hWnd);
            break;
        case Controls::DeleteUnit:
            if (CODE == BN_CLICKED)
                OnClickDeleteUnit(hWnd);
            break;
        case Controls::SearchReference:
            if (CODE == BN_CLICKED)
                OnClickSearchReference(hWnd);
            break;
        case Controls::SelectedTaskforce:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTaskforce();
            break;
        case Controls::Name:
            if (CODE == EN_CHANGE && !m_programmaticEdit)
            {
                if (SelectedTaskForceIndex < 0)
                    break;
                CNewTeamTypes::TaskforceListChanged = true;
                char buffer[512]{ 0 };
                GetWindowText(hName, buffer, 511);
                map.WriteString(CurrentTaskForceID, "Name", buffer);

                FString name = ExtraWindow::FormatTriggerDisplayName(CurrentTaskForceID, buffer);
                vcbSelectedTaskforce.ReplaceString(SelectedTaskForceIndex, name);
                vcbSelectedTaskforce.SetCurSel(SelectedTaskForceIndex);
            }
            break;
        case Controls::Number:
            if (CODE == EN_CHANGE && !m_programmaticEdit)
                OnEditchangeNumber();
            break;
        case Controls::Group:
            if (CODE == EN_CHANGE && !m_programmaticEdit)
            {
                if (SelectedTaskForceIndex < 0)
                    break;
                char buffer[512]{ 0 };
                GetWindowText(hGroup, buffer, 511);
                map.WriteString(CurrentTaskForceID, "Group", buffer);
            }
            break;
        case Controls::UnitType:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeUnitType();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeUnitType(true);
            break;
        
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CNewTaskforce::Close(hWnd);
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

void CNewTaskforce::ListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam)
{
    if (SendMessage(hUnitsListBox, LB_GETCOUNT, NULL, NULL) <= 0)
        return;

    switch (nCode)
    {
    case LBN_SELCHANGE:
    case LBN_DBLCLK:
        OnSelchangeUnitListbox();
        break;
    default:
        break;
    }
   
}

void CNewTaskforce::OnEditchangeNumber()
{
    if (SelectedTaskForceIndex < 0 || SendMessage(hUnitsListBox, LB_GETCARETINDEX, NULL, NULL) < 0)
        return;
    char buffer[512]{ 0 };
    GetWindowText(hNumber, buffer, 511);
    int idx = SendMessage(hUnitsListBox, LB_GETCARETINDEX, 0, NULL);
    FString key;
    key.Format("%d", idx);
    auto value = map.GetString(CurrentTaskForceID, key);
    auto atoms = FString::SplitString(value, 1);
    value.Format("%s,%s", buffer, atoms[1]);

    map.WriteString(CurrentTaskForceID, key, value);

    value = map.GetString(CurrentTaskForceID, key);
    atoms = FString::SplitString(value, 1);
    FString text;
    text.Format("%s %s (%s)", atoms[0], atoms[1], CViewObjectsExt::QueryUIName(atoms[1], true));
    SendMessage(hUnitsListBox, LB_DELETESTRING, idx, NULL);
    SendMessage(hUnitsListBox, LB_INSERTSTRING, idx, (LPARAM)(LPCSTR)text);
    SetListBoxSel(idx);
}

void CNewTaskforce::OnSelchangeUnitListbox()
{
    if (SelectedTaskForceIndex < 0 || SendMessage(hUnitsListBox, LB_GETCARETINDEX, NULL, NULL) < 0 || SendMessage(hUnitsListBox, LB_GETCOUNT, NULL, NULL) <= 0)
    {
        m_programmaticEdit = true;
        SendMessage(hUnitType, CB_SETCURSEL, -1, NULL);
        SendMessage(hNumber, WM_SETTEXT, 0, (LPARAM)"");
        m_programmaticEdit = false;
        return;
    }

    int idx = SendMessage(hUnitsListBox, LB_GETCARETINDEX, 0, NULL);
    if (idx < 0)
        idx = 0;
    if (idx > 5)
        idx = 5;

    FString key;
    key.Format("%d", idx);
    auto value = map.GetString(CurrentTaskForceID, key);
    auto atoms = FString::SplitString(value, 1);

    m_programmaticEdit = true;
    SendMessage(hNumber, WM_SETTEXT, 0, (LPARAM)atoms[0]);

    FString text;
    text.Format("%s - %s", atoms[1], CViewObjectsExt::QueryUIName(atoms[1], true));
    int unitIdx = SendMessage(hUnitType, CB_FINDSTRINGEXACT, 0, (LPARAM)text);

    if (unitIdx != CB_ERR)
        SendMessage(hUnitType, CB_SETCURSEL, unitIdx, NULL);
    else
        SendMessage(hUnitType, WM_SETTEXT, 0, (LPARAM)atoms[1]);
    m_programmaticEdit = false;
}

void CNewTaskforce::OnSelchangeUnitType(bool edited)
{
    if (SelectedTaskForceIndex < 0 || SendMessage(hUnitsListBox, LB_GETCARETINDEX, NULL, NULL) < 0)
        return;

    FString text = vcbUnitType.GetSelectedText(edited);
    if (text.empty())
        return;
    
    FString::TrimIndex(text);
    if (text == "None")
        text = "";

    text.Replace(",", "");

    int idx = SendMessage(hUnitsListBox, LB_GETCARETINDEX, 0, NULL);
    FString key;
    key.Format("%d", idx);
    auto value = map.GetString(CurrentTaskForceID, key);
    auto atoms = FString::SplitString(value, 1);
    value.Format("%s,%s", atoms[0], text);
    
    map.WriteString(CurrentTaskForceID, key, value);

    value = map.GetString(CurrentTaskForceID, key);
    atoms = FString::SplitString(value, 1);
    text.Format("%s %s (%s)", atoms[0], atoms[1], CViewObjectsExt::QueryUIName(atoms[1], true));
    SendMessage(hUnitsListBox, LB_DELETESTRING, idx, NULL);
    SendMessage(hUnitsListBox, LB_INSERTSTRING, idx, (LPARAM)(LPCSTR)text);
    SetListBoxSel(idx);
}

void CNewTaskforce::OnSelchangeTaskforce(bool edited, int specificIdx)
{
    auto clear = []()
    {
        SendMessage(hUnitType, CB_SETCURSEL, -1, NULL);
        m_programmaticEdit = true;
        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hGroup, WM_SETTEXT, 0, (LPARAM)"");
        m_programmaticEdit = false;
        while (SendMessage(hUnitsListBox, LB_DELETESTRING, 0, NULL) != CB_ERR);
    };

    SelectedTaskForceIndex = SendMessage(hSelectedTaskforce, CB_GETCURSEL, NULL, NULL);
    if (SelectedTaskForceIndex < 0 || SelectedTaskForceIndex >= SendMessage(hSelectedTaskforce, CB_GETCOUNT, NULL, NULL)) 
    {
        clear();
        CurrentTaskForceID = "";
        InvalidateRect(hDragPoint, nullptr, TRUE);
    }

    FString pID = vcbSelectedTaskforce.GetItemText(SelectedTaskForceIndex);
    FString::TrimIndex(pID);

    CurrentTaskForceID = pID;
    InvalidateRect(hDragPoint, nullptr, TRUE);

    CTriggerAnnotation::Type = AnnoTaskforce;
    CTriggerAnnotation::ID = CurrentTaskForceID;
    ::SendMessage(CTriggerAnnotation::GetHandle(), 114515, 0, 0);

    ::SendMessage(CTriggerAnnotation::GetHandle(), 114515, 0, 0);
    while (SendMessage(hUnitsListBox, LB_DELETESTRING, 0, NULL) != CB_ERR);
    if (auto pTaskforce = map.GetSection(pID))
    {
        auto name = map.GetString(pID, "Name");
        auto group = map.GetString(pID, "Group");
        m_programmaticEdit = true;
        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)name.GetString());
        SendMessage(hGroup, WM_SETTEXT, 0, (LPARAM)group.GetString());
        m_programmaticEdit = false;

        std::vector<FString> sortedList;
        for (int i = 0; i < 6; i++)
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
            text.Format("%s %s (%s)", atoms[0], atoms[1], CViewObjectsExt::QueryUIName(atoms[1], true));
            SendMessage(hUnitsListBox, LB_ADDSTRING, 0, (LPARAM)(LPCSTR)text);
            FString key;
            key.Format("%d", i);
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
        while (specificIdx >= SendMessage(hUnitsListBox, LB_GETCOUNT, NULL, NULL))
            specificIdx--;
        SetListBoxSel(specificIdx);
    }
    else
        if (SendMessage(hUnitsListBox, LB_GETCOUNT, NULL, NULL) > 0)
            SetListBoxSel(0);

    OnSelchangeUnitListbox();
}

void CNewTaskforce::OnClickNewTaskforce()
{
    CNewTeamTypes::TaskforceListChanged = true;
    FString key = CINI::GetAvailableKey("TaskForces");
    FString value = CMapDataExt::GetAvailableIndex(EIndexType::TaskForce);
    FString buffer2;

    FString newName = "";
    if (TaskforceSort::CreateFromTaskForceSort)
        newName = TaskforceSort::Instance.GetCurrentPrefix();
    newName += "New task force";
    map.WriteString("TaskForces", key, value);
    map.WriteString(value, "Name", newName);
    map.WriteString(value, "Group", "-1");

    ExtraWindow::SortTeams(vcbSelectedTaskforce, "TaskForces", SelectedTaskForceIndex, value);

    OnSelchangeTaskforce();
}

void CNewTaskforce::OnClickDelTaskforce(HWND& hWnd)
{
    if (SelectedTaskForceIndex < 0)
        return;
    int result = MessageBox(hWnd,
        Translations::TranslateOrDefault("TaskforceDelTFWarn", "Are you sure to delete the selected task force? If you delete it, make sure to eliminate ANY references to this task force in team-types."),
        Translations::TranslateOrDefault("TaskforceDelTFTitle", "Delete task force"), MB_YESNO);

    if (result == IDNO)
        return;

    CNewTeamTypes::TaskforceListChanged = true;
    map.DeleteSection(CurrentTaskForceID);
    std::vector<FString> deteleKeys;
    if (auto pSection = map.GetSection("TaskForces"))
    {
        for (auto& pair : pSection->GetEntities())
        {
            if (CurrentTaskForceID == pair.second)
                deteleKeys.push_back(pair.first); 
        }
    }
    for (auto& key : deteleKeys)
        map.DeleteKey("TaskForces", key);
    
    int idx = SelectedTaskForceIndex;
    SendMessage(hSelectedTaskforce, CB_DELETESTRING, idx, NULL);
    if (idx >= SendMessage(hSelectedTaskforce, CB_GETCOUNT, NULL, NULL))
        idx--;
    if (idx < 0)
        idx = 0;
    SendMessage(hSelectedTaskforce, CB_SETCURSEL, idx, NULL);
    OnSelchangeTaskforce();
}

void CNewTaskforce::OnClickCloTaskforce(HWND& hWnd)
{
    if (SelectedTaskForceIndex < 0)
        return;
    if (SendMessage(hSelectedTaskforce, CB_GETCOUNT, NULL, NULL) > 0 && SelectedTaskForceIndex >= 0)
    {
        FString key = CINI::GetAvailableKey("TaskForces");
        FString value = CMapDataExt::GetAvailableIndex(EIndexType::TaskForce);

        CINI::CurrentDocument->WriteString("TaskForces", key, value);

        auto oldname = CINI::CurrentDocument->GetString(CurrentTaskForceID, "Name", "New task force");
        FString newName = ExtraWindow::GetCloneName(oldname);
       
        CINI::CurrentDocument->WriteString(value, "Name", newName);
        CNewTeamTypes::TaskforceListChanged = true;

        auto copyitem = [&value](FString key)
            {
                if (auto ppStr = map.TryGetString(CurrentTaskForceID, key)) {
                    FString str = *ppStr;
                    str.Trim();
                    map.WriteString(value, key, str);
                }
            };

        copyitem("Group");
        copyitem("0");
        copyitem("1");
        copyitem("2");
        copyitem("3");
        copyitem("4");
        copyitem("5");

        ExtraWindow::SortTeams(vcbSelectedTaskforce, "TaskForces", SelectedTaskForceIndex, value);

        OnSelchangeTaskforce();
    }
}

void CNewTaskforce::OnClickAddUnit(HWND& hWnd)
{
    if (SelectedTaskForceIndex < 0)
        return;
    int count = SendMessage(hUnitsListBox, LB_GETCOUNT, 0, NULL);
    if (count >= 6)
        return;

    char buffer[512]{ 0 };
    FString text;
    FString key;
    key.Format("%d", count);
    map.WriteString(CurrentTaskForceID, key, "1,E1");

    text.Format("%s %s (%s)", "1", "E1", CViewObjectsExt::QueryUIName("E1", true));
    SendMessage(hUnitsListBox, LB_INSERTSTRING, count, (LPARAM)(LPCSTR)text);
    SetListBoxSel(count);
    OnSelchangeUnitListbox();
}

void CNewTaskforce::OnClickDeleteUnit(HWND& hWnd)
{
    if (SelectedTaskForceIndex < 0 || SendMessage(hUnitsListBox, LB_GETCARETINDEX, NULL, NULL) < 0)
        return;

    std::vector<int> selected;
    GetListBoxSels(selected);

    int index = 0;
    for (auto rit = selected.rbegin(); rit != selected.rend(); ++rit)
    {
        index = *rit;
        SendMessage(hUnitsListBox, LB_DELETESTRING, index, NULL);
        FString key;
        key.Format("%d", index);
        map.DeleteKey(CurrentTaskForceID, key);
    }

    OnSelchangeTaskforce(false, index);
}

void CNewTaskforce::OnClickSearchReference(HWND& hWnd)
{
    if (SelectedTaskForceIndex < 0)
        return;

    CSearhReference::SetSearchType(2);
    CSearhReference::SetSearchID(CurrentTaskForceID);
    if (CSearhReference::GetHandle() == NULL)
    { 
        CSearhReference::Create(m_parent);
    }
    else
    {
        ::SendMessage(CSearhReference::GetHandle(), 114514, 0, 0);
    }
}

bool CNewTaskforce::OnEnterKeyDown(HWND& hWnd)
{
    return false;
}

void CNewTaskforce::SetListBoxSel(int index)
{
    SendMessage(hUnitsListBox, LB_SETSEL, FALSE, -1);
    if (index >= 0)
        SendMessage(hUnitsListBox, LB_SETSEL, TRUE, index);
    SendMessage(hUnitsListBox, LB_SETCURSEL, index, NULL);
    SendMessage(hUnitsListBox, LB_SETCARETINDEX, index, TRUE);
}

void CNewTaskforce::SetListBoxSels(std::vector<int>& indices)
{
    SendMessage(hUnitsListBox, LB_SETSEL, FALSE, -1);
    for (int idx : indices)
    {
        if (idx >= 0 && idx < SendMessage(hUnitsListBox, LB_GETCOUNT, 0, 0))
        {
            SendMessage(hUnitsListBox, LB_SETSEL, TRUE, idx);
        }
    }
    if (!indices.empty())
    {
        SendMessage(hUnitsListBox, LB_SETCARETINDEX, indices[0], TRUE);
    }
}

void CNewTaskforce::GetListBoxSels(std::vector<int>& indices)
{
    int numSelected = SendMessage(hUnitsListBox, LB_GETSELCOUNT, 0, 0);
    if (numSelected > 0)
    {
        indices.resize(numSelected);
        SendMessage(hUnitsListBox, LB_GETSELITEMS, numSelected, (LPARAM)indices.data());
        std::sort(indices.begin(), indices.end());
    }
    else
    {
        indices.clear();
    }
}