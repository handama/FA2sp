#include "CNewAITrigger.h"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
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
#include <numeric>
#include "../CTriggerAnnotation/CTriggerAnnotation.h"

HWND CNewAITrigger::m_hwnd;
CFinalSunDlg* CNewAITrigger::m_parent;
CINI& CNewAITrigger::map = CINI::CurrentDocument;
MultimapHelper& CNewAITrigger::rules = Variables::RulesMap;
CINI& CNewAITrigger::fadata = CINI::FAData;

HWND CNewAITrigger::hSelectedAITrigger;
HWND CNewAITrigger::hEnabled;
HWND CNewAITrigger::hAdd;
HWND CNewAITrigger::hClone;
HWND CNewAITrigger::hDelete;
HWND CNewAITrigger::hName;
HWND CNewAITrigger::hSide;
HWND CNewAITrigger::hCountry;
HWND CNewAITrigger::hConditionType;
HWND CNewAITrigger::hComparator;
HWND CNewAITrigger::hAmount;
HWND CNewAITrigger::hComparisonObject;
HWND CNewAITrigger::hTeam1;
HWND CNewAITrigger::hTeam2;
HWND CNewAITrigger::hInitialWeight;
HWND CNewAITrigger::hMinWeight;
HWND CNewAITrigger::hMaxWeight;
HWND CNewAITrigger::hEasy;
HWND CNewAITrigger::hMedium;
HWND CNewAITrigger::hHard;
HWND CNewAITrigger::hBaseDefense;
HWND CNewAITrigger::hSkrimish;

int CNewAITrigger::SelectedAITriggerIndex = -1;
std::unique_ptr<AITrigger> CNewAITrigger::CurrentAITrigger;
std::map<int, FString> CNewAITrigger::AITriggerLabels;
std::map<int, FString> CNewAITrigger::TeamLabels[2];
std::map<int, FString> CNewAITrigger::ComparisonObjectLabels;
std::map<int, FString> CNewAITrigger::CountryLabels;
bool CNewAITrigger::Autodrop;
bool CNewAITrigger::DropNeedUpdate;
bool CNewAITrigger::TeamListChanged = false;

void CNewAITrigger::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(312),
        pWnd->GetSafeHwnd(),
        CNewAITrigger::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CNewAITrigger.\n");
        m_parent = NULL;
        return;
    }
}

void CNewAITrigger::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("AITriggerEditorTitle", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };
    
    Translate(1000, "AITriggerEditorDesc");
    Translate(1001, "AITriggerEditorAITrigger");
    Translate(1003, "AITriggerEditorEnabled");
    Translate(1004, "AITriggerEditorAdd");
    Translate(1005, "AITriggerEditorClone");
    Translate(1006, "AITriggerEditorDelete");
    Translate(1007, "AITriggerEditorName");
    Translate(1009, "AITriggerEditorSide");
    Translate(1011, "AITriggerEditorCountry");
    Translate(1013, "AITriggerEditorCondition");
    Translate(1014, "AITriggerEditorConditionType");
    Translate(1016, "AITriggerEditorComparator");
    Translate(1018, "AITriggerEditorAmount");
    Translate(1020, "AITriggerEditorComparisonObject");
    Translate(1022, "AITriggerEditorTeams");
    Translate(1023, "AITriggerEditorTeam1");
    Translate(1025, "AITriggerEditorTeam2");
    Translate(1027, "AITriggerEditorWeights");
    Translate(1028, "AITriggerEditorInitial");
    Translate(1030, "AITriggerEditorMin");
    Translate(1032, "AITriggerEditorMax");
    Translate(1034, "AITriggerEditorDifficulties");
    Translate(1035, "AITriggerEditorEasy");
    Translate(1036, "AITriggerEditorMedium");
    Translate(1037, "AITriggerEditorHard");
    Translate(1038, "AITriggerEditorBaseDefense");
    Translate(1039, "AITriggerEditorSkirmish");

    hSelectedAITrigger = GetDlgItem(hWnd, Controls::SelectedAITrigger);
    hEnabled = GetDlgItem(hWnd, Controls::Enabled);
    hAdd = GetDlgItem(hWnd, Controls::Add);
    hClone = GetDlgItem(hWnd, Controls::Clone);
    hDelete = GetDlgItem(hWnd, Controls::Delete);
    hName = GetDlgItem(hWnd, Controls::Name);
    hSide = GetDlgItem(hWnd, Controls::Side);
    hCountry = GetDlgItem(hWnd, Controls::Country);
    hConditionType = GetDlgItem(hWnd, Controls::ConditionType);
    hComparator = GetDlgItem(hWnd, Controls::Comparator);
    hAmount = GetDlgItem(hWnd, Controls::Amount);
    hComparisonObject = GetDlgItem(hWnd, Controls::ComparisonObject);
    hTeam1 = GetDlgItem(hWnd, Controls::Team1);
    hTeam2 = GetDlgItem(hWnd, Controls::Team2);
    hInitialWeight = GetDlgItem(hWnd, Controls::InitialWeight);
    hMinWeight = GetDlgItem(hWnd, Controls::MinWeight);
    hMaxWeight = GetDlgItem(hWnd, Controls::MaxWeight);
    hEasy = GetDlgItem(hWnd, Controls::Easy);
    hMedium = GetDlgItem(hWnd, Controls::Medium);
    hHard = GetDlgItem(hWnd, Controls::Hard);
    hBaseDefense = GetDlgItem(hWnd, Controls::BaseDefense);
    hSkrimish = GetDlgItem(hWnd, Controls::Skrimish);

    Update(hWnd);
}

void CNewAITrigger::Update(HWND& hWnd)
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    ExtraWindow::SortAITriggers(hSelectedAITrigger, SelectedAITriggerIndex);
    int count = SendMessage(hSelectedAITrigger, CB_GETCOUNT, NULL, NULL);
    if (SelectedAITriggerIndex < 0)
        SelectedAITriggerIndex = 0;
    if (SelectedAITriggerIndex > count - 1)
        SelectedAITriggerIndex = count - 1;
    SendMessage(hSelectedAITrigger, CB_SETCURSEL, SelectedAITriggerIndex, NULL);

    int idx = 0;
    while (SendMessage(hSide, CB_DELETESTRING, 0, NULL) != CB_ERR);

    auto transed = FinalAlertConfig::Language + "-" + "AITriggerSides";
    if (!CINI::FAData().SectionExists(transed))
        transed = "AITriggerSides";

    if (auto pSection = fadata.GetSection(transed))
    {
        for (auto& pair : pSection->GetEntities())
        {
            FString text;
            text.Format("%s - %s", pair.first, pair.second);
            SendMessage(hSide, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)text);
        }
    }
    
    idx = 0;
    while (SendMessage(hCountry, CB_DELETESTRING, 0, NULL) != CB_ERR);
    const auto& indicies = Variables::RulesMap.ParseIndicies("Countries", true);
    SendMessage(hCountry, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"<all>");
    for (auto& value : indicies)
    {
        if (value == "GDI" || value == "Nod")
            continue;
        SendMessage(hCountry, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)Translations::ParseHouseName(value, true));
    }
    
    idx = 0;
    while (SendMessage(hComparator, CB_DELETESTRING, 0, NULL) != CB_ERR);
    SendMessage(hComparator, CB_INSERTSTRING, idx++, 
        (LPARAM)(LPCSTR)(FString("0 - ") + Translations::TranslateOrDefault("AITriggerEditorComparator1", "Less than")));
    SendMessage(hComparator, CB_INSERTSTRING, idx++, 
        (LPARAM)(LPCSTR)(FString("1 - ") + Translations::TranslateOrDefault("AITriggerEditorComparator2", "Less than or equal to")));
    SendMessage(hComparator, CB_INSERTSTRING, idx++, 
        (LPARAM)(LPCSTR)(FString("2 - ") + Translations::TranslateOrDefault("AITriggerEditorComparator3", "Equal to")));
    SendMessage(hComparator, CB_INSERTSTRING, idx++, 
        (LPARAM)(LPCSTR)(FString("3 - ") + Translations::TranslateOrDefault("AITriggerEditorComparator4", "Greater than or equal to")));
    SendMessage(hComparator, CB_INSERTSTRING, idx++, 
        (LPARAM)(LPCSTR)(FString("4 - ") + Translations::TranslateOrDefault("AITriggerEditorComparator5", "Greater than")));
    SendMessage(hComparator, CB_INSERTSTRING, idx++, 
        (LPARAM)(LPCSTR)(FString("5 - ") + Translations::TranslateOrDefault("AITriggerEditorComparator6", "Not equal to")));

    idx = 0;
    while (SendMessage(hConditionType, CB_DELETESTRING, 0, NULL) != CB_ERR);
    SendMessage(hConditionType, CB_INSERTSTRING, idx++, 
        (LPARAM)(LPCSTR)(FString("-1 - ") + 
            Translations::TranslateOrDefault("AITriggerEditorCondition-1", "Always true")));
    SendMessage(hConditionType, CB_INSERTSTRING, idx++, 
        (LPARAM)(LPCSTR)(FString("0 - ") + 
            Translations::TranslateOrDefault("AITriggerEditorCondition0", "Enemy house owns X object <Comparator> N")));
    SendMessage(hConditionType, CB_INSERTSTRING, idx++, 
        (LPARAM)(LPCSTR)(FString("1 - ") + 
            Translations::TranslateOrDefault("AITriggerEditorCondition1", "Owning house owns X object <Comparator> N")));
    SendMessage(hConditionType, CB_INSERTSTRING, idx++, 
        (LPARAM)(LPCSTR)(FString("2 - ") + 
            Translations::TranslateOrDefault("AITriggerEditorCondition2", "Enemy house in low power (yellow)")));
    SendMessage(hConditionType, CB_INSERTSTRING, idx++, 
        (LPARAM)(LPCSTR)(FString("3 - ") + 
            Translations::TranslateOrDefault("AITriggerEditorCondition3", "Enemy house in low power (red)")));
    SendMessage(hConditionType, CB_INSERTSTRING, idx++, 
        (LPARAM)(LPCSTR)(FString("4 - ") + 
            Translations::TranslateOrDefault("AITriggerEditorCondition4", "Enemy house has credits <Comparator> N")));
    SendMessage(hConditionType, CB_INSERTSTRING, idx++, 
        (LPARAM)(LPCSTR)(FString("5 - ") + 
            Translations::TranslateOrDefault("AITriggerEditorCondition5", "Iron Curtain is about to be ready")));
    SendMessage(hConditionType, CB_INSERTSTRING, idx++, 
        (LPARAM)(LPCSTR)(FString("6 - ") + 
            Translations::TranslateOrDefault("AITriggerEditorCondition6", "ChronoSphere is about to be ready")));
    SendMessage(hConditionType, CB_INSERTSTRING, idx++, 
        (LPARAM)(LPCSTR)(FString("7 - ") + 
            Translations::TranslateOrDefault("AITriggerEditorCondition7", "Neutral/civilian house owns X object <Comparator> N")));

    idx = 0;
    while (SendMessage(hComparisonObject, CB_DELETESTRING, 0, NULL) != CB_ERR);
    ExtraWindow::LoadParam_TechnoTypes(hComparisonObject, -1, 1);
    SendMessage(hComparisonObject, CB_INSERTSTRING, SendMessage(hComparisonObject, CB_GETCOUNT, 0, 0), (LPARAM)(LPCSTR)"<none>");

    int tmp = 0;
    ExtraWindow::SortTeams(hTeam1, "TeamTypes", tmp);
    SendMessage(hTeam1, CB_INSERTSTRING, SendMessage(hTeam1, CB_GETCOUNT, 0, 0), (LPARAM)(LPCSTR)"<none>");
    ExtraWindow::SyncComboBoxContent(hTeam1, hTeam2, false);

    DropNeedUpdate = false;
    Autodrop = false;

    OnSelchangeAITrigger();
}

void CNewAITrigger::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CNewAITrigger::m_hwnd = NULL;
    CNewAITrigger::m_parent = NULL;

}

BOOL CALLBACK CNewAITrigger::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_ACTIVATE:
    {
        if (CurrentAITrigger)
        {
            CTriggerAnnotation::Type = AnnoAITrigger;
            CTriggerAnnotation::ID = CurrentAITrigger->ID;
            ::SendMessage(CTriggerAnnotation::GetHandle(), 114515, 0, 0);
        }
        return TRUE;
    }
    case WM_INITDIALOG:
    {
        CNewAITrigger::Initialize(hWnd);
        return TRUE;
    }	
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::Add:
            if (CODE == BN_CLICKED)
                OnClickNewAITrigger();
            break;
        case Controls::Clone:
            if (CODE == BN_CLICKED)
                OnClickCloAITrigger();
            break;
        case Controls::Delete:
            if (CODE == BN_CLICKED)
                OnClickDelAITrigger();
            break;
        case Controls::SelectedAITrigger:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeAITrigger();
            else if (CODE == CBN_DROPDOWN)
                OnSeldropdownAITrigger(hWnd);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeAITrigger(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hSelectedAITrigger, AITriggerLabels, true);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::Name:
            if (CODE == EN_CHANGE && CurrentAITrigger)
            {
                char buffer[512]{ 0 };
                GetWindowText(hName, buffer, 511);
                FString name(buffer);
                name.Replace(",", "");

                CurrentAITrigger->Name = name;
                CurrentAITrigger->Save();
                DropNeedUpdate = true;

                auto newName = ExtraWindow::FormatTriggerDisplayName(CurrentAITrigger->ID, CurrentAITrigger->Name);
                SendMessage(hSelectedAITrigger, CB_DELETESTRING, SelectedAITriggerIndex, NULL);
                SendMessage(hSelectedAITrigger, CB_INSERTSTRING, SelectedAITriggerIndex, (LPARAM)(LPCSTR)newName);
                SendMessage(hSelectedAITrigger, CB_SETCURSEL, SelectedAITriggerIndex, NULL);
            }
            break;
        case Controls::Enabled:
            if (CODE == BN_CLICKED && CurrentAITrigger)
            {
                CurrentAITrigger->Enabled = SendMessage(hEnabled, BM_GETCHECK, 0, 0);
                CurrentAITrigger->Save();
            }
            break;
        case Controls::Easy:
            if (CODE == BN_CLICKED && CurrentAITrigger)
            {
                CurrentAITrigger->EnabledInE = SendMessage(hEasy, BM_GETCHECK, 0, 0);
                CurrentAITrigger->Save();
            }
            break;
        case Controls::Medium:
            if (CODE == BN_CLICKED && CurrentAITrigger)
            {
                CurrentAITrigger->EnabledInM = SendMessage(hMedium, BM_GETCHECK, 0, 0);
                CurrentAITrigger->Save();
            }
            break;
        case Controls::Hard:
            if (CODE == BN_CLICKED && CurrentAITrigger)
            {
                CurrentAITrigger->EnabledInH = SendMessage(hHard, BM_GETCHECK, 0, 0);
                CurrentAITrigger->Save();
            }
            break;
        case Controls::BaseDefense:
            if (CODE == BN_CLICKED && CurrentAITrigger)
            {
                CurrentAITrigger->IsBaseDefense = SendMessage(hBaseDefense, BM_GETCHECK, 0, 0);
                CurrentAITrigger->Save();
            }
            break;
        case Controls::Skrimish:
            if (CODE == BN_CLICKED && CurrentAITrigger)
            {
                CurrentAITrigger->IsForSkirmish = SendMessage(hSkrimish, BM_GETCHECK, 0, 0);
                CurrentAITrigger->Save();
            }
            break;
        case Controls::InitialWeight:
            if (CODE == EN_CHANGE && CurrentAITrigger)
            {
                char buffer[512]{ 0 };
                GetWindowText(hInitialWeight, buffer, 511);
                CurrentAITrigger->InitialWeight = safe_stod(buffer);
                CurrentAITrigger->Save();
            }
            break;
        case Controls::MinWeight:
            if (CODE == EN_CHANGE && CurrentAITrigger)
            {
                char buffer[512]{ 0 };
                GetWindowText(hMinWeight, buffer, 511);
                CurrentAITrigger->MinWeight = safe_stod(buffer);
                CurrentAITrigger->Save();
            } 
            break;
        case Controls::MaxWeight:
            if (CODE == EN_CHANGE && CurrentAITrigger)
            {
                char buffer[512]{ 0 };
                GetWindowText(hMaxWeight, buffer, 511);
                CurrentAITrigger->MaxWeight = safe_stod(buffer);
                CurrentAITrigger->Save();
            }
            break;
        case Controls::Amount:
            if (CODE == EN_CHANGE && CurrentAITrigger)
            {
                char buffer[512]{ 0 };
                GetWindowText(hAmount, buffer, 511);
                CurrentAITrigger->Comparator[0] = atoi(buffer);
                CurrentAITrigger->Save();
            }
            break;
        case Controls::Country:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeCountry();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeCountry(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hCountry, CountryLabels);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::Side:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeSide();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeSide(true);
            break;
        case Controls::ConditionType:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeConditionType();
            break;
        case Controls::Comparator:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeComparator();
            break;
        case Controls::ComparisonObject:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeComparisonObject();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeComparisonObject(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hComparisonObject, ComparisonObjectLabels);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::Team1:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTeam(0);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeTeam(0, true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hTeam1, TeamLabels[0]);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            else if (CODE == CBN_DROPDOWN && TeamListChanged)
                OnDropdownTeam();
            break;
        case Controls::Team2:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTeam(1);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeTeam(1, true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hTeam2, TeamLabels[1]);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            else if (CODE == CBN_DROPDOWN && TeamListChanged)
                OnDropdownTeam();
            break;
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CNewAITrigger::Close(hWnd);
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

double CNewAITrigger::safe_stod(const char* s) {
    auto v = VEHGuard(false);
    try {
        double val = std::stod(s);
        return val;
    }
    catch (const std::exception&) {
        return 0.0;
    }
}

void CNewAITrigger::OnSelchangeAITrigger(bool edited, int specificIdx)
{
    char buffer[512]{ 0 };

    if (edited && (SendMessage(hSelectedAITrigger, CB_GETCOUNT, NULL, NULL) > 0 || !AITriggerLabels.empty()))
    {
        Autodrop = true;
        ExtraWindow::OnEditCComboBox(hSelectedAITrigger, AITriggerLabels);
        return;
    }

    SelectedAITriggerIndex = SendMessage(hSelectedAITrigger, CB_GETCURSEL, NULL, NULL);
    if (SelectedAITriggerIndex < 0 || SelectedAITriggerIndex >= SendMessage(hSelectedAITrigger, CB_GETCOUNT, NULL, NULL))
    {
        SelectedAITriggerIndex = -1;
        CurrentAITrigger = nullptr;
        SendMessage(hEasy, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hHard, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hMedium, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hEnabled, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hBaseDefense, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hSkrimish, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hSide, CB_SETCURSEL, -1, NULL);
        SendMessage(hCountry, CB_SETCURSEL, -1, NULL);
        SendMessage(hConditionType, CB_SETCURSEL, -1, NULL);
        SendMessage(hComparator, CB_SETCURSEL, -1, NULL);
        SendMessage(hComparisonObject, CB_SETCURSEL, -1, NULL);
        SendMessage(hTeam1, CB_SETCURSEL, -1, NULL);
        SendMessage(hTeam2, CB_SETCURSEL, -1, NULL);
        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hAmount, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hInitialWeight, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hMinWeight, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hMaxWeight, WM_SETTEXT, 0, (LPARAM)"");
        return;
    }

    FString pID;
    SendMessage(hSelectedAITrigger, CB_GETLBTEXT, SelectedAITriggerIndex, (LPARAM)buffer);
    pID = buffer;
    FString::TrimIndex(pID);

    CurrentAITrigger = AITrigger::create(pID);

    CTriggerAnnotation::Type = AnnoAITrigger;
    CTriggerAnnotation::ID = CurrentAITrigger->ID;
    ::SendMessage(CTriggerAnnotation::GetHandle(), 114515, 0, 0);

    SendMessage(hEasy, BM_SETCHECK, CurrentAITrigger->EnabledInE, 0);
    SendMessage(hHard, BM_SETCHECK, CurrentAITrigger->EnabledInH, 0);
    SendMessage(hMedium, BM_SETCHECK, CurrentAITrigger->EnabledInM, 0);
    SendMessage(hEnabled, BM_SETCHECK, CurrentAITrigger->Enabled, 0);
    SendMessage(hBaseDefense, BM_SETCHECK, CurrentAITrigger->IsBaseDefense, 0);
    SendMessage(hSkrimish, BM_SETCHECK, CurrentAITrigger->IsForSkirmish, 0);

    auto setCurselOrSetText = [](HWND& hwnd, FString text) {
            int idx = ExtraWindow::FindCBStringExactStart(hwnd, text + " ");
            if (idx != CB_ERR) {
                SendMessage(hwnd, CB_SETCURSEL, idx, NULL);
            }
            else {
                SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)text);
            }
        };

    int idx = SendMessage(hCountry, CB_FINDSTRINGEXACT, 0, (LPARAM)Translations::ParseHouseName(CurrentAITrigger->House, true));
    if (idx != CB_ERR) {
        SendMessage(hCountry, CB_SETCURSEL, idx, NULL);
    }
    else {
        SendMessage(hCountry, WM_SETTEXT, 0, (LPARAM)CurrentAITrigger->House);
    }

    setCurselOrSetText(hSide, CurrentAITrigger->Side);
    setCurselOrSetText(hConditionType, CurrentAITrigger->ConditionType);
    FString comparator;
    comparator.Format("%d", CurrentAITrigger->Comparator[1]);
    setCurselOrSetText(hComparator, comparator);
    setCurselOrSetText(hComparisonObject, CurrentAITrigger->ComparisonObject);
    setCurselOrSetText(hTeam1, CurrentAITrigger->Team1);
    setCurselOrSetText(hTeam2, CurrentAITrigger->Team2);
    SendMessage(hName, WM_SETTEXT, 0, (LPARAM)CurrentAITrigger->Name);
    FString amount;
    amount.Format("%d", CurrentAITrigger->Comparator[0]);
    SendMessage(hAmount, WM_SETTEXT, 0, (LPARAM)amount);
    std::ostringstream oss;
    oss.precision(6);
    oss << std::fixed << CurrentAITrigger->InitialWeight;
    FString initial = oss.str();
    oss.str("");
    oss.precision(6);
    oss << std::fixed << CurrentAITrigger->MinWeight;
    FString min = oss.str();
    oss.str("");
    oss.precision(6);
    oss << std::fixed << CurrentAITrigger->MaxWeight;
    FString max = oss.str();
    oss.str("");

    SendMessage(hInitialWeight, WM_SETTEXT, 0, (LPARAM)initial);
    SendMessage(hMinWeight, WM_SETTEXT, 0, (LPARAM)min);
    SendMessage(hMaxWeight, WM_SETTEXT, 0, (LPARAM)max);

    //CurrentAITrigger->Save();
    DropNeedUpdate = false;
}

void CNewAITrigger::OnDropdownTeam()
{
    int curSel1 = SendMessage(hTeam1, CB_GETCURSEL, NULL, NULL);
    int curSel2 = SendMessage(hTeam2, CB_GETCURSEL, NULL, NULL);
    char buffer[512]{ 0 };
    GetWindowText(hTeam1, buffer, 511);
    FString text1(buffer);
    GetWindowText(hTeam2, buffer, 511);
    FString text2(buffer);

    int tmp = 0;
    ExtraWindow::SortTeams(hTeam1, "TeamTypes", tmp);
    SendMessage(hTeam1, CB_INSERTSTRING, SendMessage(hTeam1, CB_GETCOUNT, 0, 0), (LPARAM)(LPCSTR)"<none>");
    ExtraWindow::SyncComboBoxContent(hTeam1, hTeam2, false);
    TeamListChanged = false;

    int idx = SendMessage(hTeam1, CB_FINDSTRINGEXACT, 0, text1);
    if (idx != CB_ERR)
    {
        SendMessage(hTeam1, CB_SETCURSEL, idx, NULL);
    }
    else
    {
        FString::TrimIndex(text1);
        SendMessage(hTeam1, WM_SETTEXT, NULL, text1);
    }
    idx = SendMessage(hTeam2, CB_FINDSTRINGEXACT, 0, text2);
    if (idx != CB_ERR)
    {
        SendMessage(hTeam2, CB_SETCURSEL, idx, NULL);
    }
    else
    {
        FString::TrimIndex(text2);
        SendMessage(hTeam2, WM_SETTEXT, NULL, text2);
    }
}

void CNewAITrigger::OnSelchangeCountry(bool edited)
{
    if (SelectedAITriggerIndex < 0 || SendMessage(hCountry, LB_GETCURSEL, NULL, NULL) < 0 || !CurrentAITrigger)
        return;
    int curSel = SendMessage(hCountry, CB_GETCURSEL, NULL, NULL);

    FString text;
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hCountry, CB_GETCOUNT, NULL, NULL) > 0 || !CountryLabels.empty()))
    {
        ExtraWindow::OnEditCComboBox(hCountry, CountryLabels);
    }

    if (curSel >= 0 && curSel < SendMessage(hCountry, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hCountry, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hCountry, buffer, 511);
        text = buffer;
        int idx = SendMessage(hCountry, CB_FINDSTRINGEXACT, 0, (LPARAM)text);
        if (idx != CB_ERR)
        {
            SendMessage(hCountry, CB_GETLBTEXT, idx, (LPARAM)buffer);
            text = buffer;
        }
    }

    if (!text)
        return;

    FString::TrimIndex(text);
    if (text == "")
        text = "<all>";

    text.Replace(",", "");

    CurrentAITrigger->House = Translations::ParseHouseName(text, false);
    CurrentAITrigger->Save();
}

void CNewAITrigger::OnSelchangeSide(bool edited)
{
    if (SelectedAITriggerIndex < 0 || SendMessage(hSide, LB_GETCURSEL, NULL, NULL) < 0 || !CurrentAITrigger)
        return;
    int curSel = SendMessage(hSide, CB_GETCURSEL, NULL, NULL);

    FString text;
    char buffer[512]{ 0 };

    if (curSel >= 0 && curSel < SendMessage(hSide, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hSide, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hSide, buffer, 511);
        text = buffer;
        int idx = SendMessage(hSide, CB_FINDSTRINGEXACT, 0, (LPARAM)text);
        if (idx != CB_ERR)
        {
            SendMessage(hSide, CB_GETLBTEXT, idx, (LPARAM)buffer);
            text = buffer;
        }
    }

    if (!text)
        return;

    FString::TrimIndex(text);
    if (text == "")
        text = "0";

    text.Replace(",", "");

    CurrentAITrigger->Side = text;
    CurrentAITrigger->Save();
}

void CNewAITrigger::OnSelchangeConditionType()
{
    if (SelectedAITriggerIndex < 0 || SendMessage(hConditionType, LB_GETCURSEL, NULL, NULL) < 0 || !CurrentAITrigger)
        return;
    int curSel = SendMessage(hConditionType, CB_GETCURSEL, NULL, NULL);

    FString text;
    char buffer[512]{ 0 };

    if (curSel >= 0 && curSel < SendMessage(hConditionType, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hConditionType, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (!text)
        return;

    FString::TrimIndex(text);
    if (text == "")
        text = "-1";

    text.Replace(",", "");

    CurrentAITrigger->ConditionType = text;
    CurrentAITrigger->Save();
}

void CNewAITrigger::OnSelchangeComparator()
{
    if (SelectedAITriggerIndex < 0 || SendMessage(hComparator, LB_GETCURSEL, NULL, NULL) < 0 || !CurrentAITrigger)
        return;
    int curSel = SendMessage(hComparator, CB_GETCURSEL, NULL, NULL);

    FString text;
    char buffer[512]{ 0 };

    if (curSel >= 0 && curSel < SendMessage(hComparator, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hComparator, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (!text)
        return;

    FString::TrimIndex(text);
    if (text == "")
        text = "0";

    text.Replace(",", "");

    CurrentAITrigger->Comparator[1] = atoi(text);
    CurrentAITrigger->Save();
}

void CNewAITrigger::OnSelchangeComparisonObject(bool edited)
{
    if (SelectedAITriggerIndex < 0 || SendMessage(hComparisonObject, LB_GETCURSEL, NULL, NULL) < 0 || !CurrentAITrigger)
        return;
    int curSel = SendMessage(hComparisonObject, CB_GETCURSEL, NULL, NULL);

    FString text;
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hComparisonObject, CB_GETCOUNT, NULL, NULL) > 0 || !ComparisonObjectLabels.empty()))
    {
        ExtraWindow::OnEditCComboBox(hComparisonObject, ComparisonObjectLabels);
    }

    if (curSel >= 0 && curSel < SendMessage(hComparisonObject, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hComparisonObject, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hComparisonObject, buffer, 511);
        text = buffer;
        int idx = SendMessage(hComparisonObject, CB_FINDSTRINGEXACT, 0, (LPARAM)text);
        if (idx != CB_ERR)
        {
            SendMessage(hComparisonObject, CB_GETLBTEXT, idx, (LPARAM)buffer);
            text = buffer;
        }
    }

    if (!text)
        return;

    FString::TrimIndex(text);
    if (text == "")
        text = "<none>";

    text.Replace(",", "");

    CurrentAITrigger->ComparisonObject = text;
    CurrentAITrigger->Save();
}

void CNewAITrigger::OnSelchangeTeam(int index, bool edited)
{
    auto& hwnd = index == 1 ? hTeam2 : hTeam1;
    if (SelectedAITriggerIndex < 0 || SendMessage(hwnd, LB_GETCURSEL, NULL, NULL) < 0 || !CurrentAITrigger)
        return;
    int curSel = SendMessage(hwnd, CB_GETCURSEL, NULL, NULL);

    FString text;
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hwnd, CB_GETCOUNT, NULL, NULL) > 0 || !TeamLabels[index].empty()))
    {
        ExtraWindow::OnEditCComboBox(hwnd, TeamLabels[index]);
    }

    if (curSel >= 0 && curSel < SendMessage(hwnd, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hwnd, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hwnd, buffer, 511);
        text = buffer;
        int idx = SendMessage(hwnd, CB_FINDSTRINGEXACT, 0, (LPARAM)text);
        if (idx != CB_ERR)
        {
            SendMessage(hwnd, CB_GETLBTEXT, idx, (LPARAM)buffer);
            text = buffer;
        }
    }

    if (!text)
        return;

    FString::TrimIndex(text);
    if (text == "")
        text = "<none>";

    text.Replace(",", "");

    if (index == 1)
        CurrentAITrigger->Team2 = text;
    else
        CurrentAITrigger->Team1 = text;
    CurrentAITrigger->Save();
}

void CNewAITrigger::OnClickNewAITrigger()
{
    FString id = CMapDataExt::GetAvailableIndex();
    FString value = "New AI Trigger,<none>,<all>,1,-1,<none>,0000000000000000000000000000000000000000000000000000000000000000,50.000000,30.000000,50.000000,1,0,1,0,<none>,1,1,1";

    map.WriteString("AITriggerTypes", id, value);
    map.WriteBool("AITriggerTypesEnable", id, true);

    SortAITriggers(id);
    OnSelchangeAITrigger();
}

void CNewAITrigger::OnClickCloAITrigger()
{
    if (!CurrentAITrigger) return;

    FString id = CMapDataExt::GetAvailableIndex();
    AITrigger trigger2;
    trigger2 = *CurrentAITrigger;
    trigger2.ID = id;
    trigger2.Name = ExtraWindow::GetCloneName(CurrentAITrigger->Name);
    trigger2.Save();

    SortAITriggers(id);
    OnSelchangeAITrigger();
}
void CNewAITrigger::OnClickDelAITrigger()
{
    if (!CurrentAITrigger) return;
    FString pMessage = Translations::TranslateOrDefault("AITriggerDeleteMessage",
        "Are you sure to delete this AI trigger?");

    int nResult = ::MessageBox(GetHandle(), pMessage, Translations::TranslateOrDefault("AITriggerDeleteTitle", "Delete AI Trigger"), MB_YESNO);

    if (nResult == IDYES)
    {
        int idx = SelectedAITriggerIndex;
        SendMessage(hSelectedAITrigger, CB_DELETESTRING, idx, NULL);
        if (idx >= SendMessage(hSelectedAITrigger, CB_GETCOUNT, NULL, NULL))
            idx--;
        if (idx < 0)
            idx = 0;
        SendMessage(hSelectedAITrigger, CB_SETCURSEL, idx, NULL);

        map.DeleteKey("AITriggerTypes", CurrentAITrigger->ID);
        map.DeleteKey("AITriggerTypesEnable", CurrentAITrigger->ID);

        CurrentAITrigger = nullptr;
        OnSelchangeAITrigger();
    }
}

void CNewAITrigger::OnCloseupCComboBox(HWND& hWnd, std::map<int, FString>& labels, bool isComboboxSelectOnly)
{
    if (!ExtraWindow::OnCloseupCComboBox(hWnd, labels, isComboboxSelectOnly))
    {
        if (hWnd == hSelectedAITrigger)
        {
            OnSelchangeAITrigger();
        }

    }
}

void CNewAITrigger::OnSeldropdownAITrigger(HWND& hWnd)
{
    if (Autodrop)
    {
        Autodrop = false;
        return;
    }
    if (!CurrentAITrigger)
        return;
    if (!DropNeedUpdate)
        return;

    DropNeedUpdate = false;

    SortAITriggers(CurrentAITrigger->ID);
}

void CNewAITrigger::SortAITriggers(FString id)
{
    while (SendMessage(hSelectedAITrigger, CB_DELETESTRING, 0, NULL) != CB_ERR);
    std::vector<FString> labels;
    if (auto pSection = map.GetSection("AITriggerTypes")) {
        for (auto& pair : pSection->GetEntities()) {
            labels.push_back(ExtraWindow::GetAITriggerDisplayName(pair.first));
        }
    }

    bool tmp = ExtConfigs::SortByLabelName;
    ExtConfigs::SortByLabelName = ExtConfigs::SortByLabelName_AITrigger;

    std::sort(labels.begin(), labels.end(), ExtraWindow::SortLabels);

    ExtConfigs::SortByLabelName = tmp;

    for (size_t i = 0; i < labels.size(); ++i) {
        SendMessage(hSelectedAITrigger, CB_INSERTSTRING, i, labels[i]);
    }
    if (id != "") {
        SelectedAITriggerIndex = SendMessage(hSelectedAITrigger, CB_FINDSTRINGEXACT, 0, (LPARAM)ExtraWindow::GetAITriggerDisplayName(id));
        SendMessage(hSelectedAITrigger, CB_SETCURSEL, SelectedAITriggerIndex, NULL);
    }
}

bool CNewAITrigger::OnEnterKeyDown(HWND& hWnd)
{
    if (hWnd == hSelectedAITrigger)
        OnSelchangeAITrigger(true);
    else if (hWnd == hTeam1)
        OnSelchangeTeam(0, true);
    else if (hWnd == hTeam2)
        OnSelchangeTeam(1, true);
    else if (hWnd == hCountry)
        OnSelchangeCountry(true);
    else if (hWnd == hComparisonObject)
        OnSelchangeComparisonObject(true);
    else
        return false;
    return true;
}