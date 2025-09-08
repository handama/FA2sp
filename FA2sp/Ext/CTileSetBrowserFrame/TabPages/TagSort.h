#pragma once

#include "../Body.h"

#include <unordered_map>
#include <vector>
#include <unordered_set>

class TagSort
{
public:
    static TagSort Instance;

    TagSort() : m_hWnd{ NULL } {}

    enum class MenuItem : int
    {
        AddTrigger = 0x1001,
        Refresh = 0x2000
    };

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
    void Menu_AddTrigger();
    void DeleteTrigger(FString triggerId, HTREEITEM hItemParent = TVI_ROOT) const;
    void AddTrigger(FString triggerId) const;
    const FString& GetCurrentPrefix() const;
    HWND GetHwnd() const;
    operator HWND() const;

    static std::unordered_set<FString> attachedTriggers;
    static std::unordered_map<FString, std::vector<FString>>BuildingTags;
    static std::unordered_map<FString, std::vector<FString>>AircraftTags;
    static std::unordered_map<FString, std::vector<FString>>UnitTags;
    static std::unordered_map<FString, std::vector<FString>>InfantryTags;
    static std::unordered_map<FString, FString>TagTriggers;
    static std::unordered_map<FString, FString>TriggerTags;
    static std::unordered_map<FString, std::vector<FString>>TriggerTagsParent;
    static std::unordered_map<FString, std::vector<FString>>CellTagTags;
    static std::unordered_map<FString, std::vector<FString>>TeamTags;

private:
    HTREEITEM FindLabel(HTREEITEM hItemParent, LPCSTR pszLabel) const;
    std::vector<FString> GetGroup(FString triggerId, FString& name) const;
    void AddTrigger(std::vector<FString> group, FString name, FString id) const;
    void AddAttachedTrigger(HTREEITEM hParent, FString triggerID, FString parentName) const;
    void AddAttachedTriggerReverse(HTREEITEM hParent, FString triggerID, FString parentName) const;

private:
    HWND m_hWnd;
    FString m_strPrefix;
};