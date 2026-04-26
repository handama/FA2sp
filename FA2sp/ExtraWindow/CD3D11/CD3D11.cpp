#include "CD3D11.h"
#include "../../Ext/CLoading/Body.h"
#include "DirectXCore.h"
#include "../../Miscs/Palettes.h"
#include "../../Ext/CMapData/Body.h"

HWND CD3D11::m_hwnd;

std::unique_ptr<DirectXCore> g_pDX = nullptr;

// CD3D11.cpp
void CD3D11::Create(HWND hParent) {
    m_hwnd = CreateDialog(static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(340),
        hParent,
        CD3D11::DlgProc);
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
        g_pDX = std::make_unique<DirectXCore>();
        if (!g_pDX->Initialize(m_hwnd)) {
            g_pDX = nullptr;
            EndDialog(m_hwnd, IDCANCEL);
            return;
        }
        InitializeResources();

    }
    else {
        Logger::Error("Failed to create CD3D11.\n");
    }
}

void CD3D11::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);
    CD3D11::m_hwnd = NULL;
    g_pDX = nullptr;
}

void CD3D11::Initialize(HWND& hWnd)
{

}

void CD3D11::InitializeResources()
{
    CLoadingExt::LoadShp("Furina", "Furina.shp", &CMapDataExt::Palette_Shadow, 0);
    auto pData = CLoadingExt::GetImageDataFromMap("Furina");

    if (pData->IsValidImage(pData))
    {     
        g_pDX->LoadIndexTexture("Furina", CIsoViewExt::MakeImageDataView(pData));
    }
    for (int i = 0; i < CMapDataExt::TileDataCount; ++i)
    {
        auto& tileData = CMapDataExt::TileData[i];
        CTileTypeClass* currentTile = &tileData;
        for (int j = -1; j < (int)tileData.AltTypeCount; ++j)
        {
            if (j > -1)
                currentTile = &tileData.AltTypes[j];

            if (Palette* pal = CMapDataExt::TileSetPalettes[currentTile->TileSet])
            {
                for (int k = 0; k < currentTile->TileBlockCount; ++k)
                {
                    auto tileBlock = &currentTile->TileBlockDatas[k];
                    if (tileBlock && tileBlock->ImageData)
                    {
                        g_pDX->LoadTileTexture(tileBlock, CIsoViewExt::MakeImageDataView(tileBlock, pal));
                    }
                }
            }
        }
    }
}

// 在 DlgProc 中添加定时器或 WM_PAINT 调用 Render()
BOOL CALLBACK CD3D11::DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) {
    case WM_INITDIALOG:
        CD3D11::Initialize(hwnd);
        //SetTimer(hwnd, 1, 16, nullptr);  // ~60 FPS
        return TRUE;

    case WM_PAINT:
    {
  
        break;
    }
    case WM_MOUSEMOVE:
        //if (wParam == 1)
        {
            std::vector<DirectXCore::TextureResource*> textures;
            for (int i = 0; i < CMapDataExt::TileDataCount; ++i)
            {
                auto& tileData = CMapDataExt::TileData[i];
                CTileTypeClass* currentTile = &tileData;
                for (int j = -1; j < (int)tileData.AltTypeCount; ++j)
                {
                    if (j > -1)
                        currentTile = &tileData.AltTypes[j];

                    for (int k = 0; k < currentTile->TileBlockCount; ++k)
                    {
                        auto tileBlock = &currentTile->TileBlockDatas[k];
                        if (tileBlock && tileBlock->ImageData)
                        {
                            if (auto tex = g_pDX->GetTileTexture(tileBlock))
                            {
                                textures.push_back(tex);
                            }
                        }
                    }
                }
            }

            auto Furina = g_pDX->GetTexture("Furina");

            RECT rc;
            GetClientRect(hwnd, &rc);
            float scaleFactor = 2.0f;

            UINT width = (rc.right - rc.left) * scaleFactor;
            UINT height = (rc.bottom - rc.top) * scaleFactor;

            int displayX = 0;
            int displayY = 0;

            auto lighting = LightingStruct::GetCurrentLighting();
            float AmbientMult, RedMult, GreenMult, BlueMult;
            if (lighting == LightingStruct::NoLighting)
            {
                AmbientMult = 1.0f;
                RedMult = 1.0f;
                GreenMult = 1.0f;
                BlueMult = 1.0f;
            }
            else
            {
                AmbientMult = lighting.Ambient - lighting.Ground;
                RedMult = lighting.Red * AmbientMult;
                GreenMult = lighting.Green * AmbientMult;
                BlueMult = lighting.Blue * AmbientMult;
            }

            static int j = 0;
            j++;
            if (j == textures.size() - 1)
                j = 0;
            bool end = false;
            int i = 0;
            bool oddLine = true;
            for (i = j; i < textures.size(); ++i)
            {
                auto tex = textures[i];

                DrawParams params;
                params.SetPosition(displayX - tex->sourceView.FullWidth / 2, displayY - tex->sourceView.FullHeight / 2)
                    //.SetScale(1.0f, 1.0f)
                    //.SetOpacity(opacity)
                    .SetColorMul(RedMult, GreenMult, BlueMult);
                    //.SetColorMix(0.0f, 1.0f, 0.0f, 0.5f);

                g_pDX->DrawTexture(tex, params);
                if (displayX < width)
                {
                    displayX += 60;
                }
                else
                {
                    oddLine = !oddLine;
                    if (oddLine)
                    {
                        displayX = 0;
                    }
                    else
                    {
                        displayX = 30;
                    }
                    displayY += 15;
                }
                if (displayY >= height)
                    break;

                if (i == textures.size() - 1)
                {
                    i = 0;
                    end = true;
                }
            }
            if (!end)
                j = i;


            g_pDX->DrawTexture(Furina, 0, 0);
            g_pDX->DrawTexture(Furina, 400, 0);
            g_pDX->DrawTexture(Furina, 800, 0);

            g_pDX->SetGlobalTransform(1.0f / scaleFactor, 1.0f / scaleFactor,  1.0f / scaleFactor - 1.0f, 1.0f / scaleFactor - 1.0f);
            g_pDX->Render();  // 或者在单独线程/消息循环中调用
        }
        break;

    case WM_SIZE:
        g_pDX->OnResize(hwnd);
        // Resize swap chain & RTV（需重新创建 RTV）
        break;

    case WM_CLOSE:
        CD3D11::Close(hwnd);
        return TRUE;
    case 114514: // used for update
    {
        g_pDX->ClearTextures();
        InitializeResources();
        return TRUE;
    }
    }
    return FALSE;
}
