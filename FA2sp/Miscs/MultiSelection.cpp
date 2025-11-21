#include "MultiSelection.h"

#include <CFinalSunDlg.h>
#include <CIsoView.h>
#include <Helpers/Macro.h>
#include <Drawing.h>
#include <CTileTypeClass.h>
#include <CLoading.h>

#include "../Ext/CIsoView/Body.h"
#include "../Ext/CMapData/Body.h"

#include <algorithm>
#include <span>

// #include "../Source/CIsoView.h"

#include "../FA2sp.h"
#include "../Ext/CFinalSunDlg/Body.h"
#include <Miscs/Miscs.h>
#include "../Helpers/STDHelpers.h"
#include "../Helpers/Translations.h"
#include "Palettes.h"

std::set<MapCoord> MultiSelection::SelectedCoords;
MapCoord MultiSelection::LastAddedCoord;
bool MultiSelection::IsSquareSelecting = false;
bool MultiSelection::Control_D_IsDown = false;
BGRStruct MultiSelection::ColorHolder[0x1000];
bool MultiSelection::AddBuildingOptimize = false;
CMultiSelectionOptionDlg MultiSelection::dlg;

bool MultiSelection::AddCoord(int X, int Y)
{
    if (X < 0 || Y < 0 || X > CMapData::Instance().MapWidthPlusHeight || Y > CMapData::Instance().MapWidthPlusHeight)
        return false;

    if (X == -1 || Y == -1)
        return false;

    MapCoord coords{ X,Y };
    auto itr = SelectedCoords.find(coords);
    if (itr == SelectedCoords.end())
    {
        SelectedCoords.insert(itr, coords);
        return true;
    }

    return false;
}

bool MultiSelection::RemoveCoord(int X, int Y)
{
    if (X < 0 || Y < 0 || X > CMapData::Instance().MapWidthPlusHeight || Y > CMapData::Instance().MapWidthPlusHeight)
        return false;

    if (X == -1 || Y == -1)
        return false;

    MapCoord coords{ X,Y };
    auto itr = SelectedCoords.find(coords);
    if (itr != SelectedCoords.end())
    {
        SelectedCoords.erase(itr);
        return true;
    }

    return false;
}

size_t MultiSelection::GetCount()
{
    return SelectedCoords.size();
}

void MultiSelection::Clear()
{
    SelectedCoords.clear();

    MultiSelection::LastAddedCoord.X = -1;
    MultiSelection::LastAddedCoord.Y = -1;
}

void MultiSelection::ReverseStatus(int X, int Y)
{
    MapCoord mapCoord{ X,Y };
    auto itr = SelectedCoords.find(mapCoord);
    if (itr == SelectedCoords.end())
    {
        AddCoord(X, Y);
    }
    else
    {
        RemoveCoord(X, Y);
    }
}

bool MultiSelection::IsSelected(int X, int Y)
{
    return SelectedCoords.find(MapCoord{ X,Y }) != SelectedCoords.end();
}

void MultiSelection::FindConnectedTiles(std::unordered_set<int>& process, int startX, int startY,
    std::unordered_set<int>& tileSet, bool firstRun)
{
    const auto cell = CMapData::Instance->GetCellAt(startX, startY);
    int ground = CMapDataExt::GetSafeTileIndex(cell->TileIndex);

    if (firstRun)
    {
        if (ground >= CMapDataExt::TileDataCount)
            return;
        tileSet.insert(CMapDataExt::TileData[ground].TileSet);
        if (dlg.ConsiderLAT)
        {
            for (int latidx = 0; latidx < CMapDataExt::Tile_to_lat.size(); ++latidx)
            {
                int& iSmoothSet = CMapDataExt::Tile_to_lat[latidx][0];
                int& iLatSet = CMapDataExt::Tile_to_lat[latidx][1];
                if (iSmoothSet == *tileSet.begin())
                {
                    tileSet.insert(iLatSet);
                }
                else if (iLatSet == *tileSet.begin())
                {
                    tileSet.insert(iSmoothSet);
                }
            }
        }
        if (!dlg.Connected4 && !dlg.Connected8)
        {
            for (int j = 0; j < CMapData::Instance->CellDataCount; j++) {
                int x = CMapData::Instance->GetXFromCoordIndex(j);
                int y = CMapData::Instance->GetYFromCoordIndex(j);
                if (!CMapData::Instance->IsCoordInMap(x, y))
                    continue;
                auto scCell = CMapData::Instance->GetCellAt(x, y);
                int scGround = CMapDataExt::GetSafeTileIndex(scCell->TileIndex);
                if (scGround >= CMapDataExt::TileDataCount)
                    continue;
                if (dlg.SameTileSet && tileSet.find(CMapDataExt::TileData[scGround].TileSet) == tileSet.end())
                    continue;
                if (dlg.SameHeight && scCell->Height != cell->Height)
                    continue;
                if (dlg.SameBaiscHeight && cell->Height - CMapDataExt::TileData[ground].TileBlockDatas[cell->TileSubIndex].Height
                    != scCell->Height - CMapDataExt::TileData[scGround].TileBlockDatas[scCell->TileSubIndex].Height)
                    continue;
                process.insert(j);
            }
            return;
        }
    }

    const int loop4[5][2] = { {0, 0},{0, -1},{0, 1},{1, 0},{-1, 0} };
    const int loop8[9][2] = { {0, 0},{0, -1},{0, 1},{1, 0},{-1, 0},{-1, -1},{1, 1},{1, -1},{-1, 1} };
    auto loop = [&](const int* pair)
    {
        int newX = pair[0] + startX;
        int newY = pair[1] + startY;
        if (!CMapData::Instance->IsCoordInMap(newX, newY)) return;
        int pos = newX + newY * CMapData::Instance->MapWidthPlusHeight;
        if (process.find(pos) != process.end())
            return;
        auto scCell = CMapData::Instance->GetCellAt(pos);
        int scGround = CMapDataExt::GetSafeTileIndex(scCell->TileIndex);
        if (scGround >= CMapDataExt::TileDataCount)
            return;
        if (dlg.SameTileSet && tileSet.find(CMapDataExt::TileData[scGround].TileSet) == tileSet.end())
            return;
        if (dlg.SameHeight && scCell->Height != cell->Height)
            return;
        if (dlg.SameBaiscHeight && cell->Height - CMapDataExt::TileData[ground].TileBlockDatas[cell->TileSubIndex].Height
            != scCell->Height - CMapDataExt::TileData[scGround].TileBlockDatas[scCell->TileSubIndex].Height)
            return;

        process.insert(pos);
        FindConnectedTiles(process, newX, newY, tileSet, false);
    };
    if (dlg.Connected8)
    {
        for (auto pair : loop8)
            loop(pair);
    }
    else
    {
        for (auto pair : loop4)
            loop(pair);
    }
}

DEFINE_HOOK(456EFC, CIsoView_OnMouseMove_MultiSelect_SelectStatus, 6)
{
    if (!ExtConfigs::EnableMultiSelection)
        return 0;

    GET_STACK(UINT, eFlags, STACK_OFFS(0x3D528, -0x4));
    REF_STACK(const CPoint, point, STACK_OFFS(0x3D528, -0x8));

    if (CIsoView::CurrentCommand->Command == 0x1D && (eFlags & MK_LBUTTON))
    {
        {
            auto coord = CIsoView::GetInstance()->GetCurrentMapCoord(point);
            if (CIsoView::CurrentCommand->Type == 0)
                if (MultiSelection::AddCoord(coord.X, coord.Y))
                {
                    CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
                }
            if (CIsoView::CurrentCommand->Type == 1)
                if (MultiSelection::RemoveCoord(coord.X, coord.Y))
                {
                    CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
                }
            if (CIsoView::CurrentCommand->Type == 2)
            {
                MultiSelection::Clear();
                CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            }
            if (CIsoView::CurrentCommand->Type == 7)
            {
                std::unordered_set<int> coords;
                std::unordered_set<int> tilesets;
                MultiSelection::FindConnectedTiles(coords, coord.X, coord.Y, tilesets, true);
                for (auto& coord : coords)
                {
                    int x = CMapData::Instance->GetXFromCoordIndex(coord);
                    int y = CMapData::Instance->GetYFromCoordIndex(coord);
                    MultiSelection::AddCoord(x, y);
                }
                CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            }
            if (CIsoView::CurrentCommand->Type == 8)
            {
                std::unordered_set<int> coords;
                std::unordered_set<int> tilesets;
                MultiSelection::FindConnectedTiles(coords, coord.X, coord.Y, tilesets, true);
                for (auto& coord : coords)
                {
                    int x = CMapData::Instance->GetXFromCoordIndex(coord);
                    int y = CMapData::Instance->GetYFromCoordIndex(coord);
                    MultiSelection::RemoveCoord(x, y);
                }
                CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            }               
            return 0x456EB6;
        }
    }

    return 0;
}

DEFINE_HOOK(469470, CIsoView_OnKeyDown, 5)
{
    if (!ExtConfigs::EnableMultiSelection)
        return 0;

    GET(CIsoView*, pThis, ECX);
    GET_STACK(UINT, nChar, 0x4);

    if (nChar == 'D')
    {
        if (MultiSelection::Control_D_IsDown)
            MultiSelection::Control_D_IsDown = false;
		else
            CFinalSunApp::Instance->FlatToGround = !CFinalSunApp::Instance->FlatToGround;
        pThis->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }
    else if (nChar == 'A')
        pThis->KeyboardAMode = !pThis->KeyboardAMode;

    R->EAX(pThis->Default());

    return 0x4694A9;
}

DEFINE_HOOK(433DA0, CFinalSunDlg_Tools_RaiseSingleTile, 5)
{
    if (!ExtConfigs::EnableMultiSelection)
        return 0;

    GET(CFinalSunDlg*, pThis, ECX);

    if (CMapData::Instance->MapWidthPlusHeight)
    {
        if (MultiSelection::GetCount())
        {
            RECT bounds
            {
                std::numeric_limits<LONG>::max(),
                std::numeric_limits<LONG>::max(),
                std::numeric_limits<LONG>::min(),
                std::numeric_limits<LONG>::min()
            };
            for (const auto& cell : MultiSelection::SelectedCoords)
            {
                if (cell.X < bounds.left)
                    bounds.left = cell.X;
                if (cell.X > bounds.right)
                    bounds.right = cell.X;
                if (cell.Y < bounds.top)
                    bounds.top = cell.Y;
                if (cell.Y > bounds.bottom)
                    bounds.bottom = cell.Y;
            }
            CMapData::Instance->SaveUndoRedoData(true, bounds.left, bounds.top, bounds.right + 1, bounds.bottom + 1);

            MultiSelection::ApplyForEach(
                [](CellData& cell, CellDataExt& cellExt) {
                    if (cell.Height < 14)
                        ++cell.Height;
                }
            );
            pThis->MyViewFrame.pIsoView->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        }
        else
        {
            if (CIsoView::CurrentCommand->Command != FACurrentCommand::RaiseSingleTile && !ExtConfigs::SkipBrushSizeChangeOnTools)
            {
                pThis->BrushSize.nCurSel = 0;
                pThis->BrushSize.UpdateData(FALSE);
                pThis->MyViewFrame.pIsoView->BrushSizeX = 1;
                pThis->MyViewFrame.pIsoView->BrushSizeY = 1;
            }
            pThis->PlaySound(CFinalSunDlg::FASoundType::Normal);
            CIsoView::CurrentCommand->Command = FACurrentCommand::RaiseSingleTile;
        }
    }
    else
        pThis->PlaySound(CFinalSunDlg::FASoundType::Error);

    return 0x433DB7;
}

DEFINE_HOOK(433D30, CFinalSunDlg_Tools_LowerSingleTile, 5)
{
    if (!ExtConfigs::EnableMultiSelection)
        return 0;

    GET(CFinalSunDlg*, pThis, ECX);

    if (CMapData::Instance->MapWidthPlusHeight)
    {
        if (MultiSelection::GetCount())
        {
            RECT bounds
            {
                std::numeric_limits<LONG>::max(),
                std::numeric_limits<LONG>::max(),
                std::numeric_limits<LONG>::min(),
                std::numeric_limits<LONG>::min()
            };
            for (const auto& cell : MultiSelection::SelectedCoords)
            {
                if (cell.X < bounds.left)
                    bounds.left = cell.X;
                if (cell.X > bounds.right)
                    bounds.right = cell.X;
                if (cell.Y < bounds.top)
                    bounds.top = cell.Y;
                if (cell.Y > bounds.bottom)
                    bounds.bottom = cell.Y;
            }
            CMapData::Instance->SaveUndoRedoData(true, bounds.left, bounds.top, bounds.right + 1, bounds.bottom + 1);

            MultiSelection::ApplyForEach(
                [](CellData& cell, CellDataExt& cellExt) {
                    if (cell.Height > 0)
                        --cell.Height;
                }
            );
            pThis->MyViewFrame.pIsoView->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        }
        else
        {
            if (CIsoView::CurrentCommand->Command != FACurrentCommand::LowerSingleTile && !ExtConfigs::SkipBrushSizeChangeOnTools)
            {
                pThis->BrushSize.nCurSel = 0;
                pThis->BrushSize.UpdateData(FALSE);
                pThis->MyViewFrame.pIsoView->BrushSizeX = 1;
                pThis->MyViewFrame.pIsoView->BrushSizeY = 1;
            }
            pThis->PlaySound(CFinalSunDlg::FASoundType::Normal);
            CIsoView::CurrentCommand->Command = FACurrentCommand::LowerSingleTile;
        }
    }
    else
        pThis->PlaySound(CFinalSunDlg::FASoundType::Error);

    return 0x433D47;
}

DEFINE_HOOK(433F70, CFinalSunDlg_Tools_HideSingleField, 5)
{
    if (!ExtConfigs::EnableMultiSelection)
        return 0;

    GET(CFinalSunDlg*, pThis, ECX);

    if (CMapData::Instance->MapWidthPlusHeight)
    {
        if (MultiSelection::GetCount())
        {
            MultiSelection::ApplyForEach(
                [](CellData& cell, CellDataExt& cellExt) {
                    cell.Flag.IsHiddenCell = true;
                }
            );
            pThis->MyViewFrame.pIsoView->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        }
        else
            CIsoView::CurrentCommand->Command = FACurrentCommand::HideSingleField;

        pThis->PlaySound(CFinalSunDlg::FASoundType::Normal);
    }
    else
        pThis->PlaySound(CFinalSunDlg::FASoundType::Error);

    return 0x433F83;
}

DEFINE_HOOK(4B9F7A, CreateMap_ClearUp_MultiSelection, 5)
{
    if (!ExtConfigs::EnableMultiSelection)
        return 0;

    MultiSelection::Clear();

    return 0;
}

DEFINE_HOOK(49D2C0, LoadMap_ClearUp_MultiSelection, 5)
{
    if (!ExtConfigs::EnableMultiSelection)
        return 0;

    MultiSelection::Clear();

    return 0;
}
