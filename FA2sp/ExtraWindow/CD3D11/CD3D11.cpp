#include "CD3D11.h"
#include "../../Ext/CLoading/Body.h"
#include "DirectXCore.h"
#include "../../Miscs/Palettes.h"

HWND CD3D11::m_hwnd;

static DirectXCore::TextureHandle tex;
std::vector<DirectXCore::TextureHandle> CD3D11::textures;

// CD3D11.cpp
void CD3D11::Create(HWND hParent) {
    m_hwnd = CreateDialog(static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(340),
        hParent,
        CD3D11::DlgProc);
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
        DirectXCore::InitializeDX(m_hwnd);  // 在创建后立即初始化 DX

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
}

void CD3D11::Initialize(HWND& hWnd)
{

}

void CD3D11::InitializeResources()
{
    for (auto& [_, obj] : Variables::RulesMap.GetSection("VehicleTypes"))
    {
        CLoadingExt::GetExtension()->LoadObjects(obj);
        //auto imageName = CLoadingExt::GetBuildingImageName("NATECH", 0, 0);
        //auto& clips = CLoadingExt::GetBuildingClipImageDataFromMap(imageName);
        //auto pData = CLoadingExt::BindClippedImages(clips);

        auto imageName = CLoadingExt::GetImageName(obj, 0);
        auto pData = CLoadingExt::GetImageDataFromMap(imageName);
        if (pData->IsValidImage(pData))
        {
            CD3D11::textures.push_back(DirectXCore::LoadTexture(pData));
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

    case WM_MOUSEMOVE:
        //if (wParam == 1)
        {

            //static float opacity = 1.0f;
            //opacity -= 0.02f;
            //if (opacity <= 0.0f)
            //    opacity = 1.0f;
            RECT rc;
            GetClientRect(hwnd, &rc);
            UINT width = rc.right - rc.left;
            UINT height = rc.bottom - rc.top;

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
            for (int i = j; i < textures.size(); ++i)
            {
                auto tex = textures[i];

                DrawParams params;
                params.SetPosition(displayX, displayY)
                    //.SetScale(1.0f, 1.0f)
                    //.SetOpacity(opacity)
                    .SetColorMul(RedMult, GreenMult, BlueMult);  // 红色调
                DirectXCore::DrawTexture(tex, params);
                if (displayX < width)
                {
                    displayX += 10;
                }
                else
                {
                    displayX = 0;
                    displayY += 10;
                }
                if (displayY >= height)
                    break;

                if (i == textures.size() - 1)
                    i = 0;
            }

            DirectXCore::Render();  // 或者在单独线程/消息循环中调用
        }
        break;

    case WM_SIZE:
        DirectXCore::OnResize(hwnd);
        // Resize swap chain & RTV（需重新创建 RTV）
        break;

    case WM_CLOSE:
        DirectXCore::Cleanup();
        CD3D11::Close(hwnd);
        return TRUE;
    }
    return FALSE;
}
