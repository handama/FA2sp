#pragma once

#include "../Body.h"
#include "../../CLoading/Body.h"

#include <map>
#include <vector>

struct ImageInfo
{
    FString ID;
    CLoadingExt::ObjectType Type = CLoadingExt::ObjectType::Unknown;
    ImageDataClassSafe* pData;
    FString DataName;
    int CropLeft = 0;
    int CropTop = 0;
    int CropWidth = 0;
    int CropHeight = 0;
    bool isOverlay;
};
struct GroupInfo
{
    FString DisplayName;
    FString InternalName;
    std::vector<FString> IDs;
    bool ForceFront = false;
};

class GridObjectViewer
{
public:
    static GridObjectViewer Instance;

    GridObjectViewer() : m_hView{ NULL } {}

    BOOL OnMessage(PMSG pMsg);
    void Create(HWND hParent);
    void OnSize() const;
    void ShowWindow(bool bShow);
    void ShowWindow();
    void HideWindow();
    bool IsValid() const;
    bool IsVisible() const;
    void Clear();
    HWND GetView() const;
    HWND GetControl() const;
    operator HWND() const;
    void UpdateControls();
    void UpdateImages();

private:
    static LRESULT CALLBACK ViewSubClassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK HandleViewSubClassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK ControlSubClassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK HandleControlSubClassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void ComputeCroppedInfo(ImageDataClassSafe* pd, ImageInfo& outInfo);
    void FastBlitImage(HDC hdcDest, int destX, int destY, const ImageInfo& info);
    void DrawImageDataCore(HDC hdcDest, int destX, int destY, const ImageInfo& info);
    void DrawVisibleImagesOnly(HDC hdc);
    void LayoutImages();
    void UpdateScrollBars();
    void CreateDoubleBuffer(HDC hdc, int width, int height);
    void DestroyDoubleBuffer();
    void OnSelchange();
    void OnEditchange();
    COLORREF GetBackgroundColor();

    HWND m_hView;
    HWND m_hControl;
    HWND m_hControlGroup;
    HWND m_hControlSearch;
    HWND m_hControlCurrentSelect;
    int g_currentCurSel = -1;
    std::vector<GroupInfo> g_groups;
    std::vector<ImageInfo> g_filteredImages;
    std::vector<ImageInfo> g_images;
    std::vector<std::unique_ptr<ImageDataClassSafe>> g_buildingImages;
    std::vector<RECT> g_imageRects;
    int g_selectedIndex = -1;
    WNDPROC g_oldViewProc = nullptr;
    int m_nScrollX = 0; 
    int m_nScrollY = 0; 
    int m_nContentWidth = 0;
    int m_nContentHeight = 0;

    HDC m_hMemDC = NULL;
    HBITMAP m_hMemBitmap = NULL; 
    HBITMAP m_hOldBitmap = NULL; 
};