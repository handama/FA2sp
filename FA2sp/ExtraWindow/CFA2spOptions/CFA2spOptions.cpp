#include "CFA2spOptions.h"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include <CFinalSunDlg.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CMapData/Body.h"
#include "../../Miscs/SaveMap.h"
#include "../Common.h"
#include "../../Miscs/DialogStyle.h"
#include "../../Ext/CFinalSunApp/Body.h"
#include <shlwapi.h>
#include <ShlObj.h>
#pragma comment(lib, "shlwapi.lib")

HWND CFA2spOptions::m_hwnd;
HWND CFA2spOptions::hList;
HWND CFA2spOptions::hGameEngine;
HWND CFA2spOptions::hSearch;
CFinalSunDlg* CFA2spOptions::m_parent;
bool CFA2spOptions::initialized = false;
WNDPROC CFA2spOptions::g_pOriginalListViewProc = nullptr;

LRESULT CALLBACK CFA2spOptions::ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DarkTheme::MyCallWindowProcA(g_pOriginalListViewProc, hWnd, uMsg, wParam, lParam);
}

static std::string GetFileFormat(bool* value)
{
    for (auto& f : ExtConfigs::SupportedFormats)
    {
        if (value == &f.second)
            return "." + f.first; 
    }
    return {};
}

FString GetExeFullPath()
{
    FString path = CFinalSunApp::ExePath();
    path += "\\";
    path += CFinalSunAppExt::LauncherName.IsEmpty() ? FString("\\FA2spLaunch.exe") : CFinalSunAppExt::LauncherName;
    path.Replace("\\\\", "\\");
    return path;
}

std::string MakeProgID(const std::string& ext)
{
    std::string cleanExt = ext;
    if (!cleanExt.empty() && cleanExt[0] == '.')
        cleanExt = cleanExt.substr(1);

    return std::string("FinalAlert2.HDMEdition.") + cleanExt + ".1";
}

bool IsFileAssociationOwnedByThisApp(const std::string& extension)
{
    if (extension.empty()) return false;

    std::string progId = MakeProgID(extension);

    HKEY hKey = nullptr;
    std::string keyPath = "Software\\Classes\\" + extension;
    LONG ret = RegOpenKeyExA(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_READ, &hKey);
    if (ret != ERROR_SUCCESS) {
        return false;
    }

    char buf[512] = {};
    DWORD bufSize = sizeof(buf);
    ret = RegQueryValueExA(hKey, nullptr, nullptr, nullptr, (LPBYTE)buf, &bufSize);
    RegCloseKey(hKey);

    if (ret != ERROR_SUCCESS) {
        return false;
    }

    std::string currentProgId = buf;
    return _stricmp(currentProgId.c_str(), progId.c_str()) == 0;
}

std::string ToLower(const std::string& str)
{
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower;
}

bool IsCommandLinePointingToExe(const std::string& commandLine, const std::string& targetExePath)
{
    if (commandLine.empty() || targetExePath.empty()) {
        return false;
    }

    std::string cmdLower = ToLower(commandLine);
    std::string targetLower = ToLower(targetExePath);

    size_t start = cmdLower.find_first_of("\"");
    if (start == std::string::npos) {
        start = cmdLower.find_first_not_of(" \t");
    }

    if (start == std::string::npos) {
        return false;
    }

    std::string firstToken;

    if (cmdLower[start] == '"') {
        size_t end = cmdLower.find('"', start + 1);
        if (end != std::string::npos) {
            firstToken = cmdLower.substr(start + 1, end - start - 1);
        }
    }
    else {
        size_t end = cmdLower.find(' ', start);
        if (end == std::string::npos) {
            firstToken = cmdLower.substr(start);
        }
        else {
            firstToken = cmdLower.substr(start, end - start);
        }
    }

    if (firstToken.empty()) {
        return false;
    }

    return PathMatchSpecA(firstToken.c_str(), targetLower.c_str()) ||
        (firstToken == targetLower);
}

bool IsOpenCommandPointingToMe(const std::string& extension)
{
    if (extension.empty()) {
        return false;
    }

    std::string ext = extension;
    if (ext[0] != '.') {
        ext = "." + ext;
    }

    std::string currentExe = GetExeFullPath();

    std::string progId = MakeProgID(ext);

    std::string keyPath = "Software\\Classes\\" + progId + "\\shell\\open\\command";

    HKEY hKey = nullptr;
    LONG ret = RegOpenKeyExA(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_READ, &hKey);
    if (ret != ERROR_SUCCESS) {
        ret = RegOpenKeyExA(HKEY_CLASSES_ROOT, keyPath.c_str(), 0, KEY_READ, &hKey);
        if (ret != ERROR_SUCCESS) {
            return false;
        }
    }

    char buffer[1024] = {};
    DWORD bufferSize = sizeof(buffer);
    ret = RegQueryValueExA(hKey, nullptr, nullptr, nullptr, (LPBYTE)buffer, &bufferSize);
    RegCloseKey(hKey);

    if (ret != ERROR_SUCCESS) {
        return false;
    }

    std::string commandLine = buffer;

    return IsCommandLinePointingToExe(commandLine, currentExe);
}

std::string GetCurrentFileAssociationProgID(const std::string& extension)
{
    if (extension.empty()) return "";

    HKEY hKey = nullptr;
    std::string keyPath = "Software\\Classes\\" + extension;
    LONG ret = RegOpenKeyExA(HKEY_CURRENT_USER, keyPath.c_str(), 0, KEY_READ, &hKey);
    if (ret != ERROR_SUCCESS) {
        return "";
    }

    char buf[512] = {};
    DWORD bufSize = sizeof(buf);
    ret = RegQueryValueExA(hKey, nullptr, nullptr, nullptr, (LPBYTE)buf, &bufSize);
    RegCloseKey(hKey);

    if (ret == ERROR_SUCCESS) {
        return buf;
    }
    return "";
}

bool AssociateFileExtension(
    const std::string& extension,
    const std::string& description,
    int iconIndex = 0)
{
    if (extension.empty()) return false;

    std::string ext = extension;
    if (ext[0] != '.') ext = "." + ext;

    std::string progId = MakeProgID(ext);
    std::string exePath = GetExeFullPath();

    HKEY hKey = nullptr;
    LONG ret;

    std::string key1 = "Software\\Classes\\" + ext;
    ret = RegCreateKeyExA(HKEY_CURRENT_USER, key1.c_str(), 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &hKey, nullptr);
    if (ret != ERROR_SUCCESS) return false;
    RegSetValueExA(hKey, nullptr, 0, REG_SZ, (BYTE*)progId.c_str(),
        (DWORD)(progId.length() + 1));
    RegCloseKey(hKey);

    std::string key2 = "Software\\Classes\\" + progId;
    ret = RegCreateKeyExA(HKEY_CURRENT_USER, key2.c_str(), 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &hKey, nullptr);
    if (ret != ERROR_SUCCESS) return false;
    RegSetValueExA(hKey, nullptr, 0, REG_SZ, (BYTE*)description.c_str(),
        (DWORD)(description.length() + 1));
    RegCloseKey(hKey);

    std::string keyIcon = key2 + "\\DefaultIcon";
    ret = RegCreateKeyExA(HKEY_CURRENT_USER, keyIcon.c_str(), 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &hKey, nullptr);
    if (ret == ERROR_SUCCESS) {
        std::string iconStr = exePath + "," + std::to_string(iconIndex);
        RegSetValueExA(hKey, nullptr, 0, REG_SZ, (BYTE*)iconStr.c_str(),
            (DWORD)(iconStr.length() + 1));
        RegCloseKey(hKey);
    }

    std::string keyOpen = key2 + "\\shell\\open\\command";
    ret = RegCreateKeyExA(HKEY_CURRENT_USER, keyOpen.c_str(), 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &hKey, nullptr);
    if (ret != ERROR_SUCCESS) return false;

    std::string cmd = "\"" + exePath + "\" \"%1\"";
    RegSetValueExA(hKey, nullptr, 0, REG_SZ, (BYTE*)cmd.c_str(),
        (DWORD)(cmd.length() + 1));
    RegCloseKey(hKey);

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

    return true;
}

bool UnassociateFileExtension(const std::string& extension)
{
    if (extension.empty()) return false;

    std::string ext = extension;
    if (ext[0] != '.') ext = "." + ext;

    if (!IsFileAssociationOwnedByThisApp(ext)) {
        return false;
    }

    std::string progId = MakeProgID(ext);

    std::string key1 = "Software\\Classes\\" + ext;
    SHDeleteKeyA(HKEY_CURRENT_USER, key1.c_str());

    std::string key2 = "Software\\Classes\\" + progId;
    SHDeleteKeyA(HKEY_CURRENT_USER, key2.c_str());

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

    return true;
}

void CFA2spOptions::Create(CFinalSunDlg* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(325),
        pWnd->MyViewFrame.GetSafeHwnd(),
        CFA2spOptions::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CFA2spOptions.\n");
        m_parent = NULL;
    }
}

void CFA2spOptions::Initialize(HWND& hWnd)
{
    FString buffer;
    if (Translations::GetTranslationItem("Options.Title", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](const char* pLabelName, int nIDDlgItem)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };

    Translate("Options.Search", 1003);

    hList = GetDlgItem(hWnd, Controls::List);
    hSearch = GetDlgItem(hWnd, Controls::Search);

    if (ExtConfigs::EnableDarkMode)
    {
        ::SendMessage(hList, LVM_SETTEXTBKCOLOR, 0, RGB(32, 32, 32));
        ::SendMessage(hList, LVM_SETTEXTCOLOR, 0, RGB(220, 220, 220));

        DarkTheme::SubclassListViewHeader(hList);
        g_pOriginalListViewProc = (WNDPROC)GetWindowLongPtr(hList, GWLP_WNDPROC);
        if (g_pOriginalListViewProc)
        {
            SetWindowLongPtr(hList, GWLP_WNDPROC, (LONG_PTR)ListViewSubclassProc);
        }
        InvalidateRect(hList, NULL, TRUE);
    }

    Update();
}

void CFA2spOptions::Update(const char* filter)
{
    initialized = false;
    ShowWindow(m_hwnd, SW_SHOW);
    SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    CINI fa2;
    FString path = CFinalSunAppExt::ExePathExt;
    path += "\\FinalAlert.ini";
    fa2.ClearAndLoad(path);

    for (const auto& opt : ExtConfigs::Options)
    {
        *opt.Value = fa2.GetBool("Options", opt.IniKey, *opt.Value);
    }

    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
    ppmfc::CString title = Translations::TranslateOrDefault("Options.Label", "Options");
    ListView_DeleteAllItems(hList);
    int nColumnCount = Header_GetItemCount(ListView_GetHeader(hList));
    for (int i = nColumnCount - 1; i >= 0; --i) {
        ListView_DeleteColumn(hList, i);
    }

    LVCOLUMN lvc = { 0 };
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    lvc.fmt = LVCFMT_LEFT;
    RECT rcClient = {};
    GetClientRect(hList, &rcClient);
    int fullWidth = rcClient.right - rcClient.left;
    lvc.cx = fullWidth;
    lvc.pszText = title.m_pchData;
    ListView_InsertColumn(hList, 0, &lvc);

    int index = 0;
    for (const auto& opt : ExtConfigs::Options)
    {
        if (filter && strlen(filter))
        {
            if (!ExtraWindow::IsLabelMatch(opt.DisplayName, filter) && !ExtraWindow::IsLabelMatch(opt.IniKey, filter))
                continue;
        }
        LVITEM lvi = { 0 };
        lvi.mask = LVIF_TEXT;
        lvi.iItem = index;
        lvi.pszText = (char*)opt.DisplayName.c_str();
        ListView_InsertItem(hList, &lvi);
        if (opt.Type == ExtConfigs::SpecialOptionType::BindFormat)
        {
            auto ext = GetFileFormat(opt.Value);
            ListView_SetCheckState(hList, index, 
                (IsFileAssociationOwnedByThisApp(ext) && IsOpenCommandPointingToMe(ext))
                ? TRUE : FALSE);
        }
        else
        {
            ListView_SetCheckState(hList, index, *opt.Value ? TRUE : FALSE);
        }
        index++;
    }
    initialized = true;
    return;
}

void CFA2spOptions::Close(HWND& hWnd)
{
    EndDialog(hWnd, NULL);

    CFA2spOptions::m_hwnd = NULL;
    CFA2spOptions::m_parent = NULL;

}

BOOL CALLBACK CFA2spOptions::DlgProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CFA2spOptions::Initialize(hwnd);
        return TRUE;
    }
    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->idFrom == Controls::List && pnmh->code == LVN_ITEMCHANGED && initialized)
        {
            NMLISTVIEW* pnmv = (NMLISTVIEW*)lParam;
            if ((pnmv->uChanged & LVIF_STATE) &&
                ((pnmv->uNewState ^ pnmv->uOldState) & LVIS_STATEIMAGEMASK))
            {
                int index = pnmv->iItem;
                if (index >= 0)
                {
                    TCHAR buf[256] = { 0 };
                    ListView_GetItemText(hList, index, 0, buf, 255);
                    BOOL checked = ListView_GetCheckState(hList, index);

                    CINI fa2;
                    std::string path;
                    path = CFinalSunAppExt::ExePathExt;
                    path += "\\FinalAlert.ini";
                    fa2.ClearAndLoad(path.c_str());

                    for (const auto& opt : ExtConfigs::Options)
                    {
                        if (opt.DisplayName == buf)
                        {
                            *opt.Value = (checked != FALSE);
                            if (!opt.IniKey.IsEmpty())
                                fa2.WriteBool("Options", opt.IniKey, *opt.Value);

                            if (opt.Type == ExtConfigs::SpecialOptionType::ReloadMap && CMapData::Instance->MapWidthPlusHeight)
                            {
                                MessageBox(m_parent->m_hWnd,
                                    Translations::TranslateOrDefault("Options.ReloadMap", "Reload map to apply this change."),
                                    "FA2sp", MB_OK | MB_ICONWARNING);
                            }
                            else if (opt.Type == ExtConfigs::SpecialOptionType::Restart)
                            {
                                MessageBox(m_parent->m_hWnd,
                                    Translations::TranslateOrDefault("Options.Restart", "Restart FA2 to apply this change."),
                                    "FA2sp", MB_OK | MB_ICONWARNING);
                            }
                            else if (opt.Type == ExtConfigs::SpecialOptionType::SaveMap_Timer)
                            {
                                if (!*opt.Value)
                                {
                                    ExtConfigs::SaveMap_AutoSave_Interval = -1;
                                }
                                else
                                {
                                    ExtConfigs::SaveMap_AutoSave_Interval = ExtConfigs::SaveMap_AutoSave_Interval_Real;
                                }
                                SaveMapExt::ResetTimer();
                            }
                            else if (opt.Type == ExtConfigs::SpecialOptionType::BindFormat)
                            {
                                auto ext = GetFileFormat(opt.Value);
                                if (*opt.Value)
                                {
                                    std::string ext2 = ext.substr(1);
                                    bool success = AssociateFileExtension(
                                        ext,
                                        "FinalAlert2 " + ext2 + " File",
                                        0 
                                    );
                                }
                                else
                                {
                                    UnassociateFileExtension(ext);
                                }
                            }
                        }
                    }
                    fa2.WriteToFile(path.c_str());
                }
            }
        }
        break;
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
        default:
            break;
        }
        break;
    }
    case WM_CLOSE:
    {
        CFA2spOptions::Close(hwnd);
        return TRUE;
    }
    case 114514: // used for update
    {
        OnEditchangeSearch();
        return TRUE;
    }
    }

    // Process this message through default handler
    return FALSE;
}

void CFA2spOptions::OnEditchangeSearch()
{
    char buffer[512]{ 0 };
    GetWindowText(hSearch, buffer, 511);
    Update(buffer);
}