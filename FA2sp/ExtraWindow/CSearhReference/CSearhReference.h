#pragma once

#include <FA2PP.h>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../CNewScript/CNewScript.h"
#include "../../Helpers/FString.h"

enum ScriptParamPos : int
{
    normal = 0,
    low = 1,
    high = 2
};

// A static window class
class CSearhReference
{
public:
    enum Controls { Listbox = 1000, Refresh = 1001, ObjectText = 1002};
    static void Create(CFinalSunDlg* pWnd);

    static HWND GetHandle()
    {
        return CSearhReference::m_hwnd;
    }
    static void SetSearchID(const char* id) 
    {
        SearchID = id;
    }
    // 0 team, 1 trigger, 2 taskforce/script, 3 variable
    static void SetSearchType(int idx) 
    {
        IsTeamType = false;
        IsTrigger = false;
        IsVariable = false;
        switch (idx)
        {
        case 0:
            IsTeamType = true;
            break;
        case 1:
            IsTrigger = true;
            break;
        case 3:
            IsVariable = true;
            break;
        default:
            break;
        }
    }

protected:
    static void Initialize(HWND& hWnd);
    static void Close(HWND& hWnd);
    static void Update();

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static void ListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam);
    static void OnSelchangeListbox(HWND hWnd);
    static FString GetPrefix(int type);


private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static CINI& map;
    static MultimapHelper& rules;

    static HWND hListbox;
    static HWND hRefresh;
    static HWND hObjectText;
    static FString SearchID;
    static int origWndWidth;
    static int origWndHeight;
    static int minWndWidth;
    static int minWndHeight;
    static bool minSizeSet;
public:
    static bool IsTeamType;
    static bool IsTrigger;
    static bool IsVariable;
    // first = id, second = pos
    static std::map<int, ScriptParamPos> LocalVariableScripts;
    // first = id, second = pos (not include event num), 10+ = check param affected params
    static std::map<int, int> LocalVariableEvents;
    static std::map<int, int> LocalVariableActions;

    // first = id, second = param index, value
    static std::map<int, std::map<int, FString>> LocalVariableParamAffectedEvents;
    static std::map<int, std::map<int, FString>> LocalVariableParamAffectedActions;
};

