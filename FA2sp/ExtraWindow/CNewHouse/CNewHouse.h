#pragma once

#include <FA2PP.h>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Helpers/MultimapHelper.h"
#include "../../Helpers/FString.h"
#include "../Common.h"

class VirtualComboBoxEx;

// A static window class
class CNewHouse
{
public:
    enum Controls {
        SelectedHouse = 1091,
        AddHouse = 1092,
        StandardHouses = 1093,
        DeleteHouse = 1094,
        IQ = 1095,
        Edge = 1096,
        Side = 1097,
        Color = 1098,
        Allies = 1099,
        Credits = 1100,
        NodeCount = 1102,
        TechLevel = 1103,
        BuildActivity = 1104,
        AlliesEditor = 1145,
        HumanPlayer = 1256,
        PlayerControl = 1297,
        RepairBaseNodes = 1298,
        RepairBaseNodesEasy = 1299,
        RepairBaseNodesMedium = 1300,
        RepairBaseNodesHard = 1301,
    };

    static void Create(CFinalSunDlg* pWnd);

    static HWND GetHandle()
    {
        return CNewHouse::m_hwnd;
    }

    static void OnSelchangeHouse(int specificIdx = -1);
    static void OnSelchangeIQ(bool edited = false);
    static void OnSelchangeEdge(bool edited = false);
    static void OnSelchangeSide(bool edited = false);
    static void OnSelchangeColor(bool edited = false);
    static void OnEditchangeAllies();
    static void OnEditchangeCredits();
    static void OnEditchangeNodeCount();
    static void OnSelchangeTechLevel(bool edited = false);
    static void OnSelchangeBuildActivity(bool edited = false);
    static void OnSelchangeHumanPlayer();
    static void OnClickPlayerControl();
    static void OnClickRepairBaseNodes();
    static void OnClickAddHouse();
    static void OnClickDeleteHouse(HWND& hWnd);
    static void OnClickStandardHouses();
    static void OnClickAlliesEditor();


protected:
    static void Initialize(HWND& hWnd);
    static void Update(FString targetHouse = "");
    static void Close(HWND& hWnd);

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK AllieEditorDlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

    static void LoadHouseProperties(const FString& houseName);

private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static CINI& map;
    static MultimapHelper& rules;
public:
    static HWND hSelectedHouse;
    static HWND hAddHouse;
    static HWND hStandardHouses;
    static HWND hDeleteHouse;
    static HWND hIQ;
    static HWND hEdge;
    static HWND hSide;
    static HWND hColor;
    static HWND hAllies;
    static HWND hCredits;
    static HWND hNodeCount;
    static HWND hTechLevel;
    static HWND hBuildActivity;
    static HWND hAlliesEditor;
    static HWND hHumanPlayer;
    static HWND hPlayerControl;
    static HWND hRepairBaseNodes;
    static HWND hRepairBaseNodesEasy;
    static HWND hRepairBaseNodesMedium;
    static HWND hRepairBaseNodesHard;
    static FString SelectedHouseName;
    static int SelectedHouseIndex;
    static VirtualComboBoxEx vcbSelectedHouse;
    static VirtualComboBoxEx vcbHumanPlayer;
    static VirtualComboBoxEx vcbIQ;
    static VirtualComboBoxEx vcbEdge;
    static VirtualComboBoxEx vcbSide;
    static VirtualComboBoxEx vcbColor;
    static VirtualComboBoxEx vcbTechLevel;
    static VirtualComboBoxEx vcbBuildActivity;
private:
    static bool m_programmaticEdit;
};