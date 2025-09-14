#include "CLuaConsole.h"
#include "LuaFunctions.cpp"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/MultimapHelper.h"
#include "../Common.h"
#include "../CNewAITrigger/CNewAITrigger.h"
#include "../CNewTeamTypes/CNewTeamTypes.h"
#include "../CNewScript/CNewScript.h"
#include "../CNewTaskforce/CNewTaskforce.h"
#include "../CObjectSearch/CObjectSearch.h"
#include <CLoading.h>
#include <CFinalSunDlg.h>
#include <CObjectDatas.h>
#include <CMapData.h>
#include <CIsoView.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CMapData/Body.h"
#include "../CObjectSearch/CObjectSearch.h"
#include <ctime>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "../../Miscs/MultiSelection.h"
#include <Miscs/Miscs.h>

namespace fs = std::filesystem;

HWND CLuaConsole::m_hwnd;
CFinalSunDlg* CLuaConsole::m_parent;
HWND CLuaConsole::hExecute;
HWND CLuaConsole::hRun;
HWND CLuaConsole::hOutputBox;
HWND CLuaConsole::hInputBox;
HWND CLuaConsole::hOutputText;
HWND CLuaConsole::hInputText;
HWND CLuaConsole::hScripts;
HWND CLuaConsole::hRunFile;
HWND CLuaConsole::hApply;
HWND CLuaConsole::hSearchText;
bool CLuaConsole::applyingScript = false;
bool CLuaConsole::applyingScriptFirst = true;
bool CLuaConsole::runFile = true;
//HWND CLuaConsole::hStop;
int CLuaConsole::origWndWidth;
int CLuaConsole::origWndHeight;
int CLuaConsole::minWndWidth;
int CLuaConsole::minWndHeight;
bool CLuaConsole::minSizeSet;
bool CLuaConsole::needRedraw = false;
bool CLuaConsole::recalculateOre = false;
bool CLuaConsole::updateBuilding = false;
bool CLuaConsole::updateUnit = false;
bool CLuaConsole::updateInfantry = false;
bool CLuaConsole::updateAircraft = false;
bool CLuaConsole::updateNode = false;
bool CLuaConsole::updateMinimap = false;
bool CLuaConsole::updateTrigger = false;
bool CLuaConsole::updateAITrigger = false;
bool CLuaConsole::updateScript = false;
bool CLuaConsole::updateTeam = false;
bool CLuaConsole::updateTaskforce = false;
bool CLuaConsole::updateCellTag = false;
bool CLuaConsole::skipBuildingUpdate = false;
char CLuaConsole::Buffer[BUFFER_SIZE]{ 0 };
sol::state CLuaConsole::Lua;
using namespace::LuaFunctions;

void CLuaConsole::Create(CFinalSunDlg* pWnd)
{
    HMODULE hModule = LoadLibrary(TEXT("Riched32.dll"));
    if (!hModule)
        MessageBox(NULL, Translations::TranslateOrDefault("FailedLoadRiched32DLL", "Could not Load Riched32.dll£¡"), Translations::TranslateOrDefault("Error", "Error"), MB_ICONERROR);

    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(320),
        pWnd->GetSafeHwnd(),
        CLuaConsole::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CLuaConsole.\n");
        m_parent = NULL;
        return;
    }
}

void CLuaConsole::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("LuaScriptConsoleTitle", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };

    Translate(1005, "LuaScriptConsoleDescription");
    HWND hDesc = GetDlgItem(hWnd, 1005);
    buffer = Translations::TranslateOrDefault("LuaScriptConsoleDescription",
        "Check 'Run File' to read the selected Lua script; otherwise, the input window will be used. "
        "Click 'Run' to run the selected code. "
        "Click 'Lua Brush' to run the script at the specified coordinates on the map. "
        "Scripts may damage the map, please save it or execute the snapshot function before running. "
        "Please refer to the documentation for available functions.");
    SetWindowText(hDesc, buffer);

    Translate(1006, "LuaScriptConsoleOuput");
    Translate(1007, "LuaScriptConsoleInput");
    Translate(1008, "LuaScriptConsoleScripts");
    Translate(1011, "Options.Search");
    Translate(Controls::Run, "LuaScriptConsoleRun");
    Translate(Controls::Apply, "LuaScriptConsoleBrush");
    Translate(Controls::RunFile, "LuaScriptConsoleRunFile");

    hExecute = GetDlgItem(hWnd, Controls::Execute);
    hRun = GetDlgItem(hWnd, Controls::Run);
    hOutputBox = GetDlgItem(hWnd, Controls::OutputBox);
    hInputBox = GetDlgItem(hWnd, Controls::InputBox);
    hScripts = GetDlgItem(hWnd, Controls::Scripts);
    hOutputText = GetDlgItem(hWnd, Controls::OutputText);
    hInputText = GetDlgItem(hWnd, Controls::InputText);
    hRunFile = GetDlgItem(hWnd, Controls::RunFile);
    hApply = GetDlgItem(hWnd, Controls::Apply);
    hSearchText = GetDlgItem(hWnd, Controls::SearchText);
    //hStop = GetDlgItem(hWnd, Controls::Stop);
    
    SendMessage(hOutputBox, EM_SETREADONLY, (WPARAM)TRUE, 0);
    //SendMessage(hOutputBox, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(240, 240, 240));
    ExtraWindow::SetEditControlFontSize(hInputBox, 1.4f, true);
    ExtraWindow::SetEditControlFontSize(hOutputBox, 1.4f, true);
    int tabWidth = 16;
    SendMessage(hInputBox, EM_SETTABSTOPS, 1, (LPARAM)&tabWidth);
    SendMessage(hOutputBox, EM_SETTABSTOPS, 1, (LPARAM)&tabWidth);
    applyingScript = false;
    runFile = true;
    applyingScriptFirst = true;
    SendMessage(hRunFile, BM_SETCHECK, runFile, 0);
    CIsoView::ControlKeyIsDown() = false;

    if (ExtConfigs::EnableDarkMode)
    {
        ::SendMessage(hInputBox, EM_SETBKGNDCOLOR, (WPARAM)FALSE, (LPARAM)RGB(32, 32, 32));
        CHARFORMAT cf = { 0 };
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_COLOR;
        cf.crTextColor = RGB(220, 220, 220);
        ::SendMessage(hInputBox, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);

        ::SendMessage(hOutputBox, EM_SETBKGNDCOLOR, (WPARAM)FALSE, (LPARAM)RGB(32, 32, 32));
        ::SendMessage(hOutputBox, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    }

    Lua.collect_garbage();
    Lua = sol::state();

    Lua.open_libraries(sol::lib::base, sol::lib::package
        , sol::lib::string, sol::lib::os
        , sol::lib::math, sol::lib::table
        , sol::lib::debug, sol::lib::bit32
        , sol::lib::io);

    // variables
    Lua.set_function("iso_size", []() {return CMapData::Instance->MapWidthPlusHeight; });
    Lua.set_function("width", []() {return CMapData::Instance->Size.Width; });
    Lua.set_function("height", []() {return CMapData::Instance->Size.Height; });
    Lua.set_function("local_width", []() {return CMapData::Instance->LocalSize.Width; });
    Lua.set_function("local_height", []() {return CMapData::Instance->LocalSize.Height; });
    Lua.set_function("local_top", []() {return CMapData::Instance->LocalSize.Top; });
    Lua.set_function("local_left", []() {return CMapData::Instance->LocalSize.Left; });
    Lua.set_function("waypoint_count", []() {return CINI::CurrentDocument->GetKeyCount("Waypoints"); });
    Lua.set_function("unit_count", []() {return CINI::CurrentDocument->GetKeyCount("Units"); });
    Lua.set_function("infantry_count", []() {return CINI::CurrentDocument->GetKeyCount("Infantry"); });
    Lua.set_function("building_count", []() {return CINI::CurrentDocument->GetKeyCount("Structures"); });
    Lua.set_function("aircraft_count", []() {return CINI::CurrentDocument->GetKeyCount("Aircraft"); });
    Lua.set_function("terrain_count", []() {return CINI::CurrentDocument->GetKeyCount("Terrain"); });
    Lua.set_function("smudge_count", []() {return CINI::CurrentDocument->GetKeyCount("Smudge"); });
    Lua.set_function("player_count", []() {
        if (CMapData::Instance->IsMultiOnly())
        {
            int wp_count = 0;
            for (int i = 0; i < 8; i++)
            {
                FString key;
                key.Format("%d", i);
                if (CINI::CurrentDocument->TryGetString("Waypoints", key) != nullptr)
                    wp_count++;
            }
            return wp_count;
        }
        else
        {
            return 1;
        }
        });
    Lua.set_function("house_count", []() { return CINI::CurrentDocument->GetKeyCount("Houses"); });
    Lua.set_function("country_count", []() { return CINI::CurrentDocument->GetKeyCount("Countries"); });
    Lua.set_function("node_count", [](std::string house) {
        if (house == "")
        {
            if (auto pSection = CINI::CurrentDocument->GetSection("Houses"))
            {
                int count = 0;
                for (auto& [_, house] : pSection->GetEntities())
                {
                    count += CINI::CurrentDocument->GetInteger(house, "NodeCount");
                }
                return count;
            }
        }
        if (!CINI::CurrentDocument->TryGetString("Houses", house.c_str()))
        {
            house += " House";
        }
        if (auto pSection = CINI::CurrentDocument->GetSection(house.c_str()))
        {
            return pSection->GetInteger("NodeCount");
        }
        return 0; });
    Lua.set_function("trigger_count", []() { return CINI::CurrentDocument->GetKeyCount("Triggers"); });
    Lua.set_function("tile_count", []() { return CMapDataExt::TileDataCount; });
    Lua.set_function("tile_set_count", []() { return CMapDataExt::TileSet_starts.size() - 1; });
    Lua.set_function("tag_count", []() { return CINI::CurrentDocument->GetKeyCount("Tags"); });
    Lua.set_function("theater", []() {return CINI::CurrentDocument->GetString("Map", "Theater").m_pchData; });
    Lua.set_function("is_multiplay", []() {return CMapData::Instance->IsMultiOnly(); });

    // misc functions
    Lua.set_function("print", lua_print);
    Lua.set_function("clear", clear);
    Lua.set_function("avoid_time_out", avoid_time_out);
    Lua.set_function("sleep", sleep);
    Lua.set_function("message_box", message_box);
    Lua.new_usertype<multi_select_box>("multi_select_box",
        sol::constructors<multi_select_box(std::string)>(),
        //"options", sol::readonly(&multi_select_box::options),
        "caption", &multi_select_box::caption,
        "selected_keys", &multi_select_box::selected_keys,
        "selected_values", &multi_select_box::selected_values,
        "add_option", &multi_select_box::add_option,
        "sort_options", &multi_select_box::sort_options,
        "do_modal", &multi_select_box::do_modal
    );
    Lua.new_usertype<select_box>("select_box",
        sol::constructors<select_box(std::string)>(),
        //"options", sol::readonly(&select_box::options),
        "caption", &select_box::caption,
        "selected_key", &select_box::selected_key,
        "selected_value", &select_box::selected_value,
        "add_option", &select_box::add_option,
        "sort_options", &select_box::sort_options,
        "do_modal", &select_box::do_modal
    );
    Lua.set_function("input_box", input_box);
    Lua.set_function("read_input", read_input);

    // game objects
    Lua.set_function("place_terrain", place_terrain);
    Lua.set_function("remove_terrain", remove_terrain);
    Lua.set_function("place_smudge", place_smudge);
    Lua.set_function("remove_smudge", remove_smudge);
    Lua.set_function("place_overlay", [](int y, int x, int overlay, sol::optional<int> overlayData) {
        if (!overlayData) {
            overlayData = -1;
        }
        place_overlay(y, x, overlay, overlayData.value());
        });
    Lua.set_function("remove_overlay", remove_overlay);
    Lua.set_function("smooth_ore", smooth_ore);
    Lua.set_function("place_wall", place_wall);
    Lua.set_function("place_waypoint", [](int y, int x, sol::optional<int> index) {
        if (!index) {
            index = -1;
        }
        return place_waypoint(y, x, index.value());
        });
    Lua.set_function("remove_waypoint", remove_waypoint);
    Lua.set_function("remove_waypoint_at", remove_waypoint_at);
    Lua.new_usertype<infantry>("infantry",
        sol::constructors<infantry(std::string, std::string, int, int)>(),
        "house", &infantry::House,
        "type", &infantry::TypeID,
        "x", &infantry::Y,
        "y", &infantry::X,
        "health", &infantry::Health,
        "status", &infantry::Status,
        "tag", &infantry::Tag,
        "facing", &infantry::Facing,
        "veterancy", &infantry::VeterancyPercentage,
        "group", &infantry::Group,
        "above_ground", &infantry::IsAboveGround,
        "auto_no_recruit", &infantry::AutoNORecruitType,
        "auto_yes_recruit", &infantry::AutoYESRecruitType,
        "subcell", &infantry::SubCell,
        "place", &infantry::place,
        "remove", &infantry::remove
    );
    Lua.set_function("place_infantry", place_infantry);
    Lua.set_function("remove_infantry", [](int indexY, sol::optional<int> x, sol::optional<int> pos) {
        if (!x) {
            x = -1;
        }
        if (!pos) {
            pos = -1;
        }
        remove_infantry(indexY, x.value(), pos.value());
        });
    Lua.set_function("get_infantry", [](int indexY, sol::optional<int> x, sol::optional<int> pos) {
        if (!x) {
            x = -1;
        }
        if (!pos) {
            pos = -1;
        }
        return get_infantry(indexY, x.value(), pos.value());
        });
    Lua.set_function("get_infantries", get_infantries);
    Lua.new_usertype<unit>("unit",
        sol::constructors<unit(std::string, std::string, int, int)>(),
        "house", &unit::House,
        "type", &unit::TypeID,
        "x", &unit::Y,
        "y", &unit::X,
        "health", &unit::Health,
        "status", &unit::Status,
        "tag", &unit::Tag,
        "facing", &unit::Facing,
        "veterancy", &unit::VeterancyPercentage,
        "group", &unit::Group,
        "above_ground", &unit::IsAboveGround,
        "auto_no_recruit", &unit::AutoNORecruitType,
        "auto_yes_recruit", &unit::AutoYESRecruitType,
        "follow", &unit::FollowsIndex,
        "place", &unit::place,
        "remove", &unit::remove
    );
    Lua.set_function("place_unit", place_unit);
    Lua.set_function("remove_unit", [](int indexY, sol::optional<int> x) {
        if (!x) {
            x = -1;
        }
        remove_unit(indexY, x.value());
        });
    Lua.set_function("get_unit", [](int indexY, sol::optional<int> x) {
        if (!x) {
            x = -1;
        }
        return get_unit(indexY, x.value());
        });
    Lua.set_function("get_units", get_units);
    Lua.new_usertype<aircraft>("aircraft",
        sol::constructors<aircraft(std::string, std::string, int, int)>(),
        "house", &aircraft::House,
        "type", &aircraft::TypeID,
        "x", &aircraft::Y,
        "y", &aircraft::X,
        "health", &aircraft::Health,
        "status", &aircraft::Status,
        "tag", &aircraft::Tag,
        "facing", &aircraft::Facing,
        "veterancy", &aircraft::VeterancyPercentage,
        "group", &aircraft::Group,
        "auto_no_recruit", &aircraft::AutoNORecruitType,
        "auto_yes_recruit", &aircraft::AutoYESRecruitType,
        "place", &aircraft::place,
        "remove", &aircraft::remove
    );
    Lua.set_function("place_aircraft", place_aircraft);
    Lua.set_function("remove_aircraft", [](int indexY, sol::optional<int> x) {
        if (!x) {
            x = -1;
        }
        remove_aircraft(indexY, x.value());
        });
    Lua.set_function("get_aircraft", [](int indexY, sol::optional<int> x) {
        if (!x) {
            x = -1;
        }
        return get_aircraft(indexY, x.value());
        });
    Lua.set_function("get_aircrafts", get_aircrafts);
    Lua.new_usertype<building>("building",
        sol::constructors<building(std::string, std::string, int, int)>(),
        "house", &building::House,
        "type", &building::TypeID,
        "health", &building::Health,
        "x", &building::Y,
        "y", &building::X,
        "facing", &building::Facing,
        "tag", &building::Tag,
        "ai_sell", &building::AISellable,
        "ai_rebuild", &building::AIRebuildable,
        "powered", &building::PoweredOn,
        "upgrades", &building::Upgrades,
        "spot_light", &building::SpotLight,
        "upgrade1", &building::Upgrade1,
        "upgrade2", &building::Upgrade2,
        "upgrade3", &building::Upgrade3,
        "ai_repair", &building::AIRepairable,
        "nominal", &building::Nominal,
        "place", &building::place,
        "place_node", &building::place_node,
        "remove", &building::remove
    );
    Lua.set_function("place_building", [](std::string house, std::string type, int y, int x, sol::optional<bool> ignoreOverlap) {
        if (!ignoreOverlap) {
            ignoreOverlap = false;
        }
        place_building(house, type, y, x, ignoreOverlap.value());
        });
    Lua.set_function("remove_building", [](int indexY, sol::optional<int> x) {
        if (!x) {
            x = -1;
        }
        remove_building(indexY, x.value());
        });
    Lua.set_function("get_building", [](int indexY, sol::optional<int> x) {
        if (!x) {
            x = -1;
        }
        return get_building(indexY, x.value());
        });
    Lua.set_function("get_buildings", get_buildings);
    Lua.set_function("place_node", [](std::string house, std::string type, int y, int x, sol::optional<int> index) {
        if (!index) {
            index = -1;
        }
        return place_node(house, type, y, x, index.value());
        });
    Lua.set_function("remove_node", remove_node);
    Lua.set_function("get_uiname", [](std::string id) { return std::string(CViewObjectsExt::QueryUIName(id.c_str())); });
    Lua.set_function("translate_house", [](std::string id, bool back = false) {
        return std::string(Translations::ParseHouseName(id.c_str(), !back));
        });

    // tiles
    Lua.new_usertype<cell>("cell",
        // use game coord
        "x", sol::readonly(&cell::Y),
        "y", sol::readonly(&cell::X),
        "unit", sol::readonly(&cell::Unit),
        "infantry", sol::readonly(&cell::Infantry),
        "aircraft", sol::readonly(&cell::Aircraft),
        "building", sol::readonly(&cell::Structure),
        //"TypeListIndex", &cell::TypeListIndex,
        "terrain", sol::readonly(&cell::Terrain),
        "terrain_type", sol::readonly(&cell::TerrainType),
        "smudge", sol::readonly(&cell::Smudge),
        "smudge_type", sol::readonly(&cell::SmudgeType),
        "waypoint", sol::readonly(&cell::Waypoint),
        "node_building", sol::readonly(&cell::BuildingID),
        "node_id", sol::readonly(&cell::BasenodeID),
        "node_house", sol::readonly(&cell::House),
        "overlay", &cell::Overlay,
        "overlay_data", &cell::OverlayData,
        "tile", &cell::TileIndex,
        //"TileIndexHiPart", &cell::TileIndexHiPart,
        "subtile", &cell::TileSubIndex,
        "height", &cell::Height,
        //"IceGrowth", &cell::IceGrowth,
        "cell_tag", sol::readonly(&cell::CellTag),
        "tube", sol::readonly(&cell::Tube),
        "tube_data", sol::readonly(&cell::TubeDataIndex),
        //"StatusFlag", &cell::StatusFlag,
        //"NotAValidCell", &cell::NotAValidCell,
        "hidden", &cell::IsHiddenCell,
        //"RedrawTerrain", &cell::RedrawTerrain,
        //"CliffHack", &cell::CliffHack,
        "alt_image", sol::readonly(&cell::AltIndex),
        "is_hidden", &cell::IsHidden,
        "is_multi_selected", &cell::IsMultiSelected,
        "apply", &cell::apply
    );
    Lua["cell"]["new"] = []() {
        write_lua_console("Creation of cell instances is forbidden.");
        return sol::nil;
        };
    Lua.set_function("get_cell", get_cell);
    Lua.set_function("get_cells", get_cells);
    Lua.set_function("get_multi_selected_cells", get_multi_selected_cells);
    Lua.set_function("get_hidden_cells", get_hidden_cells);
    Lua.new_usertype<tile>("tile",
        "x", sol::readonly(&tile::RelativeY),
        "y", sol::readonly(&tile::RelativeX),
        "valid", sol::readonly(&tile::Valid),
        "tile_index", sol::readonly(&tile::TileIndex),
        "tile_sub_index", sol::readonly(&tile::TileSubIndex),
        "height", sol::readonly(&tile::Height),
        "alt_count", sol::readonly(&tile::AltCount),
        "tile_set", sol::readonly(&tile::TileSet),
        "ramp_type", sol::readonly(&tile::RampType),
        "land_type", sol::readonly(&tile::LandType)
    );
    Lua["tile"]["new"] = []() {
        write_lua_console("Creation of tile instances is forbidden.");
        return sol::nil;
        };
    Lua.set_function("get_tile_block", tile::get_tile_block);
    Lua.set_function("get_whole_tile", tile::get_whole_tile);
    Lua.set_function("place_whole_tile", place_whole_tile);
    Lua.set_function("place_tile", [](int y, int x, tile tile, sol::optional<int> height, sol::optional<int> altType) {
        if (!height) {
            height = -1;
        }
        if (!altType) {
            altType = -1;
        }
        place_tile(y, x, tile, height.value(), altType.value());
        });
    Lua.set_function("set_height", set_height);
    Lua.set_function("hide_cell", [](int y, int x, sol::optional<int> type) {
        if (!type) {
            type = 1;
        }
        hide_tile(y, x, type.value());
        });
    Lua.set_function("hide_tile_set", [](int index, sol::optional<int> type) {
        if (!type) {
            type = 1;
        }
        hide_tile_set(index, type.value());
        });
    Lua.set_function("multi_select_cell", [](int y, int x, sol::optional<int> type) {
        if (!type) {
            type = 1;
        }
        multi_select_tile(y, x, type.value());
        });
    Lua.set_function("multi_select_tile_set", [](int index, sol::optional<int> type) {
        if (!type) {
            type = 1;
        }
        multi_select_tile_set(index, type.value());
        });
    Lua.set_function("create_shore", create_shore);
    Lua.set_function("smooth_lat", smooth_lat);
    Lua.set_function("create_slope", create_slope);

    // ini options
    Lua.set_function("get_string", [](std::string section, std::string key, sol::optional<std::string> def, sol::optional<std::string> loadFrom) {
        if (!loadFrom) {
            loadFrom = "map";
        }
        if (!def) {
            def = "";
        }
        return get_string(section, key, def.value(), loadFrom.value());
        });
    Lua.set_function("get_sections", [](sol::optional<std::string> loadFrom) {
        if (!loadFrom) {
            loadFrom = "map";
        }
        auto&& result = get_sections(loadFrom.value());
        sol::table luaTable = Lua.create_table();
        int index = 1;
        for (const auto& str : result) {
            luaTable[index++] = str;
        }
        return luaTable;
        });
    Lua.set_function("get_keys", [](std::string section, sol::optional<std::string> loadFrom) {
        if (!loadFrom) {
            loadFrom = "map";
        }
        return get_keys(section, loadFrom.value());
        });
    Lua.set_function("get_values", [](std::string section, sol::optional<std::string> loadFrom) {
        if (!loadFrom) {
            loadFrom = "map";
        }
        return get_values(section, loadFrom.value());
        });
    Lua.set_function("get_key_value_pairs", [](std::string section, sol::optional<std::string> loadFrom) {
        if (!loadFrom) {
            loadFrom = "map";
        }
        auto&& result = get_key_value_pairs(section, loadFrom.value());
        sol::table luaTable = Lua.create_table();
        for (const auto& [key, value] : result) {
            luaTable[key] = value;
        }
        return luaTable;
        });
    Lua.set_function("get_ordered_values", [](std::string section, sol::optional<std::string> loadFrom) {
        if (!loadFrom) {
            loadFrom = "map";
        }
        auto&& result = get_ordered_values(section, loadFrom.value());
        sol::table luaTable = Lua.create_table();
        for (const auto& [key, value] : result) {
            luaTable[key] = value;
        }
        return luaTable;
        });
    Lua.set_function("get_integer", [](std::string section, std::string key, sol::optional<int> def, sol::optional<std::string> loadFrom) {
        if (!loadFrom) {
            loadFrom = "map";
        }
        if (!def) {
            def = 0;
        }
        return get_integer(section, key, def.value(), loadFrom.value());
        });
    Lua.set_function("get_float", [](std::string section, std::string key, sol::optional<float> def, sol::optional<std::string> loadFrom) {
        if (!loadFrom) {
            loadFrom = "map";
        }
        if (!def) {
            def = 0.0f;
        }
        return get_float(section, key, def.value(), loadFrom.value());
        });
    Lua.set_function("get_bool", [](std::string section, std::string key, sol::optional<bool> def, sol::optional<std::string> loadFrom) {
        if (!loadFrom) {
            loadFrom = "map";
        }
        if (!def) {
            def = false;
        }
        return get_bool(section, key, def.value(), loadFrom.value());
        });
    Lua.set_function("write_string", write_string);
    Lua.set_function("delete_key", delete_key);
    Lua.set_function("delete_section", delete_section);
    Lua.set_function("get_free_waypoint", get_free_waypoint);
    Lua.set_function("get_free_key", get_free_key);
    Lua.set_function("get_free_id", GetAvailableIndex);
    Lua.set_function("split_string", [](std::string str, sol::optional<std::string> delimiter) {
        if (!delimiter) {
            delimiter = ",";
        }
        return split_string(str, delimiter.value());
        });
    Lua.set_function("get_param", [](std::string section, std::string key, int index
        , sol::optional<std::string> delimiter, sol::optional<std::string> loadFrom) {
            if (!loadFrom) {
                loadFrom = "map";
            }
            if (!delimiter) {
                delimiter = ",";
            }
            return get_param(section, key, index, delimiter.value(), loadFrom.value());
        });
    Lua.set_function("set_param", [](std::string section, std::string key, std::string value, int index, sol::optional<std::string> delimiter) {
        if (!delimiter) {
            delimiter = ",";
        }
        set_param(section, key, value, index, delimiter.value());
        });
    Lua.set_function("trim_index", trim_index);
    Lua.set_function("waypoint_to_string", [](std::string wp) { return STDHelpers::WaypointToString(wp).c_str(); });
    Lua.set_function("string_to_waypoint", [](std::string str) { return STDHelpers::StringToWaypointStr(str).c_str(); });

    // fa2 logic
    Lua.set_function("update_building", []() {CMapData::Instance->UpdateFieldStructureData(FALSE); needRedraw = true; });
    Lua.set_function("update_infantry", []() {CMapData::Instance->UpdateFieldInfantryData(FALSE); needRedraw = true; });
    Lua.set_function("update_unit", []() {CMapData::Instance->UpdateFieldUnitData(FALSE); needRedraw = true; });
    Lua.set_function("update_aircraft", []() {CMapData::Instance->UpdateFieldAircraftData(FALSE); needRedraw = true; });
    Lua.set_function("update_terrain", []() {CMapData::Instance->UpdateFieldTerrainData(FALSE); needRedraw = true; });
    Lua.set_function("update_waypoint", []() {CMapData::Instance->UpdateFieldWaypointData(FALSE); needRedraw = true; });
    Lua.set_function("update_node", []() {CMapData::Instance->UpdateFieldBasenodeData(FALSE); needRedraw = true; });
    Lua.set_function("update_overlay", []() {CMapData::Instance->UpdateFieldOverlayData(FALSE); needRedraw = true; });
    Lua.set_function("update_tube", []() {CMapData::Instance->UpdateFieldTubeData(FALSE); needRedraw = true; });
    Lua.set_function("update_smudge", []() {CMapData::Instance->UpdateFieldSmudgeData(FALSE); needRedraw = true; });
    Lua.set_function("update_tiles", []() {CMapData::Instance->UpdateMapFieldData(FALSE); needRedraw = true; });
    Lua.set_function("update_trigger", []() {        
        if (CNewTrigger::GetHandle())
            ::SendMessage(CNewTrigger::GetHandle(), 114514, 0, 0);
        else
            CMapDataExt::UpdateTriggers(); });
    Lua.set_function("redraw_window", redraw_window);
    Lua.set_function("update_minimap", [](sol::optional<int> y, sol::optional<int> x) {
        if (!x || !y) {
            update_minimap();
        }
        else {
            update_minimap(y.value(), x.value());
        }
        });
    Lua.set_function("create_snapshot", create_snapshot);
    Lua.set_function("restore_snapshot", restore_snapshot);
    Lua.set_function("clear_snapshot", clear_snapshot);
    Lua.set_function("save_undo", save_undo);
    Lua.set_function("save_redo", save_redo);
    Lua.set_function("in_map", [](int y, int x) {return CMapData::Instance->IsCoordInMap(x, y); });
    Lua.set_function("move_to", [](int yindex, sol::optional<int>x) {
        if (!x) {
            x = -1;
        }
        move_to(yindex, x.value()); 
        });

    // triggers & teams
    Lua.new_usertype<tag>("tag",
        sol::constructors<tag()>(),
        "id", sol::readonly(&tag::ID),
        "name", &tag::Name,
        "type", &tag::RepeatType
    );
    Lua["tag"]["new"] = []() {
        write_lua_console("Creation of tag instances is forbidden.");
        return sol::nil;
        };
    Lua.new_usertype<trigger>("trigger",
        sol::constructors<trigger(std::string), trigger()>(),
        "id", sol::readonly(&trigger::ID),
        "name", &trigger::Name,
        "house", &trigger::House,
        "tags", sol::readonly(&trigger::Tags),
        "attached_trigger", &trigger::AttachedTrigger,
        "disabled", &trigger::Disabled,
        "easy", &trigger::EasyEnabled,
        "medium", &trigger::MediumEnabled,
        "hard", &trigger::HardEnabled,
        "events", sol::readonly(&trigger::Events),
        "actions", sol::readonly(&trigger::Actions),
        "add_tag", &trigger::add_tag,
        "add_event", &trigger::add_event,
        "add_action", &trigger::add_action,
        "delete_tag", &trigger::delete_tag,
        "delete_tags", &trigger::delete_tags,
        "delete_event", &trigger::delete_event,
        "delete_action", &trigger::delete_action,
        "change_id", &trigger::change_id,
        "apply", &trigger::apply,
        "delete", &trigger::delete_trigger_self,
        "release_id", &trigger::release_id
    );
    Lua.set_function("delete_trigger", trigger::delete_trigger);
    Lua.set_function("delete_tag", trigger::delete_tag_static);
    Lua.set_function("get_trigger", trigger::get_trigger);
    Lua.set_function("place_celltag", place_celltag);
    Lua.set_function("remove_celltag", remove_celltag);
    Lua.set_function("remove_celltags", remove_celltags);

    Lua.new_usertype<ai_trigger>("ai_trigger",
        sol::constructors<ai_trigger(std::string), ai_trigger()>(),
        "id", sol::readonly(&ai_trigger::ID),
        "name", &ai_trigger::Name,
        "team1", &ai_trigger::Team1,
        "house", &ai_trigger::House,
        "tech_level", &ai_trigger::TechLevel,
        "condition", &ai_trigger::ConditionType,
        "object", &ai_trigger::ComparisonObject,
        //"comparators", sol::readonly(&ai_trigger::Comparators),
        "init_weight", &ai_trigger::InitialWeight,
        "min_weight", &ai_trigger::MinWeight,
        "max_weight", &ai_trigger::MaxWeight,
        "is_for_skirmish", &ai_trigger::IsForSkirmish,
        //"unused", &ai_trigger::unused,
        "side", &ai_trigger::Side,
        "is_base_defense", &ai_trigger::IsBaseDefense,
        "team2", &ai_trigger::Team2,
        "easy", &ai_trigger::EnabledInE,
        "medium", &ai_trigger::EnabledInM,
        "hard", &ai_trigger::EnabledInH,
        "enabled", &ai_trigger::Enabled,
        "comparator", &ai_trigger::Comparator,
        "amount", &ai_trigger::Amount,
        "apply", &ai_trigger::apply,
        "change_id", &ai_trigger::change_id,
        "release_id", &ai_trigger::release_id,
        "delete", &ai_trigger::delete_ai_trigger_self
    );
    Lua.set_function("delete_ai_trigger", ai_trigger::delete_ai_trigger);
    Lua.set_function("get_ai_trigger", ai_trigger::get_ai_trigger);

    Lua.new_usertype<script>("script",
        sol::constructors<script(std::string), script()>(),
        "id", sol::readonly(&script::ID),
        "name", &script::Name,
        "actions", sol::readonly(&script::Actions),
        "params", sol::readonly(&script::Params),
        "add_action", &script::add_action,
        "delete_action", &script::delete_action,
        "apply", &script::apply,
        "change_id", &script::change_id,
        "release_id", &script::release_id,
        "delete", &script::delete_script_self
        );
    Lua.set_function("delete_script", script::delete_script);
    Lua.set_function("get_script", script::get_script);

    Lua.new_usertype<task_force>("task_force",
        sol::constructors<task_force(std::string), task_force()>(),
        "id", sol::readonly(&task_force::ID),
        "name", &task_force::Name,
        "group", &task_force::Group,
        "numbers", sol::readonly(&task_force::Numbers),
        "units", sol::readonly(&task_force::Units),
        "add_number", &task_force::add_number,
        "delete_number", &task_force::delete_number,
        "apply", &task_force::apply,
        "change_id", &task_force::change_id,
        "release_id", &task_force::release_id,
        "delete", &task_force::delete_task_force_self
        );
    Lua.set_function("delete_task_force", task_force::delete_task_force);
    Lua.set_function("get_task_force", task_force::get_task_force);

    Lua.new_usertype<team>("team",
        sol::constructors<team(std::string), team()>(),
        "id", sol::readonly(&team::ID),
        "name", &team::Name,
        "house", &team::House,
        "task_force", &team::Taskforce,
        "script", &team::Script,
        "tag", &team::Tag,
        "veteran_level", &team::VeteranLevel,
        "priority", &team::Priority,
        "max", &team::Max,
        "techlevel", &team::Techlevel,
        "transport_waypoint", &team::TransportWaypoint,
        "group", &team::Group,
        "waypoint", &team::Waypoint,
        "mind_control_decision", &team::MindControlDecision,
        "full", &team::Full,
        "whiner", &team::Whiner,
        "droppod", &team::Droppod,
        "suicide", &team::Suicide,
        "loadable", &team::Loadable,
        "prebuild", &team::Prebuild,
        "annoyance", &team::Annoyance,
        "ion_immune", &team::IonImmune,
        "recruiter", &team::Recruiter,
        "reinforce", &team::Reinforce,
        "aggressive", &team::Aggressive,
        "autocreate", &team::Autocreate,
        "guard_slower", &team::GuardSlower,
        "on_trans_only", &team::OnTransOnly,
        "avoid_threats", &team::AvoidThreats,
        "loose_recruit", &team::LooseRecruit,
        "is_base_defense", &team::IsBaseDefense,
        "use_transport_origin", &team::UseTransportOrigin,
        "only_target_house_enemy", &team::OnlyTargetHouseEnemy,
        "transports_return_on_unload", &team::TransportsReturnOnUnload,
        "are_team_members_recruitable", &team::AreTeamMembersRecruitable,
        "apply", &team::apply,
        "change_id", &team::change_id,
        "release_id", &team::release_id,
        "delete", &team::delete_team_self
        );
    Lua.set_function("delete_team", team::delete_team);
    Lua.set_function("get_team", team::get_team);

    Lua.set_function("running_lua_brush", []() {return CLuaConsole::applyingScript; });

    Update(hWnd);
}

void CLuaConsole::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);
    ShowWindow(hWnd, SW_HIDE);

    CLuaConsole::m_hwnd = NULL;
    CLuaConsole::m_parent = NULL;
}

void CLuaConsole::Update(HWND& hWnd, const char* filter)
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    while (SendMessage(hScripts, LB_DELETESTRING, 0, NULL) != CB_ERR);
    std::string scriptPath = CFinalSunApp::ExePath();
    scriptPath += "\\Scripts\\";
    if (fs::exists(scriptPath) && fs::is_directory(scriptPath)) {
        for (const auto& entry : fs::directory_iterator(scriptPath)) {
            if (entry.is_regular_file() && entry.path().extension().string() == ".lua") {
                if ((strlen(filter) && ExtraWindow::IsLabelMatch(entry.path().filename().string().c_str(), filter)) || !strlen(filter))
                    SendMessage(hScripts, LB_ADDSTRING, 0, (LPARAM)(LPCSTR)entry.path().filename().string().c_str());
            }
        }
    }
}

BOOL CALLBACK CLuaConsole::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CLuaConsole::Initialize(hWnd);
        RECT rect;
        GetClientRect(hWnd, &rect);
        origWndWidth = rect.right - rect.left;
        origWndHeight = rect.bottom - rect.top;
        minSizeSet = false;
        return TRUE;
    }
    case WM_GETMINMAXINFO: {
        if (!minSizeSet) {
            int borderWidth = GetSystemMetrics(SM_CXBORDER);
            int borderHeight = GetSystemMetrics(SM_CYBORDER);
            int captionHeight = GetSystemMetrics(SM_CYCAPTION);
            minWndWidth = origWndWidth + 2 * borderWidth;
            minWndHeight = origWndHeight + captionHeight + 2 * borderHeight;
            minSizeSet = true;
        }
        MINMAXINFO* pMinMax = (MINMAXINFO*)lParam;
        pMinMax->ptMinTrackSize.x = minWndWidth;
        pMinMax->ptMinTrackSize.y = minWndHeight;
        return TRUE;
    }
    case WM_SIZE: {
        int newWndWidth = LOWORD(lParam);
        int newWndHeight = HIWORD(lParam);

        int heightOffset = newWndHeight - origWndHeight;
        int heightOffsetOutput = heightOffset / 2;
        int heightOffsetInput = heightOffset - heightOffsetOutput;

        RECT rect;
        GetWindowRect(hOutputBox, &rect);
        POINT topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);

        int newWidth = rect.right - rect.left + newWndWidth - origWndWidth;
        int newHeight = rect.bottom - rect.top + heightOffsetOutput;
        MoveWindow(hOutputBox, topLeft.x, topLeft.y, newWidth, newHeight, TRUE);

        GetWindowRect(hInputBox, &rect);
        topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);
        newWidth = rect.right - rect.left + newWndWidth - origWndWidth;
        newHeight = rect.bottom - rect.top + heightOffsetInput;
        MoveWindow(hInputBox, topLeft.x, topLeft.y + heightOffsetOutput, newWidth, newHeight, TRUE);

        GetWindowRect(hInputText, &rect);
        topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);
        newWidth = rect.right - rect.left;
        newHeight = rect.bottom - rect.top;
        MoveWindow(hInputText, topLeft.x, topLeft.y + heightOffsetOutput, newWidth, newHeight, TRUE);

        GetWindowRect(hScripts, &rect);
        topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);
        newWidth = rect.right - rect.left + newWndWidth - origWndWidth;
        newHeight = rect.bottom - rect.top;
        MoveWindow(hScripts, topLeft.x, topLeft.y, newWidth, newHeight, TRUE);

        GetWindowRect(hRun, &rect);
        topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);
        newWidth = rect.right - rect.left;
        newHeight = rect.bottom - rect.top;
        MoveWindow(hRun, topLeft.x + newWndWidth - origWndWidth, topLeft.y, newWidth, newHeight, TRUE);

        GetWindowRect(hApply, &rect);
        topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);
        newWidth = rect.right - rect.left;
        newHeight = rect.bottom - rect.top;
        MoveWindow(hApply, topLeft.x + newWndWidth - origWndWidth, topLeft.y, newWidth, newHeight, TRUE);

        GetWindowRect(hRunFile, &rect);
        topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);
        newWidth = rect.right - rect.left;
        newHeight = rect.bottom - rect.top;
        MoveWindow(hRunFile, topLeft.x + newWndWidth - origWndWidth, topLeft.y, newWidth, newHeight, TRUE);

        origWndWidth = newWndWidth;
        origWndHeight = newWndHeight;
        break;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::Run:
            applyingScript = false;
            OnClickRun(runFile);
            break;
        case Controls::Apply:
            applyingScript = true;
            CLuaConsole::applyingScriptFirst = true;
            CIsoView::CurrentCommand->Command = 0x23;
            break;
        case Controls::RunFile:
            if (CODE == BN_CLICKED)
            {
                runFile = SendMessage(hRunFile, BM_GETCHECK, 0, 0);
            }
            break;
        case Controls::SearchText:
            if (CODE == EN_CHANGE)
                OnEditchangeSearch(hWnd);
            break;
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CLuaConsole::Close(hWnd);
        return TRUE;
    }
    case 114514: // used for update
    {
        OnEditchangeSearch(hWnd);
        return TRUE;
    }
    }

    // Process this message through default handler
    return FALSE;
}

void CLuaConsole::UpdateCoords(int x, int y, bool firstRun, bool holdingClick)
{
    if (firstRun)
    {
        CLuaConsole::applyingScriptFirst = false;
        Lua["first_run"] = true;
    }
    else
        Lua["first_run"] = false;
    Lua["control_down"] = CIsoView::ControlKeyIsDown();
    Lua["holding_click"] = holdingClick;
    Lua["X"] = y;
    Lua["Y"] = x;
}

void CLuaConsole::OnEditchangeSearch(HWND& hWnd)
{
    char buffer[512]{ 0 };
    GetWindowText(hSearchText, buffer, 511);
    Update(hWnd, buffer);
}

void CLuaConsole::OnClickRun(bool fromFile)
{
    std::string script;
    if (fromFile)
    {
        std::string scriptPath = CFinalSunApp::ExePath();
        scriptPath += "\\Scripts\\";
        int idx = SendMessage(hScripts, LB_GETCURSEL, NULL, NULL);
        char fileName[260]{ 0 };
        SendMessage(hScripts, LB_GETTEXT, idx, (LPARAM)fileName);
        scriptPath += fileName;
        std::ifstream file(scriptPath);
        if (file.fail())
            return;
        std::stringstream buffer;
        buffer << file.rdbuf();
        script = buffer.str();
    }
    else
    {
        GetWindowText(hInputBox, Buffer, BUFFER_SIZE);
        script = Buffer;
    }

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    char* dt = ctime(&now_c);
    auto timeStart = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::ostringstream oss;
    oss << "==============================\r\n   Lua Script Console for FA2SPHE\r\n   Time: " << dt << "   ==============================";
    std::string text = oss.str();

    write_lua_console(text);
    LuaFunctions::time = timeStart;
    skipBuildingUpdate = true;
    VEHGuard guard(false);
    try {
        sol::protected_function_result result = Lua.script(script, sol::script_pass_on_error);
        if (!result.valid()) {
            sol::error err = result;
            std::string errorMessage = "Lua Error: " + std::string(err.what());

            sol::call_status status = result.status();
            switch (status) {
            case sol::call_status::syntax:
                errorMessage += " (Syntax Error)";
                break;
            case sol::call_status::runtime:
                errorMessage += " (Runtime Error)";
                break;
            case sol::call_status::memory:
                errorMessage += " (Memory Allocation Error)";
                break;
            case sol::call_status::handler:
                errorMessage += " (Message Handler Error)";
                break;
            case sol::call_status::gc:
                errorMessage += " (Garbage Collector Error)";
                break;
            case sol::call_status::file:
                errorMessage += " (File Error)";
                break;
            default:
                errorMessage += " (Unknown Error)";
                break;
            }

            write_lua_console(errorMessage);
        }
        else {
            oss.str("");
            auto timeEnd = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            oss << "Successfully executed script.\r\n   Elapsed Time: " << timeEnd - timeStart << " ms.";
            text = oss.str();
            write_lua_console(text);
        }
    }
    catch (const std::exception& e) {
        std::string errorMessage = "Critical Error: " + std::string(e.what());
        write_lua_console(errorMessage);
    }
    catch (...) {
        std::string errorMessage = "Critical Error: Unknown exception occurred.";
        write_lua_console(errorMessage);
    }

    for (auto& ini : LoadedINIs)
    {
        GameDelete(ini.second);
    }
    LoadedINIs.clear();
    skipBuildingUpdate = false;
    if (updateBuilding)
    {
        updateBuilding = false;	
        CMapDataExt::UpdateFieldStructureData_RedrawMinimap();
    }
    if (updateUnit)
    {
        updateUnit = false;
        CMapDataExt::UpdateFieldUnitData_RedrawMinimap();
    }
    if (updateInfantry)
    {
        updateInfantry = false;
        CMapDataExt::UpdateFieldInfantryData_RedrawMinimap();
    }
    if (updateAircraft)
    {
        updateAircraft = false;
        CMapDataExt::UpdateFieldAircraftData_RedrawMinimap();
    }
    if (updateNode)
    {
        updateNode = false;
        CMapData::Instance->UpdateFieldBasenodeData(FALSE);
    }
    if (updateCellTag)
    {
        updateCellTag = false;
        CMapData::Instance->UpdateFieldCelltagData(FALSE);
    }
    if (updateTrigger)
    {
        updateTrigger = false;
        if (CNewTrigger::GetHandle())
            ::SendMessage(CNewTrigger::GetHandle(), 114514, 0, 0);
        else
            CMapDataExt::UpdateTriggers();
    }
    if (updateAITrigger)
    {
        updateAITrigger = false;
        if (CNewAITrigger::GetHandle())
            ::SendMessage(CNewAITrigger::GetHandle(), 114514, 0, 0);
    }
    if (updateScript)
    {
        updateScript = false;
        if (CNewScript::GetHandle())
            ::SendMessage(CNewScript::GetHandle(), 114514, 0, 0);
    }
    if (updateTeam)
    {
        updateTeam = false;
        if (CNewTeamTypes::GetHandle())
            ::SendMessage(CNewTeamTypes::GetHandle(), 114514, 0, 0);
    }
    if (updateTaskforce)
    {
        updateTaskforce = false;
        if (CNewTaskforce::GetHandle())
            ::SendMessage(CNewTaskforce::GetHandle(), 114514, 0, 0);
    }
    if (updateMinimap)
    {
        updateMinimap = false;
        update_minimap();
    }
    if (recalculateOre)
    {
        auto pExt = CMapDataExt::GetExtension();
        pExt->MoneyCount = 0;
        for (int x = 0; x < pExt->MapWidthPlusHeight; ++x)
        {
            for (int y = 0; y < pExt->MapWidthPlusHeight; ++y)
            {
                if (pExt->IsCoordInMap(x, y))
                {
                    auto cell = pExt->GetCellAt(x, y);
                    if (pExt->IsOre(cell->Overlay))
                    {
                        pExt->MoneyCount += pExt->GetOreValue(cell->Overlay, cell->OverlayData);
                    }
                }
            }
        }
        recalculateOre = false;
    }
    if (needRedraw) {
        redraw_window();
        needRedraw = false;
    }
    CMapDataExt::DeleteBuildingByIniID = false;
}
