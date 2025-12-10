#pragma once

#include <CTileSetBrowserFrame.h>
#include "../FA2Expand.h"
#include "../../ExtraWindow/CTerrainGenerator/CTerrainGenerator.h"
#include "../../FA2sp/Helpers/FString.h"

class NOVTABLE CTileSetBrowserFrameExt : public CTileSetBrowserFrame
{
public:

	static void ProgramStartupInit();

	enum class TabPage : int
	{
		TilesetBrowser = 0,
		TriggerSort = 1,

		TagSort = 2,
		TeamSort = 3,
		TaskforceSort = 4,
		ScriptSort = 5,
		WaypointSort = 6,
	};

	//
	// Ext Functions
	//

	BOOL OnInitDialogExt();
	BOOL PreTranslateMessageExt(MSG* pMsg);
	BOOL OnNotifyExt(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	BOOL OnCommandExt(WPARAM wParam, LPARAM lParam);

	CTileSetBrowserFrameExt() {};
	~CTileSetBrowserFrameExt() {};

	// Functional Functions
	void OnBNTileManagerClicked();
	void OnBNSearchClicked();
	void OnBNTerrainGeneratorClicked();

	void InitTabControl();

private:

public:
	static CTerrainGenerator m_terrainGenerator;
	static HWND hTabCtrl;
    static bool TerrainDlgLoaded;
};

class TreeViewHelper {
public:
    struct TreeItemData {
        FString label;
        FString param;
        TreeItemData(const FString& l, const FString& p) : label(l), param(p) {}
    };

private:
    static inline std::unordered_map<HWND, std::unordered_map<HTREEITEM, std::unique_ptr<TreeItemData>>> storage;

public:
    static HTREEITEM InsertTreeItem(HWND hwndTree, const FString& label, const FString& param = "", HTREEITEM hParent = TVI_ROOT) {
        auto data = std::make_unique<TreeItemData>(label, param);

        TVINSERTSTRUCT tvis{};
        tvis.hParent = hParent;
        tvis.hInsertAfter = TVI_SORT;
        tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
        tvis.item.pszText = const_cast<LPSTR>(data->label.c_str());
        tvis.item.lParam = reinterpret_cast<LPARAM>(data.get());

        HTREEITEM hItem = TreeView_InsertItem(hwndTree, &tvis);
        if (hItem) {
            storage[hwndTree][hItem] = std::move(data);
        }
        return hItem;
    }

    static bool UpdateTreeItem(HWND hwndTree, HTREEITEM hItem, const FString& newLabel, const FString& newParam) {
        auto* data = GetTreeItemData(hwndTree, hItem);
        if (!data) return false;

        data->label = newLabel;
        data->param = newParam;

        TVITEM tvi{};
        tvi.mask = TVIF_TEXT | TVIF_PARAM;
        tvi.hItem = hItem;
        tvi.pszText = const_cast<LPSTR>(data->label.c_str());
        tvi.lParam = reinterpret_cast<LPARAM>(data);

        return TreeView_SetItem(hwndTree, &tvi) != FALSE;
    }

    static TreeItemData* GetTreeItemData(HWND hwndTree, HTREEITEM hItem) {
        auto itWnd = storage.find(hwndTree);
        if (itWnd == storage.end()) return nullptr;
        auto itItem = itWnd->second.find(hItem);
        if (itItem != itWnd->second.end()) {
            return itItem->second.get();
        }
        return nullptr;
    }

    static void DeleteTreeItem(HWND hwndTree, HTREEITEM hItem) {
        if (hItem) {
            TreeView_DeleteItem(hwndTree, hItem);
            auto itWnd = storage.find(hwndTree);
            if (itWnd != storage.end()) {
                itWnd->second.erase(hItem);
            }
        }
    }

    static void ClearTreeView(HWND hwndTree) {
        TreeView_DeleteAllItems(hwndTree);
        storage.erase(hwndTree);
    }
};