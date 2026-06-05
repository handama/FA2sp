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
    void AddTrigger(FString triggerId) const;
    const FString& GetCurrentPrefix() const;
    HWND GetHwnd() const;
    operator HWND() const;

    static FHashSet attachedTriggers;
    static FHashMap<std::vector<FString>> BuildingTags;
    static FHashMap<std::vector<FString>> AircraftTags;
    static FHashMap<std::vector<FString>> UnitTags;
    static FHashMap<std::vector<FString>> InfantryTags;
    static FHashMap<FString> TagTriggers;  
    static FHashMap<FString> TriggerTags;  
    static FHashMap<std::vector<FString>> TriggerTagsParent;
    static FHashMap<std::vector<FString>> CellTagTags;
    static FHashMap<std::vector<FString>> TeamTags;

    static bool CreateFromTagSort;

private:
    HTREEITEM FindLabel(HTREEITEM hItemParent, LPCSTR pszLabel) const;
    std::vector<FString> GetGroup(FString triggerId, FString& name) const;
    void AddTrigger(std::vector<FString> group, FString name, FString id) const;
    void AddAttachedTrigger(HTREEITEM hParent, FString triggerID, FString parentName) const;
    void AddAttachedTriggerReverse(HTREEITEM hParent, FString triggerID, FString parentName) const;

    // 标签索引缓存：将 FindLabel 的 O(n) 树遍历转为 O(1) 哈希查找
    static std::string MakeLabelKey(HTREEITEM hParent, LPCSTR pszLabel);
    void IndexAdd(HTREEITEM hParent, LPCSTR pszLabel, HTREEITEM hItem) const;
    void IndexRemove(HTREEITEM hParent, LPCSTR pszLabel) const;
    void IndexClear() const;

private:
    HWND m_hWnd;
    FString m_strPrefix;
    mutable std::unordered_map<std::string, HTREEITEM> m_labelIndex;
};