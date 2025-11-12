#include "MultiSelection.h"
#include "CopyPaste.h"
#include <CFinalSunDlg.h>
#include <CIsoView.h>
#include <Helpers/Macro.h>
#include <Drawing.h>
#include <CTileTypeClass.h>
#include <CLoading.h>
#include <CMapData.h>
#include "../Ext/CIsoView/Body.h"
#include "../Ext/CMapData/Body.h"
#include <span>
#include "../Helpers/TheaterHelpers.h"

std::set<MapCoord> CopyPaste::PastedCoords;
bool CopyPaste::CopyWholeMap = false;
bool CopyPaste::OnLButtonDownPasted = false;
bool CopyPaste::IsCutting = false;
std::vector<TileRule> CopyPaste::TileConvertRules;

const char* CopyPaste::GetString(const MyClipboardData& cell, const StringField& field, MyClipboardData* pBufferBase)
{
    if (field.Length == 0 || field.Offset == 0)
        return "";
    return (const char*)((char*)pBufferBase + field.Offset);
}

void CopyPaste::Copy(const std::set<MapCoord>& coords)
{
    std::vector<MyClipboardData> data;
    std::vector<std::string> extraStrings;

    int relativeHeight = 14;
    int lowest = 14;
    int highest = 0;

    auto pushString = [&](const char* str, StringField& field) {
        if (!str || !*str) {
            field.Offset = 0;
            field.Length = 0;
            return;
        }
        field.Offset = 0xFFFFFFFF; // place holder
        field.Length = (uint16_t)strlen(str);
        extraStrings.push_back(str);
        };

    int objectMask = 0;
    for (const auto& coords : coords)
    {
        if (!ExtConfigs::DisplayObjectsOutside && !CMapData::Instance->IsCoordInMap(coords.X, coords.Y)
            || ExtConfigs::DisplayObjectsOutside && !CMapDataExt::IsCoordInFullMap(coords.X, coords.Y))
            continue;

        auto pos = CMapData::Instance->GetCoordIndex(coords.X, coords.Y);
        auto pCell = CMapData::Instance->GetCellAt(pos);
        auto& pCellExt = CMapDataExt::CellDataExts[pos];

        MyClipboardData item = {};
        item.X = coords.X;
        item.Y = coords.Y;
        item.Overlay = pCellExt.NewOverlay;
        item.OverlayData = pCell->OverlayData;
        item.TileIndex = pCell->TileIndex;
        item.TileIndexHiPart = pCell->TileIndexHiPart;
        item.TileSubIndex = pCell->TileSubIndex;
        item.TileSet = CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(pCell->TileIndex)].TileSet;
        item.TileSetSubIndex = pCell->TileIndex - CMapDataExt::TileSet_starts[item.TileSet];
        item.Height = pCell->Height;
        item.IceGrowth = pCell->IceGrowth;
        item.Flag = pCell->Flag;

        relativeHeight = std::min(relativeHeight, pCell->Height -
            CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(pCell->TileIndex)].TileBlockDatas[pCell->TileSubIndex].Height);
        lowest = std::min(lowest, (int)pCell->Height);
        highest = std::max(highest, (int)pCell->Height);

        if (pCell->Terrain > -1)
        {
            pushString(Variables::RulesMap.GetValueAt("TerrainTypes", pCell->TerrainType), item.TerrainData);
            objectMask |= ObjectRecord::RecordType::Terrain;
        }

        if (pCell->Smudge > -1) {
            auto& smudge = CMapData::Instance->SmudgeDatas[pCell->Smudge];
            if (smudge.X == coords.Y && smudge.Y == coords.X)
            {
                pushString(Variables::RulesMap.GetValueAt("SmudgeTypes", pCell->SmudgeType), item.SmudgeData);
                objectMask |= ObjectRecord::RecordType::Smudge;
            }
        }

        if (pCell->Structure > -1) {
            int iniIndex = CMapDataExt::StructureIndexMap[pCell->Structure];
            ppmfc::CString value = CINI::CurrentDocument->GetValueAt("Structures", iniIndex);
            auto atoms = STDHelpers::SplitString(value, 16);
            if (coords.X == atoi(atoms[4]) && coords.Y == atoi(atoms[3]))
            {
                pushString(value.m_pchData, item.BuildingData);
                objectMask |= ObjectRecord::RecordType::Building;
            }
        }

        if (pCell->Aircraft > -1) {
            ppmfc::CString value = CINI::CurrentDocument->GetValueAt("Aircraft", pCell->Aircraft);
            pushString(value.m_pchData, item.AircraftData);
            objectMask |= ObjectRecord::RecordType::Aircraft;
        }

        if (pCell->Infantry[0] > -1) {
            ppmfc::CString value = CINI::CurrentDocument->GetValueAt("Infantry", pCell->Infantry[0]);
            pushString(value.m_pchData, item.InfantryData_1);
            objectMask |= ObjectRecord::RecordType::Infantry;
        }
        if (pCell->Infantry[1] > -1) {
            ppmfc::CString value = CINI::CurrentDocument->GetValueAt("Infantry", pCell->Infantry[1]);
            pushString(value.m_pchData, item.InfantryData_2);
            objectMask |= ObjectRecord::RecordType::Infantry;
        }
        if (pCell->Infantry[2] > -1) {
            ppmfc::CString value = CINI::CurrentDocument->GetValueAt("Infantry", pCell->Infantry[2]);
            pushString(value.m_pchData, item.InfantryData_3);
            objectMask |= ObjectRecord::RecordType::Infantry;
        }

        if (pCell->Unit > -1) {
            ppmfc::CString value = CINI::CurrentDocument->GetValueAt("Units", pCell->Unit);
            pushString(value.m_pchData, item.UnitData);
            objectMask |= ObjectRecord::RecordType::Unit;
        }
        data.push_back(item);
    }

    int realLowest = lowest;
    lowest -= relativeHeight;
    highest -= relativeHeight;
    while (lowest < 0) {
        relativeHeight--;
        lowest++;
    }
    while (highest > 14) {
        relativeHeight++;
        highest--;
    }
    if (relativeHeight != 0) {
        for (auto& cell : data) {
            cell.Height -= relativeHeight;
        }
    }

    size_t headerSize = 16;
    size_t structSize = sizeof(MyClipboardData) * data.size();
    size_t stringsSize = 0;
    for (auto& s : extraStrings) {
        stringsSize += s.size() + 1; // \0
    }
    size_t totalSize = headerSize + structSize + stringsSize;

    HGLOBAL hGlobal = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, totalSize);
    if (!hGlobal) {
        MessageBox(NULL, "Failed to allocate global memory!", "Error", MB_OK);
        return;
    }

    char* pBuffer = (char*)GlobalLock(hGlobal);
    if (!pBuffer) {
        MessageBox(NULL, "Failed to lock hGlobal handle!", "Error", MB_OK);
        GlobalFree(hGlobal);
        return;
    }

    reinterpret_cast<int*>(pBuffer)[0] = 0; // multi-selection flag
    reinterpret_cast<size_t*>(pBuffer)[1] = data.size();
    reinterpret_cast<int*>(pBuffer)[2] = CLoading::Instance->TheaterIdentifier;
    reinterpret_cast<int*>(pBuffer)[3] = objectMask;

    memcpy(pBuffer + headerSize, data.data(), structSize);

    char* pWrite = pBuffer + headerSize + structSize;
    MyClipboardData* arr = (MyClipboardData*)(pBuffer + headerSize);

    size_t strIndex = 0;
    auto fixOffset = [&](StringField& field) {
        if (field.Length > 0 && field.Offset == 0xFFFFFFFF) {
            const std::string& s = extraStrings[strIndex++];
            field.Offset = (uint32_t)(pWrite - pBuffer - headerSize);
            memcpy(pWrite, s.c_str(), s.size() + 1);
            pWrite += s.size() + 1;
        }
        };

    for (size_t i = 0; i < data.size(); ++i) {
        auto& item = arr[i];
        fixOffset(item.TerrainData);
        fixOffset(item.SmudgeData);
        fixOffset(item.BuildingData);
        fixOffset(item.AircraftData);
        fixOffset(item.InfantryData_1);
        fixOffset(item.InfantryData_2);
        fixOffset(item.InfantryData_3);
        fixOffset(item.UnitData);
    }

    GlobalUnlock(hGlobal);

    OpenClipboard(CFinalSunApp::Instance->m_pMainWnd->m_hWnd);
    EmptyClipboard();
    if (!SetClipboardData(CFinalSunApp::Instance->ClipboardFormat, hGlobal)) {
        MessageBox(NULL, "Failed to set clipboard data", "Error", 0);
    }
    CloseClipboard();

    if (IsCutting)
    {
        RECT bounds
        {
            std::numeric_limits<LONG>::max(),
            std::numeric_limits<LONG>::max(),
            std::numeric_limits<LONG>::min(),
            std::numeric_limits<LONG>::min()
        };
        for (const auto& cell : coords)
        {
            bounds.left = std::min<LONG>(bounds.left, cell.X);
            bounds.right = std::max<LONG>(bounds.right, cell.X);
            bounds.top = std::min<LONG>(bounds.top, cell.Y);
            bounds.bottom = std::max<LONG>(bounds.bottom, cell.Y);
        }

        ObjectRecord::RecordType all = static_cast<ObjectRecord::RecordType>(
            ObjectRecord::Building | ObjectRecord::Unit | ObjectRecord::Aircraft | ObjectRecord::Infantry |
            ObjectRecord::Terrain | ObjectRecord::Smudge
            );
        CMapDataExt::MakeMixedRecord(bounds.left, bounds.top,
            bounds.right + 2, bounds.bottom + 2, all);

        for (const auto& cell : coords)
        {
            auto nIndex = CMapData::Instance->GetCoordIndex(cell.X, cell.Y);
            auto pCell = CMapData::Instance->GetCellAt(nIndex);

            if (CIsoViewExt::PasteInfantries)
                for (int subpos = 0; subpos < 3; subpos++)
                    if (pCell->Infantry[subpos] != -1)
                        CMapData::Instance->DeleteInfantryData(pCell->Infantry[subpos]);
            if (CIsoViewExt::PasteUnits && pCell->Unit != -1)
                CMapData::Instance->DeleteUnitData(pCell->Unit);
            if (CIsoViewExt::PasteAircrafts && pCell->Aircraft != -1)
                CMapData::Instance->DeleteAircraftData(pCell->Aircraft);
            if (CIsoViewExt::PasteStructures && pCell->Structure != -1)
                CMapData::Instance->DeleteBuildingData(pCell->Structure);
            if (CIsoViewExt::PasteTerrains && pCell->Terrain != -1)
                CMapData::Instance->DeleteTerrainData(pCell->Terrain);
            if (CIsoViewExt::PasteSmudges && pCell->Smudge != -1)
                CMapData::Instance->DeleteSmudgeData(pCell->Smudge);
            if (CIsoViewExt::PasteOverlays)
                CMapDataExt::GetExtension()->SetNewOverlayAt(nIndex, 0xFFFF);
            if (CIsoViewExt::PasteGround)
            {
                CMapDataExt::GetExtension()->PlaceTileAt(cell.X, cell.Y, 0, 3);
                pCell->Height = (cell.X, cell.Y, realLowest);
            }
        }
        CIsoView::GetInstance()->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

void CopyPaste::Paste(int X, int Y, int nBaseHeight, MyClipboardData* data, size_t length, int recordType)
{
    CopyPaste::PastedCoords.clear();
    if (X < 0 || Y < 0 || X > CMapData::Instance().MapWidthPlusHeight || Y > CMapData::Instance().MapWidthPlusHeight)
        return;
    std::span<MyClipboardData> cells{ data, data + length };

    RECT bounds
    {
        std::numeric_limits<LONG>::max(),
        std::numeric_limits<LONG>::max(),
        std::numeric_limits<LONG>::min(),
        std::numeric_limits<LONG>::min()
    };
    for (const auto& cell : cells)
    {
        bounds.left = std::min<LONG>(bounds.left, cell.X);
        bounds.right = std::max<LONG>(bounds.right, cell.X);
        bounds.top = std::min<LONG>(bounds.top, cell.Y);
        bounds.bottom = std::max<LONG>(bounds.bottom, cell.Y);
    }
    const MapCoord center = { (int)std::ceil(double(bounds.left + bounds.right) / 2.0), (int)std::ceil(double(bounds.top + bounds.bottom) / 2.0) };

    if (!OnLButtonDownPasted)
    {
        CMapData::Instance->SaveUndoRedoData(true, bounds.left - center.X + X, bounds.top - center.Y + Y,
            bounds.right - center.X + X + 2, bounds.bottom - center.Y + Y + 2);
    }
    else
    {
        if (CIsoViewExt::PasteOverriding)
        {
            for (const auto& cell : cells)
            {
                int offset_x = cell.X - center.X;
                int offset_y = cell.Y - center.Y;
                if (!ExtConfigs::DisplayObjectsOutside && !CMapData::Instance->IsCoordInMap(X + offset_x, Y + offset_y)
                    || ExtConfigs::DisplayObjectsOutside && !CMapDataExt::IsCoordInFullMap(X + offset_x, Y + offset_y))
                    continue;

                auto nCellIndex = CMapData::Instance->GetCoordIndex(X + offset_x, Y + offset_y);
                auto pCell = CMapData::Instance->GetCellAt(nCellIndex);

                if (CIsoViewExt::PasteInfantries)
                    for (int subpos = 0; subpos < 3; subpos++)
                        if (pCell->Infantry[subpos] != -1)
                            recordType |= ObjectRecord::RecordType::Infantry;
                if (CIsoViewExt::PasteUnits && pCell->Unit != -1)
                    recordType |= ObjectRecord::RecordType::Unit;
                if (CIsoViewExt::PasteAircrafts && pCell->Aircraft != -1)
                    recordType |= ObjectRecord::RecordType::Aircraft;
                if (CIsoViewExt::PasteStructures && pCell->Structure != -1)
                    recordType |= ObjectRecord::RecordType::Building;
                if (CIsoViewExt::PasteTerrains && pCell->Terrain != -1)
                    recordType |= ObjectRecord::RecordType::Terrain;
                if (CIsoViewExt::PasteSmudges && pCell->Smudge != -1)
                    recordType |= ObjectRecord::RecordType::Smudge;
            }
        }
        CMapDataExt::MakeMixedRecord(bounds.left - center.X + X, bounds.top - center.Y + Y,
            bounds.right - center.X + X + 2, bounds.bottom - center.Y + Y + 2, recordType);
    }

    if (OnLButtonDownPasted && CIsoViewExt::PasteOverriding)
    {
        for (const auto& cell : cells)
        {
            int offset_x = cell.X - center.X;
            int offset_y = cell.Y - center.Y;
            if (!ExtConfigs::DisplayObjectsOutside && !CMapData::Instance->IsCoordInMap(X + offset_x, Y + offset_y)
                || ExtConfigs::DisplayObjectsOutside && !CMapDataExt::IsCoordInFullMap(X + offset_x, Y + offset_y))
                continue;

            auto nCellIndex = CMapData::Instance->GetCoordIndex(X + offset_x, Y + offset_y);
            auto pCell = CMapData::Instance->GetCellAt(nCellIndex);

            if (CIsoViewExt::PasteInfantries)
                for (int subpos = 0; subpos < 3; subpos++)
                    if (pCell->Infantry[subpos] != -1)
                        CMapData::Instance->DeleteInfantryData(pCell->Infantry[subpos]);
            if (CIsoViewExt::PasteUnits && pCell->Unit != -1)
                CMapData::Instance->DeleteUnitData(pCell->Unit);
            if (CIsoViewExt::PasteAircrafts && pCell->Aircraft != -1)
                CMapData::Instance->DeleteAircraftData(pCell->Aircraft);
            if (CIsoViewExt::PasteStructures && pCell->Structure != -1)
                CMapData::Instance->DeleteBuildingData(pCell->Structure);
            if (CIsoViewExt::PasteTerrains && pCell->Terrain != -1)
                CMapData::Instance->DeleteTerrainData(pCell->Terrain);
            if (CIsoViewExt::PasteSmudges && pCell->Smudge != -1)
                CMapData::Instance->DeleteSmudgeData(pCell->Smudge);
        }
    }

    auto lowest_height = 14;
    for (const auto& cell : cells)
    {
        int offset_x = cell.X - center.X;
        int offset_y = cell.Y - center.Y;
        if (!ExtConfigs::DisplayObjectsOutside && !CMapData::Instance->IsCoordInMap(X + offset_x, Y + offset_y)
            || ExtConfigs::DisplayObjectsOutside && !CMapDataExt::IsCoordInFullMap(X + offset_x, Y + offset_y))
            continue;

        const auto pCell = CMapData::Instance->TryGetCellAt(X + offset_x, Y + offset_y);
        if (pCell->Height < lowest_height)
            lowest_height = pCell->Height;
    }

    nBaseHeight += lowest_height;
    for (const auto& cell : cells)
    {
        int offset_x = cell.X - center.X;
        int offset_y = cell.Y - center.Y;
        if (!ExtConfigs::DisplayObjectsOutside && !CMapData::Instance->IsCoordInMap(X + offset_x, Y + offset_y)
            || ExtConfigs::DisplayObjectsOutside && !CMapDataExt::IsCoordInFullMap(X + offset_x, Y + offset_y))
            continue;

        auto nCellIndex = CMapData::Instance->GetCoordIndex(X + offset_x, Y + offset_y);
        auto pCell = CMapData::Instance->GetCellAt(nCellIndex);
        auto& pCellExt = CMapDataExt::CellDataExts[nCellIndex];

        if (CIsoViewExt::PasteOverlays)
        {
            CMapData::Instance->DeleteTiberium(std::min(pCellExt.NewOverlay, (word)0xFF), pCell->OverlayData);
            pCell->Overlay = std::min(cell.Overlay, (word)0xff);
            pCellExt.NewOverlay = cell.Overlay;
            pCell->OverlayData = cell.OverlayData;
            CMapDataExt::NewOverlay[Y + offset_y + (X + offset_x) * 512] = cell.Overlay;
            CMapData::Instance->AddTiberium(std::min(pCellExt.NewOverlay, (word)0xFF), pCell->OverlayData);
        }
        if (CIsoViewExt::PasteGround)
        {
            if (!TileConvertRules.empty())
            {
                pCell->TileIndex = cell.TileIndex;
                pCell->TileSubIndex = cell.TileSubIndex;
                pCell->Height = std::clamp(cell.Height + nBaseHeight, 0, 14);
                ConvertTile(*pCell);
            }
            else
            {
                if (cell.TileSet < CMapDataExt::TileSet_starts.size() - 1
                    && CMapDataExt::TileSet_starts[cell.TileSet] + cell.TileSetSubIndex < CMapDataExt::TileSet_starts[cell.TileSet + 1])
                    pCell->TileIndex = CMapDataExt::TileSet_starts[cell.TileSet] + cell.TileSetSubIndex;
                else
                    pCell->TileIndex = 0;
                pCell->TileSubIndex = cell.TileSubIndex;
                pCell->Height = std::clamp(cell.Height + nBaseHeight, 0, 14);
            }

            pCell->TileIndexHiPart = cell.TileIndexHiPart;
            pCell->IceGrowth = cell.IceGrowth;
            pCell->Flag = cell.Flag;
        }

        const char* buildingStr = GetString(cell, cell.BuildingData, data);
        if (OnLButtonDownPasted && CIsoViewExt::PasteStructures && *buildingStr)
        {
            FString value(buildingStr);
            auto atoms = FString::SplitString(value, 16);
            CBuildingData data;
            data.House = atoms[0];
            data.TypeID = atoms[1];
            data.Health = atoms[2];
            data.Y.Format("%d", Y + offset_y);
            data.X.Format("%d", X + offset_x);
            data.Facing = atoms[5];
            data.Tag = atoms[6];
            data.AISellable = atoms[7];
            data.AIRebuildable = atoms[8];
            data.PoweredOn = atoms[9];
            data.Upgrades = atoms[10];
            data.SpotLight = atoms[11];
            data.Upgrade1 = atoms[12];
            data.Upgrade2 = atoms[13];
            data.Upgrade3 = atoms[14];
            data.AIRepairable = atoms[15];
            data.Nominal = atoms[16];
            CMapData::Instance->SetBuildingData(&data, NULL, NULL, 0, "");
        }

        const char* smudgeStr = GetString(cell, cell.SmudgeData, data);
        if (OnLButtonDownPasted && CIsoViewExt::PasteSmudges && *smudgeStr)
        {
            CSmudgeData data;
            data.TypeID = smudgeStr;
            data.X = Y + offset_y;
            data.Y = X + offset_x;
            CMapData::Instance->SetSmudgeData(&data);
        }

        const char* terrainStr = GetString(cell, cell.TerrainData, data);
        if (OnLButtonDownPasted && CIsoViewExt::PasteTerrains && *terrainStr)
        {
            CMapData::Instance->SetTerrainData(terrainStr, CMapData::Instance->GetCoordIndex(X + offset_x, Y + offset_y));
        }

        const char* unitStr = GetString(cell, cell.UnitData, data);
        if (OnLButtonDownPasted && CIsoViewExt::PasteUnits && *unitStr)
        {
            FString value(unitStr);
            auto atoms = FString::SplitString(value, 13);
            CUnitData data;
            data.House = atoms[0];
            data.TypeID = atoms[1];
            data.Health = atoms[2];
            data.Y.Format("%d", Y + offset_y);
            data.X.Format("%d", X + offset_x);
            data.Facing = atoms[5];
            data.Status = atoms[6];
            data.Tag = atoms[7];
            data.VeterancyPercentage = atoms[8];
            data.Group = atoms[9];
            data.IsAboveGround = atoms[10];
            data.FollowsIndex = atoms[11];
            data.AutoNORecruitType = atoms[12];
            data.AutoYESRecruitType = atoms[13];
            CMapData::Instance->SetUnitData(&data, NULL, NULL, 0, "");
        }

        const char* aircraftStr = GetString(cell, cell.AircraftData, data);
        if (OnLButtonDownPasted && CIsoViewExt::PasteAircrafts && *aircraftStr)
        {
            FString value(aircraftStr);
            auto atoms = FString::SplitString(value, 11);
            CAircraftData data;
            data.House = atoms[0];
            data.TypeID = atoms[1];
            data.Health = atoms[2];
            data.Y.Format("%d", Y + offset_y);
            data.X.Format("%d", X + offset_x);
            data.Facing = atoms[5];
            data.Status = atoms[6];
            data.Tag = atoms[7];
            data.VeterancyPercentage = atoms[8];
            data.Group = atoms[9];
            data.AutoNORecruitType = atoms[10];
            data.AutoYESRecruitType = atoms[11];
            CMapData::Instance->SetAircraftData(&data, NULL, NULL, 0, "");
        }

        for (int i = 0; i < 3; ++i)
        {
            const char* inf = (i == 0) ? GetString(cell, cell.InfantryData_1, data) :
                (i == 1) ? GetString(cell, cell.InfantryData_2, data) :
                GetString(cell, cell.InfantryData_3, data);
            if (OnLButtonDownPasted && CIsoViewExt::PasteInfantries && *inf)
            {
                FString value(inf);
                auto atoms = FString::SplitString(value, 13);
                CInfantryData data;
                data.House = atoms[0];
                data.TypeID = atoms[1];
                data.Health = atoms[2];
                data.Y.Format("%d", Y + offset_y);
                data.X.Format("%d", X + offset_x);
                data.SubCell = atoms[5];
                data.Status = atoms[6];
                data.Facing = atoms[7];
                data.Tag = atoms[8];
                data.VeterancyPercentage = atoms[9];
                data.Group = atoms[10];
                data.IsAboveGround = atoms[11];
                data.AutoNORecruitType = atoms[12];
                data.AutoYESRecruitType = atoms[13];
                CMapData::Instance->SetInfantryData(&data, NULL, NULL, 0, -1);
            }
        }

        CMapData::Instance->UpdateMapPreviewAt(X + offset_x, Y + offset_y);
        CopyPaste::PastedCoords.insert(MapCoord{ X + offset_x, Y + offset_y });
    }
}

void CopyPaste::LoadTileConvertRule(char sourceTheater)
{
    TileConvertRules.clear();
    FString iniSection;
    iniSection.Format("%s2%sTileRules", 
        TheaterHelpers::GetSuffix(sourceTheater), 
        TheaterHelpers::GetSuffix(CLoading::Instance->TheaterIdentifier));

    std::string path = CFinalSunApp::Instance->ExePath();
    path += "\\TileConvertRules.ini";

    CINI ini;
    ini.ClearAndLoad(path.c_str());

    if (auto pSection = ini.GetSection(iniSection))
    {
        for (const auto& [key, value] : pSection->GetEntities())
        {
            std::vector<FString> separates = FString::SplitString(value, "|");
            if (separates.size() < 2) continue;

            TileRule rule;

            std::vector<FString> srcParts = FString::SplitString(separates[0], "-");
            int srcStart = std::atoi(srcParts[0]);
            int srcEnd = (srcParts.size() > 1) ? std::atoi(srcParts[1]) : srcStart;
            for (int i = srcStart; i <= srcEnd; ++i)
                rule.sourceTiles.push_back(i);

            bool isRandom = separates[1].Find("~") != -1;
            rule.isRandom = isRandom;

            std::vector<FString> dstParts;
            if (isRandom)
                dstParts = FString::SplitString(separates[1], "~");
            else
                dstParts = FString::SplitString(separates[1], "-");

            int dstStart = std::atoi(dstParts[0]);
            int dstEnd = (dstParts.size() > 1) ? std::atoi(dstParts[1]) : dstStart;
            for (int i = dstStart; i <= dstEnd; ++i)
                rule.destinationTiles.push_back(i);

            if (separates.size() >= 3)
            {
                if (separates[2] != "*") {
                    rule.hasHeightOverride = true;
                    rule.heightOverride = std::clamp(std::atoi(separates[2]), 0, 14);
                }
            }

            if (separates.size() >= 4)
            {
                rule.hasSubIndexOverride = true;
                rule.subIndexOverride = std::atoi(separates[3]);
            }
            TileConvertRules.push_back(rule);
        }
    }
}

void CopyPaste::ConvertTile(CellData& cell)
{
    for (const auto& rule : TileConvertRules)
    {
        if (std::find(rule.sourceTiles.begin(), rule.sourceTiles.end(), cell.TileIndex) != rule.sourceTiles.end())
        {
            if (rule.isRandom)
            {
                int rangeSize = rule.destinationTiles.size();
                if (rangeSize > 0) {
                    int randIndex = rand() % rangeSize;
                    cell.TileIndex = rule.destinationTiles[randIndex];
                }
            }
            else if (rule.destinationTiles.size() == 1)
            {
                cell.TileIndex = rule.destinationTiles[0];
            }
            else if (rule.destinationTiles.size() == rule.sourceTiles.size())
            {
                auto it = std::find(rule.sourceTiles.begin(), rule.sourceTiles.end(), cell.TileIndex);
                int idx = std::distance(rule.sourceTiles.begin(), it);
                cell.TileIndex = rule.destinationTiles[idx];
            }
            else
            {
                cell.TileIndex = rule.destinationTiles[0];
            }

            if (rule.hasHeightOverride)
                cell.Height = rule.heightOverride;

            if (rule.hasSubIndexOverride)
                cell.TileSubIndex = rule.subIndexOverride;

            return;
        }
    }
}

DEFINE_HOOK(435F10, CFinalSunDlg_Tools_Copy, 7)
{
    GET(CFinalSunDlg*, pThis, ECX);

    pThis->PlaySound(CFinalSunDlg::FASoundType::Normal);

    if (ExtConfigs::EnableMultiSelection && MultiSelection::GetCount())
    {
        CopyPaste::CopyWholeMap = false;
        CopyPaste::Copy(MultiSelection::SelectedCoords);
    }
    else
        CIsoView::CurrentCommand->Command = FACurrentCommand::TileCopy;

    return 0x435F24;
}

DEFINE_HOOK(4C3460, CMapData_Copy, 5)
{
    GET(CMapDataExt*, pThis, ECX);
    GET_STACK(int, left, 0x4);
    GET_STACK(int, top, 0x8);
    GET_STACK(int, right, 0xC);
    GET_STACK(int, bottom, 0x10);
    
    if (left == 0 && top == 0 && right == 0 && bottom == 0)
        CopyPaste::CopyWholeMap = true;
    else
        CopyPaste::CopyWholeMap = false;

    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > pThis->MapWidthPlusHeight) right = pThis->MapWidthPlusHeight;
    if (bottom > pThis->MapWidthPlusHeight) bottom = pThis->MapWidthPlusHeight;

    if (right == 0) right = pThis->MapWidthPlusHeight;
    if (bottom == 0) bottom = pThis->MapWidthPlusHeight;

    std::set<MapCoord> coords;
    for (int x = left; x < right; ++x)
    {
        for (int y = top; y < bottom; ++y)
        {
            coords.insert({ x,y });
        }
    }
    CopyPaste::Copy(coords);

    return 0x4C384B;
}

DEFINE_HOOK(459FFB, CIsoView_OnMouseMove_Paste_Snapshot_SkipRedo, 6)
{
    return 0x45A00F;
}

DEFINE_HOOK(46168E, CIsoView_OnLButtonDown_Paste_Snapshot, 6)
{
    CopyPaste::OnLButtonDownPasted = true;
    return 0x4616A2;
}

DEFINE_HOOK(435F5A, CFinalSunDlg_Tools_PasteCenter, 5)
{
    CopyPaste::OnLButtonDownPasted = true;
    return 0;
}

DEFINE_HOOK(4C3850, CMapData_PasteAt, 8)
{
    if (!ExtConfigs::EnableMultiSelection)
        return 0;

    GET_STACK(const int, X, 0x4);
    GET_STACK(const int, Y, 0x8);
    GET_STACK(const char, nBaseHeight, 0xC);

    OpenClipboard(CFinalSunApp::Instance->m_pMainWnd->m_hWnd);
    HANDLE hData = GetClipboardData(CFinalSunApp::Instance->ClipboardFormat);
    auto ptr = GlobalLock(hData);

    if (ptr)
    {
        if (reinterpret_cast<int*>(ptr)[0] == 0) // New paste
        {
            const auto length = reinterpret_cast<size_t*>(ptr)[1];
            const int identifier = reinterpret_cast<int*>(ptr)[2];
            const int recordType = reinterpret_cast<int*>(ptr)[3];
            CopyPaste::TileConvertRules.clear();
            if (identifier != CLoading::Instance->TheaterIdentifier)
            {
                CopyPaste::LoadTileConvertRule(identifier);
            }
            const auto p = reinterpret_cast<MyClipboardData*>(reinterpret_cast<char*>(ptr) + 16);
            CopyPaste::Paste(X, Y, nBaseHeight, p, length, recordType);
            CopyPaste::OnLButtonDownPasted = false;
            GlobalUnlock(hData);
            CloseClipboard();
            return 0x4C388B;
        }
        else // Vanilla paste method
        {
            GlobalUnlock(hData);
            CloseClipboard();

            CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);
            CopyPaste::OnLButtonDownPasted = false;
            return 0;
        }
    }

    CloseClipboard();
    return 0x4C388B;
}
