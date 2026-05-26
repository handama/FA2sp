#include "CNewHouse.h"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/MultimapHelper.h"

#include <CLoading.h>
#include <CFinalSunDlg.h>
#include <CObjectDatas.h>
#include <CMapData.h>
#include <CIsoView.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CMapData/Body.h"
#include "../CNewComboUInputDlg/CNewComboUInputDlg.h"
#include <Miscs/Miscs.h>
#include <CInputMessageBox.h>

HWND CNewHouse::m_hwnd;
CFinalSunDlg* CNewHouse::m_parent;
CINI& CNewHouse::map = CINI::CurrentDocument;
MultimapHelper& CNewHouse::rules = Variables::RulesMap;

HWND CNewHouse::hSelectedHouse;
HWND CNewHouse::hAddHouse;
HWND CNewHouse::hStandardHouses;
HWND CNewHouse::hDeleteHouse;
HWND CNewHouse::hIQ;
HWND CNewHouse::hEdge;
HWND CNewHouse::hSide;
HWND CNewHouse::hColor;
HWND CNewHouse::hAllies;
HWND CNewHouse::hCredits;
HWND CNewHouse::hNodeCount;
HWND CNewHouse::hTechLevel;
HWND CNewHouse::hBuildActivity;
HWND CNewHouse::hAlliesEditor;
HWND CNewHouse::hHumanPlayer;
HWND CNewHouse::hPlayerControl;
HWND CNewHouse::hRepairBaseNodes;
HWND CNewHouse::hRepairBaseNodesEasy;
HWND CNewHouse::hRepairBaseNodesMedium;
HWND CNewHouse::hRepairBaseNodesHard;

int CNewHouse::SelectedHouseIndex;
FString CNewHouse::SelectedHouseName;

VirtualComboBoxEx CNewHouse::vcbSelectedHouse;
VirtualComboBoxEx CNewHouse::vcbHumanPlayer;
VirtualComboBoxEx CNewHouse::vcbIQ;
VirtualComboBoxEx CNewHouse::vcbEdge;
VirtualComboBoxEx CNewHouse::vcbSide;
VirtualComboBoxEx CNewHouse::vcbColor;
VirtualComboBoxEx CNewHouse::vcbTechLevel;
VirtualComboBoxEx CNewHouse::vcbBuildActivity;

bool CNewHouse::m_programmaticEdit;

void CNewHouse::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(341),
        pWnd->GetSafeHwnd(),
        CNewHouse::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CNewHouse.\n");
        m_parent = NULL;
        return;
    }
}

void CNewHouse::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("Countries", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };

    Translate(1285, "HousesDesc");
    Translate(1258, "HousesDesc");
    Translate(1287, "HousesIQ");
    Translate(1288, "HousesEdge");
    Translate(1289, "HousesSide");
    Translate(1290, "HousesColor");
    Translate(1291, "HousesAllies");
    Translate(1292, "HousesCredits");
    Translate(1293, "HousesActsLike");
    Translate(1294, "HousesNodeCount");
    Translate(1295, "HousesTechlevel");
    Translate(1296, "HousesBuildActivity");
    Translate(1255, "HousesPlayerHouse");
    Translate(1258, "HousesHouse");
    Translate(1092, "HousesAddHouse");
    Translate(1093, "HousesPrepareHouses");
    Translate(1094, "HousesDeleteHouse");
    Translate(Controls::PlayerControl, "HousesPlayerControl");
    Translate(Controls::RepairBaseNodes, "HousesRepairBaseNodes");
    Translate(Controls::RepairBaseNodesEasy, "HousesRepairBaseNodesEasy");
    Translate(Controls::RepairBaseNodesMedium, "HousesRepairBaseNodesMedium");
    Translate(Controls::RepairBaseNodesHard, "HousesRepairBaseNodesHard");
    Translate(Controls::AlliesEditor, "AllieEditorTitle");

    hSelectedHouse = GetDlgItem(hWnd, Controls::SelectedHouse);
    hAddHouse = GetDlgItem(hWnd, Controls::AddHouse);
    hStandardHouses = GetDlgItem(hWnd, Controls::StandardHouses);
    hDeleteHouse = GetDlgItem(hWnd, Controls::DeleteHouse);
    hIQ = GetDlgItem(hWnd, Controls::IQ);
    hEdge = GetDlgItem(hWnd, Controls::Edge);
    hSide = GetDlgItem(hWnd, Controls::Side);
    hColor = GetDlgItem(hWnd, Controls::Color);
    hAllies = GetDlgItem(hWnd, Controls::Allies);
    hCredits = GetDlgItem(hWnd, Controls::Credits);
    hNodeCount = GetDlgItem(hWnd, Controls::NodeCount);
    hTechLevel = GetDlgItem(hWnd, Controls::TechLevel);
    hBuildActivity = GetDlgItem(hWnd, Controls::BuildActivity);
    hAlliesEditor = GetDlgItem(hWnd, Controls::AlliesEditor);
    hHumanPlayer = GetDlgItem(hWnd, Controls::HumanPlayer);
    hPlayerControl = GetDlgItem(hWnd, Controls::PlayerControl);
    hRepairBaseNodesEasy = GetDlgItem(hWnd, Controls::RepairBaseNodesEasy);
    hRepairBaseNodesMedium = GetDlgItem(hWnd, Controls::RepairBaseNodesMedium);
    hRepairBaseNodesHard = GetDlgItem(hWnd, Controls::RepairBaseNodesHard);

    vcbSelectedHouse.Attach(hSelectedHouse, nullptr, false);
    vcbHumanPlayer.Attach(hHumanPlayer, nullptr, false);
    vcbIQ.Attach(hIQ);
    vcbEdge.Attach(hEdge);
    vcbSide.Attach(hSide);
    vcbColor.Attach(hColor);
    vcbTechLevel.Attach(hTechLevel);
    vcbBuildActivity.Attach(hBuildActivity);

    auto pHouse = map.GetSection("Houses");
    if (!pHouse || pHouse->GetEntities().size() == 0)
    {      
        ::MessageBoxA(m_hwnd,
            Translations::TranslateOrDefault("HouseDialog.NoHousesExist",
                "No houses do exist, if you want to use houses, you should use Prepare houses before doing anything else."
                ), "FA2sp", NULL);
    }

    Update();
}

void CNewHouse::Update(FString targetHouse)
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    // IQ dropdown (0-5)
    vcbIQ.Clear();
    vcbIQ.AddString("0");
    vcbIQ.AddString("1");
    vcbIQ.AddString("2");
    vcbIQ.AddString("3");
    vcbIQ.AddString("4");
    vcbIQ.AddString("5");

    // Edge dropdown
    vcbEdge.Clear();
    vcbEdge.AddString("East");
    vcbEdge.AddString("North");
    vcbEdge.AddString("South");
    vcbEdge.AddString("West");

    // Side dropdown (from Countries list in Rules)
    vcbSide.Clear();
    if (auto pCountries = map.GetSection("Countries"))
    {
        for (auto& [_, value] : pCountries->GetEntities()) {
            auto text = Translations::ParseHouseName(value, true);
            vcbSide.AddString(text);
        }
    }
    
    vcbSelectedHouse.Clear();
    vcbHumanPlayer.Clear();
    vcbHumanPlayer.AddString("None");
    if (auto pSection = map.GetSection("Houses")) {
        for (auto& [_, value] : pSection->GetEntities()) {
            auto text = Translations::ParseHouseName(value, true);
            vcbHumanPlayer.AddString(text);
            vcbSelectedHouse.AddString(text);
        }
    }

    // Color dropdown
    vcbColor.Clear();
    auto&& colors = Variables::RulesMap.GetUnorderedSection("Colors");
    for (auto& [key, _] : colors) {
        auto color = Miscs::GetColorRef(nullptr, key.GetString());
        vcbColor.AddString(key, CLR_INVALID, color, true);
    }

    // TechLevel dropdown (1-10)
    vcbTechLevel.Clear();
    vcbTechLevel.AddString("0");
    vcbTechLevel.AddString("1");
    vcbTechLevel.AddString("2");
    vcbTechLevel.AddString("3");
    vcbTechLevel.AddString("4");
    vcbTechLevel.AddString("5");
    vcbTechLevel.AddString("6");
    vcbTechLevel.AddString("7");
    vcbTechLevel.AddString("8");
    vcbTechLevel.AddString("9");
    vcbTechLevel.AddString("10");

    // BuildActivity dropdown
    vcbBuildActivity.Clear();
    vcbBuildActivity.AddString("0");
    vcbBuildActivity.AddString("50");
    vcbBuildActivity.AddString("100");

    FString playerHouse = map.GetString("Basic", "Player", "None");
    playerHouse = Translations::ParseHouseName(playerHouse, true);
    int playerIndex = vcbHumanPlayer.FindStringExact(playerHouse);
    vcbHumanPlayer.SetCurSel(playerIndex);

    EnableWindow(hNodeCount, FALSE);

    if (!targetHouse.IsEmpty())
    {
        targetHouse = Translations::ParseHouseName(targetHouse, true);
        int index = vcbSelectedHouse.FindStringExact(targetHouse);
        OnSelchangeHouse(index);
    }
    else
    {
        if (SelectedHouseIndex < 0)
            SelectedHouseIndex = 0;
        if (SelectedHouseIndex > vcbSelectedHouse.GetCount() - 1)
            SelectedHouseIndex = vcbSelectedHouse.GetCount() - 1;

        OnSelchangeHouse(SelectedHouseIndex);
    }

}

void CNewHouse::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CNewHouse::m_hwnd = NULL;
    CNewHouse::m_parent = NULL;
}

BOOL CALLBACK CNewHouse::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CNewHouse::Initialize(hWnd);
        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::AddHouse:
            if (CODE == BN_CLICKED)
                OnClickAddHouse();
            break;
        case Controls::DeleteHouse:
            if (CODE == BN_CLICKED)
                OnClickDeleteHouse(hWnd);
            break;
        case Controls::StandardHouses:
            if (CODE == BN_CLICKED)
                OnClickStandardHouses();
            break;
        case Controls::AlliesEditor:
            if (CODE == BN_CLICKED)
                OnClickAlliesEditor();
            break;
        case Controls::SelectedHouse:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeHouse();
            break;
        case Controls::HumanPlayer:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeHumanPlayer();
            break;
        case Controls::IQ:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeIQ();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeIQ(true);
            break;
        case Controls::Edge:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeEdge();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeEdge(true);
            break;
        case Controls::Side:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeSide();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeSide(true);
            break;
        case Controls::Color:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeColor();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeColor(true);
            break;
        case Controls::TechLevel:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTechLevel();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeTechLevel(true);
            break;
        case Controls::BuildActivity:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeBuildActivity();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeBuildActivity(true);
            break;
        case Controls::Allies:
            if (CODE == EN_CHANGE && !m_programmaticEdit)
                OnEditchangeAllies();
            break;
        case Controls::Credits:
            if (CODE == EN_CHANGE && !m_programmaticEdit)
                OnEditchangeCredits();
            break;
        //case Controls::NodeCount:
        //    if (CODE == EN_CHANGE && !m_programmaticEdit)
        //        OnEditchangeNodeCount();
        //    break;
        case Controls::PlayerControl:
            if (CODE == BN_CLICKED)
                OnClickPlayerControl();
            break;
        case Controls::RepairBaseNodesEasy:
        case Controls::RepairBaseNodesMedium:
        case Controls::RepairBaseNodesHard:
            if (CODE == BN_CLICKED)
                OnClickRepairBaseNodes();
            break;
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CNewHouse::Close(hWnd);
        return TRUE;
    }
    case 114514: // used for update
    {
        Update();
        return TRUE;
    }
    case WM_MEASUREITEM:
    {
        VirtualComboBoxEx::SetWindowHeight(hWnd, lParam);
        return TRUE;
    }
    }

    return FALSE;
}

void CNewHouse::LoadHouseProperties(const FString& houseName)
{
    m_programmaticEdit = true;

    if (houseName.IsEmpty() || !map.GetSection(houseName))
    {
        vcbIQ.SetCurSel(-1);
        vcbEdge.SetCurSel(-1);
        vcbSide.SetCurSel(-1);
        vcbColor.SetCurSel(-1);
        vcbSelectedHouse.SetCurSel(-1);
        SetWindowText(hAllies, "");
        SetWindowText(hCredits, "");
        SetWindowText(hNodeCount, "");
        vcbTechLevel.SetCurSel(-1);
        vcbBuildActivity.SetCurSel(-1);
        SendMessage(hPlayerControl, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hRepairBaseNodesEasy, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hRepairBaseNodesMedium, BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(hRepairBaseNodesHard, BM_SETCHECK, BST_UNCHECKED, 0);
        SelectedHouseName = "";
    }
    else
    {
        // IQ
        FString iq = map.GetString(houseName, "IQ", "0");
        int idx = vcbIQ.FindStringExact(iq);
        if (idx != CB_ERR) vcbIQ.SetCurSel(idx);
        else vcbIQ.SetEditText(iq);

        // Edge
        FString edge = map.GetString(houseName, "Edge", "North");
        idx = vcbEdge.FindStringExact(edge);
        if (idx != CB_ERR) vcbEdge.SetCurSel(idx);
        else vcbEdge.SetEditText(edge);

        // Side
        FString side = map.GetString(houseName, "Country", "");
        side = Translations::ParseHouseName(side, true);
        idx = vcbSide.FindStringExact(side);
        if (idx != CB_ERR) vcbSide.SetCurSel(idx);
        else vcbSide.SetEditText(side);

        // Color
        FString color = map.GetString(houseName, "Color", "Gold");
        idx = vcbColor.FindStringExact(color);
        if (idx != CB_ERR) vcbColor.SetCurSel(idx);
        else vcbColor.SetEditText(color);

        // Allies (EDIT control)
        FString allies = map.GetString(houseName, "Allies", "");
        SetWindowText(hAllies, allies);

        // Credits (EDIT control)
        FString credits = map.GetString(houseName, "Credits", "0");
        SetWindowText(hCredits, credits);

        // NodeCount (EDIT control)
        FString nodeCount = map.GetString(houseName, "NodeCount", "0");
        SetWindowText(hNodeCount, nodeCount);

        // TechLevel
        FString techLevel = map.GetString(houseName, "TechLevel", "10");
        idx = vcbTechLevel.FindStringExact(techLevel);
        if (idx != CB_ERR) vcbTechLevel.SetCurSel(idx);
        else vcbTechLevel.SetEditText(techLevel);

        // BuildActivity
        FString buildActivity = map.GetString(houseName, "PercentBuilt", "100");
        idx = vcbBuildActivity.FindStringExact(buildActivity);
        if (idx != CB_ERR) vcbBuildActivity.SetCurSel(idx);
        else vcbBuildActivity.SetEditText(buildActivity);

        // PlayerControl checkbox
        bool playerControl = map.GetBool(houseName, "PlayerControl", false);
        SendMessage(hPlayerControl, BM_SETCHECK, playerControl ? BST_CHECKED : BST_UNCHECKED, 0);

        // RepairBaseNodes checkbox
        auto repairs = map.GetString(houseName, "RepairBaseNodes", "no,no,no");
        auto atoms = STDHelpers::SplitString(repairs, 2);
        SendMessage(hRepairBaseNodesEasy, BM_SETCHECK, STDHelpers::IsTrue(atoms[0]) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessage(hRepairBaseNodesMedium, BM_SETCHECK, STDHelpers::IsTrue(atoms[1]) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessage(hRepairBaseNodesHard, BM_SETCHECK, STDHelpers::IsTrue(atoms[2]) ? BST_CHECKED : BST_UNCHECKED, 0);
    }

    m_programmaticEdit = false;
}

void CNewHouse::OnSelchangeHouse(int specificIdx)
{
    if (specificIdx != -1)
    {
        SelectedHouseIndex = specificIdx;
        vcbSelectedHouse.SetCurSel(SelectedHouseIndex);
    }
    else
        SelectedHouseIndex = vcbSelectedHouse.GetCurSel();

    if (SelectedHouseIndex < 0 || SelectedHouseIndex >= vcbSelectedHouse.GetCount())
    {
        SelectedHouseName = "";
        LoadHouseProperties("");
        return;
    }

    FString name = vcbSelectedHouse.GetItemText(SelectedHouseIndex);
    name = Translations::ParseHouseName(name, false);
    SelectedHouseName = name;

    LoadHouseProperties(name);
}

void CNewHouse::OnSelchangeIQ(bool edited)
{
    if (SelectedHouseName.IsEmpty()) return;
    FString text = vcbIQ.GetSelectedText(edited);
    if (text.empty()) return;
    FString::TrimIndex(text);
    map.WriteString(SelectedHouseName, "IQ", text);
}

void CNewHouse::OnSelchangeEdge(bool edited)
{
    if (SelectedHouseName.IsEmpty()) return;
    FString text = vcbEdge.GetSelectedText(edited);
    if (text.empty()) return;
    FString::TrimIndex(text);
    map.WriteString(SelectedHouseName, "Edge", text);
}

void CNewHouse::OnSelchangeSide(bool edited)
{
    if (SelectedHouseName.IsEmpty()) return;
    FString text = vcbSide.GetSelectedText(edited);
    if (text.empty()) return;
    text = Translations::ParseHouseName(text, false);
    FString::TrimIndex(text);
    map.WriteString(SelectedHouseName, "Country", text);
}

void CNewHouse::OnSelchangeColor(bool edited)
{
    if (SelectedHouseName.IsEmpty()) return;
    FString text = vcbColor.GetSelectedText(edited);
    if (text.empty()) return;
    FString::TrimIndex(text);
    map.WriteString(SelectedHouseName, "Color", text);
}

void CNewHouse::OnEditchangeAllies()
{
    if (SelectedHouseName.IsEmpty()) return;
    char buffer[512]{ 0 };
    GetWindowText(hAllies, buffer, 511);
    FString text = buffer;
    map.WriteString(SelectedHouseName, "Allies", text);
}

void CNewHouse::OnEditchangeCredits()
{
    if (SelectedHouseName.IsEmpty()) return;
    char buffer[512]{ 0 };
    GetWindowText(hCredits, buffer, 511);
    FString text = buffer;
    map.WriteString(SelectedHouseName, "Credits", text);
}

void CNewHouse::OnSelchangeTechLevel(bool edited)
{
    if (SelectedHouseName.IsEmpty()) return;
    FString text = vcbTechLevel.GetSelectedText(edited);
    if (text.empty()) return;
    FString::TrimIndex(text);
    map.WriteString(SelectedHouseName, "TechLevel", text);
}

void CNewHouse::OnSelchangeBuildActivity(bool edited)
{
    if (SelectedHouseName.IsEmpty()) return;
    FString text = vcbBuildActivity.GetSelectedText(edited);
    if (text.empty()) return;
    FString::TrimIndex(text);
    map.WriteString(SelectedHouseName, "PercentBuilt", text);
}

void CNewHouse::OnEditchangeNodeCount()
{
    if (SelectedHouseName.IsEmpty()) return;
    char buffer[512]{ 0 };
    GetWindowText(hNodeCount, buffer, 511);
    FString text = buffer;
    map.WriteString(SelectedHouseName, "NodeCount", text);
}

void CNewHouse::OnSelchangeHumanPlayer()
{
    int index = vcbHumanPlayer.GetCurSel();

    FString name = vcbHumanPlayer.GetItemText(index);
    if (name.IsEmpty())
        return;

    name = Translations::ParseHouseName(name, false);
    map.WriteString("Basic", "Player", name);
}

void CNewHouse::OnClickPlayerControl()
{
    if (SelectedHouseName.IsEmpty()) return;
    bool checked = SendMessage(hPlayerControl, BM_GETCHECK, 0, 0) == BST_CHECKED;
    map.WriteString(SelectedHouseName, "PlayerControl", checked ? "yes" : "no");
}

void CNewHouse::OnClickRepairBaseNodes()
{
    if (SelectedHouseName.IsEmpty()) return;

    FString value;
    value.Format("%s,%s,%s", 
        SendMessage(hRepairBaseNodesEasy, BM_GETCHECK, 0, 0) == BST_CHECKED ? "yes" : "no",
        SendMessage(hRepairBaseNodesMedium, BM_GETCHECK, 0, 0) == BST_CHECKED ? "yes" : "no",
        SendMessage(hRepairBaseNodesHard, BM_GETCHECK, 0, 0) == BST_CHECKED ? "yes" : "no");
        
    map.WriteString(SelectedHouseName, "RepairBaseNodes", value);
}

void CNewHouse::OnClickAddHouse()
{
    FString dlgCap = Translations::TranslateOrDefault("AddHouseCap", 
        "Add House");
    FString dlgMessage = Translations::TranslateOrDefault("AddHouse", 
        "Please set the ID of the house (like GDI or Nod):");

    auto newCountry = CInputMessageBox::GetString(dlgMessage, dlgCap);
    newCountry.Replace("House", "");
    newCountry.Trim();
    auto newHouse = newCountry + " House";

    auto valueExists = [&](const ppmfc::CString& pSection, const ppmfc::CString& pValue)
    {
		if (auto section = map.GetSection(pSection))
        {       
            for (auto& [_, value] : section->GetEntities())
            {
                if (value == pValue)
                    return true;
            }
        }
		return false;
    };

    if (map.SectionExists(newHouse) || map.SectionExists(newCountry) 
        || valueExists("Houses", newHouse) || valueExists("Countries", newCountry))
    {
        FString message = Translations::TranslateOrDefault("HouseDialog.NotAvailable", 
            "Sorry this name is not available. %s is already used in the map file. You need to use another name.");
        message.Format(message, newHouse);
        ::MessageBox(m_hwnd, message, "FA2sp", NULL);
        return;
    }

    CNewComboUInputDlg dlg;
    dlg.m_type = COMBOUINPUT_ALL_CUSTOM;
    dlg.m_Caption = Translations::TranslateOrDefault("HouseDialog.SelectParentCountry", 
        "Please select parent country.");

    for (auto& country : Variables::Rules.ParseIndicies("Countries", true))
    {
        dlg.m_CustomStrings.push_back(Translations::ParseHouseName(country, true));
    }
    dlg.DoModal();
    if (dlg.m_Combo.IsEmpty())
    {
        return;
    }

    auto parentCountry = Translations::ParseHouseName(dlg.m_Combo, false);

    map.WriteString("Houses", CINI::GetAvailableKey("Houses"), newHouse);

    // Default properties
    map.WriteString(newHouse, "IQ", "5");
    map.WriteString(newHouse, "Edge", "West");
    map.WriteString(newHouse, "Color", Variables::Rules.GetString(parentCountry, "Color", "DarkRed"));
    map.WriteString(newHouse, "Allies", newHouse);
    map.WriteString(newHouse, "Credits", "0");
    map.WriteString(newHouse, "TechLevel", "10");
    map.WriteString(newHouse, "PercentBuilt", "100");
    map.WriteString(newHouse, "NodeCount", "0");
    map.WriteString(newHouse, "PlayerControl", "no");
    map.WriteString(newHouse, "Country", newCountry);
    
    map.WriteString("Countries", CINI::GetAvailableKey("Countries"), newCountry);
    map.WriteString(newCountry, "Side", Variables::Rules.GetString(parentCountry, "Side"));
    map.WriteString(newCountry, "Color", Variables::Rules.GetString(parentCountry, "Color"));
    map.WriteString(newCountry, "Prefix", Variables::Rules.GetString(parentCountry, "Prefix"));
    map.WriteString(newCountry, "Suffix", Variables::Rules.GetString(parentCountry, "Suffix"));
    map.WriteString(newCountry, "SmartAI", Variables::Rules.GetString(parentCountry, "SmartAI"));
    map.WriteString(newCountry, "CostUnitsMult", "1");
    map.WriteString(newCountry, "Name", newCountry);
    map.WriteString(newCountry, "ParentCountry", parentCountry);

    Update(newHouse);
    
    CMapDataExt::UpdateMapSectionIndicies("Houses");
    CMapDataExt::UpdateMapSectionIndicies("Countries");
    ((CViewObjectsExt*)CFinalSunDlg::Instance->MyViewFrame.pViewObjects)->Redraw();
}

void CNewHouse::OnClickDeleteHouse(HWND& hWnd)
{
    if (SelectedHouseName.IsEmpty()) return;

    FString msg = Translations::TranslateOrDefault("DeleteHouse",
        "Are you sure that you want to delete the house %1?");
        msg.ReplaceNumString(1, SelectedHouseName);

    if (MessageBox(hWnd, msg,
        Translations::TranslateOrDefault("DeleteHouseCap", "Delete House"),
        MB_YESNO) == IDYES)
    {
        std::vector<ppmfc::CString> keys;
        if (auto section = map.GetSection("Houses"))
        {       
            for (auto& [key, value] : section->GetEntities())
            {
                if (value == SelectedHouseName)
                    keys.push_back(key);
            }
        }
        for (auto& key : keys)
        {
            map.DeleteKey("Houses", key);
        }
        map.DeleteSection(SelectedHouseName);
        if (map.GetString("Basic", "Player") == SelectedHouseName)
        {
            map.WriteString("Basic", "Player", "None");
        }
        SelectedHouseName = "";
        Update();

        CMapDataExt::UpdateMapSectionIndicies("Houses");
        CMapDataExt::UpdateMapSectionIndicies("Countries");
        ((CViewObjectsExt*)CFinalSunDlg::Instance->MyViewFrame.pViewObjects)->Redraw();
    }
}

void CNewHouse::OnClickStandardHouses()
{
	//if (CMapData::Instance->IsMultiOnly())
	//{
    //    map.WriteString("Basic", "MultiplayerOnly", "1");
    //    for (auto& country : Variables::Rules.ParseIndicies("Countries", true))
    //    {
    //        map.WriteString(country, "IQ", "0");
    //        map.WriteString(country, "Edge", "West");
    //        map.WriteString(country, "Color", Variables::Rules.GetString(country, "Color", "DarkRed"));
    //        map.WriteString(country, "Allies", country);
    //        map.WriteString(country, "Credits", "0");
    //        map.WriteString(country, "TechLevel", "1");
    //        map.WriteString(country, "PercentBuilt", "0");
    //        map.WriteString(country, "NodeCount", "0");
    //        map.WriteString(country, "PlayerControl", "no");
    //        map.WriteString(country, "Country", country);
    //    }
	//	return;
	//}
    auto pHouse = map.GetSection("Houses");
    if (pHouse && pHouse->GetEntities().size() > 0)
    {      
        ::MessageBoxA(m_hwnd,
            Translations::TranslateOrDefault("HouseDialog.AlreadyHouses",
                "There are already houses in your map. You need to delete these first."
                ), "FA2sp", NULL);
        return;
    }

    for (auto& country : Variables::Rules.ParseIndicies("Countries", true))
    {
        FString house = country + " House";       
        map.WriteString("Houses", CINI::GetAvailableKey("Houses"), house);
        map.WriteString(house, "IQ", "5");
        map.WriteString(house, "Edge", "West");
        map.WriteString(house, "Color", Variables::Rules.GetString(country, "Color", "DarkRed"));
        map.WriteString(house, "Allies", house);
        map.WriteString(house, "Credits", "0");
        map.WriteString(house, "TechLevel", "10");
        map.WriteString(house, "PercentBuilt", "100");
        map.WriteString(house, "NodeCount", "0");
        map.WriteString(house, "PlayerControl", "no");
        map.WriteString(house, "Country", country);
        
        map.WriteString("Countries", CINI::GetAvailableKey("Countries"), country);
        map.WriteString(country, "Side", Variables::Rules.GetString(country, "Side"));
        map.WriteString(country, "Color", Variables::Rules.GetString(country, "Color"));
        map.WriteString(country, "Prefix", Variables::Rules.GetString(country, "Prefix"));
        map.WriteString(country, "Suffix", Variables::Rules.GetString(country, "Suffix"));
        map.WriteString(country, "SmartAI", Variables::Rules.GetString(country, "SmartAI"));
        map.WriteString(country, "CostUnitsMult", "1");
        map.WriteString(country, "Name", country);
        map.WriteString(country, "ParentCountry", country);
    }
    
    Update();

    CMapDataExt::UpdateMapSectionIndicies("Houses");
    CMapDataExt::UpdateMapSectionIndicies("Countries");
    ((CViewObjectsExt*)CFinalSunDlg::Instance->MyViewFrame.pViewObjects)->Redraw();
}

void CNewHouse::OnClickAlliesEditor()
{
    if (SelectedHouseName.IsEmpty()) return;
    DialogBox((HINSTANCE)FA2sp::hInstance, MAKEINTRESOURCE(303), m_hwnd, AllieEditorDlgProc);
}

BOOL CALLBACK CNewHouse::AllieEditorDlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    char buffer[512]{};
	switch (Msg)
	{
	case WM_INITDIALOG: {

		HWND hLBAllies = GetDlgItem(hwnd, 6302);
		HWND hLBEnemies = GetDlgItem(hwnd, 6303);
		HWND hETCurrent = GetDlgItem(hwnd, 6304);

		FSet allies;
		GetWindowText(hAllies, buffer, 511);
		for (auto& str : FString::SplitString(buffer))
			if (!STDHelpers::IsNoneOrEmpty(str))
			{
				str.Trim();
				allies.insert(str);
			}
		
		for (int i = 0; i < vcbSelectedHouse.GetCount(); ++i)
		{
            auto house = vcbSelectedHouse.GetItemText(i);
            auto rawHose = Translations::ParseHouseName(house, false);
			if (SelectedHouseName == house)
				continue;
			if (allies.find(rawHose) != allies.end())
				SendMessage(hLBAllies, LB_ADDSTRING, NULL, (LPARAM)house);
			else
				SendMessage(hLBEnemies, LB_ADDSTRING, NULL, (LPARAM)house);
		}

		SetWindowText(hETCurrent, SelectedHouseName);

		// Translate
		auto translateItem = [&](int nID, const char* lpKey)
		{
			FString buf;
			if (Translations::GetTranslationItem(lpKey, buf))
				SetWindowText(GetDlgItem(hwnd, nID), buf);
		};
		
		translateItem(6305, "AllieEditorEnemies");
		translateItem(6306, "AllieEditorAllies");
		translateItem(IDOK, "AllieEditorOK");
		translateItem(IDCANCEL, "AllieEditorCancel");

		FString buf;
		if (Translations::GetTranslationItem("AllieEditorTitle", buf))
			SetWindowText(hwnd, buf);

		return TRUE;
	}
	case WM_COMMAND: {
		WORD ID = LOWORD(wParam);
		WORD CODE = HIWORD(wParam);
		if (CODE == BN_CLICKED)
		{
			switch (ID)
			{
			case IDOK: {
				FString allies = "";
				HWND LBA = GetDlgItem(hwnd, 6302);//Allies ListBox
				int cnt = SendMessage(LBA, LB_GETCOUNT, NULL, NULL);
				if (cnt > 0)
				{
					FString buffer = "";
					for (int i = 0; i < cnt - 1; ++i)
					{
						int TextLen = SendMessage(LBA, LB_GETTEXTLEN, i, NULL);
						if (TextLen == LB_ERR)	break;
						TCHAR* str = new TCHAR[TextLen + 1];
						SendMessage(LBA, LB_GETTEXT, i, (LPARAM)str);
						buffer = Translations::ParseHouseName(str, false);
						delete[] str;
						allies += buffer + ",";
					}
					int TextLen = SendMessage(LBA, LB_GETTEXTLEN, cnt - 1, NULL);
					if (TextLen != LB_ERR)
					{
						TCHAR* str = new TCHAR[TextLen + 1];
						SendMessage(LBA, LB_GETTEXT, cnt - 1, (LPARAM)str);
						buffer = Translations::ParseHouseName(str, false);
						delete[] str;
						allies += buffer;
					}
				}
                else
                {
                    allies = SelectedHouseName;
                }

				SetWindowText(hAllies, allies);
				OnEditchangeAllies();
				EndDialog(hwnd, NULL);
				return TRUE;
			}
			case IDCANCEL: {
				EndDialog(hwnd, NULL);
				return TRUE;
			}
			case 6300: {//Go Allies: move all selected from Enemies to Allies
				HWND LBA = GetDlgItem(hwnd, 6302);//Allies ListBox
				HWND LBB = GetDlgItem(hwnd, 6303);//Enemy ListBox
				int EnemyCount = SendMessage(LBB, LB_GETCOUNT, NULL, NULL);
				if (EnemyCount <= 0)	break;
				int nSel = SendMessage(LBB, LB_GETSELCOUNT, NULL, NULL);
				if (nSel <= 0)	break;
				std::vector<int> selItems(nSel);
				SendMessage(LBB, LB_GETSELITEMS, nSel, (LPARAM)selItems.data());
                std::sort(selItems.begin(), selItems.end());
                std::vector<FString> strings;
				for (int j = nSel - 1; j >= 0; --j)
				{
					int idx = selItems[j];
					int TextLen = SendMessage(LBB, LB_GETTEXTLEN, idx, NULL);
					if (TextLen == LB_ERR) continue;
					TCHAR* str = new TCHAR[TextLen + 1];
					SendMessage(LBB, LB_GETTEXT, idx, (LPARAM)str);
					SendMessage(LBB, LB_DELETESTRING, idx, NULL);
                    strings.push_back(str);
					delete[] str;
				}
                for (int j = strings.size() - 1; j >= 0; --j)
                {
					SendMessage(LBA, LB_ADDSTRING, NULL, strings[j]);
                }
				break;
			}
			case 6301: {//Go Enemies: move all selected from Allies to Enemies
				HWND LBA = GetDlgItem(hwnd, 6302);//Allies ListBox
				HWND LBB = GetDlgItem(hwnd, 6303);//Enemy ListBox
				int AllieCount = SendMessage(LBA, LB_GETCOUNT, NULL, NULL);
				if (AllieCount <= 0)	break;
				int nSel = SendMessage(LBA, LB_GETSELCOUNT, NULL, NULL);
				if (nSel <= 0)	break;
				std::vector<int> selItems(nSel);
				SendMessage(LBA, LB_GETSELITEMS, nSel, (LPARAM)selItems.data());
                std::sort(selItems.begin(), selItems.end());
                std::vector<FString> strings;
				for (int j = nSel - 1; j >= 0; --j)
				{
					int idx = selItems[j];
					int TextLen = SendMessage(LBA, LB_GETTEXTLEN, idx, NULL);
					if (TextLen == LB_ERR) continue;
					TCHAR* str = new TCHAR[TextLen + 1];
					SendMessage(LBA, LB_GETTEXT, idx, (LPARAM)str);
					SendMessage(LBA, LB_DELETESTRING, idx, NULL);
                    strings.push_back(str);
					delete[] str;
				}
                for (int j = strings.size() - 1; j >= 0; --j)
                {
					SendMessage(LBB, LB_ADDSTRING, NULL, strings[j]);
                }
				break;
			}
			default:
				break;
			}
		}
		else if (CODE == LBN_DBLCLK)
		{
			if (ID == 6303) // Go allies
			{
				HWND LBA = GetDlgItem(hwnd, 6302);//Allies ListBox
				HWND LBB = GetDlgItem(hwnd, 6303);//Enemy ListBox
				int EnemyCount = SendMessage(LBB, LB_GETCOUNT, NULL, NULL);
				if (EnemyCount <= 0)	break;
				int EnemyCurSelIndex = SendMessage(LBB, LB_GETCURSEL, NULL, NULL);
				if (EnemyCurSelIndex < 0 || EnemyCurSelIndex >= EnemyCount)	break;
				int TextLen = SendMessage(LBB, LB_GETTEXTLEN, EnemyCurSelIndex, NULL);
				if (TextLen == LB_ERR)	break;
				TCHAR* str = new TCHAR[TextLen + 1];
				SendMessage(LBB, LB_GETTEXT, EnemyCurSelIndex, (LPARAM)str);
				SendMessage(LBB, LB_DELETESTRING, EnemyCurSelIndex, NULL);
				SendMessage(LBA, LB_ADDSTRING, NULL, (LPARAM)str);
				delete[] str;
			}
			else if (ID == 6302) // Go Enemies
			{
				HWND LBA = GetDlgItem(hwnd, 6302);//Allies ListBox
				HWND LBB = GetDlgItem(hwnd, 6303);//Enemy ListBox
				int AllieCount = SendMessage(LBA, LB_GETCOUNT, NULL, NULL);
				if (AllieCount <= 0)	break;
				int AllieCurSelIndex = SendMessage(LBA, LB_GETCURSEL, NULL, NULL);
				if (AllieCurSelIndex < 0 || AllieCurSelIndex >= AllieCount)	break;
				int TextLen = SendMessage(LBA, LB_GETTEXTLEN, AllieCurSelIndex, NULL);
				if (TextLen == LB_ERR)	break;
				TCHAR* str = new TCHAR[TextLen + 1];
				SendMessage(LBA, LB_GETTEXT, AllieCurSelIndex, (LPARAM)str);
				SendMessage(LBA, LB_DELETESTRING, AllieCurSelIndex, NULL);
				SendMessage(LBB, LB_ADDSTRING, NULL, (LPARAM)str);
				delete[] str;
			}
		}
		break;
	}
	case WM_CLOSE: {
		EndDialog(hwnd, NULL);
		return TRUE;
	}
	}
	return FALSE;
}