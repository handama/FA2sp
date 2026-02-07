#pragma once

#include <CFinalSunApp.h>

#include <array>
#include <vector>
#include <string>

class NOVTABLE CFinalSunAppExt : public CFinalSunApp
{
public:
    CFinalSunAppExt() = delete;

    static CFinalSunAppExt* GetInstance();

    BOOL InitInstanceExt();
    void ParseCommandLine(const char* cmdLine);

    static std::vector<std::string> RecentFilesExt;
    static std::array<std::pair<std::string, std::string>, 7> ExternalLinks;
    static bool HoldingKey;
    static FString ExePathExt;
};