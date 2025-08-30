#pragma once
#include <vector>
#include <random>
#include <chrono>
#include <functional>
#include <FA2PP.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Helpers/MultimapHelper.h"
#include "../../Helpers/STDHelpers.h"

class CBrickBreaker
{
public:
    static void Create(CFinalSunDlg* pWnd);
    static HWND GetHandle() { return CBrickBreaker::m_hwnd; }

protected:
    static void Initialize(HWND& hWnd);
    static void Update();  
    static void Render(HDC hdc); 
    static void Close(HWND& hWnd);
    static void ShowVictoryDialog();
    static void RestrictCursor();

    static void ResetGame(bool newSeed = true);
    static void ResetRound();

    static void CreateBackBuffer(int cx, int cy);
    static void DestroyBackBuffer();

    static void OnSize(int cx, int cy);
    static void OnMouseMove(int x, int y);
    static void OnKeyDown(WPARAM vk);
    static void PlaySoundA();

    static BOOL CALLBACK DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static bool initialized;

    static HBITMAP s_hbmp;
    static HDC     s_memdc;
    static int     s_bbCX, s_bbCY;

    struct Brick {
        RECT rc{};
        int  hp = 0;   
    };
    struct Ball {
        double x = 0, y = 0; 
        double vx = 0, vy = 0;  
        double r = 6.0; 
        bool   alive = true;
        int    hitCount = 0; 
    };

    static constexpr int GRID_ROWS = 20;
    static constexpr int GRID_COLS = 14;
    static constexpr int GRID_EMPTY_ROWS = 4;
    static std::vector<Brick> s_bricks;
    static int s_gridLeft, s_gridTop, s_gridRight, s_gridBottom;
    static int s_cellW, s_cellH;

    static RECT   s_paddle;
    static int    s_paddleW;
    static int    s_paddleH;
    static double s_paddleSpeed; 
    static std::vector<Ball> s_balls;
    static double s_ballSpeed; 
    static bool   s_roundActive;

    static int  s_splitThreshold; 
    static std::mt19937_64 s_rng;
    static UINT_PTR s_timerID;
    static std::chrono::steady_clock::time_point s_lastTick;

    static void GenerateLevel();
    static void EnsureSolvable(); 
    static Brick& BrickAt(int r, int c);
    static void ForEachBrick(const std::function<void(Brick&)>& fn);

    static void Step(double dt);
    static void CollideBallWithWorld(Ball& b);
    static void CollideBallWithBricks(Ball& b);
    static void CollideBallWithPaddle(Ball& b);

    static void StartTimer();
    static void StopTimer();

    static COLORREF ColorFor(const Brick& br);
};

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
};