#pragma once

#include <FA2PP.h>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Helpers/MultimapHelper.h"
#include "../../Helpers/FString.h"
#include "../Common.h"

class VirtualComboBoxEx;

// A static window class
class CNewTag
{
public:
    enum Controls {
        SelectedTag = 1002,
        NewTag = 1003,
        DelTag = 1005,
        CloTag = 1004,
        Name = 1007,
        Repeat = 1009,
        Trigger = 1011,
        SearchReference = 1999,
        DragPoint = 2001,
        TurnToTrigger = 1500,
    };

    static void Create(CFinalSunDlg* pWnd);
    
    static HWND GetHandle()
    {
        return CNewTag::m_hwnd;
    }
    static void OnSelchangeTag(bool edited = false, int specificIdx = -1);
    static void OnSelchangeTrigger(bool edited = false);
    static void OnSelchangeRepeat(bool edited = false);
    static void OnEditchangeName();
    static void OnClickNewTag();

protected:
    static void Initialize(HWND& hWnd);
    static void Update(bool updateTrigger = false, FString id = "");

    static void OnClickDelTag(HWND& hWnd);
    static void OnClickCloTag(HWND& hWnd);
    static void OnClickSearchReference(HWND& hWnd);
    static void OnClickTurnToTrigger(HWND& hWnd);

    static void Close(HWND& hWnd);

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK DragDotProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK DragingDotProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    static void SortTags(VirtualComboBoxEx& vcb, int& selectedIndex, FString id = "", bool clear = true);
    static void SortTriggers();
    static void OnDropdownTrigger();

private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static CINI& map;
    static MultimapHelper& rules;
public:
    static HWND hSelectedTag;
    static HWND hNewTag;
    static HWND hDelTag;
    static HWND hCloTag;
    static HWND hName;
    static HWND hRepeat;
    static HWND hTrigger;
    static HWND hTurnToTrigger;
    static HWND hSearchReference;
    static HWND hDragPoint;
    static bool TriggerListChanged;
    static FString CurrentTagID;
    static int SelectedTagIndex;
    static VirtualComboBoxEx vcbSelectedTag;
    static VirtualComboBoxEx vcbRepeat;
    static VirtualComboBoxEx vcbTrigger;
private:
    static WNDPROC OrigDragDotProc;
    static WNDPROC OrigDragingDotProc;
    static bool m_dragging;
    static POINT m_dragOffset;
    static HWND m_hDragGhost;
    static TargetHighlighter hl;
    static bool m_programmaticEdit;
    static bool m_disableRepeatSearch;
};