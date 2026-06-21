#include "CLuaDialog.h"
#include <CFinalSunDlg.h>
#include "../../FA2sp.h"
#include "../../Ext/CFinalSunApp/Body.h"
#include "../../Miscs/DialogStyle.h"
#include "../../Helpers/Translations.h"

CLuaDialog::CLuaDialog(const std::string& title, bool autoLayout, int width, int height)
    : ppmfc::CDialog(CLuaDialog::IDD, CFinalSunDlg::Instance)
    , m_title(title)
    , m_width(width)
    , m_height(height)
    , m_autoLayout(autoLayout)
{
}

void CLuaDialog::ApplyAutoLayout(ControlType type, int& x, int& y, int& w, int& h)
{
    switch (type)
    {
    case ControlType::CheckBox:  w = 200; h = 18; break;
    case ControlType::Edit:      w = 200; h = 18; break;
    case ControlType::Combobox:  w = 200; h = 18; break;
    case ControlType::ListBox:   w = 200; h = 120; break;
    case ControlType::Label:     w = 200; break;
    }

    int step = (type == ControlType::CheckBox || type == ControlType::Label) ? (h + 6) : (h + 22);
    if (m_autoLayoutY + step > m_autoLayoutMaxY)
    {
        m_autoLayoutX += m_autoLayoutColWidth;
        m_autoLayoutY = 10;
    }

    x = m_autoLayoutX;
    y = m_autoLayoutY;
    m_autoLayoutY += step;
}

void CLuaDialog::AddCheckBox(const std::string& key, const std::string& label,
    bool defaultValue,
    int x, int y, int w, int h)
{
    if (m_autoLayout)
        ApplyAutoLayout(ControlType::CheckBox, x, y, w, h);
    m_controls.push_back({ key, ControlType::CheckBox, label, x, y, w, h,
        defaultValue, false, false, "", {} });
}

void CLuaDialog::AddEdit(const std::string& key, const std::string& label,
    const std::string& defaultValue,
    int x, int y, int w, int h)
{
    if (m_autoLayout)
        ApplyAutoLayout(ControlType::Edit, x, y, w, h);
    m_controls.push_back({ key, ControlType::Edit, label, x, y, w, h,
        false, false, false, defaultValue, {} });
}

void CLuaDialog::AddComboBox(const std::string& key, const std::string& label,
    sol::table items,
    const std::string& defaultValue,
    bool readOnly,
    int x, int y, int w, int h)
{
    if (m_autoLayout)
        ApplyAutoLayout(ControlType::Combobox, x, y, w, h);
    std::vector<std::string> vec;
    for (const auto& kv : items)
    {
        if (kv.second.is<std::string>())
        {
            vec.push_back(kv.second.as<std::string>());
        }
    }
    m_controls.push_back({ key, ControlType::Combobox, label, x, y, w, h,
        false, readOnly, false, defaultValue, vec });
}

void CLuaDialog::AddListBox(const std::string& key, const std::string& label,
    sol::table items,
    int x, int y, int w, int h)
{
    if (m_autoLayout)
        ApplyAutoLayout(ControlType::ListBox, x, y, w, h);
    std::vector<std::string> vec;
    for (const auto& kv : items)
    {
        if (kv.second.is<std::string>())
        {
            vec.push_back(kv.second.as<std::string>());
        }
    }
    m_controls.push_back({ key, ControlType::ListBox, label, x, y, w, h,
        false, false, false, "", vec });
}

void CLuaDialog::AddMultiListBox(const std::string& key, const std::string& label,
    sol::table items,
    int x, int y, int w, int h)
{
    if (m_autoLayout)
        ApplyAutoLayout(ControlType::ListBox, x, y, w, h);
    std::vector<std::string> vec;
    for (const auto& kv : items)
    {
        if (kv.second.is<std::string>())
        {
            vec.push_back(kv.second.as<std::string>());
        }
    }
    m_controls.push_back({ key, ControlType::ListBox, label, x, y, w, h,
        false, false, true, "", vec });
}

void CLuaDialog::AddLabel(const std::string& text,
    int x, int y, int w, int h)
{
    if (m_autoLayout)
    {
        w = 200;
        float scale = CFinalSunAppExt::ProgramScaleFactor;
        HFONT hFont = DarkTheme::GetModernDefaultGUIFont();
        HDC hDC = ::GetDC(nullptr);
        if (hDC && hFont)
        {
            HFONT hOldFont = static_cast<HFONT>(::SelectObject(hDC, hFont));
            RECT rc = { 0, 0, static_cast<int>(w * scale) - 4, 0 }; 
            ::DrawTextA(hDC, text.c_str(), -1, &rc, DT_CALCRECT | DT_WORDBREAK | DT_EDITCONTROL);
            h = static_cast<int>((rc.bottom - rc.top) / scale);
            TEXTMETRICA tm;
            ::GetTextMetricsA(hDC, &tm);
            int minH = static_cast<int>((tm.tmHeight + 4) / scale);
            if (h < minH) h = minH;
            ::SelectObject(hDC, hOldFont);
        }
        else
        {
            h = 16;
        }
        if (hDC) ::ReleaseDC(nullptr, hDC);
        ApplyAutoLayout(ControlType::Label, x, y, w, h);
    }
    m_controls.push_back({ "", ControlType::Label, text, x, y, w, h });
}

BOOL CLuaDialog::OnInitDialog()
{
    CDialog::OnInitDialog();

    SetWindowText(m_title.c_str());
	FString buffer;
	if (Translations::GetTranslationItem("OK", buffer))
		GetDlgItem(1)->SetWindowTextA(buffer);
    if (Translations::GetTranslationItem("Cancel", buffer))
        GetDlgItem(2)->SetWindowTextA(buffer);

    const float scale = CFinalSunAppExt::ProgramScaleFactor;

    if (m_autoLayout)
    {
        int maxRight = 0, maxBottom = 0;
        for (const auto& c : m_controls)
        {
            int r = c.X + c.W;
            int b = c.Y + c.H;
            if (c.Type != ControlType::CheckBox && c.Type != ControlType::Label)
                b += 16; 
            if (r > maxRight) maxRight = r;
            if (b > maxBottom) maxBottom = b;
        }
        int clientW = maxRight + 10;
        int clientH = maxBottom + 34;
        RECT rc = { 0, 0, clientW, clientH };
        AdjustWindowRect(&rc, GetStyle(), FALSE);
        m_width = rc.right - rc.left;
        m_height = rc.bottom - rc.top;
    }

    int scaledW = static_cast<int>(m_width * scale);
    int scaledH = static_cast<int>(m_height * scale);
    SetWindowPos(nullptr, 0, 0, scaledW, scaledH, SWP_NOMOVE | SWP_NOZORDER);

    HFONT hFont = DarkTheme::GetModernDefaultGUIFont();

    RECT clientRect;
    GetClientRect(&clientRect);
    int clientW = clientRect.right - clientRect.left;
    int clientH = clientRect.bottom - clientRect.top;

    const int btnW = static_cast<int>(60 * scale);
    const int btnH = static_cast<int>(22 * scale);
    const int margin = static_cast<int>(12 * scale);
    const int btnGap = static_cast<int>(8 * scale);
    int btnY = clientH - btnH - margin;
    int cancelX = clientW - btnW - margin;
    int okX = cancelX - btnW - btnGap;

    HWND hOK = GetDlgItem(IDOK)->GetSafeHwnd();
    HWND hCancel = GetDlgItem(IDCANCEL)->GetSafeHwnd();
    if (hOK)
    {
        ::SetWindowPos(hOK, nullptr, okX, btnY, btnW, btnH, SWP_NOZORDER);
        if (hFont) ::SendMessage(hOK, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    }
    if (hCancel)
    {
        ::SetWindowPos(hCancel, nullptr, cancelX, btnY, btnW, btnH, SWP_NOZORDER);
        if (hFont) ::SendMessage(hCancel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
    }

    int labelH = static_cast<int>(16 * scale);
    int labelOffset = static_cast<int>(16 * scale);

    for (const auto& ctrl : m_controls)
    {
        int id = m_nextId++;

        int x = static_cast<int>(ctrl.X * scale);
        int y = static_cast<int>(ctrl.Y * scale);
        int w = static_cast<int>(ctrl.W * scale);
        int h = static_cast<int>(ctrl.H * scale);

        int ctrlY = y;
        if (ctrl.Type != ControlType::CheckBox && ctrl.Type != ControlType::Label)
        {
            ctrlY = y + labelOffset;
            if (!ctrl.Label.empty())
            {
                HWND hLabel = CreateWindowA("STATIC", ctrl.Label.c_str(),
                    WS_CHILD | WS_VISIBLE,
                    x, y, w, labelH,
                    GetSafeHwnd(), nullptr,
                    reinterpret_cast<HINSTANCE>(FA2sp::hInstance), nullptr);
                if (hLabel && hFont)
                    ::SendMessage(hLabel, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
            }
        }

        HWND hCtrl = nullptr;

        switch (ctrl.Type)
        {
        case ControlType::CheckBox:
            hCtrl = CreateWindowA("BUTTON", ctrl.Label.c_str(),
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                x, ctrlY, w, h,
                GetSafeHwnd(), reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                reinterpret_cast<HINSTANCE>(FA2sp::hInstance), nullptr);
            if (hCtrl)
            {
                ::SendMessage(hCtrl, BM_SETCHECK,
                    ctrl.DefaultChecked ? BST_CHECKED : BST_UNCHECKED, 0);
                if (hFont) ::SendMessage(hCtrl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
            }
            break;

        case ControlType::Edit:
            hCtrl = CreateWindowA("EDIT", ctrl.DefaultText.c_str(),
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                x, ctrlY, w, h,
                GetSafeHwnd(), reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                reinterpret_cast<HINSTANCE>(FA2sp::hInstance), nullptr);
            if (hCtrl && hFont)
                ::SendMessage(hCtrl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
            break;

        case ControlType::Combobox:
        {
            DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | CBS_AUTOHSCROLL;
            style |= ctrl.ReadOnly ? CBS_DROPDOWNLIST : CBS_DROPDOWN;
            int comboH = h + static_cast<int>(100 * scale);
            hCtrl = CreateWindowA("COMBOBOX", nullptr,
                style,
                x, ctrlY, w, comboH,
                GetSafeHwnd(), reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                reinterpret_cast<HINSTANCE>(FA2sp::hInstance), nullptr);
            if (hCtrl)
            {
                if (hFont) ::SendMessage(hCtrl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
                for (const auto& item : ctrl.Items)
                {
                    ::SendMessage(hCtrl, CB_ADDSTRING, 0,
                        reinterpret_cast<LPARAM>(item.c_str()));
                }
                int selIdx = static_cast<int>(::SendMessage(hCtrl, CB_FINDSTRINGEXACT,
                    -1, reinterpret_cast<LPARAM>(ctrl.DefaultText.c_str())));
                if (selIdx != CB_ERR)
                {
                    ::SendMessage(hCtrl, CB_SETCURSEL, selIdx, 0);
                }
                else
                {
                    ::SetWindowText(hCtrl, ctrl.DefaultText.c_str());
                }
            }
            break;
        }

        case ControlType::ListBox:
        {
            DWORD style = LBS_NOTIFY | LBS_USETABSTOPS | LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_HSCROLL;
            if (ctrl.MultiSelect)
                style |= LBS_EXTENDEDSEL;
            hCtrl = CreateWindowA("LISTBOX", nullptr,
                style,
                x, ctrlY, w, h,
                GetSafeHwnd(), reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                reinterpret_cast<HINSTANCE>(FA2sp::hInstance), nullptr);
            if (hCtrl)
            {
                if (hFont) ::SendMessage(hCtrl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
                for (const auto& item : ctrl.Items)
                {
                    ::SendMessage(hCtrl, LB_ADDSTRING, 0,
                        reinterpret_cast<LPARAM>(item.c_str()));
                }
            }
            break;
        }

        case ControlType::Label:
        {
            hCtrl = CreateWindowA("EDIT", ctrl.Label.c_str(),
                WS_CHILD | WS_VISIBLE | ES_READONLY | ES_MULTILINE,
                x, y, w, h,
                GetSafeHwnd(), nullptr,
                reinterpret_cast<HINSTANCE>(FA2sp::hInstance), nullptr);
            if (hCtrl)
            {
                if (hFont) ::SendMessage(hCtrl, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
                ::SendMessage(hCtrl, EM_SETBKGNDCOLOR, 0,
                    ::GetSysColor(COLOR_BTNFACE));
                ::SetPropW(hCtrl, L"LuaDialog_LabelEdit", (HANDLE)1);
            }
            break;
        }
        }

        if (hCtrl)
        {
            m_idToKey[id] = ctrl.Key;
            m_keyToId[ctrl.Key] = id;
        }
    }

    return TRUE;
}

void CLuaDialog::OnOK()
{
    for (const auto& [id, key] : m_idToKey)
    {
        ControlDef* pCtrl = nullptr;
        for (auto& c : m_controls)
        {
            if (c.Key == key)
            {
                pCtrl = &c;
                break;
            }
        }
        if (!pCtrl) continue;

		auto dlg = GetDlgItem(id);
        if (!dlg) continue;
		HWND hCtrl = dlg->GetSafeHwnd();
		if (!hCtrl) continue;

        switch (pCtrl->Type)
        {
        case ControlType::CheckBox:
        {
            bool checked = (::SendMessage(hCtrl, BM_GETCHECK, 0, 0) == BST_CHECKED);
            m_boolResults[key] = checked;
            break;
        }
        case ControlType::Edit:
        {
            char buffer[4096] = { 0 };
            ::GetWindowText(hCtrl, buffer, 4095);
            m_stringResults[key] = buffer;
            break;
        }
        case ControlType::Combobox:
        {
            int sel = static_cast<int>(::SendMessage(hCtrl, CB_GETCURSEL, 0, 0));
            if (sel != CB_ERR)
            {
                char buffer[4096] = { 0 };
                ::SendMessage(hCtrl, CB_GETLBTEXT, sel,
                    reinterpret_cast<LPARAM>(buffer));
                m_stringResults[key] = buffer;
            }
            else
            {
                char buffer[4096] = { 0 };
                ::GetWindowText(hCtrl, buffer, 4095);
                m_stringResults[key] = buffer;
            }
            break;
        }
        case ControlType::ListBox:
        {
            if (pCtrl->MultiSelect)
            {
                std::vector<std::string> selected;
                int count = static_cast<int>(::SendMessage(hCtrl, LB_GETCOUNT, 0, 0));
                for (int i = 0; i < count; ++i)
                {
                    if (::SendMessage(hCtrl, LB_GETSEL, i, 0) > 0)
                    {
                        char buffer[4096] = { 0 };
                        ::SendMessage(hCtrl, LB_GETTEXT, i,
                            reinterpret_cast<LPARAM>(buffer));
                        selected.push_back(buffer);
                    }
                }
                m_listResults[key] = selected;
            }
            else
            {
                int sel = static_cast<int>(::SendMessage(hCtrl, LB_GETCURSEL, 0, 0));
                char buffer[4096] = { 0 };
                if (sel != LB_ERR)
                {
                    ::SendMessage(hCtrl, LB_GETTEXT, sel,
                        reinterpret_cast<LPARAM>(buffer));
                }
                m_stringResults[key] = buffer;
            }
            break;
        }
        }
    }

    m_accepted = true;
    CDialog::OnOK();
}

void CLuaDialog::OnCancel()
{
    m_accepted = false;
    CDialog::OnCancel();
}

void CLuaDialog::DoDataExchange(ppmfc::CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
}

sol::object CLuaDialog::DoModal(sol::this_state s)
{
    INT_PTR ret = CDialog::DoModal();

    if (!m_accepted)
    {
        return sol::nil;
    }

    sol::state_view lua(s);
    sol::table result = lua.create_table();

    for (const auto& ctrl : m_controls)
    {
        if (ctrl.Type == ControlType::CheckBox)
        {
            auto it = m_boolResults.find(ctrl.Key);
            if (it != m_boolResults.end())
            {
                result[ctrl.Key] = it->second;
            }
        }
        else if (ctrl.Type == ControlType::ListBox && ctrl.MultiSelect)
        {
            auto it = m_listResults.find(ctrl.Key);
            if (it != m_listResults.end())
            {
                sol::table arr = lua.create_table();
                int idx = 1;
                for (const auto& str : it->second)
                {
                    arr[idx++] = str;
                }
                result[ctrl.Key] = arr;
            }
        }
        else
        {
            auto it = m_stringResults.find(ctrl.Key);
            if (it != m_stringResults.end())
            {
                result[ctrl.Key] = it->second;
            }
        }
    }

    return result;
}