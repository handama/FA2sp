#include "Body.h"
#include "../../FA2sp.h"

#include <Drawing.h>
#include <CINI.h>
#include <CMapData.h>

#include "../../Miscs/MultiSelection.h"
#include "../../Helpers/STDHelpers.h"

#include "../CLoading/Body.h"
#include "../CMapData/Body.h"

#include "../../Source/CIsoView.h"
#include "../../Helpers/Translations.h"
#include "../CFinalSunDlg/Body.h"
#include <Miscs/Miscs.h>
#include "../../ExtraWindow/CNewTrigger/CNewTrigger.h"
#include "../../ExtraWindow/CTerrainGenerator/CTerrainGenerator.h"


DEFINE_HOOK(45ADDB, CIsoView_Draw_ObjectInfo, 5)
{
    GET_STACK(HDC, hDC, STACK_OFFS(0x3D52C, 0x3D510));
    auto pIsoView = (CIsoViewExt*)reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
    auto point = CIsoView::GetInstance()->GetCurrentMapCoord(CIsoView::GetInstance()->MouseCurrentPosition);
    auto cell = CMapData::Instance().GetCellAt(point.X + point.Y * CMapData::Instance().MapWidthPlusHeight);

    if (CIsoView::CurrentCommand->Command == 0x1D && MultiSelection::LastAddedCoord.X > -1)
    {
        int X = MultiSelection::LastAddedCoord.X, Y = MultiSelection::LastAddedCoord.Y;
        if (CMapData::Instance().IsCoordInMap(X, Y))
        {
            int XW = abs(point.X - MultiSelection::LastAddedCoord.X) + 1;
            int YW = abs(point.Y - MultiSelection::LastAddedCoord.Y) + 1;
            if (X > point.X)
                X = point.X;
            if (Y > point.Y)
                Y = point.Y;

            CIsoView::MapCoord2ScreenCoord(X, Y);

            pIsoView->DrawLockedCellOutlinePaint(X - CIsoViewExt::drawOffsetX, Y - CIsoViewExt::drawOffsetY, YW, XW, ExtConfigs::MultiSelectionColor, false, hDC, pIsoView->m_hWnd);

        }
    }
    if (CIsoView::CurrentCommand->Command == 0x1F && CTerrainGenerator::RangeFirstCell.X > -1)
    {
        int X = CTerrainGenerator::RangeFirstCell.X, Y = CTerrainGenerator::RangeFirstCell.Y;
        if (CMapData::Instance().IsCoordInMap(X, Y))
        {
            int XW = abs(point.X - CTerrainGenerator::RangeFirstCell.X) + 1;
            int YW = abs(point.Y - CTerrainGenerator::RangeFirstCell.Y) + 1;
            if (X > point.X)
                X = point.X;
            if (Y > point.Y)
                Y = point.Y;

            CIsoView::MapCoord2ScreenCoord(X, Y);

            pIsoView->DrawLockedCellOutlinePaint(X - CIsoViewExt::drawOffsetX, Y - CIsoViewExt::drawOffsetY, YW, XW, ExtConfigs::TerrainGeneratorColor, false, hDC, pIsoView->m_hWnd);

        }
    }


    int X = point.X, Y = point.Y;
    CIsoView::MapCoord2ScreenCoord(X, Y);
    pIsoView->DrawLockedCellOutlinePaintCursor(X - CIsoViewExt::drawOffsetX, Y - CIsoViewExt::drawOffsetY, cell->Height, ExtConfigs::CursorSelectionBound_Color, hDC, pIsoView->m_hWnd, ExtConfigs::CursorSelectionBound_AutoColor);

    if (CIsoView::CurrentCommand->Command == 0x1B)
    {

        RECT rect;
        GetWindowRect(pIsoView->m_hWnd, &rect);
        int leftIndex = 0;

        if (CIsoViewExt::DrawMoneyOnMap)
        {
            leftIndex++;

            if (ExtConfigs::EnableMultiSelection)
                if (!MultiSelection::SelectedCoords.empty())
                    leftIndex++;
        }

        if (CFinalSunApp::Instance().FlatToGround)
            leftIndex++;

        SetTextAlign(hDC, TA_LEFT);

        auto roundToPrecision = [](double value, int precision)
            {
                double multiplier = std::pow(10.0, precision);
                return std::round(value * multiplier) / multiplier;
            };
        int i = 1;
        int tab = 10;
        auto Map = &CMapData::Instance();
        auto& mapIni = CINI::CurrentDocument();
        MultimapHelper mmh;
        mmh.AddINI(&CINI::Rules());
        mmh.AddINI(&CINI::CurrentDocument());

        ppmfc::CString buffer2;

        if (!Map->IsCoordInMap(point.X, point.Y))
        {
            SetTextAlign(hDC, TA_LEFT);
            return 0;
        }

        buffer2.Format(Translations::TranslateOrDefault("ObjectInfo.CurrentCoord",
            "Coordinates: %d, %d, Height: %d"), point.Y, point.X, cell->Height);

        int drawX = X - CIsoViewExt::drawOffsetX + 30;
        int drawY = Y - CIsoViewExt::drawOffsetY - 15;

        ::TextOut(hDC, drawX, drawY + 18 * i++, buffer2, buffer2.GetLength());

        bool bDrawRange = false;
        if ((CIsoView::CurrentCommand->Type >= CViewObjectsExt::ObjectTerrainType::WeaponRange && CIsoView::CurrentCommand->Type <= CViewObjectsExt::ObjectTerrainType::AllRange) || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All)
            bDrawRange = true;

        auto drawRange = [&](float XCenter, float YCenter, float range, COLORREF color, 
            bool isBuilding, ppmfc::CString objectX, ppmfc::CString objectY, bool calculateElevation = false)
            {
                if (range <= 0) return;
                range = range > ExtConfigs::RangeBound_MaxRange ? ExtConfigs::RangeBound_MaxRange : range;
                std::vector<MapCoord> mapCoordsInRange;

                float ElevationIncrement = mmh.GetSingle("ElevationModel", "ElevationIncrement");
                float ElevationIncrementBonus = mmh.GetSingle("ElevationModel", "ElevationIncrementBonus");
                float ElevationBonusCap = mmh.GetSingle("ElevationModel", "ElevationBonusCap");
                if (!isBuilding)
                    ElevationBonusCap = 1.0f;

                for (int x = XCenter - range - 1 - ElevationBonusCap * ElevationIncrementBonus; x < XCenter + range + 1 + ElevationBonusCap * ElevationIncrementBonus; x++)
                {
                    for (int y = YCenter - range - 1 - ElevationBonusCap * ElevationIncrementBonus; y < YCenter + range + 1 + ElevationBonusCap * ElevationIncrementBonus; y++)
                    {
                        if (x > 0 && x < Map->MapWidthPlusHeight && y > 0 && y < Map->MapWidthPlusHeight)
                        {
                            float RangeBonus = 0;
                            if (ExtConfigs::WeaponRangeBound_SubjectToElevation && calculateElevation)
                            {
                                float NumberOfBonuses = (Map->GetCellAt(atoi(objectX), atoi(objectY))->Height - Map->GetCellAt(x, y)->Height) / ElevationIncrement;
                                if (NumberOfBonuses > ElevationBonusCap)
                                    NumberOfBonuses = ElevationBonusCap;
                                if (NumberOfBonuses < -ElevationBonusCap)
                                    NumberOfBonuses = -ElevationBonusCap;
                                RangeBonus = NumberOfBonuses * ElevationIncrementBonus;
                                if (RangeBonus + 2 < 0)
                                    RangeBonus += 2;
                            }

                            float distance = sqrt(((float)y - YCenter) * ((float)y - YCenter) + ((float)x - XCenter) * ((float)x - XCenter));
                            if (range + RangeBonus >= distance)
                            {
                                MapCoord mc;
                                mc.X = x;
                                mc.Y = y;
                                mapCoordsInRange.push_back(mc);
                            }
                        }
                    }
                }
                for (auto& mc : mapCoordsInRange)
                {
                    int x = mc.X;
                    int y = mc.Y;
                    CIsoView::MapCoord2ScreenCoord(x, y);
                    int drawX = x - CIsoViewExt::drawOffsetX;
                    int drawY = y - CIsoViewExt::drawOffsetY;

                    bool s1 = true;
                    bool s2 = true;
                    bool s3 = true;
                    bool s4 = true;

                    for (auto& coord : mapCoordsInRange)
                    {
                        if (coord.X == mc.X - 1 && coord.Y == mc.Y)
                        {
                            s1 = false;
                        }
                        if (coord.X == mc.X + 1 && coord.Y == mc.Y)
                        {
                            s3 = false;
                        }
                        if (coord.X == mc.X && coord.Y == mc.Y - 1)
                        {
                            s4 = false;
                        }

                        if (coord.X == mc.X && coord.Y == mc.Y + 1)
                        {
                            s2 = false;
                        }
                    }
                    pIsoView->DrawLockedCellOutlinePaint(drawX, drawY, 1, 1, color, false, hDC, pIsoView->m_hWnd, s1, s2, s3, s4);
                }
            };

        auto drawWeaponRange = [&](ppmfc::CString ID, ppmfc::CString objectX, ppmfc::CString objectY, bool isBuilding = false, bool secondary = false)
            {
                auto weapon = mmh.GetString(ID, "Primary");
                int color = 0xFFFFFF;
                ppmfc::CString leftLine;
                if (weapon == "") {
                    weapon = mmh.GetString(ID, "Weapon1");
                }
                if (secondary) {
                    weapon = mmh.GetString(ID, "Secondary");

                    if (weapon == "")
                    {
                        weapon = mmh.GetString(ID, "Weapon2");
                    }
                }

                float range = 0.0f;
                float minimumRange = 0.0f;

                bool occupiedBuilding = false;
                if (mmh.GetBool(ID, "CanOccupyFire") && isBuilding && !secondary) {
                    range = mmh.GetSingle("CombatDamage", "OccupyWeaponRange");
                    occupiedBuilding = true;
                }
                else {
                    range = mmh.GetSingle(weapon, "Range");
                    minimumRange = mmh.GetSingle(weapon, "MinimumRange");
                }
                if (range > 0) {
                    float XCenter;
                    float YCenter;
                    if (isBuilding) {
                        const int Index = CMapData::Instance->GetBuildingTypeID(ID);
                        const auto& DataExt = CMapDataExt::BuildingDataExts[Index];
                        XCenter = atoi(objectX) + (DataExt.Height - 1) / 2.0;
                        YCenter = atoi(objectY) + (DataExt.Width - 1) / 2.0;
                        if (occupiedBuilding) {
                            int smallSide = 0;
                            if (DataExt.Height > DataExt.Width)
                                smallSide = DataExt.Width;
                            else
                                smallSide = DataExt.Height;
                            range += (int)(smallSide / 2.0f);
                        }
                    }
                    else
                    {
                        XCenter = atoi(objectX);
                        YCenter = atoi(objectY);
                    }

                    bool useElevation = mmh.GetBool(mmh.GetString(weapon, "Projectile"), "SubjectToElevation");
                    if (!secondary)
                    {
                        SetBkColor(hDC, ExtConfigs::WeaponRangeBound_Color);
                        leftLine = Translations::TranslateOrDefault("ViewWeaponRangeInfo", "Primary Range");
                        ::TextOut(hDC, rect.left + tab, rect.top + 10 + 18 * leftIndex++, leftLine, leftLine.GetLength());

                        if (minimumRange > 0)
                        {
                            SetBkColor(hDC, ExtConfigs::WeaponRangeMinimumBound_Color);
                            leftLine = Translations::TranslateOrDefault("WeaponMinimumRangeInfo", "Minimum Range");
                            ::TextOut(hDC, rect.left + 10 + tab, rect.top + 10 + 18 * leftIndex++, leftLine, leftLine.GetLength());
                        }

                        drawRange(XCenter, YCenter, range, ExtConfigs::WeaponRangeBound_Color, isBuilding, objectX, objectY, useElevation);
                        drawRange(XCenter, YCenter, minimumRange, ExtConfigs::WeaponRangeMinimumBound_Color, isBuilding, objectX, objectY);
                    }
                    else
                    {
                        SetBkColor(hDC, ExtConfigs::SecondaryWeaponRangeBound_Color);
                        leftLine = Translations::TranslateOrDefault("ViewSecondaryWeaponRangeInfo", "Secondary Range");
                        ::TextOut(hDC, rect.left + tab, rect.top + 10 + 18 * leftIndex++, leftLine, leftLine.GetLength());

                        if (minimumRange > 0)
                        {
                            SetBkColor(hDC, ExtConfigs::SecondaryWeaponRangeMinimumBound_Color);
                            leftLine = Translations::TranslateOrDefault("WeaponMinimumRangeInfo", "Minimum Range");
                            ::TextOut(hDC, rect.left + 10 + tab, rect.top + 10 + 18 * leftIndex++, leftLine, leftLine.GetLength());
                        }

                        drawRange(XCenter, YCenter, range, ExtConfigs::SecondaryWeaponRangeBound_Color, isBuilding, objectX, objectY, useElevation);
                        drawRange(XCenter, YCenter, minimumRange, ExtConfigs::SecondaryWeaponRangeMinimumBound_Color, isBuilding, objectX, objectY);
                    }


                    SetBkColor(hDC, 0xFFFFFF);
                }
            };

        auto drawOtherRange = [&](ppmfc::CString ID, ppmfc::CString objectX, ppmfc::CString objectY, int drawCase, bool isBuilding = false)
            {
                float range = 0.0f;
                int color = 0xFFFFFF;
                ppmfc::CString leftLine;

                if (drawCase == CViewObjectsExt::ObjectTerrainType::GapRange) {
                    range = mmh.GetSingle(ID, "GapRadiusInCells");
                    color = ExtConfigs::GapRangeBound_Color;
                    leftLine = Translations::TranslateOrDefault("ViewGapRangeInfo", "Gap Range");
                }
                else if (drawCase == CViewObjectsExt::ObjectTerrainType::SensorsRange) {
                    range = mmh.GetSingle(ID, "SensorsSight");
                    color = ExtConfigs::SensorsRangeBound_Color;
                    leftLine = Translations::TranslateOrDefault("ViewSensorsRangeInfo", "Sensors Range");
                }
                else if (drawCase == CViewObjectsExt::ObjectTerrainType::CloakRange) {
                    range = mmh.GetSingle(ID, "CloakRadiusInCells");
                    color = ExtConfigs::CloakRangeBound_Color;
                    leftLine = Translations::TranslateOrDefault("ViewCloakRangeInfo", "Cloak Range");
                }
                else if (drawCase == CViewObjectsExt::ObjectTerrainType::PsychicRange) {
                    range = mmh.GetSingle(ID, "PsychicDetectionRadius");
                    color = ExtConfigs::PsychicRangeBound_Color;
                    leftLine = Translations::TranslateOrDefault("ViewPsychicRangeInfo", "Psychic Range");
                }
                else if (drawCase == CViewObjectsExt::ObjectTerrainType::GuardRange)
                {
                    range = mmh.GetSingle(ID, "GuardRange");
                    color = ExtConfigs::GuardRangeBound_Color;
                    leftLine = Translations::TranslateOrDefault("ViewGuardRangeInfo", "Guard Range");
                    if (range == 0)
                    {
                        auto weapon = mmh.GetString(ID, "Primary");
                        if (weapon == "") {
                            weapon = mmh.GetString(ID, "Weapon1");
                        }
                        range = mmh.GetSingle(weapon, "Range");
                        if (mmh.GetBool(ID, "CanOccupyFire") && isBuilding) {
                            range = mmh.GetSingle("CombatDamage", "OccupyWeaponRange");
                            const int Index = CMapData::Instance->GetBuildingTypeID(ID);
                            const auto& DataExt = CMapDataExt::BuildingDataExts[Index];
                            int smallSide = 0;
                            if (DataExt.Height > DataExt.Width)
                                smallSide = DataExt.Width;
                            else
                                smallSide = DataExt.Height;
                            range += (int)(smallSide / 2.0f);
                        }
                    }
                }
                else if (drawCase == CViewObjectsExt::ObjectTerrainType::SightRange) {
                    range = mmh.GetSingle(ID, "Sight");
                    color = ExtConfigs::SightRangeBound_Color;
                    leftLine = Translations::TranslateOrDefault("ViewSightRangeInfo", "Sight Range");
                }

                if (range > 0) {
                    float XCenter;
                    float YCenter;
                    if (isBuilding && drawCase != CViewObjectsExt::ObjectTerrainType::SightRange) {
                        const int Index = CMapData::Instance->GetBuildingTypeID(ID);
                        const auto& DataExt = CMapDataExt::BuildingDataExts[Index];
                        XCenter = atoi(objectX) + (DataExt.Height - 1) / 2.0;
                        YCenter = atoi(objectY) + (DataExt.Width - 1) / 2.0;
                    }
                    else
                    {
                        XCenter = atoi(objectX);
                        YCenter = atoi(objectY);
                    }
                    drawRange(XCenter, YCenter, range, color, isBuilding, objectX, objectY);
                    SetBkColor(hDC, color);
                    ::TextOut(hDC, rect.left + tab, rect.top + 10 + 18 * leftIndex++, leftLine, leftLine.GetLength());
                    SetBkColor(hDC, 0xFFFFFF);
                }
            };

        auto displayRanges = [&](ppmfc::CString ID, ppmfc::CString objectX, ppmfc::CString objectY, bool isBuilding = false)
            {
                if ((CIsoView::CurrentCommand->Type >= CViewObjectsExt::ObjectTerrainType::WeaponRange && CIsoView::CurrentCommand->Type <= CViewObjectsExt::ObjectTerrainType::AllRange) || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All)
                {
                    bool All = false;
                    if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::AllRange)
                        All = true;

                    if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::GapRange || All) {
                        drawOtherRange(ID, objectX, objectY, CViewObjectsExt::ObjectTerrainType::GapRange, isBuilding);
                    }
                    if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::SensorsRange || All) {
                        drawOtherRange(ID, objectX, objectY, CViewObjectsExt::ObjectTerrainType::SensorsRange, isBuilding);
                    }
                    if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::CloakRange || All) {
                        drawOtherRange(ID, objectX, objectY, CViewObjectsExt::ObjectTerrainType::CloakRange, isBuilding);
                    }
                    if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::PsychicRange || All) {
                        drawOtherRange(ID, objectX, objectY, CViewObjectsExt::ObjectTerrainType::PsychicRange, isBuilding);
                    }
                    if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::GuardRange || All) {
                        drawOtherRange(ID, objectX, objectY, CViewObjectsExt::ObjectTerrainType::GuardRange, isBuilding);
                    }
                    if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::SightRange || All) {
                        drawOtherRange(ID, objectX, objectY, CViewObjectsExt::ObjectTerrainType::SightRange, isBuilding);
                    }
                    if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::SecondaryWeaponRange || All) {

                        drawWeaponRange(ID, objectX, objectY, isBuilding, true);
                    }
                    if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::WeaponRange || All || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All) {

                        drawWeaponRange(ID, objectX, objectY, isBuilding, false);
                    }

                }
            };
        

        if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Infantry || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Object || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::House || bDrawRange)
        {
            std::vector<int> ids;
            int index = -1;
            auto& ini = CMapData::Instance->INI;
            if (auto pSection = ini.GetSection("Infantry"))
            {
                for (auto& pair : pSection->GetEntities())
                {
                    index++;
                    auto atoms = STDHelpers::SplitString(pair.second, 4);
                    int thisY = atoi(atoms[3]);
                    int thisX = atoi(atoms[4]);
                    if (point.X == thisX && point.Y == thisY)
                    {
                        ids.push_back(index);
                    }
                }
            }
            for (auto id : ids)
            {
                //for (int subpos = 0; subpos < 3; subpos++)
                //{
                ppmfc::CString line1;
                ppmfc::CString line2;
                ppmfc::CString line3;
                ppmfc::CString line4;
                ppmfc::CString line5;
                ppmfc::CString line6;
                //auto id = cell->Infantry[subpos];
                if (id > -1)
                {
                    CInfantryData object;
                    Map->GetInfantryData(id, object);

                    if (bDrawRange)
                        displayRanges(object.TypeID, object.X, object.Y);

                    if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::House || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All)
                    {
                        ppmfc::CString leftLine1;
                        ppmfc::CString leftLine2;
                        //int objCount = 0;
                        int objThisCount = 0;

                        for (auto& inf : mapIni.GetSection("Infantry")->GetEntities())
                        {
                            auto atoms = STDHelpers::SplitString(inf.second);
                            if (atoms.size() > 4)
                            {
                                if (atoms[0] == object.House)
                                {
                                    //objCount++;
                                    if (atoms[1] == object.TypeID)
                                        objThisCount++;
                                }
                            }
                        }
                        int cost = mmh.GetInteger(object.TypeID, "Cost");
                        ppmfc::CString house;
                        Miscs::ParseHouseName(&house, object.House, true);

                        leftLine1.Format(Translations::TranslateOrDefault("ObjectInfo.HouseInfo.House", "House: %s:"), house);

                        leftLine2.Format(Translations::TranslateOrDefault("ObjectInfo.HouseInfo.Infantry",
                            "Infantry:  %s (%s), Count: %d, Cost: %d"), CMapData::GetUIName(object.TypeID), object.TypeID, objThisCount, objThisCount * cost);

                        ::TextOut(hDC, rect.left + 10, rect.top + 10 + 18 * leftIndex++, leftLine1, leftLine1.GetLength());
                        ::TextOut(hDC, rect.left + 10 + tab, rect.top + 10 + 18 * leftIndex++, leftLine2, leftLine2.GetLength());
                    }
                    if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Infantry || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Object || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All)
                    {
                        int strength = (int)((double)mmh.GetInteger(object.TypeID, "Strength") * (double)(atoi(object.Health) / 256.0));
                        if (strength == 0 && atoi(object.Health) > 0)
                            strength = 1;

                        ppmfc::CString house;
                        ppmfc::CString veteran;
                        Miscs::ParseHouseName(&house, object.House, true);
                        if (atoi(object.VeterancyPercentage) < 100)
                            veteran = Translations::TranslateOrDefault("ObjectInfo.Veterancy.Rookie",
                                "Rookie");
                        else if (atoi(object.VeterancyPercentage) < 200)
                            veteran = Translations::TranslateOrDefault("ObjectInfo.Veterancy.Veteran",
                                "Veteran");
                        else
                            veteran = Translations::TranslateOrDefault("ObjectInfo.Veterancy.Elite",
                                "Elite");

                        auto tag = STDHelpers::SplitString(mapIni.GetString("Tags", object.Tag));
                        ppmfc::CString tagName = "";
                        if (tag.size() > 1)
                            tagName = tag[1];

                        line1.Format(Translations::TranslateOrDefault("ObjectInfo.Infantry.1", 
                            "Infantry: %s (%s), ID: %d, Subcell: %s")
                            , CMapData::GetUIName(object.TypeID), object.TypeID, id, object.SubCell);
                        line2.Format(Translations::TranslateOrDefault("ObjectInfo.Infantry.2", 
                            "House: %s")
                            , house);
                        line3.Format(Translations::TranslateOrDefault("ObjectInfo.Infantry.3", 
                            "Strength: %d (%s), Mission: %s")
                            , strength, object.Health, object.Status);
                        line4.Format(Translations::TranslateOrDefault("ObjectInfo.Infantry.4", 
                            "Veterancy: %s (%s), Group: %s")
                            , veteran, object.VeterancyPercentage, object.Group);
                        line5.Format(Translations::TranslateOrDefault("ObjectInfo.Infantry.5", 
                            "Tag: %s (%s), RecruitA: %s, RecruitB: %s")
                            , tagName, object.Tag, object.AutoNORecruitType, object.AutoYESRecruitType);
                        line6.Format(Translations::TranslateOrDefault("ObjectInfo.Infantry.6", 
                            "On Bridge: %s")
                            , object.IsAboveGround);
                        ::SetBkColor(hDC, RGB(0, 255, 255));
                        ::TextOut(hDC, drawX, drawY + 18 * i++, line1, line1.GetLength());
                        ::SetBkColor(hDC, RGB(0xFF, 0xFF, 0xFF));
                        ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line2, line2.GetLength());
                        ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line3, line3.GetLength());
                        ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line4, line4.GetLength());
                        ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line5, line5.GetLength());
                        ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line6, line6.GetLength());

                    }
                }
            }

        }
        if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Vehicle || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Object || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::House || bDrawRange)
        {
            std::vector<int> ids;
            int index = -1;
            auto& ini = CMapData::Instance->INI;
            if (auto pSection = ini.GetSection("Units"))
            {
                for (auto& pair : pSection->GetEntities())
                {
                    index++;
                    auto atoms = STDHelpers::SplitString(pair.second, 4);
                    int thisY = atoi(atoms[3]);
                    int thisX = atoi(atoms[4]);
                    if (point.X == thisX && point.Y == thisY)
                    {
                        ids.push_back(index);
                    }
                }
            }
            for (auto id : ids)
            {
                ppmfc::CString line1;
                ppmfc::CString line2;
                ppmfc::CString line3;
                ppmfc::CString line4;
                ppmfc::CString line5;
                ppmfc::CString line6;
                //auto id = cell->Unit;

                if (id > -1)
                {
                    CUnitData object;
                    Map->GetUnitData(id, object);

                    if (bDrawRange)
                        displayRanges(object.TypeID, object.X, object.Y);

                    if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::House || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All)
                    {
                        ppmfc::CString leftLine1;
                        ppmfc::CString leftLine2;
                        //int objCount = 0;
                        int objThisCount = 0;

                        for (auto& inf : mapIni.GetSection("Units")->GetEntities())
                        {
                            auto atoms = STDHelpers::SplitString(inf.second);
                            if (atoms.size() > 4)
                            {
                                if (atoms[0] == object.House)
                                {
                                    //objCount++;
                                    if (atoms[1] == object.TypeID)
                                        objThisCount++;
                                }
                            }
                        }
                        int cost = mmh.GetInteger(object.TypeID, "Cost");
                        ppmfc::CString house;
                        Miscs::ParseHouseName(&house, object.House, true);

                        leftLine1.Format(Translations::TranslateOrDefault("ObjectInfo.HouseInfo.House", "House: %s:"), house);
                        leftLine2.Format(Translations::TranslateOrDefault("ObjectInfo.HouseInfo.Vehicle", 
                            "Vehicle:  %s (%s), Count: %d, Cost: %d")
                            , CMapData::GetUIName(object.TypeID), object.TypeID, objThisCount, objThisCount * cost);

                        ::TextOut(hDC, rect.left + 10, rect.top + 10 + 18 * leftIndex++, leftLine1, leftLine1.GetLength());
                        ::TextOut(hDC, rect.left + 10 + tab, rect.top + 10 + 18 * leftIndex++, leftLine2, leftLine2.GetLength());
                    }
                    if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Vehicle || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Object || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All)
                    {
                        int strength = (int)((double)mmh.GetInteger(object.TypeID, "Strength") * (double)(atoi(object.Health) / 256.0));
                        if (strength == 0 && atoi(object.Health) > 0)
                            strength = 1;

                        ppmfc::CString house;
                        ppmfc::CString veteran;
                        Miscs::ParseHouseName(&house, object.House, true);

                        if (atoi(object.VeterancyPercentage) < 100)
                            veteran = Translations::TranslateOrDefault("ObjectInfo.Veterancy.Rookie",
                                "Rookie");
                        else if (atoi(object.VeterancyPercentage) < 200)
                            veteran = Translations::TranslateOrDefault("ObjectInfo.Veterancy.Veteran",
                                "Veteran");
                        else
                            veteran = Translations::TranslateOrDefault("ObjectInfo.Veterancy.Elite",
                                "Elite");

                        auto tag = STDHelpers::SplitString(mapIni.GetString("Tags", object.Tag));
                        ppmfc::CString tagName = "";
                        if (tag.size() > 1)
                            tagName = tag[1];

                        line1.Format(Translations::TranslateOrDefault("ObjectInfo.Vehicle.1",
                            "Vehicle: %s (%s), ID: %d")
                            , CMapData::GetUIName(object.TypeID), object.TypeID, id);
                        line2.Format(Translations::TranslateOrDefault("ObjectInfo.Vehicle.2",
                            "House: %s")
                            , house);
                        line3.Format(Translations::TranslateOrDefault("ObjectInfo.Vehicle.3",
                            "Strength: %d (%s), Mission: %s")
                            , strength, object.Health, object.Status);
                        line4.Format(Translations::TranslateOrDefault("ObjectInfo.Vehicle.4",
                            "Veterancy: %s (%s), Group: %s")
                            , veteran, object.VeterancyPercentage, object.Group);
                        line5.Format(Translations::TranslateOrDefault("ObjectInfo.Vehicle.5",
                            "Tag: %s (%s), RecruitA: %s, RecruitB: %s")
                            , tagName, object.Tag, object.AutoNORecruitType, object.AutoYESRecruitType);
                        line6.Format(Translations::TranslateOrDefault("ObjectInfo.Vehicle.6",
                            "On Bridge: %s, Follows: %s")
                            , object.IsAboveGround, object.FollowsIndex);
                        ::SetBkColor(hDC, RGB(0, 255, 255));
                        ::TextOut(hDC, drawX, drawY + 18 * i++, line1, line1.GetLength());
                        ::SetBkColor(hDC, RGB(0xFF, 0xFF, 0xFF));
                        ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line2, line2.GetLength());
                        ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line3, line3.GetLength());
                        ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line4, line4.GetLength());
                        ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line5, line5.GetLength());
                        ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line6, line6.GetLength());

                    }
                   
                }
            }
        }
        if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Aircraft || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Object || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::House || bDrawRange)
        {
            std::vector<int> ids;
            int index = -1;
            auto& ini = CMapData::Instance->INI;
            if (auto pSection = ini.GetSection("Aircraft"))
            {
                for (auto& pair : pSection->GetEntities())
                {
                    index++;
                    auto atoms = STDHelpers::SplitString(pair.second, 4);
                    int thisY = atoi(atoms[3]);
                    int thisX = atoi(atoms[4]);
                    if (point.X == thisX && point.Y == thisY)
                    {
                        ids.push_back(index);
                    }
                }
            }
            for (auto id : ids)
            {
                ppmfc::CString line1;
                ppmfc::CString line2;
                ppmfc::CString line3;
                ppmfc::CString line4;
                ppmfc::CString line5;
                //auto id = cell->Aircraft;

                if (id > -1)
                {
                    CAircraftData object;
                    Map->GetAircraftData(id, object);

                    if (bDrawRange)
                        displayRanges(object.TypeID, object.X, object.Y);

                    if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::House || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All)
                    {
                        ppmfc::CString leftLine1;
                        ppmfc::CString leftLine2;
                        //int objCount = 0;
                        int objThisCount = 0;

                        for (auto& inf : mapIni.GetSection("Aircraft")->GetEntities())
                        {
                            auto atoms = STDHelpers::SplitString(inf.second);
                            if (atoms.size() > 4)
                            {
                                if (atoms[0] == object.House)
                                {
                                    //objCount++;
                                    if (atoms[1] == object.TypeID)
                                        objThisCount++;
                                }
                            }
                        }
                        int cost = mmh.GetInteger(object.TypeID, "Cost");
                        ppmfc::CString house;
                        Miscs::ParseHouseName(&house, object.House, true);

                        leftLine1.Format(Translations::TranslateOrDefault("ObjectInfo.HouseInfo.House", "House: %s:"), house);
                        leftLine2.Format(Translations::TranslateOrDefault("ObjectInfo.HouseInfo.Aircraft", 
                            "Aircraft:  %s (%s), Count: %d, Cost: %d"), CMapData::GetUIName(object.TypeID), object.TypeID, objThisCount, objThisCount * cost);

                        ::TextOut(hDC, rect.left + 10, rect.top + 10 + 18 * leftIndex++, leftLine1, leftLine1.GetLength());
                        ::TextOut(hDC, rect.left + 10 + tab, rect.top + 10 + 18 * leftIndex++, leftLine2, leftLine2.GetLength());
                    }
                    if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Aircraft || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Object || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All)
                    {
                        int strength = (int)((double)mmh.GetInteger(object.TypeID, "Strength") * (double)(atoi(object.Health) / 256.0));
                        if (strength == 0 && atoi(object.Health) > 0)
                            strength = 1;

                        ppmfc::CString house;
                        ppmfc::CString veteran;
                        Miscs::ParseHouseName(&house, object.House, true);
                        if (atoi(object.VeterancyPercentage) < 100)
                            veteran = Translations::TranslateOrDefault("ObjectInfo.Veterancy.Rookie",
                                "Rookie");
                        else if (atoi(object.VeterancyPercentage) < 200)
                            veteran = Translations::TranslateOrDefault("ObjectInfo.Veterancy.Veteran",
                                "Veteran");
                        else
                            veteran = Translations::TranslateOrDefault("ObjectInfo.Veterancy.Elite",
                                "Elite");


                        auto tag = STDHelpers::SplitString(mapIni.GetString("Tags", object.Tag));
                        ppmfc::CString tagName = "";
                        if (tag.size() > 1)
                            tagName = tag[1];

                        line1.Format(Translations::TranslateOrDefault("ObjectInfo.Aircraft.1",
                            "Aircraft: %s (%s), ID: %d")
                            , CMapData::GetUIName(object.TypeID), object.TypeID, id);
                        line2.Format(Translations::TranslateOrDefault("ObjectInfo.Aircraft.2",
                            "House: %s")
                            , house);
                        line3.Format(Translations::TranslateOrDefault("ObjectInfo.Aircraft.3",
                            "Strength: %d (%s), Mission: %s")
                            , strength, object.Health, object.Status);
                        line4.Format(Translations::TranslateOrDefault("ObjectInfo.Aircraft.4",
                            "Veteranc: %s (%s), Group: %s")
                            , veteran, object.VeterancyPercentage, object.Group);
                        line5.Format(Translations::TranslateOrDefault("ObjectInfo.Aircraft.5",
                            "Tag: %s (%s), RecruitA: %s, RecruitB: %s")
                            , tagName, object.Tag, object.AutoNORecruitType, object.AutoYESRecruitType);
                        ::SetBkColor(hDC, RGB(0, 255, 255));
                        ::TextOut(hDC, drawX, drawY + 18 * i++, line1, line1.GetLength());
                        ::SetBkColor(hDC, RGB(0xFF, 0xFF, 0xFF));
                        ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line2, line2.GetLength());
                        ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line3, line3.GetLength());
                        ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line4, line4.GetLength());
                        ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line5, line5.GetLength());
                    }
                }
            }
        }
        if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Building || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Object || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::House || bDrawRange)
        {
            ppmfc::CString line1;
            ppmfc::CString line2;
            ppmfc::CString line3;
            ppmfc::CString line4;
            ppmfc::CString line5;

            std::vector<int> ids;
            int index = -1;
            auto& ini = CMapData::Instance->INI;
            if (auto pSection = ini.GetSection("Structures"))
            {
                bool found = false;
                for (auto& pair : pSection->GetEntities())
                {
                    index++;
                    auto atoms = STDHelpers::SplitString(pair.second, 4);
                    const int Index = CMapData::Instance->GetBuildingTypeID(atoms[1]);
                    const int Y = atoi(atoms[3]);
                    const int X = atoi(atoms[4]);
                    const auto& DataExt = CMapDataExt::BuildingDataExts[Index];

                    if (!DataExt.IsCustomFoundation())
                    {
                        for (int dx = 0; dx < DataExt.Height; ++dx)
                        {
                            for (int dy = 0; dy < DataExt.Width; ++dy)
                            {
                                MapCoord coord = { X + dx, Y + dy };
                                if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
                                {
                                    if (coord.X == point.X && coord.Y == point.Y)
                                    {
                                        ids.push_back(index);
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        for (const auto& block : *DataExt.Foundations)
                        {
                            MapCoord coord = { X + block.Y, Y + block.X };
                            if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
                            {
                                if (coord.X == point.X && coord.Y == point.Y)
                                {
                                    ids.push_back(index);
                                }
                            }

                        }
                    }
                }
            }
            for (auto id : ids)
            {
                CBuildingData object;
                CMapDataExt::GetBuildingDataByIniID(id, object);

                if (bDrawRange)
                    displayRanges(object.TypeID, object.X, object.Y, true);

                if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::House || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All)
                {
                    ppmfc::CString leftLine1;
                    ppmfc::CString leftLine2;
                    ppmfc::CString leftLine3;
                    //int objCount = 0;
                    int objThisCount = 0;
                    int housePower = 0;
                    int houseLoad = 0;

                    int strength = (int)((double)mmh.GetInteger(object.TypeID, "Strength") * (double)(atoi(object.Health) / 256.0));
                    if (strength == 0 && atoi(object.Health) > 0)
                        strength = 1;

                    int cost = mmh.GetInteger(object.TypeID, "Cost");
                    ppmfc::CString house;
                    Miscs::ParseHouseName(&house, object.House, true);
                    int power = CMapDataExt::GetStructurePower(object).TotalPower;

                    for (auto& str : mapIni.GetSection("Structures")->GetEntities())
                    {
                        auto atoms = STDHelpers::SplitString(str.second);
                        if (atoms.size() > 4)
                        {
                            if (atoms[0] == object.House)
                            {
                                auto ppower = CMapDataExt::GetStructurePower(str.second);
                                housePower += ppower.Output;
                                houseLoad -= ppower.Drain;
                                if (atoms[1] == object.TypeID)
                                    objThisCount++;
                            }
                        }
                    }

                    leftLine1.Format(Translations::TranslateOrDefault("ObjectInfo.HouseInfo.House", "House: %s:"), house);
                    leftLine2.Format(Translations::TranslateOrDefault("ObjectInfo.HouseInfo.Structure",
                        "Structure:  %s (%s), Count: %d, Cost: %d"), CMapData::GetUIName(object.TypeID), object.TypeID, objThisCount, objThisCount * cost);
                    leftLine3.Format(Translations::TranslateOrDefault("ObjectInfo.HouseInfo.Power",
                        "Total Power: %d, Output: %d, Drain: %d"), housePower - houseLoad, housePower, houseLoad);


                    ::TextOut(hDC, rect.left + 10, rect.top + 10 + 18 * leftIndex++, leftLine1, leftLine1.GetLength());
                    ::TextOut(hDC, rect.left + 10 + tab, rect.top + 10 + 18 * leftIndex++, leftLine2, leftLine2.GetLength());
                    if (housePower - houseLoad >= 100)
                    {
                        ::SetBkColor(hDC, RGB(0, 255, 0));
                        ::TextOut(hDC, rect.left + 10 + tab, rect.top + 10 + 18 * leftIndex++, leftLine3, leftLine3.GetLength());
                        ::SetBkColor(hDC, RGB(255, 255, 255));
                    }
                    else if (housePower - houseLoad >= 0)
                    {
                        ::SetBkColor(hDC, RGB(255, 255, 0));
                        ::TextOut(hDC, rect.left + 10 + tab, rect.top + 10 + 18 * leftIndex++, leftLine3, leftLine3.GetLength());
                        ::SetBkColor(hDC, RGB(255, 255, 255));
                    }
                    else
                    {
                        ::SetBkColor(hDC, RGB(255, 0, 0));
                        ::TextOut(hDC, rect.left + 10 + tab, rect.top + 10 + 18 * leftIndex++, leftLine3, leftLine3.GetLength());
                        ::SetBkColor(hDC, RGB(255, 255, 255));
                    }

                }
                if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Building || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Object || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All)
                {
                    int strength = (int)((double)mmh.GetInteger(object.TypeID, "Strength") * (double)(atoi(object.Health) / 256.0));
                    if (strength == 0 && atoi(object.Health) > 0)
                        strength = 1;


                    int power = CMapDataExt::GetStructurePower(object).TotalPower;

                    ppmfc::CString house;
                    Miscs::ParseHouseName(&house, object.House, true);

                    auto tag = STDHelpers::SplitString(mapIni.GetString("Tags", object.Tag));
                    ppmfc::CString tagName = "";
                    if (tag.size() > 1)
                        tagName = tag[1];

                    line1.Format(Translations::TranslateOrDefault("ObjectInfo.Structure.1",
                        "Structure: %s (%s), ID: %d")
                        , CMapData::GetUIName(object.TypeID), object.TypeID, id);
                    line2.Format(Translations::TranslateOrDefault("ObjectInfo.Structure.2",
                        "House: %s")
                        , house);
                    line3.Format(Translations::TranslateOrDefault("ObjectInfo.Structure.3",
                        "Strength: %d (%s), AI Repair: %s")
                        , strength, object.Health, object.AIRepairable);
                    if (power != 0)
                        line3.Format(Translations::TranslateOrDefault("ObjectInfo.Structure.4",
                            "Strength: %d (%s), Power: %d, AI Repair: %s")
                            , strength, object.Health, power, object.AIRepairable);
                    line4.Format(Translations::TranslateOrDefault("ObjectInfo.Structure.5",
                        "Tag: %s (%s), Spot Light: %s, Upgrade Count: %s")
                        , tagName, object.Tag, object.SpotLight, object.Upgrades);
                    line5.Format(Translations::TranslateOrDefault("ObjectInfo.Structure.6",
                        "Upgrade 1: %s, Upgrade 2: %s, Upgrade 3: %s")
                        , object.Upgrade1, object.Upgrade2, object.Upgrade3);
                    ::SetBkColor(hDC, RGB(0, 255, 255));
                    ::TextOut(hDC, drawX, drawY + 18 * i++, line1, line1.GetLength());
                    ::SetBkColor(hDC, RGB(0xFF, 0xFF, 0xFF));
                    ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line2, line2.GetLength());
                    ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line3, line3.GetLength());
                    ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line4, line4.GetLength());
                    if (!(object.Upgrade1 == "None" && object.Upgrade2 == "None" && object.Upgrade3 == "None" && object.Upgrades == "0"))
                        ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line5, line5.GetLength());
                }
            }
        }
        if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::BaseNode || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Object || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::House)
        {
            ppmfc::CString line1;
            ppmfc::CString line2;

            ppmfc::CString targetHouse;
            ppmfc::CString targetNode;
            std::vector<int> ids;

            auto& ini = CMapData::Instance->INI;
            if (auto pSection = ini.GetSection("Houses"))
            {
                for (auto& pair : pSection->GetEntities())
                {
                    int nodeCount = ini.GetInteger(pair.second, "NodeCount", 0);
                    if (nodeCount > 0)
                    {
                        for (int i = 0; i < nodeCount; i++)
                        {
                            char key[10];
                            sprintf(key, "%03d", i);
                            auto value = ini.GetString(pair.second, key, "");
                            if (value == "")
                                continue;
                            auto atoms = STDHelpers::SplitString(value);
                            if (atoms.size() < 3)
                                continue;

                            const int Index = CMapData::Instance->GetBuildingTypeID(atoms[0]);
                            const int Y = atoi(atoms[1]);
                            const int X = atoi(atoms[2]);
                            bool found = false;
                            const auto& DataExt = CMapDataExt::BuildingDataExts[Index];

                            if (!DataExt.IsCustomFoundation())
                            {
                                for (int dx = 0; dx < DataExt.Height; ++dx)
                                {
                                    for (int dy = 0; dy < DataExt.Width; ++dy)
                                    {
                                        MapCoord coord = { X + dx, Y + dy };
                                        if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
                                        {
                                            if (coord.X == point.X && coord.Y == point.Y)
                                            {
                                                ids.push_back(i);
                                                targetHouse = pair.second;
                                                targetNode = value;
                                            }

                                        }

                                    }
                                }
                            }
                            else
                            {
                                for (const auto& block : *DataExt.Foundations)
                                {
                                    MapCoord coord = { X + block.Y, Y + block.X };
                                    if (CMapData::Instance->IsCoordInMap(coord.X, coord.Y))
                                    {
                                        if (coord.X == point.X && coord.Y == point.Y)
                                        {
                                            ids.push_back(i);
                                            targetHouse = pair.second;
                                            targetNode = value;
                                        }
                                    }

                                }
                            }

                        }
                    }
                }
            }
            for (auto id : ids)
            {
                auto atoms = STDHelpers::SplitString(targetNode);

                if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::House)
                {
                    ppmfc::CString leftLine1;
                    ppmfc::CString leftLine2;
                    ppmfc::CString leftLine3;
                    ppmfc::CString leftLine4;
                    ppmfc::CString leftLine5;
                    ppmfc::CString targetHouse2 = targetHouse;

                    int objThisCount = 0;
                    int cost = mmh.GetInteger(atoms[0], "Cost");

                    bool stopPowerCheck = false;
                    int housePowerTotal = 0;
                    int houseLoadTotal = 0;
                    int housePowerThis = 0;
                    int houseLoadThis = 0;
                    std::vector<int> powerShortage;


                    int nodeCount = ini.GetInteger(targetHouse2, "NodeCount", 0);
                    if (nodeCount > 0)
                    {
                        for (int i = 0; i < nodeCount; i++)
                        {
                            char key[10];
                            sprintf(key, "%03d", i);
                            auto value = ini.GetString(targetHouse2, key, "");
                            if (value == "")
                                continue;
                            auto atoms2 = STDHelpers::SplitString(value);
                            if (atoms2.size() < 3)
                                continue;
                            if (atoms2[0] == atoms[0])
                                objThisCount++;

                            int ppower = mmh.GetInteger(atoms2[0], "Power");
                            if (ppower > 0)
                            {
                                housePowerTotal += ppower;
                                if (!stopPowerCheck)
                                    housePowerThis += ppower;
                            }
                            else
                            {
                                houseLoadTotal -= ppower;
                                if (!stopPowerCheck)
                                    houseLoadThis -= ppower;
                            }
                            if (i == id)
                                stopPowerCheck = true;
                            if (housePowerTotal - houseLoadTotal < 0)
                                powerShortage.push_back(i);

                        }
                    }
                    Miscs::ParseHouseName(&targetHouse2, targetHouse2, true);
                    leftLine1.Format(Translations::TranslateOrDefault("ObjectInfo.HouseInfo.House", "House: %s:"), targetHouse2);
                    leftLine2.Format(Translations::TranslateOrDefault("ObjectInfo.HouseInfo.Basenode", 
                        "Basenode:  %s (%s), Count: %d, Cost: %d"), CMapData::GetUIName(atoms[0]), atoms[0], objThisCount, objThisCount * cost);

                    ::TextOut(hDC, rect.left + 10, rect.top + 10 + 18 * leftIndex++, leftLine1, leftLine1.GetLength());
                    ::TextOut(hDC, rect.left + 10 + tab, rect.top + 10 + 18 * leftIndex++, leftLine2, leftLine2.GetLength());

                    leftLine3.Format(Translations::TranslateOrDefault("ObjectInfo.HouseInfo.Power", 
                        "Total Power: %d, Output: %d, Drain: %d"), housePowerTotal - houseLoadTotal, housePowerTotal, houseLoadTotal);
                    if (housePowerTotal - houseLoadTotal >= 100)
                    {
                        ::SetBkColor(hDC, RGB(0, 255, 0));
                        ::TextOut(hDC, rect.left + 10 + tab, rect.top + 10 + 18 * leftIndex++, leftLine3, leftLine3.GetLength());
                        ::SetBkColor(hDC, RGB(255, 255, 255));
                    }
                    else if (housePowerTotal - houseLoadTotal >= 0)
                    {
                        ::SetBkColor(hDC, RGB(255, 255, 0));
                        ::TextOut(hDC, rect.left + 10 + tab, rect.top + 10 + 18 * leftIndex++, leftLine3, leftLine3.GetLength());
                        ::SetBkColor(hDC, RGB(255, 255, 255));
                    }
                    else
                    {
                        ::SetBkColor(hDC, RGB(255, 0, 0));
                        ::TextOut(hDC, rect.left + 10 + tab, rect.top + 10 + 18 * leftIndex++, leftLine3, leftLine3.GetLength());
                        ::SetBkColor(hDC, RGB(255, 255, 255));
                    }

                    leftLine4.Format(Translations::TranslateOrDefault("ObjectInfo.HouseInfo.CurrentPower",
                        "(Build till this) Total Power: %d, Output: %d, Drain: %d"), housePowerThis - houseLoadThis, housePowerThis, houseLoadThis);
                    if (housePowerThis - houseLoadThis >= 100)
                    {
                        ::SetBkColor(hDC, RGB(0, 255, 0));
                        ::TextOut(hDC, rect.left + 10 + tab, rect.top + 10 + 18 * leftIndex++, leftLine4, leftLine4.GetLength());
                        ::SetBkColor(hDC, RGB(255, 255, 255));
                    }
                    else if (housePowerThis - houseLoadThis >= 0)
                    {
                        ::SetBkColor(hDC, RGB(255, 255, 0));
                        ::TextOut(hDC, rect.left + 10 + tab, rect.top + 10 + 18 * leftIndex++, leftLine4, leftLine4.GetLength());
                        ::SetBkColor(hDC, RGB(255, 255, 255));
                    }
                    else
                    {
                        ::SetBkColor(hDC, RGB(255, 0, 0));
                        ::TextOut(hDC, rect.left + 10 + tab, rect.top + 10 + 18 * leftIndex++, leftLine4, leftLine4.GetLength());
                        ::SetBkColor(hDC, RGB(255, 255, 255));
                    }

                    if (powerShortage.size() > 0)
                    {
                        leftLine5 = Translations::TranslateOrDefault("ObjectInfo.HouseInfo.LowPowerNodes",
                            "Low power nodes:");
                        bool firstp = true;
                        for (auto sindex : powerShortage)
                        {
                            ppmfc::CString bufferp;
                            bufferp.Format("%03d", sindex);
                            if (firstp)
                            {
                                leftLine5 += " " + bufferp;
                                firstp = false;
                            }
                            else
                            {
                                leftLine5 += ", " + bufferp;
                            }
                        }
                        ::TextOut(hDC, rect.left + 10 + tab, rect.top + 10 + 18 * leftIndex++, leftLine5, leftLine5.GetLength());
                    }

                }
                if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::BaseNode || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Object || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All)
                {
                    Miscs::ParseHouseName(&targetHouse, targetHouse, true);
                    line1.Format(Translations::TranslateOrDefault("ObjectInfo.Basenode.1",
                        "Basenode: %s (%s), ID: %d")
                        , CMapData::GetUIName(atoms[0]), atoms[0], id);
                    line2.Format(Translations::TranslateOrDefault("ObjectInfo.Basenode.2",
                        "House: %s")
                        , targetHouse);
                    ::SetBkColor(hDC, RGB(0, 255, 255));
                    ::TextOut(hDC, drawX, drawY + 18 * i++, line1, line1.GetLength());
                    ::SetBkColor(hDC, RGB(0xFF, 0xFF, 0xFF));
                    ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line2, line2.GetLength());
                }

            }
        }
        if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Tile || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::AllTerrain || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All)
        {
            ppmfc::CString line1;
            ppmfc::CString line2;
            ppmfc::CString line3;
            ppmfc::CString line4;

            auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");

            int tileIndex = cell->TileIndex;
            if (tileIndex == 65535)
                tileIndex = 0;

            line2.Format(Translations::TranslateOrDefault("ObjectInfo.Tile.1",
                "Index: %d, SubTile: %d")
                , tileIndex, cell->TileSubIndex);

            auto theater = CINI::CurrentTheater();

            if (CMapDataExt::TileData && tileIndex < int(CTileTypeClass::InstanceCount()) && cell->TileSubIndex < CMapDataExt::TileData[tileIndex].TileBlockCount)
            {

                const auto& tileBlock = CMapDataExt::TileData[tileIndex].TileBlockDatas[cell->TileSubIndex];
                const auto& tile = CMapDataExt::TileData[tileIndex];

                line1.Format(Translations::TranslateOrDefault("ObjectInfo.Tile.2",
                    "Tile: %s (%d)")
                    , Translations::TranslateTileSet(tile.TileSet), tile.TileSet);

                auto ttype = CMapDataExt::TileData[tileIndex].TileBlockDatas[cell->TileSubIndex].TerrainType;
                ppmfc::CString setID;
                setID.Format("TileSet%04d", tile.TileSet);
                ppmfc::CString ttypes = "unknown";
                ppmfc::CString filename;
                filename.Format("%s%02d", theater->GetString(setID, "FileName"), tileIndex - CMapDataExt::TileSet_starts[tile.TileSet] + 1);

                if (cell->Flag.AltIndex == 1)
                    filename += "a";
                else if (cell->Flag.AltIndex == 2)
                    filename += "b";
                else if (cell->Flag.AltIndex == 3)
                    filename += "c";
                else if (cell->Flag.AltIndex == 4)
                    filename += "d";
                else if (cell->Flag.AltIndex == 5)
                    filename += "e";
                else if (cell->Flag.AltIndex == 6)
                    filename += "f";
                else if (cell->Flag.AltIndex == 7)
                    filename += "g";

                if (thisTheater == "TEMPERATE")
                    filename += ".tem";
                if (thisTheater == "SNOW")
                    filename += ".sno";
                if (thisTheater == "URBAN")
                    filename += ".urb";
                if (thisTheater == "NEWURBAN")
                    filename += ".ubn";
                if (thisTheater == "LUNAR")
                    filename += ".lun";
                if (thisTheater == "DESERT")
                    filename += ".des";

                if (ttype == 0x0)
                    ttypes = "Clear";
                else if (ttype == 0xd)
                    ttypes = "Clear";
                else if (ttype == 0xb)
                    ttypes = "Road";
                else if (ttype == 0xc)
                    ttypes = "Road";
                else if (ttype == 0x9)
                    ttypes = "Water";
                else if (ttype == 0x7)
                    ttypes = "Rock";
                else if (ttype == 0x8)
                    ttypes = "Rock";
                else if (ttype == 0xf)
                    ttypes = "Rock";
                else if (ttype == 0xa)
                    ttypes = "Beach";
                else if (ttype == 0xe)
                    ttypes = "Rough";
                else if (ttype == 0x1)
                    ttypes = "Ice";
                else if (ttype == 0x2)
                    ttypes = "Ice";
                else if (ttype == 0x3)
                    ttypes = "Ice";
                else if (ttype == 0x4)
                    ttypes = "Ice";
                else if (ttype == 0x6)
                    ttypes = "Railroad";
                else if (ttype == 0x5)
                    ttypes = "Tunnel";

                line3.Format(Translations::TranslateOrDefault("ObjectInfo.Tile.3",
                    "Terrain Type: %s (0x%x)")
                    , ttypes, ttype);
                line4.Format(Translations::TranslateOrDefault("ObjectInfo.Tile.4",
                    "Filename: %s")
                    , filename);

                ::SetBkColor(hDC, RGB(0, 255, 255));
                ::TextOut(hDC, drawX, drawY + 18 * i++, line1, line1.GetLength());
                ::SetBkColor(hDC, RGB(0xFF, 0xFF, 0xFF));
                ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line2, line2.GetLength());
                ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line3, line3.GetLength());
                ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line4, line4.GetLength());
            }
            else
            {
                line1.Format(Translations::TranslateOrDefault("ObjectInfo.Tile.2",
                    "Tile: %s (%d)")
                    , "MISSING", tileIndex);

                ::SetBkColor(hDC, RGB(0, 255, 255));
                ::TextOut(hDC, drawX, drawY + 18 * i++, line1, line1.GetLength());
                ::SetBkColor(hDC, RGB(0xFF, 0xFF, 0xFF));
                ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line2, line2.GetLength());
            }

        }
        if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Terrain || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::AllTerrain || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All)
        {
            ppmfc::CString line1;


            int id = cell->Terrain;
            int type = cell->TerrainType;
            ppmfc::CString name;
            if (auto pTerrain = CINI::Rules().GetSection("TerrainTypes"))
            {
                int index = 0;
                for (auto& pT : pTerrain->GetEntities())
                {
                    if (index == type)
                    {
                        name = pT.second;
                        break;
                    }

                    index++;
                }
            }

            if (id > -1)
            {
                auto name2 = CViewObjectsExt::QueryUIName(name);
                line1.Format(Translations::TranslateOrDefault("ObjectInfo.Terrain",
                    "Terrain: %s (%s), ID: %d")
                    , name2, name, id);
                ::SetBkColor(hDC, RGB(0, 255, 255));
                ::TextOut(hDC, drawX, drawY + 18 * i++, line1, line1.GetLength());
                ::SetBkColor(hDC, RGB(0xFF, 0xFF, 0xFF));
            }
        }
        if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Smudge || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::AllTerrain || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All)
        {
            ppmfc::CString line1;

            auto& rules = CINI::Rules();
            CSmudgeData target;
            int id = 0;
            bool found = false;
            for (auto& thisSmudge : CMapData::Instance().SmudgeDatas)
            {
                if (thisSmudge.X <= 0 || thisSmudge.Y <= 0 || thisSmudge.Flag)
                    continue;
                int thisWidth = rules.GetInteger(thisSmudge.TypeID, "Width", 1);
                int thisHeight = rules.GetInteger(thisSmudge.TypeID, "Height", 1);
                int thisX = thisSmudge.Y;
                int thisY = thisSmudge.X;//opposite
                for (int i = 0; i < thisWidth; i++)
                    for (int j = 0; j < thisHeight; j++)
                        if (thisY + i == point.Y && thisX + j == point.X)
                        {
                            target = thisSmudge;
                            found = true;
                        }
                if (found)
                    break;
                id++;
            }


            if (found)
            {
                line1.Format(Translations::TranslateOrDefault("ObjectInfo.Smudge",
                    "Smudge: %s, ID: %d")
                    , target.TypeID, id);
                ::SetBkColor(hDC, RGB(0, 255, 255));
                ::TextOut(hDC, drawX, drawY + 18 * i++, line1, line1.GetLength());
                ::SetBkColor(hDC, RGB(0xFF, 0xFF, 0xFF));
            }
        }
        if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Overlay || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::AllTerrain || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All)
        {
            ppmfc::CString line1;
            ppmfc::CString line2;

            auto overlay = cell->Overlay;
            auto overlayD = cell->OverlayData;

            ppmfc::CString name = "MISSING";
            ppmfc::CString ttype = "";

            int index = 0;
            if (auto pSection = CINI::Rules().GetSection("OverlayTypes"))
            {
                for (auto& ol : pSection->GetEntities())
                {
                    if (index == overlay)
                    {
                        auto thisname = mmh.GetString(ol.second, "Name");
                        name = Translations::TranslateOrDefault(thisname, thisname);

                        //if (CINI::Rules().GetBool(ol.second, "NoUseTileLandType"))
                        ttype = mmh.GetString(ol.second, "Land", "");
                        if (mmh.GetBool(ol.second, "Tiberium"))
                            ttype = "Tiberium";
                        if (mmh.GetBool(ol.second, "Wall"))
                        {
                            ttype = "Wall";
                            if (mmh.GetBool(ol.second, "Crushable"))
                                ttype = "Crushable Wall";
                        }

                        break;
                    }
                    index++;
                }
            }

            if (overlay != 255)
            {
                line1.Format(Translations::TranslateOrDefault("ObjectInfo.Overlay.1",
                    "Overlay: %s (%d), Overlay Data: %d")
                    , name, overlay, overlayD);
                line2.Format(Translations::TranslateOrDefault("ObjectInfo.Overlay.2",
                    "Terrain Type: %s")
                    , ttype);
                ::SetBkColor(hDC, RGB(0, 255, 255));
                ::TextOut(hDC, drawX, drawY + 18 * i++, line1, line1.GetLength());
                ::SetBkColor(hDC, RGB(0xFF, 0xFF, 0xFF));
                if (ttype != "")
                    ::TextOut(hDC, drawX + tab, drawY + 18 * i++, line2, line2.GetLength());
            }
        }
        if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Celltag || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All)
        {
            ppmfc::CString line1;

            ppmfc::CString id = "";
            if (CMapData::Instance().INI.SectionExists("CellTags"))
            {
                char tmp[10];
                _itoa((point.X * 1000 + point.Y), tmp, 10);
                id = CMapData::Instance().INI.GetString("CellTags", tmp);
            }

            if (id != "")
            {
                ppmfc::CString name = "MISSING";
                auto tag = CMapData::Instance().INI.GetString("Tags", id);
                auto atoms = STDHelpers::SplitString(tag);
                if (atoms.size() > 1)
                {
                    name = atoms[1];
                }
                line1.Format(Translations::TranslateOrDefault("ObjectInfo.CellTag",
                    "Cell Tag: %s, ID: %s")
                    , name, id);
                ::SetBkColor(hDC, RGB(0, 255, 255));
                ::TextOut(hDC, drawX, drawY + 18 * i++, line1, line1.GetLength());
                ::SetBkColor(hDC, RGB(0xFF, 0xFF, 0xFF));
            }
        }
        if (CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::Waypoints || CIsoView::CurrentCommand->Type == CViewObjectsExt::ObjectTerrainType::All)
        {
            SetTextAlign(hDC, TA_RIGHT);

            if (cell->Waypoint != -1)
            {
                auto pSection = CINI::CurrentDocument->GetSection("Waypoints");
                auto& pWP = *pSection->GetKeyAt(cell->Waypoint);
                auto& pVal = *pSection->GetValueAt(cell->Waypoint);
                int WPX = atoi(pVal) / 1000;
                int WPY = atoi(pVal) % 1000;

                if (point.X == WPX && point.Y == WPY)
                {
                    const int offset = 18;
                    int i = 1;
                    ppmfc::CString pSrc;
                    auto process = [](const char* s)
                        {
                            int n = 0;
                            int len = strlen(s);
                            for (int i = len - 1, j = 1; i >= 0; i--, j *= 26)
                            {
                                int c = toupper(s[i]);
                                if (c < 'A' || c > 'Z') return 0;
                                n += ((int)c - 64) * j;
                            }
                            if (n <= 0)
                                return -1;
                            return n - 1;
                        };

                    for (auto& triggerPair : CMapDataExt::Triggers)
                    {
                        auto& trigger = triggerPair.second;
                        bool addEvent = false;
                        bool addAction = false;
                        for (auto& thisEvent : trigger->Events)
                        {

                            auto eventInfos = STDHelpers::SplitString(CINI::FAData->GetString("EventsRA2", thisEvent.EventNum, "MISSING,0,0,0,0,MISSING,0,1,0"), 8);
                            ppmfc::CString paramType[2];
                            paramType[0] = eventInfos[1];
                            paramType[1] = eventInfos[2];
                            std::vector<ppmfc::CString> pParamTypes[2];
                            pParamTypes[0] = STDHelpers::SplitString(CINI::FAData->GetString("ParamTypes", paramType[0], "MISSING,0"));
                            pParamTypes[1] = STDHelpers::SplitString(CINI::FAData->GetString("ParamTypes", paramType[1], "MISSING,0"));
                            ppmfc::CString thisWp = "-1";
                            if (thisEvent.Params[0] == "2")
                            {
                                if (pParamTypes[0][1] == "1")// waypoint
                                {
                                    thisWp = thisEvent.Params[1];
                                    if (thisWp == pWP) addEvent = true;
                                }
                                if (pParamTypes[1][1] == "1")// waypoint
                                {
                                    thisWp = thisEvent.Params[2];
                                    if (thisWp == pWP) addEvent = true;
                                }
                            }
                            else
                            {
                                if (pParamTypes[1][1] == "1")// waypoint
                                {
                                    thisWp = thisEvent.Params[1];
                                    if (thisWp == pWP) addEvent = true;
                                }
                            }
                        }
                        if (addEvent)
                        {
                            pSrc.Format(Translations::TranslateOrDefault("ObjectInfo.Waypoint.Event",
                                "Event: %s (%s)")
                                , trigger->Name, trigger->ID);
                            TextOut(hDC, drawX, drawY + 18 * i, pSrc, strlen(pSrc));
                            pSrc = "";
                            i++;
                        }

                        for (auto& thisAction : trigger->Actions)
                        {
                            auto actionInfos = STDHelpers::SplitString(CINI::FAData->GetString("ActionsRA2", thisAction.ActionNum, "MISSING,0,0,0,0,0,0,0,0,0,MISSING,0,1,0"), 13);
                            ppmfc::CString thisWp = "-1";
                            ppmfc::CString paramType[7];
                            for (int i = 0; i < 7; i++)
                                paramType[i] = actionInfos[i + 1];

                            std::vector<ppmfc::CString> pParamTypes[6];
                            for (int i = 0; i < 6; i++)
                                pParamTypes[i] = STDHelpers::SplitString(CINI::FAData->GetString("ParamTypes", paramType[i], "MISSING,0"));

                            thisAction.Param7isWP = true;
                            for (auto& pair : CINI::FAData->GetSection("DontSaveAsWP")->GetEntities())
                            {
                                if (atoi(pair.second) == -atoi(paramType[0]))
                                    thisAction.Param7isWP = false;
                            }

                            for (int i = 0; i < 6; i++)
                            {
                                auto& param = pParamTypes[i];
                                if (param[1] == "1")// waypoint
                                {
                                    thisWp = thisAction.Params[i];
                                    if (thisWp == pWP) addAction = true;
                                }
                            }
                            if (atoi(paramType[6]) > 0 && thisAction.Param7isWP)
                            {
                                thisWp.Format("%d", process(thisAction.Params[6]));
                                if (thisWp == pWP) addAction = true;
                            }
                        }
                        if (addAction)
                        {
                            pSrc.Format(Translations::TranslateOrDefault("ObjectInfo.Waypoint.Action",
                                "Action: %s (%s)")
                                , trigger->Name, trigger->ID);
                            TextOut(hDC, drawX, drawY + 18 * i, pSrc, strlen(pSrc));
                            pSrc = "";
                            i++;
                        }
                    }

                    if (auto pSection = CINI::CurrentDocument->GetSection("ScriptTypes"))
                    {

                        for (auto& pair : pSection->GetEntities())
                        {
                            bool add = false;

                            for (int i = 0; i < 50; i++)
                            {
                                char id[10];
                                _itoa(i, id, 10);
                                auto line = CINI::CurrentDocument->GetString(pair.second, id);
                                if (line == "")
                                    continue;

                                auto app = STDHelpers::SplitString(line);
                                if (app.size() != 2)
                                    continue;

                                int actionType = atoi(app[0]);
                                switch (actionType)
                                {
                                case 1: if (app[1] == pWP) add = true; break;
                                case 3: if (app[1] == pWP) add = true; break;
                                case 15: if (app[1] == pWP) add = true; break;
                                case 16: if (app[1] == pWP) add = true; break;
                                case 59: if (app[1] == pWP) add = true; break;
                                default: break;
                                }
                            }
                            if (add)
                            {
                                pSrc.Format(Translations::TranslateOrDefault("ObjectInfo.Waypoint.Script",
                                    "Script: %s (%s)")
                                    , CINI::CurrentDocument->GetString(pair.second, "Name"), pair.second);
                                TextOut(hDC, drawX, drawY + 18 * i, pSrc, strlen(pSrc));
                                pSrc = "";
                                i++;
                            }



                        }
                    }
                    if (auto pSection = CINI::CurrentDocument->GetSection("TeamTypes"))
                    {
                        auto process = [](const char* s)
                            {
                                int n = 0;
                                int len = strlen(s);
                                for (int i = len - 1, j = 1; i >= 0; i--, j *= 26)
                                {
                                    int c = toupper(s[i]);
                                    if (c < 'A' || c > 'Z') return 0;
                                    n += ((int)c - 64) * j;
                                }
                                if (n <= 0)
                                    return -1;
                                return n - 1;
                            };
                        for (auto& pair : pSection->GetEntities())
                        {
                            auto wp = CINI::CurrentDocument->GetString(pair.second, "Waypoint");

                            if (process(wp) == atoi(pWP))
                            {
                                std::vector<ppmfc::CString> skiplist;
                                bool add = true;
                                if (ExtConfigs::Waypoint_SkipCheckList)
                                {
                                    skiplist = STDHelpers::SplitStringTrimmed(ExtConfigs::Waypoint_SkipCheckList);
                                }
                                if (skiplist.size() > 0)
                                {
                                    for (auto& wp : skiplist)
                                    {
                                        if (pWP == wp)
                                            add = false;
                                    }
                                }

                                if (add)
                                {
                                    pSrc.Format(Translations::TranslateOrDefault("ObjectInfo.Waypoint.Team",
                                        "Team: %s (%s)")
                                        , CINI::CurrentDocument->GetString(pair.second, "Name"), pair.second);
                                    TextOut(hDC, drawX, drawY + 18 * i, pSrc, strlen(pSrc));
                                    pSrc = "";
                                    i++;
                                }

                            }
                        }
                    }

                }
            }
        }

    }
    return 0;
}


