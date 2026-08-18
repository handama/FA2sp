#include "CTechnoDialog.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/Helper.h"
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CFinalSunApp/Body.h"
#include "../Common.h"
#include "../../Ext/CLoading/Body.h"
#include <CFinalSunApp.h>
#include <Miscs/Miscs.LoadParams.h>
#include "../../Ext/CMapData/Body.h"
#include <CFinalSunDlg.h>
#include "../../Miscs/DialogStyle.h"
#include "../../FA2sp.h"

static bool allowFilter = false;

CTechnoDialog::CTechnoDialog()
{
    m_nStrength = 256;
    m_bEnableHouse = FALSE;
    m_bEnableStrength = FALSE;
    m_bEnableDirection = FALSE;
    m_bEnableState = FALSE;
    m_bEnableVeteran = FALSE;
    m_bEnableGroup = FALSE;
    m_bEnableAboveGround = FALSE;
    m_bEnableFollowsIndex = FALSE;
    m_bEnableAutoNO = FALSE;
    m_bEnableAutoYES = FALSE;
    m_bEnableTag = FALSE;
    m_bEnableSellable = FALSE;
    m_bEnableRebuild = FALSE;
    m_bEnablePowered = FALSE;
    m_bEnableUpgradeCount = FALSE;
    m_bEnableSpotlight = FALSE;
    m_bEnableUpgrade1 = FALSE;
    m_bEnableUpgrade2 = FALSE;
    m_bEnableUpgrade3 = FALSE;
    m_bEnableAIRepairs = FALSE;
    m_bEnableNominal = FALSE;
}

CTechnoDialog::~CTechnoDialog()
{
}

bool CTechnoDialog::DoModal()
{
    DialogBoxParam(
        reinterpret_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(IDD),
        CFinalSunDlg::Instance->GetSafeHwnd(),
        CTechnoDialog::DlgProc,
        reinterpret_cast<LPARAM>(this)
    );

    m_comboBoxes.clear();
    return m_accepted;
}

BOOL CALLBACK CTechnoDialog::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CTechnoDialog* pThis = reinterpret_cast<CTechnoDialog*>(lParam);
        SetWindowLongPtr(hWnd, DWLP_USER, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hWnd = hWnd;
        return pThis->OnInitDialog(hWnd);
    }
    case WM_COMMAND:
    {
        CTechnoDialog* pThis = reinterpret_cast<CTechnoDialog*>(GetWindowLongPtr(hWnd, DWLP_USER));
        if (!pThis) return FALSE;

        WORD id = LOWORD(wParam);
        WORD code = HIWORD(wParam);

        if (id == 1042 && code == EN_CHANGE)
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

        return FALSE;
    }
    case WM_HSCROLL:
    {
        CTechnoDialog* pThis = reinterpret_cast<CTechnoDialog*>(GetWindowLongPtr(hWnd, DWLP_USER));
        HWND hTrack = GetDlgItem(hWnd, 1003);
        if (pThis && reinterpret_cast<HWND>(lParam) == hTrack)
        {
            char buffer[32] = {};
            sprintf_s(buffer, "%d", static_cast<int>(SendMessage(hTrack, TBM_GETPOS, 0, 0)));
            SetWindowTextA(GetDlgItem(hWnd, 1042), buffer);
            pThis->UpdateHealthDisplay(hWnd);
            return TRUE;
        }
        break;
    }
    case WM_DRAWITEM:
    {
        auto* dis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
        if (dis && dis->CtlID == 1044)
        {
            char buffer[32] = {};
            GetWindowTextA(GetDlgItem(hWnd, 1042), buffer, sizeof(buffer));
            int HP = atoi(buffer);
            HP = HP < 0 ? 0 : (HP > 256 ? 256 : HP);
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
        CTechnoDialog* pThis = reinterpret_cast<CTechnoDialog*>(GetWindowLongPtr(hWnd, DWLP_USER));
        if (pThis) pThis->OnCancel(hWnd);
        return TRUE;
    }
    }
    return FALSE;
}

BOOL CTechnoDialog::OnInitDialog(HWND hDlg)
{
    FString buffer;

    if (Translations::GetTranslationItem("OK", buffer))
        SetWindowTextA(GetDlgItem(hDlg, 1), buffer);
    if (Translations::GetTranslationItem("Cancel", buffer))
        SetWindowTextA(GetDlgItem(hDlg, 2), buffer);

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
            HWND hLabel = GetDlgItem(hDlg, item.id);
            if (hLabel) SetWindowTextA(hLabel, buffer);
        }
    }

    if (Translations::GetTranslationItem("TechnoOptionsCaption", buffer))
        SetWindowTextA(hDlg, buffer);

    // Font
    HFONT hFont = DarkTheme::GetModernDefaultGUIFont();
    if (hFont)
    {
        SendMessage(GetDlgItem(hDlg, 1), WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        SendMessage(GetDlgItem(hDlg, 2), WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    }

    // Strength: edit (1042) + trackbar (1003) + display (1043) + color block (1044)
    HWND hStrengthEdit = GetDlgItem(hDlg, 1042);
    if (hStrengthEdit)
    {
        char healthBuffer[32] = {};
        sprintf_s(healthBuffer, "%d", m_nStrength);
        SetWindowTextA(hStrengthEdit, healthBuffer);
        HWND hTrack = GetDlgItem(hDlg, 1003);
        if (hTrack)
        {
            SendMessage(hTrack, TBM_SETRANGE, TRUE, MAKELONG(0, 256));
            SendMessage(hTrack, TBM_SETPOS, TRUE, m_nStrength);
        }
        UpdateHealthDisplay(hDlg);
    }

    // Direction combo (1005) - VirtualComboBoxEx
    HWND hDirection = GetDlgItem(hDlg, 1005);
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
        if (!m_strDirection.IsEmpty())
        {
            int index = vcb->FindStringExactStart(m_strDirection + " ");
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(m_strDirection);
        }
        m_comboBoxes[hDirection] = std::move(vcb);
    }

    // House combo (1001) - VirtualComboBoxEx
    HWND hHouse = GetDlgItem(hDlg, 1001);
    if (hHouse)
    {
        TempValueHolder tmp(CMapDataExt::IsInitingPropertyDialog, true);
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hHouse, nullptr, false);
        ExtraWindow::LoadParams_Houses(*vcb, CMapData::Instance->IsMultiOnly(), false, ExtConfigs::PlayerAtXForTechnos);
        m_comboBoxes[hHouse] = std::move(vcb);
        if (!m_strHouse.IsEmpty())
        {
            int index = m_comboBoxes[hHouse]->FindStringExact(Translations::ParseHouseName(m_strHouse, true));
            if (index != CB_ERR)
                m_comboBoxes[hHouse]->SetCurSel(index);
            else
                m_comboBoxes[hHouse]->SetCurSel(0);
        }
    }

    // Tag combo (1021) - VirtualComboBoxEx
    HWND hTag = GetDlgItem(hDlg, 1021);
    if (hTag)
    {
		auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hTag, &ExtConfigs::SortByLabelName_Tag, true);
        ExtraWindow::LoadParams_Tags(*vcb, true);
        m_comboBoxes[hTag] = std::move(vcb);

        if (!m_strTag.IsEmpty())
        {
            int index = CB_ERR;
            if (m_strTag != "None")
            {
                FString name;
                name.Format("%s - %s", m_strTag,
                    FString::GetParam(CINI::CurrentDocument->GetString("Tags", m_strTag, "0,MISSING,01000000"), 1));
                index = m_comboBoxes[hTag]->FindStringExact(name);
            }
            else
            {
                index = m_comboBoxes[hTag]->FindStringExact(m_strTag);
            }
            if (index != CB_ERR)
                m_comboBoxes[hTag]->SetCurSel(index);
            else
                m_comboBoxes[hTag]->SetEditText(m_strTag);
        }
    }

    // State combo (1007) - VirtualComboBoxEx
    HWND hState = GetDlgItem(hDlg, 1007);
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
        if (!m_strState.IsEmpty())
        {
            FString key = "FootClassStatus.";
            key += m_strState;
            int index = vcb->FindStringExact(Translations::TranslateOrDefault(key, m_strState));
            if (index != CB_ERR) vcb->SetCurSel(index);
        }
        m_comboBoxes[hState] = std::move(vcb);
    }

    // Rebuild/Powered/UpgradeCount/AIRepairs/Nominal combos - VirtualComboBoxEx
    struct SimpleComboInfo { int id; std::vector<const char*> items; };
    SimpleComboInfo simpleCombos[] = {
        {1025, {"0","1"}},           // Rebuild
        {1027, {"0","1"}},           // Powered
        {1029, {"0","1","2","3"}},   // UpgradeCount
        {1039, {"0","1"}},           // AIRepairs
        {1041, {"0","1"}},           // Nominal
    };

    for (const auto& ci : simpleCombos)
    {
        HWND hCombo = GetDlgItem(hDlg, ci.id);
        if (hCombo)
        {
            auto vcb = std::make_unique<VirtualComboBoxEx>();
            vcb->Attach(hCombo, nullptr, true);
            vcb->SetAutoSearchRestriction(&allowFilter);
            for (const auto* item : ci.items)
                vcb->AddString(item);
            m_comboBoxes[hCombo] = std::move(vcb);
        }
    }

    // Set selections for Rebuild/Powered/UpgradeCount/AIRepairs/Nominal
    if (!m_strRebuild.IsEmpty())
    {
        HWND h = GetDlgItem(hDlg, 1025);
        if (h && m_comboBoxes[h])
        {
            int index = m_comboBoxes[h]->FindStringExact(m_strRebuild);
            if (index != CB_ERR)
                m_comboBoxes[h]->SetCurSel(index);
            else
                m_comboBoxes[h]->SetEditText(m_strRebuild);
        }
    }
    if (!m_strPowered.IsEmpty())
    {
        HWND h = GetDlgItem(hDlg, 1027);
        if (h && m_comboBoxes[h])
        {
            int index = m_comboBoxes[h]->FindStringExact(m_strPowered);
            if (index != CB_ERR)
                m_comboBoxes[h]->SetCurSel(index);
            else
                m_comboBoxes[h]->SetEditText(m_strPowered);
        }
    }
    if (!m_strUpgradeCount.IsEmpty())
    {
        HWND h = GetDlgItem(hDlg, 1029);
        if (h && m_comboBoxes[h])
        {
            int index = m_comboBoxes[h]->FindStringExact(m_strUpgradeCount);
            if (index != CB_ERR)
                m_comboBoxes[h]->SetCurSel(index);
            else
                m_comboBoxes[h]->SetEditText(m_strUpgradeCount);
        }
    }
    if (!m_strAIRepairs.IsEmpty())
    {
        HWND h = GetDlgItem(hDlg, 1039);
        if (h && m_comboBoxes[h])
        {
            int index = m_comboBoxes[h]->FindStringExact(m_strAIRepairs);
            if (index != CB_ERR)
                m_comboBoxes[h]->SetCurSel(index);
            else
                m_comboBoxes[h]->SetEditText(m_strAIRepairs);
        }
    }
    if (!m_strNominal.IsEmpty())
    {
        HWND h = GetDlgItem(hDlg, 1041);
        if (h && m_comboBoxes[h])
        {
            int index = m_comboBoxes[h]->FindStringExact(m_strNominal);
            if (index != CB_ERR)
                m_comboBoxes[h]->SetCurSel(index);
            else
                m_comboBoxes[h]->SetEditText(m_strNominal);
        }
    }

    // Spotlight combo (1031) - VirtualComboBoxEx
    HWND hSpotlight = GetDlgItem(hDlg, 1031);
    if (hSpotlight)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hSpotlight, nullptr, true);
        vcb->SetAutoSearchRestriction(&allowFilter);
        vcb->AddString(Translations::TranslateOrDefault("StructSpotlight.0", "0 - No spotlight"));
        vcb->AddString(Translations::TranslateOrDefault("StructSpotlight.1", "1 - Rules.ini setting"));
        vcb->AddString(Translations::TranslateOrDefault("StructSpotlight.2", "2 - Circle / Direction"));
        if (!m_strSpotlight.IsEmpty())
        {
            int index = vcb->FindStringExactStart(m_strSpotlight);
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(m_strSpotlight);
        }
        m_comboBoxes[hSpotlight] = std::move(vcb);
    }

    // VeteranLevel (1009) - ComboBox (0/100/200)
    HWND hVeteran = GetDlgItem(hDlg, 1009);
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
        if (!m_strVeteranLevel.IsEmpty())
        {
            int index = vcb->FindStringExactStart(m_strVeteranLevel + " ");
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(m_strVeteranLevel);
        }
        m_comboBoxes[hVeteran] = std::move(vcb);
    }

    // AboveGround (1013) - ComboBox (0/1)
    HWND hAbove = GetDlgItem(hDlg, 1013);
    if (hAbove)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hAbove, nullptr, true);
        vcb->SetAutoSearchRestriction(&allowFilter);
        vcb->AddString("0");
        vcb->AddString("1");
        if (!m_strAboveGround.IsEmpty())
        {
            int index = vcb->FindStringExact(m_strAboveGround);
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(m_strAboveGround);
        }
        m_comboBoxes[hAbove] = std::move(vcb);
    }

    // FollowsIndex (1015) - ComboBox ("-1 - None")
    HWND hFollow = GetDlgItem(hDlg, 1015);
    if (hFollow)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hFollow, nullptr, true);
        vcb->AddString("-1 - None");
        if (auto pSection = CINI::CurrentDocument->GetSection("Units"))
        {
			int index = 0;
			FString text;
			for (const auto& [key, value] : pSection->GetEntities())
            {
                auto id = FString::GetParam(value, 1);
                auto x = FString::GetParam(value, 4);
                auto y = FString::GetParam(value, 3);
                FString display;
                FString name = Variables::RulesMap.GetString(id, "Name");
                if (name.IsEmpty() || !Translations::GetTranslationItem(name, display))
                {
                    display = CViewObjectsExt::QueryUIName(id, true);
                }
                if (display != id)
                {
                    display.Format("%s (%s)", display, id);
                }

                text.Format("%d - %s (%s, %s)", index++, display, y, x);
                vcb->AddString(text);
            }
        }
        if (!m_strFollowsIndex.IsEmpty())
        {
            int index = vcb->FindStringExactStart(m_strFollowsIndex + " ");
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(m_strFollowsIndex);
        }
        m_comboBoxes[hFollow] = std::move(vcb);
    }

    // AutoNORecruitType (1017) - ComboBox (0/1)
    HWND hAutoNO = GetDlgItem(hDlg, 1017);
    if (hAutoNO)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hAutoNO, nullptr, true);
        vcb->SetAutoSearchRestriction(&allowFilter);
        vcb->AddString("0");
        vcb->AddString("1");
        if (!m_strAutoNO_Recruit.IsEmpty())
        {
            int index = vcb->FindStringExact(m_strAutoNO_Recruit);
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(m_strAutoNO_Recruit);
        }
        m_comboBoxes[hAutoNO] = std::move(vcb);
    }

    // AutoYESRecruitType (1019) - ComboBox (0/1)
    HWND hAutoYES = GetDlgItem(hDlg, 1019);
    if (hAutoYES)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hAutoYES, nullptr, true);
        vcb->SetAutoSearchRestriction(&allowFilter);
        vcb->AddString("0");
        vcb->AddString("1");
        if (!m_strAutoYES_Recruit.IsEmpty())
        {
            int index = vcb->FindStringExact(m_strAutoYES_Recruit);
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(m_strAutoYES_Recruit);
        }
        m_comboBoxes[hAutoYES] = std::move(vcb);
    }

    // Sellable (1023) - ComboBox (0/1)
    HWND hSellable = GetDlgItem(hDlg, 1023);
    if (hSellable)
    {
        auto vcb = std::make_unique<VirtualComboBoxEx>();
        vcb->Attach(hSellable, nullptr, true);
        vcb->SetAutoSearchRestriction(&allowFilter);
        vcb->AddString("0");
        vcb->AddString("1");
        if (!m_strSellable.IsEmpty())
        {
            int index = vcb->FindStringExact(m_strSellable);
            if (index != CB_ERR)
                vcb->SetCurSel(index);
            else
                vcb->SetEditText(m_strSellable);
        }
        m_comboBoxes[hSellable] = std::move(vcb);
    }

    ExtraWindow::DisableOtherWindows(hDlg);

    return TRUE;
}

void CTechnoDialog::CollectDataToMembers(HWND hDlg)
{
    char buffer[256];

    HWND hHouse = GetDlgItem(hDlg, 1001);
    if (hHouse && m_comboBoxes[hHouse])
    {
        m_strHouse = Translations::ParseHouseName(m_comboBoxes[hHouse]->GetSelectedText(false), false);
    }

    HWND hDirection = GetDlgItem(hDlg, 1005);
    if (hDirection && m_comboBoxes[hDirection])
    {
        m_strDirection = m_comboBoxes[hDirection]->GetSelectedText(true);
        STDHelpers::TrimIndex(m_strDirection);
    }

    HWND hState = GetDlgItem(hDlg, 1007);
    if (hState && m_comboBoxes[hState])
    {
        int index = m_comboBoxes[hState]->GetCurSel();
        if (index >= 0 && index < (int)CMapDataExt::TechnoStates.size())
            m_strState = CMapDataExt::TechnoStates[index];
        else
            m_strState = m_comboBoxes[hState]->GetSelectedText(false);
    }

    HWND hVeteran = GetDlgItem(hDlg, 1009);
    if (hVeteran && m_comboBoxes[hVeteran])
    {
        m_strVeteranLevel = m_comboBoxes[hVeteran]->GetSelectedText(true);
        STDHelpers::TrimIndex(m_strVeteranLevel);
    }

    HWND hGroup = GetDlgItem(hDlg, 1011);
    if (hGroup) { GetWindowTextA(hGroup, buffer, sizeof(buffer)); m_strGroup = buffer; }

    HWND hAbove = GetDlgItem(hDlg, 1013);
    if (hAbove && m_comboBoxes[hAbove])
        m_strAboveGround = m_comboBoxes[hAbove]->GetSelectedText(true);

    HWND hFollow = GetDlgItem(hDlg, 1015);
    if (hFollow && m_comboBoxes[hFollow])
    {
        m_strFollowsIndex = m_comboBoxes[hFollow]->GetSelectedText(true);
        STDHelpers::TrimIndex(m_strFollowsIndex);
    }

    HWND hAutoNO = GetDlgItem(hDlg, 1017);
    if (hAutoNO && m_comboBoxes[hAutoNO])
        m_strAutoNO_Recruit = m_comboBoxes[hAutoNO]->GetSelectedText(true);

    HWND hAutoYES = GetDlgItem(hDlg, 1019);
    if (hAutoYES && m_comboBoxes[hAutoYES])
        m_strAutoYES_Recruit = m_comboBoxes[hAutoYES]->GetSelectedText(true);

    HWND hTag = GetDlgItem(hDlg, 1021);
    if (hTag && m_comboBoxes[hTag])
    {
        m_strTag = m_comboBoxes[hTag]->GetSelectedText(true);
        STDHelpers::TrimIndex(m_strTag);
    }

    HWND hSellable = GetDlgItem(hDlg, 1023);
    if (hSellable && m_comboBoxes[hSellable])
        m_strSellable = m_comboBoxes[hSellable]->GetSelectedText(true);

    HWND hRebuild = GetDlgItem(hDlg, 1025);
    if (hRebuild && m_comboBoxes[hRebuild])
    {
        m_strRebuild = m_comboBoxes[hRebuild]->GetSelectedText(true);
    }

    HWND hPowered = GetDlgItem(hDlg, 1027);
    if (hPowered && m_comboBoxes[hPowered])
    {
        m_strPowered = m_comboBoxes[hPowered]->GetSelectedText(true);
    }

    HWND hUpgradeCount = GetDlgItem(hDlg, 1029);
    if (hUpgradeCount && m_comboBoxes[hUpgradeCount])
    {
        m_strUpgradeCount = m_comboBoxes[hUpgradeCount]->GetSelectedText(true);
    }

    HWND hSpotlight = GetDlgItem(hDlg, 1031);
    if (hSpotlight && m_comboBoxes[hSpotlight])
    {
        m_strSpotlight = m_comboBoxes[hSpotlight]->GetSelectedText(true);
        STDHelpers::TrimIndex(m_strSpotlight);
    }

    HWND hUpgrade1 = GetDlgItem(hDlg, 1033);
    if (hUpgrade1) { GetWindowTextA(hUpgrade1, buffer, sizeof(buffer)); m_strUpgrade1 = buffer; }

    HWND hUpgrade2 = GetDlgItem(hDlg, 1035);
    if (hUpgrade2) { GetWindowTextA(hUpgrade2, buffer, sizeof(buffer)); m_strUpgrade2 = buffer; }

    HWND hUpgrade3 = GetDlgItem(hDlg, 1037);
    if (hUpgrade3) { GetWindowTextA(hUpgrade3, buffer, sizeof(buffer)); m_strUpgrade3 = buffer; }

    HWND hAIRepairs = GetDlgItem(hDlg, 1039);
    if (hAIRepairs && m_comboBoxes[hAIRepairs])
    {
        m_strAIRepairs = m_comboBoxes[hAIRepairs]->GetSelectedText(true);
    }

    HWND hNominal = GetDlgItem(hDlg, 1041);
    if (hNominal && m_comboBoxes[hNominal])
    {
        m_strNominal = m_comboBoxes[hNominal]->GetSelectedText(true);
    }

    HWND hStrengthEdit = GetDlgItem(hDlg, 1042);
    if (hStrengthEdit)
    {
        GetWindowTextA(hStrengthEdit, buffer, sizeof(buffer));
        m_nStrength = atoi(buffer);
        m_nStrength = m_nStrength < 0 ? 0 : (m_nStrength > 256 ? 256 : m_nStrength);
    }

    // Checkbox states
    HWND hChkHouse = GetDlgItem(hDlg, 1300);
    m_bEnableHouse = (hChkHouse && SendMessage(hChkHouse, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkStrength = GetDlgItem(hDlg, 1301);
    m_bEnableStrength = (hChkStrength && SendMessage(hChkStrength, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkDirection = GetDlgItem(hDlg, 1302);
    m_bEnableDirection = (hChkDirection && SendMessage(hChkDirection, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkState = GetDlgItem(hDlg, 1303);
    m_bEnableState = (hChkState && SendMessage(hChkState, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkVeteran = GetDlgItem(hDlg, 1304);
    m_bEnableVeteran = (hChkVeteran && SendMessage(hChkVeteran, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkGroup = GetDlgItem(hDlg, 1305);
    m_bEnableGroup = (hChkGroup && SendMessage(hChkGroup, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkAbove = GetDlgItem(hDlg, 1306);
    m_bEnableAboveGround = (hChkAbove && SendMessage(hChkAbove, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkFollow = GetDlgItem(hDlg, 1307);
    m_bEnableFollowsIndex = (hChkFollow && SendMessage(hChkFollow, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkAutoNO = GetDlgItem(hDlg, 1308);
    m_bEnableAutoNO = (hChkAutoNO && SendMessage(hChkAutoNO, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkAutoYES = GetDlgItem(hDlg, 1309);
    m_bEnableAutoYES = (hChkAutoYES && SendMessage(hChkAutoYES, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkTag = GetDlgItem(hDlg, 1310);
    m_bEnableTag = (hChkTag && SendMessage(hChkTag, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkSellable = GetDlgItem(hDlg, 1311);
    m_bEnableSellable = (hChkSellable && SendMessage(hChkSellable, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkRebuild = GetDlgItem(hDlg, 1312);
    m_bEnableRebuild = (hChkRebuild && SendMessage(hChkRebuild, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkPowered = GetDlgItem(hDlg, 1313);
    m_bEnablePowered = (hChkPowered && SendMessage(hChkPowered, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkUpgradeCount = GetDlgItem(hDlg, 1314);
    m_bEnableUpgradeCount = (hChkUpgradeCount && SendMessage(hChkUpgradeCount, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkSpotlight = GetDlgItem(hDlg, 1315);
    m_bEnableSpotlight = (hChkSpotlight && SendMessage(hChkSpotlight, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkUpgrade1 = GetDlgItem(hDlg, 1316);
    m_bEnableUpgrade1 = (hChkUpgrade1 && SendMessage(hChkUpgrade1, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkUpgrade2 = GetDlgItem(hDlg, 1317);
    m_bEnableUpgrade2 = (hChkUpgrade2 && SendMessage(hChkUpgrade2, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkUpgrade3 = GetDlgItem(hDlg, 1318);
    m_bEnableUpgrade3 = (hChkUpgrade3 && SendMessage(hChkUpgrade3, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkAIRepairs = GetDlgItem(hDlg, 1319);
    m_bEnableAIRepairs = (hChkAIRepairs && SendMessage(hChkAIRepairs, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    HWND hChkNominal = GetDlgItem(hDlg, 1320);
    m_bEnableNominal = (hChkNominal && SendMessage(hChkNominal, BM_GETCHECK, 0, 0) == BST_CHECKED) ? TRUE : FALSE;

    // Post-processing
    STDHelpers::TrimIndex(m_strSpotlight);
}

void CTechnoDialog::UpdateHealthDisplay(HWND hDlg)
{
    char buffer[32] = {};
    GetWindowTextA(GetDlgItem(hDlg, 1042), buffer, sizeof(buffer));
    int currentHealth = atoi(buffer);
    currentHealth = currentHealth < 0 ? 0 : (currentHealth > 256 ? 256 : currentHealth);
    int percentage = (currentHealth * 100 + 128) / 256;
    HWND hTrack = GetDlgItem(hDlg, 1003);
    if (hTrack)
        SendMessage(hTrack, TBM_SETPOS, TRUE, currentHealth);
    FString display;
    display.Format("/256 (%d%%)", percentage);
    SetWindowTextA(GetDlgItem(hDlg, 1043), display);
    InvalidateRect(GetDlgItem(hDlg, 1044), nullptr, TRUE);
}

void CTechnoDialog::OnOK(HWND hDlg)
{
    CollectDataToMembers(hDlg);
    ExtraWindow::RestoreDisabledWindows();
    EndDialog(hDlg, IDOK);
    m_accepted = true;
}

void CTechnoDialog::OnCancel(HWND hDlg)
{
    ExtraWindow::RestoreDisabledWindows();
    EndDialog(hDlg, IDCANCEL);
    m_accepted = false;
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

void CTechnoDialog::ApplyToCurrentTechno(void* pData, CLoadingExt::GameObjectType type)
{
    switch (type)
    {
    case  CLoadingExt::GameObjectType::Infantry:
        ApplyToInfantryData(static_cast<CInfantryData*>(pData));
        break;
    case  CLoadingExt::GameObjectType::Vehicle:
        ApplyToUnitData(static_cast<CUnitData*>(pData));
        break;
    case  CLoadingExt::GameObjectType::Aircraft:
        ApplyToAircraftData(static_cast<CAircraftData*>(pData));
        break;
    case  CLoadingExt::GameObjectType::Building:
        ApplyToBuildingData(static_cast<CBuildingData*>(pData));
        break;
    default:
        break;
    }
}

bool CTechnoDialog::HasAnyEnabledItem() const
{
    if ((m_bEnableHouse && !m_strHouse.IsEmpty()) ||
        (m_bEnableDirection && !m_strDirection.IsEmpty()) ||
        (m_bEnableState && !m_strState.IsEmpty()) ||
        (m_bEnableVeteran && !m_strVeteranLevel.IsEmpty()) ||
        (m_bEnableGroup && !m_strGroup.IsEmpty()) ||
        (m_bEnableAboveGround && !m_strAboveGround.IsEmpty()) ||
        (m_bEnableFollowsIndex && !m_strFollowsIndex.IsEmpty()) ||
        (m_bEnableAutoNO && !m_strAutoNO_Recruit.IsEmpty()) ||
        (m_bEnableAutoYES && !m_strAutoYES_Recruit.IsEmpty()) ||
        (m_bEnableTag && !m_strTag.IsEmpty()) ||
        (m_bEnableSellable && !m_strSellable.IsEmpty()) ||
        (m_bEnableRebuild && !m_strRebuild.IsEmpty()) ||
        (m_bEnablePowered && !m_strPowered.IsEmpty()) ||
        (m_bEnableUpgradeCount && !m_strUpgradeCount.IsEmpty()) ||
        (m_bEnableSpotlight && !m_strSpotlight.IsEmpty()) ||
        (m_bEnableUpgrade1 && !m_strUpgrade1.IsEmpty()) ||
        (m_bEnableUpgrade2 && !m_strUpgrade2.IsEmpty()) ||
        (m_bEnableUpgrade3 && !m_strUpgrade3.IsEmpty()) ||
        (m_bEnableAIRepairs && !m_strAIRepairs.IsEmpty()) ||
        (m_bEnableNominal && !m_strNominal.IsEmpty()) ||
        (m_bEnableStrength == TRUE))
    {
        return true;
    }

    return false;
}