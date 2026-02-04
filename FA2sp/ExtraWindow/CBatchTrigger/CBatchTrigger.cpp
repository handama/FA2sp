#include "CBatchTrigger.h"
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
#include "../../Miscs/DialogStyle.h"
#include "../../Helpers/Helper.h"

HWND CBatchTrigger::m_hwnd;
CFinalSunDlg* CBatchTrigger::m_parent;
CINI& CBatchTrigger::map = CINI::CurrentDocument;
MultimapHelper& CBatchTrigger::rules = Variables::RulesMap;

HWND CBatchTrigger::hListbox;
HWND CBatchTrigger::hListView;
HWND CBatchTrigger::hEventIndex;
HWND CBatchTrigger::hActionIndex;
HWND CBatchTrigger::hAdd;
HWND CBatchTrigger::hDelete;
HWND CBatchTrigger::hAutoFill;
HWND CBatchTrigger::hUseID;
HWND CBatchTrigger::hSearch;
HWND CBatchTrigger::hMoveUp;
HWND CBatchTrigger::hMoveDown;
HWND CBatchTrigger::g_hInplaceEdit;
int CBatchTrigger::g_nEditRow = -1;
int CBatchTrigger::g_nEditCol = -1;
int CBatchTrigger::origWndWidth;
int CBatchTrigger::origWndHeight;
int CBatchTrigger::minWndWidth;
int CBatchTrigger::minWndHeight;
bool CBatchTrigger::minSizeSet;
WNDPROC CBatchTrigger::OriginalListBoxProc;
WNDPROC CBatchTrigger::g_pOriginalListViewProc = nullptr;
std::list<FString> CBatchTrigger::ListboxTriggerID;
std::vector<FString> CBatchTrigger::ListedTriggerIDs;
int CBatchTrigger::CurrentEventIndex = 0;
int CBatchTrigger::CurrentActionIndex = 0;
std::vector<CellColor> CBatchTrigger::g_specialCells;
FString CBatchTrigger::g_nEditOri;
bool CBatchTrigger::NeedClear = false;
bool CBatchTrigger::IsUpdating = false;
bool CBatchTrigger::bUseID = false;
static std::vector<ObjInfo> objects;
std::vector<ObjInfo> CBatchTrigger::objects;
std::unordered_map<FString, ObjInfo*> CBatchTrigger::idIndex;
std::unordered_map<FString, ObjInfo*> CBatchTrigger::triggerNameIndex;
std::unordered_map<FString, ObjInfo*> CBatchTrigger::tagNameIndex;
std::unordered_map<FString, ObjInfo*> CBatchTrigger::teamNameIndex;

LRESULT CALLBACK CBatchTrigger::ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DarkTheme::MyCallWindowProcA(g_pOriginalListViewProc, hWnd, uMsg, wParam, lParam);
}

void CBatchTrigger::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(333),
        pWnd->GetSafeHwnd(),
        CBatchTrigger::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CBatchTrigger.\n");
        m_parent = NULL;
        return;
    }
}

void CBatchTrigger::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("BatchTriggerTitle", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };
    
	Translate(1002, "BatchTriggerEventIndex");
	Translate(1004, "BatchTriggerActionIndex");
	Translate(1006, "BatchTriggerAdd");
	Translate(1007, "BatchTriggerRemove");
	Translate(1008, "BatchTriggerAutoFill");
	Translate(1009, "BatchTriggerUseID");
	Translate(1013, "BatchTriggerAllTriggers");
	Translate(1014, "BatchTriggerCurrentTriggers");
	Translate(1010, "BatchTriggerSearchTrigger");
	Translate(1012, "BatchTriggerDesc");
	Translate(1015, "BatchTriggerMoveUp");
	Translate(1016, "BatchTriggerMoveDown");

    hListbox = GetDlgItem(hWnd, Controls::Listbox);
    hListView = GetDlgItem(hWnd, Controls::ListView);
    hEventIndex  = GetDlgItem(hWnd, Controls::EventIndex);
    hActionIndex = GetDlgItem(hWnd, Controls::ActionIndex);
    hAdd = GetDlgItem(hWnd, Controls::Add);
    hDelete = GetDlgItem(hWnd, Controls::Delete);
    hAutoFill = GetDlgItem(hWnd, Controls::AutoFill);
    hUseID = GetDlgItem(hWnd, Controls::UseID);
    hSearch = GetDlgItem(hWnd, Controls::Search);
    hMoveUp = GetDlgItem(hWnd, Controls::MoveUp);
    hMoveDown = GetDlgItem(hWnd, Controls::MoveDown);

    ExtraWindow::SetEditControlFontSize(hListView, 1.2f);
    ExtraWindow::RegisterDropTarget(hListView, DropType::BatchTriggerListView);

    if (hListbox)
        OriginalListBoxProc = (WNDPROC)SetWindowLongPtr(hListbox, GWLP_WNDPROC, (LONG_PTR)ListBoxSubclassProc);

    SendMessage(hListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
    if (ExtConfigs::EnableDarkMode)
    {
        ::SendMessage(hListView, LVM_SETTEXTBKCOLOR, 0, RGB(32, 32, 32));
        ::SendMessage(hListView, LVM_SETTEXTCOLOR, 0, RGB(220, 220, 220));

        DarkTheme::SubclassListViewHeader(hListView);
        g_pOriginalListViewProc = (WNDPROC)GetWindowLongPtr(hListView, GWLP_WNDPROC);
        if (g_pOriginalListViewProc)
        {
            SetWindowLongPtr(hListView, GWLP_WNDPROC, (LONG_PTR)ListViewSubclassProc);
        }
        InvalidateRect(hListView, NULL, TRUE);
    }

    SendMessage(hListView, LVM_DELETEALLITEMS, 0, 0);
    while (SendMessage(hListView, LVM_DELETECOLUMN, 0, 0)) {}

    auto SetColumn = [](int index, int width, const char* Label)
    {
        LVCOLUMN lvColumn = { 0 };
        lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        lvColumn.pszText = const_cast<LPSTR>(Label);
        lvColumn.cx = width;
        SendMessage(hListView, LVM_INSERTCOLUMN, index, (LPARAM)&lvColumn);
    };

    SetColumn(Lists::ID, 100, Translations::TranslateOrDefault("BatchTriggerViewID", "ID"));
    SetColumn(Lists::Name, 100, Translations::TranslateOrDefault("BatchTriggerViewName", "Name"));
    SetColumn(Lists::EventNum, 60, Translations::TranslateOrDefault("BatchTriggerViewEvent", "Event"));
    SetColumn(Lists::EventParam1, 20, Translations::TranslateOrDefault("BatchTriggerViewP1", "P1"));
    SetColumn(Lists::EventParam2, 20, Translations::TranslateOrDefault("BatchTriggerViewP2", "P2"));
    SetColumn(Lists::ActionNum, 60, Translations::TranslateOrDefault("BatchTriggerViewAction", "Action"));
    SetColumn(Lists::ActionParam1, 20, Translations::TranslateOrDefault("BatchTriggerViewP1", "P1"));
    SetColumn(Lists::ActionParam2, 20, Translations::TranslateOrDefault("BatchTriggerViewP2", "P2"));
    SetColumn(Lists::ActionParam3, 20, Translations::TranslateOrDefault("BatchTriggerViewP3", "P3"));
    SetColumn(Lists::ActionParam4, 20, Translations::TranslateOrDefault("BatchTriggerViewP4", "P4"));
    SetColumn(Lists::ActionParam5, 20, Translations::TranslateOrDefault("BatchTriggerViewP5", "P5"));
    SetColumn(Lists::ActionParam6, 20, Translations::TranslateOrDefault("BatchTriggerViewP6", "P6"));

    Update(true);
}

void CBatchTrigger::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CBatchTrigger::m_hwnd = NULL;
    CBatchTrigger::m_parent = NULL;
}

BOOL CALLBACK CBatchTrigger::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CBatchTrigger::Initialize(hWnd);
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
    case WM_MOVE:
        ExtraWindow::UpdateDropTargetRect(hWnd);
        break;
    case WM_SIZE: {
        int newWndWidth = LOWORD(lParam);
        int newWndHeight = HIWORD(lParam);

        RECT rect;
        GetWindowRect(hListbox, &rect);

        POINT topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);

        int newWidth = rect.right - rect.left;
        int newHeight = rect.bottom - rect.top + newWndHeight - origWndHeight;
        MoveWindow(hListbox, topLeft.x, topLeft.y, newWidth, newHeight, TRUE);

        GetWindowRect(hListView, &rect);
        topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);
        newWidth = rect.right - rect.left + newWndWidth - origWndWidth;
        newHeight = rect.bottom - rect.top + newWndHeight - origWndHeight;
        MoveWindow(hListView, topLeft.x, topLeft.y, newWidth, newHeight, TRUE);

        origWndWidth = newWndWidth;
        origWndHeight = newWndHeight;

        ExtraWindow::UpdateDropTargetRect(hWnd);

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
        case Controls::Add:
            if (CODE == BN_CLICKED)
            {
                auto ret = GetListBoxSelected();
                for (int i : ret)
                {
                    if (auto id = (FString*)SendMessage(hListbox, LB_GETITEMDATA, i, NULL))
                    {
                        AddTrigger(*id);
                    }
                }
                AdjustColumnWidth();
            }
            break;
        case Controls::Delete:
            if (CODE == BN_CLICKED)
            {
                std::vector<int> selectedRows;
                int nItem = -1;
                while (true)
                {
                    nItem = ListView_GetNextItem(hListView, nItem, LVNI_SELECTED);
                    if (nItem == -1) break;
                    selectedRows.push_back(nItem);
                }

                std::sort(selectedRows.begin(), selectedRows.end());
                for (auto rit = selectedRows.rbegin(); rit != selectedRows.rend(); ++rit) {
                    char buf[512] = { 0 };
                    ListView_GetItemText(hListView, *rit, 0, buf, sizeof(buf));
                    FString ret(buf);
                    ret.Trim();
                    auto index = GetTriggerIndex(ret);
                    DeleteTrigger(index);
                }

                AdjustColumnWidth();
            }
            break;
        case Controls::AutoFill:
            if (CODE == BN_CLICKED)
                OnClickAutoFill();
            break;
        case Controls::MoveUp:
            if (CODE == BN_CLICKED)
                OnClickMove(true);
            break;
        case Controls::MoveDown:
            if (CODE == BN_CLICKED)
                OnClickMove(false);
            break;
        case Controls::UseID:
            if (CODE == BN_CLICKED)
                OnClickUseID();
            break;
        case Controls::EventIndex:
            if (CODE == EN_CHANGE)
            {
                char buffer[256]{};
                SendMessage(hEventIndex, WM_GETTEXT, (WPARAM)256, (LPARAM)buffer);
                CurrentEventIndex = atoi(buffer);
                for (int i = 0; i < ListedTriggerIDs.size(); ++i)
                    RefreshTrigger(i);
                AdjustColumnWidth();
            }
            break;
        case Controls::ActionIndex:
            if (CODE == EN_CHANGE)
            {
                char buffer[256]{};
                SendMessage(hActionIndex, WM_GETTEXT, (WPARAM)256, (LPARAM)buffer);
                CurrentActionIndex = atoi(buffer);
                for (int i = 0; i < ListedTriggerIDs.size(); ++i)
                    RefreshTrigger(i);
                AdjustColumnWidth();
            }
            break;
        case Controls::Search:
            if (CODE == EN_CHANGE)
                OnSearchEditChanged();
            break;
        default:
            break;
        }
        break;
    }
    case WM_NOTIFY:
    {
        LPNMHDR pNMHDR = (LPNMHDR)lParam;
        if (pNMHDR->idFrom == Controls::ListView && pNMHDR->code == NM_DBLCLK)
        {
            LPNMITEMACTIVATE pnmia = (LPNMITEMACTIVATE)pNMHDR;

            int row = pnmia->iItem;
            int col = pnmia->iSubItem;

            if (row < 0 || col < 1) break; 

            if (g_hInplaceEdit) EndInplaceEdit(true);

            char buf[512] = { 0 };
            ListView_GetItemText(hListView, row, col, buf, sizeof(buf));
            FString ret(buf);
            ret.Trim();
            g_hInplaceEdit = CreateInplaceEdit(hListView, row, col, ret);
            if (g_hInplaceEdit)
            {
                WNDPROC old = (WNDPROC)SetWindowLongPtr(g_hInplaceEdit, GWLP_WNDPROC, (LONG_PTR)InplaceEditProc);
                SetProp(g_hInplaceEdit, _T("OldProc"), (HANDLE)old);

                g_nEditRow = row;
                g_nEditCol = col;
                g_nEditOri = ret;
            }
            return 0;
        }
        else if (pNMHDR->hwndFrom == hListView && pNMHDR->code == NM_CUSTOMDRAW) {
            LPNMLVCUSTOMDRAW lvcd = (LPNMLVCUSTOMDRAW)lParam;

            switch (lvcd->nmcd.dwDrawStage) {
            case CDDS_PREPAINT:
                SetWindowLongPtr(hWnd, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
                return TRUE;

            case CDDS_ITEMPREPAINT:
                SetWindowLongPtr(hWnd, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
                return TRUE;

            case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
                int row = (int)lvcd->nmcd.dwItemSpec;
                int col = lvcd->iSubItem;

                for (const auto& cell : g_specialCells) {
                    if (cell.row == row && cell.col == col) {
                        lvcd->clrText = cell.fontColor;
                        SetWindowLongPtr(hWnd, DWLP_MSGRESULT, CDRF_NEWFONT);
                        return TRUE;
                    }
                    else
                    {
                        lvcd->clrText = ExtConfigs::EnableDarkMode ?
                            DarkTheme::MyGetSysColor(COLOR_WINDOWTEXT)
                            : GetSysColor(COLOR_WINDOWTEXT);
                    }
                }
                SetWindowLongPtr(hWnd, DWLP_MSGRESULT, CDRF_DODEFAULT);
                return TRUE;
            }

            default:
                SetWindowLongPtr(hWnd, DWLP_MSGRESULT, CDRF_DODEFAULT);
                return TRUE;
            }
        }
        break;
    }
    case WM_CLOSE:
    {
        CBatchTrigger::Close(hWnd);
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

LRESULT CALLBACK CBatchTrigger::ListBoxSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
    }
    return CallWindowProc(OriginalListBoxProc, hWnd, message, wParam, lParam);
}

void CBatchTrigger::ListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam)
{
    if (SendMessage(hListbox, LB_GETCOUNT, NULL, NULL) <= 0)
        return;

    switch (nCode)
    {
    case LBN_DBLCLK:
        OnDbClickListbox();
        break;
    default:
        break;
    }
}

LRESULT CALLBACK CBatchTrigger::InplaceEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CHAR:
        if (wParam == VK_RETURN)
        {
            EndInplaceEdit(true);
            return 0;
        }
        break;
    case WM_KILLFOCUS:
        EndInplaceEdit(true);
        return 0;

    case WM_DESTROY:
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)GetProp(hWnd, _T("OldProc")));
        RemoveProp(hWnd, _T("OldProc"));
        break;
    }

    WNDPROC oldProc = (WNDPROC)GetProp(hWnd, _T("OldProc"));
    return CallWindowProc(oldProc, hWnd, msg, wParam, lParam);
}

void CBatchTrigger::SetFontColor(HWND hListView, int row, int col, COLORREF color, const FString& oriValue, const FString& newValue)
{
    bool repeated = false;
    FString lastValue;
    g_specialCells.erase(std::remove_if(g_specialCells.begin(), g_specialCells.end(),
        [row, col, &lastValue, &repeated](CellColor& i) {
        if (i.row == row && i.col == col)
        {
            repeated = true;
            lastValue = i.oriValue;
            return true;
        }
        return false;
    }), g_specialCells.end());
    auto& origin = repeated ? lastValue : oriValue;
    //if (origin != newValue)
    //{
        g_specialCells.push_back({ row, col, color, origin });
    //}
    InvalidateRect(hListView, NULL, TRUE);
}

bool CBatchTrigger::GetCellRect(HWND hListView, int row, int col, RECT& outRect)
{
    if (!ListView_GetSubItemRect(hListView, row, col, LVIR_LABEL, &outRect))
        return false;

    POINT pt = { outRect.left, outRect.top };
    ClientToScreen(hListView, &pt);
    outRect.left = pt.x;
    outRect.top = pt.y;

    pt.x = outRect.right;
    pt.y = outRect.bottom;
    ClientToScreen(hListView, &pt);
    outRect.right = pt.x;
    outRect.bottom = pt.y;

    return true;
}

HWND CBatchTrigger::CreateInplaceEdit(HWND hListView, int row, int col, const char* initialText)
{
    RECT rc;
    if (!GetCellRect(hListView, row, col, rc)) return NULL;

    HWND hEdit = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        _T("EDIT"),
        nullptr,
        WS_POPUP | ES_AUTOHSCROLL,
        rc.left + 2, rc.top + 2,
        rc.right - rc.left, rc.bottom - rc.top,
        GetParent(hListView),
        nullptr,
        static_cast<HINSTANCE>(FA2sp::hInstance),
        nullptr
    );

    if (!hEdit) return NULL;

    ShowWindow(hEdit, SW_SHOW);

    SetWindowTextA(hEdit, initialText ? initialText : "");
    SendMessage(hEdit, EM_SETSEL, 0, -1);
    HFONT hFont = (HFONT)SendMessage(hListView, WM_GETFONT, 0, 0);
    SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(hEdit, EM_LIMITTEXT, 511, 0);
    SetFocus(hEdit);

    SetWindowLong(hEdit, GWL_EXSTYLE, GetWindowLong(hEdit, GWL_EXSTYLE) | WS_EX_CONTROLPARENT);

    return hEdit;
}

void CBatchTrigger::EndInplaceEdit(bool bSave)
{
    if (!g_hInplaceEdit || !IsWindow(g_hInplaceEdit)) return;

    if (bSave && g_nEditRow >= 0 && g_nEditCol >= 0)
    {
        char buf[512] = { 0 };
        GetWindowTextA(g_hInplaceEdit, buf, sizeof(buf) - 1);

        if (strlen(buf))
        {
            bool saved = true;
            LVITEM lvi = { 0 };
            lvi.mask = LVIF_TEXT;
            lvi.iItem = g_nEditRow;
            lvi.iSubItem = g_nEditCol;
            lvi.pszText = buf;
            SendMessage(hListView, LVM_SETITEM, 0, (LPARAM)&lvi);
            SaveTrigger(g_nEditRow, g_nEditCol, saved);
            if (!saved)
            {
                lvi.pszText = (char*)g_nEditOri.c_str();
                SendMessage(hListView, LVM_SETITEM, 0, (LPARAM)&lvi);
            }
            else
            {
                SetFontColor(hListView, g_nEditRow, g_nEditCol, RGB(255, 0, 0), g_nEditOri, buf);
            }
        }
    }

    DestroyWindow(g_hInplaceEdit);
    g_hInplaceEdit = NULL;
    g_nEditRow = -1;
    g_nEditCol = -1;
}

void CBatchTrigger::AddObject(const FString& id, const FString& name, ObjType type)
{
    if (name.empty()) return;

    objects.push_back({ id, name, type });
    ObjInfo* obj = &objects.back();

    idIndex[id] = obj;

    auto& index = [&]() -> std::unordered_map<FString, ObjInfo*>& {
        switch (type)
        {
        case ObjType::Trigger:   return triggerNameIndex;
        case ObjType::Tag:       return tagNameIndex;
        case ObjType::Team:      return teamNameIndex;
        }
        return triggerNameIndex;
    }();

    auto it = index.find(name);
    if (it == index.end() || atoi(obj->id) < atoi(it->second->id))
    {
        index[name] = obj;
    }
}

static bool isParam(int index)
{
    switch (index)
    {
    case CBatchTrigger::Lists::EventParam1:
    case CBatchTrigger::Lists::EventParam2:
    case CBatchTrigger::Lists::ActionNum:
    case CBatchTrigger::Lists::ActionParam1:
    case CBatchTrigger::Lists::ActionParam2:
    case CBatchTrigger::Lists::ActionParam3:
    case CBatchTrigger::Lists::ActionParam4:
    case CBatchTrigger::Lists::ActionParam5:
    case CBatchTrigger::Lists::ActionParam6:
        return true;

    default:
        break;
    }
    return false;
}

FString CBatchTrigger::IdToName(const FString& id)
{
    auto it = idIndex.find(id);
    return it == idIndex.end() ? FString{} : it->second->name;
}

FString CBatchTrigger::NameToId(const FString& name, ObjType forceType)
{
    if (forceType == ObjType::None) return name;
    const auto& index =
        (forceType == ObjType::Trigger) ? triggerNameIndex :
        (forceType == ObjType::Team) ? teamNameIndex :
        tagNameIndex;

    auto it2 = index.find(name);
    return it2 == index.end() ? FString{} : it2->second->id;
}

void CBatchTrigger::Update(bool afterInit, bool updateTrigger)
{
    IsUpdating = true;
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    FString buffer;
    buffer.Format("%d", CurrentActionIndex);
    SendMessage(hActionIndex, WM_SETTEXT, 0, buffer);
    buffer.Format("%d", CurrentEventIndex);
    SendMessage(hEventIndex, WM_SETTEXT, 0, buffer);
    SendMessage(hSearch, WM_SETTEXT, 0, (LPARAM)"");

    if (updateTrigger)
    {
        objects.clear();
        idIndex.clear();
        triggerNameIndex.clear();
        tagNameIndex.clear();
        teamNameIndex.clear();

        auto pTag = CINI::CurrentDocument().GetSection("Tags");
        auto pTrigger = CINI::CurrentDocument().GetSection("Triggers");
        auto pTeam = CINI::CurrentDocument().GetSection("TeamTypes");

        objects.reserve(
            (pTag ? pTag->GetEntities().size() : 0) + 
            (pTrigger ? pTrigger->GetEntities().size() : 0) +
            (pTeam ? pTeam->GetEntities().size() : 0)
        );

        CMapDataExt::Triggers.clear();
        std::map<FString, FString> TagMap;
        if (pTag)
        {
            for (auto& kvp : pTag->GetEntities())
            {
                auto tagAtoms = FString::SplitString(kvp.second);
                if (tagAtoms.size() < 3) continue;
                if (TagMap.find(tagAtoms[2]) == TagMap.end())
                    TagMap[tagAtoms[2]] = kvp.first;
                AddObject(kvp.first, tagAtoms[1], ObjType::Tag);
            }
        }
        if (pTrigger) {
            for (const auto& pair : pTrigger->GetEntities()) {
                std::shared_ptr<Trigger> trigger(Trigger::create(pair.first, &TagMap));
                if (!trigger) {
                    continue;
                }
                if (CMapDataExt::Triggers.find(pair.first) == CMapDataExt::Triggers.end()) {
                    AddObject(trigger->ID, trigger->Name, ObjType::Trigger);
                    CMapDataExt::Triggers[pair.first] = std::move(trigger);
                }
            }
        }
        if (pTeam) {
            for (const auto& pair : pTeam->GetEntities()) {
                AddObject(pair.second, map.GetString(pair.second, "Name"), ObjType::Team);
            }
        }

        while (SendMessage(hListbox, LB_DELETESTRING, 0, NULL) != CB_ERR);
        int idx = 0;
        bool displayId = false;
        std::vector<std::pair<FString, FString>> items;

        for (auto& [_, trigger] : CMapDataExt::Triggers) {
            FString label = displayId
                ? ExtraWindow::GetTriggerDisplayName(trigger->ID)
                : trigger->Name;

            items.emplace_back(label, trigger->ID);
        }

        bool tmp = ExtConfigs::SortByLabelName;
        ExtConfigs::SortByLabelName = ExtConfigs::SortByLabelName_Trigger;

        std::sort(items.begin(), items.end(),
            [](const auto& a, const auto& b) {
            return ExtraWindow::SortRawStrings(a.first, b.first);
        });

        ExtConfigs::SortByLabelName = tmp;

        ListboxTriggerID.clear();
        for (auto& [label, id] : items)
        {
            SendMessage(hListbox, LB_INSERTSTRING, idx, label);
            auto& data = ListboxTriggerID.emplace_back(id);
            SendMessage(hListbox, LB_SETITEMDATA, idx, (LPARAM)&data);
            idx++;
        }
    }

    SendMessage(hUseID, BM_SETCHECK, bUseID ? BST_CHECKED : BST_UNCHECKED, 0);

    std::vector<FString> listedTriggers;
    if (NeedClear)
    {
        NeedClear = false;
    }
    else
    {
        listedTriggers = ListedTriggerIDs;
    }
    g_specialCells.clear();
    while (DeleteTrigger(0));

    for (int i = 0; i < listedTriggers.size(); ++i)
    {
        AddTrigger(listedTriggers[i]);
    }

    IsUpdating = false;
    AdjustColumnWidth();

    for (int i = 0; i < TRIGGER_EDITOR_MAX_COUNT; ++i)
    {
        auto o = &CNewTrigger::Instance[i];
        if (o->CurrentTrigger)
        {
            o->CurrentTrigger = CMapDataExt::GetTrigger(o->CurrentTriggerID);
        }
    }
    return;
}

void CBatchTrigger::OnViewerSelectedChange(LPNMHDR pNMHDR)
{
    LPNMLISTVIEW pNMListView = (LPNMLISTVIEW)pNMHDR;
    if ((pNMListView->uChanged & LVIF_STATE) &&
        (pNMListView->uNewState & LVIS_SELECTED))
    {
        int selectedRow = pNMListView->iItem;
    }
}

void CBatchTrigger::OnDbClickListbox()
{
    if (SendMessage(hListbox, LB_GETCOUNT, NULL, NULL) <= 0)
    {
        return;
    }

    POINT pt;
    GetCursorPos(&pt);

    ScreenToClient(hListbox, &pt); 
    LRESULT lr = SendMessage(hListbox, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));

    int nItem = LOWORD(lr);
    BOOL bOutside = HIWORD(lr);

    if (bOutside == 0 && nItem >= 0)
    {
        if (auto id = (FString*)SendMessage(hListbox, LB_GETITEMDATA, nItem, NULL))
        {
            AddTrigger(*id);
            AdjustColumnWidth();
        }    
    }
}

static bool IsAllDigits(const std::string& s)
{
    if (s.empty()) return false;
    for (char c : s)
        if (!std::isdigit(static_cast<unsigned char>(c)))
            return false;
    return true;
}

static bool IsDigit(char c)
{
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

static std::vector<std::string> AutoFillByPattern(const std::string& a, const std::string& b, size_t count)
{
    std::vector<std::string> result;

    if (count < 2)
        return result;

    if (a == b)
    {
        for (size_t i = 0; i < count; ++i)
            result.push_back(a);
        return result;
    }

    if (IsAllDigits(a) && IsAllDigits(b))
    {
        int valueA = std::stoi(a);
        int valueB = std::stoi(b);
        int step = valueB - valueA;
        FString format;
        format.Format("0%d", a.size());
        format = "%" + format + "d";
        for (size_t i = 0; i < count; ++i)
        {
            int value = valueA + static_cast<int>(i) * step;
            FString num;
            num.Format(format, value);
            result.push_back(num);
        }
        return result;
    }

    if (a.size() != b.size())
        return result;

    if (IsAllDigits(a) && IsAllDigits(b))
    {
        int valueA = std::stoi(a);
        int valueB = std::stoi(b);
        int step = valueB - valueA;
        FString format;
        format.Format("0%d", a.size());
        format = "%" + format + "d";
        for (size_t i = 0; i < count; ++i)
        {
            int value = valueA + static_cast<int>(i) * step;
            FString num;
            num.Format(format, value);
            result.push_back(num);
        }
        return result;
    }
    const size_t n = a.size();

    size_t L = 0;
    while (L < n && a[L] == b[L])
        ++L;

    if (L == n)
        return result;

    size_t R = n - 1;
    while (R > L && a[R] == b[R])
        --R;

    if (IsDigit(a[L]) || IsDigit(b[L]))
    {
        while (L > 0 && IsDigit(a[L - 1]) && IsDigit(b[L - 1]))
            --L;

        while (R + 1 < n && IsDigit(a[R + 1]) && IsDigit(b[R + 1]))
            ++R;
    }

    for (size_t i = L; i <= R; ++i)
    {
        if (!IsDigit(a[i]) || !IsDigit(b[i]))
            return {};
    }

    std::string numA = a.substr(L, R - L + 1);
    std::string numB = b.substr(L, R - L + 1);

    if (!IsAllDigits(numA) || !IsAllDigits(numB))
        return {};

    int valueA = std::stoi(numA);
    int valueB = std::stoi(numB);
    int step = valueB - valueA;

    if (step == 0)
        return {};

    const size_t width = numA.size();

    for (size_t i = 0; i < count; ++i)
    {
        int value = valueA + static_cast<int>(i) * step;

        std::string num = std::to_string(value);

        if (num.size() < width)
            num.insert(num.begin(), width - num.size(), '0');

        std::string s;
        s.reserve(n + 8);

        s.append(a, 0, L); 
        s.append(num);
        s.append(a, R + 1, n - R - 1); 

        result.push_back(std::move(s));
    }

    return result;
}

void CBatchTrigger::OnClickAutoFill()
{
    std::map<int, std::map<int, CellColor>> cells;
    int nRowCount = SendMessage(hListView, LVM_GETITEMCOUNT, 0, 0);
    auto getValue = [](int row, int col)
    {
        char buf[512] = { 0 };
        ListView_GetItemText(hListView, row, col, buf, sizeof(buf));
        FString ret(buf);
        ret.Trim();
        return ret;
    };
    for (auto& c : g_specialCells)
    {
        auto& c2 = cells[c.col][c.row];
        c2 = c;
        c2.oriValue = getValue(c.row, c.col);
    }
    for (auto& [col, colCells] : cells)
    {
        int firstLine = -1;
        CellColor* reference[2]{};
        for (auto& [row, rowCell] : colCells)
        {
            if (firstLine == -1)
            {
                firstLine = row;
                reference[0] = &rowCell;
            }
            else if (row == firstLine + 1)
            {
                reference[1] = &rowCell;
            }
        }
        if (reference[0] && reference[1])
        {
            int size = nRowCount - firstLine;
            auto result = AutoFillByPattern(reference[0]->oriValue, reference[1]->oriValue, size);

            if (result.size() == size)
            {
                for (int row = firstLine; row < nRowCount; ++row)
                {
                    bool saved = true;
                    auto ori = getValue(row, col);
                    LVITEM lvi = { 0 };
                    lvi.mask = LVIF_TEXT;
                    lvi.iItem = row;
                    lvi.iSubItem = col;
                    lvi.pszText = (char*)result[row - firstLine].c_str();
                    SendMessage(hListView, LVM_SETITEM, 0, (LPARAM)&lvi);
                    SaveTrigger(row, col, saved);
                    if (!saved)
                    {
                        lvi.pszText = (char*)ori.c_str();
                        SendMessage(hListView, LVM_SETITEM, 0, (LPARAM)&lvi);
                    }
                }
                g_specialCells.erase(std::remove_if(g_specialCells.begin(), g_specialCells.end(),
                    [col](CellColor& i) {
                    return i.col == col;
                }), g_specialCells.end());
            }
        }
    }
    AdjustColumnWidth();
}

void CBatchTrigger::OnClickUseID()
{
    bUseID = SendMessage(hUseID, BM_GETCHECK, 0, 0);
    Update(false, false);
}

void CBatchTrigger::SelectListViewRows(const std::vector<int>& indices)
{
    int totalRows = (int)SendMessage(hListView, LVM_GETITEMCOUNT, 0, 0);
    if (totalRows <= 0)
        return;

    ListView_SetItemState(hListView, -1, 0, LVIS_SELECTED);
    int successCount = 0;

    for (int row : indices)
    {
        if (row < 0 || row >= totalRows)
            continue;

        ListView_SetItemState(hListView, row,
            LVIS_SELECTED | LVIS_FOCUSED,
            LVIS_SELECTED | LVIS_FOCUSED);
    }
    SetFocus(hListView);
}

void CBatchTrigger::OnClickMove(bool isUp)
{
    int nRowCount = SendMessage(hListView, LVM_GETITEMCOUNT, 0, 0);
    std::vector<int> selected;
    int nItem = -1;
    while (true)
    {
        nItem = ListView_GetNextItem(hListView, nItem, LVNI_SELECTED);
        if (nItem == -1) break;
        selected.push_back(nItem);
    }

    std::sort(selected.begin(), selected.end());

    for (auto& i : selected)
    {
        if (isUp && i == 0) return;
        else if (!isUp && i == nRowCount - 1) return;
    }

    if (isUp)
    {
        for (auto it = selected.begin(); it != selected.end(); ++it)
        {
            if (*it > 0 && *it < ListedTriggerIDs.size()) {
                std::swap(ListedTriggerIDs[*it], ListedTriggerIDs[*it - 1]);
                RefreshTrigger(*it);
                RefreshTrigger(*it - 1);
            }
        }
    }
    else
    {
        for (auto it = selected.rbegin(); it != selected.rend(); ++it)
        {
            if (*it >= 0 && *it < ListedTriggerIDs.size() - 1) {
                std::swap(ListedTriggerIDs[*it + 1], ListedTriggerIDs[*it]);
                RefreshTrigger(*it + 1);
                RefreshTrigger(*it);
            }
        }
    }

    for (auto& i : selected)
    {
        if (isUp) i--;
        else i++;
    }
    if (!selected.empty())
        SelectListViewRows(selected);
}

void CBatchTrigger::OnSearchEditChanged()
{
    if (IsUpdating) return;
    char buffer[512]{};
    SendMessage(hSearch, WM_GETTEXT, (WPARAM)512, (LPARAM)buffer);

    while (SendMessage(hListbox, LB_DELETESTRING, 0, NULL) != CB_ERR);
   
    int idx = 0;
    for (const auto& id : ListboxTriggerID)
    {
        auto name = FString::SplitString(map.GetString("Triggers", id, "Americans,<none>,MISSING,0,1,1,1,0"), 2)[2];
        if (!strlen(buffer) || ExtraWindow::IsLabelMatch(id, buffer) || ExtraWindow::IsLabelMatch(name, buffer))
        {
            SendMessage(hListbox, LB_INSERTSTRING, idx, name);
            SendMessage(hListbox, LB_SETITEMDATA, idx, (LPARAM)&id);
            idx++;
        }
    }
}

void CBatchTrigger::OnDroppedIntoCell(int row, int col, const FString& value)
{
    int totalRows = (int)SendMessage(hListView, LVM_GETITEMCOUNT, 0, 0);
    auto& vec = ListedTriggerIDs;
    if (row < 0 || row >= vec.size())
        return;
    auto& id = ListedTriggerIDs[row];
    if (auto trigger = CMapDataExt::GetTrigger(id))
    {
        auto setValue = [&]()
        {
            trigger->Save();
            RefreshTrigger(row);
        };
        EventParams tmpe{};
        ActionParams tmpa{};
        auto& e = trigger->EventCount > CurrentEventIndex ? trigger->Events[CurrentEventIndex] : tmpe;
        auto& a = trigger->ActionCount > CurrentActionIndex ? trigger->Actions[CurrentActionIndex] : tmpa;
        switch (col)
        {
        case Lists::EventParam1:
            e.Params[1] = value;
            setValue();
            break;
        case Lists::EventParam2:
            e.Params[2] = value;
            setValue();
            break;
        case Lists::ActionParam1:
            a.Params[1] = value;
            setValue();
            break;
        case Lists::ActionParam2:
            a.Params[2] = value;
            setValue();
            break;
        case Lists::ActionParam3:
            a.Params[3] = value;
            setValue();
            break;
        case Lists::ActionParam4:
            a.Params[4] = value;
            setValue();
            break;
        case Lists::ActionParam5:
            a.Params[5] = value;
            setValue();
            break;
        case Lists::ActionParam6:
            a.Params[6] = value;
            setValue();
            break;
        default:
            break;
        }
    }
}

void CBatchTrigger::AddTrigger(const FString& ID)
{
    if (std::find(ListedTriggerIDs.begin(), ListedTriggerIDs.end(), ID) != ListedTriggerIDs.end())
        return;
    auto itr = CMapDataExt::Triggers.find(ID);
    if (itr == CMapDataExt::Triggers.end())
        return;

    const auto& trigger = itr->second;

    ListedTriggerIDs.push_back(trigger->ID);
    int nRowCount = SendMessage(hListView, LVM_GETITEMCOUNT, 0, 0);
    InsertTrigger(nRowCount, trigger);
}

void CBatchTrigger::InsertTrigger(int index, std::shared_ptr<Trigger> trigger)
{
    auto InsertItem = [index](int idx, const FString& Label, bool p6IsWp = true)
    {
        FString text;
        bool isP6 = idx == Lists::ActionParam6;
        if (!bUseID && isP6 && p6IsWp)
        {
            text = STDHelpers::StringToWaypointStr(Label);
        }
        else if (!bUseID && isParam(idx))
        {
            text = IdToName(Label);
            if (text.empty())
                text = Label;
        }
        else
        {
            text = Label;
        }
        LVITEM lvItem = { 0 };
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = index;
        lvItem.iSubItem = idx;

        lvItem.pszText = const_cast<LPSTR>(text.c_str());

        SendMessage(hListView, idx == 0 ? LVM_INSERTITEM : LVM_SETITEM, 0, (LPARAM)&lvItem);
    };

    g_specialCells.erase(std::remove_if(g_specialCells.begin(), g_specialCells.end(),
        [index](CellColor& i) { return i.row == index; }),
        g_specialCells.end());

    const auto& e = trigger->EventCount <= CurrentEventIndex ? EventParams{} : trigger->Events[CurrentEventIndex];
    const auto& a = trigger->ActionCount <= CurrentActionIndex ? ActionParams{} : trigger->Actions[CurrentActionIndex];

    InsertItem(Lists::ID, trigger->ID);
    InsertItem(Lists::Name, trigger->Name);
    InsertItem(Lists::EventNum, e.EventNum);
    // param0 is bound to event type, not editable
    InsertItem(Lists::EventParam1, e.Params[1]);
    InsertItem(Lists::EventParam2, e.P3Enabled ? e.Params[2] : FString(""));
    InsertItem(Lists::ActionNum, a.ActionNum);
    // param0 is bound to action type, not editable
    InsertItem(Lists::ActionParam1, a.Params[1], a.Param7isWP);
    InsertItem(Lists::ActionParam2, a.Params[2], a.Param7isWP);
    InsertItem(Lists::ActionParam3, a.Params[3], a.Param7isWP);
    InsertItem(Lists::ActionParam4, a.Params[4], a.Param7isWP);
    InsertItem(Lists::ActionParam5, a.Params[5], a.Param7isWP);
    InsertItem(Lists::ActionParam6, a.Params[6], a.Param7isWP);
}

bool CBatchTrigger::DeleteTrigger(int index)
{
    int totalRows = (int)SendMessage(hListView, LVM_GETITEMCOUNT, 0, 0);
    auto& vec = ListedTriggerIDs;
    if (index < 0 || index >= vec.size())
        return false;
    vec.erase(vec.begin() + index);
    if (index >= totalRows)
        return true;

    SendMessage(hListView, LVM_DELETEITEM, index, 0);
    g_specialCells.erase(std::remove_if(g_specialCells.begin(), g_specialCells.end(),
        [index](CellColor& i) { return i.row == index; }),
        g_specialCells.end());
    for (auto& c : g_specialCells)
        if (c.row > index)
            c.row--;
    return true;
}

void CBatchTrigger::RefreshTrigger(int index)
{
    int totalRows = (int)SendMessage(hListView, LVM_GETITEMCOUNT, 0, 0);
    auto& vec = ListedTriggerIDs;
    if (index < 0 || index >= totalRows || index >= vec.size())
        return;

    auto& ID = vec[index];
    auto itr = CMapDataExt::Triggers.find(ID);
    if (itr == CMapDataExt::Triggers.end())
    {
        DeleteTrigger(index);
        return;
    }

    const auto& trigger = itr->second;

    SendMessage(hListView, LVM_DELETEITEM, index, 0);
    InsertTrigger(index, trigger);
}

void CBatchTrigger::SaveTrigger(int row, int col, bool& changed)
{
    changed = true;
    auto getValue = [row, &changed](int col, const FString* oriValue = nullptr,
        const FString* num = nullptr, bool eventHasP3 = false, bool p6IsWp = true)
    {
        ObjType type = ObjType::None;
        bool is_param = isParam(col);
        bool isP6 = col == Lists::ActionParam6;
        auto getParamType = [&type](const FString& value)
        {
            if (value == "9")
            {
                type = ObjType::Trigger;
            }
            else if (value == "11")
            {
                type = ObjType::Tag;
            }
            else if (value == "15")
            {
                type = ObjType::Team;
            }
            else if (CINI::FAData->KeyExists("NewParamTypes", value))
            {
                auto newParamTypes = FString::SplitString(CINI::FAData->GetString("NewParamTypes", value), 4);
                auto& sectionName = newParamTypes[0];
                auto& loadFrom = newParamTypes[1];

                if (sectionName == "Triggers")
                {
                    type = ObjType::Trigger;
                }
                else if (sectionName == "Tags")
                {
                    type = ObjType::Trigger;
                }
                else if (sectionName == "TeamTypes")
                {
                    type = ObjType::Team;
                }
            }
        };
        if (num && is_param && col < ActionNum)
        {
            bool isFirstP = col == EventParam1;
            int index = 2;
            if (isFirstP && eventHasP3)
                index = 1;

            auto value = CINI::FAData->GetString(ExtraWindow::GetTranslatedSectionName("EventsRA2"), *num);
            auto eventInfos = FString::SplitString(value, 8);
            FString paramType = eventInfos[index];

            std::vector<FString> pParamTypes =
                FString::SplitString(
                    CINI::FAData->GetString(
                        ExtraWindow::GetTranslatedSectionName("ParamTypes"), paramType, "MISSING,0"), 1);

            getParamType(pParamTypes[1]);
        }
        else if (num && is_param)
        {
            int index = col - ActionParam1 + 1;
            auto value = CINI::FAData->GetString(ExtraWindow::GetTranslatedSectionName("ActionsRA2"), *num);
            auto actionInfos = FString::SplitString(value, 13);
            FString paramType = actionInfos[index + 1];

            std::vector<FString> pParamTypes =
                FString::SplitString(
                    CINI::FAData->GetString(
                        ExtraWindow::GetTranslatedSectionName("ParamTypes"), paramType, "MISSING,0"), 1);

            getParamType(pParamTypes[1]);
        }

        char buf[512] = { 0 };
        ListView_GetItemText(hListView, row, col, buf, sizeof(buf));
        FString ret(buf);
        ret.Trim();
        if (!bUseID && isP6 && p6IsWp)
        {
            ret = STDHelpers::WaypointToString(ret);
        }
        else if (!bUseID && is_param && oriValue)
        {
            ret = NameToId(ret, type);
            if (ret.empty())
            {
                changed = false;
                ret = *oriValue;
            }
        }
        return ret;
    };
    FString buffer;
    auto ID = getValue(Lists::ID);
    auto itr = CMapDataExt::Triggers.find(ID);
    if (itr == CMapDataExt::Triggers.end())
    {
        DeleteTrigger(row);
        return;
    }
    auto& trigger = itr->second;
    trigger->Name = getValue(Lists::Name);
    if (trigger->EventCount > CurrentEventIndex)
    {
        auto& e = trigger->Events[CurrentEventIndex];
        if (col == Lists::EventNum)
        {
            e.EventNum = getValue(Lists::EventNum);
            auto eventInfos = FString::SplitString(
                CINI::FAData->GetString(
                    ExtraWindow::GetTranslatedSectionName("EventsRA2"), 
                    e.EventNum, "MISSING,0,0,0,0,MISSING,0,1,0"), 8);

            int param0 = atoi(eventInfos[1]);
            auto pParamTypes0 = FString::SplitString(
                CINI::FAData->GetString(
                    ExtraWindow::GetTranslatedSectionName("ParamTypes"),
                    eventInfos[1], "MISSING,0"));
            
            FString code = "0";
            if (pParamTypes0.size() == 3) 
                code = pParamTypes0[2];
            if (param0 > 0)
            {
                e.Params[0] = code;
            }
            else
            {
                buffer.Format("%d", -param0);
                e.Params[0] = buffer;
            }

            e.P3Enabled = e.Params[0] == "2";
        }
        else if (col == Lists::EventParam1) e.Params[1] = getValue(Lists::EventParam1, &e.Params[1], &e.EventNum, e.P3Enabled);
        else if (col == Lists::EventParam2) e.Params[2] = getValue(Lists::EventParam2, &e.Params[2], &e.EventNum, e.P3Enabled);
    }
    else if (col >= Lists::EventNum && col <= Lists::EventParam2)
    {
        changed = false;
        return;
    }
    if (trigger->ActionCount > CurrentActionIndex)
    {
        auto& a = trigger->Actions[CurrentActionIndex];

        if (col == Lists::ActionNum)
        {
            a.ActionNum = getValue(Lists::ActionNum);
            auto actionInfos = FString::SplitString(
                CINI::FAData->GetString(
                    ExtraWindow::GetTranslatedSectionName("ActionsRA2"),
                    a.ActionNum, "MISSING,0,0,0,0,0,0,0,0,0,MISSING,0,1,0"), 13);

            auto param0 = atoi(actionInfos[1]);
            a.Param7isWP = true;
            for (auto& pair : CINI::FAData->GetSection("DontSaveAsWP")->GetEntities())
            {
                if (atoi(pair.second) == -param0)
                    a.Param7isWP = false;
            }
            if (param0 < 0)
            {
                buffer.Format("%d", -param0);
                a.Params[0] = buffer;
            }
        }
        else if (col == Lists::ActionParam1) a.Params[1] = getValue(Lists::ActionParam1, &a.Params[1], &a.ActionNum, false, a.Param7isWP);
        else if (col == Lists::ActionParam2) a.Params[2] = getValue(Lists::ActionParam2, &a.Params[2], &a.ActionNum, false, a.Param7isWP);
        else if (col == Lists::ActionParam3) a.Params[3] = getValue(Lists::ActionParam3, &a.Params[3], &a.ActionNum, false, a.Param7isWP);
        else if (col == Lists::ActionParam4) a.Params[4] = getValue(Lists::ActionParam4, &a.Params[4], &a.ActionNum, false, a.Param7isWP);
        else if (col == Lists::ActionParam5) a.Params[5] = getValue(Lists::ActionParam5, &a.Params[5], &a.ActionNum, false, a.Param7isWP);
        else if (col == Lists::ActionParam6) a.Params[6] = getValue(Lists::ActionParam6, &a.Params[6], &a.ActionNum, false, a.Param7isWP);

    }
    else if (col >= Lists::ActionNum && col <= Lists::ActionParam6)
    {
        changed = false;
        return;
    }
    for (int i = 0; i < TRIGGER_EDITOR_MAX_COUNT; ++i)
    {
        auto o = &CNewTrigger::Instance[i];
        if (o->CurrentTriggerID == trigger->ID && o->GetHandle())
        {
            TempValueHolder<bool> tmp(o->DropNeedUpdate, true);
            int indexE = o->SelectedEventIndex;
            int indexA = o->SelectedActionIndex;
            o->OnSelchangeTrigger(false,
                o->SelectedEventIndex,
                o->SelectedActionIndex,
                false);
        }
    }

    trigger->Save();
}

int CBatchTrigger::GetTriggerIndex(const FString& ID)
{
    for (int i = 0; i < ListedTriggerIDs.size(); ++i)
    {
        if (ListedTriggerIDs[i] == ID)
            return i;
    }
    return -1;
}

std::vector<int> CBatchTrigger::GetListBoxSelected()
{
    int numSelected = SendMessage(hListbox, LB_GETSELCOUNT, 0, 0);
    std::vector<int> ret(numSelected);
    SendMessage(hListbox, LB_GETSELITEMS, numSelected, (LPARAM)ret.data());
    std::sort(ret.begin(), ret.end());
    return ret;
}

void CBatchTrigger::AdjustColumnWidth()
{
    if (IsUpdating) return;

    for (int col = 0; col < Lists::Count; ++col)
    {
        SendMessage(
            hListView,
            LVM_SETCOLUMNWIDTH,
            col,
            LVSCW_AUTOSIZE_USEHEADER
        );
    }
}