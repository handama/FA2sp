#include <DirectXMath.h>
#include <vector>
#include <cstring>
#include <string>
#include "../CLoading/Body.h" 
#include "Body.h" 
#include "DirectXCore.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using namespace Microsoft::WRL;
using namespace DirectX;
std::unique_ptr<DirectXCore> CIsoViewExt::g_pDX;

struct QuadVertex { XMFLOAT3 pos; XMFLOAT2 uv; };
struct CBPerObject {
    XMMATRIX world;
    XMFLOAT4 colorMul;
    XMFLOAT4 mixColor;
    float mixFactor;
    float padding[3];
};

DirectXCore::DirectXCore() = default;
DirectXCore::~DirectXCore() { Cleanup(); }

bool DirectXCore::Initialize(HWND hwnd) {
    Cleanup();

    if (!IsWindow(hwnd)) {
        Logger::Info("Initialize: Invalid HWND\n");
        return false;
    }

    if (!CreateDeviceAndSwapChain(hwnd)) {
        Logger::Info("CreateDeviceAndSwapChain failed\n");
        return false;
    }
    if (!CreateShadersAndInputLayout()) {
        Logger::Info("CreateShadersAndInputLayout failed\n");
        return false;
    }
    if (!CreateEffectShaders()) {
        Logger::Info("CreateEffectShaders failed\n");
        return false;
    }
    if (!CreateCompositeShaders()) {
        Logger::Info("CreateCompositeShaders failed\n");
        return false;
    }
    if (!CreateQuadVertexBuffer()) {
        Logger::Info("CreateQuadVertexBuffer failed\n");
        return false;
    }

    RECT rc;
    GetClientRect(hwnd, &rc);
    m_clientWidth = rc.right - rc.left;
    m_clientHeight = rc.bottom - rc.top;
    UpdateViewportAndRTV(hwnd);

    UINT vw = (UINT)(m_clientWidth * m_renderScale);
    UINT vh = (UINT)(m_clientHeight * m_renderScale);
    if (vw == 0) vw = 1;
    if (vh == 0) vh = 1;

    if (!CreateOffscreenResources(vw, vh)) {
        Logger::Info("CreateOffscreenResources failed\n");
        return false;
    }
    if (!CreateFinalShaders()) {
        Logger::Info("CreateFinalShaders failed\n");
        return false;
    }

    EnsureFactorTexture(vw, vh);
    EnsureScreenCopyTexture(vw, vh);

    D3D11_BLEND_DESC blendMul = {};
    blendMul.RenderTarget[0].BlendEnable = TRUE;
    blendMul.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
    blendMul.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
    blendMul.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendMul.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    blendMul.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blendMul.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendMul.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    m_pDevice->CreateBlendState(&blendMul, &m_pMulBlendState);

    m_bInitialized = true;
    Logger::Info("Initialize succeeded\n");
    return true;
}

bool DirectXCore::IsInitialized()
{
    return m_bInitialized;
}

void DirectXCore::ClearTextures() {
    m_textureMap.clear();
    m_tileTextureMap.clear();
}

void DirectXCore::Cleanup() {
    m_drawCommands.clear();
    m_textureMap.clear();
    m_tileTextureMap.clear();

    if (m_pContext) {
        m_pContext->ClearState();
        m_pContext->Flush();
    }

    m_pQuadVB.Reset();
    m_pConstantBuffer.Reset();
    m_pInputLayout.Reset();
    m_pVS.Reset();
    m_pPS.Reset();
    m_pEffectVS.Reset();
    m_pEffectPS.Reset();
    m_pCompositeVS.Reset();
    m_pCompositePS.Reset();
    m_pSamplerLinear.Reset();
    m_pSamplerPoint.Reset();
    m_pBlendState.Reset();
    m_pMulBlendState.Reset();
    m_pRTV.Reset();
    m_pSwapChain.Reset();
    m_pContext.Reset();
    m_pDevice.Reset();
    m_pFactorTexture.Reset();
    m_pFactorRTV.Reset();
    m_pFactorSRV.Reset();
    m_pScreenCopy.Reset();
    m_pScreenCopySRV.Reset();
    m_pFullscreenQuadVB.Reset();

    m_OffscreenTex.Reset();
    m_OffscreenRTV.Reset();
    m_OffscreenSRV.Reset();
    m_pFinalVS.Reset();
    m_pFinalPS.Reset();
    m_pFinalConstantBuffer.Reset();

    m_clientWidth = m_clientHeight = 0;
    m_globalScaleX = m_globalScaleY = 1.0f;
    m_globalOffsetX = m_globalOffsetY = 0.0f;
    m_renderScale = 1.0f;
    m_bInitialized = false;
}

void DirectXCore::OnResize(HWND hwnd) {
    if (!m_pSwapChain) return;

    m_pRTV.Reset();
    m_OffscreenRTV.Reset(); m_OffscreenSRV.Reset(); m_OffscreenTex.Reset();
    m_pFactorTexture.Reset(); m_pFactorRTV.Reset(); m_pFactorSRV.Reset();
    m_pScreenCopy.Reset(); m_pScreenCopySRV.Reset();

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

    UINT vw = (UINT)(width * m_renderScale);
    UINT vh = (UINT)(height * m_renderScale);
    if (vw == 0) vw = 1;
    if (vh == 0) vh = 1;

    CreateOffscreenResources(vw, vh);
    EnsureFactorTexture(vw, vh);
    EnsureScreenCopyTexture(vw, vh);
}

void DirectXCore::UpdateViewportAndRTV(HWND hwnd) {
    if (!m_pContext) return;
    D3D11_VIEWPORT vp = { 0, 0, (float)m_clientWidth, (float)m_clientHeight, 0.0f, 1.0f };
    m_pContext->RSSetViewports(1, &vp);
}

void DirectXCore::SetGlobalTransform(float scaleX, float scaleY, float offsetX, float offsetY) {
    m_globalScaleX = scaleX;
    m_globalScaleY = scaleY;
    m_globalOffsetX = offsetX;
    m_globalOffsetY = offsetY;
}

void DirectXCore::SetZoomOut(float scaleFactor) {
    if (scaleFactor < 0.01f) scaleFactor = 0.01f;
    if (m_renderScale == scaleFactor) return;
    m_renderScale = scaleFactor;

    UINT vw = (UINT)(m_clientWidth * m_renderScale);
    UINT vh = (UINT)(m_clientHeight * m_renderScale);
    if (vw == 0) vw = 1;
    if (vh == 0) vh = 1;

    m_OffscreenTex.Reset(); m_OffscreenRTV.Reset(); m_OffscreenSRV.Reset();
    m_pFactorTexture.Reset(); m_pFactorRTV.Reset(); m_pFactorSRV.Reset();
    m_pScreenCopy.Reset(); m_pScreenCopySRV.Reset();

    CreateOffscreenResources(vw, vh);
    EnsureFactorTexture(vw, vh);
    EnsureScreenCopyTexture(vw, vh);
}

bool DirectXCore::CreateDeviceAndSwapChain(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 2;
    scd.BufferDesc.Width = 0;
    scd.BufferDesc.Height = 0;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.RefreshRate.Numerator = 0;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hwnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    UINT createFlags = 0;
#ifdef _DEBUG
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createFlags, featureLevels, 2,
        D3D11_SDK_VERSION, &scd,
        &m_pSwapChain, &m_pDevice, nullptr, &m_pContext);
    if (FAILED(hr)) {
        Logger::Info("D3D11CreateDeviceAndSwapChain failed\n");
        return false;
    }

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
            float4   g_ColorMul;
            float4   g_MixColor;
            float    g_MixFactor;
        };
        struct VSInput {
            float3 pos : POSITION0;
            float2 tex : TEXCOORD0;
        };
        struct VSOutput {
            float4 pos : SV_POSITION;
            float2 uv  : TEXCOORD0;
            float4 colorMul : COLOR0;
            float4 mixColor : COLOR1;
            float  mixFactor : TEXCOORD1;
        };
        VSOutput main(VSInput input) {
            VSOutput output;
            output.pos = mul(float4(input.pos, 1.0f), g_World);
            output.uv = input.tex;
            output.colorMul = g_ColorMul;
            output.mixColor = g_MixColor;
            output.mixFactor = g_MixFactor;
            return output;
        }
    )";

    const char* psCode = R"(
        Texture2D tex : register(t0);
        SamplerState samp : register(s0);
        float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0,
                    float4 colorMul : COLOR0, float4 mixColor : COLOR1,
                    float mixFactor : TEXCOORD1) : SV_Target {
            float4 texColor = tex.Sample(samp, uv);
            float3 multRGB = texColor * colorMul.rgb;
            float3 finalRGB = lerp(multRGB.rgb, mixColor.rgb, mixFactor);
            float finalAlpha = texColor.a * colorMul.a;
            return float4(finalRGB, finalAlpha);
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
    m_pDevice->CreateSamplerState(&sampDesc, &m_pSamplerLinear);
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    m_pDevice->CreateSamplerState(&sampDesc, &m_pSamplerPoint);

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

bool DirectXCore::CreateEffectShaders() {
    const char* vsCode = R"(
        cbuffer CBPerObject : register(b0) {
            float4x4 g_World;
        };
        struct VSInput {
            float3 pos : POSITION0;
            float2 tex : TEXCOORD0;
        };
        struct VSOutput {
            float4 pos : SV_POSITION;
            float2 uv  : TEXCOORD0;
        };
        VSOutput main(VSInput input) {
            VSOutput output;
            output.pos = mul(float4(input.pos, 1.0f), g_World);
            output.uv = input.tex;
            return output;
        }
    )";
    const char* psCode = R"(
        Texture2D indexTex : register(t0);
        SamplerState samp : register(s0);
        float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_Target {
            float f = indexTex.Sample(samp, uv).r;
            float index = f * 255.0f;
            float factor = index / 128.0f;
            return float4(factor, factor, factor, 1.0f);
        }
    )";
    ComPtr<ID3DBlob> vsBlob, psBlob, error;
    HRESULT hr = D3DCompile(vsCode, strlen(vsCode), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &error);
    if (FAILED(hr)) { if (error) OutputDebugStringA((char*)error->GetBufferPointer()); return false; }
    hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &error);
    if (FAILED(hr)) { if (error) OutputDebugStringA((char*)error->GetBufferPointer()); return false; }
    hr = m_pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_pEffectVS);
    if (FAILED(hr)) return false;
    hr = m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pEffectPS);
    if (FAILED(hr)) return false;
    return true;
}

bool DirectXCore::CreateCompositeShaders() {
    const char* vsCode = R"(
        struct VSInput {
            float3 pos : POSITION;
            float2 uv  : TEXCOORD0;
        };
        struct VSOutput {
            float4 pos : SV_POSITION;
            float2 uv  : TEXCOORD0;
        };
        VSOutput main(VSInput input) {
            VSOutput output;
            output.pos = float4(input.pos, 1.0f);
            output.uv = input.uv;
            return output;
        }
    )";
    const char* psCode = R"(
        Texture2D screenTex : register(t0);
        Texture2D factorTex  : register(t1);
        SamplerState samp : register(s0);
        float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_Target {
            float4 orig = screenTex.Sample(samp, uv);
            float factor = factorTex.Sample(samp, uv).r;
            return float4(orig.rgb * factor, orig.a);
        }
    )";
    ComPtr<ID3DBlob> vsBlob, psBlob, error;
    HRESULT hr = D3DCompile(vsCode, strlen(vsCode), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &error);
    if (FAILED(hr)) { if (error) OutputDebugStringA((char*)error->GetBufferPointer()); return false; }
    hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &error);
    if (FAILED(hr)) { if (error) OutputDebugStringA((char*)error->GetBufferPointer()); return false; }
    hr = m_pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_pCompositeVS);
    if (FAILED(hr)) return false;
    hr = m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pCompositePS);
    if (FAILED(hr)) return false;
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
    QuadVertex fsVertices[] = {
        { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
        { XMFLOAT3(1.0f,  1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
    };
    D3D11_BUFFER_DESC fsbd = {};
    fsbd.Usage = D3D11_USAGE_IMMUTABLE;
    fsbd.ByteWidth = sizeof(fsVertices);
    fsbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA fsInitData = { fsVertices };
    hr = m_pDevice->CreateBuffer(&fsbd, &fsInitData, &m_pFullscreenQuadVB);

    return SUCCEEDED(hr);
}

void DirectXCore::EnsureFactorTexture(UINT width, UINT height) {
    if (m_pFactorTexture && width == m_clientWidth && height == m_clientHeight) return;
    m_pFactorTexture.Reset(); m_pFactorRTV.Reset(); m_pFactorSRV.Reset();
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R16_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    HRESULT hr = m_pDevice->CreateTexture2D(&desc, nullptr, &m_pFactorTexture);
    if (FAILED(hr)) return;
    hr = m_pDevice->CreateRenderTargetView(m_pFactorTexture.Get(), nullptr, &m_pFactorRTV);
    if (FAILED(hr)) return;
    hr = m_pDevice->CreateShaderResourceView(m_pFactorTexture.Get(), nullptr, &m_pFactorSRV);
}

void DirectXCore::EnsureScreenCopyTexture(UINT width, UINT height) {
    if (m_pScreenCopy && width == m_clientWidth && height == m_clientHeight) return;
    m_pScreenCopy.Reset(); m_pScreenCopySRV.Reset();
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    HRESULT hr = m_pDevice->CreateTexture2D(&desc, nullptr, &m_pScreenCopy);
    if (FAILED(hr)) return;
    hr = m_pDevice->CreateShaderResourceView(m_pScreenCopy.Get(), nullptr, &m_pScreenCopySRV);
}

void DirectXCore::CopyScreenToTexture() {
    if (!m_pContext || !m_OffscreenRTV || !m_pScreenCopy) return;
    m_pContext->CopyResource(m_pScreenCopy.Get(), m_OffscreenTex.Get());
}

void DirectXCore::DrawFullscreenQuad() {
    UINT stride = sizeof(QuadVertex);
    UINT offset = 0;
    m_pContext->IASetVertexBuffers(0, 1, m_pFullscreenQuadVB.GetAddressOf(), &stride, &offset);
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_pContext->Draw(4, 0);
}

bool DirectXCore::CreateOffscreenResources(UINT width, UINT height) {
    if (width == 0 || height == 0) return false;

    m_OffscreenTex.Reset();
    m_OffscreenRTV.Reset();
    m_OffscreenSRV.Reset();

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = m_pDevice->CreateTexture2D(&desc, nullptr, &m_OffscreenTex);
    if (FAILED(hr)) return false;
    hr = m_pDevice->CreateRenderTargetView(m_OffscreenTex.Get(), nullptr, &m_OffscreenRTV);
    if (FAILED(hr)) return false;
    hr = m_pDevice->CreateShaderResourceView(m_OffscreenTex.Get(), nullptr, &m_OffscreenSRV);
    return SUCCEEDED(hr);
}

bool DirectXCore::CreateFinalShaders() {
    const char* vsCode = R"(
        struct VSInput {
            float3 pos : POSITION;
            float2 uv  : TEXCOORD0;
        };
        struct VSOutput {
            float4 pos : SV_POSITION;
            float2 uv  : TEXCOORD0;
        };
        VSOutput main(VSInput input) {
            VSOutput output;
            output.pos = float4(input.pos, 1.0f);
            output.uv = input.uv;
            return output;
        }
    )";

    const char* psCode = R"(
        Texture2D tex : register(t0);
        SamplerState sam : register(s0);
        cbuffer CBFinal : register(b0) {
            float2 scale;
            float2 offset;
        };
        float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_Target {
            float2 ndc = uv * 2.0 - 1.0;
            float2 originalNdc = (ndc - offset) / scale;
            float2 originalUv = (originalNdc + 1.0) / 2.0;
            if (any(originalUv < 0.0) || any(originalUv > 1.0))
                return float4(0,0,0,0);
            return tex.Sample(sam, originalUv);
        }
    )";

    ComPtr<ID3DBlob> vsBlob, psBlob, error;
    HRESULT hr = D3DCompile(vsCode, strlen(vsCode), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &error);
    if (FAILED(hr)) { if (error) OutputDebugStringA((char*)error->GetBufferPointer()); return false; }
    hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &error);
    if (FAILED(hr)) { if (error) OutputDebugStringA((char*)error->GetBufferPointer()); return false; }

    hr = m_pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_pFinalVS);
    if (FAILED(hr)) return false;
    hr = m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pFinalPS);
    if (FAILED(hr)) return false;

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(float) * 4;
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = m_pDevice->CreateBuffer(&cbDesc, nullptr, &m_pFinalConstantBuffer);
    return SUCCEEDED(hr);
}

void DirectXCore::RenderOffscreenContent() {

    m_pContext->OMSetRenderTargets(1, m_OffscreenRTV.GetAddressOf(), nullptr);
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pContext->ClearRenderTargetView(m_OffscreenRTV.Get(), clearColor);

    float vw = (float)(m_clientWidth * m_renderScale);
    float vh = (float)(m_clientHeight * m_renderScale);
    D3D11_VIEWPORT offscreenVP = { 0, 0, vw, vh, 0.0f, 1.0f };
    m_pContext->RSSetViewports(1, &offscreenVP);

    m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
    m_pContext->VSSetShader(m_pVS.Get(), nullptr, 0);
    m_pContext->PSSetShader(m_pPS.Get(), nullptr, 0);
    m_pContext->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());
    m_pContext->IASetInputLayout(m_pInputLayout.Get());

    UINT stride = sizeof(QuadVertex);
    UINT offset = 0;
    m_pContext->IASetVertexBuffers(0, 1, m_pQuadVB.GetAddressOf(), &stride, &offset);
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    auto CalcWorldMatrixNoGlobal = [&](const DrawParams& p, int texW, int texH) -> XMMATRIX {
        float w_px = texW * p.scaleX;
        float h_px = texH * p.scaleY;
        float centerX_px = p.x + w_px * 0.5f;
        float centerY_px = p.y + h_px * 0.5f;
        float ndc_width = (w_px / vw) * 2.0f;
        float ndc_height = (h_px / vh) * 2.0f;
        float ndc_centerX = (centerX_px / vw) * 2.0f - 1.0f;
        float ndc_centerY = 1.0f - (centerY_px / vh) * 2.0f;
        XMMATRIX S = XMMatrixScaling(ndc_width, ndc_height, 1.0f);
        XMMATRIX T = XMMatrixTranslation(ndc_centerX, ndc_centerY, 0.0f);
        return S * T;
    };

    for (const auto& cmd : m_drawCommands) {
        if (cmd.bIsEffect) continue;
        TextureResource* tex = cmd.texRes;
        if (!tex || !tex->srv || tex->bIsIndexTexture) continue;

        const auto& p = cmd.params;
        XMMATRIX world = CalcWorldMatrixNoGlobal(p, tex->sourceView.FullWidth, tex->sourceView.FullHeight);
        CBPerObject cb;
        cb.world = XMMatrixTranspose(world);
        cb.colorMul = XMFLOAT4(p.redMult, p.greenMult, p.blueMult, p.opacity);
        cb.mixColor = XMFLOAT4(p.mixR, p.mixG, p.mixB, 1.0f);
        cb.mixFactor = p.mixFactor;

        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = m_pContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr)) continue;
        memcpy(mapped.pData, &cb, sizeof(cb));
        m_pContext->Unmap(m_pConstantBuffer.Get(), 0);

        m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
        m_pContext->PSSetShaderResources(0, 1, tex->srv.GetAddressOf());
        m_pContext->Draw(4, 0);
    }


    bool hasEffect = false;
    for (auto& cmd : m_drawCommands) if (cmd.bIsEffect) { hasEffect = true; break; }
    if (hasEffect) {

        CopyScreenToTexture();


        float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        m_pContext->ClearRenderTargetView(m_pFactorRTV.Get(), one);

        m_pContext->OMSetRenderTargets(1, m_pFactorRTV.GetAddressOf(), nullptr);
        m_pContext->OMSetBlendState(m_pMulBlendState.Get(), nullptr, 0xffffffff);
        m_pContext->VSSetShader(m_pEffectVS.Get(), nullptr, 0);
        m_pContext->PSSetShader(m_pEffectPS.Get(), nullptr, 0);
        m_pContext->PSSetSamplers(0, 1, m_pSamplerPoint.GetAddressOf());

        for (const auto& cmd : m_drawCommands) {
            if (!cmd.bIsEffect) continue;
            TextureResource* idxTex = cmd.texRes;
            if (!idxTex || !idxTex->srv || !idxTex->bIsIndexTexture) continue;

            const auto& p = cmd.params;
            XMMATRIX world = CalcWorldMatrixNoGlobal(p, idxTex->sourceView.FullWidth, idxTex->sourceView.FullHeight);
            CBPerObject cb;
            cb.world = XMMatrixTranspose(world);
            memset(&cb.colorMul, 0, sizeof(cb.colorMul) + sizeof(cb.mixColor) + sizeof(cb.mixFactor));

            D3D11_MAPPED_SUBRESOURCE mapped;
            HRESULT hr = m_pContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            if (FAILED(hr)) continue;
            memcpy(mapped.pData, &cb, sizeof(cb));
            m_pContext->Unmap(m_pConstantBuffer.Get(), 0);

            m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
            m_pContext->PSSetShaderResources(0, 1, idxTex->srv.GetAddressOf());
            m_pContext->Draw(4, 0);
        }

        m_pContext->OMSetRenderTargets(1, m_OffscreenRTV.GetAddressOf(), nullptr);
        m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
        m_pContext->VSSetShader(m_pCompositeVS.Get(), nullptr, 0);
        m_pContext->PSSetShader(m_pCompositePS.Get(), nullptr, 0);
        m_pContext->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());

        ID3D11ShaderResourceView* srvs[2] = { m_pScreenCopySRV.Get(), m_pFactorSRV.Get() };
        m_pContext->PSSetShaderResources(0, 2, srvs);
        DrawFullscreenQuad();

        ID3D11ShaderResourceView* nullSRV[2] = { nullptr, nullptr };
        m_pContext->PSSetShaderResources(0, 2, nullSRV);
    }
}

void DirectXCore::RenderFinalToBackBuffer() {
    D3D11_VIEWPORT windowVP = { 0, 0, (float)m_clientWidth, (float)m_clientHeight, 0.0f, 1.0f };
    m_pContext->RSSetViewports(1, &windowVP);

    m_pContext->OMSetRenderTargets(1, m_pRTV.GetAddressOf(), nullptr);
    float clearColor[4] = { 0,0,0,1 };
    m_pContext->ClearRenderTargetView(m_pRTV.Get(), clearColor);
    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    m_pContext->VSSetShader(m_pFinalVS.Get(), nullptr, 0);
    m_pContext->PSSetShader(m_pFinalPS.Get(), nullptr, 0);
    m_pContext->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());

    struct FinalCB {
        float scaleX, scaleY;
        float offsetX, offsetY;
    } cbData = { 1.0f, 1.0f, 0.0f, 0.0f };

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(m_pContext->Map(m_pFinalConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        memcpy(mapped.pData, &cbData, sizeof(cbData));
        m_pContext->Unmap(m_pFinalConstantBuffer.Get(), 0);
    }
    m_pContext->PSSetConstantBuffers(0, 1, m_pFinalConstantBuffer.GetAddressOf());

    m_pContext->PSSetShaderResources(0, 1, m_OffscreenSRV.GetAddressOf());

    UINT stride = sizeof(QuadVertex);
    UINT offset = 0;
    m_pContext->IASetVertexBuffers(0, 1, m_pFullscreenQuadVB.GetAddressOf(), &stride, &offset);
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_pContext->Draw(4, 0);

    ID3D11ShaderResourceView* nullSRV = nullptr;
    m_pContext->PSSetShaderResources(0, 1, &nullSRV);
}

void DirectXCore::Render() {
    if (!m_bInitialized || !m_pContext || !m_pSwapChain || !m_pRTV || m_drawCommands.empty()) {
        return;
    }

    RenderOffscreenContent();
    RenderFinalToBackBuffer();

    m_pSwapChain->Present(1, 0);
    m_drawCommands.clear();
}

DirectXCore::TextureResource* DirectXCore::LoadTexture(const FString& name, const ImageDataView& view) {
    if (!m_pDevice || name.empty()) return nullptr;
    if (view.FullWidth <= 0 || view.FullHeight <= 0 || !view.pImageBuffer) return nullptr;
    auto itr = m_textureMap.find(name);
    if (itr != m_textureMap.end()) return itr->second.get();

    auto texRes = std::make_unique<TextureResource>();
    texRes->sourceView = view;
    texRes->bIsIndexTexture = false;
    int w = view.FullWidth, h = view.FullHeight;
    std::vector<uint32_t> rgbaData(w * h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            unsigned char idx = view.pImageBuffer[y * w + x];
            unsigned char opacity = idx == 0 ? 0 : (view.pOpacity ? view.pOpacity[y * w + x] : 255);
            BGRStruct color = view.pPalette->Data[idx];
            uint32_t rgba = (color.R << 0) | (color.G << 8) | (color.B << 16) | (opacity << 24);
            rgbaData[y * w + x] = rgba;
        }
    }
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = w; desc.Height = h; desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA initData = { rgbaData.data(), (UINT)w * 4, 0 };
    HRESULT hr = m_pDevice->CreateTexture2D(&desc, &initData, &texRes->texture);
    if (FAILED(hr)) return nullptr;
    hr = m_pDevice->CreateShaderResourceView(texRes->texture.Get(), nullptr, &texRes->srv);
    if (FAILED(hr)) return nullptr;
    TextureResource* ret = texRes.get();
    m_textureMap[name] = std::move(texRes);
    return ret;
}

DirectXCore::TextureResource* DirectXCore::LoadTileTexture(CTileBlockClass* tileBlock, const ImageDataView& view) {
    if (!m_pDevice || !tileBlock) return nullptr;
    auto itr = m_tileTextureMap.find(tileBlock);
    if (itr != m_tileTextureMap.end()) return itr->second.get();
    auto texRes = std::make_unique<TextureResource>();
    texRes->sourceView = view;
    texRes->bIsIndexTexture = false;
    int w = view.FullWidth, h = view.FullHeight;
    std::vector<uint32_t> rgbaData(w * h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            unsigned char idx = view.pImageBuffer[y * w + x];
            unsigned char opacity = idx == 0 ? 0 : 255;
            BGRStruct color = view.pPalette->Data[idx];
            uint32_t rgba = (color.R << 0) | (color.G << 8) | (color.B << 16) | (opacity << 24);
            rgbaData[y * w + x] = rgba;
        }
    }
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = w; desc.Height = h; desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA initData = { rgbaData.data(), (UINT)w * 4, 0 };
    HRESULT hr = m_pDevice->CreateTexture2D(&desc, &initData, &texRes->texture);
    if (FAILED(hr)) return nullptr;
    hr = m_pDevice->CreateShaderResourceView(texRes->texture.Get(), nullptr, &texRes->srv);
    if (FAILED(hr)) return nullptr;
    TextureResource* ret = texRes.get();
    m_tileTextureMap[tileBlock] = std::move(texRes);
    return ret;
}

DirectXCore::TextureResource* DirectXCore::LoadIndexTexture(const FString& name, const ImageDataView& view) {
    if (!m_pDevice || name.empty()) return nullptr;
    if (view.FullWidth <= 0 || view.FullHeight <= 0 || !view.pImageBuffer) return nullptr;
    auto itr = m_textureMap.find(name);
    if (itr != m_textureMap.end()) return itr->second.get();

    auto texRes = std::make_unique<TextureResource>();
    texRes->sourceView = view;
    texRes->bIsIndexTexture = true;
    int w = view.FullWidth, h = view.FullHeight;
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = w; desc.Height = h; desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8_UNORM; desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA initData = { view.pImageBuffer, (UINT)w * 1, 0 };
    HRESULT hr = m_pDevice->CreateTexture2D(&desc, &initData, &texRes->texture);
    if (FAILED(hr)) return nullptr;
    hr = m_pDevice->CreateShaderResourceView(texRes->texture.Get(), nullptr, &texRes->srv);
    if (FAILED(hr)) return nullptr;
    TextureResource* ret = texRes.get();
    m_textureMap[name] = std::move(texRes);
    return ret;
}

DirectXCore::TextureResource* DirectXCore::GetTexture(const FString& name) const {
    auto it = m_textureMap.find(name);
    return (it != m_textureMap.end()) ? it->second.get() : nullptr;
}

DirectXCore::TextureResource* DirectXCore::GetTileTexture(CTileBlockClass* tileBlock) const {
    auto it = m_tileTextureMap.find(tileBlock);
    return (it != m_tileTextureMap.end()) ? it->second.get() : nullptr;
}

void DirectXCore::DrawTexture(TextureResource* tex, const DrawParams& params) {
    if (!tex) return;
    m_drawCommands.push_back({ tex, params, tex->bIsIndexTexture });
}