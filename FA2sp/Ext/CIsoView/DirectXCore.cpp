#include <DirectXMath.h>
#include <vector>
#include <cstring>
#include <string>
#include <algorithm>
#include "../CLoading/Body.h"
#include "Body.h"
#include "DirectXCore.h"
#include "../../Miscs/Palettes.h"
#include <glad/glad.h>

// WGL extension types (not provided by glad)
typedef HGLRC(WINAPI *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int *attribList);
typedef BOOL(WINAPI *PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);

// glBlendFunci - ARB_draw_buffers_blend (GL 3.3 extension, core in 4.0)
typedef void(APIENTRYP PFNGLBLENDFUNCIPROC)(GLuint buf, GLenum src, GLenum dst);
static PFNGLBLENDFUNCIPROC glad_glBlendFunci = nullptr;
#define glBlendFunci glad_glBlendFunci

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "opengl32.lib")

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

void GetDriverVersionFromRegistry()
{
    HKEY hKey;
    if (RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}",
            0,
            KEY_READ,
            &hKey) != ERROR_SUCCESS)
    {
        return;
    }

    WCHAR subKeyName[256];
    DWORD index = 0;
    DWORD size = 256;

    while (RegEnumKeyExW(hKey, index, subKeyName, &size,
                         nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS)
    {
        WCHAR path[512];
        swprintf_s(path,
                   L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\%s",
                   subKeyName);

        HKEY subKey;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &subKey) == ERROR_SUCCESS)
        {
            CHAR driverVer[128];
            DWORD sz = sizeof(driverVer);

            if (RegQueryValueEx(subKey, "DriverVersion", nullptr, nullptr,
                                (LPBYTE)driverVer, &sz) == ERROR_SUCCESS)
            {
                Logger::Raw("[DX] DriverVersion: %s\n", driverVer);
            }

            RegCloseKey(subKey);
        }

        size = 256;
        index++;
    }

    RegCloseKey(hKey);
}

bool DirectXCore::Initialize(HWND hwnd)
{
    Cleanup();

    if (!IsWindow(hwnd))
    {
        Logger::Raw("[DirectXCore] Initialize: Invalid HWND.\n");
        return false;
    }

    // --- Determine rendering backend ---
    // DirectXRendering is the master switch; OpenGLRendering selects GL
    // when both are enabled.
    m_bUseOpenGL = ExtConfigs::DirectXRendering && ExtConfigs::OpenGLRendering;
    Logger::Raw("[DirectXCore] Backend: %s\n", m_bUseOpenGL ? "OpenGL 3.3" : "Direct3D 11");

    if (m_bUseOpenGL)
    {
        if (!GL_Init(hwnd))
        {
            Logger::Raw("[DirectXCore] GL_Init failed.\n");
            return false;
        }
        m_bInitialized = true;
        Logger::Raw("[DirectXCore] Initialize (OpenGL) succeeded.\n");
        return true;
    }

    // ==================== D3D11 path ====================
    if (!CreateDeviceAndSwapChain(hwnd))
    {
        Logger::Raw("[DirectXCore] CreateDeviceAndSwapChain failed.\n");
        return false;
    }

    // --- Log GPU / adapter info ---
    {
        ComPtr<IDXGIDevice> pDXGIDevice;
        if (SUCCEEDED(m_pDevice.As(&pDXGIDevice)) && pDXGIDevice)
        {
            ComPtr<IDXGIAdapter> pAdapter;
            if (SUCCEEDED(pDXGIDevice->GetAdapter(&pAdapter)) && pAdapter)
            {
                DXGI_ADAPTER_DESC desc = {};
                if (SUCCEEDED(pAdapter->GetDesc(&desc)))
                {
                    // Convert wide-char description to UTF-8
                    char gpuName[256] = {};
                    WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, gpuName, sizeof(gpuName), nullptr, nullptr);

                    D3D_FEATURE_LEVEL fl = m_pDevice->GetFeatureLevel();
                    const char *flStr = "?";
                    switch (fl)
                    {
                    case D3D_FEATURE_LEVEL_11_0:
                        flStr = "11_0";
                        break;
                    case D3D_FEATURE_LEVEL_10_1:
                        flStr = "10_1";
                        break;
                    case D3D_FEATURE_LEVEL_10_0:
                        flStr = "10_0";
                        break;
                    case D3D_FEATURE_LEVEL_9_3:
                        flStr = "9_3";
                        break;
                    default:
                        break;
                    }

                    Logger::Raw("\n");
                    Logger::Raw("========== DirectX Device Info ==========\n");
                    Logger::Raw("[DX] GPU: %s\n", gpuName);
                    Logger::Raw("[DX] VendorId: 0x%04X  DeviceId: 0x%04X\n",
                                desc.VendorId, desc.DeviceId);
                    Logger::Raw("[DX] VRAM: %d MB\n", desc.DedicatedVideoMemory / (1024 * 1024));
                    Logger::Raw("[DX] System RAM Shared: %d MB\n",
                                desc.SharedSystemMemory / (1024 * 1024));
                    Logger::Raw("[DX] FeatureLevel: %s\n", flStr);

                    GetDriverVersionFromRegistry();
                    Logger::Raw("========================================\n");
                    Logger::Raw("\n");
                }
            }
        }
    }

    if (!CreateShadersAndInputLayout())
    {
        Logger::Raw("[DirectXCore] CreateShadersAndInputLayout failed.\n");
        return false;
    }
    if (!CreateEffectShaders())
    {
        Logger::Raw("[DirectXCore] CreateEffectShaders failed.\n");
        return false;
    }
    if (!CreateCompositeShaders())
    {
        Logger::Raw("[DirectXCore] CreateCompositeShaders failed.\n");
        return false;
    }
    if (!CreateLineShaders())
    {
        Logger::Raw("[DirectXCore] CreateLineShaders failed.\n");
        return false;
    }
    if (!CreateAlphaAccumShaders())
    {
        Logger::Raw("[DirectXCore] CreateAlphaAccumShaders failed.\n");
        return false;
    }
    if (!CreateShadowDarkenShaders())
    {
        Logger::Raw("[DirectXCore] CreateShadowDarkenShaders failed.\n");
        return false;
    }
    if (!CreateQuadVertexBuffer())
    {
        Logger::Raw("[DirectXCore] CreateQuadVertexBuffer failed.\n");
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
        Logger::Raw("[DirectXCore] CreateOffscreenResources failed.\n");
        return false;
    }
    if (!CreateFinalShaders())
    {
        Logger::Raw("[DirectXCore] CreateFinalShaders failed.\n");
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
    Logger::Raw("[DirectXCore] Initialize succeeded.\n");
    return true;
}

bool DirectXCore::IsInitialized()
{
    return m_bInitialized;
}

void DirectXCore::ClearTextures()
{
    if (m_bUseOpenGL)
    {
        for (auto &[k, v] : m_textureMap)
            if (v && v->glTexture)
                glDeleteTextures(1, &v->glTexture);
    }
    m_textureMap.clear();
    Logger::Raw("[DirectXCore] Clear textures.\n");
}

void DirectXCore::ClearTileTextures()
{
    if (m_bUseOpenGL)
    {
        for (auto &[k, v] : m_tileTextureMap)
            if (v && v->glTexture)
                glDeleteTextures(1, &v->glTexture);
    }
    m_tileTextureMap.clear();
    Logger::Raw("[DirectXCore] Clear tile textures.\n");
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
    m_pInstancedVS.Reset();
    m_pInstancedInputLayout.Reset();
    m_pInstanceVB.Reset();
    m_instanceVBCapacity = 0;
    m_pTrackedDSState = nullptr;
    m_trackedStencilRef = 0;
    m_pTrackedSRV = nullptr;
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
    m_pDepthStateObjectStencilWrite.Reset();
    m_pDepthStateStencilOnlyWrite.Reset();
    m_pDepthStateTerrainRedraw.Reset();
    m_pDepthStateShadowMark.Reset();
    m_pDepthStateShadowDarken.Reset();
    m_pDepthStateShadowRedraw.Reset();
    m_pBlendStateDarken.Reset();
    m_pShadowDarkenPS.Reset();
    m_pBlendStateNoColor.Reset();

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

    // --- OpenGL cleanup ---
    if (m_bUseOpenGL)
        GL_Cleanup();

    m_clientWidth = m_clientHeight = 0;
    m_globalScaleX = m_globalScaleY = 1.0f;
    m_globalOffsetX = m_globalOffsetY = 0.0f;
    m_renderScale = 1.0f;
    m_bInitialized = false;

    Logger::Raw("[DirectXCore] Cleaning up.\n");
}

void DirectXCore::OnResize(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;
    m_clientWidth = width;
    m_clientHeight = height;

    if (m_bUseOpenGL)
    {
        // Recreate GL FBOs and helper textures at new size
        wglMakeCurrent(m_hGLDC, m_hGLRC);
        if (m_glTexOffscreen)
            glDeleteTextures(1, &m_glTexOffscreen);
        if (m_glFBOOffscreen)
            glDeleteFramebuffers(1, &m_glFBOOffscreen);
        if (m_glRBODepthStencil)
            glDeleteRenderbuffers(1, &m_glRBODepthStencil);
        m_glTexOffscreen = m_glFBOOffscreen = m_glRBODepthStencil = 0;
        GL_EnsureFactorTexture();
        GL_EnsureScreenCopyTexture();
        GL_EnsureAlphaAccumTexture();
        if (!GL_CreateOffscreenResources())
            Logger::Raw("[DirectXCore] OnResize: GL_CreateOffscreenResources failed.\n");
        return;
    }

    // --- D3D11 path ---
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

    if (m_bUseOpenGL)
    {
        // Update scale FIRST so Ensure/Create functions use the new size
        m_renderScale = scaleFactor;

        if (m_glTexOffscreen)
            glDeleteTextures(1, &m_glTexOffscreen);
        if (m_glFBOOffscreen)
            glDeleteFramebuffers(1, &m_glFBOOffscreen);
        if (m_glRBODepthStencil)
            glDeleteRenderbuffers(1, &m_glRBODepthStencil);
        m_glTexOffscreen = m_glFBOOffscreen = m_glRBODepthStencil = 0;
        GL_EnsureFactorTexture();
        GL_EnsureScreenCopyTexture();
        GL_EnsureAlphaAccumTexture();
        if (!GL_CreateOffscreenResources())
        {
            Logger::Raw("[DirectXCore] SetZoomOut: GL_CreateOffscreenResources failed.\n");
            return m_renderScale;
        }
        return m_renderScale;
    }

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
        Logger::Raw("[DirectXCore] SetZoomOut: CreateOffscreenResources failed, keeping old scale.\n");
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
#ifndef NDEBUG
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                               createFlags, featureLevels, 2,
                                               D3D11_SDK_VERSION, &scd,
                                               &m_pSwapChain, &m_pDevice, nullptr, &m_pContext);
    if (FAILED(hr))
    {
        Logger::Raw("D3D11CreateDeviceAndSwapChain failed.\n");
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

    D3D11_BLEND_DESC blendNoColor = {};
    blendNoColor.RenderTarget[0].BlendEnable = FALSE;
    blendNoColor.RenderTarget[0].RenderTargetWriteMask = 0;
    m_pDevice->CreateBlendState(&blendNoColor, &m_pBlendStateNoColor);

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

    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    m_pDevice->CreateDepthStencilState(&dsDesc, &m_pDepthStateReadOnlyGE);

    dsDesc.DepthEnable = FALSE;
    m_pDevice->CreateDepthStencilState(&dsDesc, &m_pDepthStateOff);

    D3D11_DEPTH_STENCIL_DESC owDesc = {};
    owDesc.DepthEnable = TRUE;
    owDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    owDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
    owDesc.StencilEnable = TRUE;
    owDesc.StencilReadMask = 0x7F;
    owDesc.StencilWriteMask = 0xFF;
    owDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    owDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    owDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    owDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    owDesc.BackFace = owDesc.FrontFace;
    m_pDevice->CreateDepthStencilState(&owDesc, &m_pDepthStateObjectStencilWrite);

    D3D11_DEPTH_STENCIL_DESC soDesc = {};
    soDesc.DepthEnable = TRUE;
    soDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    soDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
    soDesc.StencilEnable = TRUE;
    soDesc.StencilReadMask = 0x7F;
    soDesc.StencilWriteMask = 0xFF;
    soDesc.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER;
    soDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    soDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    soDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    soDesc.BackFace = soDesc.FrontFace;
    m_pDevice->CreateDepthStencilState(&soDesc, &m_pDepthStateStencilOnlyWrite);

    D3D11_DEPTH_STENCIL_DESC trDesc = {};
    trDesc.DepthEnable = TRUE;
    trDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    trDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
    trDesc.StencilEnable = TRUE;
    trDesc.StencilReadMask = 0x7F;
    trDesc.StencilWriteMask = 0xFF;
    trDesc.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER_EQUAL;
    trDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    trDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    trDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    trDesc.BackFace = trDesc.FrontFace;
    m_pDevice->CreateDepthStencilState(&trDesc, &m_pDepthStateTerrainRedraw);

    // Shadow mark: reads low 7 bits (depth), writes only bit 7 (shadow flag).
    // stencilRef = (shadowHeight+1) | 0x80 so that REPLACE with WriteMask=0x80
    // sets bit 7 while GREATER comparison only considers bits 0-6.
    D3D11_DEPTH_STENCIL_DESC smDesc = {};
    smDesc.DepthEnable = TRUE;
    smDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    smDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    smDesc.StencilEnable = TRUE;
    smDesc.StencilReadMask = 0x7F;
    smDesc.StencilWriteMask = 0x80;
    smDesc.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER_EQUAL;
    smDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    smDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    smDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    smDesc.BackFace = smDesc.FrontFace;
    m_pDevice->CreateDepthStencilState(&smDesc, &m_pDepthStateShadowMark);

    D3D11_DEPTH_STENCIL_DESC srDesc = {};
    srDesc.DepthEnable = TRUE;
    srDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    srDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    srDesc.StencilEnable = TRUE;
    srDesc.StencilReadMask = 0x7F;
    srDesc.StencilWriteMask = 0x00;
    srDesc.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER_EQUAL;
    srDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    srDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    srDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    srDesc.BackFace = srDesc.FrontFace;
    m_pDevice->CreateDepthStencilState(&srDesc, &m_pDepthStateShadowRedraw);

    // Shadow darken: stencil test only - EQUAL 0x80 means only pixels
    // where the shadow bit is set pass.  Used by the fullscreen darken pass.
    D3D11_DEPTH_STENCIL_DESC sdDesc = {};
    sdDesc.DepthEnable = FALSE;
    sdDesc.StencilEnable = TRUE;
    sdDesc.StencilReadMask = 0x80;
    sdDesc.StencilWriteMask = 0;
    sdDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
    sdDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    sdDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    sdDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    sdDesc.BackFace = sdDesc.FrontFace;
    m_pDevice->CreateDepthStencilState(&sdDesc, &m_pDepthStateShadowDarken);

    // Darken blend: Result = Dest * Src  (multiplicative darkening).
    // Src (PS output) = (darkness, darkness, darkness, 1), so Dest.rgb *= darkness.
    D3D11_BLEND_DESC darkenBlend = {};
    darkenBlend.RenderTarget[0].BlendEnable = TRUE;
    darkenBlend.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
    darkenBlend.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;
    darkenBlend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    darkenBlend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    darkenBlend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    darkenBlend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    darkenBlend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    m_pDevice->CreateBlendState(&darkenBlend, &m_pBlendStateDarken);

    // -- Instanced vertex shader --
    // Same quad VB (slot 0) + per-instance VB (slot 1).
    // InstanceData layout (packed into float4 slots):
    //   slot:1  float4(scaleX, scaleY, transX, transY)
    //   slot:2  float4(depthZ, colorMulR, colorMulG, colorMulB)
    //   slot:3  float4(colorMulA, mixR, mixG, mixB)
    //   slot:4  float4(mixFactor, 0, 0, 0)
    {
        const char *instVS = R"(
            struct VSInput {
                float3 pos : POSITION0;
                float2 tex : TEXCOORD0;
            };
            struct VSInstance {
                float4 d0 : TEXCOORD1;   // scaleX, scaleY, transX, transY
                float4 d1 : TEXCOORD2;   // depthZ, colorMul.rgb
                float4 d2 : TEXCOORD3;   // colorMul.a, mixColor.rgb
                float4 d3 : TEXCOORD4;   // mixFactor, _, _, _
            };
            struct VSOutput {
                float4 pos      : SV_POSITION;
                float2 uv       : TEXCOORD0;
                float4 colorMul : COLOR0;
                float4 mixColor : COLOR1;
                float  mixFactor: TEXCOORD1;
            };
            VSOutput main(VSInput v, VSInstance i) {
                // Reconstruct world matrix: S * T
                // | scaleX 0       0   transX |
                // | 0      scaleY  0   transY |
                // | 0      0       1   depthZ |
                // | 0      0       0   1       |
                VSOutput o;
                float4 pos4 = float4(v.pos.x * i.d0.x + i.d0.z,
                                     v.pos.y * i.d0.y + i.d0.w,
                                     i.d1.x,  1.0f);
                o.pos = pos4;
                o.uv = v.tex;
                o.colorMul  = float4(i.d1.yzw, i.d2.x);
                o.mixColor  = i.d2.yzwx;       // mixR,mixG,mixB,1
                o.mixFactor = i.d3.x;
                return o;
            }
        )";
        ComPtr<ID3DBlob> blob, err;
        HRESULT hr = D3DCompile(instVS, strlen(instVS), nullptr, nullptr, nullptr,
                                "main", "vs_5_0", 0, 0, &blob, &err);
        if (FAILED(hr))
        {
            if (err)
                OutputDebugStringA((char *)err->GetBufferPointer());
            return false;
        }
        hr = m_pDevice->CreateVertexShader(blob->GetBufferPointer(),
                                           blob->GetBufferSize(),
                                           nullptr, &m_pInstancedVS);
        if (FAILED(hr))
            return false;

        // Input layout: slot 0 = QuadVertex (per-vertex), slot 1 = InstanceData (per-instance)
        D3D11_INPUT_ELEMENT_DESC instLayout[] = {
            // Per-vertex (slot 0)
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
             D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12,
             D3D11_INPUT_PER_VERTEX_DATA, 0},
            // Per-instance (slot 1) - 4 x float4 = 64 bytes
            {"TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,
             D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16,
             D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32,
             D3D11_INPUT_PER_INSTANCE_DATA, 1},
            {"TEXCOORD", 4, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48,
             D3D11_INPUT_PER_INSTANCE_DATA, 1},
        };
        hr = m_pDevice->CreateInputLayout(instLayout, 6, blob->GetBufferPointer(),
                                          blob->GetBufferSize(),
                                          &m_pInstancedInputLayout);
        if (FAILED(hr))
            return false;
    }

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
    // Unbind the offscreen RT before copying from it (D3D11 forbids CopyResource on a bound RT)
    m_pContext->OMSetRenderTargets(0, nullptr, nullptr);
    m_pContext->CopyResource(m_pScreenCopy.Get(), m_OffscreenTex.Get());
    // Re-bind the offscreen RT (caller expects it to be active)
    // Note: caller (Phase 4) will set its own RT immediately after
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
        desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
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
        // Unbind back buffer RTV before copying from it
        m_pContext->OMSetRenderTargets(0, nullptr, nullptr);
        m_pContext->CopyResource(m_pBackgroundCacheTexture.Get(), pBackBufferResource.Get());
        // Re-bind back buffer RTV (caller will set RTV again before next draw)
        m_pContext->OMSetRenderTargets(1, m_pRTV.GetAddressOf(), nullptr);
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
        // Unbind back buffer RTV before copying to it
        m_pContext->OMSetRenderTargets(0, nullptr, nullptr);
        m_pContext->CopyResource(pBackBufferResource.Get(), m_pBackgroundCacheTexture.Get());
        // Re-bind back buffer RTV (caller will set RTV again before next draw)
        m_pContext->OMSetRenderTargets(1, m_pRTV.GetAddressOf(), nullptr);
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
    // Just pass through directly â€? no matrix transform needed.
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
        if (error)
            OutputDebugStringA((char *)error->GetBufferPointer());
        return false;
    }
    hr = m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pMRTPS);
    if (FAILED(hr))
        return false;

    // Compile modified line PS
    hr = D3DCompile(lineModPS, strlen(lineModPS), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &error);
    if (FAILED(hr))
    {
        if (error)
            OutputDebugStringA((char *)error->GetBufferPointer());
        return false;
    }
    hr = m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pLineModPS);
    if (FAILED(hr))
        return false;

    return true;
}

bool DirectXCore::CreateShadowDarkenShaders()
{
    // Pixel shader that outputs a constant darkening factor.
    // Used with DstBlend=SRC_COLOR to multiply destination by this factor.
    const char *psCode = R"(
        float4 main(float4 pos : SV_POSITION) : SV_Target {
            return float4(0.5f, 0.5f, 0.5f, 1.0f);
        }
    )";

    ComPtr<ID3DBlob> psBlob, error;
    HRESULT hr = D3DCompile(psCode, strlen(psCode), nullptr, nullptr, nullptr,
                            "main", "ps_5_0", 0, 0, &psBlob, &error);
    if (FAILED(hr))
    {
        if (error)
            OutputDebugStringA((char *)error->GetBufferPointer());
        return false;
    }
    hr = m_pDevice->CreatePixelShader(psBlob->GetBufferPointer(),
                                      psBlob->GetBufferSize(),
                                      nullptr, &m_pShadowDarkenPS);
    return SUCCEEDED(hr);
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
    if (FAILED(hr))
        return;
    hr = m_pDevice->CreateRenderTargetView(m_pAlphaAccumTex.Get(), nullptr, &m_pAlphaAccumRTV);
    if (FAILED(hr))
        return;
    hr = m_pDevice->CreateShaderResourceView(m_pAlphaAccumTex.Get(), nullptr, &m_pAlphaAccumSRV);
}

// -- CPU-side DS-state tracking --
// Avoids expensive OMGetDepthStencilState pipeline queries.
void DirectXCore::SetDSStateTracked(ID3D11DepthStencilState *pDS, UINT stencilRef)
{
    if (m_pTrackedDSState != pDS || m_trackedStencilRef != stencilRef)
    {
        m_pContext->OMSetDepthStencilState(pDS, stencilRef);
        m_pTrackedDSState = pDS;
        m_trackedStencilRef = stencilRef;
    }
}

// -- Instance batching --
// Draws all DrawCommands in `batch` with a single DrawInstanced call.
// All commands MUST share the same texture, VS, PS, blend state, DS state.
// Pixel-perfect output identical to per-quad Draw(4,0).
void DirectXCore::FlushInstanceBatch(const std::vector<const DrawCommand *> &batch)
{
    if (batch.empty())
        return;
    const UINT count = (UINT)batch.size();

    // Ensure instance VB capacity
    if (!m_pInstanceVB || m_instanceVBCapacity < (int)count)
    {
        m_pInstanceVB.Reset();
        int cap = (count + 255) & ~255; // round up
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = cap * sizeof(InstanceData);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (FAILED(m_pDevice->CreateBuffer(&desc, nullptr, &m_pInstanceVB)))
        {
            m_instanceVBCapacity = 0;
            return;
        }
        m_instanceVBCapacity = cap;
    }

    // Fill instance data
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (FAILED(m_pContext->Map(m_pInstanceVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return;
    auto *dst = (InstanceData *)mapped.pData;
    for (const DrawCommand *cmd : batch)
    {
        const auto &p = cmd->params;
        TextureResource *tex = cmd->texRes;
        float w_px = tex->sourceView.FullWidth * p.scaleX;
        float h_px = tex->sourceView.FullHeight * p.scaleY;
        float snappedX = std::floor(p.x + 0.5f);
        float snappedY = std::floor(p.y + 0.5f);
        float ndcW = (w_px / m_vwCached) * 2.0f;
        float ndcH = (h_px / m_vhCached) * 2.0f;
        float ndcCenterX = ((snappedX + w_px * 0.5f) / m_vwCached) * 2.0f - 1.0f;
        float ndcCenterY = 1.0f - ((snappedY + h_px * 0.5f) / m_vhCached) * 2.0f;
        float depthZ = cmd->depth * (1.0f / 16777216.0f);
        dst->scaleX = ndcW;
        dst->scaleY = ndcH;
        dst->transX = ndcCenterX;
        dst->transY = ndcCenterY;
        dst->depthZ = depthZ;
        dst->colorMulR = p.redMult;
        dst->colorMulG = p.greenMult;
        dst->colorMulB = p.blueMult;
        dst->colorMulA = p.opacity;
        dst->mixR = p.mixR;
        dst->mixG = p.mixG;
        dst->mixB = p.mixB;
        dst->mixFactor = p.mixFactor;
        dst->padding[0] = dst->padding[1] = dst->padding[2] = 0;
        ++dst;
    }
    m_pContext->Unmap(m_pInstanceVB.Get(), 0);

    // Bind quad VB (slot 0) + instance VB (slot 1)
    UINT strides[2] = {sizeof(QuadVertex), sizeof(InstanceData)};
    UINT offsets[2] = {0, 0};
    ID3D11Buffer *vbs[2] = {m_pQuadVB.Get(), m_pInstanceVB.Get()};
    m_pContext->IASetVertexBuffers(0, 2, vbs, strides, offsets);

    // Bind the SRV for the batch (all commands share the same texture)
    TextureResource *tex = batch[0]->texRes;
    if (tex && tex->srv)
    {
        if (m_pTrackedSRV != tex->srv.Get())
        {
            m_pContext->PSSetShaderResources(0, 1, tex->srv.GetAddressOf());
            m_pTrackedSRV = tex->srv.Get();
        }
    }

    // Ensure sampler is set (callers may not have set it)
    m_pContext->PSSetSamplers(0, 1,
                              (m_renderScale == 1.0f) ? m_pSamplerPoint.GetAddressOf()
                                                      : m_pSamplerLinear.GetAddressOf());

    m_pContext->DrawInstanced(4, count, 0, 0);

    // Restore single VB binding for subsequent code that assumes slot 0 only
    UINT singleStride = sizeof(QuadVertex);
    UINT singleOffset = 0;
    m_pContext->IASetVertexBuffers(0, 1, m_pQuadVB.GetAddressOf(), &singleStride, &singleOffset);
}

void DirectXCore::RenderOffscreenContent()
{
    float clearColor[4] = {
        CIsoViewExt::RenderingMap ? 0.0f : (ExtConfigs::EnableDarkMode ? 0.125f : 1.0f),
        CIsoViewExt::RenderingMap ? 0.0f : (ExtConfigs::EnableDarkMode ? 0.125f : 1.0f),
        CIsoViewExt::RenderingMap ? 0.0f : (ExtConfigs::EnableDarkMode ? 0.125f : 1.0f),
        1.0f};
    m_pContext->ClearRenderTargetView(m_OffscreenRTV.Get(), clearColor);
    m_pContext->ClearDepthStencilView(m_pOffscreenDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 0.0f, 0);

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

    // Cache viewport size for instanced batching
    m_vwCached = vw;
    m_vhCached = vh;

    // Map UINT depth [0..N] to clip-space Z [0..0.9999].
    const float depthScale = 1.0f / 16777216.0f;

    // -- Simplified single-quad draw (for stencil & effect phases) --
    // Uses CPU-tracked DS state - NO OMGetDepthStencilState query.
    auto DrawOneTracked = [&](const DrawCommand &cmd)
    {
        TextureResource *tex = cmd.texRes;
        if (!tex || !tex->srv)
            return;
        const auto &p = cmd.params;
        float depthZ = cmd.depth * depthScale;
        float w_px = tex->sourceView.FullWidth * p.scaleX;
        float h_px = tex->sourceView.FullHeight * p.scaleY;
        float snappedX = std::floor(p.x + 0.5f);
        float snappedY = std::floor(p.y + 0.5f);
        float centerX_px = snappedX + w_px * 0.5f;
        float centerY_px = snappedY + h_px * 0.5f;
        float ndc_width = (w_px / vw) * 2.0f;
        float ndc_height = (h_px / vh) * 2.0f;
        float ndc_centerX = (centerX_px / vw) * 2.0f - 1.0f;
        float ndc_centerY = 1.0f - (centerY_px / vh) * 2.0f;
        XMMATRIX world = XMMatrixScaling(ndc_width, ndc_height, 1.0f) *
                         XMMatrixTranslation(ndc_centerX, ndc_centerY, depthZ);
        CBPerObject cb;
        cb.world = XMMatrixTranspose(world);
        cb.colorMul = XMFLOAT4(p.redMult, p.greenMult, p.blueMult, p.opacity);
        cb.mixColor = XMFLOAT4(p.mixR, p.mixG, p.mixB, 1.0f);
        cb.mixFactor = p.mixFactor;

        if (cmd.pCustomDSState)
        {
            UINT ref = (p.stencilRef >= 0) ? (UINT)p.stencilRef : 0;
            SetDSStateTracked(cmd.pCustomDSState, ref);
        }

        D3D11_MAPPED_SUBRESOURCE mapped;
        if (FAILED(m_pContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            return;
        memcpy(mapped.pData, &cb, sizeof(cb));
        m_pContext->Unmap(m_pConstantBuffer.Get(), 0);
        m_pContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

        if (m_pTrackedSRV != tex->srv.Get())
        {
            m_pContext->PSSetShaderResources(0, 1, tex->srv.GetAddressOf());
            m_pTrackedSRV = tex->srv.Get();
        }
        m_pContext->Draw(4, 0);

        // m_pContext->Flush();
    };

    // ====================================================================
    // Classification (unchanged)
    // ====================================================================
    std::vector<const DrawCommand *> opaqueCmds;
    std::vector<const DrawCommand *> stencilCmds;
    std::vector<const DrawCommand *> transparentCmds;
    std::vector<const DrawCommand *> effectCmds;
    std::vector<const DrawCommand *> overlayCmds;

    for (const auto &cmd : m_drawCommands)
    {
        if (cmd.bAlwaysOnTop)
        {
            overlayCmds.push_back(&cmd);
            continue;
        }
        if (cmd.bStencilDraw)
            stencilCmds.push_back(&cmd);
        if (cmd.bIsEffect)
            effectCmds.push_back(&cmd);

        if (!cmd.bScreenSpace && !cmd.bStencilDraw && !cmd.bIsEffect)
        {
            TextureResource *tex = cmd.texRes;
            if (tex && tex->srv && !tex->bIsIndexTexture)
            {
                if (cmd.params.opacity < 1.0f - 1e-6f)
                    transparentCmds.push_back(&cmd);
                else
                    opaqueCmds.push_back(&cmd);
            }
        }
    }
    // ====================================================================
    // Phase 1: Instanced opaque (depth write ON, depth test ON)
    //   Split: commands with custom DS state (stencil writes) are drawn
    //   per-quad; plain commands use instanced batching.
    // ====================================================================
    m_pContext->OMSetRenderTargets(1, m_OffscreenRTV.GetAddressOf(), m_pOffscreenDSV.Get());
    m_pContext->OMSetDepthStencilState(m_pDepthStateGE.Get(), 0);
    m_pTrackedDSState = m_pDepthStateGE.Get();
    m_trackedStencilRef = 0;
    m_pContext->PSSetShader(m_pPS.Get(), nullptr, 0);

    if (!opaqueCmds.empty())
    {
        // Separate: plain vs. stencil-writing commands
        std::vector<const DrawCommand *> opaquePlain;
        std::vector<const DrawCommand *> opaqueStencil;
        for (const DrawCommand *cmd : opaqueCmds)
        {
            if (!cmd->texRes || !cmd->texRes->sourceView.pImageBuffer)
                continue;
            if (cmd->pCustomDSState)
                opaqueStencil.push_back(cmd);
            else
                opaquePlain.push_back(cmd);
        }

        // -- Draw per-quad for stencil-aware opaque commands first --
        // These write building/object height into stencil for Phase 1.5.
        for (const DrawCommand *cmd : opaqueStencil)
        {
            DrawOneTracked(*cmd);
        }

        // -- Instanced batch for plain opaque commands --
        if (!opaquePlain.empty())
        {
            std::stable_sort(opaquePlain.begin(), opaquePlain.end(),
                             [](const DrawCommand *a, const DrawCommand *b)
                             {
                                 return a->texRes < b->texRes;
                             });

            m_pContext->VSSetShader(m_pInstancedVS.Get(), nullptr, 0);
            m_pContext->IASetInputLayout(m_pInstancedInputLayout.Get());

            std::vector<const DrawCommand *> batch;
            TextureResource *curTex = nullptr;
            for (const DrawCommand *cmd : opaquePlain)
            {
                if (cmd->texRes != curTex)
                {
                    FlushInstanceBatch(batch);
                    batch.clear();
                    curTex = cmd->texRes;
                }
                batch.push_back(cmd);
            }
            FlushInstanceBatch(batch);

            // Restore non-instanced pipeline
            m_pContext->VSSetShader(m_pVS.Get(), nullptr, 0);
            m_pContext->IASetInputLayout(m_pInputLayout.Get());
            UINT singleStride = sizeof(QuadVertex);
            UINT singleOffset = 0;
            m_pContext->IASetVertexBuffers(0, 1, m_pQuadVB.GetAddressOf(), &singleStride, &singleOffset);
            m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            m_pTrackedSRV = nullptr;
        }
    }
    m_pContext->Flush();

    // ====================================================================
    // Phase 1.5: Stencil-aware draws (per-quad, state-tracked)
    // ====================================================================
    if (ExtConfigs::PreciseDepthCalculation && !stencilCmds.empty())
    {
        // -- Sub-phase a: Write stencil (no color) --
        m_pContext->OMSetBlendState(m_pBlendStateNoColor.Get(), nullptr, 0xffffffff);
        for (const DrawCommand *cmd : stencilCmds)
        {
            if (cmd->params.bIsShadow || cmd->params.bIsOverlapShadow ||
                !cmd->params.bWriteStencil || !cmd->bStencilOnly)
                continue;
            if (!cmd->texRes || !cmd->texRes->srv)
                continue;
            DrawOneTracked(*cmd);
        }
        m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);

        // -- Sub-phase b: Read stencil (normal draw) --
        for (const DrawCommand *cmd : stencilCmds)
        {
            if (cmd->params.bIsShadow || cmd->params.bIsOverlapShadow ||
                cmd->params.bWriteStencil)
                continue;
            if (!cmd->texRes || !cmd->texRes->srv)
                continue;
            DrawOneTracked(*cmd);
        }

        // -- Sub-phase c: Shadow objects (write stencil, no color) --
        m_pContext->OMSetBlendState(m_pBlendStateNoColor.Get(), nullptr, 0xffffffff);
        for (const DrawCommand *cmd : stencilCmds)
        {
            if (!cmd->params.bIsShadow || cmd->params.bIsOverlapShadow || cmd->bStencilOnly)
                continue;
            if (!cmd->texRes || !cmd->texRes->srv)
                continue;
            DrawOneTracked(*cmd);
        }
        m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);

        // -- Sub-phase d: Overlap shadow draw --
        for (const DrawCommand *cmd : stencilCmds)
        {
            if (!cmd->params.bIsOverlapShadow || cmd->bStencilOnly)
                continue;
            if (!cmd->texRes || !cmd->texRes->srv)
                continue;
            DrawOneTracked(*cmd);
        }

        // -- Restore depth-stencil state --
        m_pContext->OMSetDepthStencilState(m_pDepthStateGE.Get(), 0);
        m_pTrackedDSState = m_pDepthStateGE.Get();
        m_trackedStencilRef = 0;

        // -- Shadow darkening pass (fullscreen) --
        if (CIsoViewExt::DrawShadows && m_pShadowDarkenPS && m_pBlendStateDarken &&
            m_pDepthStateShadowDarken && m_pOffscreenDSV)
        {
            stride = sizeof(QuadVertex);
            offset = 0;
            m_pContext->IASetVertexBuffers(0, 1, m_pFullscreenQuadVB.GetAddressOf(), &stride, &offset);
            m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            m_pContext->IASetInputLayout(m_pInputLayout.Get());

            // Bind offscreen RT with DSV (needed for stencil test)
            m_pContext->OMSetRenderTargets(1, m_OffscreenRTV.GetAddressOf(),
                                           m_pOffscreenDSV.Get());
            // Only pixels with stencil == 0x80 (bit 7 set) pass
            SetDSStateTracked(m_pDepthStateShadowDarken.Get(), 0x80);
            // Multiplicative blend: Result = Dest * Src (= Dest * 0.5)
            m_pContext->OMSetBlendState(m_pBlendStateDarken.Get(), nullptr, 0xffffffff);

            m_pContext->VSSetShader(m_pFinalVS.Get(), nullptr, 0);
            m_pContext->PSSetShader(m_pShadowDarkenPS.Get(), nullptr, 0);

            DrawFullscreenQuad();

            // Restore default blend state
            m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
            // Restore quad VB to the regular one for subsequent phases
            m_pContext->IASetVertexBuffers(0, 1, m_pQuadVB.GetAddressOf(), &stride, &offset);
            m_pContext->VSSetShader(m_pVS.Get(), nullptr, 0);
            m_pContext->PSSetShader(m_pPS.Get(), nullptr, 0);
            m_pTrackedSRV = nullptr;
            m_pTrackedDSState = m_pDepthStateGE.Get();
            m_trackedStencilRef = 0;
        }
        m_pContext->Flush();
    }

    // ====================================================================
    // Phase 2: Semi-transparent textures via MRT - INSTANCED
    // ====================================================================
    if (!transparentCmds.empty())
    {
        float zero[4] = {0, 0, 0, 0};
        m_pContext->ClearRenderTargetView(m_pAlphaAccumRTV.Get(), zero);

        ID3D11RenderTargetView *mrtRTs[2] = {m_OffscreenRTV.Get(), m_pAlphaAccumRTV.Get()};
        m_pContext->OMSetRenderTargets(2, mrtRTs, m_pOffscreenDSV.Get());
        m_pContext->OMSetBlendState(m_pAlphaAccumBlendState.Get(), nullptr, 0xffffffff);
        m_pContext->OMSetDepthStencilState(m_pDepthStateReadOnlyGE.Get(), 0);
        m_pTrackedDSState = m_pDepthStateReadOnlyGE.Get();
        m_trackedStencilRef = 0;
        m_pContext->PSSetShader(m_pMRTPS.Get(), nullptr, 0);

        // Sort by texture for batching
        std::stable_sort(transparentCmds.begin(), transparentCmds.end(),
                         [](const DrawCommand *a, const DrawCommand *b)
                         {
                             return a->texRes < b->texRes;
                         });

        m_pContext->VSSetShader(m_pInstancedVS.Get(), nullptr, 0);
        m_pContext->IASetInputLayout(m_pInstancedInputLayout.Get());

        std::vector<const DrawCommand *> batch;
        TextureResource *curTex = nullptr;
        for (const DrawCommand *cmd : transparentCmds)
        {
            if (cmd->texRes != curTex)
            {
                FlushInstanceBatch(batch);
                batch.clear();
                curTex = cmd->texRes;
            }
            batch.push_back(cmd);
        }
        FlushInstanceBatch(batch);

        // Restore single RT, normal blend, non-instanced pipeline
        m_pContext->OMSetRenderTargets(1, m_OffscreenRTV.GetAddressOf(), m_pOffscreenDSV.Get());
        m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
        m_pContext->VSSetShader(m_pVS.Get(), nullptr, 0);
        m_pContext->IASetInputLayout(m_pInputLayout.Get());
        UINT singleStride = sizeof(QuadVertex);
        UINT singleOffset = 0;
        m_pContext->IASetVertexBuffers(0, 1, m_pQuadVB.GetAddressOf(), &singleStride, &singleOffset);
        m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        m_pTrackedSRV = nullptr;
        m_pContext->Flush();
    }

    // ====================================================================
    // Phase 3: World-space lines with alpha modulation
    //   Lines sample the alpha accum buffer and attenuate output alpha
    //   by (1 - accumAlpha) so they appear "behind" semi-transparent texels.
    // ====================================================================
    {
        SetDSStateTracked(m_pDepthStateReadOnlyGE.Get(), 0);
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
        m_pContext->Flush();
    }

    // ====================================================================
    // Restore quad pipeline state (Phase 3 changed IA/VS/PS for lines)
    // ====================================================================
    stride = sizeof(QuadVertex);
    offset = 0;
    m_pContext->IASetVertexBuffers(0, 1, m_pQuadVB.GetAddressOf(), &stride, &offset);
    m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_pContext->IASetInputLayout(m_pInputLayout.Get());
    m_pContext->VSSetShader(m_pVS.Get(), nullptr, 0);
    m_pContext->PSSetShader(m_pPS.Get(), nullptr, 0);
    m_pContext->PSSetSamplers(0, 1, (m_renderScale == 1.0f) ? m_pSamplerPoint.GetAddressOf() : m_pSamplerLinear.GetAddressOf());

    // ====================================================================
    // Phase 4: Effects pass (index textures - factor - composite)
    // ====================================================================
    if (!effectCmds.empty())
    {
        CopyScreenToTexture();

        float one[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        m_pContext->ClearRenderTargetView(m_pFactorRTV.Get(), one);

        m_pContext->OMSetRenderTargets(1, m_pFactorRTV.GetAddressOf(), nullptr);
        m_pContext->OMSetBlendState(m_pMulBlendState.Get(), nullptr, 0xffffffff);
        m_pTrackedDSState = m_pDepthStateGE.Get();
        m_trackedStencilRef = 0;
        m_pContext->OMSetDepthStencilState(m_pDepthStateGE.Get(), 0);
        m_pContext->VSSetShader(m_pEffectVS.Get(), nullptr, 0);
        m_pContext->PSSetShader(m_pEffectPS.Get(), nullptr, 0);
        m_pContext->PSSetSamplers(0, 1, m_pSamplerPoint.GetAddressOf());

        for (const DrawCommand *cmd : effectCmds)
        {
            TextureResource *idxTex = cmd->texRes;
            if (!idxTex || !idxTex->srv || !idxTex->bIsIndexTexture)
                continue;

            const auto &p = cmd->params;
            float depthZ = cmd->depth * depthScale;
            float w_px = idxTex->sourceView.FullWidth * p.scaleX;
            float h_px = idxTex->sourceView.FullHeight * p.scaleY;
            float snappedX = std::floor(p.x + 0.5f);
            float snappedY = std::floor(p.y + 0.5f);
            float centerX_px = snappedX + w_px * 0.5f;
            float centerY_px = snappedY + h_px * 0.5f;
            float ndc_width = (w_px / vw) * 2.0f;
            float ndc_height = (h_px / vh) * 2.0f;
            float ndc_centerX = (centerX_px / vw) * 2.0f - 1.0f;
            float ndc_centerY = 1.0f - (centerY_px / vh) * 2.0f;
            XMMATRIX world = XMMatrixScaling(ndc_width, ndc_height, 1.0f) *
                             XMMatrixTranslation(ndc_centerX, ndc_centerY, depthZ);
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
            if (m_pTrackedSRV != idxTex->srv.Get())
            {
                m_pContext->PSSetShaderResources(0, 1, idxTex->srv.GetAddressOf());
                m_pTrackedSRV = idxTex->srv.Get();
            }
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

    // ====================================================================
    // Phase 5: Always-on-top world-space overlays (depth OFF)
    //   Text annotations, base node indices, etc.
    // ====================================================================
    bool hasOverlay = !overlayCmds.empty();
    // Quick scan for overlay lines (no need to pre-collect - FlushLineBatch counts)
    bool hasOverlayLines = false;
    for (const auto &le : m_lineEntries)
    {
        if (!le.bScreenSpace && le.bAlwaysOnTop)
        {
            hasOverlayLines = true;
            break;
        }
    }

    if (hasOverlay || hasOverlayLines)
    {
        m_pContext->OMSetRenderTargets(1, m_OffscreenRTV.GetAddressOf(), m_pOffscreenDSV.Get());
        m_pContext->OMSetDepthStencilState(m_pDepthStateOff.Get(), 0);
        m_pTrackedDSState = m_pDepthStateOff.Get();
        m_trackedStencilRef = 0;
        m_pContext->OMSetBlendState(m_pBlendState.Get(), nullptr, 0xffffffff);
    }

    if (!overlayCmds.empty())
    {
        // Sort by texture for batching
        std::stable_sort(overlayCmds.begin(), overlayCmds.end(),
                         [](const DrawCommand *a, const DrawCommand *b)
                         {
                             return a->texRes < b->texRes;
                         });

        m_pContext->VSSetShader(m_pInstancedVS.Get(), nullptr, 0);
        m_pContext->IASetInputLayout(m_pInstancedInputLayout.Get());
        m_pContext->PSSetShader(m_pPS.Get(), nullptr, 0);
        m_pContext->PSSetSamplers(0, 1,
                                  (m_renderScale == 1.0f) ? m_pSamplerPoint.GetAddressOf()
                                                          : m_pSamplerLinear.GetAddressOf());

        std::vector<const DrawCommand *> batch;
        TextureResource *curTex = nullptr;
        for (const DrawCommand *cmd : overlayCmds)
        {
            if (cmd->texRes != curTex)
            {
                FlushInstanceBatch(batch);
                batch.clear();
                curTex = cmd->texRes;
            }
            batch.push_back(cmd);
        }
        FlushInstanceBatch(batch);

        // Restore non-instanced pipeline
        m_pContext->VSSetShader(m_pVS.Get(), nullptr, 0);
        m_pContext->IASetInputLayout(m_pInputLayout.Get());
        UINT singleStride = sizeof(QuadVertex);
        UINT singleOffset = 0;
        m_pContext->IASetVertexBuffers(0, 1, m_pQuadVB.GetAddressOf(), &singleStride, &singleOffset);
        m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        m_pTrackedSRV = nullptr;
    }

    // Phase 5b: Always-on-top lines (depth OFF, no alpha modulation, regular line PS)
    if (hasOverlayLines)
    {
        FlushLineBatch(false, m_pLinePS.Get(), true);

        // Restore quad pipeline state (Phase 5b changed IA/VS/PS for lines)
        stride = sizeof(QuadVertex);
        offset = 0;
        m_pContext->IASetVertexBuffers(0, 1, m_pQuadVB.GetAddressOf(), &stride, &offset);
        m_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        m_pContext->IASetInputLayout(m_pInputLayout.Get());
        m_pContext->VSSetShader(m_pVS.Get(), nullptr, 0);
        m_pContext->PSSetShader(m_pPS.Get(), nullptr, 0);
        m_pContext->PSSetSamplers(0, 1, (m_renderScale == 1.0f) ? m_pSamplerPoint.GetAddressOf() : m_pSamplerLinear.GetAddressOf());
    }
    m_pContext->Flush();
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
            // Unbind back buffer RTV before copying to it (D3D11 forbids CopyResource on bound RT)
            m_pContext->OMSetRenderTargets(0, nullptr, nullptr);
            m_pContext->CopyResource(pBackBufferResource.Get(), m_OffscreenTex.Get());
            // Re-bind back buffer RTV
            m_pContext->OMSetRenderTargets(1, m_pRTV.GetAddressOf(), nullptr);
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
    m_pContext->Flush();
}

void DirectXCore::RenderScreenSpaceContent()
{
    D3D11_VIEWPORT screenVP = {0, 0, (float)m_clientWidth, (float)m_clientHeight, 0.0f, 1.0f};
    m_pContext->RSSetViewports(1, &screenVP);

    m_pContext->OMSetRenderTargets(1, m_pRTV.GetAddressOf(), nullptr);
    m_pContext->OMSetDepthStencilState(m_pDepthStateOff.Get(), 0);

    int screenCmdCount = 0;
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
            ++screenCmdCount;

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
    m_pContext->Flush();
}

void DirectXCore::Render()
{
    if (!m_bInitialized)
        return;

    // If nothing to render at all, skip entirely
    if (m_drawCommands.empty() && m_lineEntries.empty())
        return;

    ResetDepth();

    if (m_bUseOpenGL)
    {
        GL_RenderOffscreenContent();

        if (!CIsoViewExt::RenderingMap)
        {
            if (ExtConfigs::EnableDarkMode && ExtConfigs::EnableDarkMode_DimMap)
                GL_DarkenOffscreen(0.7f);

            GL_RenderFinalToBackBuffer();
            GL_RenderScreenSpaceContent();
        }

        SwapBuffers(m_hGLDC);
        m_drawCommands.clear();
        return;
    }

    // --- D3D11 path ---
    if (!m_pContext || !m_pSwapChain || !m_pRTV)
        return;

    RenderOffscreenContent();

    if (!CIsoViewExt::RenderingMap)
    {
        if (ExtConfigs::EnableDarkMode && ExtConfigs::EnableDarkMode_DimMap)
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
    if (!m_bInitialized)
        return;

    if (m_bUseOpenGL)
    {
        GL_RenderFinalToBackBuffer();
        GL_RenderScreenSpaceContent();
        SwapBuffers(m_hGLDC);
        m_drawCommands.clear();
        return;
    }

    if (!m_pContext || !m_pSwapChain || !m_pRTV || !m_backgroundCacheValid)
        return;

    RestoreBackgroundFromCache();
    RenderScreenSpaceContent();
    m_pSwapChain->Present(1, 0);
    m_drawCommands.clear();
}

TextureResource *DirectXCore::LoadTexture(const ImageDataView &view, BGRStruct color, bool ignoreTransparent)
{
    auto index = TextureIndex{view.pOriginData, color};
    auto [itr, inserted] = m_textureMap.try_emplace(index);
    if (!inserted)
        return itr->second.get();
    if (!view.pOriginData)
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
            if (opacity < 255 && ignoreTransparent)
                opacity = 0;
            BGRStruct color = view.pPalette->Data[idx];
            uint32_t rgba = (color.R << 0) | (color.G << 8) | (color.B << 16) | (opacity << 24);
            rgbaData[y * w + x] = rgba;
        }
    }

    // === OpenGL path ===
    if (m_bUseOpenGL)
    {
        GL_UploadTextureRGBA8(texRes.get(), w, h, rgbaData.data(), false);
        TextureResource *ret = texRes.get();
        itr->second = std::move(texRes);
        return ret;
    }

    // === D3D11 path ===
    if (!m_pDevice)
        return nullptr;
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
    if (!tileBlock)
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

    // === OpenGL path ===
    if (m_bUseOpenGL)
    {
        GL_UploadTextureRGBA8(texRes.get(), w, h, rgbaData.data(), false);
        TextureResource *ret = texRes.get();
        itr->second = std::move(texRes);
        return ret;
    }

    // === D3D11 path ===
    if (!m_pDevice)
        return nullptr;
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
    if (!view.pOriginData)
        return nullptr;
    if (view.FullWidth <= 0 || view.FullHeight <= 0 || !view.pImageBuffer)
        return nullptr;

    auto texRes = std::make_unique<TextureResource>();
    texRes->sourceView = view;
    texRes->bIsIndexTexture = true;
    int w = view.FullWidth, h = view.FullHeight;

    // === OpenGL path ===
    if (m_bUseOpenGL)
    {
        GL_UploadTextureR8(texRes.get(), w, h, view.pImageBuffer, true);
        TextureResource *ret = texRes.get();
        itr->second = std::move(texRes);
        return ret;
    }

    // === D3D11 path ===
    if (!m_pDevice)
        return nullptr;
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
    if (!m_pDevice && !m_bUseOpenGL)
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

    // === OpenGL path ===
    if (m_bUseOpenGL)
    {
        GL_UploadTextureRGBA8(texRes.get(), w, h, rgbaData.data(), false);
        TextureResource *ret = texRes.get();
        itr->second = std::move(texRes);
        return ret;
    }

    // === D3D11 path ===
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
    if (m_bUseOpenGL)
    {
        auto it = m_bitmapTextureMap.find(name);
        if (it != m_bitmapTextureMap.end() && it->second && it->second->glTexture)
            glDeleteTextures(1, &it->second->glTexture);
    }
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
    cmd.bAlwaysOnTop = params.bAlwaysOnTop;
    if (!ExtConfigs::PreciseDepthCalculation)
    {
        cmd.params.bIsShadow = false;
        cmd.params.bWriteStencil = false;
        cmd.params.SetStencilRef(-1);
    }
    if (cmd.params.bScreenSpace)
    {
        cmd.params.bWriteStencil = false;
        cmd.params.SetStencilRef(-1);
    }
    if (params.drawDepth != -1)
    {
        cmd.depth = params.drawDepth;
    }
    else
    {
        cmd.depth = params.bScreenSpace ? 0 : GetNextDepth();
    }

    if (cmd.params.stencilRef >= 0)
    {
        if (cmd.params.bIsOverlapShadow)
        {
            int shadowVal = cmd.params.stencilRef;
            cmd.bStencilDraw = true;
            cmd.bStencilOnly = false;
            cmd.bIsOverlapShadow = true;
            cmd.pCustomDSState = m_pDepthStateShadowRedraw.Get();
            cmd.params.stencilRef = shadowVal;
            cmd.params.drawDepth = cmd.depth;
            m_drawCommands.push_back(cmd);
        }
        else if (cmd.params.bIsShadow)
        {
            int shadowVal = cmd.params.stencilRef | 0x80;
            cmd.bStencilDraw = true;
            cmd.bStencilOnly = false;
            cmd.bIsShadowMark = true;
            cmd.pCustomDSState = m_pDepthStateShadowMark.Get();
            cmd.params.stencilRef = shadowVal;
            m_drawCommands.push_back(cmd);
        }
        else if (cmd.params.bWriteStencil)
        {
            cmd.bStencilDraw = false;
            cmd.pCustomDSState = m_pDepthStateObjectStencilWrite.Get();
            UINT renderDepth = cmd.depth;
            m_drawCommands.push_back(cmd);
        }
        else
        {
            cmd.bStencilDraw = true;
            cmd.pCustomDSState = m_pDepthStateTerrainRedraw.Get();
            m_drawCommands.push_back(cmd);
        }
    }
    else
    {
        cmd.bStencilDraw = false;
        cmd.pCustomDSState = nullptr;
        m_drawCommands.push_back(cmd);
    }
}

void DirectXCore::AddLineEntry(float x0, float y0, float x1, float y1,
                               uint32_t color, float thickness, UINT depth,
                               bool bScreenSpace, bool bAlwaysOnTop)
{
    m_lineEntries.push_back({x0, y0, x1, y1, color, thickness, depth, bScreenSpace, bAlwaysOnTop});
}

void DirectXCore::FlushLineBatch(bool bScreenSpace, ID3D11PixelShader *pCustomPS, bool bOverlay)
{
    if (!m_pDevice || !m_pContext)
        return;

    // Count matching entries
    int numLines = 0;
    for (const auto &le : m_lineEntries)
        if (le.bScreenSpace == bScreenSpace && le.bAlwaysOnTop == bOverlay)
            ++numLines;

    if (numLines == 0)
        return;

    const int vertsPerLine = 6;
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
    struct LineVertex
    {
        float x, y, depth;
        uint32_t color;
    }; // 16 bytes
    std::vector<LineVertex> verts;
    verts.reserve(totalVerts);

    // World-space lines use offscreen viewport; screen-space lines use window
    float vw, vh;
    if (bScreenSpace)
    {
        vw = (float)m_clientWidth;
        vh = (float)m_clientHeight;
    }
    else
    {
        vw = (float)(m_clientWidth * m_renderScale);
        vh = (float)(m_clientHeight * m_renderScale);
    }
    const float depthScale = 1.0f / 16777216.0f;

    for (const auto &le : m_lineEntries)
    {
        if (le.bScreenSpace != bScreenSpace || le.bAlwaysOnTop != bOverlay)
            continue;
        float dx = le.x1 - le.x0;
        float dy = le.y1 - le.y0;
        float len = std::sqrt(dx * dx + dy * dy);
        float halfT = le.thickness * 0.5f;

        float nx, ny;
        if (len < 1e-6f)
        {
            // Degenerate line: draw a small cross
            nx = halfT;
            ny = 0.0f;
            dx = 0.0f;
            dy = 0.0f;
        }
        else
        {
            float invLen = 1.0f / len;
            nx = -dy * invLen * halfT;
            ny = dx * invLen * halfT;
        }

        auto toNDC = [&](float px, float py) -> std::pair<float, float>
        {
            return {
                (px / vw) * 2.0f - 1.0f,
                1.0f - (py / vh) * 2.0f};
        };

        float depthZ = le.depth * depthScale;

        // Four corners of the thick line quad (triangle-strip order)
        auto [x0a, y0a] = toNDC(le.x0 - nx, le.y0 - ny);
        auto [x0b, y0b] = toNDC(le.x0 + nx, le.y0 + ny);
        auto [x1a, y1a] = toNDC(le.x1 - nx, le.y1 - ny);
        auto [x1b, y1b] = toNDC(le.x1 + nx, le.y1 + ny);

        // TRIANGLELIST: two triangles forming the thick line quad
        // V0 = start-left, V1 = start-right, V2 = end-left, V3 = end-right
        // Triangles: (V0,V1,V2) and (V1,V3,V2) â€? no shared edges with next line
        verts.push_back({x0b, y0b, depthZ, le.color}); // V0: left of start
        verts.push_back({x0a, y0a, depthZ, le.color}); // V1: right of start
        verts.push_back({x1b, y1b, depthZ, le.color}); // V2: left of end
        verts.push_back({x0a, y0a, depthZ, le.color}); // V1 again
        verts.push_back({x1a, y1a, depthZ, le.color}); // V3: right of end
        verts.push_back({x1b, y1b, depthZ, le.color}); // V2 again
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

    // Set sampler (needed by line PS variants that sample textures, e.g. m_pLineModPS)
    m_pContext->PSSetSamplers(0, 1, m_pSamplerPoint.GetAddressOf());

    // Draw all lines in one call
    m_pContext->Draw(totalVerts, 0);

    std::erase_if(m_lineEntries, [bScreenSpace, bOverlay](const auto &le)
                  { return le.bScreenSpace == bScreenSpace && le.bAlwaysOnTop == bOverlay; });
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

    // Upload to OpenGL if using GL backend
    if (m_dx->IsUsingOpenGL())
        m_dx->GL_UploadTextureDynamic(slot.res.get(), w, h, pixels.data());

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
            depth = params.bScreenSpace ? 0 : m_dx->GetNextDepth();
        }

        bool useDash = (params.dashLength > 0.f && params.gapLength > 0.f);
        if (useDash)
        {
            float dx = x1 - x0, dy = y1 - y0;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len < 1e-4f)
            {
                m_dx->AddLineEntry(x0, y0, x1, y1, rgba, params.thickness, depth, params.bScreenSpace, params.bAlwaysOnTop);
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
                m_dx->AddLineEntry(sx, sy, ex, ey, rgba, params.thickness, depth, params.bScreenSpace, params.bAlwaysOnTop);
                pos += cycleLen;
            }
        }
        else
        {
            m_dx->AddLineEntry(x0, y0, x1, y1, rgba, params.thickness, depth, params.bScreenSpace, params.bAlwaysOnTop);
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
            depth = params.bScreenSpace ? 0 : m_dx->GetNextDepth();
        }

        struct Seg
        {
            float ax, ay, bx, by;
        };
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
                if (segLen < 1e-4f)
                    continue;
                float pos = 0.f;
                while (pos < segLen)
                {
                    float dashEnd = std::min(pos + params.dashLength, segLen);
                    float t0 = pos / segLen, t1 = dashEnd / segLen;
                    m_dx->AddLineEntry(
                        s.ax + dx * t0, s.ay + dy * t0,
                        s.ax + dx * t1, s.ay + dy * t1,
                        rgba, params.borderWidth, depth, params.bScreenSpace, params.bAlwaysOnTop);
                    pos += cycleLen;
                }
            }
        }
        else
        {
            for (auto &s : segs)
                m_dx->AddLineEntry(s.ax, s.ay, s.bx, s.by,
                                   rgba, params.borderWidth, depth, params.bScreenSpace, params.bAlwaysOnTop);
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
            depth = params.bScreenSpace ? 0 : m_dx->GetNextDepth();
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

            if (i == 0)
            {
                prevPx = px;
                prevPy = py;
                continue;
            }

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
                        rgba, params.borderWidth, depth, params.bScreenSpace, params.bAlwaysOnTop);
                    pos += cycleLen;
                }
            }
            else
            {
                m_dx->AddLineEntry(prevPx, prevPy, px, py,
                                   rgba, params.borderWidth, depth, params.bScreenSpace, params.bAlwaysOnTop);
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
    const FString &text,
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
    {
        dp.SetScreenSpace();
    }
    else
    {
        dp.bAlwaysOnTop = params.bAlwaysOnTop;
    }

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
        DT_NOPREFIX;

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
    lf.lfQuality = p.fontSize >= 24 ? CLEARTYPE_NATURAL_QUALITY : NONANTIALIASED_QUALITY;
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
        DT_NOPREFIX;

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

    // === OpenGL path ===
    if (m_dx->IsUsingOpenGL())
    {
        m_dx->GL_UploadTextureRGBA8(res, w, h, pixels.data(), false);
        res->sourceView.FullWidth = w;
        res->sourceView.FullHeight = h;
        return true;
    }

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
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = pixels.data();
        initData.SysMemPitch = w * 4;

        HRESULT hr = dev->CreateTexture2D(&desc, &initData, &res->texture);
        if (FAILED(hr))
            return false;

        hr = dev->CreateShaderResourceView(res->texture.Get(), nullptr, &res->srv);
        if (FAILED(hr))
            return false;

        // Upload to OpenGL
        if (m_dx->IsUsingOpenGL())
            m_dx->GL_UploadTextureRGBA8(res, w, h, pixels.data(), false);

        res->sourceView.FullWidth = w;
        res->sourceView.FullHeight = h;
        return true;
    }

    ctx->UpdateSubresource(res->texture.Get(), 0, nullptr,
                           pixels.data(), w * 4, 0);

    // Update OpenGL texture
    if (m_dx->IsUsingOpenGL())
        m_dx->GL_UploadTextureDynamic(res, w, h, pixels.data());

    res->sourceView.FullWidth = w;
    res->sourceView.FullHeight = h;
    return true;
}

// ==========================================================================
// OpenGL 3.3 Backend Implementation
// ==========================================================================

// --- GLSL Shader Sources (converted from HLSL) ---
// Note: depth is remapped from D3D [0,1] reverse-Z to GL NDC [-1,1].
// In GL we use standard depth: clear=1.0, GL_LEQUAL, z_output=1.0-depthZ.
// Texture Y is flipped during upload, so UVs need no adjustment here.

static const char *kGLSL_MainVS = R"(#version 330 core
uniform mat4 g_World;
uniform vec4 g_ColorMul;
uniform vec4 g_MixColor;
uniform float g_MixFactor;
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
out vec2 vUV;
out vec4 vColorMul;
out vec4 vMixColor;
out float vMixFactor;
void main() {
    gl_Position = g_World * vec4(inPos, 1.0);
    gl_Position.z = 1.0 - gl_Position.z * 2.0 - 0.00000011920928955078126; // D3D [0,1] -> GL [1,0], x2 preserves 24-bit depth precision
    vUV = inUV;
    vColorMul = g_ColorMul;
    vMixColor = g_MixColor;
    vMixFactor = g_MixFactor;
}
)";

static const char *kGLSL_MainPS = R"(#version 330 core
uniform sampler2D tex;
in vec2 vUV;
in vec4 vColorMul;
in vec4 vMixColor;
in float vMixFactor;
layout(location = 0) out vec4 outColor;
void main() {
    vec4 texColor = texture(tex, vUV);
    vec3 multRGB = texColor.rgb * vColorMul.rgb;
    vec3 finalRGB = mix(multRGB, vMixColor.rgb, vMixFactor);
    float finalAlpha = texColor.a * vColorMul.a;
    if (finalAlpha < 1.0/255.0) discard;
    outColor = vec4(finalRGB, finalAlpha);
}
)";

static const char *kGLSL_InstancedVS = R"(#version 330 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
// Per-instance: 4 x vec4 = 64 bytes
layout(location = 2) in vec4 iD0; // scaleX, scaleY, transX, transY
layout(location = 3) in vec4 iD1; // depthZ, colorMul.rgb
layout(location = 4) in vec4 iD2; // colorMul.a, mixColor.rgb
layout(location = 5) in vec4 iD3; // mixFactor, _, _, _
out vec2 vUV;
out vec4 vColorMul;
out vec4 vMixColor;
out float vMixFactor;
void main() {
    vec4 pos4 = vec4(inPos.x * iD0.x + iD0.z,
                     inPos.y * iD0.y + iD0.w,
                     iD1.x, 1.0);
    gl_Position = pos4;
    gl_Position.z = 1.0 - pos4.z * 2.0 - 0.00000011920928955078126;
    vUV = inUV;
    vColorMul = vec4(iD1.yzw, iD2.x);
    vMixColor = vec4(iD2.yzw, 1.0);
    vMixFactor = iD3.x;
}
)";

static const char *kGLSL_EffectVS = R"(#version 330 core
uniform mat4 g_World;
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
out vec2 vUV;
void main() {
    gl_Position = g_World * vec4(inPos, 1.0);
    gl_Position.z = 1.0 - gl_Position.z * 2.0 - 0.00000011920928955078126;
    vUV = inUV;
}
)";

static const char *kGLSL_EffectPS = R"(#version 330 core
uniform sampler2D indexTex;
in vec2 vUV;
layout(location = 0) out vec4 outColor;
void main() {
    float f = texture(indexTex, vUV).r;
    float index = f * 255.0;
    float factor = index / 128.0;
    outColor = vec4(factor, factor, factor, 1.0);
}
)";

static const char *kGLSL_CompositeVS = R"(#version 330 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
out vec2 vUV;
void main() {
    gl_Position = vec4(inPos, 1.0);
    vUV = inUV;
}
)";

static const char *kGLSL_CompositePS = R"(#version 330 core
uniform sampler2D screenTex;
uniform sampler2D factorTex;
in vec2 vUV;
layout(location = 0) out vec4 outColor;
void main() {
    // Flip V: GL texture origin is bottom-left, fullscreen quad UVs are top-left.
    vec2 uv = vec2(vUV.x, 1.0 - vUV.y);
    vec4 orig = texture(screenTex, uv);
    float factor = texture(factorTex, uv).r;
    outColor = vec4(orig.rgb * factor, orig.a);
}
)";

static const char *kGLSL_FinalVS = R"(#version 330 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
out vec2 vUV;
void main() {
    gl_Position = vec4(inPos, 1.0);
    vUV = inUV;
}
)";

static const char *kGLSL_FinalPS = R"(#version 330 core
uniform sampler2D tex;
uniform vec2 uScale;
uniform vec2 uOffset;
in vec2 vUV;
layout(location = 0) out vec4 outColor;
void main() {
    vec2 ndc = vUV * 2.0 - 1.0;
    vec2 originalNdc = (ndc - uOffset) / uScale;
    vec2 originalUv = (originalNdc + 1.0) / 2.0;
    if (any(lessThan(originalUv, vec2(0.0))) || any(greaterThan(originalUv, vec2(1.0))))
        discard;
    // GL FBO origin is bottom-left; D3D offscreen texture origin is top-left.
    // The fullscreen quad maps NDC bottom-left to UV(0,1), which in GL samples
    // the TOP of the offscreen texture.  Flip V to match D3D behaviour.
    outColor = texture(tex, vec2(originalUv.x, 1.0 - originalUv.y));
}
)";

static const char *kGLSL_LineVS = R"(#version 330 core
layout(location = 0) in vec4 inPosDepth; // (ndcX, ndcY, depth, colorPacked)
layout(location = 1) in uint inColor;
out vec4 vCol;
void main() {
    gl_Position = vec4(inPosDepth.xy, inPosDepth.z, 1.0);
    gl_Position.z = 1.0 - gl_Position.z * 2.0 - 0.00000011920928955078126;
    uint c = inColor;
    vCol = vec4(
        float(c & 0xffu) / 255.0,
        float((c >> 8u) & 0xffu) / 255.0,
        float((c >> 16u) & 0xffu) / 255.0,
        float((c >> 24u) & 0xffu) / 255.0
    );
}
)";

static const char *kGLSL_LinePS = R"(#version 330 core
in vec4 vCol;
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vCol;
}
)";

static const char *kGLSL_LineModPS = R"(#version 330 core
uniform vec2 uViewportSize;
uniform sampler2D alphaAccum;
in vec4 vCol;
layout(location = 0) out vec4 outColor;
void main() {
    vec2 uv = gl_FragCoord.xy / uViewportSize;
    float accumAlpha = texture(alphaAccum, uv).r;
    vec4 lineColor = vCol;
    lineColor.a *= (1.0 - accumAlpha);
    outColor = lineColor;
}
)";

static const char *kGLSL_AlphaAccumPS = R"(#version 330 core
uniform sampler2D tex;
in vec2 vUV;
in vec4 vColorMul;
in vec4 vMixColor;
in float vMixFactor;
layout(location = 0) out vec4 outRT0;
layout(location = 1) out vec4 outRT1;
void main() {
    vec4 texColor = texture(tex, vUV);
    vec3 multRGB = texColor.rgb * vColorMul.rgb;
    vec3 finalRGB = mix(multRGB, vMixColor.rgb, vMixFactor);
    float finalAlpha = texColor.a * vColorMul.a;
    if (finalAlpha < 1.0/255.0) discard;
    outRT0 = vec4(finalRGB, finalAlpha);
    outRT1 = vec4(finalAlpha, 0.0, 0.0, finalAlpha);
}
)";

static const char *kGLSL_ShadowDarkenPS = R"(#version 330 core
layout(location = 0) out vec4 outColor;
void main() {
    outColor = vec4(0.5, 0.5, 0.5, 1.0);
}
)";

// ==========================================================================
// GL Helper: compile + link a program from VS and PS source
// ==========================================================================
bool DirectXCore::GL_CompileAndLinkProgram(const char *vsSrc, const char *psSrc, GLuint *outProgram)
{
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSrc, nullptr);
    glCompileShader(vs);
    GLint ok = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[1024] = {};
        glGetShaderInfoLog(vs, sizeof(log), nullptr, log);
        Logger::Raw("[GL] VS compile error: %s\n", log);
        glDeleteShader(vs);
        return false;
    }

    GLuint ps = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(ps, 1, &psSrc, nullptr);
    glCompileShader(ps);
    glGetShaderiv(ps, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[1024] = {};
        glGetShaderInfoLog(ps, sizeof(log), nullptr, log);
        Logger::Raw("[GL] PS compile error: %s\n", log);
        glDeleteShader(vs);
        glDeleteShader(ps);
        return false;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, ps);
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[1024] = {};
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        Logger::Raw("[GL] Program link error: %s\n", log);
        glDeleteProgram(prog);
        glDeleteShader(vs);
        glDeleteShader(ps);
        return false;
    }

    glDeleteShader(vs);
    glDeleteShader(ps);
    *outProgram = prog;
    return true;
}

// ==========================================================================
// GL_Init - create WGL context, load GL functions, compile shaders, set up FBOs
// ==========================================================================
bool DirectXCore::GL_Init(HWND hwnd)
{
    m_hGLDC = GetDC(hwnd);
    if (!m_hGLDC)
    {
        Logger::Raw("[GL] GL_Init: GetDC failed.\n");
        return false;
    }

    // --- Step 1: Create a dummy GL context to load WGL extensions ---
    PIXELFORMATDESCRIPTOR pfd = {sizeof(pfd), 1};
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    int pf = ChoosePixelFormat(m_hGLDC, &pfd);
    if (!pf || !SetPixelFormat(m_hGLDC, pf, &pfd))
    {
        Logger::Raw("[GL] GL_Init: ChoosePixelFormat/SetPixelFormat failed.\n");
        return false;
    }

    HGLRC tempRC = wglCreateContext(m_hGLDC);
    if (!tempRC || !wglMakeCurrent(m_hGLDC, tempRC))
    {
        Logger::Raw("[GL] GL_Init: temporary context failed.\n");
        return false;
    }

    // Load WGL_ARB_create_context and WGL_ARB_pixel_format
    auto wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    auto wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");

    if (!wglCreateContextAttribsARB)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(tempRC);
        Logger::Raw("[GL] GL_Init: WGL_ARB_create_context not supported.\n");
        return false;
    }

    // --- Step 2: Destroy dummy context, set proper pixel format ---
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(tempRC);
    ReleaseDC(hwnd, m_hGLDC);
    m_hGLDC = GetDC(hwnd); // Re-acquire DC after SetPixelFormat

    // Set the (already-set) pixel format again on the re-acquired DC
    SetPixelFormat(m_hGLDC, pf, &pfd);

    // --- Step 3: Create OpenGL 3.3 context ---
    const int contextAttribs[] = {
        0x2091 /* WGL_CONTEXT_MAJOR_VERSION_ARB */, 3,
        0x2092 /* WGL_CONTEXT_MINOR_VERSION_ARB */, 3,
        0x9126 /* WGL_CONTEXT_PROFILE_MASK_ARB */, 0x00000001 /* CORE */,
#ifndef NDEBUG
        0x2094 /* WGL_CONTEXT_FLAGS_ARB */, 0x0001 /* DEBUG */,
#endif
        0};

    m_hGLRC = wglCreateContextAttribsARB(m_hGLDC, nullptr, contextAttribs);
    if (!m_hGLRC)
    {
        // Fallback to 3.2
        const int fallbackAttribs[] = {
            0x2091, 3, 0x2092, 2, 0x9126, 0x00000001, 0};
        m_hGLRC = wglCreateContextAttribsARB(m_hGLDC, nullptr, fallbackAttribs);
        Logger::Raw("[GL] GL_Init: 3.3 context failed, trying 3.2.\n");
    }
    if (!m_hGLRC)
    {
        Logger::Raw("[GL] GL_Init: context creation failed.\n");
        return false;
    }

    if (!wglMakeCurrent(m_hGLDC, m_hGLRC))
    {
        Logger::Raw("[GL] GL_Init: wglMakeCurrent failed.\n");
        return false;
    }

    // --- Step 4: Load all OpenGL functions via glad ---
    if (!gladLoadGL())
    {
        Logger::Raw("[GL] GL_Init: gladLoadGL failed.\n");
        return false;
    }

    // Load extension: glBlendFunci (ARB_draw_buffers_blend)
    glad_glBlendFunci = (PFNGLBLENDFUNCIPROC)wglGetProcAddress("glBlendFunci");
    if (!glad_glBlendFunci)
        Logger::Raw("[GL] Warning: glBlendFunci not available - MRT blending may be incorrect.\n");

    // --- Step 5: Log GPU info ---
    Logger::Raw("\n");
    Logger::Raw("========== OpenGL Device Info ==========\n");
    Logger::Raw("[GL] Vendor:   %s\n", (const char *)glGetString(GL_VENDOR));
    Logger::Raw("[GL] Renderer: %s\n", (const char *)glGetString(GL_RENDERER));
    Logger::Raw("[GL] Version:  %s\n", (const char *)glGetString(GL_VERSION));
    Logger::Raw("[GL] GLSL:     %s\n", (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
    Logger::Raw("========================================\n");
    Logger::Raw("\n");

    RECT rc;
    GetClientRect(hwnd, &rc);
    m_clientWidth = rc.right - rc.left;
    m_clientHeight = rc.bottom - rc.top;

    if (!GL_CreateShaders())
    {
        Logger::Raw("[GL] GL_Init: GL_CreateShaders failed.\n");
        return false;
    }

    if (!GL_CreateQuadGeometry())
    {
        Logger::Raw("[GL] GL_Init: GL_CreateQuadGeometry failed.\n");
        return false;
    }

    UINT vw = (UINT)(m_clientWidth * m_renderScale);
    UINT vh = (UINT)(m_clientHeight * m_renderScale);
    if (vw == 0)
        vw = 1;
    if (vh == 0)
        vh = 1;

    if (!GL_CreateOffscreenResources())
    {
        Logger::Raw("[GL] GL_Init: GL_CreateOffscreenResources failed.\n");
        return false;
    }

    GL_EnsureFactorTexture();
    GL_EnsureScreenCopyTexture();
    GL_EnsureAlphaAccumTexture();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glClearDepth(1.0);

    m_glTrackedDepthTest = GL_TRUE;
    m_glTrackedBlend = GL_TRUE;

    m_backgroundCacheValid = false;

    return true;
}

// ==========================================================================
// GL_Cleanup
// ==========================================================================
void DirectXCore::GL_Cleanup()
{
    if (m_hGLDC && m_hGLRC)
    {
        wglMakeCurrent(m_hGLDC, m_hGLRC);

        // Delete shader programs
        GLuint progs[] = {
            m_glProgMain, m_glProgInstanced, m_glProgEffect,
            m_glProgComposite, m_glProgFinal, m_glProgLine,
            m_glProgAlphaAccum, m_glProgAlphaAccumNI, m_glProgLineMod, m_glProgShadowDarken};
        for (GLuint p : progs)
            if (p)
                glDeleteProgram(p);

        // Delete VAOs
        GLuint vaos[] = {m_glVAOQuad, m_glVAOFullscreen, m_glVAOLine};
        for (GLuint v : vaos)
            if (v)
                glDeleteVertexArrays(1, &v);

        // Delete VBOs
        GLuint vbos[] = {m_glVBOQuad, m_glVBOFullscreen, m_glVBOInstance, m_glVBOLine};
        for (GLuint b : vbos)
            if (b)
                glDeleteBuffers(1, &b);

        // Delete FBOs + textures + RBO
        if (m_glFBOOffscreen)
            glDeleteFramebuffers(1, &m_glFBOOffscreen);
        if (m_glTexOffscreen)
            glDeleteTextures(1, &m_glTexOffscreen);
        if (m_glRBODepthStencil)
            glDeleteRenderbuffers(1, &m_glRBODepthStencil);
        if (m_glFBOFactor)
            glDeleteFramebuffers(1, &m_glFBOFactor);
        if (m_glTexFactor)
            glDeleteTextures(1, &m_glTexFactor);
        if (m_glTexScreenCopy)
            glDeleteTextures(1, &m_glTexScreenCopy);
        if (m_glFBOAlphaAccum)
            glDeleteFramebuffers(1, &m_glFBOAlphaAccum);
        if (m_glTexAlphaAccum)
            glDeleteTextures(1, &m_glTexAlphaAccum);

        // Delete texture GL handles in texture maps
        for (auto &[k, v] : m_textureMap)
            if (v && v->glTexture)
                glDeleteTextures(1, &v->glTexture);
        for (auto &[k, v] : m_tileTextureMap)
            if (v && v->glTexture)
                glDeleteTextures(1, &v->glTexture);
        for (auto &[k, v] : m_bitmapTextureMap)
            if (v && v->glTexture)
                glDeleteTextures(1, &v->glTexture);

        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(m_hGLRC);
    }

    if (m_hGLDC)
    {
        ReleaseDC(WindowFromDC(m_hGLDC), m_hGLDC);
        m_hGLDC = nullptr;
    }
    m_hGLRC = nullptr;

    m_glProgMain = m_glProgInstanced = m_glProgEffect = 0;
    m_glProgComposite = m_glProgFinal = m_glProgLine = 0;
    m_glProgAlphaAccum = m_glProgAlphaAccumNI = m_glProgLineMod = m_glProgShadowDarken = 0;
    m_glVAOQuad = m_glVAOFullscreen = m_glVAOLine = 0;
    m_glVBOQuad = m_glVBOFullscreen = m_glVBOInstance = m_glVBOLine = 0;
    m_glInstanceVBCapacity = m_glLineVBCapacity = 0;
    m_glFBOOffscreen = m_glTexOffscreen = m_glRBODepthStencil = 0;
    m_glFBOFactor = m_glTexFactor = m_glTexScreenCopy = 0;
    m_glFBOAlphaAccum = m_glTexAlphaAccum = 0;

    m_glTrackedProgram = m_glTrackedTexture = m_glTrackedTexture1 = 0;
    m_glTrackedVAO = m_glTrackedFBO = 0;
}

// ==========================================================================
// GL_CreateShaders
// ==========================================================================
bool DirectXCore::GL_CreateShaders()
{
    if (!GL_CompileAndLinkProgram(kGLSL_MainVS, kGLSL_MainPS, &m_glProgMain))
    {
        Logger::Raw("[GL] Main program failed.\n");
        return false;
    }
    if (!GL_CompileAndLinkProgram(kGLSL_InstancedVS, kGLSL_MainPS, &m_glProgInstanced))
    {
        Logger::Raw("[GL] Instanced program failed.\n");
        return false;
    }
    if (!GL_CompileAndLinkProgram(kGLSL_EffectVS, kGLSL_EffectPS, &m_glProgEffect))
    {
        Logger::Raw("[GL] Effect program failed.\n");
        return false;
    }
    if (!GL_CompileAndLinkProgram(kGLSL_CompositeVS, kGLSL_CompositePS, &m_glProgComposite))
    {
        Logger::Raw("[GL] Composite program failed.\n");
        return false;
    }
    if (!GL_CompileAndLinkProgram(kGLSL_FinalVS, kGLSL_FinalPS, &m_glProgFinal))
    {
        Logger::Raw("[GL] Final program failed.\n");
        return false;
    }
    if (!GL_CompileAndLinkProgram(kGLSL_LineVS, kGLSL_LinePS, &m_glProgLine))
    {
        Logger::Raw("[GL] Line program failed.\n");
        return false;
    }
    if (!GL_CompileAndLinkProgram(kGLSL_InstancedVS, kGLSL_AlphaAccumPS, &m_glProgAlphaAccum))
    {
        Logger::Raw("[GL] AlphaAccum program failed.\n");
        return false;
    }
    // Non-instanced version for per-quad Phase 2 rendering
    if (!GL_CompileAndLinkProgram(kGLSL_MainVS, kGLSL_AlphaAccumPS, &m_glProgAlphaAccumNI))
    {
        Logger::Raw("[GL] AlphaAccumNI program failed.\n");
        return false;
    }
    if (!GL_CompileAndLinkProgram(kGLSL_LineVS, kGLSL_LineModPS, &m_glProgLineMod))
    {
        Logger::Raw("[GL] LineMod program failed.\n");
        return false;
    }
    if (!GL_CompileAndLinkProgram(kGLSL_FinalVS, kGLSL_ShadowDarkenPS, &m_glProgShadowDarken))
    {
        Logger::Raw("[GL] ShadowDarken program failed.\n");
        return false;
    }

    Logger::Raw("[GL] All shader programs compiled successfully.\n");
    return true;
}

// ==========================================================================
// GL_CreateQuadGeometry - unit quad (centered at origin) and fullscreen quad
// ==========================================================================
bool DirectXCore::GL_CreateQuadGeometry()
{
    // Unit quad vertices (centered at origin, Y-flipped since we flip textures on upload)
    float quadVerts[] = {
        // pos(x,y,z)      uv(u,v)
        -0.5f,
        -0.5f,
        0.0f,
        0.0f,
        1.0f,
        -0.5f,
        0.5f,
        0.0f,
        0.0f,
        0.0f,
        0.5f,
        -0.5f,
        0.0f,
        1.0f,
        1.0f,
        0.5f,
        0.5f,
        0.0f,
        1.0f,
        0.0f,
    };

    glGenVertexArrays(1, &m_glVAOQuad);
    glBindVertexArray(m_glVAOQuad);
    glGenBuffers(1, &m_glVBOQuad);
    glBindBuffer(GL_ARRAY_BUFFER, m_glVBOQuad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1); // uv
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glBindVertexArray(0);

    // Fullscreen quad (NDC [-1,1])
    float fsVerts[] = {
        -1.0f,
        -1.0f,
        0.0f,
        0.0f,
        1.0f,
        -1.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
        -1.0f,
        0.0f,
        1.0f,
        1.0f,
        1.0f,
        1.0f,
        0.0f,
        1.0f,
        0.0f,
    };

    glGenVertexArrays(1, &m_glVAOFullscreen);
    glBindVertexArray(m_glVAOFullscreen);
    glGenBuffers(1, &m_glVBOFullscreen);
    glBindBuffer(GL_ARRAY_BUFFER, m_glVBOFullscreen);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fsVerts), fsVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glBindVertexArray(0);

    // Line VAO (dynamic VBO created on demand in GL_FlushLineBatch)
    glGenVertexArrays(1, &m_glVAOLine);

    return true;
}

// ==========================================================================
// GL_CreateOffscreenResources - FBO with color + depth/stencil
// ==========================================================================
bool DirectXCore::GL_CreateOffscreenResources()
{
    UINT w = (UINT)(m_clientWidth * m_renderScale);
    UINT h = (UINT)(m_clientHeight * m_renderScale);
    if (w == 0)
        w = 1;
    if (h == 0)
        h = 1;

    // Color texture
    glGenTextures(1, &m_glTexOffscreen);
    glBindTexture(GL_TEXTURE_2D, m_glTexOffscreen);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Depth/stencil renderbuffer
    glGenRenderbuffers(1, &m_glRBODepthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, m_glRBODepthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);

    // FBO
    glGenFramebuffers(1, &m_glFBOOffscreen);
    glBindFramebuffer(GL_FRAMEBUFFER, m_glFBOOffscreen);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_glTexOffscreen, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_glRBODepthStencil);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        Logger::Raw("[GL] GL_CreateOffscreenResources: FBO incomplete.\n");
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void DirectXCore::GL_EnsureFactorTexture()
{
    UINT w = (UINT)(m_clientWidth * m_renderScale);
    UINT h = (UINT)(m_clientHeight * m_renderScale);
    if (w == 0)
        w = 1;
    if (h == 0)
        h = 1;

    if (m_glTexFactor)
        glDeleteTextures(1, &m_glTexFactor);
    if (m_glFBOFactor)
        glDeleteFramebuffers(1, &m_glFBOFactor);

    glGenTextures(1, &m_glTexFactor);
    glBindTexture(GL_TEXTURE_2D, m_glTexFactor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, w, h, 0, GL_RED, GL_HALF_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &m_glFBOFactor);
    glBindFramebuffer(GL_FRAMEBUFFER, m_glFBOFactor);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_glTexFactor, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DirectXCore::GL_EnsureScreenCopyTexture()
{
    UINT w = (UINT)(m_clientWidth * m_renderScale);
    UINT h = (UINT)(m_clientHeight * m_renderScale);
    if (w == 0)
        w = 1;
    if (h == 0)
        h = 1;

    if (m_glTexScreenCopy)
        glDeleteTextures(1, &m_glTexScreenCopy);

    glGenTextures(1, &m_glTexScreenCopy);
    glBindTexture(GL_TEXTURE_2D, m_glTexScreenCopy);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void DirectXCore::GL_EnsureAlphaAccumTexture()
{
    UINT w = (UINT)(m_clientWidth * m_renderScale);
    UINT h = (UINT)(m_clientHeight * m_renderScale);
    if (w == 0)
        w = 1;
    if (h == 0)
        h = 1;

    if (m_glTexAlphaAccum)
        glDeleteTextures(1, &m_glTexAlphaAccum);
    if (m_glFBOAlphaAccum)
        glDeleteFramebuffers(1, &m_glFBOAlphaAccum);

    glGenTextures(1, &m_glTexAlphaAccum);
    glBindTexture(GL_TEXTURE_2D, m_glTexAlphaAccum);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &m_glFBOAlphaAccum);
    glBindFramebuffer(GL_FRAMEBUFFER, m_glFBOAlphaAccum);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_glTexAlphaAccum, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DirectXCore::GL_CopyScreenToTexture()
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_glFBOOffscreen);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindTexture(GL_TEXTURE_2D, m_glTexScreenCopy);
    UINT w = (UINT)(m_clientWidth * m_renderScale);
    UINT h = (UINT)(m_clientHeight * m_renderScale);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w, h);
    // Restore draw FBO
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_glFBOOffscreen);
}

void DirectXCore::GL_DrawFullscreenQuad()
{
    glBindVertexArray(m_glVAOFullscreen);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// ==========================================================================
// GL Texture Upload Helpers
// ==========================================================================
void DirectXCore::GL_UploadTextureRGBA8(TextureResource *res, int w, int h, const uint32_t *pixels, bool flipY)
{
    if (res->glTexture == 0)
        glGenTextures(1, &res->glTexture);

    glBindTexture(GL_TEXTURE_2D, res->glTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    if (flipY)
    {
        std::vector<uint32_t> flipped(w * h);
        for (int y = 0; y < h; ++y)
            memcpy(&flipped[(h - 1 - y) * w], &pixels[y * w], w * 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, flipped.data());
    }
    else
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    }
}

void DirectXCore::GL_UploadTextureR8(TextureResource *res, int w, int h, const uint8_t *pixels, bool flipY)
{
    if (res->glTexture == 0)
        glGenTextures(1, &res->glTexture);

    glBindTexture(GL_TEXTURE_2D, res->glTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (flipY)
    {
        std::vector<uint8_t> flipped(w * h);
        for (int y = 0; y < h; ++y)
            memcpy(&flipped[(h - 1 - y) * w], &pixels[y * w], w);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, flipped.data());
    }
    else
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
    }
}

void DirectXCore::GL_UploadTextureDynamic(TextureResource *res, int w, int h, const uint32_t *pixels)
{
    // Used by DrawShapes - re-uploads to existing texture
    if (res->glTexture == 0)
    {
        glGenTextures(1, &res->glTexture);
        glBindTexture(GL_TEXTURE_2D, res->glTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, res->glTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    }
}

// ==========================================================================
// GL_RenderOffscreenContent - the main 5-phase render (OpenGL path)
// ==========================================================================
void DirectXCore::GL_RenderOffscreenContent()
{
    UINT vw = (UINT)(m_clientWidth * m_renderScale);
    UINT vh = (UINT)(m_clientHeight * m_renderScale);
    if (vw == 0)
        vw = 1;
    if (vh == 0)
        vh = 1;
    m_vwCached = (float)vw;
    m_vhCached = (float)vh;

    glBindFramebuffer(GL_FRAMEBUFFER, m_glFBOOffscreen);
    m_glTrackedFBO = m_glFBOOffscreen;

    float clearColor[4] = {
        CIsoViewExt::RenderingMap ? 0.0f : (ExtConfigs::EnableDarkMode ? 0.125f : 1.0f),
        CIsoViewExt::RenderingMap ? 0.0f : (ExtConfigs::EnableDarkMode ? 0.125f : 1.0f),
        CIsoViewExt::RenderingMap ? 0.0f : (ExtConfigs::EnableDarkMode ? 0.125f : 1.0f),
        1.0f};
    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    glClearDepth(1.0);
    glClearStencil(0);
    // Ensure writemasks don't block the clear (OpenGL clear is affected by
    // glDepthMask / glStencilMask, unlike D3D11 ClearDepthStencilView).
    glDepthMask(GL_TRUE);
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glViewport(0, 0, vw, vh);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_STENCIL_TEST);

    const float depthScale = 1.0f / 16777216.0f;

    // Lambda: set common uniforms for the Main PS program
    auto SetMainUniforms = [&](const DrawCommand &cmd)
    {
        const auto &p = cmd.params;
        GLint loc;
        loc = glGetUniformLocation(m_glProgMain, "g_ColorMul");
        if (loc >= 0)
            glUniform4f(loc, p.redMult, p.greenMult, p.blueMult, p.opacity);
        loc = glGetUniformLocation(m_glProgMain, "g_MixColor");
        if (loc >= 0)
            glUniform4f(loc, p.mixR, p.mixG, p.mixB, 1.0f);
        loc = glGetUniformLocation(m_glProgMain, "g_MixFactor");
        if (loc >= 0)
            glUniform1f(loc, p.mixFactor);
    };

    // Compute world matrix (same as D3D: scale * translate, transposed)
    auto ComputeWorldMatrix = [&](const DrawCommand &cmd) -> float *
    {
        static float mat[16];
        TextureResource *tex = cmd.texRes;
        if (!tex)
        {
            memset(mat, 0, sizeof(mat));
            mat[0] = mat[5] = mat[10] = mat[15] = 1.0f;
            return mat;
        }
        const auto &p = cmd.params;
        float depthZ = cmd.depth * depthScale;
        float w_px = tex->sourceView.FullWidth * p.scaleX;
        float h_px = tex->sourceView.FullHeight * p.scaleY;
        float snappedX = std::floor(p.x + 0.5f);
        float snappedY = std::floor(p.y + 0.5f);
        float centerX = snappedX + w_px * 0.5f;
        float centerY = snappedY + h_px * 0.5f;
        float ndcW = (w_px / vw) * 2.0f;
        float ndcH = (h_px / vh) * 2.0f;
        float ndcX = (centerX / vw) * 2.0f - 1.0f;
        float ndcY = 1.0f - (centerY / vh) * 2.0f;

        // Build column-major: S * T = [sx 0 0 tx; 0 sy 0 ty; 0 0 1 tz; 0 0 0 1]
        memset(mat, 0, sizeof(mat));
        mat[0] = ndcW;
        mat[5] = ndcH;
        mat[10] = 1.0f;
        mat[15] = 1.0f;
        mat[12] = ndcX;
        mat[13] = ndcY;
        mat[14] = depthZ;
        return mat;
    };

    // Classify commands (same as D3D)
    std::vector<const DrawCommand *> opaqueCmds, stencilCmds, transparentCmds;
    std::vector<const DrawCommand *> effectCmds, overlayCmds;

    for (const auto &cmd : m_drawCommands)
    {
        if (cmd.bAlwaysOnTop)
        {
            overlayCmds.push_back(&cmd);
            continue;
        }
        if (cmd.bStencilDraw)
            stencilCmds.push_back(&cmd);
        if (cmd.bIsEffect)
            effectCmds.push_back(&cmd);
        if (!cmd.bScreenSpace && !cmd.bStencilDraw && !cmd.bIsEffect)
        {
            TextureResource *tex = cmd.texRes;
            if (tex && tex->glTexture && !tex->bIsIndexTexture)
            {
                if (cmd.params.opacity < 1.0f - 1e-6f)
                    transparentCmds.push_back(&cmd);
                else
                    opaqueCmds.push_back(&cmd);
            }
        }
    }

    // ---- Phase 1: Opaque (depth write ON, depth test ON) ----
    //   Split: stencil-writing commands (pCustomDSState != null) are drawn
    //   per-quad with stencil enabled; plain commands skip stencil.
    glUseProgram(m_glProgMain);
    m_glTrackedProgram = m_glProgMain;
    glBindVertexArray(m_glVAOQuad);
    m_glTrackedVAO = m_glVAOQuad;

    if (!opaqueCmds.empty())
    {
        // Separate plain vs. stencil-writing commands (matching D3D Phase 1 split)
        std::vector<const DrawCommand *> opaquePlain;
        std::vector<const DrawCommand *> opaqueStencil;
        for (const DrawCommand *cmd : opaqueCmds)
        {
            if (!cmd->texRes || cmd->texRes->glTexture == 0)
                continue;
            // GL path: D3D11 state objects are never created - use
            // bWriteStencil instead of pCustomDSState to detect stencil writes.
            if (cmd->params.bWriteStencil)
                opaqueStencil.push_back(cmd);
            else
                opaquePlain.push_back(cmd);
        }

        // -- Draw stencil-writing opaque first (D3D: m_pDepthStateObjectStencilWrite) --
        //   Depth: GREATER_EQUAL+write, Stencil: ALWAYS/REPLACE, ReadMask=0x7F, WriteMask=0xFF
        if (!opaqueStencil.empty())
        {
            glEnable(GL_STENCIL_TEST);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glStencilMask(0xFF);
            for (const DrawCommand *cmd : opaqueStencil)
            {
                int ref = (cmd->params.stencilRef >= 0) ? cmd->params.stencilRef : 0;
                glStencilFunc(GL_ALWAYS, ref, 0x7F);
                float *mat = ComputeWorldMatrix(*cmd);
                GLint loc = glGetUniformLocation(m_glProgMain, "g_World");
                if (loc >= 0)
                    glUniformMatrix4fv(loc, 1, GL_FALSE, mat);
                SetMainUniforms(*cmd);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, cmd->texRes->glTexture);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
            glDisable(GL_STENCIL_TEST);
        }

        // -- Plain opaque --
        for (const DrawCommand *cmd : opaquePlain)
        {
            float *mat = ComputeWorldMatrix(*cmd);
            GLint loc = glGetUniformLocation(m_glProgMain, "g_World");
            if (loc >= 0)
                glUniformMatrix4fv(loc, 1, GL_FALSE, mat);
            SetMainUniforms(*cmd);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cmd->texRes->glTexture);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
    }

    glFlush();

    // ---- Phase 1.5: Stencil-aware draws (only if PreciseDepthCalculation) ----
    //   Must match D3D11's 4 sub-phases exactly: depth func/write, stencil func/op/mask.
    if (ExtConfigs::PreciseDepthCalculation && !stencilCmds.empty())
    {
        glEnable(GL_STENCIL_TEST);

        // -- Sub-phase a: Write stencil (no color) --
        //   D3D: m_pDepthStateStencilOnlyWrite
        //   Depth=GREATER_EQUAL(read-only), Stencil=GREATER/REPLACE, ReadMask=0x7F, WriteMask=0xFF
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_FALSE); // D3D: DepthWriteMask = ZERO
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glStencilMask(0xFF);
        for (const DrawCommand *cmd : stencilCmds)
        {
            if (cmd->params.bIsShadow || cmd->params.bIsOverlapShadow ||
                !cmd->params.bWriteStencil || !cmd->bStencilOnly)
                continue;
            if (!cmd->texRes || cmd->texRes->glTexture == 0)
                continue;
            int ref = (cmd->params.stencilRef >= 0) ? cmd->params.stencilRef : 0;
            glStencilFunc(GL_GREATER, ref, 0x7F); // D3D_COMPARISON_GREATER
            float *mat = ComputeWorldMatrix(*cmd);
            GLint loc = glGetUniformLocation(m_glProgMain, "g_World");
            if (loc >= 0)
                glUniformMatrix4fv(loc, 1, GL_FALSE, mat);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cmd->texRes->glTexture);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        // -- Sub-phase b: Read stencil (normal draw) --
        //   D3D: m_pDepthStateTerrainRedraw
        //   Depth=GREATER_EQUAL(write), Stencil=GREATER_EQUAL/REPLACE, ReadMask=0x7F, WriteMask=0xFF
        glDepthMask(GL_TRUE); // D3D: DepthWriteMask = ALL
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glStencilMask(0xFF);
        for (const DrawCommand *cmd : stencilCmds)
        {
            if (cmd->params.bIsShadow || cmd->params.bIsOverlapShadow ||
                cmd->params.bWriteStencil)
                continue;
            if (!cmd->texRes || cmd->texRes->glTexture == 0)
                continue;
            int ref = (cmd->params.stencilRef >= 0) ? cmd->params.stencilRef : 0;
            glStencilFunc(GL_GEQUAL, ref, 0x7F); // D3D_COMPARISON_GREATER_EQUAL
            float *mat = ComputeWorldMatrix(*cmd);
            GLint loc = glGetUniformLocation(m_glProgMain, "g_World");
            if (loc >= 0)
                glUniformMatrix4fv(loc, 1, GL_FALSE, mat);
            SetMainUniforms(*cmd);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cmd->texRes->glTexture);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        // -- Sub-phase c: Shadow objects (write stencil bit 7, no color) --
        //   D3D: m_pDepthStateShadowMark + m_pBlendStateNoColor
        //   Depth=ALWAYS(read-only), Stencil=GREATER_EQUAL/REPLACE, ReadMask=0x7F, WriteMask=0x80
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_FALSE);  // D3D: DepthWriteMask = ZERO
        glDepthFunc(GL_ALWAYS); // D3D: DepthFunc = ALWAYS (pass regardless of depth)
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glStencilMask(0x80);
        for (const DrawCommand *cmd : stencilCmds)
        {
            if (!cmd->params.bIsShadow || cmd->params.bIsOverlapShadow || cmd->bStencilOnly)
                continue;
            if (!cmd->texRes || cmd->texRes->glTexture == 0)
                continue;
            // D3D stencilRef = (shadowHeight+1) | 0x80; GL REPLACE writes full ref,
            // mask limits to bit 7.  The 0x7F mask in glStencilFunc already handles
            // the comparison masking - do NOT &0x7F the ref, or REPLACE loses bit 7.
            int ref = cmd->params.stencilRef >= 0 ? cmd->params.stencilRef : 0;
            glStencilFunc(GL_GEQUAL, ref, 0x7F); // compare only height bits
            float *mat = ComputeWorldMatrix(*cmd);
            GLint loc = glGetUniformLocation(m_glProgMain, "g_World");
            if (loc >= 0)
                glUniformMatrix4fv(loc, 1, GL_FALSE, mat);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cmd->texRes->glTexture);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        // -- Sub-phase d: Overlap shadow draw --
        //   D3D: m_pDepthStateShadowRedraw
        //   Depth=ALWAYS(read-only), Stencil=GREATER_EQUAL/KEEP, ReadMask=0x7F, WriteMask=0x00
        //   (depth func/read-only already set from sub-phase c)
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilMask(0x00);
        for (const DrawCommand *cmd : stencilCmds)
        {
            if (!cmd->params.bIsOverlapShadow || cmd->bStencilOnly)
                continue;
            if (!cmd->texRes || cmd->texRes->glTexture == 0)
                continue;
            int ref = (cmd->params.stencilRef >= 0) ? cmd->params.stencilRef : 0;
            glStencilFunc(GL_GEQUAL, ref, 0x7F);
            SetMainUniforms(*cmd);
            float *mat = ComputeWorldMatrix(*cmd);
            GLint loc = glGetUniformLocation(m_glProgMain, "g_World");
            if (loc >= 0)
                glUniformMatrix4fv(loc, 1, GL_FALSE, mat);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cmd->texRes->glTexture);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        // -- Shadow darkening pass (fullscreen) --
        //   D3D: m_pDepthStateShadowDarken + m_pBlendStateDarken + m_pShadowDarkenPS
        //   Stencil=EQUAL 0x80 / ReadMask=0x80, Blend=ZERO:SRC_COLOR, Depth=OFF
        if (CIsoViewExt::DrawShadows && m_glProgShadowDarken)
        {
            glUseProgram(m_glProgShadowDarken);
            glDisable(GL_DEPTH_TEST);
            glStencilFunc(GL_EQUAL, 0x80, 0x80);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glStencilMask(0x00);
            glBlendFunc(GL_ZERO, GL_SRC_COLOR);
            glBindVertexArray(m_glVAOFullscreen);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            // Restore
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL); // reset from sub-phase c GL_ALWAYS
            glStencilMask(0xFF);    // reset from sub-phase d / darken 0x00
            glBindVertexArray(m_glVAOQuad);
            glUseProgram(m_glProgMain);
        }

        // Restore depth/stencil to default for subsequent phases
        glDisable(GL_STENCIL_TEST);
        glStencilMask(0xFF);
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_TRUE);
        glFlush();
    }

    // ---- Phase 2: Semi-transparent (MRT alpha accumulation) ----
    if (!transparentCmds.empty())
    {
        // Attach alpha accum texture to offscreen FBO as second color attachment
        glBindFramebuffer(GL_FRAMEBUFFER, m_glFBOOffscreen);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_glTexAlphaAccum, 0);
        GLenum drawBufs[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, drawBufs);

        // Clear alpha accum
        float alphaClear[4] = {0, 0, 0, 0};
        glClearBufferfv(GL_COLOR, 1, alphaClear);

        glDepthMask(GL_FALSE);
        glUseProgram(m_glProgAlphaAccumNI);
        m_glTrackedProgram = m_glProgAlphaAccumNI;

        // Setup blend for MRT
        glEnable(GL_BLEND);
        if (glBlendFunci)
        {
            // RT0: standard alpha
            glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            // RT1: overwrite (ONE, ZERO) red channel only
            glBlendFunci(1, GL_ONE, GL_ZERO);
        }
        glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColorMaski(1, GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);

        for (const DrawCommand *cmd : transparentCmds)
        {
            if (!cmd->texRes || cmd->texRes->glTexture == 0)
                continue;
            float *mat = ComputeWorldMatrix(*cmd);
            GLint loc = glGetUniformLocation(m_glProgAlphaAccumNI, "g_World");
            if (loc >= 0)
                glUniformMatrix4fv(loc, 1, GL_FALSE, mat);
            loc = glGetUniformLocation(m_glProgAlphaAccumNI, "g_ColorMul");
            if (loc >= 0)
                glUniform4f(loc, cmd->params.redMult, cmd->params.greenMult,
                            cmd->params.blueMult, cmd->params.opacity);
            loc = glGetUniformLocation(m_glProgAlphaAccumNI, "g_MixColor");
            if (loc >= 0)
                glUniform4f(loc, cmd->params.mixR, cmd->params.mixG, cmd->params.mixB, 1.0f);
            loc = glGetUniformLocation(m_glProgAlphaAccumNI, "g_MixFactor");
            if (loc >= 0)
                glUniform1f(loc, cmd->params.mixFactor);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cmd->texRes->glTexture);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        // Restore single RT + normal blend
        glDrawBuffers(1, drawBufs); // RT0 only
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColorMaski(0, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glColorMaski(1, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
        glUseProgram(m_glProgMain);
        glFlush();
    }

    // ---- Phase 3: World-space lines with alpha modulation ----
    {
        glDepthMask(GL_FALSE);
        GL_FlushLineBatch(false, m_glProgLineMod);
        // Unbind alpha accum texture from unit 1 (matches D3D11
        // PSSetShaderResources(1, 1, &nullSRV) after Phase 3).
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFlush();
    }

    // Restore
    glUseProgram(m_glProgMain);
    glBindVertexArray(m_glVAOQuad);
    glDepthMask(GL_TRUE);

    // ---- Phase 4: Effects (index textures -> factor -> composite) ----
    if (!effectCmds.empty())
    {
        GL_CopyScreenToTexture();

        // Clear factor texture
        glBindFramebuffer(GL_FRAMEBUFFER, m_glFBOFactor);
        float one[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        glClearBufferfv(GL_COLOR, 0, one);

        glUseProgram(m_glProgEffect);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
        glBindVertexArray(m_glVAOQuad);

        for (const DrawCommand *cmd : effectCmds)
        {
            if (!cmd->texRes || cmd->texRes->glTexture == 0 || !cmd->texRes->bIsIndexTexture)
                continue;
            const auto &p = cmd->params;
            float depthZ = cmd->depth * depthScale;
            float w_px = cmd->texRes->sourceView.FullWidth * p.scaleX;
            float h_px = cmd->texRes->sourceView.FullHeight * p.scaleY;
            float snappedX = std::floor(p.x + 0.5f);
            float snappedY = std::floor(p.y + 0.5f);
            float ndcW = (w_px / vw) * 2.0f, ndcH = (h_px / vh) * 2.0f;
            float ndcX = ((snappedX + w_px * 0.5f) / vw) * 2.0f - 1.0f;
            float ndcY = 1.0f - ((snappedY + h_px * 0.5f) / vh) * 2.0f;
            float mat[16] = {};
            mat[0] = ndcW;
            mat[5] = ndcH;
            mat[10] = 1.0f;
            mat[15] = 1.0f;
            mat[12] = ndcX;
            mat[13] = ndcY;
            mat[14] = depthZ;
            GLint loc = glGetUniformLocation(m_glProgEffect, "g_World");
            if (loc >= 0)
                glUniformMatrix4fv(loc, 1, GL_FALSE, mat);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cmd->texRes->glTexture);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        // Composite: offscreen = screenCopy * factor
        // D3D11 sets DSV=NULL here -> no depth test and no depth write.
        glBindFramebuffer(GL_FRAMEBUFFER, m_glFBOOffscreen);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_ONE, GL_ZERO); // disable blend (replace)
        glDisable(GL_BLEND);
        glUseProgram(m_glProgComposite);
        glBindVertexArray(m_glVAOFullscreen);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_glTexScreenCopy);
        GLint loc = glGetUniformLocation(m_glProgComposite, "screenTex");
        if (loc >= 0)
            glUniform1i(loc, 0);
        loc = glGetUniformLocation(m_glProgComposite, "factorTex");
        if (loc >= 0)
            glUniform1i(loc, 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_glTexFactor);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glBindVertexArray(m_glVAOQuad);
        glUseProgram(m_glProgMain);
    }

    // ---- Phase 5: Always-on-top overlays ----
    bool hasOverlayLines = false;
    for (const auto &le : m_lineEntries)
        if (!le.bScreenSpace && le.bAlwaysOnTop)
        {
            hasOverlayLines = true;
            break;
        }

    if (!overlayCmds.empty() || hasOverlayLines)
    {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        for (const DrawCommand *cmd : overlayCmds)
        {
            if (!cmd->texRes || cmd->texRes->glTexture == 0)
                continue;
            float *mat = ComputeWorldMatrix(*cmd);
            GLint loc = glGetUniformLocation(m_glProgMain, "g_World");
            if (loc >= 0)
                glUniformMatrix4fv(loc, 1, GL_FALSE, mat);
            SetMainUniforms(*cmd);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cmd->texRes->glTexture);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        if (hasOverlayLines)
        {
            GL_FlushLineBatch(false, 0, true);
            // Restore quad pipeline state (GL_FlushLineBatch changed
            // VAO->m_glVAOLine, program->m_glProgLine - match D3D11 Phase 5b restore).
            glUseProgram(m_glProgMain);
            glBindVertexArray(m_glVAOQuad);
        }

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
    }

    glFlush();
}

// ==========================================================================
// GL_RenderFinalToBackBuffer
// ==========================================================================
void DirectXCore::GL_RenderFinalToBackBuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_clientWidth, m_clientHeight);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glUseProgram(m_glProgFinal);
    glBindVertexArray(m_glVAOFullscreen);

    float cbData[4] = {1.0f, 1.0f, 0.0f, 0.0f};
    GLint loc = glGetUniformLocation(m_glProgFinal, "uScale");
    if (loc >= 0)
        glUniform2f(loc, cbData[0], cbData[1]);
    loc = glGetUniformLocation(m_glProgFinal, "uOffset");
    if (loc >= 0)
        glUniform2f(loc, cbData[2], cbData[3]);

    GLint filter = GL_NEAREST;
    if (m_renderScale != 1.0f && ExtConfigs::DDrawScalingBilinear &&
        !(ExtConfigs::DDrawScalingBilinear_OnlyShrink && CIsoViewExt::ScaledFactor < 1.0f))
        filter = GL_LINEAR;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_glTexOffscreen);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Restore NEAREST for offscreen texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// ==========================================================================
// GL_RenderScreenSpaceContent
// ==========================================================================
void DirectXCore::GL_RenderScreenSpaceContent()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_clientWidth, m_clientHeight);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(m_glProgMain);
    glBindVertexArray(m_glVAOQuad);

    for (const auto &cmd : m_drawCommands)
    {
        if (!cmd.bScreenSpace || cmd.bIsEffect)
            continue;
        TextureResource *tex = cmd.texRes;
        if (!tex || tex->glTexture == 0)
            continue;

        const auto &p = cmd.params;
        float screenW = (float)m_clientWidth, screenH = (float)m_clientHeight;
        float w_px = tex->sourceView.FullWidth * p.scaleX;
        float h_px = tex->sourceView.FullHeight * p.scaleY;
        float snappedX = std::floor(p.x + 0.5f);
        float snappedY = std::floor(p.y + 0.5f);
        float ndcW = (w_px / screenW) * 2.0f, ndcH = (h_px / screenH) * 2.0f;
        float ndcX = ((snappedX + w_px * 0.5f) / screenW) * 2.0f - 1.0f;
        float ndcY = 1.0f - ((snappedY + h_px * 0.5f) / screenH) * 2.0f;
        float mat[16] = {};
        mat[0] = ndcW;
        mat[5] = ndcH;
        mat[10] = 1.0f;
        mat[15] = 1.0f;
        mat[12] = ndcX;
        mat[13] = ndcY;
        mat[14] = 0.0f;
        GLint loc = glGetUniformLocation(m_glProgMain, "g_World");
        if (loc >= 0)
            glUniformMatrix4fv(loc, 1, GL_FALSE, mat);

        loc = glGetUniformLocation(m_glProgMain, "g_ColorMul");
        if (loc >= 0)
            glUniform4f(loc, p.redMult, p.greenMult, p.blueMult, p.opacity);
        loc = glGetUniformLocation(m_glProgMain, "g_MixColor");
        if (loc >= 0)
            glUniform4f(loc, p.mixR, p.mixG, p.mixB, 1.0f);
        loc = glGetUniformLocation(m_glProgMain, "g_MixFactor");
        if (loc >= 0)
            glUniform1f(loc, p.mixFactor);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex->glTexture);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    // Screen-space lines
    GL_FlushLineBatch(true);

    glFlush();
}

// ==========================================================================
// GL_DarkenOffscreen
// ==========================================================================
void DirectXCore::GL_DarkenOffscreen(float brightness)
{
    GL_CopyScreenToTexture();

    // Fill factor texture with brightness
    glBindFramebuffer(GL_FRAMEBUFFER, m_glFBOFactor);
    float factor[4] = {brightness, brightness, brightness, 1.0f};
    glClearBufferfv(GL_COLOR, 0, factor);

    // Composite back to offscreen
    // D3D11 DarkenOffscreen sets DSV=NULL -> no depth test and no depth write.
    glBindFramebuffer(GL_FRAMEBUFFER, m_glFBOOffscreen);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glUseProgram(m_glProgComposite);
    glBindVertexArray(m_glVAOFullscreen);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_glTexScreenCopy);
    GLint loc = glGetUniformLocation(m_glProgComposite, "screenTex");
    if (loc >= 0)
        glUniform1i(loc, 0);
    loc = glGetUniformLocation(m_glProgComposite, "factorTex");
    if (loc >= 0)
        glUniform1i(loc, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_glTexFactor);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(m_glVAOQuad);
    glUseProgram(m_glProgMain);
}

// ==========================================================================
// GL_FlushLineBatch
// ==========================================================================
void DirectXCore::GL_FlushLineBatch(bool bScreenSpace, GLuint overrideProgram, bool bOverlay)
{
    int numLines = 0;
    for (const auto &le : m_lineEntries)
        if (le.bScreenSpace == bScreenSpace && le.bAlwaysOnTop == bOverlay)
            ++numLines;
    if (numLines == 0)
        return;

    const int vertsPerLine = 6;
    const int totalVerts = numLines * vertsPerLine;

    // Ensure line VBO capacity
    if (m_glVBOLine == 0 || m_glLineVBCapacity < totalVerts)
    {
        if (m_glVBOLine)
            glDeleteBuffers(1, &m_glVBOLine);
        int cap = (totalVerts + 1023) & ~1023;
        glGenBuffers(1, &m_glVBOLine);
        glBindBuffer(GL_ARRAY_BUFFER, m_glVBOLine);
        glBufferData(GL_ARRAY_BUFFER, cap * 16, nullptr, GL_DYNAMIC_DRAW);
        m_glLineVBCapacity = cap;
    }

    struct LineVertex
    {
        float x, y, depth;
        uint32_t color;
    };
    std::vector<LineVertex> verts;
    verts.reserve(totalVerts);

    float vw, vh;
    if (bScreenSpace)
    {
        vw = (float)m_clientWidth;
        vh = (float)m_clientHeight;
    }
    else
    {
        vw = (float)(m_clientWidth * m_renderScale);
        vh = (float)(m_clientHeight * m_renderScale);
    }
    const float depthScale = 1.0f / 16777216.0f;

    for (const auto &le : m_lineEntries)
    {
        if (le.bScreenSpace != bScreenSpace || le.bAlwaysOnTop != bOverlay)
            continue;
        float dx = le.x1 - le.x0, dy = le.y1 - le.y0;
        float len = std::sqrt(dx * dx + dy * dy);
        float halfT = le.thickness * 0.5f;
        float nx, ny;
        if (len < 1e-6f)
        {
            nx = halfT;
            ny = 0.0f;
        }
        else
        {
            float inv = 1.0f / len;
            nx = -dy * inv * halfT;
            ny = dx * inv * halfT;
        }

        auto toNDC = [&](float px, float py) -> std::pair<float, float>
        {
            return {(px / vw) * 2.0f - 1.0f, 1.0f - (py / vh) * 2.0f};
        };
        float depthZ = le.depth * depthScale;
        auto [x0a, y0a] = toNDC(le.x0 - nx, le.y0 - ny);
        auto [x0b, y0b] = toNDC(le.x0 + nx, le.y0 + ny);
        auto [x1a, y1a] = toNDC(le.x1 - nx, le.y1 - ny);
        auto [x1b, y1b] = toNDC(le.x1 + nx, le.y1 + ny);

        // Triangle list: (V0,V1,V2) and (V1,V3,V2)
        verts.push_back({x0b, y0b, depthZ, le.color});
        verts.push_back({x0a, y0a, depthZ, le.color});
        verts.push_back({x1b, y1b, depthZ, le.color});
        verts.push_back({x0a, y0a, depthZ, le.color});
        verts.push_back({x1a, y1a, depthZ, le.color});
        verts.push_back({x1b, y1b, depthZ, le.color});
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_glVBOLine);
    glBufferSubData(GL_ARRAY_BUFFER, 0, totalVerts * 16, verts.data());

    glBindVertexArray(m_glVAOLine);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 16, (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT, 16, (void *)12);

    GLuint prog = overrideProgram ? overrideProgram : m_glProgLine;
    glUseProgram(prog);

    // Set viewport size uniform for LineMod PS
    if (prog == m_glProgLineMod)
    {
        GLint loc = glGetUniformLocation(prog, "uViewportSize");
        if (loc >= 0)
            glUniform2f(loc, vw, vh);
        // Bind alpha accum texture to unit 1
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_glTexAlphaAccum);
        loc = glGetUniformLocation(prog, "alphaAccum");
        if (loc >= 0)
            glUniform1i(loc, 1);
    }

    glDrawArrays(GL_TRIANGLES, 0, totalVerts);

    // Clean up: remove drawn entries
    std::erase_if(m_lineEntries, [bScreenSpace, bOverlay](const auto &le)
                  { return le.bScreenSpace == bScreenSpace && le.bAlwaysOnTop == bOverlay; });
}
