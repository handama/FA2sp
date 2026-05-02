#include "CNewLocalVariables.h"
#include "../Common.h"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/MultimapHelper.h"
#include <CLoading.h>
#include <CFinalSunDlg.h>
#include <CObjectDatas.h>
#include "../CSearhReference/CSearhReference.h"

HWND CNewLocalVariables::m_hwnd;
CFinalSunDlg* CNewLocalVariables::m_parent;
CINI& CNewLocalVariables::map = CINI::CurrentDocument;
VirtualComboBoxEx CNewLocalVariables::vcbVariables;

HWND CNewLocalVariables::hVariables;
HWND CNewLocalVariables::hName;
HWND CNewLocalVariables::hValue;
HWND CNewLocalVariables::hNew;
HWND CNewLocalVariables::hSearch;
int CNewLocalVariables::SelectedIndex;
FString CNewLocalVariables::SelectedKey;

void CNewLocalVariables::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(324),
        pWnd->GetSafeHwnd(),
        CNewLocalVariables::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CNewLocalVariables.\n");
        m_parent = NULL;
        return;
    }
}

void CNewLocalVariables::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("LocalVariablesTitle", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };
    
	Translate(1000, "LocalVariablesSelect");
	Translate(1002, "LocalVariablesName");
	Translate(1004, "LocalVariablesValue");
	Translate(1006, "LocalVariablesNew");
	Translate(1007, "SearchReferenceTitle");

    hVariables = GetDlgItem(hWnd, Controls::Variables);
    hName = GetDlgItem(hWnd, Controls::Name);
    hValue = GetDlgItem(hWnd, Controls::Value);
    hNew = GetDlgItem(hWnd, Controls::New);
    hSearch = GetDlgItem(hWnd, Controls::Search);

    vcbVariables.Attach(hVariables, nullptr, false);

    Update();
}

void CNewLocalVariables::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CNewLocalVariables::m_hwnd = NULL;
    CNewLocalVariables::m_parent = NULL;
}

BOOL CALLBACK CNewLocalVariables::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CNewLocalVariables::Initialize(hWnd);
        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::Variables:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeVariable();
            break;
        case Controls::New:
            if (CODE == BN_CLICKED)
                OnClickNew();
            break;
        case Controls::Search:
            if (CODE == BN_CLICKED)
                OnClickSearchReference();
            break;
        case Controls::Name:
            if (CODE == EN_CHANGE && SelectedKey != "" && map.KeyExists("VariableNames", SelectedKey))
            {
                char buffer[512]{ 0 };
                GetWindowText(hName, buffer, 511);
                auto atom = FString::SplitString(map.GetString("VariableNames", SelectedKey), 1);
                FString value;
                value.Format("%s,%s", buffer, atom[1]);
                map.WriteString("VariableNames", SelectedKey, value);
                FString text;
                text.Format("%s - %s", SelectedKey, value);

                vcbVariables.ReplaceString(SelectedIndex, text);
                vcbVariables.SetCurSel(SelectedIndex);
            }
            break;
        case Controls::Value:
            if (CODE == EN_CHANGE && SelectedKey != "" && map.KeyExists("VariableNames", SelectedKey))
            {
                char buffer[512]{ 0 };
                GetWindowText(hValue, buffer, 511);
                auto atom = FString::SplitString(map.GetString("VariableNames", SelectedKey), 1);
                FString value;
                value.Format("%s,%s", atom[0], buffer);
                map.WriteString("VariableNames", SelectedKey, value);
                FString text;
                text.Format("%s - %s", SelectedKey, value);

                vcbVariables.ReplaceString(SelectedIndex, text);
                vcbVariables.SetCurSel(SelectedIndex);
            }
            break;
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CNewLocalVariables::Close(hWnd);
        return TRUE;
    }
    case WM_MEASUREITEM:
    {
        VirtualComboBoxEx::SetWindowHeight(hWnd, lParam);
        return TRUE;
    }
    case 114514: // used for update
    {
        Update();
        return TRUE;
    }
    }

    // Process this message through default handler
    return FALSE;
}

void CNewLocalVariables::Update()
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    ExtraWindow::ClearComboKeepText(hVariables);
    if (auto pSection = map.GetSection("VariableNames"))
    {
        int i = 0;
        FString key;
        for (const auto& [key, value] : pSection->GetEntities())
        { 
            FString text;
            text.Format("%s - %s", key, value);
            SendMessage(hVariables, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)text);
            i++;
        }
    }

    int count = SendMessage(hVariables, CB_GETCOUNT, NULL, NULL);
    if (SelectedIndex < 0)
        SelectedIndex = 0;
    if (SelectedIndex > count - 1)
        SelectedIndex = count - 1;
    SendMessage(hVariables, CB_SETCURSEL, SelectedIndex, NULL);
    OnSelchangeVariable();

    return;
}

void CNewLocalVariables::OnSelchangeVariable(bool edited)
{
    SelectedIndex = SendMessage(hVariables, CB_GETCURSEL, NULL, NULL);
    if (SelectedIndex < 0 || SelectedIndex >= SendMessage(hVariables, CB_GETCOUNT, NULL, NULL))
    {
        SelectedKey = "";
        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hValue, WM_SETTEXT, 0, (LPARAM)"");
        return;
    }

    FString key = vcbVariables.GetItemText(SelectedIndex);
    FString::TrimIndex(key);

    SelectedKey = key;

    auto atom = FString::SplitString(map.GetString("VariableNames", key), 1);

    SendMessage(hName, WM_SETTEXT, 0, (LPARAM)atom[0]);
    SendMessage(hValue, WM_SETTEXT, 0, (LPARAM)atom[1]);
}

void CNewLocalVariables::OnClickNew()
{
    auto key = CINI::GetAvailableKey("VariableNames");
    if (!ExtConfigs::ExtVariables && atoi(key) >= 100)
    {
        ::MessageBox(m_parent->m_hWnd,
            Translations::TranslateOrDefault("LocalVariablesExceed", "The number of local variables exceeds 100, following variables cannot be used. If you are using Phobos, please enable ExtVariables."),
            Translations::TranslateOrDefault("LocalVariablesExceedTitle", "Too Much Local Variables"), MB_OK);
        return;
    }

    FString value = "New Variable,0";
    map.WriteString("VariableNames", key, value);
    FString text;
    text.Format("%s - %s", key, value);
    int index = SendMessage(hVariables, CB_GETCOUNT, NULL, NULL);
    SendMessage(hVariables, CB_INSERTSTRING, index, (LPARAM)(LPCSTR)text);
    SendMessage(hVariables, CB_SETCURSEL, index, NULL);
    OnSelchangeVariable();
}

void CNewLocalVariables::OnClickSearchReference()
{
    if (SelectedIndex < 0 || SelectedKey == "" || !map.KeyExists("VariableNames", SelectedKey))
        return;

    CSearhReference::SetSearchType(3);
    CSearhReference::SetSearchID(SelectedKey);
    if (CSearhReference::GetHandle() == NULL)
    {
        CSearhReference::Create(m_parent);
    }
    else
    {
        ::SendMessage(CSearhReference::GetHandle(), 114514, 0, 0);
    }
}