#include "Body.h"

#include <Helpers/Macro.h>
#include <CPalette.h>
#include <FA2PP.h>
#include <CIsoView.h>

#include "../../FA2sp.h"
#include "../../Helpers/STDHelpers.h"
#include "../CMapData/Body.h"
#include "../CFinalSunApp/Body.h"

namespace
{
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
}

int CTileSetBrowserViewExt::GetAddedHeight(int tileIndex)
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

int CTileSetBrowserViewExt::GetAddedWidth(int tileIndex)
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

void CTileSetBrowserViewExt::GetCustomTileSize(const CustomTile* tileData, int& width, int& height)
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

LPDIRECTDRAWSURFACE7 CTileSetBrowserViewExt::RenderTile(int iTileIndex)
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
        int added_height = CTileSetBrowserViewExt::GetAddedHeight(iTileIndex);
        int added_width = CTileSetBrowserViewExt::GetAddedWidth(iTileIndex);
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
        CTileSetBrowserViewExt::GetCustomTileSize(tileData, width, height);

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
