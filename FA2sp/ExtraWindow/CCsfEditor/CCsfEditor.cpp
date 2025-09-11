#include "CCsfEditor.h"
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
#include "../CObjectSearch/CObjectSearch.h"
#include "../CNewINIEditor/CNewINIEditor.h"
#include "../CNewTrigger/CNewTrigger.h"
#include "../../Miscs/StringtableLoader.h"
#include "../../Miscs/DialogStyle.h"

HWND CCsfEditor::m_hwnd;
CFinalSunDlg* CCsfEditor::m_parent;

HWND CCsfEditor::hSelectedCSF;
HWND CCsfEditor::hNewFile;
HWND CCsfEditor::hSearch;
HWND CCsfEditor::hAdd;
HWND CCsfEditor::hClone;
HWND CCsfEditor::hDelete;
HWND CCsfEditor::hCSFViewer;
HWND CCsfEditor::hCSFEditor;
HWND CCsfEditor::hSave;
HWND CCsfEditor::hSetLabel;
HWND CCsfEditor::hReload;
HWND CCsfEditor::hApply;
std::map<FString, FString>& CCsfEditor::CurrentCSFMap = StringtableLoader::CSFFiles_Stringtable;
FString CCsfEditor::CurrentSelectedCSF;
FString CCsfEditor::CurrentSelectedCSFApply;
bool CCsfEditor::NeedUpdate = false;
WNDPROC CCsfEditor::g_pOriginalListViewProc = nullptr;

LRESULT CALLBACK CCsfEditor::ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DarkTheme::MyCallWindowProcA(g_pOriginalListViewProc, hWnd, uMsg, wParam, lParam);
}

void CCsfEditor::Create(CFinalSunDlg* pWnd)
{
    HMODULE hModule = LoadLibrary(TEXT("Riched32.dll"));
    if (!hModule)
        MessageBox(NULL, Translations::TranslateOrDefault("FailedLoadRiched32DLL", "Could not Load Riched32.dll£¡"), Translations::TranslateOrDefault("Error", "Error"), MB_ICONERROR);

    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(311),
        pWnd->GetSafeHwnd(),
        CCsfEditor::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CCsfEditor.\n");
        m_parent = NULL;
        return;
    }
}

void CCsfEditor::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("CsfEditorTitle", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };
    
    Translate(1000, "CsfEditorSelectedCsfFile");
    Translate(1002, "CsfEditorNewFile");
    Translate(1003, "CsfEditorSearchLabelText");
    Translate(1005, "CsfEditorAdd");
    Translate(1006, "CsfEditorClone");
    Translate(1007, "CsfEditorDelete");
    Translate(1010, "CsfEditorDescription1");
    Translate(1014, "CsfEditorDescription2");
    Translate(1011, "CsfEditorSave");
    Translate(1012, "CsfEditorSetLabelName");
    Translate(1015, "CsfEditorReload");
    Translate(1016, "CsfEditorApply");

    hSelectedCSF = GetDlgItem(hWnd, Controls::SelectedCSF);
    hNewFile = GetDlgItem(hWnd, Controls::NewFile);
    hSearch = GetDlgItem(hWnd, Controls::Search);
    hAdd = GetDlgItem(hWnd, Controls::Add);
    hClone = GetDlgItem(hWnd, Controls::Clone);
    hDelete = GetDlgItem(hWnd, Controls::Delete);
    hCSFViewer = GetDlgItem(hWnd, Controls::CSFViewer);
    hCSFEditor = GetDlgItem(hWnd, Controls::CSFEditor);
    hSave = GetDlgItem(hWnd, Controls::Save);
    hSetLabel = GetDlgItem(hWnd, Controls::SetLabel);
    hReload = GetDlgItem(hWnd, Controls::Reload);
    hApply = GetDlgItem(hWnd, Controls::Apply);

    SendMessage(hCSFViewer, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);
    SendMessage(hCSFEditor, EM_SETREADONLY, (WPARAM)TRUE, 0);
    ExtraWindow::SetEditControlFontSize(hCSFEditor, 1.4f, true);

    if (ExtConfigs::EnableDarkMode)
    {
        ::SendMessage(hCSFEditor, EM_SETBKGNDCOLOR, (WPARAM)FALSE, (LPARAM)RGB(32, 32, 32));
        CHARFORMAT cf = { 0 };
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_COLOR;
        cf.crTextColor = RGB(220, 220, 220);
        ::SendMessage(hCSFEditor, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
        ::SendMessage(hCSFViewer, LVM_SETTEXTBKCOLOR, 0, RGB(32, 32, 32));
        ::SendMessage(hCSFViewer, LVM_SETTEXTCOLOR, 0, RGB(220, 220, 220));

        DarkTheme::SubclassListViewHeader(hCSFViewer);
        g_pOriginalListViewProc = (WNDPROC)GetWindowLongPtr(hCSFViewer, GWLP_WNDPROC);
        if (g_pOriginalListViewProc)
        {
            SetWindowLongPtr(hCSFViewer, GWLP_WNDPROC, (LONG_PTR)ListViewSubclassProc);
        }
        InvalidateRect(hCSFViewer, NULL, TRUE);
    }

    Update(hWnd);
}

void CCsfEditor::Close(HWND& hWnd)
{
    // reduce lag
    //EndDialog(hWnd, NULL);
    ShowWindow(hWnd, SW_HIDE);

    //CCsfEditor::m_hwnd = NULL;
    //CCsfEditor::m_parent = NULL;

}

void CCsfEditor::Update(HWND& hWnd)
{
    InsertCSFContent(CurrentCSFMap);
    SendMessage(hCSFEditor, WM_SETTEXT, 0, (LPARAM)(LPCSTR)"");
    SendMessage(hSetLabel, WM_SETTEXT, 0, (LPARAM)(LPCSTR)"");
    CurrentSelectedCSFApply = "";

    char buffer[512]{ 0 };
    GetWindowText(hSearch, buffer, 511);
    if (strlen(buffer) != 0)
        OnEditchangeSearch();

    NeedUpdate = false;
}

BOOL CALLBACK CCsfEditor::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CCsfEditor::Initialize(hWnd);
        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::Search:
            if (CODE == EN_CHANGE)
                OnEditchangeSearch();
            break;
        case Controls::Reload:
            if (CODE == BN_CLICKED)
            {
                CFinalSunDlg::LastSucceededOperation = 9;
                StringtableLoader::CSFFiles_Stringtable.clear();
                StringtableLoader::LoadCSFFiles();
                Logger::Debug("Successfully loaded %d csf labels.\n", StringtableLoader::CSFFiles_Stringtable.size());

                char tmpCsfFile[0x400];
                strcpy_s(tmpCsfFile, CFinalSunApp::ExePath());
                strcat_s(tmpCsfFile, "\\RA2Tmp.csf");
                DeleteFile(tmpCsfFile);

                ExtraWindow::bEnterSearch = true;
                Update(hWnd);
                ExtraWindow::bEnterSearch = false;
                ((CViewObjectsExt*)(CFinalSunDlg::Instance->MyViewFrame.pViewObjects))->Redraw();
            }
            break;
        case Controls::Apply:
            if (CODE == BN_CLICKED)
            {
                OnClickApply();
            }
            break;
        default:
            break;
        }
        break;
    }
    case WM_NOTIFY:
    {
        LPNMHDR pNMHDR = (LPNMHDR)lParam;
        if (pNMHDR->idFrom == Controls::CSFViewer && pNMHDR->code == LVN_ITEMCHANGED)
        {
            OnViewerSelectedChange(pNMHDR);
        }
        else if (pNMHDR->idFrom == Controls::CSFViewer && pNMHDR->code == NM_DBLCLK)
        {
            OnClickApply();
        }
        break;
    }
    case WM_CLOSE:
    {
        CCsfEditor::Close(hWnd);
        return TRUE;
    }
    case 114514: // used for update
    {
        if (NeedUpdate)
            Update(hWnd);
        SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        return TRUE;
    }
    case 114515: // used for update
    {
        if (CurrentSelectedCSF != "")
        {
            auto it = CurrentCSFMap.find(CurrentSelectedCSF);
            if (it != CurrentCSFMap.end())
            {
                int index = std::distance(CurrentCSFMap.begin(), it);

                LVITEM lvItem = { 0 };
                lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                lvItem.state = LVIS_SELECTED | LVIS_FOCUSED;

                SendMessage(hCSFViewer, LVM_SETITEMSTATE, (WPARAM)index, (LPARAM)&lvItem);
                SendMessage(hCSFViewer, LVM_ENSUREVISIBLE, (WPARAM)index, TRUE);

                SetFocus(hCSFViewer);
            }
        }
        CurrentSelectedCSF = "";
        return TRUE;
    }
    case 114516: // used for update
    {
        Update(hWnd);
        return TRUE;
    }
    }

    // Process this message through default handler
    return FALSE;
}

void CCsfEditor::OnViewerSelectedChange(NMHDR* pNMHDR)
{
    LPNMLISTVIEW pNMListView = (LPNMLISTVIEW)pNMHDR;
    if ((pNMListView->uChanged & LVIF_STATE) &&
        (pNMListView->uNewState & LVIS_SELECTED)) 
    {
        int selectedRow = pNMListView->iItem;
        
        if (selectedRow >= 0) 
        {
            char buffer[512] = { 0 };
            LVITEM lvItem = { 0 };
            lvItem.iSubItem = 0;
            lvItem.pszText = buffer;
            lvItem.cchTextMax = sizeof(buffer);
            lvItem.iItem = selectedRow;
            SendMessage(hCSFViewer, LVM_GETITEMTEXT, selectedRow, (LPARAM)&lvItem);

            FString value = "";
            auto it = CurrentCSFMap.find(buffer);
            if (it != CurrentCSFMap.end())
                value = CurrentCSFMap[buffer];

            SendMessage(hCSFEditor, WM_SETTEXT, 0, value);
            SendMessage(hSetLabel, WM_SETTEXT, 0, (LPARAM)(LPCSTR)buffer);

            CurrentSelectedCSFApply = buffer;
        }
        else
        {
            SendMessage(hCSFEditor, WM_SETTEXT, 0, (LPARAM)(LPCSTR)"");
            SendMessage(hSetLabel, WM_SETTEXT, 0, (LPARAM)(LPCSTR)"");
            CurrentSelectedCSFApply = "";
        }
    }
    
}

void CCsfEditor::OnClickApply()
{
    if (CurrentSelectedCSFApply == "")
        return;

    if (IsWindowVisible(CNewTrigger::GetHandle()))
    {
        if (CNewTrigger::CurrentCSFActionParam >= 0)
        {
            FString text;
            auto it = CurrentCSFMap.find(CurrentSelectedCSFApply);
            if (it != CurrentCSFMap.end())
                text.Format("%s - %s", CurrentSelectedCSFApply, CurrentCSFMap[CurrentSelectedCSFApply]);
            SendMessage(CNewTrigger::hActionParameter[CNewTrigger::CurrentCSFActionParam], WM_SETTEXT, 0, text);
            CNewTrigger::OnSelchangeActionParam(CNewTrigger::CurrentCSFActionParam, true);
        }

    } 
}

void CCsfEditor::InsertCSFContent(std::map<FString, FString> csfMap)
{
    SendMessage(hCSFViewer, LVM_DELETEALLITEMS, 0, 0);
    while (SendMessage(hCSFViewer, LVM_DELETECOLUMN, 0, 0)) {}

    LVCOLUMN lvColumn = { 0 };
    lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvColumn.pszText = const_cast<LPSTR>(Translations::TranslateOrDefault("CsfEditorColumnLabel", "Label"));
    lvColumn.cx = 100;
    SendMessage(hCSFViewer, LVM_INSERTCOLUMN, 0, (LPARAM)&lvColumn);

    lvColumn.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvColumn.pszText = const_cast<LPSTR>(Translations::TranslateOrDefault("CsfEditorColumnText", "Text"));
    lvColumn.cx = 400;
    SendMessage(hCSFViewer, LVM_INSERTCOLUMN, 1, (LPARAM)&lvColumn);

    LVITEM lvItem = { 0 };
    lvItem.mask = LVIF_TEXT;


    int i = 0;
    for (auto& csf : csfMap)
    {
        LVITEM lvItem = { 0 };
        lvItem.mask = LVIF_TEXT;
        lvItem.iItem = i;
        lvItem.iSubItem = 0;

        lvItem.pszText = const_cast<LPSTR>(csf.first.c_str());

        SendMessage(hCSFViewer, LVM_INSERTITEM, 0, (LPARAM)&lvItem);

        LVITEM subItem = { 0 };
        subItem.mask = LVIF_TEXT;
        subItem.iItem = i;
        subItem.iSubItem = 1;

        subItem.pszText = const_cast<LPSTR>(csf.second.c_str());

        SendMessage(hCSFViewer, LVM_SETITEM, 0, (LPARAM)&subItem);

        i++;
    }

    SendMessage(hCSFViewer, LVM_SETCOLUMNWIDTH, 1, LVSCW_AUTOSIZE_USEHEADER);
}

void CCsfEditor::FilterRows(std::map<FString, FString> csfMap, const char* searchText)
{
    std::map<FString, FString> newCsfMap;

    for (auto& csf : csfMap) 
    {
        if (ExtraWindow::IsLabelMatch(csf.first, searchText) || ExtraWindow::IsLabelMatch(csf.second, searchText)) 
        {
            newCsfMap[csf.first] = csf.second;
        }
    }
    CCsfEditor::InsertCSFContent(newCsfMap);
}

void CCsfEditor::OnEditchangeSearch()
{

    if (CurrentCSFMap.size() > ExtConfigs::SearchCombobox_MaxCount && !ExtraWindow::bEnterSearch)
    {
        return;
    }

    char buffer[512]{ 0 };

    GetWindowText(hSearch, buffer, 511);
    if (strlen(buffer) == 0)
    {
        InsertCSFContent(CurrentCSFMap);
    }
    else
    {
        FilterRows(CurrentCSFMap, buffer);
    }
    NeedUpdate = false;
}

bool CCsfEditor::OnEnterKeyDown(HWND& hWnd)
{
    if (hWnd == hSearch)
        OnEditchangeSearch();
    else
        return false;
    return true;
}