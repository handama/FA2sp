#include "CNewMMXSavingOptionsDlg.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../Common.h"

CNewMMXSavingOptionsDlg::CNewMMXSavingOptionsDlg(CWnd* pParent /*=NULL*/)
	: ppmfc::CDialog(327, pParent)
{
	m_Description = "";
	//m_CustomModes = "";
	m_Maxplayers = "";
	m_MinPlayers = "";
	//m_AirWar = 0;
	//m_Cooperative = 0;
	//m_Duel = 0;
	//m_Meatgrind = 0;
	//m_MegaWealth = 0;
	//m_NavalWar = 0;
	//m_NukeWar = 0;
	//m_Standard = 0;
	//m_TeamGame = 0;
}

void CNewMMXSavingOptionsDlg::DoDataExchange(ppmfc::CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Text(pDX, 1407, m_Description);
	DDX_Text(pDX, 1435, m_MinPlayers);
	DDX_Text(pDX, 1436, m_Maxplayers);
	//DDX_Text(pDX, 1437, m_CustomModes);

	//DDX_Check(pDX, 1426, m_Standard);
	//DDX_Check(pDX, 1427, m_Meatgrind);
	//DDX_Check(pDX, 1428, m_NavalWar);
	//DDX_Check(pDX, 1429, m_NukeWar);
	//DDX_Check(pDX, 1430, m_AirWar);
	//DDX_Check(pDX, 1431, m_MegaWealth);
	//DDX_Check(pDX, 1432, m_Duel);
	//DDX_Check(pDX, 1433, m_Cooperative);
	//DDX_Check(pDX, 1434, m_TeamGame);

	FString buffer;

	if (Translations::GetTranslationItem("OK", buffer))
		GetDlgItem(1)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("Cancel", buffer))
		GetDlgItem(2)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("NewMMXSavingOptionsDlgDesc", buffer))
		GetDlgItem(1000)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("NewMMXSavingOptionsDlgCSFLabel", buffer))
		GetDlgItem(1001)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("NewMMXSavingOptionsDlgMinPlayer", buffer))
		GetDlgItem(1002)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("NewMMXSavingOptionsDlgMaxPlayer", buffer))
		GetDlgItem(1003)->SetWindowTextA(buffer);
	
	if (Translations::GetTranslationItem("NewMMXSavingOptionsDlgCaption", buffer))
		SetWindowTextA(buffer);
}

void CNewMMXSavingOptionsDlg::OnOK()
{
	UpdateData(TRUE);
	EndDialog(0);
}

void CNewMMXSavingOptionsDlg::OnCancel()
{
	EndDialog(2);
}