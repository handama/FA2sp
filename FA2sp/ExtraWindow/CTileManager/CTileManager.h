#pragma once

#include <FA2PP.h>
#include <CFinalSunDlg.h>
#include "../../Helpers/FString.h"
#include <vector>
#include <string>
#include <regex>
#include "../Common.h"

// A static window class
class CTileManager
{
public:
    enum { ListBox_Types = 6100, ListBox_Details = 6101 };

    static void Create(CFinalSunDlg* pWnd);

    static HWND GetHandle()
        { return CTileManager::m_hwnd; }

    static void UpdateTypes(HWND hWnd);
    static void UpdateDetails(HWND hWnd, int kNode = -1);

protected:
    static void Initialize(HWND& hWnd);
    static void InitNodes();
    static void Close(HWND& hWnd);

    static BOOL CALLBACK DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

    static void TypesProc(HWND hWnd, WORD nCode, LPARAM lParam);
    static void DetailsProc(HWND hWnd, WORD nCode, LPARAM lParam);

private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static std::vector<std::pair<FString, std::regex>> Nodes;
    static std::vector<std::vector<int>> Datas;
    static int origWndWidth;
    static int origWndHeight;
    static int minWndWidth;
    static int minWndHeight;
    static bool minSizeSet;
    static TransparencyHelper m_transparency;
};

