#pragma once
#include "FA2PP.h"
#include "../Helpers/MultimapHelper.h"

class FString;

class ExtraWindow
{
public:
    static FString GetTeamDisplayName(const char* id);
    static FString GetAITriggerDisplayName(const char* id);
    static FString FormatTriggerDisplayName(const char* id, const char* name);
    static FString GetEventDisplayName(const char* id, int index = -1);
    static FString GetActionDisplayName(const char* id, int index = -1);
    static void SetEditControlFontSize(HWND hWnd, float nFontSizeMultiplier, bool richEdit = false, const char* newFont = "");
    static int FindCBStringExactStart(HWND hComboBox, const char* searchText);
    static void SyncComboBoxContent(HWND hSource, HWND hTarget, bool addNone = false);
    static void AdjustDropdownWidth(HWND hWnd);
    static FString GetTriggerDisplayName(const char* id);
    static FString GetTriggerName(const char* id);
    static FString GetAITriggerName(const char* id);
    static FString GetTagName(const char* id);
    static FString GetTagDisplayName(const char* id);

    static void LoadParams(HWND& hWnd, FString idx);
    static void LoadParam_Waypoints(HWND& hWnd);
    static void LoadParam_ActionList(HWND& hWnd);
    static void LoadParam_CountryList(HWND& hWnd);
    static void LoadParam_HouseAddon_Multi(HWND& hWnd);
    static void LoadParam_HouseAddon_MultiAres(HWND& hWnd);
    static void LoadParam_TechnoTypes(HWND& hWnd, int specificType = -1, int style = 0, bool sort = true);
    static void LoadParam_Triggers(HWND& hWnd);
    static void LoadParam_Tags(HWND& hWnd);
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

private:
    static CINI& map;
    static CINI& fadata;
    static MultimapHelper& rules;
};



