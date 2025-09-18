#pragma once

#include <vector>
#include "FA2PP.h"
#include "../../FA2sp/Helpers/FString.h"
#include "../../Helpers/MultimapHelper.h"

#define COMBOUINPUT_HOUSES 0
#define COMBOUINPUT_COUNTRIES 1
#define COMBOUINPUT_TRIGGERS 2
#define COMBOUINPUT_TAGS 3
#define COMBOUINPUT_HOUSES_N 4
#define COMBOUINPUT_COUNTRIES_N 5
#define COMBOUINPUT_MANUAL 6
#define COMBOUINPUT_ALL_CUSTOM 7

class CNewMMXSavingOptionsDlg : public ppmfc::CDialog
{
public:
	CNewMMXSavingOptionsDlg(CWnd* pParent = NULL);
	ppmfc::CString	m_Description;
	//ppmfc::CString	m_CustomModes;
	ppmfc::CString	m_Maxplayers;
	ppmfc::CString	m_MinPlayers;
	//BOOL	m_AirWar;
	//BOOL	m_Cooperative;
	//BOOL	m_Duel;
	//BOOL	m_Meatgrind;
	//BOOL	m_MegaWealth;
	//BOOL	m_NavalWar;
	//BOOL	m_NukeWar;
	//BOOL	m_Standard;
	//BOOL	m_TeamGame;

protected:
	virtual void DoDataExchange(ppmfc::CDataExchange* pDX);
	virtual void OnOK();
	virtual void OnCancel();

};
