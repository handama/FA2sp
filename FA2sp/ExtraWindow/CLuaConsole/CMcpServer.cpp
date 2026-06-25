#include "CMcpServer.h"
#include "CLuaConsole.h"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include <httplib.h>
#include <json.hpp>
#include <thread>
#include <fstream>
#include <sstream>
#include <CINI.h>
#include <CMapData.h>
#include <CFinalSunDlg.h>
#include <filesystem>
#include "../../Ext/CMapData/Body.h"
#include "../Common.h"
#include "../CNewTrigger/CNewTrigger.h"
#include "../CNewAITrigger/CNewAITrigger.h"
#include "../CNewScript/CNewScript.h"
#include "../CNewTeamTypes/CNewTeamTypes.h"
#include "../CNewTaskforce/CNewTaskforce.h"
#include "../CNewLocalVariables/CNewLocalVariables.h"

using json = nlohmann::json;

// Static members
bool CMcpServer::m_running = false;
int CMcpServer::m_port = 19198;
void* CMcpServer::m_server = nullptr;

// ---------------------------------------------------------------------------
// Helper : read an INI section as a raw string
// ---------------------------------------------------------------------------
static std::string GetDocRoot()
{
    std::string path = CFinalSunApp::ExePath();
    path += "knowledge\\";
    return path;
}

// ---------------------------------------------------------------------------
// Safe JSON dump - replaces invalid UTF-8 instead of throwing
// ---------------------------------------------------------------------------
static std::string SafeDump(const json& j)
{
    return j.dump(-1, ' ', false, json::error_handler_t::replace);
}

static std::string GetRelativePath(const std::filesystem::path& full, const std::filesystem::path& root)
{
    return std::filesystem::relative(full, root).generic_string();
}

// Helper : read an INI section as a raw string
// ---------------------------------------------------------------------------
static std::string GetIniSection(const std::string& section)
{
    auto& pMap = CINI::CurrentDocument;

	auto pSection = pMap->GetSection(section.c_str());
	if (!pSection) return "Section \"" + section + "\" not found or empty.";

    std::string result;
    result += "[" + section + "]\r\n";
    for (auto& [key, value] : pSection->GetEntities())
    {
        result += std::string(key) + "=" + value.GetString() + "\r\n";
    }
    return result;
}

// ---------------------------------------------------------------------------
// Encoding helpers: convert between external (UTF-8) and internal (ANSI) encoding
// ---------------------------------------------------------------------------

// Convert incoming string (UTF-8 from client) to internal ANSI
static std::string ToInternalEncoding(const std::string& str)
{
    FString fs(str);
    auto encoding = STDHelpers::GetFileEncoding(
        reinterpret_cast<const uint8_t*>(fs.data()), fs.size());
    if (encoding == FileEncoding::UTF8 || encoding == FileEncoding::UTF8_BOM)
        fs.toANSI();
    return std::string(fs);
}

// Convert outgoing string (internal ANSI) to UTF-8 for client
static std::string ToExternalEncoding(const std::string& str)
{
    FString fs(str);
    auto encoding = STDHelpers::GetFileEncoding(
        reinterpret_cast<const uint8_t*>(fs.data()), fs.size());
    if (encoding == FileEncoding::ANSI)
        fs.toUTF8();
    return std::string(fs);
}

// ===================================================================
// Public interface
// ===================================================================

static json ProcessRequest(json& request)
{
    std::string method = request.value("method", "");
    json id = request.value("id", json(nullptr));

    // notifications have no id -> no response
    if (id.is_null()) return json::object();

    json response = { {"jsonrpc", "2.0"}, {"id", id} };

    // ----- initialize -----
    if (method == "initialize")
    {
        response["result"] = {
            {"protocolVersion", "2024-11-05"},
            {"capabilities", {{"tools", json::object()}}},
            {"serverInfo", {{"name", "FA2sp-MCP"}, {"version", "1.0.0"}}}
        };
    }
    // ----- tools/list -----
    else if (method == "tools/list")
    {
        json tools = json::array();

		tools.push_back({{"name", "run_lua"},
						 {"description", "Execute Lua script in the FA2 map editor. Automates "
										 "map editing - add/remove units, buildings, terrain; modify triggers, "
										 "scripts, teams; adjust map data; read/write INI sections."},
						 {"inputSchema", {{"type", "object"}, {"properties", {{"script", {{"type", "string"}, {"description", "Lua code to execute. Full Lua API documented via get_document."}}}}}, {"required", json::array({"script"})}}}});

		tools.push_back({
            {"name", "list_knowledge"},
            {"description", "List all available knowledge documents. Returns document names that can be used as input to get_knowledge."},
            {"inputSchema", {
                {"type", "object"},
                {"properties", json::object()},
                {"required", json::array()}
            }}
        });

        tools.push_back({
            {"name", "get_knowledge"},
            {"description", "Retrieve a specific knowledge document by its name (returned by list_knowledge)."},
            {"inputSchema", {
                {"type", "object"},
                {"properties", {
                    {"key", {
                        {"type", "string"},
                        {"description", "Document name as returned by list_knowledge (e.g. 'api\\lua api document.md')."}
                    }}
                }},
                {"required", json::array({"key"})}
            }}
        });

        response["result"] = {{"tools", tools}};
    }
    // ----- tools/call -----
    else if (method == "tools/call")
    {
        json params = request.value("params", json::object());
        std::string toolName = params.value("name", "");
        json arguments = params.value("arguments", json::object());

        if (toolName == "run_lua")
        {
            MCPRequest* mcpReq = new MCPRequest();
            mcpReq->type = 0;
            mcpReq->input = ToInternalEncoding(arguments.value("script", ""));
            mcpReq->hEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
            PostMessage(CFinalSunDlg::Instance->GetSafeHwnd(), WM_MCP_RUN_LUA, 0, (LPARAM)mcpReq);
            WaitForSingleObject(mcpReq->hEvent, INFINITE);
            std::string out = ToExternalEncoding(mcpReq->result);
            CloseHandle(mcpReq->hEvent); delete mcpReq;
            response["result"] = {{"content", json::array({{{"type", "text"}, {"text", out}}})}};
        }
        else if (toolName == "list_knowledge")
        {
            MCPRequest* mcpReq = new MCPRequest();
            mcpReq->type = 3;
            mcpReq->hEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
            PostMessage(CFinalSunDlg::Instance->GetSafeHwnd(), WM_MCP_LIST_KNOWLEDGE, 0, (LPARAM)mcpReq);
            WaitForSingleObject(mcpReq->hEvent, INFINITE);
            std::string out = ToExternalEncoding(mcpReq->result);
            CloseHandle(mcpReq->hEvent); delete mcpReq;
            response["result"] = {{"content", json::array({{{"type", "text"}, {"text", out}}})}};
        }
        else if (toolName == "get_knowledge")
        {
            std::string key = arguments.value("key", "");
            if (key.empty())
            {
                response["result"] = {{"content", json::array({{{"type", "text"}, {"text", "Error: 'key' parameter is required."}}})}};
            }
            else
            {
                MCPRequest* mcpReq = new MCPRequest();
                mcpReq->type = 4;
                mcpReq->input = ToInternalEncoding(key);
                mcpReq->hEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
                PostMessage(CFinalSunDlg::Instance->GetSafeHwnd(), WM_MCP_GET_KNOWLEDGE, 0, (LPARAM)mcpReq);
                WaitForSingleObject(mcpReq->hEvent, INFINITE);
                std::string out = ToExternalEncoding(mcpReq->result);
                CloseHandle(mcpReq->hEvent); delete mcpReq;
                response["result"] = {{"content", json::array({{{"type", "text"}, {"text", out}}})}};
            }
        }
        else
        {
            response["error"] = {{"code", -32601}, {"message", "Method not found: " + toolName}};
        }
    }
    else
    {
        response["error"] = {{"code", -32601}, {"message", "Method not found: " + method}};
    }

    return response;
}

void CMcpServer::Start(int port)
{
    if (m_running) Stop();

    m_port = port;
    auto* svr = new httplib::Server();
    m_server = svr;

    // Single-threaded ！ avoids httplib thread-pool header corruption
    svr->new_task_queue = [] { return new httplib::ThreadPool(1); };
    svr->set_keep_alive_max_count(1);

    svr->set_tcp_nodelay(true);
    svr->set_write_timeout(30, 0);
    svr->set_read_timeout(10, 0);

    // ---------- GET / ！ SSE endpoint for Streamable HTTP ----------
    svr->Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Cache-Control", "no-cache");
        res.set_header("Content-Type", "text/event-stream");
        res.set_content(
            "event: endpoint\r\n"
            "data: /\r\n"
            "\r\n",
            "text/event-stream");
    });

    // ---------- POST / ！ JSON-RPC ----------
    svr->Post("/", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");

        json request;
        try { request = json::parse(req.body); }
        catch (...) {
            res.status = 400;
            res.set_content(
                R"({"jsonrpc":"2.0","id":null,"error":{"code":-32700,"message":"Parse error"}})",
                "application/json");
            return;
        }

        json response = ProcessRequest(request);

        if (response.empty()) {
            res.status = 202;
        } else {
            // Use raw string to avoid any header-setting race
            std::string body = SafeDump(response);
            res.status = 200;
            res.set_header("Content-Type", "application/json");
            res.set_content(body, "application/json");
        }
    });

    // ---------- OPTIONS ！ CORS preflight ----------
    svr->Options("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Mcp-Session-Id");
        res.set_header("Access-Control-Max-Age", "86400");
        res.status = 204;
    });

    // Start listening on a background thread
    std::thread([svr, port]() {
        svr->listen("127.0.0.1", port);
    }).detach();

    m_running = true;

    Logger::Info("MCP Server started at http://127.0.0.1:%d/\n", port);
}

void CMcpServer::Stop()
{
    if (!m_running) return;
    auto* svr = static_cast<httplib::Server*>(m_server);
    svr->stop();
    delete svr;
    m_server = nullptr;
    m_running = false;
    Logger::Info("MCP Server stopped.\n");
}

bool CMcpServer::IsRunning()
{
    return m_running;
}

// ===================================================================
// Main-thread handlers (called from CFinalSunDlgExt::PreTranslateMessageExt)
// ===================================================================

void CMcpServer::HandleRunLua(MCPRequest* req)
{
    CLuaConsole::mcpRunning = true;
    CLuaConsole::mcpOutput.clear();

    // Reuse the Lua state exposed by CLuaConsole
    auto& Lua = CLuaConsole::Lua;
    CLuaConsole::skipBuildingUpdate = true;

    {
        VEHGuard guard(false);  // disable VEH during Lua execution
        try
        {
            sol::protected_function_result result =
                Lua.script(req->input, sol::script_pass_on_error);

            if (!result.valid())
            {
                sol::error err = result;
                std::string errStr = err.what();
                std::string errorMessage = "Lua Error: " + errStr;
                // Check for __SCRIPT_ABORT__
                if (errStr.find("__SCRIPT_ABORT__") != std::string::npos)
                {
                    CLuaConsole::mcpOutput += "Script aborted.\r\n";
                }
                else
                {
                    sol::call_status status = result.status();
                    switch (status)
                    {
                    case sol::call_status::syntax:   errorMessage += " (Syntax Error)"; break;
                    case sol::call_status::runtime:  errorMessage += " (Runtime Error)"; break;
                    case sol::call_status::memory:   errorMessage += " (Memory Error)"; break;
                    case sol::call_status::handler:  errorMessage += " (Handler Error)"; break;
                    case sol::call_status::gc:       errorMessage += " (GC Error)"; break;
                    case sol::call_status::file:     errorMessage += " (File Error)"; break;
                    default:                         errorMessage += " (Unknown Error)"; break;
                    }
                    CLuaConsole::mcpOutput += errorMessage + "\r\n";
                }
            }
            else
            {
                if (CLuaConsole::mcpOutput.empty())
                    CLuaConsole::mcpOutput += "Script executed successfully.\r\n";
            }
        }
        catch (const std::exception& e)
        {
            CLuaConsole::mcpOutput += std::string("Critical Error: ") + e.what() + "\r\n";
        }
        catch (...)
        {
            CLuaConsole::mcpOutput += "Critical Error: Unknown exception occurred.\r\n";
        }
    }

    CLuaConsole::skipBuildingUpdate = false;

    // ---- redraw logic (same as OnClickRun) ----
    #define REDRAW_IF(flag, body) \
        if (CLuaConsole::flag) { \
            CLuaConsole::flag = false; \
            body; \
        }

    REDRAW_IF(updateBuilding, CMapDataExt::UpdateFieldStructureData_RedrawMinimap());
    REDRAW_IF(updateUnit,      CMapDataExt::UpdateFieldUnitData_RedrawMinimap());
    REDRAW_IF(updateInfantry,  CMapDataExt::UpdateFieldInfantryData_RedrawMinimap());
    REDRAW_IF(updateAircraft,  CMapDataExt::UpdateFieldAircraftData_RedrawMinimap());
    REDRAW_IF(updateNode,      CMapData::Instance->UpdateFieldBasenodeData(FALSE));
    REDRAW_IF(updateCellTag,   CMapData::Instance->UpdateFieldCelltagData(FALSE));

    REDRAW_IF(updateTrigger,   {
        bool noEditor = true;
        for (int i = 0; i < TRIGGER_EDITOR_MAX_COUNT; ++i)
            if (CNewTrigger::Instance[i].GetHandle())
            { noEditor = false; ::SendMessage(CNewTrigger::Instance[i].GetHandle(), 114514, 0, 0); }
        if (noEditor) CMapDataExt::UpdateTriggers();
    });

    REDRAW_IF(updateAITrigger, { if (CNewAITrigger::GetHandle()) ::SendMessage(CNewAITrigger::GetHandle(), 114514, 0, 0); });
    REDRAW_IF(updateScript,    { if (CNewScript::GetHandle())    ::SendMessage(CNewScript::GetHandle(), 114514, 0, 0); });
    REDRAW_IF(updateTeam,      { if (CNewTeamTypes::GetHandle())  ::SendMessage(CNewTeamTypes::GetHandle(), 114514, 0, 0); });
    REDRAW_IF(updateTaskforce, { if (CNewTaskforce::GetHandle())  ::SendMessage(CNewTaskforce::GetHandle(), 114514, 0, 0); });
    REDRAW_IF(updateVariable,  { if (CNewLocalVariables::GetHandle()) ::SendMessage(CNewLocalVariables::GetHandle(), 114514, 0, 0); });

    REDRAW_IF(updateMinimap,   {
        for (int x = 0; x < CMapData::Instance->MapWidthPlusHeight; ++x)
        {
            for (int y = 0; y < CMapData::Instance->MapWidthPlusHeight; ++y)
            {
                if (CMapData::Instance->IsCoordInMap(x, y))
                {
                    CMapData::Instance->UpdateMapPreviewAt(x, y);
                }
            }
        }
		::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.Minimap.m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    });

    if (CLuaConsole::recalculateOre)
    {
        auto pExt = CMapDataExt::GetExtension();
        pExt->MoneyCount = 0;
        for (int x = 0; x < pExt->MapWidthPlusHeight; ++x)
            for (int y = 0; y < pExt->MapWidthPlusHeight; ++y)
                if (pExt->IsCoordInMap(x, y))
                {
                    auto cell = pExt->GetCellAt(x, y);
                    if (pExt->IsOre(cell->Overlay))
                        pExt->MoneyCount += pExt->GetOreValue(cell->Overlay, cell->OverlayData);
                }
        CLuaConsole::recalculateOre = false;
    }

    if (CLuaConsole::needRedraw)
    {
        if (CFinalSunDlg::Instance->MyViewFrame.pIsoView)
            ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd,
                           nullptr, nullptr, RDW_UPDATENOW | RDW_INVALIDATE);
        CLuaConsole::needRedraw = false;
    }

    CMapDataExt::DeleteBuildingByIniID = false;
    #undef REDRAW_IF

    // Build result
    req->result = std::move(CLuaConsole::mcpOutput);
    CLuaConsole::mcpRunning = false;
    CLuaConsole::mcpOutput.clear();

    SetEvent(req->hEvent);
}

void CMcpServer::HandleListKnowledge(MCPRequest* req)
{
    std::string docRoot = GetDocRoot();
    json docArray = json::array();

    if (std::filesystem::exists(docRoot))
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(docRoot))
        {
            if (entry.is_regular_file())
            {
                docArray.push_back(GetRelativePath(entry.path(), docRoot));
            }
        }
    }

    req->result = SafeDump(docArray);
    SetEvent(req->hEvent);
}

void CMcpServer::HandleGetKnowledge(MCPRequest* req)
{
    std::string docRoot = GetDocRoot();
    std::filesystem::path fullPath = std::filesystem::path(docRoot) / req->input;

    // Security: ensure the resolved path is still under docRoot
    try {
        auto canonical = std::filesystem::weakly_canonical(fullPath);
        auto rootCanon = std::filesystem::weakly_canonical(docRoot);
        auto [rootEnd, _] = std::mismatch(rootCanon.begin(), rootCanon.end(), canonical.begin());
        if (rootEnd != rootCanon.end())
        {
            req->result = "Error: Invalid path \"" + req->input + "\".";
            SetEvent(req->hEvent);
            return;
        }
    } catch (...) {
        req->result = "Error: Cannot resolve path \"" + req->input + "\".";
        SetEvent(req->hEvent);
        return;
    }

    if (!std::filesystem::exists(fullPath) || !std::filesystem::is_regular_file(fullPath))
    {
        req->result = "Error: Document \"" + req->input + "\" not found.";
        SetEvent(req->hEvent);
        return;
    }

    std::ifstream ifs(fullPath);
    if (!ifs)
    {
        req->result = "Error: Cannot open file \"" + req->input + "\".";
        SetEvent(req->hEvent);
        return;
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    req->result = oss.str();
    SetEvent(req->hEvent);
}