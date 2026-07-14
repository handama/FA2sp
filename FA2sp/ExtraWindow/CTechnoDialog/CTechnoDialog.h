#pragma once

#include <vector>
#include <memory>
#include <map>
#include "FA2PP.h"
#include "CObjectDatas.h"
#include "../../FA2sp/Helpers/FString.h"
#include "../../Helpers/MultimapHelper.h"
#include "../../Ext/CLoading/Body.h"
#include "../Common.h"

class CTechnoDialog
{
public:
    enum { IDD = 336 };

    CTechnoDialog();
    ~CTechnoDialog();

    void ApplyToUnitData(CUnitData* pUnit);
    void ApplyToInfantryData(CInfantryData* pInf);
    void ApplyToAircraftData(CAircraftData* pAir);
    void ApplyToBuildingData(CBuildingData* pBld);
    void ApplyToCurrentTechno(void* pData, CLoadingExt::GameObjectType type);
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

    bool DoModal();

protected:
    static BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
    BOOL OnInitDialog(HWND hDlg);
    void CollectDataToMembers(HWND hDlg);
    void OnOK(HWND hDlg);
    void OnCancel(HWND hDlg);

    HWND m_hWnd = nullptr;
    bool m_accepted = false;

    std::map<HWND, std::unique_ptr<VirtualComboBoxEx>> m_comboBoxes;
};