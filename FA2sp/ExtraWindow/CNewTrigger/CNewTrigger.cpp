#include "CNewTrigger.h"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/MultimapHelper.h"
#include "../Common.h"

#include <CLoading.h>
#include <CFinalSunDlg.h>
#include <CObjectDatas.h>
#include <CMapData.h>
#include <CIsoView.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CMapData/Body.h"
#include <Miscs/Miscs.h>
#include "../CObjectSearch/CObjectSearch.h"
#include "../../Ext/CTileSetBrowserFrame/TabPages/TriggerSort.h"
#include "../CNewScript/CNewScript.h"
#include <numeric>
#include "../CSearhReference/CSearhReference.h"
#include "../CBatchTrigger/CBatchTrigger.h"
#include "../CTriggerAnnotation/CTriggerAnnotation.h"
#include "../CCsfEditor/CCsfEditor.h"
#include "../CNewTeamTypes/CNewTeamTypes.h"
#include "../CNewAITrigger/CNewAITrigger.h"
#include "../../Helpers/Helper.h"

CINI& CNewTrigger::map = CINI::CurrentDocument;
CINI& CNewTrigger::fadata = CINI::FAData;
MultimapHelper& CNewTrigger::rules = Variables::RulesMap;
CNewTrigger CNewTrigger::Instance[TRIGGER_EDITOR_MAX_COUNT];
std::vector<ParamAffectedParams> CNewTrigger::ActionParamAffectedParams;
std::vector<ParamAffectedParams> CNewTrigger::EventParamAffectedParams;
bool CNewTrigger::AvoidInfiLoop = false;
bool CNewTrigger::SortTriggersExecuted = false;
bool CNewTrigger::AutoChangeName = false;
static constexpr int DRAG_THRESHOLD = 4;

void CNewTrigger::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialogParam(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(CompactMode ? 332 : 307),
        pWnd->GetSafeHwnd(),
        DlgProc,
        reinterpret_cast<LPARAM>(this)
    );

    if (m_hwnd)
    {
        RECT rc;
        GetWindowRect(m_hwnd, &rc);

        const int offset = 20;

        int index = GetCurrentInstanceIndex();
        int dx = index * offset;
        int dy = index * offset;

        SetWindowPos(
            m_hwnd,
            nullptr,
            windowPos.x == 0 ? (rc.left + dx) : windowPos.x,
            windowPos.y == 0 ? (rc.top + dy) : windowPos.y,
            0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
        );
        PostMessage(m_hwnd, WM_USER + 100, 0, 0);
        ShowWindow(m_hwnd, SW_SHOW);
        CNewTrigger::Initialize(m_hwnd);
        WindowShown = true;
    }
    else
    {
        Logger::Error("Failed to create CNewTrigger.\n");
        m_parent = NULL;
        return;
    }
}

void CNewTrigger::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("TriggerTypesTitle", buffer))
    {
        int index = GetCurrentInstanceIndex();
        if (index > 0)
            buffer.Format("%s - %d", buffer, index + 1);
        SetWindowText(hWnd, buffer);
    }

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };

    Translate(50901, "TriggerTriggeroptions");
    Translate(50902, "TriggerSelectedTrigger");
    Translate(50904, CompactMode ? "TriggerNewShort" : "TriggerNew");
    Translate(50905, CompactMode ? "TriggerCloneShort" : "TriggerClone");
    Translate(50906, CompactMode ? "TriggerDeleteShort" : "TriggerDelete");
    Translate(50907, CompactMode ? "TriggerPlaceOnMapShort" : "TriggerPlaceOnMap");
    Translate(50908, "TriggerType");
    Translate(50910, "TriggerName");
    Translate(50912, "TriggerHouse");
    Translate(50914, "TriggerAttachedtrigger");
    Translate(50915, "TriggerCannotbeitselforformsaloop");
    Translate(50917, CompactMode ? "TriggerDisabledShort" : "TriggerDisabled");
    Translate(50918, CompactMode ? "TriggerEasyShort" : "TriggerEasy");
    Translate(50919, CompactMode ? "TriggerMediumShort" : "TriggerMedium");
    Translate(50920, CompactMode ? "TriggerHardShort" : "TriggerHard");
    Translate(50921, "TriggerEventoptions");
    Translate(50922, "TriggerEventtype");
    Translate(50924, CompactMode ? "TriggerAddShort" : "TriggerAdd");
    Translate(50925, CompactMode ? "TriggerCloneShort" : "TriggerClone");
    Translate(50926, CompactMode ? "TriggerDeleteShort" : "TriggerDelete");
    Translate(50928, "TriggerEventList");
    Translate(50930, "TriggerParameter#1value");
    Translate(50932, "TriggerParameter#2value");
    Translate(50934, "TriggerActionoptions");
    Translate(50935, "TriggerActiontype");
    Translate(50937, CompactMode ? "TriggerAddShort" : "TriggerAdd");
    Translate(50939, CompactMode ? "TriggerCloneShort" : "TriggerClone");
    Translate(50938, CompactMode ? "TriggerDeleteShort" : "TriggerDelete");
    Translate(50941, "TriggerActionList");
    Translate(50943, "TriggerParameter#1value");
    Translate(50945, "TriggerParameter#2value");
    Translate(50947, "TriggerParameter#3value");
    Translate(50949, "TriggerParameter#4value");
    Translate(50951, "TriggerParameter#5value");
    Translate(50953, "TriggerParameter#6value");
    Translate(1999, CompactMode ? "SearchReferenceTitleShort" : "SearchReferenceTitle");
    Translate(2000, "TriggerOpenNewEditor");
    Translate(2002, CompactMode ? "TriggerCompactModeShort" : "TriggerCompactMode");

    hSelectedTrigger = GetDlgItem(hWnd, Controls::SelectedTrigger);
    hNewTrigger = GetDlgItem(hWnd, Controls::NewTrigger);
    hCloneTrigger = GetDlgItem(hWnd, Controls::CloneTrigger);
    hDeleteTrigger = GetDlgItem(hWnd, Controls::DeleteTrigger);
    hPlaceOnMap = GetDlgItem(hWnd, Controls::PlaceOnMap);
    hType = GetDlgItem(hWnd, Controls::Type);
    hName = GetDlgItem(hWnd, Controls::Name);
    hHouse = GetDlgItem(hWnd, Controls::House);
    hAttachedtrigger = GetDlgItem(hWnd, Controls::Attachedtrigger);
    hDisabled = GetDlgItem(hWnd, Controls::Disabled);
    hEasy = GetDlgItem(hWnd, Controls::Easy);
    hMedium = GetDlgItem(hWnd, Controls::Medium);
    hHard = GetDlgItem(hWnd, Controls::Hard);
    hEventtype = GetDlgItem(hWnd, Controls::Eventtype);
    hNewEvent = GetDlgItem(hWnd, Controls::NewEvent);
    hCloneEvent = GetDlgItem(hWnd, Controls::CloneEvent);
    hDeleteEvent = GetDlgItem(hWnd, Controls::DeleteEvent);
    hEventDescription = GetDlgItem(hWnd, Controls::EventDescription);
    hEventList = GetDlgItem(hWnd, Controls::EventList);
    hEventParameter[0] = GetDlgItem(hWnd, Controls::EventParameter1);
    hEventParameter[1] = GetDlgItem(hWnd, Controls::EventParameter2);
    hEventParameterDesc[0] = GetDlgItem(hWnd, Controls::EventParameter1Desc);
    hEventParameterDesc[1] = GetDlgItem(hWnd, Controls::EventParameter2Desc);
    hActionoptions = GetDlgItem(hWnd, Controls::Actionoptions);
    hActiontype = GetDlgItem(hWnd, Controls::Actiontype);
    hNewAction = GetDlgItem(hWnd, Controls::NewAction);
    hDeleteAction = GetDlgItem(hWnd, Controls::DeleteAction);
    hCloneAction = GetDlgItem(hWnd, Controls::CloneAction);
    hActionDescription = GetDlgItem(hWnd, Controls::ActionDescription);
    hActionList = GetDlgItem(hWnd, Controls::ActionList);
    hActionframe = GetDlgItem(hWnd, Controls::Actionframe);
    hActionParameter[0] = GetDlgItem(hWnd, Controls::ActionParameter1);
    hActionParameter[1] = GetDlgItem(hWnd, Controls::ActionParameter2);
    hActionParameter[2] = GetDlgItem(hWnd, Controls::ActionParameter3);
    hActionParameter[3] = GetDlgItem(hWnd, Controls::ActionParameter4);
    hActionParameter[4] = GetDlgItem(hWnd, Controls::ActionParameter5);
    hActionParameter[5] = GetDlgItem(hWnd, Controls::ActionParameter6);
    hActionParameterDesc[0] = GetDlgItem(hWnd, Controls::ActionParameter1Desc);
    hActionParameterDesc[1] = GetDlgItem(hWnd, Controls::ActionParameter2Desc);
    hActionParameterDesc[2] = GetDlgItem(hWnd, Controls::ActionParameter3Desc);
    hActionParameterDesc[3] = GetDlgItem(hWnd, Controls::ActionParameter4Desc);
    hActionParameterDesc[4] = GetDlgItem(hWnd, Controls::ActionParameter5Desc);
    hActionParameterDesc[5] = GetDlgItem(hWnd, Controls::ActionParameter6Desc);
    hSearchReference = GetDlgItem(hWnd, Controls::SearchReference);
    hOpenNewEditor = GetDlgItem(hWnd, Controls::OpenNewEditor);
    hDragPoint = GetDlgItem(hWnd, Controls::DragPoint);
    hCompact = GetDlgItem(hWnd, Controls::Compact);
    hActionMoveUp = GetDlgItem(hWnd, Controls::ActionMoveUp);
    hActionMoveDown = GetDlgItem(hWnd, Controls::ActionMoveDown);
    hActionSplit = GetDlgItem(hWnd, Controls::ActionSplit);
    SetWindowTextW(hActionMoveUp, L"¡ø");
    SetWindowTextW(hActionMoveDown, L"¨‹");
    Translate(2005, "TriggerActionSplit");

    if (!IsMainInstance())
        ShowWindow(hOpenNewEditor, SW_HIDE);
    else
        ShowWindow(hCompact, SW_HIDE);

    ExtraWindow::RegisterDropTarget(hAttachedtrigger, DropType::AttachedTrigger, this);
    ExtraWindow::RegisterDropTarget(hActionParameter[0], DropType::ActionParam0, this);
    ExtraWindow::RegisterDropTarget(hActionParameter[1], DropType::ActionParam1, this);
    ExtraWindow::RegisterDropTarget(hActionParameter[2], DropType::ActionParam2, this);
    ExtraWindow::RegisterDropTarget(hActionParameter[3], DropType::ActionParam3, this);
    ExtraWindow::RegisterDropTarget(hActionParameter[4], DropType::ActionParam4, this);
    ExtraWindow::RegisterDropTarget(hActionParameter[5], DropType::ActionParam5, this);
    ExtraWindow::RegisterDropTarget(hEventParameter[0], DropType::EventParam0, this);
    ExtraWindow::RegisterDropTarget(hEventParameter[1], DropType::EventParam1, this);

    LastActionParamsCount = 4;
    ActionParamsCount = 4;

    for (int i = 0; i < EVENT_PARAM_COUNT; ++i)
        EventParamsUsage[i] = std::make_pair(false, 0);
    for (int i = 0; i < ACTION_PARAM_COUNT; ++i)
    {
        ActionParamsUsage[i] = std::make_pair(false, 0);
        ShowWindow(hActionParameterDesc[i], SW_HIDE);
    }

    ExtraWindow::SetEditControlFontSize(hEventDescription, 1.3f);
    ExtraWindow::SetEditControlFontSize(hActionDescription, 1.3f);

    if (hEventList)
    {
        SetWindowLongPtr(hEventList, GWLP_USERDATA, (LONG_PTR)this);
        OriginalListBoxProcEvent = (WNDPROC)SetWindowLongPtr(hEventList, GWLP_WNDPROC, (LONG_PTR)ListBoxSubclassProcEvent);
    }
    if (hActionList)
    {
        SetWindowLongPtr(hActionList, GWLP_USERDATA, (LONG_PTR)this);
        OriginalListBoxProcAction = (WNDPROC)SetWindowLongPtr(hActionList, GWLP_WNDPROC, (LONG_PTR)ListBoxSubclassProcAction);
    }
    if (hDragPoint)
    {
        SetWindowLongPtr(hDragPoint, GWLP_USERDATA, (LONG_PTR)this);
        OrigDragDotProc = (WNDPROC)SetWindowLongPtr(hDragPoint, GWLP_WNDPROC, (LONG_PTR)DragDotProc);
    }

    CurrentTrigger = nullptr;

    Update(hWnd);
}

void CNewTrigger::Update(HWND& hWnd, bool UpdateTrigger)
{
    if (m_hwnd)
    {
        ShowWindow(m_hwnd, SW_SHOW);
        SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    }

    DropNeedUpdate = false;

    if (UpdateTrigger)
        CMapDataExt::UpdateTriggers();

    SortTriggers("", true);

    if (UpdateTrigger)
    {
        auto others = GetOtherInstances();
        for (auto& [i, o] : others)
        {
            if (o->GetHandle())
                o->CurrentTrigger = CMapDataExt::GetTrigger(o->CurrentTriggerID);
        }
    }
        
    int count = SendMessage(hSelectedTrigger, CB_GETCOUNT, NULL, NULL);
    if (SelectedTriggerIndex < 0)
        SelectedTriggerIndex = 0;
    if (SelectedTriggerIndex > count - 1)
        SelectedTriggerIndex = count - 1;
    SendMessage(hSelectedTrigger, CB_SETCURSEL, SelectedTriggerIndex, NULL);

    int idx = 0;
    while (SendMessage(hEventtype, CB_DELETESTRING, 0, NULL) != CB_ERR);
    if (auto pSection = fadata.GetSection(ExtraWindow::GetTranslatedSectionName("EventsRA2")))
    {
        for (auto& pair : pSection->GetEntities())
        {
            auto atoms = FString::SplitString(pair.second);
            if (atoms.size() >= 9)
            {
                if (atoms[7] == "1")
                {
                    SendMessage(hEventtype, CB_INSERTSTRING, idx++, 
                        (LPARAM)(LPCSTR)(FString(pair.first) + " " + FString::ReplaceSpeicalString(atoms[0])).c_str());
                }
            }
        }
    }
    if (CompactMode) ExtraWindow::AdjustDropdownWidth(hEventtype);
    idx = 0;
    while (SendMessage(hActiontype, CB_DELETESTRING, 0, NULL) != CB_ERR);
    if (auto pSection = fadata.GetSection(ExtraWindow::GetTranslatedSectionName("ActionsRA2")))
    {
        for (auto& pair : pSection->GetEntities())
        {
            auto atoms = FString::SplitString(pair.second);
            if (atoms.size() >= 14)
            {
                if (atoms[12] == "1")
                {
                    SendMessage(hActiontype, CB_INSERTSTRING, idx++, 
                        (LPARAM)(LPCSTR)(FString(pair.first) + " " + FString::ReplaceSpeicalString(atoms[0])).c_str());
                }
            }
        }
    }
    if (CompactMode) ExtraWindow::AdjustDropdownWidth(hActiontype);

    idx = 0;
    while (SendMessage(hHouse, CB_DELETESTRING, 0, NULL) != CB_ERR); 
    if (CMapData::Instance->IsMultiOnly() && ExtConfigs::PlayerAtXForTriggers)
    {
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)FString("<Player @ A>").c_str());
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)FString("<Player @ B>").c_str());
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)FString("<Player @ C>").c_str());
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)FString("<Player @ D>").c_str());
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)FString("<Player @ E>").c_str());
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)FString("<Player @ F>").c_str());
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)FString("<Player @ G>").c_str());
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)FString("<Player @ H>").c_str());
    }
    const auto& indicies = Variables::RulesMap.ParseIndicies("Countries", true);
    for (auto& value : indicies)
    {
        if (value == "GDI" || value == "Nod")
            continue;
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)Translations::ParseHouseName(value, true).c_str());
    }
    if (CompactMode) ExtraWindow::AdjustDropdownWidth(hHouse);

    idx = 0;
    while (SendMessage(hType, CB_DELETESTRING, 0, NULL) != CB_ERR);
    SendMessage(hType, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)(FString("0 - ") + Translations::TranslateOrDefault("TriggerRepeatType.OneTimeOr", "One Time OR")));
    SendMessage(hType, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)(FString("1 - ") + Translations::TranslateOrDefault("TriggerRepeatType.OneTimeAnd", "One Time AND")));
    SendMessage(hType, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)(FString("2 - ") + Translations::TranslateOrDefault("TriggerRepeatType.RepeatingOr", "Repeating OR")));
    if (CompactMode) ExtraWindow::AdjustDropdownWidth(hType);

    SendMessage(hCompact, BM_SETCHECK, CompactMode ? BST_CHECKED : BST_UNCHECKED, 0);

    Autodrop = false;

    OnSelchangeTrigger();
}

void CNewTrigger::Close(HWND& hWnd)
{
    SetWindowLongPtr(
        hEventList,
        GWLP_WNDPROC,
        (LONG_PTR)OriginalListBoxProcEvent
    );
    SetWindowLongPtr(hEventList, GWLP_USERDATA, 0);
    SetWindowLongPtr(
        hActionList,
        GWLP_WNDPROC,
        (LONG_PTR)OriginalListBoxProcAction
    );
    SetWindowLongPtr(hActionList, GWLP_USERDATA, 0);
    SetWindowLongPtr(
        hDragPoint,
        GWLP_WNDPROC,
        (LONG_PTR)OrigDragDotProc
    );
    SetWindowLongPtr(hDragPoint, GWLP_USERDATA, 0);
    EndDialog(hWnd, NULL);

    CurrentTrigger = nullptr;
    m_hwnd = NULL;
    m_parent = NULL;
}

LRESULT CALLBACK CNewTrigger::DragDotProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* self = reinterpret_cast<CNewTrigger*>(
        GetWindowLongPtr(hWnd, GWLP_USERDATA)
        );

    if (self) {
        return self->HandleDragDot(hWnd, msg, wParam, lParam);
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK CNewTrigger::HandleDragDot(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SETCURSOR:
    {
        if (CurrentTrigger)
        {
            SetCursor(LoadCursor(nullptr, IDC_HAND));
            return TRUE;
        }
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        HBRUSH hBrush = CreateSolidBrush(CurrentTrigger ? RGB(0, 200, 0) : RGB(200, 0, 0));
        FillRect(hdc, &ps.rcPaint, hBrush);
        DeleteObject(hBrush);

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        if (CurrentTrigger)
        {
            m_pressed = true;
            m_dragging = false;

            GetCursorPos(&m_pressPtScreen);
            m_lastPtScreen = m_pressPtScreen;

            SetCapture(hWnd);        
            return 0;
        }
        break;
    }
    case WM_MOUSEMOVE:
    {
        if (!m_pressed)
            break;

        POINT pt;
        GetCursorPos(&pt);

        int dx = abs(pt.x - m_pressPtScreen.x);
        int dy = abs(pt.y - m_pressPtScreen.y);

        if (!m_dragging)
        {
            if (dx >= DRAG_THRESHOLD || dy >= DRAG_THRESHOLD)
            {
                m_dragging = true;
                m_hDragGhost = CreateWindowEx(
                    WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
                    "STATIC",
                    nullptr,
                    WS_POPUP,
                    m_pressPtScreen.x - 6, m_pressPtScreen.y - 6,
                    12, 12,
                    nullptr, nullptr,
                    static_cast<HINSTANCE>(FA2sp::hInstance),
                    nullptr
                );

                if (m_hDragGhost)
                {
                    SetWindowLongPtr(m_hDragGhost, GWLP_USERDATA, (LONG_PTR)this);
                    OrigDragingDotProc = (WNDPROC)SetWindowLongPtr(m_hDragGhost, GWLP_WNDPROC, (LONG_PTR)DragingDotProc);
                    SetLayeredWindowAttributes(m_hDragGhost, 0, 200, LWA_ALPHA);
                    ShowWindow(m_hDragGhost, SW_SHOW);
                }
            }
        }

        if (m_dragging)
        {
            SetWindowPos(
                m_hDragGhost,
                nullptr,
                pt.x - 6, pt.y - 6,
                0, 0,
                SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
            );

            if (ExtraWindow::IsPointOnIsoViewAndNotCovered(pt))
            {
                TempValueHolder<int> command(CIsoView::CurrentCommand->Command, 0x25);
                TempValueHolder<int> type(CIsoView::CurrentCommand->Type, GetCurrentInstanceIndex());
                ScreenToClient(CIsoView::GetInstance()->GetSafeHwnd(), &pt);
                CIsoView::GetInstance()->OnMouseMove(0, pt);
            }
        }

        return 0;
    }
    case WM_LBUTTONUP:
    {
        if (!m_pressed)
            break;

        ReleaseCapture();
        m_pressed = false;

        POINT pt;
        GetCursorPos(&pt);

        if (m_dragging)
        {
            SetWindowLongPtr(
                m_hDragGhost,
                GWLP_WNDPROC,
                (LONG_PTR)OrigDragingDotProc
            );
            SetWindowLongPtr(m_hDragGhost, GWLP_USERDATA, 0);

            DestroyWindow(m_hDragGhost);
            m_hDragGhost = nullptr;

            if (ExtraWindow::IsPointOnIsoViewAndNotCovered(pt) && CurrentTrigger)
            {
                ScreenToClient(CIsoView::GetInstance()->GetSafeHwnd(), &pt);
                auto coord = CIsoView::GetInstance()->GetCurrentMapCoord(pt);
                int& X = coord.X; int& Y = coord.Y;
                CViewObjectsExt::ApplyTag(X, Y, CurrentTrigger->Tag);
            }
            else
            {
                auto target = ExtraWindow::FindDropTarget(pt);
                if (target.hWnd)
                {
                    auto triggerID = CurrentTriggerID + " ";
                    switch (target.type)
                    {
                    case DropType::AttachedTrigger:
                        if (target.triggerInstance && target.triggerInstance->GetHandle())
                        {
                            auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, triggerID);
                            if (idx == CB_ERR)
                            {
                                target.triggerInstance->OnSelchangeActionListbox();
                                idx = ExtraWindow::FindCBStringExactStart(target.hWnd, triggerID);
                            }
                            if (idx != CB_ERR)
                            {
                                SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                                target.triggerInstance->OnSelchangeAttachedTrigger();
                            }
                        }
                        break;
                    case DropType::ActionParam0:
                        if (target.triggerInstance && target.triggerInstance->GetHandle())
                        {
                            auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, triggerID);
                            if (idx != CB_ERR)
                            {
                                SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                                target.triggerInstance->OnSelchangeActionParam(0);
                            }
                        }
                        break;
                    case DropType::ActionParam1:
                        if (target.triggerInstance && target.triggerInstance->GetHandle())
                        {
                            auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, triggerID);
                            if (idx != CB_ERR)
                            {
                                SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                                target.triggerInstance->OnSelchangeActionParam(1);
                            }
                        }
                        break;
                    case DropType::ActionParam2:
                        if (target.triggerInstance && target.triggerInstance->GetHandle())
                        {
                            auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, triggerID);
                            if (idx != CB_ERR)
                            {
                                SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                                target.triggerInstance->OnSelchangeActionParam(2);
                            }
                        }
                        break;
                    case DropType::ActionParam3:
                        if (target.triggerInstance && target.triggerInstance->GetHandle())
                        {
                            auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, triggerID);
                            if (idx != CB_ERR)
                            {
                                SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                                target.triggerInstance->OnSelchangeActionParam(3);
                            }
                        }
                        break;
                    case DropType::ActionParam4:
                        if (target.triggerInstance && target.triggerInstance->GetHandle())
                        {
                            auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, triggerID);
                            if (idx != CB_ERR)
                            {
                                SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                                target.triggerInstance->OnSelchangeActionParam(4);
                            }
                        }
                        break;
                    case DropType::ActionParam5:
                        if (target.triggerInstance && target.triggerInstance->GetHandle())
                        {
                            auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, triggerID);
                            if (idx != CB_ERR)
                            {
                                SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                                target.triggerInstance->OnSelchangeActionParam(5);
                            }
                        }
                        break;
                    case DropType::EventParam0:
                        if (target.triggerInstance && target.triggerInstance->GetHandle())
                        {
                            auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, triggerID);
                            if (idx != CB_ERR)
                            {
                                SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                                target.triggerInstance->OnSelchangeEventParam(0);
                            }
                        }
                        break;
                    case DropType::EventParam1:
                        if (target.triggerInstance && target.triggerInstance->GetHandle())
                        {
                            auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, triggerID);
                            if (idx != CB_ERR)
                            {
                                SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                                target.triggerInstance->OnSelchangeEventParam(1);
                            }
                        }
                        break;
                    case DropType::TeamEditorTag:
                        {
                            auto tag = CurrentTrigger->Tag;
                            tag += " ";
                            auto idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tag);
                            if (idx == CB_ERR)
                            {
                                CNewTeamTypes::OnDropdownTag();
                                idx = ExtraWindow::FindCBStringExactStart(target.hWnd, tag);
                            }
                            if (idx != CB_ERR)
                            {
                                SendMessage(target.hWnd, CB_SETCURSEL, idx, NULL);
                                CNewTeamTypes::OnSelchangeTag();
                            }
                        }
                        break;
                    case DropType::BatchTriggerListView:
                    {
                        ListViewHitResult hit;
                        if (ExtraWindow::HitTestListView(target.hWnd, pt, hit))
                        {
                            CBatchTrigger::OnDroppedIntoCell(hit.item, hit.subItem, CurrentTriggerID);
                        }
                    }
                        break;
                    case DropType::Unknown:
                    default:
                        break;
                    }
                }
            }

        }
        else
        {
            CIsoView::CurrentCommand->Command = 0x25;
            CIsoView::CurrentCommand->Type = GetCurrentInstanceIndex();
        }

        m_dragging = false;     
        return 0;
    }
    }

    return CallWindowProc(
        OrigDragDotProc,
        hWnd, msg, wParam, lParam
    );
}

LRESULT CALLBACK CNewTrigger::DragingDotProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auto* self = reinterpret_cast<CNewTrigger*>(
        GetWindowLongPtr(hWnd, GWLP_USERDATA)
        );

    if (self) {
        return self->HandleDragingDot(hWnd, msg, wParam, lParam);
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK CNewTrigger::HandleDragingDot(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        HBRUSH hBrush = CreateSolidBrush(RGB(0, 200, 0));
        FillRect(hdc, &ps.rcPaint, hBrush);
        DeleteObject(hBrush);

        EndPaint(hWnd, &ps);
        return 0;
    }
    }

    return CallWindowProc(
        OrigDragingDotProc,
        hWnd, msg, wParam, lParam
    );
}

LRESULT CALLBACK CNewTrigger::ListBoxSubclassProcEvent(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
    auto* self = reinterpret_cast<CNewTrigger*>(
        GetWindowLongPtr(hWnd, GWLP_USERDATA)
        );

    if (self) {
        return self->HandleListBoxEvent(hWnd, msg, wParam, lParam);
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK CNewTrigger::ListBoxSubclassProcAction(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
    auto* self = reinterpret_cast<CNewTrigger*>(
        GetWindowLongPtr(hWnd, GWLP_USERDATA)
        );

    if (self) {
        return self->HandleListBoxAction(hWnd, msg, wParam, lParam);
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK CNewTrigger::HandleListBoxAction(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_MOUSEWHEEL:
    {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);

        RECT rc;
        GetClientRect(hWnd, &rc);

        if (pt.x >= rc.right - GetSystemMetrics(SM_CXVSCROLL))
        {
            return CallWindowProc(OriginalListBoxProcAction, hWnd, message, wParam, lParam);
        }

        int delta = (short)HIWORD(wParam);

        WPARAM keyParam = 0;
        if (delta > 0)
            keyParam = VK_UP;
        else if (delta < 0)
            keyParam = VK_DOWN;

        if (keyParam != 0)
        {
            SendMessage(hWnd, WM_KEYDOWN, keyParam, 0x00000001);
            SendMessage(hWnd, WM_KEYUP, keyParam, 0xC0000001);
        }

        return TRUE;
    }
    }
    return CallWindowProc(OriginalListBoxProcAction, hWnd, message, wParam, lParam);
}

LRESULT CALLBACK CNewTrigger::HandleListBoxEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_MOUSEWHEEL:

        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);
        RECT rc;
        GetClientRect(hWnd, &rc);

        if (pt.x >= rc.right)
        {
            return CallWindowProc(OriginalListBoxProcEvent, hWnd, message, wParam, lParam);
        }
        else
        {
            int nCurSel = (int)SendMessage(hWnd, LB_GETCURSEL, 0, 0);
            int nCount = (int)SendMessage(hWnd, LB_GETCOUNT, 0, 0);

            if (nCurSel != LB_ERR && nCount > 0)
            {
                if ((short)HIWORD(wParam) > 0 && nCurSel > 0)
                {
                    SendMessage(hWnd, LB_SETCURSEL, nCurSel - 1, 0);
                }
                else if ((short)HIWORD(wParam) < 0 && nCurSel < nCount - 1)
                {
                    SendMessage(hWnd, LB_SETCURSEL, nCurSel + 1, 0);
                }
                OnSelchangeEventListbox();

            }
            else {
                SendMessage(hWnd, LB_SETCURSEL, 0, 0);
            }
            return TRUE;
        }
    }
    return CallWindowProc(OriginalListBoxProcEvent, hWnd, message, wParam, lParam);
}

BOOL CALLBACK CNewTrigger::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    CNewTrigger* self = nullptr;

    if (Msg == WM_INITDIALOG)
    {
        self = reinterpret_cast<CNewTrigger*>(lParam);
        if (self == nullptr) return FALSE;

        self->WindowShown = false;
        self->m_hwnd = hWnd;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)self);
        return TRUE;
    }

    self = reinterpret_cast<CNewTrigger*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (self == nullptr)
        return FALSE;
    return self->HandleMsg(hWnd, Msg, wParam, lParam);
}

BOOL CALLBACK CNewTrigger::HandleMsg(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_ACTIVATE:
    {
        if (CurrentTrigger)
        {
            CTriggerAnnotation::Type = AnnoTrigger;
            CTriggerAnnotation::ID = CurrentTriggerID;
            ::SendMessage(CTriggerAnnotation::GetHandle(), 114515, 0, 0);
        }
        return TRUE;
    }
    case WM_INITDIALOG:
    {
        return TRUE;
    }
    case WM_SHOWWINDOW:
    {
        return FALSE;
    }
    case WM_USER + 100:
    {
        OnSelchangeTrigger();
        for (int i = 0; i < TRIGGER_EDITOR_MAX_COUNT; ++i)
        {
            Instance[i].DropNeedUpdate = false;
        }
        return FALSE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::EventList:
            EventListBoxProc(hWnd, CODE, lParam);
            break;
        case Controls::ActionList:
            ActionListBoxProc(hWnd, CODE, lParam);
            break;
        case Controls::NewTrigger:
            if (CODE == BN_CLICKED)
                OnClickNewTrigger();
            break;
        case Controls::CloneTrigger:
            if (CODE == BN_CLICKED)
                OnClickCloTrigger(hWnd);
            break;
        case Controls::DeleteTrigger:
            if (CODE == BN_CLICKED)
                OnClickDelTrigger(hWnd);
            break;
        case Controls::PlaceOnMap:
            if (CODE == BN_CLICKED)
                OnClickPlaceOnMap(hWnd);
            break;
        case Controls::NewEvent:
            if (CODE == BN_CLICKED)
                OnClickNewEvent(hWnd);
            break;
        case Controls::CloneEvent:
            if (CODE == BN_CLICKED)
                OnClickCloEvent(hWnd);
            break;
        case Controls::DeleteEvent:
            if (CODE == BN_CLICKED)
                OnClickDelEvent(hWnd);
            break;
        case Controls::NewAction:
            if (CODE == BN_CLICKED)
                OnClickNewAction(hWnd);
            break;
        case Controls::CloneAction:
            if (CODE == BN_CLICKED)
                OnClickCloAction(hWnd);
            break;
        case Controls::DeleteAction:
            if (CODE == BN_CLICKED)
                OnClickDelAction(hWnd);
            break;
        case Controls::SearchReference:
            if (CODE == BN_CLICKED)
                OnClickSearchReference(hWnd);
            break;
        case Controls::ActionMoveUp:
            if (CODE == STN_CLICKED)
                OnClickActionMove(hWnd, true);
            break;
        case Controls::ActionMoveDown:
            if (CODE == STN_CLICKED)
                OnClickActionMove(hWnd, false);
            break;
        case Controls::ActionSplit:
            if (CODE == BN_CLICKED)
                OnClickActionSplit(hWnd);
            break;
        case Controls::OpenNewEditor:
            if (CODE == BN_CLICKED && this == &Instance[0])
            {
                for (int i = 1; i < TRIGGER_EDITOR_MAX_COUNT; ++i)
                {
                    if (Instance[i].GetHandle() == NULL)
                    {
                        CNewTrigger::Instance[i].CompactMode = true;
                        CNewTrigger::Instance[i].Create(CFinalSunDlg::Instance);
                        break;
                    }
                }
            }
            break;
        case Controls::Name:
            if (CODE == EN_CHANGE && CurrentTrigger && !AutoChangeName)
            {
                CNewTeamTypes::TagListChanged = true;
                char buffer[512]{ 0 };
                GetWindowText(hName, buffer, 511);
                FString name(buffer);
                name.Replace(",", "");

                CurrentTrigger->Name = name;
                CurrentTrigger->TagName = name + " 1";
                CurrentTrigger->Save();

                DropNeedUpdate = true;

                auto newName = ExtraWindow::FormatTriggerDisplayName(CurrentTrigger->ID, CurrentTrigger->Name);

                SendMessage(hSelectedTrigger, CB_DELETESTRING, SelectedTriggerIndex, NULL);
                SendMessage(hSelectedTrigger, CB_INSERTSTRING, SelectedTriggerIndex, (LPARAM)(LPCSTR)newName.c_str());
                SendMessage(hSelectedTrigger, CB_SETCURSEL, SelectedTriggerIndex, NULL);

                int hAttachedtriggerCur = SendMessage(hAttachedtrigger, CB_GETCURSEL, NULL, NULL);
                SendMessage(hAttachedtrigger, CB_DELETESTRING, SelectedTriggerIndex + 1, NULL);
                SendMessage(hAttachedtrigger, CB_INSERTSTRING, SelectedTriggerIndex + 1, (LPARAM)(LPCSTR)newName.c_str());
                SendMessage(hAttachedtrigger, CB_SETCURSEL, hAttachedtriggerCur, NULL);

                if (CurrentTriggerActionParam > -1)
                {
                    int hActionParameterCur = SendMessage(hActionParameter[CurrentTriggerActionParam], CB_GETCURSEL, NULL, NULL);
                    SendMessage(hActionParameter[CurrentTriggerActionParam], CB_DELETESTRING, SelectedTriggerIndex, NULL);
                    SendMessage(hActionParameter[CurrentTriggerActionParam], CB_INSERTSTRING, SelectedTriggerIndex, (LPARAM)(LPCSTR)newName.c_str());
                    SendMessage(hActionParameter[CurrentTriggerActionParam], CB_SETCURSEL, hActionParameterCur, NULL);
                }

                auto others = GetOtherInstances();
                bool needRefresh = false;
                for (auto& [i, other] : others)
                {
                    if (other->GetHandle())
                    {
                        needRefresh = true;
                        other->DropNeedUpdate = true;

                        SendMessage(other->hSelectedTrigger, CB_DELETESTRING, SelectedTriggerIndex, NULL);
                        SendMessage(other->hSelectedTrigger, CB_INSERTSTRING, SelectedTriggerIndex, (LPARAM)(LPCSTR)newName.c_str());
                        SendMessage(other->hSelectedTrigger, CB_SETCURSEL, other->SelectedTriggerIndex, NULL);

                        hAttachedtriggerCur = SendMessage(other->hAttachedtrigger, CB_GETCURSEL, NULL, NULL);
                        SendMessage(other->hAttachedtrigger, CB_DELETESTRING, SelectedTriggerIndex + 1, NULL);
                        SendMessage(other->hAttachedtrigger, CB_INSERTSTRING, SelectedTriggerIndex + 1, (LPARAM)(LPCSTR)newName.c_str());
                        SendMessage(other->hAttachedtrigger, CB_SETCURSEL, hAttachedtriggerCur, NULL);

                        if (other->CurrentTriggerActionParam > -1)
                        {
                            int hActionParameterCur = SendMessage(other->hActionParameter[other->CurrentTriggerActionParam], CB_GETCURSEL, NULL, NULL);
                            SendMessage(other->hActionParameter[other->CurrentTriggerActionParam], CB_DELETESTRING, SelectedTriggerIndex, NULL);
                            SendMessage(other->hActionParameter[other->CurrentTriggerActionParam], CB_INSERTSTRING, SelectedTriggerIndex, (LPARAM)(LPCSTR)newName.c_str());
                            SendMessage(other->hActionParameter[other->CurrentTriggerActionParam], CB_SETCURSEL, hActionParameterCur, NULL);
                        }
                    }
                }
                if (needRefresh)
                    RefreshOtherInstances();
            }
            break;
        case Controls::Disabled:
            if (CODE == BN_CLICKED && CurrentTrigger)
            {
                CurrentTrigger->Disabled = SendMessage(hDisabled, BM_GETCHECK, 0, 0);
                CurrentTrigger->Save();
                RefreshOtherInstances();
            }
            break;
        case Controls::Easy:
            if (CODE == BN_CLICKED && CurrentTrigger)
            {
                CurrentTrigger->EasyEnabled = SendMessage(hEasy, BM_GETCHECK, 0, 0);
                CurrentTrigger->Save();
                RefreshOtherInstances();
            }
            break;
        case Controls::Medium:
            if (CODE == BN_CLICKED && CurrentTrigger)
            {
                CurrentTrigger->MediumEnabled = SendMessage(hMedium, BM_GETCHECK, 0, 0);
                CurrentTrigger->Save();
                RefreshOtherInstances();
            }
            break;
        case Controls::Hard:
            if (CODE == BN_CLICKED && CurrentTrigger)
            {
                CurrentTrigger->HardEnabled = SendMessage(hHard, BM_GETCHECK, 0, 0);
                CurrentTrigger->Save();
                RefreshOtherInstances();
            }
            break;
        case Controls::Compact:
            if (CODE == BN_CLICKED && this != &Instance[0])
            {
                CompactMode = SendMessage(hCompact, BM_GETCHECK, 0, 0);
                RECT rc;
                GetWindowRect(m_hwnd, &rc);
                windowPos.x = rc.left;
                windowPos.y = rc.top;               
                Close(hWnd);
                Create(CFinalSunDlg::Instance);
                return TRUE;
            }
            break;
        case Controls::SelectedTrigger:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTrigger();
            else if (CODE == CBN_DROPDOWN)
                OnSeldropdownTrigger(hWnd);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeTrigger(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hSelectedTrigger, TriggerLabels, true);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::House:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeHouse();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeHouse(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hHouse, HouseLabels);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::Attachedtrigger:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeAttachedTrigger();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeAttachedTrigger(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hAttachedtrigger, AttachedTriggerLabels);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            else if (CODE == CBN_DROPDOWN && DropNeedUpdate)
            {
                SortTriggers(CurrentTrigger->ID);
                DropNeedUpdate = false;
            }
            break;
        case Controls::Type:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeType();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeType(true);
            break;
        case Controls::Eventtype:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeEventType();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeEventType(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hEventtype, EventTypeLabels);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::EventParameter1:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeEventParam(0);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeEventParam(0, true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hEventParameter[0], EventParamLabels[0]);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            else if (CODE == CBN_DROPDOWN && DropNeedUpdate)
            {
                SortTriggers(CurrentTrigger->ID);
                int idx = SendMessage(hAttachedtrigger, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetTriggerDisplayName(CurrentTrigger->AttachedTrigger).c_str());
                SendMessage(hAttachedtrigger, CB_SETCURSEL, idx, NULL);
                DropNeedUpdate = false;
            }
            break;
        case Controls::EventParameter2:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeEventParam(1);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeEventParam(1, true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hEventParameter[1], EventParamLabels[1]);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            else if (CODE == CBN_DROPDOWN && DropNeedUpdate)
            {
                SortTriggers(CurrentTrigger->ID);
                int idx = SendMessage(hAttachedtrigger, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetTriggerDisplayName(CurrentTrigger->AttachedTrigger).c_str());
                SendMessage(hAttachedtrigger, CB_SETCURSEL, idx, NULL);
                DropNeedUpdate = false;
            }
            break;
        case Controls::Actiontype:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeActionType();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeActionType(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hActiontype, ActionTypeLabels);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::ActionParameter1:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeActionParam(0);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeActionParam(0, true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hActionParameter[0], ActionParamLabels[0]);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            else if (CODE == CBN_DROPDOWN)
                OnDropdownCComboBox(0);
            break;
        case Controls::ActionParameter2:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeActionParam(1);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeActionParam(1, true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hActionParameter[1], ActionParamLabels[1]);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            else if (CODE == CBN_DROPDOWN)
                OnDropdownCComboBox(1);
            break;
        case Controls::ActionParameter3:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeActionParam(2);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeActionParam(2, true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hActionParameter[2], ActionParamLabels[2]);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            else if (CODE == CBN_DROPDOWN)
                OnDropdownCComboBox(2);
            break;
        case Controls::ActionParameter4:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeActionParam(3);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeActionParam(3, true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hActionParameter[3], ActionParamLabels[3]);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            else if (CODE == CBN_DROPDOWN)
                OnDropdownCComboBox(3);
            break;
        case Controls::ActionParameter5:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeActionParam(4);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeActionParam(4, true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hActionParameter[4], ActionParamLabels[4]);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            else if (CODE == CBN_DROPDOWN)
                OnDropdownCComboBox(4);
            break;
        case Controls::ActionParameter6:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeActionParam(5);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeActionParam(5, true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hActionParameter[5], ActionParamLabels[5]);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            else if (CODE == CBN_DROPDOWN)
                OnDropdownCComboBox(5);
            break;
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CNewTrigger::Close(hWnd);
        return TRUE;
    }
    case WM_MOVE:
    case WM_SIZE:
    {
        ExtraWindow::UpdateDropTargetRect(hWnd);
        break;
    }
    case 114514: // used for update
    {
        if (this == &Instance[0])
            Update(hWnd);
        else
            Update(hWnd, Instance[0].GetHandle() == NULL);
        return TRUE;
    }

    }

    // Process this message through default handler
    return FALSE;
}

void CNewTrigger::EventListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam)
{
    if (SendMessage(hEventList, LB_GETCOUNT, NULL, NULL) <= 0)
        return;

    switch (nCode)
    {
    case LBN_SELCHANGE:
    case LBN_DBLCLK:
        OnSelchangeEventListbox();
        break;
    default:
        break;
    }

}

void CNewTrigger::ActionListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam)
{
    if (SendMessage(hActionList, LB_GETCOUNT, NULL, NULL) <= 0)
        return;

    switch (nCode)
    {
    case LBN_SELCHANGE:
    case LBN_DBLCLK:
        OnSelchangeActionListbox();
        break;
    default:
        break;
    }

}

void CNewTrigger::OnSelchangeEventListbox(bool changeCursel)
{
    if (SelectedTriggerIndex < 0 || SendMessage(hEventList, LB_GETCURSEL, NULL, NULL) < 0 || SendMessage(hEventList, LB_GETCOUNT, NULL, NULL) <= 0)
    {
        SendMessage(hEventtype, CB_SETCURSEL, -1, NULL);
        for (int i = 0; i < EVENT_PARAM_COUNT; ++i) {
            SendMessage(hEventParameter[i], CB_SETCURSEL, -1, NULL);
            SendMessage(hEventParameterDesc[i], WM_SETTEXT, 0, (LPARAM)"");
            EnableWindow(hEventParameter[i], FALSE);
        }
        if(!CompactMode)
            SendMessage(hEventDescription, WM_SETTEXT, 0, (LPARAM)"");
        return;
    }

    int idx = SendMessage(hEventList, LB_GETCURSEL, 0, NULL);
    SelectedEventIndex = idx;

    UpdateEventAndParam(-1, changeCursel);

    for (int i = 0; i < EVENT_PARAM_COUNT; ++i)
    {
        if (EventParamsUsage[i].first)
        {
            FString value = CurrentTrigger->Events[SelectedEventIndex].Params[EventParamsUsage[i].second];
            int paramIdx = ExtraWindow::FindCBStringExactStart(hEventParameter[i], value + " ");
            if (paramIdx == CB_ERR)
                paramIdx = SendMessage(hEventParameter[i], CB_FINDSTRINGEXACT, 0, (LPARAM)value.c_str());

            if (paramIdx != CB_ERR)
            {
                SendMessage(hEventParameter[i], CB_SETCURSEL, paramIdx, NULL);
            }
            else
            {
                SendMessage(hEventParameter[i], CB_SETCURSEL, -1, NULL);
                SendMessage(hEventParameter[i], WM_SETTEXT, 0, (LPARAM)value.c_str());
            }
        }
    }
}

void CNewTrigger::OnSelchangeActionListbox(bool changeCursel, int index)
{
    if (SelectedTriggerIndex < 0 || SendMessage(hActionList, LB_GETCARETINDEX, NULL, NULL) < 0 || SendMessage(hActionList, LB_GETCOUNT, NULL, NULL) <= 0)
    {
        SendMessage(hActiontype, CB_SETCURSEL, -1, NULL);
        for (int i = 0; i < ACTION_PARAM_COUNT; ++i) {

            SendMessage(hActionParameter[i], CB_SETCURSEL, -1, NULL);
            SendMessage(hActionParameterDesc[i], WM_SETTEXT, 0, (LPARAM)"");
            EnableWindow(hActionParameter[i], FALSE);
        }
        if (!CompactMode)
            SendMessage(hActionDescription, WM_SETTEXT, 0, (LPARAM)"");

        if (WindowShown)
        {
            ActionParamsCount = 4;
            AdjustActionHeight();
            LastActionParamsCount = ActionParamsCount;
        }

        return;
    }

    if (index != -1)
    {
        SelectedActionIndex = index;        
        SetActionListBoxSel(SelectedActionIndex);
    }
    else
    {
        int idx = SendMessage(hActionList, LB_GETCARETINDEX, 0, NULL);
        SelectedActionIndex = idx;
    }

    UpdateActionAndParam(-1, changeCursel);

    for (int i = 0; i < ACTION_PARAM_COUNT; ++i)
    {
        if (ActionParamsUsage[i].first)
        {
            FString value = CurrentTrigger->Actions[SelectedActionIndex].Params[ActionParamsUsage[i].second];
            FString valueOri = CurrentTrigger->Actions[SelectedActionIndex].Params[ActionParamsUsage[i].second];
            if (ActionParamsUsage[i].second == 6 && CurrentTrigger->Actions[SelectedActionIndex].Param7isWP)
            {
                value = STDHelpers::StringToWaypointStr(value);
            }
            else if (ActionParamUsesFloat)
            {
                unsigned int nValue;
                if (sscanf_s(value, "%u", &nValue) == 1)
                {
                    auto f = *(float*)&nValue;
                    if (f > 0.0f)
                        f = std::max(f, 0.000001f);
                    else if (f < 0.0f)
                        f = std::min(f, -0.000001f);
                    value.Format("%f", f);
                }
            }

            if (CurrentCSFActionParam == i)
                value.MakeLower();

            int paramIdx = ExtraWindow::FindCBStringExactStart(hActionParameter[i], value + " ");

            if (paramIdx == CB_ERR)
                paramIdx = SendMessage(hActionParameter[i], CB_FINDSTRINGEXACT, 0, (LPARAM)value.c_str());

            if (paramIdx != CB_ERR)
            {
                SendMessage(hActionParameter[i], CB_SETCURSEL, paramIdx, NULL);
            }
            else
            {
                if (CurrentCSFActionParam == i && ExtConfigs::TutorialTexts_Viewer)
                {
                    FString text = valueOri;
                    auto it = CCsfEditor::CurrentCSFMap.find(value);
                    if (it != CCsfEditor::CurrentCSFMap.end())
                        text += " - " + CCsfEditor::CurrentCSFMap[value];
                    SendMessage(hActionParameter[i], CB_SETCURSEL, -1, NULL);
                    SendMessage(hActionParameter[i], WM_SETTEXT, 0, (LPARAM)text.c_str());
                }
                else
                {
                    SendMessage(hActionParameter[i], CB_SETCURSEL, -1, NULL);
                    if (CurrentCSFActionParam == i)
                        SendMessage(hActionParameter[i], WM_SETTEXT, 0, (LPARAM)valueOri.c_str());
                    else
                        SendMessage(hActionParameter[i], WM_SETTEXT, 0, (LPARAM)value.c_str());
                }
            }
        }
    }
}

void CNewTrigger::OnSelchangeAttachedTrigger(bool edited)
{
    if (SelectedTriggerIndex < 0 || SendMessage(hAttachedtrigger, LB_GETCURSEL, NULL, NULL) < 0 || !CurrentTrigger)
        return;
    int curSel = SendMessage(hAttachedtrigger, CB_GETCURSEL, NULL, NULL);

    FString text;
    char buffer[512]{ 0 };

    if (edited && (SendMessage(hAttachedtrigger, CB_GETCOUNT, NULL, NULL) > 0 || !AttachedTriggerLabels.empty()))
    {
        ExtraWindow::OnEditCComboBox(hAttachedtrigger, AttachedTriggerLabels);
    }

    if (curSel >= 0 && curSel < SendMessage(hAttachedtrigger, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hAttachedtrigger, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hAttachedtrigger, buffer, 511);

        text = buffer;
        int idx = SendMessage(hAttachedtrigger, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetTriggerDisplayName(buffer).c_str());
        if (idx != CB_ERR)
        {
            SendMessage(hAttachedtrigger, CB_GETLBTEXT, idx, (LPARAM)buffer);
            text = buffer;
        }
    }

    if (!text)
        return;

    FString::TrimIndex(text);
    if (text == "")
        text = "<none>";

    text.Replace(",", "");

    if (text == CurrentTrigger->ID)
    {
        FString pMessage = Translations::TranslateOrDefault("TriggerAttachedTriggerSelf",
            "A trigger's attached trigger CANNOT be itself. \nDo you want to continue?");

        int nResult = ::MessageBox(GetHandle(), pMessage, Translations::TranslateOrDefault("Error", "Error"), MB_YESNO | MB_ICONWARNING);
        if (nResult == IDNO)
        {
            int idx = SendMessage(hAttachedtrigger, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetTriggerDisplayName(CurrentTrigger->AttachedTrigger).c_str());
            if (idx != CB_ERR)
            {
                SendMessage(hAttachedtrigger, CB_SETCURSEL, idx, NULL);
            }
            else
            {
                SendMessage(hAttachedtrigger, CB_SETCURSEL, 0, NULL);
                CurrentTrigger->AttachedTrigger = "<none>";
                CurrentTrigger->Save();
            }
            return;
        }
    }

    CurrentTrigger->AttachedTrigger = text;
    CurrentTrigger->Save();

    RefreshOtherInstances();
}

void CNewTrigger::OnSelchangeHouse(bool edited)
{
    if (SelectedTriggerIndex < 0 || SendMessage(hHouse, LB_GETCURSEL, NULL, NULL) < 0 || !CurrentTrigger)
        return;
    int curSel = SendMessage(hHouse, CB_GETCURSEL, NULL, NULL);

    FString text;
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hHouse, CB_GETCOUNT, NULL, NULL) > 0 || !HouseLabels.empty()))
    {
        ExtraWindow::OnEditCComboBox(hHouse, HouseLabels);
    }

    if (curSel >= 0 && curSel < SendMessage(hHouse, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hHouse, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hHouse, buffer, 511);
        text = buffer;
        int idx = SendMessage(hHouse, CB_FINDSTRINGEXACT, 0, (LPARAM)text.c_str());
        //if (idx == CB_ERR)
        //    idx = SendMessage(hHouse, CB_FINDSTRINGEXACT, 0, (LPARAM)FString::ParseHouseName(text, true).m_pchData);
        if (idx != CB_ERR)
        {
            SendMessage(hHouse, CB_GETLBTEXT, idx, (LPARAM)buffer);
            text = buffer;
        }
    }

    if (!text)
        return;

    if (text.find("<Player @") == std::string::npos)
        FString::TrimIndex(text);

    if (text == "")
        text = "<none>";

    text.Replace(",", "");

    CurrentTrigger->House = Translations::ParseHouseName(text, false);
    CurrentTrigger->Save();

    RefreshOtherInstances();
}

void CNewTrigger::OnSelchangeType(bool edited)
{
    if (SelectedTriggerIndex < 0 || SendMessage(hType, LB_GETCURSEL, NULL, NULL) < 0 || !CurrentTrigger)
        return;
    int curSel = SendMessage(hType, CB_GETCURSEL, NULL, NULL);

    FString text;
    char buffer[512]{ 0 };

    if (curSel >= 0 && curSel < SendMessage(hType, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hType, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hType, buffer, 511);
        text = buffer;
    }

    if (!text)
        return;

    FString::TrimIndex(text);
    if (text == "")
        text = "0";

    text.Replace(",", "");

    CurrentTrigger->RepeatType = text;
    CurrentTrigger->Save();

    RefreshOtherInstances();
}

void CNewTrigger::OnSelchangeEventType(bool edited)
{
    if (SelectedTriggerIndex < 0 || SendMessage(hEventtype, LB_GETCURSEL, NULL, NULL) < 0 || !CurrentTrigger)
        return;
    int curSel = SendMessage(hEventtype, CB_GETCURSEL, NULL, NULL);

    FString text;
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hEventtype, CB_GETCOUNT, NULL, NULL) > 0 || !EventTypeLabels.empty()))
    {
        ExtraWindow::OnEditCComboBox(hEventtype, EventTypeLabels);
    }

    if (curSel >= 0 && curSel < SendMessage(hEventtype, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hEventtype, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hEventtype, buffer, 511);
        text = buffer;
    }

    if (!text)
        return;

    FString::TrimIndex(text);
    if (text == "")
        text = "0";

    text.Replace(",", "");

    UpdateEventAndParam(atoi(text), false); 
    OnSelchangeEventListbox(false);

    RefreshOtherInstances();
}

void CNewTrigger::OnSelchangeActionType(bool edited)
{
    if (SelectedTriggerIndex < 0 || SendMessage(hActiontype, LB_GETCURSEL, NULL, NULL) < 0 || !CurrentTrigger)
        return;
    int curSel = SendMessage(hActiontype, CB_GETCURSEL, NULL, NULL);
    int listSel = SendMessage(hActionList, LB_GETCARETINDEX, NULL, NULL);

    FString text;
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hActiontype, CB_GETCOUNT, NULL, NULL) > 0 || !ActionTypeLabels.empty()))
    {
        ExtraWindow::OnEditCComboBox(hActiontype, ActionTypeLabels);
    }

    if (curSel >= 0 && curSel < SendMessage(hActiontype, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hActiontype, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hActiontype, buffer, 511);
        text = buffer;
    }

    if (!text)
        return;

    FString::TrimIndex(text);
    if (text == "")
        text = "0";

    text.Replace(",", "");

    UpdateActionAndParam(atoi(text), false); 
    OnSelchangeActionListbox(false, listSel);

    RefreshOtherInstances();
}

void CNewTrigger::OnSelchangeEventParam(int index, bool edited)
{
    if (SelectedTriggerIndex < 0 || SelectedEventIndex < 0 || !CurrentTrigger || index < 0 || index > 2 || !EventParamsUsage[index].first)
        return;
    int curSel = SendMessage(hEventParameter[index], CB_GETCURSEL, NULL, NULL);

    FString text;
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hEventParameter[index], CB_GETCOUNT, NULL, NULL) > 0 || !EventParamLabels[index].empty())
        && CNewTrigger::EventParameterAutoDrop[index])
    {
        ExtraWindow::OnEditCComboBox(hEventParameter[index], EventParamLabels[index]);
    }

    if (curSel >= 0 && curSel < SendMessage(hEventParameter[index], CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hEventParameter[index], CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hEventParameter[index], buffer, 511);
        text = buffer;
    }

    if (!text)
        return;

    ExtraWindow::TrimStringIndex(text);
    if (text == "")
        text = "0";

    text.Replace(",", "");

    CurrentTrigger->Events[SelectedEventIndex].Params[EventParamsUsage[index].second] = text;
    CurrentTrigger->Save();

    UpdateParamAffectedParam_Event(index);

    RefreshOtherInstances();
}

void CNewTrigger::OnSelchangeActionParam(int index, bool edited)
{
    if (SelectedTriggerIndex < 0 || SelectedActionIndex < 0 || !CurrentTrigger || index < 0 || index > 5 || !ActionParamsUsage[index].first)
        return;
    int curSel = SendMessage(hActionParameter[index], CB_GETCURSEL, NULL, NULL);

    FString text;
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hActionParameter[index], CB_GETCOUNT, NULL, NULL) > 0 || !ActionParamLabels[index].empty())
        && CNewTrigger::ActionParameterAutoDrop[index])
    {
        ExtraWindow::OnEditCComboBox(hActionParameter[index], ActionParamLabels[index]);
    }

    if (curSel >= 0 && curSel < SendMessage(hActionParameter[index], CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hActionParameter[index], CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hActionParameter[index], buffer, 511);
        text = buffer;
    }

    if (!text)
        return;

    if (CurrentTriggerActionParam == index || CurrentTeamActionParam == index)
        FString::TrimIndex(text);
    else
        ExtraWindow::TrimStringIndex(text);

    if (text == "")
        text = "0";

    text.Replace(",", "");

    if (ActionParamsUsage[index].second == 6 && CurrentTrigger->Actions[SelectedActionIndex].Param7isWP)
    {
        text = STDHelpers::WaypointToString(text);
    }
    else if (ActionParamUsesFloat)
    {
        float fValue;
        if (sscanf_s(text, "%f", &fValue) == 1)
            text.Format("%010u", *(unsigned int*)&fValue);
    }

    CurrentTrigger->Actions[SelectedActionIndex].Params[ActionParamsUsage[index].second] = text;
    CurrentTrigger->Save();

    UpdateParamAffectedParam_Action(index);

    RefreshOtherInstances();
}

void CNewTrigger::UpdateParamAffectedParam_Action(int index)
{
    auto& affList = ActionParamAffectedParams;
    for (auto& target : affList)
    {
        if (target.Index == atoi(CurrentTrigger->Actions[SelectedActionIndex].ActionNum) && target.SourceParam == index)
        {
            auto& text = CurrentTrigger->Actions[SelectedActionIndex].Params[ActionParamsUsage[index].second];
            if (target.ParamMap.find(text) != target.ParamMap.end())
            {
                auto paramType = FString::SplitString(CINI::FAData->GetString(ExtraWindow::GetTranslatedSectionName("ParamTypes"), target.ParamMap[text]), 1);
                ExtraWindow::LoadParams(hActionParameter[target.AffectedParam], paramType[1], this);
                //SendMessage(hActionParameterDesc[target.AffectedParam], WM_SETTEXT, 0, (LPARAM)paramType[0].m_pchData);
                if (paramType[1] == "10") // stringtables
                {
                    CurrentCSFActionParam = target.AffectedParam;
                }
                else if (paramType[1] == "9") // triggers
                {
                    CurrentTriggerActionParam = target.AffectedParam;
                }
                else if (paramType[1] == "15" || FString::SplitString(
                    fadata.GetString(
                        "NewParamTypes",
                        paramType[1]), size_t(0))[0]
                    == "TeamTypes")
                {
                    CurrentTeamActionParam = target.AffectedParam;
                }
                ExtraWindow::AdjustDropdownWidth(hActionParameter[target.AffectedParam]);

                auto& targetText = CurrentTrigger->Actions[SelectedActionIndex].Params[ActionParamsUsage[target.AffectedParam].second];
                int paramIdx = ExtraWindow::FindCBStringExactStart(hActionParameter[target.AffectedParam], targetText + " ");
                if (paramIdx == CB_ERR)
                    paramIdx = SendMessage(hActionParameter[target.AffectedParam], CB_FINDSTRINGEXACT, 0, (LPARAM)targetText.c_str());

                if (paramIdx != CB_ERR)
                {
                    SendMessage(hActionParameter[target.AffectedParam], CB_SETCURSEL, paramIdx, NULL);
                }
                else
                {
                    SendMessage(hActionParameter[target.AffectedParam], CB_SETCURSEL, -1, NULL);
                    SendMessage(hActionParameter[target.AffectedParam], WM_SETTEXT, 0, (LPARAM)targetText.c_str());
                }
            } 
        }
    }
}

void CNewTrigger::UpdateParamAffectedParam_Event(int index)
{
    auto& affList = EventParamAffectedParams;
    for (auto& target : affList)
    {
        if (target.Index == atoi(CurrentTrigger->Events[SelectedEventIndex].EventNum) && target.SourceParam == index)
        {
            auto& text = CurrentTrigger->Events[SelectedEventIndex].Params[EventParamsUsage[index].second];
            if (target.ParamMap.find(text) != target.ParamMap.end())
            {
                auto paramType = FString::SplitString(CINI::FAData->GetString(ExtraWindow::GetTranslatedSectionName("ParamTypes"), target.ParamMap[text]), 1);
                ExtraWindow::LoadParams(hEventParameter[target.AffectedParam], paramType[1], this);
                //SendMessage(hEventParameterDesc[target.AffectedParam], WM_SETTEXT, 0, (LPARAM)paramType[0].c_str());
                ExtraWindow::AdjustDropdownWidth(hEventParameter[target.AffectedParam]);

                auto& targetText = CurrentTrigger->Events[SelectedEventIndex].Params[EventParamsUsage[target.AffectedParam].second];
                int paramIdx = ExtraWindow::FindCBStringExactStart(hEventParameter[target.AffectedParam], targetText + " ");
                if (paramIdx == CB_ERR)
                    paramIdx = SendMessage(hEventParameter[target.AffectedParam], CB_FINDSTRINGEXACT, 0, (LPARAM)targetText.c_str());

                if (paramIdx != CB_ERR)
                {
                    SendMessage(hEventParameter[target.AffectedParam], CB_SETCURSEL, paramIdx, NULL);
                }
                else
                {
                    SendMessage(hEventParameter[target.AffectedParam], CB_SETCURSEL, -1, NULL);
                    SendMessage(hEventParameter[target.AffectedParam], WM_SETTEXT, 0, (LPARAM)targetText.c_str());
                }
            }
        }
    }
}

void CNewTrigger::OnSelchangeTrigger(bool edited, int eventListCur, int actionListCur, bool reloadTrigger)
{
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hSelectedTrigger, CB_GETCOUNT, NULL, NULL) > 0 || !TriggerLabels.empty()))
    {
        Autodrop = true;
        ExtraWindow::OnEditCComboBox(hSelectedTrigger, TriggerLabels);
        return;
    }

    SelectedTriggerIndex = SendMessage(hSelectedTrigger, CB_GETCURSEL, NULL, NULL);
    if (SelectedTriggerIndex < 0 || SelectedTriggerIndex >= SendMessage(hSelectedTrigger, CB_GETCOUNT, NULL, NULL))
    {
        SelectedTriggerIndex = -1;
        CurrentTrigger = nullptr;
        SendMessage(hEventtype, CB_SETCURSEL, -1, NULL);
        SendMessage(hActiontype, CB_SETCURSEL, -1, NULL);
        for (int i = 0; i < EVENT_PARAM_COUNT; ++i)
            SendMessage(hEventParameter[i], CB_SETCURSEL, -1, NULL);
        for (int i = 0; i < ACTION_PARAM_COUNT; ++i)
            SendMessage(hActionParameter[i], CB_SETCURSEL, -1, NULL);      
        SendMessage(hHouse, CB_SETCURSEL, -1, NULL);
        SendMessage(hType, CB_SETCURSEL, -1, NULL);
        SendMessage(hAttachedtrigger, CB_SETCURSEL, -1, NULL);
        SendMessage(hDisabled, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hEasy, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hHard, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hMedium, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hEventList, LB_SETCURSEL, -1, NULL);
        SetActionListBoxSel(-1);
        if (!CompactMode)
            SendMessage(hEventDescription, WM_SETTEXT, 0, (LPARAM)"");
        if (!CompactMode)
            SendMessage(hActionDescription, WM_SETTEXT, 0, (LPARAM)"");
        AutoChangeName = true;
        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)"");
        AutoChangeName = false;
        while (SendMessage(hEventList, LB_DELETESTRING, 0, NULL) != CB_ERR);
        while (SendMessage(hActionList, LB_DELETESTRING, 0, NULL) != CB_ERR);
        OnSelchangeActionListbox();
        OnSelchangeEventListbox();

        InvalidateRect(hDragPoint, nullptr, TRUE);
        return;
    }

    FString pID;
    SendMessage(hSelectedTrigger, CB_GETLBTEXT, SelectedTriggerIndex, (LPARAM)buffer);
    pID = buffer;
    FString::TrimIndex(pID);

    CurrentTriggerID = pID;

    if (reloadTrigger)
        CMapDataExt::ReloadTrigger(CurrentTriggerID);
    CurrentTrigger = CMapDataExt::GetTrigger(CurrentTriggerID);
    InvalidateRect(hDragPoint, nullptr, TRUE);

    if (!CurrentTrigger) return;

    CTriggerAnnotation::Type = AnnoTrigger;
    CTriggerAnnotation::ID = CurrentTriggerID;
    ::SendMessage(CTriggerAnnotation::GetHandle(), 114515, 0, 0);

    AutoChangeName = true;
    SendMessage(hName, WM_SETTEXT, 0, (LPARAM)CurrentTrigger->Name.c_str());
    AutoChangeName = false;
    
    int houseidx = SendMessage(hHouse, CB_FINDSTRINGEXACT, 0, (LPARAM)Translations::ParseHouseName(CurrentTrigger->House, true).c_str());
    if (houseidx != CB_ERR)
        SendMessage(hHouse, CB_SETCURSEL, houseidx, NULL);
    else
        SendMessage(hHouse, WM_SETTEXT, 0, (LPARAM)CurrentTrigger->House.c_str());
    
    int repeat = atoi(CurrentTrigger->RepeatType);
    if (repeat >= 0 && repeat <= 2)
        SendMessage(hType, CB_SETCURSEL, repeat, NULL);
    else if (repeat == -1)
        SendMessage(hType, WM_SETTEXT, 0, (FString("-1 - ") + Translations::TranslateOrDefault("TriggerRepeatType.NoTag", "No tag")));
    else
        SendMessage(hType, WM_SETTEXT, 0, (LPARAM)CurrentTrigger->RepeatType.c_str());
    
    int attachedTriggerIdx = SendMessage(hAttachedtrigger, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetTriggerDisplayName(CurrentTrigger->AttachedTrigger).c_str());
    if (attachedTriggerIdx != CB_ERR)
        SendMessage(hAttachedtrigger, CB_SETCURSEL, attachedTriggerIdx, NULL);
    else
        SendMessage(hAttachedtrigger, WM_SETTEXT, 0, (LPARAM)CurrentTrigger->AttachedTrigger.c_str());
    
    SendMessage(hDisabled, BM_SETCHECK, CurrentTrigger->Disabled, 0);
    SendMessage(hEasy, BM_SETCHECK, CurrentTrigger->EasyEnabled, 0);
    SendMessage(hHard, BM_SETCHECK, CurrentTrigger->HardEnabled, 0);
    SendMessage(hMedium, BM_SETCHECK, CurrentTrigger->MediumEnabled, 0);
    
    while (SendMessage(hEventList, LB_DELETESTRING, 0, NULL) != CB_ERR);
    while (SendMessage(hActionList, LB_DELETESTRING, 0, NULL) != CB_ERR);
    for (int i = 0; i < CurrentTrigger->EventCount; i++)
    {
        SendMessage(hEventList, LB_INSERTSTRING, i, (LPARAM)(LPCSTR)ExtraWindow::GetEventDisplayName(CurrentTrigger->Events[i].EventNum, i, !CompactMode));
    }
    for (int i = 0; i < CurrentTrigger->ActionCount; i++)
    {
        SendMessage(hActionList, LB_INSERTSTRING, i, (LPARAM)(LPCSTR)ExtraWindow::GetActionDisplayName(CurrentTrigger->Actions[i].ActionNum, i, !CompactMode));
    }

    if (SelectedEventIndex < 0)
        SelectedEventIndex = 0;
    if (SelectedEventIndex >= SendMessage(hEventList, LB_GETCOUNT, NULL, NULL))
        SelectedEventIndex = SendMessage(hEventList, LB_GETCOUNT, NULL, NULL) - 1;
    SendMessage(hEventList, LB_SETCURSEL, eventListCur, NULL);
    OnSelchangeEventListbox();

    if (SelectedActionIndex < 0)
        SelectedActionIndex = 0;
    if (SelectedActionIndex >= SendMessage(hActionList, LB_GETCOUNT, NULL, NULL))
        SelectedActionIndex = SendMessage(hActionList, LB_GETCOUNT, NULL, NULL) - 1;
    SetActionListBoxSel(actionListCur);
    OnSelchangeActionListbox();

    DropNeedUpdate = false;
}

void CNewTrigger::OnSeldropdownTrigger(HWND& hWnd)
{
    if (Autodrop)
    {
        Autodrop = false;
        return;
    }
    if (!CurrentTrigger)
        return;
    if (!DropNeedUpdate)
        return;

    DropNeedUpdate = false;
    auto others = GetOtherInstances();
    for (auto& [i, o] : others)
        o->DropNeedUpdate = false;

    SortTriggers(CurrentTrigger->ID);

    int idx = SendMessage(hAttachedtrigger, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetTriggerDisplayName(CurrentTrigger->AttachedTrigger).c_str());
    SendMessage(hAttachedtrigger, CB_SETCURSEL, idx, NULL);
}

void CNewTrigger::OnClickNewTrigger()
{
    TempValueHolder<bool> tmp(AutoChangeName, true);
    CNewTeamTypes::TagListChanged = true;
    FString id = CMapDataExt::GetAvailableIndex();
    FString value;
    FString house;
    char buffer[512]{ 0 };
    if (SendMessage(hHouse, CB_GETCOUNT, NULL, NULL) > 0)
    {
        SendMessage(hHouse, CB_GETLBTEXT, 0, (LPARAM)buffer);
        house = Translations::ParseHouseName(buffer, false);
    }
    else
        house = "Americans";

    FString newName =
        TriggerSort::CreateFromTriggerSort ?
        FString(TriggerSort::Instance.GetCurrentPrefix()) + "New Trigger" :
        FString("New Trigger");

    value.Format("%s,<none>,%s,0,1,1,1,0", house, newName);

    map.WriteString("Triggers", id, value);
    FString tagId = CMapDataExt::GetAvailableIndex();
    value.Format("0,%s 1,%s", newName, id);
    map.WriteString("Tags", tagId, value);

    CMapDataExt::AddTrigger(id);

    SortTriggers(id);

    OnSelchangeTrigger();
}

void CNewTrigger::OnClickCloTrigger(HWND& hWnd)
{
    if (!CurrentTrigger) return;
    TempValueHolder<bool> tmp(AutoChangeName, true);

    auto& oriID = CurrentTrigger->ID;
    auto& oriTagID = CurrentTrigger->Tag;

    FString id = CMapDataExt::GetAvailableIndex();
    FString value;
    auto& Name = CurrentTrigger->Name;

    FString newName = ExtraWindow::GetCloneName(Name);
    
    value.Format("%s,%s,%s,%s,%s,%s,%s,%s", CurrentTrigger->House, CurrentTrigger->AttachedTrigger, newName,
        CurrentTrigger->Disabled ? "1" : "0", CurrentTrigger->EasyEnabled ? "1" : "0",
        CurrentTrigger->MediumEnabled ? "1" : "0", CurrentTrigger->HardEnabled ? "1" : "0", CurrentTrigger->Obsolete);
    map.WriteString("Triggers", id, value);

    if (oriTagID != "<none>")
    {
        CNewTeamTypes::TagListChanged = true;
        FString tagId = CMapDataExt::GetAvailableIndex();
        value.Format("%s,%s 1,%s", CurrentTrigger->RepeatType, newName, id);
        map.WriteString("Tags", tagId, value);
    }

    map.WriteString("Events", id, map.GetString("Events", oriID));
    map.WriteString("Actions", id, map.GetString("Actions", oriID));

    CMapDataExt::AddTrigger(id);

    SortTriggers(id);

    OnSelchangeTrigger();
}

void CNewTrigger::OnClickDelTrigger(HWND& hWnd)
{
    if (!CurrentTrigger) return;
    TempValueHolder<bool> tmp(AutoChangeName, true);
    FString pMessage = Translations::TranslateOrDefault("TriggerDeleteMessage",
        "If you want to delete ALL attached tags, too, press Yes.\n"
        "If you don't want to delete these tags, press No.\n"
        "If you want to cancel to deletion of the trigger, press Cancel.\n"
        "Note: CellTags will be deleted too using this function if you press Yes.");

    int nResult = ::MessageBox(hWnd, pMessage, Translations::TranslateOrDefault("TriggerDeleteTitle", "Delete Trigger"), MB_YESNOCANCEL);
    if (nResult == IDYES || nResult == IDNO)
    {
        if (nResult == IDYES)
        {
            if (auto pTagsSection = map.GetSection("Tags"))
            {
                std::set<FString> TagsToRemove;
                for (auto& pair : pTagsSection->GetEntities())
                {
                    auto splits = FString::SplitString(pair.second, 2);
                    if (strcmp(splits[2], CurrentTrigger->ID) == 0)
                        TagsToRemove.insert(pair.first);
                }
                for (auto& tag : TagsToRemove)
                    map.DeleteKey("Tags", tag);

                if (!TagsToRemove.empty())
                    CNewTeamTypes::TagListChanged = true;

                if (auto pCellTagsSection = map.GetSection("CellTags"))
                {
                    std::vector<FString> CellTagsToRemove;
                    for (auto& pair : pCellTagsSection->GetEntities())
                    {
                        if (TagsToRemove.find(pair.second) != TagsToRemove.end())
                            CellTagsToRemove.push_back(pair.first);
                    }
                    for (auto& celltag : CellTagsToRemove)
                    {
                        map.DeleteKey("CellTags", celltag);
                        int nCoord = atoi(celltag);
                        int nMapCoord = CMapData::Instance->GetCoordIndex(nCoord % 1000, nCoord / 1000);
                        CMapData::Instance->CellDatas[nMapCoord].CellTag = -1;
                    }
                    CMapData::Instance->UpdateFieldCelltagData(FALSE);
                    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
                }

                if (CIsoView::CurrentCommand->Command == 4
                    && CIsoView::CurrentCommand->Type == 4
                    && CIsoView::CurrentCommand->ObjectID == CurrentTrigger->Tag
                    )
                {
                    CIsoView::CurrentCommand->Command = 0;
                    CIsoView::CurrentCommand->Type = 0;
                    CIsoView::CurrentCommand->ObjectID = "";
                }
            }
        }
        map.DeleteKey("Triggers", CurrentTrigger->ID);
        map.DeleteKey("Events", CurrentTrigger->ID);
        map.DeleteKey("Actions", CurrentTrigger->ID);
        CMapDataExt::DeleteTrigger(CurrentTrigger->ID);

        CurrentTrigger = nullptr;

        int idx = SelectedTriggerIndex;
        SendMessage(hSelectedTrigger, CB_DELETESTRING, idx, NULL);
        SendMessage(hAttachedtrigger, CB_DELETESTRING, idx + 1, NULL);
        auto others = GetOtherInstances();
        for (auto& [i, o] : others)
            if (o->GetHandle())
            {
                SendMessage(o->hSelectedTrigger, CB_DELETESTRING, idx, NULL);
                SendMessage(o->hAttachedtrigger, CB_DELETESTRING, idx + 1, NULL);
            }

        if (idx >= SendMessage(hSelectedTrigger, CB_GETCOUNT, NULL, NULL))
            idx--;
        if (idx < 0)
            idx = 0;
        SendMessage(hSelectedTrigger, CB_SETCURSEL, idx, NULL);
        OnSelchangeTrigger();

        for (auto& [i, o] : others)
            if (o->GetHandle())
            {
                o->CurrentTrigger = nullptr;
                int idx2 = o->SelectedTriggerIndex;
                if (idx2 >= SendMessage(o->hSelectedTrigger, CB_GETCOUNT, NULL, NULL))
                    idx2--;
                if (idx2 > idx)
                    idx2--;
                if (idx2 < 0)
                    idx2 = 0;
                SendMessage(o->hSelectedTrigger, CB_SETCURSEL, idx2, NULL);

                o->OnSelchangeTrigger(false,
                    o->SelectedEventIndex,
                    o->SelectedActionIndex,
                    false);
            }
    }
}

void CNewTrigger::OnClickPlaceOnMap(HWND& hWnd)
{
    if (!CurrentTrigger) return;
    if (CurrentTrigger->Tag.IsEmpty()) return;
    if (CurrentTrigger->Tag == "<none>") return;

    CIsoView::CurrentCommand->Command = 4;
    CIsoView::CurrentCommand->Type = 4;
    CIsoView::CurrentCommand->ObjectID = CurrentTrigger->Tag.c_str();
}

void CNewTrigger::OnClickNewEvent(HWND& hWnd)
{
    if (!CurrentTrigger) return;

    EventParams newEvent;
    newEvent.EventNum = "0";
    newEvent.Params[0] = "0";
    newEvent.Params[1] = "0";
    newEvent.P3Enabled = false;

    FString value;
    value.Format("%s=%s,%s,%s,%s", CurrentTrigger->ID, map.GetString("Events", CurrentTrigger->ID), newEvent.EventNum, newEvent.Params[0], newEvent.Params[1]);
    FString pMessage = Translations::TranslateOrDefault("TriggerEventLengthExceededMessage",
        "After creating the new event, the length of the event INI will exceed 511, and the excess will not work properly. \nDo you want to continue?");

    int nResult = IDYES;
    if (value.GetLength() >= 512)
        nResult = ::MessageBox(hWnd, pMessage, Translations::TranslateOrDefault("TriggerLengthExceededTitle", "Length Exceeded"), MB_YESNO | MB_ICONWARNING);

    if (nResult == IDYES)
    {
        CurrentTrigger->EventCount++;
        CurrentTrigger->Events.push_back(newEvent);
        CurrentTrigger->Save();

        while (SendMessage(hEventList, LB_DELETESTRING, 0, NULL) != CB_ERR);
        for (int i = 0; i < CurrentTrigger->EventCount; i++)
        {
            SendMessage(hEventList, LB_INSERTSTRING, i, (LPARAM)(LPCSTR)ExtraWindow::GetEventDisplayName(CurrentTrigger->Events[i].EventNum, i, !CompactMode));
        }

        SelectedEventIndex = SendMessage(hEventList, LB_GETCOUNT, NULL, NULL) - 1;
        SendMessage(hEventList, LB_SETCURSEL, SelectedEventIndex, NULL);
        OnSelchangeEventListbox();

        RefreshOtherInstances();
    }
}

void CNewTrigger::OnClickCloEvent(HWND& hWnd)
{
    if (!CurrentTrigger) return;
    if (SelectedEventIndex < 0 || SelectedEventIndex >= CurrentTrigger->EventCount) return;

    EventParams newEvent = CurrentTrigger->Events[SelectedEventIndex];

    FString value;
    value.Format("%s=%s,%s,%s,%s", CurrentTrigger->ID, map.GetString("Events", CurrentTrigger->ID), newEvent.EventNum, newEvent.Params[0], newEvent.Params[1]);
    if (newEvent.P3Enabled)
    {
        FString tmp = value;
        value.Format("%s,%s", tmp, newEvent.Params[2]);
    }

    FString pMessage = Translations::TranslateOrDefault("TriggerEventLengthExceededMessage",
        "After creating the new event, the length of the event INI will exceed 511, and the excess will not work properly. \nDo you want to continue?");

    int nResult = IDYES;
    if (value.GetLength() >= 512)
        nResult = ::MessageBox(hWnd, pMessage, Translations::TranslateOrDefault("TriggerLengthExceededTitle", "Length Exceeded"), MB_YESNO | MB_ICONWARNING);

    if (nResult == IDYES)
    {

        CurrentTrigger->EventCount++;
        CurrentTrigger->Events.push_back(newEvent);
        CurrentTrigger->Save();

        while (SendMessage(hEventList, LB_DELETESTRING, 0, NULL) != CB_ERR);
        for (int i = 0; i < CurrentTrigger->EventCount; i++)
        {
            SendMessage(hEventList, LB_INSERTSTRING, i, (LPARAM)(LPCSTR)ExtraWindow::GetEventDisplayName(CurrentTrigger->Events[i].EventNum, i, !CompactMode));
        }

        SelectedEventIndex = SendMessage(hEventList, LB_GETCOUNT, NULL, NULL) - 1;
        SendMessage(hEventList, LB_SETCURSEL, SelectedEventIndex, NULL);
        OnSelchangeEventListbox();

        RefreshOtherInstances();
    }
}

void CNewTrigger::OnClickDelEvent(HWND& hWnd)
{
    if (!CurrentTrigger) return;
    if (SelectedEventIndex < 0 || SelectedEventIndex >= CurrentTrigger->EventCount) return;

    CurrentTrigger->EventCount--;
    CurrentTrigger->Events.erase(CurrentTrigger->Events.begin() + SelectedEventIndex);
    CurrentTrigger->Save();

    while (SendMessage(hEventList, LB_DELETESTRING, 0, NULL) != CB_ERR);
    for (int i = 0; i < CurrentTrigger->EventCount; i++)
    {
        SendMessage(hEventList, LB_INSERTSTRING, i, (LPARAM)(LPCSTR)ExtraWindow::GetEventDisplayName(CurrentTrigger->Events[i].EventNum, i, !CompactMode));
    }

    SelectedEventIndex -= 1;
    if (SelectedEventIndex < 0)
        SelectedEventIndex = 0;
    if (SelectedEventIndex >= SendMessage(hEventList, LB_GETCOUNT, NULL, NULL))
        SelectedEventIndex = SendMessage(hEventList, LB_GETCOUNT, NULL, NULL) - 1;
    SendMessage(hEventList, LB_SETCURSEL, SelectedEventIndex, NULL);
    OnSelchangeEventListbox();

    RefreshOtherInstances();
}

void CNewTrigger::OnClickNewAction(HWND& hWnd)
{
    if (!CurrentTrigger) return;

    ActionParams newAction;
    newAction.ActionNum = "0";
    newAction.Params[0] = "0";
    newAction.Params[1] = "0";
    newAction.Params[2] = "0";
    newAction.Params[3] = "0";
    newAction.Params[4] = "0";
    newAction.Params[5] = "0";
    newAction.Params[6] = "A";
    newAction.Param7isWP = true;

    FString value;
    value.Format("%s=%s,%s,%s,%s,%s,%s,%s,%s,%s", CurrentTrigger->ID, map.GetString("Actions", CurrentTrigger->ID),
        newAction.ActionNum, newAction.Params[0], newAction.Params[1], newAction.Params[2], newAction.Params[3],
        newAction.Params[4], newAction.Params[5], newAction.Params[6]);
    FString pMessage = Translations::TranslateOrDefault("TriggerActionLengthExceededMessage",
        "After creating the new action, the length of the action INI will exceed 511, and the excess will not work properly. \nDo you want to continue?");

    int nResult = IDYES;
    if (value.GetLength() >= 512)
        nResult = ::MessageBox(hWnd, pMessage, Translations::TranslateOrDefault("TriggerLengthExceededTitle", "Length Exceeded"), MB_YESNO | MB_ICONWARNING);

    if (nResult == IDYES)
    {
        CurrentTrigger->ActionCount++;
        CurrentTrigger->Actions.push_back(newAction);
        CurrentTrigger->Save();

        while (SendMessage(hActionList, LB_DELETESTRING, 0, NULL) != CB_ERR);
        for (int i = 0; i < CurrentTrigger->ActionCount; i++)
        {
            SendMessage(hActionList, LB_INSERTSTRING, i, (LPARAM)(LPCSTR)ExtraWindow::GetActionDisplayName(CurrentTrigger->Actions[i].ActionNum, i, !CompactMode));
        }

        SelectedActionIndex = SendMessage(hActionList, LB_GETCOUNT, NULL, NULL) - 1;
        SetActionListBoxSel(SelectedActionIndex);
        OnSelchangeActionListbox();

        RefreshOtherInstances();
    }
}

void CNewTrigger::OnClickCloAction(HWND& hWnd)
{
    if (!CurrentTrigger) return;

    std::vector<int> selected;
    GetActionListBoxSels(selected);

    FString length;
    length.Format("%s=%s", CurrentTrigger->ID, map.GetString("Actions", CurrentTrigger->ID));
    FString pMessage = Translations::TranslateOrDefault("TriggerActionLengthExceededMessage",
        "After creating the new action, the length of the action INI will exceed 511, and the excess will not work properly. \nDo you want to continue?");

    for (auto index : selected)
    {
        if (index < 0 || index >= CurrentTrigger->ActionCount) continue;
        ActionParams newAction = CurrentTrigger->Actions[index];

        FString value;
        value.Format(",%s,%s,%s,%s,%s,%s,%s,%s",
            newAction.ActionNum, newAction.Params[0], newAction.Params[1], newAction.Params[2], newAction.Params[3],
            newAction.Params[4], newAction.Params[5], newAction.Params[6]);
        length += value;
    }

    int nResult = IDYES;
    if (length.GetLength() >= 512)
        nResult = ::MessageBox(hWnd, pMessage, Translations::TranslateOrDefault("TriggerLengthExceededTitle", "Length Exceeded"), MB_YESNO | MB_ICONWARNING);

    if (nResult == IDYES)
    {
        for (auto index : selected)
        {
            if (index < 0 || index >= CurrentTrigger->ActionCount) continue;
            ActionParams newAction = CurrentTrigger->Actions[index];
            CurrentTrigger->ActionCount++;
            CurrentTrigger->Actions.push_back(newAction);
        }

        CurrentTrigger->Save();

        while (SendMessage(hActionList, LB_DELETESTRING, 0, NULL) != CB_ERR);
        for (int i = 0; i < CurrentTrigger->ActionCount; i++)
        {
            SendMessage(hActionList, LB_INSERTSTRING, i, (LPARAM)(LPCSTR)ExtraWindow::GetActionDisplayName(CurrentTrigger->Actions[i].ActionNum, i, !CompactMode));
        }

        SelectedActionIndex = SendMessage(hActionList, LB_GETCOUNT, NULL, NULL) - 1;
        SetActionListBoxSel(SelectedActionIndex);
        OnSelchangeActionListbox();

        RefreshOtherInstances();
    }
}

void CNewTrigger::OnClickDelAction(HWND& hWnd)
{
    if (!CurrentTrigger) return;
    std::vector<int> selected;
    GetActionListBoxSels(selected);

    for (auto rit = selected.rbegin(); rit != selected.rend(); ++rit)
    {
        auto& index = *rit;
        if (index < 0 || index >= CurrentTrigger->ActionCount) continue;

        CurrentTrigger->ActionCount--;
        CurrentTrigger->Actions.erase(CurrentTrigger->Actions.begin() + index);
    }
    CurrentTrigger->Save();

    while (SendMessage(hActionList, LB_DELETESTRING, 0, NULL) != CB_ERR);
    for (int i = 0; i < CurrentTrigger->ActionCount; i++)
    {
        SendMessage(hActionList, LB_INSERTSTRING, i, (LPARAM)(LPCSTR)ExtraWindow::GetActionDisplayName(CurrentTrigger->Actions[i].ActionNum, i, !CompactMode));
    }

    SelectedActionIndex -= 1;
    if (SelectedActionIndex < 0)
        SelectedActionIndex = 0;
    if (SelectedActionIndex >= SendMessage(hActionList, LB_GETCOUNT, NULL, NULL))
        SelectedActionIndex = SendMessage(hActionList, LB_GETCOUNT, NULL, NULL) - 1;
    SetActionListBoxSel(SelectedActionIndex);
    OnSelchangeActionListbox();

    RefreshOtherInstances();
}

void CNewTrigger::UpdateEventAndParam(int changedEvent, bool changeCursel)
{
    if (!CurrentTrigger) return;
    if (CurrentTrigger->EventCount == 0) return;
    if (SelectedEventIndex > CurrentTrigger->EventCount) SelectedEventIndex = CurrentTrigger->EventCount - 1;
    if (SelectedEventIndex < 0) SelectedEventIndex = 0;

    FString buffer;
    for (int i = 0; i < EVENT_PARAM_COUNT; ++i)
        EventParamsUsage[i] = std::make_pair(false, 0);

    auto& thisEvent = CurrentTrigger->Events[SelectedEventIndex];
    if (changedEvent >= 0)
    {
        buffer.Format("%d", changedEvent);
        thisEvent.EventNum = buffer;
        SendMessage(hEventList, LB_DELETESTRING, SelectedEventIndex, NULL);
        SendMessage(hEventList, LB_INSERTSTRING, SelectedEventIndex, (LPARAM)(LPCSTR)ExtraWindow::GetEventDisplayName(thisEvent.EventNum, SelectedEventIndex, !CompactMode));
        SendMessage(hEventList, LB_SETCURSEL, SelectedEventIndex, NULL);
    }
        
    auto eventInfos = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("EventsRA2"), thisEvent.EventNum, "MISSING,0,0,0,0,MISSING,0,1,0"), 8);
    if (!CompactMode)
        SendMessage(hEventDescription, WM_SETTEXT, 0, (LPARAM)STDHelpers::ReplaceSpeicalString(eventInfos[5]).m_pchData);

    if (changeCursel)
    {
        int idx = SendMessage(hEventtype, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetEventDisplayName(thisEvent.EventNum).c_str());
        if (idx == CB_ERR)
            SendMessage(hEventtype, WM_SETTEXT, 0, (LPARAM)thisEvent.EventNum.c_str());
        else
            SendMessage(hEventtype, CB_SETCURSEL, idx, NULL);

    }

    FString paramType[2];
    paramType[0] =  eventInfos[1];
    paramType[1] =  eventInfos[2];
    std::vector<FString> pParamTypes[2]; 
    pParamTypes[0] = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("ParamTypes"), paramType[0], "MISSING,0"));
    pParamTypes[1] = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("ParamTypes"), paramType[1], "MISSING,0"));
    FString code = "0";
    if (pParamTypes[0].size() == 3) code = pParamTypes[0][2];
    int paramIdx[2];
    paramIdx[0] = atoi(paramType[0]);
    paramIdx[1] = atoi(paramType[1]);

    int usageIdx = 0;
    thisEvent.P3Enabled = false;
    if (paramIdx[0] > 0)
        thisEvent.Params[0] = code;
    else
    {
        buffer.Format("%d", -paramIdx[0]);
        thisEvent.Params[0] = buffer;
    }
    if (thisEvent.Params[0] == "2") // enable P3
    {
        thisEvent.P3Enabled = true;
        if (thisEvent.Params[2] == "")
            thisEvent.Params[2] = "0";
        if (paramIdx[0] > 0)
            EventParamsUsage[usageIdx++] = std::make_pair(true, 1);
        else
        {
            thisEvent.Params[1] = "0";
        }
        EventParamsUsage[usageIdx++] = std::make_pair(true, 2);
    }
    else
    {
        if (paramIdx[1] > 0)
            EventParamsUsage[usageIdx++] = std::make_pair(true, 1);
        else
        {
            buffer.Format("%d", -paramIdx[1]);
            thisEvent.Params[1] = buffer;
        }
    }    
    for (int i = 0; i < EVENT_PARAM_COUNT; i++)
    {
        while (SendMessage(hEventParameter[i], CB_DELETESTRING, 0, NULL) != CB_ERR);
        CNewTrigger::EventParameterAutoDrop[i] = true;
        if (thisEvent.P3Enabled)
        {
            if (EventParamsUsage[i].first)
            {
                EnableWindow(hEventParameter[i], TRUE);
                ExtraWindow::LoadParams(hEventParameter[i], pParamTypes[EventParamsUsage[i].second - 1][1], this);
                if (pParamTypes[EventParamsUsage[i].second - 1][1] == "1" && !ExtConfigs::SearchCombobox_Waypoint) // waypoints
                {
                    CNewTrigger::EventParameterAutoDrop[i] = false;
                }
                SendMessage(hEventParameterDesc[i], WM_SETTEXT, 0, (LPARAM)pParamTypes[EventParamsUsage[i].second - 1][0].c_str());
            }
            else
            {
                EnableWindow(hEventParameter[i], FALSE);
                SendMessage(hEventParameter[i], WM_SETTEXT, 0, (LPARAM)"");
                FString trans;
                trans.Format("TriggerParameter#%dvalue", i + 1);
                SendMessage(hEventParameterDesc[i], WM_SETTEXT, 0, 
                    (LPARAM)Translations::TranslateOrDefault(trans, ""));
            }

        }
        else
        {
            if (EventParamsUsage[i].first)
            {
                EnableWindow(hEventParameter[i], TRUE);
                ExtraWindow::LoadParams(hEventParameter[i], pParamTypes[EventParamsUsage[i].second][1], this);
                if (pParamTypes[EventParamsUsage[i].second][1] == "1" && !ExtConfigs::SearchCombobox_Waypoint) // waypoints
                {
                    CNewTrigger::EventParameterAutoDrop[i] = false;
                }
                SendMessage(hEventParameterDesc[i], WM_SETTEXT, 0, (LPARAM)pParamTypes[EventParamsUsage[i].second][0].c_str());
            }
            else
            {
                EnableWindow(hEventParameter[i], FALSE);
                SendMessage(hEventParameter[i], WM_SETTEXT, 0, (LPARAM)"");
                FString trans;
                trans.Format("TriggerParameter#%dvalue", i + 1);
                SendMessage(hEventParameterDesc[i], WM_SETTEXT, 0,
                    (LPARAM)Translations::TranslateOrDefault(trans, ""));
            }
        }
        ExtraWindow::AdjustDropdownWidth(hEventParameter[i]);
    }
    for (int i = 0; i < EVENT_PARAM_COUNT; i++)
    {
        UpdateParamAffectedParam_Event(i);
    }

    if (changedEvent >= 0)
        CurrentTrigger->Save();
}

void CNewTrigger::UpdateActionAndParam(int changedAction, bool changeCursel)
{
    if (!CurrentTrigger) return;
    if (CurrentTrigger->ActionCount == 0) return;
    if (SelectedActionIndex > CurrentTrigger->ActionCount) SelectedActionIndex = CurrentTrigger->ActionCount - 1;
    if (SelectedActionIndex < 0) SelectedActionIndex = 0;
    ActionParamUsesFloat = false;
    ActionParamsCount = 0;

    FString buffer;
    for (int i = 0; i < ACTION_PARAM_COUNT; ++i)
        ActionParamsUsage[i] = std::make_pair(false, 0);

    auto& thisAction = CurrentTrigger->Actions[SelectedActionIndex];
    if (changedAction >= 0)
    {
        buffer.Format("%d", changedAction);
        thisAction.ActionNum = buffer;
        SendMessage(hActionList, LB_DELETESTRING, SelectedActionIndex, NULL);
        SendMessage(hActionList, LB_INSERTSTRING, SelectedActionIndex, (LPARAM)(LPCSTR)ExtraWindow::GetActionDisplayName(thisAction.ActionNum, SelectedActionIndex, !CompactMode));
        SetActionListBoxSel(SelectedActionIndex);
    }
    auto actionInfos = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("ActionsRA2"), thisAction.ActionNum, "MISSING,0,0,0,0,0,0,0,0,0,MISSING,0,1,0"), 13);
    if (!CompactMode)
        SendMessage(hActionDescription, WM_SETTEXT, 0, (LPARAM)FString::ReplaceSpeicalString(actionInfos[10]).c_str());

    if (changeCursel)
    {
        int idx = SendMessage(hActiontype, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetActionDisplayName(thisAction.ActionNum).c_str());
        if (idx == CB_ERR)
            SendMessage(hActiontype, WM_SETTEXT, 0, (LPARAM)thisAction.ActionNum.c_str());
        else
            SendMessage(hActiontype, CB_SETCURSEL, idx, NULL);
    }

    FString paramType[7];
    for (int i = 0; i < 7; i++)
        paramType[i] = actionInfos[i + 1];

    std::vector<FString> pParamTypes[7];
    for (int i = 0; i < 7; i++)
        pParamTypes[i] = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("ParamTypes"), paramType[i], "MISSING,0"));

    int paramIdx[7];
    for (int i = 0; i < 7; i++)
        paramIdx[i] = atoi(paramType[i]);

    thisAction.Param7isWP = true;
    for (auto& pair : fadata.GetSection("DontSaveAsWP")->GetEntities())
    {
        if (atoi(pair.second) == -paramIdx[0])
            thisAction.Param7isWP = false;
    }

    int usageIdx = 0;
    for (int i = 0; i < 7; i++)
    {
        if (i != 6)
        {
            if (paramIdx[i] > 0)
            {
                if (usageIdx >= ACTION_PARAM_COUNT) break;
                ActionParamsUsage[usageIdx++] = std::make_pair(true, i);
            }
            else
            {
                buffer.Format("%d", -paramIdx[i]);
                thisAction.Params[i] = buffer;
            }
        }
        else // last param is waypoint
        {
            if (paramIdx[i] > 0)
            {
                if (usageIdx >= ACTION_PARAM_COUNT) break;
                ActionParamsUsage[usageIdx++] = std::make_pair(true, i);
                if (thisAction.Param7isWP && STDHelpers::IsNumber(thisAction.Params[i]))
                    thisAction.Params[i] = "A";
                else if (!thisAction.Param7isWP && !STDHelpers::IsNumber(thisAction.Params[i]))
                    thisAction.Params[i] = "0";
            }
            else
            {
                if (thisAction.Param7isWP)
                    thisAction.Params[i] = "A";
                else
                    thisAction.Params[i] = "0";
            }
        }
    }

    CurrentCSFActionParam = -1;
    CurrentTriggerActionParam = -1;
    CurrentTeamActionParam = -1;
    for (int i = 0; i < ACTION_PARAM_COUNT; i++)
    {
        while (SendMessage(hActionParameter[i], CB_DELETESTRING, 0, NULL) != CB_ERR);
        CNewTrigger::ActionParameterAutoDrop[i] = true;
        if (ActionParamsUsage[i].first)
        {
            ActionParamsCount++;
            EnableWindow(hActionParameter[i], TRUE);
            ShowWindow(hActionParameterDesc[i], SW_SHOW);
            if (ActionParamsUsage[i].second != 6)
            {
                ExtraWindow::LoadParams(hActionParameter[i], pParamTypes[ActionParamsUsage[i].second][1], this);

                SendMessage(hActionParameterDesc[i], WM_SETTEXT, 0, (LPARAM)pParamTypes[ActionParamsUsage[i].second][0].c_str());
                if (pParamTypes[ActionParamsUsage[i].second][1] == "10") // stringtables
                {
                    //thisAction.Params[ActionParamsUsage[i].second].MakeLower();
                    CurrentCSFActionParam = i;
                }
                else if (pParamTypes[ActionParamsUsage[i].second][1] == "1" && !ExtConfigs::SearchCombobox_Waypoint) // waypoints
                {
                    CNewTrigger::ActionParameterAutoDrop[i] = false;
                }
                else if (pParamTypes[ActionParamsUsage[i].second][1] == "9") // triggers
                {
                    CurrentTriggerActionParam = i;
                }
                else if (pParamTypes[ActionParamsUsage[i].second][1] == "15" || FString::SplitString(
                    fadata.GetString(
                        "NewParamTypes",
                        pParamTypes[ActionParamsUsage[i].second][1]), size_t(0))[0]
                    == "TeamTypes")
                {
                    CurrentTeamActionParam = i;
                }
            }
            else
            {
                if (thisAction.Param7isWP)
                {
                    SendMessage(hActionParameterDesc[i], WM_SETTEXT, 0, (LPARAM)Translations::TranslateOrDefault("TriggerP7Waypoint", "Waypoint"));
                    ExtraWindow::LoadParam_Waypoints(hActionParameter[i]);
                    if (!ExtConfigs::SearchCombobox_Waypoint)
                        CNewTrigger::ActionParameterAutoDrop[i] = false;
                }
                else
                    SendMessage(hActionParameterDesc[i], WM_SETTEXT, 0, (LPARAM)Translations::TranslateOrDefault("TriggerP7Number", "Number"));
            }
        }
        else
        {
            EnableWindow(hActionParameter[i], FALSE);
            SendMessage(hActionParameter[i], WM_SETTEXT, 0, (LPARAM)"");
            FString trans;
            trans.Format("TriggerParameter#%dvalue", i + 1);
            ShowWindow(hActionParameterDesc[i], SW_HIDE);
            SendMessage(hActionParameterDesc[i], WM_SETTEXT, 0,
                (LPARAM)Translations::TranslateOrDefault(trans, ""));
        }
        ExtraWindow::AdjustDropdownWidth(hActionParameter[i]);
    }

    for (int i = 0; i < ACTION_PARAM_COUNT; i++)
    {
        UpdateParamAffectedParam_Action(i);
    }

    if (WindowShown)
    {
        if (ActionParamsCount < 4)
            ActionParamsCount = 4;
        AdjustActionHeight();
        LastActionParamsCount = ActionParamsCount;
    }

    if (changedAction >= 0)
        CurrentTrigger->Save();
}

void CNewTrigger::AdjustActionHeight()
{
    RECT rect;
    int heightDistance = 0;
    GetWindowRect(hActionParameterDesc[0], &rect);
    heightDistance = rect.top;
    GetWindowRect(hActionParameterDesc[1], &rect);
    heightDistance = rect.top - heightDistance;

    auto adjustHeight = [this, &heightDistance](HWND& hWnd)
        {
            RECT rect;
            GetWindowRect(hWnd, &rect);
            POINT topLeft = { rect.left, rect.top };
            ScreenToClient(m_hwnd, &topLeft);
            int newWidth = rect.right - rect.left;
            int newHeight = rect.bottom - rect.top + (ActionParamsCount - LastActionParamsCount) * heightDistance;
            MoveWindow(hWnd, topLeft.x, topLeft.y, newWidth, newHeight, TRUE);
        };

    adjustHeight(hActionList);
    adjustHeight(hActionDescription);
    adjustHeight(hActionframe);

    GetWindowRect(m_hwnd, &rect);
    MoveWindow(m_hwnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top + (ActionParamsCount - LastActionParamsCount) * heightDistance, TRUE);
}

void CNewTrigger::OnDropdownCComboBox(int index)
{
    if (DropNeedUpdate)
    {
        SortTriggers(CurrentTrigger->ID, true);
        DropNeedUpdate = false;
    }

    if (index == CurrentCSFActionParam && ExtConfigs::TutorialTexts_Viewer)
    {
        PostMessage(hActionParameter[index], CB_SHOWDROPDOWN, FALSE, 0);
        CCsfEditor::TriggerCaller = GetCurrentInstanceIndex();
        if (CCsfEditor::GetHandle() == NULL)
            CCsfEditor::Create(m_parent);
        else
        {
            ::SendMessage(CCsfEditor::GetHandle(), 114514, 0, 0);
        }
        char buffer[512]{ 0 };
        GetWindowText(hActionParameter[index], buffer, 511);

        FString text(buffer);
        text.Replace(",", "");
        ExtraWindow::TrimStringIndex(text);
        text.MakeLower();
        CCsfEditor::CurrentSelectedCSF = text.c_str();

        ::SendMessage(CCsfEditor::GetHandle(), 114515, 0, 0);
    }
    else if (index == CurrentTeamActionParam && TeamListChanged)
    {
        int curSel = SendMessage(hActionParameter[index], CB_GETCURSEL, NULL, NULL);
        char buffer[512]{ 0 };
        GetWindowText(hActionParameter[index], buffer, 511);
        FString text(buffer);

        UpdateActionAndParam();
        TeamListChanged = false;

        int idx = SendMessage(hActionParameter[index], CB_FINDSTRINGEXACT, 0, text);
        if (idx != CB_ERR)
        {
            SendMessage(hActionParameter[index], CB_SETCURSEL, idx, NULL);
        }
        else
        {
            FString::TrimIndex(text);
            SendMessage(hActionParameter[index], WM_SETTEXT, NULL, text);
        }
    }
}

void CNewTrigger::OnCloseupCComboBox(HWND& hWnd, std::map<int, FString>& labels, bool isComboboxSelectOnly)
{
    if (!ExtraWindow::OnCloseupCComboBox(hWnd, labels, isComboboxSelectOnly))
    {
        if (hWnd == hActiontype)
        {
            UpdateActionAndParam();
            OnSelchangeActionListbox();
        }
        else if (hWnd == hEventtype)
        {
            UpdateEventAndParam();
            OnSelchangeEventListbox();
        }
        else if (hWnd == hSelectedTrigger)
        {
            OnSelchangeTrigger();
        }
        for (int i = 0; i < ACTION_PARAM_COUNT; i++)
            if (hWnd == hActionParameter[i])
                OnSelchangeActionListbox();
        for (int i = 0; i < EVENT_PARAM_COUNT; i++)
            if (hWnd == hEventParameter[i])
                OnSelchangeEventListbox();
    }
}

void CNewTrigger::SetActionListBoxSel(int index)
{
    SendMessage(hActionList, LB_SETSEL, FALSE, -1);
    if (index >= 0)
        SendMessage(hActionList, LB_SETSEL, TRUE, index);
    SendMessage(hActionList, LB_SETCURSEL, index, NULL);
    SendMessage(hActionList, LB_SETCARETINDEX, index, TRUE);
}

void CNewTrigger::SetActionListBoxSels(std::vector<int>& indices)
{
    SendMessage(hActionList, LB_SETSEL, FALSE, -1);
    for (int idx : indices)
    {
        if (idx >= 0 && idx < SendMessage(hActionList, LB_GETCOUNT, 0, 0))
        {
            SendMessage(hActionList, LB_SETSEL, TRUE, idx);
        }
    }
    if (!indices.empty())
    {
        SendMessage(hActionList, LB_SETCARETINDEX, indices[0], TRUE);
    }
}

void CNewTrigger::GetActionListBoxSels(std::vector<int>& indices)
{
    int numSelected = SendMessage(hActionList, LB_GETSELCOUNT, 0, 0);
    if (numSelected > 0)
    {
        indices.resize(numSelected);
        SendMessage(hActionList, LB_GETSELITEMS, numSelected, (LPARAM)indices.data());
        std::sort(indices.begin(), indices.end());
    }
    else
    {
        indices.clear();
    }
}

void CNewTrigger::OnClickSearchReference(HWND& hWnd)
{
    if (SelectedTriggerIndex < 0 || !CurrentTrigger)
        return;

    CSearhReference::SetSearchType(1);
    CSearhReference::SetSearchID(CurrentTrigger->ID);
    CSearhReference::SetTriggerCaller(GetCurrentInstanceIndex());
    if (CSearhReference::GetHandle() == NULL)
    {
        CSearhReference::Create(m_parent);
    }
    else
    {
        ::SendMessage(CSearhReference::GetHandle(), 114514, 0, 0);
    }
}

void CNewTrigger::OnClickActionMove(HWND& hWnd, bool isUp)
{
    if (SelectedTriggerIndex < 0 || !CurrentTrigger || CurrentTrigger->ActionCount < 2)
        return;

    std::vector<int> selected;
    GetActionListBoxSels(selected);

    for (auto& i : selected)
    {
        if (isUp && i == 0) return;
        else if(!isUp && i == CurrentTrigger->ActionCount - 1) return;
    }

    if (isUp)
    {
        for (auto it = selected.begin(); it != selected.end(); ++it)
        {
            if (*it > 0 && *it < CurrentTrigger->Actions.size()) {
                std::swap(CurrentTrigger->Actions[*it], CurrentTrigger->Actions[*it - 1]);
            }
        }
    }
    else
    {
        for (auto it = selected.rbegin(); it != selected.rend(); ++it)
        {
            if (*it >= 0 && *it < CurrentTrigger->Actions.size() - 1) {
                std::swap(CurrentTrigger->Actions[*it + 1], CurrentTrigger->Actions[*it]);
            }
        }
    }

    CurrentTrigger->Save();
    while (SendMessage(hActionList, LB_DELETESTRING, 0, NULL) != CB_ERR);
    for (int i = 0; i < CurrentTrigger->ActionCount; i++)
    {
        SendMessage(hActionList, LB_INSERTSTRING, i, (LPARAM)(LPCSTR)ExtraWindow::GetActionDisplayName(CurrentTrigger->Actions[i].ActionNum, i, !CompactMode));
    }

    for (auto& i : selected)
    {
        if (isUp) i--;
        else i++;
    }
    if (selected.size() > 1)
        SetActionListBoxSels(selected);
    else
        SetActionListBoxSel(isUp ? SelectedActionIndex - 1 : SelectedActionIndex + 1);

    OnSelchangeActionListbox();
    RefreshOtherInstances();
}

void CNewTrigger::OnClickActionSplit(HWND& hWnd)
{
    if (SelectedTriggerIndex < 0 || !CurrentTrigger || !CurrentTrigger->ActionCount)
        return;

    std::vector<int> selected;
    GetActionListBoxSels(selected);
    if (selected.empty())
        return;

    int firstIndex = CurrentTrigger->ActionCount;
    std::vector<ActionParams> params;
    for (auto it = selected.rbegin(); it != selected.rend(); ++it)
    {
        if (*it >= 0 && *it < CurrentTrigger->Actions.size()) {
            params.push_back(CurrentTrigger->Actions[*it]);
            CurrentTrigger->Actions.erase(CurrentTrigger->Actions.begin() + *it);
            CurrentTrigger->ActionCount--;
            firstIndex = *it;
        }
    }
    FString id = CMapDataExt::GetAvailableIndex();
    // allow new trigger
    CurrentTrigger->Actions.insert(CurrentTrigger->Actions.begin() + firstIndex,
        { "53", {"2",id,"0","0","0","0","A"}, false });
    CurrentTrigger->ActionCount++;

    CurrentTrigger->Save();

    std::reverse(params.begin(), params.end());
    TempValueHolder<bool> tmp(AutoChangeName, true);
    CNewTeamTypes::TagListChanged = true;
    FString value;
    FString newName = CurrentTrigger->Name + " #Split";

    value.Format("%s,<none>,%s,0,1,1,1,0", CurrentTrigger->House, newName);

    map.WriteString("Triggers", id, value);
    map.WriteString("Events", id, "1,13,0,0"); // elapsed 0s
    FString tagId = CMapDataExt::GetAvailableIndex();
    value.Format("0,%s 1,%s", newName, id);
    map.WriteString("Tags", tagId, value);

    CMapDataExt::AddTrigger(id);

    if (auto newTrigger = CMapDataExt::GetTrigger(id))
    {
        newTrigger->Actions = params;
        newTrigger->ActionCount = params.size();
        newTrigger->EasyEnabled = CurrentTrigger->EasyEnabled;
        newTrigger->MediumEnabled = CurrentTrigger->MediumEnabled;
        newTrigger->HardEnabled = CurrentTrigger->HardEnabled;
        newTrigger->RepeatType = CurrentTrigger->RepeatType;
        newTrigger->Disabled = true;
        if (CurrentTrigger->RepeatType == "2")
        {
            ActionParams disableSelf = { "54", {"2",id,"0","0","0","0","A"}, false };
            newTrigger->Actions.push_back(disableSelf);
            newTrigger->ActionCount++;
        }
        newTrigger->Save();
    }

    SortTriggers(id);
    OnSelchangeTrigger();
}

void CNewTrigger::SortTriggers(FString id, bool onlySelf)
{
    if (!AvoidInfiLoop || AvoidInfiLoop && !SortTriggersExecuted)
    {
        SortTriggersExecuted = true;
        TempValueHolder<bool> tmpLoop(AvoidInfiLoop, true);

        std::vector<FString> labels;
        for (auto& triggerPair : CMapDataExt::Triggers) {
            auto& trigger = triggerPair.second;
            labels.push_back(ExtraWindow::GetTriggerDisplayName(trigger->ID));
        }

        bool tmp = ExtConfigs::SortByLabelName;
        ExtConfigs::SortByLabelName = ExtConfigs::SortByLabelName_Trigger;

        std::sort(labels.begin(), labels.end(), ExtraWindow::SortLabels);

        ExtConfigs::SortByLabelName = tmp;

        auto sort = [&labels](CNewTrigger* pThis, FString id) {
            pThis->DropNeedUpdate = false;
            while (SendMessage(pThis->hSelectedTrigger, CB_DELETESTRING, 0, NULL) != CB_ERR);

            for (size_t i = 0; i < labels.size(); ++i) {
                SendMessage(pThis->hSelectedTrigger, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)labels[i].c_str());
            }
            if (pThis->CompactMode) ExtraWindow::AdjustDropdownWidth(pThis->hSelectedTrigger);
            int width = SendMessage(pThis->hSelectedTrigger, CB_GETDROPPEDWIDTH, NULL, NULL);
            ExtraWindow::SyncComboBoxContent(pThis->hSelectedTrigger, pThis->hAttachedtrigger, true);
            if (pThis->CompactMode) SendMessage(pThis->hAttachedtrigger, CB_SETDROPPEDWIDTH, width, NULL);
            if (id != "") {
                pThis->SelectedTriggerIndex = SendMessage(pThis->hSelectedTrigger, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetTriggerDisplayName(id).c_str());
                SendMessage(pThis->hSelectedTrigger, CB_SETCURSEL, pThis->SelectedTriggerIndex, NULL);
            }

            if (pThis->CurrentTriggerActionParam > -1)
            {
                ExtraWindow::SyncComboBoxContent(pThis->hSelectedTrigger,
                    pThis->hActionParameter[pThis->CurrentTriggerActionParam]);
                if (pThis->CompactMode) SendMessage(pThis->hActionParameter[pThis->CurrentTriggerActionParam],
                    CB_SETDROPPEDWIDTH, width, NULL);
            }
        };

        sort(this, id);

        if (!onlySelf)
        {
            auto others = GetOtherInstances();
            for (auto& [i, o] : others)
            {
                if (o->GetHandle())
                    sort(o, o->CurrentTriggerID);
            }
        }
    }
}

std::map<int, CNewTrigger*> CNewTrigger::GetOtherInstances()
{
    std::map<int, CNewTrigger*> ret;
    for (int i = 0; i < TRIGGER_EDITOR_MAX_COUNT; ++i)
    {
        if (this != &Instance[i])
            ret[i] = &Instance[i];
    }
    return ret;
}

CNewTrigger& CNewTrigger::GetFirstValidInstance()
{
    for (int i = 0; i < TRIGGER_EDITOR_MAX_COUNT; ++i)
    {
        if (Instance[i].GetHandle())
            return Instance[i];
    }
    return Instance[0];
}

int CNewTrigger::GetCurrentInstanceIndex()
{
    for (int i = 0; i < TRIGGER_EDITOR_MAX_COUNT; ++i)
    {
        if (this == &Instance[i])
            return i;
    }
    return -1;
}

bool CNewTrigger::IsMainInstance()
{
    return this == &Instance[0];
}

void CNewTrigger::RefreshOtherInstances()
{
    if (!AvoidInfiLoop && CurrentTrigger)
    {
        SortTriggersExecuted = false;
        TempValueHolder<bool> tmp(AvoidInfiLoop, true);
        auto others = GetOtherInstances();
        for (auto& [i, o] : others)
        {
            if (o->CurrentTrigger == CurrentTrigger && o->GetHandle())
            {
                TempValueHolder<bool> tmp(o->DropNeedUpdate, true);
                int indexE = o->SelectedEventIndex;
                int indexA = o->SelectedActionIndex;
                o->OnSelchangeTrigger(false,
                    o->SelectedEventIndex,
                    o->SelectedActionIndex,
                    false);
            }
        }

        if (CBatchTrigger::GetHandle())
        {
            for (int i = 0; i < CBatchTrigger::ListedTriggerIDs.size(); ++i)
            {
                auto& id = CBatchTrigger::ListedTriggerIDs[i];
                if (id == CurrentTriggerID)
                {
                    CBatchTrigger::RefreshTrigger(i);
                }
            }
        }
    }
}

bool CNewTrigger::OnEnterKeyDown(HWND& hWnd)
{
    if (hWnd == hSelectedTrigger)
        OnSelchangeTrigger(true);
    else if (hWnd == hAttachedtrigger)
        OnSelchangeAttachedTrigger(true);
    else if (hWnd == hHouse)
        OnSelchangeHouse(true);
    else if (hWnd == hEventtype)
        OnSelchangeEventType(true);
    else if (hWnd == hActiontype)
        OnSelchangeActionType(true);
    else if (hWnd == hEventParameter[0])
        OnSelchangeEventParam(0, true);
    else if (hWnd == hEventParameter[1])
        OnSelchangeEventParam(1, true);
    else if (hWnd == hActionParameter[0])
        OnSelchangeActionParam(0, true);
    else if (hWnd == hActionParameter[1])
        OnSelchangeActionParam(1, true);
    else if (hWnd == hActionParameter[2])
        OnSelchangeActionParam(2, true);
    else if (hWnd == hActionParameter[3])
        OnSelchangeActionParam(3, true);
    else if (hWnd == hActionParameter[4])
        OnSelchangeActionParam(4, true);
    else if (hWnd == hActionParameter[5])
        OnSelchangeActionParam(5, true);
    else
        return false;
    return true;

}