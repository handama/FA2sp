#include <DirectXMath.h>
#include "../../Ext/CLoading/Body.h"
#include "DirectXCore.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
using namespace Microsoft::WRL;
using namespace DirectX;

ComPtr<ID3D11Device>           DirectXCore::m_pDevice;
ComPtr<ID3D11DeviceContext>    DirectXCore::m_pContext;
ComPtr<IDXGISwapChain>         DirectXCore::m_pSwapChain;
ComPtr<ID3D11RenderTargetView> DirectXCore::m_pRTV;

ComPtr<ID3D11VertexShader>     DirectXCore::m_pVS;
ComPtr<ID3D11PixelShader>      DirectXCore::m_pPS;
ComPtr<ID3D11SamplerState>     DirectXCore::m_pSampler;
ComPtr<ID3D11BlendState>       DirectXCore::m_pBlendState;

ComPtr<ID3D11Texture2D>        DirectXCore::m_pTestTexture;
ComPtr<ID3D11ShaderResourceView> DirectXCore::m_pSRV;

static std::unique_ptr<ImageDataClassSafe> pData;

void DirectXCore::InitializeDX(HWND hwnd) {
    if (!CreateDeviceAndSwapChain(hwnd)) return;
    if (!CreateShaders()) return;
    CreateTestTexture();  // 这里替换为你的图像上传逻辑

    // 设置 viewport（窗口大小变化时需更新）
    RECT rc;
    GetClientRect(hwnd, &rc);
    D3D11_VIEWPORT vp = { 0, 0, (float)(rc.right - rc.left), (float)(rc.bottom - rc.top), 0.0f, 1.0f };
    m_pContext->RSSetViewports(1, &vp);
}

bool DirectXCore::CreateDeviceAndSwapChain(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Width = 0;   // 0 = 使用窗口大小
    scd.BufferDesc.Height = 0;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 60;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createFlags = 0;
#ifdef _DEBUG
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createFlags, featureLevels, 2,
        D3D11_SDK_VERSION, &scd,
        &m_pSwapChain, &m_pDevice, nullptr, &m_pContext);
    if (FAILED(hr)) return false;

    // 创建 RenderTargetView
    ComPtr<ID3D11Texture2D> pBackBuffer;
    m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer);
    m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_pRTV);

    return true;
}

bool DirectXCore::CreateShaders() {
    // 简单 Vertex Shader（用 SV_VertexID 生成全屏三角形/quad）
    const char* vsCode = R"(
        struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD; };
        VSOut main(uint id : SV_VertexID) {
            VSOut o;
            o.uv = float2((id << 1) & 2, id & 2);
            o.pos = float4(o.uv * float2(2,-2) + float2(-1,1), 0, 1);
            return o;
        }
    )";

    const char* psCode = R"(
        Texture2D tex : register(t0);
        SamplerState sam : register(s0);
        float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD) : SV_Target {
            return tex.Sample(sam, uv);
        }
    )";

    ComPtr<ID3DBlob> vsBlob, psBlob, error;
    D3DCompile(vsCode, strlen(vsCode), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &error);
    D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &error);

    m_pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_pVS);
    m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pPS);

    // Sampler（线性过滤，适合图像）
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    m_pDevice->CreateSamplerState(&sampDesc, &m_pSampler);

    // Alpha 混合（如果你有 opacity）
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    m_pDevice->CreateBlendState(&blendDesc, &m_pBlendState);

    return true;
}

// 示例：从 ImageDataClassSafe 创建纹理（简化版，假设无 per-row valid range）
bool DirectXCore::CreateTestTexture() {

    CLoadingExt::GetExtension()->LoadObjects("GATECH");
    auto imageName = CLoadingExt::GetBuildingImageName("GATECH", 0, 0);
    auto& clips = CLoadingExt::GetBuildingClipImageDataFromMap(imageName);
    pData = CLoadingExt::BindClippedImages(clips);

    ImageDataClassSafe* pImg = pData.get();

    if (!ImageDataClassSafe::IsValidImage(pImg)) return false;

    int width = pImg->FullWidth;
    int height = pImg->FullHeight;

    std::vector<uint32_t> rgbaData(width * height);  // BGRA 或 RGBA

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned char index = pImg->pImageBuffer[y * width + x];
            unsigned char opacity = pImg->pOpacity ? pImg->pOpacity[y * width + x] : 255;

            BGRStruct color = pImg->pPalette->Data[index];
            uint32_t rgba = (opacity << 24) | (color.R << 16) | (color.G << 8) | color.B;  // 注意字节序

            // 处理 ValidRangeData（跳过无效像素，设为透明）
            // if (在无效范围内) rgba = 0;

            rgbaData[y * width + x] = rgba;
        }
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = rgbaData.data();
    initData.SysMemPitch = width * 4;

    m_pDevice->CreateTexture2D(&desc, &initData, &m_pTestTexture);
    m_pDevice->CreateShaderResourceView(m_pTestTexture.Get(), nullptr, &m_pSRV);

    return true;
}

void DirectXCore::Render() {
    if (!m_pContext || !m_pRTV) return;

    // 清屏（或不清除，保留之前内容）
    float clearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
    m_pContext->ClearRenderTargetView(m_pRTV.Get(), clearColor);

    m_pContext->OMSetRenderTargets(1, m_pRTV.GetAddressOf(), nullptr);
    m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);

    m_pContext->VSSetShader(m_pVS.Get(), nullptr, 0);
    m_pContext->PSSetShader(m_pPS.Get(), nullptr, 0);
    m_pContext->PSSetShaderResources(0, 1, m_pSRV.GetAddressOf());
    m_pContext->PSSetSamplers(0, 1, m_pSampler.GetAddressOf());

    // 绘制全屏 quad（4 vertices -> triangle strip 或用 3 vertices triangle）
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_pContext->Draw(4, 0);  // 用 SV_VertexID 生成

    m_pSwapChain->Present(1, 0);  // VSync
}

void DirectXCore::Cleanup() {
    // ComPtr 会自动释放
    m_pSRV.Reset();
    m_pTestTexture.Reset();
    // ... 其他资源
}

