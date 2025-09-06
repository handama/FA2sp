#include "Hooks.INI.h"
#include "../Ext/CMapData/Body.h"
#include "../Ext/CLoading/Body.h"
#include <queue>

using std::map;
using std::vector;

int INIIncludes::LastReadIndex = -1;
bool INIIncludes::IsFirstINI = true;
bool INIIncludes::IsMapINI = false;
bool INIIncludes::MapINIWarn = false;
vector<CINI*> INIIncludes::LoadedINIs;
vector<FString> INIIncludes::LoadedINIFiles;
vector<char*> INIIncludes::RulesIncludeFiles;
map<FString, unsigned int> INIIncludes::CurrentINIIdxHelper;
std::unordered_map<FString, std::unordered_map<FString, FString>> INIIncludes::MapIncludedKeys;
std::unordered_map<CINI*, CINIInfo> CINIManager::propertyMap;
bool INIIncludes::SkipBracketFix = false;

void CINIExt::LoadINIExt(uint8_t* pFile, size_t fileSize, const char* lpSection, bool bClear, bool bTrimSpace, bool bAllowInclude)
{
    if (bClear)
    {
        auto itr = Dict.end();
        for (size_t i = 0, sz = Dict.size(); i < sz && itr != Dict.begin(); ++i) {
            --itr;
            itr->second.~INISection();
            Dict.manual_erase(itr);
        }
    }

    auto writeString = [](INISection* pSection, const FString& key, const FString& value)
    {
        std::pair<ppmfc::CString, ppmfc::CString> ins = std::make_pair(key, value);
        std::pair<INIStringDict::iterator, bool> ret;
        reinterpret_cast<FAINIEntriesMap*>(&pSection->GetEntities())->insert(&ret, &ins);
        if (!ret.second)
            new(&ret.first->second) ppmfc::CString(value);
    };

    FString content(reinterpret_cast<char*>(pFile), fileSize);
    INISection* pCurrentSection = nullptr;

    size_t idx = 0;
    const size_t len = content.length();
    static size_t plusEqual = 0;
    if (bAllowInclude)
        plusEqual = 0;
    bool findTargetSection = false;

    while (idx < len) {
        size_t lineEnd = content.find_first_of("\r\n", idx);
        if (lineEnd == FString::npos) lineEnd = len;
        FString line = content.substr(idx, lineEnd - idx);
        FString::TrimSemicolon(line);

        idx = lineEnd;
        while (idx < len && (content[idx] == '\r' || content[idx] == '\n')) ++idx;

        if (line.empty()) continue;
        if (line[0] == ';') continue;

        // ------------------- Section -------------------
        if (line[0] == '[') {
            size_t closePos = line.find(']');
            if (closePos == FString::npos) {
                if (INIIncludes::SkipBracketFix) exit(1);
                continue;
            }

            FString sectionName = line.substr(1, closePos - 1);
            FString sectionInherit;

            if (closePos + 1 < line.size() && line[closePos + 1] == ':') {
                size_t p = closePos + 2;
                if (p < line.size() && line[p] == '[') {
                    size_t close2 = line.find(']', p);
                    if (close2 != FString::npos) {
                        sectionInherit = line.substr(p + 1, close2 - (p + 1));
                    }
                }
            }

            if (!sectionName.empty()) {
                if (lpSection && sectionName != lpSection) {
                    pCurrentSection = nullptr;
                    continue;
                }
                else if (lpSection && sectionName == lpSection) {
                    findTargetSection = true;
                }
                else if (lpSection && findTargetSection) {
                    return;
                }
                pCurrentSection = AddOrGetSection(sectionName);

                // ares mode
                if (ExtConfigs::AllowInherits && !ExtConfigs::InheritType && !sectionInherit.empty()) {

                    if (auto pSectionInherit = GetSection(sectionInherit)) {
                        for (const auto& [key, value] : pSectionInherit->GetEntities())
                        {
                            writeString(pCurrentSection, key, value);
                        }
                    }
                }
                if (CMapDataExt::IsLoadingMapFile)
                {
                    CMapDataExt::MapIniSectionSorting.push_back(sectionName);
                }
            }
            continue;
        }

        // -------------------  Key=Value -------------------
        if (pCurrentSection) {
            size_t eqPos = line.find('=');
            if (eqPos == FString::npos) continue;

            FString key = line.substr(0, eqPos);
            FString value = line.substr(eqPos + 1);

            if (bTrimSpace) {
                key.Trim();
                value.Trim();
            }

            if (ExtConfigs::AllowPlusEqual) {
                if (key == "+") {
                    while (true)
                    {
                        key.Format("FA2sp%u", plusEqual);
                        ++plusEqual;
                        if (pCurrentSection->GetEntities().find(key) == pCurrentSection->GetEntities().end())
                            break;
                    }
                }
            }

            if (!key.empty()) {
                size_t currentIndex = pCurrentSection->GetEntities().size();
                writeString(pCurrentSection, key, value);

                std::pair<ppmfc::CString, int> ins =
                    std::make_pair((ppmfc::CString)key, (int)currentIndex);
                std::pair<INIIndiceDict::iterator, bool> ret;
                reinterpret_cast<FAINIIndicesMap*>(&pCurrentSection->GetIndices())->insert(&ret, &ins);
            }
        }
        else {
            continue;
        }
    }

    if (ExtConfigs::AllowIncludes && bAllowInclude)
    {
        using INIPair = std::pair<ppmfc::CString, ppmfc::CString>;
        const char* includeSection = ExtConfigs::IncludeType ? "$Include" : "#include";

        if (auto pSection = GetSection(includeSection)) {
            if (this == &CINI::CurrentDocument) {
                INIIncludes::MapIncludedKeys.clear();
                INIIncludes::MapINIWarn = true;
            }
            std::set<ppmfc::CString> includedInis;
            std::queue<ppmfc::CString> currentIncludeInis;
            for (auto& [index, key] : ParseIndiciesData(includeSection)) {
                currentIncludeInis.push(pSection->GetString(key));
            }
            while (!currentIncludeInis.empty()) {
                std::queue<ppmfc::CString> nextIncludeInis;
                while (!currentIncludeInis.empty()) {
                    ppmfc::CString includeFile = currentIncludeInis.front();
                    currentIncludeInis.pop();

                    if (!includeFile.IsEmpty()) {
                        if (includedInis.find(includeFile) == includedInis.end()) {
                            includedInis.insert(includeFile);
                            Logger::Debug("Include Ext Loaded File: %s\n", includeFile);

                            DWORD dwSize = 0;
                            auto pLoading = CLoadingExt::GetExtension();
                            CINIExt ini;
                            if (auto pBuffer = static_cast<byte*>(pLoading->ReadWholeFile(includeFile, &dwSize))) {
                                ini.LoadINIExt(pBuffer, dwSize, nullptr, true, true, false);
                            }

                            for (auto& [sectionName, pSection] : ini.Dict) {
                                auto pTargetSection = AddOrGetSection(sectionName);
                                std::vector<INIPair> targetIndicies;
                                for (const auto& [index, key] : ParseIndiciesData(sectionName)) {
                                    targetIndicies.push_back(std::make_pair(key, GetString(pTargetSection, key)));
                                }
                                for (const auto& [index, key] : ini.ParseIndiciesData(sectionName))
                                {
                                    if (key.IsEmpty()) continue;
                                    auto value = ini.GetString(&pSection, key);
                                    if (sectionName == includeSection) {
                                        nextIncludeInis.push(value);
                                    }
                                    // the include of Ares will delete the same key in registries
                                    // and then add it to the bottom
                                    // it will ignore empty values
                                    targetIndicies.erase(
                                        std::remove_if(targetIndicies.begin(), targetIndicies.end(),
                                            [&key](const INIPair& item) {
                                        return item.first == key;
                                    }),
                                        targetIndicies.end()
                                    );
                                    targetIndicies.push_back(std::make_pair(key, value));
                                    if (this == &CINI::CurrentDocument) {
                                        INIIncludes::MapIncludedKeys[sectionName][key] = GetString(pTargetSection, key);
                                    }
                                }
                                DeleteSection(sectionName);
                                pTargetSection = AddSection(sectionName);
                                int index = 0;
                                for (const auto& [key, value] : targetIndicies)
                                {
                                    writeString(pTargetSection, key, value);
                                    std::pair<ppmfc::CString, int> ins =
                                        std::make_pair((ppmfc::CString)key, index++);
                                    std::pair<INIIndiceDict::iterator, bool> ret;
                                    reinterpret_cast<FAINIIndicesMap*>(&pTargetSection->GetIndices())->insert(&ret, &ins);
                                }
                            }


                        }
                    }
                }
                currentIncludeInis = std::move(nextIncludeInis);
            }
        }
    }

    // phobos mode
    if (ExtConfigs::AllowInherits && ExtConfigs::InheritType) {
        for (auto& sectionA : Dict) {
            if (KeyExists(sectionA.first, "$Inherits"))  {
                auto inherits = STDHelpers::SplitString(GetString(sectionA.first, "$Inherits"));
                for (const auto& sectionB : inherits) {
                    if (auto pSectionB = GetSection(sectionB)) {
                        for (const auto& [key, value] : pSectionB->GetEntities()) {
                            if (!KeyExists(sectionA.first, key)) {
                                writeString(&sectionA.second, key, value);
                            }
                        }
                    }
                }
            }
        }
    }
}

// return values:
// 0 for success
// 1 for path not available
// 2 for fail to read file
DEFINE_HOOK(452CC0, CINI_ParseINI, 8)
{
    GET(CINIExt*, pThis, ECX);
    GET_STACK(const char*, lpPath, 0x4);
    GET_STACK(const char*, lpSection, 0x8);
    GET_STACK(int, bTrimSpace, 0xC);

    if (!lpPath) {
        R->EAX(1);
        return 0x4534C2;
    }
    std::ifstream file(lpPath, std::ios::binary | std::ios::ate);
    if (!file) {
        R->EAX(2);
        return 0x4534C2;
    }
    std::streampos pos = file.tellg();
    if (pos == -1) {
        R->EAX(2);
        return 0x4534C2;
    }
    size_t fileSize = static_cast<size_t>(pos);
    auto buffer = std::make_unique<uint8_t[]>(fileSize);
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(buffer.get()), fileSize)) {
        file.close();
        R->EAX(2);
        return 0x4534C2;
    }
    file.close();

    pThis->LoadINIExt(buffer.get(), fileSize, lpSection, false, /*bTrimSpace*/true, true);
 
    R->EAX(0);
    return 0x4534C2;
}

// some maps use '[' for encryption
DEFINE_HOOK(49D64F, CMapData_LoadMap_SkipBracketFix, 6)
{
    INIIncludes::SkipBracketFix = false;	
    CMapDataExt::IsLoadingMapFile = false;
    return 0;
}

DEFINE_HOOK(47FFB0, CLoading_LoadTSINI, 7)
{
    GET(CLoadingExt*, pThis, ECX);
    GET_STACK(const char*, pFile, 0x4);
    GET_STACK(CINIExt*, pINI, 0x8);
    GET_STACK(BOOL, bMerge, 0xC);

    CINIManager::GetInstance().SetProperty(pINI, { pFile });
    DWORD dwSize = 0;
    if (auto pBuffer = static_cast<byte*>(pThis->ReadWholeFile(pFile, &dwSize))) {
        pINI->LoadINIExt(pBuffer, dwSize, nullptr, !bMerge, true, true);
        GameDeleteArray(pBuffer, dwSize);
    }

    if ((pINI == &CINI::Temperate
       || pINI == &CINI::Snow
       || pINI == &CINI::Urban
       || pINI == &CINI::NewUrban
       || pINI == &CINI::Lunar
       || pINI == &CINI::Desert
       || pINI == &CINI::Rules
       || pINI == &CINI::Art
       || pINI == &CINI::Sound
       || pINI == &CINI::Eva
       || pINI == &CINI::Theme
       || pINI == &CINI::Ai)
        && FString(pFile).find("fa2extra_") == FString::npos) {
        FString extraFile = "fa2extra_";
        auto paths = FString::SplitString(pFile, "\\");
        extraFile += paths.back();
        Logger::Debug("Include Ext Loaded Extra File: %s\n", extraFile);
        DWORD dwSize = 0;
        if (auto pBuffer = static_cast<byte*>(pThis->ReadWholeFile(extraFile, &dwSize))) {
            CINIExt ini;
            ini.LoadINIExt(pBuffer, dwSize, nullptr, !bMerge, true, true);
            GameDeleteArray(pBuffer, dwSize);
            for (auto& [sectionName, pSourceSection] : ini.Dict) {
                auto pTargetSection = pINI->AddOrGetSection(sectionName);
                for (auto& [key, value] : pSourceSection.GetEntities()) {
                    pINI->WriteString(pTargetSection, key, value);
                }
            }
        }
    }

    int theaterIniType = -1;
    if (pINI == &CINI::Temperate) {
        theaterIniType = 0;
    }
    else if (pINI == &CINI::Snow) {
        theaterIniType = 1;
    }
    else if (pINI == &CINI::Urban) {
        theaterIniType = 2;
    }
    else if (pINI == &CINI::NewUrban) {
        theaterIniType = 3;
    }
    else if (pINI == &CINI::Lunar) {
        theaterIniType = 4;
    }
    else if (pINI == &CINI::Desert) {
        theaterIniType = 5;
    }
    if (theaterIniType >= 0) {
        FString sName = "";
        for (int index = 0; index < 10000; index++) {
            sName.Format("TileSet%04d", index);
            if (pINI->SectionExists(sName)) {
                auto setName = pINI->GetString(sName, "SetName");
                auto& set = CMapDataExt::TileSetOriginSetNames[theaterIniType];
                if (set.find(index) == set.end())
                {
                    set[index] = setName;
                }
            }
            else break;
        }

        int index;
        std::map<int, FString> newMarbles;
        for (index = 0; index < 10000; index++) {
            FString sectionName;
            sectionName.Format("TileSet%04d", index);
            if (!pINI->SectionExists(sectionName)) {
                break;
            }
            else {
                auto nmm = pINI->GetString(sectionName, "NewMarbleMadness");
                if (nmm != "")  {
                    newMarbles[index] = nmm;
                }
            }
        }
        if (index > 0 && newMarbles.size() > 0) {
            for (auto& [i, secName] : newMarbles) {
                FString oldSectionName;
                FString newSectionName;
                oldSectionName.Format("TileSet%04d", i);
                newSectionName.Format("TileSet%04d", index);
                pINI->WriteString(oldSectionName, "MarbleMadness", std::to_string(index).c_str());

                auto section = pINI->AddOrGetSection(secName);
                auto newSection = pINI->AddOrGetSection(newSectionName);
                // explicitly define CustomPalette, to solve compatibility issues with newly added terrains
                auto pal = pINI->GetString(section, "CustomPalette", "iso");
                pINI->WriteString(newSection, "CustomPalette", pal);
                for (auto& pair : section->GetEntities())  {
                    pINI->WriteString(newSection, pair.first, pair.second);
                }
                index++;
            }
        }
    }

    return 0x480892;
}
