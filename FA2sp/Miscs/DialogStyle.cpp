#include "DialogStyle.h"
#include "../Helpers/STDHelpers.h"

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
        if (style >= BS_CHECKBOX && style <= BS_PUSHBOX && _wcsicmp(className, L"SysTreeView32") != 0)
        {
            SetWindowTheme(hWndChild, L"", L"");
        }
        else
        {
            SetWindowTheme(hWndChild, L"DarkMode_Explorer", NULL);
        }
        SetDarkTheme(hWndChild);
    }
}

LRESULT DarkTheme::GenericWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
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
