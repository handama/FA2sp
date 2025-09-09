#pragma once

#include <FA2PP.h>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Helpers/MultimapHelper.h"
#include "../../Helpers/FString.h"

// A static window class
class CNewTeamTypes
{
public:
    enum Controls {
        SelectedTeam = 1109,
        NewTeam = 1110,
        DelTeam = 1111,
        CloTeam = 6001,
        Name = 1010,
        House = 1079,
        Taskforce = 1125,
        Script = 1124,
        Tag = 1083,
        VeteranLevel = 1143,
        Priority = 1011,
        Max = 1012,
        Techlevel = 1103,
        TransportWaypoint = 1126,
        Group = 1122,
        Waypoint = 1123,
        MindControlDecision = 1140,
        CheckBoxLoadable = 1113,
        CheckBoxFull = 1114,
        CheckBoxAnnoyance = 1115,
        CheckBoxGuardSlower = 1116,
        CheckBoxRecruiter = 1117,
        CheckBoxAutoCreate = 1119,
        CheckBoxPrebuild = 1120,
        CheckBoxReinforce = 1127,
        CheckBoxCargoPlane = 1128,
        CheckBoxWhiner = 1129,
        CheckBoxLooseRecruit = 1130,
        CheckBoxAggressive = 1131,
        CheckBoxSuicide = 1132,
        CheckBoxOnTransOnly = 1133,
        CheckBoxAvoidThreats = 1134,
        CheckBoxIonImmune = 1135,
        CheckBoxTransportsReturnOnUnload = 1136,
        CheckBoxAreTeamMembersRecruitable = 1137,
        CheckBoxIsBaseDefense = 1138,
        CheckBoxOnlyTargetHouseEnemy = 1139,
        TurnToTaskforce = 1500,
        TurnToScript = 1501,
        TurnToTag = 1502,
        SearchReference = 1999
    };

    static void Create(CFinalSunDlg* pWnd);
    

    static HWND GetHandle()
    {
        return CNewTeamTypes::m_hwnd;
    }
    static bool OnEnterKeyDown(HWND& hWnd);
    static void OnSelchangeTeamtypes(bool edited = false);
    static void OnSelchangeTaskForce(bool edited = false);
    static void OnSelchangeScript(bool edited = false);
    static void OnSelchangeTag(bool edited = false);
    static void OnClickNewTeam();

protected:
    static void Initialize(HWND& hWnd);
    static void Update(HWND& hWnd);
    static void OnSeldropdownTeamtypes(HWND& hWnd);
    static void OnSelchangeTransportWaypoint(HWND& hWnd, bool edited = false);
    static void OnSelchangeWaypoint(HWND& hWnd, bool edited = false);
    static void OnSelchangeHouse(bool edited = false);
    static void OnSelchangeVeteranLevel(HWND& hWnd, bool edited = false);
    static void OnSelchangeTechlevel(HWND& hWnd, bool edited = false);
    static void OnSelchangeMindControlDecision(HWND& hWnd, bool edited = false);
    static void OnSelchangeGroup(HWND& hWnd, bool edited = false);
    static void OnClickDelTeam(HWND& hWnd);
    static void OnClickCloTeam(HWND& hWnd);
    static void OnClickSearchReference(HWND& hWnd);

    static void OnCloseupTaskForce();
    static void OnCloseupScript();
    static void OnCloseupTeamtypes();
    static void OnCloseupTag();
    static void OnCloseupHouse();

    static void OnClickTurnToTaskforce();
    static void OnClickTurnToScript();
    static void OnClickTurnToTag();

    static void Close(HWND& hWnd);

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);


private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static CINI& map;
    static MultimapHelper& rules;
public:
    static HWND hSelectedTeam;
    static HWND hNewTeam;
    static HWND hDelTeam;
    static HWND hCloTeam;
    static HWND hName;
    static HWND hHouse;
    static HWND hTaskforce;
    static HWND hScript;
    static HWND hTag;
    static HWND hVeteranLevel;
    static HWND hPriority;
    static HWND hMax;
    static HWND hTechlevel;
    static HWND hTransportWaypoint;
    static HWND hGroup;
    static HWND hWaypoint;
    static HWND hMindControlDecision;
    static HWND hCheckBoxLoadable;
    static HWND hCheckBoxFull;
    static HWND hCheckBoxAnnoyance;
    static HWND hCheckBoxGuardSlower;
    static HWND hCheckBoxRecruiter;
    static HWND hCheckBoxAutoCreate;
    static HWND hCheckBoxPrebuild;
    static HWND hCheckBoxReinforce;
    static HWND hCheckBoxCargoPlane;
    static HWND hCheckBoxWhiner;
    static HWND hCheckBoxLooseRecruit;
    static HWND hCheckBoxAggressive;
    static HWND hCheckBoxSuicide;
    static HWND hCheckBoxOnTransOnly;
    static HWND hCheckBoxAvoidThreats;
    static HWND hCheckBoxIonImmune;
    static HWND hCheckBoxTransportsReturnOnUnload;
    static HWND hCheckBoxAreTeamMembersRecruitable;
    static HWND hCheckBoxIsBaseDefense;
    static HWND hCheckBoxOnlyTargetHouseEnemy;
    static HWND hSearchReference;
    static HWND hTurnToTaskforce;
    static HWND hTurnToScript;
    static HWND hTurnToTag;
private:
    static int SelectedTeamIndex;
    static FString CurrentTeamID;
    static std::map<int, FString> TaskForceLabels;
    static std::map<int, FString> TeamLabels;
    static std::map<int, FString> ScriptLabels;
    static std::map<int, FString> TagLabels;
    static std::map<int, FString> HouseLabels;
    static bool Autodrop;
    static bool WaypointAutodrop;
    static bool DropNeedUpdate;
    static std::vector<FString> mindControlDecisions;

};

