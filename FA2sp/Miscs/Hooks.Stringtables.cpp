#include <CMixFile.h>
#include <CLoading.h>
#include <FAMemory.h>
#include <CFinalSunApp.h>
#include <CFinalSunDlg.h>

#include "../Helpers/STDHelpers.h"
#include "../FA2sp.h"
#include "StringtableLoader.h"

#include <map>
#include <fstream>
#include "../Helpers/TheaterHelpers.h"

bool StringtableLoader::bLoadRes = false;
char* StringtableLoader::pEDIBuffer = nullptr;
wchar_t StringtableLoader::pStringBuffer[0x400] = {};
std::map<FString, FString> StringtableLoader::CSFFiles_Stringtable;

FString StringtableLoader::QueryUIName(const char* pRegName, bool bOnlyOneLine)
{
    MultimapHelper mmh;
    mmh.AddINI(&CINI::Rules());
    mmh.AddINI(&CINI::CurrentDocument());

    auto uiname = mmh.GetString(pRegName, "UIName", "");
    uiname.MakeLower();
    FString ccstring = "";
    if (uiname != "" && StringtableLoader::CSFFiles_Stringtable.find(uiname) != StringtableLoader::CSFFiles_Stringtable.end())
        ccstring = StringtableLoader::CSFFiles_Stringtable[uiname];
    if (ccstring == "")
        ccstring = mmh.GetString(pRegName, "Name", "");
    if (ccstring == "")
        ccstring = "MISSING";

    if (uiname.Find("nostr:") == 0) {
        ccstring = mmh.GetString(pRegName, "UIName", "").Mid(6);
    }

    ccstring = CINI::FALanguage().GetString("RenameID", pRegName, ccstring);
    auto theater = TheaterHelpers::GetCurrentSuffix();
    theater.MakeUpper();
    theater = "RenameID" + theater;
    ccstring = CINI::FALanguage().GetString(theater, pRegName, ccstring);

    if (bOnlyOneLine)
    {
        ccstring.Replace("\r", "");
        int idx = ccstring.Find('\n');
        return idx == -1 ? ccstring : ccstring.Mid(0, idx);
    }

    return ccstring;
}

DEFINE_HOOK(4B2610, QueryUIName, 7)
{
    GET_STACK(char*, pRegName, 0x4);

    FString ccstring = StringtableLoader::QueryUIName(pRegName);

    memset(StringtableLoader::pStringBuffer, 0, sizeof(StringtableLoader::pStringBuffer));

    int len = MultiByteToWideChar(
        CP_ACP, 0,
        ccstring.c_str(), -1,
        StringtableLoader::pStringBuffer,
        0x400
    );

    if (len == 0) {
        StringtableLoader::pStringBuffer[0] = L'\0';
    }

    R->EAX(StringtableLoader::pStringBuffer);
    return 0x4B33EB;
}

DEFINE_HOOK(492C40, CSFFiles_Stringtables_Support, 7)
{
    CFinalSunDlg::LastSucceededOperation = 9;
    StringtableLoader::CSFFiles_Stringtable.clear();
    StringtableLoader::LoadCSFFiles();
    Logger::Debug("Successfully loaded %d csf labels.\n", StringtableLoader::CSFFiles_Stringtable.size());

    char tmpCsfFile[0x400];
    strcpy_s(tmpCsfFile, CFinalSunApp::ExePath());
    strcat_s(tmpCsfFile, "\\RA2Tmp.csf");
    DeleteFile(tmpCsfFile);

    return 0x494341;
}

void StringtableLoader::LoadCSFFiles()
{
    char nameBuffer[0x400];
    if (CLoading::HasMdFile())
        strcpy_s(nameBuffer, CINI::FAData->GetString("Filenames", "CSFYR", "RA2MD.CSF"));
    else
        strcpy_s(nameBuffer, CINI::FAData->GetString("Filenames", "CSF", "RA2.CSF"));
    LoadCSFFile(nameBuffer);
    char stringtable[20];
    for (int i = 0; i <= 99; ++i)
    {
        sprintf_s(stringtable, "stringtable%02d.csf", i);
        LoadCSFFile(stringtable);
    }
    for (int i = 0; i <= 99; ++i)
    {
        sprintf_s(stringtable, "stringtable%02d.llf", i);
        LoadCSFFile(stringtable);
    }
    for (int i = 0; i <= 99; ++i)
    {
        sprintf_s(stringtable, "lcstring%02d.ecs", i);
        LoadCSFFile(stringtable);
    }

    LoadCSFFile("fa2extra.csf", true);
    if (ExtConfigs::LoadCivilianStringtable)
        LoadCSFFile("fa2civilian.csf", true);

    if (auto pSection = CINI::FAData->GetSection("ExtraStringtables"))
    {
        std::map<int, FString> collector;

        for (const auto& [key, index] : pSection->GetIndices())
            collector[index] = key;

        FString path;

        for (const auto& [_, key] : collector)
        {
            LoadCSFFile(key, CINI::FAData->GetBool("ExtraStringtables", key));
        }
    }

    WriteCSFFile();
}

void StringtableLoader::LoadCSFFile(const char* pName, bool fa2path)
{
    DWORD dwSize;
    if (auto pBuffer = CLoading::Instance->ReadWholeFile(pName, &dwSize, fa2path)) {
        FString name = pName;
        name.MakeUpper();
        if (name.Mid(name.GetLength() - 3) == "LLF") {
            auto ret = GetLinesFromBuffer((char*)pBuffer, dwSize);
            if (ParseLLFFile(ret))
                Logger::Debug("Successfully Loaded file %s.\n", pName);
        }
        else if (name.Mid(name.GetLength() - 3) == "ECS") {
            auto ret = GetLinesFromBuffer((char*)pBuffer, dwSize);
            if (ParseECSFile(ret))
                Logger::Debug("Successfully Loaded file %s.\n", pName);
        }
        else {
            if (ParseCSFFile((char*)pBuffer, dwSize))
                Logger::Debug("Successfully Loaded file %s.\n", pName);
        }
    }
}

bool StringtableLoader::ParseECSFile(std::vector<FString>& ret)
{
    FString currentLabel = "";
    FString currentContent = "";
    bool isMultiline = false;
    bool isMultilineFirst = false;
    for (const auto& line : ret) {
        if (line.Find("#") == 0) continue;

        int start = line.Find("> ");
        int multiStart = line.Find(">>");
        if (start <= 0 && multiStart <= 0 && currentLabel == "") continue;
        if (start > 0) {
            isMultiline = false;
            currentLabel = line.Mid(0, start);
            currentLabel.MakeLower();
            FString left = line.Mid(start + 2);
            currentContent = left;
        }
        else if (multiStart > 0) {
            isMultiline = true;
            isMultilineFirst = true;
            currentLabel = line.Mid(0, multiStart);
            currentLabel.MakeLower();
            currentContent = "";
            continue;
        }
        else if (currentLabel != "" && isMultiline) {
            int multiline = line.Find("  ");
            if (multiline == 0) {
                FString left = line.Mid(2);
                if (!isMultilineFirst) {
                    currentContent += "\n";
                }
                else{
                    isMultilineFirst = false;
                }
                currentContent += left;
            }
        }
        StringtableLoader::CSFFiles_Stringtable[currentLabel] = currentContent;
    }
    return true;
}

bool StringtableLoader::ParseLLFFile(std::vector<FString>& ret)
{
    FString currentLabel = "";
    FString currentContent = "";
    bool firstLineEmpty = false;
    for (auto& line : ret) {
        if (line.Find("#") == 0) continue;
        if (line.Find(" #") != -1) {
            line = line.Mid(0, line.Find(" #"));
        }

        int start = line.Find(": ");
        if (start <= 0 && currentLabel == "") continue;
        if (start > 0) {
            currentLabel = line.Mid(0, start);
            currentLabel.MakeLower();
            currentContent = "";
            FString left = line.Mid(start + 2);
            FString leftTrim = left;
            leftTrim.Trim();
            if (leftTrim == ">-") {
                firstLineEmpty = true;
                continue;
            }
            else {
                firstLineEmpty = false;
                currentContent += left;
            }
        }
        else if (currentLabel != "") {
            int multiline = line.Find("  ");
            if (multiline == 0) {
                FString left = line.Mid(2);
                if (!firstLineEmpty) {
                    currentContent += "\n";
                }
                else {
                    firstLineEmpty = false;
                }
                currentContent += left;
            }
        }
        StringtableLoader::CSFFiles_Stringtable[currentLabel] = currentContent;
    }
    return true;
}

std::vector<FString> StringtableLoader::GetLinesFromBuffer(char* buffer, DWORD size)
{
    std::vector<FString> ret;
    std::string fileContent(reinterpret_cast<const char*>(buffer), size);

    std::istringstream stream(fileContent);
    FString line;
    while (std::getline(stream, line)) {
        line.toANSI();
        ret.push_back(line);
    }
    return ret;
}

bool StringtableLoader::ParseCSFFile(char* buffer, DWORD size)
{
    char* pos = buffer;

    auto read_int = [&pos](const void* dest)
    {
        memcpy((void*)dest, pos, 4);
        pos += 4;
    };

    // Parse CSF header
    if (memcmp(pos, " FSC", 0x4) != 0) {
        return false;
    }
    pos += 4; // FSC
    pos += 4; // version
    int _numLabels;
    read_int(&_numLabels);
    pos += 4; // numstrings
    pos += 4; // useless
    pos += 4; // lang
    // Read CSF labels
    for (int i = 0; i < _numLabels; ++i)
    {
        // Read CSF label header
        int identifier;
        read_int(&identifier);
        if (identifier == 0x4C424C20) // " LBL"
        {
            int numPairs;
            read_int(&numPairs);
            int strLength;
            read_int(&strLength);
            
            char* labelstr = new char[strLength + 1];
            labelstr[strLength] = '\0';
            memcpy_s(labelstr, strLength, pos, strLength);

            pos += strLength;
            // CSF labels are not case sensitive.
            for (int k = 0; k < strLength; ++k)
                labelstr[k] = tolower(labelstr[k]);

            read_int(&identifier);
            read_int(&strLength);

            char* tmpWideBuffer = new char[(strLength << 1) + 2];
            for (int i = 0; i < strLength << 1; ++i)
                tmpWideBuffer[i] = ~pos[i];
            wchar_t* ptrWideBuffer = reinterpret_cast<wchar_t*>(tmpWideBuffer);
            ptrWideBuffer[strLength] = '\0';
            
            char* value = nullptr;
            int valueBufferSize = WideCharToMultiByte(CP_ACP, NULL, ptrWideBuffer, strLength, value, 0, NULL, NULL) + 1;
            value = new char[valueBufferSize];
            WideCharToMultiByte(CP_ACP, NULL, ptrWideBuffer, strLength, value, valueBufferSize, NULL, NULL);
            delete[] tmpWideBuffer;
            value[valueBufferSize - 1] = '\0';

            pos += (strLength << 1);
            if (identifier == 0x53545257) // "WSTR"
            {
                read_int(&strLength);
                pos += strLength;
            }

            StringtableLoader::CSFFiles_Stringtable[labelstr] = value;

            delete[] labelstr;
            delete[] value;

            for (int j = 1; j < numPairs; ++j) // Extra labels will be ignored here
            {
                read_int(&identifier);
                read_int(&strLength);
                pos += (strLength << 1);
                if (identifier == 0x53545257) // "WSTR"
                {
                    read_int(&strLength);
                    pos += strLength;
                }
            }
        }
        else {
            break;
        }
    }
    return true;
}

void StringtableLoader::WriteCSFFile()
{
    char tmpCsfFile[0x400];
    strcpy_s(tmpCsfFile, CFinalSunApp::ExePath());
    strcat_s(tmpCsfFile, "\\RA2Tmp.csf");
    std::ofstream fout;
    fout.open(tmpCsfFile, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!fout.is_open())
        return;

    auto write_to_stream = [&fout](const void* buffer, size_t size = 4) {
        fout.write((char*)buffer, size);
    };

    auto write_int = [&fout](int n) {
        fout.write((char*)&n, 4);
    };

    // CSF header
    write_to_stream(" FSC");
    write_int(3);
    write_int(StringtableLoader::CSFFiles_Stringtable.size());
    write_int(StringtableLoader::CSFFiles_Stringtable.size());
    write_to_stream("LMAO"); // useless
    write_int(0);

    // CSF labels
    for (auto& lbl : StringtableLoader::CSFFiles_Stringtable)
    {
        // label
        write_to_stream(" LBL");
        write_int(1);
        write_int(lbl.first.GetLength());
        write_to_stream(lbl.first, lbl.first.GetLength());

        // value
        write_to_stream(" RTS");
        wchar_t* value = nullptr;
        int valueBufferSize = MultiByteToWideChar(CP_ACP, NULL, lbl.second, lbl.second.GetLength(), value, 0);
        value = new wchar_t[valueBufferSize + 1];
        MultiByteToWideChar(CP_ACP, NULL, lbl.second, lbl.second.GetLength(), value, valueBufferSize);
        value[valueBufferSize] = '\0';
        char* buffer = reinterpret_cast<char*>(value);
        for (int i = 0; i < valueBufferSize << 1; ++i)
            buffer[i] = ~buffer[i];
        write_int(valueBufferSize);
        write_to_stream(buffer, valueBufferSize << 1);
        delete[] value;
    }
    fout.close();
}

bool StringtableLoader::LoadToBuffer()
{
    char directoryBuffer[0x400];
    strcpy_s(directoryBuffer, CFinalSunApp::ExePath());
    strcat_s(directoryBuffer, "\\");
    strcat_s(directoryBuffer, "RA2Tmp.csf");
    std::ifstream fin;
    fin.open(directoryBuffer, std::ios::in | std::ios::binary);
    if (fin.is_open())
    {
        fin.seekg(0, std::ios::end);
        const int size = static_cast<int>(fin.tellg());
        if (size == 0)
            return false;
        fin.seekg(0, std::ios::beg);
        StringtableLoader::pEDIBuffer = GameCreateArray<char>(size);
        bool result = false;
        if (StringtableLoader::pEDIBuffer)
            fin.read(StringtableLoader::pEDIBuffer, size);
        fin.close();
        return true;
    }
    return false;
}