#pragma once

#include "../Body.h"

#include <map>
#include <vector>

struct ImageInfo
{
    FString ID;
    ImageDataClassSafe* pData;
    int CropLeft = 0;     // 有效内容左上角相对于原始图像的偏移
    int CropTop = 0;
    int CropWidth = 0;    // 裁切后的宽度
    int CropHeight = 0;
};

class GridObjectViewer
{
public:
    static GridObjectViewer Instance;

    GridObjectViewer() : m_hWnd{ NULL } {}

    BOOL OnMessage(PMSG pMsg);
    void Create(HWND hParent);
    void OnSize() const;
    void ShowWindow(bool bShow);
    void ShowWindow();
    void HideWindow();
    bool IsValid() const;
    bool IsVisible() const;
    void Clear();
    HWND GetHwnd() const;
    operator HWND() const;

private:
    static LRESULT CALLBACK SubClassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK HandleSubClassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void ComputeCroppedInfo(ImageDataClassSafe* pd, ImageInfo& outInfo);
    void FastBlitImage(HDC hdcDest, int destX, int destY, const ImageInfo& info);
    void DrawImageDataCore(HDC hdcDest, int destX, int destY, const ImageInfo& info);
    void DrawVisibleImagesOnly(HDC hdc);
    void LayoutImages();
    void UpdateScrollBars();
    void CreateDoubleBuffer(HDC hdc, int width, int height);
    void DestroyDoubleBuffer();
    COLORREF GetBackgroundColor();

    HWND m_hWnd;
    std::vector<ImageInfo> g_images;
    std::vector<std::unique_ptr<ImageDataClassSafe>> g_buildingImages;
    std::vector<RECT>    g_imageRects;
    int                  g_selectedIndex = -1;
    WNDPROC g_oldWndProc = nullptr;
    int m_nScrollX = 0; 
    int m_nScrollY = 0; 
    int m_nContentWidth = 0;
    int m_nContentHeight = 0;

    HDC     m_hMemDC = NULL;      // 离屏内存 DC
    HBITMAP m_hMemBitmap = NULL;  // 离屏位图
    HBITMAP m_hOldBitmap = NULL;  // 用于恢复
};