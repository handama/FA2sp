#pragma once

#include "../Body.h"

#include <map>
#include <vector>
#include <unordered_set>
#include <CIsoView.h>


struct CachedEventType {
    bool param0IsWP = false; 
    bool param1IsWP = false; 
};

struct CachedActionType {
    bool paramsWP[6] = {};   
    bool param7IsWP = true;  
};

class WaypointSort
{
public:
    static WaypointSort Instance;

    WaypointSort() : m_hWnd{ NULL } {}

    enum class MenuItem : int
    {
        AddTrigger = 0x2000,
        Refresh = 0x3000
    };

    static void InitCache();

    void LoadAllTriggers();
    void Clear();
    BOOL OnNotify(LPNMTREEVIEW lpNmhdr);
    BOOL OnMessage(PMSG pMsg);
    void Create(HWND hParent);
    void OnSize() const;
    void ShowWindow(bool bShow) const;
    void ShowWindow() const;
    void HideWindow() const;
    void ShowMenu(POINT pt) const;
    bool IsValid() const;
    bool IsVisible() const;
    void DeleteTrigger(FString triggerId, HTREEITEM hItemParent = TVI_ROOT) const;
    void AddTrigger(FString triggerId, int x, int y) const;
    const FString& GetCurrentPrefix() const;
    HWND GetHwnd() const;
    operator HWND() const;

private:
    HTREEITEM FindLabel(HTREEITEM hItemParent, LPCSTR pszLabel) const;
    std::vector<FString> GetGroup(FString triggerId, FString& name) const;

    static int ProcessWaypointLetter(const char* s);

private:
    HWND m_hWnd;
    FString m_strPrefix;

    static FString sm_cachedEventsSection;
    static FString sm_cachedActionsSection;
    static FString sm_cachedParamTypesSection;
    static FString sm_cachedScriptsSection;
    static FString sm_cachedScriptParamsSection;
    static FHashMap<CachedEventType> sm_cachedEvents;
    static FHashMap<CachedActionType> sm_cachedActions;
    static FHashSet sm_cachedScriptActions;
    static std::unordered_set<int> sm_dontSaveAsWP;
    static bool sm_cacheInitialized;
};