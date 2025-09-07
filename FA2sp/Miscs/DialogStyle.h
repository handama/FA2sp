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
};
