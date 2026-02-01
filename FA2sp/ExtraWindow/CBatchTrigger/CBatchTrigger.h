#pragma once

#include <FA2PP.h>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../CNewScript/CNewScript.h"
#include "../../Helpers/FString.h"

// A static window class
class CBatchTrigger
{
public:
    enum Controls { 
        Listbox = 1000, 
        ListView = 1001,
        ObjectText = 1002
    };
    static void Create(CFinalSunDlg* pWnd);

    static HWND GetHandle()
    {
        return CBatchTrigger::m_hwnd;
    }

protected:
    static void Initialize(HWND& hWnd);
    static void Close(HWND& hWnd);
    static void Update();

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static void ListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam);
    static LRESULT CALLBACK ListBoxSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void OnDbClickListbox();

private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static CINI& map;
    static MultimapHelper& rules;

    static HWND hListbox;
    static HWND hListView;
    static int origWndWidth;
    static int origWndHeight;
    static int minWndWidth;
    static int minWndHeight;
    static bool minSizeSet; 
    static WNDPROC OriginalListBoxProc;
    static WNDPROC g_pOriginalListViewProc;
    static std::list<FString> ListboxTriggerID;
};

