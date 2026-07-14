#pragma once
#include <FA2PP.h>
#include <string>
#include <memory>
#include "../Common.h"

class CNewPropertyAircraft
{
public:
    enum { IDD = 346 };

    CNewPropertyAircraft();
    ~CNewPropertyAircraft();

    ppmfc::CString CString_HealthPoint;
    ppmfc::CString CString_Direction;
    ppmfc::CString CString_House;
    ppmfc::CString CString_Tag;
    ppmfc::CString CString_State;
    ppmfc::CString CString_Group;
    ppmfc::CString CString_VeteranLevel;
    ppmfc::CString CString_AutoCreateNoRecruitable;
    ppmfc::CString CString_AutoCreateYesRecruitable;

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