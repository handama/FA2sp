#include <CTileSetBrowserView.h>
#include <Helpers/Macro.h>
#include <CPalette.h>
#include <FA2PP.h>

#include "../../FA2sp.h"
#include "../CMapData/Body.h"
#include "../CLoading/Body.h"
#include "../../Miscs/Hooks.INI.h"
#include "../../Helpers/TheaterHelpers.h"
#include "../CFinalSunApp/Body.h"
#include "../CTileSetBrowserFrame/Body.h"

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

    if (tileIndex < CUSTOM_TILE_START)
    {
        const auto& tile = CMapDataExt::TileData[tileIndex];
        int i, e, p = 0;
        for (i = 0; i < tile.Height; i++)
        {
            for (e = 0; e < tile.Width; e++)
            {
                if (p >= tile.TileBlockCount)
                    break;

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
    }
    else
    {
        return 0;
    }

    return -cur_added;
}

static int GetAddedWidth(int tileIndex)
{
    int cur_added = 0;
    if (tileIndex < CUSTOM_TILE_START)
    {
        const auto& tile = CMapDataExt::TileData[tileIndex];
        int i, e, p = 0;;
        for (i = 0; i < tile.Height; i++)
        {
            for (e = 0; e < tile.Width; e++)
            {
                if (p >= tile.TileBlockCount)
                    break;

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
    }
    else
    {
        return 0;
    }

    return -cur_added;
}

static __forceinline void BlitTerrainTSB(void* dst, int x, int y,
    int dleft, int dtop, int dpitch, int dright, int dbottom,
    CTileBlockClass& st, Palette* pal)
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
    }
    blrect.top = y;
    if (blrect.top < 0)
    {
        srcRect.top = 1 - blrect.top;
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
                    memcpy(dest, &(*pal)[val], bpp);
                }
            }
        }
    }
}

static void GetCustomTileSize(const CustomTile* tileData, int& width, int& height)
{
    int left = INT_MAX;
    int right = INT_MIN;
    int top = INT_MAX;
    int bottom = INT_MIN;
    width = 0;
    height = 0;
    for (int i = 0; i < tileData->Width * tileData->Height; ++i)
    {
        auto& tile = tileData->TileBlockDatas[i];
        auto subtile = tile.GetDisplayTileBlock();
        if (!subtile) continue;
        int x = i % tileData->Width;
        int y = i / tileData->Width;
    
        top = std::min((x + y) * 30 / 2 - tile.GetHeight() * 30 / 2 + subtile->YMinusExY, top);
        bottom = std::max((x + y) * 30 / 2 - tile.GetHeight() * 30 / 2 + subtile->BlockHeight + subtile->YMinusExY, bottom);
        left = std::min((x - y) * 60 / 2 + subtile->XMinusExX, left);
        right = std::max((x - y) * 60 / 2 + subtile->XMinusExX + subtile->BlockWidth, right);
    }
    width = std::max(right - left, width);
    height = std::max(bottom - top, height);
}

static LPDIRECTDRAWSURFACE7 RenderTile(int iTileIndex)
{
    if (iTileIndex < CUSTOM_TILE_START)
    {
        if (CFinalSunApp::Instance->FrameMode)
        {
            if (CMapDataExt::TileData[iTileIndex].FrameModeIndex != 0xFFFF)
            {
                iTileIndex = CMapDataExt::TileData[iTileIndex].FrameModeIndex;
            }
        }
        auto& tile = CMapDataExt::TileData[iTileIndex];

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
        ddsd.dwHeight = tile.Bounds.bottom - tile.Bounds.top + added_height;
        ddsd.dwWidth = tile.Bounds.right - tile.Bounds.left + added_width;
        if (lpdd->CreateSurface(&ddsd, &lpdds, NULL) != DD_OK)
        {
            return nullptr;
        }
        auto pPal = CMapDataExt::TileSetPalettes[tile.TileSet];
        BGRStruct empty;
        auto currentPalette = PalettesManager::GetTileSetBrowserViewPalette(pPal, empty, false);

        DDBLTFX ddfx;
        memset(&ddfx, 0, sizeof(DDBLTFX));
        ddfx.dwSize = sizeof(DDBLTFX);
        lpdds->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &ddfx);

        int i, e, p = 0;;
        for (i = 0; i < tile.Height; i++)
        {
            for (e = 0; e < tile.Width; e++)
            {
                if (p >= tile.TileBlockCount)
                    break;

                int drawx = e * 60 / 2 - i * 60 / 2 - tile.Bounds.left;
                int drawy = e * 30 / 2 + i * 30 / 2 - tile.Bounds.top;

                drawx += added_width + tile.TileBlockDatas[p].XMinusExX;
                drawy += added_height + tile.TileBlockDatas[p].YMinusExY
                    - tile.TileBlockDatas[p].Height * 30 / 2;

                if (tile.TileBlockDatas[p].ImageData)
                {
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
                        ddsd.lPitch, ddsd.dwWidth, ddsd.dwHeight,
                        tile.TileBlockDatas[p],
                        currentPalette);
                    lpdds->Unlock(NULL);
                }

                p++;
            }
        }

        CIsoView::SetColorKey(lpdds, -1);
        return lpdds;
    }
    else
    {
        auto pIsoView = CIsoView::GetInstance();
        LPDIRECTDRAWSURFACE7 lpdds = NULL;
        auto lpdd = pIsoView->lpDD7;
        auto tileData = CMapDataExt::GetCustomTile(iTileIndex);

        int width;
        int height;
        GetCustomTileSize(tileData, width, height);

        DDSURFACEDESC2 ddsd;
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(DDSURFACEDESC2);
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
        ddsd.dwHeight = height;
        ddsd.dwWidth = width;
        if (lpdd->CreateSurface(&ddsd, &lpdds, NULL) != DD_OK)
        {
            return nullptr;
        }

        DDBLTFX ddfx;
        memset(&ddfx, 0, sizeof(DDBLTFX));
        ddfx.dwSize = sizeof(DDBLTFX);
        lpdds->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &ddfx);

        int minDrawX = INT_MAX;
        int minDrawY = INT_MAX;

        int i, e, p = 0;
        for (i = 0; i < tileData->Height; i++)
        {
            for (e = 0; e < tileData->Width; e++)
            {
                auto& tile = tileData->TileBlockDatas[p];
                auto block = tile.GetDisplayTileBlock();
                if (!block)
                {
                    p++;
                    continue;
                }
                if (block->ImageData)
                {
                    int drawx = e * 60 / 2 - i * 60 / 2
                        + 30
                        + block->XMinusExX;
                    int drawy = e * 30 / 2 + i * 30 / 2
                        + block->YMinusExY
                        - tile.GetHeight() * 30 / 2;

                    minDrawX = std::min(minDrawX, drawx);
                    minDrawY = std::min(minDrawY, drawy);
                }
                p++;
            }
        }
        p = 0;
        for (i = 0; i < tileData->Height; i++)
        {
            for (e = 0; e < tileData->Width; e++)
            {
                auto& tile = tileData->TileBlockDatas[p];
                auto block = tile.GetDisplayTileBlock();
                if (!block)
                {
                    p++;
                    continue;
                }
                if (block->ImageData)
                {
                    int drawx = e * 60 / 2 - i * 60 / 2
                        + 30
                        + block->XMinusExX;
                    int drawy = e * 30 / 2 + i * 30 / 2
                        + block->YMinusExY
                        - tile.GetHeight() * 30 / 2;

                    auto pPal = CMapDataExt::TileSetPalettes
                        [CMapDataExt::TileData[tile.GetDisplayTileIndex()].TileSet];
                    BGRStruct empty;
                    auto currentPalette = PalettesManager::GetTileSetBrowserViewPalette(pPal, empty, false);

                    DDBLTFX fx;
                    memset(&fx, 0, sizeof(DDBLTFX));
                    fx.dwSize = sizeof(DDBLTFX);

                    DDSURFACEDESC2 ddsd;
                    ZeroMemory(&ddsd, sizeof(ddsd));
                    ddsd.dwSize = sizeof(DDSURFACEDESC2);
                    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;

                    lpdds->GetSurfaceDesc(&ddsd);

                    lpdds->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_NOSYSLOCK, NULL);

                    BlitTerrainTSB(ddsd.lpSurface, drawx - minDrawX, drawy - minDrawY, 0, 0,
                        ddsd.lPitch, ddsd.dwWidth, ddsd.dwHeight,
                        *block,
                        currentPalette);
                    lpdds->Unlock(NULL);
                }
                p++;
            }
        }

        CIsoView::SetColorKey(lpdds, -1);
        return lpdds;
    }
}

DEFINE_HOOK(4F36A0, CTileSetBrowserView_RenderTile, 5)
{
    GET_STACK(int, iTileIndex, 0x4);

    R->EAX(RenderTile(iTileIndex));
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
        int iTileStart;
        if (pThis->CurrentTileset < 10000)
            iTileStart = CMapDataExt::TileSet_starts[pThis->CurrentTileset];
        else
            iTileStart = CMapDataExt::GetCustomTileIndex(pThis->CurrentTileset, 0);
        for (int i = 0; i < pThis->TileSurfacesCount; i++)
        {
            if (point.x > cur_x && point.y > cur_y && point.x < cur_x + tile_width && point.y < cur_y + tile_height)
            {
                int oldmode = CIsoView::CurrentCommand->Command;
                int oldid = CIsoView::CurrentCommand->Type;
                int oldset = 0;

                CIsoView::CurrentCommand->Command = 10;
                CIsoView::CurrentCommand->Type = iTileStart;
                CIsoView::CurrentCommand->Param = 0;
                CIsoView::CurrentCommand->Overlay = 0;
                CIsoView::CurrentCommand->OverlayData = 0;
                CIsoView::CurrentCommand->Height = 0;

                if (oldid > CUSTOM_TILE_START) oldset = CMapDataExt::GetCustomTileSet(oldid);
                else if (oldid > CMapDataExt::TileDataCount)
                {
                    oldid = 0;
                    oldset = CMapDataExt::TileData[oldid].TileSet;
                }

                if (oldmode != 10 || oldset != pThis->CurrentTileset)
                {
                    CFinalSunDlg::Instance->BrushSize.nCurSel = 0;
                    CFinalSunDlg::Instance->BrushSize.UpdateData(FALSE);
                    pIsoView->BrushSizeX = 1;
                    pIsoView->BrushSizeY = 1;
                }

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

DEFINE_HOOK(4F2B10, CTileSetBrowserView_SetTileSet, 7)
{
    GET(CTileSetBrowserView*, pThis, ECX);
    GET_STACK(DWORD, dwTileSet, 0x4);
    GET_STACK(BOOL, bOnlyRedraw, 0x8);

    pThis->CurrentTileset = dwTileSet;
    pThis->CurrentMode = 1;

    pThis->CurrentImageWidth = 0;
    pThis->CurrentImageHeight = 0;

    auto pIsoView = CIsoView::GetInstance();
    int tileCount = 0;
    if (dwTileSet < 10000)
    {
        int tileStart = CMapDataExt::TileSet_starts[dwTileSet];
        const auto& tileData = CMapDataExt::TileData[tileStart];
        tileCount = CMapDataExt::TileSet_starts[dwTileSet + 1] - CMapDataExt::TileSet_starts[dwTileSet];

        if (tileData.TileBlockCount && tileData.TileBlockDatas[0].ImageData)
        {
            if (!bOnlyRedraw)
            {
                CIsoView::CurrentCommand->Command = 10;
                CIsoView::CurrentCommand->Type = tileStart;
                CIsoView::CurrentCommand->Param = 0;
                CIsoView::CurrentCommand->Overlay = 0;
                CIsoView::CurrentCommand->OverlayData = 0;
                CIsoView::CurrentCommand->Height = 0;

                CFinalSunDlg::Instance->BrushSize.nCurSel = 0;
                CFinalSunDlg::Instance->BrushSize.UpdateData(FALSE);
                pIsoView->BrushSizeX = 1;
                pIsoView->BrushSizeY = 1;
            }
        }

        for (int i = 0; i < tileCount; i++)
        {
            int tileIndex = tileStart + i;
            if (tileIndex < CMapDataExt::TileDataCount)
            {
                int width = GetAddedWidth(tileIndex)
                    + CMapDataExt::TileData[tileIndex].Bounds.right
                    - CMapDataExt::TileData[tileIndex].Bounds.left;
                int height = GetAddedHeight(tileIndex)
                    + CMapDataExt::TileData[tileIndex].Bounds.bottom
                    - CMapDataExt::TileData[tileIndex].Bounds.top;

                pThis->CurrentImageWidth = std::max(width, pThis->CurrentImageWidth);
                pThis->CurrentImageHeight = std::max(height, pThis->CurrentImageHeight);
            }
        }

        pThis->CurrentImageWidth += 6;
        pThis->CurrentImageHeight += 6;

        if (pThis->TileSurfaces)
        {
            for (int i = 0; i < pThis->TileSurfacesCount; i++)
            {
                if (pThis->TileSurfaces[i]) pThis->TileSurfaces[i]->Release();
            }
            GameDeleteArray(pThis->TileSurfaces, pThis->TileSurfacesCount);
        }

        pThis->TileSurfacesCount = tileCount;

        pThis->TileSurfaces = GameCreateArray<LPDIRECTDRAWSURFACE7>(tileCount);
        for (int i = 0; i < tileCount; i++)
        {
            pThis->TileSurfaces[i] = RenderTile(tileStart + i);
            if (ExtConfigs::ShrinkTilesInTileSetBrowser)
                HalveSurface(&pThis->TileSurfaces[i]);
        }
    }
    else
    {
        const auto& tileSet = CMapDataExt::CustomTiles[dwTileSet];
        tileCount = tileSet.size();

        if (tileCount > 0)
        {
            if (!bOnlyRedraw)
            {
                CIsoView::CurrentCommand->Command = 10;
                CIsoView::CurrentCommand->Type = CMapDataExt::GetCustomTileIndex(dwTileSet, 0);
                CIsoView::CurrentCommand->Param = 0;
                CIsoView::CurrentCommand->Overlay = 0;
                CIsoView::CurrentCommand->OverlayData = 0;
                CIsoView::CurrentCommand->Height = 0;
        
                CFinalSunDlg::Instance->BrushSize.nCurSel = 0;
                CFinalSunDlg::Instance->BrushSize.UpdateData(FALSE);
                pIsoView->BrushSizeX = 1;
                pIsoView->BrushSizeY = 1;
            }
        }

        for (int i = 0; i < tileCount; i++)
        {
            auto& tileData = tileSet[i];
            int tileIndex = CMapDataExt::GetCustomTileIndex(dwTileSet, i);

            int width;
            int height;
            GetCustomTileSize(&tileData, width, height);

            pThis->CurrentImageWidth = std::max(width, pThis->CurrentImageWidth);
            pThis->CurrentImageHeight = std::max(height, pThis->CurrentImageHeight);
        }

        pThis->CurrentImageWidth += 6;
        pThis->CurrentImageHeight += 6;

        if (pThis->TileSurfaces)
        {
            for (int i = 0; i < pThis->TileSurfacesCount; i++)
            {
                if (pThis->TileSurfaces[i]) pThis->TileSurfaces[i]->Release();
            }
            GameDeleteArray(pThis->TileSurfaces, pThis->TileSurfacesCount);
        }

        pThis->TileSurfacesCount = tileCount;

        pThis->TileSurfaces = GameCreateArray<LPDIRECTDRAWSURFACE7>(tileCount);
        for (int i = 0; i < tileCount; i++)
        {
            pThis->TileSurfaces[i] = RenderTile(CMapDataExt::GetCustomTileIndex(dwTileSet, i));
            if (ExtConfigs::ShrinkTilesInTileSetBrowser)
                HalveSurface(&pThis->TileSurfaces[i]);
        }
    }
   
    if (ExtConfigs::ShrinkTilesInTileSetBrowser)
    {
        pThis->CurrentImageWidth /= 2;
        pThis->CurrentImageHeight /= 2;
    }

    RECT r;
    pThis->GetClientRect(&r);
    int max_r = r.right / pThis->CurrentImageWidth;
    if (max_r <= 0) max_r = 1;
    pThis->ScrollWidth = pThis->CurrentImageHeight * (1 + tileCount / max_r);
    pThis->GetParentFrame()->RecalcLayout(TRUE);
    pThis->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
    ::SetForegroundWindow(pIsoView->GetSafeHwnd());
    ::SetFocus(pIsoView->GetSafeHwnd());

    return 0x4F368F;
}

DEFINE_HOOK(4F1D70, CTileSetBrowserView_OnDraw, 6)
{
    GET(CTileSetBrowserView*, pThis, ECX);
    GET_STACK(CDC* , pDC, 0x4);

    if (pThis->CurrentMode == 1)
    {
        auto pIsoView = CIsoView::GetInstance();

        if (pIsoView->IsInitializing 
            || pIsoView->lpDDPrimarySurface == NULL 
            || pIsoView->lpDDPrimarySurface->IsLost() != DD_OK
            || !CMapDataExt::TileData
            || CMapDataExt::TileDataCount == 0
            || pThis->CurrentImageWidth == 0)
            return 0x4F25B0;

        RECT r;
        pThis->GetClientRect(&r);

        int max_r = r.right / pThis->CurrentImageWidth;

        int cur_y = 0;
        int cur_x = 0;

        int tileIndex;
        if (pThis->CurrentTileset < 10000)
            tileIndex = CMapDataExt::TileSet_starts[pThis->CurrentTileset];
        else
            tileIndex = CMapDataExt::GetCustomTileIndex(pThis->CurrentTileset, 0);

        for (int i = 0; i < pThis->TileSurfacesCount; i++)
        {
            if (!pThis->TileSurfaces[i]) continue;

            int curwidth, curheight;
            if (pThis->CurrentTileset < 10000)
            {
                curwidth = GetAddedWidth(tileIndex)
                    + CMapDataExt::TileData[tileIndex].Bounds.right
                    - CMapDataExt::TileData[tileIndex].Bounds.left;
                curheight = GetAddedHeight(tileIndex)
                    + CMapDataExt::TileData[tileIndex].Bounds.bottom
                    - CMapDataExt::TileData[tileIndex].Bounds.top;
            }
            else
            {
                auto tileData = CMapDataExt::GetCustomTile(tileIndex);
                GetCustomTileSize(tileData, curwidth, curheight);
            }

            if (ExtConfigs::ShrinkTilesInTileSetBrowser)
            {
                curwidth /= 2;
                curheight /= 2;
            }
            if (cur_y + curheight + (pThis->CurrentImageHeight - curheight) / 2 
                >= pThis->GetScrollPos(SB_VERT) && cur_y <= pThis->GetScrollPos(SB_VERT) + r.bottom)
            {

                HDC hDC = NULL;
                pThis->TileSurfaces[i]->GetDC(&hDC);

                HDC hTmpDC = CreateCompatibleDC(hDC);
                HBITMAP hBitmap = CreateCompatibleBitmap(hDC, curwidth, curheight);
                SelectObject(hTmpDC, hBitmap);

                BitBlt(hTmpDC, 0, 0, curwidth, curheight, hDC, 0, 0, SRCCOPY);

                pThis->TileSurfaces[i]->ReleaseDC(hDC);

                BitBlt(pDC->GetSafeHdc(), 
                    cur_x + (pThis->CurrentImageWidth - curwidth) / 2,
                    cur_y + (pThis->CurrentImageHeight - curheight) / 2,
                    curwidth, curheight, hTmpDC, 0, 0, SRCCOPY);


                DeleteDC(hTmpDC);
                DeleteObject(hBitmap);

                if (CIsoView::CurrentCommand->Command == 10 && CIsoView::CurrentCommand->Type == tileIndex)
                {
                    CPen p;
                    CBrush b;
                    p.CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
                    b.CreateStockObject(NULL_BRUSH);

                    CPen* old = pDC->SelectObject(&p);

                    pDC->SetBkMode(TRANSPARENT);
                    pDC->SelectObject(&b);
                    pDC->Rectangle(cur_x + 2, cur_y + 2, 
                        cur_x + pThis->CurrentImageWidth - 2,
                        cur_y + pThis->CurrentImageHeight - 2);

                    pDC->SelectObject(old);
                }
            }

            cur_x += pThis->CurrentImageWidth;
            if (max_r == 0) max_r = 1;
            if (i % max_r == max_r - 1)
            {
                cur_y += pThis->CurrentImageHeight;
                cur_x = 0;
            }
            tileIndex++;
        }
        return 0x4F25B0;
    }
    else
        return 0;
}

DEFINE_HOOK(4F4650, CTileSetBrowserView_GetAddedHeight, 9)
{
    GET_STACK(int, iTileIndex, 0x4);

    R->EAX(GetAddedHeight(iTileIndex));

    return 0x4F4734;
}

DEFINE_HOOK(4F0B20, CTerrainDlg_Update_Init, 7)
{
    if (CTileSetBrowserFrameExt::TerrainDlgLoaded)
        return 0x4F17AB;

    CTileSetBrowserFrameExt::TerrainDlgLoaded = true;
    return 0;
}

static bool ParsePrefixedInt(const std::string& input, const std::string& prefix, int& outValue)
{
    if (input.size() <= prefix.size())
        return false;

    if (input.compare(0, prefix.size(), prefix) != 0)
        return false;

    const char* p = input.c_str() + prefix.size();
    if (*p == '\0')
        return false;

    char* end = nullptr;
    errno = 0;
    long val = std::strtol(p, &end, 10);

    if (*end != '\0')
        return false;

    if (errno == ERANGE || val < INT_MIN || val > INT_MAX)
        return false;

    outValue = static_cast<int>(val);
    return true;
}

DEFINE_HOOK(4F128A, CTerrainDlg_Update_AddCustomTiles, 5)
{
    auto theater = TheaterHelpers::GetCurrentSuffix();

    CINIExt ini;
    FString path = CFinalSunAppExt::ExePathExt;
    path += "\\presets_";
    path += theater;
    path += ".map";
    CMapDataExt::CustomTiles.clear();
    Translations::CustomTileSetNames.clear();

    if (ini.ClearAndLoad(path) == 0)
    {
        struct TileInfo
        {
            int TileIndex;
            short TileSubIndex;
            short Height;
        };

        GET(CTileSetBrowserFrame*, pThis, ESI);
        HWND hTileComboBox = GetDlgItem(pThis->GetSafeHwnd(), 1366);
        int count = SendMessage(hTileComboBox, CB_GETCOUNT, NULL, NULL);

        std::vector<IsoMapPack5Entry> entry;
        CMapDataExt::UnPackExt(ini, entry);

        std::set<MapCoord> ignoreCoords;
        std::set<int> tileSets;
        std::map<MapCoord, TileInfo> infos;

        for (const auto& e : entry)
        {
            infos[{e.Y, e.X}] = { e.TileIndex, e.TileSubIndex, e.Level };
        }

        if (auto pSection = ini.GetSection("Waypoints"))
        {
            for (const auto& [key, value] : pSection->GetEntities())
            {
                int x = atoi(value) / 1000;
                int y = atoi(value) % 1000;
                ignoreCoords.insert({ x,y });
            }
        }

        if (auto pSection = ini.GetSection("Annotations"))
        {
            for (const auto& [key, value] : pSection->GetEntities())
            {
                int x = atoi(key) / 1000;
                int y = atoi(key) % 1000;
                auto atoms = FString::SplitString(value, 6);
                FString text = atoms[5];
                for (int i = 6; i < atoms.size() - 1; i++)
                {
                    text += ",";
                    text += atoms[i];
                }
                auto lines = FString::SplitString(text, "\\n");

                MapCoord start{ x,y };
                MapCoord end{ x,y };
                int tileSet = 0;

                for (const auto& line : lines)
                {
                    auto kvp = FString::SplitKeyValue(line);
                    int name;
                    if (kvp.first == "Size")
                    {
                        auto p = FString::SplitString(kvp.second, 1);
                        end = { start.X + atoi(p[1]),start.Y + atoi(p[0])};
                    }
                    else if (kvp.first == "TileSet")
                    {
                        tileSet = atoi(kvp.second);
                    }
                    else if (ParsePrefixedInt(kvp.first, (CFinalSunApp::Instance->Language + "-").m_pchData, name))
                    {
                        Translations::CustomTileSetNames[name] = kvp.second;
                    }
                }

                if (tileSet >= 10000 && start != end)
                {
                    bool valid = true;
                    for (int x = start.X; x < end.X; ++x)
                    {
                        for (int y = start.Y; y < end.Y; ++y)
                        {
                            auto& info = infos[{x, y}];
                            if (info.TileIndex > CMapDataExt::TileDataCount)
                                valid = false;
                        }
                    }
                    if (valid)
                    {
                        tileSets.insert(tileSet);
                        int index = 0;
                        auto& ret = CMapDataExt::CustomTiles[tileSet].emplace_back();
                        ret.Initialize(end.Y - start.Y, end.X - start.X);
                        for (int x = start.X; x < end.X; ++x)
                        {
                            for (int y = start.Y; y < end.Y; ++y)
                            {
                                auto& info = infos[{x, y}];
                                if (ignoreCoords.find({ x,y }) != ignoreCoords.end())
                                {
                                    index++;
                                    continue;
                                }
                                ret.TileBlockDatas[index].SetTileBlock(info.TileIndex, info.TileSubIndex, info.Height);
                                index++;
                            }
                        }
                    }
                }
            }
        }

        for (auto set : tileSets)
        {
            FString text;
            FString name;
            if (Translations::CustomTileSetNames.find(set) != Translations::CustomTileSetNames.end())
                name = Translations::CustomTileSetNames[set];
            else
                name = "No Name";
            text.Format("%d (%s)", set, name);
            SendMessage(hTileComboBox, CB_INSERTSTRING, count, text);
            SendMessage(hTileComboBox, CB_SETITEMDATA, count, set);
            count++;
        }
    }

    return 0;
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
