#pragma once
#include <vector>
#include <random>
#include <chrono>
#include <functional>
#include <FA2PP.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Helpers/MultimapHelper.h"
#include "../../Helpers/STDHelpers.h"

// һ����̬�Ի����ࣺש���ƻ���
class CNewEasterEgg
{
public:
    static void Create(CFinalSunDlg* pWnd);
    static HWND GetHandle() { return CNewEasterEgg::m_hwnd; }

protected:
    static void Initialize(HWND& hWnd);
    static void Update();        // �߼����£���ʱ����
    static void Render(HDC hdc); // ���Ƶ��ṩ�� HDC��˫���壩
    static void Close(HWND& hWnd);
    static void ShowVictoryDialog(); // ��ʾʤ���Ի���
    static void RestrictCursor();    // ��������굽����

    static void ResetGame(bool newSeed = true);
    static void ResetRound(); // ��������ñ��غϣ�����ؿ���

    static void CreateBackBuffer(int cx, int cy);
    static void DestroyBackBuffer();

    static void OnSize(int cx, int cy);
    static void OnMouseMove(int x, int y);
    static void OnKeyDown(WPARAM vk);
    static void PlaySoundA();

    static BOOL CALLBACK DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

private:
    // ---- �������� ----
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static bool initialized;

    // ---- ˫���� ----
    static HBITMAP s_hbmp;
    static HDC     s_memdc;
    static int     s_bbCX, s_bbCY;

    // ---- ��Ϸ���� ----
    struct Brick {
        RECT rc{};
        int  hp = 0;     // 0=�գ�1=��ͨ��ǳ����1������2=��Ӳ��������2������-1=ǽ����ң������ƣ�
    };
    struct Ball {
        double x = 0, y = 0;     // �������꣨���أ�
        double vx = 0, vy = 0;   // �ٶȣ�����/�룩
        double r = 6.0;        // �뾶
        bool   alive = true;
        int    hitCount = 0;   // ÿ��Ļ������
    };

    // �ؿ�����
    static constexpr int GRID_ROWS = 20;
    static constexpr int GRID_COLS = 14;
    static constexpr int GRID_EMPTY_ROWS = 4;
    static std::vector<Brick> s_bricks;
    static int s_gridLeft, s_gridTop, s_gridRight, s_gridBottom;
    static int s_cellW, s_cellH;

    // ��������
    static RECT   s_paddle;
    static int    s_paddleW;
    static int    s_paddleH;
    static double s_paddleSpeed; // �����ƶ��ٶ�
    static std::vector<Ball> s_balls;
    static double s_ballSpeed;   // �̶����ʣ�����仯
    static bool   s_roundActive;

    // ������ֵ
    static int  s_splitThreshold;     // �ﵽ��ֵ����

    // ���
    static std::mt19937_64 s_rng;

    // ��ʱ
    static UINT_PTR s_timerID;
    static std::chrono::steady_clock::time_point s_lastTick;

    // ---- �ڲ��߼� ----
    static void GenerateLevel();
    static void EnsureSolvable(); // ȷ����ͨ/��Ӳש�鲻������ǽ��Χ
    static Brick& BrickAt(int r, int c);
    static void ForEachBrick(const std::function<void(Brick&)>& fn);

    static void Step(double dt);
    static void CollideBallWithWorld(Ball& b);
    static void CollideBallWithBricks(Ball& b);
    static void CollideBallWithPaddle(Ball& b);

    static void StartTimer();
    static void StopTimer();

    // ����
    static COLORREF ColorFor(const Brick& br);
};