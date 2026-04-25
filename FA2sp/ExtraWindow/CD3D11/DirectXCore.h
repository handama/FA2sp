#pragma once
#include <windef.h>
#include <wrl/client.h> 
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <vector>
#include <memory>

class ImageDataClassSafe;

// 绘制参数封装
class DrawParams
{
public:
    float x = 0.0f;
    float y = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float opacity = 1.0f;
    float redMult = 1.0f;
    float greenMult = 1.0f;
    float blueMult = 1.0f;

    DrawParams& SetPosition(float _x, float _y) { x = _x; y = _y; return *this; }
    DrawParams& SetScale(float sx, float sy) { scaleX = sx; scaleY = sy; return *this; }
    DrawParams& SetOpacity(float o) { opacity = o; return *this; }
    DrawParams& SetColorMul(float r, float g, float b) { redMult = r; greenMult = g; blueMult = b; return *this; }
};

class DirectXCore
{
public:
    static bool InitializeDX(HWND hwnd);
    static void Cleanup();
    static void OnResize(HWND hwnd);

    using TextureHandle = void*;
    static TextureHandle LoadTexture(ImageDataClassSafe* pImg);

    // 推荐：使用 DrawParams 类传入所有参数
    static void DrawTexture(TextureHandle handle, const DrawParams& params);
    // 简化版：只传位置，其他使用默认
    static void DrawTexture(TextureHandle handle, float x, float y) {
        DrawParams p; p.x = x; p.y = y; DrawTexture(handle, p);
    }

    static void Render();

private:
    struct TextureResource {
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
        int width = 0, height = 0;
    };

    struct DrawCommand {
        TextureResource* texRes = nullptr;
        DrawParams params;
    };

    static bool CreateDeviceAndSwapChain(HWND hwnd);
    static bool CreateShadersAndInputLayout();
    static bool CreateQuadVertexBuffer();
    static void UpdateViewportAndRTV(HWND hwnd);

    static Microsoft::WRL::ComPtr<ID3D11Device>           m_pDevice;
    static Microsoft::WRL::ComPtr<ID3D11DeviceContext>    m_pContext;
    static Microsoft::WRL::ComPtr<IDXGISwapChain>         m_pSwapChain;
    static Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pRTV;

    static Microsoft::WRL::ComPtr<ID3D11VertexShader>     m_pVS;
    static Microsoft::WRL::ComPtr<ID3D11PixelShader>      m_pPS;
    static Microsoft::WRL::ComPtr<ID3D11InputLayout>      m_pInputLayout;
    static Microsoft::WRL::ComPtr<ID3D11SamplerState>     m_pSampler;
    static Microsoft::WRL::ComPtr<ID3D11BlendState>       m_pBlendState;
    static Microsoft::WRL::ComPtr<ID3D11Buffer>           m_pQuadVB;
    static Microsoft::WRL::ComPtr<ID3D11Buffer>           m_pConstantBuffer;

    static std::vector<std::unique_ptr<TextureResource>> m_textures;
    static std::vector<DrawCommand> m_drawCommands;
    static int m_clientWidth, m_clientHeight;
};