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

class AITrigger
{
public:
    FString ID;
    FString Name;
    FString Team1;
    FString House;
    FString TechLevel;
    FString ConditionType;
    FString ComparisonObject;
    int Comparator[8];
    double InitialWeight;
    double MinWeight;
    double MaxWeight;
    bool IsForSkirmish;
    FString unused;
    FString Side;
    bool IsBaseDefense;
    FString Team2;
    bool EnabledInE;
    bool EnabledInM;
    bool EnabledInH;

    bool Enabled;

    static std::unique_ptr<AITrigger> create(const char* id)
    {
        auto atoms = STDHelpers::SplitString(CINI::CurrentDocument().GetString("AITriggerTypes", id));
        if (atoms.size() < 18)
            return nullptr;
        return std::make_unique<AITrigger>(id);
    }
    AITrigger()
        : InitialWeight(0.0), MinWeight(0.0), MaxWeight(0.0),
        IsForSkirmish(false), IsBaseDefense(false),
        EnabledInE(false), EnabledInM(false), EnabledInH(false),
        Enabled(false) {
        for (int i = 0; i < 8; ++i) {
            Comparator[i] = 0;
        }
    }
    AITrigger(const char* id)
    {
        auto atoms = FString::SplitString(CINI::CurrentDocument().GetString("AITriggerTypes", id), 17);
        ID = id;
        Name = atoms[0];
        Team1 = atoms[1];
        House = atoms[2];
        TechLevel = atoms[3];
        ConditionType = atoms[4];
        ComparisonObject = atoms[5];
        for (int i = 0; i < 8; i++) {
            Comparator[i] = ReadComparator(atoms[6], i);
        }
        InitialWeight = std::atof(atoms[7]);
        MinWeight = std::atof(atoms[8]);
        MaxWeight = std::atof(atoms[9]);
        IsForSkirmish = atoms[10] == "0" ? false : true;
        unused = atoms[11];
        Side = atoms[12];
        IsBaseDefense = atoms[13] == "0" ? false : true;
        Team2 = atoms[14];
        EnabledInE = atoms[15] == "0" ? false : true;
        EnabledInM = atoms[16] == "0" ? false : true;
        EnabledInH = atoms[17] == "0" ? false : true;

        Enabled = CINI::CurrentDocument().GetBool("AITriggerTypesEnable", id);
    }

    void Save() const
    {
        FString value;
        FString comparator = "";
        for (int i = 0; i < 8; i++) {
            comparator += SaveComparator(Comparator[i]);
        }
        std::ostringstream oss;
        oss.precision(6);
        oss << std::fixed << InitialWeight;
        std::string initial = oss.str();
        oss.str("");
        oss.precision(6);
        oss << std::fixed << MinWeight;
        std::string min = oss.str();
        oss.str("");
        oss.precision(6);
        oss << std::fixed << MaxWeight;
        std::string max = oss.str();
        oss.str("");

        value.Format("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
            Name.c_str(),
            Team1.c_str(),
            House.c_str(),
            TechLevel.c_str(),
            ConditionType.c_str(),
            ComparisonObject.c_str(),
            comparator.c_str(),
            initial.c_str(),
            min.c_str(),
            max.c_str(),
            IsForSkirmish ? "1" : "0",
            unused.c_str(),
            Side.c_str(),
            IsBaseDefense ? "1" : "0",
            Team2.c_str(),
            EnabledInE ? "1" : "0",
            EnabledInM ? "1" : "0",
            EnabledInH ? "1" : "0"
        );
        if (Enabled) {
            CINI::CurrentDocument->WriteBool("AITriggerTypesEnable", ID, Enabled);
        }
        else {
            CINI::CurrentDocument->DeleteKey("AITriggerTypesEnable", ID);
        }
        CINI::CurrentDocument->WriteString("AITriggerTypes", ID, value);
    }

private:
    static int ReadComparator(FString text, int index)
    {
        int num = 0;
        if (text.GetLength() != 64) return num;
        if (index < 0 || index > 7) return num;

        auto thisText = text.Mid(8 * index, 8);
        unsigned char bytes[4]{ 0 };
        for (int i = 0; i < 4; ++i) {
            bytes[i] = static_cast<unsigned char>(strtol(thisText.Mid(2 * i, 2), NULL, 16));
            
        }
        num |= (bytes[0] << 0); 
        num |= (bytes[1] << 8); 
        num |= (bytes[2] << 16);
        num |= (bytes[3] << 24);

        return num;
    }
    static FString SaveComparator(int comparator)
    {
        FString ret = "";

        std::stringstream ss;
        ss << std::hex << comparator;
        FString bigEndian = ss.str();
        while (bigEndian.GetLength() < 8) {
            bigEndian = "0" + bigEndian;
        }

        for (int i = 3; i >= 0; --i) {
            ret += bigEndian.Mid(2 * i, 2);
        }
        return ret;
    }

};

// A static window class
class CNewAITrigger
{
public:
    enum Controls {
        SelectedAITrigger = 1002,
        Enabled = 1003,
        Add = 1004,
        Clone = 1005,
        Delete = 1006,
        Name = 1008,
        Side = 1010,
        Country = 1012,
        ConditionType = 1015,
        Comparator = 1017,
        Amount = 1019,
        ComparisonObject = 1021,
        Team1 = 1024,
        Team2 = 1026,
        InitialWeight = 1029,
        MinWeight = 1031,
        MaxWeight = 1033,
        Easy = 1035,
        Medium = 1036,
        Hard = 1037,
        BaseDefense = 1038,
        Skrimish = 1039,

    };

    static void Create(CFinalSunDlg* pWnd);
    

    static HWND GetHandle()
    {
        return CNewAITrigger::m_hwnd;
    }
    static bool OnEnterKeyDown(HWND& hWnd);
    static void OnSelchangeAITrigger(bool edited = false, int specificIdx = -1);
    static bool TeamListChanged;
    static void OnSelchangeTeam(int index, bool edited = false);
    static void OnDropdownTeam();

protected:
    static void Initialize(HWND& hWnd);
    static void Update(HWND& hWnd);

    static void OnSeldropdownAITrigger(HWND& hWnd);
    static void OnClickNewAITrigger();
    static void OnClickDelAITrigger();
    static void OnClickCloAITrigger();

    static void OnSelchangeConditionType();
    static void OnSelchangeComparator();
    static void OnSelchangeComparisonObject(bool edited = false);
    static void OnSelchangeCountry(bool edited = false);
    static void OnSelchangeSide(bool edited = false);

    static void OnCloseupCComboBox(HWND& hWnd, std::map<int, FString>& labels, bool isComboboxSelectOnly = false);
    static void SortAITriggers(FString id);

    static void Close(HWND& hWnd);

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static double safe_stod(const char* s);

private:
    static HWND m_hwnd;
    static CFinalSunDlg* m_parent;
    static CINI& map;
    static CINI& fadata;
    static MultimapHelper& rules;

    static HWND hSelectedAITrigger;
    static HWND hEnabled;
    static HWND hAdd;
    static HWND hClone;
    static HWND hDelete;
    static HWND hName;
    static HWND hSide;
    static HWND hCountry;
    static HWND hConditionType;
    static HWND hComparator;
    static HWND hAmount;
    static HWND hComparisonObject;
    static HWND hTeam1;
    static HWND hTeam2;
    static HWND hInitialWeight;
    static HWND hMinWeight;
    static HWND hMaxWeight;
    static HWND hEasy;
    static HWND hMedium;
    static HWND hHard;
    static HWND hBaseDefense;
    static HWND hSkrimish;

    static int SelectedAITriggerIndex;
    static std::unique_ptr<AITrigger> CurrentAITrigger;
    static std::map<int, FString> AITriggerLabels;
    static std::map<int, FString> TeamLabels[2];
    static std::map<int, FString> ComparisonObjectLabels;
    static std::map<int, FString> CountryLabels;
    static bool Autodrop;
    static bool DropNeedUpdate;
};