#include <afxwin.h>
#include "CINIOrderTracker.h"
#include <CINI.h>
#include <unordered_set>

struct INIOrderData
{
    SequencedKeyList SectionOrder;
    std::unordered_map<ppmfc::CString, SequencedKeyList> KeyOrder;
};

static std::unordered_map<CINI*, INIOrderData> g_iniOrderMap;

void CINIOrderTracker::RecordSection(CINI* ini, const ppmfc::CString& section)
{
    if (ini && !section.IsEmpty())
    {
        g_iniOrderMap[ini].SectionOrder.Add(section);
    }
}

void CINIOrderTracker::RecordKey(CINI* ini, const ppmfc::CString& section, const ppmfc::CString& key)
{
    if (ini && !section.IsEmpty() && !key.IsEmpty())
    {
        auto& data = g_iniOrderMap[ini];
        data.SectionOrder.Add(section);
        data.KeyOrder[section].Add(key);
    }
}

void CINIOrderTracker::RemoveKey(CINI* ini, const ppmfc::CString& section, const ppmfc::CString& key)
{
    if (ini)
    {
        auto it = g_iniOrderMap.find(ini);
        if (it != g_iniOrderMap.end())
        {
            auto kIt = it->second.KeyOrder.find(section);
            if (kIt != it->second.KeyOrder.end())
            {
                kIt->second.Remove(key);
            }
        }
    }
}

void CINIOrderTracker::RemoveSection(CINI* ini, const ppmfc::CString& section)
{
    if (ini)
    {
        auto it = g_iniOrderMap.find(ini);
        if (it != g_iniOrderMap.end())
        {
            it->second.SectionOrder.Remove(section);
            it->second.KeyOrder.erase(section);
        }
    }
}

void CINIOrderTracker::Clear(CINI* ini)
{
    if (ini)
    {
        auto it = g_iniOrderMap.find(ini);
        if (it != g_iniOrderMap.end())
        {
            it->second.SectionOrder.Clear();
            it->second.KeyOrder.clear();
        }
    }
}

const SequencedKeyList* CINIOrderTracker::GetSectionOrder(CINI* ini)
{
    if (ini)
    {
        auto it = g_iniOrderMap.find(ini);
        if (it != g_iniOrderMap.end() && !it->second.SectionOrder.Empty())
        {
            return &it->second.SectionOrder;
        }
    }
    return nullptr;
}

const SequencedKeyList* CINIOrderTracker::GetKeyOrder(CINI* ini, const ppmfc::CString& section)
{
    if (ini)
    {
        auto it = g_iniOrderMap.find(ini);
        if (it != g_iniOrderMap.end())
        {
            auto kIt = it->second.KeyOrder.find(section);
            if (kIt != it->second.KeyOrder.end() && !kIt->second.Empty())
            {
                return &kIt->second;
            }
        }
    }
    return nullptr;
}

SequencedKeyList* CINIOrderTracker::GetOrCreateKeyOrder(CINI* ini, const ppmfc::CString& section)
{
    if (ini && !section.IsEmpty())
    {
        auto& data = g_iniOrderMap[ini];
        data.SectionOrder.Add(section);
        return &data.KeyOrder[section];
    }
    return nullptr;
}

std::vector<SectionItem> CINIOrderTracker::GetSections(CINI* ini, bool preserveOrder)
{
    std::vector<SectionItem> result;
    if (!ini)
        return result;

    result.reserve(ini->Dict.size());

    if (!preserveOrder)
    {
        for (auto& pair : ini->Dict)
        {
            result.push_back({ pair.first, &pair.second });
        }
        return result;
    }

    std::unordered_set<ppmfc::CString> emitted;
    emitted.reserve(ini->Dict.size());

    // Pass 1: emit tracked sections in queued order that still exist in Dict
    const auto* pSectionOrder = GetSectionOrder(ini);
    if (pSectionOrder)
    {
        for (const auto& secName : *pSectionOrder)
        {
            auto it = ini->Dict.find(secName);
            if (it != ini->Dict.end())
            {
                result.push_back({ it->first, &it->second });
                emitted.insert(it->first);
            }
        }
    }

    // Pass 2: emit untracked sections in Dict order and record them into tracker
    for (auto& pair : ini->Dict)
    {
        if (emitted.find(pair.first) == emitted.end())
        {
            RecordSection(ini, pair.first);
            result.push_back({ pair.first, &pair.second });
            emitted.insert(pair.first);
        }
    }

    return result;
}

std::vector<ppmfc::CString> CINIOrderTracker::GetSectionNames(CINI* ini, bool preserveOrder)
{
    auto sections = GetSections(ini, preserveOrder);
    std::vector<ppmfc::CString> names;
    names.reserve(sections.size());
    for (auto& s : sections)
    {
        names.push_back(std::move(s.Name));
    }
    return names;
}

std::vector<KeyValueItem> CINIOrderTracker::GetEntries(CINI* ini, const ppmfc::CString& sectionName, INISection* section, bool preserveOrder)
{
    std::vector<KeyValueItem> result;
    if (!section)
    {
        if (!ini)
            return result;
        section = ini->GetSection(sectionName);
        if (!section)
            return result;
    }

    auto& entities = section->GetEntities();
    result.reserve(entities.size());

    if (!preserveOrder || !ini)
    {
        for (const auto& pair : entities)
        {
            result.push_back({ pair.first, pair.second });
        }
        return result;
    }

    std::unordered_set<ppmfc::CString> emitted;
    emitted.reserve(entities.size());

    // Pass 1: emit tracked keys in queued order that still exist in entities
    const auto* pKeyOrder = GetKeyOrder(ini, sectionName);
    if (pKeyOrder)
    {
        for (const auto& key : *pKeyOrder)
        {
            auto it = entities.find(key);
            if (it != entities.end())
            {
                result.push_back({ it->first, it->second });
                emitted.insert(it->first);
            }
        }
    }

    // Pass 2: emit untracked keys in entities order and record them into tracker
    for (const auto& pair : entities)
    {
        if (emitted.find(pair.first) == emitted.end())
        {
            RecordKey(ini, sectionName, pair.first);
            result.push_back({ pair.first, pair.second });
            emitted.insert(pair.first);
        }
    }

    return result;
}

std::vector<ppmfc::CString> CINIOrderTracker::GetKeyNames(CINI* ini, const ppmfc::CString& sectionName, INISection* section, bool preserveOrder)
{
    auto entries = GetEntries(ini, sectionName, section, preserveOrder);
    std::vector<ppmfc::CString> keys;
    keys.reserve(entries.size());
    for (auto& e : entries)
    {
        keys.push_back(std::move(e.Key));
    }
    return keys;
}
