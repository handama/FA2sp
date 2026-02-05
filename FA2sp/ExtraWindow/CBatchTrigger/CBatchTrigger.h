#pragma once

#include <FA2PP.h>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../CNewScript/CNewScript.h"
#include "../../Helpers/FString.h"

class Trigger;
class HelpDlg;

struct CellColor {
    int row;
    int col;
    COLORREF fontColor;
};

enum class ObjType
{
    Trigger,
    Tag,
    Team,
    None
};

struct ObjInfo
{
    FString id;
    FString name;
    ObjType type;
};

// A static window class
class CBatchTrigger
{
public:
    enum Lists
    {
        ID = 0,
        Name,
        EventNum,
        EventParam1,
        EventParam2,
        ActionNum,
        ActionParam1,
        ActionParam2,
        ActionParam3,
        ActionParam4,
        ActionParam5,
        ActionParam6,
        Count,
    };

    enum Controls { 
        Listbox = 1000, 
        ListView = 1001,
        EventIndex = 1003,
        ActionIndex = 1005,
        Add = 1006,
        Delete = 1007,
        AutoFill = 1008,
        UseID = 1009,
        Search = 1011,
        Help = 1012,
        MoveUp = 1015,
        MoveDown = 1016,
        DisplayID = 1017,
        ClearHighlight = 1018,
    };
    static void Create(CFinalSunDlg* pWnd);

    static HWND GetHandle()
    {
        return CBatchTrigger::m_hwnd;
    }
    static void RefreshTrigger(int index);
    static void OnDroppedIntoCell(int row, int col, const FString& value);

protected:
    static void Initialize(HWND& hWnd);
    static void Close(HWND& hWnd);
    static void Update(bool afterInit = false, bool updateTrigger = true);

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static void ListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam);
    static LRESULT CALLBACK ListBoxSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void OnDbClickListbox();
    static void OnClickAutoFill();
    static void OnSearchEditChanged();
    static void OnClickUseID();
    static void OnClickDisplayID();
    static void UpdateListBox();
    static void OnClickMove(bool isUp);
    static void OnClickClearHighlight();
    static void OnClickHelp();
    static void OnViewerSelectedChange(LPNMHDR pNMHDR);
    static void AddTrigger(const FString& ID);
    static void InsertTrigger(int index, std::shared_ptr<Trigger> trigger);
    static bool DeleteTrigger(int index);
    static void SaveTrigger(int row, int col, bool& changed);
    static int GetTriggerIndex(const FString& ID);
    static std::vector<int> GetListBoxSelected();
    static void AdjustColumnWidth();
    static void SelectListViewRows(const std::vector<int>& indices);

    static LRESULT CALLBACK InplaceEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static bool GetCellRect(HWND hListView, int row, int col, RECT& outRect);
    static HWND CreateInplaceEdit(HWND hListView, int row, int col, const char* initialText);
    static void EndInplaceEdit(bool bSave);
    static void SetFontColor(HWND hListView, int row, int col, COLORREF color);
    static void AddObject(const FString& id, const FString& name, ObjType type);
    static FString IdToName(const FString& id);
    static FString NameToId(const FString& name, ObjType forceType);

private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static CINI& map;
    static MultimapHelper& rules;

    static HWND hListbox;
    static HWND hListView;
    static HWND hEventIndex;
    static HWND hActionIndex;
    static HWND hAdd;
    static HWND hDelete;
    static HWND hAutoFill;
    static HWND hUseID;
    static HWND hSearch;
    static HWND hMoveUp;
    static HWND hMoveDown;
    static HWND hDisplayID;
    static HWND hClearHighlight;
    static HWND hHelp;
    static HWND g_hInplaceEdit;
    static int g_nEditRow;
    static int g_nEditCol;
    static int origWndWidth;
    static int origWndHeight;
    static int minWndWidth;
    static int minWndHeight;
    static bool minSizeSet; 
    static WNDPROC OriginalListBoxProc;
    static WNDPROC g_pOriginalListViewProc;
    static std::list<FString> ListboxTriggerID;
    static int CurrentEventIndex;
    static int CurrentActionIndex;
    static std::vector<CellColor> g_specialCells;
    static FString g_nEditOri;
    static bool IsUpdating;
    static bool bUseID;
    static bool bDisplayID;

    static std::vector<ObjInfo> objects;
    static std::unordered_map<FString, ObjInfo*> idIndex;
    static std::unordered_map<FString, ObjInfo*> triggerNameIndex;
    static std::unordered_map<FString, ObjInfo*> tagNameIndex;
    static std::unordered_map<FString, ObjInfo*> teamNameIndex;
    static HelpDlg hdHelp;

public:
    static std::vector<FString> ListedTriggerIDs;
    static bool NeedClear;
};

