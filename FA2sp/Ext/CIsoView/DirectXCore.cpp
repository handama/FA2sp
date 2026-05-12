#include <DirectXMath.h>
#include <vector>
#include <cstring>
#include <string>
#include "../CLoading/Body.h"
#include "Body.h"
#include "DirectXCore.h"
#include "../../Miscs/Palettes.h"

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using namespace Microsoft::WRL;
using namespace DirectX;
std::unique_ptr<DirectXCore> CIsoViewExt::g_pDX;
std::unique_ptr<DrawShapes> CIsoViewExt::g_pSP;
std::unique_ptr<TextRenderer> CIsoViewExt::g_pTR;

struct QuadVertex
{
    XMFLOAT3 pos;
    XMFLOAT2 uv;
};
struct CBPerObject
{
    XMMATRIX world;
    XMFLOAT4 colorMul;
    XMFLOAT4 mixColor;
    float mixFactor;
    float padding[3];
};

bool TextureIndex::operator==(const TextureIndex &other) const
{
    return pData == other.pData && color == other.color;
}

DrawParams &DrawParams::SetColorMul(ColorMults mults)
{
    redMult = mults.RedTint;
    greenMult = mults.GreenTint;
    blueMult = mults.BlueTint;
    return *this;
}

DrawParams &DrawParams::SetColorMix(RGBClass color, float factor)
{
    return SetColorMix(color.R / 255.0f, color.G / 255.0f, color.B / 255.0f, factor);
}

DirectXCore::DirectXCore() = default;
DirectXCore::~DirectXCore() { Cleanup(); }

bool DirectXCore::Initialize(HWND hwnd)
{
    Cleanup();

    if (!IsWindow(hwnd))
    {
        Logger::Raw("[DirectXCore] Initialize: Invalid HWND\n");
        return false;
    }

    if (!CreateDeviceAndSwapChain(hwnd))
    {
        Logger::Raw("[DirectXCore] CreateDeviceAndSwapChain failed\n");
        return false;
    }
    if (!CreateShadersAndInputLayout())
    {
        Logger::Raw("[DirectXCore] CreateShadersAndInputLayout failed\n");
        return false;
    }
    if (!CreateEffectShaders())
    {
        Logger::Raw("[DirectXCore] CreateEffectShaders failed\n");
        return false;
    }
    if (!CreateCompositeShaders())
    {
        Logger::Raw("[DirectXCore] CreateCompositeShaders failed\n");
        return false;
    }
    if (!CreateQuadVertexBuffer())
    {
        Logger::Raw("[DirectXCore] CreateQuadVertexBuffer failed\n");
        return false;
    }

    RECT rc;
    GetClientRect(hwnd, &rc);
    m_clientWidth = rc.right - rc.left;
    m_clientHeight = rc.bottom - rc.top;
    UpdateViewportAndRTV(hwnd);

    UINT vw = (UINT)(m_clientWidth * m_renderScale);
    UINT vh = (UINT)(m_clientHeight * m_renderScale);
    if (vw == 0)
        vw = 1;
    if (vh == 0)
        vh = 1;

    if (!CreateOffscreenResources(vw, vh))
    {
        Logger::Raw("[DirectXCore] CreateOffscreenResources failed\n");
        return false;
    }
    if (!CreateFinalShaders())
    {
        Logger::Raw("[DirectXCore] CreateFinalShaders failed\n");
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

    CreateBackgroundCacheTexture(m_clientWidth, m_clientHeight);
    m_backgroundCacheValid = false;

    m_bInitialized = true;
    Logger::Raw("[DirectXCore] Initialize succeeded\n");
    return true;
}

bool DirectXCore::IsInitialized()
{
    return m_bInitialized;
}

void DirectXCore::ClearTextures()
{
    m_textureMap.clear();
    Logger::Raw("[DirectXCore] Clear textures\n");
}

void DirectXCore::ClearTileTextures()
{
    m_tileTextureMap.clear();
    Logger::Raw("[DirectXCore] Clear tile textures\n");
}

void DirectXCore::Cleanup()
{
    m_drawCommands.clear();
    m_textureMap.clear();
    m_bitmapTextureMap.clear();
    m_tileTextureMap.clear();

    if (m_pContext)
    {
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
    m_pSamplerNearestNeighbor.Reset();
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

    m_pBackgroundCacheTexture.Reset();
    m_pBackgroundCacheSRV.Reset();
    m_backgroundCacheValid = false;

    m_clientWidth = m_clientHeight = 0;
    m_globalScaleX = m_globalScaleY = 1.0f;
    m_globalOffsetX = m_globalOffsetY = 0.0f;
    m_renderScale = 1.0f;
    m_bInitialized = false;

    Logger::Raw("[DirectXCore] Reset all\n");
}

void DirectXCore::OnResize(HWND hwnd)
{
    if (!m_pSwapChain)
        return;

    m_pRTV.Reset();
    m_OffscreenRTV.Reset();
    m_OffscreenSRV.Reset();
    m_OffscreenTex.Reset();
    m_pFactorTexture.Reset();
    m_pFactorRTV.Reset();
    m_pFactorSRV.Reset();
    m_pScreenCopy.Reset();
    m_pScreenCopySRV.Reset();

    RECT rc;
    GetClientRect(hwnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;
    HRESULT hr = m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (SUCCEEDED(hr))
    {
        ComPtr<ID3D11Texture2D> pBackBuffer;
        m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer);
        m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_pRTV);
    }
    m_clientWidth = width;
    m_clientHeight = height;
    UpdateViewportAndRTV(hwnd);

    UINT vw = (UINT)(width * m_renderScale);
    UINT vh = (UINT)(height * m_renderScale);
    if (vw == 0)
        vw = 1;
    if (vh == 0)
        vh = 1;

    CreateOffscreenResources(vw, vh);
    EnsureFactorTexture(vw, vh);
    EnsureScreenCopyTexture(vw, vh);

    CreateBackgroundCacheTexture(width, height);
    m_backgroundCacheValid = false;
}

void DirectXCore::UpdateViewportAndRTV(HWND hwnd)
{
    if (!m_pContext)
        return;
    D3D11_VIEWPORT vp = {0, 0, (float)m_clientWidth, (float)m_clientHeight, 0.0f, 1.0f};
    m_pContext->RSSetViewports(1, &vp);
}

void DirectXCore::SetGlobalTransform(float scaleX, float scaleY, float offsetX, float offsetY)
{
    m_globalScaleX = scaleX;
    m_globalScaleY = scaleY;
    m_globalOffsetX = offsetX;
    m_globalOffsetY = offsetY;
}

void DirectXCore::SetZoomOut(float scaleFactor)
{
    if (scaleFactor < 0.01f)
        scaleFactor = 0.01f;
    if (m_renderScale == scaleFactor)
        return;
    m_renderScale = scaleFactor;

    UINT vw = (UINT)(m_clientWidth * m_renderScale);
    UINT vh = (UINT)(m_clientHeight * m_renderScale);
    if (vw == 0)
        vw = 1;
    if (vh == 0)
        vh = 1;

    m_OffscreenTex.Reset();
    m_OffscreenRTV.Reset();
    m_OffscreenSRV.Reset();
    m_pFactorTexture.Reset();
    m_pFactorRTV.Reset();
    m_pFactorSRV.Reset();
    m_pScreenCopy.Reset();
    m_pScreenCopySRV.Reset();

    CreateOffscreenResources(vw, vh);
    EnsureFactorTexture(vw, vh);
    EnsureScreenCopyTexture(vw, vh);
}

bool DirectXCore::CreateDeviceAndSwapChain(HWND hwnd)
{
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

    D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                               createFlags, featureLevels, 2,
                                               D3D11_SDK_VERSION, &scd,
                                               &m_pSwapChain, &m_pDevice, nullptr, &m_pContext);
    if (FAILED(hr))
    {
        Logger::Raw("D3D11CreateDeviceAndSwapChain failed\n");
        return false;
    }

    ComPtr<ID3D11Texture2D> pBackBuffer;
    hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer);
    if (FAILED(hr))
        return false;
    hr = m_pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_pRTV);
    if (FAILED(hr))
        return false;
    return true;
}

bool DirectXCore::CreateShadersAndInputLayout()
{
    const char *vsCode = R"(
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

    const char *psCode = R"(
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
    if (FAILED(hr))
    {
        if (error)
            OutputDebugStringA((char *)error->GetBufferPointer());
        return false;
    }
    hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &error);
    if (FAILED(hr))
    {
        if (error)
            OutputDebugStringA((char *)error->GetBufferPointer());
        return false;
    }

    hr = m_pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_pVS);
    if (FAILED(hr))
        return false;
    hr = m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pPS);
    if (FAILED(hr))
        return false;

    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}};
    hr = m_pDevice->CreateInputLayout(layoutDesc, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_pInputLayout);
    if (FAILED(hr))
        return false;

    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    m_pDevice->CreateSamplerState(&sampDesc, &m_pSamplerLinear);
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    m_pDevice->CreateSamplerState(&sampDesc, &m_pSamplerPoint);
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    m_pDevice->CreateSamplerState(&sampDesc, &m_pSamplerNearestNeighbor);

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

bool DirectXCore::CreateEffectShaders()
{
    const char *vsCode = R"(
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
    const char *psCode = R"(
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
    if (FAILED(hr))
    {
        if (error)
            OutputDebugStringA((char *)error->GetBufferPointer());
        return false;
    }
    hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &error);
    if (FAILED(hr))
    {
        if (error)
            OutputDebugStringA((char *)error->GetBufferPointer());
        return false;
    }
    hr = m_pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_pEffectVS);
    if (FAILED(hr))
        return false;
    hr = m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pEffectPS);
    if (FAILED(hr))
        return false;
    return true;
}

bool DirectXCore::CreateCompositeShaders()
{
    const char *vsCode = R"(
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
    const char *psCode = R"(
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
    if (FAILED(hr))
    {
        if (error)
            OutputDebugStringA((char *)error->GetBufferPointer());
        return false;
    }
    hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &error);
    if (FAILED(hr))
    {
        if (error)
            OutputDebugStringA((char *)error->GetBufferPointer());
        return false;
    }
    hr = m_pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_pCompositeVS);
    if (FAILED(hr))
        return false;
    hr = m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pCompositePS);
    if (FAILED(hr))
        return false;
    return true;
}

bool DirectXCore::CreateQuadVertexBuffer()
{
    QuadVertex vertices[] = {
        {XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT2(0.0f, 1.0f)},
        {XMFLOAT3(-0.5f, 0.5f, 0.0f), XMFLOAT2(0.0f, 0.0f)},
        {XMFLOAT3(0.5f, -0.5f, 0.0f), XMFLOAT2(1.0f, 1.0f)},
        {XMFLOAT3(0.5f, 0.5f, 0.0f), XMFLOAT2(1.0f, 0.0f)}};
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA initData = {vertices};
    HRESULT hr = m_pDevice->CreateBuffer(&bd, &initData, &m_pQuadVB);
    QuadVertex fsVertices[] = {
        {XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f)},
        {XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f)},
        {XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f)},
        {XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f)},
    };
    D3D11_BUFFER_DESC fsbd = {};
    fsbd.Usage = D3D11_USAGE_IMMUTABLE;
    fsbd.ByteWidth = sizeof(fsVertices);
    fsbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA fsInitData = {fsVertices};
    hr = m_pDevice->CreateBuffer(&fsbd, &fsInitData, &m_pFullscreenQuadVB);

    return SUCCEEDED(hr);
}

void DirectXCore::EnsureFactorTexture(UINT width, UINT height)
{
    if (m_pFactorTexture && width == m_clientWidth && height == m_clientHeight)
        return;
    m_pFactorTexture.Reset();
    m_pFactorRTV.Reset();
    m_pFactorSRV.Reset();
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
    if (FAILED(hr))
        return;
    hr = m_pDevice->CreateRenderTargetView(m_pFactorTexture.Get(), nullptr, &m_pFactorRTV);
    if (FAILED(hr))
        return;
    hr = m_pDevice->CreateShaderResourceView(m_pFactorTexture.Get(), nullptr, &m_pFactorSRV);
}

void DirectXCore::EnsureScreenCopyTexture(UINT width, UINT height)
{
    if (m_pScreenCopy && width == m_clientWidth && height == m_clientHeight)
        return;
    m_pScreenCopy.Reset();
    m_pScreenCopySRV.Reset();
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
    if (FAILED(hr))
        return;
    hr = m_pDevice->CreateShaderResourceView(m_pScreenCopy.Get(), nullptr, &m_pScreenCopySRV);
}

void DirectXCore::CopyScreenToTexture()
{
    if (!m_pContext || !m_OffscreenRTV || !m_pScreenCopy)
        return;
    m_pContext->CopyResource(m_pScreenCopy.Get(), m_OffscreenTex.Get());
}

void DirectXCore::DrawFullscreenQuad()
{
    UINT stride = sizeof(QuadVertex);
    UINT offset = 0;
    m_pContext->IASetVertexBuffers(0, 1, m_pFullscreenQuadVB.GetAddressOf(), &stride, &offset);
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_pContext->Draw(4, 0);
}

bool DirectXCore::CreateOffscreenResources(UINT width, UINT height)
{
    if (width == 0 || height == 0)
        return false;

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
    if (FAILED(hr))
        return false;
    hr = m_pDevice->CreateRenderTargetView(m_OffscreenTex.Get(), nullptr, &m_OffscreenRTV);
    if (FAILED(hr))
        return false;
    hr = m_pDevice->CreateShaderResourceView(m_OffscreenTex.Get(), nullptr, &m_OffscreenSRV);
    return SUCCEEDED(hr);
}

void DirectXCore::CreateBackgroundCacheTexture(UINT width, UINT height)
{
    if (!m_pDevice)
        return;
    if (width == 0 || height == 0)
        return;

    m_pBackgroundCacheTexture.Reset();
    m_pBackgroundCacheSRV.Reset();

    ComPtr<ID3D11Texture2D> pBackBuffer;
    if (m_pSwapChain)
    {
        m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    }
    D3D11_TEXTURE2D_DESC desc = {};
    if (pBackBuffer)
    {
        pBackBuffer->GetDesc(&desc);
    }
    else
    {
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
    }

    HRESULT hr = m_pDevice->CreateTexture2D(&desc, nullptr, &m_pBackgroundCacheTexture);
    if (SUCCEEDED(hr) && m_pBackgroundCacheTexture)
    {
        m_pDevice->CreateShaderResourceView(m_pBackgroundCacheTexture.Get(), nullptr, &m_pBackgroundCacheSRV);
    }
}

void DirectXCore::UpdateBackgroundCache()
{
    if (!m_pRTV || !m_pBackgroundCacheTexture)
        return;
    ComPtr<ID3D11Resource> pBackBufferResource;
    m_pRTV->GetResource(&pBackBufferResource);
    if (pBackBufferResource)
    {
        m_pContext->CopyResource(m_pBackgroundCacheTexture.Get(), pBackBufferResource.Get());
        m_backgroundCacheValid = true;
    }
}

void DirectXCore::RestoreBackgroundFromCache()
{
    if (!m_backgroundCacheValid || !m_pBackgroundCacheTexture)
        return;
    ComPtr<ID3D11Resource> pBackBufferResource;
    m_pRTV->GetResource(&pBackBufferResource);
    if (pBackBufferResource)
    {
        m_pContext->CopyResource(pBackBufferResource.Get(), m_pBackgroundCacheTexture.Get());
    }
}

bool DirectXCore::CreateFinalShaders()
{
    const char *vsCode = R"(
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

    const char *psCode = R"(
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
    if (FAILED(hr))
    {
        if (error)
            OutputDebugStringA((char *)error->GetBufferPointer());
        return false;
    }
    hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &error);
    if (FAILED(hr))
    {
        if (error)
            OutputDebugStringA((char *)error->GetBufferPointer());
        return false;
    }

    hr = m_pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_pFinalVS);
    if (FAILED(hr))
        return false;
    hr = m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pFinalPS);
    if (FAILED(hr))
        return false;

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(float) * 4;
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = m_pDevice->CreateBuffer(&cbDesc, nullptr, &m_pFinalConstantBuffer);
    return SUCCEEDED(hr);
}

void DirectXCore::RenderOffscreenContent()
{

    m_pContext->OMSetRenderTargets(1, m_OffscreenRTV.GetAddressOf(), nullptr);
    float clearColor[4] = {
        ExtConfigs::EnableDarkMode ? 0.125f : 1.0f,
        ExtConfigs::EnableDarkMode ? 0.125f : 1.0f,
        ExtConfigs::EnableDarkMode ? 0.125f : 1.0f,
        1.0f};
    m_pContext->ClearRenderTargetView(m_OffscreenRTV.Get(), clearColor);

    float vw = (float)(m_clientWidth * m_renderScale);
    float vh = (float)(m_clientHeight * m_renderScale);
    D3D11_VIEWPORT offscreenVP = {0, 0, vw, vh, 0.0f, 1.0f};
    m_pContext->RSSetViewports(1, &offscreenVP);

    m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
    m_pContext->VSSetShader(m_pVS.Get(), nullptr, 0);
    m_pContext->PSSetShader(m_pPS.Get(), nullptr, 0);
    auto &offscreenSampler = (m_renderScale == 1.0f)
                                 ? m_pSamplerPoint
                                 : m_pSamplerLinear;
    m_pContext->PSSetSamplers(0, 1, offscreenSampler.GetAddressOf());
    m_pContext->IASetInputLayout(m_pInputLayout.Get());

    UINT stride = sizeof(QuadVertex);
    UINT offset = 0;
    m_pContext->IASetVertexBuffers(0, 1, m_pQuadVB.GetAddressOf(), &stride, &offset);
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    auto CalcWorldMatrixNoGlobal = [&](const DrawParams &p, int texW, int texH) -> XMMATRIX
    {
        float w_px = texW * p.scaleX;
        float h_px = texH * p.scaleY;
        float snappedX = std::floor(p.x + 0.5f);
        float snappedY = std::floor(p.y + 0.5f);
        float centerX_px = snappedX + w_px * 0.5f;
        float centerY_px = snappedY + h_px * 0.5f;
        float ndc_width = (w_px / vw) * 2.0f;
        float ndc_height = (h_px / vh) * 2.0f;
        float ndc_centerX = (centerX_px / vw) * 2.0f - 1.0f;
        float ndc_centerY = 1.0f - (centerY_px / vh) * 2.0f;
        XMMATRIX S = XMMatrixScaling(ndc_width, ndc_height, 1.0f);
        XMMATRIX T = XMMatrixTranslation(ndc_centerX, ndc_centerY, 0.0f);
        return S * T;
    };

    for (const auto &cmd : m_drawCommands)
    {
        if (cmd.bIsEffect)
            continue;
        if (cmd.bScreenSpace)
            continue;
        TextureResource *tex = cmd.texRes;
        if (!tex || !tex->srv || tex->bIsIndexTexture)
            continue;

        const auto &p = cmd.params;
        XMMATRIX world = CalcWorldMatrixNoGlobal(p, tex->sourceView.FullWidth, tex->sourceView.FullHeight);
        CBPerObject cb;
        cb.world = XMMatrixTranspose(world);
        cb.colorMul = XMFLOAT4(p.redMult, p.greenMult, p.blueMult, p.opacity);
        cb.mixColor = XMFLOAT4(p.mixR, p.mixG, p.mixB, 1.0f);
        cb.mixFactor = p.mixFactor;

        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = m_pContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr))
            continue;
        memcpy(mapped.pData, &cb, sizeof(cb));
        m_pContext->Unmap(m_pConstantBuffer.Get(), 0);

        m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
        m_pContext->PSSetShaderResources(0, 1, tex->srv.GetAddressOf());
        m_pContext->Draw(4, 0);
    }

    bool hasEffect = false;
    for (auto &cmd : m_drawCommands)
        if (cmd.bIsEffect)
        {
            hasEffect = true;
            break;
        }
    if (hasEffect)
    {
        CopyScreenToTexture();

        float one[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        m_pContext->ClearRenderTargetView(m_pFactorRTV.Get(), one);

        m_pContext->OMSetRenderTargets(1, m_pFactorRTV.GetAddressOf(), nullptr);
        m_pContext->OMSetBlendState(m_pMulBlendState.Get(), nullptr, 0xffffffff);
        m_pContext->VSSetShader(m_pEffectVS.Get(), nullptr, 0);
        m_pContext->PSSetShader(m_pEffectPS.Get(), nullptr, 0);
        m_pContext->PSSetSamplers(0, 1, m_pSamplerPoint.GetAddressOf());

        for (const auto &cmd : m_drawCommands)
        {
            if (!cmd.bIsEffect)
                continue;
            TextureResource *idxTex = cmd.texRes;
            if (!idxTex || !idxTex->srv || !idxTex->bIsIndexTexture)
                continue;

            const auto &p = cmd.params;
            XMMATRIX world = CalcWorldMatrixNoGlobal(p, idxTex->sourceView.FullWidth, idxTex->sourceView.FullHeight);
            CBPerObject cb;
            cb.world = XMMatrixTranspose(world);
            memset(&cb.colorMul, 0, sizeof(cb.colorMul) + sizeof(cb.mixColor) + sizeof(cb.mixFactor));

            D3D11_MAPPED_SUBRESOURCE mapped;
            HRESULT hr = m_pContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            if (FAILED(hr))
                continue;
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

        ID3D11ShaderResourceView *srvs[2] = {m_pScreenCopySRV.Get(), m_pFactorSRV.Get()};
        m_pContext->PSSetShaderResources(0, 2, srvs);
        DrawFullscreenQuad();

        ID3D11ShaderResourceView *nullSRV[2] = {nullptr, nullptr};
        m_pContext->PSSetShaderResources(0, 2, nullSRV);
    }
}

void DirectXCore::RenderFinalToBackBuffer()
{
    D3D11_VIEWPORT windowVP = {0, 0, (float)m_clientWidth, (float)m_clientHeight, 0.0f, 1.0f};
    m_pContext->RSSetViewports(1, &windowVP);

    m_pContext->OMSetRenderTargets(1, m_pRTV.GetAddressOf(), nullptr);

    if (m_renderScale == 1.0f)
    {
        ComPtr<ID3D11Resource> pBackBufferResource;
        m_pRTV->GetResource(&pBackBufferResource);
        if (pBackBufferResource && m_OffscreenTex)
        {
            m_pContext->CopyResource(pBackBufferResource.Get(), m_OffscreenTex.Get());
        }
        return;
    }

    float clearColor[4] = {0, 0, 0, 1};
    m_pContext->ClearRenderTargetView(m_pRTV.Get(), clearColor);
    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);

    m_pContext->VSSetShader(m_pFinalVS.Get(), nullptr, 0);
    m_pContext->PSSetShader(m_pFinalPS.Get(), nullptr, 0);

    auto &sampler = (!ExtConfigs::DDrawScalingBilinear ||
                     (ExtConfigs::DDrawScalingBilinear_OnlyShrink && CIsoViewExt::ScaledFactor < 1.0f))
                        ? m_pSamplerNearestNeighbor
                        : m_pSamplerLinear;

    m_pContext->PSSetSamplers(0, 1, sampler.GetAddressOf());

    struct FinalCB
    {
        float scaleX, scaleY;
        float offsetX, offsetY;
    } cbData = {1.0f, 1.0f, 0.0f, 0.0f};

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(m_pContext->Map(m_pFinalConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
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

    ID3D11ShaderResourceView *nullSRV = nullptr;
    m_pContext->PSSetShaderResources(0, 1, &nullSRV);
}

void DirectXCore::RenderScreenSpaceContent()
{
    if (m_drawCommands.empty())
        return;

    D3D11_VIEWPORT screenVP = {0, 0, (float)m_clientWidth, (float)m_clientHeight, 0.0f, 1.0f};
    m_pContext->RSSetViewports(1, &screenVP);

    m_pContext->OMSetRenderTargets(1, m_pRTV.GetAddressOf(), nullptr);

    m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);

    m_pContext->VSSetShader(m_pVS.Get(), nullptr, 0);
    m_pContext->PSSetShader(m_pPS.Get(), nullptr, 0);
    m_pContext->PSSetSamplers(0, 1, m_pSamplerPoint.GetAddressOf());
    m_pContext->IASetInputLayout(m_pInputLayout.Get());

    UINT stride = sizeof(QuadVertex);
    UINT offset = 0;
    m_pContext->IASetVertexBuffers(0, 1, m_pQuadVB.GetAddressOf(), &stride, &offset);
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    auto CalcWorldMatrixScreen = [&](const DrawParams &p, int texW, int texH) -> XMMATRIX
    {
        float screenW = (float)m_clientWidth;
        float screenH = (float)m_clientHeight;
        float w_px = texW * p.scaleX;
        float h_px = texH * p.scaleY;
        float snappedX = std::floor(p.x + 0.5f);
        float snappedY = std::floor(p.y + 0.5f);
        float centerX_px = snappedX + w_px * 0.5f;
        float centerY_px = snappedY + h_px * 0.5f;
        float ndc_width = (w_px / screenW) * 2.0f;
        float ndc_height = (h_px / screenH) * 2.0f;
        float ndc_centerX = (centerX_px / screenW) * 2.0f - 1.0f;
        float ndc_centerY = 1.0f - (centerY_px / screenH) * 2.0f;
        XMMATRIX S = XMMatrixScaling(ndc_width, ndc_height, 1.0f);
        XMMATRIX T = XMMatrixTranslation(ndc_centerX, ndc_centerY, 0.0f);
        return S * T;
    };

    for (const auto &cmd : m_drawCommands)
    {
        if (!cmd.bScreenSpace)
            continue;
        if (cmd.bIsEffect)
            continue;
        TextureResource *tex = cmd.texRes;
        if (!tex || !tex->srv)
            continue;

        const auto &p = cmd.params;
        XMMATRIX world = CalcWorldMatrixScreen(p, tex->sourceView.FullWidth, tex->sourceView.FullHeight);
        CBPerObject cb;
        cb.world = XMMatrixTranspose(world);
        cb.colorMul = XMFLOAT4(p.redMult, p.greenMult, p.blueMult, p.opacity);
        cb.mixColor = XMFLOAT4(p.mixR, p.mixG, p.mixB, 1.0f);
        cb.mixFactor = p.mixFactor;

        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = m_pContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr))
            continue;
        memcpy(mapped.pData, &cb, sizeof(cb));
        m_pContext->Unmap(m_pConstantBuffer.Get(), 0);

        m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
        m_pContext->PSSetShaderResources(0, 1, tex->srv.GetAddressOf());
        m_pContext->Draw(4, 0);
    }

    ID3D11ShaderResourceView *nullSRV = nullptr;
    m_pContext->PSSetShaderResources(0, 1, &nullSRV);
}

void DirectXCore::Render()
{
    if (!m_bInitialized || !m_pContext || !m_pSwapChain || !m_pRTV || m_drawCommands.empty())
    {
        return;
    }

    RenderOffscreenContent();
    RenderFinalToBackBuffer();
    UpdateBackgroundCache();
    RenderScreenSpaceContent();

    m_pSwapChain->Present(1, 0);
    m_drawCommands.clear();
}

void DirectXCore::RenderScreenSpaceOnly()
{
    if (!m_bInitialized || !m_pContext || !m_pSwapChain || !m_pRTV || !m_backgroundCacheValid)
        return;
    RestoreBackgroundFromCache();
    RenderScreenSpaceContent();
    m_pSwapChain->Present(1, 0);
    m_drawCommands.clear();
}

TextureResource *DirectXCore::LoadTexture(const ImageDataView &view, BGRStruct color)
{
    auto index = TextureIndex{view.pOriginData, color};
    auto [itr, inserted] = m_textureMap.try_emplace(index);
    if (!inserted)
        return itr->second.get();
    if (!m_pDevice || !view.pOriginData)
        return nullptr;
    if (view.FullWidth <= 0 || view.FullHeight <= 0 || !view.pImageBuffer)
        return nullptr;

    auto texRes = std::make_unique<TextureResource>();
    texRes->sourceView = view;
    texRes->bIsIndexTexture = false;
    int w = view.FullWidth, h = view.FullHeight;
    std::vector<uint32_t> rgbaData(w * h);
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            unsigned char idx = view.pImageBuffer[y * w + x];
            unsigned char opacity = idx == 0 ? 0 : (view.pOpacity ? view.pOpacity[y * w + x] : 255);
            BGRStruct color = view.pPalette->Data[idx];
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
    D3D11_SUBRESOURCE_DATA initData = {rgbaData.data(), (UINT)w * 4, 0};
    HRESULT hr = m_pDevice->CreateTexture2D(&desc, &initData, &texRes->texture);
    if (FAILED(hr))
        return nullptr;
    hr = m_pDevice->CreateShaderResourceView(texRes->texture.Get(), nullptr, &texRes->srv);
    if (FAILED(hr))
        return nullptr;
    TextureResource *ret = texRes.get();
    itr->second = std::move(texRes);
    return ret;
}

TextureResource *DirectXCore::LoadTileTexture(CTileBlockClass *tileBlock, const ImageDataView &view)
{
    auto [itr, inserted] = m_tileTextureMap.try_emplace(tileBlock);
    if (!inserted)
        return itr->second.get();
    if (!m_pDevice || !tileBlock)
        return nullptr;
    auto texRes = std::make_unique<TextureResource>();
    texRes->sourceView = view;
    texRes->bIsIndexTexture = false;
    int w = view.FullWidth, h = view.FullHeight;
    std::vector<uint32_t> rgbaData(w * h);
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            unsigned char idx = view.pImageBuffer[y * w + x];
            unsigned char opacity = idx == 0 ? 0 : 255;
            BGRStruct color = view.pPalette->Data[idx];
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
    D3D11_SUBRESOURCE_DATA initData = {rgbaData.data(), (UINT)w * 4, 0};
    HRESULT hr = m_pDevice->CreateTexture2D(&desc, &initData, &texRes->texture);
    if (FAILED(hr))
        return nullptr;
    hr = m_pDevice->CreateShaderResourceView(texRes->texture.Get(), nullptr, &texRes->srv);
    if (FAILED(hr))
        return nullptr;
    TextureResource *ret = texRes.get();
    itr->second = std::move(texRes);
    return ret;
}

TextureResource *DirectXCore::LoadIndexTexture(const ImageDataView &view)
{
    auto index = TextureIndex{view.pOriginData};
    auto [itr, inserted] = m_textureMap.try_emplace(index);
    if (!inserted)
        return itr->second.get();
    if (!m_pDevice || !view.pOriginData)
        return nullptr;
    if (view.FullWidth <= 0 || view.FullHeight <= 0 || !view.pImageBuffer)
        return nullptr;

    auto texRes = std::make_unique<TextureResource>();
    texRes->sourceView = view;
    texRes->bIsIndexTexture = true;
    int w = view.FullWidth, h = view.FullHeight;
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA initData = {view.pImageBuffer, (UINT)w * 1, 0};
    HRESULT hr = m_pDevice->CreateTexture2D(&desc, &initData, &texRes->texture);
    if (FAILED(hr))
        return nullptr;
    hr = m_pDevice->CreateShaderResourceView(texRes->texture.Get(), nullptr, &texRes->srv);
    if (FAILED(hr))
        return nullptr;
    TextureResource *ret = texRes.get();
    itr->second = std::move(texRes);
    return ret;
}

TextureResource *DirectXCore::LoadBitmapTexture(FString_view name, CBitmap &bitmap, bool setColorKey, COLORREF color)
{
    auto [itr, inserted] = m_bitmapTextureMap.try_emplace(name);
    if (!inserted)
        return itr->second.get();
    if (!m_pDevice)
        return nullptr;

    BITMAP bm = {};
    bitmap.GetBitmap(&bm);

    if (bm.bmWidth <= 0 || bm.bmHeight <= 0)
        return nullptr;

    const int w = bm.bmWidth;
    const int h = bm.bmHeight;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<uint32_t> rgbaData(w * h);
    HDC hdc = ::GetDC(nullptr);
    int result = ::GetDIBits(
        hdc,
        (HBITMAP)bitmap.GetSafeHandle(),
        0,
        h,
        rgbaData.data(),
        &bmi,
        DIB_RGB_COLORS);
    ::ReleaseDC(nullptr, hdc);
    if (result == 0)
        return nullptr;

    if (setColorKey)
    {
        const uint8_t transparentR = GetRValue(color);
        const uint8_t transparentG = GetGValue(color);
        const uint8_t transparentB = GetBValue(color);

        for (auto &pixel : rgbaData)
        {
            uint8_t *p =
                reinterpret_cast<uint8_t *>(&pixel);
            uint8_t b = p[0];
            uint8_t g = p[1];
            uint8_t r = p[2];
            p[0] = r;
            p[1] = g;
            p[2] = b;
            if (r == transparentR &&
                g == transparentG &&
                b == transparentB)
            {
                p[3] = 0;
            }
            else
            {
                p[3] = 255;
            }
        }
    }
    else
    {
        for (auto &pixel : rgbaData)
        {
            uint8_t *p = reinterpret_cast<uint8_t *>(&pixel);
            std::swap(p[0], p[2]);
            if (p[3] == 0)
                p[3] = 255;
        }
    }

    auto texRes = std::make_unique<TextureResource>();
    texRes->bIsIndexTexture = false;
    texRes->sourceView.FullWidth = w;
    texRes->sourceView.FullHeight = h;
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

    HRESULT hr = m_pDevice->CreateTexture2D(
        &desc,
        &initData,
        &texRes->texture);
    if (FAILED(hr))
        return nullptr;
    hr = m_pDevice->CreateShaderResourceView(
        texRes->texture.Get(),
        nullptr,
        &texRes->srv);
    if (FAILED(hr))
        return nullptr;
    TextureResource *ret = texRes.get();
    itr->second = std::move(texRes);
    return ret;
}

void DirectXCore::RemoveBitmapTexture(FString_view name)
{
    m_bitmapTextureMap.erase(name);
}

TextureResource *DirectXCore::GetTexture(void *pData, BGRStruct color) const
{
    auto it = m_textureMap.find(TextureIndex{pData, color});
    return (it != m_textureMap.end()) ? it->second.get() : nullptr;
}

TextureResource *DirectXCore::GetTileTexture(CTileBlockClass *tileBlock) const
{
    auto it = m_tileTextureMap.find(tileBlock);
    return (it != m_tileTextureMap.end()) ? it->second.get() : nullptr;
}

TextureResource *DirectXCore::GetBitmapTexture(FString_view name) const
{
    auto it = m_bitmapTextureMap.find(name);
    return (it != m_bitmapTextureMap.end()) ? it->second.get() : nullptr;
}

void DirectXCore::DrawTexture(TextureResource *tex, const DrawParams &params)
{
    if (!tex)
        return;
    m_drawCommands.push_back({tex, params, tex->bIsIndexTexture, params.bScreenSpace});
}

static constexpr float kPi = 3.14159265358979323846f;

static inline uint32_t ColorToU32(const ShapeColor &c, float extraAlpha = 1.f)
{
    float a = c.a * extraAlpha;
    auto sat = [](float v) -> uint8_t
    {
        return (uint8_t)(v < 0 ? 0 : v > 1 ? 255
                                           : v * 255.f + .5f);
    };
    return (uint32_t)sat(c.r) | ((uint32_t)sat(c.g) << 8) | ((uint32_t)sat(c.b) << 16) | ((uint32_t)sat(a) << 24);
}

void DrawShapes::Canvas::Resize(int _w, int _h)
{
    w = _w;
    h = _h;
    buf.assign(w * h, 0u);
}

void DrawShapes::Canvas::Clear()
{
    std::fill(buf.begin(), buf.end(), 0u);
}

void DrawShapes::Canvas::SetPixel(int x, int y, uint32_t rgba)
{
    if (x < 0 || y < 0 || x >= w || y >= h)
        return;
    buf[y * w + x] = rgba;
}

uint32_t DrawShapes::Canvas::BlendOver(uint32_t dst, uint32_t src) const
{
    uint8_t sa = (src >> 24) & 0xff;
    if (sa == 0)
        return dst;
    if (sa == 255)
        return src;
    float fa = sa / 255.f;
    float ia = 1.f - fa;
    auto ch = [&](int shift) -> uint8_t
    {
        float s = ((src >> shift) & 0xff) / 255.f;
        float d = ((dst >> shift) & 0xff) / 255.f;
        return (uint8_t)((s * fa + d * ia) * 255.f + .5f);
    };
    uint8_t da = (dst >> 24) & 0xff;
    uint8_t ra = (uint8_t)(sa + (uint8_t)(da * ia + .5f));
    return (uint32_t)ch(0) | ((uint32_t)ch(8) << 8) | ((uint32_t)ch(16) << 16) | ((uint32_t)ra << 24);
}

void DrawShapes::Canvas::SetPixelBlend(int x, int y, uint32_t src)
{
    if (x < 0 || y < 0 || x >= w || y >= h)
        return;
    uint32_t &dst = buf[y * w + x];
    dst = BlendOver(dst, src);
}

static bool UploadPixelsToExistingRes(ID3D11Device *dev,
                                      ID3D11DeviceContext *ctx,
                                      TextureResource *res,
                                      int w, int h,
                                      const std::vector<uint32_t> &pixels)
{
    if (res->texture)
    {
        D3D11_TEXTURE2D_DESC d;
        res->texture->GetDesc(&d);
        if ((int)d.Width != w || (int)d.Height != h)
        {
            res->texture.Reset();
            res->srv.Reset();
        }
    }
    if (!res->texture)
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = w;
        desc.Height = h;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        HRESULT hr = dev->CreateTexture2D(&desc, nullptr, &res->texture);
        if (FAILED(hr))
            return false;
        hr = dev->CreateShaderResourceView(res->texture.Get(), nullptr, &res->srv);
        if (FAILED(hr))
            return false;
    }
    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = ctx->Map(res->texture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr))
        return false;
    for (int row = 0; row < h; ++row)
        memcpy((uint8_t *)mapped.pData + row * mapped.RowPitch,
               pixels.data() + row * w, w * 4);
    ctx->Unmap(res->texture.Get(), 0);
    return true;
}

DrawShapes::DrawShapes(DirectXCore *dx) : m_dx(dx) {}

void DrawShapes::BeginFrame()
{
    for (auto &t : m_retireList)
        m_freePool.push_back(std::move(t));
    m_retireList.clear();
}

void DrawShapes::EndFrame()
{
    for (auto &t : m_inUseList)
        m_retireList.push_back(std::move(t));
    m_inUseList.clear();
}

TextureResource *DrawShapes::AcquireTempTexture(int w, int h,
                                                const std::vector<uint32_t> &pixels)
{
    if (!m_dx || w <= 0 || h <= 0)
        return nullptr;

    ID3D11Device *dev = m_dx->GetDevice();
    ID3D11DeviceContext *ctx = m_dx->GetContext();
    if (!dev || !ctx)
        return nullptr;

    TempTex slot;
    for (int i = 0; i < (int)m_freePool.size(); ++i)
    {
        if (m_freePool[i].w == w && m_freePool[i].h == h)
        {
            slot = std::move(m_freePool[i]);
            m_freePool.erase(m_freePool.begin() + i);
            break;
        }
    }

    if (!slot.res)
    {
        slot.res = std::make_unique<TextureResource>();
        slot.res->bIsIndexTexture = false;
        slot.w = w;
        slot.h = h;
    }

    if (!UploadPixelsToExistingRes(dev, ctx, slot.res.get(), w, h, pixels))
        return nullptr;

    slot.res->sourceView.FullWidth = w;
    slot.res->sourceView.FullHeight = h;
    slot.w = w;
    slot.h = h;

    TextureResource *ret = slot.res.get();
    m_inUseList.push_back(std::move(slot));
    return ret;
}

void DrawShapes::FlushCanvas(Canvas &c, float worldX, float worldY,
                             float opacity, bool bScreenSpace)
{
    TextureResource *tex = AcquireTempTexture(c.w, c.h, c.buf);
    if (!tex)
        return;

    tex->sourceView.FullWidth = c.w;
    tex->sourceView.FullHeight = c.h;

    DrawParams p;
    p.x = worldX;
    p.y = worldY;
    p.opacity = opacity;
    if (bScreenSpace)
        p.SetScreenSpace();

    m_dx->DrawTexture(tex, p);
}

void DrawShapes::RasterThickPoint(Canvas &c, float px, float py,
                                  float radius, uint32_t rgba)
{
    int x0 = (int)std::floor(px - radius - 1.f);
    int x1 = (int)std::ceil(px + radius + 1.f);
    int y0 = (int)std::floor(py - radius - 1.f);
    int y1 = (int)std::ceil(py + radius + 1.f);

    uint8_t srcA = (rgba >> 24) & 0xff;
    float fR = (rgba >> 0) & 0xff;
    float fG = (rgba >> 8) & 0xff;
    float fB = (rgba >> 16) & 0xff;

    for (int y = y0; y <= y1; ++y)
    {
        for (int x = x0; x <= x1; ++x)
        {
            float dx = x - px, dy = y - py;
            float dist = std::sqrt(dx * dx + dy * dy);
            float cov = std::max(0.f, std::min(1.f, radius - dist + .5f));
            if (cov < 1e-4f)
                continue;
            uint8_t a = (uint8_t)(srcA * cov + .5f);
            uint32_t col = (uint32_t)(fR) | ((uint32_t)(fG) << 8) | ((uint32_t)(fB) << 16) | ((uint32_t)a << 24);
            c.SetPixelBlend(x, y, col);
        }
    }
}

void DrawShapes::RasterEllipseFill(Canvas &c, float cx, float cy,
                                   float rx, float ry, uint32_t rgba)
{
    if (rx < .5f || ry < .5f)
        return;
    int y0 = (int)std::floor(cy - ry);
    int y1 = (int)std::ceil(cy + ry);
    for (int y = y0; y <= y1; ++y)
    {
        float dy = y - cy;
        float t = dy / ry;
        if (std::abs(t) > 1.f)
            continue;
        float halfW = rx * std::sqrt(1.f - t * t);
        int xL = (int)std::ceil(cx - halfW);
        int xR = (int)std::floor(cx + halfW);
        for (int x = xL; x <= xR; ++x)
            c.SetPixel(x, y, rgba);
    }
}

void DrawShapes::RasterLine(Canvas &c,
                            float x0, float y0, float x1, float y1,
                            float thickness,
                            ShapeColor color,
                            float dashLen, float gapLen,
                            bool antiAlias)
{
    float dx = x1 - x0, dy = y1 - y0;
    float len = std::sqrt(dx * dx + dy * dy);
    uint32_t rgba = ColorToU32(color);

    if (len < 1e-4f)
    {
        if (antiAlias)
        {
            RasterThickPoint(c, x0, y0, thickness * .5f, rgba);
        }
        else
        {
            int r = (int)std::ceil(thickness * .5f);
            int cx = (int)std::round(x0), cy = (int)std::round(y0);
            for (int dy2 = -r; dy2 <= r; ++dy2)
                for (int dx2 = -r; dx2 <= r; ++dx2)
                    c.SetPixel(cx + dx2, cy + dy2, rgba);
        }
        return;
    }

    // ── Non-AA path: filled quad with hard square caps ──
    if (!antiAlias)
    {
        float halfT = thickness * 0.5f;
        float invLen = 1.0f / len;
        float nx = -dy * invLen * halfT;
        float ny = dx * invLen * halfT;

        auto drawQuad = [&](float sx, float sy, float ex, float ey)
        {
            // CCW winding: (sx-nx,sy-ny) -> (ex-nx,ey-ny) -> (ex+nx,ey+ny) -> (sx+nx,sy+ny)
            float qx[4] = {sx - nx, ex - nx, ex + nx, sx + nx};
            float qy[4] = {sy - ny, ey - ny, ey + ny, sy + ny};
            int minX = (int)std::floor(std::min({qx[0], qx[1], qx[2], qx[3]}));
            int maxX = (int)std::ceil(std::max({qx[0], qx[1], qx[2], qx[3]}));
            int minY = (int)std::floor(std::min({qy[0], qy[1], qy[2], qy[3]}));
            int maxY = (int)std::ceil(std::max({qy[0], qy[1], qy[2], qy[3]}));
            for (int y = minY; y <= maxY; ++y)
            {
                for (int x = minX; x <= maxX; ++x)
                {
                    float px = (float)x + 0.5f, py = (float)y + 0.5f;
                    bool inside = true;
                    for (int i = 0; i < 4; ++i)
                    {
                        int j = (i + 1) % 4;
                        float ex2 = qx[j] - qx[i], ey2 = qy[j] - qy[i];
                        float vx = px - qx[i], vy = py - qy[i];
                        if (ex2 * vy - ey2 * vx < 0)
                        {
                            inside = false;
                            break;
                        }
                    }
                    if (inside)
                        c.SetPixel(x, y, rgba);
                }
            }
        };

        bool useDash = (dashLen > 0.f && gapLen > 0.f);
        if (!useDash)
        {
            drawQuad(x0, y0, x1, y1);
        }
        else
        {
            float cycleLen = dashLen + gapLen;
            float pos = 0.f;
            while (pos < len)
            {
                float dashEnd = std::min(pos + dashLen, len);
                float t0 = pos / len, t1 = dashEnd / len;
                drawQuad(x0 + dx * t0, y0 + dy * t0, x0 + dx * t1, y0 + dy * t1);
                pos += cycleLen;
            }
        }
        return;
    }

    // ── AA path: overlapping anti-aliased circles ──
    bool useDash = (dashLen > 0.f && gapLen > 0.f);
    float cycleLen = dashLen + gapLen;
    float radius = thickness * .5f;

    float step = 0.5f;
    int steps = (int)std::ceil(len / step);
    float dashPos = 0.f;

    for (int i = 0; i <= steps; ++i)
    {
        float t = (float)i / (float)steps;
        float px = x0 + dx * t;
        float py = y0 + dy * t;
        float distFromStart = t * len;

        if (useDash)
        {
            dashPos = std::fmod(distFromStart, cycleLen);
            if (dashPos > dashLen)
                continue;
        }
        RasterThickPoint(c, px, py, radius, rgba);
    }
}

void DrawShapes::DrawLine(float x0, float y0, float x1, float y1,
                          const LineParams &params)
{
    float minX = std::min(x0, x1);
    float minY = std::min(y0, y1);
    float maxX = std::max(x0, x1);
    float maxY = std::max(y0, y1);

    float pad = params.thickness * .5f + 2.f;
    float originX = std::floor(minX - pad);
    float originY = std::floor(minY - pad);
    int w = (int)std::ceil(maxX - originX + pad) + 1;
    int h = (int)std::ceil(maxY - originY + pad) + 1;
    if (w <= 0 || h <= 0)
        return;

    Canvas canvas;
    canvas.Resize(w, h);

    ShapeColor col = params.color;
    col.a *= params.opacity;

    RasterLine(canvas,
               x0 - originX, y0 - originY,
               x1 - originX, y1 - originY,
               params.thickness, col,
               params.dashLength, params.gapLength,
               params.antiAlias);

    FlushCanvas(canvas, originX, originY, 1.f, params.bScreenSpace);
}

void DrawShapes::DrawRect(float x, float y, float w, float h,
                          const RectParams &params)
{
    if (w < 1.f || h < 1.f)
        return;

    float pad = params.borderWidth * .5f + 2.f;
    float originX = std::floor(x - pad);
    float originY = std::floor(y - pad);
    int cw = (int)std::ceil(w + pad * 2.f) + 2;
    int ch = (int)std::ceil(h + pad * 2.f) + 2;

    Canvas canvas;
    canvas.Resize(cw, ch);

    float lx = x - originX, ly = y - originY;
    float rx = lx + w, ry = ly + h;

    if (!params.fillColor.IsTransparent())
    {
        ShapeColor fc = params.fillColor;
        fc.a *= params.opacity;
        uint32_t fillRGBA = ColorToU32(fc);
        for (int py = (int)std::ceil(ly); py <= (int)std::floor(ry); ++py)
            for (int px = (int)std::ceil(lx); px <= (int)std::floor(rx); ++px)
                canvas.SetPixel(px, py, fillRGBA);
    }

    if (params.borderWidth > 0.f && !params.borderColor.IsTransparent())
    {
        ShapeColor bc = params.borderColor;
        bc.a *= params.opacity;

        struct Seg
        {
            float ax, ay, bx, by;
        };
        Seg segs[4] = {
            {lx, ly, rx, ly}, // top
            {rx, ly, rx, ry}, // right
            {rx, ry, lx, ry}, // bottom
            {lx, ry, lx, ly}, // left
        };

        if (params.dashLength > 0.f && params.gapLength > 0.f)
        {
            float dashOffset = 0.f;
            float cycleLen = params.dashLength + params.gapLength;
            for (auto &s : segs)
            {
                float segDx = s.bx - s.ax, segDy = s.by - s.ay;
                float segLen = std::sqrt(segDx * segDx + segDy * segDy);
                float step = 0.5f;
                int steps = (int)std::ceil(segLen / step);
                float r = params.borderWidth * .5f;
                uint32_t rgba = ColorToU32(bc);
                for (int i = 0; i <= steps; ++i)
                {
                    float t = (float)i / (float)(steps == 0 ? 1 : steps);
                    float dist = t * segLen + dashOffset;
                    float pos = std::fmod(dist, cycleLen);
                    if (pos > params.dashLength)
                        continue;
                    float px = s.ax + segDx * t;
                    float py = s.ay + segDy * t;
                    RasterThickPoint(canvas, px, py, r, rgba);
                }
                dashOffset = std::fmod(dashOffset + segLen, cycleLen);
            }
        }
        else
        {
            for (auto &s : segs)
                RasterLine(canvas, s.ax, s.ay, s.bx, s.by,
                           params.borderWidth, bc, 0, 0, false);
        }
    }

    FlushCanvas(canvas, originX, originY, 1.f, params.bScreenSpace);
}

void DrawShapes::DrawEllipse(float cx, float cy, float rx, float ry,
                             const EllipseParams &params)
{
    if (rx < .5f || ry < .5f)
        return;

    float pad = params.borderWidth * .5f + 2.f;
    float originX = std::floor(cx - rx - pad);
    float originY = std::floor(cy - ry - pad);
    int cw = (int)std::ceil((rx + pad) * 2.f) + 2;
    int ch = (int)std::ceil((ry + pad) * 2.f) + 2;

    Canvas canvas;
    canvas.Resize(cw, ch);

    float lcx = cx - originX;
    float lcy = cy - originY;

    if (!params.fillColor.IsTransparent())
    {
        ShapeColor fc = params.fillColor;
        fc.a *= params.opacity;
        RasterEllipseFill(canvas, lcx, lcy, rx, ry, ColorToU32(fc));
    }

    if (params.borderWidth > 0.f && !params.borderColor.IsTransparent())
    {
        ShapeColor bc = params.borderColor;
        bc.a *= params.opacity;
        uint32_t rgba = ColorToU32(bc);

        int segs = params.segments > 0
                       ? params.segments
                       : std::max(32, (int)(2.f * kPi * std::max(rx, ry) / 1.5f));

        float cycleLen = params.dashLength + params.gapLength;
        bool useDash = (params.dashLength > 0.f && params.gapLength > 0.f);
        float dashAccum = 0.f;
        float halfBorder = params.borderWidth * .5f;

        float prevPx = 0.f, prevPy = 0.f;
        for (int i = 0; i <= segs; ++i)
        {
            float angle = 2.f * kPi * i / segs;
            float px = lcx + rx * std::cos(angle);
            float py = lcy + ry * std::sin(angle);

            if (i == 0)
            {
                prevPx = px;
                prevPy = py;
                continue;
            }

            float sdx = px - prevPx, sdy = py - prevPy;
            float segLen = std::sqrt(sdx * sdx + sdy * sdy);

            if (useDash)
            {
                float step = 0.5f;
                int steps = std::max(1, (int)std::ceil(segLen / step));
                for (int j = 0; j <= steps; ++j)
                {
                    float t = (float)j / steps;
                    float qx = prevPx + sdx * t;
                    float qy = prevPy + sdy * t;
                    float pos = std::fmod(dashAccum + t * segLen, cycleLen);
                    if (pos <= params.dashLength)
                        RasterThickPoint(canvas, qx, qy, halfBorder, rgba);
                }
                dashAccum = std::fmod(dashAccum + segLen, cycleLen);
            }
            else
            {
                RasterLine(canvas, prevPx, prevPy, px, py,
                           params.borderWidth, bc, 0, 0, false);
            }
            prevPx = px;
            prevPy = py;
        }
    }

    FlushCanvas(canvas, originX, originY, 1.f, params.bScreenSpace);
}

static std::wstring ToWide(const std::string &s)
{
    if (s.empty())
        return {};
    int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (len <= 0)
        return {};
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), w.data(), len);
    return w;
}

static uint32_t PackColor(const ShapeColor &c)
{
    auto sat = [](float v) -> uint8_t
    {
        return (uint8_t)(v < 0 ? 0 : v > 1 ? 255
                                           : v * 255.f + .5f);
    };
    return (uint32_t)sat(c.r) | ((uint32_t)sat(c.g) << 8) | ((uint32_t)sat(c.b) << 16) | ((uint32_t)sat(c.a) << 24);
}

static COLORREF ToColorRef(const ShapeColor &c)
{
    auto sat = [](float v) -> uint8_t
    {
        return (uint8_t)(v < 0 ? 0 : v > 1 ? 255
                                           : v * 255.f + .5f);
    };
    return RGB(sat(c.r), sat(c.g), sat(c.b));
}

TextRenderer::TextRenderer(DirectXCore *dx, size_t cacheCapacity)
    : m_dx(dx), m_capacity(cacheCapacity)
{
}

TextRenderer::~TextRenderer()
{
    ClearCache();
    CleanupFonts();

    if (m_hBmp)
    {
        DeleteObject(m_hBmp);
        m_hBmp = nullptr;
    }
    if (m_hDC)
    {
        DeleteDC(m_hDC);
        m_hDC = nullptr;
    }
}

void TextRenderer::DrawTexts(
    float x,
    float y,
    FString_view text,
    const TextParams &params)
{
    if (text.empty())
        return;

    TextKey key = MakeKey(text, params);
    TextureResource *tex = Lookup(key);

    if (!tex)
    {
        std::vector<uint32_t> pixels;

        int w = 0;
        int h = 0;

        if (!RasterizeGDI(text, params, pixels, w, h))
            return;

        CacheEntry entry;

        entry.texW = w;
        entry.texH = h;

        entry.texRes = std::make_unique<TextureResource>();

        entry.texRes->bIsIndexTexture = false;

        entry.texRes->sourceView.FullWidth = w;
        entry.texRes->sourceView.FullHeight = h;

        if (!UploadTexture(entry.texRes.get(), w, h, pixels))
            return;

        tex = entry.texRes.get();

        Insert(key, std::move(entry));
    }

    float drawX = x;

    const float texW =
        (float)tex->sourceView.FullWidth;

    switch (params.align)
    {
    default:
    case TextAlign::Left:
        break;

    case TextAlign::Center:
        drawX -= texW * 0.5f;
        break;

    case TextAlign::Right:
        drawX -= texW;
        break;
    }

    DrawParams dp;

    dp.x = drawX;
    dp.y = y;

    dp.opacity = params.opacity;

    if (params.bScreenSpace)
        dp.SetScreenSpace();

    m_dx->DrawTexture(tex, dp);
}

bool TextRenderer::MeasureText(const FString &text,
                               const TextParams &params,
                               int &outW, int &outH)
{
    if (text.empty())
    {
        outW = outH = 0;
        return true;
    }

    TextKey key = MakeKey(text, params);
    auto it = m_cache.find(key);
    if (it != m_cache.end())
    {
        outW = it->second.first.texW;
        outH = it->second.first.texH;
        return true;
    }

    HFONT hFont = GetOrCreateFont(params);
    if (!hFont)
        return false;

    std::wstring wtext = ToWide(text);
    HDC hdc = CreateCompatibleDC(nullptr);
    HGDIOBJ oldFont = SelectObject(hdc, hFont);

    UINT dtFlags =
        DT_CALCRECT |
        DT_TOP |
        DT_NOPREFIX |
        DT_WORDBREAK;

    switch (params.align)
    {
    case TextAlign::Left:
        dtFlags |= DT_LEFT;
        break;

    case TextAlign::Center:
        dtFlags |= DT_CENTER;
        break;

    case TextAlign::Right:
        dtFlags |= DT_RIGHT;
        break;
    }

    RECT rc = {0, 0, 4096, 4096};
    DrawTextW(hdc, wtext.c_str(), (int)wtext.size(), &rc,
              dtFlags);

    outW = (rc.right - rc.left) + params.paddingX * 2;
    outH = (rc.bottom - rc.top) + params.paddingY * 2;
    if (outW < 1)
        outW = 1;
    if (outH < 1)
        outH = 1;

    SelectObject(hdc, oldFont);
    DeleteDC(hdc);
    return true;
}

void TextRenderer::ClearCache()
{
    m_cache.clear();
    m_lruList.clear();
}

TextKey TextRenderer::MakeKey(const FString &text,
                              const TextParams &p) const
{
    TextKey k;
    k.text = text;
    k.fontName = p.fontName;
    k.fontSize = p.fontSize;
    k.bold = p.bold;
    k.italic = p.italic;
    k.underline = p.underline;
    k.colorRGBA = PackColor(p.color);
    k.bgColorRGBA = PackColor(p.bgColor);
    k.paddingX = p.paddingX;
    k.paddingY = p.paddingY;
    return k;
}

TextureResource *TextRenderer::Lookup(const TextKey &key)
{
    auto it = m_cache.find(key);
    if (it == m_cache.end())
        return nullptr;

    m_lruList.splice(m_lruList.begin(), m_lruList, it->second.second);
    return it->second.first.texRes.get();
}

void TextRenderer::Insert(const TextKey &key, CacheEntry entry)
{
    if (m_capacity > 0 && m_cache.size() >= m_capacity)
        Evict();

    m_lruList.push_front(key);
    m_cache.emplace(key, std::make_pair(std::move(entry), m_lruList.begin()));
}

void TextRenderer::Evict()
{
    if (m_lruList.empty())
        return;
    const TextKey &oldest = m_lruList.back();
    m_cache.erase(oldest);
    m_lruList.pop_back();
}

HFONT TextRenderer::GetOrCreateFont(const TextParams &p)
{
    FontKey fk;
    fk.fontName = p.fontName;
    fk.fontSize = p.fontSize;
    fk.bold = p.bold;
    fk.italic = p.italic;
    fk.underline = p.underline;

    auto it = m_fontPool.find(fk);
    if (it != m_fontPool.end())
        return it->second;

    std::wstring wFontName = ToWide(p.fontName);

    LOGFONTW lf = {};
    lf.lfHeight = -p.fontSize;
    lf.lfWeight = p.bold ? FW_BOLD : FW_NORMAL;
    lf.lfItalic = p.italic ? TRUE : FALSE;
    lf.lfUnderline = p.underline ? TRUE : FALSE;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = OUT_TT_PRECIS;
    lf.lfQuality = p.fontSize >= 20 ? CLEARTYPE_NATURAL_QUALITY : NONANTIALIASED_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    wcsncpy_s(lf.lfFaceName, wFontName.c_str(), LF_FACESIZE - 1);

    HFONT hFont = CreateFontIndirectW(&lf);
    if (hFont)
        m_fontPool[fk] = hFont;
    return hFont;
}

void TextRenderer::CleanupFonts()
{
    for (auto &[k, hf] : m_fontPool)
        if (hf)
            DeleteObject(hf);
    m_fontPool.clear();
}

void TextRenderer::EnsureDC(int w, int h)
{
    if (!m_hDC)
    {
        m_hDC = CreateCompatibleDC(nullptr);
        SetBkMode(m_hDC, TRANSPARENT);
    }

    if (w <= m_bmpW && h <= m_bmpH)
        return;

    if (m_hBmp)
    {
        SelectObject(m_hDC, GetStockObject(DEFAULT_GUI_FONT));
        DeleteObject(m_hBmp);
        m_hBmp = nullptr;
        m_pBits = nullptr;
    }

    m_bmpW = std::max(w, m_bmpW + 64);
    m_bmpH = std::max(h, m_bmpH + 64);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = m_bmpW;
    bmi.bmiHeader.biHeight = -m_bmpH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    m_hBmp = CreateDIBSection(m_hDC, &bmi, DIB_RGB_COLORS, &m_pBits, nullptr, 0);
    SelectObject(m_hDC, m_hBmp);
}

bool TextRenderer::RasterizeGDI(
    const FString &text,
    const TextParams &p,
    std::vector<uint32_t> &pixels,
    int &outW,
    int &outH)
{
    HFONT hFont = GetOrCreateFont(p);
    if (!hFont)
        return false;

    std::wstring wtext = ToWide(text);
    if (wtext.empty())
        return false;

    HDC hMeasureDC = CreateCompatibleDC(nullptr);
    HGDIOBJ oldMeasureFont = SelectObject(hMeasureDC, hFont);

    RECT rcMeasure = {0, 0, 4096, 4096};

    UINT dtFlags =
        DT_CALCRECT |
        DT_TOP |
        DT_NOPREFIX |
        DT_WORDBREAK;

    switch (p.align)
    {
    case TextAlign::Left:
        dtFlags |= DT_LEFT;
        break;

    case TextAlign::Center:
        dtFlags |= DT_CENTER;
        break;

    case TextAlign::Right:
        dtFlags |= DT_RIGHT;
        break;
    }

    DrawTextW(
        hMeasureDC,
        wtext.c_str(),
        (int)wtext.size(),
        &rcMeasure,
        dtFlags);

    SelectObject(hMeasureDC, oldMeasureFont);
    DeleteDC(hMeasureDC);

    int textW = rcMeasure.right - rcMeasure.left;
    int textH = rcMeasure.bottom - rcMeasure.top;

    outW = std::max(1, textW + p.paddingX * 2);
    outH = std::max(1, textH + p.paddingY * 2);

    EnsureDC(outW, outH);

    RECT drawRc =
        {
            p.paddingX,
            p.paddingY,
            p.paddingX + textW,
            p.paddingY + textH};

    {
        uint32_t *dst = reinterpret_cast<uint32_t *>(m_pBits);

        for (int y = 0; y < outH; ++y)
        {
            for (int x = 0; x < outW; ++x)
            {
                dst[y * m_bmpW + x] = 0xFF000000u;
            }
        }
    }

    {
        HGDIOBJ oldFont = SelectObject(m_hDC, hFont);

        SetBkMode(m_hDC, TRANSPARENT);

        // VERY IMPORTANT:
        // use white text only
        SetTextColor(m_hDC, RGB(255, 255, 255));

        dtFlags ^= DT_CALCRECT;
        DrawTextW(
            m_hDC,
            wtext.c_str(),
            (int)wtext.size(),
            &drawRc,
            dtFlags);

        SelectObject(m_hDC, oldFont);

        GdiFlush();
    }

    auto Clamp255 = [](float v) -> uint8_t
    {
        if (v <= 0.f)
            return 0;
        if (v >= 1.f)
            return 255;
        return (uint8_t)(v * 255.f + 0.5f);
    };

    uint8_t fgR = Clamp255(p.color.r);
    uint8_t fgG = Clamp255(p.color.g);
    uint8_t fgB = Clamp255(p.color.b);

    uint8_t bgR = 0;
    uint8_t bgG = 0;
    uint8_t bgB = 0;
    uint8_t bgA = 0;

    bool hasBg = !p.bgColor.IsTransparent();

    if (hasBg)
    {
        bgR = Clamp255(p.bgColor.r);
        bgG = Clamp255(p.bgColor.g);
        bgB = Clamp255(p.bgColor.b);
        bgA = Clamp255(p.bgColor.a);
    }

    float globalAlpha = p.color.a * p.opacity;

    pixels.resize(outW * outH);

    const uint32_t *src =
        reinterpret_cast<const uint32_t *>(m_pBits);

    if (!hasBg)
    {
        for (int y = 0; y < outH; ++y)
        {
            uint32_t *dst = pixels.data() + y * outW;
            const uint32_t *row = src + y * m_bmpW;

            for (int x = 0; x < outW; ++x)
            {
                uint32_t px = row[x];

                // white-on-black
                uint8_t coverage =
                    (uint8_t)((px >> 16) & 0xFF);

                float alpha =
                    (coverage / 255.f) * globalAlpha;

                uint8_t outA =
                    (uint8_t)(alpha * 255.f + 0.5f);

                bool isBorder =
                    p.bBorder &&
                    (x < p.borderThickness ||
                     y < p.borderThickness ||
                     x >= outW - p.borderThickness ||
                     y >= outH - p.borderThickness);

                if (isBorder)
                {
                    dst[x] =
                        (uint32_t)fgR |
                        ((uint32_t)fgG << 8) |
                        ((uint32_t)fgB << 16) |
                        (255u << 24);
                }
                else
                {
                    dst[x] =
                        (uint32_t)fgR |
                        ((uint32_t)fgG << 8) |
                        ((uint32_t)fgB << 16) |
                        ((uint32_t)outA << 24);
                }
            }
        }

        return true;
    }

    for (int y = 0; y < outH; ++y)
    {
        uint32_t *dst = pixels.data() + y * outW;
        const uint32_t *row = src + y * m_bmpW;

        for (int x = 0; x < outW; ++x)
        {
            uint32_t px = row[x];

            uint8_t coverage8 =
                (uint8_t)((px >> 16) & 0xFF);

            float coverage =
                (coverage8 / 255.f) * globalAlpha;

            float inv =
                1.f - coverage;

            uint8_t outR =
                (uint8_t)(bgR * inv + fgR * coverage + 0.5f);

            uint8_t outG =
                (uint8_t)(bgG * inv + fgG * coverage + 0.5f);

            uint8_t outB =
                (uint8_t)(bgB * inv + fgB * coverage + 0.5f);

            uint8_t outA =
                (uint8_t)(bgA * inv + 255.f * coverage + 0.5f);

            bool isBorder =
                p.bBorder &&
                (x < p.borderThickness ||
                 y < p.borderThickness ||
                 x >= outW - p.borderThickness ||
                 y >= outH - p.borderThickness);
            if (isBorder)
            {
                dst[x] =
                    (uint32_t)fgR |
                    ((uint32_t)fgG << 8) |
                    ((uint32_t)fgB << 16) |
                    (255u << 24);

                continue;
            }

            dst[x] =
                (uint32_t)outR |
                ((uint32_t)outG << 8) |
                ((uint32_t)outB << 16) |
                ((uint32_t)outA << 24);
        }
    }

    return true;
}

bool TextRenderer::UploadTexture(TextureResource *res,
                                 int w, int h,
                                 const std::vector<uint32_t> &pixels)
{
    if (!m_dx)
        return false;
    ID3D11Device *dev = m_dx->GetDevice();
    ID3D11DeviceContext *ctx = m_dx->GetContext();
    if (!dev || !ctx)
        return false;

    if (res->texture)
    {
        D3D11_TEXTURE2D_DESC d;
        res->texture->GetDesc(&d);
        if ((int)d.Width != w || (int)d.Height != h)
        {
            res->texture.Reset();
            res->srv.Reset();
        }
    }

    if (!res->texture)
    {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = w;
        desc.Height = h;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hr = dev->CreateTexture2D(&desc, nullptr, &res->texture);
        if (FAILED(hr))
            return false;

        hr = dev->CreateShaderResourceView(res->texture.Get(), nullptr, &res->srv);
        if (FAILED(hr))
            return false;
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = ctx->Map(res->texture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (FAILED(hr))
        return false;

    for (int row = 0; row < h; ++row)
    {
        memcpy(static_cast<uint8_t *>(mapped.pData) + row * mapped.RowPitch,
               pixels.data() + row * w,
               w * 4);
    }
    ctx->Unmap(res->texture.Get(), 0);

    res->sourceView.FullWidth = w;
    res->sourceView.FullHeight = h;
    return true;
}
