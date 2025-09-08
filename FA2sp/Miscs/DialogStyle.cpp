#include "DialogStyle.h"
#include "../Helpers/STDHelpers.h"
#include <windowsx.h>
#include "CFinalSunDlg.h"

// DEFINE_HOOK(54FC1E, CFileDialog_EnableExplorerStyle, 7)
// {
//     GET(CFileDialog*, pDialog, ESI);
// 
//     OPENFILENAME ofn = pDialog->m_ofn;
// 
//     ofn.Flags &= ~OFN_ENABLEHOOK;
// 
//     if (ofn.pvReserved)
//         R->EAX(GetOpenFileNameA(&ofn));
//     else
//         R->EAX(GetSaveFileNameA(&ofn));
// 
//     return 0x54FC37;
// }
DEFINE_HOOK(4248B3, CFinalSunDlg_OpenMap_ChangeDialogStyle, 7)
{
    FA2sp::g_VEH_Enabled = false;
    R->Stack<int>(STACK_OFFS(0x60C, (0x398 - 0x8)), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST);
    return 0;
}

DEFINE_HOOK(4248DE, CFinalSunDlg_OpenMap_ChangeDialogStyle_2, 5)
{
    FA2sp::g_VEH_Enabled = true;
    if (R->EAX() != 2)
        return 0x4248F0;
    return 0x4248E3;
}

DEFINE_HOOK(42686A, CFinalSunDlg_SaveMap_ChangeDialogStyle, 5)
{
    FA2sp::g_VEH_Enabled = false;
    R->Stack<int>(STACK_OFFS(0x3CC, (0x280 - 0x8)), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST);
    return 0;
}

DEFINE_HOOK(426897, CFinalSunDlg_SaveMap_ChangeDialogStyle_2, 5)
{
    FA2sp::g_VEH_Enabled = true;
    if (R->EAX() == 2)
        return 0x42698C;
    return 0x4268A0;
}

DEFINE_HOOK(4D312E, CFinalSunDlg_ImportMap_ChangeDialogStyle, 5)
{
    FA2sp::g_VEH_Enabled = false;
    R->Stack<int>(STACK_OFFS(0x310, (0x280 - 0x8)), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST);
    return 0;
}

DEFINE_HOOK(4D3158, CFinalSunDlg_ImportMap_ChangeDialogStyle_2, 5)
{
    FA2sp::g_VEH_Enabled = true;
    if (R->EAX() != 2)
        return 0x4D316A;
    return 0x4D315D;
}

DEFINE_HOOK(40B7B3, CINIEditor_OnClickImportINI_ChangeDialogStyle, 5)
{
    FA2sp::g_VEH_Enabled = false;
    R->Stack<int>(STACK_OFFS(0x3B0, (0x280 - 0x8)), OFN_FILEMUSTEXIST);
    return 0;
}

DEFINE_HOOK(40B7CD, CINIEditor_OnClickImportINI_ChangeDialogStyle_2, 5)
{
    FA2sp::g_VEH_Enabled = true;
    if (R->EAX() == 2)
        return 0x40B865;
    return 0x40B7D6;
}

static HBRUSH g_hDarkBackgroundBrush = NULL; 
static HBRUSH g_hDarkControlBrush = NULL;
static HBRUSH g_hDarkMenuBrush = NULL; 
static HBRUSH g_hDarkInfoBkBrush = NULL; 
static HBRUSH g_hLightTextBrush = NULL;  
static HBRUSH g_hHighlightBrush = NULL;
std::map<HMENU, std::map<UINT, MenuItemInfo>> DarkTheme::g_menuItemData;

void DarkTheme::InitDarkThemeBrushes()
{
    if (!g_hDarkBackgroundBrush)
        g_hDarkBackgroundBrush = CreateSolidBrush(RGB(32, 32, 32));
    if (!g_hDarkControlBrush)
        g_hDarkControlBrush = CreateSolidBrush(RGB(32, 32, 32));
    if (!g_hDarkMenuBrush)
        g_hDarkMenuBrush = CreateSolidBrush(RGB(40, 40, 40));  
    if (!g_hDarkInfoBkBrush)
        g_hDarkInfoBkBrush = CreateSolidBrush(RGB(50, 50, 50));
    if (!g_hLightTextBrush)
        g_hLightTextBrush = CreateSolidBrush(RGB(220, 220, 220)); 
    if (!g_hHighlightBrush)
        g_hHighlightBrush = CreateSolidBrush(RGB(60, 60, 60)); 
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
    g_hDarkBackgroundBrush = g_hDarkControlBrush = g_hDarkMenuBrush
        = g_hDarkInfoBkBrush = g_hLightTextBrush = g_hHighlightBrush =NULL;
}

int WINAPI MyFillRect(HDC hDC, const RECT* lprc, HBRUSH hbr)
{
    return ::FillRect(hDC, lprc, g_hDarkBackgroundBrush);
}

BOOL WINAPI MyPatBlt(HDC hdc, int x, int y, int w, int h, DWORD rop)
{
    if (rop == BLACKNESS || rop == WHITENESS || rop == PATCOPY)
    {
        RECT rc = { x, y, x + w, y + h };
        FillRect(hdc, &rc, g_hDarkBackgroundBrush);
        return TRUE;
    }
    return ::PatBlt(hdc, x, y, w, h, rop);
}

BOOL WINAPI MyTextOutA(HDC hdc, int x, int y, LPCSTR lpString, int c)
{
    ::SetTextColor(hdc, RGB(220, 220, 220));
    ::SetBkColor(hdc, RGB(32, 32, 32));
    return ::TextOutA(hdc, x, y, lpString, c);
}

COLORREF WINAPI MySetBkColor(HDC hdc, COLORREF color)
{
    return  ::SetBkColor(hdc, RGB(48, 48, 48));
}

BOOL WINAPI MyExtTextOutA(HDC hdc, int x, int y, UINT options,
    const RECT* lprc, LPCSTR lpString,
    UINT c, const INT* lpDx)
{
    ::SetTextColor(hdc, RGB(220, 220, 220));
    ::SetBkColor(hdc, RGB(32, 32, 32));

    return ::ExtTextOutA(hdc, x, y, options, lprc, lpString, c, lpDx);
}

COLORREF WINAPI MySetPixel(HDC hdc, int x, int y, COLORREF color)
{
    return ::SetPixel(hdc, x, y, RGB(32, 32, 32));
}

BOOL __fastcall MyOnEraseBkgnd(CFrameWnd* pThis, void* /*edx*/, CDC* pDC)
{
    CRect rect; 
    pThis->GetClientRect(&rect);
    pDC->FillSolidRect(&rect, RGB(45, 45, 45));
    return TRUE;
}

DWORD WINAPI MyGetSysColor(int nIndex)
{
    switch (nIndex)
    {
    case COLOR_MENU:  
    case COLOR_MENUBAR:  
        return RGB(40, 40, 40);
    case COLOR_MENUTEXT: 
    case COLOR_WINDOWTEXT: 
    case COLOR_BTNTEXT:
        return RGB(220, 220, 220);
    case COLOR_WINDOW:  
    case COLOR_BTNFACE:   
    case COLOR_SCROLLBAR: 
        return RGB(32, 32, 32);
    case COLOR_HIGHLIGHT:
        return RGB(60, 60, 60);
    case COLOR_HIGHLIGHTTEXT:
        return RGB(255, 255, 255);
    default:
        return ::GetSysColor(nIndex);
    }
}

HBRUSH WINAPI MyGetSysColorBrush(int nIndex)
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

HGDIOBJ WINAPI MyGetStockObject(int fnObject)
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

        HBRUSH hBgBrush = CreateSolidBrush(RGB(32, 32, 32));
        FillRect(hdc, &rc, hBgBrush);
        DeleteObject(hBgBrush);

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
            HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(96, 96, 96));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hBorderPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, borderRect.left, borderRect.top, borderRect.right, borderRect.bottom);
            SelectObject(hdc, hOldPen);
            SelectObject(hdc, hOldBrush);
            DeleteObject(hBorderPen);
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
                bgColor = RGB(72, 72, 72);
                textColor = RGB(255, 255, 255);
            }
            else
            {
                bgColor = RGB(48, 48, 48);
                textColor = RGB(220, 220, 220);
            }

            HBRUSH hItemBrush = CreateSolidBrush(bgColor);
            FillRect(hdc, &itemRect, hItemBrush);
            DeleteObject(hItemBrush);

            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(96, 96, 96));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, itemRect.left, itemRect.top, itemRect.right, itemRect.bottom);
            SelectObject(hdc, hOldPen);
            SelectObject(hdc, hOldBrush);
            DeleteObject(hPen);

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

LRESULT CALLBACK HeaderSubclassProc(
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
                HBRUSH hColBrush = g_hHighlightBrush;
                FillRect(hdc, &colRect, hColBrush);

                HPEN hPen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
                HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                MoveToEx(hdc, colRect.right - 1, colRect.top, NULL);
                LineTo(hdc, colRect.right - 1, colRect.bottom);
                SelectObject(hdc, hOldPen);
                DeleteObject(hPen);

                SetTextColor(hdc, RGB(220, 220, 220));
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
            pNMTVCD->clrTextBk = isSelected ? RGB(60, 60, 60) : RGB(32, 32, 32);
            pNMTVCD->clrText = isSelected ? RGB(255, 255, 255) : RGB(220, 220, 220);
            return CDRF_DODEFAULT;
        }
    }
    }

    return CDRF_DODEFAULT;
}

BOOL DarkTheme::IsWindows10OrGreater()
{
    typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll)
        return false;

    RtlGetVersionPtr fnRtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hNtdll, "RtlGetVersion");
    if (!fnRtlGetVersion)
        return false;

    RTL_OSVERSIONINFOW rovi = { 0 };
    rovi.dwOSVersionInfoSize = sizeof(rovi);
    if (fnRtlGetVersion(&rovi) != 0)
        return false;

    return rovi.dwMajorVersion >= 10;
}

void DarkTheme::SetDarkTheme(HWND hWndParent)
{
    if (!ExtConfigs::EnableDarkMode)
        return;

    HWND hWndChild = NULL;
    if (IsWindows10OrGreater())
    {
        BOOL darkMode = TRUE;
        DwmSetWindowAttribute(hWndParent, 19, &darkMode, sizeof(darkMode));
    }

    while ((hWndChild = FindWindowEx(hWndParent, hWndChild, NULL, NULL)) != NULL)
    {
        DWORD style = GetWindowLongPtr(hWndChild, GWL_STYLE) & 0xF;
        wchar_t className[32];
        GetClassNameW(hWndChild, className, _countof(className));
        switch (style)
        {
        //case BS_CHECKBOX        :
        case BS_AUTOCHECKBOX    :
        case BS_RADIOBUTTON     :
        case BS_3STATE          :
        case BS_AUTO3STATE      :
        case BS_GROUPBOX        :
        case BS_USERBUTTON      :
        case BS_AUTORADIOBUTTON :
        case BS_PUSHBOX         :
            if (_wcsicmp(className, L"SysTreeView32") != 0)
                SetWindowTheme(hWndChild, L"", L"");
            break;
        default:
            if (_wcsicmp(className, WC_COMBOBOXW) != 0)
                SetWindowTheme(hWndChild, L"DarkMode_Explorer", NULL);
            break;
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

LRESULT HandleMenuMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
                textColor = RGB(255, 255, 255);
            }
            else if (pdis->itemState & ODS_GRAYED)
            {
                hBgBrush = g_hDarkMenuBrush;
                textColor = RGB(120, 120, 120);
            }
            else
            {
                hBgBrush = g_hDarkMenuBrush;
                textColor = RGB(220, 220, 220);
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

                if (pInfo->bRadioCheck)
                {
                    int cx = (checkRc.left + checkRc.right) / 2;
                    int cy = (checkRc.top + checkRc.bottom) / 2;
                    int rOuter = (checkRc.right - checkRc.left) / 4; 
                    int rInner = rOuter - 2;                    

                    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(150, 150, 150));
                    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
                    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

                    Ellipse(hdc, cx - rOuter, cy - rOuter, cx + rOuter, cy + rOuter);

                    HBRUSH hFillBrush = CreateSolidBrush(RGB(200, 200, 200));
                    HBRUSH hOldFill = (HBRUSH)SelectObject(hdc, hFillBrush);
                    Ellipse(hdc, cx - rInner, cy - rInner, cx + rInner, cy + rInner);
                    SelectObject(hdc, hOldFill);
                    DeleteObject(hFillBrush);

                    SelectObject(hdc, hOldBrush);
                    SelectObject(hdc, hOldPen);
                    DeleteObject(hPen);
                }
                else
                {
                    int size = checkRc.bottom - checkRc.top;
                    int width = size;
                    RECT boxRc = { checkRc.left + 3, checkRc.top, checkRc.left + width + 3, checkRc.bottom };

                    HBRUSH hBoxBrush = CreateSolidBrush(RGB(80, 80, 80)); 
                    HPEN   hBorderPen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));

                    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBoxBrush);
                    HPEN   hOldPen = (HPEN)SelectObject(hdc, hBorderPen);

                    RoundRect(hdc, boxRc.left, boxRc.top, boxRc.right, boxRc.bottom, 4, 4);

                    MoveToEx(hdc, boxRc.left + 3, boxRc.top + size / 2, NULL);
                    LineTo(hdc, boxRc.left + size / 2, boxRc.bottom - 3);
                    LineTo(hdc, boxRc.right - 3, boxRc.top + 3);

                    SelectObject(hdc, hOldBrush);
                    SelectObject(hdc, hOldPen);

                    DeleteObject(hBoxBrush);
                    DeleteObject(hBorderPen);
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

HWND g_hMenuOverlay = NULL;
HMENU g_hMainMenu = NULL;
std::vector<TopMenuItemInfo> g_menuItems;

void UpdateHighlightStates()
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

void InitMenuItems(HWND hOverlay)
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

void DrawMenuItems(HDC hdc, RECT rc)
{
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

    for (const auto& item : g_menuItems)
    {
        HBRUSH hBgBrush;
        if (item.isHighlighted)
            hBgBrush = g_hHighlightBrush;
        else
            hBgBrush = g_hDarkBackgroundBrush;

        FillRect(hdc, &item.rect, hBgBrush);

        SetBkMode(hdc, TRANSPARENT);
        if (item.isDisabled)
            SetTextColor(hdc, RGB(120, 120, 120));
        else
            SetTextColor(hdc, RGB(240, 240, 240));

        RECT textRc = item.rect;
        //textRc.left += 10;

        DrawTextW(hdc, item.text.c_str(), -1, &textRc,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    SelectObject(hdc, hOldFont);
}

LRESULT CALLBACK MenuOverlayProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

HWND CreateMenuOverlay(HWND hParent)
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

LRESULT DarkTheme::GenericWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (ExtConfigs::EnableDarkMode 
        && CFinalSunDlg::Instance 
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
    else if (Msg >= WM_CTLCOLORMSGBOX && Msg <= WM_CTLCOLORSTATIC)
    {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetBkColor(hdc, RGB(32, 32, 32));
        SetTextColor(hdc, RGB(220, 220, 220));

        return (LRESULT)g_hDarkBackgroundBrush;
    }
    else if (Msg == WM_NOTIFY)
    {
        LPNMHDR pNMHDR = (LPNMHDR)lParam;
        if (pNMHDR->code == NM_CUSTOMDRAW)
            return HandleCustomDraw(lParam);
    }
    return FALSE;
}

LRESULT WINAPI DarkTheme::MyDefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (!ExtConfigs::EnableDarkMode)
        return ::DefWindowProcA(hWnd, Msg, wParam, lParam);

    if (Msg == WM_INITDIALOG)
    {
        LRESULT result = ::DefWindowProcA(hWnd, Msg, wParam, lParam);
        SetDarkTheme(hWnd);
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
    if (!ExtConfigs::EnableDarkMode)
        return ::CallWindowProcA(lpPrevWndFunc, hWnd, Msg, wParam, lParam);

    if (Msg == WM_INITDIALOG)
    {
        LRESULT result = ::CallWindowProcA(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
        SetDarkTheme(hWnd);
        return result;
    }

    LRESULT result = GenericWindowProcA(hWnd, Msg, wParam, lParam);
    if (result)
        return result;

    return ::CallWindowProcA(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
}

HPEN WINAPI MyCreatePen(int iStyle, int cWidth, COLORREF crColor)
{
    // Force dark theme color for pen
    if (crColor == GetSysColor(COLOR_3DDKSHADOW) ||
        crColor == GetSysColor(COLOR_3DLIGHT) ||
        crColor == GetSysColor(COLOR_3DFACE))
    {
        crColor = RGB(32, 32, 32);
    }

    return ::CreatePen(iStyle, cWidth, crColor);
}

COLORREF WINAPI MySetTextColor(HDC hdc, COLORREF crColor)
{
    if (crColor == GetSysColor(COLOR_WINDOWTEXT) ||
        crColor == GetSysColor(COLOR_BTNTEXT))
    {
        crColor = RGB(220, 220, 220);
    }
    else if (crColor == GetSysColor(COLOR_HIGHLIGHTTEXT))
    {
        crColor = RGB(255, 255, 255);
    }

    return ::SetTextColor(hdc, crColor);
}

std::string GetIniPath(const char* iniFile)
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

std::string ReadIniString(const char* iniFile, const std::string& section, const std::string& key, const std::string& defaultValue)
{
    std::string iniPath = GetIniPath(iniFile);
    char buffer[512] = { 0 };
    GetPrivateProfileString(section.c_str(), key.c_str(), defaultValue.c_str(), buffer, _countof(buffer), iniPath.c_str());
    return std::string(buffer);
}

DEFINE_HOOK(537129, ExeStart_DrakThemeHooks, 9)
{
    ExtConfigs::EnableDarkMode = STDHelpers::IsTrue(ReadIniString("FAData.ini", "ExtConfigs", "EnableDarkMode", "false").c_str());
    ExtConfigs::EnableDarkMode = STDHelpers::IsTrue(ReadIniString("FinalAlert.ini", "Options",
        "EnableDarkMode", ExtConfigs::EnableDarkMode ? "true" : "false").c_str());
    if (ExtConfigs::EnableDarkMode)
    {
        DarkTheme::InitDarkThemeBrushes();
        //RunTime::ResetMemoryContentAt(0x5914C4, MyFillRect);
        //RunTime::ResetMemoryContentAt(0x591094, MyPatBlt);
        //RunTime::ResetMemoryContentAt(0x591074, MyTextOutA);
        RunTime::ResetMemoryContentAt(0x59108C, MySetBkColor);
        //RunTime::ResetMemoryContentAt(0x5910E0, MyExtTextOutA);
        RunTime::ResetMemoryContentAt(0x591588, MyGetSysColor);
        RunTime::ResetMemoryContentAt(0x5913CC, MyGetSysColorBrush);
        RunTime::ResetMemoryContentAt(0x59107C, MyGetStockObject);
        RunTime::ResetMemoryContentAt(0x591448, DarkTheme::MyDefWindowProcA);
        RunTime::ResetMemoryContentAt(0x591464, DarkTheme::MyCallWindowProcA);
        //RunTime::ResetMemoryContentAt(0x591070, MyCreatePen);
        //RunTime::ResetMemoryContentAt(0x591078, MySetTextColor);
        RunTime::SetJump(0x5636C0, (DWORD)MyOnEraseBkgnd);
        //RunTime::ResetMemoryContentAt(0x591060, MySetPixel);
    }

	return 0;
}
