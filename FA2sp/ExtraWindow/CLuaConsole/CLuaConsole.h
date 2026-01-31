#pragma once

#include <FA2PP.h>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Sol/sol.hpp"
#include "../../FA2sp.h"

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include "Lexilla.h"

// A static window class
class CLuaConsole
{
public:
    enum Controls {
        Execute = 1000,
        Run = 1001,
        OutputBox = 1002,
        InputBox = 1003,
        Scripts = 1004,
        Description = 1005,
        OutputText = 1006,
        InputText = 1007,
        ScriptText = 1008,
        RunFile = 1009,
        Apply = 1010,
        SearchText = 1012,
        //Stop = 1011,
    };
    static void Create(CFinalSunDlg* pWnd);

    static HWND GetHandle()
    {
        return CLuaConsole::m_hwnd;
    }
    static void OnClickRun(bool fromFile);
    static void UpdateCoords(int x, int y, bool firstRun, bool holdingClick);
    static void OnEditchangeSearch(HWND& hWnd);

protected:
    static void Initialize(HWND& hWnd);
    static void SetupLuaHighlight(HWND& hWnd);
    static void Close(HWND& hWnd);
    static void Update(HWND& hWnd, const char* filter = "");

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static int origWndWidth;
    static int origWndHeight;
    static int minWndWidth;
    static int minWndHeight;
    static bool minSizeSet;

public:
    static HWND hExecute;
    static HWND hRun;
    static HWND hOutputBox;
    static HWND hInputBox;
    static HWND hOutputText;
    static HWND hInputText;
    static HWND hScripts;
    static HWND hRunFile;
    static HWND hApply;
    static HWND hSearchText;
    //static HWND hStop;
    static bool applyingScript;
    static bool applyingScriptFirst;
    static bool runFile;
    static sol::state Lua;
    static bool needRedraw;
    static bool recalculateOre;
    static bool updateBuilding;
    static bool updateUnit;
    static bool updateInfantry;
    static bool updateAircraft;
    static bool updateNode;
    static bool updateMinimap;
    static bool updateTrigger;
    static bool updateAITrigger;
    static bool updateScript;
    static bool updateTeam;
    static bool updateTaskforce;
    static bool updateCellTag;
    static bool skipBuildingUpdate;
};

