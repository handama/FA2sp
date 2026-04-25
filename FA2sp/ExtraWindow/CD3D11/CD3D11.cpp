#include "CD3D11.h"
#include "../../Ext/CLoading/Body.h"
#include "DirectXCore.h"

HWND CD3D11::m_hwnd;

// CD3D11.cpp
void CD3D11::Create(HWND hParent) {
    m_hwnd = CreateDialog(static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(340),
        hParent,
        CD3D11::DlgProc);
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
        DirectXCore::InitializeDX(m_hwnd);  // 在创建后立即初始化 DX
    }
    else {
        Logger::Error("Failed to create CD3D11.\n");
    }
}

void CD3D11::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);
    CD3D11::m_hwnd = NULL;
}

void CD3D11::Initialize(HWND& hWnd)
{

}

// 在 DlgProc 中添加定时器或 WM_PAINT 调用 Render()
BOOL CALLBACK CD3D11::DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) {
    case WM_INITDIALOG:
        CD3D11::Initialize(hwnd);
        SetTimer(hwnd, 1, 16, nullptr);  // ~60 FPS
        return TRUE;

    case WM_TIMER:
        if (wParam == 1) {
            DirectXCore::Render();  // 或者在单独线程/消息循环中调用
        }
        break;

    case WM_SIZE:
        // Resize swap chain & RTV（需重新创建 RTV）
        break;

    case WM_CLOSE:
        DirectXCore::Cleanup();
        CD3D11::Close(hwnd);
        return TRUE;
    }
    return FALSE;
}
