#include "TestCommon.h"
#include <cstdio>
#include <vector>
#include <string>
#include <shellapi.h>
#include <Helpers/Syringe.h>

void SetupLogAndConsole()
{
    // Try to attach to parent console or allocate a new one
    if (!AttachConsole(ATTACH_PARENT_PROCESS))
    {
        AllocConsole();
    }

    // Redirect stdout and stderr to UnitTest.log so test reports can be printed and inspected
    FILE* fpLog = nullptr;
    freopen_s(&fpLog, "UnitTest.log", "w", stdout);
    freopen_s(&fpLog, "UnitTest.log", "a", stderr);
}

// Hook 0x41FAD0 (CFinalSunApp::InitInstance in FA2) via Syringe DEFINE_HOOK
// At this point, CRT and MFC are fully initialized and safe to use.
DEFINE_HOOK(41FAD0, UnitTest_InitInstance, 8)
{
    SetupLogAndConsole();

    printf("=======================================================\n");
    printf("   FA2sp GoogleTest In-Process Test Runner Initiated   \n");
    printf("=======================================================\n\n");

    int rawArgc = 0;
    LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &rawArgc);
    std::vector<std::string> argStorage;
    bool hasXmlOutput = false;

    if (szArglist && rawArgc > 0)
    {
        argStorage.reserve(rawArgc + 2);
        for (int i = 0; i < rawArgc; ++i)
        {
            int sizeNeeded = WideCharToMultiByte(CP_ACP, 0, szArglist[i], -1, NULL, 0, NULL, NULL);
            std::string argStr(sizeNeeded, 0);
            WideCharToMultiByte(CP_ACP, 0, szArglist[i], -1, &argStr[0], sizeNeeded, NULL, NULL);
            if (!argStr.empty() && argStr.back() == '\0')
            {
                argStr.pop_back();
            }
            if (argStr.find("--gtest_output=") != std::string::npos)
            {
                hasXmlOutput = true;
            }
            argStorage.push_back(std::move(argStr));
        }
        LocalFree(szArglist);
    }
    else
    {
        argStorage.push_back("FinalAlert2YR.dat");
    }

    if (!hasXmlOutput)
    {
        argStorage.push_back("--gtest_output=xml:UnitTest.xml");
    }

    std::vector<char*> argvPointers;
    argvPointers.reserve(argStorage.size() + 1);
    for (auto& s : argStorage)
    {
        argvPointers.push_back(s.data());
    }
    argvPointers.push_back(nullptr);

    int argc = static_cast<int>(argStorage.size());
    char** argv = argvPointers.data();

    testing::InitGoogleTest(&argc, argv);

    int res = RUN_ALL_TESTS();

    printf("\n=======================================================\n");
    printf("   Tests completed with exit code: %d\n", res);
    printf("=======================================================\n\n");
    fflush(stdout);
    fflush(stderr);
    fclose(stdout);
    fclose(stderr);

    // Exit immediately so host FA2 does not initialize GUI windows
    ExitProcess(static_cast<UINT>(res));
    return 0;
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
    return TRUE;
}
