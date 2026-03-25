#include "GridObjectViewer.h"
#include <CFinalSunDlg.h>
#include "../../../FA2sp.h"
#include "../../../Helpers/STDHelpers.h"
#include "../../../Helpers/Translations.h"
#include "../../../Miscs/DialogStyle.h"

GridObjectViewer GridObjectViewer::Instance;
const int MIN_DISPLAY_WIDTH = 80; 
const int MIN_DISPLAY_HEIGHT = 80;

BOOL GridObjectViewer::OnMessage(PMSG pMsg)
{
    switch (pMsg->message)
    {
    case WM_RBUTTONDOWN:
        return TRUE;
    default:
        break;
    }

    return FALSE;
}

COLORREF GridObjectViewer::GetBackgroundColor()
{
    return RGB(0, 0, 0);
    //if (ExtConfigs::EnableDarkMode)
    //    return RGB(32, 32, 32);
    //return RGB(255, 255, 255);
}

void GridObjectViewer::ComputeCroppedInfo(ImageDataClassSafe* pd, ImageInfo& outInfo)
{
    outInfo.pData = pd;
    outInfo.CropLeft = 0;
    outInfo.CropTop = 0;
    outInfo.CropWidth = 0;
    outInfo.CropHeight = 0;

    if (!pd || !pd->pImageBuffer || pd->FullWidth <= 0 || pd->FullHeight <= 0)
        return;

    int w = pd->FullWidth;
    int h = pd->FullHeight;

    int minX = w;
    int maxX = -1;
    int minY = h;
    int maxY = -1;

    for (int y = 0; y < h; ++y)
    {
        LONG left = pd->pPixelValidRanges[y].First;
        LONG right = pd->pPixelValidRanges[y].Last;

        if (left > right) continue; 

        if (left < minX)  minX = left;
        if (right > maxX) maxX = right;

        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    }

    if (maxX >= minX && maxY >= minY)
    {
        outInfo.CropLeft = minX;
        outInfo.CropTop = minY;
        outInfo.CropWidth = maxX - minX + 1;
        outInfo.CropHeight = maxY - minY + 1;
    }
}

void GridObjectViewer::FastBlitImage(HDC hdcDest, int displayX, int displayY, const ImageInfo& info)
{
    auto* pd = info.pData;
    if (!pd) return;

    int cropW = info.CropWidth;
    int cropH = info.CropHeight;
    if (cropW <= 0 || cropH <= 0) return;

    int displayW = std::max(cropW, MIN_DISPLAY_WIDTH);
    int displayH = std::max(cropH, MIN_DISPLAY_HEIGHT);

    if (cropW < MIN_DISPLAY_WIDTH || cropH < MIN_DISPLAY_HEIGHT)
    {
        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = displayW;
        bmi.bmiHeader.biHeight = -displayH;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void* pBits = nullptr;
        HBITMAP hDib = CreateDIBSection(hdcDest, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
        if (!hDib || !pBits) return;

        memset(pBits, GetBackgroundColor(), static_cast<size_t>(displayH) * (((displayW * 4) + 3) & ~3));

        HDC hMemDC = CreateCompatibleDC(hdcDest);
        HBITMAP hOld = (HBITMAP)SelectObject(hMemDC, hDib);

        int offsetX = (displayW - cropW) / 2;
        int offsetY = (displayH - cropH) / 2;

        DrawImageDataCore(hMemDC, offsetX, offsetY, info);

        BitBlt(hdcDest, displayX, displayY, displayW, displayH, hMemDC, 0, 0, SRCCOPY);

        SelectObject(hMemDC, hOld);
        DeleteDC(hMemDC);
        DeleteObject(hDib);
        return;
    }

    DrawImageDataCore(hdcDest, displayX, displayY, info);
}

void GridObjectViewer::DrawImageDataCore(HDC hdcDest, int destX, int destY, const ImageInfo& info)
{
    auto* pd = info.pData;
    if (!pd) return;

    int w = pd->FullWidth;
    BYTE* src = static_cast<BYTE*>(pd->pImageBuffer.get());
    Palette* pal = pd->pPalette;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = info.CropWidth;
    bmi.bmiHeader.biHeight = -info.CropHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HBITMAP hDib = CreateDIBSection(hdcDest, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    if (!hDib || !pBits) return;

    BYTE* dest = static_cast<BYTE*>(pBits);
    int destPitch = ((info.CropWidth * 4 + 3) & ~3);

    auto bgc = GetBackgroundColor();
    for (int row = 0; row < info.CropHeight; ++row)
    {
        int srcRow = info.CropTop + row;
        LONG left = pd->pPixelValidRanges[srcRow].First;
        LONG right = pd->pPixelValidRanges[srcRow].Last;

        left = std::max(left, (LONG)info.CropLeft);
        right = std::min(right, (LONG)(info.CropLeft + info.CropWidth - 1));

        if (left > right) {
            memset(dest + row * destPitch, bgc, destPitch);
            continue;
        }

        BYTE* destRow = dest + row * destPitch;
        memset(destRow, bgc, destPitch);

        BYTE* srcPtr = src + srcRow * w + left;
        BYTE* destPtr = destRow + (left - info.CropLeft) * 4;

        for (LONG col = left; col <= right; ++col, ++srcPtr, destPtr += 4)
        {
            BYTE idx = *srcPtr;
            if (idx == 0) continue;

            BGRStruct c = pal->Data[idx];
            destPtr[0] = c.B;
            destPtr[1] = c.G;
            destPtr[2] = c.R;
            destPtr[3] = 255;
        }
    }

    HDC hMem = CreateCompatibleDC(hdcDest);
    HBITMAP hOld = (HBITMAP)SelectObject(hMem, hDib);
    BitBlt(hdcDest, destX, destY, info.CropWidth, info.CropHeight, hMem, 0, 0, SRCCOPY);

    SelectObject(hMem, hOld);
    DeleteDC(hMem);
    DeleteObject(hDib);
}

void GridObjectViewer::DrawVisibleImagesOnly(HDC hdc)
{
    RECT client{};
    GetClientRect(m_hWnd, &client);

    RECT visibleLogical = { m_nScrollX, m_nScrollY,
                            m_nScrollX + client.right,
                            m_nScrollY + client.bottom };

    for (size_t i = 0; i < g_images.size() && i < g_imageRects.size(); ++i)
    {
        RECT& displayRect = g_imageRects[i]; 

        if (displayRect.right < visibleLogical.left || displayRect.left  > visibleLogical.right ||
            displayRect.bottom < visibleLogical.top || displayRect.top   > visibleLogical.bottom)
            continue;

        int drawX = displayRect.left - m_nScrollX;
        int drawY = displayRect.top - m_nScrollY;

        FastBlitImage(hdc, drawX, drawY, g_images[i]);
    }
}

void GridObjectViewer::LayoutImages()
{
    if (g_images.empty())
    {
        m_nContentWidth = m_nContentHeight = 0;
        UpdateScrollBars();
        return;
    }

    RECT client{};
    GetClientRect(m_hWnd, &client);
    int clientWidth = client.right - client.left;

    const int padding = 20;
    const int spacing = 10;

    g_imageRects.clear();

    int x = padding;
    int y = padding;
    int rowMaxDisplayH = 0;

    for (auto& info : g_images)
    {
        int origW = info.CropWidth; 
        int origH = info.CropHeight;

        int displayW = std::max(origW, MIN_DISPLAY_WIDTH);
        int displayH = std::max(origH, MIN_DISPLAY_HEIGHT);

        if (x + displayW > clientWidth - padding && x > padding)
        {
            y += rowMaxDisplayH + spacing;
            x = padding;
            rowMaxDisplayH = 0;
        }

        int offsetX = (displayW - origW) / 2;
        int offsetY = (displayH - origH) / 2;

        RECT displayRect = { x, y, x + displayW, y + displayH };
        g_imageRects.push_back(displayRect);

        if (displayH > rowMaxDisplayH)
            rowMaxDisplayH = displayH;

        x += displayW + spacing;
    }

    if (!g_imageRects.empty())
    {
        RECT last = g_imageRects.back();
        m_nContentWidth = last.right + padding;
        m_nContentHeight = last.bottom + padding;
    }

    UpdateScrollBars();
}

void GridObjectViewer::UpdateScrollBars()
{
    RECT rc{};
    GetClientRect(m_hWnd, &rc);
    int clientW = rc.right - rc.left;
    int clientH = rc.bottom - rc.top;

    SCROLLINFO si{};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;

    si.nMin = 0;
    si.nMax = m_nContentHeight + 100;
    si.nPage = clientH;
    si.nPos = m_nScrollY;
    SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);

    si.nMax = m_nContentWidth;
    si.nPage = clientW;
    si.nPos = m_nScrollX;
    SetScrollInfo(m_hWnd, SB_HORZ, &si, TRUE);

    m_nScrollY = std::min(m_nScrollY, std::max(0, m_nContentHeight - clientH));
    m_nScrollX = std::min(m_nScrollX, std::max(0, m_nContentWidth - clientW));
}

void GridObjectViewer::CreateDoubleBuffer(HDC hdc, int width, int height)
{
    if (m_hMemDC) DestroyDoubleBuffer();

    m_hMemDC = CreateCompatibleDC(hdc);
    if (!m_hMemDC) return;

    m_hMemBitmap = CreateCompatibleBitmap(hdc, width, height);
    if (m_hMemBitmap)
    {
        m_hOldBitmap = (HBITMAP)SelectObject(m_hMemDC, m_hMemBitmap);
    }
}

void GridObjectViewer::DestroyDoubleBuffer()
{
    if (m_hOldBitmap && m_hMemDC)
        SelectObject(m_hMemDC, m_hOldBitmap);

    if (m_hMemBitmap) DeleteObject(m_hMemBitmap);
    if (m_hMemDC) DeleteDC(m_hMemDC);

    m_hMemDC = NULL;
    m_hMemBitmap = NULL;
    m_hOldBitmap = NULL;
}

LRESULT CALLBACK GridObjectViewer::SubClassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* self = reinterpret_cast<GridObjectViewer*>(
        GetWindowLongPtr(hWnd, GWLP_USERDATA)
        );

    if (self) {
        return self->HandleSubClassProc(hWnd, msg, wParam, lParam);
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK GridObjectViewer::HandleSubClassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        LayoutImages();
        break;

    case WM_SIZE:
    {
        LayoutImages();

        RECT rc;
        GetClientRect(hwnd, &rc);
        CreateDoubleBuffer(GetDC(hwnd), rc.right, rc.bottom);  

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }

    case WM_VSCROLL:
    case WM_HSCROLL:
    {
        int nBar = (msg == WM_VSCROLL) ? SB_VERT : SB_HORZ;
        int& scrollPos = (msg == WM_VSCROLL) ? m_nScrollY : m_nScrollX;

        SCROLLINFO si{};
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_ALL;
        GetScrollInfo(hwnd, nBar, &si);

        int oldPos = si.nPos;

        switch (LOWORD(wParam))
        {
        case SB_LINEUP:        si.nPos -= 30; break;
        case SB_LINEDOWN:      si.nPos += 30; break;
        case SB_PAGEUP:        si.nPos -= si.nPage; break;
        case SB_PAGEDOWN:      si.nPos += si.nPage; break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION: si.nPos = si.nTrackPos; break;
        }

        si.nPos = std::max(0, std::min(si.nPos, si.nMax - (int)si.nPage + 1));
        if (si.nPos != oldPos)
        {
            scrollPos = si.nPos;
            SetScrollInfo(hwnd, nBar, &si, TRUE);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    }

    case WM_MOUSEWHEEL:
    {
        int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        m_nScrollY -= zDelta;

        RECT rc;
        GetClientRect(hwnd, &rc);
        int maxScrollY = std::max(0, m_nContentHeight - (int)rc.bottom + 100);

        m_nScrollY = std::max(0, std::min(m_nScrollY, maxScrollY));

        SCROLLINFO si = { sizeof(SCROLLINFO), SIF_POS };
        si.nPos = m_nScrollY;
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT client;
        GetClientRect(hwnd, &client);

        if (!m_hMemDC || (client.right != GetDeviceCaps(m_hMemDC, HORZRES) /* ¼òµ¥¼ì²é */))
        {
            CreateDoubleBuffer(hdc, client.right, client.bottom);
        }

        HDC hDrawDC = m_hMemDC ? m_hMemDC : hdc;

        FillRect(hDrawDC, &client, ExtConfigs::EnableDarkMode ? DarkTheme::g_hDarkBackgroundBrush :(HBRUSH)GetStockObject(WHITE_BRUSH));

        DrawVisibleImagesOnly(hDrawDC);

        if (g_selectedIndex >= 0 && g_selectedIndex < (int)g_imageRects.size())
        {
            RECT r = g_imageRects[g_selectedIndex];
            RECT drawR = {
                r.left - m_nScrollX, r.top - m_nScrollY,
                r.right - m_nScrollX, r.bottom - m_nScrollY
            };

            RECT visibleDraw;
            if (IntersectRect(&visibleDraw, &client, &drawR))
            {
                HPEN hPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
                HPEN hOldPen = (HPEN)SelectObject(hDrawDC, hPen);
                HBRUSH hOldBrush = (HBRUSH)SelectObject(hDrawDC, GetStockObject(NULL_BRUSH));

                Rectangle(hDrawDC, drawR.left, drawR.top, drawR.right, drawR.bottom);

                SelectObject(hDrawDC, hOldBrush);
                SelectObject(hDrawDC, hOldPen);
                DeleteObject(hPen);
            }
        }

        if (m_hMemDC)
        {
            BitBlt(hdc, 0, 0, client.right, client.bottom,
                m_hMemDC, 0, 0, SRCCOPY);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
    {
        return 1;
    }

    case WM_LBUTTONDOWN:
    {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        g_selectedIndex = -1;
        for (size_t i = 0; i < g_images.size() && i < g_imageRects.size(); ++i)
        {
            RECT rect = g_imageRects[i];
            rect.left -= m_nScrollX;
            rect.right -= m_nScrollX;
            rect.top -= m_nScrollY;
            rect.bottom -= m_nScrollY;
            if (PtInRect(&rect, pt))
            {
                g_selectedIndex = (int)i;
                auto& id = g_images[i].ID;
                auto type = CLoadingExt::GetExtension()->GetItemType(id);

                switch (type)
                {
                case CLoadingExt::ObjectType::Infantry:
                    CIsoView::CurrentCommand->Type = 1;
                    break;
                case CLoadingExt::ObjectType::Terrain:
                    CIsoView::CurrentCommand->Type = 5;
                    break;
                case CLoadingExt::ObjectType::Smudge:
                    CIsoView::CurrentCommand->Type = 8;
                    break;
                case CLoadingExt::ObjectType::Vehicle:
                    CIsoView::CurrentCommand->Type = 4;
                    break;
                case CLoadingExt::ObjectType::Aircraft:
                    CIsoView::CurrentCommand->Type = 3;
                    break;
                case CLoadingExt::ObjectType::Building:
                    CIsoView::CurrentCommand->Type = 2;
                    break;
                case CLoadingExt::ObjectType::Unknown:
                default:
                    break;
                }

                CIsoView::CurrentCommand->Command = 1;
                CIsoView::CurrentCommand->Param = 1;
                CIsoView::CurrentCommand->ObjectID = id;
                break;
            }
        }
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }

    case WM_DESTROY:
        DestroyDoubleBuffer();
        Clear();
        break;
    }

    return CallWindowProc(g_oldWndProc, hwnd, msg, wParam, lParam);
}

void GridObjectViewer::Create(HWND hParent)
{
    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = static_cast<HINSTANCE>(FA2sp::hInstance);
    wc.lpszClassName = "ImageDataGallery"; 
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;

    RegisterClassEx(&wc);

    RECT rect;
    ::GetClientRect(hParent, &rect);
    m_hWnd = CreateWindowEx(NULL, "ImageDataGallery", nullptr,
        WS_CHILD | WS_BORDER | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_CLIPCHILDREN,
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hParent,
        NULL, static_cast<HINSTANCE>(FA2sp::hInstance), nullptr);
    SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);

    g_oldWndProc = (WNDPROC)SetWindowLongPtr(
        m_hWnd, GWLP_WNDPROC, (LONG_PTR)SubClassProc);

    SendMessage(m_hWnd, WM_CREATE, 0, 0);
}

void GridObjectViewer::OnSize() const
{
    RECT rect;
    ::GetClientRect(::GetParent(this->GetHwnd()), &rect);
    ::MoveWindow(this->GetHwnd(), 2, 29, rect.right - rect.left - 6, rect.bottom - rect.top - 35, FALSE);
}

void GridObjectViewer::ShowWindow(bool bShow)
{
    if (bShow)
        ShowWindow();
    else
        HideWindow();
}

void GridObjectViewer::ShowWindow()
{
    auto loadImage = [this](ImageDataClassSafe* pd, const ppmfc::CString id)
    {        
        if (!pd || !pd->pImageBuffer || pd->FullWidth <= 0 || pd->FullHeight <= 0)
            return;
        ImageInfo info;
        info.ID = id;
        ComputeCroppedInfo(pd, info);

        g_images.push_back(info);

    };
    Clear();
    auto vehicles = Variables::RulesMap.GetSection("VehicleTypes");
    for (auto& [_, obj] : vehicles)
    {
        auto imageName = CLoadingExt::GetImageName(obj, 0);
        auto pd = CLoadingExt::GetImageDataFromMap(imageName);
        loadImage(pd, obj);
    }
    auto terrains = Variables::RulesMap.GetSection("TerrainTypes");
    for (auto& [_, obj] : terrains)
    {
        auto imageName = CLoadingExt::GetImageName(obj, 0);
        auto pd = CLoadingExt::GetImageDataFromMap(imageName);
        loadImage(pd, obj);
    }
    auto buildings = Variables::RulesMap.GetSection("BuildingTypes");
    for (auto& [_, obj] : buildings)
    {
        auto imageName = CLoadingExt::GetBuildingImageName(obj, 0, 0);
        auto& clips = CLoadingExt::GetBuildingClipImageDataFromMap(imageName);
        auto pd = CLoadingExt::BindClippedImages(clips);
        g_buildingImages.push_back(std::move(pd));
        loadImage(g_buildingImages.back().get(), obj);
    }
    LayoutImages();
    ::ShowWindow(this->GetHwnd(), SW_SHOW);
}

void GridObjectViewer::HideWindow()
{
    ::ShowWindow(this->GetHwnd(), SW_HIDE);
}

bool GridObjectViewer::IsValid() const
{
    return this->GetHwnd() != NULL;
}

bool GridObjectViewer::IsVisible() const
{
    return this->IsValid() && ::IsWindowVisible(this->m_hWnd);
}

void GridObjectViewer::Clear()
{
    m_nScrollX = m_nScrollY = 0;
    g_images.clear();
    g_imageRects.clear();
    g_buildingImages.clear();
    g_selectedIndex = -1;
    InvalidateRect(m_hWnd, NULL, TRUE);
}

HWND GridObjectViewer::GetHwnd() const
{
    return this->m_hWnd;
}

GridObjectViewer::operator HWND() const
{
    return this->GetHwnd();
}
