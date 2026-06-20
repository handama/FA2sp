#pragma once
#include <FA2PP.h>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include "../../Sol/sol.hpp"

class CLuaDialog : public ppmfc::CDialog
{
public:
    enum class ControlType
    {
        CheckBox = 0,
        Edit,
        Combobox,
        ListBox,
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
    virtual ~CLuaDialog() = default;

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

    sol::object DoModal(sol::this_state s);

    enum { IDD = 345 };

protected:
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override;
    virtual void OnCancel() override;
    virtual void DoDataExchange(ppmfc::CDataExchange* pDX) override;

    void ApplyAutoLayout(ControlType type, int& x, int& y, int& w, int& h);

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
};