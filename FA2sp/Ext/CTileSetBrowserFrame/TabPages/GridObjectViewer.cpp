#include "GridObjectViewer.h"
#include <CFinalSunDlg.h>
#include "../../../FA2sp.h"
#include "../../../Helpers/STDHelpers.h"
#include "../../../Helpers/Translations.h"
#include "../../../Miscs/DialogStyle.h"
#include "../../../Helpers/Helper.h"

GridObjectViewer GridObjectViewer::Instance;
const int MIN_DISPLAY_WIDTH = 80; 
const int MIN_DISPLAY_HEIGHT = 80;

static std::vector<int> GetUnique(std::vector<int> input)
{
    std::sort(input.begin(), input.end());
    auto last = std::unique(input.begin(), input.end());
    input.erase(last, input.end());
    return input;
}

static bool ParseUInt8(const FString& s, int& out)
{
    if (s.empty())
        return false;

    int val = 0;
    for (char c : s)
    {
        if (c < '0' || c > '9')
            return false;

        val = val * 10 + (c - '0');
        if (val > 255)
            return false;
    }

    out = val;
    return true;
}

static bool ParseDataString(const FString& str,
    std::set<int>& AvailableData,
    int& Overlay)
{
    AvailableData.clear();
    Overlay = 0;

    auto ParseNumber = [](const FString& s, int& out, int maxVal) -> bool
    {
        if (s.empty())
            return false;

        int val = 0;
        for (char c : s)
        {
            if (c < '0' || c > '9')
                return false;

            val = val * 10 + (c - '0');
            if (val > maxVal)
            {
                out = maxVal;
                return true;
            }
        }

        out = val;
        return true;
    };

    int MaxOverlay = (ExtConfigs::ExtOverlays || CMapDataExt::NewINIFormat >= 5) ? 65535 : 255;
    std::vector<FString> parts = FString::SplitString(str, "#");

    if (parts.size() == 1)
    {
        if (!ParseNumber(parts[0], Overlay, MaxOverlay))
            return false;

        return true;
    }

    if (parts.size() != 2)
        return false;

    if (!ParseNumber(parts[0], Overlay, MaxOverlay))
        return false;

    if (parts[1].empty())
        return false;

    std::vector<FString> items = FString::SplitString(parts[1], ",");

    for (const auto& item : items)
    {
        if (item.empty())
            return false;

        size_t pos = item.find('-');

        if (pos == FString::npos)
        {
            int value = 0;
            if (!ParseNumber(item, value, 255))
                return false;

            AvailableData.insert(value);
        }
        else
        {
            if (item.find('-', pos + 1) != FString::npos)
                return false;

            FString left = item.substr(0, pos);
            FString right = item.substr(pos + 1);

            int start = 0, end = 0;
            if (!ParseNumber(left, start, 255) ||
                !ParseNumber(right, end, 255))
                return false;

            if (start > end)
                return false;

            for (int i = start; i <= end; ++i)
                AvailableData.insert(i);
        }
    }

    return true;
}

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
    if (!pd && !info.DataName.IsEmpty()) pd = CLoadingExt::GetImageDataFromMap(info.DataName);
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
    if (!pd && !info.DataName.IsEmpty()) pd = CLoadingExt::GetImageDataFromMap(info.DataName);
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
    GetClientRect(m_hView, &client);

    RECT visibleLogical = { m_nScrollX, m_nScrollY,
                            m_nScrollX + client.right,
                            m_nScrollY + client.bottom };

    for (size_t i = 0; i < g_filteredImages.size() && i < g_imageRects.size(); ++i)
    {
        RECT& displayRect = g_imageRects[i]; 

        if (displayRect.right < visibleLogical.left || displayRect.left  > visibleLogical.right ||
            displayRect.bottom < visibleLogical.top || displayRect.top   > visibleLogical.bottom)
            continue;

        int drawX = displayRect.left - m_nScrollX;
        int drawY = displayRect.top - m_nScrollY;

        FastBlitImage(hdc, drawX, drawY, g_filteredImages[i]);
    }
}

void GridObjectViewer::LayoutImages()
{
    if (g_filteredImages.empty())
    {
        m_nContentWidth = m_nContentHeight = 0;
        UpdateScrollBars();
        return;
    }

    RECT client{};
    GetClientRect(m_hView, &client);
    int clientWidth = client.right - client.left;

    const int padding = 20;
    const int spacing = 10;

    g_imageRects.clear();

    int x = padding;
    int y = padding;
    int rowMaxDisplayH = 0;

    for (auto& info : g_filteredImages)
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
    GetClientRect(m_hView, &rc);
    int clientW = rc.right - rc.left;
    int clientH = rc.bottom - rc.top;

    SCROLLINFO si{};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;

    si.nMin = 0;
    si.nMax = m_nContentHeight + 100;
    si.nPage = clientH;
    si.nPos = m_nScrollY;
    SetScrollInfo(m_hView, SB_VERT, &si, TRUE);

    si.nMax = m_nContentWidth;
    si.nPage = clientW;
    si.nPos = m_nScrollX;
    SetScrollInfo(m_hView, SB_HORZ, &si, TRUE);

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

LRESULT CALLBACK GridObjectViewer::ViewSubClassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* self = reinterpret_cast<GridObjectViewer*>(
        GetWindowLongPtr(hWnd, GWLP_USERDATA)
        );

    if (self) {
        return self->HandleViewSubClassProc(hWnd, msg, wParam, lParam);
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

BOOL CALLBACK GridObjectViewer::ControlSubClassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GridObjectViewer* self = (GridObjectViewer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    if (msg == WM_INITDIALOG)
    {
        self = (GridObjectViewer*)lParam;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)self);
    }

    if (self)
    {
        LRESULT ret = self->HandleControlSubClassProc(hWnd, msg, wParam, lParam);
        if (ret)
            return ret;
    }

    return FALSE;
}

LRESULT CALLBACK GridObjectViewer::HandleControlSubClassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        FString buffer;
        auto Translate = [&hwnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hwnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };

        Translate(1368, "GridObjectViewer.Group");
        Translate(1369, "GridObjectViewer.Search");
        Translate(1370, "GridObjectViewer.CurrentSelect");
        m_hControlGroup = GetDlgItem(hwnd, 1366);
        m_hControlSearch = GetDlgItem(hwnd, 1367);
        m_hControlCurrentSelect = GetDlgItem(hwnd, 1371);
        g_currentCurSel = -1;

        return FALSE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case 1366:
        {
            if (CODE == CBN_SELCHANGE)
                OnSelchange();
            break;
        }
        case 1367:
        {
            if (CODE == EN_CHANGE)
                OnEditchange();
            break;
        }
        }
        return TRUE;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    {
        if (ExtConfigs::EnableDarkMode)
        {
            HDC hdc = (HDC)wParam;
            HWND hCtrl = (HWND)lParam;

            SetTextColor(hdc, DarkColors::LightText);
            SetBkColor(hdc, DarkColors::Background);

            return (LRESULT)DarkTheme::g_hDarkBackgroundBrush;
        }

        break;
    }
    case WM_ERASEBKGND:
    {
        if (ExtConfigs::EnableDarkMode)
        {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hwnd, &rc);

            FillRect(hdc, &rc, DarkTheme::g_hDarkBackgroundBrush);
            return TRUE;
        }
        break;
    }
    case 114514: // used for update
    {
        UpdateControls();
        return TRUE;
    }
    case 114515: // used for update
    {
        UpdateImages();
        return TRUE;
    }
    default:
        break;
    }

    return FALSE;
}

void GridObjectViewer::UpdateControls()
{
    g_currentCurSel = -1;
    SendMessage(m_hControlGroup, CB_SETCURSEL, g_currentCurSel, NULL);
    g_groups.clear();
    std::vector<FString> translatedSides;
    std::vector<FString> engSides;

    auto trandSide = ExtraWindow::GetTranslatedSectionName("Sides");
    if (!CINI::FAData->SectionExists(trandSide))
        trandSide = "Sides";
    auto engSide = ExtraWindow::GetTranslatedSectionName("English-Sides");
    if (!CINI::FAData->SectionExists(engSide))
        engSide = "Sides";

    if (auto pSection = CINI::FAData->GetSection(trandSide))
        for (auto& [_, value] : pSection->GetEntities())
            translatedSides.push_back(value);
    if (auto pSection = CINI::FAData->GetSection(engSide))
        for (auto& [_, value] : pSection->GetEntities())
            engSides.push_back(value);

    if (auto pSection = CINI::FAData->GetSection("GridObjectViewerLists"))
    {
        std::set<int> overlayDatas;
        int overlay;
        for (auto& [_, section] : pSection->GetEntities())
        {
            if (auto pSection = CINI::FAData->GetSection(section))
            {
                auto& info = g_groups.emplace_back();
                auto lang = FinalAlertConfig::Language + "-";
                lang += "Name";
                info.DisplayName = CINI::FAData->GetString(section, lang);
                info.InternalName = section;
                info.ForceFront = true;
                if (info.DisplayName.IsEmpty())
                    info.DisplayName = info.InternalName;
                for (auto& [key, value] : pSection->GetEntities())
                {
                    if (key.Find("Name") > -1)
                        continue;

                    if (ParseDataString(value, overlayDatas, overlay))
                    {                       
                        info.IDs.push_back({ STDHelpers::IntToString(overlay), true, overlayDatas});
                    }
                    else
                    {
                        info.IDs.push_back({ value, false });
                    }
                }
            }
        }
    }

    auto addEditorCategory = [&](const char* section, CViewObjectsExt::PropertyBrushTypes type)
    {       
        for (auto& [_, obj] : Variables::RulesMap.GetSection(section))
        {
            GroupInfo* info = nullptr;
            if (ExtConfigs::GridObjectViewer_LoadEditorCategory)
            {
                if (auto cat = Variables::RulesMap.TryGetString(obj, "EditorCategory"))
                {
                    for (auto& g : g_groups)
                    {
                        if (g.InternalName == *cat)
                        {
                            info = &g;
                            break;
                        }
                    }
                    if (!info)
                    {
                        info = &g_groups.emplace_back();
                        info->DisplayName = Translations::TranslateOrDefault(*cat, *cat);
                        info->InternalName = *cat;
                    }
                    if (info)
                    {
                        if (!CViewObjectsExt::IsIgnored(obj))
                            info->IDs.push_back({ obj, false });
                    }
                }
            }

            if (type == CViewObjectsExt::PropertyBrushTypes::Set_Count)
                continue;
            if (!ExtConfigs::GridObjectViewer_LoadForceSides)
                continue;

            auto multiLayers = CViewObjectsExt::GetMultiLayers(obj);
            auto singleSides = GetUnique(CViewObjectsExt::GuessSide(obj, type));
            for (auto& sides : multiLayers)
            {
                if (!sides.empty())
                {
                    int side = sides.front();
                    if (side < 0) continue;
                    FString displayName = side >= (int)translatedSides.size() ? FString("") : translatedSides[side];
                    FString engName = side >= (int)engSides.size() ? displayName : engSides[side];
                    if (!engName.IsEmpty() && !displayName.IsEmpty())
                    {
                        info = nullptr;
                        for (auto& g : g_groups)
                        {
                            if (g.InternalName == engName)
                            {
                                info = &g;
                                break;
                            }
                        }
                        if (!info)
                        {
                            info = &g_groups.emplace_back();
                            info->DisplayName = displayName;
                            info->InternalName = engName;
                        }
                        if (info)
                        {
                            if (!CViewObjectsExt::IsIgnored(obj))
                                info->IDs.push_back({ obj, false });
                        }
                    }
                }
            }
            for (auto& side : singleSides)
            {
                if (side < 0) continue;
                FString displayName = side >= (int)translatedSides.size() ? FString("") : translatedSides[side];
                FString engName = side >= (int)engSides.size() ? displayName : engSides[side];
                if (!engName.IsEmpty() && !displayName.IsEmpty())
                {
                    info = nullptr;
                    for (auto& g : g_groups)
                    {
                        if (g.InternalName == engName)
                        {
                            info = &g;
                            break;
                        }
                    }
                    if (!info)
                    {
                        info = &g_groups.emplace_back();
                        info->DisplayName = displayName;
                        info->InternalName = engName;
                    }
                    if (info)
                    {
                        if (!CViewObjectsExt::IsIgnored(obj))
                            info->IDs.push_back({ obj, false });
                    }
                }
            }
        }
    };

    auto addObjectBrowserCategory = [&](const char* section, const char* faDataSection, bool isOverlay)
    {
        struct OBCInfo
        {
            int index;
            std::vector<FString> contains;
        };
        std::vector<OBCInfo> availableGroups;
        if (auto pSection = CINI::FAData->GetSection(faDataSection))
        {
            std::map<int, FString> collector;

            for (auto& pair : pSection->GetIndices())
                collector[pair.second] = pair.first;

            for (auto& pair : collector)
            {
                const auto& contains = FString::SplitString(pair.second, "|");
                const auto& groupName = pSection->GetEntities().find(pair.second)->second;
                GroupInfo* info = nullptr;

                if (!CViewObjectsExt::IsIgnored(groupName))
                {
                    int i = 0;
                    for (auto& g : g_groups)
                    {
                        if (g.InternalName == groupName)
                        {
                            info = &g;
                            availableGroups.push_back({ i,contains });
                            break;
                        }
                        i++;
                    }
                    if (!info)
                    {
                        info = &g_groups.emplace_back();
                        info->DisplayName = Translations::TranslateOrDefault(groupName, groupName);
                        info->InternalName = groupName;

                        availableGroups.push_back({ (int)g_groups.size() - 1,contains });
                    }
                }
            }
            if (availableGroups.empty())
                return;

            int index = -1;
            for (auto& [_, obj] : Variables::RulesMap.GetSection(section))
            {
                index++;
                if (CViewObjectsExt::IsIgnored(obj))
                    continue;

                for (auto& group : availableGroups)
                {
                    bool match = false;
                    for (auto& c : group.contains)
                    {
                        if (obj.Find(c) != -1)
                        {
                            match = true;
                            break;
                        }
                    }
                    if (match)
                    {
                        if (isOverlay)
                        {
                            g_groups[group.index].IDs.push_back({ STDHelpers::IntToString(index), true});
                        }
                        else
                        {
                            g_groups[group.index].IDs.push_back({ obj, false });
                        }                      
                    }
                }
            }
        }
    };

    if (ExtConfigs::GridObjectViewer_LoadForceSides 
        || ExtConfigs::GridObjectViewer_LoadEditorCategory
        || ExtConfigs::GridObjectViewer_LoadObjectBrowserCategory)
    {
        addEditorCategory("InfantryTypes", CViewObjectsExt::PropertyBrushTypes::Set_Infantry);
        addEditorCategory("VehicleTypes", CViewObjectsExt::PropertyBrushTypes::Set_Vehicle);
        addEditorCategory("AircraftTypes", CViewObjectsExt::PropertyBrushTypes::Set_Aircraft);
        addEditorCategory("BuildingTypes", CViewObjectsExt::PropertyBrushTypes::Set_Building);
        addEditorCategory("TerrainTypes", CViewObjectsExt::PropertyBrushTypes::Set_Count);
        addEditorCategory("SmudgeTypes", CViewObjectsExt::PropertyBrushTypes::Set_Count);

        if (ExtConfigs::GridObjectViewer_LoadObjectBrowserCategory)
        {
            addObjectBrowserCategory("SmudgeTypes", "ObjectBrowser.SmudgeTypes", false);
            addObjectBrowserCategory("TerrainTypes", "ObjectBrowser.TerrainTypes", false);
            addObjectBrowserCategory("OverlayTypes", "ObjectBrowser.Overlays", true);
        }

        int max = -1;
        std::map<FString, int> orderIndex;
        if (auto pSection = CINI::FAData->GetSection("ObjectCategoryPriorities"))
        {
            for (auto& [value, order] : pSection->GetEntities())
            {
                int o = atoi(order);
                orderIndex[value] = o;
                max = std::max(max, o);
            }
        }
        for (auto& order : engSides)
        {
            if (!orderIndex.contains(order))
                orderIndex[order] = ++max;
        }
        auto getIndex = [&](const FString& name) {
            auto it = orderIndex.find(name);
            return it != orderIndex.end() ? it->second : INT_MAX;
        };
        std::stable_sort(g_groups.begin(), g_groups.end(),
            [&](const GroupInfo& a, const GroupInfo& b) {
            auto hasA = orderIndex.contains(a.InternalName);
            auto hasB = orderIndex.contains(b.InternalName);
            if (hasA && hasB)
            {
                return getIndex(a.InternalName) < getIndex(b.InternalName);
            }
            if (a.ForceFront != b.ForceFront)
            {
                if (hasA )
                return a.ForceFront > b.ForceFront;
            }

            if (a.ForceFront)
            {               
                return false;
            }

            return getIndex(a.InternalName) < getIndex(b.InternalName);
        });
    }

    auto addGroup = [this](const char* section, const char* displayName) -> int
    {
        auto& g = g_groups.emplace_back();
        for (auto& [_, obj] : Variables::RulesMap.GetSection(section))
            if(!CViewObjectsExt::IsIgnored(obj))
                g.IDs.push_back({ obj, false });
        g.DisplayName = displayName;

        return g_groups.size() - 1;
    };
    
    auto addOverlay = [this](const char* displayName) -> int
    {
        auto& g = g_groups.emplace_back();
        int index = 0;
        FString buffer;
        for (auto& obj : Variables::RulesMap.ParseIndicies("OverlayTypes"))
        {
            if (index >= 255 && (ExtConfigs::ExtOverlays || CMapDataExt::NewINIFormat >= 5))
                break;
            if (!CViewObjectsExt::IgnoreOverlaySet.contains(obj))
            {
                buffer.Format("%d", index);
                g.IDs.push_back({ buffer, true });
            }
            index++;
        }
        g.DisplayName = displayName;

        return g_groups.size() - 1;
    };

    auto allI = addGroup("InfantryTypes", Translations::TranslateOrDefault("GridObjectViewer.AllInfantry", "All infantries"));
    auto allV = addGroup("VehicleTypes", Translations::TranslateOrDefault("GridObjectViewer.AllVehicle", "All vehicles"));
    auto allA = addGroup("AircraftTypes", Translations::TranslateOrDefault("GridObjectViewer.AllAircraft", "All aircrafts"));
    auto allB = addGroup("BuildingTypes", Translations::TranslateOrDefault("GridObjectViewer.AllBuilding", "All buildings"));
    auto allT = addGroup("TerrainTypes", Translations::TranslateOrDefault("GridObjectViewer.AllTerrain", "All terrain types"));
    auto allS = addGroup("SmudgeTypes", Translations::TranslateOrDefault("GridObjectViewer.AllSmudge", "All smudges"));
    auto allO = addOverlay(Translations::TranslateOrDefault("GridObjectViewer.AllOverlay", "All overlays"));

    auto& all = g_groups.emplace_back();
    all.DisplayName = Translations::TranslateOrDefault("GridObjectViewer.All", "All objects");
    for (auto& o : g_groups[allI].IDs) all.IDs.push_back(o);
    for (auto& o : g_groups[allV].IDs) all.IDs.push_back(o);
    for (auto& o : g_groups[allA].IDs) all.IDs.push_back(o);
    for (auto& o : g_groups[allB].IDs) all.IDs.push_back(o);
    for (auto& o : g_groups[allT].IDs) all.IDs.push_back(o);
    for (auto& o : g_groups[allS].IDs) all.IDs.push_back(o);
    // overlay is too much
    //for (auto& o : g_groups[allO].IDs) all.IDs.push_back(o);

    while (SendMessage(m_hControlGroup, CB_DELETESTRING, 0, NULL) != CB_ERR);
    for (auto& g : g_groups)
    {
        std::unordered_set<FString> seen;
        size_t writeIndex = 0;
        for (size_t i = 0; i < g.IDs.size(); ++i)
        {
            if (seen.insert(g.IDs[i].ID).second)
            {
                g.IDs[writeIndex++] = std::move(g.IDs[i]);
            }
        }
        g.IDs.resize(writeIndex);
    }
    std::erase_if(g_groups, [](GroupInfo& g) { return g.IDs.empty(); });
    int i = 0;
    for (auto& g : g_groups)
    {
        SendMessage(m_hControlGroup, CB_INSERTSTRING, i, g.DisplayName);
        ++i;
    }
}

void GridObjectViewer::OnSelchange()
{
    int curSel = SendMessage(m_hControlGroup, CB_GETCURSEL, NULL, NULL);
    g_currentCurSel = curSel;
    UpdateImages();
}

void GridObjectViewer::OnEditchange()
{
    char buffer[512]{ 0 };
    GetWindowText(m_hControlSearch, buffer, 511);

    g_filteredImages.clear();    
    g_selectedIndex = -1;
    m_nScrollX = 0;
    m_nScrollY = 0;

    if (strcmp(buffer, "") == 0)
        g_filteredImages = g_images;
    else
    {
        LabelMatcher matcher(buffer);
        for (auto& g : g_images)
        {
            auto uiName = CViewObjectsExt::QueryUIName(g.ID);
            if (matcher.Match(g.ID) || matcher.Match(uiName))
            {
                g_filteredImages.push_back(g);
            }
        }
    }
    LayoutImages();
    InvalidateRect(GetView(), NULL, FALSE);
}

void GridObjectViewer::UpdateImages()
{
    Clear();

    auto loadImageBuilding = [this](ImageDataClassSafe* pd, const ppmfc::CString id, const CLoadingExt::ObjectType type)
    {
        if (!pd || !pd->pImageBuffer || pd->FullWidth <= 0 || pd->FullHeight <= 0)
            return;
        ImageInfo info;
        info.ID = id;
        info.Type = type;
        ComputeCroppedInfo(pd, info);

        g_images.push_back(info);

    };
    auto loadImage = [this](const FString& DataName, 
        const ppmfc::CString id, 
        const CLoadingExt::ObjectType type,
        int overlay = -1,
        int overlayData = -1
        )
    {
        auto pd = CLoadingExt::GetImageDataFromMap(DataName);
        if (!pd || !pd->pImageBuffer || pd->FullWidth <= 0 || pd->FullHeight <= 0)
            return;
        ImageInfo info;
        info.ID = id;
        info.Type = type;
        ComputeCroppedInfo(pd, info);
        info.pData = nullptr;
        info.DataName = DataName;
        info.Overlay = overlay;
        info.OverlayData = overlayData;

        g_images.push_back(info);

    };
    if (g_currentCurSel >= 0 && g_currentCurSel < g_groups.size())
    {
        auto& g = g_groups[g_currentCurSel];
        for (auto& id : g.IDs)
        {
            if (id.IsOverlay)
            {
                auto& datas = id.OverlayDatas;
                int index = STDHelpers::ParseToInt(id.ID);
                auto oid = Variables::RulesMap.GetValueAt("OverlayTypes", index);
                if (!CLoadingExt::IsOverlayLoaded(oid))
                    CLoadingExt::GetExtension()->LoadOverlay(oid, index);
                int nDisplayLimit = Variables::RulesMap.GetInteger(oid, "OverlayDisplayLimit", ExtConfigs::OverlayDataLimit);
                nDisplayLimit = CINI::FAData->GetInteger("OverlayDisplayLimit", id.ID, nDisplayLimit);
                if (nDisplayLimit > ExtConfigs::OverlayDataLimit)
                    nDisplayLimit = ExtConfigs::OverlayDataLimit;

                for (int i = 0; i < nDisplayLimit; ++i)
                {
                    if (!datas.empty() && !datas.contains(i))
                        continue;
                    auto imageName = CLoadingExt::GetOverlayName(index, i);
                    loadImage(imageName, id.ID, CLoadingExt::ObjectType::Unknown, index, i);
                }
            }
            else
            {
                TempValueHolder<bool> tmp1(ExtConfigs::InGameDisplay_Shadow, false);
                TempValueHolder<bool> tmp2(CLoadingExt::IsLoadingObjectView, true);

                auto type = CLoadingExt::GetExtension()->GetItemType(id.ID);

                if (!CLoadingExt::IsObjectPreviewLoaded(id.ID) && !CLoadingExt::IsObjectLoaded(id.ID))
                    CLoadingExt::GetExtension()->LoadObjects(id.ID);
                switch (type)
                {
                case CLoadingExt::ObjectType::Infantry:
                {
                    int facings = CLoadingExt::GetAvailableFacing(id.ID);
                    auto imageName = CLoadingExt::GetImageName(id.ID, 5);
                    loadImage(imageName, id.ID, type);
                    break;
                }
                case CLoadingExt::ObjectType::Vehicle:
                case CLoadingExt::ObjectType::Aircraft:
                {
                    int facings = CLoadingExt::GetAvailableFacing(id.ID);
                    auto imageName = CLoadingExt::GetImageName(id.ID, facings / 4);
                    loadImage(imageName, id.ID, type);
                    break;
                }
                case CLoadingExt::ObjectType::Terrain:
                case CLoadingExt::ObjectType::Smudge:
                {
                    auto imageName = CLoadingExt::GetImageName(id.ID, 0);
                    loadImage(imageName, id.ID, type);
                    break;
                }
                case CLoadingExt::ObjectType::Building:
                {
                    bool hasTur = Variables::RulesMap.GetBool(id.ID, "Turret")
                        || Variables::RulesMap.GetBool(id.ID, "TurretAnimIsVoxel");
                    int facings = hasTur ? (ExtConfigs::ExtFacings ? 32 : 8) : 1;
                    auto itr = CLoadingExt::AvailableFacings.find(id.ID);
                    if (itr != CLoadingExt::AvailableFacings.end())
                        facings = itr->second;
                    auto imageName = CLoadingExt::GetBuildingImageName(id.ID, facings / 8 * 5, 0);
                    auto& clips = CLoadingExt::GetBuildingClipImageDataFromMap(imageName);
                    auto pd = CLoadingExt::BindClippedImages(clips);
                    g_buildingImages.push_back(std::move(pd));
                    loadImageBuilding(g_buildingImages.back().get(), id.ID, type);
                    break;
                }
                case CLoadingExt::ObjectType::Unknown:
                default:
                    break;
                }
            }
        }
        OnEditchange();
    }
    else
    {
        InvalidateRect(GetView(), NULL, FALSE);
    }
}

LRESULT CALLBACK GridObjectViewer::HandleViewSubClassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

        if (!m_hMemDC || (client.right != GetDeviceCaps(m_hMemDC, HORZRES)))
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
        for (size_t i = 0; i < g_filteredImages.size() && i < g_imageRects.size(); ++i)
        {
            RECT rect = g_imageRects[i];
            rect.left -= m_nScrollX;
            rect.right -= m_nScrollX;
            rect.top -= m_nScrollY;
            rect.bottom -= m_nScrollY;
            if (PtInRect(&rect, pt))
            {
                g_selectedIndex = (int)i;
                OnSelChanged(g_selectedIndex);
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

    return CallWindowProc(g_oldViewProc, hwnd, msg, wParam, lParam);
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

    m_hControl = CreateDialogParam(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(ExtConfigs::VerticalLayout ? 337 : 338),
        hParent,
        ControlSubClassProc,
        reinterpret_cast<LPARAM>(this)
    );
    m_hView = CreateWindowEx(NULL, "ImageDataGallery", nullptr,
        WS_CHILD | WS_BORDER | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_CLIPCHILDREN,
        0, 0, 1, 1, hParent,
        NULL, static_cast<HINSTANCE>(FA2sp::hInstance), nullptr);

    if (m_hControl && m_hView)
    {
        LONG_PTR style = GetWindowLongPtr(m_hControl, GWL_STYLE);
        style &= ~(WS_BORDER | WS_DLGFRAME | WS_THICKFRAME);
        SetWindowLongPtr(m_hControl, GWL_STYLE, style);
        SetWindowPos(m_hControl, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

        CRect rect;
        ::GetClientRect(hParent, &rect);
        CRect controlRect;
        GetWindowRect(m_hControl, &controlRect);

        SetWindowLongPtr(m_hView, GWLP_USERDATA, (LONG_PTR)this);
        g_oldViewProc = (WNDPROC)SetWindowLongPtr(
            m_hView, GWLP_WNDPROC, (LONG_PTR)ViewSubClassProc);
    }   
}

void GridObjectViewer::OnSize() const
{
    RECT rect;
    ::GetClientRect(::GetParent(this->GetView()), &rect);
    CRect controlRect;
    GetWindowRect(this->GetControl(), &controlRect);

    if (ExtConfigs::VerticalLayout)
    {
        ::MoveWindow(m_hControl, 2, 29,
            rect.right - rect.left - 6, controlRect.Height(), FALSE);
        ::MoveWindow(this->GetView(), 2, controlRect.Height(),
            rect.right - rect.left - 6, rect.bottom - rect.top - 6 - controlRect.Height(), FALSE);
    }
    else
    {
        ::MoveWindow(m_hControl, 2, 29,
            rect.right - rect.left - 6, controlRect.Height(), FALSE);
        ::MoveWindow(this->GetView(), 2, controlRect.Height(),
            rect.right - rect.left - 6, rect.bottom - rect.top - 6 - controlRect.Height(), FALSE);
    }

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
    UpdateImages();
    ::ShowWindow(this->GetView(), SW_SHOW);
    ::ShowWindow(this->GetControl(), SW_SHOW);
}

void GridObjectViewer::HideWindow()
{
    ::ShowWindow(this->GetView(), SW_HIDE);
    ::ShowWindow(this->GetControl(), SW_HIDE);
}

bool GridObjectViewer::IsValid() const
{
    return this->GetView() != NULL;
}

bool GridObjectViewer::IsVisible() const
{
    return this->IsValid() && ::IsWindowVisible(this->m_hView);
}

void GridObjectViewer::Clear()
{
    m_nScrollX = m_nScrollY = 0;
    g_filteredImages.clear();
    g_images.clear();
    g_imageRects.clear();
    g_buildingImages.clear();
    g_selectedIndex = -1;
    SetWindowText(m_hControlCurrentSelect, "");
    InvalidateRect(m_hView, NULL, TRUE);
}

HWND GridObjectViewer::GetView() const
{
    return this->m_hView;
}

HWND GridObjectViewer::GetControl() const
{
    return this->m_hControl;
}

int GridObjectViewer::GetSelectedSel() const
{
    return g_currentCurSel;
}

int GridObjectViewer::GetSelectedIndex() const
{
    return g_selectedIndex;
}

GridObjectViewer::operator HWND() const
{
    return this->GetView();
}

bool GridObjectViewer::SelectLeft()
{
    if (g_imageRects.empty()) return false;
    if (g_selectedIndex < 0)
    {
        g_selectedIndex = 0;
        return true;
    }
    if (g_selectedIndex <= 0) return false;
    g_selectedIndex--;
    return true;
}

bool GridObjectViewer::SelectRight()
{
    if (g_imageRects.empty()) return false;
    if (g_selectedIndex >= (int)g_imageRects.size() - 1) return false;
    g_selectedIndex++;
    return true;
}

bool GridObjectViewer::SelectUp()
{
    if (g_imageRects.empty()) return false;
    if (g_selectedIndex < 0)
    {
        g_selectedIndex = 0;
        return true;
    }

    RECT current = g_imageRects[g_selectedIndex];
    int centerX = (current.left + current.right) / 2;  

    int currentRowStart = FindRowStart(g_selectedIndex);
    if (currentRowStart <= 0) return false; 

    int prevRowStart = FindRowStart(currentRowStart - 1);
    int prevRowEnd = currentRowStart - 1;

    return SelectClosestInRow(prevRowStart, prevRowEnd, centerX);
}

bool GridObjectViewer::SelectDown()
{
    if (g_imageRects.empty()) return false;
    if (g_selectedIndex < 0)
    {
        g_selectedIndex = 0;
        return true;
    }

    RECT current = g_imageRects[g_selectedIndex];
    int centerX = (current.left + current.right) / 2;

    int currentRowStart = FindRowStart(g_selectedIndex);
    int nextRowStart = currentRowStart + GetRowLength(currentRowStart);

    if (nextRowStart >= (int)g_imageRects.size()) return false;

    int nextRowEnd = std::min(nextRowStart + GetRowLength(nextRowStart) - 1,
        (int)g_imageRects.size() - 1);

    return SelectClosestInRow(nextRowStart, nextRowEnd, centerX);
}

int GridObjectViewer::FindRowStart(int index)
{
    if (index < 0 || index >= (int)g_imageRects.size()) return 0;

    for (int i = index; i > 0; --i)
    {
        if (g_imageRects[i].left < g_imageRects[i - 1].right - 5) 
            return i;
    }
    return 0;
}

int GridObjectViewer::GetRowLength(int startIndex)
{
    if (startIndex < 0 || startIndex >= (int)g_imageRects.size()) return 0;

    int count = 1;
    for (int i = startIndex + 1; i < (int)g_imageRects.size(); ++i)
    {
        if (g_imageRects[i].left < g_imageRects[i - 1].right - 5)
            break;
        count++;
    }
    return count;
}

bool GridObjectViewer::SelectClosestInRow(int rowStart, int rowEnd, int targetCenterX)
{
    if (rowStart > rowEnd || rowStart < 0) return false;

    int closestIndex = rowStart;
    int minDistance = INT_MAX;

    for (int i = rowStart; i <= rowEnd; ++i)
    {
        RECT r = g_imageRects[i];
        int center = (r.left + r.right) / 2;
        int distance = abs(center - targetCenterX);

        if (distance < minDistance)
        {
            minDistance = distance;
            closestIndex = i;
        }
    }

    if (closestIndex != g_selectedIndex)
    {
        g_selectedIndex = closestIndex;
        return true;
    }
    return false;
}

void GridObjectViewer::EnsureVisible(int index)
{
    if (index < 0 || index >= (int)g_imageRects.size()) return;

    RECT r = g_imageRects[index];
    RECT client{};
    GetClientRect(m_hView, &client);

    if (r.left < m_nScrollX)
        m_nScrollX = r.left;
    else if (r.right > m_nScrollX + client.right)
        m_nScrollX = r.right - client.right;

    if (r.top < m_nScrollY)
        m_nScrollY = r.top;
    else if (r.bottom > m_nScrollY + client.bottom)
        m_nScrollY = r.bottom - client.bottom;

    UpdateScrollBars();
    InvalidateRect(m_hView, NULL, TRUE);
}

void GridObjectViewer::OnSelChanged(int index)
{
    auto& data = g_filteredImages[index];
    auto& id = data.ID;

    if (data.Overlay >= 0)
    {
        CIsoView::CurrentCommand->Command = 1;
        CIsoView::CurrentCommand->Type = 6;
        CIsoView::CurrentCommand->Param = 33;
        CIsoView::CurrentCommand->Overlay = data.Overlay;
        CIsoView::CurrentCommand->OverlayData = data.OverlayData;
        CIsoView::CurrentCommand->Height = 0;

        auto oid = Variables::RulesMap.GetValueAt("OverlayTypes", data.Overlay);
        FString display;
        FString name = Variables::RulesMap.GetString(oid, "Name");
        if (name.IsEmpty() || !Translations::GetTranslationItem(name, display))
        {
            display = CViewObjectsExt::QueryUIName(id, true);
        }
        if (display != oid)
            display += " (" + oid + ")";
        FString format = "%s, ";
        format += Translations::TranslateOrDefault("GridObjectViewer.OverlayText", "Index: %d, OverlayData: %d");
        display.Format(format, display, data.Overlay, data.OverlayData);
        SetWindowText(m_hControlCurrentSelect, display);
    }
    else
    {
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

        FString display = CViewObjectsExt::QueryUIName(id);
        if (display != id)
            display += " (" + id + ")";
        SetWindowText(m_hControlCurrentSelect, display);
    }
}