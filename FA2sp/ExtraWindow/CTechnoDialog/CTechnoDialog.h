#pragma once

#include <vector>
#include "FA2PP.h"
#include "CObjectDatas.h"
#include "../../FA2sp/Helpers/FString.h"
#include "../../Helpers/MultimapHelper.h"
#include "../../Ext/CLoading/Body.h"

class CTechnoDialog : public ppmfc::CDialog
{
public:

	CTechnoDialog(CWnd* pParent = NULL);
	void ApplyToUnitData(CUnitData* pUnit);
	void ApplyToInfantryData(CInfantryData* pInf);
	void ApplyToAircraftData(CAircraftData* pAir);
	void ApplyToBuildingData(CBuildingData* pBld);
	void ApplyToCurrentTechno(void* pData, CLoadingExt::ObjectType type);
	bool HasAnyEnabledItem() const;

	ppmfc::CString m_strHouse;
	ppmfc::CString m_strDirection;
	ppmfc::CString m_strState;
	ppmfc::CString m_strVeteranLevel;
	ppmfc::CString m_strGroup;
	ppmfc::CString m_strAboveGround;
	ppmfc::CString m_strFollowsIndex;
	ppmfc::CString m_strAutoNO_Recruit;
	ppmfc::CString m_strAutoYES_Recruit;
	ppmfc::CString m_strTag;
	ppmfc::CString m_strSellable;
	ppmfc::CString m_strRebuild;
	ppmfc::CString m_strPowered;
	ppmfc::CString m_strUpgradeCount;
	ppmfc::CString m_strSpotlight;
	ppmfc::CString m_strUpgrade1;
	ppmfc::CString m_strUpgrade2;
	ppmfc::CString m_strUpgrade3;
	ppmfc::CString m_strAIRepairs;
	ppmfc::CString m_strNominal;

	int     m_nStrength;

	BOOL    m_bEnableHouse;
	BOOL    m_bEnableStrength;
	BOOL    m_bEnableDirection;
	BOOL    m_bEnableState;
	BOOL    m_bEnableVeteran;
	BOOL    m_bEnableGroup;
	BOOL    m_bEnableAboveGround;
	BOOL    m_bEnableFollowsIndex;
	BOOL    m_bEnableAutoNO;
	BOOL    m_bEnableAutoYES;
	BOOL    m_bEnableTag;
	BOOL    m_bEnableSellable;
	BOOL    m_bEnableRebuild;
	BOOL    m_bEnablePowered;
	BOOL    m_bEnableUpgradeCount;
	BOOL    m_bEnableSpotlight;
	BOOL    m_bEnableUpgrade1;
	BOOL    m_bEnableUpgrade2;
	BOOL    m_bEnableUpgrade3;
	BOOL    m_bEnableAIRepairs;
	BOOL    m_bEnableNominal;

protected:

	HWND m_hHouse;          // 1001
	HWND m_hStrength;       // 1003 trackbar
	HWND m_hDirection;      // 1005
	HWND m_hState;          // 1007
	HWND m_hVeteranLevel;   // 1009 edit
	HWND m_hGroup;          // 1011 edit
	HWND m_hAboveGround;    // 1013 edit
	HWND m_hFollowsIndex;   // 1015 edit
	HWND m_hAutoNO;         // 1017 edit
	HWND m_hAutoYES;        // 1019 edit
	HWND m_hTag;            // 1021

	HWND m_hSellable;       // 1023 edit
	HWND m_hRebuild;        // 1025
	HWND m_hPowered;        // 1027
	HWND m_hUpgradeCount;   // 1029
	HWND m_hSpotlight;      // 1031
	HWND m_hUpgrade1;       // 1033 edit
	HWND m_hUpgrade2;       // 1035 edit
	HWND m_hUpgrade3;       // 1037 edit
	HWND m_hAIRepairs;      // 1039
	HWND m_hNominal;        // 1041

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(ppmfc::CDataExchange* pDX);
	virtual void OnOK();
	virtual void OnCancel();
	virtual void OnClose();
};
