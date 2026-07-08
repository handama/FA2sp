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
                    dwStyle &= ~WS_SYSMENU;
                    ::SetWindowLong(hTileBrowser, GWL_STYLE, dwStyle);
                    DWORD dwExStyle = ::GetWindowLong(hTileBrowser, GWL_EXSTYLE);
                    dwExStyle |= WS_EX_TOOLWINDOW;
                    ::SetWindowLong(hTileBrowser, GWL_EXSTYLE, dwExStyle);
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

                    DarkTheme::SetDarkTheme(hTileBrowser);
                    if (GridObjectViewer::Instance.IsValid()) {
                        HWND hCtrl = GridObjectViewer::Instance.GetControl();
                        if (hCtrl) {
                            DarkTheme::SetDarkTheme(hCtrl);
                            DarkTheme::SubclassAllControls(hCtrl);
                        }
                    }
                    ::ShowWindow(hTileBrowser, SW_HIDE);
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
                    dwStyle &= ~WS_SYSMENU;
                    ::SetWindowLong(hViewObjs, GWL_STYLE, dwStyle);
                    DWORD dwExStyle = ::GetWindowLong(hViewObjs, GWL_EXSTYLE);
                    dwExStyle |= WS_EX_TOOLWINDOW;
                    ::SetWindowLong(hViewObjs, GWL_EXSTYLE, dwExStyle);
                    ::SetParent(hViewObjs, NULL);
                    ::SetWindowLong(hViewObjs, GWL_HWNDPARENT, (LONG)pThis->GetSafeHwnd());

                    RECT rcMain;
                    ::GetWindowRect(pThis->GetSafeHwnd(), &rcMain);
                    int w = (rcMain.right - rcMain.left) * 1 / 6;
                    int h = (rcMain.bottom - rcMain.top) * 3 / 4;
                    int x = rcMain.left;
                    int y = rcMain.top;
                    ::SetWindowPos(hViewObjs, NULL, x, y, w, h, SWP_NOZORDER | SWP_FRAMECHANGED);

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

                pThis->Minimap.Update();
                if (pThis->Minimap.m_hWnd && !CMinimapExt::g_pfnOriginalMinimapProc)
                {
                    CMinimapExt::g_pfnOriginalMinimapProc = (WNDPROC)SetWindowLongPtr(
                        pThis->Minimap.GetSafeHwnd(),
                        GWLP_WNDPROC,
                        (LONG_PTR)CMinimapExt::MinimapWndProc
                    );
                }

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