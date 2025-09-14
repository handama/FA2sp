#include "COptions.h"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include <CFinalSunDlg.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CMapData/Body.h"
#include "../../Miscs/SaveMap.h"
#include "../Common.h"
#include "../../Miscs/DialogStyle.h"

HWND COptions::m_hwnd;
HWND COptions::hList;
HWND COptions::hGameEngine;
HWND COptions::hSearch;
CFinalSunDlg* COptions::m_parent;
bool COptions::initialized = false;
WNDPROC COptions::g_pOriginalListViewProc = nullptr;

LRESULT CALLBACK COptions::ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DarkTheme::MyCallWindowProcA(g_pOriginalListViewProc, hWnd, uMsg, wParam, lParam);
}

void COptions::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(325),
        pWnd->MyViewFrame.GetSafeHwnd(),
        COptions::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create COptions.\n");
        m_parent = NULL;
    }
}

void COptions::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("Options.Title", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](const char* pLabelName, int nIDDlgItem)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };

    Translate("Options.Search", 1003);

    hList = GetDlgItem(hWnd, Controls::List);
    hSearch = GetDlgItem(hWnd, Controls::Search);

    if (ExtConfigs::EnableDarkMode)
    {
        ::SendMessage(hList, LVM_SETTEXTBKCOLOR, 0, RGB(32, 32, 32));
        ::SendMessage(hList, LVM_SETTEXTCOLOR, 0, RGB(220, 220, 220));

        DarkTheme::SubclassListViewHeader(hList);
        g_pOriginalListViewProc = (WNDPROC)GetWindowLongPtr(hList, GWLP_WNDPROC);
        if (g_pOriginalListViewProc)
        {
            SetWindowLongPtr(hList, GWLP_WNDPROC, (LONG_PTR)ListViewSubclassProc);
        }
        InvalidateRect(hList, NULL, TRUE);
    }

    Update();
}

void COptions::Update(const char* filter)
{
    initialized = false;
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    CINI fa2;
    FString path = CFinalSunApp::ExePath();
    path += "\\FinalAlert.ini";
    fa2.ClearAndLoad(path);

    for (const auto& opt : ExtConfigs::Options)
    {
        *opt.Value = fa2.GetBool("Options", opt.IniKey, *opt.Value);
    }

    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
    ppmfc::CString title = Translations::TranslateOrDefault("Options.Label", "Options");
    ListView_DeleteAllItems(hList);
    int nColumnCount = Header_GetItemCount(ListView_GetHeader(hList));
    for (int i = nColumnCount - 1; i >= 0; --i) {
        ListView_DeleteColumn(hList, i);
    }

    LVCOLUMN lvc = { 0 };
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    lvc.fmt = LVCFMT_LEFT;
    RECT rcClient = {};
    GetClientRect(hList, &rcClient);
    int fullWidth = rcClient.right - rcClient.left;
    lvc.cx = fullWidth;
    lvc.pszText = title.m_pchData;
    ListView_InsertColumn(hList, 0, &lvc);

    int index = 0;
    for (const auto& opt : ExtConfigs::Options)
    {
        if (filter && strlen(filter))
        {
            if (!ExtraWindow::IsLabelMatch(opt.DisplayName, filter) && !ExtraWindow::IsLabelMatch(opt.IniKey, filter))
                continue;
        }
        LVITEM lvi = { 0 };
        lvi.mask = LVIF_TEXT;
        lvi.iItem = index;
        lvi.pszText = (char*)opt.DisplayName.c_str();
        ListView_InsertItem(hList, &lvi);
        ListView_SetCheckState(hList, index, *opt.Value ? TRUE : FALSE);
        index++;
    }
    initialized = true;
    return;
}


void COptions::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    COptions::m_hwnd = NULL;
    COptions::m_parent = NULL;

}

BOOL CALLBACK COptions::DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        COptions::Initialize(hwnd);
        return TRUE;
    }
    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->idFrom == Controls::List && pnmh->code == LVN_ITEMCHANGED && initialized)
        {
            NMLISTVIEW* pnmv = (NMLISTVIEW*)lParam;
            if ((pnmv->uChanged & LVIF_STATE) &&
                ((pnmv->uNewState ^ pnmv->uOldState) & LVIS_STATEIMAGEMASK))
            {
                int index = pnmv->iItem;
                if (index >= 0)
                {
                    TCHAR buf[256] = { 0 };
                    ListView_GetItemText(hList, index, 0, buf, 255);
                    BOOL checked = ListView_GetCheckState(hList, index);

                    CINI fa2;
                    std::string path;
                    path = CFinalSunApp::ExePath;
                    path += "\\FinalAlert.ini";
                    fa2.ClearAndLoad(path.c_str());

                    for (const auto& opt : ExtConfigs::Options)
                    {
                        if (opt.DisplayName == buf)
                        {
                            *opt.Value = (checked != FALSE);
                            fa2.WriteBool("Options", opt.IniKey, *opt.Value);

                            if (opt.Type == ExtConfigs::SpecialOptionType::ReloadMap && CMapData::Instance->MapWidthPlusHeight)
                            {
                                MessageBox(m_parent->m_hWnd,
                                    Translations::TranslateOrDefault("Options.ReloadMap", "Reload map to apply this change."),
                                    "FA2sp", MB_OK | MB_ICONWARNING);
                            }
                            else if (opt.Type == ExtConfigs::SpecialOptionType::Restart)
                            {
                                MessageBox(m_parent->m_hWnd,
                                    Translations::TranslateOrDefault("Options.Restart", "Restart FA2 to apply this change."),
                                    "FA2sp", MB_OK | MB_ICONWARNING);
                            }
                            else if (opt.Type == ExtConfigs::SpecialOptionType::SaveMap_Timer)
                            {
                                if (!*opt.Value)
                                {
                                    ExtConfigs::SaveMap_AutoSave_Interval = -1;
                                }
                                else
                                {
                                    ExtConfigs::SaveMap_AutoSave_Interval = ExtConfigs::SaveMap_AutoSave_Interval_Real;
                                }
                                SaveMapExt::ResetTimer();
                            }
                        }
                    }
                    fa2.WriteToFile(path.c_str());
                }
            }
        }
        break;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::Search:
            if (CODE == EN_CHANGE)
                OnEditchangeSearch();
            break;
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        COptions::Close(hwnd);
        return TRUE;
    }
    case 114514: // used for update
    {
        OnEditchangeSearch();
        return TRUE;
    }
    }

    // Process this message through default handler
    return FALSE;
}

void COptions::OnEditchangeSearch()
{
    char buffer[512]{ 0 };
    GetWindowText(hSearch, buffer, 511);
    Update(buffer);
}