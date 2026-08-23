#include "CNewTrigger.h"
#include "../CLuaConsole/CLuaConsole.h"
#include "../Common.h"
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/Translations.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace LuaFunctions
{
    bool TriggerLua_IsUsed(const std::string& id);
    void TriggerLua_RegisterUsedIndex(const std::string& id);
    void TriggerLua_UnregisterUsedIndex(const std::string& id);
    void TriggerLua_Report(const std::string& msg);
}

CNewTrigger CNewTrigger::Headless;

static void TeReport(const std::string& msg)
{
    LuaFunctions::TriggerLua_Report(msg);
}


bool CNewTrigger::TeIsOpen() const
{
    return m_hwnd != nullptr;
}

bool CNewTrigger::TeEnsureOpen()
{
    if (TeIsOpen())
        return true;

    CFinalSunDlg* pWnd = CFinalSunDlg::Instance;
    if (!pWnd)
        return false;

    HeadlessMode = true;
    CompactMode = false;
    Create(pWnd);
    return TeIsOpen();
}

void CNewTrigger::TeClose()
{
    if (!m_hwnd)
        return;

    HeadlessMode = true;
    Close(m_hwnd);
    m_hwnd = nullptr;
    HeadlessMode = false;
    CurrentTrigger = nullptr;
    CurrentTriggerID = "";
    SelectedTriggerIndex = -1;
    SelectedEventIndex = -1;
    SelectedActionIndex = -1;
}


std::string CNewTrigger::TeNewTrigger(const char* name, const char* house)
{
    if (!TeEnsureOpen())
        return "";

    OnClickNewTrigger();

    std::string id = CurrentTriggerID.c_str();
    if (!id.empty())
        LuaFunctions::TriggerLua_RegisterUsedIndex(id);

    if (name && *name && CurrentTrigger)
        TeSetTriggerProp("name", name);
    if (house && *house && CurrentTrigger)
        TeSetTriggerProp("house", house);

    CLuaConsole::updateTrigger = true;
    return CurrentTriggerID.c_str();
}

bool CNewTrigger::TeSelectTrigger(const char* id)
{
    if (!TeEnsureOpen())
        return false;
    if (!id || !*id)
        return false;

    std::string want = id;
    bool found = false;
    int count = vcbSelectedTrigger.GetCount();
    for (int i = 0; i < count; ++i)
    {
        FString item = vcbSelectedTrigger.GetItemText(i);
        FString::TrimIndex(item);
        int sp = item.Find(" ");
        FString num = (sp > 0) ? item.Mid(0, sp) : item;
        if (num == want.c_str())
        {
            vcbSelectedTrigger.SetCurSel(i);
            found = true;
            break;
        }
    }

    if (!found)
        return false;

    OnSelchangeTrigger();
    return CurrentTrigger != nullptr;
}

std::string CNewTrigger::TeGetSelectedTrigger() const
{
    return CurrentTriggerID.c_str();
}

CNewTrigger::TeTriggerInfo CNewTrigger::TeGetTriggerInfo()
{
    TeTriggerInfo ret;
    if (!CurrentTrigger)
        return ret;

    ret.ok = true;
    ret.id = CurrentTrigger->ID.c_str();
    ret.name = CurrentTrigger->Name.c_str();
    ret.house = CurrentTrigger->House.c_str();
    ret.attached_trigger = CurrentTrigger->AttachedTrigger.c_str();
    ret.disabled = CurrentTrigger->Disabled;
    ret.easy = CurrentTrigger->EasyEnabled;
    ret.medium = CurrentTrigger->MediumEnabled;
    ret.hard = CurrentTrigger->HardEnabled;
    ret.repeat_type = CurrentTrigger->RepeatType.c_str();

    TeTriggerInfo fallback;
    (void)fallback;
    int evCount = CurrentTrigger->EventCount;
    int evPrev = SelectedEventIndex;
    for (int i = 0; i < evCount; ++i)
        ret.events.push_back(BuildEventInfo(i));
    SelectedEventIndex = evPrev;

    int acCount = CurrentTrigger->ActionCount;
    int acPrev = SelectedActionIndex;
    for (int i = 0; i < acCount; ++i)
        ret.actions.push_back(BuildActionInfo(i));
    SelectedActionIndex = acPrev;

    return ret;
}

bool CNewTrigger::TeSetTriggerProp(const std::string& key, const std::string& value)
{
    if (!TeEnsureOpen())
        return false;
    if (!CurrentTrigger)
        return false;

    auto truthy = [](const std::string& v) -> bool {
        return v == "1" || v == "true" || v == "TRUE" || v == "yes";
    };

    if (key == "name")
        CurrentTrigger->Name = value.c_str();
    else if (key == "house")
        CurrentTrigger->House = value.c_str();
    else if (key == "attached_trigger")
        CurrentTrigger->AttachedTrigger = value.c_str();
    else if (key == "disabled")
        CurrentTrigger->Disabled = truthy(value);
    else if (key == "easy")
        CurrentTrigger->EasyEnabled = truthy(value);
    else if (key == "medium")
        CurrentTrigger->MediumEnabled = truthy(value);
    else if (key == "hard")
        CurrentTrigger->HardEnabled = truthy(value);
    else if (key == "repeat_type")
        CurrentTrigger->RepeatType = value.c_str();
    else
    {
        TeReport("te_set_trigger_prop: unknown property " + key);
        return false;
    }

    CurrentTrigger->Save();
    CLuaConsole::updateTrigger = true;
    return true;
}

bool CNewTrigger::TeDeleteTrigger(bool keepTags)
{
    if (!TeEnsureOpen())
        return false;
    if (!CurrentTrigger)
        return false;

    std::string id = CurrentTrigger->ID.c_str();
    bool hadTag = (CurrentTrigger->Tag != "<none>" && !CurrentTrigger->Tag.empty());

    HeadlessDeleteTags = !keepTags;
    OnClickDelTrigger(m_hwnd);
    HeadlessDeleteTags = true;

    if (!id.empty())
        LuaFunctions::TriggerLua_UnregisterUsedIndex(id);

    CLuaConsole::updateTrigger = true;
    return true;
}


std::vector<CNewTrigger::TeTypeInfo> CNewTrigger::TeGetEventTypes(const std::string& filter, int max)
{
    std::vector<TeTypeInfo> ret;
    if (!TeEnsureOpen())
        return ret;

    LabelMatcher matcher(filter.c_str());

    int count = vcbEventType.GetCount();
    for (int i = 0; i < count; ++i)
    {
        const char* pText = vcbEventType.GetItemText(i);
        std::string item = pText ? pText : "";

        int sp = (int)item.find(' ');
        if (sp <= 0)
            continue;
        std::string num = item.substr(0, sp);
        std::string name = (int)item.find_first_not_of(' ', sp) == -1
            ? "" : item.substr(item.find_first_not_of(' ', sp));

        FString descRaw = STDHelpers::ReplaceSpeicalString(
            fadata.GetString(ExtraWindow::GetTranslatedSectionName("EventsRA2"), num.c_str(), "MISSING,0,0,0,0,MISSING,0,1,0"));
        auto atoms = FString::SplitString(descRaw, 8);
        std::string desc = atoms.size() > 5 ? atoms[5].c_str() : "";

        bool match = filter.empty() || matcher.Match(name.c_str()) || matcher.Match(desc.c_str());
        if (!match)
            continue;

        if (max > 0 && (int)ret.size() >= max)
            break;

        TeTypeInfo t;
        t.num = num;
        t.name = name;
        t.desc = desc;
        ret.push_back(std::move(t));
    }
    return ret;
}

std::vector<CNewTrigger::TeTypeInfo> CNewTrigger::TeGetActionTypes(const std::string& filter, int max)
{
    std::vector<TeTypeInfo> ret;
    if (!TeEnsureOpen())
        return ret;

    LabelMatcher matcher(filter.c_str());

    int count = vcbActionType.GetCount();
    for (int i = 0; i < count; ++i)
    {
        const char* pText = vcbActionType.GetItemText(i);
        std::string item = pText ? pText : "";

        int sp = (int)item.find(' ');
        if (sp <= 0)
            continue;
        std::string num = item.substr(0, sp);
        std::string name = (int)item.find_first_not_of(' ', sp) == -1
            ? "" : item.substr(item.find_first_not_of(' ', sp));

        FString descRaw = FString::ReplaceSpeicalString(
            fadata.GetString(ExtraWindow::GetTranslatedSectionName("ActionsRA2"), num.c_str(), "MISSING,0,0,0,0,0,0,0,0,0,MISSING,0,1,0"));
        auto atoms = FString::SplitString(descRaw, 13);
        std::string desc = atoms.size() > 10 ? atoms[10].c_str() : "";

        bool match = filter.empty() || matcher.Match(name.c_str()) || matcher.Match(desc.c_str());
        if (!match)
            continue;

        if (max > 0 && (int)ret.size() >= max)
            break;

        TeTypeInfo t;
        t.num = num;
        t.name = name;
        t.desc = desc;
        ret.push_back(std::move(t));
    }
    return ret;
}

CNewTrigger::TeTypeInfo CNewTrigger::TeGetEventTypeInfo(int num)
{
    TeTypeInfo ret;
    if (!TeEnsureOpen())
        return ret;

    std::string numStr = std::to_string(num);

    int count = vcbEventType.GetCount();
    for (int i = 0; i < count; ++i)
    {
        const char* pText = vcbEventType.GetItemText(i);
        std::string item = pText ? pText : "";
        int sp = (int)item.find(' ');
        if (sp <= 0)
            continue;
        if (item.substr(0, sp) == numStr)
        {
            int ns = (int)item.find_first_not_of(' ', sp);
            ret.name = (ns == -1) ? "" : item.substr(ns);
            break;
        }
    }

    FString descRaw = STDHelpers::ReplaceSpeicalString(
        fadata.GetString(ExtraWindow::GetTranslatedSectionName("EventsRA2"), numStr.c_str(), "MISSING,0,0,0,0,MISSING,0,1,0"));
    auto atoms = FString::SplitString(descRaw, 8);
    if (atoms.size() > 0 && atoms[0] != "MISSING")
    {
        ret.num = numStr;
        if (atoms.size() > 5)
            ret.desc = atoms[5].c_str();
    }
    return ret;
}

CNewTrigger::TeTypeInfo CNewTrigger::TeGetActionTypeInfo(int num)
{
    TeTypeInfo ret;
    if (!TeEnsureOpen())
        return ret;

    std::string numStr = std::to_string(num);

    int count = vcbActionType.GetCount();
    for (int i = 0; i < count; ++i)
    {
        const char* pText = vcbActionType.GetItemText(i);
        std::string item = pText ? pText : "";
        int sp = (int)item.find(' ');
        if (sp <= 0)
            continue;
        if (item.substr(0, sp) == numStr)
        {
            int ns = (int)item.find_first_not_of(' ', sp);
            ret.name = (ns == -1) ? "" : item.substr(ns);
            break;
        }
    }

    FString descRaw = FString::ReplaceSpeicalString(
        fadata.GetString(ExtraWindow::GetTranslatedSectionName("ActionsRA2"), numStr.c_str(), "MISSING,0,0,0,0,0,0,0,0,0,MISSING,0,1,0"));
    auto atoms = FString::SplitString(descRaw, 13);
    if (atoms.size() > 0 && atoms[0] != "MISSING")
    {
        ret.num = numStr;
        if (atoms.size() > 10)
            ret.desc = atoms[10].c_str();
    }
    return ret;
}


CNewTrigger::TeEntryInfo CNewTrigger::TeSelectEvent(int idx)
{
    if (!TeEnsureOpen())
        return {};
    if (!CurrentTrigger || idx < 0 || idx >= CurrentTrigger->EventCount)
        return {};
    return BuildEventInfo(idx);
}

CNewTrigger::TeEntryInfo CNewTrigger::TeSelectAction(int idx)
{
    if (!TeEnsureOpen())
        return {};
    if (!CurrentTrigger || idx < 0 || idx >= CurrentTrigger->ActionCount)
        return {};
    return BuildActionInfo(idx);
}

static void SplitOptionText(const FString& item, std::string& value, std::string& text, bool valueFromFirstSpace)
{
    if (valueFromFirstSpace)
    {
        FString trimmed = item;
        trimmed.Trim();
        int idx = trimmed.Find(' ');
        if (idx > 0)
        {
            value = trimmed.Mid(0, idx).c_str();
            FString rest = trimmed.Mid(idx + 1);
            rest.Trim();
            if (rest.size() >= 2 && rest[0] == '(' && rest[rest.size() - 1] == ')')
                rest = rest.Mid(1, rest.size() - 2);
            text = rest.c_str();
        }
        else
        {
            value = item.c_str();
            text = item.c_str();
        }
    }
    else
    {
        int idx = item.Find(" - ");
        if (idx > 0)
        {
            value = item.Mid(0, idx).c_str();
            text = item.Mid(idx + 3).c_str();
        }
        else
        {
            value = item.c_str();
            text = item.c_str();
        }
    }
}

std::vector<CNewTrigger::TeOption> CNewTrigger::TeGetEventOptions(int slot, const std::string& filter, int max)
{
    std::vector<TeOption> ret;
    if (!TeEnsureOpen())
        return ret;
    if (!CurrentTrigger || SelectedEventIndex < 0 || slot < 0 || slot >= EVENT_PARAM_COUNT || !EventParamsUsage[slot].first)
        return ret;

    LabelMatcher matcher(filter.c_str());
    int count = vcbEventParameter[slot].GetCount();
    bool valueFromFirstSpace = EventParamType[slot] == ParamType::Trigger || EventParamType[slot] == ParamType::Team;
    for (int i = 0; i < count; ++i)
    {
        FString item = vcbEventParameter[slot].GetItemText(i);
        std::string value, text;
        SplitOptionText(item, value, text, valueFromFirstSpace);

        bool match = filter.empty() || matcher.Match(text.c_str()) || matcher.Match(value.c_str());
        if (!match)
            continue;

        if (max > 0 && (int)ret.size() >= max)
            break;

        TeOption o;
        o.value = value;
        o.text = text;
        ret.push_back(std::move(o));
    }
    return ret;
}

std::vector<CNewTrigger::TeOption> CNewTrigger::TeGetActionOptions(int slot, const std::string& filter, int max)
{
    std::vector<TeOption> ret;
    if (!TeEnsureOpen())
        return ret;
    if (!CurrentTrigger || SelectedActionIndex < 0 || slot < 0 || slot >= ACTION_PARAM_COUNT || !ActionParamsUsage[slot].first)
        return ret;

    LabelMatcher matcher(filter.c_str());
    int count = vcbActionParameter[slot].GetCount();
    bool valueFromFirstSpace = ActionParamType[slot] == ParamType::Trigger || ActionParamType[slot] == ParamType::Team;
    for (int i = 0; i < count; ++i)
    {
        FString item = vcbActionParameter[slot].GetItemText(i);
        std::string value, text;
        SplitOptionText(item, value, text, valueFromFirstSpace);

        bool match = filter.empty() || matcher.Match(text.c_str()) || matcher.Match(value.c_str());
        if (!match)
            continue;

        if (max > 0 && (int)ret.size() >= max)
            break;

        TeOption o;
        o.value = value;
        o.text = text;
        ret.push_back(std::move(o));
    }
    return ret;
}

CNewTrigger::TeEntryInfo CNewTrigger::TeSetEventType(int num)
{
    if (!TeEnsureOpen())
        return {};
    if (!CurrentTrigger || SelectedEventIndex < 0)
        return {};

    FString n;
    n.Format("%d", num);
    UpdateEventAndParam(atoi(n), true);
    OnSelchangeEventListbox(false);
    RefreshOtherInstances();
    CLuaConsole::updateTrigger = true;
    return BuildEventInfo(SelectedEventIndex);
}

CNewTrigger::TeEntryInfo CNewTrigger::TeSetActionType(int num)
{
    if (!TeEnsureOpen())
        return {};
    if (!CurrentTrigger || SelectedActionIndex < 0)
        return {};

    FString n;
    n.Format("%d", num);
    UpdateActionAndParam(atoi(n), true);
    OnSelchangeActionListbox(false);
    RefreshOtherInstances();
    CLuaConsole::updateTrigger = true;
    return BuildActionInfo(SelectedActionIndex);
}

CNewTrigger::TeSetResult CNewTrigger::TeSetEventParamDirect(int slot, const std::string& value)
{
    TeSetResult r;
    if (!TeEnsureOpen())
        return r;
    if (!CurrentTrigger || SelectedEventIndex < 0 || slot < 0 || slot >= EVENT_PARAM_COUNT || !EventParamsUsage[slot].first)
    {
        r.error = "parameter slot unused or no event selected";
        return r;
    }

    int raw = EventParamsUsage[slot].second;
    CurrentTrigger->Events[SelectedEventIndex].Params[raw] = value.c_str();
    CurrentTrigger->Save();
    UpdateParamAffectedParam_Event(slot);
    vcbEventParameter[slot].SetEditText(value.c_str());
    RefreshOtherInstances();
    CLuaConsole::updateTrigger = true;

    r.ok = true;
    r.value = value;
    r.display = value;
    return r;
}

CNewTrigger::TeSetResult CNewTrigger::TeSetEventParamFuzzy(int slot, const std::string& text)
{
    TeSetResult r;
    if (!TeEnsureOpen())
        return r;
    if (!CurrentTrigger || SelectedEventIndex < 0 || slot < 0 || slot >= EVENT_PARAM_COUNT || !EventParamsUsage[slot].first)
    {
        r.error = "parameter slot unused or no event selected";
        return r;
    }

    std::string want = text;
    std::string wantLower = want;
    std::transform(wantLower.begin(), wantLower.end(), wantLower.begin(), ::tolower);

    int count = vcbEventParameter[slot].GetCount();
    bool valueFromFirstSpace = EventParamType[slot] == ParamType::Trigger || EventParamType[slot] == ParamType::Team;
    int hit = -1;
    std::string hitValue, hitText;
    for (int i = 0; i < count; ++i)
    {
        FString item = vcbEventParameter[slot].GetItemText(i);
        std::string value, disp;
        SplitOptionText(item, value, disp, valueFromFirstSpace);
        std::string dispLower = disp;
        std::transform(dispLower.begin(), dispLower.end(), dispLower.begin(), ::tolower);
        if (value == want)
        {
            hit = i; hitValue = value; hitText = disp; break;
        }
        if (hit < 0 && disp == want)
        {
            hit = i; hitValue = value; hitText = disp;
        }
    }
    if (hit < 0)
    {
        for (int i = 0; i < count; ++i)
        {
            FString item = vcbEventParameter[slot].GetItemText(i);
            std::string value, disp;
            SplitOptionText(item, value, disp, valueFromFirstSpace);
            std::string dispLower = disp;
            std::transform(dispLower.begin(), dispLower.end(), dispLower.begin(), ::tolower);
            if (dispLower.find(wantLower) != std::string::npos)
            {
                hit = i; hitValue = value; hitText = disp; break;
            }
        }
    }

    if (hit < 0)
    {
        r.error = "te_set_event_param_fuzzy: no matching option found, nothing changed";
        TeReport(r.error + "  (param slot " + std::to_string(slot + 1) + " looking for \"" + text + "\")");
        return r;
    }

    vcbEventParameter[slot].SetCurSel(hit);
    OnSelchangeEventParam(slot, true);
    RefreshOtherInstances();
    CLuaConsole::updateTrigger = true;

    r.ok = true;
    r.value = hitValue;
    r.display = hitText;
    return r;
}

CNewTrigger::TeSetResult CNewTrigger::TeSetActionParamDirect(int slot, const std::string& value)
{
    TeSetResult r;
    if (!TeEnsureOpen())
        return r;
    if (!CurrentTrigger || SelectedActionIndex < 0 || slot < 0 || slot >= ACTION_PARAM_COUNT || !ActionParamsUsage[slot].first)
    {
        r.error = "parameter slot unused or no action selected";
        return r;
    }

    int raw = ActionParamsUsage[slot].second;
    CurrentTrigger->Actions[SelectedActionIndex].Params[raw] = value.c_str();
    CurrentTrigger->Save();
    UpdateParamAffectedParam_Action(slot);
    vcbActionParameter[slot].SetEditText(value.c_str());
    RefreshOtherInstances();
    CLuaConsole::updateTrigger = true;

    r.ok = true;
    r.value = value;
    r.display = value;
    return r;
}

CNewTrigger::TeSetResult CNewTrigger::TeSetActionParamFuzzy(int slot, const std::string& text)
{
    TeSetResult r;
    if (!TeEnsureOpen())
        return r;
    if (!CurrentTrigger || SelectedActionIndex < 0 || slot < 0 || slot >= ACTION_PARAM_COUNT || !ActionParamsUsage[slot].first)
    {
        r.error = "parameter slot unused or no action selected";
        return r;
    }

    std::string want = text;
    std::string wantLower = want;
    std::transform(wantLower.begin(), wantLower.end(), wantLower.begin(), ::tolower);

    int count = vcbActionParameter[slot].GetCount();
    bool valueFromFirstSpace = ActionParamType[slot] == ParamType::Trigger || ActionParamType[slot] == ParamType::Team;
    int hit = -1;
    std::string hitValue, hitText;
    for (int i = 0; i < count; ++i)
    {
        FString item = vcbActionParameter[slot].GetItemText(i);
        std::string value, disp;
        SplitOptionText(item, value, disp, valueFromFirstSpace);
        std::string dispLower = disp;
        std::transform(dispLower.begin(), dispLower.end(), dispLower.begin(), ::tolower);
        if (value == want)
        {
            hit = i; hitValue = value; hitText = disp; break;
        }
        if (hit < 0 && disp == want)
        {
            hit = i; hitValue = value; hitText = disp;
        }
    }
    if (hit < 0)
    {
        for (int i = 0; i < count; ++i)
        {
            FString item = vcbActionParameter[slot].GetItemText(i);
            std::string value, disp;
            SplitOptionText(item, value, disp, valueFromFirstSpace);
            std::string dispLower = disp;
            std::transform(dispLower.begin(), dispLower.end(), dispLower.begin(), ::tolower);
            if (dispLower.find(wantLower) != std::string::npos)
            {
                hit = i; hitValue = value; hitText = disp; break;
            }
        }
    }

    if (hit < 0)
    {
        r.error = "te_set_action_param_fuzzy: no matching option found, nothing changed";
        TeReport(r.error + "  (param slot " + std::to_string(slot + 1) + " looking for \"" + text + "\")");
        return r;
    }

    vcbActionParameter[slot].SetCurSel(hit);
    OnSelchangeActionParam(slot, true);
    RefreshOtherInstances();
    CLuaConsole::updateTrigger = true;

    r.ok = true;
    r.value = hitValue;
    r.display = hitText;
    return r;
}

bool CNewTrigger::TeAddEvent()
{
    if (!TeEnsureOpen())
        return false;
    if (!CurrentTrigger)
        return false;
    OnClickNewEvent(m_hwnd);
    CLuaConsole::updateTrigger = true;
    return true;
}

bool CNewTrigger::TeCloneEvent(int index)
{
    if (!TeEnsureOpen())
        return false;
    if (!CurrentTrigger || index < 0 || index >= CurrentTrigger->EventCount)
        return false;
    std::vector<int> sel{ index };
    SetEventListBoxSels(sel);
    OnClickCloEvent(m_hwnd);
    CLuaConsole::updateTrigger = true;
    return true;
}

bool CNewTrigger::TeDeleteEventSel(int index)
{
    if (!TeEnsureOpen())
        return false;
    if (!CurrentTrigger || index < 0 || index >= CurrentTrigger->EventCount)
        return false;
    std::vector<int> sel{ index };
    SetEventListBoxSels(sel);
    OnClickDelEvent(m_hwnd);
    CLuaConsole::updateTrigger = true;
    return true;
}

bool CNewTrigger::TeAddAction()
{
    if (!TeEnsureOpen())
        return false;
    if (!CurrentTrigger)
        return false;
    OnClickNewAction(m_hwnd);
    CLuaConsole::updateTrigger = true;
    return true;
}

bool CNewTrigger::TeCloneAction(int index)
{
    if (!TeEnsureOpen())
        return false;
    if (!CurrentTrigger || index < 0 || index >= CurrentTrigger->ActionCount)
        return false;
    std::vector<int> sel{ index };
    SetActionListBoxSels(sel);
    OnClickCloAction(m_hwnd);
    CLuaConsole::updateTrigger = true;
    return true;
}

bool CNewTrigger::TeDeleteActionSel(int index)
{
    if (!TeEnsureOpen())
        return false;
    if (!CurrentTrigger || index < 0 || index >= CurrentTrigger->ActionCount)
        return false;
    std::vector<int> sel{ index };
    SetActionListBoxSels(sel);
    OnClickDelAction(m_hwnd);
    CLuaConsole::updateTrigger = true;
    return true;
}


CNewTrigger::TeEntryInfo CNewTrigger::BuildEventInfo(int idx)
{
    TeEntryInfo ret;
    if (!CurrentTrigger || idx < 0 || idx >= CurrentTrigger->EventCount)
        return ret;

    SelectedEventIndex = idx;
    SetEventListBoxSel(idx);
    UpdateEventAndParam(-1, false);

    auto& ev = CurrentTrigger->Events[idx];
    ret.ok = true;
    ret.num = ev.EventNum.c_str();
    ret.name = ExtraWindow::GetEventDisplayName(ev.EventNum, idx, !CompactMode).c_str();

    auto infos = FString::SplitString(
        fadata.GetString(ExtraWindow::GetTranslatedSectionName("EventsRA2"), ev.EventNum, "MISSING,0,0,0,0,MISSING,0,1,0"), 8);
    ret.desc = infos.size() > 5 ? FString::ReplaceSpeicalString(infos[5]).c_str() : "";

    FString paramType[2];
    paramType[0] = infos.size() > 1 ? infos[1] : FString("0");
    paramType[1] = infos.size() > 2 ? infos[2] : FString("0");
    std::vector<FString> pParamTypes[2];
    pParamTypes[0] = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("ParamTypes"), paramType[0], "MISSING,0"), 1);
    pParamTypes[1] = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("ParamTypes"), paramType[1], "MISSING,0"), 1);

    ret.params.resize(EVENT_PARAM_COUNT);
    for (int s = 0; s < EVENT_PARAM_COUNT; ++s)
    {
        TeParamSlot p;
        p.slot = s + 1;
        if (EventParamsUsage[s].first)
        {
            p.used = true;
            int raw = EventParamsUsage[s].second;
            p.value = (raw >= 0 && raw < 3) ? ev.Params[raw].c_str() : "";
            int bucket = ev.P3Enabled ? (raw - 1) : 1;
            if (bucket >= 0 && bucket < 2)
            {
                p.desc = pParamTypes[bucket][0].c_str();
                p.type = pParamTypes[bucket][1].c_str();
            }
            p.display = vcbEventParameter[s].GetSelectedText(false);
        }
        ret.params[s] = std::move(p);
    }
    return ret;
}

CNewTrigger::TeEntryInfo CNewTrigger::BuildActionInfo(int idx)
{
    TeEntryInfo ret;
    if (!CurrentTrigger || idx < 0 || idx >= CurrentTrigger->ActionCount)
        return ret;

    SelectedActionIndex = idx;
    SetActionListBoxSel(idx);
    UpdateActionAndParam(-1, false);

    auto& ac = CurrentTrigger->Actions[idx];
    ret.ok = true;
    ret.num = ac.ActionNum.c_str();
    ret.name = ExtraWindow::GetActionDisplayName(ac.ActionNum, idx, !CompactMode).c_str();

    auto infos = FString::SplitString(
        fadata.GetString(ExtraWindow::GetTranslatedSectionName("ActionsRA2"), ac.ActionNum, "MISSING,0,0,0,0,0,0,0,0,0,MISSING,0,1,0"), 13);
    ret.desc = infos.size() > 10 ? FString::ReplaceSpeicalString(infos[10]).c_str() : "";

    std::vector<FString> pParamTypes[ACTION_PARAM_COUNT];
    for (int t = 0; t < ACTION_PARAM_COUNT; ++t)
    {
        FString pt = infos.size() > t + 1 ? infos[t + 1] : FString("0");
        pParamTypes[t] = FString::SplitString(fadata.GetString(ExtraWindow::GetTranslatedSectionName("ParamTypes"), pt, "MISSING,0"), 1);
    }

    ret.params.resize(ACTION_PARAM_COUNT);
    for (int s = 0; s < ACTION_PARAM_COUNT; ++s)
    {
        TeParamSlot p;
        p.slot = s + 1;
        if (ActionParamsUsage[s].first)
        {
            p.used = true;
            int raw = ActionParamsUsage[s].second;
            p.value = (raw >= 0 && raw < 7) ? ac.Params[raw].c_str() : "";
            if (raw != 6)
            {
                if (raw >= 0 && raw < ACTION_PARAM_COUNT)
                {
                    p.desc = pParamTypes[raw][0].c_str();
                    p.type = pParamTypes[raw][1].c_str();
                }
            }
            else
            {
                p.type = "1";
                if (ac.Param7isWP)
                    p.desc = Translations::TranslateOrDefault("TriggerP7Waypoint", "Waypoint").c_str();
                else
                    p.desc = Translations::TranslateOrDefault("TriggerP7Number", "Number").c_str();
            }
            p.display = vcbActionParameter[s].GetSelectedText(false);
        }
        ret.params[s] = std::move(p);
    }
    return ret;
}


static void TeFillTypeList(sol::state& Lua, sol::table& ret, const std::vector<CNewTrigger::TeTypeInfo>& list)
{
    int i = 1;
    for (auto& t : list)
    {
        sol::table e = Lua.create_table();
        e["num"] = t.num;
        e["name"] = t.name;
        e["desc"] = t.desc;
        ret[i++] = e;
    }
}

static void TeFillOptionList(sol::state& Lua, sol::table& ret, const std::vector<CNewTrigger::TeOption>& list)
{
    int i = 1;
    for (auto& o : list)
    {
        sol::table e = Lua.create_table();
        e["value"] = o.value;
        e["text"] = o.text;
        ret[i++] = e;
    }
}

static void TeFillParams(sol::state& Lua, sol::table& out, const std::vector<CNewTrigger::TeParamSlot>& params)
{
    sol::table arr = Lua.create_table();
    for (size_t i = 0; i < params.size(); ++i)
    {
        const auto& p = params[i];
        sol::table e = Lua.create_table();
        e["slot"] = p.slot;
        e["used"] = p.used;
        e["desc"] = p.desc;
        e["type"] = p.type;
        e["value"] = p.value;
        e["display"] = p.display;
        arr[i + 1] = e;
    }
    out["params"] = arr;
}

static sol::object TeToEntryOrNil(sol::state& Lua, const CNewTrigger::TeEntryInfo& info)
{
    if (!info.ok)
        return sol::nil;
    sol::table t = Lua.create_table();
    t["ok"] = true;
    t["num"] = info.num;
    t["name"] = info.name;
    t["desc"] = info.desc;
    TeFillParams(Lua, t, info.params);
    return t;
}

static sol::object TeToTriggerInfoOrNil(sol::state& Lua, const CNewTrigger::TeTriggerInfo& info)
{
    if (!info.ok)
        return sol::nil;
    sol::table t = Lua.create_table();
    t["ok"] = true;
    t["id"] = info.id;
    t["name"] = info.name;
    t["house"] = info.house;
    t["attached_trigger"] = info.attached_trigger;
    t["disabled"] = info.disabled;
    t["easy"] = info.easy;
    t["medium"] = info.medium;
    t["hard"] = info.hard;
    t["repeat_type"] = info.repeat_type;
    {
        sol::table events = Lua.create_table();
        for (size_t i = 0; i < info.events.size(); ++i)
            events[i + 1] = TeToEntryOrNil(Lua, info.events[i]);
        t["events"] = events;
    }
    {
        sol::table actions = Lua.create_table();
        for (size_t i = 0; i < info.actions.size(); ++i)
            actions[i + 1] = TeToEntryOrNil(Lua, info.actions[i]);
        t["actions"] = actions;
    }
    return t;
}

static sol::object TeToResultOrNil(sol::state& Lua, const CNewTrigger::TeSetResult& r, bool reportError)
{
    if (!r.ok)
    {
        if (reportError && !r.error.empty())
            LuaFunctions::TriggerLua_Report(r.error);
        return sol::nil;
    }
    sol::table t = Lua.create_table();
    t["ok"] = true;
    t["value"] = r.value;
    t["display"] = r.display;
    return t;
}

void CNewTrigger::RegisterHeadlessTriggerLua(sol::state& Lua)
{
    Lua.set_function("te_new_trigger", [&Lua](sol::this_state, sol::variadic_args sa) -> sol::object {
        if (!CNewTrigger::Headless.TeEnsureOpen())
            return sol::nil;
        const char* name = sa.size() > 0 ? sa[0].as<const char*>() : "";
        const char* house = sa.size() > 1 ? sa[1].as<const char*>() : "";
        std::string id = CNewTrigger::Headless.TeNewTrigger(name, house);
        if (id.empty())
            return sol::nil;
        sol::table t = Lua.create_table();
        t["id"] = id;
        return t;
    });
    Lua.set_function("te_select_trigger", [](sol::this_state, const char* id) -> bool {
        return CNewTrigger::Headless.TeSelectTrigger(id);
    });
    Lua.set_function("te_get_selected_trigger", [&Lua](sol::this_state) -> sol::object {
        if (!CNewTrigger::Headless.TeEnsureOpen())
            return sol::nil;
        std::string id = CNewTrigger::Headless.TeGetSelectedTrigger();
        if (id.empty())
            return sol::nil;
        return sol::make_object(Lua, id);
    });
    Lua.set_function("te_get_trigger", [&Lua](sol::this_state) -> sol::object {
        if (!CNewTrigger::Headless.TeEnsureOpen())
            return sol::nil;
        return TeToTriggerInfoOrNil(Lua, CNewTrigger::Headless.TeGetTriggerInfo());
    });
    Lua.set_function("te_set_trigger_prop", [](sol::this_state, const char* key, const char* value) -> bool {
        return CNewTrigger::Headless.TeSetTriggerProp(key ? key : "", value ? value : "");
    });
    Lua.set_function("te_delete_trigger", [&Lua](sol::this_state, sol::variadic_args sa) -> bool {
        if (!CNewTrigger::Headless.TeEnsureOpen())
            return false;
        bool keepTags = sa.size() > 0 && sa[0].as<bool>();
        return CNewTrigger::Headless.TeDeleteTrigger(keepTags);
    });

    Lua.set_function("te_get_event_types", [&Lua](sol::this_state, sol::variadic_args sa) -> sol::table {
        std::string filter = sa.size() > 0 ? sa[0].as<std::string>() : "";
        int max = sa.size() > 1 ? (int)sa[1].as<int>() : 50;
        sol::table ret = Lua.create_table();
        TeFillTypeList(Lua, ret, CNewTrigger::Headless.TeGetEventTypes(filter, max));
        return ret;
    });
    Lua.set_function("te_get_action_types", [&Lua](sol::this_state, sol::variadic_args sa) -> sol::table {
        std::string filter = sa.size() > 0 ? sa[0].as<std::string>() : "";
        int max = sa.size() > 1 ? (int)sa[1].as<int>() : 50;
        sol::table ret = Lua.create_table();
        TeFillTypeList(Lua, ret, CNewTrigger::Headless.TeGetActionTypes(filter, max));
        return ret;
    });
    Lua.set_function("te_get_event_type", [&Lua](sol::this_state, int num) -> sol::object {
        if (!CNewTrigger::Headless.TeEnsureOpen())
            return sol::nil;
        CNewTrigger::TeTypeInfo info = CNewTrigger::Headless.TeGetEventTypeInfo(num);
        if (info.num.empty())
            return sol::nil;
        sol::table t = Lua.create_table();
        t["num"] = info.num;
        t["name"] = info.name;
        t["desc"] = info.desc;
        return t;
    });
    Lua.set_function("te_get_action_type", [&Lua](sol::this_state, int num) -> sol::object {
        if (!CNewTrigger::Headless.TeEnsureOpen())
            return sol::nil;
        CNewTrigger::TeTypeInfo info = CNewTrigger::Headless.TeGetActionTypeInfo(num);
        if (info.num.empty())
            return sol::nil;
        sol::table t = Lua.create_table();
        t["num"] = info.num;
        t["name"] = info.name;
        t["desc"] = info.desc;
        return t;
    });

    Lua.set_function("te_select_event", [&Lua](sol::this_state, int idx) -> sol::object {
        if (!CNewTrigger::Headless.TeEnsureOpen())
            return sol::nil;
        return TeToEntryOrNil(Lua, CNewTrigger::Headless.TeSelectEvent(idx - 1));
    });
    Lua.set_function("te_get_event_options", [&Lua](sol::this_state, sol::variadic_args sa) -> sol::table {
        int slot = sa.size() > 0 ? (int)sa[0].as<int>() : 1;
        std::string filter = sa.size() > 1 ? sa[1].as<std::string>() : "";
        int max = sa.size() > 2 ? (int)sa[2].as<int>() : 50;
        sol::table ret = Lua.create_table();
        TeFillOptionList(Lua, ret, CNewTrigger::Headless.TeGetEventOptions(slot - 1, filter, max));
        return ret;
    });
    Lua.set_function("te_set_event_type", [&Lua](sol::this_state, const char* num) -> sol::object {
        if (!CNewTrigger::Headless.TeEnsureOpen())
            return sol::nil;
        return TeToEntryOrNil(Lua, CNewTrigger::Headless.TeSetEventType(atoi(num ? num : "0")));
    });
    Lua.set_function("te_set_event_param", [&Lua](sol::this_state, int slot, const char* value) -> sol::object {
        if (!CNewTrigger::Headless.TeEnsureOpen())
            return sol::nil;
        return TeToResultOrNil(Lua, CNewTrigger::Headless.TeSetEventParamDirect(slot - 1, value ? value : ""), false);
    });
    Lua.set_function("te_set_event_param_fuzzy", [&Lua](sol::this_state, int slot, const char* text) -> sol::object {
        if (!CNewTrigger::Headless.TeEnsureOpen())
            return sol::nil;
        return TeToResultOrNil(Lua, CNewTrigger::Headless.TeSetEventParamFuzzy(slot - 1, text ? text : ""), true);
    });
    Lua.set_function("te_add_event", [](sol::this_state) -> bool {
        return CNewTrigger::Headless.TeAddEvent();
    });
    Lua.set_function("te_clone_event", [](sol::this_state, int idx) -> bool {
        return CNewTrigger::Headless.TeCloneEvent(idx - 1);
    });
    Lua.set_function("te_delete_event", [](sol::this_state, int idx) -> bool {
        return CNewTrigger::Headless.TeDeleteEventSel(idx - 1);
    });

    Lua.set_function("te_select_action", [&Lua](sol::this_state, int idx) -> sol::object {
        if (!CNewTrigger::Headless.TeEnsureOpen())
            return sol::nil;
        return TeToEntryOrNil(Lua, CNewTrigger::Headless.TeSelectAction(idx - 1));
    });
    Lua.set_function("te_get_action_options", [&Lua](sol::this_state, sol::variadic_args sa) -> sol::table {
        int slot = sa.size() > 0 ? (int)sa[0].as<int>() : 1;
        std::string filter = sa.size() > 1 ? sa[1].as<std::string>() : "";
        int max = sa.size() > 2 ? (int)sa[2].as<int>() : 50;
        sol::table ret = Lua.create_table();
        TeFillOptionList(Lua, ret, CNewTrigger::Headless.TeGetActionOptions(slot - 1, filter, max));
        return ret;
    });
    Lua.set_function("te_set_action_type", [&Lua](sol::this_state, const char* num) -> sol::object {
        if (!CNewTrigger::Headless.TeEnsureOpen())
            return sol::nil;
        return TeToEntryOrNil(Lua, CNewTrigger::Headless.TeSetActionType(atoi(num ? num : "0")));
    });
    Lua.set_function("te_set_action_param", [&Lua](sol::this_state, int slot, const char* value) -> sol::object {
        if (!CNewTrigger::Headless.TeEnsureOpen())
            return sol::nil;
        return TeToResultOrNil(Lua, CNewTrigger::Headless.TeSetActionParamDirect(slot - 1, value ? value : ""), false);
    });
    Lua.set_function("te_set_action_param_fuzzy", [&Lua](sol::this_state, int slot, const char* text) -> sol::object {
        if (!CNewTrigger::Headless.TeEnsureOpen())
            return sol::nil;
        return TeToResultOrNil(Lua, CNewTrigger::Headless.TeSetActionParamFuzzy(slot - 1, text ? text : ""), true);
    });
    Lua.set_function("te_add_action", [](sol::this_state) -> bool {
        return CNewTrigger::Headless.TeAddAction();
    });
    Lua.set_function("te_clone_action", [](sol::this_state, int idx) -> bool {
        return CNewTrigger::Headless.TeCloneAction(idx - 1);
    });
    Lua.set_function("te_delete_action", [](sol::this_state, int idx) -> bool {
        return CNewTrigger::Headless.TeDeleteActionSel(idx - 1);
    });
}