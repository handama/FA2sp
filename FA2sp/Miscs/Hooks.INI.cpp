#include "Hooks.INI.h"
#include "../Ext/CMapData/Body.h"
#include "../Ext/CLoading/Body.h"

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
std::unordered_map<CINI*, CINIInfo> CINIManager::propertyMap;
bool INIIncludes::SkipBracketFix = false;

void CINIExt::LoadINIExt(uint8_t* pFile, size_t fileSize, const char* lpSection,
    bool bClear, bool bTrimSpace, bool bAllowInclude, std::queue<ppmfc::CString>* parentIncludeInis)
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

    auto encoding = STDHelpers::GetFileEncoding(pFile, fileSize);
    CINIManager::GetInstance().SetProperty(this, encoding);
    bool loadAsUTF8 = ExtConfigs::UTF8Support_InferEncoding && encoding == UTF8 || encoding == UTF8_BOM;
    FString content(reinterpret_cast<char*>(pFile), fileSize);
    if (loadAsUTF8)
        content.toANSI();

    INISection* pCurrentSection = nullptr;
    FString CurrentSectionName;
    FString PendingComment;

    size_t idx = 0;
    const size_t len = content.length();
    static size_t plusEqual = 0;
    static std::map<CINIExt*, std::map<FString, std::vector<FString>>> InheritSections;
    std::set<FString> LoadedSections;
    if (bAllowInclude) {
        plusEqual = 0;
        InheritSections.clear();
    }
    bool findTargetSection = false;
    bool firstLine = true;
    bool keepComment = this == &CINI::CurrentDocument && ExtConfigs::SaveMap_KeepComments;

    std::istringstream iss(content);
    FString line;
    while (std::getline(iss, line))
    {
        line.Trim();
        if (!line.empty())
        {
            if (firstLine) {
                firstLine = false;
                if (encoding == UTF8_ASCII && line.find("UTF8") != FString::npos)
                    loadAsUTF8 = true;
            }
            if (!keepComment) {
                if (line[0] == ';') continue;
            }
            else {
                if (line[0] == ';') {
                    FString comment = line.substr(1);
                    comment.Trim();
                    PendingComment += comment + "\n";
                    continue;
                }
            }

            // ------------------- Section -------------------
            if (line[0] == '[') {
                size_t closePos = line.find(']');

                if (closePos == FString::npos) {
                    if (INIIncludes::SkipBracketFix) exit(1);
                    continue;
                }

                FString inlineSectionComment;
                if (keepComment) {
                    size_t commentPos = line.find(';', closePos + 1);
                    if (commentPos != FString::npos) {
                        inlineSectionComment = line.substr(commentPos + 1);
                        inlineSectionComment.Trim();
                        line = line.substr(0, commentPos);
                    }
                }

                CurrentSectionName = line.substr(1, closePos - 1);

                if (closePos + 1 < line.size() && line[closePos + 1] == ':') {
                    size_t p = closePos + 2;
                    if (p < line.size() && line[p] == '[') {
                        size_t close2 = line.find(']', p);
                        if (close2 != FString::npos) {
                            InheritSections[this][CurrentSectionName].push_back(line.substr(p + 1, close2 - (p + 1)));
                        }
                    }
                }

                if (!CurrentSectionName.empty()) {
                    if (lpSection && CurrentSectionName != lpSection) {
                        pCurrentSection = nullptr;
                        continue;
                    }
                    else if (lpSection && CurrentSectionName == lpSection) {
                        findTargetSection = true;
                    }
                    else if (lpSection && findTargetSection) {
                        return;
                    }

                    // only read first repeated section in the main file
                    // for include files, all sections will be read
                    if (bAllowInclude) {
                        if (LoadedSections.find(CurrentSectionName) != LoadedSections.end()) {
                            pCurrentSection = nullptr;
                        }
                        else {
                            pCurrentSection = AddOrGetSection(CurrentSectionName);
                            LoadedSections.insert(CurrentSectionName);
                        }
                    }
                    else {
                        pCurrentSection = AddOrGetSection(CurrentSectionName);
                    }

                    if (CMapDataExt::IsLoadingMapFile && ExtConfigs::SaveMap_PreserveINISorting) {
                        auto it = std::find(CMapDataExt::MapIniSectionSorting.begin(), CMapDataExt::MapIniSectionSorting.end(), CurrentSectionName);
                        if (it == CMapDataExt::MapIniSectionSorting.end()) {
                            CMapDataExt::MapIniSectionSorting.push_back(CurrentSectionName);
                        }
                    }

                    if (keepComment) {
                        PendingComment.Trim();
                        if (!PendingComment.empty()) {
                            CMapDataExt::MapFrontsectionComments[CurrentSectionName] = PendingComment;
                            PendingComment.clear();
                        }
                        inlineSectionComment.Trim();
                        if (!inlineSectionComment.empty()) {
                            CMapDataExt::MapInsectionComments[CurrentSectionName] = inlineSectionComment;
                        }
                    }
                }
                continue;
            }

            // -------------------  Key=Value -------------------
            if (pCurrentSection) {
                size_t eqPos = line.find('=');

                FString inlineComment;
                if (keepComment) {
                    size_t commentPos = line.find(';', eqPos + 1);
                    if (commentPos != FString::npos) {
                        inlineComment = line.substr(commentPos + 1);
                        inlineComment.Trim();
                        line = line.substr(0, commentPos);
                    }
                }

                if (eqPos == FString::npos) continue;

                FString key = line.substr(0, eqPos);
                FString value = line.substr(eqPos + 1);

                int semicolon = value.Find(';');
                if (semicolon > 0) {
                    value = value.Mid(0, semicolon);
                }

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

                    if (keepComment) {
                        PendingComment.Trim();
                        if (!PendingComment.empty()) {
                            CMapDataExt::MapFrontlineComments[CurrentSectionName][key] = PendingComment;
                            PendingComment.clear();
                        }
                        inlineComment.Trim();
                        if (!inlineComment.empty()) {
                            CMapDataExt::MapInlineComments[CurrentSectionName][key] = inlineComment;
                        }
                    }

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
        else if (!PendingComment.IsEmpty())
        {
            PendingComment += "\n";
        }
    }
 
    auto loadAresInheritedIni = [&](CINIExt* ini)
    {
        // ares mode
        if (ExtConfigs::AllowInherits && !ExtConfigs::InheritType) {
            auto itr = InheritSections.find(ini);
            if (itr != InheritSections.end()) {
                const auto& pairs = itr->second;
                for (const auto& [main, inherits] : pairs) {
                    auto pSectionA = GetSection(main);
                    auto pSectionA_New = ini->GetSection(main);
                    for (const auto& inherit : inherits) {
                        auto pSectionB = GetSection(inherit);
                        if (pSectionA && pSectionB) {
                            for (const auto& [key, value] : pSectionB->GetEntities())
                            {
                                if (pSectionA_New
                                    && pSectionA_New->GetEntities().find(key)
                                    != pSectionA_New->GetEntities().end())
                                    continue;

                                size_t currentIndex = pSectionA->GetEntities().size();

                                writeString(pSectionA, key, value);

                                std::pair<ppmfc::CString, int> ins =
                                    std::make_pair((ppmfc::CString)key, (int)currentIndex);
                                std::pair<INIIndiceDict::iterator, bool> ret;
                                reinterpret_cast<FAINIIndicesMap*>(&pSectionA->GetIndices())->insert(&ret, &ins);
                            }
                        }
                    }            
                }
                InheritSections.erase(itr);
            }
        }
    };

    if (ExtConfigs::AllowIncludes && bAllowInclude)
    {
        using INIPair = std::pair<ppmfc::CString, ppmfc::CString>;
        const char* includeSection = ExtConfigs::IncludeType ? "$Include" : "#include";
        auto pIncludeSection = GetSection(includeSection);
        if (pIncludeSection || parentIncludeInis) {
            if (this == &CINI::CurrentDocument) {
                INIIncludes::MapINIWarn = true;
            }
            std::set<ppmfc::CString> includedInis;
            std::queue<ppmfc::CString> currentIncludeInis;
            if (parentIncludeInis)
            {
                currentIncludeInis = *parentIncludeInis;
            }
            else
            {
                for (auto& [index, key] : ParseIndiciesData(includeSection)) {
                    currentIncludeInis.push(pIncludeSection->GetString(key));
                }
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
                                GameDeleteArray(pBuffer, dwSize);
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
                            loadAresInheritedIni(&ini);
                        }
                    }
                }
                currentIncludeInis = std::move(nextIncludeInis);
            }            
        }
    }
    // main ini
    if(bAllowInclude)
        loadAresInheritedIni(this);
    // phobos mode
    if (ExtConfigs::AllowInherits && ExtConfigs::InheritType) {
        for (auto& sectionA : Dict) {
            if (KeyExists(sectionA.first, "$Inherits"))  {
                auto inherits = STDHelpers::SplitString(GetString(sectionA.first, "$Inherits"));
                for (const auto& sectionB : inherits) {
                    if (auto pSectionB = GetSection(sectionB)) {
                        for (const auto& [key, value] : pSectionB->GetEntities()) {
                            if (!KeyExists(sectionA.first, key)) {
                                size_t currentIndex = sectionA.second.GetEntities().size();

                                writeString(&sectionA.second, key, value);

                                std::pair<ppmfc::CString, int> ins =
                                    std::make_pair((ppmfc::CString)key, (int)currentIndex);
                                std::pair<INIIndiceDict::iterator, bool> ret;
                                reinterpret_cast<FAINIIndicesMap*>(&sectionA.second.GetIndices())->insert(&ret, &ins);
                            }
                        }
                    }
                }
            }
        }
    }

    if (CMapDataExt::IsLoadingMapFile && this == &CINI::CurrentDocument) {
        if (loadAsUTF8) {
            Logger::Debug("CINIExt::LoadINIExt(): Load map file using UTF8 encoding.\n");
            CMapDataExt::IsUTF8File = true;
        }
        else {
            CMapDataExt::IsUTF8File = false;
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

DEFINE_HOOK(47FFB0, CLoading_LoadTSINI, 7)
{
    GET(CLoadingExt*, pThis, ECX);
    GET_STACK(const char*, pFile, 0x4);
    GET_STACK(CINIExt*, pINI, 0x8);
    GET_STACK(BOOL, bMerge, 0xC);

    CINIManager::GetInstance().SetProperty(pINI, pFile);
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

DEFINE_HOOK(4536B0, CINI_WriteToFile, 8)
{
    GET(CINIExt*, pThis, ECX);
    GET_STACK(const char*, lpPath, 0x4);

    std::ofstream out(lpPath, std::ios::binary | std::ios::out);
    if (!out.is_open()) {
        R->EAX(0);
        return 0x453A10;
    }

    std::ostringstream oss;
    for (auto& [sectionName, pSection] : pThis->Dict)
    {
        oss << "[" << sectionName << "]\n";
        for (const auto& pair : pSection.GetEntities())
            oss << pair.first << "=" << pair.second << "\n";
        oss << "\n";
    }

    FString output = oss.str();
    auto info = CINIManager::GetInstance().GetProperty(pThis);
    if (info.Encoding == UTF8)
    {
        output.toUTF8();
    }
    else if(info.Encoding == UTF8_BOM)
    {
        output.toUTF8();
        const unsigned char bom[3] = { 0xEF, 0xBB, 0xBF };
        out.write(reinterpret_cast<const char*>(bom), 3);
    }

    out.write(output.data(), output.size());
    if (!out.good()) {
        R->EAX(0);
        return 0x453A10;
    }
    R->EAX(1);
    return 0x453A10;
}

DEFINE_HOOK(446520, CINI_GetAvailableIndex, 7)
{
    GET_STACK(ppmfc::CString*, pRet, 0x4);

    auto index = CMapDataExt::GetAvailableIndex();
    new(pRet) ppmfc::CString(index);
    R->EAX(pRet);

    return 0x446FB3;
}
