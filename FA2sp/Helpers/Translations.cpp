#include "Translations.h"

#include <CFinalSunApp.h>
#include <Helpers/Macro.h>
#include <CINI.h>
#include "../FA2sp.h"
#include <CMapData.h>
#include "TheaterHelpers.h"
#include "../FA2sp.Constants.h"
#include "FString.h"
#include "../Miscs/StringtableLoader.h"
#include "../Ext/CFinalSunApp/Body.h"

ppmfc::CString FinalAlertConfig::lpPath;
char FinalAlertConfig::pLastRead[0x400];

// Load after ExePath is initialized
DEFINE_HOOK(41F7F5, Translations_Initialzation, 9)
{
    FinalAlertConfig::lpPath = CFinalSunAppExt::ExePathExt;
    FinalAlertConfig::lpPath += "FinalAlert.ini";
    FinalAlertConfig::ReadString("FinalSun", "Language", "English");
    strcpy_s(Translations::pLanguage[0], FinalAlertConfig::pLastRead);
    strcpy_s(Translations::pLanguage[1], FinalAlertConfig::pLastRead);
    strcpy_s(Translations::pLanguage[2], FinalAlertConfig::pLastRead);
    strcpy_s(Translations::pLanguage[3], FinalAlertConfig::pLastRead);
    strcat_s(Translations::pLanguage[0], "-StringsRA2");
    strcat_s(Translations::pLanguage[1], "-TranslationsRA2");
    strcat_s(Translations::pLanguage[2], "-Strings");
    strcat_s(Translations::pLanguage[3], "-Translations");
    return 0;
}

DWORD FinalAlertConfig::ReadString(const char* pSection, const char* pKey, const char* pDefault, char* pBuffer)
{
    DWORD dwRet = GetPrivateProfileString(pSection, pKey, pDefault, FinalAlertConfig::pLastRead, 0x400, lpPath);
    if (pBuffer)
        strcpy_s(pBuffer, 0x400, pLastRead);
    return dwRet;
}
void FinalAlertConfig::WriteString(const char* pSection, const char* pKey, const char* pContent)
{
    WritePrivateProfileString(pSection, pKey, pContent, lpPath);
};

char Translations::pLanguage[4][0x400];
ppmfc::CString Translations::CurrentTileSet;
bool Translations::GetTranslationItem(const char* pLabelName, ppmfc::CString& ret)
{
    auto& falanguage = CINI::FALanguage();

    for (const auto& language : Translations::pLanguage)
    {
        if (strstr(language, "RenameID") != NULL)
            continue;
        if (auto section = falanguage.GetSection(language))
        {
            auto itr = section->GetEntities().find(pLabelName);
            if (itr != section->GetEntities().end())
            {
                ppmfc::CString buffer = itr->second;
                buffer.Replace("\\n", "\n");
                buffer.Replace("\\t", "\t");
                buffer.Replace("\\r", "\r");
                TranslateStringVariables(9, buffer, __str(PROGRAM_TITLE));
                ret = buffer;
                return true;
            }
        }
    }

    return false;
}

bool Translations::GetTranslationItem(const char* pLabelName, FString& ret)
{
    auto& falanguage = CINI::FALanguage();

    for (const auto& language : Translations::pLanguage)
    {
        if (strstr(language, "RenameID") != NULL)
            continue;
        if (auto section = falanguage.GetSection(language))
        {
            auto itr = section->GetEntities().find(pLabelName);
            if (itr != section->GetEntities().end())
            {
                FString buffer = itr->second;
                buffer.Replace("\\n", "\n");
                buffer.Replace("\\t", "\t");
                buffer.Replace("\\r", "\r");
                TranslateStringVariables(9, buffer, __str(PROGRAM_TITLE));
                ret = buffer;
                return true;
            }
        }
    }

    return false;
}

const char* Translations::TranslateOrDefault(const char* lpLabelName, const char* lpDefault)
{
    for (const auto& language : Translations::pLanguage)
    {
        if (strstr(language, "RenameID") != NULL)
            continue;
        if (auto section = CINI::FALanguage->GetSection(language))
        {
            auto itr = section->GetEntities().find(lpLabelName);
            if (itr != section->GetEntities().end())
            {
                ppmfc::CString buffer = itr->second;
                buffer.Replace("\\n", "\n");
                buffer.Replace("\\t", "\t");
                buffer.Replace("\\r", "\r");
                TranslateStringVariables(9, buffer, __str(PROGRAM_TITLE));
                return buffer;
            }
                
        }
    }

    return lpDefault;
}

const char* Translations::TranslateStringVariables(int n, const char* originaltext, const char* inserttext)
{
    char c[50];
    _itoa(n, c, 10);

    char seekedstring[50];
    seekedstring[0] = '%';
    seekedstring[1] = 0;
    strcat(seekedstring, c);

    FString orig = originaltext;
    if (orig.Find(seekedstring) < 0) return orig;

    orig.Replace(seekedstring, inserttext);

    return orig;
}

void Translations::TranslateStringVariables(int n, ppmfc::CString& text, const char* inserttext)
{
    char c[50];
    _itoa(n, c, 10);

    char seekedstring[50];
    seekedstring[0] = '%';
    seekedstring[1] = 0;
    strcat(seekedstring, c);

    if (text.Find(seekedstring) < 0) return;

    text.Replace(seekedstring, inserttext);
}

void Translations::TranslateItem(CWnd* pWnd, int nSubID, const char* lpKey)
{
    ppmfc::CString buffer;
    if (Translations::GetTranslationItem(lpKey, buffer))
        pWnd->SetDlgItemText(nSubID, buffer);
}

void Translations::TranslateItem(CWnd* pWnd, const char* lpKey)
{
    ppmfc::CString buffer;
    if (Translations::GetTranslationItem(lpKey, buffer))
        pWnd->SetWindowText(buffer);
}

ppmfc::CString Translations::TranslateTileSet(int index)
{
    if (!CMapData::Instance->MapWidthPlusHeight)
        return "MISSING";

    ppmfc::CString setID;
    setID.Format("TileSet%04d", index);
    auto setName = CINI::CurrentTheater()->GetString(setID, "SetName", setID);
    ppmfc::CString result = TranslateOrDefault(setName, setName);

    auto theater = TheaterHelpers::GetCurrentSuffix();
    theater.MakeUpper();
    theater = "RenameID" + theater;
    result = CINI::FALanguage().GetString("RenameID", setID, result);
    result = CINI::FALanguage().GetString(theater, setID, result);

    return result;
}


FString Translations::ParseHouseName(FString src, bool IDToUIName)
{
    if (ExtConfigs::NoHouseNameTranslation)
    {
        return src;
    }
    else
    {
        auto& countries = CINI::Rules->GetSection("Countries")->GetEntities();
        FString translated;

        if (IDToUIName)
        {
            for (auto& pair : countries)
            {
                if (ExtConfigs::BetterHouseNameTranslation)
                    translated = StringtableLoader::QueryUIName(pair.second) + "(" + pair.second + ")";
                else
                    translated = StringtableLoader::QueryUIName(pair.second);

                src.Replace(pair.second, translated);
            }
        }
        else
        {
            for (auto& pair : countries)
            {
                if (ExtConfigs::BetterHouseNameTranslation)
                    translated = StringtableLoader::QueryUIName(pair.second) + "(" + pair.second + ")";
                else
                    translated = StringtableLoader::QueryUIName(pair.second);
                src.Replace(translated, pair.second);
            }
        }
    }
    return src;
}

DEFINE_HOOK(43DA80, FALanguage_GetTranslationItem, 7)
{
    GET_STACK(ppmfc::CString*, pRet, 0x4);
    GET_STACK(ppmfc::CString, pString, 0x8);

    ppmfc::CString pResult;
    Translations::GetTranslationItem(pString, pResult);
    new(pRet) ppmfc::CString(pResult);
    R->EAX(pRet);
    return 0x43E2AE;
}

DEFINE_HOOK(43C3C0, Miscs_ParseHouseName, 7)
{
    GET_STACK(ppmfc::CString*, pRet, 0x4);
    REF_STACK(ppmfc::CString, src, 0x8);
    GET_STACK(bool, IDToUIName, 0xC);
    if (ExtConfigs::NoHouseNameTranslation)
    {
        new(pRet) ppmfc::CString(src);
        R->EAX(pRet);

        return 0x43CA72;
    }
    else
    {
        auto& countries = CINI::Rules->GetSection("Countries")->GetEntities();
        FString translated;

        if (IDToUIName)
        {
            for (auto& pair : countries)
            {
                if (ExtConfigs::BetterHouseNameTranslation)
                    translated = StringtableLoader::QueryUIName(pair.second) + "(" + pair.second + ")";
                else
                    translated = StringtableLoader::QueryUIName(pair.second);

                src.Replace(pair.second, translated);
            }
        }
        else
        {
            for (auto& pair : countries)
            {
                if (ExtConfigs::BetterHouseNameTranslation)
                    translated = StringtableLoader::QueryUIName(pair.second) + "(" + pair.second + ")";
                else
                    translated = StringtableLoader::QueryUIName(pair.second);
                src.Replace(translated, pair.second);
            }
        }
        new(pRet) ppmfc::CString(src);
        R->EAX(pRet);

        return 0x43CA72;

    }
    return 0;
}

DEFINE_HOOK(4F0C3D, CTerrainDlg_Update_GetTileSet, 5)
{
    GET_STACK(ppmfc::CString, tileset, STACK_OFFS(0x1A4, 0x188));
    Translations::CurrentTileSet = tileset;
    return 0;
}

DEFINE_HOOK(4F0E1A, CTerrainDlg_Update_SetTileName, 8)
{
    auto theater = TheaterHelpers::GetCurrentSuffix();
    theater.MakeUpper();
    theater = "RenameID" + theater;
    auto name = CINI::FALanguage().TryGetString(theater, Translations::CurrentTileSet);
    if (!name) {
        name = CINI::FALanguage().TryGetString("RenameID", Translations::CurrentTileSet);
    }

    if (name) {
        R->EAX(name);
    }
    return 0;
}

DEFINE_HOOK(4F1620, CTerrainDlg_Update_SetOverlayName, 8)
{
    GET(int, index, ESI);
    auto theater = TheaterHelpers::GetCurrentSuffix();
    theater.MakeUpper();
    theater = "RenameID" + theater;
    const auto ovrID = Variables::RulesMap.GetValueAt("OverlayTypes", index);
    auto name = CINI::FALanguage().TryGetString(theater, ovrID);
    if (!name) {
        name = CINI::FALanguage().TryGetString("RenameID", ovrID);
    }
    
    if (name) {
        R->EAX(name);
    }
    return 0;
}