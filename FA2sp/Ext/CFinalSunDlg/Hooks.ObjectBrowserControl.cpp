#include "Body.h"

#include <FA2PP.h>
#include <Helpers/Macro.h>

#include "../../FA2sp.h"
#include "../../Miscs/TheaterInfo.h"

#include <CBrushSize.h>
#include <CTileTypeClass.h>
#include <CIsoView.h>
#include "../../Helpers/STDHelpers.h"
#include "../CIsoView/Body.h"
#include "../CMapData/Body.h"
#include "../../ExtraWindow/CTerrainGenerator/CTerrainGenerator.h"
#include "../../ExtraWindow/CLuaConsole/CLuaConsole.h"
#include "../../Helpers/Helper.h"

DEFINE_HOOK(51CD20, CViewObjects_Redraw, 7)
{
    if (ExtConfigs::BrowserRedraw)
    {
        CViewObjectsExt::RedrawCalledCount++;
        if (CViewObjectsExt::RedrawCalledCount % 2 == 0)
            return 0x523173;
        GET(CViewObjectsExt*, pThis, ECX);
        pThis->Redraw();
        return 0x523173;
    }
    return 0;
}

DEFINE_HOOK(51B54E, CViewObjects_SkipWaterBrushSize, 6)
{
    return 0x51B5AF;
}

DEFINE_HOOK(51AF40, CViewObjects_OnSelectChanged, 7)
{
    GET_STACK(LPNMTREEVIEW, pNM, 0x4);

    return
        ((CViewObjectsExt*)&CFinalSunDlg::Instance->ViewObjects)->UpdateEngine(pNM->itemNew.lParam) ?
        0x51CC8B :
        0;
}

int infantryLoop = 0;
MapCoord lastCoord = { 0,0 };
// skip to use our own method;
DEFINE_HOOK(45CD22, CIsoView_OnMouseMove_LButtonDown_PlaceObject, 9)
{
    GET(const int, X, EDI);
    GET(const int, Y, EBX);
    auto& Map = CMapData::Instance();
    auto cell = Map.TryGetCellAt(X, Y);

    if (CViewObjectsExt::PlacingRandomRock >= 0)
    {
        CIsoView::GetInstance()->DrawMouseAttachedStuff(X, Y);
        return 0x45CD2B;
    }
    if (CViewObjectsExt::PlacingRandomSmudge >= 0)
    {
        CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Smudge, true);
        CSmudgeData smudge;
        smudge.X = Y;
        smudge.Y = X;//opposite
        smudge.Flag = 0;
        smudge.TypeID = CIsoView::CurrentCommand->ObjectID;

        if (cell->Smudge > -1)
            Map.DeleteSmudgeData(cell->Smudge);

        auto& rules = Variables::RulesMap;
        //check overlapping
        int width = rules.GetInteger(smudge.TypeID, "Width", 1);
        int height = rules.GetInteger(smudge.TypeID, "Height", 1);
        for (auto& thisSmudge : Map.SmudgeDatas)
        {
            if (thisSmudge.X <= 0 || thisSmudge.Y <= 0 || thisSmudge.Flag)
                continue;
            int thisWidth = rules.GetInteger(thisSmudge.TypeID, "Width", 1);
            int thisHeight = rules.GetInteger(thisSmudge.TypeID, "Height", 1);
            int thisX = thisSmudge.Y;
            int thisY = thisSmudge.X;//opposite
            for (int i = 0; i < thisWidth; i++)
                for (int j = 0; j < thisHeight; j++)
                    for (int k = 0; k < width; k++)
                        for (int l = 0; l < height; l++)
                            if (thisY + i == Y + k && thisX + j == X + l)
                                return 0x45CD2B;

        }

        Map.SetSmudgeData(&smudge);
        return 0x45CD2B;
    }   
    if (CViewObjectsExt::PlacingRandomTerrain >= 0)
    {
        CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Terrain, true);
        if (cell->Terrain > -1)
            Map.DeleteTerrainData(cell->Terrain);
        Map.SetTerrainData(CIsoView::CurrentCommand->ObjectID, Map.GetCoordIndex(X, Y));
        return 0x45CD2B;
    }  
    if (CViewObjectsExt::PlacingRandomInfantry >= 0)
    {
        CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Infantry, true);
        CInfantryData newTechno;
        if (ExtConfigs::InfantrySubCell_Edit_Place && ExtConfigs::InfantrySubCell_Edit)
        {
            bool single = ExtConfigs::InfantrySubCell_Edit_Single;
            ExtConfigs::InfantrySubCell_Edit_Single = true;
            int curIdx = CIsoViewExt::GetSelectedSubcellInfantryIdx(X, Y);
            if (curIdx > -1)
                Map.DeleteInfantryData(curIdx);
            newTechno.SubCell.Format("%d", CIsoViewExt::GetSelectedSubcellInfantryIdx(X, Y, true));
            ExtConfigs::InfantrySubCell_Edit_Single = single;
        }
        else
        {
            if (lastCoord != MapCoord{ X,Y })
            {
                lastCoord = { X,Y };
                infantryLoop = 0;
            }
            if (cell->Infantry[infantryLoop] > -1)
            {
                Map.DeleteInfantryData(cell->Infantry[infantryLoop]);
                infantryLoop++;
                if (infantryLoop >= 3)
                    infantryLoop = 0;
            }
            newTechno.SubCell = "-1";
        }

        newTechno.House = CIsoView::CurrentHouse(); 
        newTechno.TypeID = CIsoView::CurrentCommand->ObjectID;
        newTechno.X.Format("%d", X);
        newTechno.Y.Format("%d", Y);

        newTechno.Status = ExtConfigs::DefaultInfantryProperty.Status;
        newTechno.Tag = ExtConfigs::DefaultInfantryProperty.Tag;
        newTechno.Facing = CViewObjectsExt::PlacingRandomRandomFacing ? STDHelpers::GetRandomFacing(): ExtConfigs::DefaultInfantryProperty.Facing;
        newTechno.VeterancyPercentage = ExtConfigs::DefaultInfantryProperty.VeterancyPercentage;
        newTechno.Group = ExtConfigs::DefaultInfantryProperty.Group;
        newTechno.IsAboveGround = ExtConfigs::DefaultInfantryProperty.IsAboveGround;
        newTechno.AutoNORecruitType = ExtConfigs::DefaultInfantryProperty.AutoNORecruitType;
        newTechno.AutoYESRecruitType = ExtConfigs::DefaultInfantryProperty.AutoYESRecruitType;
        newTechno.Health = ExtConfigs::DefaultInfantryProperty.Health;

        Map.SetInfantryData(&newTechno, nullptr, nullptr, 0, -1);
        return 0x45CD2B;
    }
    if (CViewObjectsExt::PlacingRandomVehicle >= 0)
    {
        CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Unit, true);
        CUnitData newTechno;

        if (cell->Unit > -1)
            Map.DeleteUnitData(cell->Unit);

        newTechno.Facing = CViewObjectsExt::PlacingRandomRandomFacing ? STDHelpers::GetRandomFacing() : ExtConfigs::DefaultUnitProperty.Facing;
        newTechno.Health = ExtConfigs::DefaultUnitProperty.Health;
        newTechno.Status = ExtConfigs::DefaultUnitProperty.Status;
        newTechno.Tag = ExtConfigs::DefaultUnitProperty.Tag;
        newTechno.VeterancyPercentage = ExtConfigs::DefaultUnitProperty.VeterancyPercentage;
        newTechno.Group = ExtConfigs::DefaultUnitProperty.Group;
        newTechno.IsAboveGround = ExtConfigs::DefaultUnitProperty.IsAboveGround;
        newTechno.FollowsIndex = ExtConfigs::DefaultUnitProperty.FollowsIndex;
        newTechno.AutoNORecruitType = ExtConfigs::DefaultUnitProperty.AutoNORecruitType;
        newTechno.AutoYESRecruitType = ExtConfigs::DefaultUnitProperty.AutoYESRecruitType;

        newTechno.House = CIsoView::CurrentHouse();
        newTechno.TypeID = CIsoView::CurrentCommand->ObjectID;
        newTechno.X.Format("%d", X);
        newTechno.Y.Format("%d", Y);

        Map.SetUnitData(&newTechno, nullptr, nullptr, 0, "");
        return 0x45CD2B;
    }
    if (CViewObjectsExt::PlacingRandomStructure >= 0)
    {
        CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Building, true);
        CBuildingData newTechno;

        if (cell->Structure > -1)
        {
            CBuildingData building;
            Map.GetBuildingData(cell->Structure, building);
            if (atoi(building.X) == X && (atoi(building.Y) == Y)) // replace onlyif coord is the same
                Map.DeleteBuildingData(cell->Structure);
            else
                return 0x45CD2B;
        }

        newTechno.Facing = CViewObjectsExt::PlacingRandomRandomFacing ? STDHelpers::GetRandomFacing() : ExtConfigs::DefaultBuildingProperty.Facing;
        newTechno.Health = ExtConfigs::DefaultBuildingProperty.Health;
        newTechno.Tag = ExtConfigs::DefaultBuildingProperty.Tag;
        newTechno.AISellable = ExtConfigs::DefaultBuildingProperty.AISellable;
        newTechno.AIRebuildable = ExtConfigs::DefaultBuildingProperty.AIRebuildable;
        newTechno.PoweredOn = ExtConfigs::DefaultBuildingProperty.PoweredOn;
        newTechno.Upgrades = ExtConfigs::DefaultBuildingProperty.Upgrades;
        newTechno.SpotLight = ExtConfigs::DefaultBuildingProperty.SpotLight;
        newTechno.Upgrade1 = ExtConfigs::DefaultBuildingProperty.Upgrade1;
        newTechno.Upgrade2 = ExtConfigs::DefaultBuildingProperty.Upgrade2;
        newTechno.Upgrade3 = ExtConfigs::DefaultBuildingProperty.Upgrade3;
        newTechno.AIRepairable = CViewObjectsExt::PlacingRandomStructureAIRepairs ? "1" : "0";
        newTechno.Nominal = ExtConfigs::DefaultBuildingProperty.Nominal;

        newTechno.House = CIsoView::CurrentHouse();
        newTechno.TypeID = CIsoView::CurrentCommand->ObjectID;
        newTechno.X.Format("%d", X);
        newTechno.Y.Format("%d", Y);

        Map.SetBuildingData(&newTechno, nullptr, nullptr, 0, "");
        return 0x45CD2B;
    }
    if (CViewObjectsExt::PlacingRandomAircraft >= 0)
    {
        CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Aircraft, true);
        CAircraftData newTechno;

        if (cell->Aircraft > -1)
            Map.DeleteAircraftData(cell->Aircraft);

        newTechno.Facing = CViewObjectsExt::PlacingRandomRandomFacing ? STDHelpers::GetRandomFacing() : ExtConfigs::DefaultAircraftProperty.Facing;
        newTechno.Health = ExtConfigs::DefaultAircraftProperty.Health;
        newTechno.Status = ExtConfigs::DefaultAircraftProperty.Status;
        newTechno.Tag = ExtConfigs::DefaultAircraftProperty.Tag;
        newTechno.VeterancyPercentage = ExtConfigs::DefaultAircraftProperty.VeterancyPercentage;
        newTechno.Group = ExtConfigs::DefaultAircraftProperty.Group;
        newTechno.AutoNORecruitType = ExtConfigs::DefaultAircraftProperty.AutoNORecruitType;
        newTechno.AutoYESRecruitType = ExtConfigs::DefaultAircraftProperty.AutoYESRecruitType;

        newTechno.House = CIsoView::CurrentHouse();
        newTechno.TypeID = CIsoView::CurrentCommand->ObjectID;
        newTechno.X.Format("%d", X);
        newTechno.Y.Format("%d", Y);

        Map.SetAircraftData(&newTechno, nullptr, nullptr, 0, "");
        return 0x45CD2B;
    }
    if (CViewObjectsExt::PlacingWall >= 0)
    {
        int overlay = CViewObjectsExt::PlacingWall / 5;
        int damageLevel = CViewObjectsExt::PlacingWall % 5 - 1;
        if (damageLevel == 3)
            damageLevel = -2;
        if (damageLevel == -1)
            damageLevel = 0;

        for (int i = 0; i < CFinalSunDlg::Instance->MyViewFrame.pIsoView->BrushSizeX; i++)
            for (int j = 0; j < CFinalSunDlg::Instance->MyViewFrame.pIsoView->BrushSizeY; j++)
                CMapDataExt::PlaceWallAt(Map.GetCoordIndex(X + i, Y + j), overlay, damageLevel);
        
        return 0x45CD2B;
    }

    if (CIsoView::CurrentCommand->Command == 1)
    {
        switch (CIsoView::CurrentCommand->Type)
        {
        case 1:
            CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Infantry, true);
            break;
        case 2:
            CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Building, true);
            break;
        case 3:
            CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Aircraft, true);
            break;
        case 4:
            CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Unit, true);
            break;
        case 5:
            CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Terrain, true);
            break;
        case 8:
            CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Smudge, true);
            break;
        default:
            break;
        }      
    }
    else if (CIsoView::CurrentCommand->Command == 22)
    {
        CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Terrain, true);
    }
    CIsoView::GetInstance()->DrawMouseAttachedStuff(X, Y);
    return 0x45CD2B;
}

DEFINE_HOOK(4F0A40, CTerrainDlg_OnSelchangeTileset, 7)
{
    CViewObjectsExt::InitializeOnUpdateEngine();

    return 0;
}

//DEFINE_HOOK(4F17B0, CTerrainDlg_OnSelchangeOverlay, 5)
//{
//    CViewObjectsExt::PlacingWall = -1;
//    return 0;
//}

DEFINE_HOOK(4572E1, CIsoView_OnMouseMove_Cliff, 6)
{
    auto point = CIsoView::GetInstance()->GetCurrentMapCoord(CIsoView::GetInstance()->MouseCurrentPosition);
    if (CIsoView::CurrentCommand->Command == 0x1E)
    {
        CViewObjectsExt::PlaceConnectedTile_OnMouseMove(point.X, point.Y);
    }
    if (CViewObjectsExt::PlacingRandomRock >= 0)
    {
        if (CIsoView::CurrentCommand->Command != 0x1)
        {
            CViewObjectsExt::PlacingRandomRock = -1;
            return 0;
        }
        std::vector<int> randomList;

        if (auto pSection = CINI::FAData().GetSection("PlaceRandomOverlayList"))
        {
            if (auto pSection2 = CINI::FAData().GetSection(*pSection->GetValueAt(CViewObjectsExt::PlacingRandomRock)))
            {
                for (auto& pKey : pSection2->GetEntities())
                {
                    if (pKey.first.Find("Name") < 0 && pKey.first != "BannedTheater")
                    {
                        randomList.push_back(atoi(pKey.second));
                    }
                }
            }
        }
        CIsoView::CurrentCommand->Command = 0x1; // place
        CIsoView::CurrentCommand->Type = 6;
        CIsoView::CurrentCommand->Param = 33;
        CIsoView::CurrentCommand->Overlay = STDHelpers::RandomSelectInt(randomList);
        CIsoView::CurrentCommand->OverlayData = 0;
    }
    if (CViewObjectsExt::PlacingRandomTerrain >= 0)
    {
        if (CIsoView::CurrentCommand->Command != 0x1)
        {
            CViewObjectsExt::PlacingRandomTerrain = -1;
            return 0;
        }
        std::vector<FString> randomList;

        if (auto pSection = CINI::FAData().GetSection("PlaceRandomTreeObList"))
        {
            if (auto pSection2 = CINI::FAData().GetSection(*pSection->GetValueAt(CViewObjectsExt::PlacingRandomTerrain)))
            {
                for (auto& pKey : pSection2->GetEntities())
                {
                    if (pKey.first.Find("Name") < 0 && pKey.first != "BannedTheater")
                    {
                        randomList.push_back(pKey.second);
                    }
                }
            }
        }
        CIsoView::CurrentCommand->Command = 0x1; // place
        CIsoView::CurrentCommand->Type = 5;
        CIsoView::CurrentCommand->ObjectID = STDHelpers::RandomSelect(randomList);
    }
    if (CViewObjectsExt::PlacingRandomSmudge >= 0)
    {
        if (CIsoView::CurrentCommand->Command != 0x1)
        {
            CViewObjectsExt::PlacingRandomSmudge = -1;
            return 0;
        }
        std::vector<FString> randomList;

        if (auto pSection = CINI::FAData().GetSection("PlaceRandomSmudgeList"))
        {
            if (auto pSection2 = CINI::FAData().GetSection(*pSection->GetValueAt(CViewObjectsExt::PlacingRandomSmudge)))
            {
                for (auto& pKey : pSection2->GetEntities())
                {
                    if (pKey.first.Find("Name") < 0 && pKey.first != "BannedTheater")
                    {
                        randomList.push_back(pKey.second);
                    }
                }
            }
        }
        auto smudgeTypeID = STDHelpers::RandomSelect(randomList);
        CIsoView::CurrentCommand->Command = 0x1; // place
        CIsoView::CurrentCommand->Type = 8;
        CIsoView::CurrentCommand->ObjectID = smudgeTypeID;
    }
    if (CViewObjectsExt::PlacingRandomInfantry >= 0)
    {
        CViewObjectsExt::PlacingRandomRandomFacing = false;
        if (CIsoView::CurrentCommand->Command != 0x1)
        {
            CViewObjectsExt::PlacingRandomInfantry = -1;
            return 0;
        }
        std::vector<FString> randomList;

        if (auto pSection = CINI::FAData().GetSection("PlaceRandomInfantryObList"))
        {
            auto& section = *pSection->GetValueAt(CViewObjectsExt::PlacingRandomInfantry);
            if (auto pSection2 = CINI::FAData().GetSection(section))
            {
                for (auto& pKey : pSection2->GetEntities())
                {
                    if (pKey.first.Find("Name") < 0 && pKey.first != "BannedTheater" && pKey.first != "RandomFacing")
                    {
                        randomList.push_back(pKey.second);
                    }
                }
                CViewObjectsExt::PlacingRandomRandomFacing = CINI::FAData().GetBool(section, "RandomFacing");
            }
        }
        auto TypeID = STDHelpers::RandomSelect(randomList);
        CIsoView::CurrentCommand->Command = 0x1; // place
        CIsoView::CurrentCommand->Type = 1;
        CIsoView::CurrentCommand->ObjectID = TypeID;
    }
    if (CViewObjectsExt::PlacingRandomStructure >= 0)
    {
        CViewObjectsExt::PlacingRandomRandomFacing = false;
        if (CIsoView::CurrentCommand->Command != 0x1)
        {
            CViewObjectsExt::PlacingRandomStructure = -1;
            return 0;
        }
        std::vector<FString> randomList;

        if (auto pSection = CINI::FAData().GetSection("PlaceRandomBuildingObList"))
        {
            auto& section = *pSection->GetValueAt(CViewObjectsExt::PlacingRandomStructure);
            if (auto pSection2 = CINI::FAData().GetSection(section))
            {
                for (auto& pKey : pSection2->GetEntities())
                {
                    if (pKey.first.Find("Name") < 0 && pKey.first != "BannedTheater" && pKey.first != "RandomFacing" && pKey.first != "AIRepairs" )
                    {
                        randomList.push_back(pKey.second);
                    }
                }
                CViewObjectsExt::PlacingRandomRandomFacing = CINI::FAData().GetBool(section, "RandomFacing");
                CViewObjectsExt::PlacingRandomStructureAIRepairs = CINI::FAData().GetBool(section, "AIRepairs");
            }
        }
        auto TypeID = STDHelpers::RandomSelect(randomList);
        CIsoView::CurrentCommand->Command = 0x1; // place
        CIsoView::CurrentCommand->Type = 2;
        CIsoView::CurrentCommand->ObjectID = TypeID;
    }
    if (CViewObjectsExt::PlacingRandomAircraft >= 0)
    {
        CViewObjectsExt::PlacingRandomRandomFacing = false;
        if (CIsoView::CurrentCommand->Command != 0x1)
        {
            CViewObjectsExt::PlacingRandomAircraft = -1;
            return 0;
        }
        std::vector<FString> randomList;

        if (auto pSection = CINI::FAData().GetSection("PlaceRandomAircraftObList"))
        {
            auto& section = *pSection->GetValueAt(CViewObjectsExt::PlacingRandomAircraft);
            if (auto pSection2 = CINI::FAData().GetSection(section))
            {
                for (auto& pKey : pSection2->GetEntities())
                {
                    if (pKey.first.Find("Name") < 0 && pKey.first != "BannedTheater" && pKey.first != "RandomFacing")
                    {
                        randomList.push_back(pKey.second);
                    }
                }
                CViewObjectsExt::PlacingRandomRandomFacing = CINI::FAData().GetBool(section, "RandomFacing");
            }
        }
        auto TypeID = STDHelpers::RandomSelect(randomList);
        CIsoView::CurrentCommand->Command = 0x1; // place
        CIsoView::CurrentCommand->Type = 3;
        CIsoView::CurrentCommand->ObjectID = TypeID;
    }    
    if (CViewObjectsExt::PlacingRandomVehicle >= 0)
    {
        CViewObjectsExt::PlacingRandomRandomFacing = false;
        if (CIsoView::CurrentCommand->Command != 0x1)
        {
            CViewObjectsExt::PlacingRandomVehicle = -1;
            return 0;
        }
        std::vector<FString> randomList;

        if (auto pSection = CINI::FAData().GetSection("PlaceRandomVehicleObList"))
        {
            auto& section = *pSection->GetValueAt(CViewObjectsExt::PlacingRandomVehicle);
            if (auto pSection2 = CINI::FAData().GetSection(section))
            {
                for (auto& pKey : pSection2->GetEntities())
                {
                    if (pKey.first.Find("Name") < 0 && pKey.first != "BannedTheater" && pKey.first != "RandomFacing")
                    {
                        randomList.push_back(pKey.second);
                    }
                }
                CViewObjectsExt::PlacingRandomRandomFacing = CINI::FAData().GetBool(section, "RandomFacing");
            }
        }
        auto TypeID = STDHelpers::RandomSelect(randomList);
        CIsoView::CurrentCommand->Command = 0x1; // place
        CIsoView::CurrentCommand->Type = 4;
        CIsoView::CurrentCommand->ObjectID = TypeID;
    }
    if (CViewObjectsExt::PlacingWall >= 0)
    {
        if (CIsoView::CurrentCommand->Command != 0x1)
        {
            CViewObjectsExt::PlacingWall = -1;
            return 0;
        }

        CIsoView::CurrentCommand->Command = 0x1; // place
        CIsoView::CurrentCommand->Type = 6;
        CIsoView::CurrentCommand->Param = 33;
        CIsoView::CurrentCommand->Overlay = CViewObjectsExt::PlacingWall / 5;
        //CFinalSunDlg::Instance->BrushSize.nCurSel = 0;
        //CFinalSunDlg::Instance->BrushSize.UpdateData(FALSE);
        //CFinalSunDlg::Instance->MyViewFrame.pIsoView->BrushSizeX = 1;
        //CFinalSunDlg::Instance->MyViewFrame.pIsoView->BrushSizeY = 1;

        if (CViewObjectsExt::PlacingWall % 5 == 4 || CViewObjectsExt::PlacingWall % 5 == 0)
        {
            CIsoView::CurrentCommand->OverlayData = 0;
        }
        else
            CIsoView::CurrentCommand->OverlayData = (CViewObjectsExt::PlacingWall % 5 - 1) * 16;
    }

    return 0;
}
DEFINE_HOOK(4347B8, CFinalSunDlg_OnEditUndo, 6)
{
    if (CIsoView::CurrentCommand->Command == 0x1E)
    {
        if (!CViewObjectsExt::CliffConnectionCoordRecords.empty())
        {
            CViewObjectsExt::CliffConnectionCoord = CViewObjectsExt::CliffConnectionCoordRecords.back();
            CViewObjectsExt::CliffConnectionCoordRecords.pop_back();
            if (CViewObjectsExt::CliffConnectionCoordRecords.empty())
                CViewObjectsExt::PlaceConnectedTile_Start = true;
        }
        if (!CViewObjectsExt::LastPlacedCTRecords.empty())
        {
            CViewObjectsExt::LastPlacedCT = CViewObjectsExt::LastPlacedCTRecords.back();
            CViewObjectsExt::LastPlacedCTRecords.pop_back();
        }
        if (!CViewObjectsExt::LastCTTileRecords.empty())
        {
            CViewObjectsExt::LastCTTile = CViewObjectsExt::LastCTTileRecords.back();
            CViewObjectsExt::LastCTTileRecords.pop_back();
        }
        if (!CViewObjectsExt::LastHeightRecords.empty())
        {
            CViewObjectsExt::CliffConnectionHeight = CViewObjectsExt::LastHeightRecords.back();
            CViewObjectsExt::LastHeightRecords.pop_back();
            CViewObjectsExt::NextCTHeightOffset = 0;
            CViewObjectsExt::HeightChanged = false;
        }
        CViewObjectsExt::LastSuccessfulIndex = -1;
    }
    
    return 0;
}

DEFINE_HOOK(4353F0, CFinalSunDlg_OnEditRedo, 5)
{
    if (CIsoView::CurrentCommand->Command == 0x1E)
    {
        CFinalSunDlg::PlaySound(CFinalSunDlg::FASoundType::Error);
        return 0x43544F;
    }
    return 0;
}

DEFINE_HOOK(461766, CIsoView_OnLButtonDown_PropertyBrush, 5)
{
    GET(const int, X, EDI);
    GET(const int, Y, ESI);

    if (CIsoViewExt::EnableDistanceRuler)
    {
        if (CIsoViewExt::DistanceRuler.empty() || 
            (!CIsoViewExt::DistanceRuler.empty() && CIsoViewExt::DistanceRuler.back() != MapCoord{ X,Y }))
        {
            CIsoViewExt::DistanceRuler.push_back({ X,Y });
            if (CIsoViewExt::DistanceRuler.size() > ExtConfigs::DistanceRuler_Records)
                CIsoViewExt::DistanceRuler.erase(CIsoViewExt::DistanceRuler.begin());
        }
    }

    auto pIsoView = (CIsoViewExt*)CIsoView::GetInstance();
    auto& command = pIsoView->LastAltCommand;
    if ((GetKeyState(VK_MENU) & 0x8000) && CIsoView::CurrentCommand->Command == 0) // tile picker
    {
        auto cell = CMapData::Instance->GetCellAt(X, Y);
        if ((GetKeyState(VK_CONTROL) & 0x8000))
        {
            HWND hParent = CFinalSunDlg::Instance->MyViewFrame.pTileSetBrowserFrame->DialogBar.GetSafeHwnd();
            HWND hTileComboBox = GetDlgItem(hParent, 1366);
            int tileSet = CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileSet;
            int nTileCount = SendMessage(hTileComboBox, CB_GETCOUNT, NULL, NULL);
            char buffer[512] = { 0 };
            FString tileName;

            for (int idx = 0; idx < nTileCount; ++idx)
            {
                SendMessage(hTileComboBox, CB_GETLBTEXT, idx, (LPARAM)(LPCSTR)buffer);
                tileName = buffer;
                FString::TrimIndex(tileName);
                if (atoi(tileName) == tileSet)
                {
                    SendMessage(hTileComboBox, CB_SETCURSEL, idx, NULL);
                    SendMessage(hParent, WM_COMMAND, MAKEWPARAM(1366, CBN_SELCHANGE), (LPARAM)hTileComboBox);
                    break;
                }
            }
        }
        CIsoView::CurrentCommand->Command = 10;
        CIsoView::CurrentCommand->Type = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
        CIsoView::CurrentCommand->Param = 0;
        CIsoView::CurrentCommand->Height = 0;
        CViewObjectsExt::NeedChangeTreeViewSelect = false;
        return 0x466860;
    }
    else if ((GetKeyState(VK_MENU) & 0x8000) && 
        (CIsoView::CurrentCommand->Command == 1 ||
            CIsoView::CurrentCommand->Command == 10 ||
            CIsoView::CurrentCommand->Command == 22 ||
            CIsoView::CurrentCommand->Command == 4
            && CIsoView::CurrentCommand->Type == 4))
    {
        auto pMap = CMapDataExt::GetExtension();
        if (command.isSame())
        {
            auto mapCoords = pIsoView->GetLinePoints({ command.X, command.Y }, { X,Y });

            if (command.Command == 10) // place tile
            {
                int x1 = command.X;
                int y1 = command.Y;
                int x2 = X;
                int y2 = Y;

                if (x1 > x2) std::swap(x1, x2);
                if (y1 > y2) std::swap(y1, y2);

                if (command.Type < CUSTOM_TILE_START)
                {
                    int index = CMapDataExt::GetSafeTileIndex(command.Type);
                    const auto& tileData = CMapDataExt::TileData[index];
                    int width = CMapDataExt::TileData[index].Width * pIsoView->BrushSizeY;
                    int height = CMapDataExt::TileData[index].Height * pIsoView->BrushSizeX;

                    pMap->SaveUndoRedoData(TRUE, x1 - width - 4,
                        y1 - height - 4,
                        x2 - width + pIsoView->BrushSizeX * width + 7,
                        y2 - height + pIsoView->BrushSizeY * height + 7);

                    if (width > 1 || height > 1)
                    {
                        mapCoords = pIsoView->GetLineRectangles({ command.X, command.Y }, { X,Y }, height, width);
                    }

                    for (const auto& mc : mapCoords)
                    {
                        for (int i = 0; i < pIsoView->BrushSizeX; ++i)
                        {
                            for (int e = 0; e < pIsoView->BrushSizeY; ++e)
                            {
                                int tileIndex = command.Type;
                                if (command.Param == 1) // random tile
                                {
                                    tileIndex = CIsoViewExt::GetRandomTileIndex();
                                }
                                pMap->PlaceTileAt(mc.X + 1 + (i - 1) * CMapDataExt::TileData[index].Height,
                                    mc.Y + 1 + (e - 1) * CMapDataExt::TileData[index].Width, tileIndex, 2);
                            }
                        }

                    }
                    if (!mapCoords.empty() && !CFinalSunApp::Instance->DisableAutoLat)
                    {
                        std::set<MapCoord> editedCoords;
                        for (int i = 0; i < pMap->CellDataExts.size(); ++i)
                        {
                            auto& cell = pMap->CellDataExts[i];
                            if (cell.LineToolProcessed)
                            {
                                editedCoords.insert({ pMap->GetXFromCoordIndex(i), pMap->GetYFromCoordIndex(i) });
                            }
                        }
                        std::set<MapCoord> editedLatCoords;
                        for (const auto& p : editedCoords) {
                            editedLatCoords.insert({ p.X + 1, p.Y });
                            editedLatCoords.insert({ p.X - 1, p.Y });
                            editedLatCoords.insert({ p.X, p.Y + 1 });
                            editedLatCoords.insert({ p.X, p.Y - 1 });
                            editedLatCoords.insert({ p.X, p.Y });
                        }
                        for (const auto& p : editedLatCoords)
                        {
                            pMap->SmoothTileAt(p.X, p.Y, true);
                        }
                        for (auto& cell : CMapDataExt::CellDataExts)
                        {
                            cell.LineToolProcessed = false;
                        }
                    }
                }
                else
                {
                    auto tileData = CMapDataExt::GetCustomTile(command.Type);
                    int width = tileData->Width * pIsoView->BrushSizeY;
                    int height = tileData->Height * pIsoView->BrushSizeX;

                    pMap->SaveUndoRedoData(TRUE, x1 - width - 4,
                        y1 - height - 4,
                        x2 - width + pIsoView->BrushSizeX * width + 7,
                        y2 - height + pIsoView->BrushSizeY * height + 7);

                    if (width > 1 || height > 1)
                    {
                        mapCoords = pIsoView->GetLineRectangles({ command.X, command.Y }, { X,Y }, height, width);
                    }

                    for (const auto& mc : mapCoords)
                    {
                        for (int i = 0; i < pIsoView->BrushSizeX; ++i)
                        {
                            for (int e = 0; e < pIsoView->BrushSizeY; ++e)
                            {
                                pMap->PlaceTileAt(mc.X + 1 + (i - 1) * tileData->Height,
                                    mc.Y + 1 + (e - 1) * tileData->Width, command.Type, 2);
                            }
                        }
                    }
                    if (!mapCoords.empty() && !CFinalSunApp::Instance->DisableAutoLat)
                    {
                        std::set<MapCoord> editedCoords;
                        for (int i = 0; i < pMap->CellDataExts.size(); ++i)
                        {
                            auto& cell = pMap->CellDataExts[i];
                            if (cell.LineToolProcessed)
                            {
                                editedCoords.insert({ pMap->GetXFromCoordIndex(i), pMap->GetYFromCoordIndex(i) });
                            }
                        }
                        std::set<MapCoord> editedLatCoords;
                        for (const auto& p : editedCoords) {
                            editedLatCoords.insert({ p.X + 1, p.Y });
                            editedLatCoords.insert({ p.X - 1, p.Y });
                            editedLatCoords.insert({ p.X, p.Y + 1 });
                            editedLatCoords.insert({ p.X, p.Y - 1 });
                            editedLatCoords.insert({ p.X, p.Y });
                        }
                        for (const auto& p : editedLatCoords)
                        {
                            pMap->SmoothTileAt(p.X, p.Y, true);
                        }
                        for (auto& cell : CMapDataExt::CellDataExts)
                        {
                            cell.LineToolProcessed = false;
                        }
                    }
                }
            }
            else if (command.Command == 1) // place objects
            {
                if (command.Type == 6) // overlay
                {
                    TempValueHolder<bool> track(CIsoViewExt::EnableAutoTrack, true);
                    pMap->SaveUndoRedoData(true, 0, 0, 0, 0);
                    std::vector<int> randomRockList;
                    if (CViewObjectsExt::PlacingRandomRock >= 0)
                    {
                        if (auto pSection = CINI::FAData().GetSection("PlaceRandomOverlayList"))
                        {
                            if (auto pSection2 = CINI::FAData().GetSection(*pSection->GetValueAt(CViewObjectsExt::PlacingRandomRock)))
                            {
                                for (auto& pKey : pSection2->GetEntities())
                                {
                                    if (pKey.first.Find("Name") < 0 && pKey.first != "BannedTheater")
                                    {
                                        randomRockList.push_back(atoi(pKey.second));
                                    }
                                }
                            }
                        }
                    }
                    for (const auto& mc : mapCoords)
                    {
                        if (CViewObjectsExt::PlacingWall >= 0)
                        {
                            int overlay = CViewObjectsExt::PlacingWall / 5;
                            int damageLevel = CViewObjectsExt::PlacingWall % 5 - 1;
                            if (damageLevel == 3)
                                damageLevel = -2;
                            if (damageLevel == -1)
                                damageLevel = 0;

                            for (int i = 0; i < CFinalSunDlg::Instance->MyViewFrame.pIsoView->BrushSizeX; i++)
                                for (int j = 0; j < CFinalSunDlg::Instance->MyViewFrame.pIsoView->BrushSizeY; j++)
                                    CMapDataExt::PlaceWallAt(pMap->GetCoordIndex(mc.X + i, mc.Y + j), overlay, damageLevel);

                        }
                        else if (!randomRockList.empty())
                        {
                            CIsoView::CurrentCommand->Overlay = STDHelpers::RandomSelectInt(randomRockList);
                            CIsoView::CurrentCommand->OverlayData = 0;
                            CIsoView::GetInstance()->DrawMouseAttachedStuff(mc.X, mc.Y);
                        }
                        else if (command.Param == 5) // bridge
                        {
                            return 0;
                        }
                        else
                        {                           
                            CIsoView::GetInstance()->DrawMouseAttachedStuff(mc.X, mc.Y);
                        }
                    }
                }
                else if (CViewObjectsExt::PlacingRandomSmudge >= 0)
                {
                    CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Smudge);
                    std::vector<FString> randomList;
                    if (auto pSection = CINI::FAData().GetSection("PlaceRandomSmudgeList"))
                    {
                        if (auto pSection2 = CINI::FAData().GetSection(*pSection->GetValueAt(CViewObjectsExt::PlacingRandomSmudge)))
                        {
                            for (auto& pKey : pSection2->GetEntities())
                            {
                                if (pKey.first.Find("Name") < 0 && pKey.first != "BannedTheater")
                                {
                                    randomList.push_back(pKey.second);
                                }
                            }
                        }
                    }
                    for (const auto& mc : mapCoords)
                    {
                        auto cell = pMap->GetCellAt(mc.X, mc.Y);
                        CSmudgeData smudge;
                        smudge.X = mc.Y;
                        smudge.Y = mc.X;//opposite
                        smudge.Flag = 0;
                        smudge.TypeID = STDHelpers::RandomSelect(randomList);

                        if (cell->Smudge < 0)
                        {
                            auto& rules = Variables::RulesMap;
                            //check overlapping
                            int width = rules.GetInteger(smudge.TypeID, "Width", 1);
                            int height = rules.GetInteger(smudge.TypeID, "Height", 1);
                            bool place = true;
                            for (auto& thisSmudge : pMap->SmudgeDatas)
                            {
                                if (thisSmudge.X <= 0 || thisSmudge.Y <= 0 || thisSmudge.Flag)
                                    continue;
                                int thisWidth = rules.GetInteger(thisSmudge.TypeID, "Width", 1);
                                int thisHeight = rules.GetInteger(thisSmudge.TypeID, "Height", 1);
                                int thisX = thisSmudge.Y;
                                int thisY = thisSmudge.X;//opposite
                                for (int i = 0; i < thisWidth; i++)
                                    for (int j = 0; j < thisHeight; j++)
                                        for (int k = 0; k < width; k++)
                                            for (int l = 0; l < height; l++)
                                                if (thisY + i == mc.Y + k && thisX + j == mc.X + l)
                                                    place = false;

                            }
                            if (place)
                            {
                                pMap->SetSmudgeData(&smudge);
                            }
                        }
                    }
                }
                else if (CViewObjectsExt::PlacingRandomInfantry >= 0)
                {
                    CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Infantry);
                    std::vector<FString> randomList;

                    if (auto pSection = CINI::FAData().GetSection("PlaceRandomInfantryObList"))
                    {
                        auto& section = *pSection->GetValueAt(CViewObjectsExt::PlacingRandomInfantry);
                        if (auto pSection2 = CINI::FAData().GetSection(section))
                        {
                            for (auto& pKey : pSection2->GetEntities())
                            {
                                if (pKey.first.Find("Name") < 0 && pKey.first != "BannedTheater" && pKey.first != "RandomFacing")
                                {
                                    randomList.push_back(pKey.second);
                                }
                            }
                            CViewObjectsExt::PlacingRandomRandomFacing = CINI::FAData().GetBool(section, "RandomFacing");
                        }
                    }
                    for (const auto& mc : mapCoords)
                    {
                        CInfantryData newTechno;

                        newTechno.SubCell.Format("%d", command.Subpos);

                        newTechno.House = CIsoView::CurrentHouse();
                        newTechno.TypeID = STDHelpers::RandomSelect(randomList);
                        newTechno.X.Format("%d", mc.X);
                        newTechno.Y.Format("%d", mc.Y);

                        newTechno.Status = ExtConfigs::DefaultInfantryProperty.Status;
                        newTechno.Tag = ExtConfigs::DefaultInfantryProperty.Tag;
                        newTechno.Facing = CViewObjectsExt::PlacingRandomRandomFacing ? STDHelpers::GetRandomFacing() : ExtConfigs::DefaultInfantryProperty.Facing;
                        newTechno.VeterancyPercentage = ExtConfigs::DefaultInfantryProperty.VeterancyPercentage;
                        newTechno.Group = ExtConfigs::DefaultInfantryProperty.Group;
                        newTechno.IsAboveGround = ExtConfigs::DefaultInfantryProperty.IsAboveGround;
                        newTechno.AutoNORecruitType = ExtConfigs::DefaultInfantryProperty.AutoNORecruitType;
                        newTechno.AutoYESRecruitType = ExtConfigs::DefaultInfantryProperty.AutoYESRecruitType;
                        newTechno.Health = ExtConfigs::DefaultInfantryProperty.Health;

                        pMap->SetInfantryData(&newTechno, nullptr, nullptr, 0, -1);
                    }
                }
                else if (CViewObjectsExt::PlacingRandomTerrain >= 0
                    || CViewObjectsExt::PlacingRandomVehicle >= 0
                    || CViewObjectsExt::PlacingRandomStructure >= 0
                    || CViewObjectsExt::PlacingRandomAircraft >= 0
                    )
                {
                    std::vector<FString> randomList;
                    FString list;
                    int index = 0;
                    if (CViewObjectsExt::PlacingRandomTerrain >= 0)
                    {
                        CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Terrain);
                        list = "PlaceRandomTreeObList";
                        index = CViewObjectsExt::PlacingRandomTerrain;
                    }
                    else if (CViewObjectsExt::PlacingRandomVehicle >= 0)
                    {
                        CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Unit);
                        list = "PlaceRandomVehicleObList";
                        index = CViewObjectsExt::PlacingRandomVehicle;
                    }
                    else if (CViewObjectsExt::PlacingRandomStructure >= 0)
                    {
                        CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Building);
                        list = "PlaceRandomBuildingObList";
                        index = CViewObjectsExt::PlacingRandomStructure;
                    }
                    else if (CViewObjectsExt::PlacingRandomAircraft >= 0)
                    {
                        CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Aircraft);
                        list = "PlaceRandomAircraftObList";
                        index = CViewObjectsExt::PlacingRandomAircraft;
                    }

                    if (auto pSection = CINI::FAData().GetSection(list))
                    {
                        auto& section = *pSection->GetValueAt(index);
                        if (auto pSection2 = CINI::FAData().GetSection(section))
                        {
                            if (auto pSection2 = CINI::FAData().GetSection(section))
                            {
                                for (auto& pKey : pSection2->GetEntities())
                                {
                                    if (pKey.first.Find("Name") < 0 && pKey.first != "BannedTheater" && pKey.first != "RandomFacing" && pKey.first != "AIRepairs")
                                    {
                                        randomList.push_back(pKey.second);
                                    }
                                }
                                CViewObjectsExt::PlacingRandomRandomFacing = CINI::FAData().GetBool(section, "RandomFacing");
                                CViewObjectsExt::PlacingRandomStructureAIRepairs = CINI::FAData().GetBool(section, "AIRepairs");
                            }
                        }
                    }

                    auto& Map = CMapData::Instance();
                    for (const auto& mc : mapCoords)
                    {
                        auto cell = Map.TryGetCellAt(mc.X, mc.Y);
                        if (CViewObjectsExt::PlacingRandomTerrain >= 0)
                        {
                            if (cell->Terrain < 0)
                                Map.SetTerrainData(STDHelpers::RandomSelect(randomList), Map.GetCoordIndex(mc.X, mc.Y));
                        }
                        else if (CViewObjectsExt::PlacingRandomVehicle >= 0)
                        {
                            if (cell->Unit < 0)
                            {
                                CUnitData newTechno;
                                newTechno.Facing = CViewObjectsExt::PlacingRandomRandomFacing ? STDHelpers::GetRandomFacing() : ExtConfigs::DefaultUnitProperty.Facing;
                                newTechno.Health = ExtConfigs::DefaultUnitProperty.Health;
                                newTechno.Status = ExtConfigs::DefaultUnitProperty.Status;
                                newTechno.Tag = ExtConfigs::DefaultUnitProperty.Tag;
                                newTechno.VeterancyPercentage = ExtConfigs::DefaultUnitProperty.VeterancyPercentage;
                                newTechno.Group = ExtConfigs::DefaultUnitProperty.Group;
                                newTechno.IsAboveGround = ExtConfigs::DefaultUnitProperty.IsAboveGround;
                                newTechno.FollowsIndex = ExtConfigs::DefaultUnitProperty.FollowsIndex;
                                newTechno.AutoNORecruitType = ExtConfigs::DefaultUnitProperty.AutoNORecruitType;
                                newTechno.AutoYESRecruitType = ExtConfigs::DefaultUnitProperty.AutoYESRecruitType;

                                newTechno.House = CIsoView::CurrentHouse();
                                newTechno.TypeID = STDHelpers::RandomSelect(randomList);
                                newTechno.X.Format("%d", mc.X);
                                newTechno.Y.Format("%d", mc.Y);
                                Map.SetUnitData(&newTechno, nullptr, nullptr, 0, "");
                            }
                        }
                        else if (CViewObjectsExt::PlacingRandomStructure >= 0)
                        {
                            if (cell->Structure < 0)
                            {
                                CBuildingData newTechno;
                                newTechno.Facing = CViewObjectsExt::PlacingRandomRandomFacing ? STDHelpers::GetRandomFacing() : ExtConfigs::DefaultBuildingProperty.Facing;
                                newTechno.Health = ExtConfigs::DefaultBuildingProperty.Health;
                                newTechno.Tag = ExtConfigs::DefaultBuildingProperty.Tag;
                                newTechno.AISellable = ExtConfigs::DefaultBuildingProperty.AISellable;
                                newTechno.AIRebuildable = ExtConfigs::DefaultBuildingProperty.AIRebuildable;
                                newTechno.PoweredOn = ExtConfigs::DefaultBuildingProperty.PoweredOn;
                                newTechno.Upgrades = ExtConfigs::DefaultBuildingProperty.Upgrades;
                                newTechno.SpotLight = ExtConfigs::DefaultBuildingProperty.SpotLight;
                                newTechno.Upgrade1 = ExtConfigs::DefaultBuildingProperty.Upgrade1;
                                newTechno.Upgrade2 = ExtConfigs::DefaultBuildingProperty.Upgrade2;
                                newTechno.Upgrade3 = ExtConfigs::DefaultBuildingProperty.Upgrade3;
                                newTechno.AIRepairable = CViewObjectsExt::PlacingRandomStructureAIRepairs ? "1" : "0";
                                newTechno.Nominal = ExtConfigs::DefaultBuildingProperty.Nominal;

                                newTechno.House = CIsoView::CurrentHouse();
                                newTechno.TypeID = STDHelpers::RandomSelect(randomList);
                                newTechno.X.Format("%d", mc.X);
                                newTechno.Y.Format("%d", mc.Y);

                                Map.SetBuildingData(&newTechno, nullptr, nullptr, 0, "");
                            }
                        }
                        else if (CViewObjectsExt::PlacingRandomAircraft >= 0)
                        {
                            if (cell->Aircraft < 0)
                            {
                                CAircraftData newTechno;

                                newTechno.Facing = CViewObjectsExt::PlacingRandomRandomFacing ? STDHelpers::GetRandomFacing() : ExtConfigs::DefaultAircraftProperty.Facing;
                                newTechno.Health = ExtConfigs::DefaultAircraftProperty.Health;
                                newTechno.Status = ExtConfigs::DefaultAircraftProperty.Status;
                                newTechno.Tag = ExtConfigs::DefaultAircraftProperty.Tag;
                                newTechno.VeterancyPercentage = ExtConfigs::DefaultAircraftProperty.VeterancyPercentage;
                                newTechno.Group = ExtConfigs::DefaultAircraftProperty.Group;
                                newTechno.AutoNORecruitType = ExtConfigs::DefaultAircraftProperty.AutoNORecruitType;
                                newTechno.AutoYESRecruitType = ExtConfigs::DefaultAircraftProperty.AutoYESRecruitType;

                                newTechno.House = CIsoView::CurrentHouse();
                                newTechno.TypeID = STDHelpers::RandomSelect(randomList);
                                newTechno.X.Format("%d", mc.X);
                                newTechno.Y.Format("%d", mc.Y);

                                Map.SetAircraftData(&newTechno, nullptr, nullptr, 0, "");
                            }
                        }
                    }
                }
                else
                {
                    if (command.Type == 8) // smudge
                    {
                        CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Smudge);
                        for (const auto& mc : mapCoords)
                        {
                            auto cell = pMap->GetCellAt(mc.X, mc.Y);
                            CSmudgeData smudge;
                            smudge.X = mc.Y;
                            smudge.Y = mc.X;//opposite
                            smudge.Flag = 0;
                            smudge.TypeID = command.ObjectID;

                            if (cell->Smudge < 0)
                            {
                                auto& rules = Variables::RulesMap;
                                //check overlapping
                                int width = rules.GetInteger(smudge.TypeID, "Width", 1);
                                int height = rules.GetInteger(smudge.TypeID, "Height", 1);
                                bool place = true;
                                for (auto& thisSmudge : pMap->SmudgeDatas)
                                {
                                    if (thisSmudge.X <= 0 || thisSmudge.Y <= 0 || thisSmudge.Flag)
                                        continue;
                                    int thisWidth = rules.GetInteger(thisSmudge.TypeID, "Width", 1);
                                    int thisHeight = rules.GetInteger(thisSmudge.TypeID, "Height", 1);
                                    int thisX = thisSmudge.Y;
                                    int thisY = thisSmudge.X;//opposite
                                    for (int i = 0; i < thisWidth; i++)
                                        for (int j = 0; j < thisHeight; j++)
                                            for (int k = 0; k < width; k++)
                                                for (int l = 0; l < height; l++)
                                                    if (thisY + i == mc.Y + k && thisX + j == mc.X + l)
                                                        place = false;

                                }
                                if (place)
                                {
                                    pMap->SetSmudgeData(&smudge);
                                }
                            }
                        }
                    }
                    else
                    {
                        switch (CIsoView::CurrentCommand->Type)
                        {
                        case 1:
                            CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Infantry);
                            break;
                        case 2:
                            CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Building);
                            break;
                        case 3:
                            CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Aircraft);
                            break;
                        case 4:
                            CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Unit);
                            break;
                        case 5:
                            CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Terrain);
                            break;
                        case 8:
                            CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Smudge);
                            break;
                        default:
                            break;
                        }
                        CIsoViewExt::LastCommand::requestSubpos = true;
                        for (const auto& mc : mapCoords)
                        {
                            CIsoView::GetInstance()->DrawMouseAttachedStuff(mc.X, mc.Y);
                        }
                        CIsoViewExt::LastCommand::requestSubpos = false;
                    }
                }
            }
            else if (command.Command == 22) // random tree
            {
                CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Terrain);
                for (const auto& mc : mapCoords)
                {
                    CIsoView::GetInstance()->DrawMouseAttachedStuff(mc.X, mc.Y);
                }
            }
            else if (command.Command == 4) // place tag
            {
                for (auto& m : mapCoords)
                {
                    int pos = pMap->GetCoordIndex(m.X, m.Y);
                    pMap->AddCelltag(CIsoView::CurrentCommand->ObjectID, pos);
                }
                CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            }
            command.reset();
        }
        else
        {
            command.record(X, Y);
        }
        return 0x466860;
    }
    command.reset();

    if (CIsoView::CurrentCommand->Command == 0x17)
    {
        CViewObjectsExt::ApplyPropertyBrush(X, Y);

        return 0x466860;
    }
    else if (CIsoView::CurrentCommand->Command == 0x18)
    {
        CViewObjectsExt::ApplyInfantrySubCell(X, Y);

        return 0x466860;
    }
    else if (CIsoView::CurrentCommand->Command == 0x1A)
    {
        if (CIsoView::CurrentCommand->Type == 0 || CIsoView::CurrentCommand->Type == 1)
        {
            CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Basenode, true);
            CViewObjectsExt::MoveBaseNodeOrder(X, Y);
            return 0x466860;
        }
        else if (CIsoView::CurrentCommand->Type == 2)
        {
            CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Basenode, true);
            CViewObjectsExt::MoveBaseNode(X, Y);
            return 0x466860;
        }
    }
    else if (CIsoView::CurrentCommand->Command == 0x1E)
    {
        CViewObjectsExt::PlaceConnectedTile_OnLButtonDown(X, Y);

        return 0x466860;
    }
    else if (CIsoView::CurrentCommand->Command == 0x1F)
    {
        if (CTerrainGenerator::RangeFirstCell.X < 0) {
            CTerrainGenerator::RangeFirstCell.X = X;
            CTerrainGenerator::RangeFirstCell.Y = Y;
            return 0x466860;
        }
        else if (CTerrainGenerator::RangeSecondCell.X < 0) {
            CTerrainGenerator::RangeSecondCell.X = X;
            CTerrainGenerator::RangeSecondCell.Y = Y;
            CTerrainGenerator::OnSetRangeDone();
            return 0x46686A;
        }
    }
    else if (CIsoView::CurrentCommand->Command == 0x20)
    {
        CViewObjectsExt::ModifyOre(X, Y);

        return 0x466860;
    }
    else if (CIsoView::CurrentCommand->Command == 0x21)
    {
        if (CIsoView::CurrentCommand->Type == 0)
        {
            CViewObjectsExt::AddAnnotation(X, Y);
            return 0x466860;
        }
        else
        {
            CViewObjectsExt::RemoveAnnotation(X, Y);
            return 0x466860;
        }
    }
    else if (CIsoView::CurrentCommand->Command == 0x1D && CIsoView::CurrentCommand->Type == 3)
    {
        CViewObjectsExt::BatchAddMultiSelection(X, Y, true);

        return 0x466860;
    }
    else if (CIsoView::CurrentCommand->Command == 0x1D && CIsoView::CurrentCommand->Type == 4)
    {
        CViewObjectsExt::BatchAddMultiSelection(X, Y, false);

        return 0x466860;
    }
    else if (CIsoView::CurrentCommand->Command == 0x1D && CIsoView::CurrentCommand->Type == 10)
    {
        CViewObjectsExt::SquareBatchAddMultiSelection(X, Y, true);

        return 0x466860;
    }
    else if (CIsoView::CurrentCommand->Command == 0x1D && CIsoView::CurrentCommand->Type == 11)
    {
        CViewObjectsExt::SquareBatchAddMultiSelection(X, Y, false);

        return 0x466860;
    }	
    else if (CIsoView::CurrentCommand->Command == 0x22)
    {
        if (!CIsoViewExt::IsPressingTube)
        {
            CIsoViewExt::TubeNodes.clear();
        }
        CIsoViewExt::IsPressingTube = true;
        if (CIsoViewExt::TubeNodes.empty())
        {
            CIsoViewExt::TubeNodes.push_back({ X,Y });
        }
        else
        {
            if (CIsoViewExt::TubeNodes.back() != MapCoord{ X, Y })
                CIsoViewExt::TubeNodes.push_back({ X,Y });
        }
        return 0x466860;
    }	
    else if (CIsoView::CurrentCommand->Command == 0x6)
    {
        CViewObjectsExt::DeleteTube(X, Y);
        return 0x466860;
    }	
    else if (CIsoView::CurrentCommand->Command == 0x23)
    {
        CLuaConsole::UpdateCoords(X, Y, CLuaConsole::applyingScriptFirst, false);
        CLuaConsole::OnClickRun(CLuaConsole::runFile);
        return 0x466860;
    }	
    else if (CIsoView::CurrentCommand->Command == 0x24)
    {
        if (CIsoView::CurrentCommand->Type < 2)
            CViewObjectsExt::SetWpTagColor(X, Y, CIsoView::CurrentCommand->Type == 0);
        else
            CViewObjectsExt::RemoveWpTagColor(X, Y, CIsoView::CurrentCommand->Type == 2);
        return 0x466860;
    }
    else if (CIsoView::CurrentCommand->Command == 1 && CIsoView::CurrentCommand->Type == 7) // change owner
    {
        CViewObjectsExt::ApplyChangeOwner(X, Y);
        return 0x466860;
    }
    else if (CIsoView::CurrentCommand->Command == 0x25)
    {        
        // 0-9: trigger editors
        if (CIsoView::CurrentCommand->Type >= 0
            && CIsoView::CurrentCommand->Type <= 9
            && CNewTrigger::Instance[CIsoView::CurrentCommand->Type].CurrentTrigger)
        {
            CViewObjectsExt::ApplyTag(X, Y, CNewTrigger::Instance[CIsoView::CurrentCommand->Type].CurrentTrigger->Tag);
            return 0x466860;
        }
    }

    return 0;
}

DEFINE_HOOK(45BF73, CIsoView_OnMouseMove_PropertyBrush, 9)
{
    GET(const int, X, EDI);
    GET(const int, Y, EBX);

    if (CIsoView::CurrentCommand->Command == 0x17)
    {
        CViewObjectsExt::ApplyPropertyBrush(X, Y);

        return 0x45CD6D;
    }    
    else if (CIsoView::CurrentCommand->Command == 0x18)
    {
        CViewObjectsExt::ApplyInfantrySubCell(X, Y);

        return 0x45CD6D;
    }        
    else if (CIsoView::CurrentCommand->Command == 0x1A)
    {
        if (CIsoView::CurrentCommand->Type == 0 || CIsoView::CurrentCommand->Type == 1)
        {
            CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Basenode, true);
            CViewObjectsExt::MoveBaseNodeOrder(X, Y);
            return 0x45CD6D;
        }
    }         
    else if (CIsoView::CurrentCommand->Command == 0x20)
    {
        CViewObjectsExt::ModifyOre(X, Y);

        return 0x45CD6D;
    }    
    else if (CIsoView::CurrentCommand->Command == 0x21)
    {
        if (CIsoView::CurrentCommand->Type == 0)
        {
            CViewObjectsExt::AddAnnotation(X, Y);
            return 0x45CD6D;
        }
        else
        {
            CViewObjectsExt::RemoveAnnotation(X, Y);
            return 0x45CD6D;
        }
    }
    else if (CIsoView::CurrentCommand->Command == 0x1D && CIsoView::CurrentCommand->Type == 3)
    {
        CViewObjectsExt::BatchAddMultiSelection(X, Y, true);

        return 0x45CD6D;
    }        
    else if (CIsoView::CurrentCommand->Command == 0x1D && CIsoView::CurrentCommand->Type == 4)
    {
        CViewObjectsExt::BatchAddMultiSelection(X, Y, false);

        return 0x45CD6D;
    }
    else if (CIsoView::CurrentCommand->Command == 0x1D && CIsoView::CurrentCommand->Type == 10)
    {
        CViewObjectsExt::SquareBatchAddMultiSelection(X, Y, true);

        return 0x45CD6D;
    }        
    else if (CIsoView::CurrentCommand->Command == 0x1D && CIsoView::CurrentCommand->Type == 11)
    {
        CViewObjectsExt::SquareBatchAddMultiSelection(X, Y, false);

        return 0x45CD6D;
    }
    else if (CIsoView::CurrentCommand->Command == 0x6)
    {
        CViewObjectsExt::DeleteTube(X, Y);
        return 0x45CD6D;
    }
    else if (CIsoView::CurrentCommand->Command == 0x23)
    {
        CLuaConsole::UpdateCoords(X, Y, CLuaConsole::applyingScriptFirst, true);
        CLuaConsole::OnClickRun(CLuaConsole::runFile);
        return 0x45CD6D;
    }
    else if (CIsoView::CurrentCommand->Command == 0x24)
    {
        if (CIsoView::CurrentCommand->Type < 2)
            CViewObjectsExt::SetWpTagColor(X, Y, CIsoView::CurrentCommand->Type == 0);
        else
            CViewObjectsExt::RemoveWpTagColor(X, Y, CIsoView::CurrentCommand->Type == 2);
        return 0x45CD6D;
    }
    else if (CIsoView::CurrentCommand->Command == 1 && CIsoView::CurrentCommand->Type == 7) // change owner
    {
        CViewObjectsExt::ApplyChangeOwner(X, Y);
        return 0x45CD6D;
    }
    else if (CIsoView::CurrentCommand->Command == 0x25)
    {
        // 0-9: trigger editors
        if (CIsoView::CurrentCommand->Type >= 0
            && CIsoView::CurrentCommand->Type <= 9
            && CNewTrigger::Instance[CIsoView::CurrentCommand->Type].CurrentTrigger)
        {
            CViewObjectsExt::ApplyTag(X, Y, CNewTrigger::Instance[CIsoView::CurrentCommand->Type].CurrentTrigger->Tag);
            return 0x45CD6D;
        }
    }

    return CIsoView::CurrentCommand->Command == FACurrentCommand::WaypointHandle ? 0x45BF7C : 0x45C168;
}

// Now indices can be updated
// Add a house won't update indices, so there might be hidden risks if not reloading the map.
// That's why these hooks are not used.
//DEFINE_HOOK_AGAIN(40A5CB, CINIEditor_Update, 6)
DEFINE_HOOK(44EB1C, CHouses_ONBNDeleteHouseClicked_UpdateTreeview, 7)
{
    CMapDataExt::UpdateMapSectionIndicies("Houses");
    CMapDataExt::UpdateMapSectionIndicies("Countries");
    return 0;
}

DEFINE_HOOK(44E320, CHouses_ONBNAddHouseClicked_UpdateTreeview, 7)
{
    //CFinalSunDlg::Instance->MyViewFrame.pViewObjects->Update();
    CMapDataExt::UpdateMapSectionIndicies("Houses");
    CMapDataExt::UpdateMapSectionIndicies("Countries");
    return 0;
}

//DEFINE_HOOK(51AFB8, ObjectBrowserControl_OnSelectedChanged, 6)
//{
//    GET_STACK(ObjectBrowserControlExt*, pThis, 0x10);
//    GET(int, nData, ECX);
//    return pThis->UpdateEngine(nData);
//}

//void PrintNode(CTreeCtrl* pTree, HTREEITEM hNode)
//{
//    static int depth = 0;
//    if (pTree->ItemHasChildren(hNode))
//    {
//        HTREEITEM hNextItem;
//        HTREEITEM hChildItem = pTree->GetChildItem(hNode);
//
//        while (hChildItem != NULL)
//        {
//            CString spaces;
//            for (int i = 0; i < depth; ++i)
//                spaces += "    ";
//            Logger::Debug("%s%s %d\n", spaces, pTree->GetItemText(hChildItem), pTree->GetItemData(hChildItem));
//            if (pTree->ItemHasChildren(hChildItem))
//            {
//                ++depth;
//                PrintNode(pTree, hChildItem);
//                --depth;
//            }
//            hNextItem = pTree->GetNextItem(hChildItem, TVGN_NEXT);
//            hChildItem = hNextItem;
//        }
//    }
//}
//
//DEFINE_HOOK(523139, ObjectBrowserControl_Redraw_End, 5)
//{
//    GET(CTreeCtrl*, pTree, ESI);
//    auto hRoot = pTree->GetSelectedItem();
//    
//    PrintNode(pTree, hRoot);
//    
//    return 0;
//}