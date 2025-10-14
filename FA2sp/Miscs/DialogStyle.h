#pragma once
#include <Helpers/Macro.h>
#include <./../FA2sp/Logger.h>
#include <./../FA2sp/FA2sp.h>
#include <../../MFC42/include/AFXWIN.H>
#include <./../FA2pp/MFC/ppmfc_cwnd.h>
#include <Uxtheme.h>
#pragma comment(lib, "uxtheme.lib")
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#include <shobjidl.h>

struct MenuItemInfo
{
    std::wstring text;
    BOOL bChecked;
    BOOL bRadioCheck;
    UINT uID;
};

struct TopMenuItemInfo
{
    std::wstring text;
    RECT rect;
    bool isHighlighted;
    bool isDisabled;
};

struct DarkBtnState {
    bool hover = false;
    bool tracking = false;
};

struct FileDialogFeatures {
    bool hasDirectUI = false;
    bool hasTree = false;
    bool hasToolbar = false;
    bool hasReBar = false;
};

struct FilterSpecEx {
    std::wstring name;
    std::wstring pattern;
};

namespace DarkColors
{
    constexpr COLORREF Background = RGB(32, 32, 32);
    constexpr COLORREF Control = RGB(32, 32, 32);
    constexpr COLORREF Menu = RGB(40, 40, 40);
    constexpr COLORREF InfoBk = RGB(50, 50, 50);
    constexpr COLORREF LightText = RGB(220, 220, 220);
    constexpr COLORREF Highlight = RGB(64, 64, 64);
    constexpr COLORREF DisabledBg = RGB(48, 48, 48);
    constexpr COLORREF DisabledText = RGB(128, 128, 128);

    constexpr COLORREF BtnBg = RGB(32, 32, 32);
    constexpr COLORREF BtnBgHover = RGB(64, 64, 64);
    constexpr COLORREF GroupBoxBorder = RGB(64, 64, 64);
    constexpr COLORREF TextColor = RGB(220, 220, 220);
    constexpr COLORREF DisabledTextColor = RGB(120, 120, 120);
    constexpr COLORREF CheckBorder = RGB(180, 180, 180);
    constexpr COLORREF CheckFill = RGB(50, 90, 128);
    constexpr COLORREF RadioBorder = RGB(180, 180, 180);
    constexpr COLORREF RadioFill = RGB(200, 200, 200);
    constexpr COLORREF HoverBorder = RGB(100, 180, 255);

    constexpr COLORREF DarkGray = RGB(45, 45, 45);
    constexpr COLORREF MediumGray = RGB(60, 60, 60);
    constexpr COLORREF LightGray = RGB(72, 72, 72);
    constexpr COLORREF BorderGray = RGB(96, 96, 96);
    constexpr COLORREF White = RGB(255, 255, 255);
}

class DarkTheme
{
public:
    static LRESULT WINAPI MyDefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static LRESULT WINAPI MyCallWindowProcA(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static std::string GetIniPath(const char* iniFile);
    static std::string ReadIniString(const char* iniFile, const std::string& section, const std::string& key, const std::string& defaultValue);
    static std::vector<FilterSpecEx> ConvertFilter(LPCSTR lpstrFilter);
    static BOOL HandleDialogResult(IFileDialog* pfd, OPENFILENAMEA* ofn, std::vector<COMDLG_FILTERSPEC>* specs, bool isSave);
    static BOOL __stdcall MyGetOpenFileNameA(LPOPENFILENAMEA ofn);
    static BOOL __stdcall MyGetSaveFileNameA(LPOPENFILENAMEA ofn);
    static LRESULT WINAPI GenericWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static LRESULT WINAPI HandleCustomDraw(LPARAM lParam);
    static void SetDarkTheme(HWND hWndParent);
    static void InitDarkThemeBrushes();
    static void CleanupDarkThemeBrushes();
    static LRESULT CALLBACK TabCtrlSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static BOOL SubclassListViewHeader(HWND hListView);
    static void EnableOwnerDrawMenu(HMENU hMenu, bool isTopLevel = true);
    static HWND CreateMenuOverlay(HWND hParent);
    static void InitializeMenuOverlay(HWND hWnd);
    static void UpdateMenuOverlayPosition(HWND hWnd);
    static void CleanupMenuOverlay();
    static void UpdateHighlightStates();
    static void InitMenuItems(HWND hOverlay);
    static void DrawMenuItems(HDC hdc, RECT rc);
    static void SubclassAllControls(HWND hWndParent);
    static void InitDialogOptions(HWND hWnd);

    static int WINAPI MyFillRect(HDC hDC, const RECT* lprc, HBRUSH hbr);
    static BOOL WINAPI MyPatBlt(HDC hdc, int x, int y, int w, int h, DWORD rop);
    static BOOL WINAPI MyTextOutA(HDC hdc, int x, int y, LPCSTR lpString, int c);
    static COLORREF WINAPI MySetBkColor(HDC hdc, COLORREF color);
    static BOOL WINAPI MyExtTextOutA(HDC hdc, int x, int y, UINT options, const RECT* lprc, LPCSTR lpString, UINT c, const INT* lpDx);
    static COLORREF WINAPI MySetPixel(HDC hdc, int x, int y, COLORREF color);
    static BOOL __fastcall MyOnEraseBkgnd(CFrameWnd* pThis, void* edx, CDC* pDC);
    static DWORD WINAPI MyGetSysColor(int nIndex);
    static HBRUSH WINAPI MyGetSysColorBrush(int nIndex);
    static HGDIOBJ WINAPI MyGetStockObject(int fnObject);
    static HPEN WINAPI MyCreatePen(int iStyle, int cWidth, COLORREF crColor);
    static COLORREF WINAPI MySetTextColor(HDC hdc, COLORREF crColor);

    static LRESULT CALLBACK HeaderSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT HandleMenuMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK MenuOverlayProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ComboBoxSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT CALLBACK DarkButtonSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT CALLBACK DarkGroupBoxclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT CALLBACK DarkStatusBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

    static void DrawComboBoxArrow(HDC hdc, RECT rc, bool enabled);
    static void DrawCheckMark(HDC hdc, RECT rc);
    static void SubclassDarkButton(HWND hwndButton);
    static void SubclassDarkGroupBox(HWND hwndButton);
    static void UnsubclassDarkButton(HWND hwndButton);
    static void SubclassAllAutoButtons(HWND hParent);
    static void EnumChildrenFileDialogCheck(HWND hWnd, FileDialogFeatures& features);
    static bool IsModernFileDialog(HWND hWnd);

    static HBRUSH g_hDarkBackgroundBrush;
    static HBRUSH g_hDarkControlBrush;
    static HBRUSH g_hDarkMenuBrush;
    static HBRUSH g_hDarkInfoBkBrush;
    static HBRUSH g_hLightTextBrush;
    static HBRUSH g_hHighlightBrush;
    static HBRUSH g_hDisabledBgBrush;
    static HBRUSH g_hDisabledTextBrush;

    static HBRUSH g_hBtnBgBrush;
    static HBRUSH g_hBtnBgHoverBrush;
    static HBRUSH g_hCheckFillBrush;
    static HBRUSH g_hRadioFillBrush;

    static HPEN g_hBorderPen;
    static HPEN g_hBackGroundPen;
    static HPEN g_hCheckBorderPen;
    static HPEN g_hRadioBorderPen;
    static HPEN g_hHoverBorderPen;
    static HPEN g_hGroupBoxBorderPen;

    static HWND g_hMenuOverlay;
    static HMENU g_hMainMenu;

    static std::map<HMENU, std::map<UINT, MenuItemInfo>> g_menuItemData;
    static std::vector<TopMenuItemInfo> g_menuItems;
    static bool b_isImportingINI;
    static bool b_isSelectingGameFolder;
};