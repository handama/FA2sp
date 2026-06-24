#pragma once
#include <FA2PP.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../../Sol/sol.hpp"
#include "../Common.h"

class CLuaDialog
{
public:
    enum class ControlType
    {
        CheckBox = 0,
        Edit,
        Combobox,
        ListBox,
        Label,
    };

    struct ControlDef
    {
        std::string Key;
        ControlType Type;
        std::string Label;
        int X, Y, W, H;
        bool DefaultChecked = false;
        bool ReadOnly = false;
        bool MultiSelect = false;
        std::string DefaultText;
        std::vector<std::string> Items;
    };

    CLuaDialog(const std::string& title, bool autoLayout = false, int width = 420, int height = 320);
    virtual ~CLuaDialog();

    void AddCheckBox(const std::string& key, const std::string& label,
        bool defaultValue,
        int x = 0, int y = 0, int w = 0, int h = 0);

    void AddEdit(const std::string& key, const std::string& label,
        const std::string& defaultValue,
        int x = 0, int y = 0, int w = 0, int h = 0);

    void AddComboBox(const std::string& key, const std::string& label,
        sol::table items,
        const std::string& defaultValue,
        bool readOnly = false,
        int x = 0, int y = 0, int w = 0, int h = 0);

    void AddListBox(const std::string& key, const std::string& label,
        sol::table items,
        int x = 0, int y = 0, int w = 0, int h = 0);

    void AddMultiListBox(const std::string& key, const std::string& label,
        sol::table items,
        int x = 0, int y = 0, int w = 0, int h = 0);

    void AddLabel(const std::string& key, const std::string& text,
        int x = 0, int y = 0, int w = 0, int h = 0);

    void OnEvent(const std::string& key, const std::string& eventType, sol::function callback);
    bool GetBool(const std::string& key);
    std::string GetString(const std::string& key);
    void SetEnabled(const std::string& key, bool enabled);
    void SetVisible(const std::string& key, bool visible);
    void SetListItems(const std::string& key, sol::table items);
    void SetComboItems(const std::string& key, sol::table items);
    void SetText(const std::string& key, const std::string& text);
    void SetCheck(const std::string& key, bool checked);
    bool GetEnabled(const std::string& key);
    bool GetVisible(const std::string& key);
    void SetPosition(const std::string& key, int x, int y, int w, int h);
    void SetWindowSize(int width, int height);

    sol::object DoModal(sol::this_state s);

    enum { IDD = 345 };

protected:
    void ApplyAutoLayout(ControlType type, int& x, int& y, int& w, int& h);

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hWnd);
    void OnOK(HWND hWnd);
    void OnCancel(HWND hWnd);
    void CollectResults(HWND hWnd);
    void RestoreDisabledWindows();

    void RepositionButtons(HWND hWnd);

    HWND GetCtrlHwnd(const std::string& key);
    void FireEvent(const std::string& ctrlKey, const std::string& eventType);
    void FireAllInitialEvents();

    std::string m_title;
    int m_width;
    int m_height;
    bool m_autoLayout;
    int m_autoLayoutX = 10;
    int m_autoLayoutY = 10;
    int m_autoLayoutColWidth = 215;
    int m_autoLayoutMaxY = 500;
    std::vector<ControlDef> m_controls;
    std::map<int, std::string> m_idToKey;
    std::map<std::string, int> m_keyToId;
    std::map<std::string, std::string> m_stringResults;
    std::map<std::string, bool> m_boolResults;
    std::map<std::string, std::vector<std::string>> m_listResults;
    int m_nextId = 10000;
    bool m_accepted = false;

    std::map<HWND, std::unique_ptr<VirtualComboBoxEx>> m_comboBoxes;
    std::vector<HWND> m_disabledWindows;
    std::map<std::string, sol::function> m_events;
    HWND m_hWnd = nullptr;
};