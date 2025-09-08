#pragma once

#include "../Body.h"

#include <map>
#include <vector>
#include <CIsoView.h>

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

private:
    HWND m_hWnd;
    FString m_strPrefix;
};