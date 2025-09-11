#pragma once
#include "../FA2sp.h"

#include <map>
#include <fstream>

class StringtableLoader
{
public:
    static void LoadCSFFiles();
    static void LoadCSFFile(const char* pName, bool fa2path = false);
    static std::vector<FString> GetLinesFromBuffer(char* buffer, DWORD size);
    static bool ParseCSFFile(char* buffer, DWORD size);
    static bool ParseLLFFile(std::vector<FString>& ret);
    static bool ParseECSFile(std::vector<FString>& ret);
    static void WriteCSFFile();
    static bool LoadToBuffer();
    static FString QueryUIName(const char* pRegName, bool bOnlyOneLine = false);

    static std::map<FString, FString> CSFFiles_Stringtable;
    static char* pEDIBuffer;
    static wchar_t pStringBuffer[0x400];
    static bool bLoadRes;
};