#include "CNewEasterEgg.h"
#include "../../FA2sp/FA2sp.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <windowsx.h>
#include <mmsystem.h>
#include "../../Helpers/Translations.h"
#pragma comment(lib, "winmm.lib")

HWND CGoBang::m_hwnd = nullptr;
CFinalSunDlg* CGoBang::m_parent = nullptr;
bool CGoBang::initialized = false;

int CGoBang::board[CGoBang::BOARD_N][CGoBang::BOARD_N] = {};
int CGoBang::margin = 20;
int CGoBang::cellSize = 32;
int CGoBang::boardLeft = 0;
int CGoBang::boardTop = 0;
int CGoBang::boardRight = 0;
int CGoBang::boardBottom = 0;
int CGoBang::clientW = 500;
int CGoBang::clientH = 500;

bool CGoBang::playerTurn = false;
bool CGoBang::gameOver = false;
int CGoBang::aiLastMoveR = -1;
int CGoBang::aiLastMoveC = -1; 

std::vector<CGoBang::Move> CGoBang::history;
std::mt19937 CGoBang::rng{ std::random_device{}() };
HFONT CGoBang::hfStatusText = nullptr;

static void DrawCircle(HDC hdc, int cx, int cy, int r, COLORREF fill, COLORREF frame) {
    HBRUSH b = CreateSolidBrush(fill); HBRUSH oldb = (HBRUSH)SelectObject(hdc, b);
    HPEN p = CreatePen(PS_SOLID, 2, frame); HPEN oldp = (HPEN)SelectObject(hdc, p);
    Ellipse(hdc, cx - r, cy - r, cx + r, cy + r);
    SelectObject(hdc, oldb); DeleteObject(b);
    SelectObject(hdc, oldp); DeleteObject(p);
}

static void TextCenter(HDC hdc, int x, int y, const wchar_t* s) {
    SIZE sz{}; GetTextExtentPoint32W(hdc, s, (int)wcslen(s), &sz);
    TextOutW(hdc, x - sz.cx / 2, y - sz.cy / 2, s, (int)wcslen(s));
}

void CGoBang::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        reinterpret_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(326),
        pWnd ? pWnd->MyViewFrame.GetSafeHwnd() : nullptr,
        CGoBang::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else {
        Logger::Error("Failed to create CGoBang.\n");
        m_parent = nullptr;
    }
}

void CGoBang::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("EasterEggTitle", buffer))
        SetWindowText(hWnd, buffer);

    RECT rc; GetClientRect(hWnd, &rc);
    clientW = rc.right - rc.left; clientH = rc.bottom - rc.top;
    RecalcLayout();

    LOGFONTW lfStatus{};
    lfStatus.lfHeight = -15;
    lfStatus.lfWeight = FW_NORMAL;
    wcscpy_s(lfStatus.lfFaceName, L"Cambria");
    hfStatusText = CreateFontIndirectW(&lfStatus);

    ResetGame();
    InvalidateRect(hWnd, nullptr, TRUE); 
    initialized = true;
}

void CGoBang::Close(HWND& hWnd)
{
    EndDialog(hWnd, 0);
    m_hwnd = nullptr;
    m_parent = nullptr;
    initialized = false;
}

void CGoBang::RecalcLayout()
{
    int w = clientW, h = clientH;
    int maxBoard = std::min(w - 2 * margin, h - 2 * margin);
    cellSize = std::max(18, maxBoard / (BOARD_N - 1)); 
    int total = cellSize * (BOARD_N - 1);
    boardLeft = (w - total) / 2;
    boardTop = (h - total) / 2;
    boardRight = boardLeft + total;
    boardBottom = boardTop + total;
}

void CGoBang::Render(HDC hdc)
{
    RECT rc; GetClientRect(m_hwnd, &rc);
    HBRUSH bk = CreateSolidBrush(RGB(220, 200, 160));
    FillRect(hdc, &rc, bk);
    DeleteObject(bk);

    HPEN pen = CreatePen(PS_SOLID, 1, RGB(40, 40, 40));
    HGDIOBJ oldPen = SelectObject(hdc, pen);

    for (int i = 0; i < BOARD_N; ++i) {
        int x = boardLeft + i * cellSize;
        int y1 = boardTop;
        int y2 = boardBottom;
        MoveToEx(hdc, x, y1, nullptr);
        LineTo(hdc, x, y2);
    }
    for (int j = 0; j < BOARD_N; ++j) {
        int y = boardTop + j * cellSize;
        int x1 = boardLeft;
        int x2 = boardRight;
        MoveToEx(hdc, x1, y, nullptr);
        LineTo(hdc, x2, y);
    }

    SelectObject(hdc, oldPen);
    DeleteObject(pen);

    int pts[] = { 7, 3, 11 };
    HBRUSH dot = CreateSolidBrush(RGB(40, 40, 40));
    for (int dr = 3; dr <= 11; dr += 4) {
        for (int dc = 3; dc <= 11; dc += 4) {
            int x = boardLeft + dc * cellSize;
            int y = boardTop + dr * cellSize;
            Ellipse(hdc, x - 3, y - 3, x + 3, y + 3);
        }
    }
    DeleteObject(dot);

    for (int r = 0; r < BOARD_N; ++r) {
        for (int c = 0; c < BOARD_N; ++c) {
            int who = board[r][c];
            if (who == 0) continue;
            int cx = boardLeft + c * cellSize;
            int cy = boardTop + r * cellSize;
            int rad = cellSize / 2 - 2;
        
            if (who == 1 && r == aiLastMoveR && c == aiLastMoveC) {
                DrawCircle(hdc, cx, cy, cellSize / 2, RGB(255, 190, 138), RGB(255, 127, 39));
            }

            HBRUSH b = CreateSolidBrush(who == 1 ? RGB(10, 10, 10) : RGB(245, 245, 245));
            HGDIOBJ oldb = SelectObject(hdc, b);
            Ellipse(hdc, cx - rad, cy - rad, cx + rad, cy + rad);
            SelectObject(hdc, oldb);
            DeleteObject(b);
        }
    }

    std::string status;
    if (gameOver) {
        status = Translations::TranslateOrDefault("EasterEggGameOverMessage", "Game over - press R to reset");
    }
    else {
        status = Translations::TranslateOrDefault("EasterEggPlayingMessage", 
            "You found the easter egg :-)     Press R to restart, press Z to retract a move");
    }
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));
    SelectObject(hdc, hfStatusText);
    TextOutA(hdc, 8, 8, status.c_str(), (int)status.size());
}

bool CGoBang::CheckWin(int who)
{
    const int dr[4] = { 0,1,1,1 };
    const int dc[4] = { 1,0,1,-1 };
    for (int r = 0; r < BOARD_N; ++r) for (int c = 0; c < BOARD_N; ++c) {
        if (board[r][c] != who) continue;
        for (int k = 0; k < 4; ++k) {
            int cnt = 1;
            int rr = r + dr[k], cc = c + dc[k];
            while (InBoard(rr, cc) && board[rr][cc] == who) { cnt++; rr += dr[k]; cc += dc[k]; }
            if (cnt >= 5) return true;
        }
    }
    return false;
}

std::vector<std::pair<int, int>> CGoBang::generateCandidateMoves()
{
    bool mark[BOARD_N][BOARD_N] = {};
    int range = 2;
    for (int r = 0; r < BOARD_N; ++r) for (int c = 0; c < BOARD_N; ++c) {
        if (board[r][c] != 0) {
            for (int dr = -range; dr <= range; ++dr) for (int dc = -range; dc <= range; ++dc) {
                int rr = r + dr, cc = c + dc;
                if (InBoard(rr, cc) && board[rr][cc] == 0) mark[rr][cc] = true;
            }
        }
    }
    bool any = false;
    for (int r = 0; r < BOARD_N; ++r) for (int c = 0; c < BOARD_N; ++c) if (board[r][c] != 0) any = true;
    std::vector<std::pair<int, int>> cand;
    if (!any) {
        cand.push_back({ BOARD_N / 2, BOARD_N / 2 });
        return cand;
    }
    for (int r = 0; r < BOARD_N; ++r) for (int c = 0; c < BOARD_N; ++c) if (mark[r][c]) cand.push_back({ r,c });
    if (cand.empty()) {
        for (int r = 0; r < BOARD_N; ++r) for (int c = 0; c < BOARD_N; ++c) if (board[r][c] == 0) cand.push_back({ r,c });
    }
    return cand;
}

int CGoBang::evaluatePoint(int r, int c, int who)
{
    const int dr[4] = { 0,1,1,1 };
    const int dc[4] = { 1,0,1,-1 };
    int opp = (who == 1) ? 2 : 1;
    int score = 0;

    for (int k = 0; k < 4; ++k) {
        int cnt_own = 1;
        int openEnds = 0;

        int rr = r + dr[k], cc = c + dc[k];
        while (InBoard(rr, cc) && board[rr][cc] == who) { cnt_own++; rr += dr[k]; cc += dc[k]; }
        if (InBoard(rr, cc) && board[rr][cc] == 0) openEnds++;

        rr = r - dr[k]; cc = c - dc[k];
        while (InBoard(rr, cc) && board[rr][cc] == who) { cnt_own++; rr -= dr[k]; cc -= dc[k]; }
        if (InBoard(rr, cc) && board[rr][cc] == 0) openEnds++;

        int add = 0;
        if (cnt_own >= 5) add += 100000;        
        else if (cnt_own == 4 && openEnds == 2) add += 10000; 
        else if (cnt_own == 4 && openEnds == 1) add += 2000;  
        else if (cnt_own == 3 && openEnds == 2) add += 1000;  
        else if (cnt_own == 3 && openEnds == 1) add += 200;   
        else if (cnt_own == 2 && openEnds == 2) add += 100;   
        else if (cnt_own == 2 && openEnds == 1) add += 10;
        else add += cnt_own * 2;

        score += add;
    }

    int oppScore = 0;
    for (int k = 0; k < 4; ++k) {
        int cnt = 1;
        int openEnds = 0;
        int rr = r + dr[k], cc = c + dc[k];
        while (InBoard(rr, cc) && board[rr][cc] == opp) { cnt++; rr += dr[k]; cc += dc[k]; }
        if (InBoard(rr, cc) && board[rr][cc] == 0) openEnds++;
        rr = r - dr[k]; cc = c - dc[k];
        while (InBoard(rr, cc) && board[rr][cc] == opp) { cnt++; rr -= dr[k]; cc -= dc[k]; }
        if (InBoard(rr, cc) && board[rr][cc] == 0) openEnds++;

        if (cnt >= 5) oppScore += 90000;
        else if (cnt == 4 && openEnds == 2) oppScore += 9000;
        else if (cnt == 4 && openEnds == 1) oppScore += 1500;
        else if (cnt == 3 && openEnds == 2) oppScore += 800;
        else if (cnt == 3 && openEnds == 1) oppScore += 150;
        else oppScore += cnt * 2;
    }

    return score + oppScore * 2;
}

void CGoBang::AIMove()
{
    if (gameOver) return;
    auto cand = generateCandidateMoves();
    int bestScore = -1;
    std::vector<std::pair<int, int>> bests;
    for (auto& p : cand) {
        int r = p.first, c = p.second;
        if (board[r][c] != 0) continue;
        int sc = evaluatePoint(r, c, 1); 
        if (sc > bestScore) {
            bestScore = sc;
            bests.clear();
            bests.push_back(p);
        }
        else if (sc == bestScore) {
            bests.push_back(p);
        }
    }
    if (bests.empty()) return;
    std::uniform_int_distribution<int> dist(0, (int)bests.size() - 1);
    auto pick = bests[dist(rng)];
    board[pick.first][pick.second] = 1;
    history.push_back({ pick.first, pick.second, 1 });
    aiLastMoveR = pick.first;
    aiLastMoveC = pick.second;

    if (CheckWin(1)) {
        gameOver = true;
        InvalidateRect(m_hwnd, nullptr, TRUE);
        MessageBox(m_hwnd, Translations::TranslateOrDefault("EasterEggGoBangAIWin", "AI wins"),
            Translations::TranslateOrDefault("EasterEggGameOverTitle", "Game over"), MB_OK | MB_ICONINFORMATION);
        return;
    }

    bool boardFull = true;
    for (int r = 0; r < BOARD_N; ++r) {
        for (int c = 0; c < BOARD_N; ++c) {
            if (board[r][c] == 0) {
                boardFull = false;
                break;
            }
        }
        if (!boardFull) break;
    }
    if (boardFull) {
        gameOver = true;
        InvalidateRect(m_hwnd, nullptr, TRUE);
        MessageBox(m_hwnd, Translations::TranslateOrDefault("EasterEggGoBangDraw", "Draw! The board is full."),
            Translations::TranslateOrDefault("EasterEggGameOverTitle", "Game over"), MB_OK | MB_ICONINFORMATION);
        return;
    }

    playerTurn = true;
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void CGoBang::UndoLastRound()
{
    if (history.empty()) return;
    if (!history.empty()) {
        auto last = history.back(); history.pop_back();
        board[last.r][last.c] = 0;
    }
    if (!history.empty()) {
        auto last = history.back(); history.pop_back();
        board[last.r][last.c] = 0;
    }
    gameOver = false;
    playerTurn = true;
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void CGoBang::ResetGame()
{
    for (int r = 0; r < BOARD_N; ++r) for (int c = 0; c < BOARD_N; ++c) board[r][c] = 0;
    history.clear();
    gameOver = false;
    aiLastMoveR = -1;
    aiLastMoveC = -1;

    int cr = BOARD_N / 2, cc = BOARD_N / 2;
    board[cr][cc] = 1;
    aiLastMoveR = cr;
    aiLastMoveC = cc;
    InvalidateRect(m_hwnd, nullptr, TRUE);

    playerTurn = true;
}

BOOL CALLBACK CGoBang::DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg) {
    case WM_INITDIALOG:
        m_hwnd = hwnd;
        Initialize(hwnd);
        return TRUE;

    case WM_SIZE:
    {
        RECT rc; GetClientRect(hwnd, &rc);
        clientW = rc.right - rc.left;
        clientH = rc.bottom - rc.top;
        RecalcLayout();
        InvalidateRect(hwnd, nullptr, TRUE);
    }
    return TRUE;

    case WM_LBUTTONUP:
        if (gameOver) break;
        if (!playerTurn) break;
        {
            int mx = GET_X_LPARAM(lParam);
            int my = GET_Y_LPARAM(lParam);
            int bestR = -1, bestC = -1; int bestDist = 1000000;
            for (int r = 0; r < BOARD_N; ++r) for (int c = 0; c < BOARD_N; ++c) {
                if (board[r][c] != 0) continue;
                int cx = boardLeft + c * cellSize;
                int cy = boardTop + r * cellSize;
                int dx = cx - mx, dy = cy - my;
                int d2 = dx * dx + dy * dy;
                if (d2 < bestDist) { bestDist = d2; bestR = r; bestC = c; }
            }
            int tol = (cellSize / 2 + 6);
            if (bestR >= 0 && bestC >= 0) {
                int cx = boardLeft + bestC * cellSize;
                int cy = boardTop + bestR * cellSize;
                if (abs(cx - mx) <= tol && abs(cy - my) <= tol) {
                    if (board[bestR][bestC] == 0) {
                        board[bestR][bestC] = 2; 
                        history.push_back({ bestR,bestC,2 });

                        if (CheckWin(2)) {
                            gameOver = true;
                            InvalidateRect(hwnd, nullptr, TRUE);
                            MessageBox(hwnd, Translations::TranslateOrDefault("EasterEggGoBangYouWin", "You win!"),
                                Translations::TranslateOrDefault("EasterEggGameOverTitle", "Game over"), MB_OK | MB_ICONINFORMATION);
                            break;
                        }
                        playerTurn = false;
                        InvalidateRect(hwnd, nullptr, TRUE);
                        AIMove();
                    }
                }
            }
        }
        return TRUE;

    case WM_KEYUP:
        if (wParam == 'R' || wParam == 'r') {
            ResetGame();
            return TRUE;
        }
        else if (wParam == 'Z' || wParam == 'z') {
            UndoLastRound();
            return TRUE;
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
        Render(hdc);
        EndPaint(hwnd, &ps);
    }
    return TRUE;

    case WM_CLOSE:
        Close(hwnd);
        return TRUE;
    }
    return FALSE;
}

HWND CChineseChess::m_hwnd = nullptr;
CFinalSunDlg* CChineseChess::m_parent = nullptr;
bool CChineseChess::initialized = false;
int CChineseChess::clientW = 500, CChineseChess::clientH = 500;
int CChineseChess::leftX = 10, CChineseChess::topY = 10, CChineseChess::cell = 36, CChineseChess::boardW = 0, CChineseChess::boardH = 0;
HFONT CChineseChess::hfText = nullptr;
HFONT CChineseChess::hfStatusText = nullptr;

Board CChineseChess::bd;
bool  CChineseChess::gameOver = false;
int   CChineseChess::selR = -1, CChineseChess::selC = -1;
int CChineseChess::aiLastMoveFromR = -1;
int CChineseChess::aiLastMoveFromC = -1;
int CChineseChess::aiLastMoveToR = -1;
int CChineseChess::aiLastMoveToC = -1;

std::unordered_map<uint64_t, int> Board::positionHistory;
std::vector<std::pair<Move, uint64_t>> Board::history;
uint64_t Board::zobristTable[16][10][9];
bool Board::zobristInitialized = false;

int Board::init[10][9] = {
        { BK_JU, BK_MA, BK_XIANG, BK_SHI, BK_JIANG, BK_SHI, BK_XIANG, BK_MA, BK_JU },
        { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
        { EMPTY, BK_PAO, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, BK_PAO, EMPTY },
        { BK_BING, EMPTY, BK_BING, EMPTY, BK_BING, EMPTY, BK_BING, EMPTY, BK_BING },
        { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
        { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
        { RD_BING, EMPTY, RD_BING, EMPTY, RD_BING, EMPTY, RD_BING, EMPTY, RD_BING },
        { EMPTY, RD_PAO, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, RD_PAO, EMPTY },
        { EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY },
        { RD_JU, RD_MA, RD_XIANG, RD_SHI, RD_SHUAI, RD_SHI, RD_XIANG, RD_MA, RD_JU }
};

void CChineseChess::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        reinterpret_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(326),
        pWnd ? pWnd->MyViewFrame.GetSafeHwnd() : nullptr,
        CChineseChess::DlgProc
    );
    if (m_hwnd) ShowWindow(m_hwnd, SW_SHOW);
    else       Logger::Error("Failed to create CChineseChess.\n");
}

void CChineseChess::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("EasterEggTitle", buffer))
        SetWindowText(hWnd, buffer);

    RECT rc; GetClientRect(hWnd, &rc);
    clientW = rc.right - rc.left; clientH = rc.bottom - rc.top;
    calcLayout();

    LOGFONTW lf{}; lf.lfHeight = -std::max(12, cell / 3); lf.lfWeight = FW_SEMIBOLD; wcscpy_s(lf.lfFaceName, L"Microsoft YaHei");
    hfText = CreateFontIndirectW(&lf);

    LOGFONTW lfStatus{};
    lfStatus.lfHeight = -15; 
    lfStatus.lfWeight = FW_NORMAL;
    wcscpy_s(lfStatus.lfFaceName, L"Cambria");
    hfStatusText = CreateFontIndirectW(&lfStatus);

    Board::initZobrist();
    newGame();
    initialized = true;
}

void CChineseChess::Close(HWND& hWnd)
{
    if (hfText) { DeleteObject(hfText); hfText = nullptr; }
    EndDialog(hWnd, 0);
    m_hwnd = nullptr; m_parent = nullptr; initialized = false;
}

void CChineseChess::calcLayout()
{
    cell = std::max(28, std::min((clientW - 20) / 8, (clientH - 20) / 9)) - 10;
    boardW = cell * 8; boardH = cell * 9;
    leftX = (clientW - boardW) / 2;
    topY = (clientH - boardH) / 2 + 10;
}

void DrawStar(HDC hdc, int x, int y, int cell, bool left, bool right, bool up, bool down)
{
    int d = 4;
    int l = 8; 

    if (left && up) {
        MoveToEx(hdc, x - d, y - d, nullptr);
        LineTo(hdc, x - d - l, y - d);
        MoveToEx(hdc, x - d, y - d, nullptr);
        LineTo(hdc, x - d, y - d - l);
    }
    if (right && up) {
        MoveToEx(hdc, x + d, y - d, nullptr);
        LineTo(hdc, x + d + l, y - d);
        MoveToEx(hdc, x + d, y - d, nullptr);
        LineTo(hdc, x + d, y - d - l);
    }
    if (left && down) {
        MoveToEx(hdc, x - d, y + d, nullptr);
        LineTo(hdc, x - d - l, y + d);
        MoveToEx(hdc, x - d, y + d, nullptr);
        LineTo(hdc, x - d, y + d + l);
    }
    if (right && down) {
        MoveToEx(hdc, x + d, y + d, nullptr);
        LineTo(hdc, x + d + l, y + d);
        MoveToEx(hdc, x + d, y + d, nullptr);
        LineTo(hdc, x + d, y + d + l);
    }
}

void CChineseChess::draw(HDC hdc)
{
    RECT rc; GetClientRect(m_hwnd, &rc);
    HBRUSH bk = CreateSolidBrush(RGB(220, 200, 160)); FillRect(hdc, &rc, bk); DeleteObject(bk);

    HPEN p = CreatePen(PS_SOLID, 1, RGB(80, 60, 40)); HPEN oldp = (HPEN)SelectObject(hdc, p);
    for (int c = 0; c < 9; ++c) {
        int x = leftX + c * cell;
        MoveToEx(hdc, x, topY, nullptr);
        if (c == 0 || c == 8) LineTo(hdc, x, topY + boardH);
        else {
            LineTo(hdc, x, topY + 4 * cell);
            MoveToEx(hdc, x, topY + 5 * cell, nullptr);
            LineTo(hdc, x, topY + boardH);
        }
    }

    for (int r = 0; r < 10; ++r) {
        int y = topY + r * cell;
        MoveToEx(hdc, leftX, y, nullptr);
        LineTo(hdc, leftX + boardW, y);
    }

    auto palaceX = [&](int col)->int { return leftX + col * cell; };
    auto palaceY = [&](int row)->int { return topY + row * cell; };

    MoveToEx(hdc, palaceX(3), palaceY(0), nullptr); LineTo(hdc, palaceX(5), palaceY(2));
    MoveToEx(hdc, palaceX(5), palaceY(0), nullptr); LineTo(hdc, palaceX(3), palaceY(2));

    MoveToEx(hdc, palaceX(3), palaceY(7), nullptr); LineTo(hdc, palaceX(5), palaceY(9));
    MoveToEx(hdc, palaceX(5), palaceY(7), nullptr); LineTo(hdc, palaceX(3), palaceY(9));
    SelectObject(hdc, oldp); DeleteObject(p);

    auto drawStarAt = [&](int r, int c, bool left, bool right) {
        int displayR = 9 - r;
        int cx = leftX + c * cell;
        int cy = topY + displayR * cell;
        DrawStar(hdc, cx, cy, cell, left, right, true, true);
    };

    drawStarAt(3, 0, false, true);
    drawStarAt(3, 2, true, true);
    drawStarAt(3, 4, true, true);
    drawStarAt(3, 6, true, true);
    drawStarAt(3, 8, true, false);

    drawStarAt(6, 0, false, true);
    drawStarAt(6, 2, true, true);
    drawStarAt(6, 4, true, true);
    drawStarAt(6, 6, true, true);
    drawStarAt(6, 8, true, false);

    drawStarAt(2, 1, true, true);
    drawStarAt(2, 7, true, true);

    drawStarAt(7, 1, true, true);
    drawStarAt(7, 7, true, true);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(100, 70, 40));
    HFONT oldf = (HFONT)SelectObject(hdc, hfText);
    const wchar_t* ch1 = L"楚河"; const wchar_t* ch2 = L"汉界";
    SIZE sz{}; GetTextExtentPoint32W(hdc, ch1, 2, &sz);
    TextOutW(hdc, leftX + boardW / 2 - sz.cx - 40, topY + 4 * cell + cell / 2 - sz.cy / 2, ch1, 2);
    TextOutW(hdc, leftX + boardW / 2 + 40, topY + 4 * cell + cell / 2 - sz.cy / 2, ch2, 2);

    if (selR >= 0 && selC >= 0) {
        int displayR = 9 - selR; 
        int cx = leftX + selC * cell;
        int cy = topY + displayR * cell;
        DrawCircle(hdc, cx, cy, cell / 2 - 4, RGB(255, 255, 200), RGB(200, 150, 0));
    }

    if (aiLastMoveFromR >= 0 && aiLastMoveFromC >= 0) {
        int displayR = 9 - aiLastMoveFromR;
        int cx = leftX + aiLastMoveFromC * cell;
        int cy = topY + displayR * cell;
        DrawCircle(hdc, cx, cy, cell / 4 - 4, RGB(255, 190, 138), RGB(255, 127, 39));
    }
    if (aiLastMoveToR >= 0 && aiLastMoveToC >= 0) {
        int displayR = 9 - aiLastMoveToR;
        int cx = leftX + aiLastMoveToC * cell;
        int cy = topY + displayR * cell;
        DrawCircle(hdc, cx, cy, cell / 2 - 4, RGB(255, 190, 138), RGB(255, 127, 39));
    }

    for (int r = 0; r < 10; ++r) for (int c = 0; c < 9; ++c) {
        int p = bd.g[r][c];
        if (!p) continue;
        int displayR = 9 - r;
        int cx = leftX + c * cell;
        int cy = topY + displayR * cell;
        bool red = IsRed(p);
        COLORREF face = red ? RGB(250, 245, 230) : RGB(235, 240, 245);
        COLORREF frame = red ? RGB(190, 40, 40) : RGB(40, 40, 90);
        DrawCircle(hdc, cx, cy, cell / 2 - 6, face, frame);

        const wchar_t* name = L"?";
        switch (p) {
        case BK_JU: name = L"車"; break; case RD_JU: name = L"俥"; break;
        case BK_MA: name = L"馬"; break; case RD_MA: name = L"傌"; break;
        case BK_XIANG: name = L"象"; break; case RD_XIANG: name = L"相"; break;
        case BK_SHI: name = L"士"; break; case RD_SHI: name = L"仕"; break;
        case BK_JIANG: name = L"將"; break; case RD_SHUAI: name = L"帥"; break;
        case BK_PAO: name = L"砲"; break; case RD_PAO: name = L"炮"; break;
        case BK_BING: name = L"卒"; break; case RD_BING: name = L"兵"; break;
        }
        SetTextColor(hdc, red ? RGB(200, 40, 40) : RGB(40, 60, 120));
        TextCenter(hdc, cx, cy, name);
    }

    std::string s;
    if (gameOver) s = Translations::TranslateOrDefault("EasterEggChessGameOverMessage", "Game over - press R to reset, press E to generate an end-game");
    else s = Translations::TranslateOrDefault("EasterEggChessPlayingMessage",
        "You found the easter egg :-)   Press R to restart, press Z to retract, press E to generate an end-game");
    SetTextColor(hdc, RGB(0, 0, 0));    
    SelectObject(hdc, hfStatusText);
    TextOutA(hdc, 8, 8, s.c_str(), (int)s.size());
    SelectObject(hdc, oldf);
}

void CChineseChess::newGame() {
    bd.reset();
    bd.searchStep = STDHelpers::RandomSelectInt(3, 5);
    Board::history.clear();
    Board::positionHistory.clear();
    gameOver = false;
    selR = selC = -1;
    aiLastMoveFromR = aiLastMoveFromC = -1;
    aiLastMoveToR = aiLastMoveToC = -1;
    Board::positionHistory[bd.zobristKey]++;
    aiStep(true);
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void CChineseChess::endWithMessage(const char* msg)
{
    gameOver = true;
    InvalidateRect(m_hwnd, nullptr, TRUE);
    MessageBox(m_hwnd, msg, Translations::TranslateOrDefault("EasterEggGameOverTitle", "Game over"), MB_OK | MB_ICONINFORMATION);
}

void CChineseChess::aiStep(bool first) {
    if (gameOver) return;
    if (bd.sideToMove != RED) return;

    Move m;
    if (first)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::discrete_distribution<> dis({ 40, 22, 12, 12, 12 });
        int choice = dis(gen);

        switch (choice) {
        case 0: 
            if (std::uniform_int_distribution<>(0, 1)(gen) == 0) {
                m.sr = 7; m.sc = 7; m.tr = 7; m.tc = 4;
            }
            else {
                m.sr = 7; m.sc = 1; m.tr = 7; m.tc = 4;
            }
            break;
        case 1: 
            if (std::uniform_int_distribution<>(0, 1)(gen) == 0) {
                m.sr = 6; m.sc = 2; m.tr = 5; m.tc = 2; 
            }
            else {
                m.sr = 6; m.sc = 6; m.tr = 5; m.tc = 6; 
            }
            break;
        case 2: 
            if (std::uniform_int_distribution<>(0, 1)(gen) == 0) {
                m.sr = 7; m.sc = 7; m.tr = 7; m.tc = 5;
            }
            else {
                m.sr = 7; m.sc = 1; m.tr = 7; m.tc = 3; 
            }
            break;
        case 3: 
            if (std::uniform_int_distribution<>(0, 1)(gen) == 0) {
                m.sr = 9; m.sc = 1; m.tr = 7; m.tc = 2; 
            }
            else {
                m.sr = 9; m.sc = 7; m.tr = 7; m.tc = 6; 
            }
            break;
        case 4: 
            if (std::uniform_int_distribution<>(0, 1)(gen) == 0) {
                m.sr = 7; m.sc = 1; m.tr = 7; m.tc = 2; 
            }
            else {
                m.sr = 7; m.sc = 7; m.tr = 7; m.tc = 6; 
            }
            break;
        }
        m.capture = bd.g[m.tr][m.tc];
    }
    else
    {
        if (!bd.pickBestMove(m)) {
            if (bd.inCheck(RED)) endWithMessage(Translations::TranslateOrDefault("EasterEggYouCheckMate", "Check mate! You win!"));
            else endWithMessage(Translations::TranslateOrDefault("EasterEggAICannotMove", "No pieces can move, you win."));
            return;
        }
    }

    aiLastMoveFromR = m.sr;
    aiLastMoveFromC = m.sc;
    aiLastMoveToR = m.tr;
    aiLastMoveToC = m.tc;

    int captured = bd.g[m.tr][m.tc];
    bd.doMove(m);
    Board::positionHistory[bd.zobristKey]++;
    Board::history.push_back({ m, bd.zobristKey });
    m.capture = captured;

    if (bd.inCheck(BLACK) && Board::positionHistory[bd.zobristKey] >= 3) {
        endWithMessage(Translations::TranslateOrDefault("EasterEggAIPerpetualCheckLose", "Perpetual check! AI loses."));
        return;
    }

    if (bd.inCheck(BLACK)) {
        PlaySoundW(L"SystemExclamation", NULL, SND_ALIAS | SND_ASYNC);
        std::vector<Move> ms; bd.genMoves(BLACK, ms);
        if (ms.empty()) { endWithMessage(Translations::TranslateOrDefault("EasterEggAICheckMate", "Check mate! AI wins.")); return; }
    }
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void CChineseChess::playerClick(int x, int y) {
    if (gameOver) return;
    if (bd.sideToMove != BLACK) return;

    std::vector<Move> ms;
    bd.genMoves(BLACK, ms);
    if (ms.empty()) {
        if (bd.inCheck(BLACK)) {
            endWithMessage(Translations::TranslateOrDefault("EasterEggAICheckMate", "Check mate! AI wins."));
        }
        else {
            endWithMessage(Translations::TranslateOrDefault("EasterEggYouCannotMove", "No pieces can move, AI wins!"));
        }
        return;
    }
    // only check
    if (x == -1 && y == -1)
    {
        return;
    }

    int c = (x - leftX + cell / 2) / cell;
    int r = 9 - (y - topY + cell / 2) / cell;
    if (r < 0 || r >= 10 || c < 0 || c >= 9) return;

    int p = bd.g[r][c];

    if (selR == -1) {
        if (p != EMPTY && isPlayerPiece(p)) {
            selR = r;
            selC = c;
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
        return;
    }

    if (selR >= 0) {
        Move chosen{};
        bool ok = false;
        for (auto& m : ms) {
            if (m.sr == selR && m.sc == selC && m.tr == r && m.tc == c) {
                int captured = bd.g[m.tr][m.tc];
                bd.doMove(m);
                bool stillInCheck = bd.inCheck(BLACK);
                bd.undoMove(m);
                if (captured == RD_SHUAI || !stillInCheck) {
                    chosen = m;
                    ok = true;
                    break;
                }
            }
        }

        if (ok) {
            int captured = bd.g[chosen.tr][chosen.tc];
            bd.doMove(chosen);
            Board::positionHistory[bd.zobristKey]++;
            Board::history.push_back({ chosen, bd.zobristKey });
            chosen.capture = captured;
            selR = selC = -1;
            InvalidateRect(m_hwnd, nullptr, TRUE);
            UpdateWindow(m_hwnd);

            if (captured == RD_SHUAI) {
                endWithMessage(Translations::TranslateOrDefault("EasterEggEatKing", "You captured the Red king! You win!"));
                return;
            }

            if (bd.inCheck(RED)) {
                std::vector<Move> ms2;
                bd.genMoves(RED, ms2);
                if (ms2.empty()) {
                    endWithMessage(Translations::TranslateOrDefault("EasterEggYouCheckMate", "Check mate! You win!"));
                    return;
                }
            }

            if (bd.inCheck(RED) && Board::positionHistory[bd.zobristKey] >= 3) {
                endWithMessage(Translations::TranslateOrDefault("EasterEggPlayerPerpetualCheckLose", "Perpetual check! You lose."));
                return;
            }

            aiStep();
            playerClick(-1, -1);
        }
        else {
            if (p != EMPTY && isPlayerPiece(p)) {
                selR = r;
                selC = c;
            }
            else {
                selR = selC = -1;
            }
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
    }
}

void CChineseChess::undoRound() {
    if (Board::history.empty() || Board::history.size() == 1) return;
    auto pop1 = [&]() {
        if (Board::history.empty()) return;
        auto [m, key] = Board::history.back();
        Board::history.pop_back();
        Board::positionHistory[key]--;
        if (Board::positionHistory[key] == 0) Board::positionHistory.erase(key);
        bd.undoMove(m);
    };
    pop1();
    if (!Board::history.empty()) pop1();
    gameOver = false; selR = selC = -1;
    bd.sideToMove = BLACK;
    aiLastMoveFromR = aiLastMoveFromC = -1;
    aiLastMoveToR = aiLastMoveToC = -1;
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void CChineseChess::endGame()
{
    bd.setEndGame(bd.getEndGame());
    Board::history.clear();
    Board::positionHistory.clear();
    gameOver = false;
    selR = selC = -1;
    aiLastMoveFromR = aiLastMoveFromC = -1;
    aiLastMoveToR = aiLastMoveToC = -1;
    Board::positionHistory[bd.zobristKey]++;
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

BOOL CALLBACK CChineseChess::DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg) {
    case WM_INITDIALOG:
        m_hwnd = hwnd; Initialize(hwnd); return TRUE;

    case WM_SIZE: {
        RECT rc; GetClientRect(hwnd, &rc);
        clientW = rc.right - rc.left; clientH = rc.bottom - rc.top;
        calcLayout(); InvalidateRect(hwnd, nullptr, TRUE);
        return TRUE;
    }

    case WM_LBUTTONUP: {
        int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        playerClick(x, y);
        return TRUE;
    }

    case WM_KEYUP:
        if (wParam == 'R' || wParam == 'r') { newGame(); return TRUE; }
        if (wParam == 'Z' || wParam == 'z') { undoRound(); return TRUE; }
        if (wParam == 'E' || wParam == 'e') { endGame(); return TRUE; }
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
        draw(hdc);
        EndPaint(hwnd, &ps);
        return TRUE;
    }

    case WM_CLOSE:
        Close(hwnd); return TRUE;
    }

    return FALSE;
}