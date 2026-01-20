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

private:
    Trigger(const char* id, std::map<FString, FString>* pTagMap = nullptr)
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
        RepeatType = "-1";

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

};

// A static window class
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
        SearchReference = 1999
    };


    static void Create(CFinalSunDlg* pWnd);


    static HWND GetHandle()
    {
        return CNewTrigger::m_hwnd;
    }
    static bool OnEnterKeyDown(HWND& hWnd);
    static void OnSelchangeTrigger(bool edited = false, int eventListCur = 0, int actionListCur = 0);
    static void OnSelchangeAttachedTrigger(bool edited = false);
    static void OnSelchangeEventType(bool edited = false);
    static void OnSelchangeActionType(bool edited = false);
    static void OnSelchangeEventParam(int index, bool edited = false);
    static void OnSelchangeActionParam(int index, bool edited = false);
    static void OnClickNewTrigger();
    static void OnSelchangeEventListbox(bool changeCursel = true);
    static void OnSelchangeActionListbox(bool changeCursel = true);

protected:
    static void Initialize(HWND& hWnd);
    static void Update(HWND& hWnd);

    static void OnSeldropdownTrigger(HWND& hWnd);
    
    static void OnClickDelTrigger(HWND& hWnd);
    static void OnClickCloTrigger(HWND& hWnd);
    static void OnClickPlaceOnMap(HWND& hWnd);
    static void OnClickNewEvent(HWND& hWnd);
    static void OnClickDelEvent(HWND& hWnd);
    static void OnClickCloEvent(HWND& hWnd);
    static void OnClickNewAction(HWND& hWnd);
    static void OnClickDelAction(HWND& hWnd);
    static void OnClickCloAction(HWND& hWnd);
    static void OnClickSearchReference(HWND& hWnd);

    static void OnSelchangeHouse(bool edited = false);
    static void OnSelchangeType(bool edited = false);
    static void UpdateEventAndParam(int changedEvent = -1, bool changeCursel = true);
    static void UpdateActionAndParam(int changedAction = -1, bool changeCursel = true);
    static void AdjustActionHeight();
    static void UpdateParamAffectedParam_Action(int index);
    static void UpdateParamAffectedParam_Event(int index);

    static void OnCloseupCComboBox(HWND& hWnd, std::map<int, FString>& labels, bool isComboboxSelectOnly = false);
    static void OnDropdownCComboBox(int index);

    static void SortTriggers(FString id = "");

    static void Close(HWND& hWnd);

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ListBoxSubclassProcEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ListBoxSubclassProcAction(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static void EventListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam);
    static void ActionListBoxProc(HWND hWnd, WORD nCode, LPARAM lParam);

private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static CINI& map;
    static CINI& fadata;
    static MultimapHelper& rules;
public:
    static HWND hSelectedTrigger;
    static HWND hNewTrigger;
    static HWND hCloneTrigger;
    static HWND hDeleteTrigger;
    static HWND hPlaceOnMap;
    static HWND hType;
    static HWND hName;
    static HWND hHouse;
    static HWND hAttachedtrigger;
    static HWND hDisabled;
    static HWND hEasy;
    static HWND hMedium;
    static HWND hHard;
    static HWND hEventtype;
    static HWND hNewEvent;
    static HWND hCloneEvent;
    static HWND hDeleteEvent;
    static HWND hEventDescription;
    static HWND hEventList;
    static HWND hEventParameter[EVENT_PARAM_COUNT];
    static HWND hEventParameterDesc[EVENT_PARAM_COUNT];
    static HWND hActionoptions;
    static HWND hActiontype;
    static HWND hNewAction;
    static HWND hDeleteAction;
    static HWND hCloneAction;
    static HWND hActionDescription;
    static HWND hActionList;
    static HWND hActionframe;
    static HWND hSearchReference;
    static HWND hActionParameter[ACTION_PARAM_COUNT];
    static HWND hActionParameterDesc[ACTION_PARAM_COUNT];

    static int CurrentCSFActionParam;
    static int CurrentTriggerActionParam;
    static int CurrentTeamActionParam;
    static std::vector<ParamAffectedParams> ActionParamAffectedParams;
    static std::vector<ParamAffectedParams> EventParamAffectedParams;
    static bool ActionParamUsesFloat;
    static bool TeamListChanged;

private:
    static int SelectedTriggerIndex;
    static int SelectedEventIndex;
    static int SelectedActionIndex;
    static int ActionParamsCount;
    static int LastActionParamsCount;
    static bool WindowShown;
    static FString CurrentTriggerID;
    static std::shared_ptr<Trigger> CurrentTrigger;
    static std::map<int, FString> HouseLabels;
    static std::map<int, FString> TriggerLabels;
    static std::map<int, FString> AttachedTriggerLabels;
    static std::map<int, FString> ActionTypeLabels;
    static std::map<int, FString> EventTypeLabels;
    static std::map<int, FString> EventParamLabels[EVENT_PARAM_COUNT];
    static std::map<int, FString> ActionParamLabels[ACTION_PARAM_COUNT];
    static std::pair<bool, int> EventParamsUsage[EVENT_PARAM_COUNT];
    static std::pair<bool, int> ActionParamsUsage[ACTION_PARAM_COUNT];

    static bool EventParameterAutoDrop[EVENT_PARAM_COUNT];
    static bool ActionParameterAutoDrop[ACTION_PARAM_COUNT];

    static bool Autodrop;
    static bool DropNeedUpdate;
    static WNDPROC OriginalListBoxProcEvent;
    static WNDPROC OriginalListBoxProcAction;
    static RECT rectComboLBox;
    static HWND hComboLBox;
};

