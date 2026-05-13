#pragma once
#include <windef.h>
#include <wrl/client.h> 
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <functional>
#include <vector>
#include <memory>
#include <string>
#include <map>
#include "Body.h"
#include "..\FA2sp\Helpers\FString.h"

struct ColorMults;
class CTileBlockClass;
class RGBClass;

struct ImageDataView
{
    int FullWidth;
    int FullHeight;
    const BYTE* pImageBuffer;
    const BYTE* pOpacity;
    const Palette* pPalette;
    enum ImageDataViewType
    {
        Unknown = -1,
        ImageDataSafe,
        ImageData,
        TileBlockData,
    };
    ImageDataViewType Type = ImageDataViewType::Unknown;
    void* pOriginData;
};

struct TextureIndex {
    const void* pData;
    const BGRStruct color;

    bool operator==(const TextureIndex& other) const;
};

namespace std {
    template<>
    struct hash<TextureIndex> {
        size_t operator()(const TextureIndex& k) const noexcept {
            size_t seed = 0;
            seed ^= hash<const void*>{}(k.pData) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hash<int>{}(*reinterpret_cast<const int*>(&k.color)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };
}

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
    bool bScreenSpace = false;

    DrawParams& SetPosition(float _x, float _y) { x = _x; y = _y; return *this; }
    DrawParams& SetScale(float sx, float sy) { scaleX = sx; scaleY = sy; return *this; }
    DrawParams& SetOpacity(float o) { opacity = o; return *this; }
    DrawParams& SetColorMul(float r, float g, float b) { redMult = r; greenMult = g; blueMult = b; return *this; }
    DrawParams& SetColorMul(ColorMults mults);
    DrawParams& SetColorMix(float r, float g, float b, float factor) {
        float newFactor =
            mixFactor + factor * (1.0f - mixFactor);
        if (newFactor > 0.0001f)
        {
            float oldWeight = mixFactor * (1.0f - factor);
            float newWeight = factor;
            mixR =
                (mixR * oldWeight + r * newWeight)
                / newFactor;
            mixG =
                (mixG * oldWeight + g * newWeight)
                / newFactor;
            mixB =
                (mixB * oldWeight + b * newWeight)
                / newFactor;
        }
        mixFactor = newFactor;
        return *this;
    }
    DrawParams& SetColorMix(RGBClass color, float factor);
    DrawParams& SetScreenSpace() { bScreenSpace = true; return *this; }
};

struct TextureResource {
    ImageDataView sourceView;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    bool bIsIndexTexture = false;
};

class DirectXCore
{
public:
    DirectXCore();
    ~DirectXCore();

    bool Initialize(HWND hwnd);
    bool IsInitialized();
    void Cleanup();
    void ClearTextures();
    void ClearTileTextures();
    void OnResize(HWND hwnd);

    TextureResource* LoadTexture(const ImageDataView& view, BGRStruct color = { 0,0,0 });
    TextureResource* LoadTileTexture(CTileBlockClass* tileBlock, const ImageDataView& view);
    TextureResource* LoadIndexTexture(const ImageDataView& view);
    TextureResource* LoadBitmapTexture(FString_view name, CBitmap& bitmap, 
        bool setColorKey = true, COLORREF color = RGB(255, 255, 255));
    void RemoveBitmapTexture(FString_view name);

    TextureResource* GetTexture(void* pData, BGRStruct color = {0,0,0}) const;
    TextureResource* GetTileTexture(CTileBlockClass* tileBlock) const;
    TextureResource* GetBitmapTexture(FString_view name) const;

    void DrawTexture(TextureResource* tex, const DrawParams& params);
    void DrawTexture(TextureResource* tex, float x, float y) {
        DrawParams p; p.x = x; p.y = y; DrawTexture(tex, p);
    }

    void SetGlobalTransform(float scaleX, float scaleY, float offsetX, float offsetY);
    void ResetGlobalTransform() { SetGlobalTransform(1.0f, 1.0f, 0.0f, 0.0f); }

    void SetZoomOut(float scaleFactor);
    float GetZoomOut() const { return m_renderScale; }

    void Render();
    void RenderScreenSpaceOnly();

    // Global depth counter: each non-screen-space draw call increments this.
    // The value is written into the depth buffer so GPU GreaterEqual testing
    // produces correct occlusion automatically.
    UINT GetNextDepth() { return m_globalDepth++; }
    void ResetDepth() { m_globalDepth = 0; }
    int GetClientWidth() const { return m_clientWidth; }
    int GetClientHeight() const { return m_clientHeight; }

    // GPU line batching: DrawShapes pushes LineEntry records here instead of
    // CPU-rasterising each line into a texture.  They are all flushed in a
    // single Draw() call during RenderOffscreenContent().
    void AddLineEntry(float x0, float y0, float x1, float y1,
                      uint32_t color, float thickness, UINT depth,
                      bool bScreenSpace = false);

private:
    struct DrawCommand {
        TextureResource* texRes = nullptr;
        DrawParams params;
        bool bIsEffect = false;
        bool bScreenSpace = false;
        UINT depth = 0;
    };

    bool CreateDeviceAndSwapChain(HWND hwnd);
    bool CreateShadersAndInputLayout();
    bool CreateEffectShaders();
    bool CreateCompositeShaders();
    bool CreateQuadVertexBuffer();
    void UpdateViewportAndRTV(HWND hwnd);
    void EnsureFactorTexture(UINT width, UINT height);
    void EnsureScreenCopyTexture(UINT width, UINT height);
    void CopyScreenToTexture();
    void DrawFullscreenQuad();

    bool CreateOffscreenResources(UINT width, UINT height);
    void CreateBackgroundCacheTexture(UINT width, UINT height);
    bool CreateFinalShaders();
    bool CreateLineShaders();
    void RenderOffscreenContent();
    void RenderFinalToBackBuffer();
    void RenderScreenSpaceContent();
    void FlushLineBatch(bool bScreenSpace);

    void UpdateBackgroundCache();     
    void RestoreBackgroundFromCache();

    Microsoft::WRL::ComPtr<ID3D11Device>           m_pDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>    m_pContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain>         m_pSwapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pRTV;

    Microsoft::WRL::ComPtr<ID3D11VertexShader>     m_pVS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>      m_pPS;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>      m_pInputLayout;
    Microsoft::WRL::ComPtr<ID3D11SamplerState>     m_pSamplerLinear;
    Microsoft::WRL::ComPtr<ID3D11SamplerState>     m_pSamplerPoint;
    Microsoft::WRL::ComPtr<ID3D11SamplerState>     m_pSamplerNearestNeighbor;
    Microsoft::WRL::ComPtr<ID3D11BlendState>       m_pBlendState;
    Microsoft::WRL::ComPtr<ID3D11Buffer>           m_pQuadVB;
    Microsoft::WRL::ComPtr<ID3D11Buffer>           m_pConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer>           m_pFullscreenQuadVB;

    Microsoft::WRL::ComPtr<ID3D11VertexShader>     m_pEffectVS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>      m_pEffectPS;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>        m_pFactorTexture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_pFactorRTV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pFactorSRV;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>        m_pScreenCopy;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pScreenCopySRV;
    Microsoft::WRL::ComPtr<ID3D11BlendState>       m_pMulBlendState;

    Microsoft::WRL::ComPtr<ID3D11VertexShader>     m_pLineVS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>      m_pLinePS;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>      m_pLineInputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer>           m_pLineVB;
    int                                            m_lineVBCapacity = 0;

    Microsoft::WRL::ComPtr<ID3D11VertexShader>     m_pCompositeVS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>      m_pCompositePS;

    Microsoft::WRL::ComPtr<ID3D11VertexShader>     m_pFinalVS;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>      m_pFinalPS;
    Microsoft::WRL::ComPtr<ID3D11Buffer>           m_pFinalConstantBuffer;
    Microsoft::WRL::ComPtr<ID3D11Texture2D>        m_OffscreenTex;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_OffscreenRTV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_OffscreenSRV;

    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_pBackgroundCacheTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_pBackgroundCacheSRV;

    Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_pOffscreenDSBuffer;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   m_pOffscreenDSV;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState>  m_pDepthStateGE;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState>  m_pDepthStateReadOnlyGE;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState>  m_pDepthStateOff;

    std::unordered_map<TextureIndex, std::unique_ptr<TextureResource>> m_textureMap;
    FHashMap<std::unique_ptr<TextureResource>> m_bitmapTextureMap;
    std::unordered_map<CTileBlockClass*, std::unique_ptr<TextureResource>> m_tileTextureMap;
    std::vector<DrawCommand> m_drawCommands;

    int m_clientWidth = 0;
    int m_clientHeight = 0;
    float m_globalScaleX = 1.0f;
    float m_globalScaleY = 1.0f;
    float m_globalOffsetX = 0.0f;
    float m_globalOffsetY = 0.0f;
    float m_renderScale = 1.0f;
    bool m_bInitialized = false;
    bool m_backgroundCacheValid = false;

    UINT m_globalDepth = 0;

    // GPU line batching
    struct LineEntry {
        float x0, y0, x1, y1;
        uint32_t color;
        float thickness;
        UINT depth;
        bool bScreenSpace;
    };
    std::vector<LineEntry> m_lineEntries;

public:
    ID3D11Device* GetDevice() { return m_pDevice.Get(); }
    ID3D11DeviceContext* GetContext() { return m_pContext.Get(); }
};

struct ShapeColor {
    float r = 0.f, g = 0.f, b = 0.f, a = 1.f;

    ShapeColor() = default;
    ShapeColor(float r, float g, float b, float a = 1.f) : r(r), g(g), b(b), a(a) {}

    static ShapeColor FromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return { r / 255.f, g / 255.f, b / 255.f, a / 255.f };
    }
    static ShapeColor FromCOLORREF(COLORREF color) {
        return { GetRValue(color) / 255.f, GetGValue(color) / 255.f, GetBValue(color) / 255.f, 1.0f };
    }
    static ShapeColor Transparent() { return { 0, 0, 0, 0 }; }

    bool IsTransparent() const { return a < 1e-4f; }

    uint32_t ToRGBA8() const {
        auto c = [](float v) -> uint8_t { return (uint8_t)(v < 0 ? 0 : v > 1 ? 255 : v * 255.f + .5f); };
        return (uint32_t)c(r) | ((uint32_t)c(g) << 8) | ((uint32_t)c(b) << 16) | ((uint32_t)c(a) << 24);
    }
};

struct LineParams {
    ShapeColor color = { 1,1,1,1 }; 
    float      thickness = 1.f;     
    float      dashLength = 0.f;    
    float      gapLength = 0.f;     
    float      opacity = 1.f;       
    bool       bScreenSpace = false;
    bool       antiAlias = false;

    LineParams& SetColor(ShapeColor c) { color = c; return *this; }
    LineParams& SetThickness(float t) { thickness = t; return *this; }
    LineParams& SetOpacity(float o) { opacity = o; return *this; }
    LineParams& SetScreenSpace() { bScreenSpace = true; return *this; }
    LineParams& SetDash(float dash, float gap) { dashLength = dash; gapLength = gap; return *this; }
    LineParams& SetAntiAlias(bool b = true) { antiAlias = b; return *this; }
};

struct RectParams {
    ShapeColor borderColor = { 1,1,1,1 }; 
    ShapeColor fillColor = ShapeColor::Transparent(); 
    float      borderWidth = 1.f;    
    float      dashLength = 0.f;     
    float      gapLength = 0.f;      
    float      opacity = 1.f;        
    bool       bScreenSpace = false;

    RectParams& SetBorderColor(ShapeColor c) { borderColor = c; return *this; }
    RectParams& SetFillColor(ShapeColor c) { fillColor = c; return *this; }
    RectParams& SetBorderWidth(float w) { borderWidth = w; return *this; }
    RectParams& SetDash(float dash, float gap) { dashLength = dash; gapLength = gap; return *this; }
    RectParams& SetOpacity(float o) { opacity = o; return *this; }
    RectParams& SetScreenSpace() { bScreenSpace = true; return *this; }
    RectParams& NoBorder() { borderWidth = 0; return *this; }
};

struct EllipseParams {
    ShapeColor borderColor = { 1,1,1,1 };
    ShapeColor fillColor = ShapeColor::Transparent();
    float      borderWidth = 1.f;
    float      dashLength = 0.f;      
    float      gapLength = 0.f;
    float      opacity = 1.f;
    int        segments = 0;          
    bool       bScreenSpace = false;

    EllipseParams& SetBorderColor(ShapeColor c) { borderColor = c; return *this; }
    EllipseParams& SetFillColor(ShapeColor c) { fillColor = c; return *this; }
    EllipseParams& SetBorderWidth(float w) { borderWidth = w; return *this; }
    EllipseParams& SetDash(float dash, float gap) { dashLength = dash; gapLength = gap; return *this; }
    EllipseParams& SetOpacity(float o) { opacity = o; return *this; }
    EllipseParams& SetSegments(int s) { segments = s; return *this; }
    EllipseParams& SetScreenSpace() { bScreenSpace = true; return *this; }
    EllipseParams& NoBorder() { borderWidth = 0; return *this; }
};

class DrawShapes {
public:
    explicit DrawShapes(DirectXCore* dx);
    ~DrawShapes() = default;

    void BeginFrame();
    void EndFrame();

    void DrawLine(float x0, float y0, float x1, float y1,
        const LineParams& params = {});

    void DrawRect(float x, float y, float w, float h,
        const RectParams& params = {});

    void DrawEllipse(float cx, float cy, float rx, float ry,
        const EllipseParams& params = {});

    void DrawCircle(float cx, float cy, float radius,
        const EllipseParams& params = {}) {
        DrawEllipse(cx, cy, radius, radius, params);
    }

private:
    struct Canvas {
        std::vector<uint32_t> buf;
        int w = 0, h = 0;
        void Resize(int _w, int _h);
        void Clear();
        void SetPixel(int x, int y, uint32_t rgba);
        uint32_t BlendOver(uint32_t dst, uint32_t src) const;
        void SetPixelBlend(int x, int y, uint32_t src);
    };

    void FlushCanvas(Canvas& c, float worldX, float worldY,
        float opacity, bool bScreenSpace);

    void RasterLine(Canvas& c, float x0, float y0, float x1, float y1,
        float thickness, ShapeColor color,
        float dashLen, float gapLen, bool antiAlias = false);

    void RasterThickPoint(Canvas& c, float px, float py,
        float radius, uint32_t rgba);

    void RasterEllipseFill(Canvas& c, float cx, float cy,
        float rx, float ry, uint32_t rgba);

    DirectXCore* m_dx;

    struct TempTex {
        int w = 0, h = 0;
        std::unique_ptr<TextureResource> res;
    };
    std::vector<TempTex> m_freePool;   
    std::vector<TempTex> m_inUseList;  
    std::vector<TempTex> m_retireList; 

    TextureResource* AcquireTempTexture(int w, int h,
        const std::vector<uint32_t>& pixels);
};

enum class TextAlign
{
    Left,
    Center,
    Right
};

struct TextParams {
    FString     fontName = "Cambria";
    int             fontSize = 14;    
    bool            bold = false;
    bool            italic = false;
    bool            underline = false;
    ShapeColor      color = { 1,1,1,1 };
    ShapeColor      bgColor = ShapeColor::Transparent();
    int             paddingX = 2;        
    int             paddingY = 1;          
    int             borderThickness = 1;   
    float           opacity = 1.f;         
    bool            bScreenSpace = false;  
    bool            bBorder = false;       
    TextAlign align = TextAlign::Left;

    TextParams& SetFont(const char* name) { fontName = name; return *this; }
    TextParams& SetFontSize(int size) { fontSize = size; return *this; }
    TextParams& SetBold(bool b = true) { bold = b;      return *this; }
    TextParams& SetItalic(bool b = true) { italic = b;    return *this; }
    TextParams& SetUnderline(bool b = true) { underline = b; return *this; }
    TextParams& SetColor(ShapeColor c) { color = c;     return *this; }
    TextParams& SetBgColor(ShapeColor c) { bgColor = c;   return *this; }
    TextParams& SetPadding(int x, int y) { paddingX = x; paddingY = y; return *this; }
    TextParams& SetOpacity(float o) { opacity = o;   return *this; }
    TextParams& SetScreenSpace() { bScreenSpace = true; return *this; }
    TextParams& SetBorder() { bBorder = true; return *this; }
    TextParams& SetBorderThickness(int t) { borderThickness = t; return *this; }
    TextParams& SetAlign(TextAlign a) { align = a; return *this; }
    TextParams& SetAlignCenter() { align = TextAlign::Center; return *this; }
    TextParams& SetAlignRight() { align = TextAlign::Right; return *this; }
    TextParams& SetAlignLeft() { align = TextAlign::Left; return *this; }
};

struct TextKey {
    FString  text;
    FString  fontName;
    int          fontSize = 0;
    bool         bold = false;
    bool         italic = false;
    bool         underline = false;
    uint32_t     colorRGBA = 0;
    uint32_t     bgColorRGBA = 0;
    int          paddingX = 0;
    int          paddingY = 0;

    bool operator==(const TextKey& o) const noexcept {
        return text == o.text && fontName == o.fontName
            && fontSize == o.fontSize && bold == o.bold
            && italic == o.italic && underline == o.underline
            && colorRGBA == o.colorRGBA && bgColorRGBA == o.bgColorRGBA
            && paddingX == o.paddingX && paddingY == o.paddingY;
    }
};

namespace std {
    template<>
    struct hash<TextKey> {
        size_t operator()(const TextKey& k) const noexcept {
            size_t seed = hash<string>{}(k.text);
            auto mix = [&](size_t v) {
                seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };
            mix(hash<string>{}(k.fontName));
            mix(hash<int>{}(k.fontSize));
            mix(hash<bool>{}(k.bold));
            mix(hash<bool>{}(k.italic));
            mix(hash<bool>{}(k.underline));
            mix(hash<uint32_t>{}(k.colorRGBA));
            mix(hash<uint32_t>{}(k.bgColorRGBA));
            mix(hash<int>{}(k.paddingX));
            mix(hash<int>{}(k.paddingY));
            return seed;
        }
    };
}

struct FontKey {
    FString  fontName;
    int          fontSize = 0;
    bool         bold = false;
    bool         italic = false;
    bool         underline = false;

    bool operator==(const FontKey& o) const noexcept {
        return fontName == o.fontName && fontSize == o.fontSize
            && bold == o.bold && italic == o.italic && underline == o.underline;
    }
};

namespace std {
    template<>
    struct hash<FontKey> {
        size_t operator()(const FontKey& k) const noexcept {
            size_t seed = hash<FString>{}(k.fontName);
            auto mix = [&](size_t v) {
                seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };
            mix(hash<int>{}(k.fontSize));
            mix(hash<bool>{}(k.bold));
            mix(hash<bool>{}(k.italic));
            mix(hash<bool>{}(k.underline));
            return seed;
        }
    };
}

class TextRenderer {
public:
    explicit TextRenderer(DirectXCore* dx, size_t cacheCapacity = 512);
    ~TextRenderer();

    void DrawTexts(float x, float y, FString_view text, const TextParams& params = {});
    bool MeasureText(const FString& text, const TextParams& params, int& outW, int& outH);
    void ClearCache();
    void SetCacheCapacity(size_t cap) { m_capacity = cap; }
    size_t GetCacheSize() const { return m_cache.size(); }

private:
    struct CacheEntry {
        std::unique_ptr<TextureResource> texRes;
        int texW = 0, texH = 0;
    };

    using LruList = std::list<TextKey>;
    using CacheMap = std::unordered_map<TextKey,
        std::pair<CacheEntry, LruList::iterator>>;

    TextKey     MakeKey(const FString& text, const TextParams& p) const;
    TextureResource* Lookup(const TextKey& key);
    void        Insert(const TextKey& key, CacheEntry entry);
    void        Evict();

    bool RasterizeGDI(const FString& text, const TextParams& p,
        std::vector<uint32_t>& pixels, int& outW, int& outH);

    HFONT GetOrCreateFont(const TextParams& p);
    void  CleanupFonts();

    bool UploadTexture(TextureResource* res,
        int w, int h,
        const std::vector<uint32_t>& pixels);

    DirectXCore* m_dx;
    size_t       m_capacity;

    LruList  m_lruList;
    CacheMap m_cache;

    std::unordered_map<FontKey, HFONT> m_fontPool;

    HDC  m_hDC = nullptr;
    HBITMAP m_hBmp = nullptr;  
    int  m_bmpW = 0;
    int  m_bmpH = 0;
    void* m_pBits = nullptr; 
    void EnsureDC(int w, int h);
};
