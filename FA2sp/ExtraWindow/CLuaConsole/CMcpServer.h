#pragma once
#include <string>
#include <FA2PP.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// Custom window messages dispatched to CFinalSunDlg for main-thread execution
#define WM_MCP_RUN_LUA           (WM_APP + 200)
#define WM_MCP_LIST_KNOWLEDGE    (WM_APP + 201)
#define WM_MCP_GET_KNOWLEDGE     (WM_APP + 202)
#define WM_MCP_SEARCH_KNOWLEDGE  (WM_APP + 203)
#define WM_MCP_LIST_SKILL        (WM_APP + 204)
#define WM_MCP_GET_SKILL         (WM_APP + 205)

struct MCPRequest
{
    int type;              // 0=run_lua, 3=list_knowledge, 4=get_knowledge, 5=search_knowledge
    std::string input;     // script / doc relpath / search query
    std::string result;    // output captured during execution
    HANDLE hEvent;         // signalled when main-thread processing completes
    bool trackIniChanges = false; // if true, capture INI state before/after script and report diff
};


// ---------------------------------------------------------------------------
// Block types after markdown splitting
// ---------------------------------------------------------------------------
struct SearchBlock
{
    std::string heading;   // nearest heading (e.g. "## create_taskforce")
    std::string content;   // full paragraph / code block text
    std::string file_path; // relative path from knowledge root
    int block_id = 0;
};

// ---------------------------------------------------------------------------
// A single search hit
// ---------------------------------------------------------------------------
struct SearchResult
{
    std::string source;    // file path (relative)
    std::string title;     // derived from file path
    std::string section;   // heading
    double relevance = 0;  // 0.0 ~ 1.0, higher = better
    std::string content;   // block content
};

// ---------------------------------------------------------------------------
// Lightweight TF-IDF search engine over markdown knowledge files
// Zero external dependencies, C++17 only.
// ---------------------------------------------------------------------------
class CSearchIndex
{
public:
    // Build index from all .md files under docRoot (recursive)
    void Build(const std::string& docRoot);

    // Search the index, return top-N results sorted by relevance
    std::vector<SearchResult> Search(const std::string& query, int topN = 5) const;

    // Discard all index data
    void Clear();

    size_t GetBlockCount() const { return m_blocks.size(); }

private:
    // ---- Block splitting ----
    static std::vector<SearchBlock> SplitMarkdown(
        const std::string& content, const std::string& filePath);

    // ---- Tokenization ----
    static std::vector<std::string> Tokenize(const std::string& text);

    static bool IsCJK(unsigned int cp);

    // ---- Index data ----
    struct Posting
    {
        int block_id;
        int term_count;   // times this term appears in the block
    };

    std::vector<SearchBlock> m_blocks;

    // term -> list of (block_id, count)
    std::unordered_map<std::string, std::vector<Posting>> m_inverted_index;

    // block_id -> total term count (for TF denominator)
    std::unordered_map<int, int> m_block_lengths;

    // term -> IDF = log(N / df)
    std::unordered_map<std::string, double> m_idf;
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
    static void HandleSearchKnowledge(MCPRequest* req);
    static void HandleListSkill(MCPRequest* req);
    static void HandleGetSkill(MCPRequest* req);

    // Build / rebuild the search index from knowledge files
    static void BuildSearchIndex();

private:
    static bool m_running;
    static int m_port;
    static void* m_server;          // opaque pointer to httplib::Server
    static CSearchIndex m_searchIndex;
};
