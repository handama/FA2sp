#include "CTriggerAnnotation.h"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include <CFinalSunDlg.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CMapData/Body.h"
#include "../../Miscs/SaveMap.h"
#include "../Common.h"
#include "../../Miscs/DialogStyle.h"
#include "../../Ext/CFinalSunApp/Body.h"
#define BUFFER_SIZE 800000
HWND CTriggerAnnotation::m_hwnd;
HWND CTriggerAnnotation::hText;
HWND CTriggerAnnotation::hEdit;
CFinalSunDlg* CTriggerAnnotation::m_parent;
int CTriggerAnnotation::origWndWidth;
int CTriggerAnnotation::origWndHeight;
int CTriggerAnnotation::minWndWidth;
int CTriggerAnnotation::minWndHeight;
bool CTriggerAnnotation::minSizeSet;
FString CTriggerAnnotation::ID;
AnnotationType CTriggerAnnotation::Type;
char Buffer[BUFFER_SIZE]{ 0 };

void CTriggerAnnotation::Create(CFinalSunDlg* pWnd)
{
    HMODULE hModule = LoadLibrary(TEXT("Riched32.dll"));
    if (!hModule)
        MessageBox(NULL, Translations::TranslateOrDefault("FailedLoadRiched32DLL", "Could not Load Riched32.dll£¡"), Translations::TranslateOrDefault("Error", "Error"), MB_ICONERROR);

    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(328),
        pWnd->MyViewFrame.GetSafeHwnd(),
        CTriggerAnnotation::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CTriggerAnnotation.\n");
        m_parent = NULL;
    }
}

void CTriggerAnnotation::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("TriggerAnnotation.Title", buffer))
        SetWindowText(hWnd, buffer);

    hText = GetDlgItem(hWnd, Controls::Text);
    hEdit = GetDlgItem(hWnd, Controls::Edit);

    ExtraWindow::SetEditControlFontSize(hEdit, 1.4f, true, "Consolas");
    SendMessage(hEdit, EM_LIMITTEXT, (WPARAM)BUFFER_SIZE, 0);
    SendMessage(hEdit, EM_SETEVENTMASK, 0, (LPARAM)(ENM_CHANGE));

    if (ExtConfigs::EnableDarkMode)
    {
        ::SendMessage(hEdit, EM_SETBKGNDCOLOR, (WPARAM)FALSE, (LPARAM)RGB(32, 32, 32));
        CHARFORMAT cf = { 0 };
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_COLOR;
        cf.crTextColor = RGB(220, 220, 220);
        ::SendMessage(hEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    }

    Update();
}

void CTriggerAnnotation::Update(const char* filter)
{
    if (ID.IsEmpty())
    {
        SendMessage(hText, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)"");
        return;
    }
    FString name, text, edit;
    switch (Type)
    {
    case AnnoTrigger:
        name = ExtraWindow::GetTriggerDisplayName(ID);
        break;
    case AnnoTeam:
    case AnnoScript:
    case AnnoTaskforce:
        name = ExtraWindow::GetTeamDisplayName(ID);
        break;
    case AnnoAITrigger:
        name = ExtraWindow::GetAITriggerDisplayName(ID);
        break;
    case AnnoTag:
        name = ExtraWindow::GetTagDisplayName(ID);
        break;
    default:
        break;
    }

    text.Format(Translations::TranslateOrDefault("TriggerAnnotation.Text", "Current Object: %s"), name);
    SendMessage(hText, WM_SETTEXT, 0, text);

    FString annotation = CINI::CurrentDocument->GetString("TriggerAnnotations", ID);
    if (!annotation.IsEmpty())
    {
        size_t start = annotation.Find("START,");
        if (start != 0)
            start = 0;
        else
            start = 6;
        size_t end = annotation.Find(",END");
        edit = annotation.Mid(start, end - start);
        edit.Replace("\\n", "\n");
    }
    SendMessage(hEdit, WM_SETTEXT, 0, edit);

    return;
}

void CTriggerAnnotation::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CTriggerAnnotation::m_hwnd = NULL;
    CTriggerAnnotation::m_parent = NULL;
}

BOOL CALLBACK CTriggerAnnotation::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CTriggerAnnotation::Initialize(hWnd);
        RECT rect;
        GetClientRect(hWnd, &rect);
        origWndWidth = rect.right - rect.left;
        origWndHeight = rect.bottom - rect.top;
        minSizeSet = false;
        return TRUE;
    }
    case WM_GETMINMAXINFO: {
        if (!minSizeSet) {
            int borderWidth = GetSystemMetrics(SM_CXBORDER);
            int borderHeight = GetSystemMetrics(SM_CYBORDER);
            int captionHeight = GetSystemMetrics(SM_CYCAPTION);
            minWndWidth = origWndWidth + 2 * borderWidth;
            minWndHeight = origWndHeight + captionHeight + 2 * borderHeight;
            minSizeSet = true;
        }
        MINMAXINFO* pMinMax = (MINMAXINFO*)lParam;
        pMinMax->ptMinTrackSize.x = minWndWidth;
        pMinMax->ptMinTrackSize.y = minWndHeight;
        return TRUE;
    }
    case WM_SIZE: {
        int newWndWidth = LOWORD(lParam);
        int newWndHeight = HIWORD(lParam);

        RECT rect;
        GetWindowRect(hText, &rect);

        POINT topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);

        int newWidth = newWidth = rect.right - rect.left + newWndWidth - origWndWidth;
        int newHeight = rect.bottom - rect.top;
        MoveWindow(hText, topLeft.x, topLeft.y, newWidth, newHeight, TRUE);

        GetWindowRect(hEdit, &rect);
        topLeft = { rect.left, rect.top };
        ScreenToClient(hWnd, &topLeft);
        newWidth = rect.right - rect.left + newWndWidth - origWndWidth;
        newHeight = rect.bottom - rect.top + newWndHeight - origWndHeight;
        MoveWindow(hEdit, topLeft.x, topLeft.y, newWidth, newHeight, TRUE);

        origWndWidth = newWndWidth;
        origWndHeight = newWndHeight;
        break;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::Edit:
            if (CODE == EN_CHANGE)
                OnEditchangeEdit();
            break;
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CTriggerAnnotation::Close(hWnd);
        return TRUE;
    }
    case 114514: // used for update
    {
        ShowWindow(m_hwnd, SW_SHOW);
        SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        Update();
        return TRUE;
    }
    case 114515:
    {
        Update();
        return TRUE;
    }
    }

    // Process this message through default handler
    return FALSE;
}


void CTriggerAnnotation::OnEditchangeEdit()
{
    if (ID.IsEmpty())
        return;
    GetWindowText(hEdit, Buffer, BUFFER_SIZE - 1);
    FString text(Buffer);
    if (text.IsEmpty())
    {
        CINI::CurrentDocument->DeleteKey("TriggerAnnotations", ID);
        return;
    }
    text.Replace("\n\r", "\\n");
    text.Replace("\r\n", "\\n");
    text.Replace("\n", "\\n");
    text.Replace("\r", "\\n");
    text = "START," + text;
    text = text + ",END";
    CINI::CurrentDocument->WriteString("TriggerAnnotations", ID, text);
}
