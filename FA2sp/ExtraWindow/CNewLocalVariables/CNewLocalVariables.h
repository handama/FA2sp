#pragma once

#include <FA2PP.h>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../CNewScript/CNewScript.h"
#include "../../Helpers/FString.h"

class VirtualComboBoxEx;
// A static window class
class CNewLocalVariables
{
public:
    enum Controls { 
        Variables = 1001,
        Name = 1003,
        Value = 1005,
        New = 1006,
        Search = 1007 };
    static void Create(CFinalSunDlg* pWnd);

    static HWND GetHandle()
    {
        return CNewLocalVariables::m_hwnd;
    }

protected:
    static void Initialize(HWND& hWnd);
    static void Close(HWND& hWnd);
    static void Update();

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

    static void OnSelchangeVariable(bool edited = false);
    static void OnClickNew();
    static void OnClickSearchReference();

private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static CINI& map;
    static VirtualComboBoxEx vcbVariables;

    static HWND hVariables;
    static HWND hName;
    static HWND hValue;
    static HWND hNew;
    static HWND hSearch;
    static int SelectedIndex;
    static FString SelectedKey;
};

