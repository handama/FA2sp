#include "CNewTaskforce.h"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/MultimapHelper.h"
#include "../Common.h"

#include <CLoading.h>
#include <CFinalSunDlg.h>
#include <CObjectDatas.h>
#include <CMapData.h>
#include <CIsoView.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CMapData/Body.h"
#include <Miscs/Miscs.h>
#include "../CObjectSearch/CObjectSearch.h"
#include "../../Ext/CTileSetBrowserFrame/TabPages/TeamSort.h"
#include "../../Ext/CTileSetBrowserFrame/TabPages/TaskForceSort.h"
#include "../CNewScript/CNewScript.h"
#include <numeric>
#include "../CSearhReference/CSearhReference.h"
#include "../CTriggerAnnotation/CTriggerAnnotation.h"

HWND CNewTaskforce::m_hwnd;
CFinalSunDlg* CNewTaskforce::m_parent;
CINI& CNewTaskforce::map = CINI::CurrentDocument;
MultimapHelper& CNewTaskforce::rules = Variables::RulesMap;


HWND CNewTaskforce::hSelectedTaskforce;
HWND CNewTaskforce::hNewTaskforce;
HWND CNewTaskforce::hDelTaskforce;
HWND CNewTaskforce::hCloTaskforce;
HWND CNewTaskforce::hAddUnit;
HWND CNewTaskforce::hDeleteUnit;
HWND CNewTaskforce::hName;
HWND CNewTaskforce::hGroup;
HWND CNewTaskforce::hUnitsListBox;
HWND CNewTaskforce::hNumber;
HWND CNewTaskforce::hUnitType;
HWND CNewTaskforce::hSearchReference;

int CNewTaskforce::SelectedTaskForceIndex = -1;
FString CNewTaskforce::CurrentTaskForceID;
std::map<int, FString> CNewTaskforce::TaskForceLabels;
std::map<int, FString> CNewTaskforce::UnitTypeLabels;
bool CNewTaskforce::Autodrop;
bool CNewTaskforce::DropNeedUpdate;
WNDPROC CNewTaskforce::OriginalListBoxProc;

void CNewTaskforce::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(305),
        pWnd->GetSafeHwnd(),
        CNewTaskforce::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CNewTaskforce.\n");
        m_parent = NULL;
        return;
    }
}

void CNewTaskforce::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("TaskforceTitle", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };
    
    Translate(50800, "TaskforceList");
    Translate(50801, "TaskforceUnits");
    Translate(50802, "TaskforceGroup");
    Translate(50803, "TaskforceUnitNumber");
    Translate(50804, "TaskforceUnitType");
    Translate(50805, "TaskforceSelected");
    Translate(50806, "TaskforceName");
    Translate(1151, "TaskforceNewTask");
    Translate(1150, "TaskforceDelTask");
    Translate(50807, "TaskforceCloTask");
    Translate(1146, "TaskforceNewUnit");
    Translate(1147, "TaskforceDelUnit");
    Translate(1999, "SearchReferenceTitle");
    
    hSelectedTaskforce = GetDlgItem(hWnd, Controls::SelectedTaskforce);
    hNewTaskforce = GetDlgItem(hWnd, Controls::NewTaskforce);
    hDelTaskforce = GetDlgItem(hWnd, Controls::DelTaskforce);
    hCloTaskforce = GetDlgItem(hWnd, Controls::CloTaskforce);
    hAddUnit = GetDlgItem(hWnd, Controls::AddUnit);
    hDeleteUnit = GetDlgItem(hWnd, Controls::DeleteUnit);
    hName = GetDlgItem(hWnd, Controls::Name);
    hGroup = GetDlgItem(hWnd, Controls::Group);
    hUnitsListBox = GetDlgItem(hWnd, Controls::UnitsListBox);
    hNumber = GetDlgItem(hWnd, Controls::Number);
    hUnitType = GetDlgItem(hWnd, Controls::UnitType);
    hSearchReference = GetDlgItem(hWnd, Controls::SearchReference);

    if (hUnitsListBox)
        OriginalListBoxProc = (WNDPROC)SetWindowLongPtr(hUnitsListBox, GWLP_WNDPROC, (LONG_PTR)ListBoxSubclassProc);

    Update(hWnd);
}

void CNewTaskforce::Update(HWND& hWnd)
{
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    if (TaskforceSort::Instance.IsVisible())
        TaskforceSort::Instance.LoadAllTriggers();

    DropNeedUpdate = false;

    int idx = 0;

    ExtraWindow::SortTeams(hSelectedTaskforce, "TaskForces", SelectedTaskForceIndex);

    int count = SendMessage(hSelectedTaskforce, CB_GETCOUNT, NULL, NULL);
    if (SelectedTaskForceIndex < 0)
        SelectedTaskForceIndex = 0;
    if (SelectedTaskForceIndex > count - 1)
        SelectedTaskForceIndex = count - 1;
    SendMessage(hSelectedTaskforce, CB_SETCURSEL, SelectedTaskForceIndex, NULL);


    while (SendMessage(hUnitType, CB_DELETESTRING, 0, NULL) != CB_ERR);
    ExtraWindow::LoadParam_TechnoTypes(hUnitType, 4, 1);
    Autodrop = false;
    
    OnSelchangeTaskforce();
}

void CNewTaskforce::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CNewTaskforce::m_hwnd = NULL;
    CNewTaskforce::m_parent = NULL;

}

LRESULT CALLBACK CNewTaskforce::ListBoxSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_MOUSEWHEEL:

        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);
        RECT rc;
        GetClientRect(hWnd, &rc);

        if (pt.x >= rc.right)
        {
            return CallWindowProc(OriginalListBoxProc, hWnd, message, wParam, lParam);
        }
        else
        {
            int nCurSel = (int)SendMessage(hWnd, LB_GETCURSEL, 0, 0);
            int nCount = (int)SendMessage(hWnd, LB_GETCOUNT, 0, 0);

            if (nCurSel != LB_ERR && nCount > 0)
            {
                if ((short)HIWORD(wParam) > 0 && nCurSel > 0)
                {
                    SendMessage(hWnd, LB_SETCURSEL, nCurSel - 1, 0);
                }
                else if ((short)HIWORD(wParam) < 0 && nCurSel < nCount - 1)
                {
                    SendMessage(hWnd, LB_SETCURSEL, nCurSel + 1, 0);
                }
                OnSelchangeUnitListbox();

            }
            else {
                SendMessage(hWnd, LB_SETCURSEL, 0, 0);
            }
            return TRUE;
        }
    }
    return CallWindowProc(OriginalListBoxProc, hWnd, message, wParam, lParam);
}
BOOL CALLBACK CNewTaskforce::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_ACTIVATE:
    {
        if (SelectedTaskForceIndex >= 0 && SelectedTaskForceIndex < SendMessage(hSelectedTaskforce, CB_GETCOUNT, NULL, NULL))
        {
            CTriggerAnnotation::Type = AnnoTaskforce;
            CTriggerAnnotation::ID = CurrentTaskForceID;
            ::SendMessage(CTriggerAnnotation::GetHandle(), 114515, 0, 0);
        }
        return TRUE;
    }
    case WM_INITDIALOG:
    {
        CNewTaskforce::Initialize(hWnd);
        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::UnitsListBox:
            ListBoxProc(hWnd, CODE, lParam);
            break;
        case Controls::NewTaskforce:
            if (CODE == BN_CLICKED)
                OnClickNewTaskforce();
            break;
        case Controls::DelTaskforce:
            if (CODE == BN_CLICKED)
                OnClickDelTaskforce(hWnd);
            break;
        case Controls::CloTaskforce:
            if (CODE == BN_CLICKED)
                OnClickCloTaskforce(hWnd);
            break;
        case Controls::AddUnit:
            if (CODE == BN_CLICKED)
                OnClickAddUnit(hWnd);
            break;
        case Controls::DeleteUnit:
            if (CODE == BN_CLICKED)
                OnClickDeleteUnit(hWnd);
            break;
        case Controls::SearchReference:
            if (CODE == BN_CLICKED)
                OnClickSearchReference(hWnd);
            break;
        case Controls::SelectedTaskforce:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTaskforce();
            else if (CODE == CBN_DROPDOWN)
                OnSeldropdownTaskforce(hWnd);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeTaskforce(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupTaskforce();
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::Name:
            if (CODE == EN_CHANGE)
            {
                if (SelectedTaskForceIndex < 0)
                    break;
                char buffer[512]{ 0 };
                GetWindowText(hName, buffer, 511);
                map.WriteString(CurrentTaskForceID, "Name", buffer);

                DropNeedUpdate = true;

                FString name;
                name.Format("%s (%s)", CurrentTaskForceID, buffer);
                SendMessage(hSelectedTaskforce, CB_DELETESTRING, SelectedTaskForceIndex, NULL);
                SendMessage(hSelectedTaskforce, CB_INSERTSTRING, SelectedTaskForceIndex, (LPARAM)(LPCSTR)name);
                SendMessage(hSelectedTaskforce, CB_SETCURSEL, SelectedTaskForceIndex, NULL);
            }
            break;
        case Controls::Number:
            if (CODE == EN_CHANGE)
                OnEditchangeNumber();
            break;
        case Controls::Group:
            if (CODE == EN_CHANGE)
            {
                if (SelectedTaskForceIndex < 0)
                    break;
                char buffer[512]{ 0 };
                GetWindowText(hGroup, buffer, 511);
                map.WriteString(CurrentTaskForceID, "Group", buffer);
            }
            break;
        case Controls::UnitType:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeUnitType();
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeUnitType(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupUnitType();
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CNewTaskforce::Close(hWnd);
        return TRUE;
    }
    case 114514: // used for update
    {
        Update(hWnd);
        return TRUE;
    }

    }

    // Process this message through default handler
    return FALSE;
}

void CNewTaskforce::ListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam)
{
    if (SendMessage(hUnitsListBox, LB_GETCOUNT, NULL, NULL) <= 0)
        return;

    switch (nCode)
    {
    case LBN_SELCHANGE:
    case LBN_DBLCLK:
        OnSelchangeUnitListbox();
        break;
    default:
        break;
    }
   
}


void CNewTaskforce::OnEditchangeNumber()
{
    if (SelectedTaskForceIndex < 0 || SendMessage(hUnitsListBox, LB_GETCURSEL, NULL, NULL) < 0)
        return;
    char buffer[512]{ 0 };
    GetWindowText(hNumber, buffer, 511);
    int idx = SendMessage(hUnitsListBox, LB_GETCURSEL, 0, NULL);
    FString key;
    key.Format("%d", idx);
    auto value = map.GetString(CurrentTaskForceID, key);
    auto atoms = FString::SplitString(value, 1);
    value.Format("%s,%s", buffer, atoms[1]);

    map.WriteString(CurrentTaskForceID, key, value);

    value = map.GetString(CurrentTaskForceID, key);
    atoms = FString::SplitString(value, 1);
    FString text;
    text.Format("%s %s (%s)", atoms[0], atoms[1], CViewObjectsExt::QueryUIName(atoms[1], true));
    SendMessage(hUnitsListBox, LB_DELETESTRING, idx, NULL);
    SendMessage(hUnitsListBox, LB_INSERTSTRING, idx, (LPARAM)(LPCSTR)text);
    SendMessage(hUnitsListBox, LB_SETCURSEL, idx, NULL);

}

void CNewTaskforce::OnSelchangeUnitListbox()
{
    if (SelectedTaskForceIndex < 0 || SendMessage(hUnitsListBox, LB_GETCURSEL, NULL, NULL) < 0 || SendMessage(hUnitsListBox, LB_GETCOUNT, NULL, NULL) <= 0)
    {
        SendMessage(hUnitType, CB_SETCURSEL, -1, NULL);
        SendMessage(hNumber, WM_SETTEXT, 0, (LPARAM)"");
        return;
    }


    int idx = SendMessage(hUnitsListBox, LB_GETCURSEL, 0, NULL);
    if (idx < 0)
        idx = 0;
    if (idx > 5)
        idx = 5;

    FString key;
    key.Format("%d", idx);
    auto value = map.GetString(CurrentTaskForceID, key);
    auto atoms = FString::SplitString(value, 1);

    SendMessage(hNumber, WM_SETTEXT, 0, (LPARAM)atoms[0]);

    FString text;
    text.Format("%s (%s)", atoms[1], CViewObjectsExt::QueryUIName(atoms[1], true));
    int unitIdx = SendMessage(hUnitType, CB_FINDSTRINGEXACT, 0, (LPARAM)text);

    if (unitIdx != CB_ERR)
        SendMessage(hUnitType, CB_SETCURSEL, unitIdx, NULL);
    else
        SendMessage(hUnitType, WM_SETTEXT, 0, (LPARAM)atoms[1]);

}

void CNewTaskforce::OnSelchangeUnitType(bool edited)
{
    if (SelectedTaskForceIndex < 0 || SendMessage(hUnitsListBox, LB_GETCURSEL, NULL, NULL) < 0)
        return;
    int curSel = SendMessage(hUnitType, CB_GETCURSEL, NULL, NULL);

    FString text;
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };
    
    if (edited && (SendMessage(hUnitType, CB_GETCOUNT, NULL, NULL) > 0 || !UnitTypeLabels.empty()))
    {
        ExtraWindow::OnEditCComboBox(hUnitType, UnitTypeLabels);
    }
    
    if (curSel >= 0 && curSel < SendMessage(hUnitType, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hUnitType, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hUnitType, buffer, 511);
        text = buffer;
    }
    
    if (!text)
        return;
    
    FString::TrimIndex(text);
    if (text == "None")
        text = "";

    text.Replace(",", "");

    int idx = SendMessage(hUnitsListBox, LB_GETCURSEL, 0, NULL);
    FString key;
    key.Format("%d", idx);
    auto value = map.GetString(CurrentTaskForceID, key);
    auto atoms = FString::SplitString(value, 1);
    value.Format("%s,%s", atoms[0], text);
    
    map.WriteString(CurrentTaskForceID, key, value);

    value = map.GetString(CurrentTaskForceID, key);
    atoms = FString::SplitString(value, 1);
    text.Format("%s %s (%s)", atoms[0], atoms[1], CViewObjectsExt::QueryUIName(atoms[1], true));
    SendMessage(hUnitsListBox, LB_DELETESTRING, idx, NULL);
    SendMessage(hUnitsListBox, LB_INSERTSTRING, idx, (LPARAM)(LPCSTR)text);
    SendMessage(hUnitsListBox, LB_SETCURSEL, idx, NULL);
}

void CNewTaskforce::OnCloseupUnitType()
{
    if (!ExtraWindow::OnCloseupCComboBox(hUnitType, UnitTypeLabels))
        OnSelchangeUnitListbox();

}

void CNewTaskforce::OnSeldropdownTaskforce(HWND& hWnd)
{
    if (Autodrop)
    {
        Autodrop = false;
        return;
    }
    if (!DropNeedUpdate)
        return;

    DropNeedUpdate = false;

    ExtraWindow::SortTeams(hSelectedTaskforce, "TaskForces", SelectedTaskForceIndex, CurrentTaskForceID);
}

void CNewTaskforce::OnSelchangeTaskforce(bool edited, int specificIdx)
{
    char buffer[512]{ 0 };
    char buffer2[512]{ 0 };

    if (edited && (SendMessage(hSelectedTaskforce, CB_GETCOUNT, NULL, NULL) > 0 || !TaskForceLabels.empty()))
    {
        Autodrop = true;
        ExtraWindow::OnEditCComboBox(hSelectedTaskforce, TaskForceLabels);
        return;
    }

    SelectedTaskForceIndex = SendMessage(hSelectedTaskforce, CB_GETCURSEL, NULL, NULL);
    if (SelectedTaskForceIndex < 0 || SelectedTaskForceIndex >= SendMessage(hSelectedTaskforce, CB_GETCOUNT, NULL, NULL)) 
    {
        SendMessage(hUnitType, CB_SETCURSEL, -1, NULL);
        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hGroup, WM_SETTEXT, 0, (LPARAM)"");
        while (SendMessage(hUnitsListBox, LB_DELETESTRING, 0, NULL) != CB_ERR);
    }

    FString pID;
    SendMessage(hSelectedTaskforce, CB_GETLBTEXT, SelectedTaskForceIndex, (LPARAM)buffer);
    pID = buffer;
    FString::TrimIndex(pID);

    CurrentTaskForceID = pID;

    CTriggerAnnotation::Type = AnnoTaskforce;
    CTriggerAnnotation::ID = CurrentTaskForceID;
    ::SendMessage(CTriggerAnnotation::GetHandle(), 114515, 0, 0);

    ::SendMessage(CTriggerAnnotation::GetHandle(), 114515, 0, 0);
    while (SendMessage(hUnitsListBox, LB_DELETESTRING, 0, NULL) != CB_ERR);
    if (auto pTaskforce = map.GetSection(pID))
    {
        auto name = map.GetString(pID, "Name");
        auto group = map.GetString(pID, "Group");
        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)name.m_pchData);
        SendMessage(hGroup, WM_SETTEXT, 0, (LPARAM)group.m_pchData);

        std::vector<FString> sortedList;
        for (int i = 0; i < 6; i++)
        {
            FString key;
            key.Format("%d", i);
            auto value = map.GetString(pID, key);
            if (value != "")
            {
                if (FString::SplitString(value).size() == 2)
                    sortedList.push_back(value);
            }
            map.DeleteKey(pID, key);
        }
        int i = 0;
        for (auto& value : sortedList)
        {
            auto atoms = FString::SplitString(value, 1);
            FString text;
            text.Format("%s %s (%s)", atoms[0], atoms[1], CViewObjectsExt::QueryUIName(atoms[1], true));
            SendMessage(hUnitsListBox, LB_ADDSTRING, 0, (LPARAM)(LPCSTR)text);
            FString key;
            key.Format("%d", i);
            map.WriteString(pID, key, value);
            i++;
        }
    }
    if (specificIdx > -1)
    {
        while (specificIdx >= SendMessage(hUnitsListBox, LB_GETCOUNT, NULL, NULL))
            specificIdx--;
        SendMessage(hUnitsListBox, LB_SETCURSEL, specificIdx, NULL);
    }
    else
        if (SendMessage(hUnitsListBox, LB_GETCOUNT, NULL, NULL) > 0)
            SendMessage(hUnitsListBox, LB_SETCURSEL, 0, NULL);

    OnSelchangeUnitListbox();
    DropNeedUpdate = false;
}

void CNewTaskforce::OnCloseupTaskforce()
{
    if (!ExtraWindow::OnCloseupCComboBox(hSelectedTaskforce, TaskForceLabels, true))
        OnSelchangeTaskforce();
}

void CNewTaskforce::OnClickNewTaskforce()
{
    FString key = CINI::GetAvailableKey("TaskForces");
    FString value = CMapDataExt::GetAvailableIndex();
    FString buffer2;

    FString newName = "";
    if (TaskforceSort::CreateFromTaskForceSort)
        newName = TaskforceSort::Instance.GetCurrentPrefix();
    newName += "New task force";
    map.WriteString("TaskForces", key, value);
    map.WriteString(value, "Name", newName);
    map.WriteString(value, "Group", "-1");

    ExtraWindow::SortTeams(hSelectedTaskforce, "TaskForces", SelectedTaskForceIndex, value);

    OnSelchangeTaskforce();
}

void CNewTaskforce::OnClickDelTaskforce(HWND& hWnd)
{
    if (SelectedTaskForceIndex < 0)
        return;
    int result = MessageBox(hWnd,
        Translations::TranslateOrDefault("TaskforceDelTFWarn", "Are you sure to delete the selected task force? If you delete it, make sure to eliminate ANY references to this task force in team-types."),
        Translations::TranslateOrDefault("TaskforceDelTFTitle", "Delete task force"), MB_YESNO);

    if (result == IDNO)
        return;

    map.DeleteSection(CurrentTaskForceID);
    std::vector<FString> deteleKeys;
    if (auto pSection = map.GetSection("TaskForces"))
    {
        for (auto& pair : pSection->GetEntities())
        {
            if (CurrentTaskForceID == pair.second)
                deteleKeys.push_back(pair.first); 
        }
    }
    for (auto& key : deteleKeys)
        map.DeleteKey("TaskForces", key);
    
    int idx = SelectedTaskForceIndex;
    SendMessage(hSelectedTaskforce, CB_DELETESTRING, idx, NULL);
    if (idx >= SendMessage(hSelectedTaskforce, CB_GETCOUNT, NULL, NULL))
        idx--;
    if (idx < 0)
        idx = 0;
    SendMessage(hSelectedTaskforce, CB_SETCURSEL, idx, NULL);
    OnSelchangeTaskforce();
}

void CNewTaskforce::OnClickCloTaskforce(HWND& hWnd)
{
    if (SelectedTaskForceIndex < 0)
        return;
    if (SendMessage(hSelectedTaskforce, CB_GETCOUNT, NULL, NULL) > 0 && SelectedTaskForceIndex >= 0)
    {
        FString key = CINI::GetAvailableKey("TaskForces");
        FString value = CMapDataExt::GetAvailableIndex();

        CINI::CurrentDocument->WriteString("TaskForces", key, value);

        auto oldname = CINI::CurrentDocument->GetString(CurrentTaskForceID, "Name", "New task force");
        FString newName = ExtraWindow::GetCloneName(oldname);
       
        CINI::CurrentDocument->WriteString(value, "Name", newName);

        auto copyitem = [&value](FString key)
            {
                if (auto ppStr = map.TryGetString(CurrentTaskForceID, key)) {
                    FString str = *ppStr;
                    str.Trim();
                    map.WriteString(value, key, str);
                }
            };

        copyitem("Group");
        copyitem("0");
        copyitem("1");
        copyitem("2");
        copyitem("3");
        copyitem("4");
        copyitem("5");

        ExtraWindow::SortTeams(hSelectedTaskforce, "TaskForces", SelectedTaskForceIndex, value);

        OnSelchangeTaskforce();
    }
}

void CNewTaskforce::OnClickAddUnit(HWND& hWnd)
{
    if (SelectedTaskForceIndex < 0)
        return;
    int count = SendMessage(hUnitsListBox, LB_GETCOUNT, 0, NULL);
    if (count >= 6)
        return;

    char buffer[512]{ 0 };
    FString text;
    FString key;
    key.Format("%d", count);
    map.WriteString(CurrentTaskForceID, key, "1,E1");

    text.Format("%s %s (%s)", "1", "E1", CViewObjectsExt::QueryUIName("E1", true));
    SendMessage(hUnitsListBox, LB_INSERTSTRING, count, (LPARAM)(LPCSTR)text);
    SendMessage(hUnitsListBox, LB_SETCURSEL, count, NULL);
    OnSelchangeUnitListbox();
}
void CNewTaskforce::OnClickDeleteUnit(HWND& hWnd)
{
    if (SelectedTaskForceIndex < 0 || SendMessage(hUnitsListBox, LB_GETCURSEL, NULL, NULL) < 0)
        return;
    int idx = SendMessage(hUnitsListBox, LB_GETCURSEL, 0, NULL);
    SendMessage(hUnitsListBox, LB_DELETESTRING, idx, NULL);
    FString key;
    key.Format("%d", idx);
    map.DeleteKey(CurrentTaskForceID, key);

    OnSelchangeTaskforce(false, idx);
}

void CNewTaskforce::OnClickSearchReference(HWND& hWnd)
{
    if (SelectedTaskForceIndex < 0)
        return;

    CSearhReference::SetSearchType(2);
    CSearhReference::SetSearchID(CurrentTaskForceID);
    if (CSearhReference::GetHandle() == NULL)
    { 
        CSearhReference::Create(m_parent);
    }
    else
    {
        ::SendMessage(CSearhReference::GetHandle(), 114514, 0, 0);
    }
}

bool CNewTaskforce::OnEnterKeyDown(HWND& hWnd)
{
    if (hWnd == hSelectedTaskforce)
        OnSelchangeTaskforce(true);
    else if (hWnd == hUnitType)
        OnSelchangeUnitType(true);
    else
        return false;
    return true;
}