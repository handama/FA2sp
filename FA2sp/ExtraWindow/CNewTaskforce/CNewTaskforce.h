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
class CNewTaskforce
{
public:
    enum Controls {
        SelectedTaskforce = 1144,
        NewTaskforce = 1151,
        DelTaskforce = 1150,
        CloTaskforce = 50807,
        AddUnit = 1146,
        DeleteUnit = 1147,
        Name = 1010,
        Group = 1122,
        UnitsListBox = 1145,
        Number = 1148,
        UnitType = 1149,
        SearchReference = 1999,
        DragPoint = 2001,
    };

    static void Create(CFinalSunDlg* pWnd);
    

    static HWND GetHandle()
    {
        return CNewTaskforce::m_hwnd;
    }
    static bool OnEnterKeyDown(HWND& hWnd);
    static void OnSelchangeTaskforce(bool edited = false, int specificIdx = -1);
    static void OnSelchangeUnitType(bool edited = false);
    static void OnClickNewTaskforce();

protected:
    static void Initialize(HWND& hWnd);
    static void Update(HWND& hWnd);

    static void OnSeldropdownTaskforce(HWND& hWnd);
    static void OnClickDelTaskforce(HWND& hWnd);
    static void OnClickCloTaskforce(HWND& hWnd);
    static void OnClickAddUnit(HWND& hWnd);
    static void OnClickDeleteUnit(HWND& hWnd);
    static void OnClickSearchReference(HWND& hWnd);
    static void OnEditchangeNumber();
    static void OnSelchangeUnitListbox();

    static void OnCloseupUnitType();
    static void OnCloseupTaskforce();

    static void Close(HWND& hWnd);

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static HRESULT CALLBACK ListBoxSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static void ListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam);
    static LRESULT CALLBACK DragDotProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK DragingDotProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static CINI& map;
    static MultimapHelper& rules;
public:
    static HWND hSelectedTaskforce;
    static HWND hNewTaskforce;
    static HWND hDelTaskforce;
    static HWND hCloTaskforce;
    static HWND hAddUnit;
    static HWND hDeleteUnit;
    static HWND hName;
    static HWND hGroup;
    static HWND hUnitsListBox;
    static HWND hNumber;
    static HWND hUnitType;
    static HWND hSearchReference;
    static HWND hDragPoint;
private:
    static int SelectedTaskForceIndex;
    static FString CurrentTaskForceID;
    static std::map<int, FString> TaskForceLabels;
    static std::map<int, FString> UnitTypeLabels;
    static bool Autodrop;
    static bool DropNeedUpdate;
    static WNDPROC OriginalListBoxProc;

    static WNDPROC OrigDragDotProc;
    static WNDPROC OrigDragingDotProc;
    static bool m_dragging;
    static POINT m_dragOffset;
    static HWND m_hDragGhost;
};