#include "Body.h"
#include <CFinalSunDlg.h>
#include <CIsoView.h>

#include <Helpers/Macro.h>
#include "../CFinalSunDlg/Body.h"
#include "../../FA2sp.Constants.h"
#include "../../Helpers/Translations.h"
#include "../../FA2sp.h"
#include "../CIsoView/Body.h"

#include <shlobj.h>
#include <afxwin.h>
#include "../../Helpers/STDHelpers.h"
#include "../CTileSetBrowserFrame/TabPages/GridObjectViewer.h"

DEFINE_HOOK(41FAD0, CFinalSunApp_InitInstance, 8)
{
    R->EAX(CFinalSunAppExt::GetInstance()->InitInstanceExt());

    return 0x422052;
}

DEFINE_HOOK(4229E0, CFinalSunApp_ProcessMessageFilter, 7)
{
    if (!CMapData::Instance->MapWidthPlusHeight) return 0;

    REF_STACK(LPMSG, lpMsg, 0x8);

    if (CViewObjectsExt::CurrentConnectedTileType < 0 
        || CViewObjectsExt::CurrentConnectedTileType > CViewObjectsExt::ConnectedTileSets.size()
        || CIsoView::CurrentCommand->Command != 0x1E)
    {
        auto& gov = GridObjectViewer::Instance;
        if (lpMsg->message == WM_KEYDOWN && gov.IsVisible())
        {
            bool changed = false;
            switch (lpMsg->wParam)
            {
            case VK_LEFT:
                changed = gov.SelectLeft();
                break;
            case VK_RIGHT:
                changed = gov.SelectRight();
                break;

            case VK_UP:
                changed = gov.SelectUp();
                break;

            case VK_DOWN:
                changed = gov.SelectDown();
                break;
            }
            if (changed)
            {
                InvalidateRect(gov.GetView(), NULL, TRUE);
                gov.EnsureVisible(gov.GetSelectedIndex());
                gov.OnSelChanged(gov.GetSelectedIndex()); 

                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(CIsoView::GetInstance()->GetSafeHwnd(), &pt);
                CIsoView::GetInstance()->OnMouseMove(0, pt);
            }
        }
    }
    else
    {
        int currentCTtype = CViewObjectsExt::ConnectedTileSets[CViewObjectsExt::CurrentConnectedTileType].Type;
        if (!CViewObjectsExt::IsInPlaceCliff_OnMouseMove)
        {
            if (lpMsg->message == WM_KEYDOWN)
            {
                auto point = CIsoView::GetInstance()->GetCurrentMapCoord(CIsoView::GetInstance()->MouseCurrentPosition);
                if (lpMsg->wParam == VK_UP && (currentCTtype == CViewObjectsExt::DirtRoad || currentCTtype == CViewObjectsExt::CityDirtRoad || currentCTtype == CViewObjectsExt::Highway))
                {
                    if (CViewObjectsExt::CliffConnectionHeight < 14 && CViewObjectsExt::LastPlacedCT.GetNextHeightOffset() < 1 || CViewObjectsExt::NextCTHeightOffset < 0 || CViewObjectsExt::CliffConnectionHeight < 13)
                    {
                        CViewObjectsExt::RaiseNextCT();
                        CViewObjectsExt::PlaceConnectedTile_OnMouseMove(point.X, point.Y, false);
                    }

                }
                else if (lpMsg->wParam == VK_DOWN && (currentCTtype == CViewObjectsExt::DirtRoad || currentCTtype == CViewObjectsExt::CityDirtRoad || currentCTtype == CViewObjectsExt::Highway))
                {
                    if (CViewObjectsExt::CliffConnectionHeight > 0 || CViewObjectsExt::NextCTHeightOffset > 0 || CViewObjectsExt::CliffConnectionHeight == 0 && CViewObjectsExt::LastPlacedCT.GetNextHeightOffset() > 0)
                    {
                        CViewObjectsExt::LowerNextCT();
                        CViewObjectsExt::PlaceConnectedTile_OnMouseMove(point.X, point.Y, false);
                    }
                }
                else if (lpMsg->wParam == VK_PRIOR)
                {
                    if (CViewObjectsExt::CliffConnectionHeight < 14)
                    {
                        CViewObjectsExt::CliffConnectionHeight++;
                    }

                }
                else if (lpMsg->wParam == VK_NEXT)
                {
                    if (CViewObjectsExt::CliffConnectionHeight > 0)
                    {
                        CViewObjectsExt::CliffConnectionHeight--;
                    }
                }
            }
        }
    }

    return 0;
}

//DEFINE_HOOK(41F720, CFinalSunApp_Initialize, 7)
//{
//    SetProcessDPIAware();
//    return 0;
//}

