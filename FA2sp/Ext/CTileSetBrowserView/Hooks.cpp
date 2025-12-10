#include <CTileSetBrowserView.h>
#include <Helpers/Macro.h>
#include <CPalette.h>
#include <FA2PP.h>

#include "../../FA2sp.h"
#include "../CMapData/Body.h"
#include "../CLoading/Body.h"

ImageDataClass CurrentOverlay;
ImageDataClass* CurrentOverlayPtr = nullptr;
void* NULLPTR = nullptr;

static HRESULT HalveSurface(LPDIRECTDRAWSURFACE7* lpSurface)
{
    if (!lpSurface || !*lpSurface) return E_INVALIDARG;
    LPDIRECTDRAWSURFACE7 lpSrc = *lpSurface;

    DDSURFACEDESC2 ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);

    HRESULT hr = lpSrc->GetSurfaceDesc(&ddsd);
    if (FAILED(hr)) return hr;

    DWORD newWidth = ddsd.dwWidth / 2;
    DWORD newHeight = ddsd.dwHeight / 2;

    LPDIRECTDRAW7 lpDD = nullptr;
    hr = lpSrc->GetDDInterface((LPVOID*)&lpDD);
    if (FAILED(hr)) return hr;

    DDSURFACEDESC2 ddsdNew;
    ZeroMemory(&ddsdNew, sizeof(ddsdNew));
    ddsdNew.dwSize = sizeof(ddsdNew);
    ddsdNew.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsdNew.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsdNew.dwWidth = newWidth;
    ddsdNew.dwHeight = newHeight;
    ddsdNew.ddpfPixelFormat = ddsd.ddpfPixelFormat;

    LPDIRECTDRAWSURFACE7 lpDest = nullptr;
    hr = lpDD->CreateSurface(&ddsdNew, &lpDest, NULL);
    lpDD->Release();
    if (FAILED(hr)) return hr;

    RECT srcRect = { 0, 0, (LONG)ddsd.dwWidth, (LONG)ddsd.dwHeight };
    RECT dstRect = { 0, 0, (LONG)newWidth, (LONG)newHeight };

    hr = lpDest->Blt(&dstRect, lpSrc, &srcRect, DDBLT_WAIT, NULL);
    if (FAILED(hr)) {
        lpDest->Release();
        return hr;
    }

    lpSrc->Release();
    *lpSurface = lpDest;

    return DD_OK;
}

static bool setCurrentOverlay(ImageDataClassSafe* pData)
{
    if (pData && pData->pImageBuffer)
    {
        BGRStruct empty;
        CurrentOverlay.pImageBuffer = pData->pImageBuffer.get();
        CurrentOverlay.pPixelValidRanges = (ImageDataClass::ValidRangeData*)pData->pPixelValidRanges.get();
        CurrentOverlay.pPalette = PalettesManager::GetTileSetBrowserViewPalette(pData->pPalette, empty, false);
        CurrentOverlay.ValidX = pData->ValidX;
        CurrentOverlay.ValidY = pData->ValidY;
        CurrentOverlay.ValidWidth = pData->ValidWidth;
        CurrentOverlay.ValidHeight = pData->ValidHeight;
        CurrentOverlay.FullWidth = pData->FullWidth;
        CurrentOverlay.FullHeight = pData->FullHeight;
        CurrentOverlay.Flag = pData->Flag;
        CurrentOverlay.BuildingFlag = pData->BuildingFlag;
        CurrentOverlay.IsOverlay = pData->IsOverlay;
        CurrentOverlayPtr = &CurrentOverlay;
        return true;
    }
    return false;
}

static int GetAddedHeight(int tileIndex)
{
    int cur_added = 0;
    const auto& tile = CMapDataExt::TileData[tileIndex];
    int i, e, p = 0;;
    for (i = 0; i < tile.Height; i++)
    {
        for (e = 0; e < tile.Width; e++)
        {
            if (tile.TileBlockDatas[p].ImageData == NULL)
            {
                p++;
                continue;
            }
            int drawy = e * 30 / 2 + i * 30 / 2 - tile.Bounds.top;
            drawy += tile.TileBlockDatas[p].YMinusExY - tile.TileBlockDatas[p].Height * 30 / 2;
            if (drawy < cur_added) cur_added = drawy;
            p++;
        }
    }

    return -cur_added;
}

static int GetAddedWidth(int tileIndex)
{
    int cur_added = 0;
    const auto& tile = CMapDataExt::TileData[tileIndex];
    int i, e, p = 0;;
    for (i = 0; i < tile.Height; i++)
    {
        for (e = 0; e < tile.Width; e++)
        {
            if (tile.TileBlockDatas[p].ImageData == NULL)
            {
                p++;
                continue;
            }
            int drawx = e * 60 / 2 - i * 60 / 2 - tile.Bounds.left;
            drawx += tile.TileBlockDatas[p].XMinusExX;
            if (drawx < cur_added) cur_added = drawx;
            p++;
        }
    }

    return -cur_added;
}

static Palette* currentPalette = nullptr;
static __forceinline void BlitTerrainTSB(void* dst, int x, int y,
    int dleft, int dtop, int dpitch, int dright, int dbottom,
    CTileBlockClass& st)
{
    const int bpp = 4;
    BYTE* src = st.ImageData;
    const auto& swidth = st.BlockWidth;
    const auto& sheight = st.BlockHeight;

    if (src == NULL || dst == NULL)
        return;

    if (x + swidth < dleft || y + sheight < dtop)
        return;
    if (x >= dright || y >= dbottom)
        return;

    RECT blrect{};
    RECT srcRect{};
    srcRect.left = 0;
    srcRect.top = 0;
    srcRect.right = swidth;
    srcRect.bottom = sheight;
    blrect.left = x;
    if (blrect.left < 0)
    {
        srcRect.left = 1 - blrect.left;
        blrect.left = 1;
    }
    blrect.top = y;
    if (blrect.top < 0)
    {
        srcRect.top = 1 - blrect.top;
        blrect.top = 1;
    }
    blrect.right = (x + swidth);
    if (x + swidth > dright)
    {
        srcRect.right = dright - x;
        blrect.right = dright;
    }
    blrect.bottom = (y + sheight);
    if (y + sheight > dbottom)
    {
        srcRect.bottom = dbottom - y;
        blrect.bottom = dbottom;
    }

    short i, e;
    for (e = srcRect.top; e < srcRect.bottom; e++)
    {
        short& left = st.pPixelValidRanges[e].First;
        short& right = st.pPixelValidRanges[e].Last;

        for (i = left; i <= right; i++)
        {
            if (i < srcRect.left || i >= srcRect.right)
            {
            }
            else
            {
                BYTE& val = src[i + e * swidth];
                if (val)
                {
                    void* dest = ((BYTE*)dst + (blrect.left + i) * bpp + (blrect.top + e) * dpitch);
                    memcpy(dest, &(*currentPalette)[val], bpp);
                }
            }
        }
    }
}

DEFINE_HOOK(4F36A0, CTileSetBrowserView_RenderTile, 5)
{
    GET_STACK(int, iTileIndex, 0x4);

    if (CFinalSunApp::Instance->FrameMode)
    {
        if (CMapDataExt::TileData[iTileIndex].FrameModeIndex != 0xFFFF)
        {
            iTileIndex = CMapDataExt::TileData[iTileIndex].FrameModeIndex;
        }
    }

    auto pIsoView = CIsoView::GetInstance();
    LPDIRECTDRAWSURFACE7 lpdds = NULL;
    auto lpdd = pIsoView->lpDD7;

    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
    ddsd.dwSize = sizeof(DDSURFACEDESC2);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    int added_height = GetAddedHeight(iTileIndex);
    int added_width = GetAddedWidth(iTileIndex);
    ddsd.dwHeight = CMapDataExt::TileData[iTileIndex].Bounds.bottom - CMapDataExt::TileData[iTileIndex].Bounds.top + added_height;
    ddsd.dwWidth = CMapDataExt::TileData[iTileIndex].Bounds.right - CMapDataExt::TileData[iTileIndex].Bounds.left + added_width;
    if (lpdd->CreateSurface(&ddsd, &lpdds, NULL) != DD_OK)
    {
        R->EAX(NULL);
        return 0x4F3BEF;
    }
    auto pPal = CMapDataExt::TileSetPalettes[CMapDataExt::TileData[iTileIndex].TileSet];
    BGRStruct empty;
    currentPalette = PalettesManager::GetTileSetBrowserViewPalette(pPal, empty, false);

    DDBLTFX ddfx;
    memset(&ddfx, 0, sizeof(DDBLTFX));
    ddfx.dwSize = sizeof(DDBLTFX);
    lpdds->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &ddfx);

    int y_added = ddsd.dwHeight - (CMapDataExt::TileData[iTileIndex].Height * 30 / 2 + CMapDataExt::TileData[iTileIndex].Width * 30 / 2);

    int i, e, p = 0;;
    for (i = 0; i < CMapDataExt::TileData[iTileIndex].Height; i++)
    {
        for (e = 0; e < CMapDataExt::TileData[iTileIndex].Width; e++)
        {
            int drawx = e * 60 / 2 - i * 60 / 2 - CMapDataExt::TileData[iTileIndex].Bounds.left;
            int drawy = e * 30 / 2 + i * 30 / 2 - CMapDataExt::TileData[iTileIndex].Bounds.top;

            drawx += added_width + CMapDataExt::TileData[iTileIndex].TileBlockDatas[p].XMinusExX;
            drawy += added_height + CMapDataExt::TileData[iTileIndex].TileBlockDatas[p].YMinusExY 
                - CMapDataExt::TileData[iTileIndex].TileBlockDatas[p].Height * 30 / 2;

            if (CMapDataExt::TileData[iTileIndex].TileBlockDatas[p].ImageData)
            {
                RECT dest;
                dest.left = drawx;
                dest.top = drawy;
                dest.right = drawx + CMapDataExt::TileData[iTileIndex].TileBlockDatas[p].BlockWidth;
                dest.bottom = drawy + CMapDataExt::TileData[iTileIndex].TileBlockDatas[p].BlockHeight;
                DDBLTFX fx;
                memset(&fx, 0, sizeof(DDBLTFX));
                fx.dwSize = sizeof(DDBLTFX);

                DDSURFACEDESC2 ddsd;
                ZeroMemory(&ddsd, sizeof(ddsd));
                ddsd.dwSize = sizeof(DDSURFACEDESC2);
                ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;

                lpdds->GetSurfaceDesc(&ddsd);

                lpdds->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_NOSYSLOCK, NULL);

                BlitTerrainTSB(ddsd.lpSurface, drawx, drawy, 0, 0,
                    ddsd.lPitch, ddsd.dwWidth, ddsd.dwHeight, CMapDataExt::TileData[iTileIndex].TileBlockDatas[p]);
                lpdds->Unlock(NULL);
            }

            p++;
        }
    }

    CIsoView::SetColorKey(lpdds, -1);

    R->EAX(lpdds);
    return 0x4F3BEF;
}

DEFINE_HOOK(4F3C00, CTileSetBrowserView_OnLButtonDown, 7)
{
    GET(CTileSetBrowserView*, pThis, ECX);
    GET_STACK(UINT, nFlags, 0x4);
    GET_STACK(CPoint, point, 0x8);

    CViewObjectsExt::InitializeOnUpdateEngine();

    RECT r;
    pThis->GetClientRect(&r);

    if (pThis->TileSurfacesCount == 0)
        return 0x4F3EDE;

    auto pIsoView = CIsoView::GetInstance();
    SCROLLINFO scrinfo;
    scrinfo.cbSize = sizeof(SCROLLINFO);
    scrinfo.fMask = SIF_ALL;
    pThis->GetScrollInfo(SB_HORZ, &scrinfo);
    point.x += scrinfo.nPos;
    pThis->GetScrollInfo(SB_VERT, &scrinfo);
    point.y += scrinfo.nPos;

    int max_r = r.right / pThis->CurrentImageWidth;
    if (max_r == 0) max_r = 1;

    int cur_y = 0;
    int cur_x = 0;

    int tile_width = pThis->CurrentImageWidth;
    int tile_height = pThis->CurrentImageHeight;

    if (pThis->CurrentMode == 1)
    {
        auto iTileStart = CMapDataExt::TileSet_starts[pThis->CurrentTileset];
        for (int i = 0; i < pThis->TileSurfacesCount; i++)
        {
            if (point.x > cur_x && point.y > cur_y && point.x < cur_x + tile_width && point.y < cur_y + tile_height)
            {
                CIsoView::CurrentCommand->Command = 10;
                CIsoView::CurrentCommand->Type = iTileStart;
                CIsoView::CurrentCommand->Param = 0;
                CIsoView::CurrentCommand->Overlay = 0;
                CIsoView::CurrentCommand->OverlayData = 0;
                CIsoView::CurrentCommand->Height = 0;

                pThis->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
                return 0x4F3EDE;
            }

            cur_x += tile_width;
            if (i % max_r == max_r - 1)
            {
                cur_y += tile_height;
                cur_x = 0;
            }
            iTileStart++;
        }
    }
    else if (pThis->CurrentMode == 2)
    {
        FString ovlIdx;
        ovlIdx.Format("%d", pThis->SelectedOverlayIndex);
        int nDisplayLimit = CINI::FAData->GetInteger("OverlayDisplayLimit", ovlIdx, 60);
        for (int i = 0; i < std::min(nDisplayLimit, 60); i++)
        {
            auto imageName = CLoadingExt::GetOverlayName(pThis->SelectedOverlayIndex, i);
            auto pData = CLoadingExt::GetImageDataFromMap(imageName);

            if (pData && pData->pImageBuffer)
            {
                if (point.x > cur_x && point.y > cur_y && point.x < cur_x + tile_width && point.y < cur_y + tile_height)
                {
                    CIsoView::CurrentCommand->Command = 1;
                    CIsoView::CurrentCommand->Type = 6;
                    CIsoView::CurrentCommand->Param = 33;
                    CIsoView::CurrentCommand->Overlay = pThis->SelectedOverlayIndex;
                    CIsoView::CurrentCommand->OverlayData = i;
                    CIsoView::CurrentCommand->Height = 0;

                    pThis->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
                    return 0x4F3EDE;
                }
    
                cur_x += tile_width;
                if (i % max_r == max_r - 1)
                {
                    cur_y += tile_height;
                    cur_x = 0;
                }
            }  
        }
    }

    ::SetForegroundWindow(pIsoView->GetSafeHwnd());
    ::SetFocus(pIsoView->GetSafeHwnd());
    pThis->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
    return 0x4F3EDE;
}


DEFINE_HOOK(4F4650, CTileSetBrowserView_GetAddedHeight, 9)
{
    GET_STACK(int, iTileIndex, 0x4);

    R->EAX(GetAddedHeight(iTileIndex));

    return 0x4F4734;
}

DEFINE_HOOK(4F1E93, CTileSetBrowserView_OnDraw_ExtraWidth, 6)
{
    GET(int, iWidth, ECX);
    GET(int, iTileIndex, EDI);
    int newWidth = GetAddedWidth(iTileIndex) + iWidth;
    if (ExtConfigs::ShrinkTilesInTileSetBrowser)
    {
        iWidth /= 2;
        newWidth /= 2;
    }
    if (newWidth > iWidth || ExtConfigs::ShrinkTilesInTileSetBrowser)
        R->ECX(newWidth);

    return 0;
}

DEFINE_HOOK(4F34B1, CTileSetBrowserView_SetTileSet_ExtraWidth, A)
{
    GET(int, iWidth, EAX);
    GET(CTileSetBrowserView*, pThis, ESI);
    GET_STACK(int, iTileIndex, STACK_OFFS(0x220, 0x208));
    int newWidth = GetAddedWidth(iTileIndex) + iWidth;
    if (newWidth > iWidth)
        iWidth = newWidth;

    if (iWidth > pThis->CurrentImageWidth)
        pThis->CurrentImageWidth = iWidth;

    return 0x4F34BB;
}

DEFINE_HOOK(4F2243, CTileSetBrowserView_OnDraw_LoadOverlayImage, 6)
{
    GET(CTileSetBrowserView*, pThis, ESI);
    GET(const int, i, ECX);

    auto imageName = CLoadingExt::GetOverlayName(pThis->SelectedOverlayIndex, i);
    auto pData = CLoadingExt::GetImageDataFromMap(imageName);
    if (setCurrentOverlay(pData))
    {
        R->EAX(&CurrentOverlay);
    }
    return 0;
}

DEFINE_HOOK(4F361E, CTileSetBrowserView_SetTileSet_ShrinkImage, 6)
{
    if (!ExtConfigs::ShrinkTilesInTileSetBrowser)
        return 0;

    GET(CTileSetBrowserView*, pThis, ESI);
    for (int i = 0; i < pThis->TileSurfacesCount; ++i)
    {
        HalveSurface(&pThis->TileSurfaces[i]);
    }
    pThis->CurrentImageWidth /= 2;
    pThis->CurrentImageHeight /= 2;
    pThis->ScrollWidth /= 2;
    return 0;
}

DEFINE_HOOK(4F4774, CTileSetBrowserView_SetOverlay_LoadOverlayImage, 5)
{
    GET(CTileSetBrowserView*, pThis, ESI);
    GET(int, Overlay, EBX);
    const int max_ovrl_img = 60;

    int need_pos = -1;
    int need_width = 0;
    int need_height = 0;
    int iovrlcount = 0;

    if (CMapData::Instance->MapWidthPlusHeight)
    {
        auto obj = Variables::RulesMap.GetValueAt("OverlayTypes", Overlay);
        if (!CLoadingExt::IsOverlayLoaded(obj))
        {
            CLoadingExt::GetExtension()->LoadOverlay(obj, Overlay);
        }
        for (int i = 0; i < max_ovrl_img; i++)
        {
            auto imageName = CLoadingExt::GetOverlayName(Overlay, i);
            auto pData = CLoadingExt::GetImageDataFromMap(imageName);
            if (pData && pData->pImageBuffer)
            {
                iovrlcount++;
            }
        }
        for (int i = 0; i < max_ovrl_img; i++)
        {
            auto imageName = CLoadingExt::GetOverlayName(Overlay, i);
            auto pData = CLoadingExt::GetImageDataFromMap(imageName);
            if (pData && pData->pImageBuffer)
            {
                need_pos = i;
                need_width = pData->FullWidth;
                need_height = pData->FullHeight;
                break;
            }
        }
    }
    
    R->ECX(need_pos);
    R->EDI(need_width);
    R->EBP(need_height);
    R->Stack(STACK_OFFS(0x90, 0x80), iovrlcount);

    return 0x4F48D0;
}

DEFINE_HOOK(4F258B, CTileSetBrowserView_OnDraw_SetOverlayFrameToDisplay, 7)
{
    GET(CTileSetBrowserView*, pThis, ESI);
    GET(const int, i, ECX);

    ppmfc::CString ovlIdx;
    ovlIdx.Format("%d", pThis->SelectedOverlayIndex);
    int nDisplayLimit = CINI::FAData->GetInteger("OverlayDisplayLimit", ovlIdx, 60);
    if (nDisplayLimit > 60)
        nDisplayLimit = 60;

    R->Stack(STACK_OFFS(0xDC, 0xB8), i);
    return i < nDisplayLimit ? 0x4F2230 : 0x4F2598;
}

DEFINE_HOOK(4F22F7, CTileSetBrowserView_OnDraw_OverlayPalette, 5)
{
    GET(Palette*, pPalette, EAX);
    GET_STACK(RGBTRIPLE*, pBytePalette, STACK_OFFS(0xDC, 0xBC));

    for (int i = 0; i < 256; i++)
    {
        RGBTRIPLE ret;
        ret.rgbtBlue = pPalette->Data[i].B;
        ret.rgbtGreen = pPalette->Data[i].G;
        ret.rgbtRed = pPalette->Data[i].R;
        pBytePalette[i] = ret;
    }

    return 0x4F2315;
}

DEFINE_HOOK(4F22D6, CTileSetBrowserView_OnDraw_OverlayBackground, 6)
{
    //  32 : 255
    R->EAX(ExtConfigs::EnableDarkMode ? 0x20202020 : 0xFFFFFFFF);
    R->ECX(R->ECX() >> 2);
    return 0x4F22DC;
}

DEFINE_HOOK(4F1EAD, CTileSetBrowserView_OnDraw_SkipDisableTile_Height, 5)
{
    //disable this tile
    //GET(int, currentTileSet, EAX);
    //GET_STACK(int, currentTileIndex, STACK_OFFS(0xDC, 0xC4));
    //return 0x4F21F5;
    if (ExtConfigs::ShrinkTilesInTileSetBrowser)
    {
        GET(int, iHeight, EDI);
        R->EDI(iHeight / 2);
    }
    return 0x4F1F68;
}

DEFINE_HOOK(4F3D23, CTileSetBrowserView_OnLButtonDown_SkipDisableTile, 7)
{
    R->Stack(STACK_OFFS(0x210, 0x1E8), R->EDX());
    //disable this tile
    //GET(int, currentTileSet, EAX);
    //GET_STACK(int, currentTileIndex, STACK_OFFS(0x210, 0x1F0));
    //return 0x4F3DE1;
    return 0x4F3DF7;
}
