#include "Body.h"

#include "../../FA2sp.h"

#include "TabPages/TriggerSort.h"
#include "TabPages/TeamSort.h"

#include "../../Helpers/Translations.h"
#include "TabPages/ScriptSort.h"
#include "TabPages/TaskForceSort.h"
#include "TabPages/WaypointSort.h"
#include "TabPages/TagSort.h"
#include "TabPages/GridObjectViewer.h"

DEFINE_HOOK(4F1A40, CTileSetBrowserFrame_CreateContent, 5)
{
    GET(CTileSetBrowserFrameExt*, pThis, ECX);
    GET_STACK(LPCREATESTRUCT, lpcs, 0x4);
    GET_STACK(ppmfc::CCreateContext*, pContent, 0x8);

    pThis->InitTabControl();
    
    auto const pTab = ppmfc::CWnd::FromHandle(CTileSetBrowserFrameExt::hTabCtrl);

    RECT rect;
    pThis->GetClientRect(&rect);
    pThis->DialogBar.Create(pTab, MAKEINTRESOURCE(ExtConfigs::VerticalLayout ? 0xE4 : 0xE3), 0x2800, 5);

    Translations::TranslateItem(&pThis->DialogBar, 6102, "DialogBar.TileManager");
    Translations::TranslateItem(&pThis->DialogBar, 1368, "DialogBar.TerrainOrGround");
    Translations::TranslateItem(&pThis->DialogBar, 1369, "DialogBar.OverlayAndSpecial");
    Translations::TranslateItem(&pThis->DialogBar, 6250, "DialogBar.GlobalSearch");
    Translations::TranslateItem(&pThis->DialogBar, 6251, "DialogBar.TerrainGenerator");

    pThis->View.Create(nullptr, nullptr, 0x50300000, rect, pTab, 1, nullptr);
    pThis->ppmfc::CFrameWnd::RecalcLayout();
    SIZE sz{ rect.right, pThis->View.ScrollWidth };
    pThis->View.SetScrollSizes(1, sz);
    R->EAX(pThis->ppmfc::CFrameWnd::OnCreateClient(lpcs, pContent));

    return 0x4F1AF6;
}

DEFINE_HOOK(4F1B00, CTileSetBrowserFrame_RecalcLayout, 7)
{
    GET(CTileSetBrowserFrameExt*, pThis, ECX);

    RECT frameRect, tabRect;
    pThis->GetClientRect(&frameRect);
    ::MoveWindow(CTileSetBrowserFrameExt::hTabCtrl, 0, 0, frameRect.right - frameRect.left,
        frameRect.bottom - frameRect.top, TRUE);

    ::GetClientRect(CTileSetBrowserFrameExt::hTabCtrl, &tabRect);

    HWND hTemp = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(ExtConfigs::VerticalLayout ? 0xE4 : 0xE3),
        CTileSetBrowserFrameExt::hTabCtrl,
        NULL
    );

    CRect rect;
    GetWindowRect(hTemp, &rect);

    if (ExtConfigs::VerticalLayout)
    {
        pThis->DialogBar.MoveWindow(2, 29, tabRect.right - tabRect.left - 6, rect.Height(), FALSE);
        pThis->View.MoveWindow(2, 29 + rect.Height(), tabRect.right - tabRect.left - 6, tabRect.bottom - rect.Height() - 29, FALSE);
    }
    else
    {
        pThis->DialogBar.MoveWindow(2, 29, tabRect.right - tabRect.left - 6, rect.Height(), FALSE);
        pThis->View.MoveWindow(2, 29 + rect.Height(), tabRect.right - tabRect.left - 6, tabRect.bottom - rect.Height() - 29, FALSE);
    }

    EndDialog(hTemp, NULL);

    SIZE sz{ tabRect.right,pThis->View.ScrollWidth };
    pThis->View.SetScrollSizes(1, sz);

    TriggerSort::Instance.OnSize();
    TeamSort::Instance.OnSize();
    TaskforceSort::Instance.OnSize();
    ScriptSort::Instance.OnSize();
    WaypointSort::Instance.OnSize();
    TagSort::Instance.OnSize();
    GridObjectViewer::Instance.OnSize();

    return 0x4F1B8A;
}
