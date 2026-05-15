#include <DirectXMath.h>
#include <vector>
#include <cstring>
#include <string>
#include <algorithm>
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
    if (!CreateLineShaders())
    {
        Logger::Raw("[DirectXCore] CreateLineShaders failed\n");
        return false;
    }
    if (!CreateAlphaAccumShaders())
    {
        Logger::Raw("[DirectXCore] CreateAlphaAccumShaders failed\n");
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
    EnsureAlphaAccumTexture(vw, vh);

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
    m_pLineVS.Reset();
    m_pLinePS.Reset();
    m_pLineInputLayout.Reset();
    m_pLineVB.Reset();
    m_lineVBCapacity = 0;
    m_lineEntries.clear();

    m_pMRTPS.Reset();
    m_pLineModPS.Reset();
    m_pLineConstantBuffer.Reset();
    m_pAlphaAccumBlendState.Reset();
    m_pAlphaAccumTex.Reset();
    m_pAlphaAccumRTV.Reset();
    m_pAlphaAccumSRV.Reset();

    m_pOffscreenDSBuffer.Reset();
    m_pOffscreenDSV.Reset();
    m_pDepthStateGE.Reset();
    m_pDepthStateReadOnlyGE.Reset();
    m_pDepthStateOff.Reset();

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
    m_pOffscreenDSBuffer.Reset();
    m_pOffscreenDSV.Reset();
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

    // Clamp offscreen dimensions to D3D11 max texture size
    const UINT maxTexDim = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    if (vw > maxTexDim)
    {
        float clamp = (float)maxTexDim / vw;
        vw = maxTexDim;
        vh = (UINT)(vh * clamp);
    }
    if (vh > maxTexDim)
    {
        float clamp = (float)maxTexDim / vh;
        vh = maxTexDim;
        vw = (UINT)(vw * clamp);
    }
    if (vw == 0)
        vw = 1;
    if (vh == 0)
        vh = 1;

    CreateOffscreenResources(vw, vh);
    EnsureFactorTexture(vw, vh);
    EnsureScreenCopyTexture(vw, vh);
    EnsureAlphaAccumTexture(vw, vh);

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

float DirectXCore::SetZoomOut(float scaleFactor)
{
    if (scaleFactor < 0.01f)
        scaleFactor = 0.01f;

    // Clamp to D3D11 max texture dimension (16384 for feature level 11_0)
    const UINT maxTexDim = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    float maxSafeScaleX = (float)maxTexDim / std::max(1, m_clientWidth);
    float maxSafeScaleY = (float)maxTexDim / std::max(1, m_clientHeight);
    float maxSafeScale = std::min(maxSafeScaleX, maxSafeScaleY);
    if (scaleFactor > maxSafeScale)
        scaleFactor = maxSafeScale;

    if (m_renderScale == scaleFactor)
        return m_renderScale;

    UINT vw = (UINT)(m_clientWidth * scaleFactor);
    UINT vh = (UINT)(m_clientHeight * scaleFactor);
    if (vw == 0)
        vw = 1;
    if (vh == 0)
        vh = 1;

    m_OffscreenTex.Reset();
    m_OffscreenRTV.Reset();
    m_OffscreenSRV.Reset();
    m_pOffscreenDSBuffer.Reset();
    m_pOffscreenDSV.Reset();
    m_pFactorTexture.Reset();
    m_pFactorRTV.Reset();
    m_pFactorSRV.Reset();
    m_pScreenCopy.Reset();
    m_pScreenCopySRV.Reset();

    if (!CreateOffscreenResources(vw, vh))
    {
        Logger::Raw("[DirectXCore] SetZoomOut: CreateOffscreenResources failed, keeping old scale\n");
        return m_renderScale;
    }
    EnsureFactorTexture(vw, vh);
    EnsureScreenCopyTexture(vw, vh);
    EnsureAlphaAccumTexture(vw, vh);

    m_renderScale = scaleFactor;
    return m_renderScale;
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
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
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
            // Discard nearly transparent fragments so they don't
            // write to the depth buffer and occlude things behind.
            clip(finalAlpha - 1.0f / 255.0f);
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

    // Independent-blend state for MRT alpha accumulation:
    // RT0 = normal alpha blending, RT1 = overwrite (ONE, ZERO) with R-only mask.
    D3D11_BLEND_DESC accumBlendDesc = {};
    accumBlendDesc.AlphaToCoverageEnable = FALSE;
    accumBlendDesc.IndependentBlendEnable = TRUE;
    accumBlendDesc.RenderTarget[0].BlendEnable = TRUE;
    accumBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    accumBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    accumBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    accumBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    accumBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    accumBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    accumBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    accumBlendDesc.RenderTarget[1].BlendEnable = TRUE;
    accumBlendDesc.RenderTarget[1].SrcBlend = D3D11_BLEND_ONE;
    accumBlendDesc.RenderTarget[1].DestBlend = D3D11_BLEND_ZERO;
    accumBlendDesc.RenderTarget[1].BlendOp = D3D11_BLEND_OP_ADD;
    accumBlendDesc.RenderTarget[1].SrcBlendAlpha = D3D11_BLEND_ONE;
    accumBlendDesc.RenderTarget[1].DestBlendAlpha = D3D11_BLEND_ZERO;
    accumBlendDesc.RenderTarget[1].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    accumBlendDesc.RenderTarget[1].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED;
    m_pDevice->CreateBlendState(&accumBlendDesc, &m_pAlphaAccumBlendState);

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(CBPerObject);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    m_pDevice->CreateBuffer(&cbDesc, nullptr, &m_pConstantBuffer);

    // Small constant buffer for line shader: stores viewport size for UV calc.
    D3D11_BUFFER_DESC lineCbDesc = {};
    lineCbDesc.ByteWidth = sizeof(float) * 4; // float2 viewportSize + float2 pad
    lineCbDesc.Usage = D3D11_USAGE_DYNAMIC;
    lineCbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    lineCbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    m_pDevice->CreateBuffer(&lineCbDesc, nullptr, &m_pLineConstantBuffer);

    // Depth/stencil states
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
    dsDesc.StencilEnable = FALSE;
    m_pDevice->CreateDepthStencilState(&dsDesc, &m_pDepthStateGE);

    // Read-only depth state for semi-transparent textures:
    // They still participate in depth testing (get occluded by foreground)
    // but do NOT write depth, so they don't block things drawn later.
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    m_pDevice->CreateDepthStencilState(&dsDesc, &m_pDepthStateReadOnlyGE);

    dsDesc.DepthEnable = FALSE;
    m_pDevice->CreateDepthStencilState(&dsDesc, &m_pDepthStateOff);

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
    m_pOffscreenDSBuffer.Reset();
    m_pOffscreenDSV.Reset();

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
    if (FAILED(hr))
        return false;

    // Create depth/stencil buffer matching offscreen dimensions
    D3D11_TEXTURE2D_DESC dsDesc = {};
    dsDesc.Width = width;
    dsDesc.Height = height;
    dsDesc.MipLevels = 1;
    dsDesc.ArraySize = 1;
    dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsDesc.SampleDesc.Count = 1;
    dsDesc.Usage = D3D11_USAGE_DEFAULT;
    dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    hr = m_pDevice->CreateTexture2D(&dsDesc, nullptr, &m_pOffscreenDSBuffer);
    if (FAILED(hr))
        return false;
    hr = m_pDevice->CreateDepthStencilView(m_pOffscreenDSBuffer.Get(), nullptr, &m_pOffscreenDSV);
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

bool DirectXCore::CreateLineShaders()
{
    // Vertex shader: receives LineVertex { x, y, depth, color } as float4.
    // x,y are already in NDC; depth is the pre-assigned depth value.
    // Just pass through directly — no matrix transform needed.
    const char *vsCode = R"(
        struct VSInput {
            float4 pos_depth : POSITION;  // (ndcX, ndcY, depth, colorPacked)
            uint  color     : TEXCOORD0;
        };
        struct VSOutput {
            float4 pos : SV_POSITION;
            float4 col : COLOR0;
        };
        VSOutput main(VSInput input) {
            VSOutput output;
            output.pos = float4(input.pos_depth.xy, input.pos_depth.z, 1.0f);
            // Unpack RGBA8 to float4
            uint c = input.color;
            output.col = float4(
                (c & 0xff) / 255.0f,
                ((c >> 8) & 0xff) / 255.0f,
                ((c >> 16) & 0xff) / 255.0f,
                ((c >> 24) & 0xff) / 255.0f
            );
            return output;
        }
    )";
    const char *psCode = R"(
        struct PSInput {
            float4 pos : SV_POSITION;
            float4 col : COLOR0;
        };
        float4 main(PSInput input) : SV_Target {
            return input.col;
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

    hr = m_pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_pLineVS);
    if (FAILED(hr))
        return false;
    hr = m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pLinePS);
    if (FAILED(hr))
        return false;

    // Vertex layout: { float3 pos; uint color; } = 16 bytes / vertex
    // POSITION reads x,y,depth (3 floats).  HLSL float4 .w defaults to 1.0.
    // TEXCOORD reads the packed RGBA8 color at byte offset 12.
    D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32_UINT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = m_pDevice->CreateInputLayout(layoutDesc, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_pLineInputLayout);
    return SUCCEEDED(hr);
}

bool DirectXCore::CreateAlphaAccumShaders()
{
    // MRT pixel shader: outputs main color to SV_Target0 AND alpha
    // to SV_Target1 for the accumulation buffer.
    const char *mrtPS = R"(
        Texture2D tex : register(t0);
        SamplerState samp : register(s0);
        struct PSInput {
            float4 pos : SV_POSITION;
            float2 uv  : TEXCOORD0;
            float4 colorMul : COLOR0;
            float4 mixColor : COLOR1;
            float  mixFactor : TEXCOORD1;
        };
        struct PSOutput {
            float4 rt0 : SV_Target0;
            float4 rt1 : SV_Target1;
        };
        PSOutput main(PSInput input) {
            PSOutput o;
            float4 texColor = tex.Sample(samp, input.uv);
            float3 multRGB = texColor.rgb * input.colorMul.rgb;
            float3 finalRGB = lerp(multRGB, input.mixColor.rgb, input.mixFactor);
            float finalAlpha = texColor.a * input.colorMul.a;
            clip(finalAlpha - 1.0f / 255.0f);
            o.rt0 = float4(finalRGB, finalAlpha);
            o.rt1 = float4(finalAlpha, 0.0f, 0.0f, finalAlpha);
            return o;
        }
    )";

    // Modified line pixel shader: samples the alpha accumulation buffer
    // and attenuates line alpha by (1 - accumAlpha) so that lines behind
    // semi-transparent textures are partially visible ("behind glass").
    const char *lineModPS = R"(
        cbuffer LineCB : register(b1) {
            float2 viewportSize;
            float2 pad;
        };
        Texture2D<float> alphaAccum : register(t1);
        SamplerState samp : register(s0);
        float4 main(float4 pos : SV_POSITION, float4 col : COLOR0) : SV_Target {
            float2 uv = pos.xy / viewportSize;
            float accumAlpha = alphaAccum.Sample(samp, uv);
            float4 lineColor = col;
            lineColor.a *= (1.0f - accumAlpha);
            return lineColor;
        }
    )";

    ComPtr<ID3DBlob> psBlob, error;
    HRESULT hr;

    // Compile MRT PS
    hr = D3DCompile(mrtPS, strlen(mrtPS), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &error);
    if (FAILED(hr))
    {
        if (error) OutputDebugStringA((char *)error->GetBufferPointer());
        return false;
    }
    hr = m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pMRTPS);
    if (FAILED(hr)) return false;

    // Compile modified line PS
    hr = D3DCompile(lineModPS, strlen(lineModPS), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &error);
    if (FAILED(hr))
    {
        if (error) OutputDebugStringA((char *)error->GetBufferPointer());
        return false;
    }
    hr = m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pLineModPS);
    if (FAILED(hr)) return false;

    return true;
}

void DirectXCore::EnsureAlphaAccumTexture(UINT width, UINT height)
{
    if (m_pAlphaAccumTex && width == m_clientWidth && height == m_clientHeight)
        return;
    m_pAlphaAccumTex.Reset();
    m_pAlphaAccumRTV.Reset();
    m_pAlphaAccumSRV.Reset();

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = m_pDevice->CreateTexture2D(&desc, nullptr, &m_pAlphaAccumTex);
    if (FAILED(hr)) return;
    hr = m_pDevice->CreateRenderTargetView(m_pAlphaAccumTex.Get(), nullptr, &m_pAlphaAccumRTV);
    if (FAILED(hr)) return;
    hr = m_pDevice->CreateShaderResourceView(m_pAlphaAccumTex.Get(), nullptr, &m_pAlphaAccumSRV);
}

void DirectXCore::RenderOffscreenContent()
{
    float clearColor[4] = {
        CIsoViewExt::RenderingMap ? 0.0f : (ExtConfigs::EnableDarkMode ? 0.125f : 1.0f),
        CIsoViewExt::RenderingMap ? 0.0f : (ExtConfigs::EnableDarkMode ? 0.125f : 1.0f),
        CIsoViewExt::RenderingMap ? 0.0f : (ExtConfigs::EnableDarkMode ? 0.125f : 1.0f),
        1.0f};
    m_pContext->ClearRenderTargetView(m_OffscreenRTV.Get(), clearColor);
    m_pContext->ClearDepthStencilView(m_pOffscreenDSV.Get(), D3D11_CLEAR_DEPTH, 0.0f, 0);

    float vw = (float)(m_clientWidth * m_renderScale);
    float vh = (float)(m_clientHeight * m_renderScale);
    D3D11_VIEWPORT offscreenVP = {0, 0, vw, vh, 0.0f, 1.0f};
    m_pContext->RSSetViewports(1, &offscreenVP);

    m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
    m_pContext->VSSetShader(m_pVS.Get(), nullptr, 0);
    auto &offscreenSampler = (m_renderScale == 1.0f)
                                 ? m_pSamplerPoint
                                 : m_pSamplerLinear;
    m_pContext->PSSetSamplers(0, 1, offscreenSampler.GetAddressOf());
    m_pContext->IASetInputLayout(m_pInputLayout.Get());

    UINT stride = sizeof(QuadVertex);
    UINT offset = 0;
    m_pContext->IASetVertexBuffers(0, 1, m_pQuadVB.GetAddressOf(), &stride, &offset);
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // Map UINT depth [0..N] to clip-space Z [0..0.9999].
    // D24_UNORM maps [0..1] linearly; we reserve the full range.
    const float depthScale = 1.0f / 16777216.0f;

    auto CalcWorldMatrixNoGlobal = [&](const DrawParams &p, int texW, int texH, float depthZ) -> XMMATRIX
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
        XMMATRIX T = XMMatrixTranslation(ndc_centerX, ndc_centerY, depthZ);
        return S * T;
    };

    auto DrawOneTexture = [&](const DrawCommand &cmd)
    {
        TextureResource *tex = cmd.texRes;
        if (!tex || !tex->srv)
            return;
        const auto &p = cmd.params;
        float depthZ = cmd.depth * depthScale;
        XMMATRIX world = CalcWorldMatrixNoGlobal(p, tex->sourceView.FullWidth, tex->sourceView.FullHeight, depthZ);
        CBPerObject cb;
        cb.world = XMMatrixTranspose(world);
        cb.colorMul = XMFLOAT4(p.redMult, p.greenMult, p.blueMult, p.opacity);
        cb.mixColor = XMFLOAT4(p.mixR, p.mixG, p.mixB, 1.0f);
        cb.mixFactor = p.mixFactor;
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (FAILED(m_pContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            return;
        memcpy(mapped.pData, &cb, sizeof(cb));
        m_pContext->Unmap(m_pConstantBuffer.Get(), 0);
        m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
        m_pContext->PSSetShaderResources(0, 1, tex->srv.GetAddressOf());
        m_pContext->Draw(4, 0);
    };

    // ====================================================================
    // Phase 1: Draw ALL opaque textures (depth write ON, depth test ON)
    // ====================================================================
    m_pContext->OMSetRenderTargets(1, m_OffscreenRTV.GetAddressOf(), m_pOffscreenDSV.Get());
    m_pContext->OMSetDepthStencilState(m_pDepthStateGE.Get(), 0);
    m_pContext->PSSetShader(m_pPS.Get(), nullptr, 0);

    for (const auto &cmd : m_drawCommands)
    {
        if (cmd.bIsEffect || cmd.bScreenSpace)
            continue;
        if (!cmd.texRes || !cmd.texRes->srv || cmd.texRes->bIsIndexTexture)
            continue;
        if (cmd.params.opacity < 1.0f - 1e-6f)
            continue;
        DrawOneTexture(cmd);
    }

    // ====================================================================
    // Phase 2: Semi-transparent textures via MRT
    //   RT0 = Offscreen (main color with alpha blending)
    //   RT1 = AlphaAccum (raw alpha, overwrite, R-channel only)
    // ====================================================================
    bool hasTransparent = false;
    for (const auto &cmd : m_drawCommands)
    {
        if (cmd.bIsEffect || cmd.bScreenSpace) continue;
        if (cmd.params.opacity < 1.0f - 1e-6f)
        {
            hasTransparent = true;
            break;
        }
    }

    if (hasTransparent)
    {
        float zero[4] = {0, 0, 0, 0};
        m_pContext->ClearRenderTargetView(m_pAlphaAccumRTV.Get(), zero);

        ID3D11RenderTargetView *mrtRTs[2] = {m_OffscreenRTV.Get(), m_pAlphaAccumRTV.Get()};
        m_pContext->OMSetRenderTargets(2, mrtRTs, m_pOffscreenDSV.Get());
        m_pContext->OMSetBlendState(m_pAlphaAccumBlendState.Get(), nullptr, 0xffffffff);
        m_pContext->OMSetDepthStencilState(m_pDepthStateReadOnlyGE.Get(), 0);
        m_pContext->PSSetShader(m_pMRTPS.Get(), nullptr, 0);

        for (const auto &cmd : m_drawCommands)
        {
            if (cmd.bIsEffect || cmd.bScreenSpace)
                continue;
            if (!cmd.texRes || !cmd.texRes->srv || cmd.texRes->bIsIndexTexture)
                continue;
            if (cmd.params.opacity >= 1.0f - 1e-6f)
                continue;
            DrawOneTexture(cmd);
        }

        // Restore single RT and normal blend state
        m_pContext->OMSetRenderTargets(1, m_OffscreenRTV.GetAddressOf(), m_pOffscreenDSV.Get());
        m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
    }

    // ====================================================================
    // Phase 3: World-space lines with alpha modulation
    //   Lines sample the alpha accum buffer and attenuate output alpha
    //   by (1 - accumAlpha) so they appear "behind" semi-transparent texels.
    // ====================================================================
    {
        m_pContext->OMSetDepthStencilState(m_pDepthStateReadOnlyGE.Get(), 0);
        if (m_pAlphaAccumSRV)
            m_pContext->PSSetShaderResources(1, 1, m_pAlphaAccumSRV.GetAddressOf());

        // Update line constant buffer with viewport size
        float lineCBData[4] = {vw, vh, 0, 0};
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(m_pContext->Map(m_pLineConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            memcpy(mapped.pData, lineCBData, sizeof(lineCBData));
            m_pContext->Unmap(m_pLineConstantBuffer.Get(), 0);
        }
        m_pContext->VSSetConstantBuffers(1, 1, m_pLineConstantBuffer.GetAddressOf());
        m_pContext->PSSetConstantBuffers(1, 1, m_pLineConstantBuffer.GetAddressOf());

        FlushLineBatch(false, m_pLineModPS.Get());

        // Clean up extra SRV binding
        ID3D11ShaderResourceView *nullSRV = nullptr;
        m_pContext->PSSetShaderResources(1, 1, &nullSRV);
    }

    // ====================================================================
    // Restore state for effects pass
    // ====================================================================
    stride = sizeof(QuadVertex);
    offset = 0;
    m_pContext->IASetVertexBuffers(0, 1, m_pQuadVB.GetAddressOf(), &stride, &offset);
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_pContext->IASetInputLayout(m_pInputLayout.Get());
    m_pContext->VSSetShader(m_pVS.Get(), nullptr, 0);
    m_pContext->PSSetShader(m_pPS.Get(), nullptr, 0);
    m_pContext->PSSetSamplers(0, 1, (m_renderScale == 1.0f) ? m_pSamplerPoint.GetAddressOf() : m_pSamplerLinear.GetAddressOf());

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
        m_pContext->OMSetDepthStencilState(m_pDepthStateGE.Get(), 0);
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
            float depthZ = cmd.depth * depthScale;
            XMMATRIX world = CalcWorldMatrixNoGlobal(p, idxTex->sourceView.FullWidth, idxTex->sourceView.FullHeight, depthZ);
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
    D3D11_VIEWPORT screenVP = {0, 0, (float)m_clientWidth, (float)m_clientHeight, 0.0f, 1.0f};
    m_pContext->RSSetViewports(1, &screenVP);

    m_pContext->OMSetRenderTargets(1, m_pRTV.GetAddressOf(), nullptr);
    m_pContext->OMSetDepthStencilState(m_pDepthStateOff.Get(), 0);

    if (!m_drawCommands.empty())
    {
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

    // Flush GPU-batched screen-space lines
    m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
    FlushLineBatch(true);
}

void DirectXCore::Render()
{
    if (!m_bInitialized || !m_pContext || !m_pSwapChain || !m_pRTV)
        return;

    // If nothing to render at all, skip entirely
    if (m_drawCommands.empty() && m_lineEntries.empty())
        return;

    ResetDepth();

    RenderOffscreenContent();
    if (!CIsoViewExt::RenderingMap)
    {
        if (ExtConfigs::EnableDarkMode && ExtConfigs::EnableDarkMode_DimMap )
        {
            DarkenOffscreen(0.7f);
        }


        RenderFinalToBackBuffer();
        UpdateBackgroundCache();
        RenderScreenSpaceContent();
    }

    m_pSwapChain->Present(1, 0);
    m_drawCommands.clear();
}

void DirectXCore::DarkenOffscreen(float brightness)
{
    if (!m_pContext || !m_OffscreenRTV || !m_OffscreenTex || !m_pScreenCopy || !m_pFactorRTV)
        return;

    // Copy current offscreen content to screen-copy texture
    CopyScreenToTexture();

    // Clear factor texture to the constant darkening value (composite PS reads .r)
    float factor[4] = {brightness, brightness, brightness, 1.0f};
    m_pContext->ClearRenderTargetView(m_pFactorRTV.Get(), factor);

    // Composite: offscreen = screenCopy * factor (reuses existing composite shaders)
    m_pContext->OMSetRenderTargets(1, m_OffscreenRTV.GetAddressOf(), nullptr);
    m_pContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
    m_pContext->VSSetShader(m_pCompositeVS.Get(), nullptr, 0);
    m_pContext->PSSetShader(m_pCompositePS.Get(), nullptr, 0);
    m_pContext->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());

    ID3D11ShaderResourceView *srvs[2] = {m_pScreenCopySRV.Get(), m_pFactorSRV.Get()};
    m_pContext->PSSetShaderResources(0, 2, srvs);
    DrawFullscreenQuad();

    // Unbind SRVs so they don't interfere with subsequent draws
    ID3D11ShaderResourceView *nullSRV[2] = {nullptr, nullptr};
    m_pContext->PSSetShaderResources(0, 2, nullSRV);
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
    DrawCommand cmd;
    cmd.texRes = tex;
    cmd.params = params;
    cmd.bIsEffect = tex->bIsIndexTexture;
    cmd.bScreenSpace = params.bScreenSpace;
    if (params.drawDepth != -1)
    {
        cmd.depth = params.drawDepth;
    }
    else
    {
        cmd.depth = params.bScreenSpace ? 0 : GetNextDepth();
    }
    m_drawCommands.push_back(cmd);
}

int DirectXCore::DrawTexture(TextureResource* tex, const DrawParams& params, std::vector<int>& drawCommandIndices)
{
    drawCommandIndices.clear();
    if (!tex)
        return -1;

    // Calculate bounding rect of the new command
    float newL = params.x;
    float newT = params.y;
    float newR = params.x + tex->sourceView.FullWidth * params.scaleX;
    float newB = params.y + tex->sourceView.FullHeight * params.scaleY;

    // Ensure proper ordering (left <= right, top <= bottom)
    if (newL > newR) std::swap(newL, newR);
    if (newT > newB) std::swap(newT, newB);

    // Check overlap with each existing draw command
    for (int i = 0; i < (int)m_drawCommands.size(); ++i)
    {
        const auto& cmd = m_drawCommands[i];
        if (!cmd.texRes)
            continue;
        if (cmd.bScreenSpace != params.bScreenSpace)
            continue;

        float l = cmd.params.x;
        float t = cmd.params.y;
        float r = cmd.params.x + cmd.texRes->sourceView.FullWidth * cmd.params.scaleX;
        float b = cmd.params.y + cmd.texRes->sourceView.FullHeight * cmd.params.scaleY;

        if (l > r) std::swap(l, r);
        if (t > b) std::swap(t, b);

        // AABB overlap test
        if (newL < r && newR > l && newT < b && newB > t)
            drawCommandIndices.push_back(i);
    }

    // Queue the new command (same logic as the original DrawTexture)
    DrawCommand cmd;
    cmd.texRes = tex;
    cmd.params = params;
    cmd.bIsEffect = tex->bIsIndexTexture;
    cmd.bScreenSpace = params.bScreenSpace;
    if (params.drawDepth != -1)
    {
        cmd.depth = params.drawDepth;
    }
    else
    {
        cmd.depth = params.bScreenSpace ? 0 : GetNextDepth();
    }
    m_drawCommands.push_back(cmd);
    return (int)(m_drawCommands.size() - 1);
}

void DirectXCore::AddLineEntry(float x0, float y0, float x1, float y1,
                               uint32_t color, float thickness, UINT depth,
                               bool bScreenSpace)
{
    m_lineEntries.push_back({x0, y0, x1, y1, color, thickness, depth, bScreenSpace});
}

void DirectXCore::FlushLineBatch(bool bScreenSpace, ID3D11PixelShader *pCustomPS)
{
    if (!m_pDevice || !m_pContext)
        return;

    // Count matching entries
    int numLines = 0;
    for (const auto &le : m_lineEntries)
        if (le.bScreenSpace == bScreenSpace)
            ++numLines;

    if (numLines == 0)
        return;

    const int vertsPerLine = 6; // TRIANGLELIST: 2 triangles × 3 verts
    const int totalVerts = numLines * vertsPerLine;

    // Ensure vertex buffer is large enough
    if (!m_pLineVB || m_lineVBCapacity < totalVerts)
    {
        m_pLineVB.Reset();
        int newCapacity = (totalVerts + 1023) & ~1023;
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = newCapacity * 16;
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(m_pDevice->CreateBuffer(&bd, nullptr, &m_pLineVB)))
        {
            m_lineVBCapacity = 0;
            return;
        }
        m_lineVBCapacity = newCapacity;
    }

    // Build vertex data on stack then map-and-copy
    struct LineVertex { float x, y, depth; uint32_t color; }; // 16 bytes
    std::vector<LineVertex> verts;
    verts.reserve(totalVerts);

    // World-space lines use offscreen viewport; screen-space lines use window
    float vw, vh;
    if (bScreenSpace) {
        vw = (float)m_clientWidth;
        vh = (float)m_clientHeight;
    } else {
        vw = (float)(m_clientWidth * m_renderScale);
        vh = (float)(m_clientHeight * m_renderScale);
    }
    const float depthScale = 1.0f / 16777216.0f;

    for (const auto &le : m_lineEntries)
    {
        if (le.bScreenSpace != bScreenSpace)
            continue;
        float dx = le.x1 - le.x0;
        float dy = le.y1 - le.y0;
        float len = std::sqrt(dx * dx + dy * dy);
        float halfT = le.thickness * 0.5f;

        float nx, ny;
        if (len < 1e-6f)
        {
            // Degenerate line: draw a small cross
            nx = halfT; ny = 0.0f;
            dx = 0.0f; dy = 0.0f;
        }
        else
        {
            float invLen = 1.0f / len;
            nx = -dy * invLen * halfT;
            ny =  dx * invLen * halfT;
        }

        // Pixel → NDC conversion (same math as CalcWorldMatrixNoGlobal)
        auto toNDC = [&](float px, float py) -> std::pair<float, float>
        {
            return {
                (px / vw) * 2.0f - 1.0f,
                1.0f - (py / vh) * 2.0f
            };
        };

        float depthZ = le.depth * depthScale;

        // Four corners of the thick line quad (triangle-strip order)
        auto [x0a, y0a] = toNDC(le.x0 - nx, le.y0 - ny);
        auto [x0b, y0b] = toNDC(le.x0 + nx, le.y0 + ny);
        auto [x1a, y1a] = toNDC(le.x1 - nx, le.y1 - ny);
        auto [x1b, y1b] = toNDC(le.x1 + nx, le.y1 + ny);

        // TRIANGLELIST: two triangles forming the thick line quad
        // V0 = start-left, V1 = start-right, V2 = end-left, V3 = end-right
        // Triangles: (V0,V1,V2) and (V1,V3,V2) — no shared edges with next line
        verts.push_back({x0b, y0b, depthZ, le.color});  // V0: left of start
        verts.push_back({x0a, y0a, depthZ, le.color});  // V1: right of start
        verts.push_back({x1b, y1b, depthZ, le.color});  // V2: left of end
        verts.push_back({x0a, y0a, depthZ, le.color});  // V1 again
        verts.push_back({x1a, y1a, depthZ, le.color});  // V3: right of end
        verts.push_back({x1b, y1b, depthZ, le.color});  // V2 again
    }

    // Upload to GPU
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (FAILED(m_pContext->Map(m_pLineVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return;
    memcpy(mapped.pData, verts.data(), totalVerts * 16);
    m_pContext->Unmap(m_pLineVB.Get(), 0);

    // Set state
    UINT stride = 16;
    UINT offset = 0;
    m_pContext->IASetVertexBuffers(0, 1, m_pLineVB.GetAddressOf(), &stride, &offset);
    m_pContext->IASetInputLayout(m_pLineInputLayout.Get());
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pContext->VSSetShader(m_pLineVS.Get(), nullptr, 0);
    m_pContext->PSSetShader(pCustomPS ? pCustomPS : m_pLinePS.Get(), nullptr, 0);

    // Draw all lines in one call
    m_pContext->Draw(totalVerts, 0);

    std::erase_if(m_lineEntries, [bScreenSpace](const auto &le) { return le.bScreenSpace == bScreenSpace; });
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
    // GPU batch path: used for non-AA lines (supports dashed).
    if (!params.antiAlias && m_dx)
    {
        ShapeColor col = params.color;
        col.a *= params.opacity;
        uint32_t rgba = ColorToU32(col);
        UINT depth = 0;
        if (params.drawDepth != -1)
        {
            depth = params.drawDepth;
        }
        else
        {
            depth = params.bScreenSpace ? 0 :  m_dx->GetNextDepth();
        }

        bool useDash = (params.dashLength > 0.f && params.gapLength > 0.f);
        if (useDash)
        {
            float dx = x1 - x0, dy = y1 - y0;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len < 1e-4f) {
                m_dx->AddLineEntry(x0, y0, x1, y1, rgba, params.thickness, depth, params.bScreenSpace);
                return;
            }
            float cycleLen = params.dashLength + params.gapLength;
            float pos = 0.f;
            while (pos < len)
            {
                float dashEnd = std::min(pos + params.dashLength, len);
                float t0 = pos / len, t1 = dashEnd / len;
                float sx = x0 + dx * t0, sy = y0 + dy * t0;
                float ex = x0 + dx * t1, ey = y0 + dy * t1;
                m_dx->AddLineEntry(sx, sy, ex, ey, rgba, params.thickness, depth, params.bScreenSpace);
                pos += cycleLen;
            }
        }
        else
        {
            m_dx->AddLineEntry(x0, y0, x1, y1, rgba, params.thickness, depth, params.bScreenSpace);
        }
        return;
    }

    // Fallback CPU path (AA only)
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

    bool hasFill = !params.fillColor.IsTransparent();
    bool hasBorder = (params.borderWidth > 0.f && !params.borderColor.IsTransparent());
    bool useDash = (params.dashLength > 0.f && params.gapLength > 0.f);

    if (!hasFill && !hasBorder)
        return;

    // ── Fill via CPU canvas ──
    if (hasFill)
    {
        float pad = 2.f;
        float originX = std::floor(x - pad);
        float originY = std::floor(y - pad);
        int cw = (int)std::ceil(w + pad * 2.f) + 2;
        int ch = (int)std::ceil(h + pad * 2.f) + 2;

        Canvas canvas;
        canvas.Resize(cw, ch);

        float lx = x - originX, ly = y - originY;
        float rrx = lx + w, rry = ly + h;

        ShapeColor fc = params.fillColor;
        fc.a *= params.opacity;
        uint32_t fillRGBA = ColorToU32(fc);
        for (int py = (int)std::ceil(ly); py <= (int)std::floor(rry); ++py)
            for (int px = (int)std::ceil(lx); px <= (int)std::floor(rrx); ++px)
                canvas.SetPixel(px, py, fillRGBA);

        FlushCanvas(canvas, originX, originY, 1.f, params.bScreenSpace);
    }

    // ── Border via GPU line batching (supports dashed) ──
    if (hasBorder && m_dx)
    {
        ShapeColor bc = params.borderColor;
        bc.a *= params.opacity;
        uint32_t rgba = ColorToU32(bc);
        UINT depth = 0;
        if (params.drawDepth != -1)
        {
            depth = params.drawDepth;
        }
        else
        {
            depth = params.bScreenSpace ? 0 :  m_dx->GetNextDepth();
        }

        struct Seg { float ax, ay, bx, by; };
        Seg segs[4] = {
            {x, y, x + w, y},
            {x + w, y, x + w, y + h},
            {x + w, y + h, x, y + h},
            {x, y + h, x, y},
        };

        if (useDash)
        {
            float cycleLen = params.dashLength + params.gapLength;
            for (auto &s : segs)
            {
                float dx = s.bx - s.ax, dy = s.by - s.ay;
                float segLen = std::sqrt(dx * dx + dy * dy);
                if (segLen < 1e-4f) continue;
                float pos = 0.f;
                while (pos < segLen)
                {
                    float dashEnd = std::min(pos + params.dashLength, segLen);
                    float t0 = pos / segLen, t1 = dashEnd / segLen;
                    m_dx->AddLineEntry(
                        s.ax + dx * t0, s.ay + dy * t0,
                        s.ax + dx * t1, s.ay + dy * t1,
                        rgba, params.borderWidth, depth, params.bScreenSpace);
                    pos += cycleLen;
                }
            }
        }
        else
        {
            for (auto &s : segs)
                m_dx->AddLineEntry(s.ax, s.ay, s.bx, s.by,
                                   rgba, params.borderWidth, depth, params.bScreenSpace);
        }
    }
}

void DrawShapes::DrawEllipse(float cx, float cy, float rx, float ry,
                             const EllipseParams &params)
{
    if (rx < .5f || ry < .5f)
        return;

    bool hasFill = !params.fillColor.IsTransparent();
    bool hasBorder = (params.borderWidth > 0.f && !params.borderColor.IsTransparent());
    bool useDash = (params.dashLength > 0.f && params.gapLength > 0.f);

    if (!hasFill && !hasBorder)
        return;

    // ── Fill via CPU canvas ──
    if (hasFill)
    {
        float pad = 2.f;
        float originX = std::floor(cx - rx - pad);
        float originY = std::floor(cy - ry - pad);
        int cw = (int)std::ceil((rx + pad) * 2.f) + 2;
        int ch = (int)std::ceil((ry + pad) * 2.f) + 2;

        Canvas canvas;
        canvas.Resize(cw, ch);

        float lcx = cx - originX;
        float lcy = cy - originY;

        ShapeColor fc = params.fillColor;
        fc.a *= params.opacity;
        RasterEllipseFill(canvas, lcx, lcy, rx, ry, ColorToU32(fc));

        FlushCanvas(canvas, originX, originY, 1.f, params.bScreenSpace);
    }

    // ── Border via GPU line batching (supports dashed) ──
    if (hasBorder && m_dx)
    {
        ShapeColor bc = params.borderColor;
        bc.a *= params.opacity;
        uint32_t rgba = ColorToU32(bc);
        UINT depth = 0;
        if (params.drawDepth != -1)
        {
            depth = params.drawDepth;
        }
        else
        {
            depth = params.bScreenSpace ? 0 :  m_dx->GetNextDepth();
        }

        int segs = params.segments > 0
                       ? params.segments
                       : std::max(32, (int)(2.f * kPi * std::max(rx, ry) / 1.5f));

        float prevPx = 0.f, prevPy = 0.f;
        for (int i = 0; i <= segs; ++i)
        {
            float angle = 2.f * kPi * i / segs;
            float px = cx + rx * std::cos(angle);
            float py = cy + ry * std::sin(angle);

            if (i == 0) { prevPx = px; prevPy = py; continue; }

            float dx = px - prevPx, dy = py - prevPy;
            float segLen = std::sqrt(dx * dx + dy * dy);

            if (useDash && segLen >= 1e-4f)
            {
                float cycleLen = params.dashLength + params.gapLength;
                float pos = 0.f;
                while (pos < segLen)
                {
                    float dashEnd = std::min(pos + params.dashLength, segLen);
                    float t0 = pos / segLen, t1 = dashEnd / segLen;
                    m_dx->AddLineEntry(
                        prevPx + dx * t0, prevPy + dy * t0,
                        prevPx + dx * t1, prevPy + dy * t1,
                        rgba, params.borderWidth, depth, params.bScreenSpace);
                    pos += cycleLen;
                }
            }
            else
            {
                m_dx->AddLineEntry(prevPx, prevPy, px, py,
                                   rgba, params.borderWidth, depth, params.bScreenSpace);
            }
            prevPx = px;
            prevPy = py;
        }
    }
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
    const FString& text,
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
        DT_SINGLELINE;

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
        DT_SINGLELINE;

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
