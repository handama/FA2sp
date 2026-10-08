#pragma once

#include <MFC/ppmfc_cstring.h>
#include <unordered_map>
#include <vector>
#include "SequencedKeyList.h"

class CINI;
class INISection;

struct SectionItem
{
    ppmfc::CString Name;
    INISection* Section{ nullptr };

    operator const ppmfc::CString&() const { return Name; }
    operator const char*() const { return Name.GetString(); }
};

struct KeyValueItem
{
    ppmfc::CString Key;
    ppmfc::CString Value;
};

class CINIOrderTracker
{
public:
    static void RecordSection(CINI* ini, const ppmfc::CString& section);
    static void RecordKey(CINI* ini, const ppmfc::CString& section, const ppmfc::CString& key);
    static void RemoveKey(CINI* ini, const ppmfc::CString& section, const ppmfc::CString& key);
    static void RemoveSection(CINI* ini, const ppmfc::CString& section);
    static void Clear(CINI* ini);
    static const SequencedKeyList* GetSectionOrder(CINI* ini);
    static const SequencedKeyList* GetKeyOrder(CINI* ini, const ppmfc::CString& section);
    static SequencedKeyList* GetOrCreateKeyOrder(CINI* ini, const ppmfc::CString& section);

    // Range / iteration helpers: returns items in preserved queue order (or map dictionary order if preserveOrder is false)
    static std::vector<SectionItem> GetSections(CINI* ini, bool preserveOrder = true);
    static std::vector<ppmfc::CString> GetSectionNames(CINI* ini, bool preserveOrder = true);
    static std::vector<KeyValueItem> GetEntries(CINI* ini, const ppmfc::CString& sectionName, INISection* section = nullptr, bool adaptiveSorting = true);
    static std::vector<ppmfc::CString> GetKeyNames(CINI* ini, const ppmfc::CString& sectionName, INISection* section = nullptr, bool adaptiveSorting = true);
};
