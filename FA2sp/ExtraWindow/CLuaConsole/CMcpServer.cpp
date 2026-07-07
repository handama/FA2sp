#include "CMcpServer.h"
#include "CLuaConsole.h"
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
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"

using json = nlohmann::json;

// Static members
bool CMcpServer::m_running = false;
int CMcpServer::m_port = 19198;
void* CMcpServer::m_server = nullptr;
CSearchIndex CMcpServer::m_searchIndex;

// ---------------------------------------------------------------------------
// Helper : read an INI section as a raw string
// ---------------------------------------------------------------------------
static std::string GetDocRoot()
{
    std::string path = CFinalSunApp::ExePath();
    path += "mcp\\knowledge\\";
    return path;
}

static std::string GetPromptRoot()
{
    std::string path = CFinalSunApp::ExePath();
    path += "mcp\\prompts\\";
    return path;
}

static std::string GetSkillRoot()
{
    std::string path = CFinalSunApp::ExePath();
    path += "mcp\\skills\\";
    return path;
}

static std::string GetScriptRoot()
{
    std::string path = CFinalSunApp::ExePath();
    path += "Scripts\\";
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
            {"capabilities", {
                {"tools", json::object()},
                {"prompts", json::object()}
            }},
            {"serverInfo", {{"name", "FA2sp-MCP"}, {"version", "1.0.0"}}}
        };
    }
    // ----- tools/list -----
    else if (method == "tools/list")
    {
        json tools = json::array();

        tools.push_back({
            {"name", "list_skills"},
            {"description", "List all available skill documents. You should ALWAYS use this tool first "
                            "to discover relevant skills before attempting any task. Returns document names "
                            "that can be used as input to get_skill. You should always check 'map_editing_basic_principles.md'."},
            {"inputSchema", {
                {"type", "object"},
                {"properties", json::object()},
                {"required", json::array()}
            }}
        });

        tools.push_back({
            {"name", "get_skill"},
            {"description", "Retrieve a specific skill document by its name (returned by list_skills)."},
            {"inputSchema", {
                {"type", "object"},
                {"properties", {
                    {"key", {
                        {"type", "string"},
                        {"description", "Skill document name as returned by list_skills (e.g. 'my_skill.md')."}
                    }}
                }},
                {"required", json::array({"key"})}
            }}
        });

		tools.push_back({{"name", "run_lua"},
						 {"description", "Execute Lua script in the FA2 map editor. Automates "
										 "map editing - add/remove units, buildings, terrain; modify triggers, "
										 "scripts, teams; adjust map data; read/write INI sections."},
						 {"inputSchema", {{"type", "object"},
							 {"properties", {
								 {"script", {{"type", "string"}, {"description", "Lua code to execute. Supports standard Lua functions and extended API functions. Use list_knowledge to discover available documentation, then use get_knowledge to retrieve the relevant documents and obtain the list of supported functions and their usage details."}}},
								 {"track_ini_changes", {{"type", "boolean"}, {"description", "Optional. If true, the map INI state will be captured before and after script execution, and the changes will be reported in the result. Default: false."}}}
							 }},
							 {"required", json::array({"script"})}}}});

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
                        {"description", "Document name as returned by list_knowledge (e.g. 'api\\00_conventions.md')."}
                    }}
                }},
                {"required", json::array({"key"})}
            }}
        });

        tools.push_back({
            {"name", "search_knowledge"},
            {"description", "Search knowledge documents using TF-IDF keyword matching. "
                            "Returns ranked results with relevance scores. Best for finding specific "
                            "APIs, examples, or concepts across all knowledge files."},
            {"inputSchema", {
                {"type", "object"},
                {"properties", {
                    {"query", {
                        {"type", "string"},
                        {"description", "Search query string. Supports Chinese and English keywords. "
                                        "Example: 'rhino tank team' or 'create_taskforce'"}
                    }}
                }},
                {"required", json::array({"query"})}
            }}
        });

        tools.push_back({
            {"name", "list_scripts"},
            {"description", "List all available Lua script files (.lua) in the Scripts directory. "
                            "Returns filenames that can be used as input to get_script. "
                            "When you are unsure about certain implementations, you can refer to "
                            "existing relevant scripts as reference."},
            {"inputSchema", {
                {"type", "object"},
                {"properties", json::object()},
                {"required", json::array()}
            }}
        });

        tools.push_back({
            {"name", "get_script"},
            {"description", "Retrieve a specific Lua script file by its filename (returned by list_scripts)."},
            {"inputSchema", {
                {"type", "object"},
                {"properties", {
                    {"key", {
                        {"type", "string"},
                        {"description", "Script filename as returned by list_scripts (e.g. 'my_script.lua')."}
                    }}
                }},
                {"required", json::array({"key"})}
            }}
        });

        tools.push_back({
            {"name", "save_script"},
            {"description", "Save a Lua script to the Scripts directory. "
                            "IMPORTANT: Only use this tool when the user explicitly requests to save or "
                            "store a script. Do NOT use it automatically or without user confirmation. "
                            "Add a comment at the beginning of the file describing the script's purpose."},
            {"inputSchema", {
                {"type", "object"},
                {"properties", {
                    {"key", {
                        {"type", "string"},
                        {"description", "Filename to save as (e.g. 'my_script.lua'). Must end with .lua."}
                    }},
                    {"content", {
                        {"type", "string"},
                        {"description", "The Lua script content to save."}
                    }}
                }},
                {"required", json::array({"key", "content"})}
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
            mcpReq->trackIniChanges = arguments.value("track_ini_changes", false);
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
            std::string out = mcpReq->result;
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
        else if (toolName == "search_knowledge")
        {
            std::string query = arguments.value("query", "");
            if (query.empty())
            {
                response["result"] = {{"content", json::array({{{"type", "text"}, {"text", "Error: 'query' parameter is required."}}})}};
            }
            else
            {
                MCPRequest* mcpReq = new MCPRequest();
                mcpReq->type = 5;
                mcpReq->input = ToInternalEncoding(query);
                mcpReq->hEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
                PostMessage(CFinalSunDlg::Instance->GetSafeHwnd(), WM_MCP_SEARCH_KNOWLEDGE, 0, (LPARAM)mcpReq);
                WaitForSingleObject(mcpReq->hEvent, INFINITE);
                std::string out = ToExternalEncoding(mcpReq->result);
                CloseHandle(mcpReq->hEvent); delete mcpReq;
                response["result"] = {{"content", json::array({{{"type", "text"}, {"text", out}}})}};
            }
        }
        else if (toolName == "list_skills")
        {
            MCPRequest* mcpReq = new MCPRequest();
            mcpReq->type = 6;
            mcpReq->hEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
            PostMessage(CFinalSunDlg::Instance->GetSafeHwnd(), WM_MCP_LIST_SKILL, 0, (LPARAM)mcpReq);
            WaitForSingleObject(mcpReq->hEvent, INFINITE);
            std::string out = mcpReq->result;
            CloseHandle(mcpReq->hEvent); delete mcpReq;
            response["result"] = {{"content", json::array({{{"type", "text"}, {"text", out}}})}};
        }
        else if (toolName == "get_skill")
        {
            std::string key = arguments.value("key", "");
            if (key.empty())
            {
                response["result"] = {{"content", json::array({{{"type", "text"}, {"text", "Error: 'key' parameter is required."}}})}};
            }
            else
            {
                MCPRequest* mcpReq = new MCPRequest();
                mcpReq->type = 7;
                mcpReq->input = ToInternalEncoding(key);
                mcpReq->hEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
                PostMessage(CFinalSunDlg::Instance->GetSafeHwnd(), WM_MCP_GET_SKILL, 0, (LPARAM)mcpReq);
                WaitForSingleObject(mcpReq->hEvent, INFINITE);
                std::string out = ToExternalEncoding(mcpReq->result);
                CloseHandle(mcpReq->hEvent); delete mcpReq;
                response["result"] = {{"content", json::array({{{"type", "text"}, {"text", out}}})}};
            }
        }
        else if (toolName == "list_scripts")
        {
            MCPRequest* mcpReq = new MCPRequest();
            mcpReq->type = 8;
            mcpReq->hEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
            PostMessage(CFinalSunDlg::Instance->GetSafeHwnd(), WM_MCP_LIST_SCRIPTS, 0, (LPARAM)mcpReq);
            WaitForSingleObject(mcpReq->hEvent, INFINITE);
            std::string out = mcpReq->result;
            CloseHandle(mcpReq->hEvent); delete mcpReq;
            response["result"] = {{"content", json::array({{{"type", "text"}, {"text", out}}})}};
        }
        else if (toolName == "get_script")
        {
            std::string key = arguments.value("key", "");
            if (key.empty())
            {
                response["result"] = {{"content", json::array({{{"type", "text"}, {"text", "Error: 'key' parameter is required."}}})}};
            }
            else
            {
                MCPRequest* mcpReq = new MCPRequest();
                mcpReq->type = 9;
                mcpReq->input = ToInternalEncoding(key);
                mcpReq->hEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
                PostMessage(CFinalSunDlg::Instance->GetSafeHwnd(), WM_MCP_GET_SCRIPT, 0, (LPARAM)mcpReq);
                WaitForSingleObject(mcpReq->hEvent, INFINITE);
                std::string out = ToExternalEncoding(mcpReq->result);
                CloseHandle(mcpReq->hEvent); delete mcpReq;
                response["result"] = {{"content", json::array({{{"type", "text"}, {"text", out}}})}};
            }
        }
        else if (toolName == "save_script")
        {
            std::string key = arguments.value("key", "");
            std::string content = arguments.value("content", "");
            if (key.empty())
            {
                response["result"] = {{"content", json::array({{{"type", "text"}, {"text", "Error: 'key' parameter is required."}}})}};
            }
            else if (content.empty())
            {
                response["result"] = {{"content", json::array({{{"type", "text"}, {"text", "Error: 'content' parameter is required."}}})}};
            }
            else
            {
                MCPRequest* mcpReq = new MCPRequest();
                mcpReq->type = 10;
                mcpReq->input = ToInternalEncoding(key + "\n" + content);
                mcpReq->hEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
                PostMessage(CFinalSunDlg::Instance->GetSafeHwnd(), WM_MCP_SAVE_SCRIPT, 0, (LPARAM)mcpReq);
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
    // ----- prompts/list -----
    else if (method == "prompts/list")
    {
        std::string promptRoot = GetPromptRoot();
        json promptsList = json::array();

        if (std::filesystem::exists(promptRoot))
        {
            for (const auto& entry : std::filesystem::directory_iterator(promptRoot))
            {
                if (!entry.is_regular_file()) continue;
                auto ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext != ".md") continue;

                std::string name = ToExternalEncoding(entry.path().stem().string());
                std::string description = name;

                // Read first non-empty, non-heading line as description
                std::ifstream ifs(entry.path());
                if (ifs)
                {
                    std::string line;
                    while (std::getline(ifs, line))
                    {
                        if (!line.empty() && line.back() == '\r') line.pop_back();
                        if (!line.empty() && line[0] != '#')
                        {
                            description = ToExternalEncoding(line);
                            break;
                        }
                    }
                }

                promptsList.push_back({{"name", name}, {"description", description}});
            }
        }

        response["result"] = {{"prompts", promptsList}};
    }
    // ----- prompts/get -----
    else if (method == "prompts/get")
    {
        json params = request.value("params", json::object());
        std::string name = params.value("name", "");

        std::string promptRoot = GetPromptRoot();
        std::filesystem::path fullPath = std::filesystem::path(promptRoot) / (name + ".md");

        // Security: ensure the resolved path is still under promptRoot
        bool valid = true;
        try {
            auto canonical = std::filesystem::weakly_canonical(fullPath);
            auto rootCanon = std::filesystem::weakly_canonical(promptRoot);
            auto [rootEnd, _] = std::mismatch(rootCanon.begin(), rootCanon.end(), canonical.begin());
            if (rootEnd != rootCanon.end()) valid = false;
        } catch (...) { valid = false; }

        if (!valid || !std::filesystem::exists(fullPath) || !std::filesystem::is_regular_file(fullPath))
        {
            response["error"] = {{"code", -32602}, {"message", "Prompt not found: " + name}};
        }
        else
        {
            std::ifstream ifs(fullPath);
            if (!ifs)
            {
                response["error"] = {{"code", -32602}, {"message", "Cannot open prompt: " + name}};
            }
            else
            {
                std::ostringstream oss;
                oss << ifs.rdbuf();
                // Detect encoding and convert to UTF-8 for AI consumption
                std::string content = ToExternalEncoding(oss.str());

                response["result"] = {
                    {"description", "Prompt: " + name},
                    {"messages", json::array({
                        json::object({
                            {"role", "user"},
                            {"content", {
                                {"type", "text"},
                                {"text", content}
                            }}
                        })
                    })}
                };
            }
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

    // Single-threaded - avoids httplib thread-pool header corruption
    svr->new_task_queue = [] { return new httplib::ThreadPool(1); };
    svr->set_keep_alive_max_count(1);

    svr->set_tcp_nodelay(true);
    svr->set_write_timeout(30, 0);
    svr->set_read_timeout(10, 0);

    // ---------- GET / - SSE endpoint for Streamable HTTP ----------
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

    // ---------- POST / - JSON-RPC ----------
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

    // ---------- OPTIONS - CORS preflight ----------
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

    // Build search index (on the calling thread, before returning)
    BuildSearchIndex();

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
// INI state capture and diff (shells - implement internals yourself)
// ===================================================================

// Capture the current map INI state as a string.
// Return format: raw INI text (e.g. "[Section]\r\nKey=Value\r\n...")
// Return empty string on failure.
static std::string CaptureMapIniState()
{
	std::ostringstream oss;
    for (auto& section : CINI::CurrentDocument->Dict)
    {
        if (!strcmp(section.first, "Preview")
            || !strcmp(section.first, "PreviewPack")
        )
            continue;

        oss << "[" << section.first << "]" << "\n";
        for (const auto& pair : section.second.GetEntities())
        {
            oss << pair.first << "=" << pair.second << "\n";
        }
    }
	return oss.str();
}

// Parse raw INI text into a map: section -> (key -> value)
static std::map<std::string, std::map<std::string, std::string>> ParseIniText(const std::string& text)
{
    std::map<std::string, std::map<std::string, std::string>> ini;
    std::string currentSection;
    std::istringstream stream(text);
    std::string line;

    while (std::getline(stream, line))
    {
        // Strip \r
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        // Skip empty lines
        if (line.empty())
            continue;

        // Section header
        if (line.front() == '[' && line.back() == ']')
        {
            currentSection = line.substr(1, line.size() - 2);
            // Ensure the section exists in the map
            ini[currentSection];
            continue;
        }

        // Key=Value
        auto eq = line.find('=');
        if (eq != std::string::npos && !currentSection.empty())
        {
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            ini[currentSection][key] = value;
        }
    }

    return ini;
}

// Compute a human-readable diff between two captured INI states.
// before/after are strings returned by CaptureMapIniState().
// Return a human-readable summary of changes (added/modified/removed keys).
// Return empty string if no changes detected.
static std::string ComputeIniDiff(const std::string& before, const std::string& after)
{
    auto beforeMap = ParseIniText(before);
    auto afterMap = ParseIniText(after);

    std::string result;

    // Collect all section names
    std::set<std::string> allSections;
    for (const auto& [s, _] : beforeMap) allSections.insert(s);
    for (const auto& [s, _] : afterMap)  allSections.insert(s);

    for (const auto& section : allSections)
    {
        bool inBefore = beforeMap.count(section) > 0;
        bool inAfter  = afterMap.count(section) > 0;

        if (!inBefore && inAfter)
        {
            // Section added
            result += "[+" + section + "]\n";
            for (const auto& [k, v] : afterMap.at(section))
                result += "  + " + k + "=" + v + "\n";
        }
        else if (inBefore && !inAfter)
        {
            // Section removed
            result += "[-" + section + "]\n";
            for (const auto& [k, v] : beforeMap.at(section))
                result += "  - " + k + "=" + v + "\n";
        }
        else
        {
            // Section exists in both - compare keys
            const auto& beforeKeys = beforeMap.at(section);
            const auto& afterKeys  = afterMap.at(section);

            // Collect all key names
            std::set<std::string> allKeys;
            for (const auto& [k, _] : beforeKeys) allKeys.insert(k);
            for (const auto& [k, _] : afterKeys)  allKeys.insert(k);

            bool sectionChanged = false;
            std::string sectionDiff;

            for (const auto& key : allKeys)
            {
                bool keyInBefore = beforeKeys.count(key) > 0;
                bool keyInAfter  = afterKeys.count(key) > 0;

                if (!keyInBefore && keyInAfter)
                {
                    // Key added
                    sectionChanged = true;
                    sectionDiff += "  + " + key + "=" + afterKeys.at(key) + "\n";
                }
                else if (keyInBefore && !keyInAfter)
                {
                    // Key removed
                    sectionChanged = true;
                    sectionDiff += "  - " + key + "=" + beforeKeys.at(key) + "\n";
                }
                else if (beforeKeys.at(key) != afterKeys.at(key))
                {
                    // Key modified
                    sectionChanged = true;
                    sectionDiff += "  ~ " + key + ": \"" + beforeKeys.at(key) + "\" -> \"" + afterKeys.at(key) + "\"\n";
                }
            }

            if (sectionChanged)
            {
                result += "[~" + section + "]\n";
                result += sectionDiff;
            }
        }
    }

    // Trim trailing newline
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
        result.pop_back();

    return result;
}

// ===================================================================
// Main-thread handlers (called from CFinalSunDlgExt::PreTranslateMessageExt)
// ===================================================================

void CMcpServer::HandleRunLua(MCPRequest* req)
{
    CLuaConsole::mcpRunning = true;
    CLuaConsole::mcpOutput.clear();

    // Capture INI state before script execution (if tracking enabled)
    std::string iniBefore;
    if (req->trackIniChanges)
    {
        iniBefore = CaptureMapIniState();
    }

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

    // Capture INI state after script and compute diff (if tracking enabled)
    if (req->trackIniChanges)
    {
        std::string iniAfter = CaptureMapIniState();
        std::string iniDiff = ComputeIniDiff(iniBefore, iniAfter);
        if (!iniDiff.empty())
        {
            CLuaConsole::mcpOutput += "\r\n--- INI Changes ---\r\n";
            CLuaConsole::mcpOutput += iniDiff;
            CLuaConsole::mcpOutput += "\r\n--- End of INI Changes ---\r\n";
        }
        else
        {
            CLuaConsole::mcpOutput += "\r\n--- No INI changes detected. ---\r\n";
        }
    }

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
                docArray.push_back(ToExternalEncoding(GetRelativePath(entry.path(), docRoot)));
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

void CMcpServer::BuildSearchIndex()
{
    m_searchIndex.Build(GetDocRoot());
}

void CMcpServer::HandleSearchKnowledge(MCPRequest* req)
{
    auto results = m_searchIndex.Search(req->input, 5);

    // Convert query to UTF-8 for JSON output
    std::string queryUtf8 = ToExternalEncoding(req->input);

    json j;
    j["query"] = queryUtf8;
    j["too_many"] = false;

    json resultsArray = json::array();
    for (const auto& r : results)
    {
        json item;
        item["source"]    = ToExternalEncoding(r.source);
        item["title"]     = ToExternalEncoding(r.title);
        item["section"]   = ToExternalEncoding(r.section);
        item["relevance"] = r.relevance;
        item["content"]   = ToExternalEncoding(r.content);
        resultsArray.push_back(item);
    }
    j["results"] = resultsArray;

    req->result = SafeDump(j);
    SetEvent(req->hEvent);
}

void CMcpServer::HandleListSkill(MCPRequest* req)
{
    std::string skillRoot = GetSkillRoot();
    json docArray = json::array();

    if (std::filesystem::exists(skillRoot))
    {
        // Non-recursive: only list files directly in the skill root
        for (const auto& entry : std::filesystem::directory_iterator(skillRoot))
        {
            if (entry.is_regular_file())
            {
                docArray.push_back(ToExternalEncoding(entry.path().filename().string()));
            }
        }
    }

    req->result = SafeDump(docArray);
    SetEvent(req->hEvent);
}

void CMcpServer::HandleGetSkill(MCPRequest* req)
{
    std::string skillRoot = GetSkillRoot();
    std::filesystem::path fullPath = std::filesystem::path(skillRoot) / req->input;

    // Security: ensure the resolved path is still under skillRoot
    try {
        auto canonical = std::filesystem::weakly_canonical(fullPath);
        auto rootCanon = std::filesystem::weakly_canonical(skillRoot);
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

// ===================================================================
// Script handlers (called from main thread)
// ===================================================================

void CMcpServer::HandleListScripts(MCPRequest* req)
{
    std::string scriptRoot = GetScriptRoot();
    json fileArray = json::array();

    if (std::filesystem::exists(scriptRoot))
    {
        for (const auto& entry : std::filesystem::directory_iterator(scriptRoot))
        {
            if (!entry.is_regular_file()) continue;
            auto ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext != ".lua") continue;
            fileArray.push_back(ToExternalEncoding(entry.path().filename().string()));  
        }
    }

    req->result = SafeDump(fileArray);
    SetEvent(req->hEvent);
}

void CMcpServer::HandleGetScript(MCPRequest* req)
{
    std::string scriptRoot = GetScriptRoot();
    std::filesystem::path fullPath = std::filesystem::path(scriptRoot) / req->input;

    // Security: ensure the resolved path is still under scriptRoot
    try {
        auto canonical = std::filesystem::weakly_canonical(fullPath);
        auto rootCanon = std::filesystem::weakly_canonical(scriptRoot);
        auto [rootEnd, _] = std::mismatch(rootCanon.begin(), rootCanon.end(), canonical.begin());
        if (rootEnd != rootCanon.end())
        {
            req->result = "Error: Invalid path.";
            SetEvent(req->hEvent);
            return;
        }
    } catch (...) {
        req->result = "Error: Cannot resolve path.";
        SetEvent(req->hEvent);
        return;
    }

    if (!std::filesystem::exists(fullPath) || !std::filesystem::is_regular_file(fullPath))
    {
        req->result = "Error: Script not found.";
        SetEvent(req->hEvent);
        return;
    }

    std::ifstream ifs(fullPath);
    if (!ifs)
    {
        req->result = "Error: Cannot open file.";
        SetEvent(req->hEvent);
        return;
    }
    std::ostringstream oss;
    oss << ifs.rdbuf();
    req->result = oss.str();
    SetEvent(req->hEvent);
}

void CMcpServer::HandleSaveScript(MCPRequest* req)
{
    std::string scriptRoot = GetScriptRoot();

    // req->input contains "key\ncontent"
    auto delim = req->input.find('\n');
    if (delim == std::string::npos)
    {
        req->result = "Error: Invalid request format.";
        SetEvent(req->hEvent);
        return;
    }

    std::string key = req->input.substr(0, delim);
    std::string content = ToExternalEncoding(req->input.substr(delim + 1)); 
    std::filesystem::path fullPath = std::filesystem::path(scriptRoot) / key;

    // Security: ensure the resolved path is still under scriptRoot
    try {
        auto canonical = std::filesystem::weakly_canonical(fullPath);
        auto rootCanon = std::filesystem::weakly_canonical(scriptRoot);
        auto [rootEnd, _] = std::mismatch(rootCanon.begin(), rootCanon.end(), canonical.begin());
        if (rootEnd != rootCanon.end())
        {
            req->result = "Error: Invalid path.";
            SetEvent(req->hEvent);
            return;
        }
    } catch (...) {
        // Path may not exist yet - that's ok
    }

    // Create Scripts directory if it doesn't exist
    try {
        std::filesystem::create_directories(scriptRoot);
    } catch (...) {
        req->result = "Error: Cannot create Scripts directory.";
        SetEvent(req->hEvent);
        return;
    }

    std::ofstream ofs(fullPath, std::ios::binary);
    if (!ofs)
    {
        req->result = "Error: Cannot write file.";
        SetEvent(req->hEvent);
        return;
    }
    ofs.write(content.data(), content.size());
    ofs.close();

    req->result = "Script saved successfully: " + key;
    SetEvent(req->hEvent);
}


void CSearchIndex::Build(const std::string& docRoot)
{
    Clear();

    if (!std::filesystem::exists(docRoot))
        return;

    int nextId = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(docRoot))
    {
        if (!entry.is_regular_file())
            continue;

        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != ".md")
            continue;

        std::ifstream ifs(entry.path());
        if (!ifs) continue;
        std::ostringstream oss;
        oss << ifs.rdbuf();
        std::string content = oss.str();

        // Convert UTF-8 content to ANSI for internal indexing
        FString fs(content);
        auto encoding = STDHelpers::GetFileEncoding(
            reinterpret_cast<const uint8_t*>(fs.data()), fs.size());
        if (encoding == FileEncoding::UTF8 || encoding == FileEncoding::UTF8_BOM)
            fs.toANSI();
        content = std::string(fs);

        std::string relPath = std::filesystem::relative(entry.path(), docRoot).generic_string();

        auto blocks = SplitMarkdown(content, relPath);
        for (auto& b : blocks)
        {
            b.block_id = nextId++;
            auto tokens = Tokenize(b.content);
            m_block_lengths[b.block_id] = (int)tokens.size();

            // count per-token occurrences in this block
            std::unordered_map<std::string, int> countMap;
            for (const auto& t : tokens)
                countMap[t]++;

            for (auto& [term, cnt] : countMap)
                m_inverted_index[term].push_back({b.block_id, cnt});

            m_blocks.push_back(std::move(b));
        }
    }

    // Compute IDF
    int N = (int)m_blocks.size();
    for (auto& [term, postings] : m_inverted_index)
    {
        int df = (int)postings.size();
        m_idf[term] = std::log((double)N / df);
    }
}

std::vector<SearchResult> CSearchIndex::Search(const std::string& query, int topN) const
{
    if (query.empty() || m_blocks.empty())
        return {};

    auto queryTokens = Tokenize(query);
    if (queryTokens.empty())
        return {};

    // ---- gather candidate block IDs (union) ----
    std::unordered_set<int> candidates;
    for (const auto& t : queryTokens)
    {
        auto it = m_inverted_index.find(t);
        if (it != m_inverted_index.end())
            for (const auto& p : it->second)
                candidates.insert(p.block_id);
    }

    if (candidates.empty())
        return {};

    // ---- compute TF-IDF score for each candidate ----
    struct ScoredBlock { int id; double score; };
    std::vector<ScoredBlock> scored;

    for (int bid : candidates)
    {
        double score = 0.0;
        int blen = m_block_lengths.count(bid) ? m_block_lengths.at(bid) : 1;

        for (const auto& t : queryTokens)
        {
            auto it = m_inverted_index.find(t);
            if (it == m_inverted_index.end()) continue;

            // find term count in this block
            int tf = 0;
            for (const auto& p : it->second)
                if (p.block_id == bid) { tf = p.term_count; break; }

            double idf = m_idf.count(t) ? m_idf.at(t) : 0.0;
            score += ((double)tf / blen) * idf;
        }

        // bonus: heading match
        const auto& blk = m_blocks[bid];
        if (!blk.heading.empty())
        {
            for (const auto& t : queryTokens)
            {
                // case-insensitive search in heading
                std::string hlow = blk.heading;
                std::transform(hlow.begin(), hlow.end(), hlow.begin(), ::tolower);
                if (hlow.find(t) != std::string::npos)
                {
                    score *= 1.5;
                    break; // only apply once
                }
            }
        }

        scored.push_back({bid, score});
    }

    // ---- sort descending ----
    std::sort(scored.begin(), scored.end(),
        [](const ScoredBlock& a, const ScoredBlock& b) { return a.score > b.score; });

    // ---- normalize and build results ----
    double maxScore = scored.empty() ? 1.0 : scored[0].score;
    if (maxScore <= 0.0) maxScore = 1.0;

    std::vector<SearchResult> results;
    int limit = std::min(topN, (int)scored.size());
    for (int i = 0; i < limit; ++i)
    {
        const auto& blk = m_blocks[scored[i].id];
        SearchResult sr;
        sr.source    = blk.file_path;
        sr.section   = blk.heading;

        // derive title from file path: strip extension, replace last / with space
        {
            std::string path = blk.file_path;
            auto dot = path.rfind('.');
            if (dot != std::string::npos) path = path.substr(0, dot);
            auto slash = path.rfind('/');
            if (slash != std::string::npos) path = path.substr(slash + 1);
            sr.title = path;
        }

        sr.relevance = scored[i].score / maxScore;
        sr.content   = blk.content;
        results.push_back(sr);
    }

    return results;
}

void CSearchIndex::Clear()
{
    m_blocks.clear();
    m_inverted_index.clear();
    m_block_lengths.clear();
    m_idf.clear();
}

// ===================================================================
// Block splitting
// ===================================================================

std::vector<SearchBlock> CSearchIndex::SplitMarkdown(
    const std::string& content, const std::string& filePath)
{
    std::vector<SearchBlock> blocks;
    std::string currentHeading;
    std::string buffer;
    bool inCodeBlock = false;

    auto flush = [&]() {
        if (buffer.empty()) return;
        // trim trailing whitespace
        while (!buffer.empty() && (buffer.back() == '\n' || buffer.back() == '\r'))
            buffer.pop_back();
        if (!buffer.empty())
        {
            SearchBlock b;
            b.heading   = currentHeading;
            b.content   = buffer;
            b.file_path = filePath;
            blocks.push_back(std::move(b));
        }
        buffer.clear();
    };

    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line))
    {
        // strip trailing \r (Windows line endings)
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        // code block fence
        if (line.size() >= 3 && line.substr(0, 3) == "```")
        {
            if (inCodeBlock)
            {
                // end of code block
                buffer += line + "\n";
                flush();
                inCodeBlock = false;
            }
            else
            {
                // start of code block: flush pending paragraph first
                flush();
                buffer += line + "\n";
                inCodeBlock = true;
            }
            continue;
        }

        if (inCodeBlock)
        {
            buffer += line + "\n";
            continue;
        }

        // heading line
        if (!line.empty() && line[0] == '#')
        {
            flush();
            currentHeading = line;
            // also emit heading as a block
            SearchBlock hb;
            hb.heading   = currentHeading;
            hb.content   = line;
            hb.file_path = filePath;
            blocks.push_back(std::move(hb));
            continue;
        }

        // empty line: flush paragraph
        if (line.empty())
        {
            flush();
            continue;
        }

        // ordinary text line
        if (!buffer.empty()) buffer += "\n";

        // Long line (>150 chars): flush current buffer, emit this line as its own block
        if (line.size() > 150)
        {
            flush();
            SearchBlock b;
            b.heading   = currentHeading;
            b.content   = line;
            b.file_path = filePath;
            blocks.push_back(std::move(b));
            continue;
        }

        buffer += line;
    }

    flush(); // final flush
    return blocks;
}

// ===================================================================
// Tokenization
// ===================================================================

bool CSearchIndex::IsCJK(unsigned int cp)
{
    return (cp >= 0x4E00 && cp <= 0x9FFF)   // CJK Unified
        || (cp >= 0x3400 && cp <= 0x4DBF)   // CJK Ext-A
        || (cp >= 0x20000 && cp <= 0x2A6DF) // CJK Ext-B
        || (cp >= 0xF900 && cp <= 0xFAFF)   // CJK Compatibility
        || (cp >= 0x3040 && cp <= 0x309F)   // Hiragana
        || (cp >= 0x30A0 && cp <= 0x30FF)   // Katakana
        || (cp >= 0xAC00 && cp <= 0xD7AF);  // Hangul
}

std::vector<std::string> CSearchIndex::Tokenize(const std::string& text)
{
    std::vector<std::string> tokens;
    if (text.empty()) return tokens;

    enum State { Idle, Latin, CJK } state = Idle;
    std::string latinBuf;
    int prevCJK = 0; // previous CJK codepoint for bigram

    auto flushLatin = [&]() {
        if (!latinBuf.empty())
        {
            std::transform(latinBuf.begin(), latinBuf.end(), latinBuf.begin(), ::tolower);
            tokens.push_back(latinBuf);
            latinBuf.clear();
        }
    };

    auto encodeCodepoint = [](unsigned int cp, std::string& out) {
        if (cp < 0x80) { out += (char)cp; }
        else if (cp < 0x800) {
            out += (char)(0xC0 | (cp >> 6));
            out += (char)(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += (char)(0xE0 | (cp >> 12));
            out += (char)(0x80 | ((cp >> 6) & 0x3F));
            out += (char)(0x80 | (cp & 0x3F));
        } else {
            out += (char)(0xF0 | (cp >> 18));
            out += (char)(0x80 | ((cp >> 12) & 0x3F));
            out += (char)(0x80 | ((cp >> 6) & 0x3F));
            out += (char)(0x80 | (cp & 0x3F));
        }
    };

    auto addUnigram = [&](int cp) {
        std::string token;
        encodeCodepoint(cp, token);
        tokens.push_back(token);
    };

    auto addBigram = [&](int cp1, int cp2) {
        std::string bigram;
        encodeCodepoint(cp1, bigram);
        encodeCodepoint(cp2, bigram);
        tokens.push_back(bigram);
    };

    const unsigned char* s = (const unsigned char*)text.data();
    const unsigned char* end = s + text.size();

    while (s < end)
    {
        unsigned int cp;
        int len;

        if (*s < 0x80) { cp = *s; len = 1; }
        else if ((*s & 0xE0) == 0xC0) {
            cp = (*s & 0x1F) << 6;
            if (s + 1 < end) cp |= (s[1] & 0x3F);
            len = 2;
        }
        else if ((*s & 0xF0) == 0xE0) {
            cp = (*s & 0x0F) << 12;
            if (s + 1 < end) cp |= (s[1] & 0x3F) << 6;
            if (s + 2 < end) cp |= (s[2] & 0x3F);
            len = 3;
        }
        else if ((*s & 0xF8) == 0xF0) {
            cp = (*s & 0x07) << 18;
            if (s + 1 < end) cp |= (s[1] & 0x3F) << 12;
            if (s + 2 < end) cp |= (s[2] & 0x3F) << 6;
            if (s + 3 < end) cp |= (s[3] & 0x3F);
            len = 4;
        }
        else { s++; continue; }

        if (IsCJK(cp))
        {
            flushLatin();
            // Index both single CJK character and adjacent bigram
            addUnigram(cp);
            if (prevCJK != 0)
                addBigram(prevCJK, cp);
            prevCJK = cp;
            state = CJK;
        }
        else if (std::isalnum(cp))
        {
            if (state == CJK) { prevCJK = 0; }
            latinBuf += (char)std::tolower(cp);
            state = Latin;
        }
        else
        {
            flushLatin();
            prevCJK = 0;
            state = Idle;
        }

        s += len;
    }

    flushLatin();
    return tokens;
}