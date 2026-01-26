#pragma once

#include <FA2PP.h>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Helpers/MultimapHelper.h"
#include "../../Helpers/FString.h"

// A static window class
class CNewScript
{
public:
    enum Controls {
        SelectedScript = 1193,
        NewScript = 1154,
        DelScript = 1066,
        CloScript = 6300,
        AddAction = 1173,
        DeleteAction = 1174,
        CloneAction = 6301,
        Name = 1010,
        ActionsListBox = 1170,
        ActionType = 1064,
        ActionParam = 1196,
        ActionExtraParam = 6304,
        Description = 1407,
        MoveUp = 6305,
        MoveDown = 6306,
        ActionParamDes = 1198,
        ActionExtraParamDes = 6303,
        Insert = 6302,
        SearchReference = 1999,
        DragPoint = 2001,
    };

    static void Create(CFinalSunDlg* pWnd);


    static HWND GetHandle()
    {
        return CNewScript::m_hwnd;
    }
    static bool OnEnterKeyDown(HWND& hWnd);
    static void OnSelchangeScript(bool edited = false, int specificIdx = -1);
    static void OnSelchangeActionType(bool edited = false);
    static void OnSelchangeActionParam(bool edited = false);
    static void OnSelchangeActionExtraParam(bool edited = false);
    static void OnSelchangeActionListbox();
    static void OnClickNewScript();

protected:
    static void Initialize(HWND& hWnd);
    static void Update(HWND& hWnd);

    static void OnSeldropdownScript(HWND& hWnd);
    static void OnClickDelScript(HWND& hWnd);
    static void OnClickCloScript(HWND& hWnd);
    static void OnClickAddAction(HWND& hWnd);
    static void OnClickCloneAction(HWND& hWnd);
    static void OnClickDeleteAction(HWND& hWnd);
    static void OnClickSearchReference(HWND& hWnd);
    static void OnClickMoveupAction(HWND& hWnd, bool reverse);
    static void UpdateActionAndParam(int actionChanged = -1, int listBoxCurChanged = -1, bool changeActionIdx = true);

    static void OnCloseupActionType();
    static void OnCloseupScript();
    static void OnCloseupActionParam();
    static void OnCloseupActionExtraParam();
    
    static void Close(HWND& hWnd);

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ListBoxSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static void ListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam);
    static LRESULT CALLBACK DragDotProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK DragingDotProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static CINI& map;
    static CINI& fadata;
    static MultimapHelper& rules;
public:
    static HWND hSelectedScript;
    static HWND hNewScript;
    static HWND hDelScript;
    static HWND hCloScript;
    static HWND hAddAction;
    static HWND hDeleteAction;
    static HWND hCloneAction;
    static HWND hName;
    static HWND hActionsListBox;
    static HWND hActionType;
    static HWND hActionParam;
    static HWND hActionExtraParam;
    static HWND hDescription;
    static HWND hMoveUp;
    static HWND hMoveDown;
    static HWND hActionParamDes;
    static HWND hActionExtraParamDes;
    static HWND hInsert;
    static HWND hSearchReference;
    static HWND hDragPoint;
    static FString CurrentScriptID;
    static std::map<FString, bool> ActionHasExtraParam;
    static std::map<FString, bool> ActionIsStringParam;
private:
    static int SelectedScriptIndex;
    static std::map<int, FString> ScriptLabels;
    static std::map<int, FString> ActionTypeLabels;
    static std::map<int, FString> ActionParamLabels;
    static std::map<int, FString> ActionExtraParamLabels;
    static bool Autodrop;
    static bool ParamAutodrop[2];
    static bool DropNeedUpdate;
    static bool bInsert;
    static WNDPROC OriginalListBoxProc;
    static WNDPROC OrigDragDotProc;
    static WNDPROC OrigDragingDotProc;
    static bool m_dragging;
    static POINT m_dragOffset;
    static HWND m_hDragGhost;
};

