#pragma once

#include "../Body.h"

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

class TeamSort
{
public:
    static TeamSort Instance;

    TeamSort() : m_hWnd{ NULL } {}

    enum class MenuItem : int
    {
        AddTrigger = 0x1002,
        Refresh = 0x3000
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
    static bool CreateFromTeamSort;

private:
    HTREEITEM FindLabel(HTREEITEM hItemParent, LPCSTR pszLabel) const;
    std::vector<FString> GetGroup(FString triggerId, FString& name) const;
    void AddTrigger(std::vector<FString> group, FString name, FString id) const;

    static std::string MakeLabelKey(HTREEITEM hParent, LPCSTR pszLabel);
    void IndexAdd(HTREEITEM hParent, LPCSTR pszLabel, HTREEITEM hItem) const;
    void IndexRemove(HTREEITEM hParent, LPCSTR pszLabel) const;
    void IndexClear() const;

private:
    HWND m_hWnd;
    FString m_strPrefix;
    mutable std::unordered_map<std::string, HTREEITEM> m_labelIndex;
};