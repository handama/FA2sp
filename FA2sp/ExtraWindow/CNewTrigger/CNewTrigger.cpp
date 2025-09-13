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
#include "../CCsfEditor/CCsfEditor.h"

HWND CNewTrigger::m_hwnd;
CFinalSunDlg* CNewTrigger::m_parent;
CINI& CNewTrigger::map = CINI::CurrentDocument;
CINI& CNewTrigger::fadata = CINI::FAData;
MultimapHelper& CNewTrigger::rules = Variables::RulesMap;

HWND CNewTrigger::hSelectedTrigger;
HWND CNewTrigger::hNewTrigger;
HWND CNewTrigger::hCloneTrigger;
HWND CNewTrigger::hDeleteTrigger;
HWND CNewTrigger::hPlaceOnMap;
HWND CNewTrigger::hType;
HWND CNewTrigger::hName;
HWND CNewTrigger::hHouse;
HWND CNewTrigger::hAttachedtrigger;
HWND CNewTrigger::hDisabled;
HWND CNewTrigger::hEasy;
HWND CNewTrigger::hMedium;
HWND CNewTrigger::hHard;
HWND CNewTrigger::hEventtype;
HWND CNewTrigger::hNewEvent;
HWND CNewTrigger::hCloneEvent;
HWND CNewTrigger::hDeleteEvent;
HWND CNewTrigger::hEventDescription;
HWND CNewTrigger::hEventList;
HWND CNewTrigger::hEventParameter[EVENT_PARAM_COUNT];
HWND CNewTrigger::hEventParameterDesc[EVENT_PARAM_COUNT];
HWND CNewTrigger::hActionoptions;
HWND CNewTrigger::hActiontype;
HWND CNewTrigger::hNewAction;
HWND CNewTrigger::hDeleteAction;
HWND CNewTrigger::hCloneAction;
HWND CNewTrigger::hActionDescription;
HWND CNewTrigger::hActionList;
HWND CNewTrigger::hActionframe;
HWND CNewTrigger::hSearchReference;
HWND CNewTrigger::hActionParameter[ACTION_PARAM_COUNT];
HWND CNewTrigger::hActionParameterDesc[ACTION_PARAM_COUNT];
int CNewTrigger::CurrentCSFActionParam;
int CNewTrigger::CurrentTriggerActionParam;
int CNewTrigger::SelectedTriggerIndex = -1;
int CNewTrigger::SelectedEventIndex = -1;
int CNewTrigger::SelectedActionIndex = -1;
int CNewTrigger::ActionParamsCount;
int CNewTrigger::LastActionParamsCount;
bool CNewTrigger::WindowShown;
FString CNewTrigger::CurrentTriggerID;
std::shared_ptr<Trigger> CNewTrigger::CurrentTrigger;
std::map<int, FString> CNewTrigger::TriggerLabels;
std::map<int, FString> CNewTrigger::AttachedTriggerLabels;
std::map<int, FString> CNewTrigger::HouseLabels;
std::map<int, FString> CNewTrigger::ActionTypeLabels;
std::map<int, FString> CNewTrigger::EventTypeLabels;
std::map<int, FString> CNewTrigger::ActionParamLabels[ACTION_PARAM_COUNT];
std::map<int, FString> CNewTrigger::EventParamLabels[EVENT_PARAM_COUNT];
std::pair<bool, int> CNewTrigger::EventParamsUsage[EVENT_PARAM_COUNT];
std::pair<bool, int> CNewTrigger::ActionParamsUsage[ACTION_PARAM_COUNT];
std::vector<ParamAffectedParams> CNewTrigger::ActionParamAffectedParams;
std::vector<ParamAffectedParams> CNewTrigger::EventParamAffectedParams;
bool CNewTrigger::EventParameterAutoDrop[EVENT_PARAM_COUNT];
bool CNewTrigger::ActionParameterAutoDrop[ACTION_PARAM_COUNT];
bool CNewTrigger::Autodrop;
bool CNewTrigger::DropNeedUpdate;
bool CNewTrigger::ActionParamUsesFloat;
WNDPROC CNewTrigger::OriginalListBoxProcEvent;
WNDPROC CNewTrigger::OriginalListBoxProcAction;
RECT CNewTrigger::rectComboLBox = { 0 };
HWND CNewTrigger::hComboLBox = NULL;


void CNewTrigger::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(307),
        pWnd->GetSafeHwnd(),
        CNewTrigger::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
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
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };

    Translate(50901, "TriggerTriggeroptions");
    Translate(50902, "TriggerSelectedTrigger");
    Translate(50904, "TriggerNew");
    Translate(50905, "TriggerClone");
    Translate(50906, "TriggerDelete");
    Translate(50907, "TriggerPlaceOnMap");
    Translate(50908, "TriggerType");
    Translate(50910, "TriggerName");
    Translate(50912, "TriggerHouse");
    Translate(50914, "TriggerAttachedtrigger");
    Translate(50915, "TriggerCannotbeitselforformsaloop");
    Translate(50917, "TriggerDisabled");
    Translate(50918, "TriggerEasy");
    Translate(50919, "TriggerMedium");
    Translate(50920, "TriggerHard");
    Translate(50921, "TriggerEventoptions");
    Translate(50922, "TriggerEventtype");
    Translate(50924, "TriggerAdd");
    Translate(50925, "TriggerClone");
    Translate(50926, "TriggerDelete");
    Translate(50928, "TriggerEventList");
    Translate(50930, "TriggerParameter#1value");
    Translate(50932, "TriggerParameter#2value");
    Translate(50934, "TriggerActionoptions");
    Translate(50935, "TriggerActiontype");
    Translate(50937, "TriggerAdd");
    Translate(50938, "TriggerDelete");
    Translate(50939, "TriggerClone");
    Translate(50941, "TriggerActionList");
    Translate(50943, "TriggerParameter#1value");
    Translate(50945, "TriggerParameter#2value");
    Translate(50947, "TriggerParameter#3value");
    Translate(50949, "TriggerParameter#4value");
    Translate(50951, "TriggerParameter#5value");
    Translate(50953, "TriggerParameter#6value");
    Translate(1999, "SearchReferenceTitle");

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

    LastActionParamsCount = 4;
    ActionParamsCount = 4;
    WindowShown = false;

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
        OriginalListBoxProcEvent = (WNDPROC)SetWindowLongPtr(hEventList, GWLP_WNDPROC, (LONG_PTR)ListBoxSubclassProcEvent);
    if (hActionList)
        OriginalListBoxProcAction = (WNDPROC)SetWindowLongPtr(hActionList, GWLP_WNDPROC, (LONG_PTR)ListBoxSubclassProcAction);

    CurrentTrigger = nullptr;

    Update(hWnd);
}

void CNewTrigger::Update(HWND& hWnd)
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    DropNeedUpdate = false;

    CMapDataExt::UpdateTriggers();

    SortTriggers();
        
    int count = SendMessage(hSelectedTrigger, CB_GETCOUNT, NULL, NULL);
    if (SelectedTriggerIndex < 0)
        SelectedTriggerIndex = 0;
    if (SelectedTriggerIndex > count - 1)
        SelectedTriggerIndex = count - 1;
    SendMessage(hSelectedTrigger, CB_SETCURSEL, SelectedTriggerIndex, NULL);

    int idx = 0;
    while (SendMessage(hEventtype, CB_DELETESTRING, 0, NULL) != CB_ERR);
    if (auto pSection = fadata.GetSection("EventsRA2"))
    {
        for (auto& pair : pSection->GetEntities())
        {
            auto atoms = FString::SplitString(pair.second);
            if (atoms.size() >= 9)
            {
                if (atoms[7] == "1")
                {
                    SendMessage(hEventtype, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)(FString(pair.first) + " " + atoms[0]).c_str());
                }
            }
        }
    }
    idx = 0;
    while (SendMessage(hActiontype, CB_DELETESTRING, 0, NULL) != CB_ERR);
    if (auto pSection = fadata.GetSection("ActionsRA2"))
    {
        for (auto& pair : pSection->GetEntities())
        {
            auto atoms = FString::SplitString(pair.second);
            if (atoms.size() >= 14)
            {
                if (atoms[12] == "1")
                {
                    SendMessage(hActiontype, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)(FString(pair.first) + " " + atoms[0]).c_str());
                }
            }
        }
    }

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

    idx = 0;
    while (SendMessage(hType, CB_DELETESTRING, 0, NULL) != CB_ERR);
    SendMessage(hType, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)(FString("0 - ") + Translations::TranslateOrDefault("TriggerRepeatType.OneTimeOr", "One Time OR")));
    SendMessage(hType, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)(FString("1 - ") + Translations::TranslateOrDefault("TriggerRepeatType.OneTimeAnd", "One Time AND")));
    SendMessage(hType, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)(FString("2 - ") + Translations::TranslateOrDefault("TriggerRepeatType.RepeatingOr", "Repeating OR")));

    Autodrop = false;

    OnSelchangeTrigger();
}

void CNewTrigger::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CNewTrigger::m_hwnd = NULL;
    CNewTrigger::m_parent = NULL;

}

LRESULT CALLBACK CNewTrigger::ListBoxSubclassProcAction(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
            return CallWindowProc(OriginalListBoxProcAction, hWnd, message, wParam, lParam);
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
                OnSelchangeActionListbox();

            }
            else {
                SendMessage(hWnd, LB_SETCURSEL, 0, 0);
            }
            return TRUE;
        }
    }
    return CallWindowProc(OriginalListBoxProcAction, hWnd, message, wParam, lParam);
}

LRESULT CALLBACK CNewTrigger::ListBoxSubclassProcEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CNewTrigger::Initialize(hWnd);
        return TRUE;
    }
    case WM_SHOWWINDOW:
    {
        if (!WindowShown)
        {
            WindowShown = true;
            OnSelchangeTrigger();
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
        case Controls::Name:
            if (CODE == EN_CHANGE && CurrentTrigger)
            {
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
            }
            break;
        case Controls::Disabled:
            if (CODE == BN_CLICKED && CurrentTrigger)
            {
                CurrentTrigger->Disabled = SendMessage(hDisabled, BM_GETCHECK, 0, 0);
                CurrentTrigger->Save();
            }
            break;
        case Controls::Easy:
            if (CODE == BN_CLICKED && CurrentTrigger)
            {
                CurrentTrigger->EasyEnabled = SendMessage(hEasy, BM_GETCHECK, 0, 0);
                CurrentTrigger->Save();
            }
            break;
        case Controls::Medium:
            if (CODE == BN_CLICKED && CurrentTrigger)
            {
                CurrentTrigger->MediumEnabled = SendMessage(hMedium, BM_GETCHECK, 0, 0);
                CurrentTrigger->Save();
            }
            break;
        case Controls::Hard:
            if (CODE == BN_CLICKED && CurrentTrigger)
            {
                CurrentTrigger->HardEnabled = SendMessage(hHard, BM_GETCHECK, 0, 0);
                CurrentTrigger->Save();
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
                int idx = SendMessage(hAttachedtrigger, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetTriggerDisplayName(CurrentTrigger->AttachedTrigger).c_str());
                SendMessage(hAttachedtrigger, CB_SETCURSEL, idx, NULL);
                if (CurrentTriggerActionParam > -1)
                {
                    OnSelchangeActionListbox();
                }
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
                if (CurrentTriggerActionParam > -1)
                {
                    OnSelchangeActionListbox();
                }
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
    case 114514: // used for update
    {
        Update(hWnd);
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

void CNewTrigger::OnSelchangeActionListbox(bool changeCursel)
{
    if (SelectedTriggerIndex < 0 || SendMessage(hActionList, LB_GETCURSEL, NULL, NULL) < 0 || SendMessage(hActionList, LB_GETCOUNT, NULL, NULL) <= 0)
    {
        SendMessage(hActiontype, CB_SETCURSEL, -1, NULL);
        for (int i = 0; i < ACTION_PARAM_COUNT; ++i) {

            SendMessage(hActionParameter[i], CB_SETCURSEL, -1, NULL);
            SendMessage(hActionParameterDesc[i], WM_SETTEXT, 0, (LPARAM)"");
            EnableWindow(hActionParameter[i], FALSE);
        }
        SendMessage(hActionDescription, WM_SETTEXT, 0, (LPARAM)"");

        if (WindowShown)
        {
            ActionParamsCount = 4;
            AdjustActionHeight();
            LastActionParamsCount = ActionParamsCount;
        }

        return;
    }

    int idx = SendMessage(hActionList, LB_GETCURSEL, 0, NULL);
    SelectedActionIndex = idx;

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
                    value.Format("%f", *(float*)&nValue);
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

}

void CNewTrigger::OnSelchangeActionType(bool edited)
{
    if (SelectedTriggerIndex < 0 || SendMessage(hActiontype, LB_GETCURSEL, NULL, NULL) < 0 || !CurrentTrigger)
        return;
    int curSel = SendMessage(hActiontype, CB_GETCURSEL, NULL, NULL);

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
    OnSelchangeActionListbox(false);
    
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

    FString::TrimIndex(text);
    if (text == "")
        text = "0";

    text.Replace(",", "");

    CurrentTrigger->Events[SelectedEventIndex].Params[EventParamsUsage[index].second] = text;
    CurrentTrigger->Save();

    UpdateParamAffectedParam_Event(index);
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

    FString::TrimIndex(text);
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
                auto paramType = FString::SplitString(CINI::FAData->GetString("ParamTypes", target.ParamMap[text]), 1);
                ExtraWindow::LoadParams(hActionParameter[target.AffectedParam], paramType[1]);
                //SendMessage(hActionParameterDesc[target.AffectedParam], WM_SETTEXT, 0, (LPARAM)paramType[0].m_pchData);
                if (paramType[1] == "10") // stringtables
                {
                    CurrentCSFActionParam = target.AffectedParam;
                }
                else if (paramType[1] == "9") // triggers
                {
                    CurrentTriggerActionParam = target.AffectedParam;
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
                auto paramType = FString::SplitString(CINI::FAData->GetString("ParamTypes", target.ParamMap[text]), 1);
                ExtraWindow::LoadParams(hEventParameter[target.AffectedParam], paramType[1]);
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

void CNewTrigger::OnSelchangeTrigger(bool edited, int eventListCur, int actionListCur)
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
        SendMessage(hActionList, LB_SETCURSEL, -1, NULL);
        SendMessage(hEventDescription, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hActionDescription, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)"");
        while (SendMessage(hEventList, LB_DELETESTRING, 0, NULL) != CB_ERR);
        while (SendMessage(hActionList, LB_DELETESTRING, 0, NULL) != CB_ERR);
        OnSelchangeActionListbox();
        OnSelchangeEventListbox();
        return;
    }

    FString pID;
    SendMessage(hSelectedTrigger, CB_GETLBTEXT, SelectedTriggerIndex, (LPARAM)buffer);
    pID = buffer;
    FString::TrimIndex(pID);

    CurrentTriggerID = pID;

    CMapDataExt::DeleteTrigger(CurrentTriggerID);
    CMapDataExt::AddTrigger(CurrentTriggerID);

    CurrentTrigger = CMapDataExt::GetTrigger(CurrentTriggerID);
    if (!CurrentTrigger) return;

    SendMessage(hName, WM_SETTEXT, 0, (LPARAM)CurrentTrigger->Name.c_str());
    
    int houseidx = SendMessage(hHouse, CB_FINDSTRINGEXACT, 0, (LPARAM)Translations::ParseHouseName(CurrentTrigger->House, true).c_str());
    if (houseidx != CB_ERR)
        SendMessage(hHouse, CB_SETCURSEL, houseidx, NULL);
    else
        SendMessage(hHouse, WM_SETTEXT, 0, (LPARAM)CurrentTrigger->House.c_str());
    
    int repeat = atoi(CurrentTrigger->RepeatType);
    if (repeat >= 0 && repeat <= 2)
        SendMessage(hType, CB_SETCURSEL, repeat, NULL);
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
        SendMessage(hEventList, LB_INSERTSTRING, i, (LPARAM)(LPCSTR)ExtraWindow::GetEventDisplayName(CurrentTrigger->Events[i].EventNum, i));
    }
    for (int i = 0; i < CurrentTrigger->ActionCount; i++)
    {
        SendMessage(hActionList, LB_INSERTSTRING, i, (LPARAM)(LPCSTR)ExtraWindow::GetActionDisplayName(CurrentTrigger->Actions[i].ActionNum, i));
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
    SendMessage(hActionList, LB_SETCURSEL, actionListCur, NULL);
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

    SortTriggers(CurrentTrigger->ID);

    int idx = SendMessage(hAttachedtrigger, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetTriggerDisplayName(CurrentTrigger->AttachedTrigger).c_str());
    SendMessage(hAttachedtrigger, CB_SETCURSEL, idx, NULL);

    if (CurrentTriggerActionParam > -1)
    {
        OnSelchangeActionListbox();
    }
}

void CNewTrigger::OnClickNewTrigger()
{
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
        if (idx >= SendMessage(hSelectedTrigger, CB_GETCOUNT, NULL, NULL))
            idx--;
        if (idx < 0)
            idx = 0;
        SendMessage(hSelectedTrigger, CB_SETCURSEL, idx, NULL);

        OnSelchangeTrigger();
    }
}

void CNewTrigger::OnClickPlaceOnMap(HWND& hWnd)
{
    if (!CurrentTrigger) return;
    if (!CurrentTrigger->Tag) return;
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
            SendMessage(hEventList, LB_INSERTSTRING, i, (LPARAM)(LPCSTR)ExtraWindow::GetEventDisplayName(CurrentTrigger->Events[i].EventNum, i));
        }

        SelectedEventIndex = SendMessage(hEventList, LB_GETCOUNT, NULL, NULL) - 1;
        SendMessage(hEventList, LB_SETCURSEL, SelectedEventIndex, NULL);
        OnSelchangeEventListbox();
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
            SendMessage(hEventList, LB_INSERTSTRING, i, (LPARAM)(LPCSTR)ExtraWindow::GetEventDisplayName(CurrentTrigger->Events[i].EventNum, i));
        }

        SelectedEventIndex = SendMessage(hEventList, LB_GETCOUNT, NULL, NULL) - 1;
        SendMessage(hEventList, LB_SETCURSEL, SelectedEventIndex, NULL);
        OnSelchangeEventListbox();
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
        SendMessage(hEventList, LB_INSERTSTRING, i, (LPARAM)(LPCSTR)ExtraWindow::GetEventDisplayName(CurrentTrigger->Events[i].EventNum, i));
    }

    SelectedEventIndex -= 1;
    if (SelectedEventIndex < 0)
        SelectedEventIndex = 0;
    if (SelectedEventIndex >= SendMessage(hEventList, LB_GETCOUNT, NULL, NULL))
        SelectedEventIndex = SendMessage(hEventList, LB_GETCOUNT, NULL, NULL) - 1;
    SendMessage(hEventList, LB_SETCURSEL, SelectedEventIndex, NULL);
    OnSelchangeEventListbox();
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
            SendMessage(hActionList, LB_INSERTSTRING, i, (LPARAM)(LPCSTR)ExtraWindow::GetActionDisplayName(CurrentTrigger->Actions[i].ActionNum, i));
        }

        SelectedActionIndex = SendMessage(hActionList, LB_GETCOUNT, NULL, NULL) - 1;
        SendMessage(hActionList, LB_SETCURSEL, SelectedActionIndex, NULL);
        OnSelchangeActionListbox();
    }
}

void CNewTrigger::OnClickCloAction(HWND& hWnd)
{
    if (!CurrentTrigger) return;
    if (SelectedActionIndex < 0 || SelectedActionIndex >= CurrentTrigger->ActionCount) return;

    ActionParams newAction = CurrentTrigger->Actions[SelectedActionIndex];

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
            SendMessage(hActionList, LB_INSERTSTRING, i, (LPARAM)(LPCSTR)ExtraWindow::GetActionDisplayName(CurrentTrigger->Actions[i].ActionNum, i));
        }

        SelectedActionIndex = SendMessage(hActionList, LB_GETCOUNT, NULL, NULL) - 1;
        SendMessage(hActionList, LB_SETCURSEL, SelectedActionIndex, NULL);
        OnSelchangeActionListbox();
    }
}

void CNewTrigger::OnClickDelAction(HWND& hWnd)
{
    if (!CurrentTrigger) return;
    if (SelectedActionIndex < 0 || SelectedActionIndex >= CurrentTrigger->ActionCount) return;

    CurrentTrigger->ActionCount--;
    CurrentTrigger->Actions.erase(CurrentTrigger->Actions.begin() + SelectedActionIndex);
    CurrentTrigger->Save();

    while (SendMessage(hActionList, LB_DELETESTRING, 0, NULL) != CB_ERR);
    for (int i = 0; i < CurrentTrigger->ActionCount; i++)
    {
        SendMessage(hActionList, LB_INSERTSTRING, i, (LPARAM)(LPCSTR)ExtraWindow::GetActionDisplayName(CurrentTrigger->Actions[i].ActionNum, i));
    }

    SelectedActionIndex -= 1;
    if (SelectedActionIndex < 0)
        SelectedActionIndex = 0;
    if (SelectedActionIndex >= SendMessage(hActionList, LB_GETCOUNT, NULL, NULL))
        SelectedActionIndex = SendMessage(hActionList, LB_GETCOUNT, NULL, NULL) - 1;
    SendMessage(hActionList, LB_SETCURSEL, SelectedActionIndex, NULL);
    OnSelchangeActionListbox();
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
        SendMessage(hEventList, LB_INSERTSTRING, SelectedEventIndex, (LPARAM)(LPCSTR)ExtraWindow::GetEventDisplayName(thisEvent.EventNum, SelectedEventIndex));
        SendMessage(hEventList, LB_SETCURSEL, SelectedEventIndex, NULL);
    }
        
    auto eventInfos = FString::SplitString(fadata.GetString("EventsRA2", thisEvent.EventNum, "MISSING,0,0,0,0,MISSING,0,1,0"), 8);

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
    pParamTypes[0] = FString::SplitString(fadata.GetString("ParamTypes", paramType[0], "MISSING,0"));
    pParamTypes[1] = FString::SplitString(fadata.GetString("ParamTypes", paramType[1], "MISSING,0"));
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
                ExtraWindow::LoadParams(hEventParameter[i], pParamTypes[EventParamsUsage[i].second - 1][1]);
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
                ExtraWindow::LoadParams(hEventParameter[i], pParamTypes[EventParamsUsage[i].second][1]);
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
        SendMessage(hActionList, LB_INSERTSTRING, SelectedActionIndex, (LPARAM)(LPCSTR)ExtraWindow::GetActionDisplayName(thisAction.ActionNum, SelectedActionIndex));
        SendMessage(hActionList, LB_SETCURSEL, SelectedActionIndex, NULL);
    }
    auto actionInfos = FString::SplitString(fadata.GetString("ActionsRA2", thisAction.ActionNum, "MISSING,0,0,0,0,0,0,0,0,0,MISSING,0,1,0"), 13);

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
        pParamTypes[i] = FString::SplitString(fadata.GetString("ParamTypes", paramType[i], "MISSING,0"));

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
                ExtraWindow::LoadParams(hActionParameter[i], pParamTypes[ActionParamsUsage[i].second][1]);

                SendMessage(hActionParameterDesc[i], WM_SETTEXT, 0, (LPARAM)pParamTypes[ActionParamsUsage[i].second][0].c_str());
                if (pParamTypes[ActionParamsUsage[i].second][1] == "10") // stringtables
                {
                    //thisAction.Params[ActionParamsUsage[i].second].MakeLower();
                    CurrentCSFActionParam = i;
                }
                if (pParamTypes[ActionParamsUsage[i].second][1] == "1" && !ExtConfigs::SearchCombobox_Waypoint) // waypoints
                {
                    CNewTrigger::ActionParameterAutoDrop[i] = false;
                }
                else if (pParamTypes[ActionParamsUsage[i].second][1] == "9") // triggers
                {
                    CurrentTriggerActionParam = i;
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

    auto adjustHeight = [&heightDistance](HWND& hWnd)
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
        SortTriggers(CurrentTrigger->ID);
        int idx = SendMessage(hAttachedtrigger, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetTriggerDisplayName(CurrentTrigger->AttachedTrigger).c_str());
        SendMessage(hAttachedtrigger, CB_SETCURSEL, idx, NULL);
        if (CurrentTriggerActionParam > -1)
        {
            OnSelchangeActionListbox();
        }
        DropNeedUpdate = false;
    }

    if (index == CurrentCSFActionParam && ExtConfigs::TutorialTexts_Viewer)
    {
        PostMessage(hActionParameter[index], CB_SHOWDROPDOWN, FALSE, 0);
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
        FString::TrimIndex(text);
        text.MakeLower();
        CCsfEditor::CurrentSelectedCSF = text.c_str();

        ::SendMessage(CCsfEditor::GetHandle(), 114515, 0, 0);
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

void CNewTrigger::OnClickSearchReference(HWND& hWnd)
{
    if (SelectedTriggerIndex < 0 || !CurrentTrigger)
        return;

    CSearhReference::SetSearchType(1);
    CSearhReference::SetSearchID(CurrentTrigger->ID);
    if (CSearhReference::GetHandle() == NULL)
    {
        CSearhReference::Create(m_parent);
    }
    else
    {
        ::SendMessage(CSearhReference::GetHandle(), 114514, 0, 0);
    }
}

void CNewTrigger::SortTriggers(FString id)
{
    while (SendMessage(hSelectedTrigger, CB_DELETESTRING, 0, NULL) != CB_ERR);
    std::vector<FString> labels;
    for (auto& triggerPair : CMapDataExt::Triggers) {
        auto& trigger = triggerPair.second;
        labels.push_back(ExtraWindow::GetTriggerDisplayName(trigger->ID));
    }
   
    bool tmp = ExtConfigs::SortByLabelName;
    ExtConfigs::SortByLabelName = ExtConfigs::SortByLabelName_Trigger;

    std::sort(labels.begin(), labels.end(), ExtraWindow::SortLabels);

    ExtConfigs::SortByLabelName = tmp;

    for (size_t i = 0; i < labels.size(); ++i) {
        SendMessage(hSelectedTrigger, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)labels[i].c_str());
    }
    ExtraWindow::SyncComboBoxContent(hSelectedTrigger, hAttachedtrigger, true);
    if (id != "") {
        SelectedTriggerIndex = SendMessage(hSelectedTrigger, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetTriggerDisplayName(id).c_str());
        SendMessage(hSelectedTrigger, CB_SETCURSEL, SelectedTriggerIndex, NULL);
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