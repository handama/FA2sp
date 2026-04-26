#pragma once
#include <windef.h>
#include <wrl/client.h> 
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <vector>
#include <memory>
#include <string>
#include <map>
#include "../../Ext/CIsoView/Body.h" 

// 绘制参数（保持不变）
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
    float mixR = 0.0f;
    float mixG = 0.0f;
    float mixB = 0.0f;
    float mixFactor = 0.0f;

    DrawParams& SetPosition(float _x, float _y) { x = _x; y = _y; return *this; }
    DrawParams& SetScale(float sx, float sy) { scaleX = sx; scaleY = sy; return *this; }
    DrawParams& SetOpacity(float o) { opacity = o; return *this; }
    DrawParams& SetColorMul(float r, float g, float b) { redMult = r; greenMult = g; blueMult = b; return *this; }
    DrawParams& SetColorMix(float r, float g, float b, float factor) {
        mixR = r; mixG = g; mixB = b; mixFactor = factor;
        return *this;
    }
};

class DirectXCore
{
public:
    DirectXCore();
    ~DirectXCore();

    bool Initialize(HWND hwnd);
    void Cleanup();
    void ClearTextures();               // 清空所有纹理
    void OnResize(HWND hwnd);

    // 纹理资源结构体（公开，以便外部直接访问其成员）
    struct TextureResource {
        ImageDataView sourceView;                       // 存储原始视图（浅拷贝）
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    };

    // 加载纹理（必须提供名称）
    TextureResource* LoadTexture(const FString& name, const ImageDataView& view);
    TextureResource* LoadTileTexture(CTileBlockClass* tileBlock, const ImageDataView& view);

    // 通过名称获取纹理资源（包含所有元数据）
    TextureResource* GetTexture(const FString& name) const;
    TextureResource* GetTileTexture(CTileBlockClass* tileBlock) const;

    // 绘制（原有接口，使用 TextureHandle 兼容旧代码，但推荐直接传 TextureResource*）
    void DrawTexture(TextureResource* tex, const DrawParams& params);
    void DrawTexture(TextureResource* tex, float x, float y) {
        DrawParams p; p.x = x; p.y = y; DrawTexture(tex, p);
    }

    void SetGlobalTransform(float scaleX, float scaleY, float offsetX, float offsetY);
    void ResetGlobalTransform() { SetGlobalTransform(1.0f, 1.0f, 0.0f, 0.0f); }
    void Render();

private:
    struct DrawCommand {
        TextureResource* texRes = nullptr;
        DrawParams params;
    };

    bool CreateDeviceAndSwapChain(HWND hwnd);
    bool CreateShadersAndInputLayout();
    bool CreateQuadVertexBuffer();
    void UpdateViewportAndRTV(HWND hwnd);

    Microsoft::WRL::ComPtr<ID3D11Device>           m_pDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>    m_pContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain>         m_pSwapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pRTV;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>     m_pVS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>      m_pPS;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>      m_pInputLayout;
    Microsoft::WRL::ComPtr<ID3D11SamplerState>     m_pSampler;
    Microsoft::WRL::ComPtr<ID3D11BlendState>       m_pBlendState;
    Microsoft::WRL::ComPtr<ID3D11Buffer>           m_pQuadVB;
    Microsoft::WRL::ComPtr<ID3D11Buffer>           m_pConstantBuffer;

    FMap<std::unique_ptr<TextureResource>> m_textureMap;
    std::map<CTileBlockClass*, std::unique_ptr<TextureResource>> m_tileTextureMap;
    std::vector<DrawCommand> m_drawCommands;
    int m_clientWidth = 0;
    int m_clientHeight = 0;
    float m_globalScaleX = 1.0f;
    float m_globalScaleY = 1.0f;
    float m_globalOffsetX = 0.0f;
    float m_globalOffsetY = 0.0f;
    bool m_bInitialized = false;
};