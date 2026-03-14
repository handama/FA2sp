#include "Body.h"
#include "../CFinalSunDlg/Body.h"

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
    case WM_SIZING:
    {
        RECT* r = (RECT*)lParam;
        UINT edge = (UINT)wParam;

        DWORD style = GetWindowLongPtr(hWnd, GWL_STYLE);
        DWORD exStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
        BOOL hasMenu = ::GetMenu(hWnd) != NULL;

        RECT rcClient = { 0,0,InitWidth,(LONG)(InitWidth / ASPECT_RATIO + 0.5) };
        RECT rcWindow = rcClient;
        AdjustWindowRectEx(&rcWindow, style, hasMenu, exStyle);

        LONG ncW = (rcWindow.right - rcWindow.left) - (rcClient.right - rcClient.left);
        LONG ncH = (rcWindow.bottom - rcWindow.top) - (rcClient.bottom - rcClient.top);

        LONG winW = r->right - r->left;
        LONG winH = r->bottom - r->top;

        LONG clientW = winW - ncW;
        LONG clientH = winH - ncH;

        LONG newClientW = clientW;
        LONG newClientH = clientH;

        if (edge == WMSZ_LEFT || edge == WMSZ_RIGHT)
        {
            newClientH = (LONG)(clientW / ASPECT_RATIO + 0.5);
        }
        else if (edge == WMSZ_TOP || edge == WMSZ_BOTTOM)
        {
            newClientW = (LONG)(clientH * ASPECT_RATIO + 0.5);
        }
        else
        {
            newClientH = (LONG)(clientW / ASPECT_RATIO + 0.5);
        }

        LONG minW = InitWidth / 2;
        LONG maxW = InitWidth * 4;

        if (newClientW < minW)
        {
            newClientW = minW;
            newClientH = (LONG)(newClientW / ASPECT_RATIO + 0.5);
        }

        if (newClientW > maxW)
        {
            newClientW = maxW;
            newClientH = (LONG)(newClientW / ASPECT_RATIO + 0.5);
        }

        LONG newWinW = newClientW + ncW;
        LONG newWinH = newClientH + ncH;

        LONG centerX = (r->left + r->right) / 2;
        LONG centerY = (r->top + r->bottom) / 2;

        if (edge == WMSZ_LEFT || edge == WMSZ_RIGHT)
        {
            r->top = centerY - newWinH / 2;
            r->bottom = r->top + newWinH;
        }

        if (edge == WMSZ_TOP || edge == WMSZ_BOTTOM)
        {
            r->left = centerX - newWinW / 2;
            r->right = r->left + newWinW;
        }

        switch (edge)
        {
        case WMSZ_LEFT:
        case WMSZ_TOPLEFT:
        case WMSZ_BOTTOMLEFT:
            r->left = r->right - newWinW;
            break;

        case WMSZ_RIGHT:
        case WMSZ_TOPRIGHT:
        case WMSZ_BOTTOMRIGHT:
            r->right = r->left + newWinW;
            break;
        }

        switch (edge)
        {
        case WMSZ_TOP:
        case WMSZ_TOPLEFT:
        case WMSZ_TOPRIGHT:
            r->top = r->bottom - newWinH;
            break;

        case WMSZ_BOTTOM:
        case WMSZ_BOTTOMLEFT:
        case WMSZ_BOTTOMRIGHT:
            r->bottom = r->top + newWinH;
            break;
        }

        CurrentScale = (double)newClientW / InitWidth;

        return TRUE;
    }

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

            LONG minW = InitWidth / 2;
            LONG maxW = InitWidth * 4;

            pMMI->ptMinTrackSize.x = minW + ncW;
            pMMI->ptMinTrackSize.y = (LONG)(minW / ASPECT_RATIO + 0.5) + ncH;

            pMMI->ptMaxTrackSize.x = maxW + ncW;
            pMMI->ptMaxTrackSize.y = (LONG)(maxW / ASPECT_RATIO + 0.5) + ncH;
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