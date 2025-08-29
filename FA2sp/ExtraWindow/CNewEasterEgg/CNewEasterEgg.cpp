#include "CNewEasterEgg.h"
#include "../../FA2sp/FA2sp.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <windowsx.h>
#include <mmsystem.h>
#include "../../Helpers/Translations.h"
#pragma comment(lib, "winmm")

HWND CNewEasterEgg::m_hwnd = nullptr;
CFinalSunDlg* CNewEasterEgg::m_parent = nullptr;
bool CNewEasterEgg::initialized = false;
HBITMAP CNewEasterEgg::s_hbmp = nullptr;
HDC     CNewEasterEgg::s_memdc = nullptr;
int     CNewEasterEgg::s_bbCX = 0, CNewEasterEgg::s_bbCY = 0;
std::vector<CNewEasterEgg::Brick> CNewEasterEgg::s_bricks;
int CNewEasterEgg::s_gridLeft = 20, CNewEasterEgg::s_gridTop = 20;
int CNewEasterEgg::s_gridRight = 0, CNewEasterEgg::s_gridBottom = 0;
int CNewEasterEgg::s_cellW = 0, CNewEasterEgg::s_cellH = 0;
RECT   CNewEasterEgg::s_paddle = { 0,0,0,0 };
int    CNewEasterEgg::s_paddleW = 120;
int    CNewEasterEgg::s_paddleH = 14;
double CNewEasterEgg::s_paddleSpeed = 580.0;
std::vector<CNewEasterEgg::Ball> CNewEasterEgg::s_balls;
double CNewEasterEgg::s_ballSpeed = 360.0;
bool   CNewEasterEgg::s_roundActive = false;
int  CNewEasterEgg::s_splitThreshold = 4;
std::mt19937_64 CNewEasterEgg::s_rng{ std::random_device{}() };
UINT_PTR CNewEasterEgg::s_timerID = 0;
std::chrono::steady_clock::time_point CNewEasterEgg::s_lastTick;

constexpr double M_PI = 3.14159265358979323846;

void CNewEasterEgg::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        reinterpret_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(326),
        pWnd ? pWnd->MyViewFrame.GetSafeHwnd() : nullptr,
        CNewEasterEgg::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else {
        Logger::Error("Failed to create CNewEasterEgg.\n");
        m_parent = nullptr;
    }
}

void CNewEasterEgg::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("EasterEggTitle", buffer))
        SetWindowText(hWnd, buffer);

    RECT rc{}; GetClientRect(hWnd, &rc);
    OnSize(rc.right - rc.left, rc.bottom - rc.top);

    ResetGame(true);
    StartTimer();
    RestrictCursor();
    initialized = true;
}

void CNewEasterEgg::RestrictCursor()
{
    if (!m_hwnd) return;
    RECT rc;
    GetClientRect(m_hwnd, &rc);
    POINT pt1 = { rc.left, rc.top };
    POINT pt2 = { rc.right, rc.bottom };
    ClientToScreen(m_hwnd, &pt1);
    ClientToScreen(m_hwnd, &pt2);
    rc.left = pt1.x;
    rc.top = pt1.y;
    rc.right = pt2.x;
    rc.bottom = pt2.y;
    ClipCursor(&rc);
}

void CNewEasterEgg::Close(HWND& hWnd)
{
    ClipCursor(nullptr);
    StopTimer();
    DestroyBackBuffer();
    EndDialog(hWnd, 0);
    m_hwnd = nullptr;
    m_parent = nullptr;
    initialized = false;
}


void CNewEasterEgg::ShowVictoryDialog()
{
    StopTimer();
    s_roundActive = false;
    ClipCursor(nullptr);
    PlaySoundW(MAKEINTRESOURCEW(2004), reinterpret_cast<HINSTANCE>(FA2sp::hInstance), SND_ASYNC | SND_RESOURCE);
    int result = MessageBox(
        m_hwnd,
        Translations::TranslateOrDefault("EasterEggWin", "Congratulations! You cleared all bricks!\nWould you like to play again?"),
        Translations::TranslateOrDefault("EasterEggWinTitle", "Victory!"),
        MB_YESNO | MB_ICONINFORMATION
    );
    if (result == IDYES) {
        ResetGame(true);
        StartTimer();
        RestrictCursor(); 
    }
    else {
        Close(m_hwnd); 
    }
}

void CNewEasterEgg::Update()
{
    if (!initialized) return;

    auto now = std::chrono::steady_clock::now();
    double dt = 0.0;
    if (s_lastTick.time_since_epoch().count() != 0) {
        dt = std::chrono::duration<double>(now - s_lastTick).count();
        dt = std::min(dt, 0.03);
    }
    s_lastTick = now;

    Step(dt);
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void CNewEasterEgg::Render(HDC hdc)
{
    if (!s_memdc) return;

    HBRUSH bk = CreateSolidBrush(RGB(18, 18, 18));
    RECT rc{}; rc.right = s_bbCX; rc.bottom = s_bbCY;
    FillRect(s_memdc, &rc, bk); DeleteObject(bk);

    RECT gridRC{ s_gridLeft, s_gridTop, s_gridRight, s_gridBottom };
    HBRUSH gridBk = CreateSolidBrush(RGB(28, 28, 28));
    FillRect(s_memdc, &gridRC, gridBk); DeleteObject(gridBk);

    for (auto& br : s_bricks) {
        if (br.hp == 0) continue;
        HBRUSH b = CreateSolidBrush(ColorFor(br));
        FillRect(s_memdc, &br.rc, b);
        DeleteObject(b);

        HPEN pen = CreatePen(PS_SOLID, 1, RGB(230, 230, 230));
        HGDIOBJ oldp = SelectObject(s_memdc, pen);
        MoveToEx(s_memdc, br.rc.left, br.rc.top, nullptr);
        LineTo(s_memdc, br.rc.right - 1, br.rc.top);
        LineTo(s_memdc, br.rc.right - 1, br.rc.bottom - 1);
        LineTo(s_memdc, br.rc.left, br.rc.bottom - 1);
        LineTo(s_memdc, br.rc.left, br.rc.top);
        SelectObject(s_memdc, oldp);
        DeleteObject(pen);
    }

    HBRUSH pb = CreateSolidBrush(RGB(240, 200, 64));
    FillRect(s_memdc, &s_paddle, pb);
    DeleteObject(pb);

    HBRUSH wb = CreateSolidBrush(RGB(240, 240, 240));
    HGDIOBJ oldb = SelectObject(s_memdc, wb);
    HPEN nop = CreatePen(PS_NULL, 0, 0);
    HGDIOBJ oldp = SelectObject(s_memdc, nop);
    for (auto& b : s_balls) if (b.alive) {
        int l = int(std::round(b.x - b.r));
        int t = int(std::round(b.y - b.r));
        int r = int(std::round(b.x + b.r));
        int btm = int(std::round(b.y + b.r));
        Ellipse(s_memdc, l, t, r, btm);
    }
    SelectObject(s_memdc, oldp); DeleteObject(nop);
    SelectObject(s_memdc, oldb); DeleteObject(wb);

    SetBkMode(s_memdc, TRANSPARENT);
    SetTextColor(s_memdc, RGB(220, 220, 220));
    std::string hud = Translations::TranslateOrDefault("EasterEggDesc", "You found the easter egg :-)      Press ESC to exit the game.");
    TextOut(s_memdc, 10, 2, hud.c_str(), (int)hud.size());

    BitBlt(hdc, 0, 0, s_bbCX, s_bbCY, s_memdc, 0, 0, SRCCOPY);
}

BOOL CALLBACK CNewEasterEgg::DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
        m_hwnd = hwnd;
        Initialize(hwnd);
        return TRUE;

    case WM_SIZE:
        OnSize(LOWORD(wParam) ? LOWORD(lParam) : LOWORD(lParam), HIWORD(lParam));
        RestrictCursor(); 
        return TRUE;

    case WM_TIMER:
        if (wParam == s_timerID) Update();
        return TRUE;

    case WM_MOUSEMOVE:
        OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        return TRUE;

    case WM_KEYDOWN:
        OnKeyDown(wParam);
        return TRUE;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (s_memdc) Render(hdc);
        EndPaint(hwnd, &ps);
        return TRUE;
    }

    case WM_ERASEBKGND:
        return TRUE;

    case WM_CLOSE:
        Close(hwnd);
        return TRUE;

    case WM_ACTIVATE:
        if (LOWORD(wParam) != WA_INACTIVE) {
            RestrictCursor();
        }
        else {
            ClipCursor(nullptr);
        }
        return TRUE;
    }
    return FALSE;
}

void CNewEasterEgg::CreateBackBuffer(int cx, int cy)
{
    DestroyBackBuffer();
    HDC hdc = GetDC(m_hwnd);
    s_memdc = CreateCompatibleDC(hdc);
    s_hbmp = CreateCompatibleBitmap(hdc, cx, cy);
    SelectObject(s_memdc, s_hbmp);
    ReleaseDC(m_hwnd, hdc);
    s_bbCX = cx; s_bbCY = cy;
}

void CNewEasterEgg::DestroyBackBuffer()
{
    if (s_memdc) { DeleteDC(s_memdc); s_memdc = nullptr; }
    if (s_hbmp) { DeleteObject(s_hbmp); s_hbmp = nullptr; }
    s_bbCX = s_bbCY = 0;
}

void CNewEasterEgg::OnSize(int cx, int cy)
{
    if (cx <= 0 || cy <= 0) return;
    CreateBackBuffer(cx, cy);

    s_gridLeft = 20; s_gridRight = cx - 20;
    s_gridTop = 20; s_gridBottom = cy - 80;
    s_cellW = (s_gridRight - s_gridLeft) / GRID_COLS;
    s_cellH = (s_gridBottom - s_gridTop) / GRID_ROWS;

    int px = cx / 2;
    int py = cy - 40;
    s_paddleW = std::max(80, cx / 7);
    s_paddleH = 14;
    s_paddle.left = px - s_paddleW / 2;
    s_paddle.right = s_paddle.left + s_paddleW;
    s_paddle.top = py - s_paddleH / 2;
    s_paddle.bottom = s_paddle.top + s_paddleH;
}

void CNewEasterEgg::ResetGame(bool newSeed)
{
    if (newSeed) s_rng.seed(std::random_device{}());
    GenerateLevel();
    s_roundActive = true;

    s_balls.clear();
    Ball b;
    b.r = 7.0;
    b.x = (s_paddle.left + s_paddle.right) * 0.5;
    b.y = s_paddle.top - b.r - 2;
    b.vx = 0;
    b.vy = -s_ballSpeed;
    b.hitCount = 0; 
    s_balls.push_back(b);
}

void CNewEasterEgg::ResetRound()
{
    s_roundActive = true;
    s_balls.clear();

    Ball b;
    b.r = 7.0;
    b.x = (s_paddle.left + s_paddle.right) * 0.5;
    b.y = s_paddle.top - b.r - 2;
    b.hitCount = 0;
    std::uniform_real_distribution<double> d(-0.4, 0.4);
    double angle = -M_PI / 2 + d(s_rng) * 0.35; 
    b.vx = std::cos(angle) * s_ballSpeed;
    b.vy = std::sin(angle) * s_ballSpeed;
    s_balls.push_back(b);
}

// ---- ¹Ø¿¨ ----
CNewEasterEgg::Brick& CNewEasterEgg::BrickAt(int r, int c)
{
    return s_bricks[r * GRID_COLS + c];
}

void CNewEasterEgg::ForEachBrick(const std::function<void(Brick&)>& fn)
{
    for (auto& br : s_bricks) fn(br);
}

void CNewEasterEgg::GenerateLevel()
{
    s_bricks.assign(GRID_ROWS * GRID_COLS, Brick{});

    for (int r = 0; r < GRID_ROWS; ++r) {
        for (int c = 0; c < GRID_COLS; ++c) {
            Brick& br = BrickAt(r, c);
            br.rc.left = s_gridLeft + c * s_cellW;
            br.rc.right = s_gridLeft + (c + 1) * s_cellW;
            br.rc.top = s_gridTop + r * s_cellH;
            br.rc.bottom = s_gridTop + (r + 1) * s_cellH;
            br.hp = 0;
        }
    }

    std::uniform_real_distribution<double> prob(0.0, 1.0);

    for (int r = 0; r < GRID_ROWS; ++r) {
        for (int c = 0; c < GRID_COLS; ++c) {
            Brick& br = BrickAt(r, c);
            double p = (r < 4) ? 0.78 : 0.55;

            double x = prob(s_rng);
            if (x < p) {
                if (prob(s_rng) < 0.25) br.hp = 2; 
                else br.hp = 1;
            }
            else {
                br.hp = 0;
            }

            if (prob(s_rng) < 0.06 && r > 0) {
                br.hp = -1; 
            }
        }
    }

    for (int r = GRID_ROWS - GRID_EMPTY_ROWS; r < GRID_ROWS; ++r) {
        if (r < 0) continue;
        for (int c = 0; c < GRID_COLS; ++c) {
            BrickAt(r, c).hp = 0;
        }
    }

    EnsureSolvable();

    s_gridRight = s_gridLeft + GRID_COLS * s_cellW;
    s_gridBottom = s_gridTop + GRID_ROWS * s_cellH;
}

void CNewEasterEgg::EnsureSolvable()
{
    auto isWall = [&](int r, int c)->bool {
        if (r < 0 || r >= GRID_ROWS || c < 0 || c >= GRID_COLS) return true;
        return BrickAt(r, c).hp == -1;
    };
    auto isBrick = [&](int r, int c)->bool {
        int hp = BrickAt(r, c).hp;
        return hp == 1 || hp == 2;
    };

    for (int r = 0; r < GRID_ROWS; ++r) {
        for (int c = 0; c < GRID_COLS; ++c) {
            if (!isBrick(r, c)) continue;

            bool up = isWall(r - 1, c), dn = isWall(r + 1, c), lf = isWall(r, c - 1), rt = isWall(r, c + 1);
            if (up && dn && lf && rt) {
                std::vector<std::pair<int, int>> cand = { {r - 1,c},{r + 1,c},{r,c - 1},{r,c + 1} };
                std::shuffle(cand.begin(), cand.end(), s_rng);
                for (auto [rr, cc] : cand) {
                    if (rr < 0 || rr >= GRID_ROWS || cc < 0 || cc >= GRID_COLS) continue;
                    BrickAt(rr, cc).hp = 1; 
                    break;
                }
            }
        }
    }
}

static inline double clampd(double v, double lo, double hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

void CNewEasterEgg::Step(double dt)
{
    if (!s_roundActive) return;

    for (auto& b : s_balls) if (b.alive) {
        b.x += b.vx * dt;
        b.y += b.vy * dt;

        CollideBallWithWorld(b);
        CollideBallWithBricks(b);
        CollideBallWithPaddle(b);
    }

    int aliveCount = 0;
    for (auto& b : s_balls) if (b.alive) ++aliveCount;
    if (aliveCount == 0) {
        ResetRound();
    }

    bool anyBreakable = false;
    for (auto& br : s_bricks) if (br.hp == 1 || br.hp == 2) { anyBreakable = true; break; }
    if (!anyBreakable) {
        ShowVictoryDialog();
    }
}

void CNewEasterEgg::CollideBallWithWorld(Ball& b)
{
    double L = 0, R = s_bbCX, T = 0, B = s_bbCY;
    bool playSound = false;
    if (b.x - b.r < L) { b.x = L + b.r; b.vx = std::abs(b.vx); playSound = true; }
    if (b.x + b.r > R) { b.x = R - b.r; b.vx = -std::abs(b.vx); playSound = true; }
    if (b.y - b.r < T) { b.y = T + b.r; b.vy = std::abs(b.vy); playSound = true; }
    if (b.y - b.r > B) { b.alive = false; }
    if (playSound) {
        PlaySoundA();
    }
}

void CNewEasterEgg::CollideBallWithBricks(Ball& b)
{
    struct Collision {
        Brick* brick;
        double overlap;
        enum { LEFT, RIGHT, TOP, BOTTOM } side;
        double time;
    };
    std::vector<Collision> collisions;

    double prevX = b.x - b.vx * 0.016;
    double prevY = b.y - b.vy * 0.016;

    for (auto& br : s_bricks) {
        if (br.hp == 0) continue;

        RECT brRect = br.rc;
        brRect.left -= (int)b.r;
        brRect.right += (int)b.r;
        brRect.top -= (int)b.r;
        brRect.bottom += (int)b.r;

        double nx = clampd(b.x, br.rc.left, br.rc.right);
        double ny = clampd(b.y, br.rc.top, br.rc.bottom);
        double dx = b.x - nx;
        double dy = b.y - ny;
        double d2 = dx * dx + dy * dy;
        if (d2 > b.r * b.r) continue;

        double overlapL = (b.x + b.r) - br.rc.left;
        double overlapR = br.rc.right - (b.x - b.r);
        double overlapT = (b.y + b.r) - br.rc.top;
        double overlapB = br.rc.bottom - (b.y - b.r);

        double minOverlap = std::min(std::min(overlapL, overlapR), std::min(overlapT, overlapB));
        Collision col;
        col.brick = &br;
        col.overlap = minOverlap;

        if (minOverlap == overlapL && b.vx > 0) col.side = Collision::LEFT;
        else if (minOverlap == overlapR && b.vx < 0) col.side = Collision::RIGHT;
        else if (minOverlap == overlapT && b.vy > 0) col.side = Collision::TOP;
        else if (minOverlap == overlapB && b.vy < 0) col.side = Collision::BOTTOM;
        else continue;

        double speed = std::max(std::abs(b.vx), std::abs(b.vy));
        double time = speed > 0 ? minOverlap / speed : minOverlap;
        col.time = time;
        collisions.push_back(col);
    }

    if (collisions.empty()) return;

    auto minCol = std::min_element(collisions.begin(), collisions.end(),
        [](const Collision& a, const Collision& b) { return a.time < b.time; });
    Brick& br = *minCol->brick;

    switch (minCol->side) {
    case Collision::LEFT:
        b.x = br.rc.left - b.r; b.vx = -std::abs(b.vx); break;
    case Collision::RIGHT:
        b.x = br.rc.right + b.r; b.vx = std::abs(b.vx); break;
    case Collision::TOP:
        b.y = br.rc.top - b.r; b.vy = -std::abs(b.vy); break;
    case Collision::BOTTOM:
        b.y = br.rc.bottom + b.r; b.vy = std::abs(b.vy); break;
    }

    if (br.hp == -1) {
        PlaySoundA();
    }
    else if (br.hp == 2) {
        const_cast<Brick&>(br).hp = 1;
        PlaySoundA();
    }
    else if (br.hp == 1) {
        const_cast<Brick&>(br).hp = 0;
        PlaySoundA();
        b.hitCount++;

        if (b.hitCount >= s_splitThreshold) {
            b.hitCount = 0;
            Ball nb = b;
            double ang = std::atan2(b.vy, b.vx);
            double delta = 8.0 * M_PI / 180.0; 
            ang += (std::signbit(ang) ? delta : -delta);
            nb.vx = std::cos(ang) * s_ballSpeed;
            nb.vy = std::sin(ang) * s_ballSpeed;
            nb.hitCount = 0; 
            s_balls.push_back(nb);
        }
    }
}

void CNewEasterEgg::CollideBallWithPaddle(Ball& b)
{
    if (b.vy > 0) {
        RECT pr = s_paddle;
        pr.left -= (int)b.r; pr.right += (int)b.r;
        pr.top -= (int)b.r; pr.bottom += (int)b.r;

        if (b.x >= pr.left && b.x <= pr.right && b.y >= pr.top && b.y <= pr.bottom) {
            b.y = s_paddle.top - b.r;
            double cx = (s_paddle.left + s_paddle.right) * 0.5;
            double rel = (b.x - cx) / (s_paddleW * 0.5); // [-1,1]
            rel = clampd(rel, -1.0, 1.0);

            double maxAng = 60.0 * M_PI / 180.0;
            double ang = -M_PI / 2 + rel * maxAng;

            b.vx = std::cos(ang) * s_ballSpeed;
            b.vy = std::sin(ang) * s_ballSpeed;

            b.hitCount = 0;
            PlaySoundA();
        }
    }
}

void CNewEasterEgg::OnMouseMove(int x, int y)
{
    int half = s_paddleW / 2;
    int nx = clampd(x, half + 6, s_bbCX - half - 6);
    int cx = nx;
    int py = s_bbCY - 40;
    s_paddle.left = cx - half;
    s_paddle.right = cx + half;
    s_paddle.top = py - s_paddleH / 2;
    s_paddle.bottom = s_paddle.top + s_paddleH;
}

void CNewEasterEgg::OnKeyDown(WPARAM vk)
{
    if (vk == VK_ESCAPE) {
        Close(m_hwnd);
        return;
    }
    int half = s_paddleW / 2;
    int cx = (s_paddle.left + s_paddle.right) / 2;
    if (vk == VK_LEFT) cx -= (int)(s_paddleSpeed * 0.1);
    else if (vk == VK_RIGHT) cx += (int)(s_paddleSpeed * 0.1);
    int nx = (int)clampd(cx, half + 6, s_bbCX - half - 6);
    int py = s_bbCY - 40;
    s_paddle.left = nx - half;
    s_paddle.right = nx + half;
    s_paddle.top = py - s_paddleH / 2;
    s_paddle.bottom = s_paddle.top + s_paddleH;
}

void CNewEasterEgg::PlaySoundA()
{
    int rndSound = STDHelpers::RandomSelectInt(2000, 2004);
    PlaySoundW(MAKEINTRESOURCEW(rndSound), reinterpret_cast<HINSTANCE>(FA2sp::hInstance), SND_ASYNC | SND_RESOURCE);
}

void CNewEasterEgg::StartTimer()
{
    if (s_timerID) return;
    s_lastTick = std::chrono::steady_clock::now();
    s_timerID = SetTimer(m_hwnd, 1, 16, nullptr); // ~60fps
}
void CNewEasterEgg::StopTimer()
{
    if (s_timerID) {
        KillTimer(m_hwnd, s_timerID);
        s_timerID = 0;
    }
}

COLORREF CNewEasterEgg::ColorFor(const Brick& br)
{
    if (br.hp == -1) return RGB(64, 64, 64);  
    if (br.hp == 2)  return RGB(0, 0, 128); 
    if (br.hp == 1)  return RGB(80, 160, 255);
    return RGB(0, 0, 0);
}