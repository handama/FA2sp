#pragma once

#include <CINI.h>

#include <vector>
#include <map>

class MultimapHelper
{
public:
    MultimapHelper() = default;
    ~MultimapHelper() = default;
    MultimapHelper(std::initializer_list<CINI*> list);

    void AddINI(CINI* pINI);
    void Clear();

    CINI* GetINIAt(int idx);
    std::vector<CINI*> GetINIData();

    ppmfc::CString* TryGetString(ppmfc::CString pSection, ppmfc::CString pKey);
    bool KeyExists(ppmfc::CString pSection, ppmfc::CString pKey);
    int GetInteger(ppmfc::CString pSection, ppmfc::CString pKey, int nDefault = 0);
    float GetSingle(ppmfc::CString pSection, ppmfc::CString pKey, float nDefault = 0.0f);
    ppmfc::CString GetString(ppmfc::CString pSection, ppmfc::CString pKey, ppmfc::CString pDefault = "");
    bool GetBool(ppmfc::CString pSection, ppmfc::CString pKey, bool nDefault = false);
    ppmfc::CString GetValueAt(ppmfc::CString section, int index, ppmfc::CString Default = "");
    ppmfc::CString GetKeyAt(ppmfc::CString section, int index, ppmfc::CString Default = "");
    void ClearMap(ppmfc::CString section = "");

    std::vector<ppmfc::CString> ParseIndicies(ppmfc::CString pSection, bool bParseIntoValue = false);
    std::map<ppmfc::CString, ppmfc::CString, INISectionEntriesComparator> GetSection(ppmfc::CString pSection);
    std::map<ppmfc::CString, ppmfc::CString, INISectionEntriesComparator> GetUnorderedUnionSection(ppmfc::CString pSection);
    std::vector < std::pair<ppmfc::CString, ppmfc::CString>> GetUnorderedSection(ppmfc::CString pSection);
    std::map<ppmfc::CString, std::vector<ppmfc::CString>, INISectionEntriesComparator> GetSectionSplitValues(ppmfc::CString pSection, const char* pSplit);

private:
    std::vector<CINI*> data;
    std::map<ppmfc::CString, std::vector<ppmfc::CString>> KeyIndiciesMap;
    std::map<ppmfc::CString, std::vector<ppmfc::CString>> ValueIndiciesMap;
};