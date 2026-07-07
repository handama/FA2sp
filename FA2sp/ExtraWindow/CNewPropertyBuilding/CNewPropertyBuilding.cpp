#include "CNewPropertyBuilding.h"
#include <CFinalSunDlg.h>
#include "../../FA2sp.h"
#include "../../Ext/CFinalSunApp/Body.h"
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CMapData/Body.h"
#include "../../Miscs/DialogStyle.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/Helper.h"

CNewPropertyBuilding::CNewPropertyBuilding()
{
}

CNewPropertyBuilding::~CNewPropertyBuilding()
{
}

bool CNewPropertyBuilding::DoModal()
{
    DialogBoxParam(
        reinterpret_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(IDD),
        CFinalSunDlg::Instance->GetSafeHwnd(),
        CNewPropertyBuilding::DlgProc,
        reinterpret_cast<LPARAM>(this)
    );

    m_comboBoxes.clear();
    return m_accepted;
}

BOOL CALLBACK CNewPropertyBuilding::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CNewPropertyBuilding* pThis = reinterpret_cast<CNewPropertyBuilding*>(lParam);
        SetWindowLongPtr(hWnd, DWLP_USER, reinterpret_cast<LONG_PTR>(pThis));
        return pThis->OnInitDialog(hWnd);
    }
    case WM_COMMAND:
    {
        CNewPropertyBuilding* pThis = reinterpret_cast<CNewPropertyBuilding*>(GetWindowLongPtr(hWnd, DWLP_USER));
        if (!pThis) return FALSE;

        WORD id = LOWORD(wParam);
        WORD code = HIWORD(wParam);

        if (id == IDOK && code == BN_CLICKED)
        {
            pThis->OnOK(hWnd);
            return TRUE;
        }
        if (id == IDCANCEL && code == BN_CLICKED)
        {
            pThis->OnCancel(hWnd);
            return TRUE;
        }
        if (id >= 1300 && id <= 1313 && code == BN_CLICKED)
        {
            bool checked = SendMessage(GetDlgItem(hWnd, id), BM_GETCHECK, 0, 0);
            int idx = id - 1300;
            if (idx >= 0 && idx < 14)
                CViewObjectsExt::BuildingBrushBools[idx] = checked;
        }

        return FALSE;
    }
    case WM_MEASUREITEM:
    {
        VirtualComboBoxEx::SetWindowHeight(hWnd, lParam);
        return TRUE;
    }
    case WM_CLOSE:
    {
        CNewPropertyBuilding* pThis = reinterpret_cast<CNewPropertyBuilding*>(GetWindowLongPtr(hWnd, DWLP_USER));
        if (pThis) pThis->OnCancel(hWnd);
        return TRUE;
    }

    }
    return FALSE;
}

BOOL CNewPropertyBuilding::OnInitDialog(HWND hDlg)
{
    m_hWnd = hDlg;

    FString buffer;

    SetWindowTextA(hDlg, Translations::TranslateOrDefault("StructCap", "Building options"));

    if (Translations::GetTranslationItem("OK", buffer))
        SetWindowTextA(GetDlgItem(hDlg, IDOK), buffer);
    if (Translations::GetTranslationItem("Cancel", buffer))
        SetWindowTextA(GetDlgItem(hDlg, IDCANCEL), buffer);

    TranslateLabels(hDlg);

    HFONT hFont = DarkTheme::GetModernDefaultGUIFont();
    if (hFont)
    {
        SendMessage(GetDlgItem(hDlg, IDOK), WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        SendMessage(GetDlgItem(hDlg, IDCANCEL), WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    }

    HWND hLongDesc = GetDlgItem(hDlg, 1233);
    if (hLongDesc) ShowWindow(hLongDesc, SW_HIDE);

    const char* directions[] = { "0","32","64","96","128","160","192","224" };

    // Strength trackbar (1080)
    HWND hStrength = GetDlgItem(hDlg, 1080);
    if (hStrength)
    {
        SendMessage(hStrength, TBM_SETRANGE, TRUE, MAKELONG(0, 256));
        int pos = 256;
        if (!CString_HealthPoint.IsEmpty())
            pos = atoi(CString_HealthPoint);
        SendMessage(hStrength, TBM_SETPOS, TRUE, pos);
    }

    // Direction combo (1088)
    HWND hDirection = GetDlgItem(hDlg, 1088);
    if (hDirection)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hDirection, nullptr, true);
        for (int i = 0; i < 8; ++i)
            vcb->AddString(directions[i]);
        if (!CString_Direction.IsEmpty())
        {
            int index = vcb->FindStringExact(CString_Direction);
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(CString_Direction);
        }
        m_comboBoxes[hDirection] = std::move(vcb);
    }

    // Rebuild (1084)
    HWND hRebuild = GetDlgItem(hDlg, 1084);
    if (hRebuild)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hRebuild, nullptr, true);
        vcb->AddString("0");
        vcb->AddString("1");
        if (!CString_Rebuildable.IsEmpty())
        {
            int index = vcb->FindStringExact(CString_Rebuildable);
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(CString_Rebuildable);
        }
        m_comboBoxes[hRebuild] = std::move(vcb);
    }

    // Powered (1085)
    HWND hPowered = GetDlgItem(hDlg, 1085);
    if (hPowered)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hPowered, nullptr, true);
        vcb->AddString("0");
        vcb->AddString("1");
        if (!CString_EnergySupport.IsEmpty())
        {
            int index = vcb->FindStringExact(CString_EnergySupport);
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(CString_EnergySupport);
        }
        m_comboBoxes[hPowered] = std::move(vcb);
    }

    // UpgradeCount (1086)
    HWND hUpgradeCount = GetDlgItem(hDlg, 1086);
    if (hUpgradeCount)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hUpgradeCount, nullptr, true);
        vcb->AddString("0");
        vcb->AddString("1");
        vcb->AddString("2");
        vcb->AddString("3");
        if (!CString_UpgradeCount.IsEmpty())
        {
            int index = vcb->FindStringExact(CString_UpgradeCount);
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(CString_UpgradeCount);
        }
        m_comboBoxes[hUpgradeCount] = std::move(vcb);
    }

    // Spotlight (1090)
    HWND hSpotlight = GetDlgItem(hDlg, 1090);
    if (hSpotlight)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hSpotlight, nullptr, true);
        vcb->AddString(Translations::TranslateOrDefault("StructSpotlight.0", "0 - No spotlight"));
        vcb->AddString(Translations::TranslateOrDefault("StructSpotlight.1", "1 - Rules.ini setting"));
        vcb->AddString(Translations::TranslateOrDefault("StructSpotlight.2", "2 - Circle / Direction"));
        if (!CString_Spotlight.IsEmpty())
        {
            int index = vcb->FindStringExactStart(CString_Spotlight);
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(CString_Spotlight);
        }
        m_comboBoxes[hSpotlight] = std::move(vcb);
    }

    // AIRepairs (1093)
    HWND hAIRepairs = GetDlgItem(hDlg, 1093);
    if (hAIRepairs)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hAIRepairs, nullptr, true);
        vcb->AddString("0");
        vcb->AddString("1");
        if (!CString_AIRepairs.IsEmpty())
        {
            int index = vcb->FindStringExact(CString_AIRepairs);
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(CString_AIRepairs);
        }
        m_comboBoxes[hAIRepairs] = std::move(vcb);
    }

    // Nominal (1094)
    HWND hNominal = GetDlgItem(hDlg, 1094);
    if (hNominal)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hNominal, nullptr, true);
        vcb->AddString("0");
        vcb->AddString("1");
        if (!CString_ShowName.IsEmpty())
        {
            int index = vcb->FindStringExact(CString_ShowName);
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(CString_ShowName);
        }
        m_comboBoxes[hNominal] = std::move(vcb);
    }

    // House combo (1079) - VirtualComboBoxEx (Aircraft pattern)
    HWND hHouse = GetDlgItem(hDlg, 1079);
    if (hHouse)
    {
        TempValueHolder tmp(CMapDataExt::IsInitingPropertyDialog, true);
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hHouse, nullptr, false);
        ExtraWindow::LoadParams_Houses(*vcb, CMapData::Instance->IsMultiOnly(), false, ExtConfigs::PlayerAtXForTechnos);
        m_comboBoxes[hHouse] = std::move(vcb);
        if (!CString_House.IsEmpty())
        {
            int index = m_comboBoxes[hHouse]->FindStringExact(Translations::ParseHouseName(CString_House, true));
            if (index != CB_ERR)
                m_comboBoxes[hHouse]->SetCurSel(index);
            else
                m_comboBoxes[hHouse]->SetCurSel(0);
        }
    }

    // Tag combo (1083) - VirtualComboBoxEx (Aircraft pattern)
    HWND hTag = GetDlgItem(hDlg, 1083);
    if (hTag)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hTag, &ExtConfigs::SortByLabelName_Tag, true);
        ExtraWindow::LoadParams_Tags(*vcb, true);
        m_comboBoxes[hTag] = std::move(vcb);

        if (!CString_Tag.IsEmpty())
        {
            int index = CB_ERR;
            if (CString_Tag != "None")
            {
                FString name;
                name.Format("%s - %s", CString_Tag,
                    FString::GetParam(CINI::CurrentDocument->GetString("Tags", CString_Tag, "0,MISSING,01000000"), 1));
                index = m_comboBoxes[hTag]->FindStringExact(name);
            }
            else
            {
                index = m_comboBoxes[hTag]->FindStringExact(CString_Tag);
            }
            if (index != CB_ERR)
                m_comboBoxes[hTag]->SetCurSel(index);
            else
                m_comboBoxes[hTag]->SetEditText(CString_Tag);
        }
    }

    // Sellable (1087) - Edit
    HWND hSellable = GetDlgItem(hDlg, 1087);
    if (hSellable && !CString_Sellable.IsEmpty())
        SetWindowTextA(hSellable, CString_Sellable);

	HWND hUpgrades[3];
    const int nodes[3] = {1089, 1091, 1092};
	for (int idx = 0; idx < 3; ++idx)
	{
        hUpgrades[idx] = GetDlgItem(hDlg, nodes[idx]);
        if (hUpgrades[idx])
        {
            auto vcb = std::make_unique<VirtualComboBoxEx>();
            vcb->Attach(hUpgrades[idx], nullptr, true);
            m_comboBoxes[hUpgrades[idx]] = std::move(vcb);
        }
    }
    if (CViewObjectsExt::InitPropertyDlgFromProperty)
    {
        EnableWindow(hUpgrades[0], TRUE);
        EnableWindow(hUpgrades[1], TRUE);
        EnableWindow(hUpgrades[2], TRUE);
    }
    else
    {
        int nUpgrades = Variables::RulesMap.GetInteger(CString_ObjectID, "Upgrades", 0);
        if (!CString_ObjectID.IsEmpty())
        {
            nUpgrades = std::clamp(nUpgrades, 0, 3); 

            EnableWindow(hUpgrades[0], nUpgrades > 0);
            EnableWindow(hUpgrades[1], nUpgrades > 1);
            EnableWindow(hUpgrades[2], nUpgrades > 2);

            if (nUpgrades > 0)
            {
                const auto& upgrades = CMapDataExt::PowersUpBuildings[CString_ObjectID];
                for (const auto& upgrade : upgrades)
                {
                    const auto UIName = CViewObjectsExt::QueryUIName(upgrade);
                    FString name;
                    name.Format("%s (%s)", upgrade, UIName);

                    for (int i = 0; i < nUpgrades; ++i)
                    {                       
                        m_comboBoxes[hUpgrades[i]]->AddString(name);
                    }
                }
            }

			auto vcb = m_comboBoxes[hUpgrades[0]].get();
			int index = vcb->FindStringExactStart(CString_Upgrade1 + " ");
			if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(CString_Upgrade1);

			vcb = m_comboBoxes[hUpgrades[1]].get();
			index = vcb->FindStringExact(CString_Upgrade2);
			if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(CString_Upgrade2);
                
			vcb = m_comboBoxes[hUpgrades[2]].get();
			index = vcb->FindStringExact(CString_Upgrade3);
			if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(CString_Upgrade3);
        }
    }

    if (!CViewObjectsExt::InitPropertyDlgFromProperty)
    {
        for (int i = 0; i < 14; ++i)
        {
            HWND hCheck = GetDlgItem(hDlg, 1300 + i);
            if (hCheck)
            {
                ShowWindow(hCheck, SW_HIDE);
                EnableWindow(hCheck, FALSE);
            }
        }
    }
    else
    {
        for (int i = 0; i < 14; ++i)
        {
            HWND hCheck = GetDlgItem(hDlg, 1300 + i);
            if (hCheck && CViewObjectsExt::BuildingBrushBools[i])
                SendMessage(hCheck, BM_SETCHECK, BST_CHECKED, 0);
        }
    }

    ExtraWindow::DisableOtherWindows(hDlg);

    return TRUE;
}

void CNewPropertyBuilding::OnOK(HWND hDlg)
{
    CollectResults(hDlg);
    ExtraWindow::RestoreDisabledWindows();
    EndDialog(hDlg, IDOK);
    m_accepted = true;
}

void CNewPropertyBuilding::OnCancel(HWND hDlg)
{
    ExtraWindow::RestoreDisabledWindows();
    EndDialog(hDlg, IDCANCEL);
    m_accepted = false;
}

void CNewPropertyBuilding::CollectResults(HWND hDlg)
{
    char buffer[256];

    HWND hStrength = GetDlgItem(hDlg, 1080);
    if (hStrength)
    {
        int pos = static_cast<int>(SendMessage(hStrength, TBM_GETPOS, 0, 0));
        sprintf(buffer, "%d", pos);
        CString_HealthPoint = buffer;
    }

    // Direction (1088)
    HWND hDirection = GetDlgItem(hDlg, 1088);
    if (hDirection && m_comboBoxes[hDirection])
        CString_Direction = m_comboBoxes[hDirection]->GetSelectedText(true);

    // House (1079) - Aircraft pattern with ParseHouseName
    HWND hHouse = GetDlgItem(hDlg, 1079);
    if (hHouse && m_comboBoxes[hHouse])
        CString_House = Translations::ParseHouseName(m_comboBoxes[hHouse]->GetSelectedText(false), false);

    // Tag (1083) - Aircraft pattern with TrimIndex
    HWND hTag = GetDlgItem(hDlg, 1083);
    if (hTag && m_comboBoxes[hTag])
    {
        CString_Tag = m_comboBoxes[hTag]->GetSelectedText(true);
        STDHelpers::TrimIndex(CString_Tag);
    }

    // Sellable (1087) - Edit, keep GetWindowTextA
    GetWindowTextA(GetDlgItem(hDlg, 1087), buffer, sizeof(buffer)); CString_Sellable = buffer;

    // Rebuild (1084)
    HWND hRebuild = GetDlgItem(hDlg, 1084);
    if (hRebuild && m_comboBoxes[hRebuild])
        CString_Rebuildable = m_comboBoxes[hRebuild]->GetSelectedText(true);

    // Powered (1085)
    HWND hPowered = GetDlgItem(hDlg, 1085);
    if (hPowered && m_comboBoxes[hPowered])
        CString_EnergySupport = m_comboBoxes[hPowered]->GetSelectedText(true);

    // UpgradeCount (1086)
    HWND hUpgradeCount = GetDlgItem(hDlg, 1086);
    if (hUpgradeCount && m_comboBoxes[hUpgradeCount])
        CString_UpgradeCount = m_comboBoxes[hUpgradeCount]->GetSelectedText(true);

    // Spotlight (1090)
    HWND hSpotlight = GetDlgItem(hDlg, 1090);
    if (hSpotlight && m_comboBoxes[hSpotlight])
    {
        CString_Spotlight = m_comboBoxes[hSpotlight]->GetSelectedText(true);
        STDHelpers::TrimIndex(CString_Spotlight);
    }

    // Upgrade1 (1089)
    HWND hUpgrade1 = GetDlgItem(hDlg, 1089);
    if (hUpgrade1 && m_comboBoxes[hUpgrade1])
    {
        CString_Upgrade1 = m_comboBoxes[hUpgrade1]->GetSelectedText(true);
        STDHelpers::TrimIndex(CString_Upgrade1);
    }

    // Upgrade2 (1091)
    HWND hUpgrade2 = GetDlgItem(hDlg, 1091);
    if (hUpgrade2 && m_comboBoxes[hUpgrade2])
    {
        CString_Upgrade2 = m_comboBoxes[hUpgrade2]->GetSelectedText(true);
        STDHelpers::TrimIndex(CString_Upgrade2);
    }

    // Upgrade3 (1092)
    HWND hUpgrade3 = GetDlgItem(hDlg, 1092);
    if (hUpgrade3 && m_comboBoxes[hUpgrade3])
    {
        CString_Upgrade3 = m_comboBoxes[hUpgrade3]->GetSelectedText(true);
        STDHelpers::TrimIndex(CString_Upgrade3);
    }

    // AIRepairs (1093)
    HWND hAIRepairs = GetDlgItem(hDlg, 1093);
    if (hAIRepairs && m_comboBoxes[hAIRepairs])
        CString_AIRepairs = m_comboBoxes[hAIRepairs]->GetSelectedText(true);

    // Nominal (1094)
    HWND hNominal = GetDlgItem(hDlg, 1094);
    if (hNominal && m_comboBoxes[hNominal])
        CString_ShowName = m_comboBoxes[hNominal]->GetSelectedText(true);
}

void CNewPropertyBuilding::TranslateLabels(HWND hDlg)
{
    struct LabelMapping { int id; const char* key; };
    LabelMapping mappings[] = {
        {1258, "StructHouse"},
        {1259, "StructStrength"},
        {1262, "StructDirection"},
        {1263, "StructTag"},
        {1264, "StructP1"},
        {1271, "StructAIRepairs"},
        {1272, "StructEnergy"},
        {1273, "StructUpgradeCount"},
        {1274, "StructSpotlight"},
        {1275, "StructUpgrade1"},
        {1276, "StructUpgrade2"},
        {1277, "StructUpgrade3"},
        {1265, "StructP2"},
        {1266, "StructP3"},
    };

    FString buffer;
    for (const auto& m : mappings)
    {
        HWND hLabel = GetDlgItem(hDlg, m.id);
        if (hLabel && Translations::GetTranslationItem(m.key, buffer))
            SetWindowTextA(hLabel, buffer);
    }
}

