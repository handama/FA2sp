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

HWND CBatchTrigger::m_hwnd;
CFinalSunDlg* CBatchTrigger::m_parent;
CINI& CBatchTrigger::map = CINI::CurrentDocument;
MultimapHelper& CBatchTrigger::rules = Variables::RulesMap;

HWND CBatchTrigger::hListbox;
HWND CBatchTrigger::hListView;
int CBatchTrigger::origWndWidth;
int CBatchTrigger::origWndHeight;
int CBatchTrigger::minWndWidth;
int CBatchTrigger::minWndHeight;
bool CBatchTrigger::minSizeSet;
WNDPROC CBatchTrigger::OriginalListBoxProc;
WNDPROC CBatchTrigger::g_pOriginalListViewProc = nullptr;
std::list<FString> CBatchTrigger::ListboxTriggerID;

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
    hListView = GetDlgItem(hWnd, Controls::ListView);

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

    Update();
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
        default:
            break;
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
        auto id = (FString*)SendMessage(hListbox, LB_GETITEMDATA, nItem, NULL);
        MessageBox(NULL, *id, "", NULL);
    }
}

void CBatchTrigger::Update()
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

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
        return ExtraWindow::SortLabels(a.first, b.first);
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
  
    return;
}
