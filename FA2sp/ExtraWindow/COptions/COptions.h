#pragma once

#include <FA2PP.h>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include "../../Ext/CFinalSunDlg/Body.h"

// A static window class
class COptions
{
public:
    static void Create(CFinalSunDlg* pWnd);
    enum Controls {
        List = 1000,
        GameEngine = 1002,
        Search = 1004,
    };

    static HWND GetHandle()
    {
        return COptions::m_hwnd;
    }

protected:
    static void Initialize(HWND& hWnd);
    static void Update(const char* filter = nullptr);
    static void Close(HWND& hWnd);

    static BOOL CALLBACK DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

    static void OnEditchangeSearch();


private:
    static HWND m_hwnd;
    static HWND hList;
    static HWND hGameEngine;
    static HWND hSearch;
    static CFinalSunDlg* m_parent;
    static bool initialized;
    static WNDPROC g_pOriginalListViewProc;
    static LRESULT CALLBACK ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

