#include "CNewPropertyAircraft.h"
#include <CFinalSunDlg.h>
#include "../../FA2sp.h"
#include "../../Ext/CFinalSunApp/Body.h"
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CMapData/Body.h"
#include "../../Miscs/DialogStyle.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/Helper.h"

static bool allowFilter = false;

CNewPropertyAircraft::CNewPropertyAircraft()
{
}

CNewPropertyAircraft::~CNewPropertyAircraft()
{
}

bool CNewPropertyAircraft::DoModal()
{
    DialogBoxParam(
        reinterpret_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(IDD),
        CFinalSunDlg::Instance->GetSafeHwnd(),
        CNewPropertyAircraft::DlgProc,
        reinterpret_cast<LPARAM>(this)
    );

    m_comboBoxes.clear();
    return m_accepted;
}

BOOL CALLBACK CNewPropertyAircraft::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CNewPropertyAircraft* pThis = reinterpret_cast<CNewPropertyAircraft*>(lParam);
        SetWindowLongPtr(hWnd, DWLP_USER, reinterpret_cast<LONG_PTR>(pThis));
        return pThis->OnInitDialog(hWnd);
    }
    case WM_COMMAND:
    {
        CNewPropertyAircraft* pThis = reinterpret_cast<CNewPropertyAircraft*>(GetWindowLongPtr(hWnd, DWLP_USER));
        if (!pThis) return FALSE;

        WORD id = LOWORD(wParam);
        WORD code = HIWORD(wParam);

        if (id == 1080 && code == EN_CHANGE)
        {
            pThis->UpdateHealthDisplay(hWnd);
            return TRUE;
        }
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
        if (id >= 1300 && id <= 1308 && code == BN_CLICKED)
        {
            bool checked = SendMessage(GetDlgItem(hWnd, id), BM_GETCHECK, 0, 0);
            int idx = id - 1300;
            if (idx >= 0 && idx < 9)
                CViewObjectsExt::AircraftBrushBools[idx] = checked;
        }
        return FALSE;
    }
    case WM_HSCROLL:
    {
        CNewPropertyAircraft* pThis = reinterpret_cast<CNewPropertyAircraft*>(GetWindowLongPtr(hWnd, DWLP_USER));
        HWND hTrack = GetDlgItem(hWnd, 1315);
        if (pThis && reinterpret_cast<HWND>(lParam) == hTrack)
        {
            char buffer[32] = {};
            sprintf_s(buffer, "%d", static_cast<int>(SendMessage(hTrack, TBM_GETPOS, 0, 0)));
            SetWindowTextA(GetDlgItem(hWnd, 1080), buffer);
            pThis->UpdateHealthDisplay(hWnd);
            return TRUE;
        }
        break;
    }
    case WM_DRAWITEM:
    {
        auto* dis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
        if (dis && dis->CtlID == 1316)
        {
            auto* pThis = reinterpret_cast<CNewPropertyAircraft*>(GetWindowLongPtr(hWnd, DWLP_USER));
            int health = static_cast<int>(SendMessage(GetDlgItem(hWnd, 1315), TBM_GETPOS, 0, 0));
            int HP = pThis && pThis->m_totalHealth > 0
                ? (health * 256 + pThis->m_totalHealth / 2) / pThis->m_totalHealth : 256;
            COLORREF color = RGB(0, 192, 0);
            if (static_cast<int>((CMapDataExt::ConditionRed + 0.001f) * 256) > HP)
                color = RGB(220, 0, 0);
            else if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP)
                color = RGB(220, 180, 0);
            HBRUSH brush = CreateSolidBrush(color);
            FillRect(dis->hDC, &dis->rcItem, brush);
            DeleteObject(brush);
            FrameRect(dis->hDC, &dis->rcItem, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            return TRUE;
        }
        break;
    }
    case WM_MEASUREITEM:
    {
        VirtualComboBoxEx::SetWindowHeight(hWnd, lParam);
        return TRUE;
    }
    case WM_CLOSE:
    {
        CNewPropertyAircraft* pThis = reinterpret_cast<CNewPropertyAircraft*>(GetWindowLongPtr(hWnd, DWLP_USER));
        if (pThis) pThis->OnCancel(hWnd);
        return TRUE;
    }
    }
    return FALSE;
}

BOOL CNewPropertyAircraft::OnInitDialog(HWND hDlg)
{
    m_hWnd = hDlg;

    FString buffer;

    SetWindowTextA(hDlg, Translations::TranslateOrDefault("AirCap", "Aircraft Options"));

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

    // Strength input (1080)
    HWND hStrength = GetDlgItem(hDlg, 1080);
    if (hStrength)
    {
        m_totalHealth = CString_ObjectID.IsEmpty()
            ? 256
            : Variables::RulesMap.GetInteger(CString_ObjectID, "Strength", 256);
        if (m_totalHealth <= 0)
            m_totalHealth = 256;
        int currentHealth = CString_HealthPoint.IsEmpty()
            ? m_totalHealth
            : (atoi(CString_HealthPoint) * m_totalHealth + 128) / 256;
        char healthBuffer[32] = {};
        sprintf_s(healthBuffer, "%d", currentHealth);
        SetWindowTextA(hStrength, healthBuffer);
        HWND hTrack = GetDlgItem(hDlg, 1315);
        if (hTrack)
        {
            SendMessage(hTrack, TBM_SETRANGE, TRUE, MAKELONG(0, m_totalHealth));
            SendMessage(hTrack, TBM_SETPOS, TRUE, currentHealth);
        }
        UpdateHealthDisplay(hDlg);
    }

    // Direction combo (1088) - VirtualComboBoxEx
    HWND hDirection = GetDlgItem(hDlg, 1088);
    if (hDirection)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hDirection, nullptr, true);
        vcb->SetAutoSearchRestriction(&allowFilter);
        const char* directionKeys[] = {
            "Direction.NorthEast", "Direction.East", "Direction.SouthEast", "Direction.South",
            "Direction.SouthWest", "Direction.West", "Direction.NorthWest", "Direction.North"
        };
        const char* directionDefaults[] = {
            "North-East", "East", "South-East", "South",
            "South-West", "West", "North-West", "North"
        };
        for (int i = 0; i < 8; ++i)
        {
            FString direction;
            direction.Format("%d - %s",
                i * 32, Translations::TranslateOrDefault(directionKeys[i], directionDefaults[i]));
            vcb->AddString(direction);
        }
        if (!CString_Direction.IsEmpty())
        {
            int index = vcb->FindStringExactStart(CString_Direction + " ");
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(CString_Direction);
        }
        m_comboBoxes[hDirection] = std::move(vcb);
    }

    // State combo (1082) - VirtualComboBoxEx
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
            if (index != CB_ERR) vcb->SetCurSel(index);
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

    // Group (1084) - Edit
    HWND hGroup = GetDlgItem(hDlg, 1084);
    if (hGroup && !CString_Group.IsEmpty())
        SetWindowTextA(hGroup, CString_Group);

    // AutoNORecruitType (1085) - ComboBox (0/1)
    HWND hAutoNO = GetDlgItem(hDlg, 1085);
    if (hAutoNO)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hAutoNO, nullptr, true);
        vcb->SetAutoSearchRestriction(&allowFilter);
        vcb->AddString("0");
        vcb->AddString("1");
        if (!CString_AutoCreateNoRecruitable.IsEmpty())
        {
            int index = vcb->FindStringExact(CString_AutoCreateNoRecruitable);
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(CString_AutoCreateNoRecruitable);
        }
        m_comboBoxes[hAutoNO] = std::move(vcb);
    }

    // AutoYESRecruitType (1086) - ComboBox (0/1)
    HWND hAutoYES = GetDlgItem(hDlg, 1086);
    if (hAutoYES)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hAutoYES, nullptr, true);
        vcb->SetAutoSearchRestriction(&allowFilter);
        vcb->AddString("0");
        vcb->AddString("1");
        if (!CString_AutoCreateYesRecruitable.IsEmpty())
        {
            int index = vcb->FindStringExact(CString_AutoCreateYesRecruitable);
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(CString_AutoCreateYesRecruitable);
        }
        m_comboBoxes[hAutoYES] = std::move(vcb);
    }

    // VeteranLevel (1087) - ComboBox (0/100/200)
    HWND hVeteran = GetDlgItem(hDlg, 1087);
    if (hVeteran)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hVeteran, nullptr, true);
        vcb->SetAutoSearchRestriction(&allowFilter);
        const char* veteranKeys[] = {
            "ObjectInfo.Veterancy.Rookie", "ObjectInfo.Veterancy.Veteran", "ObjectInfo.Veterancy.Elite"
        };
        const char* veteranDefaults[] = { "Rookie", "Veteran", "Elite" };
        for (int i = 0; i < 3; ++i)
        {
            FString veteran;
            veteran.Format("%d - %s", i * 100,
                Translations::TranslateOrDefault(veteranKeys[i], veteranDefaults[i]));
            vcb->AddString(veteran);
        }
        if (!CString_VeteranLevel.IsEmpty())
        {
            int index = vcb->FindStringExactStart(CString_VeteranLevel + " ");
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(CString_VeteranLevel);
        }
        m_comboBoxes[hVeteran] = std::move(vcb);
    }

    if (!CViewObjectsExt::InitPropertyDlgFromProperty)
    {
        for (int i = 0; i < 9; ++i)
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
        for (int i = 0; i < 9; ++i)
        {
            HWND hCheck = GetDlgItem(hDlg, 1300 + i);
            if (hCheck && CViewObjectsExt::AircraftBrushBools[i])
                SendMessage(hCheck, BM_SETCHECK, BST_CHECKED, 0);
        }
    }

    ExtraWindow::DisableOtherWindows(hDlg);

    return TRUE;
}

void CNewPropertyAircraft::OnOK(HWND hDlg)
{
    CollectResults(hDlg);
    ExtraWindow::RestoreDisabledWindows();
    EndDialog(hDlg, IDOK);
    m_accepted = true;
}

void CNewPropertyAircraft::OnCancel(HWND hDlg)
{
    ExtraWindow::RestoreDisabledWindows();
    EndDialog(hDlg, IDCANCEL);
    m_accepted = false;
}

void CNewPropertyAircraft::CollectResults(HWND hDlg)
{
    char buffer[256] = {};

    // Strength
    HWND hStrength = GetDlgItem(hDlg, 1080);
    if (hStrength)
    {
        GetWindowTextA(hStrength, buffer, sizeof(buffer));
        int health = atoi(buffer);
        health = health < 0 ? 0 : (health > m_totalHealth ? m_totalHealth : health);
        if (!CString_ObjectID.IsEmpty())
            health = (health * 256 + m_totalHealth / 2) / m_totalHealth;
        else
            health = health > 256 ? 256 : health;
        sprintf_s(buffer, "%d", health);
        CString_HealthPoint = buffer;
    }

    // Direction (1088)
    HWND hDirection = GetDlgItem(hDlg, 1088);
    if (hDirection && m_comboBoxes[hDirection])
    {
        CString_Direction = m_comboBoxes[hDirection]->GetSelectedText(true);
        STDHelpers::TrimIndex(CString_Direction);
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

	// State
    HWND hState = GetDlgItem(hDlg, 1082);
    if (hState && m_comboBoxes[hState])
    {
        int index = m_comboBoxes[hState]->GetCurSel();
        if (index >= 0 && index < (int)CMapDataExt::TechnoStates.size())
            CString_State = CMapDataExt::TechnoStates[index];
        else
            CString_State = m_comboBoxes[hState]->GetSelectedText(false);
    }

    // Group
    HWND hGroup = GetDlgItem(hDlg, 1084);
    if (hGroup)
    {
        GetWindowTextA(hGroup, buffer, sizeof(buffer));
        CString_Group = buffer;
    }
    
    // AutoNORecruitType
    HWND hAutoNO = GetDlgItem(hDlg, 1085);
    if (hAutoNO && m_comboBoxes[hAutoNO])
        CString_AutoCreateNoRecruitable = m_comboBoxes[hAutoNO]->GetSelectedText(true);
    
    // AutoYESRecruitType
    HWND hAutoYES = GetDlgItem(hDlg, 1086);
    if (hAutoYES && m_comboBoxes[hAutoYES])
        CString_AutoCreateYesRecruitable = m_comboBoxes[hAutoYES]->GetSelectedText(true);

    // VeteranLevel
    HWND hVeteran = GetDlgItem(hDlg, 1087);
    if (hVeteran && m_comboBoxes[hVeteran])
    {
        CString_VeteranLevel = m_comboBoxes[hVeteran]->GetSelectedText(true);
        STDHelpers::TrimIndex(CString_VeteranLevel);
    }
}

void CNewPropertyAircraft::UpdateHealthDisplay(HWND hDlg)
{
    char buffer[32] = {};
    GetWindowTextA(GetDlgItem(hDlg, 1080), buffer, sizeof(buffer));
    int currentHealth = atoi(buffer);
    currentHealth = currentHealth < 0 ? 0 : (currentHealth > m_totalHealth ? m_totalHealth : currentHealth);
    int percentage = m_totalHealth > 0 ? (currentHealth * 100 + m_totalHealth / 2) / m_totalHealth : 0;
    HWND hTrack = GetDlgItem(hDlg, 1315);
    if (hTrack)
        SendMessage(hTrack, TBM_SETPOS, TRUE, currentHealth);
    FString display;
    display.Format("/%d (%d%%)", m_totalHealth, percentage);
    SetWindowTextA(GetDlgItem(hDlg, 1314), display);
    InvalidateRect(GetDlgItem(hDlg, 1316), nullptr, TRUE);
}

void CNewPropertyAircraft::TranslateLabels(HWND hDlg)
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
        {1266, "UnitP5"},
        {1267, "UnitP6"},
    };

    FString buffer;
    for (const auto& m : mappings)
    {
        HWND hLabel = GetDlgItem(hDlg, m.id);
        if (hLabel && Translations::GetTranslationItem(m.key, buffer))
            SetWindowTextA(hLabel, buffer);
    }
}