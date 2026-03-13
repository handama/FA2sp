#include "Body.h"

WNDPROC CMinimapExt::g_pfnOriginalMinimapProc = NULL;
double CMinimapExt::ASPECT_RATIO = 1.0;
int CMinimapExt::InitWidth = 100;
double CMinimapExt::CurrentScale = 1.0;

LRESULT CALLBACK CMinimapExt::MinimapWndProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* pMMI = (MINMAXINFO*)lParam;

        if (pMMI && InitWidth > 0)
        {
            DWORD style = GetWindowLongPtr(hWnd, GWL_STYLE);
            DWORD exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
            BOOL hasMenu = ::GetMenu(hWnd) != NULL;

            RECT rcClient = { 0,0,InitWidth,(LONG)(InitWidth / ASPECT_RATIO + 0.5) };
            RECT rcWindow = rcClient;

            AdjustWindowRectEx(&rcWindow, style, hasMenu, exStyle);

            LONG ncW = (rcWindow.right - rcWindow.left) - (rcClient.right - rcClient.left);
            LONG ncH = (rcWindow.bottom - rcWindow.top) - (rcClient.bottom - rcClient.top);

            LONG minClientW = InitWidth / 2;
            LONG maxClientW = InitWidth * 4;

            LONG minClientH = (LONG)(minClientW / ASPECT_RATIO + 0.5);
            LONG maxClientH = (LONG)(maxClientW / ASPECT_RATIO + 0.5);

            pMMI->ptMinTrackSize.x = minClientW + ncW;
            pMMI->ptMinTrackSize.y = minClientH + ncH;

            pMMI->ptMaxTrackSize.x = maxClientW + ncW;
            pMMI->ptMaxTrackSize.y = maxClientH + ncH;
        }

        break;
    }
    case WM_WINDOWPOSCHANGING:
    {
        WINDOWPOS* wp = (WINDOWPOS*)lParam;

        if (!(wp->flags & SWP_NOSIZE) && InitWidth > 0)
        {
            DWORD style = GetWindowLongPtr(hWnd, GWL_STYLE);
            DWORD exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
            BOOL hasMenu = ::GetMenu(hWnd) != NULL;

            RECT rcClient = { 0,0,InitWidth,(LONG)(InitWidth / ASPECT_RATIO + 0.5) };
            RECT rcWindow = rcClient;

            AdjustWindowRectEx(&rcWindow, style, hasMenu, exStyle);

            LONG ncW = (rcWindow.right - rcWindow.left) - (rcClient.right - rcClient.left);
            LONG ncH = (rcWindow.bottom - rcWindow.top) - (rcClient.bottom - rcClient.top);

            LONG winW = wp->cx;
            LONG winH = wp->cy;

            LONG clientW = winW - ncW;
            LONG clientH = winH - ncH;

            if (clientW <= 0) clientW = InitWidth;
            if (clientH <= 0) clientH = (LONG)(InitWidth / ASPECT_RATIO + 0.5);

            LONG newClientW = clientW;
            LONG newClientH = (LONG)(newClientW / ASPECT_RATIO + 0.5);

            LONG minClientW = InitWidth / 2;
            LONG maxClientW = InitWidth * 4;

            if (newClientW < minClientW)
            {
                newClientW = minClientW;
                newClientH = (LONG)(newClientW / ASPECT_RATIO + 0.5);
            }

            if (newClientW > maxClientW)
            {
                newClientW = maxClientW;
                newClientH = (LONG)(newClientW / ASPECT_RATIO + 0.5);
            }

            wp->cx = newClientW + ncW;
            wp->cy = newClientH + ncH;

            CurrentScale = (double)newClientW / InitWidth;
        }

        break;
    }

    default:
        if (g_pfnOriginalMinimapProc)
            return CallWindowProc(g_pfnOriginalMinimapProc, hWnd, uMsg, wParam, lParam);
        else
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}