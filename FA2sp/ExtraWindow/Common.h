#pragma once
#include "FA2PP.h"
#include "../Helpers/MultimapHelper.h"

class FString;
class CNewTrigger;

enum class DropType : int
{
    AttachedTrigger = 0,
    ActionParam0,
    ActionParam1,
    ActionParam2,
    ActionParam3,
    ActionParam4,
    ActionParam5,
    EventParam0,
    EventParam1,
    TeamEditorTag,
    TeamEditorTaskForce,
    TeamEditorScript,
    AIEditorTeam0,
    AIEditorTeam1,
    BatchTriggerListView,

    Unknown,
};

struct DropTarget
{
    HWND hWnd;
    RECT screenRect;
    DropType type;
    CNewTrigger* triggerInstance;
    HWND hRoot;
};

struct ListViewHitResult
{
    int item = -1;
    int subItem = -1;
    UINT flags = 0;
};

class ExtraWindow
{
public:
    static FString GetTeamDisplayName(const char* id);
    static FString GetAITriggerDisplayName(const char* id);
    static FString FormatTriggerDisplayName(const char* id, const char* name);
    static FString GetEventDisplayName(const char* id, int index = -1, bool addIndex = true);
    static FString GetActionDisplayName(const char* id, int index = -1, bool addIndex = true);
    static void SetEditControlFontSize(HWND hWnd, float nFontSizeMultiplier, bool richEdit = false, const char* newFont = "");
    static int FindCBStringExactStart(HWND hComboBox, const char* searchText);
    static void SyncComboBoxContent(HWND hSource, HWND hTarget, bool addNone = false);
    static void AdjustDropdownWidth(HWND hWnd);
    static FString GetTriggerDisplayName(const char* id);
    static FString GetTriggerName(const char* id);
    static FString GetAITriggerName(const char* id);
    static FString GetTagName(const char* id);
    static FString GetTagDisplayName(const char* id);
    static FString GetTranslatedSectionName(const char* section);

    static void LoadParams(HWND& hWnd, FString idx, CNewTrigger* instance = nullptr);
    static void LoadParam_Waypoints(HWND& hWnd);
    static void LoadParam_ActionList(HWND& hWnd);
    static void LoadParam_CountryList(HWND& hWnd);
    static void LoadParam_HouseAddon_Multi(HWND& hWnd);
    static void LoadParam_HouseAddon_MultiAres(HWND& hWnd);
    static void LoadParam_TechnoTypes(HWND& hWnd, int specificType = -1, int style = 0, bool sort = true);
    static void LoadParam_Triggers(HWND& hWnd, CNewTrigger* instance);
    static void LoadParam_Tags(HWND& hWnd);
    static void LoadParam_Teamtypes(HWND& hWnd);
    static void LoadParam_Stringtables(HWND& hWnd);

    static bool bComboLBoxSelected;
    static bool bEnterSearch; 
    // true means click inside combobox
    static bool OnCloseupCComboBox(HWND& hWnd, std::map<int, FString>& labels, bool isComboboxSelectOnly = false);
    static void OnEditCComboBox(HWND& hWnd, std::map<int, FString>& labels);

    static bool SortLabels(FString a, FString b);
    static bool SortRawStrings(std::string sa, std::string sb);
    static void SortTeams(HWND& hWnd, FString section, int& selectedIndex, FString id = "");
    static void SortAITriggers(HWND& hWnd, int& selectedIndex, FString id = "");
    static bool IsLabelMatch(const char* target, const char* source, bool exactMatch = false);
    static FString GetCloneName(FString oriName);
    static void LoadFrom(MultimapHelper& mmh, FString loadfrom);
    static void TrimStringIndex(FString& str);

    static void RegisterDropTarget(HWND hWnd, DropType type, CNewTrigger* trigger = nullptr);
    static void UpdateDropTargetRect(HWND hWnd);
    static DropTarget FindDropTarget(POINT screenPt);
    static void UpdateHoverTarget(POINT pt);
    static void UnregisterDropTarget(HWND hWnd);
    static void UnregisterDropTargetsOfWindow(HWND hMainWnd);
    static bool IsPointOnIsoViewAndNotCovered(POINT ptScreen);
    static FString GetScintillaText(HWND hScintilla);
    static void SetScintillaText(HWND hScintilla, FString& text);
    static bool HitTestListView(HWND hListView, POINT ptScreen, ListViewHitResult& out);

    static std::vector<DropTarget> g_DropTargets;

private:
    static CINI& map;
    static CINI& fadata;
    static MultimapHelper& rules;
};



