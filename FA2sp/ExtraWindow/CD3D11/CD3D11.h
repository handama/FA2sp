#pragma once

#include <map>
#include <vector>
#include <string>
#include <regex>
#include <memory>
#include <windef.h>

class ImageDataClassSafe;

class CD3D11 {
public:
    static HWND m_hwnd;
    static HWND GetHandle()
    {
        return m_hwnd;
    }

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static void Create(HWND hParent);
    static void Close(HWND& hWnd);
    static void Initialize(HWND& hWnd);
};