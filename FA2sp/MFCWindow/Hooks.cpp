#include <FA2PP.h>
#include <CMyViewFrame.h>
#include <CTileSetBrowserFrame.h>
#include <CIsoView.h>

#include <Helpers/Macro.h>

#include "../FA2sp.h"
#include "../Miscs/DialogStyle.h"
#include "../Ext/CMinimap/Body.h"
#include "../Ext/CIsoView/Body.h"
#include "../Ext/CFinalSunApp/Body.h"
#include "../Ext/CTileSetBrowserFrame/Body.h"
#include "../Ext/CTileSetBrowserFrame/TabPages/GridObjectViewer.h"

namespace TransparencyMenu
{
    const UINT IDM_OPAQUE      = 0x8000;
    const UINT IDM_NEAR_FULL   = 0x8001;
    const UINT IDM_HALF        = 0x8002;
    const UINT IDM_TRANSPARENT = 0x8003;

    WNDPROC g_prevTileBrowserProc = nullptr;
    WNDPROC g_prevViewObjsProc = nullptr;
    WNDPROC g_prevMinimapProc = nullptr;

    int g_restingTileBrowserAlpha = 255;
    int g_restingViewObjsAlpha = 255;
    int g_restingMinimapAlpha = 255;

    const UINT_PTR HOVER_TIMER_ID = 0x9000;
    const UINT HOVER_TIMER_INTERVAL = 100;

    void SetTransparency(HWND hWnd, int alpha)
    {
        DWORD dwEx = ::GetWindowLong(hWnd, GWL_EXSTYLE);
        if (!(dwEx & WS_EX_LAYERED))
        {
            dwEx |= WS_EX_LAYERED;
            ::SetWindowLong(hWnd, GWL_EXSTYLE, dwEx);
        }
        ::SetLayeredWindowAttributes(hWnd, 0, alpha, LWA_ALPHA);
    }

    void ApplyTransparency(HWND hWnd, int alpha, WNDPROC& prevProc)
    {
        SetTransparency(hWnd, alpha);

        if (&prevProc == &g_prevTileBrowserProc)
            g_restingTileBrowserAlpha = alpha;
        else if (&prevProc == &g_prevViewObjsProc)
            g_restingViewObjsAlpha = alpha;
        else if (&prevProc == &g_prevMinimapProc)
            g_restingMinimapAlpha = alpha;

        // Manage hover timer: only needed when semi-transparent
        if (alpha == 255)
            ::KillTimer(hWnd, HOVER_TIMER_ID);
        else
            ::SetTimer(hWnd, HOVER_TIMER_ID, HOVER_TIMER_INTERVAL, nullptr);

        CINI fa2;
        FString path = CFinalSunAppExt::ExePathExt;
        path += "\\FinalAlert.ini";
    
        fa2.ClearAndLoad(path);
    
        if (&prevProc == &g_prevTileBrowserProc)
        {
            fa2.WriteString("UserInterface", "TileBrowserOpacity", std::to_string(alpha).c_str());
        }
        else if (&prevProc == &g_prevViewObjsProc)
        {
            fa2.WriteString("UserInterface", "ViewObjectsOpacity", std::to_string(alpha).c_str());
        }
        else if (&prevProc == &g_prevMinimapProc || &prevProc == &CMinimapExt::g_pfnOriginalMinimapProc)
        {
            fa2.WriteString("UserInterface", "MinimapOpacity", std::to_string(alpha).c_str());
        }

		fa2.WriteToFile(path);
	}

    int* GetRestingAlpha(WNDPROC& prevProc)
    {
        if (&prevProc == &g_prevTileBrowserProc)
            return &g_restingTileBrowserAlpha;
        else if (&prevProc == &g_prevViewObjsProc)
            return &g_restingViewObjsAlpha;
        else if (&prevProc == &g_prevMinimapProc)
            return &g_restingMinimapAlpha;
        return nullptr;
    }

    // Returns true if any ComboLBox (combobox dropdown list) is visible.
    bool HasComboLBox()
    {
        bool found = false;
        ::EnumWindows([](HWND hTop, LPARAM lParam) -> BOOL {
            auto* pFound = reinterpret_cast<bool*>(lParam);
            DWORD pid;
            ::GetWindowThreadProcessId(hTop, &pid);
            if (pid == ::GetCurrentProcessId() && ::IsWindowVisible(hTop))
            {
                wchar_t className[16];
                if (::GetClassNameW(hTop, className, 16) == 0)
                    return TRUE;
                if (wcscmp(className, L"ComboLBox") == 0)
                {
                    *pFound = true;
                    return FALSE;
                }
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&found));
        return found;
    }

    bool IsCursorOverWindow(HWND hWnd)
    {
        POINT pt;
        ::GetCursorPos(&pt);
        HWND hWndUnder = ::WindowFromPoint(pt);

        if (!hWndUnder)
            return false;
        if (hWnd == hWndUnder || ::IsChild(hWnd, hWndUnder))
            return true;

        // Walk owner chain for popups (combobox dropdowns, etc.)
        HWND hOwner = hWndUnder;
        while ((hOwner = ::GetWindow(hOwner, GW_OWNER)) != NULL)
        {
            if (hOwner == hWnd || ::IsChild(hWnd, hOwner))
                return true;
        }

        // Rect-based fallback: only active when a ComboLBox is visible,
        // to avoid false positives when other windows overlap ours.
        if (HasComboLBox())
        {
            RECT rc;
            if (::GetWindowRect(hWnd, &rc) && ::PtInRect(&rc, pt))
            {
                DWORD pid;
                ::GetWindowThreadProcessId(hWndUnder, &pid);
                if (pid == ::GetCurrentProcessId())
                    return true;
            }
        }

        return false;
    }

    LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, WNDPROC& prevProc)
    {
        switch (msg)
        {
        case WM_MOUSEMOVE:
        case WM_NCMOUSEMOVE:
        {
            if (!GetProp(hWnd, "TransparencyHover"))
            {
                SetProp(hWnd, "TransparencyHover", (HANDLE)TRUE);
                SetTransparency(hWnd, 255);
            }
            break;
        }

        case WM_TIMER:
        {
            if (wParam == HOVER_TIMER_ID)
            {
                int* pResting = GetRestingAlpha(prevProc);
                if (!pResting)
                    break;

                // No timer needed when resting is fully opaque
                if (*pResting == 255)
                {
                    RemoveProp(hWnd, "TransparencyHover");
                    ::KillTimer(hWnd, HOVER_TIMER_ID);
                    break;
                }

                bool isOver = IsCursorOverWindow(hWnd);

                if (isOver && !GetProp(hWnd, "TransparencyHover"))
                {
                    SetProp(hWnd, "TransparencyHover", (HANDLE)TRUE);
                    SetTransparency(hWnd, 255);
                }
                else if (!isOver && GetProp(hWnd, "TransparencyHover"))
                {
                    SetTransparency(hWnd, *pResting);
                    RemoveProp(hWnd, "TransparencyHover");
                }
            }
            break;
        }

        case WM_NCRBUTTONDOWN:
            if (wParam == HTCAPTION)
            {
                HMENU hMenu = ::CreatePopupMenu();
                ::AppendMenu(hMenu, MF_STRING, IDM_OPAQUE,      Translations::TranslateOrDefault("TransparencyMenu.100", "Opaque (100%)"));
                ::AppendMenu(hMenu, MF_STRING, IDM_NEAR_FULL,   Translations::TranslateOrDefault("TransparencyMenu.75", "75% Opacity"));
                ::AppendMenu(hMenu, MF_STRING, IDM_HALF,        Translations::TranslateOrDefault("TransparencyMenu.50", "50% Opacity"));
                ::AppendMenu(hMenu, MF_STRING, IDM_TRANSPARENT, Translations::TranslateOrDefault("TransparencyMenu.25", "25% Opacity"));

                POINT pt;
                ::GetCursorPos(&pt);
                ::SetForegroundWindow(hWnd);
                ::TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, hWnd, nullptr);
                ::DestroyMenu(hMenu);
                return 0;
            }
            break;

        case WM_INITMENUPOPUP:
            // MFC disables menu items that have no command handler.
            // If this popup contains our IDs, consume the message
            // so MFC leaves our items enabled.
            if (::GetMenuState((HMENU)wParam, IDM_OPAQUE, MF_BYCOMMAND) != -1)
                return 0;
            break;

        case WM_COMMAND:
        {
            UINT id = LOWORD(wParam);
            int alpha = -1;
            if (id == IDM_OPAQUE)      alpha = 255;
            else if (id == IDM_NEAR_FULL)   alpha = 191;
            else if (id == IDM_HALF)   alpha = 128;
            else if (id == IDM_TRANSPARENT) alpha = 64;
            if (alpha >= 0)
            {
                ApplyTransparency(hWnd, alpha, prevProc);
                return 0;
            }
        }
        break;
        }
        return CallWindowProc(prevProc, hWnd, msg, wParam, lParam);
    }

    LRESULT CALLBACK TileBrowserProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        return WndProc(hWnd, msg, wParam, lParam, g_prevTileBrowserProc);
    }

    LRESULT CALLBACK ViewObjsProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        return WndProc(hWnd, msg, wParam, lParam, g_prevViewObjsProc);
    }

    LRESULT CALLBACK MinimapProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        return WndProc(hWnd, msg, wParam, lParam, g_prevMinimapProc);
    }

    HICON GetTransparentIcon() {
        static HICON hEmptyIcon = NULL;
        if (hEmptyIcon == NULL) {
            HBITMAP hbmMask = CreateBitmap(1, 1, 1, 1, NULL);
            if (hbmMask) {
                unsigned char data = 0xFF;
                SetBitmapBits(hbmMask, 1, &data);
    
                ICONINFO ii = {0};
                ii.fIcon = TRUE;
                ii.hbmMask = hbmMask;
    
                hEmptyIcon = CreateIconIndirect(&ii);
                DeleteObject(hbmMask);
            }
        }
        return hEmptyIcon;
    }
}

static bool CMyViewFrameInitialized = false;
DEFINE_HOOK(4D2680, CMyViewFrame_OnCreateClient, 5)
{
    GET(CMyViewFrame*, pThis, ECX);
    GET_STACK(LPCREATESTRUCT, lpcs, 0x4);
    GET_STACK(ppmfc::CCreateContext*, pContent, 0x8);

    RECT rect{ 0,0,200,200 };
    SIZE size{ 200,200 };

    BOOL bRes = FALSE;
    if (bRes = pThis->SplitterWnd.CreateStatic(pThis, 1, 2, WS_CHILD | WS_VISIBLE))
    {
        // When ViewObjectsFloating, swap order: CRightFrame at col 0, CViewObjects at col 1
        // so the surviving pane is at index 0 (same pattern as TileSetBrowserFloating)
        int colRight = ExtConfigs::ViewObjectsFloating ? 0 : 1;
        int colViewObjs = ExtConfigs::ViewObjectsFloating ? 1 : 0;
        ppmfc::CRuntimeClass* pClass0 = ExtConfigs::ViewObjectsFloating
            ? reinterpret_cast<ppmfc::CRuntimeClass*>(&CRightFrame::RuntimeClass)
            : reinterpret_cast<ppmfc::CRuntimeClass*>(&CViewObjects::RuntimeClass);
        ppmfc::CRuntimeClass* pClass1 = ExtConfigs::ViewObjectsFloating
            ? reinterpret_cast<ppmfc::CRuntimeClass*>(&CViewObjects::RuntimeClass)
            : reinterpret_cast<ppmfc::CRuntimeClass*>(&CRightFrame::RuntimeClass);

        int tileBrowserOpacity = 255;
        int viewObjectsOpacity = 255;
        int minimapOpacity = 255;

        CINI fa2;
        FString path = CFinalSunAppExt::ExePathExt;
        path += "\\FinalAlert.ini";
    
        fa2.ClearAndLoad(path);

        tileBrowserOpacity = fa2.GetInteger("UserInterface", "TileBrowserOpacity", 255);
        viewObjectsOpacity = fa2.GetInteger("UserInterface", "ViewObjectsOpacity", 255);
        minimapOpacity = fa2.GetInteger("UserInterface", "MinimapOpacity", 255);

        tileBrowserOpacity = std::max(0, std::min(255, tileBrowserOpacity));
        viewObjectsOpacity = std::max(0, std::min(255, viewObjectsOpacity));
        minimapOpacity = std::max(0, std::min(255, minimapOpacity));

        if (bRes = pThis->SplitterWnd.CreateView(0, 0, pClass0, size, pContent))
        {
            if (bRes = pThis->SplitterWnd.CreateView(0, 1, pClass1, size, pContent))
            {
                // CMyViewFrame::OnCreateClient(): windows created\n
                pThis->pRightFrame = (CRightFrame*)pThis->SplitterWnd.GetPane(0, colRight);
                pThis->pIsoView = (CIsoView*)pThis->pRightFrame->CSplitter.GetPane(0, 0);
                pThis->pIsoView->pParent = pThis;

                // --- TileSetBrowser floating window ---
                if (ExtConfigs::TileSetBrowserFloating) {
                    CTileSetBrowserFrame* pTileBrowser;
                    if (ExtConfigs::VerticalLayout) {
                        pTileBrowser = (CTileSetBrowserFrame*)pThis->pRightFrame->CSplitter.GetPane(0, 1);
                        pThis->pRightFrame->CSplitter.m_nCols = 1;
                        pThis->pRightFrame->CSplitter.SetColumnInfo(1, 0, 0);
                    } else {
                        pTileBrowser = (CTileSetBrowserFrame*)pThis->pRightFrame->CSplitter.GetPane(1, 0);
                        pThis->pRightFrame->CSplitter.m_nRows = 1;
                        pThis->pRightFrame->CSplitter.SetRowInfo(1, 0, 0);
                    }
                    pThis->pRightFrame->CSplitter.RecalcLayout();

                    HWND hTileBrowser = pTileBrowser->GetSafeHwnd();
                    DWORD dwStyle = ::GetWindowLong(hTileBrowser, GWL_STYLE);
                    dwStyle &= ~WS_CHILD;
                    dwStyle |= WS_OVERLAPPEDWINDOW;
                    dwStyle &= ~WS_MINIMIZEBOX;
                    ::SetWindowLong(hTileBrowser, GWL_STYLE, dwStyle);
                    ::SetParent(hTileBrowser, NULL);
                    ::SetWindowLong(hTileBrowser, GWL_HWNDPARENT, (LONG)pThis->GetSafeHwnd());

                    RECT rcMain;
                    ::GetWindowRect(pThis->GetSafeHwnd(), &rcMain);
                    int w, h, x, y;
                    if (ExtConfigs::VerticalLayout) {
                        w = (rcMain.right - rcMain.left) * 1 / 4;
                        h = (rcMain.bottom - rcMain.top) * 3 / 4;
                        x = rcMain.right - w;
                        y = rcMain.top;
                    } else {
                        w = (rcMain.right - rcMain.left) * 3 / 4;
                        h = (rcMain.bottom - rcMain.top) * 3 / 8;
                        x = rcMain.right - w;
                        y = rcMain.bottom - h;
                    }
                    ::SetWindowPos(hTileBrowser, NULL, x, y, w, h, SWP_NOZORDER | SWP_FRAMECHANGED);
                    // SetParent can strip WS_EX_LAYERED when reparenting child->top-level
                    {
                        DWORD dwEx = ::GetWindowLong(hTileBrowser, GWL_EXSTYLE);
                        dwEx |= WS_EX_LAYERED;
                        ::SetWindowLong(hTileBrowser, GWL_EXSTYLE, dwEx);
                    }
                    ::SetLayeredWindowAttributes(hTileBrowser, 0, tileBrowserOpacity, LWA_ALPHA);
                    TransparencyMenu::g_restingTileBrowserAlpha = tileBrowserOpacity;

                    HICON hEmptyIcon = TransparencyMenu::GetTransparentIcon();
                    if (hEmptyIcon) {
                        ::SendMessage(hTileBrowser, WM_SETICON, ICON_SMALL, (LPARAM)hEmptyIcon);
                        ::SendMessage(hTileBrowser, WM_SETICON, ICON_BIG, (LPARAM)hEmptyIcon);           
                    }
                    
                    DarkTheme::SetDarkTheme(hTileBrowser);
                    if (GridObjectViewer::Instance.IsValid()) {
                        HWND hCtrl = GridObjectViewer::Instance.GetControl();
                        if (hCtrl) {
                            DarkTheme::SetDarkTheme(hCtrl);
                            DarkTheme::SubclassAllControls(hCtrl);
                        }
                    }
                    ::ShowWindow(hTileBrowser, SW_HIDE);
                    TransparencyMenu::g_prevTileBrowserProc = (WNDPROC)::SetWindowLongPtr(
                        hTileBrowser, GWLP_WNDPROC, (LONG_PTR)TransparencyMenu::TileBrowserProc);
                    if (tileBrowserOpacity < 255)
                        ::SetTimer(hTileBrowser, TransparencyMenu::HOVER_TIMER_ID, TransparencyMenu::HOVER_TIMER_INTERVAL, nullptr);
                    pThis->pTileSetBrowserFrame = pTileBrowser;
                }
                else if (ExtConfigs::VerticalLayout) {
                    pThis->pTileSetBrowserFrame = (CTileSetBrowserFrame*)pThis->pRightFrame->CSplitter.GetPane(0, 1);
                }
                else {
                    pThis->pTileSetBrowserFrame = (CTileSetBrowserFrame*)pThis->pRightFrame->CSplitter.GetPane(1, 0);
                }

                // --- CViewObjects floating window ---
                if (ExtConfigs::ViewObjectsFloating) {
                    CViewObjects* pViewObjs = (CViewObjects*)pThis->SplitterWnd.GetPane(0, 1);
                    // Shrink main splitter column count so it never iterates the detached pane
                    pThis->SplitterWnd.m_nCols = 1;
                    pThis->SplitterWnd.SetColumnInfo(1, 0, 0);
                    pThis->SplitterWnd.RecalcLayout();

                    HWND hViewObjs = pViewObjs->GetSafeHwnd();
                    DWORD dwStyle = ::GetWindowLong(hViewObjs, GWL_STYLE);
                    dwStyle &= ~WS_CHILD;
                    dwStyle |= WS_OVERLAPPEDWINDOW;
                    dwStyle &= ~WS_MINIMIZEBOX;
                    ::SetWindowLong(hViewObjs, GWL_STYLE, dwStyle);
                    ::SetParent(hViewObjs, NULL);
                    ::SetWindowLong(hViewObjs, GWL_HWNDPARENT, (LONG)pThis->GetSafeHwnd());

                    RECT rcMain;
                    ::GetWindowRect(pThis->GetSafeHwnd(), &rcMain);
                    int w = (rcMain.right - rcMain.left) * 1 / 6;
                    int h = (rcMain.bottom - rcMain.top) * 3 / 4;
                    int x = rcMain.left;
                    int y = rcMain.top;
                    ::SetWindowPos(hViewObjs, NULL, x, y, w, h, SWP_NOZORDER | SWP_FRAMECHANGED);
                    // SetParent can strip WS_EX_LAYERED when reparenting child->top-level
                    {
                        DWORD dwEx = ::GetWindowLong(hViewObjs, GWL_EXSTYLE);
                        dwEx |= WS_EX_LAYERED;
                        ::SetWindowLong(hViewObjs, GWL_EXSTYLE, dwEx);
                    }
                    ::SetLayeredWindowAttributes(hViewObjs, 0, viewObjectsOpacity, LWA_ALPHA);
                    TransparencyMenu::g_restingViewObjsAlpha = viewObjectsOpacity;

                    HICON hEmptyIcon = TransparencyMenu::GetTransparentIcon();
                    if (hEmptyIcon) {
                        ::SendMessage(hViewObjs, WM_SETICON, ICON_SMALL, (LPARAM)hEmptyIcon);
                        ::SendMessage(hViewObjs, WM_SETICON, ICON_BIG, (LPARAM)hEmptyIcon);           
                    }

                    // Restore tree styles lost during SetWindowLong/SetParent
					// (SysTreeView32 resets TVS_HASBUTTONS/LINES/LINESATROOT when reparented)
					HWND hTree = pViewObjs->GetTreeCtrl().GetSafeHwnd();
					LONG treeStyle = ::GetWindowLong(hTree, GWL_STYLE);
					treeStyle |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT;
					::SetWindowLong(hTree, GWL_STYLE, treeStyle);

					if (ExtConfigs::EnableDarkMode) {
						// SetWindowTheme for dark scrollbar (DwmSetWindowAttribute
						// does not affect common control scrollbars)
						SetWindowTheme(hTree, L"DarkMode_Explorer", NULL);
						// Re-apply tree styles (SetWindowTheme may reset them)
						::SetWindowLong(hTree, GWL_STYLE, treeStyle);
						// Fine-tune background/text colors
						::SendMessage(hTree, TVM_SETBKCOLOR, 0, RGB(32, 32, 32));
						::SendMessage(hTree, TVM_SETTEXTCOLOR, 0, RGB(220, 220, 220));
						// Dark title bar for the frame window
						BOOL darkMode = TRUE;
						DwmSetWindowAttribute(hViewObjs, 19, &darkMode, sizeof(darkMode));
						DwmSetWindowAttribute(hViewObjs, 20, &darkMode, sizeof(darkMode));
					}

					::ShowWindow(hViewObjs, SW_HIDE);
                    TransparencyMenu::g_prevViewObjsProc = (WNDPROC)::SetWindowLongPtr(
                        hViewObjs, GWLP_WNDPROC, (LONG_PTR)TransparencyMenu::ViewObjsProc);
                    if (viewObjectsOpacity < 255)
                        ::SetTimer(hViewObjs, TransparencyMenu::HOVER_TIMER_ID, TransparencyMenu::HOVER_TIMER_INTERVAL, nullptr);
                    pThis->pViewObjects = pViewObjs;
                }
                else {
                    pThis->pViewObjects = (CViewObjects*)pThis->SplitterWnd.GetPane(0, colViewObjs);
                }

                pThis->Minimap.CreateEx(0, nullptr, "Minimap", 0, rect, pThis, 0);

                LONG style = GetWindowLong(pThis->Minimap, GWL_STYLE);
                style &= ~(WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
                style |= WS_SYSMENU | WS_SIZEBOX;
                SetWindowLong(pThis->Minimap, GWL_STYLE, style);

                DarkTheme::SetDarkTheme(pThis->Minimap);

                // Read minimap opacity from INI
                {
                    CINI fa2;
                    FString path = CFinalSunAppExt::ExePathExt;
                    path += "\\FinalAlert.ini";
                    fa2.ClearAndLoad(path);
                    minimapOpacity = fa2.GetInteger("UserInterface", "MinimapOpacity", 255);
                    minimapOpacity = std::max(0, std::min(255, minimapOpacity));
                }

                // Set layered for transparency support
                {
                    DWORD dwEx = ::GetWindowLong(pThis->Minimap, GWL_EXSTYLE);
                    dwEx |= WS_EX_LAYERED;
                    ::SetWindowLong(pThis->Minimap, GWL_EXSTYLE, dwEx);
                }
                ::SetLayeredWindowAttributes(pThis->Minimap, 0, minimapOpacity, LWA_ALPHA);
                TransparencyMenu::g_restingMinimapAlpha = minimapOpacity;

                HICON hEmptyIcon = TransparencyMenu::GetTransparentIcon();
                if (hEmptyIcon) {
                    ::SendMessage(pThis->Minimap, WM_SETICON, ICON_SMALL, (LPARAM)hEmptyIcon);
                    ::SendMessage(pThis->Minimap, WM_SETICON, ICON_BIG, (LPARAM)hEmptyIcon);
                }

                pThis->Minimap.Update();
                if (pThis->Minimap.m_hWnd && !CMinimapExt::g_pfnOriginalMinimapProc)
                {
                    TransparencyMenu::g_prevMinimapProc = CMinimapExt::MinimapWndProc;
                    CMinimapExt::g_pfnOriginalMinimapProc = (WNDPROC)SetWindowLongPtr(
                        pThis->Minimap.GetSafeHwnd(),
                        GWLP_WNDPROC,
                        (LONG_PTR)TransparencyMenu::MinimapProc
                    );
                }
                if (minimapOpacity < 255)
                    ::SetTimer(pThis->Minimap, TransparencyMenu::HOVER_TIMER_ID, TransparencyMenu::HOVER_TIMER_INTERVAL, nullptr);

                if (bRes = pThis->StatusBar.CreateEx(pThis, 0x900))
                {
                    pThis->ppmfc::CFrameWnd::OnCreateClient(lpcs, pContent);
                }
            }
        }
    }
    R->EAX(bRes);
    CMyViewFrameInitialized = true;

    return 0x4D26BF;
}

DEFINE_HOOK(4D3E50, CRightFrame_OnClientCreate, 5)
{
	GET(CRightFrame*, pThis, ECX);
	GET_STACK(LPCREATESTRUCT, lpcs, 0x4);
	GET_STACK(ppmfc::CCreateContext*, pContent, 0x8);

	BOOL bRes = FALSE;
	
	if (ExtConfigs::VerticalLayout)
	{
		SIZE size{700, 200};

		if (bRes = pThis->CSplitter.CreateStatic(pThis, 1, 2, WS_CHILD | WS_VISIBLE))
		{
			if (bRes = pThis->CSplitter.CreateView(0, 0, &CIsoView::RuntimeClass, size, pContent))
			{
				size = {200, 200};
				if (bRes = pThis->CSplitter.CreateView(0, 1, &CTileSetBrowserFrame::RuntimeClass, size, pContent))
				{
					auto const oct = GetSystemMetrics(SM_CXFULLSCREEN) / 8;
					pThis->CSplitter.SetColumnInfo(0, 5 * oct, 20);
					pThis->CSplitter.SetColumnInfo(1, 3 * oct, 10);

					pThis->ppmfc::CFrameWnd::OnCreateClient(lpcs, pContent);
				}
			}
		}
	}
	else
	{
		SIZE size{200, 700};

		if (bRes = pThis->CSplitter.CreateStatic(pThis, 2, 1, WS_CHILD | WS_VISIBLE))
		{
			if (bRes = pThis->CSplitter.CreateView(0, 0, &CIsoView::RuntimeClass, size, pContent))
			{
				size = {200, 100};
				if (bRes = pThis->CSplitter.CreateView(1, 0, &CTileSetBrowserFrame::RuntimeClass, size, pContent))
				{
					auto const oct = GetSystemMetrics(SM_CYFULLSCREEN) / 8;
					pThis->CSplitter.SetRowInfo(0, 5 * oct, 20);
					pThis->CSplitter.SetRowInfo(1, 3 * oct, 10);

					pThis->ppmfc::CFrameWnd::OnCreateClient(lpcs, pContent);
				}
			}
		}
	}

	R->EAX(bRes);
	return 0x4D3E8D;
}

DEFINE_HOOK(468690, CIsoView_OnSize_Size, A)
{
	if (!CMyViewFrameInitialized || !CViewObjectsExt::Initialized)
		return 0;

	GET(CIsoViewExt*, pThis, ECX);

	CRect rcFrame;
	CFinalSunDlg::Instance->MyViewFrame.pRightFrame->GetClientRect(&rcFrame);
	if (rcFrame.Width() <= 0 || rcFrame.Height() <= 0)
		return 0;

	if (ExtConfigs::TileSetBrowserFloating)
	{
		CTileSetBrowserFrameExt::RefreshWindows();
		return 0;
	}

	bool shouldWrite = false;

	if (ExtConfigs::VerticalLayout)
	{
		int cxCur, cxMin;
		CFinalSunDlg::Instance->MyViewFrame.pRightFrame->CSplitter.GetColumnInfo(0, cxCur, cxMin);

		int totalWidth = rcFrame.Width() - GetSystemMetrics(SM_CXHSCROLL);
		if (totalWidth > 0)
		{
			float newWidthPercentage = (float)cxCur / totalWidth;
			if (newWidthPercentage < 0.0f)
				newWidthPercentage = 0.0f;
			if (newWidthPercentage > 1.0f)
				newWidthPercentage = 1.0f;

			if (fabs(newWidthPercentage - ExtConfigs::IsoViewWidthPercentage) > 0.03f)
			{
				ExtConfigs::IsoViewWidthPercentage = newWidthPercentage;
				shouldWrite = true;
			}
		}
	}
	else
	{
		int cyCur, cyMin;
		CFinalSunDlg::Instance->MyViewFrame.pRightFrame->CSplitter.GetRowInfo(0, cyCur, cyMin);

		int totalHeight = rcFrame.Height() - GetSystemMetrics(SM_CYHSCROLL);
		if (totalHeight > 0)
		{
			float newHeightPercentage = (float)cyCur / totalHeight;
			if (newHeightPercentage < 0.0f)
				newHeightPercentage = 0.0f;
			if (newHeightPercentage > 1.0f)
				newHeightPercentage = 1.0f;

			if (fabs(newHeightPercentage - ExtConfigs::IsoViewHeightPercentage) > 0.03f)
			{
				ExtConfigs::IsoViewHeightPercentage = newHeightPercentage;
				shouldWrite = true;
			}
		}
	}

	CTileSetBrowserFrameExt::RefreshWindows();

	if (!shouldWrite)
		return 0;

	CINI fa2;
	FString path = CFinalSunAppExt::ExePathExt;
	path += "\\FinalAlert.ini";

	fa2.ClearAndLoad(path);

	if (ExtConfigs::VerticalLayout)
	{
		std::ostringstream oss;
		oss.precision(3);
		oss << std::fixed << ExtConfigs::IsoViewWidthPercentage;
		fa2.WriteString("UserInterface", "IsoViewWidthPercentage", oss.str().c_str());
	}
	else
	{
		std::ostringstream oss;
		oss.precision(3);
		oss << std::fixed << ExtConfigs::IsoViewHeightPercentage;
		fa2.WriteString("UserInterface", "IsoViewHeightPercentage", oss.str().c_str());
	}

	fa2.WriteToFile(path);

	return 0;
}

DEFINE_HOOK(4D288F, CMyViewFrame_OnSize_StatusBar, 6)
{
	R->EDX(R->EDX() - 130 * CFinalSunAppExt::ProgramScaleFactor);
	return 0x4D2895;
}