#include "Body.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Miscs/TheaterInfo.h"
#include "../../FA2sp.h"
#include <CINI.h>
#include <CMapData.h>
#include <CIsoView.h>
#include <CTileTypeClass.h>
#include <CLoading.h>
#include "../CTileSetBrowserFrame/Body.h"
#include "../CMapData/Body.h"
#include "../../Miscs/MultiSelection.h"

std::unordered_map<int, ConnectedTileInfo> CViewObjectsExt::TreeView_ConnectedTileMap;
std::vector<ConnectedTileSet> CViewObjectsExt::ConnectedTileSets;
int CViewObjectsExt::CurrentConnectedTileType;

MapCoord CViewObjectsExt::CliffConnectionCoord;
std::vector<MapCoord> CViewObjectsExt::CliffConnectionCoordRecords;
int CViewObjectsExt::CliffConnectionTile;
int CViewObjectsExt::CliffConnectionHeight;
int CViewObjectsExt::CliffConnectionHeightAdjust;
ConnectedTiles CViewObjectsExt::LastPlacedCT;
int CViewObjectsExt::LastTempPlacedCTIndex;
int CViewObjectsExt::LastTempFacing;
std::vector<ConnectedTiles> CViewObjectsExt::LastPlacedCTRecords;
ConnectedTiles CViewObjectsExt::ThisPlacedCT;
int CViewObjectsExt::LastCTTile;
int CViewObjectsExt::LastSuccessfulIndex;
int CViewObjectsExt::NextCTHeightOffset;
int CViewObjectsExt::LastSuccessfulHeightOffset;
bool CViewObjectsExt::LastSuccessfulOpposite;
bool CViewObjectsExt::IsUsingTXCliff = false;
bool CViewObjectsExt::PlaceConnectedTile_Start = false;
bool CViewObjectsExt::HeightChanged;
bool CViewObjectsExt::IsInPlaceCliff_OnMouseMove;
std::vector<int> CViewObjectsExt::LastCTTileRecords;
std::vector<int> CViewObjectsExt::LastHeightRecords;


void CViewObjectsExt::ConnectedTile_Initialize() 
{
    TreeView_ConnectedTileMap.clear();
    CurrentConnectedTileType = -1;
    ConnectedTileSets.clear();
    auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");
    if (CMapDataExt::TileData)
    {
        std::string path = CFinalSunApp::Instance->ExePath();
        path += "\\ConnectedTileDrawer.ini";

        CINI ini;
        ini.ClearAndLoad(path.c_str());
        

        if (auto pSection = ini.GetSection("ConnectedTiles"))
        {
            for (auto& pair : pSection->GetEntities())
            {
                ConnectedTileSet cts;
                if (auto pSection2 = ini.GetSection(pair.second))
                {
                    cts.StartTile = ini.GetInteger(pair.second, "StartTile");
                    cts.Allowed = false;
                    auto allowedTheaters = STDHelpers::SplitString(ini.GetString(pair.second, "AllowedTheaters"));
                    cts.Name = ini.GetString(pair.second, "Name");
                    cts.SetName = pair.second;
                    auto type = ini.GetString(pair.second, "Type");
                    auto sptype = ini.GetString(pair.second, "SpecialType");
                    cts.WaterCliff = ini.GetBool(pair.second, "WaterCliff");
                    if (type == "Cliff")
                        cts.Type = ConnectedTileSetTypes::Cliff;
                    else if (type == "CityCliff")
                        cts.Type = ConnectedTileSetTypes::CityCliff;
                    else if (type == "IceCliff")
                        cts.Type = ConnectedTileSetTypes::IceCliff;
                    else if (type == "DirtRoad")
                        cts.Type = ConnectedTileSetTypes::DirtRoad;
                    else if (type == "Highway")
                        cts.Type = ConnectedTileSetTypes::Highway;
                    else if (type == "Shore")
                        cts.Type = ConnectedTileSetTypes::Shore;
                    else if (type == "PaveShore")
                        cts.Type = ConnectedTileSetTypes::PaveShore;
                    else if (type == "CityDirtRoad")
                        cts.Type = ConnectedTileSetTypes::CityDirtRoad;
                    else if (type == "RailRoad")
                        cts.Type = ConnectedTileSetTypes::RailRoad;

                    if (sptype == "SnowSnow")
                        cts.SpecialType = ConnectedTileSetSpecialTypes::SnowSnow;
                    else if (sptype == "SnowStone")
                        cts.SpecialType = ConnectedTileSetSpecialTypes::SnowStone;
                    else if (sptype == "StoneStone")
                        cts.SpecialType = ConnectedTileSetSpecialTypes::StoneStone;
                    else if (sptype == "StoneSnow")
                        cts.SpecialType = ConnectedTileSetSpecialTypes::StoneSnow;
                    else if (sptype == "SnowWater")
                        cts.SpecialType = ConnectedTileSetSpecialTypes::SnowWater;
                    else if (sptype == "StoneWater")
                        cts.SpecialType = ConnectedTileSetSpecialTypes::StoneWater;
                    else
                        cts.SpecialType = -1;

                    cts.IsTXCityCliff = false;
                    for (auto& t : allowedTheaters)
                    {
                        std::string t1 = (std::string)t;
                        std::transform(t1.begin(), t1.end(), t1.begin(), [](unsigned char c) {return std::tolower(c); });
                        std::string t2 = (std::string)thisTheater;
                        std::transform(t2.begin(), t2.end(), t2.begin(), [](unsigned char c) {return std::tolower(c); });
                        if (t1 == t2)
                            cts.Allowed = true;
                    }

                    for (int i = 0; i < 100; i++)
                    {
                        FString buffer;
                        buffer.Format("%s.%d", pair.second, i);
                        if (ini.SectionExists(buffer))
                        {
                            ConnectedTiles ct;

                            auto cp0 = STDHelpers::SplitString(ini.GetString(buffer, "ConnectionPoint0"));
                            ct.ConnectionPoint0.X = atoi(cp0[1]);
                            ct.ConnectionPoint0.Y = atoi(cp0[0]);
                            auto cp1 = STDHelpers::SplitString(ini.GetString(buffer, "ConnectionPoint1"));
                            ct.ConnectionPoint1.X = atoi(cp1[1]);
                            ct.ConnectionPoint1.Y = atoi(cp1[0]);

                            ct.Direction0 = ini.GetInteger(buffer, "ConnectionPoint0.Direction");
                            ct.Direction1 = ini.GetInteger(buffer, "ConnectionPoint1.Direction");

                            ct.Side0 = ini.GetString(buffer, "ConnectionPoint0.Side") == "Back";
                            ct.Side1 = ini.GetString(buffer, "ConnectionPoint1.Side") == "Back";

                            ct.Index = i;

                            ct.AdditionalOffset = ini.GetInteger(buffer, "AdditionalOffset");
                            ct.HeightAdjust = ini.GetInteger(buffer, "HeightAdjust");

                            auto tileIndices = STDHelpers::SplitString(ini.GetString(buffer, "TileIndices"));
                            for (auto& ti : tileIndices)
                            {
                                ct.TileIndices.push_back(atoi(ti) + ct.AdditionalOffset);
                            }

                            cts.ConnectedTile.push_back(ct);
                        }
                        else
                            break;
                    }

                }
                CViewObjectsExt::ConnectedTileSets.push_back(cts);
            }

            for (auto& cts : CViewObjectsExt::ConnectedTileSets)
            {
                FString key;
                for (int i = 0; i < 10; ++i)
                {
                    cts.ToSetPress[i] = -1;
                    key.Format("ToSet.Press%d", i);
                    if (auto pVaule = ini.TryGetString(cts.SetName, key))
                    {
                        int j = 0;
                        for (auto& cts2 : CViewObjectsExt::ConnectedTileSets)
                        {
                            if (cts2.SetName == *pVaule)
                            {
                                cts.ToSetPress[i] = j;
                                break;
                            }
                            j++;
                        }
                    }
                }
            }
        }
    }
}

void CViewObjectsExt::Redraw_ConnectedTile(CViewObjectsExt* pThis)
{
    int index = 0;
    HTREEITEM& hCT = ExtNodes[Root_Cliff];
    if (hCT == NULL)    return;

    HTREEITEM hCliff = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_Cliff",-1, hCT);
    HTREEITEM hCliffLand = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_CliffLand",-1, hCliff);
    HTREEITEM hCliffWater = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_CliffWater",-1, hCliff);

    HTREEITEM hCliffLandSnowSnow = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_CliffTXSnowSnow", -1, hCliffLand);
    HTREEITEM hCliffLandSnowStone = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_CliffTXSnowStone", -1, hCliffLand);
    HTREEITEM hCliffLandStoneSnow = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_CliffTXStoneSnow", -1, hCliffLand);
    HTREEITEM hCliffLandStoneStone = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_CliffTXStoneStone", -1, hCliffLand);
    HTREEITEM hCliffSnowWater = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_CliffTXSnowWater", -1, hCliffWater);
    HTREEITEM hCliffStoneWater = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_CliffTXStoneWater", -1, hCliffWater);

    HTREEITEM hDirtRoad = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_Road",-1, hCT);
    HTREEITEM hShore = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_Shore",-1, hCT);
    HTREEITEM hHighway = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_PavedRoad", -1, hCT);

    HTREEITEM hRailroad = pThis == nullptr ? NULL : pThis->InsertTranslatedString("Tracks", -1, hCT);

    std::vector<HTREEITEM> subNodes;
    subNodes.push_back(hCliffLandSnowSnow);
    subNodes.push_back(hCliffLandSnowStone);
    subNodes.push_back(hCliffLandStoneSnow);
    subNodes.push_back(hCliffLandStoneStone);
    subNodes.push_back(hCliffSnowWater);
    subNodes.push_back(hCliffStoneWater);
    subNodes.push_back(hCliffLand);
    subNodes.push_back(hCliffWater);
    subNodes.push_back(hDirtRoad);
    subNodes.push_back(hShore);
    subNodes.push_back(hHighway);
    subNodes.push_back(hCliff);
    subNodes.push_back(hRailroad);

    auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");

    int i = -1;
    for (auto& ct : ConnectedTileSets)
    {
        i++;
        if (ct.Allowed)
        {
            if (ct.ConnectedTile.empty()) continue;
            int firstTileIndex = ct.ConnectedTile.front().TileIndices[0] + ct.StartTile;
            int lastTileIndex = ct.ConnectedTile.back().TileIndices[0] + ct.StartTile;

            if (ct.Type == ConnectedTileSetTypes::CityCliff)
            {
                // 29 & 30 are special diagonals in TX
                if (ct.ConnectedTile.size() > 28)
                    lastTileIndex = ct.ConnectedTile[28].TileIndices[0] + ct.StartTile;
                if (ct.ConnectedTile.size() > 30)
                {
                    int lastTileIndexTX = ct.ConnectedTile[30].TileIndices[0] + ct.StartTile;
                    if (lastTileIndexTX < CMapDataExt::TileDataCount)
                    {
                        int lastTilesetTX = CMapDataExt::TileData[lastTileIndexTX].TileSet;
                        FString buffer;
                        buffer.Format("TileSet%04d", lastTilesetTX);

                        auto exist = CINI::CurrentTheater->GetBool(buffer, "AllowToPlace", true);
                        auto exist2 = CINI::CurrentTheater->GetString(buffer, "FileName", "");
                        if (exist && strcmp(exist2, "") != 0)
                        {
                            ct.IsTXCityCliff = true;
                        }
                    }
                }
            }

            if (ct.Type != ConnectedTileSetTypes::RailRoad)
            {
                if (firstTileIndex > CMapDataExt::TileDataCount || lastTileIndex > CMapDataExt::TileDataCount)
                    continue;

                int firstTileset = CMapDataExt::TileData[firstTileIndex].TileSet;
                int lastTileset = CMapDataExt::TileData[lastTileIndex].TileSet;
                if (!CMapDataExt::IsValidTileSet(firstTileset) || !CMapDataExt::IsValidTileSet(lastTileset))
                    continue;
            }
            
            ConnectedTileInfo info{};
            info.Index = i;
            info.Front = true;
            ConnectedTileInfo info2{};
            info2.Index = i;
            info2.Front = false;

            switch (ct.Type)
            {
            case ConnectedTileSetTypes::Cliff:
            case ConnectedTileSetTypes::CityCliff:
            case ConnectedTileSetTypes::IceCliff:
                if (ct.WaterCliff)
                {
                    if (ct.SpecialType < 0)
                    {
                        TreeView_ConnectedTileMap[index] = info;
                        if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hCliffWater);
                        index++;
                        TreeView_ConnectedTileMap[index] = info2;
                        if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hCliffWater);
                        index++;
                    }
                    else
                    {
 
                        if (ct.SpecialType == SnowWater)
                        {
                            TreeView_ConnectedTileMap[index] = info;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hCliffSnowWater);
                            index++;
                            TreeView_ConnectedTileMap[index] = info2;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hCliffSnowWater);
                            index++;
                        }
                        else if (ct.SpecialType == StoneWater)
                        {
                            TreeView_ConnectedTileMap[index] = info;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hCliffStoneWater);
                            index++;
                            TreeView_ConnectedTileMap[index] = info2;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hCliffStoneWater);
                            index++;
                        }
  
                    }
                }
                else
                {
                    if (ct.SpecialType < 0)
                    {
                        TreeView_ConnectedTileMap[index] = info;
                        if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hCliffLand);
                        index++;
                        TreeView_ConnectedTileMap[index] = info2;
                        if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hCliffLand);
                        index++;
                    }
                    else
                    {
                        if (ct.SpecialType == SnowSnow)
                        {
                            TreeView_ConnectedTileMap[index] = info;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hCliffLandSnowSnow);
                            index++;
                            TreeView_ConnectedTileMap[index] = info2;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hCliffLandSnowSnow);
                            index++;
                        }
                        else if (ct.SpecialType == SnowStone)
                        {
                            TreeView_ConnectedTileMap[index] = info;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hCliffLandSnowStone);
                            index++;
                            TreeView_ConnectedTileMap[index] = info2;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hCliffLandSnowStone);
                            index++;
                        }
                        else if (ct.SpecialType == StoneSnow)
                        {
                            TreeView_ConnectedTileMap[index] = info;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hCliffLandStoneSnow);
                            index++;
                            TreeView_ConnectedTileMap[index] = info2;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hCliffLandStoneSnow);
                            index++;
                        }
                        else if (ct.SpecialType == StoneStone)
                        {
                            TreeView_ConnectedTileMap[index] = info;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hCliffLandStoneStone);
                            index++;
                            TreeView_ConnectedTileMap[index] = info2;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hCliffLandStoneStone);
                            index++;
                        }

                    }

                }
                break;
            case ConnectedTileSetTypes::DirtRoad:
            case ConnectedTileSetTypes::CityDirtRoad:
                TreeView_ConnectedTileMap[index] = info;
                if (pThis) pThis->InsertString(ct.Name, Const_ConnectedTile + index, hDirtRoad);
                index++;
                break;
            case ConnectedTileSetTypes::Highway:
                TreeView_ConnectedTileMap[index] = info;
                if (pThis) pThis->InsertString(ct.Name, Const_ConnectedTile + index, hHighway);
                index++;
                break;
            case ConnectedTileSetTypes::RailRoad:
                TreeView_ConnectedTileMap[index] = info;
                if (pThis) pThis->InsertString(ct.Name, Const_ConnectedTile + index, hRailroad);
                index++;
                break;
            case ConnectedTileSetTypes::Shore:
            case ConnectedTileSetTypes::PaveShore:
                TreeView_ConnectedTileMap[index] = info;
                if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hShore);
                index++;
                TreeView_ConnectedTileMap[index] = info2;
                if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hShore);
                index++;
                break;
            default:
                break;
            }
        }
    }

    if (pThis)
    {
        for (auto& subnode : subNodes)
        {
            if (!pThis->GetTreeCtrl().ItemHasChildren(subnode))
                pThis->GetTreeCtrl().DeleteItem(subnode);
        }
    }
}

void CViewObjectsExt::PlaceConnectedTile_OnMouseMove(int X, int Y, bool place)
{
    if (!CMapDataExt::IsCoordInFullMap(X, Y))
        return;
    CViewObjectsExt::IsInPlaceCliff_OnMouseMove = true;

    auto handleExit = []()
        {
            CViewObjectsExt::NextCTHeightOffset = 0;
            CViewObjectsExt::IsInPlaceCliff_OnMouseMove = false;
        };

    if (CViewObjectsExt::CliffConnectionCoord.X == -1 && CViewObjectsExt::CliffConnectionCoord.Y == -1)
    {
        handleExit();
        return;
    }

    auto& mapData = CMapData::Instance();
    auto cellDatas = mapData.CellDatas;
    std::vector<int> cliffConnectionTiles;
    bool forceFront = false;
    bool forceBack = false;

    ConnectedTileSet tileSet;
    bool UrbanCliff = false;
    bool IceCliff = false;
    bool cityRoad = false;
    bool railRoad = false;

    int subPos = 0;

    int dwposFix = -1;
    int dwposFix2 = -1;
    std::map <int, CellData> tmpCellDatas;
    auto thisTile = CMapDataExt::TileData[0];

    MapCoord cursor;
    cursor.X = X; cursor.Y = Y;
    int facing = CMapDataExt::GetFacing(CViewObjectsExt::CliffConnectionCoord, cursor);

    int xx = CViewObjectsExt::CliffConnectionCoord.X - cursor.X;
    int yy = CViewObjectsExt::CliffConnectionCoord.Y - cursor.Y;
    double distance = sqrt(xx * xx + yy * yy);

    int distanceX = abs(xx);
    int distanceY = abs(yy);

    int SmallDistance = 5;
    int LargeDistance = 9;

    int offsetConnectX = 0;
    int offsetConnectY = 0;
    int offsetPlaceX = 0;
    int offsetPlaceY = 0;
    
    bool IsPavedRoadsRandom = false;
    int MultiPlaceDirection = -1;
    bool thisTileHeightOffest = false;
    bool opposite = false;

    auto getOppositeDirection = [](int dir)
        {
            if (dir > 7 || dir < 0)
                return 0;
            if (dir == 0)
                return 4;
            if (dir == 1)
                return 5;
            if (dir == 2)
                return 6;
            if (dir == 3)
                return 7;
            if (dir == 4)
                return 0;
            if (dir == 5)
                return 1;
            if (dir == 6)
                return 2;
            if (dir == 7)
                return 3;
            return 0;
        };

    if (CIsoView::CurrentCommand->Type >= 9000)
    {
        handleExit();
        return;
    }

    int ctIndex = CIsoView::CurrentCommand->Type;
    CurrentConnectedTileType = TreeView_ConnectedTileMap[ctIndex].Index;
    tileSet = CViewObjectsExt::ConnectedTileSets[CurrentConnectedTileType];

    switch (tileSet.Type)
    {
    case ConnectedTileSetTypes::Cliff:
        if (TreeView_ConnectedTileMap[ctIndex].Front && CViewObjectsExt::LastPlacedCT.Index == -1)
        {
            forceFront = true;
        }
        else if (!TreeView_ConnectedTileMap[ctIndex].Front && CViewObjectsExt::LastPlacedCT.Index == -1)
        {
            forceBack = true;
        }
        break;
    case ConnectedTileSetTypes::CityCliff:
        if (TreeView_ConnectedTileMap[ctIndex].Front && CViewObjectsExt::LastPlacedCT.Index == -1)
        {
            forceFront = true;
        }
        else if (!TreeView_ConnectedTileMap[ctIndex].Front && CViewObjectsExt::LastPlacedCT.Index == -1)
        {
            forceBack = true;
        }
        UrbanCliff = true;
        break;
    case ConnectedTileSetTypes::IceCliff:
        if (TreeView_ConnectedTileMap[ctIndex].Front && CViewObjectsExt::LastPlacedCT.Index == -1)
        {
            forceFront = true;
        }
        else if (!TreeView_ConnectedTileMap[ctIndex].Front && CViewObjectsExt::LastPlacedCT.Index == -1)
        {
            forceBack = true;
        }
        IceCliff = true;
        break;
    case ConnectedTileSetTypes::DirtRoad:
        break;
    case ConnectedTileSetTypes::CityDirtRoad:
        cityRoad = true;
        break;
    case ConnectedTileSetTypes::Highway:
        break;
    case ConnectedTileSetTypes::RailRoad:
        railRoad = true;
        break;
    case ConnectedTileSetTypes::Shore:
        if (TreeView_ConnectedTileMap[ctIndex].Front)
        {
            forceFront = true;
        }
        else
        {
            forceBack = true;
        }
        break;
    case ConnectedTileSetTypes::PaveShore:
        if (TreeView_ConnectedTileMap[ctIndex].Front)
        {
            forceFront = true;
        }
        else
        {
            forceBack = true;
        }
        break;
    default:
        break;
    }

    //very stupid code
    if (tileSet.Type == ConnectedTileSetTypes::Cliff || tileSet.Type == ConnectedTileSetTypes::CityCliff || tileSet.Type == ConnectedTileSetTypes::IceCliff)
    {
        if (!tileSet.Name)
        {
            handleExit();
            return;
        }

        int index = -1;

        if (NULL == CMapDataExt::TileData)
        {
            handleExit();
            return;
        }

        if (place)
        {
            mapData.SaveUndoRedoData(true, CViewObjectsExt::CliffConnectionCoord.X - 4, CViewObjectsExt::CliffConnectionCoord.Y - 4,
                CViewObjectsExt::CliffConnectionCoord.X + 4, CViewObjectsExt::CliffConnectionCoord.Y + 4);
        }

        //        7 
        //     6     0
        //  5           1
        //     4     2
        //        3

        int loop = 0;
        while (cliffConnectionTiles.empty())
        {
            loop++;
            if (facing == 1)
            {
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22)
                {
                    facing = 3;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    facing = 3;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18)
                {
                    facing = 0;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17)
                {
                    facing = 7;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30)
                {
                    facing = 2;
                    continue;
                }
                if (!forceFront && (forceBack
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Index == 16
                    || CViewObjectsExt::LastPlacedCT.Index == 17
                    || CViewObjectsExt::LastPlacedCT.Index == 18
                    || CViewObjectsExt::LastPlacedCT.Index == 21
                    || CViewObjectsExt::LastPlacedCT.Index == 28
                    || CViewObjectsExt::LastPlacedCT.Index == 22
                    || CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1))
                {
                    if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                    {
                        offsetPlaceY -= 1;
                    }

                    std::vector<int> backCornet1;
                    std::vector<int> backCornet2;
                    backCornet1.push_back(14);
                    backCornet1.push_back(15);
                    backCornet2.push_back(17);
                    backCornet2.push_back(18);

                    offsetPlaceY += 1;
                    offsetConnectX -= 1;

                    index = STDHelpers::RandomSelectInt(backCornet1);
                    if (distance < SmallDistance)
                        index = 16;

                    if (!UrbanCliff)
                    {
                        if (CViewObjectsExt::LastPlacedCT.Index == 12
                            || CViewObjectsExt::LastPlacedCT.Index == 13
                            || CViewObjectsExt::LastPlacedCT.Index == 27
                            || CViewObjectsExt::LastPlacedCT.Index == 14
                            || CViewObjectsExt::LastPlacedCT.Index == 25
                            || CViewObjectsExt::LastPlacedCT.Index == 15
                            || CViewObjectsExt::LastPlacedCT.Index == 16)
                        {
                            if (distance > LargeDistance)
                            {
                                if (place)
                                    index = CViewObjectsExt::ThisPlacedCT.Index;
                                else
                                    index = STDHelpers::RandomSelectInt(backCornet2);
                            }
                        }

                    }
                    else if (UrbanCliff && index != 16)
                    {
                        index = 25;
                    }

                    if (CViewObjectsExt::LastPlacedCT.Index == 7
                        || CViewObjectsExt::LastPlacedCT.Index == 8
                        || CViewObjectsExt::LastPlacedCT.Index == 9
                        || CViewObjectsExt::LastPlacedCT.Index == 10
                        || CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Index == 30
                        || CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        index = 18;
                        offsetPlaceX -= 1;
                    }


                    if (index == 17 || index == 18)
                    {
                        offsetPlaceX += 1;
                        offsetConnectY -= 1;
                    }

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else
                {

                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        facing = 7;
                        continue;
                    }

                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9)
                    {
                        facing = 2;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6)
                    {
                        facing = 2;
                        continue;
                    }

                    if (CViewObjectsExt::LastPlacedCT.Index == 7
                        || CViewObjectsExt::LastPlacedCT.Index == 8
                        || CViewObjectsExt::LastPlacedCT.Index == 9
                        || CViewObjectsExt::LastPlacedCT.Index == 10
                        || CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Index == 11)
                        offsetPlaceY += 1;
                    if (CViewObjectsExt::LastPlacedCT.Index == 12
                        || CViewObjectsExt::LastPlacedCT.Index == 27
                        || CViewObjectsExt::LastPlacedCT.Index == 13)
                        offsetPlaceX -= 1;

                    offsetConnectY += 1;
                    offsetConnectX -= 1;
                    index = 5;
                    if (distance < SmallDistance)
                        index = 6;
                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
            }
            else if (facing == 2)
            {
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22)
                {
                    facing = 1;
                    continue;
                }
                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18)
                {
                    facing = 0;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22)
                {
                    facing = 3;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    facing = 3;
                    continue;
                }
                if ((CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18) && !tileSet.IsTXCityCliff)
                {
                    facing = 1;
                    continue;
                }
                if (!forceFront && (forceBack || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16))
                {
                    if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                    {
                        offsetPlaceX -= 1;
                        offsetPlaceY -= 1;
                    }

                    offsetPlaceX += 1;
                    offsetPlaceY += 1;
                    offsetConnectX -= 1;

                    if (CViewObjectsExt::LastPlacedCT.Index == 5
                        || CViewObjectsExt::LastPlacedCT.Index == 6
                        || CViewObjectsExt::LastPlacedCT.Index == 7
                        || CViewObjectsExt::LastPlacedCT.Index == 8
                        || CViewObjectsExt::LastPlacedCT.Index == 9
                        || CViewObjectsExt::LastPlacedCT.Index == 10
                        || CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Index == 30
                        || CViewObjectsExt::LastPlacedCT.Index == 11)
                        offsetPlaceX -= 1;

                    index = 12;
                    if (distance < SmallDistance)
                        index = 13;

                    if (distance > LargeDistance && UrbanCliff)
                    {
                        index = 27;
                        offsetConnectY += 1;
                    }
                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else
                {
                    bool txcliff = false;
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27)
                    {
                        if (!tileSet.IsTXCityCliff)
                        {
                            facing = 1;
                            continue;
                        }
                        else
                        {
                            txcliff = true;
                        }
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18)
                    {
                        facing = 0;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1)
                    {
                        facing = 1;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3 
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        facing = 7;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        if (!tileSet.IsTXCityCliff) 
                        {
                            facing = 1;
                            continue;
                        }
                        else
                        {
                            txcliff = true;
                        }
                    }

                    offsetConnectY += 1;
                    index = 3;
                    if (distance < SmallDistance)
                        index = 4;

                    if ((CViewObjectsExt::LastPlacedCT.Index == 5 || CViewObjectsExt::LastPlacedCT.Index == 6) && distance > LargeDistance && tileSet.IsTXCityCliff)
                    {
                        offsetPlaceY -= 1;
                        index = 30;
                    }

                    if (txcliff)
                    {
                        index = 30;
                    }

                    if (index == 30 && ( CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18))
                    {
                        offsetPlaceX -= 1;
                        offsetPlaceY -= 1;
                    }

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
            }
            else if (facing == 3)
            {
                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22)
                {
                    facing = 1;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18)
                {
                    facing = 1;
                    continue;
                }
                if (!forceFront && (forceBack || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11))
                {
                    if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                    {
                        offsetPlaceY -= 1;
                    }
                    offsetPlaceY += 1;
                    offsetConnectX += 1;
                    index = 10;
                    if (distance < SmallDistance)
                        index = 11;
                    if (distance > LargeDistance)
                        index = 9;

                    if (index == 9)
                    {
                        offsetPlaceY -= 1;
                        if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                        {
                            offsetPlaceY += 1;
                        }
                    }

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    offsetPlaceY += 1;
                    offsetPlaceX += 1;
                    offsetConnectX += 1;

                    index = 10;
                    if (distance < SmallDistance)
                        index = 11;

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                    {
                        offsetPlaceY -= 1;
                    }
                    offsetConnectY += 1;
                    index = 0;
                    if (distance < SmallDistance)
                        index = 1;

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else
                {
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1)
                    {
                        facing = 1;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        facing = 5;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        facing = 1;
                        continue;
                    }

                    if (CViewObjectsExt::LastPlacedCT.Index == 5
                        || CViewObjectsExt::LastPlacedCT.Index == 6
                        || CViewObjectsExt::LastPlacedCT.Index == 3
                        || CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Index == 30
                        || CViewObjectsExt::LastPlacedCT.Index == 4
                        || CViewObjectsExt::LastPlacedCT.Index == 0
                        || CViewObjectsExt::LastPlacedCT.Index == 1
                        || CViewObjectsExt::LastPlacedCT.Index == 2)
                        offsetPlaceX += 1;
                    offsetConnectY += 1;
                    index = 0;
                    if (distance < SmallDistance)
                        index = 1;
                    if (distance > LargeDistance)
                        index = 2;

                    if (index == 2)
                    {
                        offsetPlaceX -= 1;
                        if (forceFront && CViewObjectsExt::PlaceConnectedTile_Start)
                        {
                            offsetPlaceX += 1;
                        }
                    }

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }

            }
            else if (facing == 4)
            {
                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18)
                {
                    facing = 6;
                    continue;
                }
                if ((CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22) && !tileSet.IsTXCityCliff)
                {
                    facing = 5;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18)
                {
                    facing = 5;
                    continue;
                }

                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    facing = 3;
                    continue;
                }

                if (!forceFront && (forceBack || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Index == 16))
                {
                    if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                    {
                        offsetPlaceX -= 1;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Index != 21
                        && CViewObjectsExt::LastPlacedCT.Index != 22
                        && CViewObjectsExt::LastPlacedCT.Index != 28
                        && CViewObjectsExt::LastPlacedCT.Index != 14
                        && CViewObjectsExt::LastPlacedCT.Index != 15
                        && CViewObjectsExt::LastPlacedCT.Index != 25
                        && CViewObjectsExt::LastPlacedCT.Index != 16)
                        offsetPlaceX += 1;
                    offsetConnectX += 1;

                    index = 21;
                    if (distance < SmallDistance)
                        index = 22;

                    if (distance > LargeDistance && UrbanCliff)
                    {
                        index = 28;
                    }


                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else
                {
                    bool txcliff = false;
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                    {
                        facing = 3;
                        continue;
                    }
                    if ((CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22))
                    {
                        if (!tileSet.IsTXCityCliff)
                        {
                            facing = 1;
                            continue;
                        }
                        else
                        {
                            txcliff = true;
                        }
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        if (!tileSet.IsTXCityCliff)
                        {
                            facing = 5;
                            continue;
                        }
                        else
                        {
                            txcliff = true;
                        }
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9)
                    {
                        facing = 1;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        facing = 5;
                        continue;
                    }
                    offsetConnectX += 1;
                    index = 7;
                    if (distance < SmallDistance)
                        index = 8;

                    if ((CViewObjectsExt::LastPlacedCT.Index == 5 || CViewObjectsExt::LastPlacedCT.Index == 6) && distance > LargeDistance && tileSet.IsTXCityCliff)
                    {
                        offsetPlaceX -= 1;
                        opposite = true;
                        index = 30;
                    }

                    if (txcliff)
                    {
                        opposite = true;
                        index = 30;
                    }

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
            }
            else if (facing == 5)
            {
                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18)
                {
                    facing = 6;
                    continue;
                }
                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    facing = 3;
                    continue;
                }

                if (!forceFront && (forceBack || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4
                    || CViewObjectsExt::LastPlacedCT.Index == 12
                    || CViewObjectsExt::LastPlacedCT.Index == 13
                    || CViewObjectsExt::LastPlacedCT.Index == 27
                    || CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Index == 16
                    || CViewObjectsExt::LastPlacedCT.Index == 17
                    || CViewObjectsExt::LastPlacedCT.Index == 18
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22
                    || CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11))
                {
                    if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                    {
                        offsetPlaceY += 1;
                    }
                    opposite = true;

                    std::vector<int> backCornet1;
                    std::vector<int> backCornet2;
                    backCornet1.push_back(14);
                    backCornet1.push_back(15);
                    backCornet2.push_back(17);
                    backCornet2.push_back(18);

                    offsetPlaceY -= 1;
                    offsetConnectX += 1;

                    index = STDHelpers::RandomSelectInt(backCornet1);
                    if (distance < SmallDistance)
                        index = 16;
                    if (!UrbanCliff)
                    {
                        if (CViewObjectsExt::LastPlacedCT.Index == 14
                            || CViewObjectsExt::LastPlacedCT.Index == 15
                            || CViewObjectsExt::LastPlacedCT.Index == 25
                            || CViewObjectsExt::LastPlacedCT.Index == 16
                            || CViewObjectsExt::LastPlacedCT.Index == 21
                            || CViewObjectsExt::LastPlacedCT.Index == 28
                            || CViewObjectsExt::LastPlacedCT.Index == 22)
                        {
                            if (distance > LargeDistance)
                            {
                                if (place)
                                    index = CViewObjectsExt::ThisPlacedCT.Index;
                                else
                                    index = STDHelpers::RandomSelectInt(backCornet2);
                            }

                        }

                    }
                    else if (UrbanCliff && index != 16)
                    {
                        index = 25;

                    }

                    if (CViewObjectsExt::LastPlacedCT.Index == 0
                        || CViewObjectsExt::LastPlacedCT.Index == 1
                        || CViewObjectsExt::LastPlacedCT.Index == 2
                        || CViewObjectsExt::LastPlacedCT.Index == 3
                        || CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        index = 18;
                        offsetPlaceX += 1;
                    }

                    if (CViewObjectsExt::LastPlacedCT.Index == 10
                        || CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        offsetPlaceX += 1;
                        offsetPlaceY += 1;
                    }

                    if (index == 17 || index == 18)
                    {
                        offsetPlaceY += 1;
                        offsetConnectX -= 1;
                    }



                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else
                {
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        facing = 4;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9)
                    {
                        facing = 7;
                        continue;
                    }
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6)
                    {
                        facing = 7;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Index == 0
                        || CViewObjectsExt::LastPlacedCT.Index == 1
                        || CViewObjectsExt::LastPlacedCT.Index == 2
                        || CViewObjectsExt::LastPlacedCT.Index == 3
                        || CViewObjectsExt::LastPlacedCT.Index == 4
                        || CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Index == 21
                        || CViewObjectsExt::LastPlacedCT.Index == 28
                        || CViewObjectsExt::LastPlacedCT.Index == 22)
                        offsetPlaceX += 1;
                    opposite = true;
                    offsetConnectY -= 1;
                    offsetConnectX += 1;
                    index = 5;
                    if (distance < SmallDistance)
                        index = 6;
                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
            }
            else if (facing == 6)
            {
                bool txcliff = false;
                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17)
                {
                    facing = 0;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9)
                {
                    facing = 7;
                    continue;
                }

                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9)
                {
                    if (!tileSet.IsTXCityCliff) 
                    {
                        facing = 5;
                        continue;
                    }
                    else
                    {
                        txcliff = true;
                    }

                }

                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    facing = 3;
                    continue;
                }

                if (!forceFront && (forceBack || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11
                    || CViewObjectsExt::LastPlacedCT.Index == 12
                    || CViewObjectsExt::LastPlacedCT.Index == 13
                    || CViewObjectsExt::LastPlacedCT.Index == 27
                    || CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Index == 16
                    || CViewObjectsExt::LastPlacedCT.Index == 17
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22))
                {
                    if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                    {
                        offsetPlaceY += 1;
                    }
                    opposite = true;

                    if (CViewObjectsExt::LastPlacedCT.Index == 10
                        || CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        offsetPlaceX += 1;
                    }
                    else
                        offsetPlaceY -= 1;

                    index = 12;
                    if (distance < SmallDistance)
                        index = 13;
                    if (distance > LargeDistance && UrbanCliff && CViewObjectsExt::LastPlacedCT.Index != 28)
                    {
                        index = 27;
                        offsetPlaceY -= 1;
                    }
                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else
                {
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        if (!tileSet.IsTXCityCliff)
                        {
                            facing = 5;
                            continue;
                        }
                        else
                        {
                            txcliff = true;
                        }
                    }
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        facing = 4;
                        continue;
                    }
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6)
                    {
                        facing = 7;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Index == 5
                        || CViewObjectsExt::LastPlacedCT.Index == 6)
                        offsetPlaceX -= 1;
                    opposite = true;
                    offsetConnectY -= 1;
                    index = 3;
                    if (distance < SmallDistance)
                        index = 4;
                    if ((CViewObjectsExt::LastPlacedCT.Index == 5 ||CViewObjectsExt::LastPlacedCT.Index == 6) && distance > LargeDistance && tileSet.IsTXCityCliff)
                    {
                        offsetPlaceX += 1;
                        index = 29;
                    }

                    if (txcliff)
                    {
                        index = 29;
                    }
                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
            }
            else if (facing == 7)
            {
                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17)
                {
                    facing = 0;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22)
                {
                    facing = 6;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    facing = 6;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17)
                {
                    facing = 6;
                    continue;
                }
                if (!forceFront && (forceBack || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18))
                {
                    if (CViewObjectsExt::LastPlacedCT.Index == 5
                        || CViewObjectsExt::LastPlacedCT.Index == 6)
                        offsetPlaceY -= 1;
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27)
                    {
                        offsetPlaceX -= 1;
                        offsetPlaceY -= 1;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Index == 18)
                    {
                        offsetPlaceX -= 1;
                        offsetPlaceY -= 1;
                    }

                    opposite = true;
                    offsetConnectX -= 1;
                    offsetConnectY -= 1;
                    index = 10;
                    if (distance < SmallDistance)
                        index = 11;
                    if (distance > LargeDistance)
                        index = 9;

                    if (index == 9)
                        offsetConnectY += 1;

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else
                {
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                    {
                        facing = 0;
                        continue;
                    }

                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        facing = 5;
                        continue;
                    }
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        facing = 1;
                        continue;
                    }
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9)
                    {
                        facing = 5;
                        continue;
                    }

                    if (CViewObjectsExt::LastPlacedCT.Index == 5
                        || CViewObjectsExt::LastPlacedCT.Index == 6)
                        offsetPlaceX -= 1;
                    opposite = true;
                    offsetConnectX -= 1;
                    offsetConnectY -= 1;
                    index = 0;
                    if (distance < SmallDistance)
                        index = 1;
                    if (distance > LargeDistance)
                        index = 2;

                    if (index == 2)
                        offsetConnectX += 1;

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }

            }
            else if (facing == 0)
            {
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22)
                {
                    facing = 6;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    facing = 6;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17)
                {
                    facing = 6;
                    continue;
                }

                if (!forceFront && (forceBack || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                    || CViewObjectsExt::LastPlacedCT.Index == 21
                    || CViewObjectsExt::LastPlacedCT.Index == 28
                    || CViewObjectsExt::LastPlacedCT.Index == 22
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18
                    ))
                {
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1)
                    {
                        facing = 1;
                        continue;
                    }
                    if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                    {
                        offsetPlaceY -= 1;
                    }
                    opposite = true;
                    offsetConnectX -= 1;
                    offsetConnectY -= 1;
                    offsetPlaceY += 1;
                    index = 21;
                    if (distance < SmallDistance)
                        index = 22;

                    if (distance > LargeDistance && UrbanCliff && CViewObjectsExt::LastPlacedCT.Index != 27)
                    {
                        index = 28;
                    }

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else
                {
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        facing = 5;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        facing = 7;
                        continue;
                    }
                    
                    bool txcliff = false;
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        if (!tileSet.IsTXCityCliff)
                        {
                            facing = 1;
                            continue;
                        }
                        else
                        {
                            txcliff = true;
                        }

                    }
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9)
                    {
                        facing = 2;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6)
                    {
                        facing = 2;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Index == 18)
                    {
                        offsetPlaceX -= 1;
                        offsetPlaceY -= 1;
                    }

                    if (CViewObjectsExt::LastPlacedCT.Index == 5
                        || CViewObjectsExt::LastPlacedCT.Index == 6)
                        offsetPlaceY -= 1;
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13)
                    {
                        offsetPlaceX -= 1;
                        offsetPlaceY -= 1;
                    }
                    opposite = true;
                    offsetConnectX -= 1;
                    index = 7;
                    if (distance < SmallDistance)
                        index = 8;

                    if ((CViewObjectsExt::LastPlacedCT.Index == 5 || CViewObjectsExt::LastPlacedCT.Index == 6) &&distance > LargeDistance && tileSet.IsTXCityCliff)
                    {
                        opposite = false;
                        offsetPlaceY += 1;
                        index = 29;
                    }

                    if (txcliff)
                    {
                        opposite = false;
                        index = 29;
                    }
                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
            }
            else
            {
                handleExit();
                return;
            }

            if (index < 0)
            {
                handleExit();
                return;
            }

            if (loop > 3)
            {
                handleExit();
                return;
            }
        }

        CViewObjectsExt::ThisPlacedCT = tileSet.ConnectedTile[index];
        if (!place)
        {
            CViewObjectsExt::LastTempPlacedCTIndex = index;
            CViewObjectsExt::LastTempFacing = facing;
            CViewObjectsExt::CliffConnectionTile = STDHelpers::RandomSelectInt(cliffConnectionTiles, true, index);
        }
        else
        {
            if (index != CViewObjectsExt::LastTempPlacedCTIndex || facing != CViewObjectsExt::LastTempFacing)
            {
                CViewObjectsExt::CliffConnectionTile = STDHelpers::RandomSelectInt(cliffConnectionTiles, true, index);
            }
            CViewObjectsExt::LastTempPlacedCTIndex = -1;
            CViewObjectsExt::LastTempFacing = -1;
        }

        if (opposite)
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y;
        }
        else
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y;
        }

        thisTile = CMapDataExt::TileData[CViewObjectsExt::CliffConnectionTile];


        std::vector<int> fixRandom;
        fixRandom.push_back(0);
        fixRandom.push_back(1);

        if (IceCliff
            && (index == 5 || index == 6)
            && (CViewObjectsExt::LastPlacedCT.Index == 5
                || CViewObjectsExt::LastPlacedCT.Index == 6
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[25].TileIndices[0] + tileSet.StartTile;
            if (index == 6)
                offsetY -= 1;

            offsetX += 1;
            offsetY += 1;
            dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;
            if (opposite)
            {
                if (index == 6)
                {
                    offsetX -= 1;
                    offsetY += 1;
                }

                dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }

            if (dwposFix2 < 0 || dwposFix2 > mapData.CellDataCount)
                dwposFix2 = 0;

            auto thisTileFix = CMapDataExt::TileData[idxFix];
            tmpCellDatas[dwposFix2] = cellDatas[dwposFix2];

            cellDatas[dwposFix2].TileIndex = idxFix;
            cellDatas[dwposFix2].TileSubIndex = 0;
            cellDatas[dwposFix2].Flag.AltIndex = STDHelpers::RandomSelectInt(fixRandom);
            auto newHeight = CViewObjectsExt::CliffConnectionHeight + thisTileFix.TileBlockDatas[0].Height;
            if (newHeight > 14) newHeight = 14;
            if (newHeight < 0) newHeight = 0;
            cellDatas[dwposFix2].Height = newHeight;
        }
        else if (IceCliff
            && (opposite && (index == 0
                || index == 1
                || index == 2
                || index == 3
                || index == 4
                || index == 7
                || index == 8
                || index == 9
                || index == 10
                || index == 11))
            &&
            (CViewObjectsExt::LastPlacedCT.Index == 5
                || CViewObjectsExt::LastPlacedCT.Index == 6)
            )
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[25].TileIndices[0] + tileSet.StartTile;
            if (index == 6)
                offsetY -= 1;

            offsetX += 1;
            offsetY += 1;
            dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;
            if (opposite)
            {
                if (index == 6)
                {
                    offsetX -= 1;
                    offsetY += 1;
                }

                dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }
            if (dwposFix2 < 0 || dwposFix2 > mapData.CellDataCount)
                dwposFix2 = 0;

            auto thisTileFix = CMapDataExt::TileData[idxFix];
            tmpCellDatas[dwposFix2] = cellDatas[dwposFix2];

            cellDatas[dwposFix2].TileIndex = idxFix;
            cellDatas[dwposFix2].TileSubIndex = 0;
            cellDatas[dwposFix2].Flag.AltIndex = STDHelpers::RandomSelectInt(fixRandom);
            auto newHeight = CViewObjectsExt::CliffConnectionHeight + thisTileFix.TileBlockDatas[0].Height;
            if (newHeight > 14) newHeight = 14;
            if (newHeight < 0) newHeight = 0;
            cellDatas[dwposFix2].Height = newHeight;
        }

        if ((index == 5 || index == 6)
            && (CViewObjectsExt::LastPlacedCT.Index == 5
                || CViewObjectsExt::LastPlacedCT.Index == 6
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                || CViewObjectsExt::LastPlacedCT.Index == 29
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[20].TileIndices[0] + tileSet.StartTile;
            if (index == 6)
                offsetY -= 1;
            dwposFix = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;
            if (opposite)
            {
                if (index == 6)
                {
                    offsetX -= 1;
                    offsetY += 1;
                }

                dwposFix = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }
            if (dwposFix < 0 || dwposFix > mapData.CellDataCount)
                dwposFix = 0;

            auto thisTileFix = CMapDataExt::TileData[idxFix];
            tmpCellDatas[dwposFix] = cellDatas[dwposFix];

            cellDatas[dwposFix].TileIndex = idxFix;
            cellDatas[dwposFix].TileSubIndex = 0;
            cellDatas[dwposFix].Flag.AltIndex = STDHelpers::RandomSelectInt(fixRandom);
            auto newHeight = CViewObjectsExt::CliffConnectionHeight + thisTileFix.TileBlockDatas[0].Height;
            if (newHeight > 14) newHeight = 14;
            if (newHeight < 0) newHeight = 0;
            cellDatas[dwposFix].Height = newHeight;
        }
        else if ((index == 29)
            && (CViewObjectsExt::LastPlacedCT.Index == 5
                || CViewObjectsExt::LastPlacedCT.Index == 6))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[20].TileIndices[0] + tileSet.StartTile;
            offsetY -= 1;
            dwposFix = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;
            if (opposite)
            {
                offsetX -= 1;
                offsetY += 1;
                dwposFix = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }
            if (dwposFix < 0 || dwposFix > mapData.CellDataCount)
                dwposFix = 0;

            auto thisTileFix = CMapDataExt::TileData[idxFix];
            tmpCellDatas[dwposFix] = cellDatas[dwposFix];

            cellDatas[dwposFix].TileIndex = idxFix;
            cellDatas[dwposFix].TileSubIndex = 0;
            cellDatas[dwposFix].Flag.AltIndex = STDHelpers::RandomSelectInt(fixRandom);
            auto newHeight = CViewObjectsExt::CliffConnectionHeight + thisTileFix.TileBlockDatas[0].Height;
            if (newHeight > 14) newHeight = 14;
            if (newHeight < 0) newHeight = 0;
            cellDatas[dwposFix].Height = newHeight;
        }
        else if ((index == 0 && !opposite || index == 1 && !opposite || index == 2 && !opposite || index == 3 && !opposite || index == 4 && !opposite || index == 21 && !opposite || index == 22 && !opposite || index == 28 && !opposite)
            && (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[20].TileIndices[0] + tileSet.StartTile;
            if (index == 4)
            {
                offsetY -= 1;
            }
            if (index == 2)
            {
                offsetX += 1;
            }
            if (index == 1)
            {
                offsetX -= 1;
                offsetY -= 1;
            }
            if (index == 22)
            {
                offsetX -= 1;
                offsetY -= 1;
            }
            if (index == 21)
            {
                offsetY -= 1;
            }

            dwposFix = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;

            if (opposite)
            {
                dwposFix = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }
            if (dwposFix < 0 || dwposFix > mapData.CellDataCount)
                dwposFix = 0;
            auto thisTileFix = CMapDataExt::TileData[idxFix];
            tmpCellDatas[dwposFix] = cellDatas[dwposFix];

            cellDatas[dwposFix].TileIndex = idxFix;
            cellDatas[dwposFix].TileSubIndex = 0;
            cellDatas[dwposFix].Flag.AltIndex = STDHelpers::RandomSelectInt(fixRandom);
            auto newHeight = CViewObjectsExt::CliffConnectionHeight + thisTileFix.TileBlockDatas[0].Height;
            if (newHeight > 14) newHeight = 14;
            if (newHeight < 0) newHeight = 0;
            cellDatas[dwposFix].Height = newHeight;
        }
        else if ((index == 7 || index == 8 || index == 9 || index == 10 || index == 11 || index == 12 || index == 13 || index == 27)
            && (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[20].TileIndices[0] + tileSet.StartTile;

            if (index == 8)
            {
                offsetX -= 1;
            }
            if (index == 9)
            {
                offsetY += 1;
            }
            if (index == 11)
            {
                offsetX -= 1;
                offsetY -= 1;
            }
            if (index == 12)
            {
                offsetX -= 1;
            }
            if (index == 13)
            {
                offsetX -= 1;
                offsetY -= 1;
            }
            if (index == 27)
            {
                offsetY -= 1;
            }

            dwposFix = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;

            if (opposite)
            {
                dwposFix = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }
            if (dwposFix < 0 || dwposFix > mapData.CellDataCount)
                dwposFix = 0;
            auto thisTileFix = CMapDataExt::TileData[idxFix];
            tmpCellDatas[dwposFix] = cellDatas[dwposFix];

            cellDatas[dwposFix].TileIndex = idxFix;
            cellDatas[dwposFix].TileSubIndex = 0;
            cellDatas[dwposFix].Flag.AltIndex = STDHelpers::RandomSelectInt(fixRandom);
            auto newHeight = CViewObjectsExt::CliffConnectionHeight + thisTileFix.TileBlockDatas[0].Height;
            if (newHeight > 14) newHeight = 14;
            if (newHeight < 0) newHeight = 0;
            cellDatas[dwposFix].Height = newHeight;
        }

        if (((index == 5 || index == 6 || index == 7 && opposite || index == 8 && opposite || index == 9 && opposite || index == 10 && opposite || index == 11 && opposite || index == 30 && !opposite)
            && (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13))
            || (index == 18
                && !CViewObjectsExt::LastPlacedCT.Opposite && (CViewObjectsExt::LastPlacedCT.Index == 7
                    || CViewObjectsExt::LastPlacedCT.Index == 8
                    || CViewObjectsExt::LastPlacedCT.Index == 9
                    || CViewObjectsExt::LastPlacedCT.Index == 10
                    || CViewObjectsExt::LastPlacedCT.Index == 11))
            || CViewObjectsExt::LastPlacedCT.Index == 18
            && opposite && (index == 7
                || index == 8
                || index == 9
                || index == 10
                || index == 11))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[24].TileIndices[0] + tileSet.StartTile;
            if (index == 5)
            {
                offsetX += 1;
            }
            if (index == 6)
            {
                offsetX += 1;
                offsetY -= 1;
            }
            if (index == 30)
            {
                offsetX += 1;
                offsetY += 1;
            }
            if (index == 9)
            {
                offsetX += 1;
            }
            if (index == 10)
            {
                offsetX += 1;
            }
            if (index == 11)
            {
                offsetX += 1;
            }
            if (index == 7 || index == 8)
            {
                offsetX += 1;
            }


            dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;

            if (opposite)
            {
                dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;

            }
            if (dwposFix2 < 0 || dwposFix2 > mapData.CellDataCount)
                dwposFix2 = 0;

            auto thisTileFix = CMapDataExt::TileData[idxFix];
            tmpCellDatas[dwposFix2] = cellDatas[dwposFix2];

            cellDatas[dwposFix2].TileIndex = idxFix;
            cellDatas[dwposFix2].TileSubIndex = 0;
            cellDatas[dwposFix2].Flag.AltIndex = STDHelpers::RandomSelectInt(fixRandom);
            auto newHeight = CViewObjectsExt::CliffConnectionHeight + thisTileFix.TileBlockDatas[0].Height;
            if (newHeight > 14) newHeight = 14;
            if (newHeight < 0) newHeight = 0;
            cellDatas[dwposFix2].Height = newHeight;
        }
        else if ((index == 27 && !opposite || index == 10 && !opposite || index == 11 && !opposite || index == 12 && !opposite || index == 13 && !opposite)
            && (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[24].TileIndices[0] + tileSet.StartTile;

            if (index == 11)
            {
                offsetY -= 1;
            }
            if (index == 10)
            {
                offsetX += 1;
            }
            if (index == 13)
            {
                offsetY -= 1;
            }
            if (index == 27)
            {
                offsetY -= 1;
                offsetX += 1;
            }

            dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;

            if (opposite)
            {
                dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;

            }
            if (dwposFix2 < 0 || dwposFix2 > mapData.CellDataCount)
                dwposFix2 = 0;

            auto thisTileFix = CMapDataExt::TileData[idxFix];
            tmpCellDatas[dwposFix2] = cellDatas[dwposFix2];

            cellDatas[dwposFix2].TileIndex = idxFix;
            cellDatas[dwposFix2].TileSubIndex = 0;
            cellDatas[dwposFix2].Flag.AltIndex = STDHelpers::RandomSelectInt(fixRandom);
            auto newHeight = CViewObjectsExt::CliffConnectionHeight + thisTileFix.TileBlockDatas[0].Height;
            if (newHeight > 14) newHeight = 14;
            if (newHeight < 0) newHeight = 0;
            cellDatas[dwposFix2].Height = newHeight;
        }

        if (((index == 0 || index == 1 || index == 21 && !opposite || index == 22 && !opposite || index == 28 && !opposite
            || index == 2 && opposite || index == 3 && opposite || index == 4 && opposite || index == 5 && opposite || index == 6 && opposite)
            && (CViewObjectsExt::LastPlacedCT.Index == 0
                || CViewObjectsExt::LastPlacedCT.Index == 1
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22))
            || index == 18
            && !CViewObjectsExt::LastPlacedCT.Opposite && (CViewObjectsExt::LastPlacedCT.Index == 0
                || CViewObjectsExt::LastPlacedCT.Index == 1
                || CViewObjectsExt::LastPlacedCT.Index == 2
                || CViewObjectsExt::LastPlacedCT.Index == 3
                || CViewObjectsExt::LastPlacedCT.Index == 4)
            || CViewObjectsExt::LastPlacedCT.Index == 18
            && opposite && (index == 0
                || index == 1
                || index == 2
                || index == 3
                || index == 4)
            || index == 30 && opposite && 
            (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[23].TileIndices[0] + tileSet.StartTile;
            if (index == 1 || index == 22)
            {
                offsetX -= 1;
            }
            if (index == 28)
            {
                offsetY += 1;
            }
            if (index == 30)
            {
                offsetX += 1;
                offsetY += 1;
            }
            if (index == 0)
            {
                offsetY += 1;
            }

            dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;
            if (opposite)
            {
                if (index == 1)
                {
                    offsetX += 1;
                    offsetY += 1;
                }
                if (index == 2 || index == 3 || index == 4 || index == 5 || index == 6)
                {
                    offsetY += 1;
                }
                if (index == 6)
                {
                    offsetX -= 1;
                }
                dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }
            if (dwposFix2 < 0 || dwposFix2 > mapData.CellDataCount)
                dwposFix2 = 0;
            auto thisTileFix = CMapDataExt::TileData[idxFix];
            tmpCellDatas[dwposFix2] = cellDatas[dwposFix2];

            cellDatas[dwposFix2].TileIndex = idxFix;
            cellDatas[dwposFix2].TileSubIndex = 0;
            cellDatas[dwposFix2].Flag.AltIndex = STDHelpers::RandomSelectInt(fixRandom);
            auto newHeight = CViewObjectsExt::CliffConnectionHeight + thisTileFix.TileBlockDatas[0].Height;
            if (newHeight > 14) newHeight = 14;
            if (newHeight < 0) newHeight = 0;
            cellDatas[dwposFix2].Height = newHeight;
        }
        if ((index == 14 && !opposite || index == 15 && !opposite || index == 25 && !opposite || index == 16 && !opposite || index == 21 && opposite || index == 22 && opposite || index == 28 && opposite)
            && (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[19].TileIndices[0] + tileSet.StartTile;
            if (index == 25 || CViewObjectsExt::LastPlacedCT.Index == 25)
                idxFix = tileSet.ConnectedTile[26].TileIndices[0] + tileSet.StartTile;

            if (index == 14 || index == 15 || index == 25)
            {
                offsetX += 1;
                offsetY += 1;
            }
            if (index == 16)
            {
                offsetX += 1;
            }
            if (index == 21 || index == 22 || index == 28)
            {
                offsetX += 1;
            }
            dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;

            if (opposite)
            {
                dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }
            if (dwposFix2 < 0 || dwposFix2> mapData.CellDataCount)
                dwposFix2 = 0;

            auto thisTileFix = CMapDataExt::TileData[idxFix];
            tmpCellDatas[dwposFix2] = cellDatas[dwposFix2];

            cellDatas[dwposFix2].TileIndex = idxFix;
            cellDatas[dwposFix2].TileSubIndex = 0;
            cellDatas[dwposFix2].Flag.AltIndex = STDHelpers::RandomSelectInt(fixRandom);
            auto newHeight = CViewObjectsExt::CliffConnectionHeight + thisTileFix.TileBlockDatas[0].Height;
            if (newHeight > 14) newHeight = 14;
            if (newHeight < 0) newHeight = 0;
            cellDatas[dwposFix2].Height = newHeight;
        }
        else if ((index == 14 && opposite || index == 15 && opposite || index == 25 && opposite || index == 16 && opposite || index == 12 && opposite || index == 13 && opposite || index == 27 && opposite)
            && (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[19].TileIndices[0] + tileSet.StartTile;
            if (index == 25 || CViewObjectsExt::LastPlacedCT.Index == 25)
                idxFix = tileSet.ConnectedTile[26].TileIndices[0] + tileSet.StartTile;

            if (index == 14 || index == 15 || index == 25)
            {
                offsetX += 1;
                offsetY += 1;
            }
            if (index == 16)
            {
                offsetY += 1;
            }
            if (index == 12 || index == 13)
            {
                offsetY += 1;
            }
            if (index == 27)
            {
                offsetY += 2;
            }

            dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;

            if (opposite)
            {
                dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }
            if (dwposFix2 < 0 || dwposFix2> mapData.CellDataCount)
                dwposFix2 = 0;

            auto thisTileFix = CMapDataExt::TileData[idxFix];
            tmpCellDatas[dwposFix2] = cellDatas[dwposFix2];

            cellDatas[dwposFix2].TileIndex = idxFix;
            cellDatas[dwposFix2].TileSubIndex = 0;
            cellDatas[dwposFix2].Flag.AltIndex = STDHelpers::RandomSelectInt(fixRandom);
            auto newHeight = CViewObjectsExt::CliffConnectionHeight + thisTileFix.TileBlockDatas[0].Height;
            if (newHeight > 14) newHeight = 14;
            if (newHeight < 0) newHeight = 0;
            cellDatas[dwposFix2].Height = newHeight;
        }

    }
    else if (tileSet.Type == ConnectedTileSetTypes::DirtRoad || tileSet.Type == ConnectedTileSetTypes::CityDirtRoad) //much smarter
    {
        SmallDistance = 3;
        int MiddleDistance = 5;
        int MiddleDistanceHorizontal = 6;
        LargeDistance = 7;

        if (!tileSet.Name)
        {
            handleExit();
            return;
        }

        int index = -1;

        if (NULL == CMapDataExt::TileData)
        {
            handleExit();
            return;
        }

        auto getSuitableBendy = [&tileSet, &getOppositeDirection, &opposite, &cityRoad](int lastDirection, int direction)
            {
                for (int i = 0; i < 24; i++)
                {
                    if (cityRoad && i == 6)
                        continue; //buggy tile
                    if (tileSet.ConnectedTile[i].Direction0 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction1 == direction)
                    {
                        opposite = false;
                        return i;
                    }

                    if (tileSet.ConnectedTile[i].Direction1 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction0 == direction)
                    {
                        opposite = true;
                        return i;
                    }
                }
                return -1;
            };

        //        7 
        //     6     0
        //  5           1
        //     4     2
        //        3

        int loop = 0;
        while (cliffConnectionTiles.empty())
        {
            loop++;
            if (CViewObjectsExt::LastPlacedCT.Index == -1
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Direction1 == facing
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Direction0 == facing)
            {
                if (facing == 1)
                {
                    //offsetPlaceY += 1;
                    //offsetConnectX -= 1;

                    index = 26;
                    if (distance < SmallDistance)
                        index = 27;
                    if (distance > MiddleDistanceHorizontal)
                        index = 24;
                    if ((distance > MiddleDistanceHorizontal) && cityRoad)
                        index = 25;
                }
                else if (facing == 2)
                {  
                    if (distance < SmallDistance)
                        index = 33;
                    else if (distance < MiddleDistance)
                        index = 32;
                    else if (distance < LargeDistance)
                        index = 31;
                    else
                        index = 30;
                }
                else if (facing == 3)
                {
                    index = 37;
                    if (distance < SmallDistance)
                        index = 38;
                    if (distance > MiddleDistanceHorizontal)
                        index = 36;
                }
                else if (facing == 4)
                {
                    if (distance < SmallDistance)
                        index = 44;
                    else if (distance < MiddleDistance)
                        index = 43;
                    else if (distance < LargeDistance)
                        index = 42;
                    else
                        index = 41;
                }
                else if (facing == 5)
                {
                    opposite = true;
                    index = 26;
                    if (distance < SmallDistance)
                        index = 27;
                    if (distance > MiddleDistanceHorizontal)
                        index = 24;
                    if ((distance > MiddleDistanceHorizontal) && cityRoad)
                        index = 25;
                }
                else if (facing == 6)
                {
                    opposite = true;
                    if (distance < SmallDistance)
                        index = 33;
                    else if (distance < MiddleDistance)
                        index = 32;
                    else if (distance < LargeDistance)
                        index = 31;
                    else
                        index = 30;
                }
                else if (facing == 7)
                {
                    opposite = true;
                    index = 37;
                    if (distance < SmallDistance)
                        index = 38;
                    if (distance > MiddleDistanceHorizontal)
                        index = 36;
                }
                else if (facing == 0)
                {
                    opposite = true;
                    if (distance < SmallDistance)
                        index = 44;
                    else if (distance < MiddleDistance)
                        index = 43;
                    else if (distance < LargeDistance)
                        index = 42;
                    else
                        index = 41;
                }
                else
                {
                    handleExit();
                    return;
                }
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 1 && facing == 0 && distance > LargeDistance && !cityRoad)
            {
                index = 29;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 1 && facing == 2 && distance > LargeDistance && !cityRoad)
            {
                index = 28;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 2 && facing == 1 && distance > LargeDistance && !cityRoad)
            {
                index = 35;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 2 && facing == 3 && distance > LargeDistance && !cityRoad)
            {
                index = 34;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 3 && facing == 2 && distance > LargeDistance && !cityRoad)
            {
                index = 39;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 3 && facing == 4 && distance > LargeDistance && !cityRoad)
            {
                index = 40;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 4 && facing == 3 && distance > LargeDistance && !cityRoad)
            {
                index = 46;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 4 && facing == 5 && distance > LargeDistance && !cityRoad)
            {
                index = 45;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 5 && facing == 4 && distance > LargeDistance && !cityRoad)
            {
                opposite = true;
                index = 29;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 5 && facing == 6 && distance > LargeDistance && !cityRoad)
            {
                opposite = true;
                index = 28;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 6 && facing == 5 && distance > LargeDistance && !cityRoad)
            {
                opposite = true;
                index = 35;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 6 && facing == 7 && distance > LargeDistance && !cityRoad)
            {
                opposite = true;
                index = 34;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 7 && facing == 6 && distance > LargeDistance && !cityRoad)
            {
                opposite = true;
                index = 39;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 7 && facing == 0 && distance > LargeDistance && !cityRoad)
            {
                opposite = true;
                index = 40;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 0 && facing == 7 && distance > LargeDistance && !cityRoad)
            {
                opposite = true;
                index = 46;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 0 && facing == 1 && distance > LargeDistance && !cityRoad)
            {
                opposite = true;
                index = 45;
            }
            else
            {
                index = getSuitableBendy(CViewObjectsExt::LastPlacedCT.GetNextDirection(), facing);
            }

            if (CViewObjectsExt::NextCTHeightOffset > 0)
            {
                if (index == 30 || index == 31 || index == 32 || index == 33)
                {
                    if (opposite)
                        index = 49;
                    else
                        index = 47;
                }
                else if (index == 41 || index == 42 || index == 43 || index == 44)
                {
                    if (opposite)
                        index = 50;
                    else
                        index = 48;
                }
            }
            else if (CViewObjectsExt::NextCTHeightOffset < 0)
            {
                if (index == 30 || index == 31 || index == 32 || index == 33)
                {
                    if (opposite)
                        index = 47;
                    else
                        index = 49;
                }
                else if (index == 41 || index == 42 || index == 43 || index == 44)
                {
                    if (opposite)
                        index = 48;
                    else
                        index = 50;
                }
            }
            if (index >= 47)
                thisTileHeightOffest = true;


            if (CViewObjectsExt::LastSuccessfulIndex == -1 && index == -1)
            {
                handleExit();
                return;
            }

            if (index < 0)
            {
                if (getOppositeDirection(CViewObjectsExt::LastPlacedCT.GetNextDirection()) != tileSet.ConnectedTile[CViewObjectsExt::LastSuccessfulIndex].GetThisDirection(CViewObjectsExt::LastSuccessfulOpposite))
                {
                    handleExit();
                    return;
                }

                opposite = CViewObjectsExt::LastSuccessfulOpposite;
                index = CViewObjectsExt::LastSuccessfulIndex;
                if (CViewObjectsExt::LastSuccessfulHeightOffset != 0 || tileSet.ConnectedTile[index].HeightAdjust != 0)
                {
                    handleExit();
                    return;
                }
            }
            else
            {
                CViewObjectsExt::LastSuccessfulOpposite = opposite;
                CViewObjectsExt::LastSuccessfulIndex = index;
            }

            if (!tileSet.ConnectedTile[index].TileIndices.empty())
                for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                {
                    cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                }

            if (loop > 3)
            {
                handleExit();
                return;
            }
        }

        if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 1)
        {
            offsetConnectX -= 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 2)
        {
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 3)
        {
            offsetConnectX += 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 4)
        {
            offsetConnectX += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 5)
        {
            offsetConnectX += 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 6)
        {
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 7)
        {
            offsetConnectX -= 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 0)
        {
            offsetConnectX -= 1;
        }

        if (CViewObjectsExt::PlaceConnectedTile_Start)
        {
            if (facing == 1)
            {
                offsetPlaceY += 1;
                offsetPlaceX -= 1;
            }
            else if (facing == 2)
            {
                offsetPlaceX += 1;
            }
            else if (facing == 3)
            {
                offsetPlaceY += 1;
                offsetPlaceX += 1;
            }
            else if (facing == 6)
            {
                offsetPlaceX += 1;
            }
        }


        CViewObjectsExt::ThisPlacedCT = tileSet.ConnectedTile[index];
        if (!place)
        {
            CViewObjectsExt::LastTempPlacedCTIndex = index;
            CViewObjectsExt::LastTempFacing = facing;
            CViewObjectsExt::CliffConnectionTile = STDHelpers::RandomSelectInt(cliffConnectionTiles, true, index);
        }
        else
        {
            if (index != CViewObjectsExt::LastTempPlacedCTIndex || facing != CViewObjectsExt::LastTempFacing)
            {
                CViewObjectsExt::CliffConnectionTile = STDHelpers::RandomSelectInt(cliffConnectionTiles, true, index);
            }
            CViewObjectsExt::LastTempPlacedCTIndex = -1;
            CViewObjectsExt::LastTempFacing = -1;
        }


        if (opposite)
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y;
        }
        else
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y;
        }

        thisTile = CMapDataExt::TileData[CViewObjectsExt::CliffConnectionTile];


    }
    else if (tileSet.Type == ConnectedTileSetTypes::Shore)
    {
        SmallDistance = 4;
        LargeDistance = 7;

        if (!tileSet.Name)
        {
            handleExit();
            return;
        }

        int index = -1;

        if (NULL == CMapDataExt::TileData)
        {
            handleExit();
            return;
        }

        auto getSuitableBendy = [&tileSet, &getOppositeDirection, &opposite, &distance, &SmallDistance, &LargeDistance](bool lastSide, int lastDirection, int direction)
            {
                bool met = false;
                for (int i = 0; i < 24; i++)
                {
                    if (tileSet.ConnectedTile[i].Direction0 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction1 == direction && tileSet.ConnectedTile[i].Side0 == lastSide)
                    {
                        met = true;
                        opposite = false;
                    }
                    else if (tileSet.ConnectedTile[i].Direction1 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction0 == direction && tileSet.ConnectedTile[i].Side1 == lastSide)
                    {
                        met = true;
                        opposite = true;
                    }
                    if (met)
                    {
                        if (i == 0 && distance < SmallDistance)
                            i = 1;
                        else if (i == 5 && distance < SmallDistance)
                            i = 6;
                        else if (i == 10 && distance < SmallDistance)
                            i = 11;
                        else if (i == 15 && distance < SmallDistance)
                            i = 16;
                        return i;
                    }
                }
                return -1;
            };

        auto getSuitableBendy1357 = [&tileSet, &getOppositeDirection, &opposite, &distance, &SmallDistance, &LargeDistance](bool lastSide, int lastDirection, int direction)
            {
                if (direction == 1 || direction == 5)
                    for (int i = 0; i < 24; i++)
                    {
                        if (i != 3 && i != 7 && i != 12 && i != 18)
                            continue;

                        if (((direction == 1 && i == 3)
                            || (direction == 5 && i == 7)
                            || (direction == 1 && i == 12)
                            || (direction == 5 && i == 18))
                            && tileSet.ConnectedTile[i].Direction0 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Side0 == lastSide)
                        {
                            if (tileSet.ConnectedTile.size() < 25)
                                return -1;

                            opposite = false;

                            if (direction == 5 && i == 7 && distance >= SmallDistance)
                            {
                                opposite = true;
                                i = 24;
                            }
                            if (direction == 5 && i == 24 && distance > LargeDistance)
                            {
                                opposite = true;
                                i = 25;
                            }

                            return i;
                        }
                        else if (((direction == 5 && i == 3)
                            || (direction == 1 && i == 7)
                            || (direction == 5 && i == 12)
                            || (direction == 1 && i == 18))
                            && tileSet.ConnectedTile[i].Direction1 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Side1 == lastSide)
                        {
                            if (tileSet.ConnectedTile.size() < 25)
                                return -1;

                            opposite = true;

                            if (direction == 1 && i == 7 && distance >= SmallDistance)
                            {
                                opposite = false;
                                i = 24;
                            }
                            if (direction == 1 && i == 24 && distance > LargeDistance)
                            {
                                opposite = false;
                                i = 25;
                            }

                            return i;
                        }

                    }
                if (direction == 3 || direction == 7)
                    for (int i = 0; i < 24; i++)
                    {
                        if (i != 2 && i != 8 && i != 13 && i != 17)
                            continue;

                        if (direction == 3 && tileSet.ConnectedTile[i].Direction0 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Side0 == lastSide)
                        {
                            opposite = false;
                            return i;
                        }
                        else if (direction == 7 && tileSet.ConnectedTile[i].Direction1 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Side1 == lastSide)
                        {
                            opposite = true;
                            return i;
                        }

                    }
                return -1;
            };

        //        7 
        //     6     0
        //  5           1
        //     4     2
        //        3

        int loop = 0;
        while (cliffConnectionTiles.empty())
        {
            loop++;
            if (CViewObjectsExt::LastPlacedCT.Index == -1 && forceFront)
            {
                if (facing == 1)
                {
                    index = 3;
                }
                else if (facing == 2)
                {
                    index = 0;
                    if (distance < SmallDistance)
                        index = 1;
                }
                else if (facing == 3)
                {
                    index = 2;
                }
                else if (facing == 4)
                {
                    index = 5;
                    if (distance < SmallDistance)
                        index = 6;
                }
                else if (facing == 5)
                {
                    opposite = true;
                    index = 3;
                }
                else if (facing == 6)
                {
                    opposite = true;
                    index = 0;
                    if (distance < SmallDistance)
                        index = 1;
                }
                else if (facing == 7)
                {
                    opposite = true;
                    index = 2;
                }
                else if (facing == 0)
                {
                    opposite = true;
                    index = 5;
                    if (distance < SmallDistance)
                        index = 6;
                }
                else
                {
                    handleExit();
                    return;
                }
            }
            else if (CViewObjectsExt::LastPlacedCT.Index == -1 && forceBack)
            {
                if (facing == 1)
                {
                    index = 12;
                }
                else if (facing == 2)
                {
                    index = 10;
                    if (distance < SmallDistance)
                        index = 11;
                }
                else if (facing == 3)
                {
                    index = 13;
                }
                else if (facing == 4)
                {
                    index = 15;
                    if (distance < SmallDistance)
                        index = 16;
                }
                else if (facing == 5)
                {
                    opposite = true;
                    index = 12;
                }
                else if (facing == 6)
                {
                    opposite = true;
                    index = 10;
                    if (distance < SmallDistance)
                        index = 11;
                }
                else if (facing == 7)
                {
                    opposite = true;
                    index = 13;
                }
                else if (facing == 0)
                {
                    opposite = true;
                    index = 15;
                    if (distance < SmallDistance)
                        index = 16;
                }
                else
                {
                    handleExit();
                    return;
                }
            }
            else if (facing == 1 || facing == 3 || facing == 5 || facing == 7)
            {
                index = getSuitableBendy1357(CViewObjectsExt::LastPlacedCT.GetNextSide(), CViewObjectsExt::LastPlacedCT.GetNextDirection(), facing);
            }
            else
            {
                index = getSuitableBendy(CViewObjectsExt::LastPlacedCT.GetNextSide(), CViewObjectsExt::LastPlacedCT.GetNextDirection(), facing);
            }

            if (CViewObjectsExt::LastSuccessfulIndex == -1 && index == -1)
            {
                handleExit();
                return;
            }


            if (index < 0)
            {
                if (getOppositeDirection(CViewObjectsExt::LastPlacedCT.GetNextDirection()) != tileSet.ConnectedTile[CViewObjectsExt::LastSuccessfulIndex].GetThisDirection(CViewObjectsExt::LastSuccessfulOpposite))
                {
                    handleExit();
                    return;
                }

                opposite = CViewObjectsExt::LastSuccessfulOpposite;
                index = CViewObjectsExt::LastSuccessfulIndex;
                if (CViewObjectsExt::LastSuccessfulHeightOffset != 0 || tileSet.ConnectedTile[index].HeightAdjust != 0)
                {
                    handleExit();
                    return;
                }
            }
            else
            {
                CViewObjectsExt::LastSuccessfulOpposite = opposite;
                CViewObjectsExt::LastSuccessfulIndex = index;
            }


            if (!tileSet.ConnectedTile[index].TileIndices.empty())
                for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                {
                    cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                }

            if (loop > 3)
            {
                handleExit();
                return;
            }
        }

        if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 1)
        {
            offsetConnectX -= 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 2)
        {
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 3)
        {
            offsetConnectX += 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 4)
        {
            offsetConnectX += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 5)
        {
            offsetConnectX += 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 6)
        {
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 7)
        {
            offsetConnectX -= 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 0)
        {
            offsetConnectX -= 1;
        }

        CViewObjectsExt::ThisPlacedCT = tileSet.ConnectedTile[index];
        if (!place)
        {
            CViewObjectsExt::LastTempPlacedCTIndex = index;
            CViewObjectsExt::LastTempFacing = facing;
            CViewObjectsExt::CliffConnectionTile = STDHelpers::RandomSelectInt(cliffConnectionTiles, true, index);
        }
        else
        {
            if (index != CViewObjectsExt::LastTempPlacedCTIndex || facing != CViewObjectsExt::LastTempFacing)
            {
                CViewObjectsExt::CliffConnectionTile = STDHelpers::RandomSelectInt(cliffConnectionTiles, true, index);
            }
            CViewObjectsExt::LastTempPlacedCTIndex = -1;
            CViewObjectsExt::LastTempFacing = -1;
        }


        if (opposite)
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y;
        }
        else
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y;
        }

        thisTile = CMapDataExt::TileData[CViewObjectsExt::CliffConnectionTile];
    }
    else if (tileSet.Type == ConnectedTileSetTypes::PaveShore)
    {
        facing = CMapDataExt::GetFacing4(CViewObjectsExt::CliffConnectionCoord, cursor);
        SmallDistance = 3;
        LargeDistance = 5;


        if (!tileSet.Name)
        {
            handleExit();
            return;
        }

        int index = -1;

        if (NULL == CMapDataExt::TileData)
        {
            handleExit();
            return;
        }

        auto getSuitableBendy = [&tileSet, &getOppositeDirection, &opposite, &distance, &SmallDistance, &LargeDistance](bool lastSide, int lastDirection, int direction)
            {
                bool met = false;
                for (int i = 0; i < 12; i++)
                {
                    if (tileSet.ConnectedTile[i].Direction0 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction1 == direction && tileSet.ConnectedTile[i].Side0 == lastSide)
                    {
                        met = true;
                        opposite = false;
                    }
                    else if (tileSet.ConnectedTile[i].Direction1 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction0 == direction && tileSet.ConnectedTile[i].Side1 == lastSide)
                    {
                        met = true;
                        opposite = true;
                    }
                    if (met)
                    {
                        return i;
                    }
                }
                return -1;
            };

        //        7 
        //     6     0
        //  5           1
        //     4     2
        //        3

        int loop = 0;
        while (cliffConnectionTiles.empty())
        {
            loop++;
            if (CViewObjectsExt::LastPlacedCT.Index == -1 && forceFront)
            {
                if (facing == 2)
                {
                    index = 4;
                }
                else if (facing == 4)
                {
                    index = 2;
                }
                else if (facing == 6)
                {
                    opposite = true;
                    index = 4;
                }
                else if (facing == 0)
                {
                    opposite = true;
                    index = 2;
                }
                else
                {
                    handleExit();
                    return;
                }
            }
            else if (CViewObjectsExt::LastPlacedCT.Index == -1 && forceBack)
            {
                if (facing == 2)
                {
                    index = 0;
                }
                else if (facing == 4)
                {
                    index = 6;
                }
                else if (facing == 6)
                {
                    opposite = true;
                    index = 0;
                }
                else if (facing == 0)
                {
                    opposite = true;
                    index = 6;
                }
                else
                {
                    handleExit();
                    return;
                }
            }
            else
            {
                index = getSuitableBendy(CViewObjectsExt::LastPlacedCT.GetNextSide(), CViewObjectsExt::LastPlacedCT.GetNextDirection(), facing);
            }

            if (CViewObjectsExt::LastSuccessfulIndex == -1 && index == -1)
            {
                handleExit();
                return;
            }

            if (index < 0)
            {
                if (getOppositeDirection(CViewObjectsExt::LastPlacedCT.GetNextDirection()) != tileSet.ConnectedTile[CViewObjectsExt::LastSuccessfulIndex].GetThisDirection(CViewObjectsExt::LastSuccessfulOpposite))
                {
                    handleExit();
                    return;
                }

                opposite = CViewObjectsExt::LastSuccessfulOpposite;
                index = CViewObjectsExt::LastSuccessfulIndex;
                if (CViewObjectsExt::LastSuccessfulHeightOffset != 0 || tileSet.ConnectedTile[index].HeightAdjust != 0)
                {
                    handleExit();
                    return;
                }
            }
            else
            {
                CViewObjectsExt::LastSuccessfulOpposite = opposite;
                CViewObjectsExt::LastSuccessfulIndex = index;
            }


            if (!tileSet.ConnectedTile[index].TileIndices.empty())
                for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                {
                    cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                }

            if (loop > 3)
            {
                handleExit();
                return;
            }
        }

        if (index == 0 || index == 2 || index == 4 || index == 6)
            MultiPlaceDirection = facing;

        if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 1)
        {
            offsetConnectX -= 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 2)
        {
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 3)
        {
            offsetConnectX += 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 4)
        {
            offsetConnectX += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 5)
        {
            offsetConnectX += 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 6)
        {
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 7)
        {
            offsetConnectX -= 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 0)
        {
            offsetConnectX -= 1;
        }

        CViewObjectsExt::ThisPlacedCT = tileSet.ConnectedTile[index];
        if (!place)
        {
            CViewObjectsExt::LastTempPlacedCTIndex = index;
            CViewObjectsExt::LastTempFacing = facing;
            CViewObjectsExt::CliffConnectionTile = STDHelpers::RandomSelectInt(cliffConnectionTiles, true, index);
        }
        else
        {
            if (index != CViewObjectsExt::LastTempPlacedCTIndex || facing != CViewObjectsExt::LastTempFacing)
            {
                CViewObjectsExt::CliffConnectionTile = STDHelpers::RandomSelectInt(cliffConnectionTiles, true, index);
            }
            CViewObjectsExt::LastTempPlacedCTIndex = -1;
            CViewObjectsExt::LastTempFacing = -1;
        }


        if (opposite)
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y;
        }
        else
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y;
        }

        thisTile = CMapDataExt::TileData[CViewObjectsExt::CliffConnectionTile];
    }
    else if (tileSet.Type == ConnectedTileSetTypes::Highway)
    {
        facing = CMapDataExt::GetFacing4(CViewObjectsExt::CliffConnectionCoord, cursor);
        SmallDistance = 3;
        LargeDistance = 5;

        if (!tileSet.Name)
        {
            handleExit();
            return;
        }

        int index = -1;

        if (NULL == CMapDataExt::TileData)
        {
            handleExit();
            return;
        }

        auto getSuitableBendy = [&tileSet, &getOppositeDirection, &opposite, &distance, &SmallDistance, &LargeDistance](bool lastSide, int lastDirection, int direction)
            {
                bool met = false;
                for (int i = 0; i < 6; i++)
                {
                    if (tileSet.ConnectedTile[i].Direction0 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction1 == direction && tileSet.ConnectedTile[i].Side0 == lastSide)
                    {
                        met = true;
                        opposite = false;
                    }
                    else if (tileSet.ConnectedTile[i].Direction1 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction0 == direction && tileSet.ConnectedTile[i].Side1 == lastSide)
                    {
                        met = true;
                        opposite = true;
                    }
                    if (met)
                    {
                        return i;
                    }
                }
                return -1;
            };

        //        7 
        //     6     0
        //  5           1
        //     4     2
        //        3

        int loop = 0;
        while (cliffConnectionTiles.empty())
        {
            loop++;
            if (CViewObjectsExt::LastPlacedCT.Index == -1)
            {
                if (facing == 2)
                {
                    index = 0;
                }
                else if (facing == 4)
                {
                    index = 1;
                }
                else if (facing == 6)
                {
                    opposite = true;
                    index = 0;
                }
                else if (facing == 0)
                {
                    opposite = true;
                    index = 1;
                }
                else
                {
                    handleExit();
                    return;
                }
            }
            else
            {
                index = getSuitableBendy(CViewObjectsExt::LastPlacedCT.GetNextSide(), CViewObjectsExt::LastPlacedCT.GetNextDirection(), facing);
            }

            if (index == 0 || index == 1)
                IsPavedRoadsRandom = true;

            if (CViewObjectsExt::NextCTHeightOffset > 0)
            {
                if (index == 0)
                {
                    if (opposite)
                        index = 8;
                    else
                        index = 6;
                }
                else if (index == 1)
                {
                    if (opposite)
                        index = 9;
                    else
                        index = 7;
                }
            }
            else if (CViewObjectsExt::NextCTHeightOffset < 0)
            {
                if (index == 0)
                {
                    if (opposite)
                        index = 6;
                    else
                        index = 8;
                }
                else if (index == 1)
                {
                    if (opposite)
                        index = 7;
                    else
                        index = 9;
                }
            }
            if (index >= 6)
                thisTileHeightOffest = true;

            if (CViewObjectsExt::LastSuccessfulIndex == -1 && index == -1)
            {
                handleExit();
                return;
            }

            if (index < 0)
            {
                if (getOppositeDirection(CViewObjectsExt::LastPlacedCT.GetNextDirection()) != tileSet.ConnectedTile[CViewObjectsExt::LastSuccessfulIndex].GetThisDirection(CViewObjectsExt::LastSuccessfulOpposite))
                {
                    handleExit();
                    return;
                }

                opposite = CViewObjectsExt::LastSuccessfulOpposite;
                index = CViewObjectsExt::LastSuccessfulIndex;
                if (CViewObjectsExt::LastSuccessfulHeightOffset != 0 || tileSet.ConnectedTile[index].HeightAdjust != 0)
                {
                    handleExit();
                    return;
                }
            }
            else
            {
                CViewObjectsExt::LastSuccessfulOpposite = opposite;
                CViewObjectsExt::LastSuccessfulIndex = index;
            }


            if (!tileSet.ConnectedTile[index].TileIndices.empty())
                for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                {
                    cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                }

            if (loop > 3)
            {
                handleExit();
                return;
            }
        }

        if (index == 0 || index == 1)
            MultiPlaceDirection = facing;

        if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 1)
        {
            offsetConnectX -= 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 2)
        {
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 3)
        {
            offsetConnectX += 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 4)
        {
            offsetConnectX += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 5)
        {
            offsetConnectX += 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 6)
        {
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 7)
        {
            offsetConnectX -= 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 0)
        {
            offsetConnectX -= 1;
        }

        CViewObjectsExt::ThisPlacedCT = tileSet.ConnectedTile[index];
        if (!place)
        {
            CViewObjectsExt::LastTempPlacedCTIndex = index;
            CViewObjectsExt::LastTempFacing = facing;
            CViewObjectsExt::CliffConnectionTile = STDHelpers::RandomSelectInt(cliffConnectionTiles, true, index);
        }
        else
        {
            if (index != CViewObjectsExt::LastTempPlacedCTIndex || facing != CViewObjectsExt::LastTempFacing)
            {
                CViewObjectsExt::CliffConnectionTile = STDHelpers::RandomSelectInt(cliffConnectionTiles, true, index);
            }
            CViewObjectsExt::LastTempPlacedCTIndex = -1;
            CViewObjectsExt::LastTempFacing = -1;
        }


        if (opposite)
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y;
        }
        else
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y;
        }

        thisTile = CMapDataExt::TileData[CViewObjectsExt::CliffConnectionTile];
    }
    else if (tileSet.Type == ConnectedTileSetTypes::RailRoad)
    {
        int index = -1;
        auto getSuitableBendy = [&tileSet, &getOppositeDirection, &opposite, &distance, &SmallDistance, &LargeDistance](int lastDirection, int direction)
            {
                bool met = false;
                for (int i = 0; i < tileSet.ConnectedTile.size(); i++)
                {
                    if (tileSet.ConnectedTile[i].Direction0 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction1 == direction)
                    {
                        met = true;
                        opposite = false;
                    }
                    else if (tileSet.ConnectedTile[i].Direction1 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction0 == direction)
                    {
                        met = true;
                        opposite = true;
                    }
                    if (met)
                    {
                        return i;
                    }
                }
                return -1;
            };
        if (CViewObjectsExt::LastPlacedCT.Index == -1)
        {
            if (facing == 0)
            {
                index = 2;
            }
            else if (facing == 1)
            {
                index = 1;
            }
            else if (facing == 2)
            {
                index = 3;
            }
            else if (facing == 3)
            {
                opposite = true;
                index = 0;
            }
            else if (facing == 4)
            {
                opposite = true;
                index = 2;
            }
            else if (facing == 5)
            {
                opposite = true;
                index = 1;
            }
            else if (facing == 6)
            {
                opposite = true;
                index = 3;
            }
            else if (facing == 7)
            {
                index = 0;
            }
        }
        else
        {
            index = getSuitableBendy(CViewObjectsExt::LastPlacedCT.GetNextDirection(), facing);       
        }

        if (index < 0)
        {
            if (getOppositeDirection(CViewObjectsExt::LastPlacedCT.GetNextDirection()) != tileSet.ConnectedTile[CViewObjectsExt::LastSuccessfulIndex].GetThisDirection(CViewObjectsExt::LastSuccessfulOpposite))
            {
                handleExit();
                return;
            }

            opposite = CViewObjectsExt::LastSuccessfulOpposite;
            index = CViewObjectsExt::LastSuccessfulIndex;
            if (CViewObjectsExt::LastSuccessfulHeightOffset != 0 || tileSet.ConnectedTile[index].HeightAdjust != 0)
            {
                handleExit();
                return;
            }
        }
        else
        {
            CViewObjectsExt::LastSuccessfulOpposite = opposite;
            CViewObjectsExt::LastSuccessfulIndex = index;
        }

        if (!tileSet.ConnectedTile[index].TileIndices.empty())
        {
            CViewObjectsExt::CliffConnectionTile = tileSet.ConnectedTile[index].TileIndices[0] + tileSet.StartTile;
        }
        else
        {
            handleExit();
            return;
        }
        CViewObjectsExt::ThisPlacedCT = tileSet.ConnectedTile[index];

        if (getOppositeDirection(CViewObjectsExt::LastPlacedCT.GetNextDirection(false))
            == CViewObjectsExt::LastPlacedCT.GetNextDirection(true)) // straight track
        {      
            if (CViewObjectsExt::LastPlacedCT.Index == CViewObjectsExt::ThisPlacedCT.Index)
            {
                if (CViewObjectsExt::LastPlacedCT.GetNextDirection() != facing)
                {
                    handleExit();
                    return;
                }
            }
        }

        if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 1)
        {
            offsetConnectX -= 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 2)
        {
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 3)
        {
            offsetConnectX += 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 4)
        {
            offsetConnectX += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 5)
        {
            offsetConnectX += 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 6)
        {
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 7)
        {
            offsetConnectX -= 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 0)
        {
            offsetConnectX -= 1;
        }

        std::map<int, byte> tmpOverlayDatas;
        std::map<int, word> tmpOverlays;
        if (0 <= CViewObjectsExt::CliffConnectionTile &&
            (!ExtConfigs::ExtOverlays && CViewObjectsExt::CliffConnectionTile < 256)
            || (ExtConfigs::ExtOverlays && CViewObjectsExt::CliffConnectionTile < 65536))
        {
            int repeat = 1;
            if (facing == 0 && index == 2)
            {
                repeat = std::max(abs(CViewObjectsExt::CliffConnectionCoord.X - cursor.X), 1);
                for (int i = 0; i < repeat; ++i)
                {
                    int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX - i;
                    if (pos < CMapData::Instance->CellDataCount)
                    {
                        tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                        tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                    }
                }
            }
            else if (facing == 4 && index == 2)
            {
                repeat = std::max(abs(CViewObjectsExt::CliffConnectionCoord.X - cursor.X), 1);
                for (int i = 0; i < repeat; ++i)
                {
                    int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + i;
                    if (pos < CMapData::Instance->CellDataCount)
                    {
                        tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                        tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                    }
                }
            }
            else if (facing == 2 && index == 3)
            {
                repeat = std::max(abs(CViewObjectsExt::CliffConnectionCoord.Y - cursor.Y), 1);
                for (int i = 0; i < repeat; ++i)
                {
                    int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + i) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX;
                    if (pos < CMapData::Instance->CellDataCount)
                    {
                        tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                        tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                    }
                }
            }
            else if (facing == 6 && index == 3)
            {
                repeat = std::max(abs(CViewObjectsExt::CliffConnectionCoord.Y - cursor.Y), 1);
                for (int i = 0; i < repeat; ++i)
                {
                    int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY - i) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX;
                    if (pos < CMapData::Instance->CellDataCount)
                    {
                        tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                        tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                    }
                }
            }
            else if (facing == 1 && index == 1)
            {
                repeat = std::max(abs(((CViewObjectsExt::CliffConnectionCoord.X - CViewObjectsExt::CliffConnectionCoord.Y)
                    - (cursor.X - cursor.Y)) / 2), 1);
                for (int i = 0; i < repeat; ++i)
                {
                    int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + i) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX - i;
                    if (pos < CMapData::Instance->CellDataCount)
                    {
                        tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                        tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                    }
                }
            }
            else if (facing == 5 && index == 1)
            {
                repeat = std::max(abs(((CViewObjectsExt::CliffConnectionCoord.X - CViewObjectsExt::CliffConnectionCoord.Y)
                    - (cursor.X - cursor.Y)) / 2), 1);
                for (int i = 0; i < repeat; ++i)
                {
                    int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY - i) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + i;
                    if (pos < CMapData::Instance->CellDataCount)
                    {
                        tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                        tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                    }
                }
            }
            else if (facing == 7 && index == 0)
            {
                repeat = std::max(abs(((CViewObjectsExt::CliffConnectionCoord.X + CViewObjectsExt::CliffConnectionCoord.Y)
                    - (cursor.X + cursor.Y)) / 2), 1);
                for (int i = 0; i < repeat; ++i)
                {
                    int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY - i) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX - i;
                    if (pos < CMapData::Instance->CellDataCount)
                    {
                        tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                        tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                    }
                }
            }
            else if (facing == 3 && index == 0)
            {
                repeat = std::max(abs(((CViewObjectsExt::CliffConnectionCoord.X + CViewObjectsExt::CliffConnectionCoord.Y)
                    - (cursor.X + cursor.Y)) / 2), 1);
                for (int i = 0; i < repeat; ++i)
                {
                    int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + i) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + i;
                    if (pos < CMapData::Instance->CellDataCount)
                    {
                        tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                        tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                    }
                }
            }
            else
            {
                int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX;
                if (pos < CMapData::Instance->CellDataCount)
                {
                    tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                    tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                }
            }

            if (place)
            {
                int l = CMapData::Instance->MapWidthPlusHeight; int t = CMapData::Instance->MapWidthPlusHeight; int r = 0; int b = 0;
                for (const auto& [pos, _] : tmpOverlays)
                {
                    int x = CMapData::Instance->GetXFromCoordIndex(pos);
                    int y = CMapData::Instance->GetYFromCoordIndex(pos);
                    if (x < l) l = x;
                    if (y < t) t = y;
                    if (x > r) r = x;
                    if (y > b) b = y;
                }
                // expand 1 size for ore
                mapData.SaveUndoRedoData(true,
                    l - 1,
                    t - 1,
                    r + 2,
                    b + 2
                );
            }

            for (const auto& [pos, _] : tmpOverlays)
            {
                CMapDataExt::GetExtension()->SetNewOverlayAt(pos, CViewObjectsExt::CliffConnectionTile);
                CMapData::Instance->SetOverlayDataAt(pos, 0);
            }

            offsetConnectX *= repeat;
            offsetConnectY *= repeat;
            ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);

            if (place)
            {
                CViewObjectsExt::LastPlacedCTRecords.push_back(CViewObjectsExt::LastPlacedCT);
                CViewObjectsExt::CliffConnectionCoordRecords.push_back(CViewObjectsExt::CliffConnectionCoord);
                CViewObjectsExt::LastCTTileRecords.push_back(CViewObjectsExt::CliffConnectionTile);
                CViewObjectsExt::LastHeightRecords.push_back(CViewObjectsExt::CliffConnectionHeight);
                CViewObjectsExt::LastCTTile = CViewObjectsExt::CliffConnectionTile;

                CViewObjectsExt::LastPlacedCT = CViewObjectsExt::ThisPlacedCT;
                CViewObjectsExt::LastPlacedCT.Opposite = opposite;
                CViewObjectsExt::NextCTHeightOffset = 0;

                if (opposite)
                {
                    CViewObjectsExt::CliffConnectionCoord.X += offsetConnectX + offsetPlaceX + CViewObjectsExt::LastPlacedCT.ConnectionPoint0.X;
                    CViewObjectsExt::CliffConnectionCoord.Y += offsetConnectY + offsetPlaceY + CViewObjectsExt::LastPlacedCT.ConnectionPoint0.Y;
                }
                else
                {
                    CViewObjectsExt::CliffConnectionCoord.X += offsetConnectX + offsetPlaceX + CViewObjectsExt::LastPlacedCT.ConnectionPoint1.X;
                    CViewObjectsExt::CliffConnectionCoord.Y += offsetConnectY + offsetPlaceY + CViewObjectsExt::LastPlacedCT.ConnectionPoint1.Y;
                }
            }
            else
            {
                for (const auto& [pos, ovr] : tmpOverlays)
                {
                    CMapDataExt::GetExtension()->SetNewOverlayAt(pos, ovr);
                }
                for (const auto& [pos, overd] : tmpOverlayDatas)
                {
                    CMapData::Instance->SetOverlayDataAt(pos, overd);
                }
            }
        }

        handleExit();
        return;
    }

    if (CViewObjectsExt::NextCTHeightOffset <= 0 && !thisTileHeightOffest)
    {
        CViewObjectsExt::HeightChanged = false;
        CViewObjectsExt::CliffConnectionHeightAdjust = 0;
    }

    if (CViewObjectsExt::NextCTHeightOffset < 0 && thisTileHeightOffest)
    {
        CViewObjectsExt::CliffConnectionHeightAdjust = -1;
        CViewObjectsExt::HeightChanged = true;
    }

    if (CViewObjectsExt::LastPlacedCT.GetNextHeightOffset() > 0)
    {
        CViewObjectsExt::CliffConnectionHeightAdjust = 1;
        CViewObjectsExt::HeightChanged = true;
    }

    if (CViewObjectsExt::CliffConnectionHeightAdjust == 1 && thisTileHeightOffest && CViewObjectsExt::NextCTHeightOffset < 0)
    {
        CViewObjectsExt::CliffConnectionHeightAdjust = 0;
        CViewObjectsExt::HeightChanged = true;
    }

    if (CViewObjectsExt::LastPlacedCT.GetNextHeightOffset() == 0 && !thisTileHeightOffest)
        CViewObjectsExt::CliffConnectionHeightAdjust = 0;

    int thisTileHeight = thisTile.Height;
    int thisTileWidth = thisTile.Width;
    int HorizontalLoop = 1;
    std::vector<int> PavedRoadRandom = { 0,1,2,3 };

    if (MultiPlaceDirection == 0 || MultiPlaceDirection == 4)
    {
        if (opposite)
            offsetPlaceX -= thisTileHeight * (distanceX - 1);
        else
            offsetConnectX += thisTileHeight * (distanceX - 1);
        thisTileHeight *= distanceX;
    }
    if (MultiPlaceDirection == 2 || MultiPlaceDirection == 6)
    {
        if (opposite)
            offsetPlaceY -= thisTileWidth * (distanceY - 1);
        else
            offsetConnectY += thisTileWidth * (distanceY - 1);
        HorizontalLoop = distanceY / thisTileWidth;
    }


    if (place && tileSet.Type != ConnectedTileSetTypes::Cliff && tileSet.Type != ConnectedTileSetTypes::CityCliff && tileSet.Type != ConnectedTileSetTypes::IceCliff)
    {
        if (MultiPlaceDirection == 0)
            mapData.SaveUndoRedoData(true,
                CViewObjectsExt::CliffConnectionCoord.X - thisTileHeight - 1,
                CViewObjectsExt::CliffConnectionCoord.Y - thisTile.Width - 1,
                CViewObjectsExt::CliffConnectionCoord.X + thisTile.Height + 1,
                CViewObjectsExt::CliffConnectionCoord.Y + thisTile.Width + 1
            );
        else if (MultiPlaceDirection == 4)
            mapData.SaveUndoRedoData(true,
                CViewObjectsExt::CliffConnectionCoord.X - thisTile.Height - 1,
                CViewObjectsExt::CliffConnectionCoord.Y - thisTile.Width - 1,
                CViewObjectsExt::CliffConnectionCoord.X + thisTileHeight + 1,
                CViewObjectsExt::CliffConnectionCoord.Y + thisTile.Width + 1
            );
        else if (MultiPlaceDirection == 2)
            mapData.SaveUndoRedoData(true,
                CViewObjectsExt::CliffConnectionCoord.X - thisTile.Height - 1,
                CViewObjectsExt::CliffConnectionCoord.Y - thisTile.Width - 1,
                CViewObjectsExt::CliffConnectionCoord.X + thisTile.Height + 1,
                CViewObjectsExt::CliffConnectionCoord.Y + thisTile.Width * HorizontalLoop + 1
            );
        else if (MultiPlaceDirection == 6)
            mapData.SaveUndoRedoData(true,
                CViewObjectsExt::CliffConnectionCoord.X - thisTile.Height - 1,
                CViewObjectsExt::CliffConnectionCoord.Y - thisTile.Width * HorizontalLoop - 1,
                CViewObjectsExt::CliffConnectionCoord.X + thisTile.Height + 1,
                CViewObjectsExt::CliffConnectionCoord.Y + thisTile.Width + 1
            );
        else
            mapData.SaveUndoRedoData(true,
                CViewObjectsExt::CliffConnectionCoord.X - thisTile.Height - 1,
                CViewObjectsExt::CliffConnectionCoord.Y - thisTile.Width - 1,
                CViewObjectsExt::CliffConnectionCoord.X + thisTile.Height + 1,
                CViewObjectsExt::CliffConnectionCoord.Y + thisTile.Width + 1
            );
    }
    for (int k = 0; k < HorizontalLoop; k++)
    {
        for (int i = 0; i < thisTileHeight; i++)
        {
            for (int j = 0 + thisTileWidth * k; j < thisTileWidth * (k + 1); j++)
            {
                if (thisTile.TileBlockDatas[subPos].ImageData != NULL)
                {
                    int dwpos, x, y;
                    if (opposite)
                    {
                        x = CliffConnectionCoord.X + i - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX;
                        y = CliffConnectionCoord.Y + j - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY;
                        
                    }
                    else
                    {
                        x = CliffConnectionCoord.X + i - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX;
                        y = CliffConnectionCoord.Y + j - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY;
                    }
                    if (CMapDataExt::IsCoordInFullMap(x, y))
                    {
                        dwpos = y * mapData.MapWidthPlusHeight + x;
                        tmpCellDatas[dwpos] = cellDatas[dwpos];

                        cellDatas[dwpos].TileIndex = CViewObjectsExt::CliffConnectionTile;
                        cellDatas[dwpos].TileSubIndex = subPos;
                        cellDatas[dwpos].Flag.AltIndex = 0;
                        if (IsPavedRoadsRandom)
                            cellDatas[dwpos].Flag.AltIndex = STDHelpers::RandomSelectInt(PavedRoadRandom);

                        auto newHeight = CViewObjectsExt::CliffConnectionHeight + thisTile.TileBlockDatas[subPos].Height + CViewObjectsExt::CliffConnectionHeightAdjust;

                        if (newHeight > 14) newHeight = 14;
                        if (newHeight < 0) newHeight = 0;
                        cellDatas[dwpos].Height = newHeight;
                    }
                }
                subPos++;

                if (subPos >= thisTile.TileBlockCount)
                    subPos = 0;
            }
        }
    }
    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);

    if (!place)
    {
        //undo
        subPos = 0;
        if (dwposFix > -1)
        {
            if (dwposFix < mapData.CellDataCount && tmpCellDatas.find(dwposFix) != tmpCellDatas.end())
            {
                cellDatas[dwposFix] = tmpCellDatas[dwposFix];
            }
        }
        if (dwposFix2 > -1)
        {
            if (dwposFix2 < mapData.CellDataCount && tmpCellDatas.find(dwposFix2) != tmpCellDatas.end())
            {
                cellDatas[dwposFix2] = tmpCellDatas[dwposFix2];
            }
        }
        for (int k = 0; k < HorizontalLoop; k++)
        {
            for (int i = 0; i < thisTileHeight; i++)
            {
                for (int j = 0 + thisTileWidth * k; j < thisTileWidth * (k + 1); j++)
                {
                    if (thisTile.TileBlockDatas[subPos].ImageData != NULL)
                    {
                        int dwpos, x, y;
                        if (opposite)
                        {
                            x = CliffConnectionCoord.X + i - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX;
                            y = CliffConnectionCoord.Y + j - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY;

                        }
                        else
                        {
                            x = CliffConnectionCoord.X + i - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX;
                            y = CliffConnectionCoord.Y + j - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY;
                        }
                        if (CMapDataExt::IsCoordInFullMap(x, y))
                        {
                            dwpos = y * mapData.MapWidthPlusHeight + x;
                            if (tmpCellDatas.find(dwpos) != tmpCellDatas.end())
                            {
                                cellDatas[dwpos] = tmpCellDatas[dwpos];
                            }
                        }
                    }
                    subPos++;

                    if (subPos >= thisTile.TileBlockCount)
                        subPos = 0;
                }
            }
        }
    }

    if (CViewObjectsExt::CliffConnectionHeight < 0)
        CViewObjectsExt::CliffConnectionHeight = 0;
    if (CViewObjectsExt::CliffConnectionHeight > 14)
        CViewObjectsExt::CliffConnectionHeight = 14;

    if (place)
    {
        //update mapPreview
        if (CViewObjectsExt::PlaceConnectedTile_Start)
            CViewObjectsExt::PlaceConnectedTile_Start = false;
        subPos = 0;
        int idx = 0;
        if (dwposFix > -1)
        {
            CMapData::Instance->UpdateMapPreviewAt(dwposFix % mapData.MapWidthPlusHeight, dwposFix / mapData.MapWidthPlusHeight);
            idx++;
        }
        if (dwposFix2 > -1)
        {
            CMapData::Instance->UpdateMapPreviewAt(dwposFix2 % mapData.MapWidthPlusHeight, dwposFix2 / mapData.MapWidthPlusHeight);
            idx++;
        }
        for (int k = 0; k < HorizontalLoop; k++)
        {
            for (int i = 0; i < thisTileHeight; i++)
            {
                for (int j = 0 + thisTileWidth * k; j < thisTileWidth * (k + 1); j++)
                {
                    if (thisTile.TileBlockDatas[subPos].ImageData != NULL)
                    {
                        int dwpos;
                        if (opposite)
                        {
                            dwpos = (CliffConnectionCoord.Y + j - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X + i - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX;
                        }
                        else
                        {
                            dwpos = (CliffConnectionCoord.Y + j - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X + i - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX;
                        }
                        CMapData::Instance->UpdateMapPreviewAt(dwpos % mapData.MapWidthPlusHeight, dwpos / mapData.MapWidthPlusHeight);
                        idx++;
                    }
                    subPos++;

                    if (subPos >= thisTile.TileBlockCount)
                        subPos = 0;
                }
            }
        }


        CViewObjectsExt::LastPlacedCTRecords.push_back(CViewObjectsExt::LastPlacedCT);
        CViewObjectsExt::CliffConnectionCoordRecords.push_back(CViewObjectsExt::CliffConnectionCoord);
        CViewObjectsExt::LastCTTileRecords.push_back(CViewObjectsExt::CliffConnectionTile);
        CViewObjectsExt::LastHeightRecords.push_back(CViewObjectsExt::CliffConnectionHeight);
        CViewObjectsExt::LastCTTile = CViewObjectsExt::CliffConnectionTile;

        CViewObjectsExt::LastPlacedCT = CViewObjectsExt::ThisPlacedCT;
        CViewObjectsExt::LastPlacedCT.Opposite = opposite;
        CViewObjectsExt::LastSuccessfulHeightOffset = CViewObjectsExt::CliffConnectionHeightAdjust;
        CViewObjectsExt::NextCTHeightOffset = 0;
        CViewObjectsExt::HeightChanged = false;

        CViewObjectsExt::CliffConnectionHeight += CViewObjectsExt::CliffConnectionHeightAdjust;

        if (opposite)
        {
            CViewObjectsExt::CliffConnectionCoord.X += offsetConnectX + offsetPlaceX + CViewObjectsExt::LastPlacedCT.ConnectionPoint0.X;
            CViewObjectsExt::CliffConnectionCoord.Y += offsetConnectY + offsetPlaceY + CViewObjectsExt::LastPlacedCT.ConnectionPoint0.Y;
        }
        else
        {
            CViewObjectsExt::CliffConnectionCoord.X += offsetConnectX + offsetPlaceX + CViewObjectsExt::LastPlacedCT.ConnectionPoint1.X;
            CViewObjectsExt::CliffConnectionCoord.Y += offsetConnectY + offsetPlaceY + CViewObjectsExt::LastPlacedCT.ConnectionPoint1.Y;
        }
    }
    CViewObjectsExt::IsInPlaceCliff_OnMouseMove = false;
    return;
}

void CViewObjectsExt::PlaceConnectedTile_OnLButtonDown(int X, int Y)
{
    if (!CMapDataExt::IsCoordInFullMap(X, Y))
        return;
    auto& mapData = CMapData::Instance();
    auto cellDatas = mapData.CellDatas;

    if (CViewObjectsExt::CliffConnectionCoord.X == -1 || CViewObjectsExt::CliffConnectionCoord.Y == -1 || CViewObjectsExt::CliffConnectionHeight == -1)
    {
        CViewObjectsExt::CliffConnectionCoord.X = X;
        CViewObjectsExt::CliffConnectionCoord.Y = Y;
        CViewObjectsExt::CliffConnectionCoordRecords.clear();
        auto dwpos = Y * mapData.MapWidthPlusHeight + X;
        CViewObjectsExt::CliffConnectionHeight = cellDatas[dwpos].Height;
        CViewObjectsExt::PlaceConnectedTile_Start = true;
        CViewObjectsExt::LastTempPlacedCTIndex = -1;
        CViewObjectsExt::LastTempFacing = -1;
        return;
    }

    CViewObjectsExt::PlaceConnectedTile_OnMouseMove(X, Y, true);
}
