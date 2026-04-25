#include <DirectXMath.h>
#include <vector>
#include "../../Ext/CLoading/Body.h"
#include "DirectXCore.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using namespace Microsoft::WRL;
using namespace DirectX;

// 静态成员定义
ComPtr<ID3D11Device>           DirectXCore::m_pDevice;
ComPtr<ID3D11DeviceContext>    DirectXCore::m_pContext;
ComPtr<IDXGISwapChain>         DirectXCore::m_pSwapChain;
ComPtr<ID3D11RenderTargetView> DirectXCore::m_pRTV;

ComPtr<ID3D11VertexShader>     DirectXCore::m_pVS;
ComPtr<ID3D11PixelShader>      DirectXCore::m_pPS;
ComPtr<ID3D11InputLayout>      DirectXCore::m_pInputLayout;
ComPtr<ID3D11SamplerState>     DirectXCore::m_pSampler;
ComPtr<ID3D11BlendState>       DirectXCore::m_pBlendState;
ComPtr<ID3D11Buffer>           DirectXCore::m_pQuadVB;
ComPtr<ID3D11Buffer>           DirectXCore::m_pConstantBuffer;

std::vector<std::unique_ptr<DirectXCore::TextureResource>> DirectXCore::m_textures;
std::vector<DirectXCore::DrawCommand> DirectXCore::m_drawCommands;
int DirectXCore::m_clientWidth = 0;
int DirectXCore::m_clientHeight = 0;

struct QuadVertex { XMFLOAT3 pos; XMFLOAT2 uv; };
struct CBPerObject { XMMATRIX world; XMFLOAT4 color; };

// ------------------------------------------------------------
bool DirectXCore::InitializeDX(HWND hwnd) {
    if (!CreateDeviceAndSwapChain(hwnd)) return false;
    if (!CreateShadersAndInputLayout()) return false;
    if (!CreateQuadVertexBuffer()) return false;

    RECT rc;
    GetClientRect(hwnd, &rc);
    m_clientWidth = rc.right - rc.left;
    m_clientHeight = rc.bottom - rc.top;
    UpdateViewportAndRTV(hwnd);
    return true;
}

void DirectXCore::Cleanup() {
    m_drawCommands.clear();
    m_textures.clear();
    m_pQuadVB.Reset();
    m_pConstantBuffer.Reset();
    m_pInputLayout.Reset();
    m_pVS.Reset();
    m_pPS.Reset();
    m_pSampler.Reset();
    m_pBlendState.Reset();
    m_pRTV.Reset();
    m_pSwapChain.Reset();
    m_pContext.Reset();
    m_pDevice.Reset();
    m_clientWidth = m_clientHeight = 0;
}

void DirectXCore::OnResize(HWND hwnd) {
    if (!m_pSwapChain) return;
    m_pRTV.Reset();

    RECT rc;
    GetClientRect(hwnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;
    HRESULT hr = m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (SUCCEEDED(hr)) {
        ComPtr<ID3D11Texture2D> pBackBuffer;
        m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer);
        m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_pRTV);
    }
    m_clientWidth = width;
    m_clientHeight = height;
    UpdateViewportAndRTV(hwnd);
}

void DirectXCore::UpdateViewportAndRTV(HWND hwnd) {
    if (!m_pContext) return;
    D3D11_VIEWPORT vp = { 0, 0, (float)m_clientWidth, (float)m_clientHeight, 0.0f, 1.0f };
    m_pContext->RSSetViewports(1, &vp);
}

bool DirectXCore::CreateDeviceAndSwapChain(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 1;
    scd.BufferDesc.Width = 0;
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

    ComPtr<ID3D11Texture2D> pBackBuffer;
    hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer);
    if (FAILED(hr)) return false;
    hr = m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_pRTV);
    if (FAILED(hr)) return false;
    return true;
}

bool DirectXCore::CreateShadersAndInputLayout() {
    const char* vsCode = R"(
        cbuffer CBPerObject : register(b0) {
            float4x4 g_World;
            float4   g_Color;
        };
        struct VSInput {
            float3 pos : POSITION0;
            float2 tex : TEXCOORD0;
        };
        struct VSOutput {
            float4 pos : SV_POSITION;
            float2 tex : TEXCOORD0;
            float4 color : COLOR0;
        };
        VSOutput main(VSInput input) {
            VSOutput output;
            output.pos = mul(float4(input.pos, 1.0f), g_World);
            output.tex = input.tex;
            output.color = g_Color;
            return output;
        }
    )";

    const char* psCode = R"(
        Texture2D tex : register(t0);
        SamplerState samp : register(s0);
        float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0, float4 color : COLOR0) : SV_Target {
            return tex.Sample(samp, uv) * color;
        }
    )";

    ComPtr<ID3DBlob> vsBlob, psBlob, error;
    HRESULT hr = D3DCompile(vsCode, strlen(vsCode), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &error);
    if (FAILED(hr)) { if (error) OutputDebugStringA((char*)error->GetBufferPointer()); return false; }
    hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &error);
    if (FAILED(hr)) { if (error) OutputDebugStringA((char*)error->GetBufferPointer()); return false; }

    hr = m_pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_pVS);
    if (FAILED(hr)) return false;
    hr = m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pPS);
    if (FAILED(hr)) return false;

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    hr = m_pDevice->CreateInputLayout(layoutDesc, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_pInputLayout);
    if (FAILED(hr)) return false;

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    m_pDevice->CreateSamplerState(&sampDesc, &m_pSampler);

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

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(CBPerObject);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    m_pDevice->CreateBuffer(&cbDesc, nullptr, &m_pConstantBuffer);
    return true;
}

bool DirectXCore::CreateQuadVertexBuffer() {
    QuadVertex vertices[] = {
        { XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-0.5f,  0.5f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(0.5f, -0.5f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(0.5f,  0.5f, 0.0f), XMFLOAT2(1.0f, 0.0f) }
    };
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA initData = { vertices };
    HRESULT hr = m_pDevice->CreateBuffer(&bd, &initData, &m_pQuadVB);
    return SUCCEEDED(hr);
}

DirectXCore::TextureHandle DirectXCore::LoadTexture(ImageDataClassSafe* pImg) {
    if (!m_pDevice) return nullptr;
    if (!ImageDataClassSafe::IsValidImage(pImg) || pImg->FullWidth == 0 || pImg->FullHeight == 0)
        return nullptr;

    auto texRes = std::make_unique<TextureResource>();
    texRes->width = pImg->FullWidth;
    texRes->height = pImg->FullHeight;
    int w = texRes->width, h = texRes->height;

    std::vector<uint32_t> rgbaData(w * h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            unsigned char idx = pImg->pImageBuffer[y * w + x];
            unsigned char opacity = idx == 0 ? 0 : (pImg->pOpacity ? pImg->pOpacity[y * w + x] : 255);
            BGRStruct color = pImg->pPalette->Data[idx];
            uint32_t rgba = (color.R << 0) | (color.G << 8) | (color.B << 16) | (opacity << 24);
            rgbaData[y * w + x] = rgba;
        }
    }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = rgbaData.data();
    initData.SysMemPitch = w * 4;

    HRESULT hr = m_pDevice->CreateTexture2D(&desc, &initData, &texRes->texture);
    if (FAILED(hr)) return nullptr;
    hr = m_pDevice->CreateShaderResourceView(texRes->texture.Get(), nullptr, &texRes->srv);
    if (FAILED(hr)) return nullptr;

    m_textures.push_back(std::move(texRes));
    return m_textures.back().get();
}

void DirectXCore::DrawTexture(TextureHandle handle, const DrawParams& params) {
    if (!handle) return;
    TextureResource* tex = reinterpret_cast<TextureResource*>(handle);
    m_drawCommands.push_back({ tex, params });
}

void DirectXCore::Render() {
    if (!m_pContext || !m_pRTV || m_drawCommands.empty()) return;

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pContext->ClearRenderTargetView(m_pRTV.Get(), clearColor);

    m_pContext->OMSetRenderTargets(1, m_pRTV.GetAddressOf(), nullptr);
    m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
    m_pContext->VSSetShader(m_pVS.Get(), nullptr, 0);
    m_pContext->PSSetShader(m_pPS.Get(), nullptr, 0);
    m_pContext->PSSetSamplers(0, 1, m_pSampler.GetAddressOf());
    m_pContext->IASetInputLayout(m_pInputLayout.Get());

    UINT stride = sizeof(QuadVertex);
    UINT offset = 0;
    m_pContext->IASetVertexBuffers(0, 1, m_pQuadVB.GetAddressOf(), &stride, &offset);
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    for (const auto& cmd : m_drawCommands) {
        TextureResource* tex = cmd.texRes;
        if (!tex) continue;

        const auto& p = cmd.params;
        float w_px = tex->width * p.scaleX;
        float h_px = tex->height * p.scaleY;
        float centerX_px = p.x + w_px * 0.5f;
        float centerY_px = p.y + h_px * 0.5f;

        float ndc_width = (w_px / m_clientWidth) * 2.0f;
        float ndc_height = (h_px / m_clientHeight) * 2.0f;
        float ndc_centerX = (centerX_px / m_clientWidth) * 2.0f - 1.0f;
        float ndc_centerY = 1.0f - (centerY_px / m_clientHeight) * 2.0f;

        XMMATRIX S = XMMatrixScaling(ndc_width, ndc_height, 1.0f);
        XMMATRIX T = XMMatrixTranslation(ndc_centerX, ndc_centerY, 0.0f);
        XMMATRIX world = S * T;

        CBPerObject cb;
        cb.world = XMMatrixTranspose(world);
        cb.color = XMFLOAT4(p.redMult, p.greenMult, p.blueMult, p.opacity);

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_pContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            memcpy(mapped.pData, &cb, sizeof(cb));
            m_pContext->Unmap(m_pConstantBuffer.Get(), 0);
        }

        m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
        m_pContext->PSSetShaderResources(0, 1, tex->srv.GetAddressOf());
        m_pContext->Draw(4, 0);
    }

    m_pSwapChain->Present(1, 0);
    m_drawCommands.clear();
}