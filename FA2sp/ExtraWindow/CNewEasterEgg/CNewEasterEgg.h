#pragma once
#include <vector>
#include <random>
#include <chrono>
#include <functional>
#include <FA2PP.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Helpers/MultimapHelper.h"
#include "../../Helpers/STDHelpers.h"
#include <CLoading.h>

#pragma warning(push)
#pragma warning(disable: 4154)
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
    static int aiLastMoveR;
    static int aiLastMoveC;
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

struct Board
{
    int g[10][9]{};
    Side sideToMove = RED;
    uint64_t zobristKey;
    static uint64_t zobristTable[16][10][9];
    static bool zobristInitialized;
    static int init[10][9];
    int searchStep;

    static std::unordered_map<uint64_t, int> positionHistory;
    static std::vector<std::pair<Move, uint64_t>> history;

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

    void updateZobristKey() {
        zobristKey = 0;
        for (int r = 0; r < 10; ++r)
            for (int c = 0; c < 9; ++c)
                if (g[r][c] != EMPTY)
                    zobristKey ^= zobristTable[g[r][c]][r][c];
        zobristKey ^= (sideToMove == RED ? 0 : 1);
    }

    void reset() {
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
        for (int r = kingR - 1; r >= 0; --r) {
            int p = g[r][kingC];
            if (p == EMPTY) continue;
            if (SameSide(p, kingPiece)) break;
            if ((IsRed(p) ? (p == RD_JU || p == RD_SHUAI) : (p == BK_JU || p == BK_JIANG))) return true;
            break;
        }
        for (int r = kingR + 1; r < 10; ++r) {
            int p = g[r][kingC];
            if (p == EMPTY) continue;
            if (SameSide(p, kingPiece)) break;
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

        static int HR[8] = { -2, -1, 1, 2, 2, 1, -1, -2 };
        static int HC[8] = { 1,  2, 2, 1,-1,-2, -2, -1 };
        for (int k = 0; k < 8; ++k) {
            int r = kingR + HR[k], c = kingC + HC[k];
            if (!inBoard(r, c)) continue;
            int p = g[r][c];
            if (p == EMPTY) continue;
            if (SameSide(p, kingPiece)) continue;
            int lr, lc;
            if (HR[k] == -2 && HC[k] == 1) { lr = r + 1; lc = c; }  
            else if (HR[k] == -1 && HC[k] == 2) { lr = r; lc = c - 1; } 
            else if (HR[k] == 1 && HC[k] == 2) { lr = r; lc = c - 1; }  
            else if (HR[k] == 2 && HC[k] == 1) { lr = r - 1; lc = c; }  
            else if (HR[k] == 2 && HC[k] == -1) { lr = r - 1; lc = c; } 
            else if (HR[k] == 1 && HC[k] == -2) { lr = r; lc = c + 1; } 
            else if (HR[k] == -1 && HC[k] == -2) { lr = r; lc = c + 1; }
            else if (HR[k] == -2 && HC[k] == -1) { lr = r + 1; lc = c; }
            if (!inBoard(lr, lc) || g[lr][lc] != EMPTY) continue;
            if (IsRed(p) ? p == RD_MA : p == BK_MA) return true;
        }

        if (s == RED) {
            int r = kingR - 1, c = kingC;
            if (inBoard(r, c) && g[r][c] == BK_BING) return true;
            int rl = kingR, cl = kingC - 1;
            int rr = kingR, cr = kingC + 1; 
            if (inBoard(rl, cl) && g[rl][cl] == BK_BING && riverCrossed(BLACK, rl)) return true;
            if (inBoard(rr, cr) && g[rr][cr] == BK_BING && riverCrossed(BLACK, rr)) return true;
        }
        else {
            int r = kingR + 1, c = kingC; 
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

                bool capturesKing = false;
                if (IsRed(p) && dst == BK_JIANG) capturesKing = true;
                if (IsBlack(p) && dst == RD_SHUAI) capturesKing = true;

                if (!capturesKing) {
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

    void doMove(const Move& m) {
        int p = g[m.sr][m.sc];
        zobristKey ^= zobristTable[p][m.sr][m.sc];
        if (m.capture != EMPTY)
            zobristKey ^= zobristTable[m.capture][m.tr][m.tc]; 
        zobristKey ^= zobristTable[p][m.tr][m.tc]; 
        zobristKey ^= (sideToMove == RED ? 0 : 1);
        m_lastCaptured = g[m.tr][m.tc];
        std::swap(g[m.tr][m.tc], g[m.sr][m.sc]);
        g[m.sr][m.sc] = EMPTY;
        sideToMove = (sideToMove == RED ? BLACK : RED);
    }

    void undoMove(const Move& m) {
        sideToMove = (sideToMove == RED ? BLACK : RED);
        zobristKey ^= (sideToMove == RED ? 0 : 1);
        int p = g[m.tr][m.tc];
        zobristKey ^= zobristTable[p][m.tr][m.tc]; 
        if (m.capture != EMPTY)
            zobristKey ^= zobristTable[m.capture][m.tr][m.tc]; 
        zobristKey ^= zobristTable[p][m.sr][m.sc];
        g[m.sr][m.sc] = g[m.tr][m.tc];
        g[m.tr][m.tc] = m.capture;
    }
    int m_lastCaptured = EMPTY;

    int evaluate() const {
        int score = 0;

        int majorCount = 0;
        for (int r = 0; r < 10; ++r) {
            for (int c = 0; c < 9; ++c) {
                int p = g[r][c];
                if (p == RD_JU || p == BK_JU ||
                    p == RD_MA || p == BK_MA ||
                    p == RD_PAO || p == BK_PAO) {
                    ++majorCount;
                }
            }
        }

        bool endgame = (majorCount <= 7);

        auto val = [&](int p)->int {
            switch (p) {
            case RD_JU: case BK_JU: return 500;
            case RD_MA: case BK_MA: return endgame ? 300 : 250; 
            case RD_XIANG: case BK_XIANG: return 120;
            case RD_SHI: case BK_SHI: return 120;
            case RD_SHUAI: case BK_JIANG: return 10000;
            case RD_PAO: case BK_PAO: return endgame ? 240 : 300; 
            case RD_BING: case BK_BING: return 90;
            default: return 0;
            }
        };

        for (int r = 0; r < 10; ++r) {
            for (int c = 0; c < 9; ++c) {
                int p = g[r][c];
                if (!p) continue;
                int v = val(p);

                if (p == RD_BING) {
                    if (r <= 4) v += 40;
                    if (r <= 2) v += 20;
                }
                if (p == BK_BING) {
                    if (r >= 5) v += 40;
                    if (r >= 7) v += 20;
                }

                int centerBonus = 4 - abs(c - 4);
                v += centerBonus;

                score += (IsRed(p) ? v : -v);
            }
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

        std::unordered_map<uint64_t, int> tempPositionHistory; 
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

        for (auto& m : ms) {
            uint64_t oldKey = zobristKey;
            doMove(m);

            if (inCheck(RED)) {
                undoMove(m);
                continue;
            }

            bool isPerpetualCheck = false;
            if (inCheck(BLACK)) {
                if (positionHistory[zobristKey] + 1 >= 3) {
                    isPerpetualCheck = true;
                }
            }

            int sc = isPerpetualCheck ? -std::numeric_limits<int>::max() : search(searchStep, -30000, 30000);
            undoMove(m);
            zobristKey = oldKey;

            if (isPerpetualCheck) continue;

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

    int(&getEndGame())[10][9]{
        static int board[10][9];

        memset(board, 0, sizeof(board));
        DWORD length = 0;
        BYTE* buffer = nullptr;
        buffer = static_cast<BYTE*>(CLoading::Instance->ReadWholeFile("ChessEndgame.txt", &length));
        if (!buffer) {
            auto EndGames = "9/4N4/4k4/9/9/9/4P1P2/5K3/7C1/5n3 b\n"
"N4kC2/7c1/8b/8p/9/9/9/9/9/4K4 b\n"
"9/9/2P1ka1N1/9/9/9/9/5A3/4NK2p/5A3 b\n"
"5k3/8c/4ba3/9/9/9/9/3AK4/4AR3/9 b\n"
"3k1a3/2C6/9/9/9/1p6p/2P6/9/9/5K3 b\n"
"3R5/4a4/3k1a3/9/9/9/6P2/9/9/4KAB1c b\n"
"5k3/4a3r/5a2b/9/9/2P6/9/B2K5/9/R1B6 b\n"
"2c1k4/3C5/7P1/2c3C2/9/9/9/9/3K5/9 b\n"
"5Nb2/4k4/9/9/7C1/9/9/5K3/9/5n3 b\n"
"4k2c1/4a4/5a3/9/6b1R/9/9/5K3/4A1c2/5A3 b\n"
"4P4/4k4/5a2b/9/6b2/9/4p1P2/5K3/3NA4/9 b\n"
"5k3/9/4b4/9/9/9/4R4/4pAp2/9/3AK3n b\n"
"9/9/3k5/9/4R4/9/4P4/9/4c4/4KR3 b\n"
"3k5/6c2/b8/9/6b2/9/9/R1R1KA3/4A4/6B2 b\n"
"9/4N4/4bk3/9/9/1pP6/9/5A1p1/5K3/9 b\n"
"9/9/4k4/9/4P4/9/9/4BK2B/8R/7r1 b\n"
"9/9/4ka3/9/6b2/9/9/7R1/9/2BK3n1 b\n"
"3k5/9/3a5/7cP/9/9/9/3p1p3/4K4/5R3 b\n"
"4P4/5k3/3aba2b/9/9/9/6P2/3K5/4Ap3/9 b\n"
"3a1c3/3k1R3/3a5/5c3/9/P8/9/9/9/5K3 b\n"
"9/9/4k4/9/9/9/6P1P/9/5R3/5KBp1 b\n"
"9/9/1P2kP3/9/9/9/9/3pB4/9/3KCA3 b\n"
"9/3Ra4/3ak4/9/9/9/9/5K3/4AC3/5AB1n b\n"
"9/9/3kba3/9/6b2/9/4R3r/3A5/3RK4/3A5 b\n"
"5k3/9/3a4R/9/6b2/9/9/3A5/5Rp2/5K3 b\n"
"1P7/9/3a1k2P/9/9/9/9/9/9/3Kc3N b\n"
"5a3/9/3aRk3/4P4/6b2/9/9/p8/4K3p/9 b\n"
"5P3/5k3/3a1a2b/9/9/2Bp5/6P2/9/5K3/9 b\n"
"4k4/3N5/9/9/9/5cB2/9/B8/3K3n1/5N3 b\n"
"3N1P3/4a4/P4kc1P/9/9/9/9/9/9/3K5 b\n"
"5k3/4RN3/5a3/8p/9/9/9/9/9/5K3 b\n"
"9/9/4kaP2/9/9/9/9/5A3/4NK2p/5AB2 b\n"
"4k1b2/9/5a3/9/6N2/6B2/9/5A3/4NK2p/5AB2 b\n"
"4k4/4N4/9/9/9/9/9/5AR2/8n/5K1C1 b\n"
"9/4k4/5N3/2n6/8C/9/9/5K2p/9/9 b\n"
"4k4/4a4/3a5/9/9/1n3R3/9/4K4/5n3/9 b\n"
"9/5N3/3kCa3/9/9/9/5c3/5K3/9/2B2AB2 b\n"
"9/9/5k3/9/2b3b2/9/9/8C/5C2c/5KB2 b\n"
"5R3/4a4/3k1a3/9/9/9/5c3/5K2B/9/5AB1R b\n"
"4P4/4k4/5a2b/9/6b2/9/9/1p1K5/1CC6/9 b\n"
"9/3k5/5a3/4p4/9/5p3/9/6p2/1C7/5KC2 b\n"
"3k5/9/9/9/9/9/9/9/4K4/4RARcc b\n"
"2bC1kb2/9/3a1a3/9/9/9/P8/5K3/9/2c3B2 b\n"
"9/4a4/4kC3/9/9/9/2P6/8n/5K1R1/9 b\n"
"9/4C4/3kN4/8p/9/9/9/9/4AK2c/9 b\n"
"3a2b2/3ka1R2/8b/9/2c6/9/7c1/5K3/9/9 b\n"
"5Nb1c/9/4k4/9/9/9/9/5C3/5N3/5K3 b\n"
"3kCa3/4R4/5a3/9/9/9/9/3A1A3/9/4K1n2 b\n"
"3k5/9/5a3/9/9/1RP3n2/9/4K4/7n1/9 b\n"
"5k3/2c6/4b4/9/9/6B2/9/3AKRR2/4A4/9 b\n"
"9/3k5/3a5/p1p6/9/9/9/4BA3/4K4/5Rp2 b\n"
"3a1a3/9/4bk2b/9/9/9/6p2/3AK1c2/3RA4/9 b\n"
"4P4/3ka4/9/9/9/6B2/9/5K1R1/8n/6B2 b\n"
"9/2R2k3/b3n3b/8p/7N1/9/9/9/9/4K4 b\n"
"5kb1R/9/8C/9/9/9/7c1/9/9/3K4c b\n"
"5R3/9/3k5/1NP6/9/9/9/4K4/9/8r b\n"
"3k5/4aCR2/5a3/7n1/9/9/9/9/9/4K4 b\n"
"9/9/5k3/9/6b2/9/6P1P/3KB3B/3N5/7n1 b\n"
"3PCa3/4ak3/b7P/8p/6b2/9/9/9/9/4K4 b\n"
"9/9/3aka3/9/6b2/9/8P/9/5N3/2B2KBp1 b\n"
"9/4k4/8R/9/6b2/9/4p4/4BA3/4K4/9 b\n"
"3a5/3k5/4bR3/9/6b2/9/9/4p4/5K1Rp/9 b\n"
"5P3/9/4k3b/7C1/9/9/8n/5K3/9/5AB2 b\n"
"4Pa3/4R4/5k3/9/9/9/9/3A5/5Rp2/5K3 b\n"
"9/9/4k4/9/9/9/9/1R3K3/9/5nB2 b\n"
"5k3/4N4/9/8N/9/9/9/9/n8/3AKAn2 b\n"
"9/9/3P1k3/p8/9/6Bp1/P8/3K5/9/5A3 b\n"
"9/5R3/4kCP2/9/9/9/9/9/5c3/5K3 b\n"
"C7c/4a4/b2k1a3/9/9/9/9/5K2B/9/3A1CB2 b\n"
"4k4/9/5a3/9/9/9/9/3A1K3/9/2RA1cB2 b\n"
"4P4/4a4/3Nk4/9/9/6B2/8n/5K3/9/5AB2 b\n"
"3k5/9/3ab3r/9/6b2/6P2/9/5K2B/9/6B1R b\n"
"9/9/5k3/2n6/6n2/9/9/3K5/3RA4/9 b\n"
"9/4k4/3aRa3/p7p/6b2/9/9/9/4K4/5A3 b\n"
"4k4/4a4/4Ra3/9/2b3b2/9/9/9/5KRp1/9 b\n"
"9/9/4ka3/9/9/9/9/9/4N3n/2BAKAB1R b\n"
"9/3k5/9/9/9/2B2N3/9/4K1n2/4Ac3/4N1B2 b\n"
"7Nc/9/5k3/9/P1b6/9/9/9/4N4/3AKA3 b\n"
"9/9/5k3/9/6b1p/9/9/3A5/5Rp2/3A1KB2 b\n"
"3ak4/9/4ba3/8C/9/6B2/9/B2r5/8R/5K3 b\n"
"9/9/5k3/9/6b2/5p3/9/4BK3/4C4/3R5 b\n"
"3aCa3/4Nk3/4N4/8p/9/9/9/9/9/4K4 b\n"
"4k2r1/3Ca4/9/9/9/9/9/9/6p2/4K3R b\n"
"9/9/5k3/9/9/Rn7/5p3/B8/9/4K3n b\n"
"9/4kN3/5a3/9/9/9/4p4/4BK3/4A4/9 b\n"
"2bC5/9/3k5/9/9/9/4c4/4B4/4R4/4KA3 b\n"
"9/9/4k4/9/9/9/9/9/5N3/2B2KBp1 b\n"
"3a1C3/4a4/4kC3/9/9/p3p4/9/3AK4/9/9 b\n"
"8R/4ak3/4b3b/9/9/9/9/3ApA3/4nK3/9 b\n"
"9/4kC3/5a3/9/9/9/4p4/4BK3/4A4/9 b\n"
"9/9/4k4/4p4/8r/9/9/4NK3/4A1R2/9 b\n"
"1P3k3/2C6/5a3/9/9/2p6/9/9/5K3/3A1A2c b\n"
"4P4/4k4/5c3/9/3R2b2/9/9/5K3/8n/9 b\n"
"9/4Rk3/5a3/6p1p/6b2/9/9/9/9/4KA3 b\n"
"2b1Ra3/3ka4/3N4b/1n7/9/9/9/9/9/3K5 b\n"
"5a3/4kC3/5C3/9/9/9/9/4BK3/9/3c5 b\n"
"9/5k2c/3a1a2b/9/9/9/9/4cK3/9/2R6 b\n"
"9/9/3k1a3/9/9/9/3Rp4/3KB4/4N4/5A3 b\n"
"9/2P2k3/3a1a2P/9/9/9/9/3A1K3/9/1C3A2c b\n"
"4P4/3ka4/9/9/9/6B2/9/5K1C1/8n/6B2 b\n"
"3N5/3k5/9/9/9/9/7p1/2R1K4/5n3/6B2 b\n"
"5a3/4a4/b4k3/p7p/9/8p/9/5A3/9/5K1N1 b\n"
"9/9/5k3/9/2b3b2/5p3/9/4BK3/4C4/3R5 b\n"
"3k1a1n1/9/9/9/1R2n4/9/9/5K3/9/4cA3 b\n"
"4Nk2c/9/9/9/9/9/9/4C4/4N4/3AK4 b\n"
"9/9/3k5/9/9/9/2n6/9/4A4/3ARKBn1 b\n"
"9/9/5k3/9/2b3b2/9/9/4R3B/4R3c/4K1B2 b\n"
"9/3Rk4/3a5/2p6/9/9/9/4B4/8p/4K4 b\n"
"9/9/4k4/9/9/2P6/9/3KBA2B/8R/7r1 b\n"
"4k4/9/3a5/6n2/6b2/9/5c3/3R1K3/4A4/9 b\n"
"3kNa3/9/5a3/9/2b3b2/9/9/3ApA3/9/3K5 b\n"
"9/4a4/4k4/p8/9/9/5p3/5Ap1B/3K3N1/3A5 b\n"
"3C1P3/4a4/P4kc1P/9/9/9/9/9/9/3K5 b\n"
"9/9/5k3/9/9/9/8P/4KA3/4R4/6BcR b\n"
"8N/3k5/9/9/9/9/7p1/4K4/3nA2C1/5A3 b\n"
"3a1a3/9/4bk3/9/9/9/6p2/3AK1c2/3RA4/9 b\n"
"5Pb2/9/4k3R/8p/6b2/9/9/9/9/5K3 b\n"
"5NC2/9/3kba3/9/9/6B2/5c3/5K3/9/6B2 b\n"
"9/9/5k3/9/9/6P2/3R5/3KB4/3C5/6Bc1 b\n"
"9/9/5k3/9/2b3b2/9/8c/8B/5N3/4CKB2 b\n"
"4P4/4k4/5a3/5C3/9/6B2/8n/5K3/9/5AB2 b\n"
"2b6/3k2c2/9/9/9/9/6p2/B4K3/9/R3c4 b\n"
"2Ra5/5k3/9/9/9/9/2c6/3AKn3/9/7c1 b\n"
"6b2/5C3/4k4/9/8p/9/4p4/4BC3/4K4/9 b\n"
"9/8C/4k4/9/2b3b2/9/9/4Bp3/9/3ACKB2 b\n"
"9/5k3/5a3/9/9/9/6p2/4BK3/3RR4/5AB2 b\n"
"9/9/5k3/9/9/6B2/9/2rA1A2B/7R1/3K5 b\n"
"5a3/4k4/1P1a5/9/9/9/9/9/2R1A4/3KcAB2 b\n"
"4c4/4Rk2c/9/9/9/9/9/4R4/4K4/9 b\n"
"9/8c/2P1kN3/p8/9/9/5N3/9/5K3/9 b\n"
"9/9/4k4/9/9/9/4p4/4pA3/5K3/3C2B2 b\n"
"9/5C3/3ak2P1/9/4p4/9/9/5Ap2/9/3K5 b\n"
"9/3N5/3k5/3n5/9/9/3C5/3KB3B/9/n4A3 b\n"
"9/9/3k1a3/9/3P5/9/9/3KBA3/5R3/5A2r b\n"
"9/9/5k3/9/9/9/9/5A3/4AK3/4RRB1n b\n"
"9/5k3/b4N3/9/6b2/9/2P3P2/7pB/9/5K3 b\n"
"9/4a4/4ka3/9/9/6B2/9/5A3/4NK2p/2R2AB2 b\n"
"9/9/4Rk3/9/9/9/9/3pBA3/4K4/9 b\n"
"3a1N3/5k3/4N4/9/9/9/9/7n1/4K4/3A4n b\n"
"9/9/4k4/9/9/4p4/9/4KN3/3C1N3/2B3B2 b\n"
"9/9/3aka3/9/2b3b2/9/9/9/5C3/2B2KBp1 b\n"
"9/9/5k3/9/2b3b2/9/5Cp2/3A1K3/4C4/3A5 b\n"
"5P3/5k3/5a2P/9/6b2/9/9/9/p4K3/9 b\n"
"9/4a4/3k1a3/9/9/9/3Rp4/3K1A3/4C4/5R3 b\n"
"3a1kC2/6c2/b2a4R/9/6b2/9/9/3K5/9/9 b\n"
"6b2/5k3/3a5/9/9/9/9/4K3R/9/4RA2r b\n"
"3a1CN2/5k3/9/9/6b2/9/9/9/9/3KcAB2 b\n"
"9/9/4k1P2/7N1/6b2/9/9/5A3/4NK2p/5A3 b\n"
"4Ck2c/9/9/9/9/9/9/4C4/4A4/3AK4 b\n"
"9/9/5k3/9/R1bP5/6B2/9/3K5/9/8r b\n"
"9/2R1a4/4ka3/9/9/2n6/9/3A5/8n/5K1R1 b\n"
"9/4a4/3ak4/9/6b2/9/2p6/3K5/8R/5c3 b\n"
"9/3ka4/5a2b/9/6b2/9/6p2/3A1K3/6Rc1/9 b\n"
"4c4/9/3Rk4/9/9/9/9/4C4/4N4/4KA3 b\n"
"9/9/4ka3/9/9/9/9/4Bp3/3NKN3/6B2 b\n"
"9/7c1/4bk1P1/9/9/9/4P4/3AK4/4C4/9 b\n"
"5a3/4a4/5k1P1/9/9/9/9/5Ap2/4C4/3ACK3 b\n"
"2b6/3k1c3/4b3c/9/9/9/4R4/B8/9/5K3 b\n"
"3PPa3/4a4/b3k4/9/9/9/9/3ABAp2/4K4/9 b\n"
"9/9/5k3/9/9/9/8P/5Ap2/4NK3/3R5 b\n"
"4c4/9/3k1a1P1/9/9/9/9/7R1/5K2p/9 b\n"
"9/4a4/4ka3/9/9/6B2/9/5A3/4NK2p/2R2A3 b\n"
"9/4a4/4ka3/9/9/2Rp4p/9/9/1N7/5K3 b\n"
"4ca3/3k5/3a5/6p2/6b2/9/9/9/9/4KN1N1 b\n"
"3k5/9/1n7/9/5C3/2B6/9/5K3/7R1/2B2n3 b\n"
"9/9/3k1a3/9/9/9/9/4BK3/4AN3/5AB1n b\n"
"9/4k4/4Ra3/9/6b2/9/9/9/8p/4K4 b\n"
"9/9/4k3P/5P3/4P4/9/9/5K3/3R5/8r b\n"
"9/8c/4bk3/8p/9/9/9/4K4/9/4R4 b\n"
"9/9/4Nk3/9/9/9/4P4/9/9/4K3n b\n"
"3aNa3/4N4/4k4/9/2b3b2/9/4p4/4BA3/4K4/9 b\n"
"9/9/4k4/9/9/9/9/6n1B/4N4/3AKA2R b\n"
"9/9/5k3/9/2b3b2/5p3/9/4BK3/4R4/5AB2 b\n"
"5k3/4a4/8R/4p4/5R2r/9/9/5K3/9/6B2 b\n"
"9/5R3/4ka3/9/9/6N2/9/5A3/4NK2p/5A3 b\n"
"2c1C4/P3ak3/5a3/8P/9/9/9/9/9/5K3 b\n"
"9/5k3/9/9/9/1CB6/3C5/3A1A2B/n2K5/6n2 b\n"
"3k5/3N4c/5a2b/9/9/9/9/2NKB3B/9/9 b\n"
"1n1a5/5k3/3a5/9/9/7n1/8c/3K5/9/7R1 b\n"
"9/9/4k4/9/9/9/9/5AN2/4NK2p/2R2A3 b\n"
"6R2/5R3/3kba3/9/9/9/5c3/B4K2B/9/9 b\n"
"6c2/9/2P2k2b/6P2/9/9/9/9/4N4/3AKA3 b\n"
"9/9/5k3/9/2b3b2/9/9/9/5C1Nc/5KB2 b\n"
"9/3ka4/8b/9/6b2/9/7R1/4BK2B/8R/7r1 b\n"
"3k1nb2/3C5/8b/9/9/9/9/3K1R3/6n2/9 b\n"
"4P4/4a4/4k4/9/9/9/6R2/B8/5R3/3K3n1 b\n"
"9/5C3/5k3/8p/9/9/9/9/9/3Kn2C1 b\n"
"3Nk4/4a4/b2a5/9/6b2/8P/9/5K2B/9/6c2 b\n"
"5a3/4a4/5k3/9/9/9/5Np2/3ABK3/4N4/3A2B2 b\n"
"3kCa3/9/4b4/9/9/9/9/5A3/7n1/4KAB2 b\n"
"9/6c2/3k5/9/6b2/9/9/4KA3/4A4/2B3B1C b\n"
"3ak4/5R3/3a5/9/9/9/4P4/5A3/9/3K1c3 b\n"
"9/9/4ka3/9/9/6B2/5R3/5K1R1/8n/6B2 b\n"
"9/9/5k3/9/2b1P1b2/9/4P4/9/4c4/3RK4 b\n"
"8c/9/4bk2N/9/9/4R4/9/4Kc3/9/9 b\n"
"9/3ka4/3a1C1R1/9/9/9/9/4K3B/9/6Br1 b\n"
"9/9/5k3/9/6b2/9/5cP2/5C3/5R3/5K3 b\n"
"9/3ka4/8b/9/6b2/4R4/6R2/4K3B/4A2n1/9 b\n"
"4Pa3/3k5/5a3/9/9/4C4/9/4K4/4An1C1/9 b\n"
"9/5k3/3a1a3/9/9/9/9/3K1A2B/3RA4/6BcR b\n"
"3k5/9/8c/3C5/9/9/9/3A1A3/4K4/4R3r b\n"
"4C1b2/4k4/5a3/9/9/9/9/5K3/9/2RA4c b\n"
"9/3ka1R2/5a3/2P3P2/9/9/9/5K3/9/8r b\n"
"3Rk3c/9/9/9/9/4p4/3c5/3K5/9/5R3 b\n"
"3NCa3/4a4/4k4/9/2b3b2/9/9/9/4N4/3AKA3 b\n"
"4C1b2/4a4/3ak3b/p8/9/9/9/3K4B/1p7/3A5 b\n"
"3k5/9/8b/9/2bc5/9/2p6/3K2p2/9/4R4 b\n"
"9/9/5k2N/9/6b2/9/6P2/3ApA3/9/5K3 b\n"
"9/3ka4/5a3/9/6b2/9/7R1/4BK2B/8R/7r1 b\n"
"9/4kC3/5a3/9/6b2/9/4Rp3/4K4/4A4/9 b\n"
"9/9/5k3/9/4P4/9/4P4/4c3B/3NK4/9 b\n"
"9/9/4k4/9/9/9/9/8B/3CN3c/3AKA3 b\n"
"9/9/3k1a1N1/9/9/9/8r/5K3/6R2/8C b\n"
"9/4k4/5N3/9/9/6C2/9/5K1n1/9/5AB2 b\n"
"9/9/5k3/9/9/9/5Np2/4BK3/4N4/3A2B2 b\n"
"5k3/8c/4ba3/9/9/9/6c2/3AR3B/9/5K3 b\n"
"3n4c/4k4/9/6p2/9/9/9/2R2A3/5K3/9 b\n"
"9/5k3/9/9/9/3p3R1/2p6/2nK3p1/9/9 b\n"
"3N5/5k3/3aba3/5N3/9/9/3c5/3K4B/9/9 b\n"
"6N2/9/5k3/9/9/9/2C6/5n3/5K1n1/5A3 b\n"
"5k3/4aN3/3aN4/8p/6b2/9/9/9/5K3/9 b\n"
"3aPa3/4P4/5k3/9/9/9/9/3A5/5Rp2/5K3 b\n"
"9/9/5k3/8p/8R/9/9/8B/5Cp2/3A1KB2 b\n"
"9/1R1k5/3r4b/9/4p4/9/2P6/5K3/9/3A5 b\n"
"5k3/5N3/4b4/9/9/9/9/3ARp3/9/5K3 b\n"
"4k1b2/9/5a3/7N1/9/9/9/5A3/4NK2p/5A3 b\n"
"3R1k3/4a4/5a3/9/9/9/9/9/9/3K1RB1c b\n"
"9/4k4/5N3/9/9/9/9/6N2/8n/3A1K1C1 b\n"
"4P4/4a4/4ka3/9/9/9/6C2/B8/5C3/3K3n1 b\n"
"3a1k3/8c/4ba3/9/9/9/9/9/5K3/4R4 b\n"
"4k4/4a4/9/9/4n4/9/8n/5K3/4A2R1/4RA3 b\n"
"9/9/4k4/9/9/9/5R3/3K5/5R3/2rA1c3 b\n"
"8N/3k5/9/9/9/9/7p1/4K4/3nA2R1/5A3 b\n"
"9/4C4/4ka3/9/9/9/9/9/4N4/3AKA3 b\n"
"6b2/4a4/3ak4/9/4P4/9/9/4K4/9/3ARA2r b\n"
"4P4/4ak3/3a1N3/9/9/9/9/3A1N3/9/4K1n2 b\n"
"9/4kN3/3a1a3/9/2b3b2/9/9/3KBp3/4N4/9 b\n"
"9/9/5k3/9/9/9/9/9/4R4/4KABCc b\n"
"4ka3/C8/5a3/p8/4p4/9/5p3/3AK4/4A4/9 b\n"
"9/9/5k3/9/6b2/9/4P4/B2AK4/4C4/6B1c b\n"
"3kNa3/9/5a3/9/6b2/9/9/3ApA3/9/3K5 b\n"
"4kaR2/9/3a5/9/6b2/9/9/9/7N1/3K2c2 b\n"
"9/9/4k4/9/9/9/9/8B/5R2c/5KB2 b\n"
"9/9/4ka3/9/6b2/9/9/4Bp3/3NKN3/6B2 b\n"
"5k3/9/5aC2/5c3/9/9/8C/B7B/3n1K3/9 b\n"
"9/9/2P1k1P2/9/9/9/9/5A3/4NK2p/5AB2 b\n"
"4P4/5k3/5a3/9/9/9/9/3ApA3/9/5K3 b\n"
"9/9/5k3/9/9/p8/9/3K1Ap2/1N7/9 b\n"
"9/4a4/5k3/9/9/9/9/5Ap2/4C4/2RACK3 b\n"
"4PR3/4a4/4k4/9/2b3b2/6B2/9/5K1n1/9/6B2 b\n"
"5Cb2/9/5k3/9/9/9/4c4/4C4/4A4/3AK4 b\n"
"9/9/3aka2N/9/2b3b2/9/9/9/4N4/3AKA3 b\n"
"4Pa3/4k4/3a1N3/9/6b2/9/9/9/p4K3/9 b\n"
"3kNa3/9/4ba3/9/9/6B2/6P2/9/9/4K3n b\n"
"5k3/9/9/9/9/4R3r/2c6/4KA3/5R3/5A3 b\n"
"5P3/9/4k3b/7P1/9/9/9/5K3/8n/5AR2 b\n"
"5a3/9/2P1k4/9/9/9/9/9/3K5/3ANA2c b\n"
"9/4Nk3/4N4/9/6b2/9/9/3A5/5Rp2/5K3 b\n"
"9/4a4/4k4/9/9/6B1R/9/5A2B/4NK2p/5A3 b\n"
"3k5/8c/4b4/9/9/9/9/3RKA3/4AR3/9 b\n"
"9/9/5kC1b/6p2/6b2/9/9/7Cp/9/4K4 b\n"
"4P4/4k4/3a5/9/6b2/2B6/3C5/3K5/4n4/2B6 b\n"
"3a1Rb2/3Na4/3kb4/9/9/9/9/5K2B/5c3/9 b\n"
"9/9/4k4/9/9/9/6P1C/5n3/7C1/5KB2 b\n"
"9/9/5k3/9/6b2/5p3/9/3A1K3/4R4/2B2AB2 b\n"
"9/9/3k4b/9/2b6/2B2N3/9/5K3/1R7/3A1ABr1 b\n"
"4P4/9/5k3/9/6b2/p8/8P/4BAp2/5K3/5A3 b\n"
"4PP3/4a4/5k3/9/2b3b2/9/6P2/3ApA3/9/5K3 b\n"
"9/3k5/3N5/9/p8/6Bp1/8P/3K5/9/3A5 b\n"
"9/9/5k3/9/9/2P3P2/9/3KB4/3R5/6Bc1 b\n"
"6b2/5c3/5k3/9/2P6/9/9/9/4N4/3AKA3 b\n"
"9/5k3/5C2R/8p/9/9/9/4BK3/9/8c b\n"
"9/9/5k3/9/9/9/5Np2/4BK3/4C4/3A2B2 b\n"
"9/9/4k4/9/9/9/6p2/3ABK3/3RR4/5A3 b\n"
"9/4Ck3/4N4/8p/9/6B2/9/5Ap2/5K3/6B2 b\n"
"9/9/4ka3/9/6b2/6B2/5R3/5K1R1/8n/6B2 b\n"
"1N2k4/2c6/9/9/9/9/9/3K1n2B/9/5C3 b\n"
"9/3k5/3N5/9/6b2/4p1N2/8n/9/4K4/3A5 b\n"
"5k3/9/9/8p/9/9/9/B8/9/1R1KcAB2 b\n"
"9/9/5k3/9/6b2/5p3/9/4BK3/3R5/4RAB2 b\n"
"9/9/4Rk3/9/4c4/9/4P4/4Rc3/9/4K4 b\n"
"5a1N1/9/b2ak4/4p4/6p2/6B2/9/5K3/8p/9 b\n"
"9/9/4k4/9/9/9/9/9/5R1Nc/5KB2 b\n"
"3a1a3/9/4kC3/4p4/9/9/9/5A1C1/4nK3/9 b\n"
"9/3k5/9/9/2p6/2B6/9/5K1pB/7N1/9 b\n"
"9/8c/b4k2b/9/9/9/9/4C4/4N4/3AK4 b\n"
"9/9/5k3/9/2b3b2/9/5Np2/3A1K3/4C4/3A5 b\n"
"9/9/4k1P2/9/9/9/9/7cB/4AC3/3AK1B2 b\n"
"4P4/4C4/3k1a3/9/9/9/9/5A3/3Np4/3K5 b\n"
"4k1b2/4a2CC/5a3/9/9/9/8r/5K3/9/5R3 b\n"
"9/4Rk3/3aPa3/p8/9/9/9/9/9/5K3 b\n"
"5P3/9/4ka2b/9/9/9/6P2/9/4N4/3AKA3 b\n"
"9/4k4/4C4/8p/9/4p4/5C3/4KA3/9/9 b\n"
"3a5/4k2c1/9/9/9/9/9/5A3/3R1K1cR/9 b\n"
"9/4C4/3kb4/9/6b2/9/9/9/4c4/4KC3 b\n"
"9/2ck1R3/4b4/9/9/9/9/4Rc3/9/5K3 b\n"
"2b1C1b2/4ak3/9/9/9/9/9/5R3/9/3AKAn2 b\n"
"9/9/3aka3/8P/9/9/9/3K4B/9/4c3C b\n"
"4C4/4a4/b2ak3R/9/6b2/9/9/3K1A3/9/2r2A3 b\n"
"9/3ka4/4Ra3/9/6b2/9/9/3ApA3/9/3K5 b\n"
"3ak4/4a4/9/1P7/6b1R/9/8P/9/9/4K3c b\n"
"9/4a4/3abk2R/9/6b2/9/4P4/3K5/9/8n b\n"
"3a1k3/9/9/p8/9/9/5Cp2/3A1K2B/9/9 b\n"
"9/4a1C2/5k3/p8/9/8p/9/5A1pB/9/5KB2 b\n"
"5aC2/4ak3/8b/9/8c/9/4P4/9/9/4KA3 b\n"
"3Ck4/9/8c/9/9/2B5c/9/3K4N/9/2B6 b\n"
"3a1C3/5k3/5N2b/9/6b2/9/9/5K3/9/7n1 b\n"
"9/9/3k5/9/9/3p5/9/3K1A3/4A2n1/6R2 b\n"
"3aNk3/4aN3/9/9/6b2/9/9/B8/9/4cKB2 b\n"
"4k4/4a4/5a3/9/4n4/9/8n/5K3/4A2R1/4RA3 b\n"
"5k3/4a3R/b7b/9/9/9/8c/3K4B/9/3A5 b\n"
"4k4/8R/4ba2b/9/9/9/8c/B3KA2B/4A4/9 b\n"
"9/9/3aka2R/9/6b2/9/4p1P2/4BA3/4K4/9 b\n"
"9/9/5k3/8R/9/7n1/4P4/3K5/9/9 b\n"
"3aCa3/5k3/4bN3/9/9/9/9/7pB/5K3/9 b\n"
"9/9/3aka3/9/2b3b2/9/9/5A3/4AK3/2B2RB1n b\n"
"9/5k3/9/9/9/1CB6/3C5/3A4B/n2K5/6n2 b\n"
"9/4a4/3ak4/9/9/6P2/4P4/7N1/4K4/8n b\n"
"9/9/5k3/9/9/1R3p3/4n4/4BK3/9/9 b\n"
"9/9/4k1P2/9/9/9/9/7cB/4AN3/3AK1B2 b\n"
"2NC1a3/9/3kba3/9/9/9/9/4KA3/7n1/5A3 b\n"
"9/9/5k3/9/2b3b2/5p3/9/4BK3/4R4/5RB2 b\n"
"9/4a4/3a1k3/9/9/9/9/4K3R/9/4RA2r b\n"
"9/9/5k3/8p/9/9/4C4/5ARp1/9/5K3 b\n"
"9/3k5/3a1a3/9/9/9/9/4R4/4AK3/5C2r b\n"
"9/9/4k4/9/9/9/9/8R/5R1c1/5KB2 b\n"
"4PCb2/4k4/3a1a3/9/6bn1/9/9/9/5K3/9 b\n"
"2b1C1b2/4k4/3a1a3/9/9/9/3C5/5A3/3K5/5An2 b\n"
"9/4R4/3aka3/9/9/9/9/9/4N4/3AKA3 b\n"
"5P3/4k4/3aNa3/9/6b2/9/9/5A3/9/4KAn2 b\n"
"9/4a4/3a1k3/9/6b2/5p3/9/3A1K2B/4C4/3A2B2 b\n"
"8c/9/4k4/9/9/9/9/5C3/5N3/5K3 b\n"
"9/4k3C/4b4/9/9/6B2/8r/4KA2B/4R4/9 b\n"
"R3C4/4k4/5a3/9/6b2/9/9/5K3/9/8c b\n"
"9/8c/b4k2b/9/9/9/9/4C4/4A4/3AK4 b\n"
"4k4/9/P8/9/2p5p/6B2/8P/5K3/9/9 b\n"
"5Pb2/4k4/5N2b/9/9/6B2/8n/5K3/9/5AB2 b\n"
"6R2/4nk3/4ba2b/9/9/9/9/9/9/1C1K3n1 b\n"
"R1b2k3/9/b8/9/2P6/9/9/9/9/3K3Rr b\n"
"4k1b1R/9/8b/6n2/9/6B2/9/4cK3/4A4/9 b\n"
"9/3k5/8C/9/6b2/9/9/4BK3/4AC3/5A2n b\n"
"4k4/4aN3/3ab4/8p/6b2/9/9/9/5K3/9 b\n"
"9/4a4/3a1k3/9/6b2/5p3/9/4BK3/4C4/3A2B2 b\n"
"9/9/3aka3/9/6b2/9/8P/5A3/5K1p1/2B2AB2 b\n"
"4k4/4a4/9/9/9/9/9/4BK3/4AC3/5AB1n b\n"
"9/9/4k4/9/9/8R/6R2/5A2n/5K1C1/9 b\n"
"9/3k2P2/5a3/9/9/9/6p2/5K3/4ARR2/5A3 b\n"
"9/9/4k4/9/4C4/9/9/9/5K3/5A2n b\n"
"3N5/4k4/7cN/9/9/9/9/5K2B/9/3A4c b\n"
"9/4a4/5k3/9/9/5p3/9/3A1K3/4C4/3A2B2 b\n"
"3NkP3/4a4/5a3/9/9/9/9/4KA3/4A2n1/9 b\n"
"9/4k4/5a3/9/9/9/9/9/5K3/1CN5c b\n"
"9/9/3k5/6p2/4P4/9/p8/3A1K3/9/3A3Rp b\n"
"9/9/4k4/9/9/9/9/1C3K3/3N5/5nB2 b\n"
"4k4/2P6/4Na3/9/9/9/9/3AK4/9/4cAB2 b\n"
"4Nk3/4a4/4b4/9/9/9/9/3A1Ap2/5K3/9 b\n"
"4PP3/4k4/3a1a3/9/2b3b2/9/9/9/4N4/3AKA3 b\n"
"2b1Ra2N/4a4/5k3/8r/6b2/9/9/3K5/4A4/9 b\n"
"3aP1R2/9/3k4b/9/6b2/9/9/3A1A3/5K3/4n4 b\n"
"3kNa3/4a4/9/9/2b3b2/6B2/9/5K1C1/8n/6B2 b\n"
"5R3/5k3/5a2b/6p2/6b2/9/9/9/8p/4K1B2 b\n"
"9/3k5/3a5/9/6b2/6B2/9/5K1R1/8n/6B2 b\n"
"3a5/5k3/5a3/9/9/8p/4C1p2/7pB/9/5KB2 b\n"
"9/5k3/C7r/p8/9/9/9/5A3/3K1R3/5A3 b\n"
"9/R3k3c/5a3/9/9/9/9/4c4/5K3/5C3 b\n"
"3k5/7c1/4b4/9/9/2B6/9/2RRKA3/4A4/6B2 b\n"
"9/9/3k1a3/9/6b2/9/3Np4/3KB4/4N4/5A3 b\n"
"3k5/8C/9/9/7n1/9/9/3A1K2c/9/4NA3 b\n"
"3k5/2c6/3aR4/9/6b2/2B6/9/3K5/9/2B5p b\n"
"9/9/4k2P1/9/9/9/9/4Cp3/9/3ACK3 b\n"
"5R3/4a4/3k1a3/9/6PR1/9/9/5K2B/9/8r b\n"
"4Pa3/4k4/4ba3/9/9/9/3RP4/4K1n2/9/9 b\n"
"9/9/4k4/9/9/2R6/9/5K2c/6n2/6B2 b\n"
"2bRra3/9/4ka3/9/9/9/9/9/9/3ACKB2 b\n"
"3a1a3/8R/3k5/9/9/9/6p2/5K1p1/7p1/3A5 b\n"
"9/4a4/4ka3/9/9/2Bp4p/9/9/1N7/5K3 b\n"
"4P4/3Nk4/3a5/9/6b2/9/4p4/4BK3/4A4/9 b\n"
"5a3/4a4/3Nk4/9/9/9/6P2/9/5N3/3K3n1 b\n"
"9/9/5k3/9/2b3b2/5p3/9/3A1K3/4R4/5AB2 b\n"
"3C5/3kR4/5a3/9/9/9/9/4KA3/4A2n1/9 b\n"
"4P4/3k5/9/9/6b2/9/6P1R/4K4/3n5/9 b\n"
"9/4a4/3kRa3/2p5p/6b2/9/9/8B/9/4K1B2 b\n"
"9/9/5k3/9/9/9/4P4/B2AK4/4C4/6B1c b\n"
"9/5k3/5a2C/8p/6b2/9/9/9/5K3/3A1A2p b\n"
"9/4k4/5N3/9/9/9/8n/5K3/9/5AB2 b\n"
"3k5/3N5/5a3/9/9/9/9/9/9/2RKcA2N b\n"
"4k4/4a3N/5a3/9/8r/9/7R1/5A3/4K2p1/9 b\n"
"9/4a4/5k3/9/9/5p3/9/4BK3/4C4/3A2B2 b\n"
"9/9/4ka3/9/2b3b2/6B2/5R3/5K1R1/8n/6B2 b\n"
"9/3k5/4b4/8R/9/8R/9/8r/9/1n3K3 b\n"
"5k3/1R7/3a5/6p2/9/9/9/3K1n1n1/9/6B2 b\n"
"3P5/9/b2Ck4/9/9/9/4p4/4BK3/9/9 b\n"
"2b1C1b2/4ak3/9/9/9/9/9/5C3/9/3AKAn2 b\n"
"4P4/4k4/3Nb4/9/6b2/9/9/9/3K3n1/9 b\n"
"9/9/5k3/8p/9/9/9/3K5/4A4/3A1Rp1p b\n"
"9/3k5/3a5/9/9/8C/4P1P2/7n1/4K4/9 b\n"
"3aNk3/9/5aP2/9/9/9/9/4K4/4A4/3c1A3 b\n"
"3a5/4k4/5a3/9/9/9/9/5RC1c/3K1p3/9 b\n"
"4ka3/2P1a4/9/9/9/5C3/9/9/5K3/5R2r b\n"
"5k3/8P/9/9/9/9/4p1P2/B2A1Ap2/9/5KB2 b\n"
"3k1a3/5R3/5a3/9/8c/5p3/9/4KA3/4A4/6B2 b\n"
"2b2k3/4aN3/5a2N/9/6b2/9/5c3/5K3/9/5AB2 b\n"
"9/9/4k4/9/6b2/9/9/8B/4AC3/3AKRB1n b\n"
"9/9/4k4/9/4P4/9/8r/4KA3/5R3/9 b\n"
"9/9/5k3/9/9/9/6P2/9/4N3c/2RAKA3 b\n"
"9/9/4k4/9/9/9/6P1P/5A3/5K1p1/5A3 b\n"
"4k4/9/8P/9/9/9/Np7/5K2B/9/8c b\n"
"9/9/5k3/9/6b2/9/5cP2/5A3/5R3/5K3 b\n"
"9/3k5/c8/8C/9/9/9/3A1K2B/4A2N1/8c b\n"
"5k3/4R4/3a1a3/9/9/9/9/3ABK3/3R5/6B1r b\n"
"9/5k3/5c3/9/9/1R4B1c/9/4BK3/5N3/9 b\n"
"5k3/7c1/4R4/9/6b2/9/9/4K4/9/4R1B1c b\n"
"3aR1b2/3ka4/4b4/9/9/9/9/4K3B/3nA4/9 b\n"
"9/9/3k1a3/9/9/9/6p2/5K3/4ARR2/5A3 b\n"
"3aC1R2/9/4ka2b/9/9/9/9/5K2B/9/5c3 b\n"
"3P4c/9/b3k4/9/6b2/9/9/5N3/5R3/5K3 b\n"
"3aC4/5k3/4ba3/p8/9/9/9/5K3/9/3A1A2p b\n"
"4k4/4R4/3a1ac2/9/9/9/9/5A1pB/4K4/3A2B2 b\n"
"9/9/4k1P2/9/9/9/9/5K2p/4ANN2/9 b\n"
"9/9/4ka3/9/9/9/9/5N3/5K3/2B2RB1n b\n"
"4Ca3/4a4/4k4/9/2b3b2/9/9/9/4N4/3AKA3 b\n"
"9/9/5k3/9/9/9/9/9/4C4/2B1KABCc b\n"
"9/4aNR2/4ka3/7n1/9/9/9/9/9/5K3 b\n"
"9/4a4/3aNk3/9/9/2p5p/9/9/9/3KN4 b\n"
"9/9/3k5/9/9/9/3Rp4/3KB4/4N4/5R3 b\n"
"9/5k3/3ara3/9/9/9/9/3ABK3/3R5/6B2 b\n"
"3k5/4a4/4ba3/9/9/4R4/6R2/4K3B/4A2n1/9 b\n"
"9/9/5k3/9/8c/9/4P4/8B/9/3RK4 b\n"
"9/9/2P2k3/8p/9/9/9/3A5/3K5/4R1B1c b\n"
"3a1R3/9/4ka3/9/4p4/9/9/1n1K5/9/9 b\n"
"9/5k3/3aP4/5n3/9/9/9/3K1A3/9/1Np6 b\n"
"9/9/5k3/9/6b2/9/4p1P2/3ABA3/9/5K3 b\n"
"5k3/9/3a5/5C3/9/9/9/3R1K3/9/3r1R3 b\n"
"5k3/5N3/9/9/9/9/9/9/5K3/2C1n4 b\n"
"9/9/5k3/8N/9/9/9/5A3/4KR3/8r b\n"
"9/9/5k3/9/9/9/5Rp2/3A1K3/4R4/5A3 b\n"
"3k2b2/6R2/9/9/6b2/9/5n3/9/5K3/4nAB2 b\n"
"9/4P4/4k4/9/9/9/4Pp3/4KA3/9/9 b\n"
"9/9/3k5/9/7R1/4R4/6n2/5K3/9/9 b\n"
"5c3/9/4Rk3/9/9/9/9/5C3/5N3/5K3 b\n"
"9/4a4/3Rk4/2p6/4c4/2B6/9/3K5/9/3A2B2 b\n"
"4P4/4a4/4k4/9/9/9/9/4BRp2/9/4K4 b\n"
"4ka3/2r1a3N/9/9/9/9/1R7/9/6p2/3K5 b\n"
"9/4Nk3/4N4/8p/9/6B2/9/5Ap2/5K3/6B2 b\n"
"9/5k3/4bN2b/p7N/9/6Bp1/9/9/9/5K3 b\n"
"5kb2/9/4b4/9/8r/6P2/9/3K4B/8R/6B2 b\n"
"9/4k4/b3N3b/9/9/9/9/5KCp1/6C2/9 b\n"
"9/3k5/8C/9/9/6B2/9/4BK3/4AC3/5A2n b\n"
"9/9/4k3b/9/9/9/p8/2RA1K1p1/7p1/9 b\n"
"3c5/4a4/4ka3/9/9/9/6Rp1/5K3/9/8R b\n"
"4P4/4a4/4k4/9/9/9/4p4/3CBK3/9/9 b\n"
"9/3k5/3a5/9/9/4R4/7rn/3K5/9/4RA3 b\n"
"3ak4/5N3/3a4b/9/9/9/9/n8/p8/4NK3 b\n"
"3a1a3/5kC2/8R/6p2/6b2/9/9/8p/9/4K4 b\n"
"9/9/5k3/9/2b3b2/9/9/4KA3/4NN3/6c2 b\n"
"9/9/4k4/9/8P/6B2/4n3P/5K1R1/9/5AB2 b\n"
"9/4kC3/5a3/8p/9/9/9/3A1A3/9/5KRp1 b\n"
"9/3ka4/4ba3/9/9/3p3R1/9/3KB4/9/7n1 b\n"
"5P3/9/5k2b/9/9/9/9/3AK1C2/7n1/5A1C1 b\n"
"9/9/4bk3/6p2/2b6/2P3C2/9/5A1p1/9/5K3 b\n"
"3k1a3/9/9/9/9/9/9/8B/9/2RpcKB2 b\n"
"9/9/4k4/9/9/9/9/5K2B/4NNC2/8c b\n"
"3R1P3/4a4/P4kc1P/9/9/9/9/9/9/3K5 b\n"
"C5b2/4a4/3k1a3/P8/9/8P/9/5K3/9/3c5 b\n"
"9/5P3/5k3/9/9/9/4Pp3/4K4/9/9 b\n"
"9/8R/5k3/9/9/9/9/4BAp1B/9/3ACK3 b\n"
"9/2c6/3k5/9/2b3b2/9/9/3AKR3/4R4/9 b\n"
"9/9/4k3P/p8/9/9/4Pp3/9/9/3K4C b\n"
"4k4/4aC3/3a3P1/p8/9/9/9/5Ap2/9/3K5 b\n"
"3aPa3/5k3/9/9/2b3b2/9/9/B4R3/3K5/6n2 b\n"
"9/4Rk3/5a3/6p1p/6b2/9/9/9/4K4/6B2 b\n"
"9/9/5k3/9/4P1b1c/9/4P4/9/4N4/3AK4 b\n"
"3aCa3/4Rk3/4N4/9/9/9/9/9/4K4/8n b\n"
"9/9/4ka3/9/2b3b2/9/8P/9/5R3/2B2KBp1 b\n"
"3k5/3Na4/9/9/9/4C4/6n2/5K3/4A2C1/9 b\n"
"6N2/3k5/3a5/9/9/9/8R/9/4N3r/3AK4 b\n"
"6b2/3k5/5a3/5P3/9/9/9/8r/9/5KR2 b\n"
"3k5/3N5/5a3/9/9/9/9/9/9/2RKcA2R b\n"
"3R5/4a4/5k3/4p4/9/9/9/5A3/5K3/7Cn b\n"
"9/9/4k4/9/8C/9/4n3P/5K3/9/5AB2 b\n"
"9/9/3k5/N8/9/4p4/5N3/4K4/2c1A4/9 b\n"
"5a3/5R3/3k1a3/9/9/9/4P4/4R4/5c3/5K3 b\n"
"2b1Ra3/3ka4/3N4b/1n7/9/9/9/9/9/3K1A3 b\n"
"9/9/4k4/9/9/9/2P3P1P/5A3/5K1p1/5A3 b\n"
"5k3/5N3/9/9/2b3b2/9/9/5A3/7n1/3AK1B2 b\n"
"9/9/5k3/9/5P3/6B2/4P4/7R1/3K5/2rA2B2 b\n"
"9/4k4/4Na3/9/9/9/9/6R2/8n/5K1R1 b\n"
"5c3/9/b2Rk4/9/2b6/9/9/4B3p/9/4KA3 b\n"
"4C1b2/4k3N/5a2b/9/9/9/9/3A1K3/9/2c2A3 b\n"
"4k4/4aN3/4b4/8p/9/9/9/9/5K3/9 b\n"
"3kNa3/4a4/9/9/6b2/6B2/9/5K1C1/8n/6B2 b\n"
"3k5/3N5/5a3/9/9/9/9/9/4A4/2CKcAB2 b\n"
"5Nb2/5k3/3a5/1R7/9/9/4n4/3K5/9/9 b\n"
"9/9/5k3/9/9/9/5pP1P/9/4CK3/3R5 b\n"
"6b1R/7cP/4bk3/9/9/9/9/5n3/4K4/9 b\n"
"9/4a4/4ka3/9/9/R8/9/B2K5/6n2/8n b\n"
"4k4/4a4/4P4/9/6b2/9/4p1P2/4BA3/9/3K5 b\n"
"9/9/5k3/9/6b2/6B2/9/5C3/9/3AK1n2 b\n"
"9/9/3ak4/4p3p/9/9/9/8C/5K3/6p2 b\n"
"8C/9/3k1a3/9/9/9/9/B4K2B/9/8c b\n"
"9/4a4/5k3/9/9/5p3/9/4BK3/4N4/3R2B2 b\n"
"9/9/3a1k3/9/6b2/5p3/9/4BK3/4R4/5R3 b\n"
"3a1k3/9/5a3/9/9/8p/4p4/4BA3/4NK3/9 b\n"
"3a1CR2/4k4/5a2b/9/9/9/4p4/4BK3/4A4/9 b\n"
"9/9/5k3/9/9/9/4R4/3K5/9/8r b\n"
"9/9/4bk3/8p/9/9/5Cp2/3A1K3/9/3A1N3 b\n"
"5P3/4a4/4ka2P/9/6b2/9/6P2/9/4N4/3AKA3 b\n"
"2b1Ra3/4a4/3k5/9/6b2/9/9/9/9/4K2n1 b\n"
"9/9/4k4/9/6b2/9/6P2/4BRp2/9/4K4 b\n"
"5a3/9/3k1a3/9/9/9/6P2/3N4B/3K5/4c4 b\n"
"9/9/5k2b/9/9/1pB6/5Rp2/3A1K3/9/6B2 b\n"
"3a5/4a4/4k4/p8/9/9/5p3/5Ap1B/3K3N1/3A5 b\n";
            buffer = (BYTE*)EndGames;
            length = strlen(EndGames);
        }
        std::vector<std::string> fens;
        std::string current;
        for (size_t i = 0; i < length; ++i) {
            if (buffer[i] == '\n' || i == length - 1) {
                if (i == length - 1 && buffer[i] != '\n') {
                    current += buffer[i];
                }
                if (!current.empty()) {
                    fens.push_back(current);
                    current.clear();
                }
            }
            else {
                current += buffer[i];
            }
        }

        if (fens.empty()) {
            return init;
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, fens.size() - 1);
        std::string fen = fens[dis(gen)];

        std::istringstream fenStream(fen);
        std::string boardPart;
        std::getline(fenStream, boardPart, ' ');
        std::string sidePart;
        std::getline(fenStream, sidePart); 

        std::istringstream rowStream(boardPart);
        std::string row;
        int r = 0;
        while (std::getline(rowStream, row, '/') && r < 10) {
            int c = 0;
            for (char ch : row) {
                if (c >= 9) break;
                if (std::isdigit(ch)) {
                    c += (ch - '0');
                }
                else {
                    int piece;
                    switch (ch) {
                    case 'k': piece = BK_JIANG; break;
                    case 'a': piece = BK_SHI; break;
                    case 'b': piece = BK_XIANG; break;
                    case 'n': piece = BK_MA; break;
                    case 'r': piece = BK_JU; break;
                    case 'c': piece = BK_PAO; break;
                    case 'p': piece = BK_BING; break;
                    case 'K': piece = RD_SHUAI; break;
                    case 'A': piece = RD_SHI; break;
                    case 'B': piece = RD_XIANG; break;
                    case 'N': piece = RD_MA; break;
                    case 'R': piece = RD_JU; break;
                    case 'C': piece = RD_PAO; break;
                    case 'P': piece = RD_BING; break;
                    default: delete[] board; throw std::runtime_error("Invalid FEN character: " + std::string(1, ch));
                    }
                    board[r][c] = piece;
                    c++;
                }
            }
            if (c != 9) {
                delete[] board;
                throw std::runtime_error("Invalid FEN row length in: " + fen);
            }
            r++;
        }
        if (r != 10) {
            return init;
        }

        if (sidePart.empty() || (sidePart[0] != 'b' && sidePart.substr(0, 1) != "b")) {
            return init;
        }

        return board;
    }

    void setEndGame(int(&board)[10][9]) {
        for (int r = 0; r < 10; ++r) for (int c = 0; c < 9; ++c) g[r][c] = board[r][c];
        sideToMove = BLACK;
        updateZobristKey();
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

    static void calcLayout();
    static void draw(HDC hdc);
    static void newGame();
    static void aiStep(bool first = false);
    static void playerClick(int x, int y);
    static bool isPlayerPiece(int p) { return IsBlack(p); }
    static void endWithMessage(const char* msg);
    static void undoRound();
    static void endGame();
};
#pragma warning(pop)