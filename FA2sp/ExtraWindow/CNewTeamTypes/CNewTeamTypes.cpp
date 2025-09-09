#include "CNewTeamTypes.h"
#include "../CNewTaskforce/CNewTaskforce.h"
#include "../CNewScript/CNewScript.h"
#include "../CNewTrigger/CNewTrigger.h"
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
#include "../CObjectSearch/CObjectSearch.h"
#include "../../Ext/CTileSetBrowserFrame/TabPages/TeamSort.h"
#include <numeric>
#include "../CSearhReference/CSearhReference.h"
#include <chrono>

HWND CNewTeamTypes::m_hwnd;
CFinalSunDlg* CNewTeamTypes::m_parent;
CINI& CNewTeamTypes::map = CINI::CurrentDocument;
MultimapHelper& CNewTeamTypes::rules = Variables::RulesMap;
HWND CNewTeamTypes::hSelectedTeam;
HWND CNewTeamTypes::hNewTeam;
HWND CNewTeamTypes::hDelTeam;
HWND CNewTeamTypes::hCloTeam;
HWND CNewTeamTypes::hName;
HWND CNewTeamTypes::hHouse;
HWND CNewTeamTypes::hTaskforce;
HWND CNewTeamTypes::hScript;
HWND CNewTeamTypes::hTag;
HWND CNewTeamTypes::hVeteranLevel;
HWND CNewTeamTypes::hPriority;
HWND CNewTeamTypes::hMax;
HWND CNewTeamTypes::hTechlevel;
HWND CNewTeamTypes::hTransportWaypoint;
HWND CNewTeamTypes::hGroup;
HWND CNewTeamTypes::hWaypoint;
HWND CNewTeamTypes::hMindControlDecision;
HWND CNewTeamTypes::hCheckBoxLoadable;
HWND CNewTeamTypes::hCheckBoxFull;
HWND CNewTeamTypes::hCheckBoxAnnoyance;
HWND CNewTeamTypes::hCheckBoxGuardSlower;
HWND CNewTeamTypes::hCheckBoxRecruiter;
HWND CNewTeamTypes::hCheckBoxAutoCreate;
HWND CNewTeamTypes::hCheckBoxPrebuild;
HWND CNewTeamTypes::hCheckBoxReinforce;
HWND CNewTeamTypes::hCheckBoxCargoPlane;
HWND CNewTeamTypes::hCheckBoxWhiner;
HWND CNewTeamTypes::hCheckBoxLooseRecruit;
HWND CNewTeamTypes::hCheckBoxAggressive;
HWND CNewTeamTypes::hCheckBoxSuicide;
HWND CNewTeamTypes::hCheckBoxOnTransOnly;
HWND CNewTeamTypes::hCheckBoxAvoidThreats;
HWND CNewTeamTypes::hCheckBoxIonImmune;
HWND CNewTeamTypes::hCheckBoxTransportsReturnOnUnload;
HWND CNewTeamTypes::hCheckBoxAreTeamMembersRecruitable;
HWND CNewTeamTypes::hCheckBoxIsBaseDefense;
HWND CNewTeamTypes::hCheckBoxOnlyTargetHouseEnemy;
HWND CNewTeamTypes::hSearchReference;
HWND CNewTeamTypes::hTurnToTaskforce;
HWND CNewTeamTypes::hTurnToScript;
HWND CNewTeamTypes::hTurnToTag;

int CNewTeamTypes::SelectedTeamIndex = -1;
FString CNewTeamTypes::CurrentTeamID;
std::map<int, FString> CNewTeamTypes::TaskForceLabels;
std::map<int, FString> CNewTeamTypes::TeamLabels;
std::map<int, FString> CNewTeamTypes::ScriptLabels;
std::map<int, FString> CNewTeamTypes::TagLabels;
std::map<int, FString> CNewTeamTypes::HouseLabels;
bool CNewTeamTypes::Autodrop;
bool CNewTeamTypes::WaypointAutodrop;
bool CNewTeamTypes::DropNeedUpdate;
std::vector<FString> CNewTeamTypes::mindControlDecisions;


void CNewTeamTypes::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(304),
        pWnd->GetSafeHwnd(),
        CNewTeamTypes::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CNewTeamTypes.\n");
        m_parent = NULL;
        return;
    }
}

void CNewTeamTypes::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("TeamTypesTitle", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };
    
	Translate(1110, "TeamTypesNewTeam");
	Translate(1111, "TeamTypesDelTeam");
	Translate(6001, "TeamTypesCloTeam");
	Translate(50200, "TeamTypesMainDesc");
	Translate(50201, "TeamTypesCurrentTeamLabel");
	Translate(50202, "TeamTypesSelectedTeam");
	Translate(50203, "TeamTypesLabelName");
	Translate(50204, "TeamTypesLabelHouse");
	Translate(50205, "TeamTypesLabelTaskforce");
	Translate(50206, "TeamTypesLabelScript");
	Translate(50207, "TeamTypesLabelTag");
	Translate(50208, "TeamTypesLabelVeteranLevel");
	Translate(50209, "TeamTypesLabelPriority");
	Translate(50210, "TeamTypesLabelMax");
	Translate(50211, "TeamTypesLabelTechlevel");
	Translate(1413 , "TeamTypesLabelTransportWaypoint");
	Translate(50212, "TeamTypesLabelGroup");
	Translate(50213, "TeamTypesLabelWaypoint");
	Translate(1446, "TeamTypesLabelMindControlDecision");
	Translate(1113, "TeamTypesCheckBoxLoadable");
	Translate(1114, "TeamTypesCheckBoxFull");
	Translate(1115, "TeamTypesCheckBoxAnnoyance");
	Translate(1116, "TeamTypesCheckBoxGuardSlower");
	Translate(1117, "TeamTypesCheckBoxRecruiter");
	Translate(1119, "TeamTypesCheckBoxAutoCreate");
	Translate(1120, "TeamTypesCheckBoxPrebuild");
	Translate(1127, "TeamTypesCheckBoxReinforce");
	Translate(1128, "TeamTypesCheckBoxCargoPlane");
	Translate(1129, "TeamTypesCheckBoxWhiner");
	Translate(1130, "TeamTypesCheckBoxLooseRecruit");
	Translate(1131, "TeamTypesCheckBoxAggressive");
	Translate(1132, "TeamTypesCheckBoxSuicide");
	Translate(1133, "TeamTypesCheckBoxOnTransOnly");
	Translate(1134, "TeamTypesCheckBoxAvoidThreats");
	Translate(1135, "TeamTypesCheckBoxIonImmune");
	Translate(1136, "TeamTypesCheckBoxTransportsReturnOnUnload");
	Translate(1137, "TeamTypesCheckBoxAreTeamMembersRecruitable");
	Translate(1138, "TeamTypesCheckBoxIsBaseDefense");
	Translate(1139, "TeamTypesCheckBoxOnlyTargetHouseEnemy"); 
    Translate(1999, "SearchReferenceTitle");

    hSelectedTeam = GetDlgItem(hWnd, Controls::SelectedTeam);
    hNewTeam = GetDlgItem(hWnd, Controls::NewTeam);
    hDelTeam = GetDlgItem(hWnd, Controls::DelTeam);
    hCloTeam = GetDlgItem(hWnd, Controls::CloTeam);
    hName = GetDlgItem(hWnd, Controls::Name);
    hHouse = GetDlgItem(hWnd, Controls::House);
    hTaskforce = GetDlgItem(hWnd, Controls::Taskforce);
    hScript = GetDlgItem(hWnd, Controls::Script);
    hTag = GetDlgItem(hWnd, Controls::Tag);
    hVeteranLevel = GetDlgItem(hWnd, Controls::VeteranLevel);
    hPriority = GetDlgItem(hWnd, Controls::Priority);
    hMax = GetDlgItem(hWnd, Controls::Max);
    hTechlevel = GetDlgItem(hWnd, Controls::Techlevel);
    hTransportWaypoint = GetDlgItem(hWnd, Controls::TransportWaypoint);
    hGroup = GetDlgItem(hWnd, Controls::Group);
    hWaypoint = GetDlgItem(hWnd, Controls::Waypoint);
    hMindControlDecision = GetDlgItem(hWnd, Controls::MindControlDecision);
    hCheckBoxLoadable = GetDlgItem(hWnd, Controls::CheckBoxLoadable);
    hCheckBoxFull = GetDlgItem(hWnd, Controls::CheckBoxFull);
    hCheckBoxAnnoyance = GetDlgItem(hWnd, Controls::CheckBoxAnnoyance);
    hCheckBoxGuardSlower = GetDlgItem(hWnd, Controls::CheckBoxGuardSlower);
    hCheckBoxRecruiter = GetDlgItem(hWnd, Controls::CheckBoxRecruiter);
    hCheckBoxAutoCreate = GetDlgItem(hWnd, Controls::CheckBoxAutoCreate);
    hCheckBoxPrebuild = GetDlgItem(hWnd, Controls::CheckBoxPrebuild);
    hCheckBoxReinforce = GetDlgItem(hWnd, Controls::CheckBoxReinforce);
    hCheckBoxCargoPlane = GetDlgItem(hWnd, Controls::CheckBoxCargoPlane);
    hCheckBoxWhiner = GetDlgItem(hWnd, Controls::CheckBoxWhiner);
    hCheckBoxLooseRecruit = GetDlgItem(hWnd, Controls::CheckBoxLooseRecruit);
    hCheckBoxAggressive = GetDlgItem(hWnd, Controls::CheckBoxAggressive);
    hCheckBoxSuicide = GetDlgItem(hWnd, Controls::CheckBoxSuicide);
    hCheckBoxOnTransOnly = GetDlgItem(hWnd, Controls::CheckBoxOnTransOnly);
    hCheckBoxAvoidThreats = GetDlgItem(hWnd, Controls::CheckBoxAvoidThreats);
    hCheckBoxIonImmune = GetDlgItem(hWnd, Controls::CheckBoxIonImmune);
    hCheckBoxTransportsReturnOnUnload = GetDlgItem(hWnd, Controls::CheckBoxTransportsReturnOnUnload);
    hCheckBoxAreTeamMembersRecruitable = GetDlgItem(hWnd, Controls::CheckBoxAreTeamMembersRecruitable);
    hCheckBoxIsBaseDefense = GetDlgItem(hWnd, Controls::CheckBoxIsBaseDefense);
    hCheckBoxOnlyTargetHouseEnemy = GetDlgItem(hWnd, Controls::CheckBoxOnlyTargetHouseEnemy);
    hSearchReference = GetDlgItem(hWnd, Controls::SearchReference);
    hTurnToTaskforce = GetDlgItem(hWnd, Controls::TurnToTaskforce);
    hTurnToScript = GetDlgItem(hWnd, Controls::TurnToScript);
    hTurnToTag = GetDlgItem(hWnd, Controls::TurnToTag);

    mindControlDecisions.clear();
    mindControlDecisions.push_back(FString("0 - ") + Translations::TranslateOrDefault("MindControlDecisions.0", "Don't use this logic"));
    mindControlDecisions.push_back(FString("1 - ") + Translations::TranslateOrDefault("MindControlDecisions.1", "Recruit"));
    mindControlDecisions.push_back(FString("2 - ") + Translations::TranslateOrDefault("MindControlDecisions.2", "Send to Grinder"));
    mindControlDecisions.push_back(FString("3 - ") + Translations::TranslateOrDefault("MindControlDecisions.3", "Send to Bio Reactor"));
    mindControlDecisions.push_back(FString("4 - ") + Translations::TranslateOrDefault("MindControlDecisions.4", "Assign to hunt"));
    mindControlDecisions.push_back(FString("5 - ") + Translations::TranslateOrDefault("MindControlDecisions.5", "Do nothing"));

    Update(hWnd);
}

void CNewTeamTypes::Update(HWND& hWnd)
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    if (TeamSort::Instance.IsVisible())
        TeamSort::Instance.LoadAllTriggers();
    DropNeedUpdate = false;
    int idx = 0;

    ExtraWindow::SortTeams(hSelectedTeam, "TeamTypes", SelectedTeamIndex);
    int count = SendMessage(hSelectedTeam, CB_GETCOUNT, NULL, NULL);
    if (SelectedTeamIndex < 0)
        SelectedTeamIndex = 0;
    if (SelectedTeamIndex > count - 1)
        SelectedTeamIndex = count - 1;
    SendMessage(hSelectedTeam, CB_SETCURSEL, SelectedTeamIndex, NULL);


    idx = 0;
    while (SendMessage(hHouse, CB_DELETESTRING, 0, NULL) != CB_ERR);
    auto&& entries = rules.ParseIndicies("Countries", true);
    if (CMapData::Instance->IsMultiOnly())
    {
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"<Player @ A>");
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"<Player @ B>");
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"<Player @ C>");
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"<Player @ D>");
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"<Player @ E>");
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"<Player @ F>");
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"<Player @ G>");
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"<Player @ H>");
    }
    for (size_t i = 0, sz = entries.size(); i < sz; ++i)
    {
        auto& country = entries[i];
        if (country == "GDI" || country == "Nod")
            continue;
        
        SendMessage(hHouse, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)Translations::ParseHouseName(country, true));
    }

    int tmp = 0;
    ExtraWindow::SortTeams(hTaskforce, "TaskForces", tmp);
    ExtraWindow::SortTeams(hScript, "ScriptTypes", tmp);

    idx = 0;
    while (SendMessage(hTag, CB_DELETESTRING, 0, NULL) != CB_ERR);
    std::vector<FString> labels;
    if (auto pSection = map.GetSection("Tags")) {
        for (auto& pair : pSection->GetEntities()) {
            auto atoms = FString::SplitString(pair.second, 1);
            FString name;
            name.Format("%s (%s)", pair.first, atoms[1]);
            labels.push_back(name);
        }
    }
    std::sort(labels.begin(), labels.end(), ExtraWindow::SortLabels);
    SendMessage(hTag, CB_INSERTSTRING, 0, (LPARAM)(LPCSTR)"None");
    for (size_t i = 0; i < labels.size(); ++i) {
        SendMessage(hTag, CB_INSERTSTRING, i+1, (LPARAM)(LPCSTR)labels[i]);
    }

    idx = 0;
    while (SendMessage(hVeteranLevel, CB_DELETESTRING, 0, NULL) != CB_ERR);
    SendMessage(hVeteranLevel, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"1");
    SendMessage(hVeteranLevel, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"2");
    SendMessage(hVeteranLevel, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"3");

    idx = 0;
    while (SendMessage(hTechlevel, CB_DELETESTRING, 0, NULL) != CB_ERR);
    SendMessage(hTechlevel, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"0");
    SendMessage(hTechlevel, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"1");
    SendMessage(hTechlevel, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"2");
    SendMessage(hTechlevel, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"3");
    SendMessage(hTechlevel, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"4");
    SendMessage(hTechlevel, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"5");
    SendMessage(hTechlevel, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"6");
    SendMessage(hTechlevel, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"7");
    SendMessage(hTechlevel, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"8");
    SendMessage(hTechlevel, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"9");
    SendMessage(hTechlevel, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"10");

    idx = 0;
    while (SendMessage(hWaypoint, CB_DELETESTRING, 0, NULL) != CB_ERR);
    while (SendMessage(hTransportWaypoint, CB_DELETESTRING, 0, NULL) != CB_ERR);
    SendMessage(hTransportWaypoint, CB_INSERTSTRING, idx, (LPARAM)(LPCSTR)"None");

    if (auto pSection = CINI::CurrentDocument->GetSection("Waypoints"))
    {
        for (auto& pair : pSection->GetEntities())
        {
            SendMessage(hWaypoint, CB_INSERTSTRING, idx, (LPARAM)(LPCSTR)pair.first.m_pchData);
            idx++;
            SendMessage(hTransportWaypoint, CB_INSERTSTRING, idx, (LPARAM)(LPCSTR)pair.first.m_pchData);
        }
    }

    idx = 0;
    while (SendMessage(hGroup, CB_DELETESTRING, 0, NULL) != CB_ERR);
    SendMessage(hGroup, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)"-1");

    idx = 0;
    while (SendMessage(hMindControlDecision, CB_DELETESTRING, 0, NULL) != CB_ERR);
    for (auto& decision : mindControlDecisions)
        SendMessage(hMindControlDecision, CB_INSERTSTRING, idx++, (LPARAM)(LPCSTR)decision);

    Autodrop = false;
    ExtraWindow::AdjustDropdownWidth(hSelectedTeam);
    OnSelchangeTeamtypes();
}

void CNewTeamTypes::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CNewTeamTypes::m_hwnd = NULL;
    CNewTeamTypes::m_parent = NULL;

}

BOOL CALLBACK CNewTeamTypes::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CNewTeamTypes::Initialize(hWnd);
        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::NewTeam:
            if (CODE == BN_CLICKED)
                OnClickNewTeam();
            break;
        case Controls::DelTeam:
            if (CODE == BN_CLICKED)
                OnClickDelTeam(hWnd);
            break;
        case Controls::CloTeam:
            if (CODE == BN_CLICKED)
                OnClickCloTeam(hWnd);
            break;
        case Controls::TurnToTaskforce:
            if (CODE == BN_CLICKED)
                OnClickTurnToTaskforce();
            break;
        case Controls::TurnToScript:
            if (CODE == BN_CLICKED)
                OnClickTurnToScript();
            break;
        case Controls::TurnToTag:
            if (CODE == BN_CLICKED)
                OnClickTurnToTag();
            break;
        case Controls::SelectedTeam:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTeamtypes();
            else if (CODE == CBN_DROPDOWN)
                OnSeldropdownTeamtypes(hWnd);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeTeamtypes(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupTeamtypes();
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::Name:
            if (CODE == EN_CHANGE)
            {
                if (SelectedTeamIndex < 0)
                    break;
                char buffer[512]{ 0 };
                GetWindowText(hName, buffer, 511);
                map.WriteString(CurrentTeamID, "Name", buffer);

                DropNeedUpdate = true;

                FString name;
                name.Format("%s (%s)", CurrentTeamID, buffer);
                SendMessage(hSelectedTeam, CB_DELETESTRING, SelectedTeamIndex, NULL);
                SendMessage(hSelectedTeam, CB_INSERTSTRING, SelectedTeamIndex, (LPARAM)(LPCSTR)name);
                SendMessage(hSelectedTeam, CB_SETCURSEL, SelectedTeamIndex, NULL);

                ExtraWindow::AdjustDropdownWidth(hSelectedTeam);
            }
            break;
        case Controls::House:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeHouse();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeHouse(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupHouse();
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::Taskforce:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTaskForce();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeTaskForce(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupTaskForce();
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::Script:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeScript();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeScript(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupScript();
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::Tag:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTag();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeTag(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupTag();
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::TransportWaypoint:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTransportWaypoint(hWnd);
            else if (CODE == CBN_EDITCHANGE || CODE == CBN_CLOSEUP)
                OnSelchangeTransportWaypoint(hWnd, true);
            break;
        case Controls::Waypoint :
            if (CODE == CBN_SELCHANGE)
                OnSelchangeWaypoint(hWnd);
            else if (CODE == CBN_EDITCHANGE || CODE == CBN_CLOSEUP)
                OnSelchangeWaypoint(hWnd, true);
            break;
        case Controls::VeteranLevel:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeVeteranLevel(hWnd);
            else if (CODE == CBN_EDITCHANGE || CODE == CBN_CLOSEUP)
                OnSelchangeVeteranLevel(hWnd, true);
            break;
        case Controls::Techlevel:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTechlevel(hWnd);
            else if (CODE == CBN_EDITCHANGE || CODE == CBN_CLOSEUP)
                OnSelchangeTechlevel(hWnd, true);
            break;
        case Controls::MindControlDecision:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeMindControlDecision(hWnd);
            else if (CODE == CBN_EDITCHANGE || CODE == CBN_CLOSEUP)
                OnSelchangeMindControlDecision(hWnd, true);
            break;
        case Controls::Group:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeGroup(hWnd);
            else if (CODE == CBN_EDITCHANGE || CODE == CBN_CLOSEUP)
                OnSelchangeGroup(hWnd, true);
            break;
        case Controls::Priority:
            if (CODE == EN_CHANGE)
            {
                if (SelectedTeamIndex < 0)
                    break;
                char buffer[512]{ 0 };
                GetWindowText(hPriority, buffer, 511);
                map.WriteString(CurrentTeamID, "Priority", buffer);
            }
            break;
        case Controls::Max:
            if (CODE == EN_CHANGE)
            {
                if (SelectedTeamIndex < 0)
                    break;
                char buffer[512]{ 0 };
                GetWindowText(hMax, buffer, 511);
                map.WriteString(CurrentTeamID, "Max", buffer);
            }
            break;
        case Controls::SearchReference:
            if (CODE == BN_CLICKED)
                OnClickSearchReference(hWnd);
            break;
#if true
        case Controls::CheckBoxLoadable:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "Loadable", SendMessage(hCheckBoxLoadable, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxFull:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "Full", SendMessage(hCheckBoxFull, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxAnnoyance:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "Annoyance", SendMessage(hCheckBoxAnnoyance, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxGuardSlower:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "GuardSlower", SendMessage(hCheckBoxGuardSlower, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxRecruiter:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "Recruiter", SendMessage(hCheckBoxRecruiter, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxAutoCreate:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "Autocreate", SendMessage(hCheckBoxAutoCreate, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxPrebuild:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "Prebuild", SendMessage(hCheckBoxPrebuild, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxReinforce:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "Reinforce", SendMessage(hCheckBoxReinforce, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxCargoPlane:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "Droppod", SendMessage(hCheckBoxCargoPlane, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxWhiner:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "Whiner", SendMessage(hCheckBoxWhiner, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxLooseRecruit:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "LooseRecruit", SendMessage(hCheckBoxLooseRecruit, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxAggressive:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "Aggressive", SendMessage(hCheckBoxAggressive, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxSuicide:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "Suicide", SendMessage(hCheckBoxSuicide, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxOnTransOnly:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "OnTransOnly", SendMessage(hCheckBoxOnTransOnly, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxAvoidThreats:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "AvoidThreats", SendMessage(hCheckBoxAvoidThreats, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxIonImmune:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "IonImmune", SendMessage(hCheckBoxIonImmune, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxTransportsReturnOnUnload:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "TransportsReturnOnUnload", SendMessage(hCheckBoxTransportsReturnOnUnload, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxAreTeamMembersRecruitable:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "AreTeamMembersRecruitable", SendMessage(hCheckBoxAreTeamMembersRecruitable, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxIsBaseDefense:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "IsBaseDefense", SendMessage(hCheckBoxIsBaseDefense, BM_GETCHECK, 0, 0));
            break;
        case Controls::CheckBoxOnlyTargetHouseEnemy:
            if (CODE == BN_CLICKED && SelectedTeamIndex >= 0)
                map.WriteBool(CurrentTeamID, "OnlyTargetHouseEnemy", SendMessage(hCheckBoxOnlyTargetHouseEnemy, BM_GETCHECK, 0, 0));
            break;
#endif
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CNewTeamTypes::Close(hWnd);
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

void CNewTeamTypes::OnSelchangeTransportWaypoint(HWND& hWnd, bool edited)
{
    if (SelectedTeamIndex < 0)
        return;
    char buffer[512]{0};
    SendMessage(hTransportWaypoint,
        CB_GETLBTEXT,
        SendMessage(hTransportWaypoint, CB_GETCURSEL, NULL, NULL),
        (LPARAM)buffer);
    if (edited)
        GetWindowText(hTransportWaypoint, buffer, 511);
    FString text = buffer;
    text.Trim();
    if (text == "None")
    {
        map.WriteBool(CurrentTeamID, "UseTransportOrigin", false);
        map.DeleteKey(CurrentTeamID, "TransportWaypoint");
        return;
    }
    map.WriteBool(CurrentTeamID, "UseTransportOrigin", true);
    map.WriteString(CurrentTeamID, "TransportWaypoint", STDHelpers::WaypointToString(text));
}

void CNewTeamTypes::OnSelchangeWaypoint(HWND& hWnd, bool edited)
{
    if (SelectedTeamIndex < 0)
        return;
    char buffer[512]{0};
    SendMessage(hWaypoint,
        CB_GETLBTEXT,
        SendMessage(hWaypoint, CB_GETCURSEL, NULL, NULL),
        (LPARAM)buffer);
    if (edited)
        GetWindowText(hWaypoint, buffer, 511);
    FString text = buffer;
    text.Trim();

    map.WriteString(CurrentTeamID, "Waypoint", STDHelpers::WaypointToString(text));
}

void CNewTeamTypes::OnSelchangeTaskForce(bool edited)
{
    if (SelectedTeamIndex < 0)
        return;
    int curSel = SendMessage(hTaskforce, CB_GETCURSEL, NULL, NULL);
    FString text;
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hTaskforce, CB_GETCOUNT, NULL, NULL) > 0 || !TaskForceLabels.empty()))
    {
        ExtraWindow::OnEditCComboBox(hTaskforce, TaskForceLabels);
    }

    if (curSel >= 0 && curSel < SendMessage(hTaskforce, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hTaskforce, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hTaskforce, buffer, 511);
        text = buffer;
    }

    if (!text)
        return;

    FString::TrimIndex(text);
    if (text == "None")
        text = "";

    map.WriteString(CurrentTeamID, "TaskForce", text);    
}

void CNewTeamTypes::OnCloseupTaskForce()
{
    if (!ExtraWindow::OnCloseupCComboBox(hTaskforce, TaskForceLabels))
    {
        OnSelchangeTeamtypes();
    }
}

void CNewTeamTypes::OnSelchangeScript(bool edited)
{
    if (SelectedTeamIndex < 0)
        return;
    int curSel = SendMessage(hScript, CB_GETCURSEL, NULL, NULL);
    FString text;
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hScript, CB_GETCOUNT, NULL, NULL) > 0 || !ScriptLabels.empty()))
    {
        ExtraWindow::OnEditCComboBox(hScript, ScriptLabels);
    }
    if (curSel >= 0 && curSel < SendMessage(hScript, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hScript, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hScript, buffer, 511);
        text = buffer;
    }
    if (!text)
        return;

    FString::TrimIndex(text);
    if (text == "None")
        text = "";

    map.WriteString(CurrentTeamID, "Script", text);
}

void CNewTeamTypes::OnCloseupScript()
{
    if (!ExtraWindow::OnCloseupCComboBox(hScript, ScriptLabels))
    {
        OnSelchangeTeamtypes();
    }
}

void CNewTeamTypes::OnSelchangeTag(bool edited)
{
    if (SelectedTeamIndex < 0)
        return;
    int curSel = SendMessage(hTag, CB_GETCURSEL, NULL, NULL);
    FString text;
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hTag, CB_GETCOUNT, NULL, NULL) > 0 || !TagLabels.empty()))
    {
        ExtraWindow::OnEditCComboBox(hTag, TagLabels);
    }
    if (curSel >= 0 && curSel < SendMessage(hTag, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hTag, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hTag, buffer, 511);
        text = buffer;
    }

    FString::TrimIndex(text);
    if (text == "None")
        text = "";

    if (text == "")
        map.DeleteKey(CurrentTeamID, "Tag");
    else
        map.WriteString(CurrentTeamID, "Tag", text);
}

void CNewTeamTypes::OnCloseupTag()
{
    if (!ExtraWindow::OnCloseupCComboBox(hTag, TagLabels))
    {
        OnSelchangeTeamtypes();
    }
}


void CNewTeamTypes::OnSelchangeHouse(bool edited)
{
    if (SelectedTeamIndex < 0)
        return;
    int curSel = SendMessage(hHouse, CB_GETCURSEL, NULL, NULL);
    int count = SendMessage(hHouse, CB_GETCOUNT, NULL, NULL);
    FString text;
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hHouse, CB_GETCOUNT, NULL, NULL) > 0 || !HouseLabels.empty()))
    {
        ExtraWindow::OnEditCComboBox(hHouse, HouseLabels);
    }
    if (curSel >= 0 && curSel < count)
    {
        SendMessage(hHouse, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hHouse, buffer, 511);
        text = buffer;
        int idx = SendMessage(hHouse, CB_FINDSTRINGEXACT, 0, (LPARAM)Translations::ParseHouseName(text, true));
        if (idx != CB_ERR)
        {
            SendMessage(hHouse, CB_GETLBTEXT, idx, (LPARAM)buffer);
            text = buffer;
        }
    }
    if (!text)
        return;

    text.Trim();
    map.WriteString(CurrentTeamID, "House", Translations::ParseHouseName(text, false));
}

void CNewTeamTypes::OnCloseupHouse()
{
    if (!ExtraWindow::OnCloseupCComboBox(hHouse, HouseLabels))
    {
        OnSelchangeTeamtypes();
    }
}

void CNewTeamTypes::OnSelchangeVeteranLevel(HWND& hWnd, bool edited)
{
    if (SelectedTeamIndex < 0)
        return;
    int curSel = SendMessage(hVeteranLevel, CB_GETCURSEL, NULL, NULL);
    int count = SendMessage(hVeteranLevel, CB_GETCOUNT, NULL, NULL);
    char buffer[512]{ 0 };
    FString text;
    if (curSel >= 0 && curSel < count)
    {
        SendMessage(hVeteranLevel, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hVeteranLevel, buffer, 511);
        text = buffer;
        int idx = SendMessage(hVeteranLevel, CB_FINDSTRINGEXACT, 0, (LPARAM)text);
        if (idx != CB_ERR)
        {
            SendMessage(hVeteranLevel, CB_GETLBTEXT, idx, (LPARAM)buffer);
            text = buffer;
        }
    }
    if (!text)
        return;

    text.Trim();
    map.WriteString(CurrentTeamID, "VeteranLevel", text);
}

void CNewTeamTypes::OnSelchangeTechlevel(HWND& hWnd, bool edited)
{
    if (SelectedTeamIndex < 0)
        return;
    int curSel = SendMessage(hTechlevel, CB_GETCURSEL, NULL, NULL);
    int count = SendMessage(hTechlevel, CB_GETCOUNT, NULL, NULL);
    char buffer[512]{ 0 };
    FString text;
    if (curSel >= 0 && curSel < count)
    {
        SendMessage(hTechlevel, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hTechlevel, buffer, 511);
        text = buffer;
        int idx = SendMessage(hTechlevel, CB_FINDSTRINGEXACT, 0, (LPARAM)text);
        if (idx != CB_ERR)
        {
            SendMessage(hTechlevel, CB_GETLBTEXT, idx, (LPARAM)buffer);
            text = buffer;
        }
    }
    if (!text)
        return;

    text.Trim();
    map.WriteString(CurrentTeamID, "TechLevel", text);
}

void CNewTeamTypes::OnSelchangeGroup(HWND& hWnd, bool edited)
{
    if (SelectedTeamIndex < 0)
        return;
    int curSel = SendMessage(hGroup, CB_GETCURSEL, NULL, NULL);
    int count = SendMessage(hGroup, CB_GETCOUNT, NULL, NULL);
    char buffer[512]{ 0 };
    FString text;
    if (curSel >= 0 && curSel < count)
    {
        SendMessage(hGroup, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hGroup, buffer, 511);
        text = buffer;
        int idx = SendMessage(hGroup, CB_FINDSTRINGEXACT, 0, (LPARAM)text);
        if (idx != CB_ERR)
        {
            SendMessage(hGroup, CB_GETLBTEXT, idx, (LPARAM)buffer);
            text = buffer;
        }
    }
    if (!text)
        return;

    text.Trim();
    map.WriteString(CurrentTeamID, "Group", text);
}

void CNewTeamTypes::OnSelchangeMindControlDecision(HWND& hWnd, bool edited)
{
    if (SelectedTeamIndex < 0)
        return;
    int curSel = SendMessage(hMindControlDecision, CB_GETCURSEL, NULL, NULL);
    int count = SendMessage(hMindControlDecision, CB_GETCOUNT, NULL, NULL);
    char buffer[512]{ 0 };
    FString text;
    if (curSel >= 0 && curSel < count)
    {
        SendMessage(hMindControlDecision, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hMindControlDecision, buffer, 511);
        text = buffer;
        int idx = SendMessage(hMindControlDecision, CB_FINDSTRING, 0, (LPARAM)text);
        if (idx != CB_ERR)
        {
            SendMessage(hMindControlDecision, CB_GETLBTEXT, idx, (LPARAM)buffer);
            text = buffer;
        }
    }
    if (!text)
        return;

    int idx = 0;
    for (auto& decision : mindControlDecisions)
    {
        if (text == decision)
        {
            text.Format("%d", idx);
            break;
        }
        idx++;
    }

    text.Trim();
    map.WriteString(CurrentTeamID, "MindControlDecision", text);
}

void CNewTeamTypes::OnSeldropdownTeamtypes(HWND& hWnd)
{
    if (Autodrop)
    {
        Autodrop = false;
        return;
    }

    if (!DropNeedUpdate)
        return;

    DropNeedUpdate = false;

    ExtraWindow::SortTeams(hSelectedTeam, "TeamTypes", SelectedTeamIndex, CurrentTeamID);
}

void CNewTeamTypes::OnSelchangeTeamtypes(bool edited)
{
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hSelectedTeam, CB_GETCOUNT, NULL, NULL) > 0 || !TeamLabels.empty()))
    {
        Autodrop = true;
        ExtraWindow::OnEditCComboBox(hSelectedTeam, TeamLabels);
        return;
    }

    SelectedTeamIndex = SendMessage(hSelectedTeam, CB_GETCURSEL, NULL, NULL);
    if (SelectedTeamIndex < 0 || SelectedTeamIndex >= SendMessage(hSelectedTeam, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hHouse, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hTaskforce, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hScript, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hTag, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hVeteranLevel, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hPriority, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hMax, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hTechlevel, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hTransportWaypoint, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hGroup, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hWaypoint, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hMindControlDecision, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hCheckBoxLoadable, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxFull, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxAnnoyance, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxGuardSlower, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxRecruiter, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxAutoCreate, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxPrebuild, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxReinforce, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxCargoPlane, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxWhiner, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxLooseRecruit, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxAggressive, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxSuicide, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxOnTransOnly, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxAvoidThreats, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxIonImmune, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxTransportsReturnOnUnload, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxAreTeamMembersRecruitable, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxIsBaseDefense, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hCheckBoxOnlyTargetHouseEnemy, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hSearchReference, BM_SETCHECK, BST_UNCHECKED, 0);
        return;
    }

    FString pID;
    SendMessage(hSelectedTeam, CB_GETLBTEXT, SelectedTeamIndex, (LPARAM)buffer);
    pID = buffer;
    FString::TrimIndex(pID);

    CurrentTeamID = pID;
    if (auto pTeam = map.GetSection(pID))
    {
        auto name = map.GetString(pID, "Name");
        auto house = map.GetString(pID, "House");
        auto taskforce = map.GetString(pID, "TaskForce");
        auto script = map.GetString(pID, "Script");
        auto tag = map.GetString(pID, "Tag");
        auto tWaypoint = STDHelpers::StringToWaypointStr(map.GetString(pID, "TransportWaypoint"));
        auto waypoint = STDHelpers::StringToWaypointStr(map.GetString(pID, "Waypoint"));


        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)name.m_pchData);

        int houseidx = SendMessage(hHouse, CB_FINDSTRINGEXACT, 0, (LPARAM)Translations::ParseHouseName(house, true));
        if (houseidx != CB_ERR)
            SendMessage(hHouse, CB_SETCURSEL, houseidx, NULL);
        else
            SendMessage(hHouse, WM_SETTEXT, 0, (LPARAM)house.m_pchData);

        bool found = false;
        for (int idx = 0; idx < SendMessage(hTaskforce, CB_GETCOUNT, NULL, NULL); idx++)
        {
            FString id;
            SendMessage(hTaskforce, CB_GETLBTEXT, idx, (LPARAM)buffer);
            id = buffer;
            FString::TrimIndex(id);
            if (id == taskforce)
            {
                SendMessage(hTaskforce, CB_SETCURSEL, idx, NULL);
                found = true;
                break;
            }
        }
        if (taskforce == "")
            taskforce = "None";
        if (!found)
            SendMessage(hTaskforce, WM_SETTEXT, 0, (LPARAM)taskforce.m_pchData);

        found = false;
        for (int idx = 0; idx < SendMessage(hScript, CB_GETCOUNT, NULL, NULL); idx++)
        {
            FString id;
            SendMessage(hScript, CB_GETLBTEXT, idx, (LPARAM)buffer);
            id = buffer;
            FString::TrimIndex(id);
            if (id == script)
            {
                SendMessage(hScript, CB_SETCURSEL, idx, NULL);
                found = true;
                break;
            }
        }
        if (script == "")
            script = "None";
        if (!found)
            SendMessage(hScript, WM_SETTEXT, 0, (LPARAM)script.m_pchData);

        found = false;
        for (int idx = 0; idx < SendMessage(hTag, CB_GETCOUNT, NULL, NULL); idx++)
        {
            FString id;
            SendMessage(hTag, CB_GETLBTEXT, idx, (LPARAM)buffer);
            id = buffer;
            FString::TrimIndex(id);
            if (id == tag)
            {
                SendMessage(hTag, CB_SETCURSEL, idx, NULL);
                found = true;
                break;
            }
        }
        if (tag == "")
            tag = "None";
        if (!found)
            SendMessage(hTag, WM_SETTEXT, 0, (LPARAM)tag.m_pchData);

        SendMessage(hVeteranLevel, WM_SETTEXT, 0, (LPARAM)map.GetString(pID, "VeteranLevel").m_pchData);
        SendMessage(hTechlevel, WM_SETTEXT, 0, (LPARAM)map.GetString(pID, "TechLevel").m_pchData);
        SendMessage(hTransportWaypoint, WM_SETTEXT, 0, (LPARAM)tWaypoint.m_pchData);
        SendMessage(hWaypoint, WM_SETTEXT, 0, (LPARAM)waypoint.m_pchData);
        SendMessage(hPriority, WM_SETTEXT, 0, (LPARAM)map.GetString(pID, "Priority").m_pchData);
        SendMessage(hMax, WM_SETTEXT, 0, (LPARAM)map.GetString(pID, "Max").m_pchData);
        SendMessage(hGroup, WM_SETTEXT, 0, (LPARAM)map.GetString(pID, "Group").m_pchData);
        int idxMCD = SendMessage(hMindControlDecision, CB_FINDSTRING, 0, (LPARAM)map.GetString(pID, "MindControlDecision").m_pchData);
        if (idxMCD != CB_ERR)
            SendMessage(hMindControlDecision, CB_SETCURSEL, idxMCD, NULL);
        else
            SendMessage(hMindControlDecision, WM_SETTEXT, 0, (LPARAM)map.GetString(pID, "MindControlDecision").m_pchData);

        SendMessage(hCheckBoxLoadable, BM_SETCHECK, map.GetBool(pID, "Loadable"), 0);
        SendMessage(hCheckBoxFull, BM_SETCHECK, map.GetBool(pID, "Full"), 0);
        SendMessage(hCheckBoxAnnoyance, BM_SETCHECK, map.GetBool(pID, "Annoyance"), 0);
        SendMessage(hCheckBoxGuardSlower, BM_SETCHECK, map.GetBool(pID, "GuardSlower"), 0);
        SendMessage(hCheckBoxRecruiter, BM_SETCHECK, map.GetBool(pID, "Recruiter"), 0);
        SendMessage(hCheckBoxAutoCreate, BM_SETCHECK, map.GetBool(pID, "Autocreate"), 0);
        SendMessage(hCheckBoxPrebuild, BM_SETCHECK, map.GetBool(pID, "Prebuild"), 0);
        SendMessage(hCheckBoxReinforce, BM_SETCHECK, map.GetBool(pID, "Reinforce"), 0);
        SendMessage(hCheckBoxCargoPlane, BM_SETCHECK, map.GetBool(pID, "Droppod"), 0);
        SendMessage(hCheckBoxWhiner, BM_SETCHECK, map.GetBool(pID, "Whiner"), 0);
        SendMessage(hCheckBoxLooseRecruit, BM_SETCHECK, map.GetBool(pID, "LooseRecruit"), 0);
        SendMessage(hCheckBoxAggressive, BM_SETCHECK, map.GetBool(pID, "Aggressive"), 0);
        SendMessage(hCheckBoxSuicide, BM_SETCHECK, map.GetBool(pID, "Suicide"), 0);
        SendMessage(hCheckBoxOnTransOnly, BM_SETCHECK, map.GetBool(pID, "OnTransOnly"), 0);
        SendMessage(hCheckBoxAvoidThreats, BM_SETCHECK, map.GetBool(pID, "AvoidThreats"), 0);
        SendMessage(hCheckBoxIonImmune, BM_SETCHECK, map.GetBool(pID, "IonImmune"), 0);
        SendMessage(hCheckBoxTransportsReturnOnUnload, BM_SETCHECK, map.GetBool(pID, "TransportsReturnOnUnload"), 0);
        SendMessage(hCheckBoxAreTeamMembersRecruitable, BM_SETCHECK, map.GetBool(pID, "AreTeamMembersRecruitable"), 0);
        SendMessage(hCheckBoxIsBaseDefense, BM_SETCHECK, map.GetBool(pID, "IsBaseDefense"), 0);
        SendMessage(hCheckBoxOnlyTargetHouseEnemy, BM_SETCHECK, map.GetBool(pID, "OnlyTargetHouseEnemy"), 0);

    }
    DropNeedUpdate = false;
}

void CNewTeamTypes::OnCloseupTeamtypes()
{
    if (!ExtraWindow::OnCloseupCComboBox(hSelectedTeam, TeamLabels, true))
    {
        OnSelchangeTeamtypes();
    }
}

void CNewTeamTypes::OnClickTurnToTaskforce()
{
    if (SelectedTeamIndex < 0)
        return;

    char buffer[512]{ 0 };
    GetWindowText(hTaskforce, buffer, 511);
    if (strcmp(buffer, "None") == 0)
        return;

    if (CNewTaskforce::GetHandle() == NULL)
        CNewTaskforce::Create(m_parent);

    auto dlg = GetDlgItem(CNewTaskforce::GetHandle(), CNewTaskforce::Controls::SelectedTaskforce);
    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)(LPCSTR)buffer);
    if (idx == CB_ERR)
        return;
    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
    CNewTaskforce::OnSelchangeTaskforce();
    SetWindowPos(CNewTaskforce::GetHandle(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

void CNewTeamTypes::OnClickTurnToScript()
{
    if (SelectedTeamIndex < 0)
        return;
    
    char buffer[512]{ 0 };
    GetWindowText(hScript, buffer, 511);
    if (strcmp(buffer, "None") == 0)
        return;

    if (CNewScript::GetHandle() == NULL)
        CNewScript::Create(m_parent);

    auto dlg = GetDlgItem(CNewScript::GetHandle(), CNewScript::Controls::SelectedScript);
    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, (LPARAM)(LPCSTR)buffer);
    if (idx == CB_ERR)
        return;
    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
    CNewScript::OnSelchangeScript();
    SetWindowPos(CNewScript::GetHandle(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

void CNewTeamTypes::OnClickTurnToTag()
{
    if (SelectedTeamIndex < 0)
        return;

    char buffer[512]{ 0 };
    FString text;
    GetWindowText(hTag, buffer, 511);
    text = buffer;

    if (text == "None")
        return;

    FString::TrimIndex(text);

    // actually turn to first trigger with this tag, since tag is not important
    if (CNewTrigger::GetHandle() == NULL)
        CNewTrigger::Create(m_parent);

    for (const auto& [ID, trigger] : CMapDataExt::Triggers)
    {
        if (trigger->Tag == text)
        {
            text = trigger->ID;
            break;
        }
    }

    auto dlg = GetDlgItem(CNewTrigger::GetHandle(), CNewTrigger::Controls::SelectedTrigger);
    auto idx = SendMessage(dlg, CB_FINDSTRINGEXACT, 0, ExtraWindow::GetTriggerDisplayName(text));
    if (idx == CB_ERR)
        return;
    SendMessage(dlg, CB_SETCURSEL, idx, NULL);
    CNewTrigger::OnSelchangeTrigger();
    SetWindowPos(CNewTrigger::GetHandle(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

void CNewTeamTypes::OnClickNewTeam()
{
    FString key = CINI::GetAvailableKey("TeamTypes");
    FString value = CMapDataExt::GetAvailableIndex();
    char buffer[512];
    FString buffer2;

    FString newName = "";
    if (TeamSort::CreateFromTeamSort)
        newName = TeamSort::Instance.GetCurrentPrefix();
    newName += "New Teamtype";
    map.WriteString("TeamTypes", key, value);
    map.WriteString(value, "Max", "5");
    map.WriteString(value, "Full", "no");
    map.WriteString(value, "Name", newName);
    map.WriteString(value, "Group", "-1");
    if (SendMessage(hHouse, CB_GETCOUNT, NULL, NULL) > 0)
    {
        SendMessage(hHouse, CB_GETLBTEXT, 0, (LPARAM)buffer);
        map.WriteString(value, "House", Translations::ParseHouseName(buffer, false));
    }
    else
        map.WriteString(value, "House", "<all>");
    if (SendMessage(hScript, CB_GETCOUNT, NULL, NULL) > 0)
    {
        SendMessage(hScript, CB_GETLBTEXT, 0, (LPARAM)buffer);
        buffer2 = buffer;
        FString::TrimIndex(buffer2);
        map.WriteString(value, "Script", buffer2);
    }
    else
        map.WriteString(value, "Script", "");
    map.WriteString(value, "Whiner", "no");
    map.WriteString(value, "Droppod", "no");
    map.WriteString(value, "Suicide", "no");
    map.WriteString(value, "Loadable", "no");
    map.WriteString(value, "Prebuild", "no");
    map.WriteString(value, "Priority", "5");
    if (SendMessage(hWaypoint, CB_GETCOUNT, NULL, NULL) > 0)
    {
        SendMessage(hWaypoint, CB_GETLBTEXT, 0, (LPARAM)buffer);
        map.WriteString(value, "Waypoint", STDHelpers::WaypointToString(ppmfc::CString(buffer)));
    }
    else
        map.WriteString(value, "Waypoint", "A");
    map.WriteString(value, "Annoyance", "no");
    map.WriteString(value, "IonImmune", "no");
    map.WriteString(value, "Recruiter", "no");
    map.WriteString(value, "Reinforce", "no");
    if (SendMessage(hTaskforce, CB_GETCOUNT, NULL, NULL) > 0)
    {
        SendMessage(hTaskforce, CB_GETLBTEXT, 0, (LPARAM)buffer);
        buffer2 = buffer;
        FString::TrimIndex(buffer2);
        map.WriteString(value, "TaskForce", buffer2);
    }
    else
        map.WriteString(value, "TaskForce", "");
    map.WriteString(value, "TechLevel", "0");
    map.WriteString(value, "Aggressive", "no");
    map.WriteString(value, "Autocreate", "no");
    map.WriteString(value, "GuardSlower", "no");
    map.WriteString(value, "OnTransOnly", "no");
    map.WriteString(value, "AvoidThreats", "no");
    map.WriteString(value, "LooseRecruit", "no");    
    map.WriteString(value, "VeteranLevel", "1");
    map.WriteString(value, "IsBaseDefense", "no");
    map.WriteString(value, "UseTransportOrigin", "no");
    map.WriteString(value, "MindControlDecision", "0");
    map.WriteString(value, "OnlyTargetHouseEnemy", "no");
    map.WriteString(value, "TransportsReturnOnUnload", "no");
    map.WriteString(value, "AreTeamMembersRecruitable", "no");


    ExtraWindow::SortTeams(hSelectedTeam, "TeamTypes", SelectedTeamIndex, value);

    OnSelchangeTeamtypes();
}

void CNewTeamTypes::OnClickDelTeam(HWND& hWnd)
{
    if (SelectedTeamIndex < 0)
        return;
    int result = MessageBox(hWnd,
        Translations::TranslateOrDefault("TeamTypesDelTeamWarn", "Are you sure that you want to delete the selected team-type? If you delete it, don't forget to delete any reference to the team-type."),
        Translations::TranslateOrDefault("TeamTypesDelTeamTitle", "Delete team-type"), MB_YESNO);

    if (result == IDNO)
        return;

    map.DeleteSection(CurrentTeamID);
    std::vector<FString> deteleKeys;
    if (auto pSection = map.GetSection("TeamTypes"))
    {
        for (auto& pair : pSection->GetEntities())
        {
            if (CurrentTeamID == pair.second)
                deteleKeys.push_back(pair.first); 
        }
    }
    for (auto& key : deteleKeys)
        map.DeleteKey("TeamTypes", key);
    
    int idx = SelectedTeamIndex;
    SendMessage(hSelectedTeam, CB_DELETESTRING, idx, NULL);
    if (idx >= SendMessage(hSelectedTeam, CB_GETCOUNT, NULL, NULL))
        idx--;
    if (idx < 0)
        idx = 0;
    SendMessage(hSelectedTeam, CB_SETCURSEL, idx, NULL);
    OnSelchangeTeamtypes();
}

void CNewTeamTypes::OnClickCloTeam(HWND& hWnd)
{
    if (SelectedTeamIndex < 0)
        return;
    if (SendMessage(hSelectedTeam, CB_GETCOUNT, NULL, NULL) > 0 && SelectedTeamIndex >= 0)
    {
        FString key = CINI::GetAvailableKey("TeamTypes");
        FString value = CMapDataExt::GetAvailableIndex();

        CINI::CurrentDocument->WriteString("TeamTypes", key, value);

        auto oldname = CINI::CurrentDocument->GetString(CurrentTeamID, "Name", "New Teamtype");
        FString newName = ExtraWindow::GetCloneName(oldname);

        CINI::CurrentDocument->WriteString(value, "Name", newName);

        auto copyitem = [&value](FString key)
            {
                if (auto ppStr = map.TryGetString(CurrentTeamID, key)) {
                    FString str = *ppStr;
                    str.Trim();
                    map.WriteString(value, key, str);
                }
            };

        copyitem("Max");
        copyitem("Tag");
        copyitem("Full");
        copyitem("Group");
        copyitem("House");
        copyitem("Script");
        copyitem("Whiner");
        copyitem("Droppod");
        copyitem("Suicide");
        copyitem("Loadable");
        copyitem("Prebuild");
        copyitem("Priority");
        copyitem("Waypoint");
        copyitem("Annoyance");
        copyitem("IonImmune");
        copyitem("Recruiter");
        copyitem("Reinforce");
        copyitem("TaskForce");
        copyitem("TechLevel");
        copyitem("Aggressive");
        copyitem("Autocreate");
        copyitem("GuardSlower");
        copyitem("OnTransOnly");
        copyitem("AvoidThreats");
        copyitem("LooseRecruit");
        copyitem("VeteranLevel");
        copyitem("IsBaseDefense");
        copyitem("TransportWaypoint");
        copyitem("UseTransportOrigin");
        copyitem("MindControlDecision");
        copyitem("OnlyTargetHouseEnemy");
        copyitem("TransportsReturnOnUnload");
        copyitem("AreTeamMembersRecruitable");

        ExtraWindow::SortTeams(hSelectedTeam, "TeamTypes", SelectedTeamIndex, value);

        OnSelchangeTeamtypes();
    }
}

void CNewTeamTypes::OnClickSearchReference(HWND& hWnd)
{
    if (SelectedTeamIndex < 0)
        return;

    CSearhReference::SetSearchType(0);
    CSearhReference::SetSearchID(CurrentTeamID);
    if (CSearhReference::GetHandle() == NULL)
    {
        CSearhReference::Create(m_parent);
    }
    else
    {
        ::SendMessage(CSearhReference::GetHandle(), 114514, 0, 0);
    }

}

bool CNewTeamTypes::OnEnterKeyDown(HWND& hWnd)
{
    if (hWnd == hSelectedTeam)
        OnSelchangeTeamtypes(true);
    else if (hWnd == hHouse)
        OnSelchangeHouse(true);
    else if (hWnd == hTag)
        OnSelchangeTag(true);
    else if (hWnd == hTaskforce)
        OnSelchangeTaskForce(true);
    else if (hWnd == hScript)
        OnSelchangeScript(true);
    else
        return false;
    return true;
}