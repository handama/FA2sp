#include <Helpers/Macro.h>
#include "../../Helpers/WinVer.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/TheaterHelpers.h"
#include "../CMapData/Body.h"
#include "Body.h"
#include "../CFinalSunApp/Body.h"

static std::map<HWND, WNDPROC> g_OriginalProcs;
static bool g_LayoutLocked;
static int vaildToolbars = 0;

struct ToolbarSet {
    HWND hTbA = NULL;
    HWND hTbB = NULL;
    HWND hTbC = NULL;
    HWND hTbD = NULL;
};
static ToolbarSet g_Toolbars;

void SaveConfigIni(const char* lpSection, const char* lpKey, const char* lpValue)
{
    CINI ini;
    ppmfc::CString path = CFinalSunAppExt::ExePathExt;
    path += "\\FinalAlert.ini";
    ini.ClearAndLoad(path);
    ini.WriteString(lpSection, lpKey, lpValue);
    ini.WriteToFile(path);
}

FString GetConfigIni(const char* lpSection, const char* lpKey, const char* lpDefault = "")
{
    CINI ini;
    ppmfc::CString path = CFinalSunAppExt::ExePathExt;
    path += "\\FinalAlert.ini";
    ini.ClearAndLoad(path);
    return ini.GetString(lpSection, lpKey, lpDefault);
}

BOOL RemoveToolbarFromReBar(HWND hReBar, HWND hTargetToolbar)
{
    if (!hReBar || !hTargetToolbar) return FALSE;

    int bandCount = (int)SendMessage(hReBar, RB_GETBANDCOUNT, 0, 0);
    if (bandCount <= 0) return FALSE;

    for (int i = 0; i < bandCount; i++)
    {
        REBARBANDINFO rbi = { 0 };
        rbi.cbSize = sizeof(REBARBANDINFO);
        rbi.fMask = RBBIM_CHILD;

        if (SendMessage(hReBar, RB_GETBANDINFO, (WPARAM)i, (LPARAM)&rbi))
        {
            if (rbi.hwndChild == hTargetToolbar)
            {
                SendMessage(hReBar, RB_SHOWBAND, (WPARAM)i, (LPARAM)FALSE);
                LRESULT res = SendMessage(hReBar, RB_DELETEBAND, (WPARAM)i, 0);
                SendMessage(hReBar, WM_SIZE, 0, 0);
                vaildToolbars--;

                return (res != 0);
            }
        }
    }
    return FALSE;
}

void LockReBarBands(HWND hReBar, bool bLock)
{
    int nBandCount = (int)SendMessage(hReBar, RB_GETBANDCOUNT, 0, 0);
    if (nBandCount <= 0) return;

    for (int i = 0; i < nBandCount; i++)
    {
        REBARBANDINFO rbbi = { 0 };
        rbbi.cbSize = sizeof(REBARBANDINFO);
        rbbi.fMask = RBBIM_STYLE;

        SendMessage(hReBar, RB_GETBANDINFO, (WPARAM)i, (LPARAM)&rbbi);

        if (bLock)
        {
            rbbi.fStyle |= RBBS_NOGRIPPER;
            rbbi.fStyle &= ~RBBS_GRIPPERALWAYS;
        }
        else
        {
            rbbi.fStyle &= ~(RBBS_NOGRIPPER);
            rbbi.fStyle |= RBBS_GRIPPERALWAYS;
        }

        rbbi.fMask = RBBIM_STYLE;
        SendMessage(hReBar, RB_SETBANDINFO, (WPARAM)i, (LPARAM)&rbbi);
    }

    SendMessage(hReBar, WM_SIZE, 0, 0);
    InvalidateRect(hReBar, NULL, TRUE);
}

LRESULT CALLBACK SubclassedToolbarProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_CONTEXTMENU)
    {
        POINT pt;
        GetCursorPos(&pt);

        HMENU hMenu = CreatePopupMenu();
        if (hMenu)
        {
            UINT flags = MF_STRING;
            if (g_LayoutLocked) flags |= MF_CHECKED;
            AppendMenu(hMenu, flags, 1001, Translations::TranslateOrDefault("ToolbarMenu.LockLayout", "Lock layout"));
            if (vaildToolbars > 1)
                AppendMenu(hMenu, MF_STRING, 1002, Translations::TranslateOrDefault("ToolbarMenu.HideToolbar", "Hide toolbar"));
            AppendMenu(hMenu, MF_STRING, 1003, Translations::TranslateOrDefault("ToolbarMenu.ResetToolbar", "Reset toolbar"));

            SetForegroundWindow(hWnd); 
            UINT cmd = TrackPopupMenu(hMenu,
                TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_TOPALIGN,
                pt.x, pt.y, 0, hWnd, NULL);

            DestroyMenu(hMenu);

            switch (cmd)
            {
            case 1001:
            {
                g_LayoutLocked = !g_LayoutLocked;
                LockReBarBands(CFinalSunDlg::Instance->ReBarCtrl.GetSafeHwnd(), g_LayoutLocked);
                SaveConfigIni("UserInterface", "LockToolBarLayout", g_LayoutLocked ? "1" : "0");
                break;
            }
            case 1002:
            {
                FString key;
                if (hWnd == g_Toolbars.hTbA)
                    key = "HideToolBarA";
                else if (hWnd == g_Toolbars.hTbB)
                    key = "HideToolBarB";
                else if (hWnd == g_Toolbars.hTbC)
                    key = "HideToolBarC";
                else if (hWnd == g_Toolbars.hTbD)
                    key = "HideToolBarD";
                if (!key.IsEmpty())
                    SaveConfigIni("UserInterface", key, "1");
                CFinalSunDlgExt::InitToolbar();
                HWND hMain = CFinalSunDlg::Instance->GetSafeHwnd();
                if (hMain)
                {
                    ::SendMessage(hMain, WM_SIZE, SIZE_RESTORED, 0);
                    ::RedrawWindow(hMain, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_FRAME);
                    ::SetWindowPos(hMain, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                }
                return 0;
            }
            case 1003:
            {
                CINI ini;
                ppmfc::CString path = CFinalSunAppExt::ExePathExt;
                path += "\\FinalAlert.ini";
                ini.ClearAndLoad(path);
                ini.DeleteKey("UserInterface", "HideToolBarA");
                ini.DeleteKey("UserInterface", "HideToolBarB");
                ini.DeleteKey("UserInterface", "HideToolBarC");
                ini.DeleteKey("UserInterface", "HideToolBarD");
                ini.WriteToFile(path);
                CFinalSunDlgExt::InitToolbar();
                HWND hMain = CFinalSunDlg::Instance->GetSafeHwnd();
                if (hMain)
                {
                    ::SendMessage(hMain, WM_SIZE, SIZE_RESTORED, 0);
                    ::RedrawWindow(hMain, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_FRAME);
                    ::SetWindowPos(hMain, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                }
                return 0;
            }
            }
            return 0; 
        }
    }

    auto it = g_OriginalProcs.find(hWnd);
    if (it != g_OriginalProcs.end())
    {
        return CallWindowProc(it->second, hWnd, uMsg, wParam, lParam);
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HWND CreateToolbarFromResource(HWND hWndParent, int resource, int bmpResource, std::map<UINT, void*> checkButtons = {})
{
    HINSTANCE hInst = static_cast<HINSTANCE>(FA2sp::hInstance);

    HRSRC hRsrc = FindResource(hInst, MAKEINTRESOURCE(resource), RT_TOOLBAR);
    if (!hRsrc) return NULL;
    HGLOBAL hGlobal = LoadResource(hInst, hRsrc);
    if (!hGlobal) return NULL;
    void* pData = LockResource(hGlobal);
    if (!pData) return NULL;

    struct TOOLBARDATA {
        WORD wVersion;
        WORD wWidth;
        WORD wHeight;
        WORD wItemCount;
    } *pTbData = (TOOLBARDATA*)pData;

    if (pTbData->wVersion != 1) return NULL;
    WORD* pButtonIDs = (WORD*)(pTbData + 1);

    HWND hTb = CreateWindowEx(
        0,
        TOOLBARCLASSNAME,
        NULL,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS |
        CCS_NODIVIDER | CCS_NORESIZE | TBSTYLE_TRANSPARENT,
        0, 0, 0, 0,
        hWndParent,
        NULL,
        hInst,
        NULL);

    if (!hTb) return NULL;

    SendMessage(hTb, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(hTb, TB_SETBITMAPSIZE, 0, MAKELPARAM(pTbData->wWidth, pTbData->wHeight));

    HIMAGELIST hImgList = ImageList_Create(
        pTbData->wWidth,
        pTbData->wHeight, 
        ILC_COLOR24 | ILC_MASK,
        pTbData->wItemCount,
        4
    );

    if (!hImgList) {
        DestroyWindow(hTb);
        return NULL;
    }

    HBITMAP hBmp = (HBITMAP)LoadImage(
        hInst,
        MAKEINTRESOURCE(bmpResource),
        IMAGE_BITMAP,
        0, 0,
        LR_DEFAULTCOLOR | LR_CREATEDIBSECTION
    );

    if (!hBmp) {
        ImageList_Destroy(hImgList);
        DestroyWindow(hTb);
        return NULL;
    }

    int addedCount = ImageList_AddMasked(hImgList, hBmp, RGB(255, 0, 255));

    DeleteObject(hBmp); 

    if (addedCount == -1) {
        ImageList_Destroy(hImgList);
        DestroyWindow(hTb);
        return NULL;
    }

    SendMessage(hTb, TB_SETIMAGELIST, 0, (LPARAM)hImgList);

    TBBUTTON tbb = { 0 };
    tbb.fsState = TBSTATE_ENABLED;
    int actualButtonIndex = 0;

    for (int i = 0; i < pTbData->wItemCount; i++)
    {
        WORD cmdID = pButtonIDs[i];
        bool initialChecked = false;

        if (cmdID == 0)
        {
            tbb.fsStyle = BTNS_SEP;
            tbb.iBitmap = 0; 
            tbb.idCommand = 0;
            tbb.dwData = 0;
            tbb.iString = 0;
        }
        else
        {
            tbb.fsStyle = BTNS_BUTTON;
            auto itr = checkButtons.find(cmdID);
            if (itr != checkButtons.end())
            {
                tbb.fsStyle |= BTNS_CHECK;
                CheckButtonInfo info;
                info.hParent = hTb;
                info.cmdID = cmdID;
                info.index = actualButtonIndex;
                info.isChecked = false;
                info.pExternalBool = itr->second;
                initialChecked = *(BOOL*)itr->second;
                CFinalSunDlgExt::CheckButtonMap[cmdID] = info;
            }
            tbb.iBitmap = actualButtonIndex;
            tbb.idCommand = cmdID;
            tbb.dwData = 0;
            tbb.iString = 0;
            actualButtonIndex++;
        }

        SendMessage(hTb, TB_ADDBUTTONS, 1, (LPARAM)&tbb);
        SendMessage(hTb, TB_CHECKBUTTON, cmdID, MAKELPARAM(initialChecked ? TRUE : FALSE, 0));
    }

    int btnWidth = pTbData->wWidth + 8;
    int btnHeight = pTbData->wHeight + 8;
    SendMessage(hTb, TB_SETBUTTONSIZE, 0, MAKELPARAM(btnWidth, btnHeight));
    SendMessage(hTb, TB_AUTOSIZE, 0, 0);
    InvalidateRect(hTb, NULL, TRUE);

    g_OriginalProcs[hTb] = (WNDPROC)SetWindowLongPtr(
        hTb,
        GWLP_WNDPROC,
        (LONG_PTR)SubclassedToolbarProc
    );

    g_LayoutLocked = STDHelpers::IsTrue(GetConfigIni("UserInterface", "LockToolBarLayout"));
    return hTb;
}

BOOL AddToolbarToReBar(HWND hReBar, HWND hToolbar, int width, bool newLine = false, const char* text = nullptr, int height = 24)
{
    if (!hReBar || !hToolbar) return FALSE;

    REBARBANDINFO rbbi = { 0 };
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
    if (text)
        rbbi.fMask |= RBBIM_TEXT;
    rbbi.fStyle = RBBS_GRIPPERALWAYS | RBBS_CHILDEDGE | RBBS_FIXEDBMP;
    if (newLine)
        rbbi.fStyle |= RBBS_BREAK;
    rbbi.hwndChild = hToolbar;
    rbbi.cxMinChild = width;
    rbbi.cyMinChild = height;
    rbbi.cx = width;
    if (text)
    {
        rbbi.lpText = (LPSTR)text; 
        rbbi.cch = lstrlen(text) + 1; 
    }

    LRESULT res = SendMessage(hReBar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbbi);

    return (res != 0);
}

BOOL RemoveToolbarByBandIndex(HWND hReBar, int bandIndex)
{
    if (!hReBar || bandIndex < 0) return FALSE;

    SendMessage(hReBar, RB_SHOWBAND, (WPARAM)bandIndex, (LPARAM)FALSE);
    LRESULT res = SendMessage(hReBar, RB_DELETEBAND, (WPARAM)bandIndex, 0);

    return (res != 0);
}

void CFinalSunDlgExt::InitToolbar()
{
    HWND hReBar = CFinalSunDlg::Instance->ReBarCtrl.GetSafeHwnd();

    RemoveToolbarFromReBar(hReBar, g_Toolbars.hTbA);
    RemoveToolbarFromReBar(hReBar, g_Toolbars.hTbC);
    RemoveToolbarFromReBar(hReBar, g_Toolbars.hTbD);
    RemoveToolbarFromReBar(hReBar, g_Toolbars.hTbB);

    static bool g_bToolbarsInitialized = false;
    if (!g_bToolbarsInitialized)
    {
        HWND hParent = ::GetParent(CFinalSunDlg::Instance->ReBarCtrl.GetSafeHwnd());

        std::map<UINT, void*> checkButtons =
        {
            {30000, (void*)(&CIsoViewExt::DrawStructures)},
            {30001, (void*)(&CIsoViewExt::DrawInfantries)},
            {30002, (void*)(&CIsoViewExt::DrawUnits)},
            {30003, (void*)(&CIsoViewExt::DrawAircrafts)},
            {30008, (void*)(&CIsoViewExt::DrawOverlays)},
            {30009, (void*)(&CIsoViewExt::DrawTerrains)},
            {30010, (void*)(&CIsoViewExt::DrawSmudges)},
            {30004, (void*)(&CIsoViewExt::DrawBasenodes)},
            {30013, (void*)(&CIsoViewExt::DrawBaseNodeIndex)},
            {30005, (void*)(&CIsoViewExt::DrawWaypoints)},
            {30006, (void*)(&CIsoViewExt::DrawCelltags)},
            {30011, (void*)(&CIsoViewExt::DrawTubes)},
            {30007, (void*)(&CIsoViewExt::DrawMoneyOnMap)},
            {30012, (void*)(&CIsoViewExt::DrawBounds)},
            {30021, (void*)(&CIsoViewExt::DrawVeterancy)},
            {30014, (void*)(&CIsoViewExt::RockCells)},
            {30022, (void*)(&CIsoViewExt::DrawShadows)},
            {30023, (void*)(&CIsoViewExt::DrawAlphaImages)},
            {30024, (void*)(&CIsoViewExt::DrawAnnotations)},
            {30025, (void*)(&CIsoViewExt::DrawFires)},
            {40115, (void*)(&CFinalSunApp::Instance->FrameMode)},
            {40085, (void*)(&CFinalSunApp::Instance->FlatToGround)},
            {40163, (void*)(&CIsoViewExt::EnableDistanceRuler)},
            {30107, (void*)(&CFinalSunDlgExt::HasMinimap)},
            {40123, (void*)(&CFinalSunApp::Instance->ShowBuildingCells)},
            {40159, (void*)(&ExtConfigs::TreeViewCameo_Display)},
            {40104, (void*)(&CFinalSunApp::Instance->DisableAutoShore)},
            {40105, (void*)(&CFinalSunApp::Instance->DisableAutoLat)},
        };

        g_Toolbars.hTbA = CreateToolbarFromResource(hParent, 1100, ExtConfigs::EnableDarkMode ? 1113 : 1110, checkButtons);
        g_Toolbars.hTbB = CFinalSunDlg::Instance->BrushSize.GetSafeHwnd();
        g_Toolbars.hTbC = CreateToolbarFromResource(hParent, 1101, ExtConfigs::EnableDarkMode ? 1114 : 1111, checkButtons);
        g_Toolbars.hTbD = CreateToolbarFromResource(hParent, 1102, ExtConfigs::EnableDarkMode ? 1115 : 1112, checkButtons);
        g_OriginalProcs[g_Toolbars.hTbB] = (WNDPROC)SetWindowLongPtr(
            g_Toolbars.hTbB,
            GWLP_WNDPROC,
            (LONG_PTR)SubclassedToolbarProc
        );

        g_bToolbarsInitialized = true;
    }
    vaildToolbars = 4;

    RECT rc;
    ::GetClientRect(hReBar, &rc);
    int availableWidth = rc.right - rc.left;
    const int EXTRA = 30;

    struct TbItem {
        HWND hTb;
        int idealWidth;
        bool visible = true; 
        const char* text = nullptr;
    };

    TbItem tbB = {};
    std::vector<TbItem> others;
    if (!STDHelpers::IsTrue(GetConfigIni("UserInterface", "HideToolBarA")))
        others.push_back({ g_Toolbars.hTbA, 500 });
    if (!STDHelpers::IsTrue(GetConfigIni("UserInterface", "HideToolBarB")))
        tbB = { g_Toolbars.hTbB, 200 };
    if (!STDHelpers::IsTrue(GetConfigIni("UserInterface", "HideToolBarC")))
        others.push_back({ g_Toolbars.hTbC, 508 });
    if (!STDHelpers::IsTrue(GetConfigIni("UserInterface", "HideToolBarD")))
        others.push_back({ g_Toolbars.hTbD, 540});

    bool hasB = (tbB.hTb != NULL && IsWindow(tbB.hTb));

    int currentRowWidth = 0;
    int rowIndex = 0; 

    std::vector<HWND> currentRowHWNDs;
    currentRowWidth = 0;

    for (auto& item : others)
    {
        int added = item.idealWidth + EXTRA;
        int withB = hasB ? currentRowWidth + added + (tbB.idealWidth + EXTRA) : currentRowWidth + added;
        if (withB <= availableWidth)
        {
            AddToolbarToReBar(hReBar, item.hTb, item.idealWidth, false, item.text);
            currentRowHWNDs.push_back(item.hTb);
            currentRowWidth += added;
            rowIndex++;
        }
        else
        {
            break; 
        }
    }

    if (hasB)
    {
        AddToolbarToReBar(hReBar, tbB.hTb, tbB.idealWidth, false, tbB.text);
        currentRowHWNDs.push_back(tbB.hTb);
        rowIndex++;
    }

    if (!currentRowHWNDs.empty())
    {
        if (hasB && rowIndex >= 2)
            ::SendMessage(hReBar, RB_MAXIMIZEBAND, rowIndex - 2, TRUE);
        else if (!hasB && rowIndex >= 1)
            ::SendMessage(hReBar, RB_MAXIMIZEBAND, rowIndex - 1, TRUE);
    }

    std::vector<TbItem> remaining;
    bool startedRemaining = false;
    for (auto& item : others)
    {
        if (!startedRemaining)
        {
            if (std::find_if(currentRowHWNDs.begin(), currentRowHWNDs.end(),
                [&](HWND h) { return h == item.hTb; }) != currentRowHWNDs.end())
                continue;
            startedRemaining = true;
        }
        remaining.push_back(item);
    }

    bool first = true;
    currentRowWidth = 0;
    currentRowHWNDs.clear();

    for (auto& item : remaining)
    {
        int added = item.idealWidth + EXTRA;

        if (!first && currentRowWidth + added <= availableWidth)
        {
            AddToolbarToReBar(hReBar, item.hTb, item.idealWidth, !currentRowHWNDs.empty(), item.text);
            currentRowHWNDs.push_back(item.hTb);
            currentRowWidth += added;
        }
        else
        {
            if (!currentRowHWNDs.empty())
            {
                ::SendMessage(hReBar, RB_MAXIMIZEBAND, rowIndex - 1, TRUE);
            }

            currentRowHWNDs.clear();
            currentRowWidth = 0;

            AddToolbarToReBar(hReBar, item.hTb, item.idealWidth, true, item.text);
            currentRowHWNDs.push_back(item.hTb);
            currentRowWidth += added;
            rowIndex++;
        }
        first = false;
    }

    if (!currentRowHWNDs.empty())
    {
        ::SendMessage(hReBar, RB_MAXIMIZEBAND, rowIndex - 1, TRUE);
    }

    if (g_LayoutLocked)
        LockReBarBands(hReBar, true);
}

DEFINE_HOOK(423FB8, CFinalSunDlg_OnInitDialog_ChangeToolBarStyles, 5)
{
    if (!FA2sp::WinInfo.IsWindowsVistaOrGreater())
        return 0;

    RunTime::NopMemory(0x4241C0, 9);
    RunTime::NopMemory(0x4241D8, 2);
    return 0;
}

DEFINE_HOOK(423FEB, CFinalSunDlg_OnInitDialog_SkipToolBar, 6)
{
    if (!FA2sp::WinInfo.IsWindowsVistaOrGreater())
        return 0;

    return 0x424117;
}

DEFINE_HOOK(424271, CFinalSunDlg_OnInitDialog_SkipShowWindow, 6)
{
    if (!FA2sp::WinInfo.IsWindowsVistaOrGreater())
    {
        ShowWindow(CFinalSunDlg::Instance->ToolBar1, SW_SHOW);
        ShowWindow(CFinalSunDlg::Instance->ToolBar2, SW_SHOW);
        ShowWindow(CFinalSunDlg::Instance->ToolBar3, SW_SHOW);
    }

    return 0x424283;
}

DEFINE_HOOK(424132, CFinalSunDlg_OnInitDialog_InsertToolBar, 7)
{
    if (!FA2sp::WinInfo.IsWindowsVistaOrGreater())
        return 0;

    CFinalSunDlgExt::InitToolbar();

    return 0x424264;
}

