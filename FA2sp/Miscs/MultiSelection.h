#pragma once

#include <CMapData.h>
#include "..\Ext\CMapData\Body.h"
#include <set>
#include <concepts>
#include "../ExtraWindow/CMultiSelectionOptionDlg/CMultiSelectionOptionDlg.h"
#include "CopyPaste.h"

class MultiSelection
{
public:
    static bool AddCoord(int X, int Y);
    static bool RemoveCoord(int X, int Y);
    static size_t GetCount();
    static void Clear();
    static void ReverseStatus(int X, int Y);
    static void InvertSelection();
    static bool IsSelected(int X, int Y);
    static void FindConnectedTiles(std::unordered_set<int>& process, int startX, int startY,
        std::unordered_set<int>& tileSet, bool firstRun);

    static CMultiSelectionOptionDlg dlg;

    template<typename _Fn> requires std::invocable<_Fn, CellData&, CellDataExt&>
    static void ApplyForEach(_Fn _Func)
    {
        for (auto& coord : SelectedCoords)
        {
            auto pos = CMapData::Instance->GetCoordIndex(coord.X, coord.Y);
            auto pCell = CMapData::Instance->GetCellAt(pos);
            auto& cellExt = CMapDataExt::CellDataExts[pos];
            _Func(*pCell, cellExt);
        }
    }

    static bool IsSquareSelecting;
    static bool Control_D_IsDown;
    static BGRStruct ColorHolder[0x1000];
    static std::set<MapCoord> SelectedCoords;
    static MapCoord LastAddedCoord;

    static bool AddBuildingOptimize;
};

