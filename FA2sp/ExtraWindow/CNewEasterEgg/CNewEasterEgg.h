#pragma once
#include <vector>
#include <random>
#include <chrono>
#include <functional>
#include <FA2PP.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Helpers/MultimapHelper.h"
#include "../../Helpers/STDHelpers.h"

class CGoBang
{
public:
    static HWND GetHandle() { return CGoBang::m_hwnd; }
    static void Create(CFinalSunDlg* pWnd);

protected:
    static void Initialize(HWND& hWnd);
    static void Update();
    static void Close(HWND& hWnd);

    static BOOL CALLBACK DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static bool initialized;

    static const int BOARD_N = 15;
    static int board[BOARD_N][BOARD_N];

    static int margin;
    static int cellSize;
    static int boardLeft, boardTop, boardRight, boardBottom;
    static int clientW, clientH;
    static bool playerTurn;
    static bool gameOver;
    struct Move { int r, c, who; };
    static std::vector<Move> history;
    static std::mt19937 rng;
    static HFONT hfStatusText;

    // AI stuff
    static std::vector<std::pair<int, int>> neighborsOfBoard(); 
    static std::vector<std::pair<int, int>> generateCandidateMoves();
    static int evaluatePoint(int r, int c, int who);
    static void AIMove();

    static void RecalcLayout();
    static void Render(HDC hdc);
    static bool CheckWin(int who);
    static bool InBoard(int r, int c) { return r >= 0 && r < BOARD_N && c >= 0 && c < BOARD_N; }
    static void ResetGame();
    static void UndoLastRound(); 
};

#define ARRAYSIZE_A(a) (int)(sizeof(a)/sizeof((a)[0]))

struct Move {
    int sr, sc, tr, tc;
    int capture;  
};

enum Side { BLACK = 0, RED = 1 };
enum Piece {
    EMPTY = 0,
    BK_JU = 1, BK_MA = 2, BK_XIANG = 3, BK_SHI = 4, BK_JIANG = 5, BK_PAO = 6, BK_BING = 7,
    RD_JU = 9, RD_MA = 10, RD_XIANG = 11, RD_SHI = 12, RD_SHUAI = 13, RD_PAO = 14, RD_BING = 15
};
static inline bool IsRed(int p) { return p >= 9; }
static inline bool IsBlack(int p) { return p > 0 && p < 9; }
static inline Side PieceSide(int p) { return IsRed(p) ? RED : BLACK; }
static inline bool SameSide(int a, int b) { if (a == EMPTY || b == EMPTY) return false; return IsRed(a) == IsRed(b); }

struct Board {
    int g[10][9]{};
    Side sideToMove = RED; 
    uint64_t zobristKey;
    static uint64_t zobristTable[16][10][9];
    static bool zobristInitialized;

    // 初始化Zobrist表
    static void initZobrist() {
        if (zobristInitialized) return;
        std::random_device rd;
        std::mt19937_64 gen(rd());
        for (int p = 0; p < 16; ++p)
            for (int r = 0; r < 10; ++r)
                for (int c = 0; c < 9; ++c)
                    zobristTable[p][r][c] = gen();
        zobristInitialized = true;
    }

    // 计算当前局面的Zobrist哈希值
    void updateZobristKey() {
        zobristKey = 0;
        for (int r = 0; r < 10; ++r)
            for (int c = 0; c < 9; ++c)
                if (g[r][c] != EMPTY)
                    zobristKey ^= zobristTable[g[r][c]][r][c];
        zobristKey ^= (sideToMove == RED ? 0 : 1); // 考虑轮到哪方走
    }

    void reset() {
        int init[10][9] = {
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
        for (int r = 0; r < 10; ++r) for (int c = 0; c < 9; ++c) g[r][c] = init[r][c];
        sideToMove = RED; 
        updateZobristKey();
    }

    bool inBoard(int r, int c) const { return r >= 0 && r < 10 && c >= 0 && c < 9; }

    static bool inPalace(Side s, int r, int c) {
        if (s == RED)   return (r >= 7 && r <= 9 && c >= 3 && c <= 5);
        else         return (r >= 0 && r <= 2 && c >= 3 && c <= 5);
    }
    static bool riverCrossed(Side s, int r) {

        if (s == RED) return r <= 4; else return r >= 5;
    }

    bool flyingGenerals() const {
        int rR = -1, cR = -1, rB = -1, cB = -1;
        for (int r = 0; r < 10; ++r) for (int c = 0; c < 9; ++c) {
            if (g[r][c] == RD_SHUAI) { rR = r; cR = c; }
            if (g[r][c] == BK_JIANG) { rB = r; cB = c; }
        }
        if (cR == -1 || cB == -1 || cR != cB) return false;
        int c = cR;
        int s = (rB < rR ? 1 : -1);
        for (int r = rB + s; r != rR; r += s) {
            if (g[r][c] != EMPTY) return false; 
        }
        return true;
    }

    bool inCheck(Side s) const {
        int kingR = -1, kingC = -1, kingPiece = (s == RED ? RD_SHUAI : BK_JIANG);
        for (int r = 0; r < 10; ++r) for (int c = 0; c < 9; ++c) if (g[r][c] == kingPiece) { kingR = r; kingC = c; }
        if (kingR == -1) return false; 

        for (int c = kingC - 1; c >= 0; --c) {
            int p = g[kingR][c];
            if (p == EMPTY) continue;
            if (SameSide(p, kingPiece)) break;
            if ((IsRed(p) ? (p == RD_JU) : (p == BK_JU))) return true;
            break;
        }
        for (int c = kingC + 1; c < 9; ++c) {
            int p = g[kingR][c];
            if (p == EMPTY) continue;
            if (SameSide(p, kingPiece)) break;
            if ((IsRed(p) ? (p == RD_JU) : (p == BK_JU))) return true;
            break;
        }
        int blocker = 0;
        for (int r = kingR - 1; r >= 0; --r) {
            int p = g[r][kingC];
            if (p == EMPTY) continue;
            if (SameSide(p, kingPiece)) { blocker = 1; break; }
            if ((IsRed(p) ? (p == RD_JU || p == RD_SHUAI) : (p == BK_JU || p == BK_JIANG))) return true;
            break;
        }
        blocker = 0;
        for (int r = kingR + 1; r < 10; ++r) {
            int p = g[r][kingC];
            if (p == EMPTY) continue;
            if (SameSide(p, kingPiece)) { blocker = 1; break; }
            if ((IsRed(p) ? (p == RD_JU || p == RD_SHUAI) : (p == BK_JU || p == BK_JIANG))) return true;
            break;
        }

        int cnt = 0;
        for (int c = kingC - 1; c >= 0; --c) {
            int p = g[kingR][c];
            if (p == EMPTY) continue;
            cnt++;
            if (cnt == 2) {
                if (!SameSide(p, kingPiece) && (IsRed(p) ? p == RD_PAO : p == BK_PAO)) return true;
                break;
            }
        }
        cnt = 0;
        for (int c = kingC + 1; c < 9; ++c) {
            int p = g[kingR][c];
            if (p == EMPTY) continue;
            cnt++;
            if (cnt == 2) {
                if (!SameSide(p, kingPiece) && (IsRed(p) ? p == RD_PAO : p == BK_PAO)) return true;
                break;
            }
        }
        cnt = 0;
        for (int r = kingR - 1; r >= 0; --r) {
            int p = g[r][kingC];
            if (p == EMPTY) continue;
            cnt++;
            if (cnt == 2) {
                if (!SameSide(p, kingPiece) && (IsRed(p) ? p == RD_PAO : p == BK_PAO)) return true;
                break;
            }
        }
        cnt = 0;
        for (int r = kingR + 1; r < 10; ++r) {
            int p = g[r][kingC];
            if (p == EMPTY) continue;
            cnt++;
            if (cnt == 2) {
                if (!SameSide(p, kingPiece) && (IsRed(p) ? p == RD_PAO : p == BK_PAO)) return true;
                break;
            }
        }

        static int HR[8] = { -2,-1,1,2, 2,1,-1,-2 };
        static int HC[8] = { 1, 2,2,1,-1,-2,-2,-1 };
        static int LR[8] = { -1, 0,0,1, 1,0, 0,-1 };
        static int LC[8] = { 0, 1,1,0, 0,-1,-1, 0 };
        for (int k = 0; k < 8; ++k) {
            int r = kingR + HR[k], c = kingC + HC[k];
            int lr = kingR + LR[k], lc = kingC + LC[k];
            if (!inBoard(r, c)) continue;
            int p = g[r][c];
            if (p == EMPTY) continue;
            if (SameSide(p, kingPiece)) continue;
            if (!inBoard(lr, lc) || g[lr][lc] != EMPTY) continue;
            if (IsRed(p) ? p == RD_MA : p == BK_MA) return true;
        }

        if (s == RED) {
            int r = kingR + 1, c = kingC;
            if (inBoard(r, c) && g[r][c] == BK_BING) return true;
            int rl = kingR, cl = kingC - 1;
            int rr = kingR, cr = kingC + 1;
            if (inBoard(rl, cl) && g[rl][cl] == BK_BING && riverCrossed(BLACK, rl)) return true;
            if (inBoard(rr, cr) && g[rr][cr] == BK_BING && riverCrossed(BLACK, rr)) return true;
        }
        else {
            int r = kingR - 1, c = kingC;
            if (inBoard(r, c) && g[r][c] == RD_BING) return true;
            int rl = kingR, cl = kingC - 1;
            int rr = kingR, cr = kingC + 1;
            if (inBoard(rl, cl) && g[rl][cl] == RD_BING && riverCrossed(RED, rl)) return true;
            if (inBoard(rr, cr) && g[rr][cr] == RD_BING && riverCrossed(RED, rr)) return true;
        }

        return false;
    }

    void genMoves(Side s, std::vector<Move>& out) const {
        out.clear();
        for (int r = 0; r < 10; ++r) for (int c = 0; c < 9; ++c) {
            int p = g[r][c];
            if (p == EMPTY) continue;
            if ((s == RED && !IsRed(p)) || (s == BLACK && !IsBlack(p))) continue;

            auto addIfLegal = [&](int tr, int tc) {
                if (!inBoard(tr, tc)) return;
                int dst = g[tr][tc];
                if (dst != EMPTY && SameSide(dst, p)) return;

                bool ok = false;
                int dr = tr - r, dc = tc - c;
                switch (p) {
                case RD_JU: case BK_JU: {
                    if (dr != 0 && dc != 0) break;
                    int sgnr = (dr > 0) - (dr < 0);
                    int sgnc = (dc > 0) - (dc < 0);
                    int rr = r + sgnr, cc = c + sgnc; bool blocked = false;
                    while (rr != tr || cc != tc) {
                        if (g[rr][cc] != EMPTY) { blocked = true; break; }
                        rr += sgnr; cc += sgnc;
                    }
                    ok = !blocked;
                } break;
                case RD_PAO: case BK_PAO: {
                    if (dr != 0 && dc != 0) break;
                    int sgnr = (dr > 0) - (dr < 0);
                    int sgnc = (dc > 0) - (dc < 0);
                    int rr = r + sgnr, cc = c + sgnc; int cnt = 0;
                    while (rr != tr || cc != tc) {
                        if (g[rr][cc] != EMPTY) cnt++;
                        rr += sgnr; cc += sgnc;
                    }
                    if (dst == EMPTY) ok = (cnt == 0);
                    else            ok = (cnt == 1);
                } break;
                case RD_MA: case BK_MA: {
                    int adr = abs(dr), adc = abs(dc);
                    if (!((adr == 1 && adc == 2) || (adr == 2 && adc == 1))) break;
                    int lr = r + (dr == 0 ? 0 : (dr > 0 ? 1 : -1));
                    int lc = c + (dc == 0 ? 0 : (dc > 0 ? 1 : -1));
                    if (adr == 2) {
                        lr = r + (dr > 0 ? 1 : -1);
                        lc = c;
                    }
                    else {
                        lr = r;
                        lc = c + (dc > 0 ? 1 : -1);
                    }
                    if (!inBoard(lr, lc) || g[lr][lc] != EMPTY) break;
                    ok = true;
                } break;
                case RD_XIANG: case BK_XIANG: {
                    if (abs(dr) != 2 || abs(dc) != 2) break;

                    int er = r + dr / 2, ec = c + dc / 2;
                    if (g[er][ec] != EMPTY) break;
                    if (IsRed(p) && tr >= 5) ok = true;
                    if (IsBlack(p) && tr <= 4) ok = true;
                } break;
                case RD_SHI: case BK_SHI: {
                    if (abs(dr) != 1 || abs(dc) != 1) break;
                    if (inPalace(PieceSide(p), tr, tc)) ok = true;
                } break;
                case RD_SHUAI: case BK_JIANG: {
                    if (abs(dr) + abs(dc) != 1) break;
                    if (inPalace(PieceSide(p), tr, tc)) {
                        ok = true;
                    }
                } break;
                case RD_BING: {
                    if (dr == -1 && dc == 0) ok = true;
                    else if (riverCrossed(RED, r) && dr == 0 && abs(dc) == 1) ok = true;
                } break;
                case BK_BING: {
                    if (dr == 1 && dc == 0) ok = true;
                    else if (riverCrossed(BLACK, r) && dr == 0 && abs(dc) == 1) ok = true;
                } break;
                default: break;
                }
                if (!ok) return;

                Board tmp = *this;
                tmp.g[tr][tc] = p; tmp.g[r][c] = EMPTY;

                // 特殊处理：如果能吃掉对方的将/帅，则总是合法
                bool capturesKing = false;
                if (IsRed(p) && dst == BK_JIANG) capturesKing = true;
                if (IsBlack(p) && dst == RD_SHUAI) capturesKing = true;

                if (!capturesKing) {
                    // 如果不能吃掉将/帅，则检查常规规则
                    if (tmp.flyingGenerals()) return;
                    if (tmp.inCheck(PieceSide(p))) return;
                }

                Move m{ r,c,tr,tc,dst };
                out.push_back(m);
            };

            switch (p) {
            case RD_JU: case BK_JU:
            case RD_PAO: case BK_PAO: {
                const int dr[4] = { -1,1,0,0 };
                const int dc[4] = { 0,0,-1,1 };
                for (int k = 0; k < 4; ++k) {
                    int rr = r + dr[k], cc = c + dc[k];
                    while (inBoard(rr, cc)) {
                        addIfLegal(rr, cc);
                        if (g[rr][cc] != EMPTY) {
                            int rr2 = rr + dr[k], cc2 = cc + dc[k];
                            while (inBoard(rr2, cc2)) {
                                addIfLegal(rr2, cc2);
                                if (g[rr2][cc2] != EMPTY) break;
                                rr2 += dr[k]; cc2 += dc[k];
                            }
                            break;
                        }
                        rr += dr[k]; cc += dc[k];
                    }
                }
            } break;
            case RD_MA: case BK_MA: {
                static int MR[8] = { -2,-1,1,2, 2,1,-1,-2 };
                static int MC[8] = { 1, 2,2,1,-1,-2,-2,-1 };
                for (int k = 0; k < 8; ++k) addIfLegal(r + MR[k], c + MC[k]);
            } break;
            case RD_XIANG: case BK_XIANG: {
                int d[4][2] = { {2,2},{2,-2},{-2,2},{-2,-2} };
                for (auto& v : d) addIfLegal(r + v[0], c + v[1]);
            } break;
            case RD_SHI: case BK_SHI: {
                int d[4][2] = { {1,1},{1,-1},{-1,1},{-1,-1} };
                for (auto& v : d) addIfLegal(r + v[0], c + v[1]);
            } break;
            case RD_SHUAI: case BK_JIANG: {
                int d[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
                for (auto& v : d) addIfLegal(r + v[0], c + v[1]);
            } break;
            case RD_BING:
            case BK_BING: {
                addIfLegal(r + (IsRed(p) ? -1 : +1), c);
                if (riverCrossed(PieceSide(p), r)) {
                    addIfLegal(r, c - 1);
                    addIfLegal(r, c + 1);
                }
            } break;
            default: break;
            }
        }
    }

    // 执行走法时更新哈希
    void doMove(const Move& m) {
        int p = g[m.sr][m.sc];
        zobristKey ^= zobristTable[p][m.sr][m.sc]; // 移除起点棋子
        if (m.capture != EMPTY)
            zobristKey ^= zobristTable[m.capture][m.tr][m.tc]; // 移除被吃棋子
        zobristKey ^= zobristTable[p][m.tr][m.tc]; // 添加目标位置棋子
        zobristKey ^= (sideToMove == RED ? 0 : 1); // 切换轮到方
        m_lastCaptured = g[m.tr][m.tc];
        std::swap(g[m.tr][m.tc], g[m.sr][m.sc]);
        g[m.sr][m.sc] = EMPTY;
        sideToMove = (sideToMove == RED ? BLACK : RED);
    }

    // 撤销走法时恢复哈希
    void undoMove(const Move& m) {
        sideToMove = (sideToMove == RED ? BLACK : RED);
        zobristKey ^= (sideToMove == RED ? 0 : 1); // 恢复轮到方
        int p = g[m.tr][m.tc];
        zobristKey ^= zobristTable[p][m.tr][m.tc]; // 移除目标位置棋子
        if (m.capture != EMPTY)
            zobristKey ^= zobristTable[m.capture][m.tr][m.tc]; // 恢复被吃棋子
        zobristKey ^= zobristTable[p][m.sr][m.sc]; // 恢复起点棋子
        g[m.sr][m.sc] = g[m.tr][m.tc];
        g[m.tr][m.tc] = m.capture;
    }
    int m_lastCaptured = EMPTY;

    int evaluate() const {
        int score = 0;
        auto val = [&](int p)->int {
            switch (p) {
            case RD_JU: case BK_JU: return 500;
            case RD_MA: case BK_MA: return 250;
            case RD_XIANG: case BK_XIANG: return 120;
            case RD_SHI: case BK_SHI: return 120;
            case RD_SHUAI: case BK_JIANG: return 10000;
            case RD_PAO: case BK_PAO: return 275;
            case RD_BING: case BK_BING: return 90;
            default: return 0;
            }
        };
        for (int r = 0; r < 10; ++r) for (int c = 0; c < 9; ++c) {
            int p = g[r][c]; if (!p) continue;
            int v = val(p);
            if (p == RD_BING) { if (r <= 4) v += 40; if (r <= 2) v += 20; }
            if (p == BK_BING) { if (r >= 5) v += 40; if (r >= 7) v += 20; }
            int centerBonus = 4 - (abs(c - 4));
            v += centerBonus;
            score += (IsRed(p) ? v : -v);
        }
        if (inCheck(RED)) score -= 80;
        if (inCheck(BLACK)) score += 80;
        return score;
    }

    int search(int depth, int alpha, int beta) {
        if (depth == 0) return evaluate();

        std::vector<Move> ms;
        genMoves(sideToMove, ms);
        if (ms.empty()) {
            int sc = evaluate();
            if (inCheck(sideToMove)) return (sideToMove == RED ? -20000 : +20000) + sc / 10;
            return sc;
        }

        std::sort(ms.begin(), ms.end(), [&](const Move& a, const Move& b) {
            int av = (g[a.tr][a.tc] == EMPTY ? 0 : 10 + abs(g[a.tr][a.tc]));
            int bv = (g[b.tr][b.tc] == EMPTY ? 0 : 10 + abs(g[b.tr][b.tc]));
            return av > bv;
        });

        std::unordered_map<uint64_t, int> tempPositionHistory; // 临时记录搜索中的局面
        if (sideToMove == RED) {
            int best = -std::numeric_limits<int>::max();
            for (auto& m : ms) {
                uint64_t oldKey = zobristKey;
                int cap = g[m.tr][m.tc];
                doMove(m);

                if (inCheck(RED)) {
                    undoMove(m);
                    continue;
                }

                // 检查长将
                bool isPerpetualCheck = false;
                if (inCheck(BLACK)) {
                    tempPositionHistory[zobristKey]++;
                    if (tempPositionHistory[zobristKey] >= 3) {
                        isPerpetualCheck = true;
                    }
                }

                int sc = isPerpetualCheck ? -std::numeric_limits<int>::max() : search(depth - 1, alpha, beta);
                undoMove(m);
                zobristKey = oldKey;

                best = std::max(best, sc);
                alpha = std::max(alpha, sc);
                if (beta <= alpha) break;
            }
            return best;
        }
        else {
            int best = std::numeric_limits<int>::max();
            for (auto& m : ms) {
                uint64_t oldKey = zobristKey;
                int cap = g[m.tr][m.tc];
                doMove(m);

                if (inCheck(BLACK)) {
                    undoMove(m);
                    continue;
                }

                int sc = search(depth - 1, alpha, beta);
                undoMove(m);
                zobristKey = oldKey;

                best = std::min(best, sc);
                beta = std::min(beta, sc);
                if (beta <= alpha) break;
            }
            return best;
        }
    }

    bool pickBestMove(Move& out) {
        std::vector<Move> ms;
        genMoves(RED, ms);
        if (ms.empty()) return false;

        int bestScore = -std::numeric_limits<int>::max();
        std::vector<Move> bestMoves;

        std::sort(ms.begin(), ms.end(), [&](const Move& a, const Move& b) {
            int av = (g[a.tr][a.tc] == EMPTY ? 0 : 10 + abs(g[a.tr][a.tc]));
            int bv = (g[b.tr][b.tc] == EMPTY ? 0 : 10 + abs(g[b.tr][b.tc]));
            return av > bv;
        });

        std::unordered_map<uint64_t, int> tempPositionHistory; // 临时记录搜索中的局面
        for (auto& m : ms) {
            uint64_t oldKey = zobristKey;
            doMove(m);

            if (inCheck(RED)) {
                undoMove(m);
                continue;
            }

            // 检查是否会导致长将
            bool isPerpetualCheck = false;
            if (inCheck(BLACK)) {
                tempPositionHistory[zobristKey]++;
                if (tempPositionHistory[zobristKey] >= 3) {
                    isPerpetualCheck = true;
                }
            }

            int sc = isPerpetualCheck ? -std::numeric_limits<int>::max() : search(3, -30000, 30000);
            undoMove(m);
            zobristKey = oldKey; // 恢复哈希值

            if (isPerpetualCheck) continue; // 跳过导致长将的走法

            if (sc > bestScore) {
                bestScore = sc;
                bestMoves.clear();
                bestMoves.push_back(m);
            }
            else if (sc == bestScore) {
                bestMoves.push_back(m);
            }
        }

        if (!bestMoves.empty()) {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, bestMoves.size() - 1);
            out = bestMoves[dis(gen)];
            return true;
        }

        return false;
    }
};


class CChineseChess
{
public:
    static HWND GetHandle() { return CChineseChess::m_hwnd; }
    static void Create(CFinalSunDlg* pWnd);

protected:
    static void Initialize(HWND& hWnd);
    static void Close(HWND& hWnd);
    static BOOL CALLBACK DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static bool initialized;

    static int clientW, clientH;
    static int leftX, topY, cell, boardW, boardH;
    static HFONT hfText;
    static HFONT hfStatusText;

    static Board bd;
    static bool gameOver;
    static int  selR, selC;
    static int aiLastMoveFromR, aiLastMoveFromC;
    static int aiLastMoveToR, aiLastMoveToC;

    static std::unordered_map<uint64_t, int> positionHistory; // 记录局面出现次数
    static std::vector<std::pair<Move, uint64_t>> history; // 存储走法和对应哈希值

    static void calcLayout();
    static void draw(HDC hdc);
    static void newGame();
    static void aiStep(); 
    static void playerClick(int x, int y);
    static bool isPlayerPiece(int p) { return IsBlack(p); }
    static void endWithMessage(const wchar_t* msg);
    static void undoRound();
};