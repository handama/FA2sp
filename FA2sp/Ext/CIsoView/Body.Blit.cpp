#include "Body.h"

#include "../../FA2sp.h"
#include <WindowsX.h>
#include <CPalette.h>
#include <CMapData.h>
#include <Drawing.h>
#include <CINI.h>
#include <ranges>
#include "../../Helpers/STDHelpers.h"
#include "../CMapData/Body.h"
#include "../../Helpers/Translations.h"
#include "../../Miscs/MultiSelection.h"
#include "../../Miscs/Palettes.h"
#include "../CLoading/Body.h"
#include <immintrin.h>
#include <mutex>
#include "../../Miscs/TheaterInfo.h"
#include <stack>
#include <array>

static byte oreOpacityTable[13] =
{
    177, 166, 154, 143, 131, 119, 108, 96, 85, 73, 61, 50, 38
};

static byte playerLocationOpacityTable[8] =
{
    105, 90, 75, 60, 45, 30, 15, 0
};

static bool TilePixels[1800] =
{
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false,
        false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false,
        false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false,
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false,
        false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false,
        false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
};

static BYTE alphaBlendTable[256][256];
static const int yOffsetTable[61] = {
    0,0,0,0,0,0,0,0,0,0,   // 0~9
    1,1,2,2,3,3,4,4,5,5,   // 10~19
    6,6,7,7,8,8,9,9,10,10, // 20~29
    11,11,12,12,13,13,14,14,15,15, // 30~39
    16,15,14,14,13,13,12,12,11,11, // 40~49
    10,10,9,9,8,8,7,7,6,6,         // 50~59
    5                              // 60
};

void CIsoViewExt::InitAlphaTable() {
    static bool initialized = false;
    if (!initialized) {
        for (int i = 0; i < 256; i++) {
            for (int j = 0; j < 256; j++) {
                alphaBlendTable[i][j] = static_cast<BYTE>((i * j) >> 8);
            }
        }
        initialized = true;
    }
}

void CIsoViewExt::BlitTransparent(LPDIRECTDRAWSURFACE7 pic, int x, int y, int width, int height, BYTE alpha, LPDIRECTDRAWSURFACE7 surface)
{
    if (!pic || alpha == 0) {
        return;
    }

    const int X_OFFSET = 1;
    const int Y_OFFSET = -29;
    const int BPP = 4;

    auto pThis = CIsoView::GetInstance();
    RECT windowRect;
    if (!surface) {
        windowRect = CIsoViewExt::GetScaledWindowRect();
        surface = GetBackBuffer();
    }
    else if (surface == GetBackBuffer()) {
        windowRect = CIsoViewExt::GetScaledWindowRect();
    }
    else {
        pThis->GetWindowRect(&windowRect);
    }

    x += X_OFFSET;
    y += Y_OFFSET;

    if (width == -1 || height == -1) {
        DDSURFACEDESC2 ddsd = { sizeof(DDSURFACEDESC2), DDSD_WIDTH | DDSD_HEIGHT };
        if (pic->GetSurfaceDesc(&ddsd) != DD_OK) {
            return;
        }
        width = ddsd.dwWidth;
        height = ddsd.dwHeight;
    }

    if (x + width < 0 || y + height < 0) {
        return;
    }
    if (x > windowRect.right || y > windowRect.bottom) {
        return;
    }

    RECT srcRect = { 0, 0, width, height };
    RECT destRect = { x, y, x + width, y + height };
    if (destRect.left < 0) {
        srcRect.left = -destRect.left;
        destRect.left = 0;
    }
    if (destRect.top < 0) {
        srcRect.top = -destRect.top;
        destRect.top = 0;
    }
    if (destRect.right > windowRect.right) {
        srcRect.right = width - (destRect.right - windowRect.right);
        destRect.right = windowRect.right;
    }
    if (destRect.bottom > windowRect.bottom) {
        srcRect.bottom = height - (destRect.bottom - windowRect.bottom);
        destRect.bottom = windowRect.bottom;
    }

    DDSURFACEDESC2 destDesc = { sizeof(DDSURFACEDESC2) };
    DDSURFACEDESC2 srcDesc = { sizeof(DDSURFACEDESC2) };
    if (surface->Lock(NULL, &destDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL) != DD_OK) {
        return;
    }
    if (pic->Lock(NULL, &srcDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL) != DD_OK) {
        surface->Unlock(NULL);
        return;
    }

    DDCOLORKEY colorKey;
    if (pic->GetColorKey(DDCKEY_SRCBLT, &colorKey) != DD_OK) {
        pic->Unlock(NULL);
        surface->Unlock(NULL);
        return;
    }
    DWORD colorKeyLow = colorKey.dwColorSpaceLowValue;
    DWORD colorKeyHigh = colorKey.dwColorSpaceHighValue;

    BYTE* destPixels = static_cast<BYTE*>(destDesc.lpSurface);
    BYTE* srcPixels = static_cast<BYTE*>(srcDesc.lpSurface);
    int destPitch = destDesc.lPitch;
    int srcPitch = srcDesc.lPitch;
    int maxDestX = destDesc.dwWidth;
    int maxDestY = destDesc.dwHeight;

    for (LONG row = 0; row < srcRect.bottom - srcRect.top; ++row) {
        LONG dy = destRect.top + row;
        if (dy < 0 || dy >= maxDestY) {
            continue;
        }

        BYTE* destLine = destPixels + dy * destPitch + destRect.left * BPP;
        BYTE* srcLine = srcPixels + row * srcPitch;
        for (LONG col = 0; col < srcRect.right - srcRect.left; ++col) {
            LONG dx = destRect.left + col;
            if (dx < 0 || dx >= maxDestX) {
                continue;
            }

            int srcIndex = col * BPP;
            int destIndex = col * BPP;
            DWORD srcColor = *reinterpret_cast<DWORD*>(srcLine + srcIndex);
            if (srcColor >= colorKeyLow && srcColor <= colorKeyHigh) {
                continue;
            }

            BYTE* destPtr = destLine + destIndex;
            if (destIndex >= 0 && destIndex < maxDestY * destPitch) {
                BYTE srcR = srcLine[srcIndex + 2];
                BYTE srcG = srcLine[srcIndex + 1];
                BYTE srcB = srcLine[srcIndex];
                BYTE destR = destPtr[2];
                BYTE destG = destPtr[1];
                BYTE destB = destPtr[0];

                destPtr[2] = alphaBlendTable[srcR][alpha] + alphaBlendTable[destR][255 - alpha];
                destPtr[1] = alphaBlendTable[srcG][alpha] + alphaBlendTable[destG][255 - alpha];
                destPtr[0] = alphaBlendTable[srcB][alpha] + alphaBlendTable[destB][255 - alpha];
            }
        }
    }

    pic->Unlock(NULL);
    surface->Unlock(NULL);
}

void CIsoViewExt::BlitTransparentDesc(LPDIRECTDRAWSURFACE7 pic, LPDIRECTDRAWSURFACE7 surface, DDSURFACEDESC2* pDestDesc,
    int x, int y, int width, int height, BYTE alpha)
{
    if (!pic || !pDestDesc || alpha == 0) {
        return;
    }

    const int X_OFFSET = 1;
    const int Y_OFFSET = -29;
    const int BPP = 4;

    auto pThis = CIsoView::GetInstance();
    RECT windowRect;
    if (surface == GetBackBuffer()) {
        windowRect = CIsoViewExt::GetScaledWindowRect();
    }
    else {
        pThis->GetWindowRect(&windowRect);
    }

    x += X_OFFSET;
    y += Y_OFFSET;

    if (width == -1 || height == -1) {
        DDSURFACEDESC2 ddsd = { sizeof(DDSURFACEDESC2), DDSD_WIDTH | DDSD_HEIGHT };
        if (pic->GetSurfaceDesc(&ddsd) != DD_OK) {
            return;
        }
        width = ddsd.dwWidth;
        height = ddsd.dwHeight;
    }

    if (x + width < 0 || y + height < 0) {
        return;
    }
    if (x > windowRect.right || y > windowRect.bottom) {
        return;
    }

    RECT srcRect = { 0, 0, width, height };
    RECT destRect = { x, y, x + width, y + height };
    if (destRect.left < 0) {
        srcRect.left = -destRect.left;
        destRect.left = 0;
    }
    if (destRect.top < 0) {
        srcRect.top = -destRect.top;
        destRect.top = 0;
    }
    if (destRect.right > windowRect.right) {
        srcRect.right = width - (destRect.right - windowRect.right);
        destRect.right = windowRect.right;
    }
    if (destRect.bottom > windowRect.bottom) {
        srcRect.bottom = height - (destRect.bottom - windowRect.bottom);
        destRect.bottom = windowRect.bottom;
    }

    DDSURFACEDESC2 srcDesc = { sizeof(DDSURFACEDESC2) };
    if (pic->Lock(NULL, &srcDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL) != DD_OK) {
        return;
    }

    DDCOLORKEY colorKey;
    if (pic->GetColorKey(DDCKEY_SRCBLT, &colorKey) != DD_OK) {
        pic->Unlock(NULL);
        return;
    }
    DWORD colorKeyLow = colorKey.dwColorSpaceLowValue;
    DWORD colorKeyHigh = colorKey.dwColorSpaceHighValue;

    BYTE* destPixels = static_cast<BYTE*>(pDestDesc->lpSurface);
    BYTE* srcPixels = static_cast<BYTE*>(srcDesc.lpSurface);
    int destPitch = pDestDesc->lPitch;
    int srcPitch = srcDesc.lPitch;
    int maxDestX = pDestDesc->dwWidth;
    int maxDestY = pDestDesc->dwHeight;

    for (LONG row = 0; row < srcRect.bottom - srcRect.top; ++row) {
        LONG dy = destRect.top + row;
        if (dy < 0 || dy >= maxDestY) {
            continue;
        }

        BYTE* destLine = destPixels + dy * destPitch + destRect.left * BPP;
        BYTE* srcLine = srcPixels + row * srcPitch;
        for (LONG col = 0; col < srcRect.right - srcRect.left; ++col) {
            LONG dx = destRect.left + col;
            if (dx < 0 || dx >= maxDestX) {
                continue;
            }

            int srcIndex = col * BPP;
            int destIndex = col * BPP;
            DWORD srcColor = *reinterpret_cast<DWORD*>(srcLine + srcIndex);
            if (srcColor >= colorKeyLow && srcColor <= colorKeyHigh) {
                continue;
            }

            BYTE* destPtr = destLine + destIndex;
            if (destIndex >= 0 && destIndex < maxDestY * destPitch) {
                if (alpha == 255)
                {
                    memcpy(destPtr, &srcColor, BPP);
                }
                else
                {
                    BYTE srcR = srcLine[srcIndex + 2];
                    BYTE srcG = srcLine[srcIndex + 1];
                    BYTE srcB = srcLine[srcIndex];
                    BYTE destR = destPtr[2];
                    BYTE destG = destPtr[1];
                    BYTE destB = destPtr[0];

                    destPtr[2] = alphaBlendTable[srcR][alpha] + alphaBlendTable[destR][255 - alpha];
                    destPtr[1] = alphaBlendTable[srcG][alpha] + alphaBlendTable[destG][255 - alpha];
                    destPtr[0] = alphaBlendTable[srcB][alpha] + alphaBlendTable[destB][255 - alpha];
                }
            }
        }
    }

    pic->Unlock(NULL);
}

void CIsoViewExt::BlitTransparentDescNoLock(LPDIRECTDRAWSURFACE7 pic, LPDIRECTDRAWSURFACE7 surface, DDSURFACEDESC2* pDestDesc,
    DDSURFACEDESC2& srcDesc, DDCOLORKEY& srcColorKey, int x, int y, int width, int height, BYTE alpha)
{
    if (!pic || !pDestDesc || alpha == 0) {
        return;
    }

    const int X_OFFSET = 1;
    const int Y_OFFSET = -29;
    const int BPP = 4;

    auto pThis = CIsoView::GetInstance();
    RECT windowRect;
    if (surface == GetBackBuffer()) {
        windowRect = CIsoViewExt::GetScaledWindowRect();
    }
    else {
        pThis->GetWindowRect(&windowRect);
    }

    x += X_OFFSET;
    y += Y_OFFSET;

    if (width == -1 || height == -1) {
        DDSURFACEDESC2 ddsd = { sizeof(DDSURFACEDESC2), DDSD_WIDTH | DDSD_HEIGHT };
        if (pic->GetSurfaceDesc(&ddsd) != DD_OK) {
            return;
        }
        width = ddsd.dwWidth;
        height = ddsd.dwHeight;
    }

    if (x + width < 0 || y + height < 0) {
        return;
    }
    if (x > windowRect.right || y > windowRect.bottom) {
        return;
    }

    RECT srcRect = { 0, 0, width, height };
    RECT destRect = { x, y, x + width, y + height };
    if (destRect.left < 0) {
        srcRect.left = -destRect.left;
        destRect.left = 0;
    }
    if (destRect.top < 0) {
        srcRect.top = -destRect.top;
        destRect.top = 0;
    }
    if (destRect.right > windowRect.right) {
        srcRect.right = width - (destRect.right - windowRect.right);
        destRect.right = windowRect.right;
    }
    if (destRect.bottom > windowRect.bottom) {
        srcRect.bottom = height - (destRect.bottom - windowRect.bottom);
        destRect.bottom = windowRect.bottom;
    }

    DWORD& colorKeyLow = srcColorKey.dwColorSpaceLowValue;
    DWORD& colorKeyHigh = srcColorKey.dwColorSpaceHighValue;

    BYTE* destPixels = static_cast<BYTE*>(pDestDesc->lpSurface);
    BYTE* srcPixels = static_cast<BYTE*>(srcDesc.lpSurface);
    int destPitch = pDestDesc->lPitch;
    int srcPitch = srcDesc.lPitch;
    int maxDestX = pDestDesc->dwWidth;
    int maxDestY = pDestDesc->dwHeight;

    for (LONG row = 0; row < srcRect.bottom - srcRect.top; ++row) {
        LONG dy = destRect.top + row;
        if (dy < 0 || dy >= maxDestY) {
            continue;
        }

        BYTE* destLine = destPixels + dy * destPitch + destRect.left * BPP;
        BYTE* srcLine = srcPixels + row * srcPitch;
        for (LONG col = 0; col < srcRect.right - srcRect.left; ++col) {
            LONG dx = destRect.left + col;
            if (dx < 0 || dx >= maxDestX) {
                continue;
            }

            int srcIndex = col * BPP;
            int destIndex = col * BPP;
            DWORD srcColor = *reinterpret_cast<DWORD*>(srcLine + srcIndex);
            if (srcColor >= colorKeyLow && srcColor <= colorKeyHigh) {
                continue;
            }

            BYTE* destPtr = destLine + destIndex;
            if (destIndex >= 0 && destIndex < maxDestY * destPitch) {
                if (alpha == 255)
                {
                    memcpy(destPtr, &srcColor, BPP);
                }
                else
                {
                    BYTE srcB = (BYTE)(srcColor & 0xFF);
                    BYTE srcG = (BYTE)((srcColor >> 8) & 0xFF);
                    BYTE srcR = (BYTE)((srcColor >> 16) & 0xFF);
                    BYTE destR = destPtr[2];
                    BYTE destG = destPtr[1];
                    BYTE destB = destPtr[0];

                    destPtr[2] = alphaBlendTable[srcR][alpha] + alphaBlendTable[destR][255 - alpha];
                    destPtr[1] = alphaBlendTable[srcG][alpha] + alphaBlendTable[destG][255 - alpha];
                    destPtr[0] = alphaBlendTable[srcB][alpha] + alphaBlendTable[destB][255 - alpha];
                }
            }
        }
    }
}

void CIsoViewExt::BlitSHPTransparent(CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, ImageDataClass* pd, Palette* newPal, BYTE alpha, COLORREF houseColor, int extraLightType, bool remap)
{
    if (alpha == 0 || !pd || pd->Flag == ImageDataFlag::SurfaceData || !pd->pImageBuffer || !dst) {
        return;
    }

    const int X_OFFSET = 31;
    const int Y_OFFSET = -29;
    const int BPP = 4;

    x += X_OFFSET;
    y += Y_OFFSET;

    BYTE* src = static_cast<BYTE*>(pd->pImageBuffer);
    int swidth = pd->FullWidth;
    int sheight = pd->FullHeight;

    if (x + swidth < window.left || y + sheight < window.top) {
        return;
    }
    if (x >= window.right || y >= window.bottom) {
        return;
    }

    RECT srcRect = { 0, 0, swidth, sheight };
    RECT destRect = { x, y, x + swidth, y + sheight };
    if (destRect.left < 0) {
        srcRect.left = 1 - destRect.left;
    }
    if (destRect.top < 0) {
        srcRect.top = 1 - destRect.top;
    }
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        destRect.right = window.right;
    }
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        destRect.bottom = window.bottom;
    }

    if (!newPal) {
        newPal = pd->pPalette;
    }
    bool isMultiSelected = false;
    RGBClass oreColor;
    bool isEmphasizingOre = false;
    byte oreOpacity;

    if (extraLightType == -10 || extraLightType >= 500) {
        isMultiSelected = MultiSelection::IsSelected(CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y);
        if (extraLightType >= 500) {
            int overlay = extraLightType - 500;
            if (overlay == 0x18 || overlay == 0x19 || // BRIDGE1, BRIDGE2
                overlay == 0x3B || overlay == 0x3C || // RAILBRDG1, RAILBRDG2
                overlay == 0xED || overlay == 0xEE || // BRIDGEB1, BRIDGEB2
                (overlay >= 0x4A && overlay <= 0x65) || // LOBRDG 1-28
                (overlay >= 0xCD && overlay <= 0xEC)) { // LOBRDGB 1-4
                isMultiSelected = MultiSelection::IsSelected(
                    CIsoViewExt::CurrentDrawCellLocation.X + 1,
                    CIsoViewExt::CurrentDrawCellLocation.Y + 1);
            }

            if (RenderingMap && RenderEmphasizeOres && CMapDataExt::IsOre(overlay))
            {
                isEmphasizingOre = true;
                auto ovrd = CMapData::Instance->GetOverlayDataAt(
                    CMapData::Instance->GetCoordIndex(
                        CIsoViewExt::CurrentDrawCellLocation.X,
                        CIsoViewExt::CurrentDrawCellLocation.Y));
                oreColor = CMapDataExt::GetOverlayTypeData(overlay).RadarColor;
                oreOpacity = oreOpacityTable[std::min(ovrd, (byte)13)];
            }
        }
    }

    BGRStruct color;
    auto pRGB = reinterpret_cast<ColorStruct*>(&houseColor);
    color.R = pRGB->red;
    color.G = pRGB->green;
    color.B = pRGB->blue;
    if (LightingStruct::CurrentLighting != LightingStruct::NoLighting) {
        if (extraLightType >= 500) {
            newPal = PalettesManager::GetOverlayPalette(newPal, CIsoViewExt::CurrentDrawCellLocation, extraLightType - 500);
        }
        else {
            newPal = PalettesManager::GetObjectPalette(newPal, color, remap, CIsoViewExt::CurrentDrawCellLocation, false, extraLightType);
        }
    }
    else {
        newPal = PalettesManager::GetPalette(newPal, color, remap);
    }

    BYTE* srcBase = src;
    BYTE* destBase = static_cast<BYTE*>(dst) + destRect.top * boundary.dpitch + destRect.left * BPP;
    BYTE* surfaceEnd = static_cast<BYTE*>(dst) + boundary.dpitch * boundary.dwHeight;

    for (LONG row = srcRect.top; row < srcRect.bottom; ++row) {
        LONG left = pd->pPixelValidRanges[row].First;
        LONG right = pd->pPixelValidRanges[row].Last;
        if (left < srcRect.left) {
            left = srcRect.left;
        }
        if (right >= srcRect.right) {
            right = srcRect.right - 1;
        }
        if (left > right) {
            continue;
        }

        BYTE* srcPtr = srcBase + row * swidth + left;
        BYTE* destPtr = destBase + row * boundary.dpitch + left * BPP;
        for (LONG col = left; col <= right; ++col, ++srcPtr, destPtr += BPP) {
            if (destRect.left + col < 0) {
                continue;
            }

            BYTE pixelValue = *srcPtr;
            if (pixelValue && destPtr >= dst && destPtr + BPP <= surfaceEnd) {
                BGRStruct c = newPal->Data[pixelValue];
                if (alpha < 255) {
                    BGRStruct oriColor = *reinterpret_cast<BGRStruct*>(destPtr);
                    c.B = alphaBlendTable[c.B][alpha] + alphaBlendTable[oriColor.B][255 - alpha];
                    c.G = alphaBlendTable[c.G][alpha] + alphaBlendTable[oriColor.G][255 - alpha];
                    c.R = alphaBlendTable[c.R][alpha] + alphaBlendTable[oriColor.R][255 - alpha];
                }
                if (isMultiSelected && (!RenderingMap || RenderingMap && RenderCurrentLayers)) {
                    RGBClass* selColor = reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor);
                    c.B = (c.B * 2 + selColor->B) / 3;
                    c.G = (c.G * 2 + selColor->G) / 3;
                    c.R = (c.R * 2 + selColor->R) / 3;
                }
                if (isEmphasizingOre)
                {
                    c.B = alphaBlendTable[c.B][oreOpacity] + alphaBlendTable[oreColor.B][255 - oreOpacity];
                    c.G = alphaBlendTable[c.G][oreOpacity] + alphaBlendTable[oreColor.G][255 - oreOpacity];
                    c.R = alphaBlendTable[c.R][oreOpacity] + alphaBlendTable[oreColor.R][255 - oreOpacity];
                }
                memcpy(destPtr, &c, BPP);
            }
        }
    }
}

void CIsoViewExt::BlitSHPTransparent(CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, ImageDataClassSafe* pd, Palette* newPal, BYTE alpha,
    COLORREF houseColor, int extraLightType, bool remap)
{
    if (alpha == 0 || !pd || pd->Flag == ImageDataFlag::SurfaceData || !pd->pImageBuffer || !dst) {
        return;
    }

    const int X_OFFSET = 31;
    const int Y_OFFSET = -29;
    const int BPP = 4;

    x += X_OFFSET;
    y += Y_OFFSET;

    BYTE* srcBase = static_cast<BYTE*>(pd->pImageBuffer.get());
    int swidth = pd->FullWidth;
    int sheight = pd->FullHeight;

    if (x + swidth < window.left || y + sheight < window.top ||
        x >= window.right || y >= window.bottom) {
        return;
    }

    RECT srcRect = { 0, 0, swidth, sheight };
    RECT destRect = { x, y, x + swidth, y + sheight };

    if (destRect.left < 0) srcRect.left = 1 - destRect.left;
    if (destRect.top < 0)  srcRect.top = 1 - destRect.top;
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        destRect.right = window.right;
    }
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        destRect.bottom = window.bottom;
    }

    if (srcRect.right <= srcRect.left || srcRect.bottom <= srcRect.top) {
        return;
    }

    if (!newPal) [[likely]] {
        newPal = pd->pPalette;
    }

    bool isMultiSelected = false;
    bool isEmphasizingOre = false;
    RGBClass oreColor{};
    byte oreOpacity = 0;

    if (extraLightType == -10 || extraLightType >= 500) {
        isMultiSelected = MultiSelection::IsSelected(
            CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y);

        if (extraLightType >= 500 && RenderingMap && RenderEmphasizeOres) {
            int overlay = extraLightType - 500;
            if (CMapDataExt::IsOre(overlay)) {
                isEmphasizingOre = true;
                int pos = CMapData::Instance->GetCoordIndex(
                    CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y);
                auto ovrd = CMapData::Instance->GetOverlayDataAt(pos);
                oreColor = CMapDataExt::GetOverlayTypeData(overlay).RadarColor;
                oreOpacity = oreOpacityTable[std::min(ovrd, (byte)13)];
            }
        }
    }

    BGRStruct houseBGR{};
    auto pRGB = reinterpret_cast<ColorStruct*>(&houseColor);
    houseBGR.R = pRGB->red;
    houseBGR.G = pRGB->green;
    houseBGR.B = pRGB->blue;

    if (LightingStruct::CurrentLighting != LightingStruct::NoLighting) {
        if (extraLightType >= 500) {
            newPal = PalettesManager::GetOverlayPalette(newPal, CIsoViewExt::CurrentDrawCellLocation, extraLightType - 500);
        }
        else if (extraLightType > -100) {
            newPal = PalettesManager::GetObjectPalette(newPal, houseBGR, remap,
                CIsoViewExt::CurrentDrawCellLocation, false, extraLightType);
        }
    }
    else {
        if (extraLightType >= 500 && ExtConfigs::InGameDisplay_RemapableOverlay) {
            auto it = CViewObjectsExt::WallDamageStages.find(extraLightType - 500);
            if (it != CViewObjectsExt::WallDamageStages.end()) {
                int pos = CMapData::Instance->GetCoordIndex(
                    CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y);
                if (pos < static_cast<int>(CMapDataExt::CellDataExts.size())) {
                    auto pCellRGB = reinterpret_cast<ColorStruct*>(&CMapDataExt::CellDataExts[pos].RemapableColor);
                    houseBGR.R = pCellRGB->red;
                    houseBGR.G = pCellRGB->green;
                    houseBGR.B = pCellRGB->blue;
                    remap = true;
                }
            }
        }
        newPal = PalettesManager::GetPalette(newPal, houseBGR, remap);
    }

    const bool doAlphaBlend = alpha < 255;
    const bool doMultiSel = isMultiSelected && (!RenderingMap || (RenderingMap && RenderCurrentLayers));
    const bool doOre = isEmphasizingOre;

    BYTE* destBase = static_cast<BYTE*>(dst) + destRect.top * boundary.dpitch + destRect.left * 4;
    BYTE* surfaceEnd = static_cast<BYTE*>(dst) + boundary.dpitch * boundary.dwHeight;

    for (LONG row = srcRect.top; row < srcRect.bottom; ++row) {
        LONG left = std::max((LONG)pd->pPixelValidRanges[row].First, srcRect.left);
        LONG right = std::min((LONG)pd->pPixelValidRanges[row].Last, srcRect.right - 1);
        if (left > right) continue;

        BYTE* srcPtr = srcBase + row * swidth + left;
        BYTE* destPtr = destBase + row * boundary.dpitch + left * BPP;

        for (LONG col = left; col <= right; ++col, ++srcPtr, destPtr += BPP) {
            if (destRect.left + col < 0) continue;

            BYTE pixelValue = *srcPtr;
            if (pixelValue == 0) [[likely]] continue;

            if (destPtr + BPP > surfaceEnd) break;

            BGRStruct c = newPal->Data[pixelValue];

            if (doAlphaBlend) {
                BGRStruct ori = *reinterpret_cast<const BGRStruct*>(destPtr);
                c.B = alphaBlendTable[c.B][alpha] + alphaBlendTable[ori.B][255 - alpha];
                c.G = alphaBlendTable[c.G][alpha] + alphaBlendTable[ori.G][255 - alpha];
                c.R = alphaBlendTable[c.R][alpha] + alphaBlendTable[ori.R][255 - alpha];
            }

            if (doMultiSel) {
                RGBClass* selColor = reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor);
                c.B = (c.B * 2 + selColor->B) / 3;
                c.G = (c.G * 2 + selColor->G) / 3;
                c.R = (c.R * 2 + selColor->R) / 3;
            }

            if (doOre) {
                c.B = alphaBlendTable[c.B][oreOpacity] + alphaBlendTable[oreColor.B][255 - oreOpacity];
                c.G = alphaBlendTable[c.G][oreOpacity] + alphaBlendTable[oreColor.G][255 - oreOpacity];
                c.R = alphaBlendTable[c.R][oreOpacity] + alphaBlendTable[oreColor.R][255 - oreOpacity];
            }

            memcpy(destPtr, &c, BPP);
        }
    }
}

void CIsoViewExt::BlitSHPTransparent_Building(CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, ImageDataClassSafe* pd, Palette* newPal, BYTE alpha,
    COLORREF houseColor, COLORREF addOnColor, bool isRubble, bool isTerrain)
{
    if (alpha == 0 || !pd || pd->Flag == ImageDataFlag::SurfaceData || !pd->pImageBuffer || !dst) {
        return;
    }

    const int X_OFFSET = 31;
    const int Y_OFFSET = -29;
    const int BPP = *(int*)0x72A8C0;

    x += X_OFFSET;
    y += Y_OFFSET;

    BYTE* src = static_cast<BYTE*>(pd->pImageBuffer.get());
    BYTE* opacity = pd->pOpacity.get();
    int swidth = pd->FullWidth;
    int sheight = pd->FullHeight;

    bool hasPerPixelOpacity = (opacity != nullptr);

    if (x + swidth < window.left || y + sheight < window.top) {
        return;
    }
    if (x >= window.right || y >= window.bottom) {
        return;
    }

    RECT srcRect = { 0, 0, swidth, sheight };
    RECT destRect = { x, y, x + swidth, y + sheight };
    if (destRect.left < 0) {
        srcRect.left = 1 - destRect.left;
    }
    if (destRect.top < 0) {
        srcRect.top = 1 - destRect.top;
    }
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        destRect.right = window.right;
    }
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        destRect.bottom = window.bottom;
    }

    // buildings use pre-calculate palettes
    if (!newPal) [[likely]] {
        newPal = pd->pPalette;
        BGRStruct color;
        auto pRGB = reinterpret_cast<ColorStruct*>(&houseColor);
        color.R = pRGB->red;
        color.G = pRGB->green;
        color.B = pRGB->blue;
        if (LightingStruct::CurrentLighting == LightingStruct::NoLighting) {
            newPal = PalettesManager::GetPalette(newPal, color, !isTerrain && !isRubble);
        }
        else {
            newPal = PalettesManager::GetObjectPalette(newPal, color, !isTerrain && !isRubble,
                CIsoViewExt::CurrentDrawCellLocation, false, isRubble || isTerrain ? 4 : 3);
        }
    }

    BYTE* srcBase = src;
    BYTE* opacityBase = opacity;
    BYTE* destBase = static_cast<BYTE*>(dst) + destRect.top * boundary.dpitch + destRect.left * BPP;
    BYTE* surfaceEnd = static_cast<BYTE*>(dst) + boundary.dpitch * boundary.dwHeight;

    for (LONG row = srcRect.top; row < srcRect.bottom; ++row) {
        LONG left = pd->pPixelValidRanges[row].First;
        LONG right = pd->pPixelValidRanges[row].Last;
        if (left < srcRect.left) {
            left = srcRect.left;
        }
        if (right >= srcRect.right) {
            right = srcRect.right - 1;
        }
        if (left > right) {
            continue;
        }

        BYTE* srcPtr = srcBase + row * swidth + left;
        BYTE* destPtr = destBase + row * boundary.dpitch + left * BPP;
        BYTE* opacityPtr = opacityBase ? (opacityBase + row * swidth + left) : NULL;
        for (LONG col = left; col <= right; ++col, ++srcPtr, ++opacityPtr, destPtr += BPP) {
            if (destRect.left + col < 0) {
                continue;
            }

            BYTE pixelValue = *srcPtr;
            if (pixelValue && destPtr >= dst && destPtr + BPP <= surfaceEnd) {
                BGRStruct c = newPal->Data[pixelValue];
                BYTE finalAlpha = alpha;
                if (hasPerPixelOpacity && opacityBase)
                {
                    unsigned int temp = (unsigned int)alpha * (*opacityPtr) / 255u;
                    finalAlpha = (BYTE)temp;
                    if (finalAlpha == 0) continue;
                }
                if (finalAlpha < 255) {
                    BGRStruct oriColor = *reinterpret_cast<BGRStruct*>(destPtr);
                    c.B = alphaBlendTable[c.B][finalAlpha] + alphaBlendTable[oriColor.B][255 - finalAlpha];
                    c.G = alphaBlendTable[c.G][finalAlpha] + alphaBlendTable[oriColor.G][255 - finalAlpha];
                    c.R = alphaBlendTable[c.R][finalAlpha] + alphaBlendTable[oriColor.R][255 - finalAlpha];
                }
                memcpy(destPtr, &c, BPP);
            }
        }
    }
}

void CIsoViewExt::BlitSHPTransparent_AlphaImage(CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, ImageDataClassSafe* pd)
{
    if (!pd || pd->Flag == ImageDataFlag::SurfaceData || !pd->pImageBuffer || !dst) {
        return;
    }

    const int X_OFFSET = 31;
    const int Y_OFFSET = -29;
    const int BPP = 4;

    x += X_OFFSET;
    y += Y_OFFSET;

    BYTE* srcBase = static_cast<BYTE*>(pd->pImageBuffer.get());
    int swidth = pd->FullWidth;
    int sheight = pd->FullHeight;

    if (x + swidth < window.left || y + sheight < window.top ||
        x >= window.right || y >= window.bottom) {
        return;
    }

    RECT srcRect = { 0, 0, swidth, sheight };
    RECT destRect = { x, y, x + swidth, y + sheight };

    if (destRect.left < 0) srcRect.left = 1 - destRect.left;
    if (destRect.top < 0)  srcRect.top = 1 - destRect.top;
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        destRect.right = window.right;
    }
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        destRect.bottom = window.bottom;
    }

    if (srcRect.right <= srcRect.left || srcRect.bottom <= srcRect.top) {
        return;
    }

    BYTE* destBase = static_cast<BYTE*>(dst) + destRect.top * boundary.dpitch + destRect.left * 4;
    BYTE* surfaceEnd = static_cast<BYTE*>(dst) + boundary.dpitch * boundary.dwHeight;

    for (LONG row = srcRect.top; row < srcRect.bottom; ++row) {
        LONG left = srcRect.left;
        LONG right = srcRect.right - 1;
        if (left > right) continue;

        BYTE* srcPtr = srcBase + row * swidth + left;
        BYTE* destPtr = destBase + row * boundary.dpitch + left * BPP;

        for (LONG col = left; col <= right; ++col, ++srcPtr, destPtr += BPP) {
            if (destRect.left + col < 0) continue;

            BYTE pixelValue = *srcPtr;
            if (pixelValue == 127 || destPtr + BPP > surfaceEnd) [[likely]] continue;

            BGRStruct ori = *reinterpret_cast<const BGRStruct*>(destPtr);
            BGRStruct c;

            int factor = pixelValue * 2;
            c.B = std::min((ori.B * factor) >> 8, 255);
            c.G = std::min((ori.G * factor) >> 8, 255);
            c.R = std::min((ori.R * factor) >> 8, 255);

            memcpy(destPtr, &c, BPP);
        }
    }
}

void CIsoViewExt::BlitSHPTransparent(LPDDSURFACEDESC2 lpDesc, int x, int y, ImageDataClass* pd, Palette* newPal, BYTE alpha, COLORREF houseColor)
{
    auto pThis = CIsoView::GetInstance();
    RECT window = CIsoViewExt::GetScaledWindowRect();
    DDBoundary boundary{ lpDesc->dwWidth, lpDesc->dwHeight, lpDesc->lPitch };
    CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary, x, y, pd, newPal, alpha, houseColor);
}

void CIsoViewExt::BlitSHPTransparent(LPDDSURFACEDESC2 lpDesc, int x, int y, ImageDataClassSafe* pd, Palette* newPal, BYTE alpha, COLORREF houseColor)
{
    auto pThis = CIsoView::GetInstance();
    RECT window = CIsoViewExt::GetScaledWindowRect();
    DDBoundary boundary{ lpDesc->dwWidth, lpDesc->dwHeight, lpDesc->lPitch };
    CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary, x, y, pd, newPal, alpha, houseColor);
}

template<bool FlatToGround, bool MaskShadow, bool CellHeight,
    bool AlphaBlend, bool MultiSel, bool Ore, bool Player>
void BlitTerrainImpl(
    CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, CTileBlockClass* subTile,
    Palette* pal, BYTE alpha, std::vector<byte>* mask,
    std::vector<byte>* heightMask, byte height,
    std::vector<int>* cellHeightMask, int tileSet,
    const RGBClass* selColor, byte playerOpacity, const RGBClass* playerColor,
    bool hasOre, const RGBClass& oreColor, byte oreOpacity)
{
    const int TILE_WIDTH = 60;
    const int TILE_HEIGHT = 30;
    const int X_OFFSET = 61;
    const int Y_OFFSET = 1;
    const int BPP = 4;
    const BGRStruct SHADOW_COLOR = { 0, 0, 0 };

    x += X_OFFSET;
    y += Y_OFFSET;

    int swidth = subTile->BlockWidth;
    int sheight = subTile->BlockHeight;

    RECT srcRect = { 0, 0, swidth, sheight };
    RECT destRect = { x, y, x + swidth, y + sheight };

    if (destRect.left < 0) srcRect.left = 1 - destRect.left;
    if (destRect.top < 0)  srcRect.top = 1 - destRect.top;
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        destRect.right = window.right;
    }
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        destRect.bottom = window.bottom;
    }

    if (srcRect.right <= srcRect.left || srcRect.bottom <= srcRect.top) return;

    BYTE* srcBase = static_cast<BYTE*>(subTile->ImageData);
    BYTE* destBase = static_cast<BYTE*>(dst) + destRect.top * boundary.dpitch + destRect.left * 4;
    BYTE* surfaceEnd = static_cast<BYTE*>(dst) + boundary.dpitch * boundary.dwHeight;

    for (LONG row = srcRect.top; row < srcRect.bottom; ++row) {
        LONG left = std::max((LONG)subTile->pPixelValidRanges[row].First, srcRect.left);
        LONG right = std::min((LONG)subTile->pPixelValidRanges[row].Last, srcRect.right - 1);
        if (left > right) continue;

        BYTE* srcPtr = srcBase + row * swidth + left;
        BYTE* destPtr = destBase + row * boundary.dpitch + left * BPP;

        for (LONG col = left; col <= right; ++col, ++srcPtr, destPtr += BPP) {
            if constexpr (FlatToGround) {
                int posInTile = col + subTile->XMinusExX + (row + subTile->YMinusExY) * TILE_WIDTH;
                if (col + subTile->XMinusExX < 0 ||
                    col + subTile->XMinusExX >= TILE_WIDTH ||
                    row + subTile->YMinusExY < 0 ||
                    row + subTile->YMinusExY >= TILE_HEIGHT ||
                    !TilePixels[posInTile]) {
                    continue;
                }
            }

            BYTE pixelValue = *srcPtr;
            if (pixelValue == 0) [[likely]] continue;

            if (destPtr + BPP > surfaceEnd) break;

            BGRStruct c = pal->Data[pixelValue];

            if constexpr (MaskShadow) {
                int wx = destRect.left + col;
                int wy = destRect.top + row;
                if (wx >= window.left && wx < window.right &&
                    wy >= window.top && wy < window.bottom) {
                    int index = wx - window.left + (wy - window.top) * (window.right - window.left);
                    if ((*heightMask)[index] >= height) {
                        if (auto shadow = (*mask)[index]) {
                            BYTE keep = alphaBlendTable[255][127];
                            BYTE blend = 255 - keep;
                            c.R = alphaBlendTable[SHADOW_COLOR.R][blend] + alphaBlendTable[c.R][keep];
                            c.G = alphaBlendTable[SHADOW_COLOR.G][blend] + alphaBlendTable[c.G][keep];
                            c.B = alphaBlendTable[SHADOW_COLOR.B][blend] + alphaBlendTable[c.B][keep];
                        }
                    }
                }
            }

            if constexpr (CellHeight) { 
                int wx = destRect.left + col; 
                int wy = destRect.top + row; 
                if (wx >= window.left && wx < window.right 
                    && wy >= window.top && wy < window.bottom)
                { 
                    int yOffset = 0; 
                    int cellRowIdx = col + subTile->XMinusExX; 
                    if (cellRowIdx >= 0 && cellRowIdx <= 30) 
                        yOffset = (cellRowIdx + 2) / 2; 
                    else if (cellRowIdx > 30 && cellRowIdx <= 60) 
                        yOffset = (60 - cellRowIdx + 1) / 2; 
                    yOffset = std::min(15, yOffset); 
                    int offset = (-subTile->YMinusExY - 15 - (row - srcRect.top)); 
                    int value = height * 30 - yOffset + (subTile->YMinusExY < 0 ? (offset + 30) : 0) - 2; 
                    value = std::max(0, value); 
                    int index = wx - window.left + (wy - window.top) * (window.right - window.left); 
                    (*cellHeightMask)[index] = value; 
                } 
            }

            if constexpr (AlphaBlend) {
                BGRStruct ori = *reinterpret_cast<const BGRStruct*>(destPtr);
                c.B = alphaBlendTable[c.B][alpha] + alphaBlendTable[ori.B][255 - alpha];
                c.G = alphaBlendTable[c.G][alpha] + alphaBlendTable[ori.G][255 - alpha];
                c.R = alphaBlendTable[c.R][alpha] + alphaBlendTable[ori.R][255 - alpha];
            }

            if constexpr (MultiSel) {
                c.B = (c.B * 2 + selColor->B) / 3;
                c.G = (c.G * 2 + selColor->G) / 3;
                c.R = (c.R * 2 + selColor->R) / 3;
            }

            if constexpr (Ore) {
                if (hasOre) {
                    c.B = alphaBlendTable[c.B][oreOpacity] + alphaBlendTable[oreColor.B][255 - oreOpacity];
                    c.G = alphaBlendTable[c.G][oreOpacity] + alphaBlendTable[oreColor.G][255 - oreOpacity];
                    c.R = alphaBlendTable[c.R][oreOpacity] + alphaBlendTable[oreColor.R][255 - oreOpacity];
                }
            }

            if constexpr (Player) {
                if (playerOpacity > 0) {
                    c.B = alphaBlendTable[c.B][playerOpacity] + alphaBlendTable[playerColor->B][255 - playerOpacity];
                    c.G = alphaBlendTable[c.G][playerOpacity] + alphaBlendTable[playerColor->G][255 - playerOpacity];
                    c.R = alphaBlendTable[c.R][playerOpacity] + alphaBlendTable[playerColor->R][255 - playerOpacity];
                }
            }

            memcpy(destPtr, &c, BPP);
        }
    }
}

using ImplFunc = void(*)(CIsoView*, void*, const RECT&, const DDBoundary&, int, int,
    CTileBlockClass*, Palette*, BYTE, std::vector<byte>*,
    std::vector<byte>*, byte, std::vector<int>*, int,
    const RGBClass*, byte, const RGBClass*, bool, const RGBClass&, byte);

template<size_t... Is>
static constexpr std::array<ImplFunc, 128> make_dispatch_table(std::index_sequence<Is...>) {
    return { BlitTerrainImpl<(Is & 1) != 0, (Is & 2) != 0, (Is & 4) != 0,
                             (Is & 8) != 0, (Is & 16) != 0, (Is & 32) != 0,
                             (Is & 64) != 0>... };
}
static constexpr auto DISPATCH_TABLE = make_dispatch_table(std::make_index_sequence<128>());

void CIsoViewExt::BlitTerrain(CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, CTileBlockClass* subTile, Palette* pal, BYTE alpha,
    std::vector<byte>* mask, std::vector<byte>* heightMask, byte height, std::vector<int>* cellHeightMask, int tileSet)
{
    if (alpha == 0 || !subTile || !subTile->HasValidImage || !subTile->ImageData || !dst || !subTile->pPixelValidRanges) {
        return;
    }

    Palette* newPal = pal;
    BGRStruct dummyColor{};
    if (LightingStruct::CurrentLighting != LightingStruct::NoLighting) {
        newPal = PalettesManager::GetObjectPalette(pal, dummyColor, false, CIsoViewExt::CurrentDrawCellLocation);
    }

    bool multiSelected = MultiSelection::IsSelected(CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y);
    bool isEmphasizingOre = false;
    RGBClass oreColor{};
    byte oreOpacity = 0;
    if (RenderingMap && RenderEmphasizeOres) {
        int pos = CMapData::Instance->GetCoordIndex(CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y);
        auto ovr = CMapDataExt::GetExtension()->GetOverlayAt(pos);
        if (CMapDataExt::IsOre(ovr)) {
            isEmphasizingOre = true;
            auto ovrd = CMapData::Instance->GetOverlayDataAt(pos);
            oreColor = CMapDataExt::GetOverlayTypeData(ovr).RadarColor;
            oreOpacity = oreOpacityTable[std::min(ovrd, (byte)13)];
        }
    }
    bool isEmphasizingPlayer = false;
    byte playerOpacity = 0;
    if (RenderingMap && RenderMarkStartings) {
        int players = CMapDataExt::GetPlayerLocationCountAtCell(
            CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y);
        if (players > 0) {
            isEmphasizingPlayer = true;
            playerOpacity = playerLocationOpacityTable[players - 1];
        }
    }

    const bool doFlatToGround = ExtConfigs::FlatToGroundHideExtra && CFinalSunApp::Instance->FlatToGround;
    const bool doMaskShadow = mask != nullptr;
    const bool doCellHeight = cellHeightMask != nullptr;
    const bool doAlphaBlend = alpha < 255;
    const bool doMultiSel = multiSelected && (!RenderingMap || (RenderingMap && RenderCurrentLayers));
    const bool doOre = isEmphasizingOre;
    const bool doPlayer = isEmphasizingPlayer;

    const RGBClass* selColor = doMultiSel ? reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor) : nullptr;
    const RGBClass* playerColor = doPlayer ? reinterpret_cast<RGBClass*>(&ExtConfigs::PlayerLocation_Color) : nullptr;

    int idx = (doFlatToGround ? 1 : 0) |
        (doMaskShadow ? 2 : 0) |
        (doCellHeight ? 4 : 0) |
        (doAlphaBlend ? 8 : 0) |
        (doMultiSel ? 16 : 0) |
        (doOre ? 32 : 0) |
        (doPlayer ? 64 : 0);

    DISPATCH_TABLE[idx](
        pThis, dst, window, boundary, x, y, subTile, newPal, alpha,
        mask, heightMask, height, cellHeightMask, tileSet,
        selColor, playerOpacity, playerColor, isEmphasizingOre, oreColor, oreOpacity
        );
}

void CIsoViewExt::MaskShadowPixels(
    const RECT& window, int x, int y,
    ImageDataClassSafe* pd,
    std::vector<char>& mask,
    std::vector<byte>& heightMask,
    byte height)
{
    if (!pd || !pd->pImageBuffer || !pd->pPixelValidRanges || mask.empty()) {
        return;
    }

    constexpr int X_OFFSET = 31;
    constexpr int Y_OFFSET = -29;

    x += X_OFFSET;
    y += Y_OFFSET;

    const int swidth = pd->FullWidth;
    const int sheight = pd->FullHeight;

    if (x + swidth < window.left || y + sheight < window.top ||
        x >= window.right || y >= window.bottom) {
        return;
    }

    RECT srcRect{ 0,0,swidth,sheight };
    RECT destRect{ x,y,x + swidth,y + sheight };

    if (destRect.left < window.left)
        srcRect.left = window.left - destRect.left;
    if (destRect.top < window.top)
        srcRect.top = window.top - destRect.top;
    if (destRect.right > window.right)
        srcRect.right -= destRect.right - window.right;
    if (destRect.bottom > window.bottom)
        srcRect.bottom -= destRect.bottom - window.bottom;

    const int maskWidth = window.right - window.left;

    BYTE* src = static_cast<BYTE*>(pd->pImageBuffer.get());

    for (int row = srcRect.top; row < srcRect.bottom; ++row)
    {
        auto& range = pd->pPixelValidRanges[row];

        int left = std::max<int>(range.First, srcRect.left);
        int right = std::min<int>(range.Last, srcRect.right - 1);
        if (left > right) continue;

        BYTE* srcPtr = src + row * swidth + left;

        int dy = destRect.top + row - window.top;
        int dxBase = destRect.left + left - window.left;

        BYTE* maskPtr = reinterpret_cast<BYTE*>(mask.data()) + dy * maskWidth + dxBase;
        BYTE* heightPtr = heightMask.data() + dy * maskWidth + dxBase;

        for (int col = left; col <= right; ++col)
        {
            BYTE pixel = *srcPtr++;
            if (!pixel) {
                ++maskPtr;
                ++heightPtr;
                continue;
            }

            *maskPtr++ = 1;
            *heightPtr++ = height;
        }
    }
}

void CIsoViewExt::DrawShadowMask(
    void* dst,
    const DDBoundary& boundary,
    const RECT& window,
    const std::vector<byte>& mask,
    const std::vector<byte>& shadowHeightMask,
    const std::vector<int>& cellHeightMask)
{
    if (!dst || mask.empty()) return;

    const int BPP = 4;
    constexpr BYTE ALPHA = 128;
    constexpr int KEEP = 255 - ALPHA;
    constexpr int BLEND = ALPHA; 

    const int maskWidth = window.right - window.left;
    const int maskHeight = window.bottom - window.top;

    BYTE* base = static_cast<BYTE*>(dst);
    const int width = boundary.dwWidth;
    const int height = boundary.dwHeight;

    static std::array<unsigned short, 256> attenuationFactors = []() {
        std::array<unsigned short, 256> factors{};
        unsigned int factor = 256; 
        for (int i = 0; i < 256; ++i) {
            factors[i] = static_cast<unsigned short>(factor);
            factor = (factor * KEEP) / 255;
        }
        return factors;
    }();

    for (int y = 0; y < maskHeight; ++y)
    {
        int dy = window.top + y;
        if (static_cast<unsigned>(dy) >= static_cast<unsigned>(height)) continue;

        BYTE* destLine = base + dy * boundary.dpitch + window.left * BPP;
        const BYTE* maskPtr = mask.data() + y * maskWidth;
        const BYTE* shadowPtr = shadowHeightMask.data() + y * maskWidth;
        const int* cellPtr = cellHeightMask.data() + y * maskWidth;

        for (int x = 0; x < maskWidth; ++x)
        {
            BYTE count = maskPtr[x];
            if (count == 0) continue;

            int dx = window.left + x;
            if (static_cast<unsigned>(dx) >= static_cast<unsigned>(width)) continue;

            if (cellPtr[x] > 30 * shadowPtr[x]) continue;

            BYTE* dest = destLine + x * BPP;
            BGRStruct* color = reinterpret_cast<BGRStruct*>(dest);

            unsigned short factor = attenuationFactors[count];
            color->R = (color->R * factor) >> 8;
            color->G = (color->G * factor) >> 8;
            color->B = (color->B * factor) >> 8;
        }
    }
}
