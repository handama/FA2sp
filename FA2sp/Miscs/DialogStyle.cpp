#include "DialogStyle.h"
#include "../Helpers/STDHelpers.h"
#include <windowsx.h>
#include "CFinalSunDlg.h"
#include <psapi.h>

HBRUSH DarkTheme::g_hDarkBackgroundBrush = NULL;
HBRUSH DarkTheme::g_hDarkControlBrush = NULL;
HBRUSH DarkTheme::g_hDarkMenuBrush = NULL;
HBRUSH DarkTheme::g_hDarkInfoBkBrush = NULL;
HBRUSH DarkTheme::g_hLightTextBrush = NULL;
HBRUSH DarkTheme::g_hHighlightBrush = NULL;
HBRUSH DarkTheme::g_hDisabledBgBrush = NULL;
HBRUSH DarkTheme::g_hDisabledTextBrush = NULL;
HBRUSH DarkTheme::g_hBtnBgBrush = NULL;
HBRUSH DarkTheme::g_hBtnBgHoverBrush = NULL;
HBRUSH DarkTheme::g_hCheckFillBrush = NULL;
HBRUSH DarkTheme::g_hRadioFillBrush = NULL;
HPEN DarkTheme::g_hBackGroundPen = NULL;
HPEN DarkTheme::g_hBorderPen = NULL;
HPEN DarkTheme::g_hCheckBorderPen = NULL;
HPEN DarkTheme::g_hRadioBorderPen = NULL;
HPEN DarkTheme::g_hHoverBorderPen = NULL;
HPEN DarkTheme::g_hGroupBoxBorderPen = NULL;
HWND DarkTheme::g_hMenuOverlay = NULL;
HMENU DarkTheme::g_hMainMenu = NULL;
std::vector<TopMenuItemInfo> DarkTheme::g_menuItems;
std::map<HMENU, std::map<UINT, MenuItemInfo>> DarkTheme::g_menuItemData;
bool DarkTheme::b_isImportingINI = false;
bool DarkTheme::b_isSelectingGameFolder = false;

DEFINE_HOOK(40B740, CINIEditor_OnClickImportINI, 7)
{
    DarkTheme::b_isImportingINI = true;
    return 0;
}
DEFINE_HOOK(50E220, CFinalSunDlg_SelectMainExecutive, 7)
{
    DarkTheme::b_isSelectingGameFolder = true;
    return 0;
}

void DarkTheme::InitDarkThemeBrushes()
{
    if (!g_hDarkBackgroundBrush)
        g_hDarkBackgroundBrush = CreateSolidBrush(DarkColors::Background);
    if (!g_hDarkControlBrush)
        g_hDarkControlBrush = CreateSolidBrush(DarkColors::Control);
    if (!g_hDarkMenuBrush)
        g_hDarkMenuBrush = CreateSolidBrush(DarkColors::Menu);
    if (!g_hDarkInfoBkBrush)
        g_hDarkInfoBkBrush = CreateSolidBrush(DarkColors::InfoBk);
    if (!g_hLightTextBrush)
        g_hLightTextBrush = CreateSolidBrush(DarkColors::LightText);
    if (!g_hHighlightBrush)
        g_hHighlightBrush = CreateSolidBrush(DarkColors::Highlight);
    if (!g_hDisabledBgBrush)
        g_hDisabledBgBrush = CreateSolidBrush(DarkColors::DisabledBg);
    if (!g_hDisabledTextBrush)
        g_hDisabledTextBrush = CreateSolidBrush(DarkColors::DisabledText);

    if (!g_hBtnBgBrush)
        g_hBtnBgBrush = CreateSolidBrush(DarkColors::BtnBg);
    if (!g_hBtnBgHoverBrush)
        g_hBtnBgHoverBrush = CreateSolidBrush(DarkColors::BtnBgHover);
    if (!g_hCheckFillBrush)
        g_hCheckFillBrush = CreateSolidBrush(DarkColors::CheckFill);
    if (!g_hRadioFillBrush)
        g_hRadioFillBrush = CreateSolidBrush(DarkColors::RadioFill);

    if (!g_hBorderPen)
        g_hBorderPen = CreatePen(PS_SOLID, 1, DarkColors::BorderGray);
    if (!g_hBackGroundPen)
        g_hBackGroundPen = CreatePen(PS_SOLID, 1, DarkColors::Background);
    if (!g_hCheckBorderPen)
        g_hCheckBorderPen = CreatePen(PS_SOLID, 1, DarkColors::CheckBorder);
    if (!g_hRadioBorderPen)
        g_hRadioBorderPen = CreatePen(PS_SOLID, 1, DarkColors::RadioBorder);
    if (!g_hHoverBorderPen)
        g_hHoverBorderPen = CreatePen(PS_SOLID, 1, DarkColors::HoverBorder);
    if (!g_hGroupBoxBorderPen)
        g_hGroupBoxBorderPen = CreatePen(PS_SOLID, 1, DarkColors::GroupBoxBorder);
}

void DarkTheme::CleanupDarkThemeBrushes()
{
    if (g_hDarkBackgroundBrush)
        DeleteObject(g_hDarkBackgroundBrush);
    if (g_hDarkControlBrush)
        DeleteObject(g_hDarkControlBrush);
    if (g_hDarkMenuBrush)
        DeleteObject(g_hDarkMenuBrush);
    if (g_hDarkInfoBkBrush)
        DeleteObject(g_hDarkInfoBkBrush);
    if (g_hLightTextBrush)
        DeleteObject(g_hLightTextBrush);
    if (g_hHighlightBrush)
        DeleteObject(g_hHighlightBrush);
    if (g_hDisabledBgBrush)
        DeleteObject(g_hDisabledBgBrush);
    if (g_hDisabledTextBrush)
        DeleteObject(g_hDisabledTextBrush);

    if (g_hBtnBgBrush)
        DeleteObject(g_hBtnBgBrush);
    if (g_hBtnBgHoverBrush)
        DeleteObject(g_hBtnBgHoverBrush);
    if (g_hCheckFillBrush)
        DeleteObject(g_hCheckFillBrush);
    if (g_hRadioFillBrush)
        DeleteObject(g_hRadioFillBrush);

    if (g_hBorderPen)
        DeleteObject(g_hBorderPen);
    if (g_hBackGroundPen)
        DeleteObject(g_hBackGroundPen);
    if (g_hCheckBorderPen)
        DeleteObject(g_hCheckBorderPen);
    if (g_hRadioBorderPen)
        DeleteObject(g_hRadioBorderPen);
    if (g_hHoverBorderPen)
        DeleteObject(g_hHoverBorderPen);
    if (g_hGroupBoxBorderPen)
        DeleteObject(g_hGroupBoxBorderPen);

    g_hDarkBackgroundBrush = g_hDarkControlBrush = g_hDarkMenuBrush
        = g_hDarkInfoBkBrush = g_hLightTextBrush = g_hHighlightBrush
        = g_hDisabledBgBrush = g_hDisabledTextBrush = g_hBtnBgBrush 
        = g_hBtnBgHoverBrush = g_hCheckFillBrush = g_hRadioFillBrush = NULL;
    g_hBorderPen = g_hCheckBorderPen = g_hRadioBorderPen 
        = g_hHoverBorderPen = g_hGroupBoxBorderPen = g_hBackGroundPen = NULL;
}

int WINAPI DarkTheme::MyFillRect(HDC hDC, const RECT* lprc, HBRUSH hbr)
{
    return ::FillRect(hDC, lprc, g_hDarkBackgroundBrush);
}

BOOL WINAPI DarkTheme::MyPatBlt(HDC hdc, int x, int y, int w, int h, DWORD rop)
{
    if (rop == BLACKNESS || rop == WHITENESS || rop == PATCOPY)
    {
        RECT rc = { x, y, x + w, y + h };
        FillRect(hdc, &rc, g_hDarkBackgroundBrush);
        return TRUE;
    }
    return ::PatBlt(hdc, x, y, w, h, rop);
}

BOOL WINAPI DarkTheme::MyTextOutA(HDC hdc, int x, int y, LPCSTR lpString, int c)
{
    ::SetTextColor(hdc, DarkColors::LightText);
    ::SetBkColor(hdc, DarkColors::Background);
    return ::TextOutA(hdc, x, y, lpString, c);
}

COLORREF WINAPI DarkTheme::MySetBkColor(HDC hdc, COLORREF color)
{
    return  ::SetBkColor(hdc, DarkColors::DisabledBg);
}

BOOL WINAPI DarkTheme::MyExtTextOutA(HDC hdc, int x, int y, UINT options,
    const RECT* lprc, LPCSTR lpString,
    UINT c, const INT* lpDx)
{
    ::SetTextColor(hdc, DarkColors::LightText);
    ::SetBkColor(hdc, DarkColors::Background);

    return ::ExtTextOutA(hdc, x, y, options, lprc, lpString, c, lpDx);
}

COLORREF WINAPI DarkTheme::MySetPixel(HDC hdc, int x, int y, COLORREF color)
{
    return ::SetPixel(hdc, x, y, DarkColors::Background);
}

BOOL __fastcall DarkTheme::MyOnEraseBkgnd(CFrameWnd* pThis, void* /*edx*/, CDC* pDC)
{
    CRect rect;
    pThis->GetClientRect(&rect);
    pDC->FillSolidRect(&rect, DarkColors::DarkGray);
    return TRUE;
}

DWORD WINAPI DarkTheme::MyGetSysColor(int nIndex)
{
    switch (nIndex)
    {
    case COLOR_MENU:
    case COLOR_MENUBAR:
        return DarkColors::Menu;
    case COLOR_MENUTEXT:
    case COLOR_WINDOWTEXT:
    case COLOR_BTNTEXT:
        return DarkColors::LightText;
    case COLOR_WINDOW:
    case COLOR_BTNFACE:
    case COLOR_SCROLLBAR:
        return DarkColors::Background;
    case COLOR_HIGHLIGHT:
        return DarkColors::MediumGray;
    case COLOR_HIGHLIGHTTEXT:
        return DarkColors::White;
    default:
        return ::GetSysColor(nIndex);
    }
}

HBRUSH WINAPI DarkTheme::MyGetSysColorBrush(int nIndex)
{
    switch (nIndex)
    {
    case COLOR_WINDOW:
    case COLOR_WINDOWFRAME:
    case COLOR_BACKGROUND:
    case COLOR_APPWORKSPACE:
        return g_hDarkBackgroundBrush;

    case COLOR_BTNFACE:
        return g_hDarkControlBrush;

    case COLOR_MENU:
    case COLOR_MENUBAR:
        return g_hDarkMenuBrush;

    case COLOR_INFOBK:
        return g_hDarkInfoBkBrush;

    case COLOR_WINDOWTEXT:
    case COLOR_MENUTEXT:
    case COLOR_BTNTEXT:
    case COLOR_CAPTIONTEXT:
    case COLOR_INFOTEXT:
    case COLOR_BTNHIGHLIGHT:
        return g_hLightTextBrush;

    default:
        return ::GetSysColorBrush(nIndex);
    }
}

HGDIOBJ WINAPI DarkTheme::MyGetStockObject(int fnObject)
{
    switch (fnObject)
    {
    case WHITE_BRUSH:
    case LTGRAY_BRUSH:
    case GRAY_BRUSH:
        return g_hDarkBackgroundBrush;
    case DKGRAY_BRUSH:
        return g_hDarkControlBrush;
    default:
        return ::GetStockObject(fnObject);
    }
}

HPEN WINAPI DarkTheme::MyCreatePen(int iStyle, int cWidth, COLORREF crColor)
{
    if (crColor == GetSysColor(COLOR_3DDKSHADOW) ||
        crColor == GetSysColor(COLOR_3DLIGHT) ||
        crColor == GetSysColor(COLOR_3DFACE))
    {
        crColor = DarkColors::Background;
    }

    return ::CreatePen(iStyle, cWidth, crColor);
}

COLORREF WINAPI DarkTheme::MySetTextColor(HDC hdc, COLORREF crColor)
{
    if (crColor == GetSysColor(COLOR_WINDOWTEXT) ||
        crColor == GetSysColor(COLOR_BTNTEXT))
    {
        crColor = DarkColors::LightText;
    }
    else if (crColor == GetSysColor(COLOR_HIGHLIGHTTEXT))
    {
        crColor = DarkColors::White;
    }

    return ::SetTextColor(hdc, crColor);
}

LRESULT CALLBACK DarkTheme::TabCtrlSubclassProc(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    static HFONT hDefaultFont = NULL;
    switch (uMsg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rc;
        GetClientRect(hWnd, &rc);

        FillRect(hdc, &rc, g_hDarkBackgroundBrush);

        if (!hDefaultFont)
        {
            hDefaultFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }
        HFONT hFont = hDefaultFont;
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        int itemCount = TabCtrl_GetItemCount(hWnd);
        int selItem = TabCtrl_GetCurSel(hWnd);

        RECT tabArea = { 0 };
        if (itemCount > 0)
        {
            RECT firstTabRect;
            TabCtrl_GetItemRect(hWnd, 0, &firstTabRect);
            tabArea.top = 0;
            tabArea.bottom = firstTabRect.bottom;
            tabArea.left = rc.left;
            tabArea.right = rc.right;
        }

        RECT borderRect = rc;
        borderRect.top = tabArea.bottom;
        if (itemCount > 0)
        {
            HPEN hOldPen = (HPEN)SelectObject(hdc, g_hBorderPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, borderRect.left, borderRect.top, borderRect.right, borderRect.bottom);
            SelectObject(hdc, hOldPen);
            SelectObject(hdc, hOldBrush);
        }

        for (int i = 0; i < itemCount; i++)
        {
            RECT itemRect;
            TabCtrl_GetItemRect(hWnd, i, &itemRect);

            TCHAR text[256];
            TCITEM tci = { 0 };
            tci.mask = TCIF_TEXT;
            tci.pszText = text;
            tci.cchTextMax = 256;
            TabCtrl_GetItem(hWnd, i, &tci);

            COLORREF bgColor, textColor;
            if (i == selItem)
            {
                bgColor = DarkColors::LightGray;
                textColor = DarkColors::White;
            }
            else
            {
                bgColor = DarkColors::DisabledBg;
                textColor = DarkColors::LightText;
            }

            HBRUSH hItemBrush = CreateSolidBrush(bgColor);
            FillRect(hdc, &itemRect, hItemBrush);
            DeleteObject(hItemBrush);

            HPEN hOldPen = (HPEN)SelectObject(hdc, g_hBorderPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, itemRect.left, itemRect.top, itemRect.right, itemRect.bottom);
            SelectObject(hdc, hOldPen);
            SelectObject(hdc, hOldBrush);

            SetTextColor(hdc, textColor);
            SetBkMode(hdc, TRANSPARENT);

            DrawText(hdc, text, -1, &itemRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        SelectObject(hdc, hOldFont);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
    {
        return TRUE;
    }
    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, TabCtrlSubclassProc, uIdSubclass);
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK DarkTheme::HeaderSubclassProc(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    static HFONT hDefaultFont = NULL;
    switch (uMsg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rc;
        GetClientRect(hWnd, &rc);

        FillRect(hdc, &rc, g_hDarkBackgroundBrush);

        int colCount = Header_GetItemCount(hWnd);
        if (!hDefaultFont)
        {
            hDefaultFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }
        HFONT hFont = hDefaultFont;
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        for (int i = 0; i < colCount; i++)
        {
            RECT colRect;
            Header_GetItemRect(hWnd, i, &colRect);

            HDITEM hdi = { 0 };
            TCHAR text[256];
            hdi.mask = HDI_TEXT | HDI_FORMAT;
            hdi.pszText = text;
            hdi.cchTextMax = 256;

            if (Header_GetItem(hWnd, i, &hdi))
            {
                FillRect(hdc, &colRect, g_hHighlightBrush);

                HPEN hOldPen = (HPEN)SelectObject(hdc, g_hBorderPen);
                MoveToEx(hdc, colRect.right - 1, colRect.top, NULL);
                LineTo(hdc, colRect.right - 1, colRect.bottom);
                SelectObject(hdc, hOldPen);

                SetTextColor(hdc, DarkColors::LightText);
                SetBkMode(hdc, TRANSPARENT);

                RECT textRect = colRect;
                textRect.left += 5;
                textRect.right -= 5;

                UINT format = DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
                if (hdi.fmt & HDF_CENTER)
                    format |= DT_CENTER;
                else if (hdi.fmt & HDF_RIGHT)
                    format |= DT_RIGHT;
                else
                    format |= DT_LEFT;

                DrawText(hdc, text, -1, &textRect, format);
            }
        }
        SelectObject(hdc, hOldFont);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
    {
        return TRUE;
    }
    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, HeaderSubclassProc, uIdSubclass);
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

BOOL DarkTheme::SubclassListViewHeader(HWND hListView)
{
    HWND hHeader = FindWindowEx(hListView, NULL, WC_HEADER, NULL);
    if (!hHeader)
        return FALSE;

    SetWindowSubclass(hHeader, HeaderSubclassProc, 0, 0);
    return TRUE;
}

LRESULT DarkTheme::HandleCustomDraw(LPARAM lParam)
{
    LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);

    wchar_t className[32] = { 0 };
    GetClassNameW(pNMCD->hdr.hwndFrom, className, 32);
    bool isTreeView = (wcscmp(className, L"SysTreeView32") == 0);

    switch (pNMCD->dwDrawStage)
    {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
    {
        HDC hdc = pNMCD->hdc;
        RECT rc = pNMCD->rc;
        BOOL isSelected = (pNMCD->uItemState & CDIS_SELECTED) != 0;

        if (isTreeView)
        {
            LPNMTVCUSTOMDRAW pNMTVCD = reinterpret_cast<LPNMTVCUSTOMDRAW>(pNMCD);
            pNMTVCD->clrTextBk = isSelected ? DarkColors::MediumGray : DarkColors::Background;
            pNMTVCD->clrText = isSelected ? DarkColors::White : DarkColors::LightText;
            return CDRF_DODEFAULT;
        }

        break;
    }
    }

    return CDRF_DODEFAULT;
}

void DarkTheme::SetDarkTheme(HWND hWndParent)
{
    if (!ExtConfigs::EnableDarkMode)
        return;

    HWND hWndChild = NULL;

    BOOL darkMode19 = TRUE;
    BOOL darkMode20 = TRUE;
    DwmSetWindowAttribute(hWndParent, 19, &darkMode19, sizeof(darkMode19));
    DwmSetWindowAttribute(hWndParent, 20, &darkMode20, sizeof(darkMode20));

    while ((hWndChild = FindWindowEx(hWndParent, hWndChild, NULL, NULL)) != NULL)
    {
        DWORD style = GetWindowLongPtr(hWndChild, GWL_STYLE) & 0xF;
        wchar_t className[32];
        GetClassNameW(hWndChild, className, _countof(className));
        if (_wcsicmp(className, WC_COMBOBOXW) != 0)
            SetWindowTheme(hWndChild, L"DarkMode_Explorer", NULL);

        if (_wcsicmp(className, L"SysListView32") == 0)
        {
            ::SendMessage(hWndChild, LVM_SETTEXTBKCOLOR, 0, DarkColors::Background);
            ::SendMessage(hWndChild, LVM_SETTEXTCOLOR, 0, DarkColors::TextColor);
        }

        SetDarkTheme(hWndChild);
    }
}

void DarkTheme::EnableOwnerDrawMenu(HMENU hMenu, bool isTopLevel)
{
    if (!ExtConfigs::EnableDarkMode)
        return;

    int count = GetMenuItemCount(hMenu);
    for (int i = 0; i < count; ++i)
    {
        MENUITEMINFOW mii = { sizeof(mii) };
        mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_STRING | MIIM_SUBMENU;

        wchar_t buf[256] = { 0 };
        mii.dwTypeData = buf;
        mii.cch = 256;

        if (GetMenuItemInfoW(hMenu, i, TRUE, &mii))
        {
            if (mii.fType & MFT_SEPARATOR)
                continue;

            if (!isTopLevel)
            {
                MenuItemInfo info;
                info.text = buf;
                info.bChecked = (mii.fState & MFS_CHECKED) != 0;
                info.bRadioCheck = (mii.fType & MFT_RADIOCHECK) != 0;
                info.uID = mii.wID;

                DarkTheme::g_menuItemData[hMenu][mii.wID] = info;

                mii.fMask = MIIM_FTYPE | MIIM_DATA;
                mii.fType = MFT_OWNERDRAW;
                mii.dwItemData = (ULONG_PTR)&DarkTheme::g_menuItemData[hMenu][mii.wID];

                SetMenuItemInfoW(hMenu, i, TRUE, &mii);
            }

            if (mii.hSubMenu)
            {
                EnableOwnerDrawMenu(mii.hSubMenu, false);
            }
        }
    }
}

LRESULT DarkTheme::HandleMenuMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITMENUPOPUP:
    {
        HMENU hMenu = (HMENU)wParam;
        MENUINFO mi = { sizeof(MENUINFO) };
        mi.fMask = MIM_BACKGROUND;
        mi.hbrBack = g_hDarkMenuBrush;
        SetMenuInfo(hMenu, &mi);
        break;
    }
    case WM_MEASUREITEM:
    {
        LPMEASUREITEMSTRUCT pmis = (LPMEASUREITEMSTRUCT)lParam;
        if (pmis->CtlType == ODT_MENU)
        {
            MenuItemInfo* pInfo = (MenuItemInfo*)pmis->itemData;
            if (!pInfo)
            {
                pmis->itemHeight = 24;
                pmis->itemWidth = 100;
                return TRUE;
            }

            HDC hdc = GetDC(hWnd);
            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

            std::wstring leftText = pInfo->text;
            std::wstring rightText;
            size_t tabPos = pInfo->text.find(L'\t');
            if (tabPos != std::wstring::npos)
            {
                leftText = pInfo->text.substr(0, tabPos);
                rightText = pInfo->text.substr(tabPos + 1);
            }

            SIZE sizeLeft{}, sizeRight{};
            GetTextExtentPoint32W(hdc, leftText.c_str(), (int)leftText.length(), &sizeLeft);
            if (!rightText.empty())
                GetTextExtentPoint32W(hdc, rightText.c_str(), (int)rightText.length(), &sizeRight);

            pmis->itemHeight = std::max((long)24, sizeLeft.cy + 8);
            pmis->itemWidth = sizeLeft.cx + sizeRight.cx + 60;

            SelectObject(hdc, hOldFont);
            ReleaseDC(hWnd, hdc);

            return TRUE;
        }
        break;
    }

    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
        if (pdis->CtlType == ODT_MENU)
        {
            MenuItemInfo* pInfo = (MenuItemInfo*)pdis->itemData;
            if (!pInfo)
                return FALSE;

            HDC hdc = pdis->hDC;
            RECT rc = pdis->rcItem;

            COLORREF textColor;
            HBRUSH hBgBrush;

            if (pdis->itemState & ODS_SELECTED)
            {
                hBgBrush = g_hDarkBackgroundBrush;
                textColor = DarkColors::White;
            }
            else if (pdis->itemState & ODS_GRAYED)
            {
                hBgBrush = g_hDarkMenuBrush;
                textColor = DarkColors::DisabledText;
            }
            else
            {
                hBgBrush = g_hDarkMenuBrush;
                textColor = DarkColors::LightText;
            }

            FillRect(hdc, &rc, hBgBrush);

            SetTextColor(hdc, textColor);
            SetBkMode(hdc, TRANSPARENT);

            if (pInfo->bChecked)
            {
                RECT checkRc = rc;
                checkRc.right = checkRc.left + 20;
                checkRc.top += (rc.bottom - rc.top - 16) / 2;
                checkRc.bottom = checkRc.top + 16;
                HBITMAP hBmp = (HBITMAP)LoadImage(static_cast<HINSTANCE>(FA2sp::hInstance), 
                    MAKEINTRESOURCE(pInfo->bRadioCheck ? 1029 : 1028),
                    IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
                if (hBmp)
                {
                    HDC hMemDC = CreateCompatibleDC(hdc);
                    HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hBmp);

                    BITMAP bm;
                    GetObject(hBmp, sizeof(bm), &bm);

                    int x = rc.left + 3;
                    int y = rc.top + (rc.bottom - rc.top - bm.bmHeight) / 2;

                    TransparentBlt(hdc, x, y, bm.bmWidth, bm.bmHeight,
                        hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, DarkColors::Background);

                    SelectObject(hMemDC, hOldBmp);
                    DeleteDC(hMemDC);
                }
            }

            std::wstring leftText = pInfo->text;
            std::wstring rightText;
            size_t tabPos = pInfo->text.find(L'\t');
            if (tabPos != std::wstring::npos)
            {
                leftText = pInfo->text.substr(0, tabPos);
                rightText = pInfo->text.substr(tabPos + 1);
            }

            RECT textRc = rc;
            textRc.left += 24;

            DrawTextW(hdc, leftText.c_str(), -1, &textRc,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            if (!rightText.empty())
            {
                RECT accelRc = rc;
                accelRc.right -= 10;
                DrawTextW(hdc, rightText.c_str(), -1, &accelRc,
                    DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
            }
            return TRUE;
        }
        break;
    }
    }
    return 0;
}

void DarkTheme::UpdateHighlightStates()
{
    if (!g_hMainMenu) return;
    int count = GetMenuItemCount(g_hMainMenu);
    for (int i = 0; i < count && i < (int)g_menuItems.size(); i++)
    {
        MENUITEMINFO mii = { sizeof(MENUITEMINFO) };
        mii.fMask = MIIM_STATE;
        if (GetMenuItemInfo(g_hMainMenu, i, TRUE, &mii))
        {
            g_menuItems[i].isHighlighted = (mii.fState & MFS_HILITE) != 0;
            g_menuItems[i].isDisabled = (mii.fState & MFS_DISABLED) != 0;
        }
    }
}

void DarkTheme::InitMenuItems(HWND hOverlay)
{
    g_menuItems.clear();
    if (!g_hMainMenu) return;

    int count = GetMenuItemCount(g_hMainMenu);
    HWND hParent = GetParent(hOverlay);

    for (int i = 0; i < count; i++)
    {
        RECT rcItem{};
        if (GetMenuItemRect(hParent, g_hMainMenu, i, &rcItem))
        {
            MapWindowPoints(HWND_DESKTOP, hOverlay, (POINT*)&rcItem, 2);

            WCHAR text[256] = L"";
            MENUITEMINFOW mii = { sizeof(MENUITEMINFOW) };
            mii.fMask = MIIM_STRING | MIIM_STATE;
            mii.dwTypeData = text;
            mii.cch = 256;

            if (GetMenuItemInfoW(g_hMainMenu, i, TRUE, &mii))
            {
                TopMenuItemInfo item;
                item.text = text;
                item.rect = rcItem;
                item.isHighlighted = (mii.fState & MFS_HILITE) != 0;
                item.isDisabled = (mii.fState & MFS_DISABLED) != 0;

                g_menuItems.push_back(item);
            }
        }
    }
}

void DarkTheme::DrawMenuItems(HDC hdc, RECT rc)
{
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    for (const auto& item : g_menuItems)
    {
        HBRUSH hBgBrush;
        if (item.isHighlighted)
            hBgBrush = DarkTheme::g_hHighlightBrush;
        else
            hBgBrush = DarkTheme::g_hDarkBackgroundBrush;

        FillRect(hdc, &item.rect, hBgBrush);

        SetBkMode(hdc, TRANSPARENT);
        if (item.isDisabled)
            SetTextColor(hdc, DarkColors::DisabledText);
        else
            SetTextColor(hdc, DarkColors::LightText);

        RECT textRc = item.rect;

        DrawTextW(hdc, item.text.c_str(), -1, &textRc,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SelectObject(hdc, hOldFont);
}

LRESULT CALLBACK DarkTheme::MenuOverlayProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        UpdateHighlightStates();

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rc;
        GetClientRect(hWnd, &rc);

        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

        FillRect(hdcMem, &rc, g_hDarkBackgroundBrush);
        RECT rcBottom = rc;
        rcBottom.top = rcBottom.bottom - 2;
        FillRect(hdcMem, &rcBottom, g_hDarkInfoBkBrush);

        DrawMenuItems(hdcMem, rc);

        BitBlt(hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);

        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_NCHITTEST:
        return HTTRANSPARENT;
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}

void GetMenuBarRect(HWND hWnd, RECT* pRect)
{
    MENUBARINFO mbi = { sizeof(mbi) };
    if (GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi))
    {
        *pRect = mbi.rcBar;
        pRect->bottom += 3;
    }
    else
    {
        SetRectEmpty(pRect);
    }
}

HWND DarkTheme::CreateMenuOverlay(HWND hParent)
{
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = MenuOverlayProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszClassName = L"MenuOverlay";

    RegisterClassExW(&wc);

    RECT rc;
    GetMenuBarRect(hParent, &rc);

    HWND hOverlay = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        L"MenuOverlay", NULL, WS_POPUP,
        rc.left, rc.top,
        rc.right - rc.left, rc.bottom - rc.top,
        hParent, NULL, GetModuleHandle(NULL), NULL);

    SetLayeredWindowAttributes(hOverlay, 0, 255, LWA_ALPHA);
    return hOverlay;
}

void DarkTheme::InitializeMenuOverlay(HWND hWnd)
{
    g_hMainMenu = GetMenu(hWnd);
    if (!g_hMainMenu) return;

    if (g_hMenuOverlay) DestroyWindow(g_hMenuOverlay);
    g_hMenuOverlay = CreateMenuOverlay(hWnd);

    if (g_hMenuOverlay)
    {
        InitMenuItems(g_hMenuOverlay);
        ShowWindow(g_hMenuOverlay, SW_SHOW);
        UpdateWindow(g_hMenuOverlay);
    }
}

void DarkTheme::CleanupMenuOverlay()
{
    if (g_hMenuOverlay)
    {
        DestroyWindow(g_hMenuOverlay);
        g_hMenuOverlay = NULL;
    }
}

void DarkTheme::UpdateMenuOverlayPosition(HWND hWnd)
{
    if (!g_hMenuOverlay) return;

    RECT rc;
    GetMenuBarRect(hWnd, &rc);
    SetWindowPos(g_hMenuOverlay, NULL,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        SWP_NOZORDER | SWP_NOACTIVATE);

    InvalidateRect(g_hMenuOverlay, NULL, TRUE);
}

void DarkTheme::DrawComboBoxArrow(HDC hdc, RECT rc, bool enabled)
{
    int arrowWidth = GetSystemMetrics(SM_CXVSCROLL);
    RECT arrowRc = rc;
    arrowRc.left = arrowRc.right - arrowWidth;

    FillRect(hdc, &arrowRc, enabled ? g_hHighlightBrush : g_hDarkMenuBrush);

    HPEN hPen = CreatePen(PS_SOLID, 1, enabled ? DarkColors::RadioFill : DarkColors::DisabledText);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    int centerX = arrowRc.left + (arrowRc.right - arrowRc.left) / 2;
    int centerY = arrowRc.top + (arrowRc.bottom - arrowRc.top) / 2;

    int arrowSize = 4;
    for (int i = 0; i < arrowSize; i++)
    {
        MoveToEx(hdc, centerX - i, centerY + arrowSize / 2 - i, NULL);
        LineTo(hdc, centerX + i + 1, centerY + arrowSize / 2 - i);
    }

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

LRESULT CALLBACK DarkTheme::ComboBoxSubclassProc(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        LONG style = GetWindowLong(hWnd, GWL_STYLE);
        BOOL bDropdownList = (style & CBS_DROPDOWNLIST) == CBS_DROPDOWNLIST;

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rc;
        GetClientRect(hWnd, &rc);

        if (bDropdownList)
        {
            int idx = (int)SendMessage(hWnd, CB_GETCURSEL, 0, 0);
            if (idx >= 0)
            {
                char text[512]{ 0 };
                SendMessage(hWnd, CB_GETLBTEXT, idx, (LPARAM)text);

                RECT rcText = rc;
                rcText.right -= GetSystemMetrics(SM_CXVSCROLL);
                rcText.left += 5;

                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, DarkColors::LightText);

                HFONT hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
                HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

                DrawText(hdc, text, -1, &rcText, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);

                SelectObject(hdc, hOldFont);
            }
        }

        HPEN hOldPen = (HPEN)SelectObject(hdc, g_hBorderPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

        DrawComboBoxArrow(hdc, rc, IsWindowEnabled(hWnd));

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);

        EndPaint(hWnd, &ps);

        return TRUE;
    }
    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);

        FillRect(hdc, &rc, g_hDisabledBgBrush);

        return TRUE;
    }
    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;

        SetTextColor(hdc, IsWindowEnabled(hWnd) ? DarkColors::LightText : DarkColors::DisabledText);
        SetBkMode(hdc, TRANSPARENT);

        return (LRESULT)g_hDarkControlBrush;
    }
    case WM_CTLCOLORLISTBOX:
    {
        HDC hdc = (HDC)wParam;
        HWND hComboLBox = (HWND)lParam;
        SetTextColor(hdc, IsWindowEnabled(hWnd) ? DarkColors::LightText : DarkColors::DisabledText);
        SetBkMode(hdc, TRANSPARENT);
        if (!GetPropW(hComboLBox, L"DarkThemeApplied"))
        {
            SetWindowTheme(hComboLBox, L"DarkMode_Explorer", NULL);
            SetPropW(hComboLBox, L"DarkThemeApplied", (HANDLE)1);
        }
        return (LRESULT)g_hDarkControlBrush;
    }

    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, ComboBoxSubclassProc, uIdSubclass);
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK DarkTheme::EditSubclassProc(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_NCPAINT:
    {
        DefWindowProc(hWnd, uMsg, wParam, lParam);
        HDC hdc = GetWindowDC(hWnd);

        RECT rcWindow;
        GetWindowRect(hWnd, &rcWindow);
        OffsetRect(&rcWindow, -rcWindow.left, -rcWindow.top);

        int nCover = 3;

        HPEN hOldPen = (HPEN)SelectObject(hdc, g_hBackGroundPen);

        for (int i = 0; i < nCover; ++i)
            MoveToEx(hdc, rcWindow.left, rcWindow.top + i, NULL), LineTo(hdc, rcWindow.right, rcWindow.top + i);
        for (int i = 0; i < nCover; ++i)
            MoveToEx(hdc, rcWindow.left + i, rcWindow.top, NULL), LineTo(hdc, rcWindow.left + i, rcWindow.bottom);
        for (int i = 0; i < nCover; ++i)
            MoveToEx(hdc, rcWindow.left, rcWindow.bottom - 1 - i, NULL), LineTo(hdc, rcWindow.right, rcWindow.bottom - 1 - i);
        for (int i = 0; i < nCover; ++i)
            MoveToEx(hdc, rcWindow.right - 1 - i, rcWindow.top, NULL), LineTo(hdc, rcWindow.right - 1 - i, rcWindow.bottom);

        SelectObject(hdc, g_hBorderPen);

        MoveToEx(hdc, rcWindow.left, rcWindow.top, NULL);
        LineTo(hdc, rcWindow.right, rcWindow.top); 
        MoveToEx(hdc, rcWindow.left, rcWindow.top, NULL);
        LineTo(hdc, rcWindow.left, rcWindow.bottom); 
        MoveToEx(hdc, rcWindow.left, rcWindow.bottom - 1, NULL);
        LineTo(hdc, rcWindow.right, rcWindow.bottom - 1); 
        MoveToEx(hdc, rcWindow.right - 1, rcWindow.top, NULL);
        LineTo(hdc, rcWindow.right - 1, rcWindow.bottom);

        ReleaseDC(hWnd, hdc);

        return 0;
    }
    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
    
        FillRect(hdc, &rc, g_hDarkBackgroundBrush);
    
        return TRUE;
    }
    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;
    
        SetTextColor(hdc, DarkColors::LightText);
        SetBkMode(hdc, TRANSPARENT);
    
        return (LRESULT)g_hDisabledBgBrush;
    }
    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, EditSubclassProc, uIdSubclass);
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void DarkTheme::DrawCheckMark(HDC hdc, RECT rc)
{
    int h = rc.bottom - rc.top;
    int w = rc.right - rc.left;

    RECT fillRc = rc;

    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, g_hCheckFillBrush);
    HPEN hOldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));

    RoundRect(hdc, fillRc.left, fillRc.top, fillRc.right + 1, fillRc.bottom + 1, 4, 4);

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);

    POINT p1 = { rc.left + w / 6, (rc.top + rc.bottom) / 2 };
    POINT p2 = { rc.left + w / 2 - 1, rc.bottom - h / 5 };
    POINT p3 = { rc.right - w / 6, rc.top + h / 6 };

    HPEN hPen = CreatePen(PS_SOLID, std::max(1, h / 8), DarkColors::RadioFill);
    HPEN hOld = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush2 = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    MoveToEx(hdc, p1.x, p1.y, NULL);
    LineTo(hdc, p2.x, p2.y);
    LineTo(hdc, p3.x, p3.y);

    SelectObject(hdc, hOldBrush2);
    SelectObject(hdc, hOld);
    DeleteObject(hPen);
}

void DarkTheme::SubclassDarkButton(HWND hwndButton)
{
    if (!hwndButton) return;

    DarkBtnState* state = new DarkBtnState{};
    UINT_PTR id = 1;
    SetLastError(0);
    if (!SetWindowSubclass(hwndButton, DarkButtonSubclassProc, id, (DWORD_PTR)state))
    {
        delete state;
    }
}

void DarkTheme::SubclassDarkGroupBox(HWND hwndButton)
{
    if (!hwndButton) return;

    SetWindowSubclass(hwndButton, DarkGroupBoxclassProc, 0, 0);
}

void DarkTheme::UnsubclassDarkButton(HWND hwndButton)
{
    if (!hwndButton) return;
    RemoveWindowSubclass(hwndButton, DarkButtonSubclassProc, 1);
}

void DarkTheme::SubclassAllAutoButtons(HWND hParent)
{
    if (!ExtConfigs::EnableDarkMode)
        return;

    EnumChildWindows(hParent,
        [](HWND hwnd, LPARAM lParam)->BOOL {
        wchar_t cls[64] = {};
        GetClassNameW(hwnd, cls, _countof(cls));
        if (wcscmp(cls, L"Button") == 0)
        {
            LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
            int type = style & 0xF;

            switch (type)
            {
            case BS_CHECKBOX:
            case BS_AUTOCHECKBOX:
            case BS_3STATE:
            case BS_AUTO3STATE:
            case BS_RADIOBUTTON:
            case BS_AUTORADIOBUTTON:
                SubclassDarkButton(hwnd);
                break;
            case BS_GROUPBOX:
                SubclassDarkGroupBox(hwnd);
                break;
            default:
                break;
            }
        }
        return TRUE;
    }, 0);
}

LRESULT CALLBACK DarkTheme::DarkButtonSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    DarkBtnState* state = reinterpret_cast<DarkBtnState*>(dwRefData);

    switch (uMsg)
    {
    case WM_NCDESTROY:
    {
        RemoveWindowSubclass(hwnd, DarkButtonSubclassProc, uIdSubclass);
        if (state) { delete state; }
        return DefSubclassProc(hwnd, uMsg, wParam, lParam);
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_MOUSEMOVE:
    {
        if (state && !state->tracking)
        {
            TRACKMOUSEEVENT tme = { sizeof(tme) };
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            state->tracking = true;
        }

        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT rc; GetClientRect(hwnd, &rc);
        bool hover = PtInRect(&rc, pt) != 0;
        if (state && hover != state->hover)
        {
            state->hover = hover;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    }
    case WM_MOUSELEAVE:
    {
        if (state)
        {
            state->tracking = false;
            if (state->hover)
            {
                state->hover = false;
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        break;
    }

    case BM_SETCHECK:
    case BM_CLICK:
    case BM_SETSTATE:
    {
        LRESULT res = DefSubclassProc(hwnd, uMsg, wParam, lParam);
        InvalidateRect(hwnd, NULL, TRUE);
        return res;
    }

    case WM_ENABLE:
    {
        LRESULT res = DefSubclassProc(hwnd, uMsg, wParam, lParam);
        InvalidateRect(hwnd, NULL, TRUE);
        return res;
    }

    case WM_LBUTTONUP:
    {
        LRESULT res = DefSubclassProc(hwnd, uMsg, wParam, lParam);
        InvalidateRect(hwnd, NULL, TRUE);
        return res;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rcClient; GetClientRect(hwnd, &rcClient);
        int w = rcClient.right - rcClient.left;
        int h = rcClient.bottom - rcClient.top;

        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmp = CreateCompatibleBitmap(hdc, w, h);
        HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcMem, hbmp);

        FillRect(hdcMem, &rcClient, g_hBtnBgBrush);

        BOOL enabled = IsWindowEnabled(hwnd);

        wchar_t textBuf[256] = {};
        GetWindowTextW(hwnd, textBuf, _countof(textBuf));

        LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
        UINT type = style & BS_TYPEMASK;

        bool isCheckbox = (type == BS_CHECKBOX) || (type == BS_AUTOCHECKBOX) ||
            (type == BS_3STATE) || (type == BS_AUTO3STATE);
        bool isRadio = (type == BS_RADIOBUTTON) || (type == BS_AUTORADIOBUTTON);
        bool isGroupBox = (type == BS_GROUPBOX);

        int checked = (int)SendMessage(hwnd, BM_GETCHECK, 0, 0);

        const int kGlyphSize = 12;
        RECT glyphRc = { 2, (h - kGlyphSize) / 2, 2 + kGlyphSize, (h - kGlyphSize) / 2 + kGlyphSize };

        if (isCheckbox)
        {
            HPEN hOldPen = (HPEN)SelectObject(hdcMem, (state && state->hover) ? g_hHoverBorderPen : g_hCheckBorderPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdcMem, GetStockObject(NULL_BRUSH));

            if (checked == BST_CHECKED)
                DrawCheckMark(hdcMem, glyphRc);

            RoundRect(hdcMem, glyphRc.left, glyphRc.top, glyphRc.right, glyphRc.bottom, 4, 4);

            SelectObject(hdcMem, hOldBrush);
            SelectObject(hdcMem, hOldPen);
        }
        else if (isRadio)
        {
            HBITMAP hBmp = (HBITMAP)LoadImage(static_cast<HINSTANCE>(FA2sp::hInstance),
                MAKEINTRESOURCE(checked == BST_CHECKED ? 
                    ((state && state->hover) ? 1031 : 1029): ((state && state->hover) ? 1032 : 1030)),
                IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
            if (hBmp)
            {
                HDC hBmpDC = CreateCompatibleDC(hdc);
                HBITMAP hOldBmp = (HBITMAP)SelectObject(hBmpDC, hBmp);

                BITMAP bm;
                GetObject(hBmp, sizeof(bm), &bm);

                TransparentBlt(
                    hdcMem,  
                    glyphRc.left - 2, glyphRc.top - 2,
                    bm.bmWidth, bm.bmHeight,
                    hBmpDC, 0, 0, bm.bmWidth, bm.bmHeight,
                    DarkColors::Background);

                SelectObject(hBmpDC, hOldBmp);
                DeleteDC(hBmpDC);
                DeleteObject(hBmp);
            }
        }

        RECT textRc = { glyphRc.right + 4, 0, w - 4, h };
        SetBkMode(hdcMem, TRANSPARENT);
        SetTextColor(hdcMem, enabled ? DarkColors::TextColor : DarkColors::DisabledTextColor);
        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);
        DrawTextW(hdcMem, textBuf, -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdcMem, hOldFont);

        BitBlt(hdc, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);
        SelectObject(hdcMem, hbmpOld);
        DeleteObject(hbmp);
        DeleteDC(hdcMem);

        EndPaint(hwnd, &ps);
        return 0;
    }

    default:
        break;
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK DarkTheme::DarkGroupBoxclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rcClient; GetClientRect(hwnd, &rcClient);

        BOOL enabled = IsWindowEnabled(hwnd);

        wchar_t textBuf[256] = {};
        GetWindowTextW(hwnd, textBuf, _countof(textBuf));

        RECT textRc = rcClient;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, enabled ? DarkColors::TextColor : DarkColors::DisabledTextColor);

        HFONT hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        DrawTextW(hdc, textBuf, -1, &textRc, DT_CALCRECT | DT_SINGLELINE);
        int textHeight = textRc.bottom - textRc.top;
        textRc.left += 8;
        textRc.right += 8;

        HPEN hOldPen = (HPEN)SelectObject(hdc, g_hGroupBoxBorderPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

        int y = textHeight / 2;
        MoveToEx(hdc, 0, y, NULL);
        LineTo(hdc, textRc.left - 2, y);
        MoveToEx(hdc, textRc.right + 2, y, NULL);
        LineTo(hdc, rcClient.right, y);
        LineTo(hdc, rcClient.right, rcClient.bottom);
        LineTo(hdc, 0, rcClient.bottom);
        LineTo(hdc, 0, y);

        DrawTextW(hdc, textBuf, -1, &textRc, DT_SINGLELINE | DT_LEFT | DT_TOP);

        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldFont);

        EndPaint(hwnd, &ps);
        return 0;
    }
    default:
        break;
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK DarkTheme::DarkStatusBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        FillRect(hdc, &rcClient, g_hDarkBackgroundBrush);

        int partCount = (int)SendMessage(hwnd, SB_GETPARTS, 0, 0);

        HFONT hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, IsWindowEnabled(hwnd) ? DarkColors::LightText : DarkColors::DisabledText);

        for (int i = 0; i < partCount; i++)
        {
            RECT rcPart{};
            SendMessage(hwnd, SB_GETRECT, i, (LPARAM)&rcPart);

            if (i < partCount - 1)
            {
                HPEN hOldPen = (HPEN)SelectObject(hdc, g_hBorderPen);
                MoveToEx(hdc, rcPart.right - 1, rcPart.top + 2, NULL);
                LineTo(hdc, rcPart.right - 1, rcPart.bottom - 2);
                SelectObject(hdc, hOldPen);
            }

            char buf[512] = {};
            SendMessage(hwnd, SB_GETTEXT, i, (LPARAM)buf);

            UINT dtFlags = DT_SINGLELINE | DT_VCENTER;

            if (i == partCount - 1)
                dtFlags |= DT_RIGHT | DT_END_ELLIPSIS;
            else
                dtFlags |= DT_LEFT | DT_END_ELLIPSIS;

            DrawText(hdc, buf, -1, &rcPart, dtFlags);
        }

        SelectObject(hdc, hOldFont);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, DarkStatusBarProc, uIdSubclass);
        break;
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

void DarkTheme::SubclassAllControls(HWND hWndParent)
{
    if (!ExtConfigs::EnableDarkMode)
        return;

    HWND hWndChild = NULL;
    while ((hWndChild = FindWindowEx(hWndParent, hWndChild, WC_COMBOBOX, NULL)) != NULL)
    {
        SetWindowSubclass(hWndChild, ComboBoxSubclassProc, 0, 0);
    }

    hWndChild = NULL;
    while ((hWndChild = FindWindowEx(hWndParent, hWndChild, WC_EDIT, NULL)) != NULL)
    {
        SetWindowSubclass(hWndChild, EditSubclassProc, 0, 0);
    }

    hWndChild = NULL;
    while ((hWndChild = FindWindowEx(hWndParent, hWndChild, WC_LISTBOX, NULL)) != NULL)
    {
        SetWindowSubclass(hWndChild, EditSubclassProc, 0, 0);
    }
    
    hWndChild = NULL;
    while ((hWndChild = FindWindowEx(hWndParent, hWndChild, "RichEdit", NULL)) != NULL)
    {
        SetWindowSubclass(hWndChild, EditSubclassProc, 0, 0);
    }

    hWndChild = NULL;
    while ((hWndChild = FindWindowEx(hWndParent, hWndChild, "SysListView32", NULL)) != NULL)
    {
        SetWindowSubclass(hWndChild, EditSubclassProc, 0, 0);
    }

    hWndChild = NULL;
    while ((hWndChild = FindWindowEx(hWndParent, hWndChild, "AfxFrameOrView42s", NULL)) != NULL)
    {
        SetWindowSubclass(hWndChild, EditSubclassProc, 0, 0);
    }

    hWndChild = NULL;
    while ((hWndChild = FindWindowEx(hWndParent, hWndChild, "msctls_statusbar32", NULL)) != NULL)
    {
        SetWindowSubclass(hWndChild, DarkStatusBarProc, 1, 0);
    }

    hWndChild = NULL;
    while ((hWndChild = FindWindowEx(hWndParent, hWndChild, NULL, NULL)) != NULL)
    {
        SubclassAllControls(hWndChild);
    }
}

LRESULT DarkTheme::GenericWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (!ExtConfigs::EnableDarkMode)
        return FALSE;

    if (CFinalSunDlg::Instance
        && hWnd == CFinalSunDlg::Instance->GetSafeHwnd())
    {
        switch (Msg)
        {
        case WM_MOVE:
        case WM_SIZE:
            DarkTheme::UpdateMenuOverlayPosition(hWnd);
        }
    }

    LRESULT menuResult = HandleMenuMessages(hWnd, Msg, wParam, lParam);
    if (menuResult != 0)
        return menuResult;

    if (Msg == WM_ERASEBKGND)
    {
        HDC hdc = (HDC)wParam;
        RECT rc;
        ::GetClientRect(hWnd, &rc);
        ::FillRect(hdc, &rc, g_hDarkBackgroundBrush);

        return TRUE;
    }
    else if (Msg == WM_PAINT)
    {
        if (GetPropW(hWnd, L"IS_MESSAGEBOX"))
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);
            FillRect(hdc, &rc, g_hDarkBackgroundBrush);
            EndPaint(hWnd, &ps);
        }
    }
    else if (Msg >= WM_CTLCOLORMSGBOX && Msg <= WM_CTLCOLORSTATIC)
    {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetBkColor(hdc, DarkColors::Background);
        SetTextColor(hdc, DarkColors::LightText);
        return (LRESULT)g_hDarkBackgroundBrush;
    }
    else if (Msg == WM_NOTIFY)
    {
        LPNMHDR pNMHDR = (LPNMHDR)lParam;
        if (pNMHDR->code == NM_CUSTOMDRAW)
            return HandleCustomDraw(lParam);
    }
    else if (Msg == WM_CTLCOLOR)
    {
        return (HRESULT)GetStockObject(DKGRAY_BRUSH);
    }
    return FALSE;
}


void DarkTheme::EnumChildrenFileDialogCheck(HWND hWnd, FileDialogFeatures& features)
{
    HWND hChild = nullptr;
    while ((hChild = FindWindowExW(hWnd, hChild, nullptr, nullptr)) != nullptr)
    {
        wchar_t buf[256];
        if (GetClassNameW(hChild, buf, 256))
        {
            if (_wcsicmp(buf, L"DirectUIHWND") == 0)  features.hasDirectUI = true;
            else if (_wcsicmp(buf, L"SysTreeView32") == 0)  features.hasTree = true;
            else if (_wcsicmp(buf, L"ToolbarWindow32") == 0) features.hasToolbar = true;
            else if (_wcsicmp(buf, L"ReBarWindow32") == 0)   features.hasReBar = true;
        }

        EnumChildrenFileDialogCheck(hChild, features);
    }
}

bool DarkTheme::IsModernFileDialog(HWND hWnd)
{
    wchar_t cls[256];
    if (!GetClassNameW(hWnd, cls, 256))
        return false;
    if (wcscmp(cls, L"#32770") != 0)
        return false;

    FileDialogFeatures features;
    EnumChildrenFileDialogCheck(hWnd, features);

    return features.hasDirectUI && features.hasTree &&
        features.hasToolbar && features.hasReBar;
}

void DarkTheme::InitDialogOptions(HWND hWnd)
{
    if (ExtConfigs::EnableDarkMode)
    {
        WCHAR className[32] = {};
        GetClassNameW(hWnd, className, 32);
        if (wcscmp(className, L"#32770") == 0)
        {
            for (int i = IDOK; i <= IDCONTINUE; ++i)
            {
                if (GetDlgItem(hWnd, i))
                {
                    HMODULE hMod = (HMODULE)GetWindowLongPtrW(hWnd, GWLP_HINSTANCE);
                    if (hMod != NULL)
                    {
                        wchar_t modPath[MAX_PATH];
                        if (GetModuleFileNameW(hMod, modPath, _countof(modPath)) != 0)
                        {
                            // CHOOSECOLOR dialog
                            if (wcsstr(modPath, L"comdlg32") != nullptr || wcsstr(modPath, L"comdlg32.dll") != nullptr)
                                break;
                        }
                    }
                    SetPropW(hWnd, L"IS_MESSAGEBOX", (HANDLE)1);
                    break;
                }
            }
        }
        SetDarkTheme(hWnd);
        SubclassAllControls(hWnd);
        SubclassAllAutoButtons(hWnd);
    }
    HWND mapValidatorList = ::GetDlgItem(hWnd, 1357);
    HWND mapValidatorText = ::GetDlgItem(hWnd, 1358);
    if (mapValidatorList && mapValidatorText)
    {
        LVCOLUMN col = { 0 };
        col.mask = LVCF_WIDTH;
        col.cx = 800;
        ListView_InsertColumn(mapValidatorList, 0, &col);
    }
}

LRESULT WINAPI DarkTheme::MyDefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg == WM_INITDIALOG)
    {
        LRESULT result = ::DefWindowProcA(hWnd, Msg, wParam, lParam);
        if (IsModernFileDialog(hWnd))
        {
            return result;
        }
        InitDialogOptions(hWnd);
        return result;
    }

    LRESULT result = GenericWindowProcA(hWnd, Msg, wParam, lParam);
    if (result)
        return result;

    return ::DefWindowProcA(hWnd, Msg, wParam, lParam);
}

LRESULT WINAPI DarkTheme::MyCallWindowProcA(
    WNDPROC lpPrevWndFunc,
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam)
{
    if (Msg == WM_INITDIALOG)
    {
        LRESULT result = ::CallWindowProcA(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
        if (IsModernFileDialog(hWnd))
        {
            return result;
        }
        InitDialogOptions(hWnd);       
        return result;
    }

    LRESULT result = GenericWindowProcA(hWnd, Msg, wParam, lParam);
    if (result)
        return result;

    return ::CallWindowProcA(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
}

std::string DarkTheme::GetIniPath(const char* iniFile)
{
    char path[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, path, MAX_PATH);
    std::string iniPath(path);
    size_t pos = iniPath.find_last_of("\\/");
    if (pos != std::string::npos)
        iniPath = iniPath.substr(0, pos + 1);
    iniPath += iniFile;
    return iniPath;
}

std::string DarkTheme::ReadIniString(const char* iniFile, const std::string& section, const std::string& key, const std::string& defaultValue)
{
    std::string iniPath = GetIniPath(iniFile);
    char buffer[512] = { 0 };
    GetPrivateProfileString(section.c_str(), key.c_str(), defaultValue.c_str(), buffer, _countof(buffer), iniPath.c_str());
    return std::string(buffer);
}

std::vector<FilterSpecEx> DarkTheme::ConvertFilter(LPCSTR lpstrFilter)
{
    std::vector<FilterSpecEx> filters;
    if (!lpstrFilter) return filters;

    LPCSTR p = lpstrFilter;
    while (*p)
    {
        std::string name = p;
        p += name.size() + 1;
        if (!*p) break;

        std::string pattern = p;
        p += pattern.size() + 1;

        filters.push_back({ STDHelpers::StringToWString(name), STDHelpers::StringToWString(pattern)});
    }
    return filters;
}

BOOL DarkTheme::HandleDialogResult(IFileDialog* pfd, OPENFILENAMEA* ofn, std::vector<COMDLG_FILTERSPEC>* specs, bool isSave)
{
    if (!pfd || !ofn || !ofn->lpstrFile) return FALSE;

    IShellItem* pItem = nullptr;
    if (FAILED(pfd->GetResult(&pItem)) || !pItem) return FALSE;

    PWSTR pszFilePath = nullptr;
    if (FAILED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)) || !pszFilePath)
    {
        pItem->Release();
        return FALSE;
    }

    std::wstring wpath = pszFilePath;
    pItem->Release();
    CoTaskMemFree(pszFilePath);

    if (isSave && specs && !specs->empty())
    {
        size_t dotPos = wpath.find_last_of(L'.');
        size_t slashPos = wpath.find_last_of(L"\\/");

        if (dotPos == std::wstring::npos || (slashPos != std::wstring::npos && dotPos < slashPos))
        {
            UINT idx = ofn->nFilterIndex ? ofn->nFilterIndex : 1;
            if (idx > specs->size()) idx = (UINT)specs->size();

            std::wstring ext = (*specs)[idx - 1].pszSpec;

            size_t star = ext.find(L'*');
            size_t dot = ext.find(L'.', star);
            if (dot != std::wstring::npos) ext = ext.substr(dot);

            if (!ext.empty())
                wpath += ext;
        }
    }

    std::string pathA = STDHelpers::WStringToString(wpath);
    lstrcpynA(ofn->lpstrFile, pathA.c_str(), ofn->nMaxFile);

    return TRUE;
}

BOOL WINAPI DarkTheme::MyGetOpenFileNameA(LPOPENFILENAMEA ofn)
{
    if (!ofn || !ofn->lpstrFile) return FALSE;

    auto veh = VEHGuard(false);

    HRESULT hr = CoInitializeEx(nullptr,
        COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    IFileOpenDialog* pFileOpen = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileOpen));
    if (FAILED(hr)) { if (SUCCEEDED(hr)) CoUninitialize(); return FALSE; }

    DWORD dwOptions;
    pFileOpen->GetOptions(&dwOptions);
    dwOptions |= FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST;
    if (ofn->Flags & OFN_ALLOWMULTISELECT) dwOptions |= FOS_ALLOWMULTISELECT;
    pFileOpen->SetOptions(dwOptions);

    if (ofn->lpstrInitialDir)
    {
        IShellItem* psiFolder = nullptr;
        std::wstring dir = STDHelpers::StringToWString(ofn->lpstrInitialDir);
        if (SUCCEEDED(SHCreateItemFromParsingName(dir.c_str(), nullptr, IID_PPV_ARGS(&psiFolder))))
        {
            pFileOpen->SetDefaultFolder(psiFolder);
            psiFolder->Release();
        }
    }

    if (DarkTheme::b_isSelectingGameFolder)
    {
        DarkTheme::b_isSelectingGameFolder = false;
#ifdef CHINESE
        ofn->lpstrFilter = "Mix 文件 (ra2md.mix)\0*.mix\0可执行文件 (gamemd.exe)\0*.exe\0所有文件 (*.*)\0*.*\0";
        ofn->lpstrTitle = "选择文件";
#else
        ofn->lpstrFilter = "Mix Files (ra2md.mix)\0*.mix\0Executable Files (gamemd.exe)\0*.exe\0All Files (*.*)\0*.*\0";
        ofn->lpstrTitle = "Select File";
#endif
    }

    auto filters = ConvertFilter(ofn->lpstrFilter);
    if (!filters.empty())
    {
        std::vector<COMDLG_FILTERSPEC> specs(filters.size());
        for (int i = 0; i < filters.size(); ++i)
        {
            specs[i].pszName = filters[i].name.c_str();
            specs[i].pszSpec = filters[i].pattern.c_str();
        }
        pFileOpen->SetFileTypes(static_cast<UINT>(specs.size()), specs.data());
        pFileOpen->SetFileTypeIndex(ofn->nFilterIndex);
    }

    hr = pFileOpen->Show(ofn->hwndOwner);
    if (FAILED(hr)) { pFileOpen->Release(); CoUninitialize(); return FALSE; }

    BOOL ok = HandleDialogResult(pFileOpen, ofn, nullptr, false);
    pFileOpen->Release();
    CoUninitialize();
    return ok;
}

BOOL WINAPI DarkTheme::MyGetSaveFileNameA(LPOPENFILENAMEA ofn)
{
    if (DarkTheme::b_isImportingINI)
    {
        DarkTheme::b_isImportingINI = false;
        return MyGetOpenFileNameA(ofn);
    }

    if (!ofn || !ofn->lpstrFile) return FALSE;

    auto veh = VEHGuard(false);

    HRESULT hr = CoInitializeEx(nullptr,
        COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    IFileSaveDialog* pFileSave = nullptr;
    hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileSave));
    if (FAILED(hr)) { if (SUCCEEDED(hr)) CoUninitialize(); return FALSE; }

    DWORD dwOptions;
    pFileSave->GetOptions(&dwOptions);
    dwOptions |= FOS_OVERWRITEPROMPT | FOS_PATHMUSTEXIST;
    pFileSave->SetOptions(dwOptions);

    if (ofn->lpstrInitialDir)
    {
        IShellItem* psiFolder = nullptr;
        std::wstring dir = STDHelpers::StringToWString(ofn->lpstrInitialDir);
        if (SUCCEEDED(SHCreateItemFromParsingName(dir.c_str(), nullptr, IID_PPV_ARGS(&psiFolder))))
        {
            pFileSave->SetDefaultFolder(psiFolder);
            psiFolder->Release();
        }
    }

    auto filters = ConvertFilter(ofn->lpstrFilter);
    std::vector<COMDLG_FILTERSPEC> specs(filters.size());
    if (!filters.empty())
    {
        for (int i = 0; i < filters.size(); ++i)
        {
            specs[i].pszName = filters[i].name.c_str();
            specs[i].pszSpec = filters[i].pattern.c_str();
        }
        pFileSave->SetFileTypes(static_cast<UINT>(filters.size()), specs.data());
        pFileSave->SetFileTypeIndex(ofn->nFilterIndex);
    }

    hr = pFileSave->Show(ofn->hwndOwner);
    if (FAILED(hr)) { pFileSave->Release(); CoUninitialize(); return FALSE; }

    BOOL ok = HandleDialogResult(pFileSave, ofn, &specs, true);
    pFileSave->Release();
    CoUninitialize();
    return ok;
}

DEFINE_HOOK(537129, ExeStart_DrakThemeHooks, 9)
{
    ExtConfigs::EnableDarkMode = STDHelpers::IsTrue(DarkTheme::ReadIniString("FAData.ini", "ExtConfigs", "EnableDarkMode", "false").c_str());
    ExtConfigs::EnableDarkMode = STDHelpers::IsTrue(DarkTheme::ReadIniString("FinalAlert.ini", "Options",
        "EnableDarkMode", ExtConfigs::EnableDarkMode ? "true" : "false").c_str());
    if (ExtConfigs::EnableDarkMode)
    {
        DarkTheme::InitDarkThemeBrushes();
        //RunTime::ResetMemoryContentAt(0x5914C4, DarkTheme::MyFillRect);
        //RunTime::ResetMemoryContentAt(0x591094, DarkTheme::MyPatBlt);
        //RunTime::ResetMemoryContentAt(0x591074, DarkTheme::MyTextOutA);
        RunTime::ResetMemoryContentAt(0x59108C, DarkTheme::MySetBkColor);
        //RunTime::ResetMemoryContentAt(0x5910E0, DarkTheme::MyExtTextOutA);
        RunTime::ResetMemoryContentAt(0x591588, DarkTheme::MyGetSysColor);
        RunTime::ResetMemoryContentAt(0x5913CC, DarkTheme::MyGetSysColorBrush);
        RunTime::ResetMemoryContentAt(0x59107C, DarkTheme::MyGetStockObject);
        //RunTime::ResetMemoryContentAt(0x591070, DarkTheme::MyCreatePen);
        //RunTime::ResetMemoryContentAt(0x591078, DarkTheme::MySetTextColor);
        RunTime::SetJump(0x5636C0, (DWORD)DarkTheme::MyOnEraseBkgnd);
        //RunTime::ResetMemoryContentAt(0x591060, DarkTheme::MySetPixel);
    }
    RunTime::ResetMemoryContentAt(0x591448, DarkTheme::MyDefWindowProcA);
    RunTime::ResetMemoryContentAt(0x591464, DarkTheme::MyCallWindowProcA);
    RunTime::ResetMemoryContentAt(0x5915D4, DarkTheme::MyGetOpenFileNameA);
    RunTime::ResetMemoryContentAt(0x5915DC, DarkTheme::MyGetSaveFileNameA);

    return 0;
}