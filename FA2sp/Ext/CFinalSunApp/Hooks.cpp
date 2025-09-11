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

DEFINE_HOOK(41FAD0, CFinalSunApp_InitInstance, 8)
{
    R->EAX(CFinalSunAppExt::GetInstance()->InitInstanceExt());

    return 0x422052;
}

DEFINE_HOOK(4229E0, CFinalSunApp_ProcessMessageFilter, 7)
{
    REF_STACK(LPMSG, lpMsg, 0x8);
    if (!CMapData::Instance->MapWidthPlusHeight) return 0;
    if (CViewObjectsExt::CurrentConnectedTileType < 0 || CViewObjectsExt::CurrentConnectedTileType > CViewObjectsExt::ConnectedTileSets.size()) return 0;
    int currentCTtype = CViewObjectsExt::ConnectedTileSets[CViewObjectsExt::CurrentConnectedTileType].Type;
    if (currentCTtype == CViewObjectsExt::DirtRoad || currentCTtype == CViewObjectsExt::CityDirtRoad || currentCTtype == CViewObjectsExt::Highway
        && !CViewObjectsExt::IsInPlaceCliff_OnMouseMove)
    {
        if (lpMsg->message == WM_KEYDOWN)
        {
            auto point = CIsoView::GetInstance()->GetCurrentMapCoord(CIsoView::GetInstance()->MouseCurrentPosition);
            if (lpMsg->wParam == VK_PRIOR || lpMsg->wParam == VK_UP)
            {
                if (CViewObjectsExt::CliffConnectionHeight < 14 && CViewObjectsExt::LastPlacedCT.GetNextHeightOffset() < 1 || CViewObjectsExt::NextCTHeightOffset < 0 || CViewObjectsExt::CliffConnectionHeight < 13)
                {
                    CViewObjectsExt::RaiseNextCT();
                    CViewObjectsExt::PlaceConnectedTile_OnMouseMove(point.X, point.Y, false);
                }

            }
            else if (lpMsg->wParam == VK_NEXT || lpMsg->wParam == VK_DOWN)
            {
                if (CViewObjectsExt::CliffConnectionHeight > 0 || CViewObjectsExt::NextCTHeightOffset > 0 || CViewObjectsExt::CliffConnectionHeight == 0 && CViewObjectsExt::LastPlacedCT.GetNextHeightOffset() > 0)
                {
                    CViewObjectsExt::LowerNextCT();
                    CViewObjectsExt::PlaceConnectedTile_OnMouseMove(point.X, point.Y, false);
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

