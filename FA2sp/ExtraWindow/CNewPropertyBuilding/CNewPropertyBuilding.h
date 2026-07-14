#pragma once
#include <FA2PP.h>
#include <string>
#include <memory>
#include "../Common.h"

class CNewPropertyBuilding
{
public:
    enum { IDD = 347 };

    CNewPropertyBuilding();
    ~CNewPropertyBuilding();

    ppmfc::CString CString_HealthPoint;
    ppmfc::CString CString_Direction;
    ppmfc::CString CString_House;
    ppmfc::CString CString_Tag;
    ppmfc::CString CString_Sellable;
    ppmfc::CString CString_Rebuildable;
    ppmfc::CString CString_EnergySupport;
    ppmfc::CString CString_UpgradeCount;
    ppmfc::CString CString_Spotlight;
    ppmfc::CString CString_Upgrade1;
    ppmfc::CString CString_Upgrade2;
    ppmfc::CString CString_Upgrade3;
    ppmfc::CString CString_AIRepairs;
    ppmfc::CString CString_ShowName;
    ppmfc::CString CString_ObjectID;

    bool DoModal();

protected:
    static BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
    BOOL OnInitDialog(HWND hDlg);
    void OnOK(HWND hDlg);
    void OnCancel(HWND hDlg);
    void CollectResults(HWND hDlg);
    void TranslateLabels(HWND hDlg);

    bool m_accepted = false;
    HWND m_hWnd = nullptr;

    std::map<HWND, std::unique_ptr<VirtualComboBoxEx>> m_comboBoxes;
};