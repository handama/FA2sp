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

DEFINE_HOOK(4F4650, CTileSetBrowserView_GetAddedHeight, 9)
{
    GET_STACK(int, iTileIndex, 0x4);
    int cur_added = 0;
    auto tile = CMapDataExt::TileData[iTileIndex];
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

    R->EAX(-cur_added);
    return 0x4F4734;
}

static int GetAddedWidth(int tileIndex)
{
    int cur_added = 0;
    auto tile = CMapDataExt::TileData[tileIndex];
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

static int RenderTileWidthOffset = 0;
DEFINE_HOOK(4F36EE, CTileSetBrowserView_RenderTile_GetWidth, 6)
{
    GET(int, iTileIndex, EBX);
    RenderTileWidthOffset = GetAddedWidth(iTileIndex);

    return 0;
}

DEFINE_HOOK(4F3764, CTileSetBrowserView_RenderTile_SetWidth_DDSD, 7)
{
    GET(int, iWidth, ECX);
    if (iWidth + RenderTileWidthOffset > iWidth)
    {
        R->ECX(RenderTileWidthOffset + iWidth);
    }
    else
    {
        RenderTileWidthOffset = 0;
    }

    return 0;
}

DEFINE_HOOK(4F384B, CTileSetBrowserView_RenderTile_SetWidth_DrawX, 7)
{
    GET(int, iWidth, ESI);
    R->ESI(iWidth + RenderTileWidthOffset);

    return 0;
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
    auto pData = CLoadingExt::GetImageDataFromServer(imageName);
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

int OnLButtonDown_OverlayData = 0;
DEFINE_HOOK(4F4590, CTileSetBrowserView_OnLButtonDown_LoadOverlayImage_1, 5)
{
    GET(CTileSetBrowserView*, pThis, EBP);

    OnLButtonDown_OverlayData = 0;
    auto imageName = CLoadingExt::GetOverlayName(pThis->SelectedOverlayIndex, OnLButtonDown_OverlayData);
    auto pData = CLoadingExt::GetImageDataFromServer(imageName);
    if (setCurrentOverlay(pData))
    {
        R->ECX(&CurrentOverlayPtr);
    }
    else
    {
        R->ECX(&NULLPTR);
    }
    return 0x4F4596;
}

DEFINE_HOOK(4F45F7, CTileSetBrowserView_OnLButtonDown_LoadOverlayImage_2, 5)
{
    GET(CTileSetBrowserView*, pThis, EBP);

    OnLButtonDown_OverlayData++;
    auto imageName = CLoadingExt::GetOverlayName(pThis->SelectedOverlayIndex, OnLButtonDown_OverlayData);
    auto pData = CLoadingExt::GetImageDataFromServer(imageName);
    if (setCurrentOverlay(pData))
    {
        R->ECX(&CurrentOverlayPtr);
    }
    else
    {
        R->ECX(&NULLPTR);
    }
    if (R->ESI() < 60)
        return 0x4F4596;
    return 0x4F3E9A;
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
            auto pData = CLoadingExt::GetImageDataFromServer(imageName);
            if (pData && pData->pImageBuffer)
            {
                iovrlcount++;
            }
        }
        for (int i = 0; i < max_ovrl_img; i++)
        {
            auto imageName = CLoadingExt::GetOverlayName(Overlay, i);
            auto pData = CLoadingExt::GetImageDataFromServer(imageName);
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

static bool DrawTranspInsideTilesChanged = false;
DEFINE_HOOK(4F36DD, CTileSetBrowserView_RenderTile_DrawTranspInsideTiles, 5)
{
    GET(unsigned int, tileIndex, EBX);
    tileIndex = CMapDataExt::GetSafeTileIndex(tileIndex);
    if (0 <= tileIndex && tileIndex < CMapDataExt::TileDataCount)
    {
        auto pPal = CMapDataExt::TileSetPalettes[CMapDataExt::TileData[tileIndex].TileSet];
        BGRStruct empty;
        DrawTranspInsideTilesChanged = true;
        memcpy(&CLoadingExt::TempISOPalette, Palette::PALETTE_ISO, sizeof(Palette));
        memcpy(Palette::PALETTE_ISO, PalettesManager::GetTileSetBrowserViewPalette(pPal, empty, false), sizeof(Palette));
    }
    return 0;
}

DEFINE_HOOK(4F35CE, CTileSetBrowserView_RenderTile_DrawTranspInsideTiles_2, 6)
{
    if (DrawTranspInsideTilesChanged)
    {
        DrawTranspInsideTilesChanged = false;
        memcpy(Palette::PALETTE_ISO, &CLoadingExt::TempISOPalette, sizeof(Palette));
    }
    return 0;
}

//DEFINE_HOOK(4F1D70, ASDG, 6)
//{
//	Logger::Debug("%s %s %s %s %s %s\n", 
//        reinterpret_cast<const char*>(0x5D3358),
//        reinterpret_cast<const char*>(0x5D3354),
//        reinterpret_cast<const char*>(0x5D3350),
//        reinterpret_cast<const char*>(0x5D334C),
//        reinterpret_cast<const char*>(0x5D3348),
//        reinterpret_cast<const char*>(0x5D3344)
//        );
//	return 0;
//}

//
//DEFINE_HOOK(4F23CD, CTileSetBrowserView_CreateBMPPreview_DebugAsBmp, 6)
//{
//	GET(BITMAPINFO*, pBmi, EDX);
//	GET(char*, lpBits, EAX);
//
//	WORD* buffer = (WORD*)lpBits;
//	FILE* fp;
//	fopen_s(&fp, "bmpDebug.bmp", "wb");
//	if (fp)
//	{
//		int width = pBmi->bmiHeader.biWidth;
//		int height = pBmi->bmiHeader.biHeight;
//
//		size_t arrayLen = width * height;
//
//#pragma pack(push, 1)
//		struct bmpfile_full_header {
//			unsigned char magic[2];
//			DWORD filesz;
//			WORD creator1;
//			WORD creator2;
//			DWORD bmp_offset;
//			DWORD header_sz;
//			DWORD width;
//			DWORD height;
//			WORD nplanes;
//			WORD bitspp;
//			DWORD compress_type;
//			DWORD bmp_bytesz;
//			DWORD hres;
//			DWORD vres;
//			DWORD ncolors;
//			DWORD nimpcolors;
//			DWORD R; //
//			DWORD G; //
//			DWORD B; //
//		} h;
//#pragma pack(pop)
//		h.magic[0] = 'B';
//		h.magic[1] = 'M';
//		h.creator1 = h.creator2 = 0;
//		h.header_sz = 40;
//		h.width = width;
//		h.height = height;
//		h.nplanes = 1;
//		h.bitspp = 24;
//		h.compress_type = BI_BITFIELDS;
//		h.bmp_bytesz = arrayLen * 2;
//		h.hres = 4000;
//		h.vres = 4000;
//		h.ncolors = h.nimpcolors = 0;
//		h.R = 0xF800;
//		h.G = 0x07E0;
//		h.B = 0x001F;
//		h.bmp_offset = sizeof(h);
//		h.filesz = h.bmp_offset + h.bmp_bytesz;
//		fwrite(&h, 1, sizeof(h), fp);
//		std::unique_ptr<WORD[]> pixelData(new WORD[arrayLen * 2]);
//		WORD* pixels = pixelData.get();
//#define WIDTHBYTES(bits) (((bits) + 31) / 32 * 4)
//		int pitch = WIDTHBYTES(width * 16);
//#undef WIDTHBYTES
//		for (int r = 0; r < height * 2; ++r) {
//			memcpy(pixels, reinterpret_cast<void*>(buffer), width * 2);
//			pixels += width;
//			buffer += pitch / 2; // /2 because buffer is a WORD * and pitch is in bytes
//		}
//		fwrite(pixelData.get(), 1, arrayLen * 4, fp);
//		fclose(fp);
//	}
//
//	return 0;
//}