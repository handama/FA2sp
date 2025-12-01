#include "../Helpers/STDHelpers.h"
#include "../ExtraWindow/Common.h"
#include "../FA2sp.h"
#include "../ExtraWindow/CNewComboUInputDlg/CNewComboUInputDlg.h"
#include "../Helpers/Translations.h"
#include "../Ext/CMapData/Body.h"
#include "../Ext/CIsoView/Body.h"
#include "../Source/CIsoView.h"
#include "MultiSelection.h"
#include "UserScripts.h"

enum UserScriptType {
    ID_REQUIRE_FA2SPHE = 71,
    ID_GET_AVAILABLE_INDEX,
    ID_KEY_EXISTS,
    ID_UINPUT_GET_MANUAL,
    ID_SECTION_EXISTS,
    ID_RANDOM_BETWEEN,
    ID_GET_AVAILABLE_KEY,
    ID_GET_THEATER,
    ID_PLACE_TILE_AT,
    ID_IS_COORD_IN_MAP,
    ID_GET_CELL_INFO,
    ID_GET_KEY_AT,
    ID_GET_VALUE_AT,
    ID_SET_HEIGHT_AT,
    ID_STRING_EQUALS,
    ID_DELETE_SMUDGE,
    ID_SET_OVERLAY,
    ID_SET_OVERLAY_DATA,
    ID_GET_VARIABLE_IN_POOL,
    ID_SET_VARIABLE_IN_POOL,
    ID_WAYPOINT_TO_STRING,
    ID_STRING_TO_WAYPOINT,
    ID_HIDE_TILE_AT,
    ID_HIDE_TILE_SET,
    ID_HIDE_TILE_INDEX,
    ID_MULTI_SELECT_TILE_AT,
    ID_MULTI_SELECT_TILE_SET,
    ID_MULTI_SELECT_TILE_INDEX,

    ID_NEW_COUNT,

};

int UserScriptExt::ParamCount = -1;
bool UserScriptExt::EditVaribale = false;
bool UserScriptExt::EditParams = false;
bool UserScriptExt::NeedRedraw = false;
std::unordered_map<ppmfc::CString, ppmfc::CString> UserScriptExt::VariablePool;
std::vector<ppmfc::CString> UserScriptExt::Temps;
ppmfc::CString UserScriptExt::ParamsTemp[10];

DEFINE_HOOK(511713, CUserScripts_GetParamCount, 7)
{
    GET(int, paramCount, EBP);
    UserScriptExt::ParamCount = paramCount;
    return 0;
}

DEFINE_HOOK(510ED9, CUserScripts_NewFunction_SetIDNum, 8)
{
    GET_STACK(ppmfc::CString, name, STACK_OFFS(0xA20, 0x9FC));

    int IDNum = -1;
    if (name == "RequireFA2SPHE")
        IDNum = ID_REQUIRE_FA2SPHE;
    else if (name == "GetAvailableIndex")
        IDNum = ID_GET_AVAILABLE_INDEX;
    else if (name == "KeyExists")
        IDNum = ID_KEY_EXISTS;
    else if (name == "UInputSelect")
        IDNum = ID_UINPUT_GET_MANUAL;
    else if (name == "SectionExists")
        IDNum = ID_SECTION_EXISTS;
    else if (name == "RandomBetween")
        IDNum = ID_RANDOM_BETWEEN;
    else if (name == "GetAvailableKey")
        IDNum = ID_GET_AVAILABLE_KEY;
    else if (name == "GetTheater")
        IDNum = ID_GET_THEATER;
    else if (name == "PlaceTileAt")
        IDNum = ID_PLACE_TILE_AT;
    else if (name == "IsCoordInMap")
        IDNum = ID_IS_COORD_IN_MAP;
    else if (name == "GetCellInfo")
        IDNum = ID_GET_CELL_INFO;
    else if (name == "GetKeyAt")
        IDNum = ID_GET_KEY_AT;
    else if (name == "GetValueAt")
        IDNum = ID_GET_VALUE_AT;
    else if (name == "SetHeightAt")
        IDNum = ID_SET_HEIGHT_AT;
    else if (name == "StringEquals")
        IDNum = ID_STRING_EQUALS;
    else if (name == "DeleteSmudge")
        IDNum = ID_DELETE_SMUDGE;
    else if (name == "SetOverlay")
        IDNum = ID_SET_OVERLAY;
    else if (name == "SetOverlayData")
        IDNum = ID_SET_OVERLAY_DATA;
    else if (name == "GetVariableInPool")
        IDNum = ID_GET_VARIABLE_IN_POOL;
    else if (name == "SetVariableInPool")
        IDNum = ID_SET_VARIABLE_IN_POOL;
    else if (name == "WaypointToString")
        IDNum = ID_WAYPOINT_TO_STRING;
    else if (name == "StringToWaypoint")
        IDNum = ID_STRING_TO_WAYPOINT;
    else if (name == "HideTileAt")
        IDNum = ID_HIDE_TILE_AT;
    else if (name == "HideTileSet")
        IDNum = ID_HIDE_TILE_SET;
    else if (name == "HideTileIndex")
        IDNum = ID_HIDE_TILE_INDEX;
    else if (name == "MultiSelectTileAt")
        IDNum = ID_MULTI_SELECT_TILE_AT;
    else if (name == "MultiSelectTileSet")
        IDNum = ID_MULTI_SELECT_TILE_SET;
    else if (name == "MultiSelectTileIndex")
        IDNum = ID_MULTI_SELECT_TILE_INDEX;

    if (IDNum > -1)
        R->Stack(STACK_OFFS(0xA20, 0xA00), IDNum);

    return 0;
}

DEFINE_HOOK(511832, CUserScripts_NewFunction_DisableReplaceVariable0, 5)
{
    GET(int, IDNum, EBX);

    switch (IDNum)
    {
    case 69: // 511832
    case ID_GET_AVAILABLE_INDEX:
    case ID_KEY_EXISTS:
    case ID_UINPUT_GET_MANUAL:
    case ID_SECTION_EXISTS:
    case ID_RANDOM_BETWEEN:
    case ID_GET_AVAILABLE_KEY:
    case ID_GET_THEATER:
    case ID_IS_COORD_IN_MAP:
    case ID_GET_CELL_INFO:
    case ID_GET_KEY_AT:
    case ID_GET_VALUE_AT:
    case ID_STRING_EQUALS:
    case ID_GET_VARIABLE_IN_POOL:
    case ID_WAYPOINT_TO_STRING:
    case ID_STRING_TO_WAYPOINT:
        return 0x51183C;
        break;
    default:
        break;
    }

    return 0x511837;
}

DEFINE_HOOK(5134D8, CUserScripts_EditParams, 5)
{
    if (UserScriptExt::EditParams) {
        UserScriptExt::EditParams = false;
        R->ESI(UserScriptExt::ParamsTemp);
    }
    return 0;
}

DEFINE_HOOK(516974, CUserScripts_NewFunction_SwitchID, 8)
{
    GET(int, IDNum, EBX);
    GET(ppmfc::CString*, Params, ESI);

    if (IDNum == -1)
        return 0;
    
    UserScriptExt::EditVaribale = false;

    switch (IDNum)
    {
    case ID_REQUIRE_FA2SPHE:
    {
        UserScriptExt::EditParams = true;
        UserScriptExt::ParamsTemp[0] = Translations::TranslateOrDefault("UserScriptRequireFA2SPHE", "This script requires FA2SP HDM Edition to run, do not copy it to other versions.");
        break;
    }
    // 0: variable, 1: execute
    case ID_GET_AVAILABLE_INDEX:
    {
        if (UserScriptExt::ParamCount < 1) break;
        if (UserScriptExt::ParamCount > 1) {
            auto execute = UserScriptExt::GetParam(Params, 1);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        // this is how we change param 0
        UserScriptExt::Temps.push_back(CMapDataExt::GetAvailableIndex());
        UserScriptExt::EditVaribale = true;
        break;
    }
    // 0: variable, 1: section, 2: key, 3: execute, 4: loadfrom = map
    case ID_KEY_EXISTS:
    {
        if (UserScriptExt::ParamCount < 3) break;
        if (UserScriptExt::ParamCount > 3) {
            auto execute = UserScriptExt::GetParam(Params, 3);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        MultimapHelper mmh;
        mmh.AddINI(&CINI::CurrentDocument);
        if (UserScriptExt::ParamCount > 4) {
            mmh.Clear();
            auto loadfrom = UserScriptExt::GetParam(Params, 4);
            ExtraWindow::LoadFrom(mmh, loadfrom);
        }
        auto section = UserScriptExt::GetParam(Params, 1);
        auto key = UserScriptExt::GetParam(Params, 2);

        if (mmh.TryGetString(section, key) != nullptr) {
            UserScriptExt::Temps.push_back("1");
            UserScriptExt::EditVaribale = true;
        }
        else {
            UserScriptExt::Temps.push_back("0");
            UserScriptExt::EditVaribale = true;
        }
        break;
    }
    // 0: variable, 1: caption, 2: section, 3: execute, 4: loadfrom = map, 5: readkey = false, 6 = load value as name = false, 7 = use strict order = false
    case ID_UINPUT_GET_MANUAL:
    {
        if (UserScriptExt::ParamCount < 3) break;
        if (UserScriptExt::ParamCount > 3) {
            auto execute = UserScriptExt::GetParam(Params, 3);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        CNewComboUInputDlg dlg;
        dlg.mmh.AddINI(&CINI::CurrentDocument);
        if (UserScriptExt::ParamCount > 4) {
            dlg.mmh.Clear();
            auto loadfrom = UserScriptExt::GetParam(Params, 4);
            dlg.LoadFrom = atoi(loadfrom);
            ExtraWindow::LoadFrom(dlg.mmh, loadfrom);
        }
        if (UserScriptExt::ParamCount > 5) {
            auto readkey = UserScriptExt::GetParam(Params, 5);
            if (UserScriptExt::IsValSet(readkey)) {
                dlg.ReadValue = false;
            }
        }
        if (UserScriptExt::ParamCount > 6) {
            auto loadValue = UserScriptExt::GetParam(Params, 6);
            if (UserScriptExt::IsValSet(loadValue)) {
                dlg.LoadValueAsName = true;
            }
        }
        if (UserScriptExt::ParamCount > 7) {
            auto order = UserScriptExt::GetParam(Params, 7);
            if (UserScriptExt::IsValSet(order)) {
                dlg.UseStrictOrder = true;
            }
        }
        dlg.m_type = COMBOUINPUT_MANUAL;
        dlg.m_Caption = UserScriptExt::GetParam(Params, 1);
        dlg.m_Section = UserScriptExt::GetParam(Params, 2);

        dlg.DoModal();
        
        UserScriptExt::Temps.push_back(dlg.m_Combo);
        UserScriptExt::EditVaribale = true;
        break;
    }
    // 0: variable, 1: section, 2: execute, 3: loadfrom
    case ID_SECTION_EXISTS:
    {
        if (UserScriptExt::ParamCount < 2) break;
        if (UserScriptExt::ParamCount > 2) {
            auto execute = UserScriptExt::GetParam(Params, 2);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        MultimapHelper mmh;
        mmh.AddINI(&CINI::CurrentDocument);
        if (UserScriptExt::ParamCount > 3) {
            mmh.Clear();
            auto loadfrom = UserScriptExt::GetParam(Params, 3);
            ExtraWindow::LoadFrom(mmh, loadfrom);
        }
        auto section = UserScriptExt::GetParam(Params, 1);

        if (!mmh.GetSection(section).empty()) {
            UserScriptExt::Temps.push_back("1");
            UserScriptExt::EditVaribale = true;
        }
        else {
            UserScriptExt::Temps.push_back("0");
            UserScriptExt::EditVaribale = true;
        }
        break;
    }
    // 0: variable, 1: [Start, 2: End), 3: execute
    case ID_RANDOM_BETWEEN:
    {
        if (UserScriptExt::ParamCount < 3) break;
        if (UserScriptExt::ParamCount > 3) {
            auto execute = UserScriptExt::GetParam(Params, 3);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        int start = atoi(UserScriptExt::GetParam(Params, 1));
        int end = atoi(UserScriptExt::GetParam(Params, 2));
        if (end <= start) break;
        int result = STDHelpers::RandomSelectInt(start, end);
        UserScriptExt::Temps.push_back(STDHelpers::IntToString(result));
        UserScriptExt::EditVaribale = true;

        break;
    }
    // 0: variable, 1: section, 2: execute
    case ID_GET_AVAILABLE_KEY:
    {
        if (UserScriptExt::ParamCount < 2) break;
        if (UserScriptExt::ParamCount > 2) {
            auto execute = UserScriptExt::GetParam(Params, 2);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        UserScriptExt::Temps.push_back(CINI::GetAvailableKey(UserScriptExt::GetParam(Params, 1)));
        UserScriptExt::EditVaribale = true;
        break;
    }
    // 0: variable, 1: execute
    case ID_GET_THEATER:
    {
        if (UserScriptExt::ParamCount < 1) break;
        if (UserScriptExt::ParamCount > 1) {
            auto execute = UserScriptExt::GetParam(Params, 1);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        UserScriptExt::Temps.push_back(STDHelpers::IntToString(CMapDataExt::CurrentTheaterIndex));
        UserScriptExt::EditVaribale = true;
        break;
    }
    // 0: X, 1: Y , 2: tile index, 3: subtile index = 0, 4: alt image = -1 (random), 5: execute
    case ID_PLACE_TILE_AT:
    {
        if (UserScriptExt::ParamCount < 3) break;
        if (UserScriptExt::ParamCount > 5) {
            auto execute = UserScriptExt::GetParam(Params, 5);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        int x = atoi(UserScriptExt::GetParam(Params, 0));
        int y = atoi(UserScriptExt::GetParam(Params, 1));
        int index = atoi(UserScriptExt::GetParam(Params, 2));
        int subIdx = UserScriptExt::ParamCount > 3 ? atoi(UserScriptExt::GetParam(Params, 3)) : 0;
        int altIdx = UserScriptExt::ParamCount > 4 ? atoi(UserScriptExt::GetParam(Params, 4)) : -1;

        if (CMapData::Instance->IsCoordInMap(x, y) && 0 <= index && index < CMapDataExt::TileDataCount) {
            auto cell = CMapData::Instance->GetCellAt(x, y);
            cell->TileIndex = index;
            if (subIdx >= CMapDataExt::TileData[index].TileBlockCount) {
                subIdx = CMapDataExt::TileData[index].TileBlockCount - 1;
            }
            if (altIdx >= CMapDataExt::TileData[index].AltTypeCount + 1) {
                altIdx = CMapDataExt::TileData[index].AltTypeCount;
            }
            cell->TileSubIndex = subIdx;
            cell->Flag.AltIndex = altIdx == -1 ? STDHelpers::RandomSelectInt(0, CMapDataExt::TileData[index].AltTypeCount + 1) : altIdx;
            CMapData::Instance->UpdateMapPreviewAt(x, y);
            UserScriptExt::NeedRedraw = true;
        }
        //too much display cause internal error
        //else
        //{
        //    UserScriptExt::EditParams = true;
        //    UserScriptExt::ParamsTemp[0] = Translations::TranslateOrDefault("UserScriptPlaceTileAtInvalidTileOrCoord", "Failed to place tile, coord not in map or wrong tile index.\n\r");
        //}
        break;
    }
    // 0: variable, 1: X, 2: Y, 3: execute
    case ID_IS_COORD_IN_MAP:
    {
        if (UserScriptExt::ParamCount < 3) break;
        if (UserScriptExt::ParamCount > 3) {
            auto execute = UserScriptExt::GetParam(Params, 3);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        int x = atoi(UserScriptExt::GetParam(Params, 1));
        int y = atoi(UserScriptExt::GetParam(Params, 2));
        if (CMapData::Instance->IsCoordInMap(x, y)) {
            UserScriptExt::Temps.push_back("1");
            UserScriptExt::EditVaribale = true;
        }
        else {
            UserScriptExt::Temps.push_back("0");
            UserScriptExt::EditVaribale = true;
        }
        break;
    }
    // 0: variable, 1: X, 2: Y, 3: info type, 4: execute
    case ID_GET_CELL_INFO:
    {
        if (UserScriptExt::ParamCount < 4) break;
        if (UserScriptExt::ParamCount > 4) {
            auto execute = UserScriptExt::GetParam(Params, 4);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        int x = atoi(UserScriptExt::GetParam(Params, 1));
        int y = atoi(UserScriptExt::GetParam(Params, 2));
        enum infoType{
            tileIndex = 0,
            subtileIndex,
            altImage,
            height,
            landType,
            rampType,
            allowToPlace,
            tileSet,
            infantry0Index, //(none = -1)
            infantry1Index,
            infantry2Index,
            aircraftIndex,
            structureIndex,
            unitIndex,
            terrainIndex,
            smudgeIndex,
            waypointIndex,
            overlay,
            overlayData,
            isCellMultiSelected,
            isCellHidden,
            Count
        };
        int type = atoi(UserScriptExt::GetParam(Params, 3));
        if (CMapData::Instance->IsCoordInMap(x, y) && 0 < type && type < Count) {
            auto cell = CMapData::Instance->GetCellAt(x, y);
            switch (type)
            {
            case tileIndex:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(CMapDataExt::GetSafeTileIndex(cell->TileIndex)));
                break;
            case subtileIndex:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(cell->TileSubIndex));
                break;
            case altImage:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(cell->Flag.AltIndex));
                break;
            case height:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(cell->Height));
                break;
            case landType:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(int(CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileBlockDatas[cell->TileSubIndex].TerrainType)));
                break;
            case rampType:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileBlockDatas[cell->TileSubIndex].RampType));
                break;
            case allowToPlace:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].AllowToPlace));
                break;
            case tileSet:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileSet));
                break;
            case infantry0Index:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(cell->Infantry[0]));
                break;
            case infantry1Index:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(cell->Infantry[1]));
                break;
            case infantry2Index:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(cell->Infantry[2]));
                break;
            case aircraftIndex:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(cell->Aircraft));
                break;
            case structureIndex:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(cell->Structure));
                break;
            case unitIndex:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(cell->Unit));
                break;
            case terrainIndex:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(cell->Terrain));
                break;
            case smudgeIndex:
            {
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
                            if (thisY + i == y && thisX + j == x)
                            {
                                target = thisSmudge;
                                found = true;
                            }
                    if (found)
                        break;
                    id++;
                }
                if (!found) {
                    id = -1;
                }
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(id));
                break;
            }
            case waypointIndex:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(cell->Waypoint));
                break;
            case overlay:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(cell->Overlay));
                break;
            case overlayData:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(cell->OverlayData));
                break;
            case isCellMultiSelected:
            {
                bool selected = false;
                for (const auto& coord : MultiSelection::SelectedCoords) {
                    if (coord.X == x && coord.Y == y) {
                        UserScriptExt::Temps.push_back("1");
                        selected = true;
                        break;
                    }
                }
                if (!selected) {
                    UserScriptExt::Temps.push_back("0");
                }
            }
            break;
            case isCellHidden:
                UserScriptExt::Temps.push_back(STDHelpers::IntToString(cell->IsHidden()));
                break;
            default:
                break;
            }

            UserScriptExt::EditVaribale = true;
        }
        //else
        //{
        //    UserScriptExt::EditParams = true;
        //    UserScriptExt::ParamsTemp[0] = Translations::TranslateOrDefault("UserScriptGetCellInfoInvalidTypeOrCoord", "Failed to get cell info, coord not in map or wrong type.\n\r");
        //}
        break;
    }
    // 0: variable, 1: section, 2: index, 3: execute, 4 load from = 0 (0 = map, 1 = rules, 2 = rules + map)
    case ID_GET_KEY_AT:
    {
        if (UserScriptExt::ParamCount < 3) break;
        if (UserScriptExt::ParamCount > 3) {
            auto execute = UserScriptExt::GetParam(Params, 3);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        int load = 0;
        auto section = UserScriptExt::GetParam(Params, 1);
        auto index = atoi(UserScriptExt::GetParam(Params, 2));
        if (UserScriptExt::ParamCount > 4) {
            load = atoi(UserScriptExt::GetParam(Params, 4));
        }
        if (load == 1) {
            auto key = Variables::Rules.GetKeyAt(section, index);
            if (key != "") {
                UserScriptExt::Temps.push_back(key);
                UserScriptExt::EditVaribale = true;
            }
        }
        else if (load == 2) {
            auto key = Variables::RulesMap.GetKeyAt(section, index);
            if (key != "") {
                UserScriptExt::Temps.push_back(key);
                UserScriptExt::EditVaribale = true;
            }
        }
        else {
            auto pSection = CINI::CurrentDocument->GetSection(section);
            if (auto pStr = pSection->GetKeyAt(index)) {
                UserScriptExt::Temps.push_back(*pStr);
                UserScriptExt::EditVaribale = true;
            }
        }
        break;
    }
    // 0: variable, 1: section, 2: index, 3: execute, 4 load from = 0 (0 = map, 1 = rules, 2 = rules + map)
    case ID_GET_VALUE_AT:
    {
        if (UserScriptExt::ParamCount < 3) break;
        if (UserScriptExt::ParamCount > 3) {
            auto execute = UserScriptExt::GetParam(Params, 3);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        int load = 0;
        auto section = UserScriptExt::GetParam(Params, 1);
        auto index = atoi(UserScriptExt::GetParam(Params, 2));
        if (UserScriptExt::ParamCount > 4) {
            load = atoi(UserScriptExt::GetParam(Params, 4));
        }
        if (load == 1) {
            auto val = Variables::Rules.GetValueAt(section, index);
            if (val != "") {
                UserScriptExt::Temps.push_back(val);
                UserScriptExt::EditVaribale = true;
            }
        }
        else if (load == 2) {
            auto val = Variables::RulesMap.GetValueAt(section, index);
            if (val != "") {
                UserScriptExt::Temps.push_back(val);
                UserScriptExt::EditVaribale = true;
            }
        }
        else {
            auto pSection = CINI::CurrentDocument->GetSection(section);
            if (auto pStr = pSection->GetValueAt(index)) {
                UserScriptExt::Temps.push_back(*pStr);
                UserScriptExt::EditVaribale = true;
            }
        }
        break;
    }
    // 0: X, 1: Y, 2: height, 3: absolute = true, 4: execute
    case ID_SET_HEIGHT_AT:
    {
        if (UserScriptExt::ParamCount < 3) break;
        if (UserScriptExt::ParamCount > 4) {
            auto execute = UserScriptExt::GetParam(Params, 4);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        bool absolute = true;
        if (UserScriptExt::ParamCount > 3) {
            auto abs = UserScriptExt::GetParam(Params, 3);
            absolute = UserScriptExt::IsValSet(abs);
        }
        int height = atoi(UserScriptExt::GetParam(Params, 2));
        int x = atoi(UserScriptExt::GetParam(Params, 0));
        int y = atoi(UserScriptExt::GetParam(Params, 1));

        if (CMapData::Instance->IsCoordInMap(x, y)) {
            auto cell = CMapData::Instance->GetCellAt(x, y);
            if (!absolute)
                height += cell->Height;
            if (height < 0) height = 0;
            if (height > 14) height = 14;
            cell->Height = height;
            UserScriptExt::NeedRedraw = true;
        }
        //else
        //{
        //    UserScriptExt::EditParams = true;
        //    UserScriptExt::ParamsTemp[0] = Translations::TranslateOrDefault("UserScriptSetHeightAtInvalidCoord", "Failed to set height, coord not in map.\n\r");
        //}

        break;
    }
    // 0: variable, 1: string A, 2: string B, 3: execute
    case ID_STRING_EQUALS:
    {
        if (UserScriptExt::ParamCount < 3) break;
        if (UserScriptExt::ParamCount > 3) {
            auto execute = UserScriptExt::GetParam(Params, 3);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        if (UserScriptExt::GetParam(Params, 1) == UserScriptExt::GetParam(Params, 2)){
            UserScriptExt::Temps.push_back("1");
            UserScriptExt::EditVaribale = true;
        }
        else {
            UserScriptExt::Temps.push_back("0");
            UserScriptExt::EditVaribale = true;
        }

        break;
    }
    // 0: pos, 1: execute
    case ID_DELETE_SMUDGE:
    {
        if (UserScriptExt::ParamCount < 1) break;
        if (UserScriptExt::ParamCount > 1) {
            auto execute = UserScriptExt::GetParam(Params, 1);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        int index = atoi(UserScriptExt::GetParam(Params, 0));
        if (index < 0 || index >= CMapData::Instance->SmudgeDatas.size()) {
            UserScriptExt::EditParams = true;
            UserScriptExt::ParamsTemp[0] = Translations::TranslateOrDefault("UserScriptDeleteSmudgeInvalid", "Smudge deletion failed, invalid index");
        }
        else {
            CMapData::Instance->DeleteSmudgeData(index);
            UserScriptExt::NeedRedraw = true;
        }
        break;
    }
    // 0: X, 1: Y, 2: overlay, 3: execute
    case ID_SET_OVERLAY:
    {
        if (UserScriptExt::ParamCount < 3) break;
        if (UserScriptExt::ParamCount > 3) {
            auto execute = UserScriptExt::GetParam(Params, 3);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        WORD overlay = atoi(UserScriptExt::GetParam(Params, 2));
        int x = atoi(UserScriptExt::GetParam(Params, 0));
        int y = atoi(UserScriptExt::GetParam(Params, 1));

        if (CMapData::Instance->IsCoordInMap(x, y)) {
            int pos = CMapData::Instance->GetCoordIndex(x, y);
            CMapDataExt::GetExtension()->SetNewOverlayAt(pos, overlay);
            UserScriptExt::NeedRedraw = true;
        }
        break;
    }
    // 0: X, 1: Y, 2: overlay data, 3: execute
    case ID_SET_OVERLAY_DATA:
    {
        if (UserScriptExt::ParamCount < 3) break;
        if (UserScriptExt::ParamCount > 3) {
            auto execute = UserScriptExt::GetParam(Params, 3);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        unsigned char overlaydata = atoi(UserScriptExt::GetParam(Params, 2));
        int x = atoi(UserScriptExt::GetParam(Params, 0));
        int y = atoi(UserScriptExt::GetParam(Params, 1));

        if (CMapData::Instance->IsCoordInMap(x, y)) {
            int pos = CMapData::Instance->GetCoordIndex(x, y);
            CMapData::Instance->SetOverlayDataAt(pos, overlaydata);
            UserScriptExt::NeedRedraw = true;
        }
        break;
    }
    // 0: variable, 1: key, 2: execute
    case ID_GET_VARIABLE_IN_POOL:
    {
        if (UserScriptExt::ParamCount < 2) break;
        if (UserScriptExt::ParamCount > 2) {
            auto execute = UserScriptExt::GetParam(Params, 2);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        auto key = UserScriptExt::GetParam(Params, 1);
        auto it = UserScriptExt::VariablePool.find(key);
        if (it != UserScriptExt::VariablePool.end()) {
            UserScriptExt::Temps.push_back(it->second);
            UserScriptExt::EditVaribale = true;
        }
        else {
            UserScriptExt::Temps.push_back("ERR_NOT_FOUND");
            UserScriptExt::EditVaribale = true;
        }
        break;
    }
    // 0: key, 1: value, 2: execute
    case ID_SET_VARIABLE_IN_POOL:
    {
        if (UserScriptExt::ParamCount < 2) break;
        if (UserScriptExt::ParamCount > 2) {
            auto execute = UserScriptExt::GetParam(Params, 2);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        UserScriptExt::VariablePool[UserScriptExt::GetParam(Params, 0)] = UserScriptExt::GetParam(Params, 1);
        break;
    }
    // 0: variable, 1: waypoint(int), 2: execute
    case ID_WAYPOINT_TO_STRING:
    {
        if (UserScriptExt::ParamCount < 2) break;
        if (UserScriptExt::ParamCount > 2) {
            auto execute = UserScriptExt::GetParam(Params, 2);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        auto wp = atoi(UserScriptExt::GetParam(Params, 1));
        UserScriptExt::Temps.push_back(STDHelpers::WaypointToString(wp));
        UserScriptExt::EditVaribale = true;
        break;
    }
    // 0: variable, 1: waypoint(string), 2: execute
    case ID_STRING_TO_WAYPOINT:
    {
        if (UserScriptExt::ParamCount < 2) break;
        if (UserScriptExt::ParamCount > 2) {
            auto execute = UserScriptExt::GetParam(Params, 2);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        auto wp = UserScriptExt::GetParam(Params, 1);
        UserScriptExt::Temps.push_back(STDHelpers::StringToWaypointStr(wp));
        UserScriptExt::EditVaribale = true;
        break;
    }
    // 0: X, 1: Y, 2: mode (0 = hide, 1 = show, 2 = reverse), 3: execute
    case ID_HIDE_TILE_AT:
    {
        if (UserScriptExt::ParamCount < 3) break;
        if (UserScriptExt::ParamCount > 3) {
            auto execute = UserScriptExt::GetParam(Params, 3);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        int mode = atoi(UserScriptExt::GetParam(Params, 2));
        int x = atoi(UserScriptExt::GetParam(Params, 0));
        int y = atoi(UserScriptExt::GetParam(Params, 1));

        if (CMapData::Instance->IsCoordInMap(x, y)) {
            auto cell = CMapData::Instance->GetCellAt(x, y);
            if (mode == 0) {
                cell->Flag.IsHiddenCell = 1;
            }
            else if (mode == 1) {
                cell->Flag.IsHiddenCell = 0;
            }
            else if (mode == 2) {
                cell->Flag.IsHiddenCell ^= 1;
            }
            UserScriptExt::NeedRedraw = true;
        }
        break;
    }
    // 0: tile set, 1: mode (0 = hide, 1 = show, 2 = reverse), 2: execute
    case ID_HIDE_TILE_SET:
    {
        if (UserScriptExt::ParamCount < 2) break;
        if (UserScriptExt::ParamCount > 2) {
            auto execute = UserScriptExt::GetParam(Params, 2);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        int mode = atoi(UserScriptExt::GetParam(Params, 1));
        int index = atoi(UserScriptExt::GetParam(Params, 0));
        if (index >= 0 && index < CMapDataExt::TileSet_starts.size() - 1) {
            for (int i = CMapDataExt::TileSet_starts[index]; i < CMapDataExt::TileSet_starts[index + 1]; i++) {
                if (mode == 0) {
                    (*CTileTypeClass::Instance)[i].IsHidden = 1;
                }
                else if (mode == 1) {
                    (*CTileTypeClass::Instance)[i].IsHidden = 0;
                }
                else if (mode == 2) {
                    (*CTileTypeClass::Instance)[i].IsHidden ^= 1;
                }
            }
        }
        UserScriptExt::NeedRedraw = true;
        break;
    }
    // 0: tile index, 1: mode (0 = hide, 1 = show, 2 = reverse), 2: execute, 3: subtile = -1
    case ID_HIDE_TILE_INDEX:
    {
        if (UserScriptExt::ParamCount < 2) break;
        if (UserScriptExt::ParamCount > 2) {
            auto execute = UserScriptExt::GetParam(Params, 2);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        int mode = atoi(UserScriptExt::GetParam(Params, 1));
        int index = CMapDataExt::GetSafeTileIndex(atoi(UserScriptExt::GetParam(Params, 0)));
        int subtile = -1;
        if (UserScriptExt::ParamCount > 3) {
            subtile = atoi(UserScriptExt::GetParam(Params, 3));
        }
        if (index >= 0 && index < CMapDataExt::TileDataCount) {
            if (subtile < 0) {

                if (mode == 0) {
                    (*CTileTypeClass::Instance)[index].IsHidden = 1;
                }
                else if (mode == 1) {
                    (*CTileTypeClass::Instance)[index].IsHidden = 0;
                }
                else if (mode == 2) {
                    (*CTileTypeClass::Instance)[index].IsHidden ^= 1;
                }

            }
            else {
                for (int j = 0; j < CMapData::Instance->CellDataCount; j++) {
                    auto& cell = CMapData::Instance->CellDatas[j];
                    if (CMapDataExt::GetSafeTileIndex(cell.TileIndex) == index && cell.TileSubIndex == subtile) {
                        if (mode == 0) {
                            cell.Flag.IsHiddenCell = 1;
                        }
                        else if (mode == 1) {
                            cell.Flag.IsHiddenCell = 0;
                        }
                        else if (mode == 2) {
                            cell.Flag.IsHiddenCell ^= 1;
                        }
                    }
                }
            }
        }

        UserScriptExt::NeedRedraw = true;
        break;
    }
    // 0: X, 1: Y, 2: mode (0 = add, 1 = remove, 2 = reverse), 3: execute
    case ID_MULTI_SELECT_TILE_AT:
    {
        if (UserScriptExt::ParamCount < 3) break;
        if (UserScriptExt::ParamCount > 3) {
            auto execute = UserScriptExt::GetParam(Params, 3);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        int mode = atoi(UserScriptExt::GetParam(Params, 2));
        int x = atoi(UserScriptExt::GetParam(Params, 0));
        int y = atoi(UserScriptExt::GetParam(Params, 1));

        if (CMapData::Instance->IsCoordInMap(x, y)) {
            auto cell = CMapData::Instance->GetCellAt(x, y);
            if (mode == 0) {
                MultiSelection::AddCoord(x, y);
            }
            else if (mode == 1) {
                MultiSelection::RemoveCoord(x, y);
            }
            else if (mode == 2) {
                MultiSelection::ReverseStatus(x, y);
            }
            UserScriptExt::NeedRedraw = true;
        }
        break;
    }
    // 0: tile set, 1: mode (0 = add, 1 = remove, 2 = reverse), 2: execute
    case ID_MULTI_SELECT_TILE_SET:
    {
        if (UserScriptExt::ParamCount < 2) break;
        if (UserScriptExt::ParamCount > 2) {
            auto execute = UserScriptExt::GetParam(Params, 2);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        int mode = atoi(UserScriptExt::GetParam(Params, 1));
        int index = atoi(UserScriptExt::GetParam(Params, 0));
        if (index >= 0 && index < CMapDataExt::TileSet_starts.size() - 1) {
            for (int j = 0; j < CMapData::Instance->CellDataCount; j++) {
                auto& cell = CMapData::Instance->CellDatas[j];
                for (int i = CMapDataExt::TileSet_starts[index]; i < CMapDataExt::TileSet_starts[index + 1]; i++) {
                    if (CMapDataExt::GetSafeTileIndex(cell.TileIndex) == i) {
                        int x = CMapData::Instance->GetXFromCoordIndex(j);
                        int y = CMapData::Instance->GetYFromCoordIndex(j);
                        if (mode == 0) {
                            MultiSelection::AddCoord(x, y);
                        }
                        else if (mode == 1) {
                            MultiSelection::RemoveCoord(x, y);
                        }
                        else if (mode == 2) {
                            MultiSelection::ReverseStatus(x, y);
                        }
                        break;
                    }
                }
            }
        }
        UserScriptExt::NeedRedraw = true;
        break;
    }
    // 0: tile index, 1: mode (0 = add, 1 = remove, 2 = reverse), 2: execute, 3: subtile = -1
    case ID_MULTI_SELECT_TILE_INDEX:
    {
        if (UserScriptExt::ParamCount < 2) break;
        if (UserScriptExt::ParamCount > 2) {
            auto execute = UserScriptExt::GetParam(Params, 2);
            if (!UserScriptExt::IsValSet(execute))
                break;
        }
        int subtile = -1;
        if (UserScriptExt::ParamCount > 3) {
            subtile = atoi(UserScriptExt::GetParam(Params, 3));
        }
        int mode = atoi(UserScriptExt::GetParam(Params, 1));
        int index = CMapDataExt::GetSafeTileIndex(atoi(UserScriptExt::GetParam(Params, 0)));
        if (index >= 0 && index < CMapDataExt::TileDataCount) {
            for (int j = 0; j < CMapData::Instance->CellDataCount; j++) {
                auto& cell = CMapData::Instance->CellDatas[j];
                if (CMapDataExt::GetSafeTileIndex(cell.TileIndex) == index && (subtile < 0 ? true : (subtile == cell.TileSubIndex))) {
                    int x = CMapData::Instance->GetXFromCoordIndex(j);
                    int y = CMapData::Instance->GetYFromCoordIndex(j);
                    if (mode == 0) {
                        MultiSelection::AddCoord(x, y);
                    }
                    else if (mode == 1) {
                        MultiSelection::RemoveCoord(x, y);
                    }
                    else if (mode == 2) {
                        MultiSelection::ReverseStatus(x, y);
                    }
                }
            }
        }
        UserScriptExt::NeedRedraw = true;
        break;
    }
    default:
    {
        break;
    }
    }

    if (UserScriptExt::EditVaribale)
        return 0x51213B;
    if (UserScriptExt::EditParams)
        return 0x5134D8;

    return 0x516870;
}

DEFINE_HOOK(512152, CUserScripts_ReadINI_EmptyJump, 5)
{
    if (UserScriptExt::ParamCount > 4)
        return 0x51213B;
    else
        return 0;
}

//DEFINE_HOOK_AGAIN(512152, CUserScripts_ReadINI, 5)
DEFINE_HOOK(51213B, CUserScripts_ReadINI, 6)
{
    // this is how we change param 0
    if (UserScriptExt::EditVaribale) {
        UserScriptExt::EditVaribale = false;
        R->EAX(&UserScriptExt::Temps.back());
        return 0;
    }

    // 0: variable, 1: section, 2: key, 3: execute, 4: loadfrom
    if (UserScriptExt::ParamCount > 4) {
        GET(ppmfc::CString*, Params, ESI);
        auto section = UserScriptExt::GetParam(Params, 1);
        auto key = UserScriptExt::GetParam(Params, 2);
        auto loadFrom = UserScriptExt::GetParam(Params, 4);
        MultimapHelper mmh;
        
        ExtraWindow::LoadFrom(mmh, loadFrom);
        auto value = mmh.TryGetString(section, key);

        if (value) {
            ppmfc::CString str = *value;
            str.Trim();
            UserScriptExt::Temps.push_back(str);
            R->EAX(&UserScriptExt::Temps.back());
        }
        else {
            UserScriptExt::Temps.push_back("");
            R->EAX(&UserScriptExt::Temps.back());
        }
    }


    return 0;
}

DEFINE_HOOK(516ABD, CUserScripts_End_ClearData, 7)
{
    UserScriptExt::ParamCount = -1;
    UserScriptExt::EditVaribale = false;
    UserScriptExt::EditParams = false;
    UserScriptExt::VariablePool.clear();
    UserScriptExt::Temps.clear();
    if (UserScriptExt::NeedRedraw) {
        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
        UserScriptExt::NeedRedraw = false;
    }
    for (int i = 0; i < 10; i++)
        UserScriptExt::ParamsTemp[i] = "";
    return 0;
}

//0x51698F loop break