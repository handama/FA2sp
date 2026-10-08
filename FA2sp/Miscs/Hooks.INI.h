#pragma once
#include <CINI.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <CLoading.h>

#include "../Helpers/STDHelpers.h"
#include "../FA2sp.h"
#include <CFinalSunApp.h>
#include <CMixFile.h>
#include <fstream>
#include <queue>

using std::map;
using std::vector;

class NOVTABLE CINIExt : public CINI
{
public:
    using FnRecordKey_t = void(*)(CINI*, const ppmfc::CString&, const ppmfc::CString&);
    using FnRemoveKey_t = void(*)(CINI*, const ppmfc::CString&, const ppmfc::CString&);
    using FnRecordSection_t = void(*)(CINI*, const ppmfc::CString&);
    using FnRemoveSection_t = void(*)(CINI*, const ppmfc::CString&);

    static FnRecordKey_t FnRecordKey;
    static FnRemoveKey_t FnRemoveKey;
    static FnRecordSection_t FnRecordSection;
    static FnRemoveSection_t FnRemoveSection;

    static void SetAdaptiveSorting(bool enable);
    static void SetKeepSectionSorting(bool enable);
    static bool GetAdaptiveSorting();
    static bool GetKeepSectionSorting();

    bool WriteString(ppmfc::CString pSection, ppmfc::CString pKey, ppmfc::CString pValue);
    bool WriteString(INISection* pSection, ppmfc::CString pKey, ppmfc::CString pValue);
    bool DeleteKey(ppmfc::CString pSection, ppmfc::CString pKey);
    bool DeleteKey(INISection* pSection, ppmfc::CString pKey);
    bool DeleteSection(ppmfc::CString pSection);
    INISection* AddSection(ppmfc::CString pSectionName);
    INISection* AddOrGetSection(ppmfc::CString pSectionName);

    void LoadINIExt(uint8_t* pFile, size_t fileSize, const char* lpSection,
        bool bClear, bool bTrimSpace, bool bAllowInclude, std::vector<std::pair<ppmfc::CString, ppmfc::CString>>* parentIncludeInis = nullptr);
    void LoadFASetting(const FString& path);
    void InheritSectionRecursive(const ppmfc::CString& sectionName, std::set<ppmfc::CString>& visited);
    static bool IsLoadingFAini;

    static constexpr reference<CINIExt, 0x7ACC80> const CurrentDocument{};
};

struct CINIInfo
{
    FString Name;
    FileEncoding Encoding = Unknown;
};

class CINIManager
{
private:
    static std::unordered_map<CINI*, CINIInfo> propertyMap;
    CINIManager() = default;

public:
    static CINIManager& GetInstance() {
        static CINIManager instance;
        return instance;
    }

    void SetProperty(CINI* instance, FString Name) {
        if (instance) {
            propertyMap[instance].Name = Name;
        }
    }

    void SetProperty(CINI* instance, FileEncoding Encoding) {
        if (instance) {
            propertyMap[instance].Encoding = Encoding;
        }
    }

    CINIInfo GetProperty(CINI* instance, CINIInfo defaultValue = {}) {
        auto it = propertyMap.find(instance);
        return (it != propertyMap.end()) ? it->second : defaultValue;
    }

    void RemoveInstance(CINI* instance) {
        propertyMap.erase(instance);
    }

};

class INIIncludes
{
public:
    static int LastReadIndex;
    static vector<CINI*> LoadedINIs;
    static vector<FString> LoadedINIFiles;
    static map<FString, unsigned int> CurrentINIIdxHelper;
    static vector<char*> RulesIncludeFiles;
    static bool IsFirstINI;
    static bool IsMapINI;
    static bool MapINIWarn;
    static bool SkipBracketFix;
};