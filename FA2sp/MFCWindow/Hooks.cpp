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
        if (bRes = pThis->SplitterWnd.CreateView(0, 0, reinterpret_cast<ppmfc::CRuntimeClass*>(0x598710), size, pContent))
        {
            if (bRes = pThis->SplitterWnd.CreateView(0, 1, &CRightFrame::RuntimeClass, size, pContent))
            {
                // CMyViewFrame::OnCreateClient(): windows created\n
                pThis->pRightFrame = (CRightFrame*)pThis->SplitterWnd.GetPane(0, 1);
                pThis->pIsoView = (CIsoView*)pThis->pRightFrame->CSplitter.GetPane(0, 0);
                pThis->pIsoView->pParent = pThis;
                if (ExtConfigs::VerticalLayout) {
                    pThis->pTileSetBrowserFrame = (CTileSetBrowserFrame*)pThis->pRightFrame->CSplitter.GetPane(0, 1);
                }
                else {
                    pThis->pTileSetBrowserFrame = (CTileSetBrowserFrame*)pThis->pRightFrame->CSplitter.GetPane(1, 0);
                }
                pThis->pViewObjects = (CViewObjects*)pThis->SplitterWnd.GetPane(0, 0);
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