#pragma once

#include <CFinalSunApp.h>

#include <array>
#include <vector>
#include <string>

struct Version {
    int major = 0;
    int minor = 0;
    int revision = 0;
};

class NOVTABLE CFinalSunAppExt : public CFinalSunApp
{
public:
    CFinalSunAppExt() = delete;

    static CFinalSunAppExt* GetInstance();

    BOOL InitInstanceExt();
    void ParseCommandLine(const char* cmdLine);
    static void CheckUpdate();

    static std::vector<std::string> RecentFilesExt;
    static std::array<std::pair<std::string, std::string>, 7> ExternalLinks;
    static bool HoldingKey;
    static bool HasNewVersion;
    static FString NewVersion;
    static FString ExePathExt;
    static FString LauncherName;
};