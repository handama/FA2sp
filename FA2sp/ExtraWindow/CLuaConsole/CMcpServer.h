#pragma once
#include <string>
#include <FA2PP.h>

// Custom window messages dispatched to CFinalSunDlg for main-thread execution
#define WM_MCP_RUN_LUA          (WM_APP + 200)
#define WM_MCP_LIST_KNOWLEDGE   (WM_APP + 201)
#define WM_MCP_GET_KNOWLEDGE    (WM_APP + 202)

struct MCPRequest
{
    int type;              // 0=run_lua, 3=list_knowledge, 4=get_knowledge
    std::string input;     // script / doc relpath
    std::string result;    // output captured during execution
    HANDLE hEvent;         // signalled when main-thread processing completes
};

class CMcpServer
{
public:
    static void Start(int port = 9090);
    static void Stop();
    static bool IsRunning();

    // Called from main thread via PreTranslateMessageExt
    static void HandleRunLua(MCPRequest* req);
    static void HandleListKnowledge(MCPRequest* req);
    static void HandleGetKnowledge(MCPRequest* req);

private:
    static bool m_running;
    static int m_port;
    static void* m_server;      // opaque pointer to httplib::Server
};