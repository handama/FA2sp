#include "CNewPropertyInfantry.h"
#include <CFinalSunDlg.h>
#include "../../FA2sp.h"
#include "../../Ext/CFinalSunApp/Body.h"
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CMapData/Body.h"
#include "../../Miscs/DialogStyle.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/Helper.h"

CNewPropertyInfantry::CNewPropertyInfantry()
{
}

CNewPropertyInfantry::~CNewPropertyInfantry()
{
}

bool CNewPropertyInfantry::DoModal()
{
    DialogBoxParam(
        reinterpret_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(IDD),
        CFinalSunDlg::Instance->GetSafeHwnd(),
        CNewPropertyInfantry::DlgProc,
        reinterpret_cast<LPARAM>(this)
    );

    m_comboBoxes.clear();
    return m_accepted;
}

BOOL CALLBACK CNewPropertyInfantry::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CNewPropertyInfantry* pThis = reinterpret_cast<CNewPropertyInfantry*>(lParam);
        SetWindowLongPtr(hWnd, DWLP_USER, reinterpret_cast<LONG_PTR>(pThis));
        return pThis->OnInitDialog(hWnd);
    }
    case WM_COMMAND:
    {
        CNewPropertyInfantry* pThis = reinterpret_cast<CNewPropertyInfantry*>(GetWindowLongPtr(hWnd, DWLP_USER));
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

        if (id >= 1300 && id <= 1309 && code == BN_CLICKED)
        {
            bool checked = SendMessage(GetDlgItem(hWnd, id), BM_GETCHECK, 0, 0);
            int idx = id - 1300;
            if (idx >= 0 && idx < 10)
                CViewObjectsExt::InfantryBrushBools[idx] = checked;
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
        CNewPropertyInfantry* pThis = reinterpret_cast<CNewPropertyInfantry*>(GetWindowLongPtr(hWnd, DWLP_USER));
        if (pThis) pThis->OnCancel(hWnd);
        return TRUE;
    }

    }
    return FALSE;
}

BOOL CNewPropertyInfantry::OnInitDialog(HWND hDlg)
{
    m_hWnd = hDlg;

    FString buffer;

    SetWindowTextA(hDlg, Translations::TranslateOrDefault("InfCap", "Infantry options"));

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
        const char* directions[] = { "0","32","64","96","128","160","192","224" };
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

    // State combo (1082)
    HWND hState = GetDlgItem(hDlg, 1082);
    if (hState)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hState, nullptr, false);
        for (int i = 0; i < CMapDataExt::TechnoStates.size(); ++i)
        {
            const auto& state = CMapDataExt::TechnoStates[i];
            FString key = "FootClassStatus.";
            key += state;
            vcb->AddString(Translations::TranslateOrDefault(key, state));
        }
        if (!CString_State.IsEmpty())
        {
            FString key = "FootClassStatus.";
            key += CString_State;
            int index = vcb->FindStringExact(Translations::TranslateOrDefault(key, CString_State));
            if (index != CB_ERR)
                vcb->SetCurSel(index);
        }
        m_comboBoxes[hState] = std::move(vcb);
    }

    // House combo (1079) - VirtualComboBoxEx
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
            {
                m_comboBoxes[hHouse]->SetCurSel(index);
            }
            else
            {
                m_comboBoxes[hHouse]->SetCurSel(0);
            }
		}
    }

    // Tag combo (1083) - VirtualComboBoxEx
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
            {
                m_comboBoxes[hTag]->SetCurSel(index);
            }
            else
            {
                m_comboBoxes[hTag]->SetEditText(CString_Tag);
            }
		}
    }

    // Edit controls
    HWND hGroup = GetDlgItem(hDlg, 1084);       // Group
    HWND hAbove = GetDlgItem(hDlg, 1085);        // AboveGround
    HWND hAutoNO = GetDlgItem(hDlg, 1086);       // AutoNORecruitType
    HWND hVeteran = GetDlgItem(hDlg, 1087);      // VeteranLevel
    HWND hAutoYES = GetDlgItem(hDlg, 1090);      // AutoYESRecruitType

    if (hGroup && !CString_Group.IsEmpty())
        SetWindowTextA(hGroup, CString_Group);
    if (hAbove && !CString_OnBridge.IsEmpty())
        SetWindowTextA(hAbove, CString_OnBridge);
    if (hAutoNO && !CString_AutoCreateNoRecruitable.IsEmpty())
        SetWindowTextA(hAutoNO, CString_AutoCreateNoRecruitable);
    if (hVeteran && !CString_VerteranStatus.IsEmpty())
        SetWindowTextA(hVeteran, CString_VerteranStatus);
    if (hAutoYES && !CString_AutoCreateYesRecruitable.IsEmpty())
        SetWindowTextA(hAutoYES, CString_AutoCreateYesRecruitable);

    if (!CViewObjectsExt::InitPropertyDlgFromProperty)
    {
        for (int i = 0; i < 10; ++i)
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
        for (int i = 0; i < 10; ++i)
        {
            HWND hCheck = GetDlgItem(hDlg, 1300 + i);
            if (hCheck && CViewObjectsExt::InfantryBrushBools[i])
                SendMessage(hCheck, BM_SETCHECK, BST_CHECKED, 0);
        }
    }

    ExtraWindow::DisableOtherWindows(hDlg);

    return TRUE;
}

void CNewPropertyInfantry::OnOK(HWND hDlg)
{
    CollectResults(hDlg);
    ExtraWindow::RestoreDisabledWindows();
    EndDialog(hDlg, IDOK);
    m_accepted = true;
}

void CNewPropertyInfantry::OnCancel(HWND hDlg)
{
    ExtraWindow::RestoreDisabledWindows();
    EndDialog(hDlg, IDCANCEL);
    m_accepted = false;
}

void CNewPropertyInfantry::CollectResults(HWND hDlg)
{
    char buffer[256];

    HWND hStrength = GetDlgItem(hDlg, 1080);
    if (hStrength)
    {
        int pos = static_cast<int>(SendMessage(hStrength, TBM_GETPOS, 0, 0));
        sprintf(buffer, "%d", pos);
        CString_HealthPoint = buffer;
    }

    HWND hDirection = GetDlgItem(hDlg, 1088);
    if (hDirection && m_comboBoxes[hDirection])
    {
        CString_Direction = m_comboBoxes[hDirection]->GetSelectedText(true);
    }

    // House
    HWND hHouse = GetDlgItem(hDlg, 1079);
    if (hHouse && m_comboBoxes[hHouse])
    {
        CString_House = Translations::ParseHouseName(m_comboBoxes[hHouse]->GetSelectedText(false), false);
    }

    // Tag
    HWND hTag = GetDlgItem(hDlg, 1083);
    if (hTag && m_comboBoxes[hTag])
    {
        CString_Tag = m_comboBoxes[hTag]->GetSelectedText(true);
		STDHelpers::TrimIndex(CString_Tag);
	}
    HWND hState = GetDlgItem(hDlg, 1082);
    if (hState && m_comboBoxes[hState])
    {
        int index = m_comboBoxes[hState]->GetCurSel();
        if (index >= 0 && index < CMapDataExt::TechnoStates.size())
        {
            CString_State = CMapDataExt::TechnoStates[index];
        }
        else
        {
            CString_State = m_comboBoxes[hState]->GetSelectedText(false);
        }
    }
    GetWindowTextA(GetDlgItem(hDlg, 1084), buffer, sizeof(buffer)); CString_Group = buffer;
    GetWindowTextA(GetDlgItem(hDlg, 1085), buffer, sizeof(buffer)); CString_OnBridge = buffer;
    GetWindowTextA(GetDlgItem(hDlg, 1086), buffer, sizeof(buffer)); CString_AutoCreateNoRecruitable = buffer;
    GetWindowTextA(GetDlgItem(hDlg, 1087), buffer, sizeof(buffer)); CString_VerteranStatus = buffer;
    GetWindowTextA(GetDlgItem(hDlg, 1090), buffer, sizeof(buffer)); CString_AutoCreateYesRecruitable = buffer;
}

void CNewPropertyInfantry::TranslateLabels(HWND hDlg)
{
    struct LabelMapping { int id; const char* key; };
    LabelMapping mappings[] = {
        {1258, "StructHouse"},
        {1259, "StructStrength"},
        {1261, "UnitState"},
        {1262, "StructDirection"},
        {1263, "StructTag"},
        {1264, "UnitP1"},
        {1265, "UnitP2"},
        {1266, "UnitP3"},
        {1267, "UnitP5"},
        {1268, "UnitP6"},
    };

    FString buffer;
    for (const auto& m : mappings)
    {
        HWND hLabel = GetDlgItem(hDlg, m.id);
        if (hLabel && Translations::GetTranslationItem(m.key, buffer))
            SetWindowTextA(hLabel, buffer);
    }
}



