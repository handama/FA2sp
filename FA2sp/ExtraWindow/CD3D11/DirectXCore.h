#pragma once
#include <windef.h>
#include <wrl/client.h> 
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
class DirectXCore
{
public:
    static void InitializeDX(HWND hwnd);  // 新增：DX 初始化
    static void Render();                 // 新增：每帧渲染
    static void Cleanup();

private:
    static bool CreateDeviceAndSwapChain(HWND hwnd);
    static bool CreateShaders();
    static bool CreateTestTexture();  // 示例：从你的 ImageData 创建纹理


    // DX11 对象
    static Microsoft::WRL::ComPtr<ID3D11Device>           m_pDevice;
    static Microsoft::WRL::ComPtr<ID3D11DeviceContext>    m_pContext;
    static Microsoft::WRL::ComPtr<IDXGISwapChain>         m_pSwapChain;
    static Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pRTV;

    static Microsoft::WRL::ComPtr<ID3D11VertexShader>     m_pVS;
    static Microsoft::WRL::ComPtr<ID3D11PixelShader>      m_pPS;
    static Microsoft::WRL::ComPtr<ID3D11SamplerState>     m_pSampler;
    static Microsoft::WRL::ComPtr<ID3D11BlendState>       m_pBlendState;
 
    static Microsoft::WRL::ComPtr<ID3D11Texture2D>        m_pTestTexture;
    static Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pSRV;
};