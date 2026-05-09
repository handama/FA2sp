#include "CNewTag.h"
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
#include "../../Ext/CTileSetBrowserFrame/TabPages/TagSort.h"
#include "../CNewScript/CNewScript.h"
#include <numeric>
#include "../CSearhReference/CSearhReference.h"
#include "../CTriggerAnnotation/CTriggerAnnotation.h"
#include "../CNewTeamTypes/CNewTeamTypes.h"
#include "../CBatchTrigger/CBatchTrigger.h"

HWND CNewTag::m_hwnd;
CFinalSunDlg* CNewTag::m_parent;
CINI& CNewTag::map = CINI::CurrentDocument;
MultimapHelper& CNewTag::rules = Variables::RulesMap;

HWND CNewTag::hSelectedTag;
HWND CNewTag::hNewTag;
HWND CNewTag::hDelTag;
HWND CNewTag::hCloTag;
HWND CNewTag::hRepeat;
HWND CNewTag::hName;
HWND CNewTag::hTrigger;
HWND CNewTag::hTurnToTrigger;
HWND CNewTag::hSearchReference;
HWND CNewTag::hDragPoint;
int CNewTag::SelectedTagIndex;
FString CNewTag::CurrentTagID;

VirtualComboBoxEx CNewTag::vcbSelectedTag;
VirtualComboBoxEx CNewTag::vcbRepeat;
VirtualComboBoxEx CNewTag::vcbTrigger;

WNDPROC CNewTag::OrigDragDotProc;
WNDPROC CNewTag::OrigDragingDotProc;
bool CNewTag::m_dragging;
POINT CNewTag::m_dragOffset;
HWND CNewTag::m_hDragGhost;
TargetHighlighter CNewTag::hl;
bool CNewTag::m_programmaticEdit;
bool CNewTag::m_disableRepeatSearch;
bool CNewTag::TriggerListChanged;

void CNewTag::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(340),
        pWnd->GetSafeHwnd(),
        CNewTag::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CNewTag.\n");
        m_parent = NULL;
        return;
    }
}

void CNewTag::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("Tag.Title", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };
    
    Translate(1000, "Tag.Description");
    Translate(1001, "Tag.Tag");
    Translate(1003, "Tag.Add");
    Translate(1004, "Tag.Clone");
    Translate(1005, "Tag.Delete");
    Translate(1006, "Tag.Name");
    Translate(1008, "Tag.Repeat");
    Translate(1010, "Tag.Trigger");
    Translate(1012, "Tag.RepeatCases");
    Translate(1999, "SearchReferenceTitle");
   
    hSelectedTag = GetDlgItem(hWnd, Controls::SelectedTag);
    hNewTag = GetDlgItem(hWnd, Controls::NewTag);
    hDelTag = GetDlgItem(hWnd, Controls::DelTag);
    hCloTag = GetDlgItem(hWnd, Controls::CloTag);
    hRepeat = GetDlgItem(hWnd, Controls::Repeat);
    hName = GetDlgItem(hWnd, Controls::Name);
    hTrigger = GetDlgItem(hWnd, Controls::Trigger);
    hTurnToTrigger = GetDlgItem(hWnd, Controls::TurnToTrigger);
    hSearchReference = GetDlgItem(hWnd, Controls::SearchReference);
    hDragPoint = GetDlgItem(hWnd, Controls::DragPoint);

    if (hDragPoint)
    {
        OrigDragDotProc = (WNDPROC)SetWindowLongPtr(hDragPoint, GWLP_WNDPROC, (LONG_PTR)DragDotProc);
    }
    hl.SetBorderColor(ExtConfigs::EnableDarkMode ? RGB(0, 90, 0) : RGB(0, 180, 0));
    hl.SetBorderThickness(3);
    hl.SetBorderRadius(0);

    m_disableRepeatSearch = false;
    vcbSelectedTag.Attach(hSelectedTag, &ExtConfigs::SortByLabelName_Tag, false);
    vcbRepeat.Attach(hRepeat);
    vcbRepeat.SetAutoSearchRestriction(&m_disableRepeatSearch);
    vcbTrigger.Attach(hTrigger, &ExtConfigs::SortByLabelName_Trigger);

    Update(true);
}

void CNewTag::Update(bool updateTrigger, FString id)
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    if (updateTrigger)
        CMapDataExt::UpdateTriggers();

    if (TagSort::Instance.IsVisible())
        TagSort::Instance.LoadAllTriggers();

    SortTags(vcbSelectedTag, SelectedTagIndex);
    vcbSelectedTag.AdjustCurSel(SelectedTagIndex);

    vcbRepeat.Clear();
    vcbRepeat.AddString((FString("0 - ") + Translations::TranslateOrDefault("TriggerRepeatType.OneTimeOr", "One Time OR")));
    vcbRepeat.AddString((FString("1 - ") + Translations::TranslateOrDefault("TriggerRepeatType.OneTimeAnd", "One Time AND")));
    vcbRepeat.AddString((FString("2 - ") + Translations::TranslateOrDefault("TriggerRepeatType.RepeatingOr", "Repeating OR")));

    SortTriggers();

    int index = -1;
    if (!id.IsEmpty())
    {
        auto text = ExtraWindow::GetTagDisplayName(id);
        index = vcbSelectedTag.FindStringExact(text);
    }

    OnSelchangeTag(false, index);
}

void CNewTag::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CNewTag::m_hwnd = NULL;
    CNewTag::m_parent = NULL;
}

LRESULT CALLBACK CNewTag::DragDotProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SETCURSOR:
    {
        if (CurrentTagID != "")
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

        HBRUSH hBrush = CreateSolidBrush(CurrentTagID != "" ? RGB(0, 200, 0) : RGB(200, 0, 0));
        FillRect(hdc, &ps.rcPaint, hBrush);
        DeleteObject(hBrush);

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        if (CurrentTagID != "")
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
        if (target.hWnd && IsWindowEnabled(target.hWnd) 
            && (target.type == DropType::TeamEditorTag ||
            target.type == DropType::ActionParam0 ||
            target.type == DropType::ActionParam1 ||
            target.type == DropType::ActionParam2 ||
            target.type == DropType::ActionParam3 ||
            target.type == DropType::ActionParam4 ||
            target.type == DropType::ActionParam5 ||
            target.type == DropType::EventParam0 ||
            target.type == DropType::EventParam1 ||
            target.type == DropType::BatchTriggerListView)
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
            auto tagID = CurrentTagID + " ";
            switch (target.type)
            {
            case DropType::TeamEditorTag:
                if (CNewTeamTypes::CurrentTeamID != "")
                {
                    auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    if (idx == CB_ERR)
                    {
                        CNewTeamTypes::OnDropdownTag();
                        idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    }
                    if (idx != CB_ERR)
                    {
                        SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                        CNewTeamTypes::OnSelchangeTag();
                    }
                }
                break;
            case DropType::ActionParam0:
                if (target.triggerInstance && target.triggerInstance->GetHandle())
                {
                    auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    if (idx == CB_ERR)
                    {
                        target.triggerInstance->OnSelchangeActionListbox();
                        idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    }
                    if (idx != CB_ERR)
                    {
                        SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                        target.triggerInstance->OnSelchangeActionParam(0);
                    }
                }
                break;
            case DropType::ActionParam1:
                if (target.triggerInstance && target.triggerInstance->GetHandle())
                {
                    auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    if (idx == CB_ERR)
                    {
                        target.triggerInstance->OnSelchangeActionListbox();
                        idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    }
                    if (idx != CB_ERR)
                    {
                        SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                        target.triggerInstance->OnSelchangeActionParam(1);
                    }
                }
                break;
            case DropType::ActionParam2:
                if (target.triggerInstance && target.triggerInstance->GetHandle())
                {
                    auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    if (idx == CB_ERR)
                    {
                        target.triggerInstance->OnSelchangeActionListbox();
                        idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    }
                    if (idx != CB_ERR)
                    {
                        SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                        target.triggerInstance->OnSelchangeActionParam(2);
                    }
                }
                break;
            case DropType::ActionParam3:
                if (target.triggerInstance && target.triggerInstance->GetHandle())
                {
                    auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    if (idx == CB_ERR)
                    {
                        target.triggerInstance->OnSelchangeActionListbox();
                        idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    }
                    if (idx != CB_ERR)
                    {
                        SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                        target.triggerInstance->OnSelchangeActionParam(3);
                    }
                }
                break;
            case DropType::ActionParam4:
                if (target.triggerInstance && target.triggerInstance->GetHandle())
                {
                    auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    if (idx == CB_ERR)
                    {
                        target.triggerInstance->OnSelchangeActionListbox();
                        idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    }
                    if (idx != CB_ERR)
                    {
                        SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                        target.triggerInstance->OnSelchangeActionParam(4);
                    }
                }
                break;
            case DropType::ActionParam5:
                if (target.triggerInstance && target.triggerInstance->GetHandle())
                {
                    auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    if (idx == CB_ERR)
                    {
                        target.triggerInstance->OnSelchangeActionListbox();
                        idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    }
                    if (idx != CB_ERR)
                    {
                        SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                        target.triggerInstance->OnSelchangeActionParam(5);
                    }
                }
                break;
            case DropType::EventParam0:
                if (target.triggerInstance && target.triggerInstance->GetHandle())
                {
                    auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    if (idx == CB_ERR)
                    {
                        target.triggerInstance->OnSelchangeEventListbox();
                        idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    }
                    if (idx != CB_ERR)
                    {
                        SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                        target.triggerInstance->OnSelchangeEventParam(0);
                    }
                }
                break;
            case DropType::EventParam1:
                if (target.triggerInstance && target.triggerInstance->GetHandle())
                {
                    auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    if (idx == CB_ERR)
                    {
                        target.triggerInstance->OnSelchangeEventListbox();
                        idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tagID);
                    }
                    if (idx != CB_ERR)
                    {
                        SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                        target.triggerInstance->OnSelchangeEventParam(1);
                    }
                }
                break;
            case DropType::BatchTriggerListView:
            {
                ListViewHitResult hit;
                if (ExtraWindow::HitTestListView(target.hWnd, pt, hit))
                {
                    CBatchTrigger::OnDroppedIntoCell(hit.item, hit.subItem, CurrentTagID);
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

LRESULT CALLBACK CNewTag::DragingDotProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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

BOOL CALLBACK CNewTag::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_ACTIVATE:
    {
        if (SelectedTagIndex >= 0 && SelectedTagIndex <vcbSelectedTag.GetCount())
        {
            CTriggerAnnotation::Type = AnnoTag;
            CTriggerAnnotation::ID = CurrentTagID;
            ::SendMessage(CTriggerAnnotation::GetHandle(), 114515, 0, 0);
        }
        return TRUE;
    }
    case WM_INITDIALOG:
    {
        CNewTag::Initialize(hWnd);
        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::NewTag:
            if (CODE == BN_CLICKED)
                OnClickNewTag();
            break;
        case Controls::DelTag:
            if (CODE == BN_CLICKED)
                OnClickDelTag(hWnd);
            break;
        case Controls::CloTag:
            if (CODE == BN_CLICKED)
                OnClickCloTag(hWnd);
            break;
        case Controls::SearchReference:
            if (CODE == BN_CLICKED)
                OnClickSearchReference(hWnd);
            break;
        case Controls::TurnToTrigger:
            if (CODE == BN_CLICKED)
                OnClickTurnToTrigger(hWnd);
            break;
        case Controls::SelectedTag:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTag();
            break;
        case Controls::Trigger:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTrigger();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeTrigger(true);
            else if (CODE == CBN_DROPDOWN && TriggerListChanged)
                OnDropdownTrigger();
            break;
        case Controls::Repeat:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeRepeat();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeRepeat(true);
            break;
        case Controls::Name:
            if (CODE == EN_CHANGE && !m_programmaticEdit)
                OnEditchangeName();
            break;
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CNewTag::Close(hWnd);
        return TRUE;
    }
    case 114514: // used for update
    {
        Update(true);
        return TRUE;
    }
    case 114515: // used for Trigger update
    {
        Update(false, CurrentTagID);
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

void CNewTag::SortTags(VirtualComboBoxEx& vcb, int& selectedIndex, FString id, bool clear)
{
    if (clear)
        vcb.Clear();
    std::vector<FString> labels;
    if (auto pSection = map.GetSection("Tags")) {
        for (auto& pair : pSection->GetEntities()) {
            labels.push_back(ExtraWindow::GetTagDisplayName(pair.first));
        }
    }

    bool tmp = ExtConfigs::SortByLabelName;
    ExtConfigs::SortByLabelName = ExtConfigs::SortByLabelName_Tag;
    ExtraWindow::SortLabels(labels);
    ExtConfigs::SortByLabelName = tmp;

    vcb.AddStrings(labels);
    if (id != "") {
        selectedIndex = vcb.FindStringExact(ExtraWindow::GetTagDisplayName(id));
        vcb.SetCurSel(selectedIndex);
    }
}

void CNewTag::SortTriggers()
{
    std::vector<FString> labels;
    for (auto& triggerPair : CMapDataExt::Triggers) {
        auto& trigger = triggerPair.second;
        labels.push_back(ExtraWindow::GetTriggerDisplayName(trigger->ID));
    }

    bool tmp = ExtConfigs::SortByLabelName;
    ExtConfigs::SortByLabelName = ExtConfigs::SortByLabelName_Trigger;
    ExtraWindow::SortLabels(labels);
    ExtConfigs::SortByLabelName = tmp;

    vcbTrigger.Clear();
    vcbTrigger.AddStrings(labels);
    TriggerListChanged = false;
}

void CNewTag::OnDropdownTrigger()
{
    int curSel = vcbTrigger.GetCurSel();
    FString text = vcbTrigger.GetEditText();
    FString::TrimIndex(text);
    text += " ";

    SortTriggers();

    int idx = vcbTrigger.FindStringExactStart(text);
    if (idx != CB_ERR)
    {
        vcbTrigger.SetCurSel(idx);
    }
    else
    {
        FString::TrimIndex(text);
        vcbTrigger.SetEditText(text);
    }
}

void CNewTag::OnSelchangeTrigger(bool edited)
{
    if (SelectedTagIndex < 0)
        return;

    FString text = vcbTrigger.GetSelectedText(edited);
    if (text.empty())
        return;

    FString::TrimIndex(text);
    text.Replace(",", "");

    FString value = map.GetString("Tags", CurrentTagID);
    value.SetParam(2, text);

    map.WriteString("Tags", CurrentTagID, value);
}

void CNewTag::OnSelchangeRepeat(bool edited)
{
    if (SelectedTagIndex < 0)
        return;

    FString text = vcbRepeat.GetSelectedText(edited);
    if (text.empty())
        return;

    FString::TrimIndex(text);
    text.Replace(",", "");

    FString value = map.GetString("Tags", CurrentTagID);
    value.SetParam(0, text);

    map.WriteString("Tags", CurrentTagID, value);
}

void CNewTag::OnEditchangeName()
{
    if (SelectedTagIndex < 0)
        return;

    CNewTeamTypes::TagListChanged = true;
    char buffer[512]{ 0 };
    GetWindowText(hName, buffer, 511);
    FString text = buffer;
    text.Replace(",", "");
    FString value = map.GetString("Tags", CurrentTagID);
    value.SetParam(1, text);
    map.WriteString("Tags", CurrentTagID, value);

    FString name = ExtraWindow::FormatTriggerDisplayName(CurrentTagID, text);
    vcbSelectedTag.ReplaceString(SelectedTagIndex, name);
    vcbSelectedTag.SetCurSel(SelectedTagIndex);
}

void CNewTag::OnSelchangeTag(bool edited, int specificIdx)
{
    auto clear = []()
    {
        m_programmaticEdit = true;
        vcbSelectedTag.SetCurSel(-1);
        vcbRepeat.SetCurSel(-1);
        vcbTrigger.SetCurSel(-1);
        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)"");
        m_programmaticEdit = false;
    };
    if (specificIdx != -1)
        SelectedTagIndex = specificIdx;
    else
        SelectedTagIndex = vcbSelectedTag.GetCurSel();
    if (SelectedTagIndex < 0 || SelectedTagIndex >= vcbSelectedTag.GetCount())
    {
        clear();
        CurrentTagID = "";
        InvalidateRect(hDragPoint, nullptr, TRUE);
    }

    if (TriggerListChanged)
        SortTriggers();

    FString pID = vcbSelectedTag.GetItemText(SelectedTagIndex);
    FString::TrimIndex(pID);

    CurrentTagID = pID;
    InvalidateRect(hDragPoint, nullptr, TRUE);

    CTriggerAnnotation::Type = AnnoTag;
    CTriggerAnnotation::ID = CurrentTagID;
    ::SendMessage(CTriggerAnnotation::GetHandle(), 114515, 0, 0);

    if (auto pTag = map.TryGetString("Tags", CurrentTagID))
    {
        auto values = FString::SplitString(*pTag, 2);
        values[0].Trim();
        values[1].Trim();
        values[2].Trim();
        auto repeat = atoi(values[0]);
        auto& name = values[1];
        auto& trigger = values[2];

        m_programmaticEdit = true;
        SendMessage(hName, WM_SETTEXT, 0, name);
        if (repeat >= 0 && repeat <= 2)
            vcbRepeat.SetCurSel(repeat);
        else
            vcbRepeat.SetEditText(values[0]);

        auto displayName = ExtraWindow::GetTriggerDisplayName(trigger);
        int idx = vcbTrigger.FindStringExact(displayName);
        if (idx != CB_ERR)
            vcbTrigger.SetCurSel(idx);
        else
            vcbTrigger.SetEditText(trigger);
        m_programmaticEdit = false;
    }
    else
    {
        clear();
    }
}

void CNewTag::OnClickNewTag()
{
    auto pTrigger = map.GetSection("Triggers");
    if (!pTrigger || pTrigger->GetEntities().empty())
    {
        ::MessageBox(GetHandle(), 
            Translations::TranslateOrDefault("Tag.NewTagFailed",
            "Before creating tags, you need at least one trigger."), 
            Translations::TranslateOrDefault("Error", "Error"),
            MB_ICONWARNING);
        return;
    }

    CNewTeamTypes::TagListChanged = true;

    FString key = CMapDataExt::GetAvailableIndex(EIndexType::Tag);
    FString value;

    FString name = "";
    if (TagSort::CreateFromTagSort)
        name = TagSort::Instance.GetCurrentPrefix();
    name += "New Tag";
    value.Format("0,%s,%s", name, *pTrigger->GetKeyAt(0));

    map.WriteString("Tags", key, value);

    SortTags(vcbSelectedTag, SelectedTagIndex, key);
    OnSelchangeTag();
}

void CNewTag::OnClickDelTag(HWND& hWnd)
{
    if (SelectedTagIndex < 0)
        return;
    int result = MessageBox(hWnd,
        Translations::TranslateOrDefault("Tag.DelWarn", "Are you sure to delete the selected tag? This may cause the attached trigger to don't work anymore, if no other tag has the trigger attached."),
        Translations::TranslateOrDefault("Tag.DelTitle", "Delete tag"), MB_YESNO);

    if (result == IDNO)
        return;

    CNewTeamTypes::TagListChanged = true;
    map.DeleteKey("Tags", CurrentTagID);

    int idx = SelectedTagIndex;
    vcbSelectedTag.DeleteString(idx);
    if (vcbSelectedTag.GetCount())
        idx--;
    if (idx < 0)
        idx = 0;
    vcbSelectedTag.SetCurSel(idx);
    OnSelchangeTag();
}

void CNewTag::OnClickCloTag(HWND& hWnd)
{
    if (SelectedTagIndex < 0)
        return;

    auto key = CMapDataExt::GetAvailableIndex(EIndexType::Tag);
    FString value = map.GetString("Tags", CurrentTagID);
    auto oldName = FString::GetParam(value, 1);
    FString newName = ExtraWindow::GetCloneName(oldName);
    value.SetParam(1, newName);

    CINI::CurrentDocument->WriteString("Tags", key, value);

    SortTags(vcbSelectedTag, SelectedTagIndex, key);
    OnSelchangeTag();
}

void CNewTag::OnClickTurnToTrigger(HWND& hWnd)
{
    if (SelectedTagIndex < 0)
        return;

    FString text = vcbTrigger.GetEditText();
    if (text.empty())
        return;

    FString::TrimIndex(text);

    auto& editor = CNewTrigger::GetFirstValidInstance();
    if (editor.GetHandle() == NULL)
        editor.Create(m_parent);
    for (const auto& [ID, trigger] : CMapDataExt::Triggers)
    {
        if (trigger->Tag == text)
        {
            text = trigger->ID;
            break;
        }
    }

    auto dlg = GetDlgItem(editor.GetHandle(), CNewTrigger::Controls::SelectedTrigger);
    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, ExtraWindow::GetTriggerDisplayName(text));
    if (idx == CB_ERR)
        return;
    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
    editor.OnSelchangeTrigger();
    SetWindowPos(editor.GetHandle(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

void CNewTag::OnClickSearchReference(HWND& hWnd)
{
    if (SelectedTagIndex < 0)
        return;

    CSearhReference::SetSearchType(4);
    CSearhReference::SetSearchID(CurrentTagID);
    if (CSearhReference::GetHandle() == NULL)
    { 
        CSearhReference::Create(m_parent);
    }
    else
    {
        ::SendMessage(CSearhReference::GetHandle(), 114514, 0, 0);
    }
}
