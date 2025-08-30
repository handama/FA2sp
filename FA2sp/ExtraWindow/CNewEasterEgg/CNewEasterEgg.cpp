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

HWND CBrickBreaker::m_hwnd = nullptr;
CFinalSunDlg* CBrickBreaker::m_parent = nullptr;
bool CBrickBreaker::initialized = false;
HBITMAP CBrickBreaker::s_hbmp = nullptr;
HDC     CBrickBreaker::s_memdc = nullptr;
int     CBrickBreaker::s_bbCX = 0, CBrickBreaker::s_bbCY = 0;
std::vector<CBrickBreaker::Brick> CBrickBreaker::s_bricks;
int CBrickBreaker::s_gridLeft = 20, CBrickBreaker::s_gridTop = 20;
int CBrickBreaker::s_gridRight = 0, CBrickBreaker::s_gridBottom = 0;
int CBrickBreaker::s_cellW = 0, CBrickBreaker::s_cellH = 0;
RECT   CBrickBreaker::s_paddle = { 0,0,0,0 };
int    CBrickBreaker::s_paddleW = 120;
int    CBrickBreaker::s_paddleH = 14;
double CBrickBreaker::s_paddleSpeed = 580.0;
std::vector<CBrickBreaker::Ball> CBrickBreaker::s_balls;
double CBrickBreaker::s_ballSpeed = 360.0;
bool   CBrickBreaker::s_roundActive = false;
int  CBrickBreaker::s_splitThreshold = 4;
std::mt19937_64 CBrickBreaker::s_rng{ std::random_device{}() };
UINT_PTR CBrickBreaker::s_timerID = 0;
std::chrono::steady_clock::time_point CBrickBreaker::s_lastTick;

constexpr double M_PI = 3.14159265358979323846;

void CBrickBreaker::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        reinterpret_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(326),
        pWnd ? pWnd->MyViewFrame.GetSafeHwnd() : nullptr,
        CBrickBreaker::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else {
        Logger::Error("Failed to create CBrickBreaker.\n");
        m_parent = nullptr;
    }
}

void CBrickBreaker::Initialize(HWND& hWnd)
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

void CBrickBreaker::RestrictCursor()
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

void CBrickBreaker::Close(HWND& hWnd)
{
    ClipCursor(nullptr);
    StopTimer();
    DestroyBackBuffer();
    EndDialog(hWnd, 0);
    m_hwnd = nullptr;
    m_parent = nullptr;
    initialized = false;
}


void CBrickBreaker::ShowVictoryDialog()
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

void CBrickBreaker::Update()
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

void CBrickBreaker::Render(HDC hdc)
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

BOOL CALLBACK CBrickBreaker::DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
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

void CBrickBreaker::CreateBackBuffer(int cx, int cy)
{
    DestroyBackBuffer();
    HDC hdc = GetDC(m_hwnd);
    s_memdc = CreateCompatibleDC(hdc);
    s_hbmp = CreateCompatibleBitmap(hdc, cx, cy);
    SelectObject(s_memdc, s_hbmp);
    ReleaseDC(m_hwnd, hdc);
    s_bbCX = cx; s_bbCY = cy;
}

void CBrickBreaker::DestroyBackBuffer()
{
    if (s_memdc) { DeleteDC(s_memdc); s_memdc = nullptr; }
    if (s_hbmp) { DeleteObject(s_hbmp); s_hbmp = nullptr; }
    s_bbCX = s_bbCY = 0;
}

void CBrickBreaker::OnSize(int cx, int cy)
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

void CBrickBreaker::ResetGame(bool newSeed)
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

void CBrickBreaker::ResetRound()
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

CBrickBreaker::Brick& CBrickBreaker::BrickAt(int r, int c)
{
    return s_bricks[r * GRID_COLS + c];
}

void CBrickBreaker::ForEachBrick(const std::function<void(Brick&)>& fn)
{
    for (auto& br : s_bricks) fn(br);
}

void CBrickBreaker::GenerateLevel()
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

void CBrickBreaker::EnsureSolvable()
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

void CBrickBreaker::Step(double dt)
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

void CBrickBreaker::CollideBallWithWorld(Ball& b)
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

void CBrickBreaker::CollideBallWithBricks(Ball& b)
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

void CBrickBreaker::CollideBallWithPaddle(Ball& b)
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

void CBrickBreaker::OnMouseMove(int x, int y)
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

void CBrickBreaker::OnKeyDown(WPARAM vk)
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

void CBrickBreaker::PlaySoundA()
{
    int rndSound = STDHelpers::RandomSelectInt(2000, 2004);
    PlaySoundW(MAKEINTRESOURCEW(rndSound), reinterpret_cast<HINSTANCE>(FA2sp::hInstance), SND_ASYNC | SND_RESOURCE);
}

void CBrickBreaker::StartTimer()
{
    if (s_timerID) return;
    s_lastTick = std::chrono::steady_clock::now();
    s_timerID = SetTimer(m_hwnd, 1, 16, nullptr); // ~60fps
}
void CBrickBreaker::StopTimer()
{
    if (s_timerID) {
        KillTimer(m_hwnd, s_timerID);
        s_timerID = 0;
    }
}

COLORREF CBrickBreaker::ColorFor(const Brick& br)
{
    if (br.hp == -1) return RGB(64, 64, 64);  
    if (br.hp == 2)  return RGB(0, 0, 128); 
    if (br.hp == 1)  return RGB(80, 160, 255);
    return RGB(0, 0, 0);
}

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
int CGoBang::clientW = 300;
int CGoBang::clientH = 300;

bool CGoBang::playerTurn = false;
bool CGoBang::gameOver = false;

std::vector<CGoBang::Move> CGoBang::history;
std::mt19937 CGoBang::rng{ std::random_device{}() };

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
    RECT rc; GetClientRect(hWnd, &rc);
    clientW = rc.right - rc.left; clientH = rc.bottom - rc.top;
    RecalcLayout();

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
            HBRUSH b = CreateSolidBrush(who == 1 ? RGB(10, 10, 10) : RGB(245, 245, 245));
            HGDIOBJ oldb = SelectObject(hdc, b);
            Ellipse(hdc, cx - rad, cy - rad, cx + rad, cy + rad);
            SelectObject(hdc, oldb);
            DeleteObject(b);
        }
    }

    std::wstring status;
    if (gameOver) {
        status = L"游戏结束 - 按 R 重置";
    }
    else {
        status = playerTurn ? L"轮到你（白）" : L"电脑思考中（黑先）";
    }
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));
    TextOutW(hdc, 8, 8, status.c_str(), (int)status.size());
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

    if (CheckWin(1)) {
        gameOver = true;
        InvalidateRect(m_hwnd, nullptr, TRUE);
        MessageBoxW(m_hwnd, L"电脑（黑）胜利！", L"游戏结束", MB_OK | MB_ICONINFORMATION);
        return;
    }

    playerTurn = true;
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void CGoBang::ResetGame()
{
    for (int r = 0; r < BOARD_N; ++r) for (int c = 0; c < BOARD_N; ++c) board[r][c] = 0;
    history.clear();
    gameOver = false;

    // AI first
    int cr = BOARD_N / 2, cc = BOARD_N / 2;
    board[cr][cc] = 1;
    history.push_back({ cr,cc,1 });
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
                            MessageBoxW(hwnd, L"你（白）胜利！", L"游戏结束", MB_OK | MB_ICONINFORMATION);
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