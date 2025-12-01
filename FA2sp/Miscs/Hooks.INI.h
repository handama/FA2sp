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

using std::map;
using std::vector;

class NOVTABLE CINIExt : public CINI
{
public:
    void LoadINIExt(uint8_t* pFile, size_t fileSize, const char* lpSection, bool bClear, bool bTrimSpace, bool bAllowInclude);
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
    static std::unordered_map<FString, std::unordered_map<FString, FString>> MapIncludedKeys;
};