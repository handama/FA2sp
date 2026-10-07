#include <afxwin.h>
#include "CINIOrderTracker.h"

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
