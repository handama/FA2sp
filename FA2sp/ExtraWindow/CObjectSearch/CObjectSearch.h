#pragma once

#include <FA2PP.h>
#include <CTileSetBrowserFrame.h>
#include <map>
#include <vector>
#include <string>
#include <regex>

// A static window class
class CObjectSearch
{
public:
    enum Controls { Input = 6209, Search = 6210, ListBox = 6211, Range = 6200, TreeView = 6201, 
        Map = 6202, TileSet = 6203, ListBoxButton = 6204, ExactMatch = 6208, PropertyBushFilter = 6212, 
        Waypoints = 6205, MapCoords = 6206, Trigger = 6214, AttachedTrigger = 6215, Event_Action = 6216,
        TriggerParam = 6217, ListBox_Range = 6213, TeamType = 6218, TaskForce = 6219, Script = 6220, Tag = 6221, AITrigger = 6222
    };
    enum FindType { Aircraft = 0, Infantry, Structure, Unit, Terrain };

    static void Create(CTileSetBrowserFrame* pWnd);

    static HWND GetHandle()
    {
        return CObjectSearch::m_hwnd;
    }

    static void MoveToMapCoord(int x, int y);

protected:
    static void Initialize(HWND& hWnd);
    static void Close(HWND& hWnd);

    static BOOL CALLBACK DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

    static void UpdateDetailsTreeView(HWND hWnd, TVITEMA hItem);
    static void UpdateDetailsTile(HWND hWnd, int index);
    static void UpdateDetailsWaypoint(HWND hWnd);

    static void OnSearchButtonUp(HWND hWnd);
    static void UpdateTypes(HWND hWnd);
    static void OnRangeChanged(HWND hWnd);
    static void OnListBoxRangeChanged(HWND hWnd);
    static void ListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam);
    static void SearchObjects(HWND hWnd, const char* source);
    static void SearchTriggers(HWND hWnd, const char* source);
    static void SearchAttachedTriggers(HWND hWnd, const char* source);
    static void SearchEvent_Action(HWND hWnd, const char* source);
    static void SearchTriggerParam(HWND hWnd, const char* source);
    static void SearchTeamType(HWND hWnd, const char* source);
    static void SearchTaskForce(HWND hWnd, const char* source);
    static void SearchScript(HWND hWnd, const char* source);
    static void SearchAITrigger(HWND hWnd, const char* source);
    static void SearchInfoTag(HWND hWnd, const char* source);
    static HTREEITEM FindLabel(HWND hWnd, HTREEITEM hItemParent, LPCSTR pszLabel);
    
    static void ToggleListBoxRangeVisibility(HWND hWnd, bool show);
    static void ToggleWindowSize(HWND hWnd);

    static bool IsLabelMatch(const char* target, const char* source);

private:
    static HWND m_hwnd;
    static CTileSetBrowserFrame* m_parent;
    static std::vector<std::pair<std::string, std::regex>> Nodes;
    static std::map<int, FString> Datas;
    static int ListBoxIndex;
    static std::vector<TVITEMA> ListBox_TreeView;
    static std::vector<int> ListBox_Tile;
    static std::vector<std::pair<int, int>> ListBox_MapCoord;
    static std::vector<FString> ListBoxTexts;
    static bool bExactMatch;
    static bool bTreeView;
    static bool bMap;
    static bool bTileSet;
    static bool bListBoxButton;
    static bool bWaypoints;
    static bool bMapCoords;
    static bool bPropertyBushFilter;
    static bool ToggleWindowSize_once;

    static bool bTrigger;
    static bool bAttachedTrigger;
    static bool bEvent_Action;
    static bool bTriggerParam;
    static bool bTeamType;
    static bool bTaskForce;
    static bool bScript;
    static bool bTag;
    static bool bAITrigger;

};

