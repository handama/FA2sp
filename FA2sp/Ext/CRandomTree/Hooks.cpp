#include "Body.h"

#include <Helpers/Macro.h>
#include <CINI.h>

#include "../../FA2sp.h"
#include "../CFinalSunDlg/Body.h"
#include "../CLoading/Body.h"
#pragma comment(lib, "Msimg32.lib")

DEFINE_HOOK(4D4150, CRandomTree_OnInitDialog, 7)
{
    if (ExtConfigs::RandomTerrainObjects)
    {
        GET(CRandomTree*, pThis, ECX);

        pThis->ppmfc::CDialog::OnInitDialog();

        auto&& terrains = Variables::RulesMap.ParseIndicies("TerrainTypes", true);
        for (size_t i = 0, sz = terrains.size(); i < sz; ++i)
        {
            if (!CViewObjectsExt::IsIgnored(terrains[i]))
                pThis->CLBAvailable.AddString(terrains[i]);
        }

        return 0x4D4865;
    }
    
    return 0;
}

static ppmfc::CString LastSelectedTree;
DEFINE_HOOK(4D4FF7, CRandomTreeDlg_Draw, 7)
{
    GET(CRandomTree*, pThis, EDI);

    if (pThis->CString_Terrain == LastSelectedTree)
        return 0x4D53C3;

    LastSelectedTree = pThis->CString_Terrain;

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(pThis->GetSafeHwnd(), &ps);
    CDC* pDC = CDC::FromHandle(hdc);

    RECT pr;
    pThis->CBNPreviewImage.GetWindowRect(&pr);
    pThis->ScreenToClient(&pr);
    pr.top += 15;
    pr.bottom -= 5;
    pr.left += 5;
    pr.right -= 5;

    const auto& imageName = CLoadingExt::GetImageName(pThis->CString_Terrain, 0);
    if (!CLoadingExt::IsObjectLoaded(pThis->CString_Terrain))
    {
        CLoading::Instance->LoadObjects(pThis->CString_Terrain);
    }
    auto pData = CLoadingExt::GetImageDataFromServer(imageName);
    if (pData->pImageBuffer)
    {
        CBitmap bmp;
        CLoadingExt::LoadShpToBitmap(pData, bmp);
        CDC memDC;
        memDC.CreateCompatibleDC(pDC);
        CBitmap* pOldBmp = memDC.SelectObject(&bmp);
        BITMAP bitmap;
        bmp.GetBitmap(&bitmap);
     
        pDC->FillSolidRect(&pr, ExtConfigs::EnableDarkMode ? RGB(32, 32, 32) : RGB(255, 255, 255));
        COLORREF transparentColor = RGB(255, 0, 255);
        TransparentBlt(
            pDC->GetSafeHdc(),
            pr.left, pr.top, bitmap.bmWidth, bitmap.bmHeight,
            memDC.GetSafeHdc(),
            0, 0, bitmap.bmWidth, bitmap.bmHeight,
            transparentColor
        );
        memDC.SelectObject(pOldBmp);
    }
    else
    {
        pDC->FillSolidRect(&pr, ExtConfigs::EnableDarkMode ? RGB(32, 32, 32) : RGB(255, 255, 255));
    }

    EndPaint(pThis->GetSafeHwnd(), &ps);
    return 0x4D53C3;
}
