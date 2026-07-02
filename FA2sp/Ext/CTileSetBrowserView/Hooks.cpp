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

static BYTE* ScaleBGRBuffer(
    const BYTE* src, int srcW, int srcH, LONG srcPitch,
    float scaleFactor, int& outW, int& outH, LONG& outPitch)
{
    if (fabs(1.0f - scaleFactor) < 0.001f)
    {
        outW = srcW; outH = srcH; outPitch = srcPitch;
        return nullptr;
    }

    outW = std::max(1, (int)std::lround(srcW * scaleFactor));
    outH = std::max(1, (int)std::lround(srcH * scaleFactor));

    outPitch = outW * 3;
    if (outPitch % sizeof(DWORD))
        outPitch += sizeof(DWORD) - (outW * 3) % sizeof(DWORD);

    BYTE* dst = new BYTE[outPitch * outH];

    float xScale = (float)srcW / outW;
    float yScale = (float)srcH / outH;

    for (int dy = 0; dy < outH; ++dy)
    {
        float sy = (dy + 0.5f) * yScale - 0.5f;
        if (sy < 0) sy = 0;
        if (sy >= srcH - 1) sy = srcH - 1.001f;
        int y0 = (int)sy;
        int y1 = std::min(y0 + 1, srcH - 1);
        float wy = sy - y0;

        const BYTE* srcRow0 = src + y0 * srcPitch;
        const BYTE* srcRow1 = src + y1 * srcPitch;
        BYTE* dstRow = dst + dy * outPitch;

        for (int dx = 0; dx < outW; ++dx)
        {
            float sx = (dx + 0.5f) * xScale - 0.5f;
            if (sx < 0) sx = 0;
            if (sx >= srcW - 1) sx = srcW - 1.001f;
            int x0 = (int)sx;
            int x1 = std::min(x0 + 1, srcW - 1);
            float wx = sx - x0;

            const BYTE* p00 = srcRow0 + x0 * 3;
            const BYTE* p10 = srcRow0 + x1 * 3;
            const BYTE* p01 = srcRow1 + x0 * 3;
            const BYTE* p11 = srcRow1 + x1 * 3;

            for (int c = 0; c < 3; ++c)
            {
                float v00 = p00[c], v10 = p10[c], v01 = p01[c], v11 = p11[c];
                float vTop = v00 * (1.0f - wx) + v10 * wx;
                float vBottom = v01 * (1.0f - wx) + v11 * wx;
                dstRow[dx * 3 + c] = (BYTE)(vTop * (1.0f - wy) + vBottom * wy + 0.5f);
            }
        }
    }

    return dst;
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

static bool HasValidImage(const CTileTypeClass* tileData)
{
	for (int i = 0; i < tileData->TileBlockCount; ++i)
    {
		auto& block = tileData->TileBlockDatas[i];
        if (block.ImageData)
            return true;
	}
	return false;
}

static bool HasValidImage(const CustomTile* tileData)
{
	for (int i = 0; i < tileData->Width * tileData->Height; ++i)
    {
		auto& block = tileData->TileBlockDatas[i];
		auto tileBlock = block.GetTileBlock();
		if (tileBlock && tileBlock->ImageData)
            return true;
	}
	return false;
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
        iTileIndex = CMapDataExt::GetSafeTileIndex(iTileIndex);
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
        ddsd.dwHeight = std::max(tile.Bounds.bottom - tile.Bounds.top, 30l) + added_height;
        ddsd.dwWidth = std::max(tile.Bounds.right - tile.Bounds.left, 60l) + added_width;
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

                auto& tiledata = CMapDataExt::TileData[tile.GetDisplayTileIndex()];
                int randomIndex = STDHelpers::RandomSelectInt(-1, tiledata.AltTypeCount);
                if (tiledata.AltTypeCount > 0 && randomIndex > -1)
                {
                    bool isBridge = (tiledata.TileSet == CMapDataExt::BridgeSet
                        || tiledata.TileSet == CMapDataExt::WoodBridgeSet);
                    auto& altType = tiledata.AltTypes[randomIndex];
                    if (!isBridge && tile.SubTileIndex < altType.TileBlockCount)
                    {
                        block = &altType.TileBlockDatas[tile.SubTileIndex];
                    }
                }

                if (block && block->ImageData)
                {
                    int drawx = e * 60 / 2 - i * 60 / 2
                        + 30
                        + block->XMinusExX;
                    int drawy = e * 30 / 2 + i * 30 / 2
                        + block->YMinusExY
                        - tile.GetHeight() * 30 / 2;

                    auto pPal = CMapDataExt::TileSetPalettes
                        [tiledata.TileSet];
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

struct CompactTileInfo
{
    int tileIndex;
    int surfIndex;
    int x, y;
    int width, height;
    int rawWidth, rawHeight;
};
static std::vector<CompactTileInfo> s_CompactLayout;
static int s_CompactTotalHeight = 0;
static int s_CompactLayoutClientWidth = 0;

struct CompactOverlayInfo
{
    int overlayIndex;
    int overlayData;
    int x, y;
    int width, height;
};
static std::vector<CompactOverlayInfo> s_CompactOverlayLayout;
static int s_CompactOverlayTotalHeight = 0;
static int s_CompactOverlayLayoutClientWidth = 0;
static int s_CompactOverlayLayoutOverlayIndex = -1;
static float s_CompactOverlayLayoutScaleFactor = 0.0f;

static void GetTileScaledSize(CTileSetBrowserView* pThis, int tileIndex, int& w, int& h)
{
    if (pThis->CurrentTileset < 10000)
    {
        auto& tile = CMapDataExt::TileData[tileIndex];
        w = GetAddedWidth(tileIndex)
            + std::max(tile.Bounds.right - tile.Bounds.left, 60l);
        h = GetAddedHeight(tileIndex)
            + std::max(tile.Bounds.bottom - tile.Bounds.top, 30l);
    }
    else
    {
        auto tileData = CMapDataExt::GetCustomTile(tileIndex);
        GetCustomTileSize(tileData, w, h);
    }
    w = (int)(w * CTileSetBrowserFrameExt::TileSetBrowserViewScaledFactor);
    h = (int)(h * CTileSetBrowserFrameExt::TileSetBrowserViewScaledFactor);
}

static void BuildCompactLayout(CTileSetBrowserView* pThis)
{
    s_CompactLayout.clear();
    s_CompactTotalHeight = 0;

    RECT rc;
    pThis->GetClientRect(&rc);
    s_CompactLayoutClientWidth = rc.right;

    int clientWidth = rc.right;
    const int spacing = 4;

    int curX = 0, curY = 2, rowHeight = 0;
    std::vector<size_t> rowIndices;

    int tileCount = pThis->TileSurfacesCount;
    DWORD dwTileSet = pThis->CurrentTileset;

    for (int i = 0; i < tileCount; i++)
    {
        int realTileIdx = (dwTileSet < 10000)
            ? (CMapDataExt::TileSet_starts[dwTileSet] + i)
            : CMapDataExt::GetCustomTileIndex(dwTileSet, i);

        if (!pThis->TileSurfaces[i])
            continue;

        if (dwTileSet < 10000)
        {
            auto& tile = CMapDataExt::TileData[realTileIdx];
            if (!HasValidImage(&tile))
                continue;
        }
        else
        {
            auto tileData = CMapDataExt::GetCustomTile(realTileIdx);
            if (!HasValidImage(tileData))
                continue;
        }

        int w, h;
        GetTileScaledSize(pThis, realTileIdx, w, h);
        int rawW = w, rawH = h;
        w += 6; // 3px margin each side
        h += 6;
        w = std::max(w, 40);
        h = std::max(h, 40);

        if (curX > 0 && curX + w > clientWidth)
        {
            for (auto& ri : rowIndices)
                s_CompactLayout[ri].y = curY + (rowHeight - s_CompactLayout[ri].height) / 2;
            curY += rowHeight + spacing;
            curX = 0;
            rowHeight = 0;
            rowIndices.clear();
        }

        s_CompactLayout.push_back({realTileIdx, i, curX + 3, 0, w, h, rawW, rawH});
        rowIndices.push_back(s_CompactLayout.size() - 1);
        curX += w + spacing;
        rowHeight = std::max(rowHeight, h);
    }

    // Commit last row
    for (auto& ri : rowIndices)
        s_CompactLayout[ri].y = curY + (rowHeight - s_CompactLayout[ri].height) / 2;
    curY += rowHeight;

    s_CompactTotalHeight = curY;
}

static void BuildCompactOverlayLayout(CTileSetBrowserView* pThis)
{
    s_CompactOverlayLayout.clear();
    s_CompactOverlayTotalHeight = 0;

    RECT rc;
    pThis->GetClientRect(&rc);
    s_CompactOverlayLayoutClientWidth = rc.right;

    int clientWidth = rc.right;
    const int spacing = 4;

    int overlayIndex = pThis->SelectedOverlayIndex;

    FString ovlIdx;
    ovlIdx.Format("%d", overlayIndex);
    int nDisplayLimit = Variables::RulesMap.GetInteger(
        Variables::RulesMap.GetValueAt("OverlayTypes", overlayIndex),
        "OverlayDisplayLimit", ExtConfigs::OverlayDataLimit);
    nDisplayLimit = CINI::FAData->GetInteger("OverlayDisplayLimit", ovlIdx, nDisplayLimit);
    if (nDisplayLimit > ExtConfigs::OverlayDataLimit)
        nDisplayLimit = ExtConfigs::OverlayDataLimit;

    float scaleFactor = CTileSetBrowserFrameExt::OverlayBrowserViewScaledFactor;

    int curX = 0, curY = 2, rowHeight = 0;
    std::vector<size_t> rowIndices;

    for (int i = 0; i < nDisplayLimit; i++)
    {
        auto imageName = CLoadingExt::GetOverlayName(overlayIndex, i);
        auto pData = CLoadingExt::GetImageDataFromMap(imageName);
        if (!ImageDataClassSafe::IsValidImage(pData))
            continue;

        int w = std::max(1, (int)std::lround(pData->FullWidth * scaleFactor));
        int h = std::max(1, (int)std::lround(pData->FullHeight * scaleFactor));
        w += 6;
        h += 6;
        w = std::max(w, 40);
        h = std::max(h, 40);

        if (curX > 0 && curX + w > clientWidth)
        {
            for (auto& ri : rowIndices)
                s_CompactOverlayLayout[ri].y = curY + (rowHeight - s_CompactOverlayLayout[ri].height) / 2;
            curY += rowHeight + spacing;
            curX = 0;
            rowHeight = 0;
            rowIndices.clear();
        }

        s_CompactOverlayLayout.push_back({overlayIndex, i, curX + 3, 0, w, h});
        rowIndices.push_back(s_CompactOverlayLayout.size() - 1);
        curX += w + spacing;
        rowHeight = std::max(rowHeight, h);
    }

    // Commit last row
    for (auto& ri : rowIndices)
        s_CompactOverlayLayout[ri].y = curY + (rowHeight - s_CompactOverlayLayout[ri].height) / 2;
    curY += rowHeight;

    s_CompactOverlayTotalHeight = curY;
    s_CompactOverlayLayoutOverlayIndex = overlayIndex;
    s_CompactOverlayLayoutScaleFactor = scaleFactor;
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

    if (pThis->CurrentImageWidth <= 0) pThis->CurrentImageWidth = 1;
    int max_r = r.right / pThis->CurrentImageWidth;
    if (max_r == 0) max_r = 1;

    int cur_y = 3;
    int cur_x = 3;

    int tile_width = pThis->CurrentImageWidth;
    int tile_height = pThis->CurrentImageHeight;

    if (pThis->CurrentMode == 1)
    {
        if (ExtConfigs::TileSetBrowserViewCompactArrange && !s_CompactLayout.empty())
        {
            for (const auto& info : s_CompactLayout)
            {
                if (point.x > info.x && point.y > info.y
                    && point.x < info.x + info.width && point.y < info.y + info.height)
                {
                    int oldmode = CIsoView::CurrentCommand->Command;
                    int oldid = CIsoView::CurrentCommand->Type;
                    int oldset = 0;

                    CIsoView::CurrentCommand->Command = 10;
                    CIsoView::CurrentCommand->Type = info.tileIndex;
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
            }
        }
        else
        {
            int iTileStart;
            if (pThis->CurrentTileset < 10000)
                iTileStart = CMapDataExt::TileSet_starts[pThis->CurrentTileset];
            else
                iTileStart = CMapDataExt::GetCustomTileIndex(pThis->CurrentTileset, 0);

            int displayIndex = 0;
            for (int i = 0; i < pThis->TileSurfacesCount; i++)
            {
                if (!pThis->TileSurfaces[i])
                {
                    iTileStart++;
                    continue;
                }

                if (pThis->CurrentTileset < 10000)
                {
                    auto& tile = CMapDataExt::TileData[iTileStart];
                    if (!HasValidImage(&tile))
                    {
                        iTileStart++;
                        continue;
                    }
                }
                else
                {
                    auto tileData = CMapDataExt::GetCustomTile(iTileStart);
                    if (!HasValidImage(tileData))
                    {
                        iTileStart++;
                        continue;
                    }
                }

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
                if (max_r == 0) max_r = 1;
                if (displayIndex % max_r == max_r - 1)
                {
                    cur_y += tile_height;
                    cur_x = 3;
                }
                iTileStart++;
                displayIndex++;
            }
        } 
    }
    else if (pThis->CurrentMode == 2)
    {
        if (ExtConfigs::TileSetBrowserViewCompactArrange && !s_CompactOverlayLayout.empty())
        {
            for (const auto& info : s_CompactOverlayLayout)
            {
                if (point.x > info.x && point.y > info.y
                    && point.x < info.x + info.width && point.y < info.y + info.height)
                {
                    CIsoView::CurrentCommand->Command = 1;
                    CIsoView::CurrentCommand->Type = 6;
                    CIsoView::CurrentCommand->Param = 33;
                    CIsoView::CurrentCommand->Overlay = info.overlayIndex;
                    CIsoView::CurrentCommand->OverlayData = info.overlayData;
                    CIsoView::CurrentCommand->Height = 0;

                    pThis->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
                    return 0x4F3EDE;
                }
            }
        }
        else
        {
            FString ovlIdx;
            ovlIdx.Format("%d", pThis->SelectedOverlayIndex);
            int nDisplayLimit = Variables::RulesMap.GetInteger(
                Variables::RulesMap.GetValueAt("OverlayTypes", pThis->SelectedOverlayIndex),
                "OverlayDisplayLimit", ExtConfigs::OverlayDataLimit);
            nDisplayLimit = CINI::FAData->GetInteger("OverlayDisplayLimit", ovlIdx, nDisplayLimit);
            for (int i = 0; i < std::min(nDisplayLimit, ExtConfigs::OverlayDataLimit); i++)
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
                        cur_x = 3;
                    }
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

    CTileSetBrowserFrameExt::TileSetBrowserView_Instance = pThis;
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

		if (HasValidImage(&tileData))
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
                    + std::max(CMapDataExt::TileData[tileIndex].Bounds.right
                    - CMapDataExt::TileData[tileIndex].Bounds.left, 60l);
                int height = GetAddedHeight(tileIndex)
                    + std::max(CMapDataExt::TileData[tileIndex].Bounds.bottom
                    - CMapDataExt::TileData[tileIndex].Bounds.top, 30l);

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
            CIsoViewExt::ScaleSurface(&pThis->TileSurfaces[i], CTileSetBrowserFrameExt::TileSetBrowserViewScaledFactor);
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
            CIsoViewExt::ScaleSurface(&pThis->TileSurfaces[i], CTileSetBrowserFrameExt::TileSetBrowserViewScaledFactor);
        }
    }
   
    pThis->CurrentImageWidth *= CTileSetBrowserFrameExt::TileSetBrowserViewScaledFactor;
    pThis->CurrentImageHeight *= CTileSetBrowserFrameExt::TileSetBrowserViewScaledFactor;
    if (pThis->CurrentImageWidth <= 0) pThis->CurrentImageWidth = 1;

    RECT r;
    pThis->GetClientRect(&r);
    int max_r = r.right / pThis->CurrentImageWidth;
    if (max_r <= 0) max_r = 1;

    if (ExtConfigs::TileSetBrowserViewCompactArrange)
    {
        BuildCompactLayout(pThis);
        pThis->ScrollWidth = s_CompactTotalHeight;
    }
    else
    {
        s_CompactLayout.clear();
        s_CompactTotalHeight = 0;
        s_CompactLayoutClientWidth = 0;
        pThis->ScrollWidth = pThis->CurrentImageHeight * (1 + tileCount / max_r);
    }
    pThis->GetParentFrame()->RecalcLayout(TRUE);
    pThis->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
    ::SetForegroundWindow(pIsoView->GetSafeHwnd());
    ::SetFocus(pIsoView->GetSafeHwnd());

    return 0x4F368F;
}

DEFINE_HOOK(4F1D70, CTileSetBrowserView_OnDraw, 6)
{
    GET(CTileSetBrowserView*, pThis, ECX);
    GET_STACK(CDC*, pDC, 0x4);

    if (pThis->CurrentImageWidth <= 0) return 0x4F25B0;

    CTileSetBrowserFrameExt::TileSetBrowserView_Instance = pThis;
    RECT r;
    pThis->GetClientRect(&r);
    int max_r = r.right / pThis->CurrentImageWidth;
    int cur_y = 3;
    int cur_x = 3;

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

        if (ExtConfigs::TileSetBrowserViewCompactArrange && !s_CompactLayout.empty())
        {
            RECT rcNow;
            pThis->GetClientRect(&rcNow);
            if (rcNow.right != s_CompactLayoutClientWidth)
            {
                BuildCompactLayout(pThis);
                pThis->ScrollWidth = s_CompactTotalHeight;
                pThis->GetParentFrame()->RecalcLayout(TRUE);
            }

            int scrollY = pThis->GetScrollPos(SB_VERT);
            for (const auto& info : s_CompactLayout)
            {
                if (info.y + info.height < scrollY)
                    continue;
                if (info.y > scrollY + r.bottom)
                    continue;

                int surfW = info.rawWidth;
                int surfH = info.rawHeight;
                int drawX = info.x + (info.width - info.rawWidth) / 2;
                int drawY = info.y + (info.height - info.rawHeight) / 2;

                HDC hDC = NULL;
                pThis->TileSurfaces[info.surfIndex]->GetDC(&hDC);

                HDC hTmpDC = CreateCompatibleDC(hDC);
                HBITMAP hBitmap = CreateCompatibleBitmap(hDC, surfW, surfH);
                SelectObject(hTmpDC, hBitmap);

                BitBlt(hTmpDC, 0, 0, surfW, surfH, hDC, 0, 0, SRCCOPY);

                pThis->TileSurfaces[info.surfIndex]->ReleaseDC(hDC);

                BitBlt(pDC->GetSafeHdc(),
                    drawX, drawY,
                    surfW, surfH, hTmpDC, 0, 0, SRCCOPY);

                DeleteDC(hTmpDC);
                DeleteObject(hBitmap);

                if (CIsoView::CurrentCommand->Command == 10
                    && CIsoView::CurrentCommand->Type == info.tileIndex)
                {
                    CPen p;
                    CBrush b;
                    p.CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
                    b.CreateStockObject(NULL_BRUSH);

                    CPen* old = pDC->SelectObject(&p);

                    pDC->SetBkMode(TRANSPARENT);
                    pDC->SelectObject(&b);
                    pDC->Rectangle(info.x - 2, info.y - 2,
                        info.x + info.width + 2,
                        info.y + info.height + 2);

                        if (CTileSetBrowserFrameExt::TileSetBrowserViewScaledFactor >= 1.5f)
                        {
                            pDC->Rectangle(info.x - 3, info.y - 3,
                                info.x + info.width + 3,
                                info.y + info.height + 3);
                        }

                    pDC->SelectObject(old);
                }
            }
            return 0x4F25B0;
        }

        int tileIndex;
        if (pThis->CurrentTileset < 10000)
            tileIndex = CMapDataExt::TileSet_starts[pThis->CurrentTileset];
        else
            tileIndex = CMapDataExt::GetCustomTileIndex(pThis->CurrentTileset, 0);

		int displayIndex = 0;
		for (int i = 0; i < pThis->TileSurfacesCount; i++)
        {
            if (!pThis->TileSurfaces[i])
            {
				tileIndex++;
                continue;
            }

            int curwidth, curheight;
            if (pThis->CurrentTileset < 10000)
            {
                auto& tile = CMapDataExt::TileData[tileIndex];
                if (!HasValidImage(&tile))
                {
                    tileIndex++;
                    continue;
                }
				curwidth = GetAddedWidth(tileIndex) +
						    std::max(tile.Bounds.right 
                            - tile.Bounds.left, 60l);
				curheight = GetAddedHeight(tileIndex) + 
                            std::max(tile.Bounds.bottom 
                            - tile.Bounds.top, 30l);

			}
            else
            {
                auto tileData = CMapDataExt::GetCustomTile(tileIndex);
                if (!HasValidImage(tileData))
                {
                    tileIndex++;
                    continue;
                }
                GetCustomTileSize(tileData, curwidth, curheight);
            }

            curwidth *= CTileSetBrowserFrameExt::TileSetBrowserViewScaledFactor;
            curheight *= CTileSetBrowserFrameExt::TileSetBrowserViewScaledFactor;
            
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

                if (CIsoView::CurrentCommand->Command == 10 
                    && CIsoView::CurrentCommand->Type == tileIndex)
                {
                    CPen p;
                    CBrush b;
                    p.CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
                    b.CreateStockObject(NULL_BRUSH);

                    CPen* old = pDC->SelectObject(&p);

                    pDC->SetBkMode(TRANSPARENT);
                    pDC->SelectObject(&b);
                    pDC->Rectangle(cur_x - 2, cur_y - 2,
                                    cur_x + pThis->CurrentImageWidth + 2,
                                    cur_y + pThis->CurrentImageHeight + 2);

                    if (CTileSetBrowserFrameExt::TileSetBrowserViewScaledFactor >= 1.5f)
                    {
                        pDC->Rectangle(cur_x - 3, cur_y - 3,
                            cur_x + pThis->CurrentImageWidth + 3,
                            cur_y + pThis->CurrentImageHeight + 3);
                    }

                    pDC->SelectObject(old);
				}
			}

            cur_x += pThis->CurrentImageWidth;
            if (max_r == 0) max_r = 1;
            if (displayIndex % max_r == max_r - 1)
            {
                cur_y += pThis->CurrentImageHeight;
                cur_x = 3;
            }
            tileIndex++;
            displayIndex++;
        }
        return 0x4F25B0;
    }
    else if (pThis->CurrentMode == 2)
    {       
        FString ovlIdx;
        ovlIdx.Format("%d", pThis->SelectedOverlayIndex);
        int nDisplayLimit = Variables::RulesMap.GetInteger(
            Variables::RulesMap.GetValueAt("OverlayTypes", pThis->SelectedOverlayIndex),
            "OverlayDisplayLimit", ExtConfigs::OverlayDataLimit);
        nDisplayLimit = CINI::FAData->GetInteger("OverlayDisplayLimit", ovlIdx, nDisplayLimit);
        if (nDisplayLimit > ExtConfigs::OverlayDataLimit)
            nDisplayLimit = ExtConfigs::OverlayDataLimit;

        if (ExtConfigs::TileSetBrowserViewCompactArrange)
        {
            RECT rcNow;
            pThis->GetClientRect(&rcNow);
            if (s_CompactOverlayLayout.empty() || rcNow.right != s_CompactOverlayLayoutClientWidth
                || pThis->SelectedOverlayIndex != s_CompactOverlayLayoutOverlayIndex
                || fabsf(CTileSetBrowserFrameExt::OverlayBrowserViewScaledFactor - s_CompactOverlayLayoutScaleFactor) > 0.001f)
            {
                BuildCompactOverlayLayout(pThis);
                pThis->ScrollWidth = s_CompactOverlayTotalHeight;
                pThis->GetParentFrame()->RecalcLayout(TRUE);
            }

            int scrollY = pThis->GetScrollPos(SB_VERT);
            float scaleFactor = CTileSetBrowserFrameExt::OverlayBrowserViewScaledFactor;

            for (const auto& info : s_CompactOverlayLayout)
            {
                if (info.y + info.height < scrollY)
                    continue;
                if (info.y > scrollY + r.bottom)
                    continue;

                auto imageName = CLoadingExt::GetOverlayName(info.overlayIndex, info.overlayData);
                auto pData = CLoadingExt::GetImageDataFromMap(imageName);
                if (!ImageDataClassSafe::IsValidImage(pData))
                    continue;

                int curwidth = pData->FullWidth;
                int curheight = pData->FullHeight;

                BITMAPINFO biinfo;
                memset(&biinfo, 0, sizeof(BITMAPINFO));
                biinfo.bmiHeader.biBitCount = 24;
                biinfo.bmiHeader.biWidth = curwidth;
                biinfo.bmiHeader.biHeight = curheight;
                biinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                biinfo.bmiHeader.biClrUsed = 0;
                biinfo.bmiHeader.biPlanes = 1;
                biinfo.bmiHeader.biCompression = BI_RGB;
                biinfo.bmiHeader.biClrImportant = 0;

                int pitch = curwidth * 3;
                if (pitch == 0)
                    continue;
                if (pitch % sizeof(DWORD))
                    pitch += sizeof(DWORD) - (curwidth * 3) % sizeof(DWORD);

                BYTE* colors = new(BYTE[pitch * curheight]);
                memset(colors, ExtConfigs::EnableDarkMode ? 32 : 255, pitch * curheight);

                BGRStruct empty;
                auto pPalette = PalettesManager::GetTileSetBrowserViewPalette(
                    pData->pPalette, empty, false, {}, CMapDataExt::IsOre(pThis->SelectedOverlayIndex));

                if (pData->pPixelValidRanges)
                {
                    for (int k = 0; k < curheight; k++)
                    {
                        short first = pData->pPixelValidRanges[k].First;
                        short last = pData->pPixelValidRanges[k].Last;
                        if (first >= last)
                            continue;
                        BYTE* rowDst = colors + (curheight - k - 1) * pitch;
                        const unsigned char* rowSrc = pData->pImageBuffer.get() + k * curwidth;
                        for (int l = first; l < last; l++)
                        {
                            auto src = rowSrc[l];
                            if (src)
                                memcpy(&rowDst[l * 3], &(*pPalette)[src], 3);
                        }
                    }
                }
                else
                {
                    for (int k = 0; k < curheight; k++)
                    {
                        for (int l = 0; l < curwidth; l++)
                        {
                            auto src = pData->pImageBuffer[l + k * curwidth];
                            if (src)
                                memcpy(&colors[l * 3 + (curheight - k - 1) * pitch], &(*pPalette)[src], 3);
                        }
                    }
                }

                int scaledW, scaledH;
                LONG scaledPitch;
                BYTE* scaledColors = ScaleBGRBuffer(colors, curwidth, curheight, pitch,
                    scaleFactor, scaledW, scaledH, scaledPitch);
                if (scaledColors)
                {
                    delete[] colors;
                    colors = scaledColors;
                    curwidth = scaledW;
                    curheight = scaledH;
                    pitch = scaledPitch;
                    biinfo.bmiHeader.biWidth = curwidth;
                    biinfo.bmiHeader.biHeight = curheight;
                }

                int drawX = info.x + (info.width - curwidth) / 2;
                int drawY = info.y + (info.height - curheight) / 2;

                StretchDIBits(pDC->GetSafeHdc(),
                    drawX, drawY, curwidth, curheight,
                    0, 0, curwidth, curheight, colors, &biinfo, DIB_RGB_COLORS, SRCCOPY);

                delete[] colors;

                if (CIsoView::CurrentCommand->Command == 1
                    && CIsoView::CurrentCommand->Overlay == info.overlayIndex
                    && CIsoView::CurrentCommand->OverlayData == info.overlayData
                    && CIsoView::CurrentCommand->Param == 33
                    && CIsoView::CurrentCommand->Type == 6)
                {
                    CPen p;
                    CBrush b;
                    p.CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
                    b.CreateStockObject(NULL_BRUSH);
                    CPen* old = pDC->SelectObject(&p);
                    pDC->SetBkMode(TRANSPARENT);
                    pDC->SelectObject(&b);
                    pDC->Rectangle(info.x - 2, info.y - 2,
                        info.x + info.width + 2, info.y + info.height + 2);

                    if (CTileSetBrowserFrameExt::OverlayBrowserViewScaledFactor >= 1.5f)
                    {
                        pDC->Rectangle(info.x - 3, info.y - 3,
                            info.x + info.width + 3, info.y + info.height + 3);
                    }
                    pDC->SelectObject(old);
                }
            }
            return 0x4F25B0;
        }

        float scaleFactor = CTileSetBrowserFrameExt::OverlayBrowserViewScaledFactor;
		for (int i = 0;i < nDisplayLimit; ++i)
		{
            auto imageName = CLoadingExt::GetOverlayName(pThis->SelectedOverlayIndex, i);
            auto pData = CLoadingExt::GetImageDataFromMap(imageName);
			if (ImageDataClassSafe::IsValidImage(pData))
			{
				int curwidth = pData->FullWidth;
				int curheight = pData->FullHeight;

                int testW = std::max(1, (int)std::lround(curwidth * scaleFactor));
                int testH = std::max(1, (int)std::lround(curheight * scaleFactor));
            
                if (cur_y + testH + (pThis->CurrentImageHeight - testH) / 2
                    >= pThis->GetScrollPos(SB_VERT) && cur_y <= pThis->GetScrollPos(SB_VERT) + r.bottom)
                {
                    BITMAPINFO biinfo;
                    memset(&biinfo, 0, sizeof(BITMAPINFO));
                    biinfo.bmiHeader.biBitCount = 24;
                    biinfo.bmiHeader.biWidth = curwidth;
                    biinfo.bmiHeader.biHeight = curheight;
                    biinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    biinfo.bmiHeader.biClrUsed = 0;
                    biinfo.bmiHeader.biPlanes = 1;
                    biinfo.bmiHeader.biCompression = BI_RGB;
                    biinfo.bmiHeader.biClrImportant = 0;

                    int pitch = curwidth * 3;
                    if (pitch == 0)
                        goto advance_cursor;

                    if (pitch % sizeof(DWORD))
                    {
                        pitch += sizeof(DWORD) - (curwidth * 3) % sizeof(DWORD);
                    }

                    BYTE* colors = new(BYTE[pitch * curheight]);
                    memset(colors, ExtConfigs::EnableDarkMode ? 32 : 255, pitch * (curheight));

                    BGRStruct empty;
                    auto pPalette = PalettesManager::GetTileSetBrowserViewPalette(
                        pData->pPalette, empty, false, {}, CMapDataExt::IsOre(pThis->SelectedOverlayIndex));

                    if (pData->pPixelValidRanges)
                    {
                        int k;
                        for (k = 0; k < curheight; k++)
                        {
                            short first = pData->pPixelValidRanges[k].First;
                            short last = pData->pPixelValidRanges[k].Last;
                            if (first >= last)
                                continue; 

                            BYTE* rowDst = colors + (curheight - k - 1) * pitch;
                            const unsigned char* rowSrc = pData->pImageBuffer.get() + k * curwidth;

                            for (int l = first; l < last; l++)
                            {
                                auto src = rowSrc[l];
                                if (src)
                                {
                                    memcpy(&rowDst[l * 3], &(*pPalette)[src], 3);
                                }
                            }
                        }
                    }
                    else
                    {
                        int k, l;
                        for (k = 0; k < curheight; k++)
                        {
                            for (l = 0; l < curwidth; l++)
                            {
                                auto src = pData->pImageBuffer[l + k * curwidth];
                                if (src)
                                {
                                    memcpy(&colors[l * 3 + (curheight - k - 1) * pitch], &(*pPalette)[src], 3);
                                }
                            }
                        }
                    }

                    int scaledW, scaledH;
                    LONG scaledPitch;
                    BYTE* scaledColors = ScaleBGRBuffer(colors, curwidth, curheight, pitch,
                        scaleFactor, scaledW, scaledH, scaledPitch);

                    if (scaledColors)
                    {
                        delete[] colors;
                        colors = scaledColors;
                        curwidth = scaledW;
                        curheight = scaledH;
                        pitch = scaledPitch;

                        biinfo.bmiHeader.biWidth = curwidth;
                        biinfo.bmiHeader.biHeight = curheight;
                    }

                    StretchDIBits(pDC->GetSafeHdc(), 
                        cur_x + (pThis->CurrentImageWidth - curwidth) / 2,
                        cur_y + (pThis->CurrentImageHeight - curheight) / 2, 
                        curwidth, curheight,
                        0, 0, curwidth, curheight, colors, &biinfo, DIB_RGB_COLORS, SRCCOPY);

                    delete[] colors;

                    if (CIsoView::CurrentCommand->Command == 1 
                        && CIsoView::CurrentCommand->Overlay == pThis->SelectedOverlayIndex
                        && CIsoView::CurrentCommand->OverlayData == i 
                        && CIsoView::CurrentCommand->Param == 33 
                        && CIsoView::CurrentCommand->Type == 6)
                    {
                        CPen p;
                        CBrush b;
                        p.CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
                        b.CreateStockObject(NULL_BRUSH);

                        CPen* old = pDC->SelectObject(&p);

                        pDC->SetBkMode(TRANSPARENT);
                        pDC->SelectObject(&b);
                        pDC->Rectangle(cur_x - 2, cur_y - 2, 
                            cur_x + pThis->CurrentImageWidth + 2,
                             cur_y + pThis->CurrentImageHeight + 2);

                        if (CTileSetBrowserFrameExt::OverlayBrowserViewScaledFactor >= 1.5f)
                        {
                            pDC->Rectangle(cur_x - 3, cur_y - 3, 
                                cur_x + pThis->CurrentImageWidth + 3,
                                 cur_y + pThis->CurrentImageHeight + 3);
                        }

                        pDC->SelectObject(old);
                    }
                }

                advance_cursor:
				cur_x += pThis->CurrentImageWidth;
				if (max_r == 0)
					max_r = 1;
				if (i % max_r == max_r - 1)
				{
					cur_y += pThis->CurrentImageHeight;
					cur_x = 3;
				}
			}
		}

        return 0x4F25B0;
    }
    return 0x4F25B0;
}

DEFINE_HOOK(4F4650, CTileSetBrowserView_GetAddedHeight, 9)
{
    GET_STACK(int, iTileIndex, 0x4);

    R->EAX(GetAddedHeight(iTileIndex));

    return 0x4F4734;
}

DEFINE_HOOK(4F0B20, CTileSetBrowserView_Update_Init, 7)
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

DEFINE_HOOK(4F128A, CTileSetBrowserView_Update_AddCustomTiles, 5)
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
                    else if (ParsePrefixedInt(kvp.first, (FinalAlertConfig::Language + "-"), name))
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

DEFINE_HOOK(4F4774, CTileSetBrowserView_SetOverlay_LoadOverlayImage, 5)
{
    GET(CTileSetBrowserView*, pThis, ESI);
    GET(int, Overlay, EBX);

    FString ovlIdx;
    ovlIdx.Format("%d", pThis->SelectedOverlayIndex);
    int max_ovrl_img = Variables::RulesMap.GetInteger(
        Variables::RulesMap.GetValueAt("OverlayTypes", pThis->SelectedOverlayIndex),
        "OverlayDisplayLimit", ExtConfigs::OverlayDataLimit);
    max_ovrl_img = CINI::FAData->GetInteger("OverlayDisplayLimit", ovlIdx, max_ovrl_img);
    if (max_ovrl_img > ExtConfigs::OverlayDataLimit)
        max_ovrl_img = ExtConfigs::OverlayDataLimit;

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
            if (ImageDataClassSafe::IsValidImage(pData))
            {
                need_pos = i;
                iovrlcount++;
                need_width = std::max(need_width, (int)(pData->FullWidth * CTileSetBrowserFrameExt::OverlayBrowserViewScaledFactor));
                need_height = std::max(need_height, (int)(pData->FullHeight * CTileSetBrowserFrameExt::OverlayBrowserViewScaledFactor));
            }
        }
    }
    need_width += 6;
    need_height += 6;

    R->ECX(need_pos);
    R->EDI(need_width);
    R->EBP(need_height);
    R->Stack(STACK_OFFS(0x90, 0x80), iovrlcount);

    return 0x4F48D0;
}

DEFINE_HOOK(4F12C0, CTileSetBrowserView_Update_LoadOverlay, 5)
{
    CViewObjectsExt::Redraw_Initialize();

    HWND hParent = CFinalSunDlg::Instance->MyViewFrame.pTileSetBrowserFrame->DialogBar.GetSafeHwnd();
    HWND hOverlayComboBox = GetDlgItem(hParent, 1367);

    const int max_ovrl_img = ExtConfigs::OverlayDataLimit;
    auto&& section = Variables::RulesMap.ParseIndicies("OverlayTypes", true);
    for (int i = 0, sz = (ExtConfigs::ExtOverlays || CMapDataExt::NewINIFormat >= 5) ?
        section.size() : std::min((UINT)255, section.size()); i < sz; ++i)
    {
        auto& id = section[i];
        if (!Variables::RulesMap.GetSection(id).empty())
        {
            bool forceDisplay = false;
            if (ExtConfigs::EnableVeinholeLogic && (i == 178 || i == 167 || i == 126))
                forceDisplay = true;
            if (!forceDisplay && CViewObjectsExt::IgnoreOverlaySet.find(id) != CViewObjectsExt::IgnoreOverlaySet.end())
                continue;

            FString text;
            FString display;
            FString name = Variables::RulesMap.GetString(id, "Name");
            if (name.IsEmpty() || !Translations::GetTranslationItem(name, text))
            {
                text = CViewObjectsExt::QueryUIName(id, true);
            }
            display.Format("%04d (%s)", i, text);

            int index = (int)::SendMessage(hOverlayComboBox, CB_ADDSTRING, 0, display);
            ::SendMessage(hOverlayComboBox, CB_SETITEMDATA, index, (LPARAM)i);
        }        
    }

    return 0x4F1793;
}
