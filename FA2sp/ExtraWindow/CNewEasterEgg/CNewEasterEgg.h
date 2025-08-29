#pragma once
#include <vector>
#include <random>
#include <chrono>
#include <functional>
#include <FA2PP.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Helpers/MultimapHelper.h"
#include "../../Helpers/STDHelpers.h"

// 一个静态对话框类：砖块破坏者
class CNewEasterEgg
{
public:
    static void Create(CFinalSunDlg* pWnd);
    static HWND GetHandle() { return CNewEasterEgg::m_hwnd; }

protected:
    static void Initialize(HWND& hWnd);
    static void Update();        // 逻辑更新（定时器）
    static void Render(HDC hdc); // 绘制到提供的 HDC（双缓冲）
    static void Close(HWND& hWnd);
    static void ShowVictoryDialog(); // 显示胜利对话框
    static void RestrictCursor();    // 限制鼠标光标到窗口

    static void ResetGame(bool newSeed = true);
    static void ResetRound(); // 丢球后重置本回合（不变关卡）

    static void CreateBackBuffer(int cx, int cy);
    static void DestroyBackBuffer();

    static void OnSize(int cx, int cy);
    static void OnMouseMove(int x, int y);
    static void OnKeyDown(WPARAM vk);
    static void PlaySoundA();

    static BOOL CALLBACK DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

private:
    // ---- 基本窗口 ----
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static bool initialized;

    // ---- 双缓冲 ----
    static HBITMAP s_hbmp;
    static HDC     s_memdc;
    static int     s_bbCX, s_bbCY;

    // ---- 游戏对象 ----
    struct Brick {
        RECT rc{};
        int  hp = 0;     // 0=空；1=普通（浅蓝，1击）；2=坚硬（深蓝，2击）；-1=墙（深灰，不可破）
    };
    struct Ball {
        double x = 0, y = 0;     // 中心坐标（像素）
        double vx = 0, vy = 0;   // 速度（像素/秒）
        double r = 6.0;        // 半径
        bool   alive = true;
        int    hitCount = 0;   // 每球的击碎计数
    };

    // 关卡网格
    static constexpr int GRID_ROWS = 20;
    static constexpr int GRID_COLS = 14;
    static constexpr int GRID_EMPTY_ROWS = 4;
    static std::vector<Brick> s_bricks;
    static int s_gridLeft, s_gridTop, s_gridRight, s_gridBottom;
    static int s_cellW, s_cellH;

    // 挡板与球
    static RECT   s_paddle;
    static int    s_paddleW;
    static int    s_paddleH;
    static double s_paddleSpeed; // 键盘移动速度
    static std::vector<Ball> s_balls;
    static double s_ballSpeed;   // 固定速率，方向变化
    static bool   s_roundActive;

    // 分裂阈值
    static int  s_splitThreshold;     // 达到阈值分裂

    // 随机
    static std::mt19937_64 s_rng;

    // 计时
    static UINT_PTR s_timerID;
    static std::chrono::steady_clock::time_point s_lastTick;

    // ---- 内部逻辑 ----
    static void GenerateLevel();
    static void EnsureSolvable(); // 确保普通/坚硬砖块不被四面墙包围
    static Brick& BrickAt(int r, int c);
    static void ForEachBrick(const std::function<void(Brick&)>& fn);

    static void Step(double dt);
    static void CollideBallWithWorld(Ball& b);
    static void CollideBallWithBricks(Ball& b);
    static void CollideBallWithPaddle(Ball& b);

    static void StartTimer();
    static void StopTimer();

    // 工具
    static COLORREF ColorFor(const Brick& br);
};