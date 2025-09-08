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

class DarkTheme
{
public:
    static LRESULT WINAPI MyDefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static LRESULT WINAPI MyCallWindowProcA(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static LRESULT WINAPI GenericWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static LRESULT WINAPI HandleCustomDraw(LPARAM lParam);
    static void SetDarkTheme(HWND hWndParent);
    static BOOL IsWindows10OrGreater();
    static void InitDarkThemeBrushes();
    static void CleanupDarkThemeBrushes();
    static LRESULT CALLBACK TabCtrlSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static BOOL SubclassListViewHeader(HWND hListView);
    static void EnableOwnerDrawMenu(HMENU hMenu, bool isTopLevel = true);
    static void UpdateMenuItems(HWND hOverlay);
    static void InitializeMenuOverlay(HWND hWnd);
    static void UpdateMenuOverlayPosition(HWND hWnd);
    static void CleanupMenuOverlay();

    static std::map<HMENU, std::map<UINT, MenuItemInfo>> g_menuItemData;
};
