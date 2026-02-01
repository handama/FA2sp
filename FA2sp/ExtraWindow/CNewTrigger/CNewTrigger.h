#pragma once

#include <FA2PP.h>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Helpers/MultimapHelper.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/FString.h"

#define EVENT_PARAM_COUNT 2
#define ACTION_PARAM_COUNT 6
#define TRIGGER_EDITOR_MAX_COUNT 10

struct ParamAffectedParams
{
    int Index;
    int SourceParam;
    int AffectedParam;
    std::map<FString, FString> ParamMap;
};

struct EventParams
{
    FString EventNum;
    bool P3Enabled;
    FString Params[3];
};

struct ActionParams
{
    FString ActionNum;
    FString Params[7];
    bool Param7isWP;
};

class Trigger
{
public:
    FString ID;
    FString Name;
    FString House;
    FString Tag;
    FString TagName;
    FString RepeatType;
    FString AttachedTrigger;
    FString Obsolete;
    bool Disabled;
    bool EasyEnabled;
    bool MediumEnabled;
    bool HardEnabled;
    int EventCount;
    std::vector<EventParams> Events;
    int ActionCount;
    std::vector<ActionParams> Actions;

    static Trigger* create(const char* id, std::map<FString, FString>* pTagMap = nullptr)
    {
        auto atoms = FString::SplitString(CINI::CurrentDocument().GetString("Triggers", id));
        if (atoms.size() < 8)
            return nullptr;
        return new Trigger(id, pTagMap);
    }

    void Save() {
        FString trigger;
        trigger.Format("%s,%s,%s,%s,%s,%s,%s,%s", House, AttachedTrigger, Name,
            Disabled ? "1" : "0", EasyEnabled ? "1" : "0", MediumEnabled ? "1" : "0", HardEnabled ? "1" : "0", Obsolete);
        CINI::CurrentDocument().WriteString("Triggers", ID, trigger);
        auto tag = CINI::CurrentDocument().GetString("Tags", Tag, "");
        if (tag != "")
        {
            auto atoms = FString::SplitString(tag, 2);
            if (atoms[2] == ID)
            {
                tag.Format("%s,%s,%s", RepeatType, TagName, atoms[2]);
                CINI::CurrentDocument().WriteString("Tags", Tag, tag);
            }
        }
        FString cEvent;
        cEvent.Format("%d", EventCount);
        for (auto& thisEvent : Events)
        {
            FString tmp;
            if (thisEvent.P3Enabled)
                tmp.Format(",%s,%s,%s,%s", thisEvent.EventNum, thisEvent.Params[0], thisEvent.Params[1], thisEvent.Params[2]);
            else
                tmp.Format(",%s,%s,%s", thisEvent.EventNum, thisEvent.Params[0], thisEvent.Params[1]);
            cEvent += tmp;
        }
        CINI::CurrentDocument().WriteString("Events", ID, cEvent);

        FString cAction;
        cAction.Format("%d", ActionCount);
        for (auto& thisAction : Actions)
        {
            FString tmp;
            tmp.Format(",%s,%s,%s,%s,%s,%s,%s,%s", thisAction.ActionNum,
                thisAction.Params[0], thisAction.Params[1], thisAction.Params[2], thisAction.Params[3]
                , thisAction.Params[4], thisAction.Params[5], thisAction.Params[6]);
            cAction += tmp;
        }
        CINI::CurrentDocument().WriteString("Actions", ID, cAction);
    }

    void LoadFromMap(const char* id, std::map<FString, FString>* pTagMap = nullptr)
    {
        auto atoms = FString::SplitString(CINI::CurrentDocument().GetString("Triggers", id));
        ID = id;
        House = atoms[0];
        AttachedTrigger = atoms[1];
        Name = atoms[2];
        Disabled = atoms[3] == "1" ? true : false;
        EasyEnabled = atoms[4] == "1" ? true : false;
        MediumEnabled = atoms[5] == "1" ? true : false;
        HardEnabled = atoms[6] == "1" ? true : false;
        Obsolete = atoms[7];
        Tag = "<none>";
        TagName = "";
        RepeatType = "-1";
        Events.clear();
        Actions.clear();
        EventCount = 0;
        ActionCount = 0;

        if (pTagMap)
        {
            auto& tagMap = *pTagMap;
            auto itr = tagMap.find(ID);
            if (itr != tagMap.end())
            {
                Tag = itr->second;
                auto atoms = FString::SplitString(CINI::CurrentDocument().GetString("Tags", Tag), 2);
                RepeatType = atoms[0];
                TagName = atoms[1];
            }
        }
        else
        {
            // let's assume that tag is the next ID of trigger, for most of them are.
            int assumeIdx = atoi(ID) + 1;
            FString assumeIdxTag;
            assumeIdxTag.Format("%08d", assumeIdx);
            auto assumeTagValue = CINI::CurrentDocument().GetString("Tags", assumeIdxTag);
            bool scanTag = true;

            if (assumeTagValue != "")
            {
                auto assumeTagAtoms = FString::SplitString(CINI::CurrentDocument().GetString("Tags", assumeIdxTag), 2);
                if (assumeTagAtoms[2] == ID)
                {
                    Tag = assumeIdxTag;
                    RepeatType = assumeTagAtoms[0];
                    TagName = assumeTagAtoms[1];
                    scanTag = false;
                }
            }
            if (scanTag)
            {
                if (auto pSection = CINI::CurrentDocument().GetSection("Tags"))
                {
                    for (auto& kvp : pSection->GetEntities())
                    {
                        auto tagAtoms = FString::SplitString(kvp.second);
                        if (tagAtoms.size() < 3) continue;
                        if (tagAtoms[2] == ID)
                        {
                            Tag = kvp.first;
                            RepeatType = tagAtoms[0];
                            TagName = tagAtoms[1];
                            break;
                        }
                    }
                }
            }
        }

        auto eventAtoms = FString::SplitString(CINI::CurrentDocument().GetString("Events", ID));
        if (!eventAtoms.empty())
        {
            EventCount = atoi(eventAtoms[0]);
            if (EventCount != 0)
            {
                int readIdx = 1; //read atoms one by one;
                bool p0 = true;
                bool p1 = false;
                bool p2 = false;
                bool p3 = false;
                EventParams thisEvent;
                while (true)
                {
                    FString atom;
                    if (eventAtoms.size() > readIdx)
                        atom = eventAtoms[readIdx];
                    else
                        atom = "0";
                    if (p0)
                    {
                        p0 = false;
                        p1 = true;
                        p2 = false;
                        p3 = false;
                        thisEvent.EventNum = atom;
                    }
                    else if (p1)
                    {
                        p0 = false;
                        p1 = false;
                        p2 = true;
                        p3 = false;
                        thisEvent.Params[0] = atom;
                        if (atoi(atom) == 2) thisEvent.P3Enabled = true;
                        else thisEvent.P3Enabled = false;
                    }
                    else if (p2)
                    {
                        p2 = false;
                        thisEvent.Params[1] = atom;
                        if (thisEvent.P3Enabled)
                        {
                            p0 = false;
                            p1 = false;
                            p3 = true;
                        }
                        else
                        {
                            p0 = true;
                            p1 = false;
                            p3 = false;
                            Events.push_back(thisEvent);
                        }
                    }
                    else if (p3)
                    {
                        p0 = true;
                        p1 = false;
                        p2 = false;
                        p3 = false;
                        thisEvent.Params[2] = atom;
                        Events.push_back(thisEvent);
                    }
                    if (Events.size() == EventCount) break;
                    readIdx++;
                }
            }
        }
        else
            EventCount = 0;

        auto actionAtoms = FString::SplitString(CINI::CurrentDocument().GetString("Actions", ID));
        if (!actionAtoms.empty())
        {
            ActionCount = atoi(actionAtoms[0]);
            if (ActionCount != 0)
            {
                actionAtoms = FString::SplitStringAction(CINI::CurrentDocument().GetString("Actions", ID), ActionCount * 8);
                for (int i = 0; i < ActionCount; i++)
                {
                    ActionParams thisAction;
                    thisAction.ActionNum = actionAtoms[1 + i * 8];
                    thisAction.Params[0] = actionAtoms[2 + i * 8];
                    thisAction.Params[1] = actionAtoms[3 + i * 8];
                    thisAction.Params[2] = actionAtoms[4 + i * 8];
                    thisAction.Params[3] = actionAtoms[5 + i * 8];
                    thisAction.Params[4] = actionAtoms[6 + i * 8];
                    thisAction.Params[5] = actionAtoms[7 + i * 8];
                    thisAction.Params[6] = actionAtoms[8 + i * 8];
                    thisAction.Param7isWP = true;
                    for (auto& pair : CINI::FAData().GetSection("DontSaveAsWP")->GetEntities())
                    {
                        if (atoi(pair.second) == -atoi(thisAction.Params[0]))
                            thisAction.Param7isWP = false;
                    }
                    Actions.push_back(thisAction);
                }
            }
        }
        else
            ActionCount = 0;
    }

private:
    Trigger(const char* id, std::map<FString, FString>* pTagMap = nullptr)
    {
        LoadFromMap(id, pTagMap);
    }

};

// A window class
class CNewTrigger
{
public:
    enum Controls {
        SelectedTrigger = 50903,
        NewTrigger = 50904,
        CloneTrigger = 50905,
        DeleteTrigger = 50906,
        PlaceOnMap = 50907,
        Type = 50909,
        Name = 50911,
        House = 50913,
        Attachedtrigger = 50916,
        Disabled = 50917,
        Easy = 50918,
        Medium = 50919,
        Hard = 50920,
        Eventtype = 50923,
        NewEvent = 50924,
        CloneEvent = 50925,
        DeleteEvent = 50926,
        EventDescription = 50927,
        EventList = 50929,
        EventParameter1Desc = 50930,
        EventParameter1 = 50931,
        EventParameter2Desc = 50932,
        EventParameter2 = 50933,
        Actionoptions = 50934,
        Actiontype = 50940,
        Actionframe = 50934,
        NewAction = 50937,
        DeleteAction = 50938,
        CloneAction = 50939,
        ActionDescription = 50936,
        ActionList = 50942,
        ActionParameter1Desc = 50943,
        ActionParameter1 = 50944,
        ActionParameter2Desc = 50945,
        ActionParameter2 = 50946,
        ActionParameter3Desc = 50947,
        ActionParameter3 = 50948,
        ActionParameter4Desc = 50949,
        ActionParameter4 = 50950,
        ActionParameter5Desc = 50951,
        ActionParameter5 = 50952,
        ActionParameter6Desc = 50953,
        ActionParameter6 = 50954,
        SearchReference = 1999,
        OpenNewEditor = 2000,
        DragPoint = 2001,
        Compact = 2002,
        ActionMoveUp = 2003,
        ActionMoveDown = 2004,
    };

    void Create(CFinalSunDlg* pWnd);

    HWND GetHandle()
    {
        return m_hwnd;
    }
    static CNewTrigger& GetFirstValidInstance();

    bool OnEnterKeyDown(HWND& hWnd);
    void OnSelchangeTrigger(bool edited = false, int eventListCur = 0, int actionListCur = 0, bool reloadTrigger = true);
    void OnSelchangeAttachedTrigger(bool edited = false);
    void OnSelchangeEventType(bool edited = false);
    void OnSelchangeActionType(bool edited = false);
    void OnSelchangeEventParam(int index, bool edited = false);
    void OnSelchangeActionParam(int index, bool edited = false);
    void OnClickNewTrigger();
    void OnSelchangeEventListbox(bool changeCursel = true);
    void OnSelchangeActionListbox(bool changeCursel = true);

protected:
    void Initialize(HWND& hWnd);
    void Update(HWND& hWnd, bool UpdateTrigger = true);

    void OnSeldropdownTrigger(HWND& hWnd);
    
    void OnClickDelTrigger(HWND& hWnd);
    void OnClickCloTrigger(HWND& hWnd);
    void OnClickPlaceOnMap(HWND& hWnd);
    void OnClickNewEvent(HWND& hWnd);
    void OnClickDelEvent(HWND& hWnd);
    void OnClickCloEvent(HWND& hWnd);
    void OnClickNewAction(HWND& hWnd);
    void OnClickDelAction(HWND& hWnd);
    void OnClickCloAction(HWND& hWnd);
    void OnClickSearchReference(HWND& hWnd);
    void OnClickActionMove(HWND& hWnd, bool isUp);

    void OnSelchangeHouse(bool edited = false);
    void OnSelchangeType(bool edited = false);
    void UpdateEventAndParam(int changedEvent = -1, bool changeCursel = true);
    void UpdateActionAndParam(int changedAction = -1, bool changeCursel = true);
    void AdjustActionHeight();
    void UpdateParamAffectedParam_Action(int index);
    void UpdateParamAffectedParam_Event(int index);

    void OnCloseupCComboBox(HWND& hWnd, std::map<int, FString>& labels, bool isComboboxSelectOnly = false);
    void OnDropdownCComboBox(int index);

    void SortTriggers(FString id = "", bool onlySelf = false);

    void Close(HWND& hWnd);

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    BOOL CALLBACK HandleMsg(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

    static LRESULT CALLBACK ListBoxSubclassProcEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ListBoxSubclassProcAction(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK HandleListBoxEvent(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK HandleListBoxAction(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static LRESULT CALLBACK DragDotProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK HandleDragDot(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static LRESULT CALLBACK DragingDotProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK HandleDragingDot(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void EventListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam);
    void ActionListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam);

    std::map<int, CNewTrigger*> GetOtherInstances();
    void RefreshOtherInstances();
    int GetCurrentInstanceIndex();
    bool IsMainInstance();

private:
    HWND m_hwnd;
    CFinalSunDlg* m_parent;
    static CINI& map;
    static CINI& fadata;
    static MultimapHelper& rules;
public:
    HWND hSelectedTrigger;
    HWND hNewTrigger;
    HWND hCloneTrigger;
    HWND hDeleteTrigger;
    HWND hPlaceOnMap;
    HWND hType;
    HWND hName;
    HWND hHouse;
    HWND hAttachedtrigger;
    HWND hDisabled;
    HWND hEasy;
    HWND hMedium;
    HWND hHard;
    HWND hEventtype;
    HWND hNewEvent;
    HWND hCloneEvent;
    HWND hDeleteEvent;
    HWND hEventDescription;
    HWND hEventList;
    HWND hEventParameter[EVENT_PARAM_COUNT];
    HWND hEventParameterDesc[EVENT_PARAM_COUNT];
    HWND hActionoptions;
    HWND hActiontype;
    HWND hNewAction;
    HWND hDeleteAction;
    HWND hCloneAction;
    HWND hActionDescription;
    HWND hActionList;
    HWND hActionframe;
    HWND hSearchReference;
    HWND hOpenNewEditor;
    HWND hDragPoint;
    HWND hCompact;
    HWND hActionMoveUp;
    HWND hActionMoveDown;
    HWND hActionParameter[ACTION_PARAM_COUNT];
    HWND hActionParameterDesc[ACTION_PARAM_COUNT];

    int CurrentCSFActionParam;
    int CurrentTriggerActionParam;
    int CurrentTeamActionParam;
    static std::vector<ParamAffectedParams> ActionParamAffectedParams;
    static std::vector<ParamAffectedParams> EventParamAffectedParams;
    bool ActionParamUsesFloat;
    bool TeamListChanged;

    static CNewTrigger Instance[TRIGGER_EDITOR_MAX_COUNT];

    FString CurrentTriggerID;
    std::shared_ptr<Trigger> CurrentTrigger;

private:
    int SelectedTriggerIndex;
    int SelectedEventIndex;
    int SelectedActionIndex;
    int ActionParamsCount;
    int LastActionParamsCount;
    bool WindowShown;
    std::map<int, FString> HouseLabels;
    std::map<int, FString> TriggerLabels;
    std::map<int, FString> AttachedTriggerLabels;
    std::map<int, FString> ActionTypeLabels;
    std::map<int, FString> EventTypeLabels;
    std::map<int, FString> EventParamLabels[EVENT_PARAM_COUNT];
    std::map<int, FString> ActionParamLabels[ACTION_PARAM_COUNT];
    std::pair<bool, int> EventParamsUsage[EVENT_PARAM_COUNT];
    std::pair<bool, int> ActionParamsUsage[ACTION_PARAM_COUNT];

    bool EventParameterAutoDrop[EVENT_PARAM_COUNT];
    bool ActionParameterAutoDrop[ACTION_PARAM_COUNT];

    bool Autodrop;
    bool DropNeedUpdate;
    bool CompactMode = false;
    WNDPROC OriginalListBoxProcEvent;
    WNDPROC OriginalListBoxProcAction;
    WNDPROC OrigDragDotProc;
    WNDPROC OrigDragingDotProc;
    RECT rectComboLBox;
    HWND hComboLBox;
    static bool AvoidInfiLoop;
    static bool SortTriggersExecuted;
    static bool AutoChangeName;

    bool   m_pressed = false;
    bool   m_dragging = false;

    POINT  m_pressPtScreen; 
    POINT  m_lastPtScreen;

    POINT m_dragOffset{};
    HWND m_hDragGhost = nullptr;
    POINT windowPos{};
};

