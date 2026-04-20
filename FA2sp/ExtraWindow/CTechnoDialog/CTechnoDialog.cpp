#include "CTechnoDialog.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../Common.h"
#include "../../Ext/CLoading/Body.h"
#include <CFinalSunApp.h>
#include <Miscs/Miscs.LoadParams.h>
#include "../../Ext/CMapData/Body.h"

CTechnoDialog::CTechnoDialog(CWnd* pParent /*=NULL*/)
	: ppmfc::CDialog(336, pParent)
{

}

void CTechnoDialog::DoDataExchange(ppmfc::CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_CBString(pDX, 1001, m_strHouse);
    //DDX_Slider(pDX, 1003, m_nStrength); 
    DDX_CBString(pDX, 1005, m_strDirection);
    DDX_CBString(pDX, 1007, m_strState);
    DDX_Text(pDX, 1009, m_strVeteranLevel);
    DDX_Text(pDX, 1011, m_strGroup);
    DDX_Text(pDX, 1013, m_strAboveGround);
    DDX_Text(pDX, 1015, m_strFollowsIndex);
    DDX_Text(pDX, 1017, m_strAutoNO_Recruit);
    DDX_Text(pDX, 1019, m_strAutoYES_Recruit);
    DDX_CBString(pDX, 1021, m_strTag);

    DDX_Text(pDX, 1023, m_strSellable);
    DDX_CBString(pDX, 1025, m_strRebuild);
    DDX_CBString(pDX, 1027, m_strPowered);
    DDX_CBString(pDX, 1029, m_strUpgradeCount);
    DDX_CBString(pDX, 1031, m_strSpotlight);
    DDX_Text(pDX, 1033, m_strUpgrade1);
    DDX_Text(pDX, 1035, m_strUpgrade2);
    DDX_Text(pDX, 1037, m_strUpgrade3);
    DDX_CBString(pDX, 1039, m_strAIRepairs);
    DDX_CBString(pDX, 1041, m_strNominal);

    DDX_Check(pDX, 1300, m_bEnableHouse);
    DDX_Check(pDX, 1301, m_bEnableStrength);
    DDX_Check(pDX, 1302, m_bEnableDirection);
    DDX_Check(pDX, 1303, m_bEnableState);
    DDX_Check(pDX, 1304, m_bEnableVeteran);
    DDX_Check(pDX, 1305, m_bEnableGroup);
    DDX_Check(pDX, 1306, m_bEnableAboveGround);
    DDX_Check(pDX, 1307, m_bEnableFollowsIndex);
    DDX_Check(pDX, 1308, m_bEnableAutoNO);
    DDX_Check(pDX, 1309, m_bEnableAutoYES);
    DDX_Check(pDX, 1310, m_bEnableTag);

    DDX_Check(pDX, 1311, m_bEnableSellable);
    DDX_Check(pDX, 1312, m_bEnableRebuild);
    DDX_Check(pDX, 1313, m_bEnablePowered);
    DDX_Check(pDX, 1314, m_bEnableUpgradeCount);
    DDX_Check(pDX, 1315, m_bEnableSpotlight);
    DDX_Check(pDX, 1316, m_bEnableUpgrade1);
    DDX_Check(pDX, 1317, m_bEnableUpgrade2);
    DDX_Check(pDX, 1318, m_bEnableUpgrade3);
    DDX_Check(pDX, 1319, m_bEnableAIRepairs);
    DDX_Check(pDX, 1320, m_bEnableNominal);
}

BOOL CTechnoDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	FString buffer;

	if (Translations::GetTranslationItem("OK", buffer))
		GetDlgItem(1)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("Cancel", buffer))
		GetDlgItem(2)->SetWindowTextA(buffer);

    struct { int id; const char* key; } labels[] = {
        {1000, "StructHouse"},
        {1002, "StructStrength"},
        {1004, "StructDirection"},
        {1006, "UnitState"},
        {1008, "UnitP1"},
        {1010, "UnitP2"},
        {1012, "UnitP3"},
        {1014, "UnitP4"},
        {1016, "UnitP5"},
        {1018, "UnitP6"},
        {1020, "StructTag"},
        {1022, "StructP1"},
        {1024, "StructAIRepairs"},
        {1026, "StructEnergy"},
        {1028, "StructUpgradeCount"},
        {1030, "StructSpotlight"},
        {1032, "StructUpgrade1"},
        {1034, "StructUpgrade2"},
        {1036, "StructUpgrade3"},
        {1038, "StructP2"},
        {1040, "StructP3"},
    };

    for (const auto& item : labels)
    {
        if (Translations::GetTranslationItem(item.key, buffer))
        {
            GetDlgItem(item.id)->SetWindowTextA(buffer);
        }
    }

	if (Translations::GetTranslationItem("TechnoOptionsCaption", buffer))
		SetWindowTextA(buffer);

    m_hHouse = ::GetDlgItem(m_hWnd, 1001);
    m_hStrength = ::GetDlgItem(m_hWnd, 1003);
    m_hDirection = ::GetDlgItem(m_hWnd, 1005);
    m_hState = ::GetDlgItem(m_hWnd, 1007);
    m_hVeteranLevel = ::GetDlgItem(m_hWnd, 1009);
    m_hGroup = ::GetDlgItem(m_hWnd, 1011);
    m_hAboveGround = ::GetDlgItem(m_hWnd, 1013);
    m_hFollowsIndex = ::GetDlgItem(m_hWnd, 1015);
    m_hAutoNO = ::GetDlgItem(m_hWnd, 1017);
    m_hAutoYES = ::GetDlgItem(m_hWnd, 1019);
    m_hTag = ::GetDlgItem(m_hWnd, 1021);

    m_hSellable = ::GetDlgItem(m_hWnd, 1023);
    m_hRebuild = ::GetDlgItem(m_hWnd, 1025);
    m_hPowered = ::GetDlgItem(m_hWnd, 1027);
    m_hUpgradeCount = ::GetDlgItem(m_hWnd, 1029);
    m_hSpotlight = ::GetDlgItem(m_hWnd, 1031);
    m_hUpgrade1 = ::GetDlgItem(m_hWnd, 1033);
    m_hUpgrade2 = ::GetDlgItem(m_hWnd, 1035);
    m_hUpgrade3 = ::GetDlgItem(m_hWnd, 1037);
    m_hAIRepairs = ::GetDlgItem(m_hWnd, 1039);
    m_hNominal = ::GetDlgItem(m_hWnd, 1041);

    if (m_hStrength)
    {
        ::SendMessage(m_hStrength, TBM_SETRANGE, TRUE, MAKELONG(0, 256));
        ::SendMessage(m_hStrength, TBM_SETPOS, TRUE, 256); 
    }

    auto AddStrings = [&](HWND hCombo, const char* const items[], int count)
    {
        if (!hCombo) return;
        ::SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
        for (int i = 0; i < count; ++i)
        {
            ::SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)items[i]);
        }
    };

    const char* yesNo[] = { "0", "1" };
    const char* upgradeCount[] = { "0", "1" , "2" , "3" };
    const char* directions[] = { "0","32","64","96","128","160","192","224" };

    if (m_hDirection)   AddStrings(m_hDirection, directions, _countof(directions));
    if (m_hRebuild)     AddStrings(m_hRebuild, yesNo, 2);
    if (m_hPowered)     AddStrings(m_hPowered, yesNo, 2);
    if (m_hAIRepairs)   AddStrings(m_hAIRepairs, yesNo, 2);
    if (m_hNominal)     AddStrings(m_hNominal, yesNo, 2);
    if (m_hUpgradeCount)AddStrings(m_hUpgradeCount, upgradeCount, 4);

    if (m_hSpotlight)
    {
        ::SendMessage(m_hSpotlight, CB_INSERTSTRING, 0,
            Translations::TranslateOrDefault("StructSpotlight.0", "0 - No spotlight"));
        ::SendMessage(m_hSpotlight, CB_INSERTSTRING, 1,
            Translations::TranslateOrDefault("StructSpotlight.1", "1 - Rules.ini setting"));
        ::SendMessage(m_hSpotlight, CB_INSERTSTRING, 2,
            Translations::TranslateOrDefault("StructSpotlight.2", "2 - Circle / Direction"));
    }

    auto cbbHouse = reinterpret_cast<ppmfc::CComboBox*>(ppmfc::CWnd::FromHandle(m_hHouse));
    auto cbbTag = reinterpret_cast<ppmfc::CComboBox*>(ppmfc::CWnd::FromHandle(m_hTag));
    if (!CMapData::Instance->IsMultiOnly())
    {
        Miscs::LoadParams::Houses(cbbHouse, false, false, false);
    }
    else
    {
        if (ExtConfigs::PlayerAtXForTechnos)
            Miscs::LoadParams::Houses(cbbHouse, false, false, true);
        else
            Miscs::LoadParams::Houses(cbbHouse, false, false, false);
    }
    Miscs::LoadParams::Tags(cbbTag, true);

    if (m_hState)
    {
        ExtraWindow::ClearComboKeepText(m_hState);

        for (int i = 0; i < CMapDataExt::TechnoStates.size(); ++i)
        {
            const auto& state = CMapDataExt::TechnoStates[i];
            FString key = "FootClassStatus.";
            key += state;
            ::SendMessage(m_hState, CB_INSERTSTRING, i, 
                Translations::TranslateOrDefault(key, state));
        }
    }

	return TRUE;
}

void CTechnoDialog::ApplyToUnitData(CUnitData* pUnit)
{
    if (!pUnit) return;

    if (m_bEnableHouse && !m_strHouse.IsEmpty())
        pUnit->House = m_strHouse;

    if (m_bEnableStrength)
    {
        pUnit->Health.Format("%d", m_nStrength);
    }

    if (m_bEnableDirection && !m_strDirection.IsEmpty())
        pUnit->Facing = m_strDirection;

    if (m_bEnableState && !m_strState.IsEmpty())
        pUnit->Status = m_strState;

    if (m_bEnableTag && !m_strTag.IsEmpty())
        pUnit->Tag = m_strTag;

    if (m_bEnableVeteran && !m_strVeteranLevel.IsEmpty())
        pUnit->VeterancyPercentage = m_strVeteranLevel;

    if (m_bEnableGroup && !m_strGroup.IsEmpty())
        pUnit->Group = m_strGroup;

    if (m_bEnableAboveGround && !m_strAboveGround.IsEmpty())
        pUnit->IsAboveGround = m_strAboveGround;

    if (m_bEnableFollowsIndex && !m_strFollowsIndex.IsEmpty())
        pUnit->FollowsIndex = m_strFollowsIndex;

    if (m_bEnableAutoNO && !m_strAutoNO_Recruit.IsEmpty())
        pUnit->AutoNORecruitType = m_strAutoNO_Recruit;

    if (m_bEnableAutoYES && !m_strAutoYES_Recruit.IsEmpty())
        pUnit->AutoYESRecruitType = m_strAutoYES_Recruit;
}

void CTechnoDialog::ApplyToInfantryData(CInfantryData* pInf)
{
    if (!pInf) return;

    if (m_bEnableHouse && !m_strHouse.IsEmpty())
        pInf->House = m_strHouse;

    if (m_bEnableStrength)
    {
        pInf->Health.Format("%d", m_nStrength);
    }

    if (m_bEnableDirection && !m_strDirection.IsEmpty())
        pInf->Facing = m_strDirection;

    if (m_bEnableState && !m_strState.IsEmpty())
        pInf->Status = m_strState;

    if (m_bEnableTag && !m_strTag.IsEmpty())
        pInf->Tag = m_strTag;

    if (m_bEnableVeteran && !m_strVeteranLevel.IsEmpty())
        pInf->VeterancyPercentage = m_strVeteranLevel;

    if (m_bEnableGroup && !m_strGroup.IsEmpty())
        pInf->Group = m_strGroup;

    if (m_bEnableAboveGround && !m_strAboveGround.IsEmpty())
        pInf->IsAboveGround = m_strAboveGround;

    if (m_bEnableAutoNO && !m_strAutoNO_Recruit.IsEmpty())
        pInf->AutoNORecruitType = m_strAutoNO_Recruit;

    if (m_bEnableAutoYES && !m_strAutoYES_Recruit.IsEmpty())
        pInf->AutoYESRecruitType = m_strAutoYES_Recruit;
}

void CTechnoDialog::ApplyToBuildingData(CBuildingData* pBld)
{
    if (!pBld) return;

    if (m_bEnableHouse && !m_strHouse.IsEmpty())
        pBld->House = m_strHouse;

    if (m_bEnableStrength)
    {
        pBld->Health.Format("%d", m_nStrength);
    }

    if (m_bEnableDirection && !m_strDirection.IsEmpty())
        pBld->Facing = m_strDirection;

    if (m_bEnableTag && !m_strTag.IsEmpty())
        pBld->Tag = m_strTag;

    if (m_bEnableSellable && !m_strSellable.IsEmpty())
        pBld->AISellable = m_strSellable;

    if (m_bEnableRebuild && !m_strRebuild.IsEmpty())
        pBld->AIRebuildable = m_strRebuild;

    if (m_bEnablePowered && !m_strPowered.IsEmpty())
        pBld->PoweredOn = m_strPowered;

    if (m_bEnableUpgradeCount && !m_strUpgradeCount.IsEmpty())
        pBld->Upgrades = m_strUpgradeCount;

    if (m_bEnableSpotlight && !m_strSpotlight.IsEmpty())
        pBld->SpotLight = m_strSpotlight;

    if (m_bEnableUpgrade1 && !m_strUpgrade1.IsEmpty())
        pBld->Upgrade1 = m_strUpgrade1;

    if (m_bEnableUpgrade2 && !m_strUpgrade2.IsEmpty())
        pBld->Upgrade2 = m_strUpgrade2;

    if (m_bEnableUpgrade3 && !m_strUpgrade3.IsEmpty())
        pBld->Upgrade3 = m_strUpgrade3;

    if (m_bEnableAIRepairs && !m_strAIRepairs.IsEmpty())
        pBld->AIRepairable = m_strAIRepairs;

    if (m_bEnableNominal && !m_strNominal.IsEmpty())
        pBld->Nominal = m_strNominal;
}

void CTechnoDialog::ApplyToAircraftData(CAircraftData* pAir)
{
    if (!pAir) return;

    if (m_bEnableHouse && !m_strHouse.IsEmpty())
        pAir->House = m_strHouse;

    if (m_bEnableStrength)
    {
        pAir->Health.Format("%d", m_nStrength);
    }

    if (m_bEnableDirection && !m_strDirection.IsEmpty())
        pAir->Facing = m_strDirection;

    if (m_bEnableState && !m_strState.IsEmpty())
        pAir->Status = m_strState;

    if (m_bEnableTag && !m_strTag.IsEmpty())
        pAir->Tag = m_strTag;

    if (m_bEnableVeteran && !m_strVeteranLevel.IsEmpty())
        pAir->VeterancyPercentage = m_strVeteranLevel;

    if (m_bEnableGroup && !m_strGroup.IsEmpty())
        pAir->Group = m_strGroup;

    if (m_bEnableAutoNO && !m_strAutoNO_Recruit.IsEmpty())
        pAir->AutoNORecruitType = m_strAutoNO_Recruit;

    if (m_bEnableAutoYES && !m_strAutoYES_Recruit.IsEmpty())
        pAir->AutoYESRecruitType = m_strAutoYES_Recruit;
}

void CTechnoDialog::ApplyToCurrentTechno(void* pData, CLoadingExt::ObjectType type)
{
    switch (type)
    {
    case  CLoadingExt::ObjectType::Infantry:
        ApplyToInfantryData(static_cast<CInfantryData*>(pData));
        break;
    case  CLoadingExt::ObjectType::Vehicle:
        ApplyToUnitData(static_cast<CUnitData*>(pData));
        break;
    case  CLoadingExt::ObjectType::Aircraft:
        ApplyToAircraftData(static_cast<CAircraftData*>(pData));
        break;
    case  CLoadingExt::ObjectType::Building:
        ApplyToBuildingData(static_cast<CBuildingData*>(pData));
        break;
    default:
        break;
    }
}

void CTechnoDialog::OnOK()
{
	UpdateData(TRUE);

    m_nStrength = (int)::SendMessage(m_hStrength, TBM_GETPOS, 0, 0);
    int idx = ::SendMessage(m_hState, CB_GETCURSEL, NULL, NULL);
    if (idx < CMapDataExt::TechnoStates.size() && idx >= 0)
    {
        m_strState = CMapDataExt::TechnoStates[idx];
    }
    m_strHouse = Translations::ParseHouseName(m_strHouse, false);
    STDHelpers::TrimIndex(m_strSpotlight);
    STDHelpers::TrimIndex(m_strTag);
	EndDialog(IDOK);
}

void CTechnoDialog::OnCancel()
{
	EndDialog(IDCANCEL);
}

void CTechnoDialog::OnClose()
{
    OnCancel();
}
