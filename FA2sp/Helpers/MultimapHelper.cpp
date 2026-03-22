/*
* NOTICE
* Implement of this class could be really composite and
* hard to understand. Not only to other users, but for me
* as well, so why not just use it as it's easy to call 
* rather than understand how it works, isn't it?
* 
* UPDATE ON 3/31/2021
* "I cannot understand this one either now." - secsome
*/

#include "MultimapHelper.h"
#include "STDHelpers.h"

#include <set>

MultimapHelper::MultimapHelper(std::initializer_list<CINI*> list)
{
    for (auto pINI : list)
        data.push_back(pINI);
}

void MultimapHelper::AddINI(CINI* pINI)
{
    data.push_back(pINI);
}

void MultimapHelper::Clear()
{
    data.clear();
}

CINI* MultimapHelper::GetINIAt(int idx)
{
    return data.at(idx);
}
std::vector<CINI*>  MultimapHelper::GetINIData()
{
    return data;
}

ppmfc::CString* MultimapHelper::TryGetString(ppmfc::CString pSection, ppmfc::CString pKey)
{
    for (auto ritr = data.rbegin(); ritr != data.rend(); ++ritr)
    {
        if (auto pRet = (*ritr)->TryGetString(pSection, pKey))
            if (!pRet->IsEmpty())
                return pRet;
    }
    return nullptr;
}

bool MultimapHelper::KeyExists(ppmfc::CString pSection, ppmfc::CString pKey)
{
    for (auto ritr = data.rbegin(); ritr != data.rend(); ++ritr)
    {
        if ((*ritr)->KeyExists(pSection, pKey))
            return true;
    }
    return false;
}

ppmfc::CString MultimapHelper::GetString(ppmfc::CString pSection, ppmfc::CString pKey, ppmfc::CString pDefault)
{
    auto const pResult = TryGetString(pSection, pKey);
    return pResult ? *pResult : pDefault;
}

int MultimapHelper::GetInteger(ppmfc::CString pSection, ppmfc::CString pKey, int nDefault) {
    ppmfc::CString&& pStr = this->GetString(pSection, pKey, "");
    int ret = 0;
    if (sscanf_s(pStr, "%d", &ret) == 1)
        return ret;
    return nDefault;
}

float MultimapHelper::GetSingle(ppmfc::CString pSection, ppmfc::CString pKey, float nDefault)
{
    ppmfc::CString&& pStr = this->GetString(pSection, pKey, "");
    float ret = 0.0f;
    if (sscanf_s(pStr, "%f", &ret) == 1)
    {
        if (strchr(pStr, '%%'))
            ret *= 0.01f;
        return ret;
    }
    return nDefault;
}

bool MultimapHelper::GetBool(ppmfc::CString pSection, ppmfc::CString pKey, bool nDefault) {
    ppmfc::CString&& pStr = this->GetString(pSection, pKey, "");
    switch (toupper(static_cast<unsigned char>(*pStr)))
    {
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'T':
    case 'Y':
        return true;
    case '0':
    case 'F':
    case 'N':
        return false;
    default:
        return nDefault;
    }
}

std::set<ppmfc::CString> MultimapHelper::GetUniqueValues(ppmfc::CString pSection)
{
    std::set<ppmfc::CString> ret;
    if (UniqueValuesMap.find(pSection) != UniqueValuesMap.end())
    {
        return UniqueValuesMap[pSection];
    }
    int index = 0;
    ppmfc::CString tmp;
    for (auto& pINI : data)
        if (pINI)
            if (auto section = pINI->GetSection(pSection))
            {
                for (auto& pair : section->GetEntities())
                {
                    ret.insert(pair.second);
                }
            }
    UniqueValuesMap[pSection] = ret;
    return ret;
}

std::vector<ppmfc::CString> MultimapHelper::ParseIndicies(ppmfc::CString pSection, bool bParseIntoValue)
{
    if (bParseIntoValue && ValueIndiciesMap.find(pSection) != ValueIndiciesMap.end())
    {
        return ValueIndiciesMap[pSection];
    }
    else if (!bParseIntoValue && KeyIndiciesMap.find(pSection) != KeyIndiciesMap.end())
    {
        return KeyIndiciesMap[pSection];
    }

    std::vector<ppmfc::CString> ret;
    std::map<unsigned int, ppmfc::CString> tmp;
    std::map<ppmfc::CString, unsigned int> tmp2;
    std::map<ppmfc::CString, ppmfc::CString> tmp3; // Value - Key
    std::map<unsigned int, CINI*> belonging;

    for (auto& pINI : data)
    {
        if (pINI)
        {
            auto&& cur = pINI->ParseIndiciesData(pSection);
            for (auto& pair : cur)
            {
                ppmfc::CString value = pINI->GetString(pSection, pair.second, pair.second);
                auto&& unitr = tmp2.find(value);
                if (unitr == tmp2.end())
                {
                    belonging[tmp2.size()] = pINI;
                    tmp2[value] = tmp2.size();
                }
                else
                {
                    belonging[unitr->second] = pINI;
                }
                tmp3[value] = pair.second;
            }
        }
    }
    
    for (auto& pair : tmp2)
        tmp[pair.second] = pair.first;

    ret.resize(tmp.size());
    size_t idx = 0;
    for (auto& x : tmp)
        ret[idx++] = x.second;

    if (!bParseIntoValue)
        for (size_t i = 0, sz = ret.size(); i < sz; ++i)
            ret[i] = tmp3[ret[i]];

    std::vector<ppmfc::CString> tmp4;

    for (size_t i = 0, sz = ret.size(); i < sz; ++i)
    {
        if (ret[i] == "")
            continue;
        tmp4.push_back(ret[i]);
    }

    if (bParseIntoValue)
    {
        ValueIndiciesMap[pSection] = tmp4;
    }
    else
    {
        KeyIndiciesMap[pSection] = tmp4;
    }

    return tmp4;
}

ppmfc::CString MultimapHelper::GetValueAt(ppmfc::CString section, int index, ppmfc::CString Default)
{
    if (ValueIndiciesMap.find(section) != ValueIndiciesMap.end())
    {
        auto&& indicies = ValueIndiciesMap[section];
        return index < indicies.size() ? indicies[index] : Default;
    }
    auto&& indicies = ParseIndicies(section, true);
    return index < indicies.size() ? indicies[index] : Default;
}

ppmfc::CString MultimapHelper::GetKeyAt(ppmfc::CString section, int index, ppmfc::CString Default)
{
    if (KeyIndiciesMap.find(section) != KeyIndiciesMap.end())
    {
        auto&& indicies = KeyIndiciesMap[section];
        return index < indicies.size() ? indicies[index] : Default;
    }
    auto&& indicies = ParseIndicies(section, false);
    return index < indicies.size() ? indicies[index] : Default;
}

void MultimapHelper::ClearMap(ppmfc::CString section)
{
    if (section == "")
    {
        KeyIndiciesMap.clear();
        ValueIndiciesMap.clear();
        UniqueValuesMap.clear();
    }
    else
    {
        KeyIndiciesMap.erase(section);
        ValueIndiciesMap.erase(section);
        UniqueValuesMap.erase(section);
    }
}

std::map<ppmfc::CString, ppmfc::CString, INISectionEntriesComparator> MultimapHelper::GetSection(ppmfc::CString pSection)
{
    std::map<ppmfc::CString, ppmfc::CString, INISectionEntriesComparator> ret;
    int index = 0;
    ppmfc::CString tmp;
    for (auto& pINI : data)
        if (pINI)
            if (auto section = pINI->GetSection(pSection))
            {
                for (auto& pair : section->GetEntities())
                {
                    if (STDHelpers::IsNoneOrEmpty(pair.first) ||
                        STDHelpers::IsNoneOrEmpty(pair.second))
                    {
                        ++index;
                        continue;
                    }
                    tmp.Format("%d", index++);
                    ret[tmp] = pair.second;
                }
            }         
    return ret;
}

std::map<ppmfc::CString, ppmfc::CString, INISectionEntriesComparator> MultimapHelper::GetUnorderedUnionSection(ppmfc::CString pSection)
{
    std::map<ppmfc::CString, ppmfc::CString, INISectionEntriesComparator> ret;
    ppmfc::CString tmp;
    for (auto& pINI : data)
        if (pINI)
            if (auto section = pINI->GetSection(pSection))
            {
                for (auto& pair : section->GetEntities())
                {
                    if (STDHelpers::IsNoneOrEmpty(pair.first) ||
                        STDHelpers::IsNoneOrEmpty(pair.second))
                    {
                        continue;
                    }
                    ret[pair.first] = pair.second;
                }
            }         
    return ret;
}

std::vector <std::pair<ppmfc::CString, ppmfc::CString>> MultimapHelper::GetUnorderedSection(ppmfc::CString pSection)
{
    std::vector<std::pair<ppmfc::CString, ppmfc::CString>> ret;

    for (auto& pINI : data)
        if (pINI)
            if (auto section = pINI->GetSection(pSection))
            {
                auto& entry = section->GetEntities();
                for (std::pair<ppmfc::CString, ppmfc::CString> ol : entry)
                {
                    ret.push_back(ol);
                }
            }         
    return ret;
}

std::map<ppmfc::CString, std::vector<ppmfc::CString>, INISectionEntriesComparator> MultimapHelper::GetSectionSplitValues(ppmfc::CString pSection, const char* pSplit = ",")
{
    std::map<ppmfc::CString, std::vector<ppmfc::CString>, INISectionEntriesComparator> ret;
    ppmfc::CString tmp;
    for (auto& pINI : data)
        if (pINI)
            if (auto section = pINI->GetSection(pSection))
                for (auto& pair : section->GetEntities())
                    if (!STDHelpers::IsNoneOrEmpty(pair.first) &&
                        !STDHelpers::IsNoneOrEmpty(pair.second) &&
                        pair.first != "Name")
                    {
	                    tmp.Format("%s", pair.first);
	                    ret[tmp] = STDHelpers::SplitStringTrimmed(pair.second, pSplit);
                    }
    return ret;
}
