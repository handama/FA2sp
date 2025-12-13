#include "Translations.h"
#include <CFinalSunApp.h>
#include <CFinalSunDlg.h>
#include <Helpers/Macro.h>
#include <CINI.h>
#include "../FA2sp.h"
#include <CMapData.h>
#include "TheaterHelpers.h"
#include "../FA2sp.Constants.h"
#include "FString.h"
#include "../Miscs/StringtableLoader.h"
#include "../Ext/CFinalSunApp/Body.h"
#include <regex>

FString FinalAlertConfig::lpPath;
char FinalAlertConfig::pLastRead[0x400];

// Load after ExePath is initialized
DEFINE_HOOK(41F7F5, Translations_Initialzation, 9)
{
    FString exePath = CFinalSunAppExt::ExePath();
    size_t lastSlash = exePath.find_last_of('\\');
    if (lastSlash != std::string::npos)
        exePath = exePath.substr(0, lastSlash);

    FinalAlertConfig::lpPath = exePath;
    FinalAlertConfig::lpPath += "\\FinalAlert.ini";
    FinalAlertConfig::ReadString("FinalSun", "Language", "English");
    strcpy_s(Translations::pLanguage[0], FinalAlertConfig::pLastRead);
    strcpy_s(Translations::pLanguage[1], FinalAlertConfig::pLastRead);
    strcpy_s(Translations::pLanguage[2], FinalAlertConfig::pLastRead);
    strcpy_s(Translations::pLanguage[3], FinalAlertConfig::pLastRead);
    strcat_s(Translations::pLanguage[0], "-StringsRA2");
    strcat_s(Translations::pLanguage[1], "-TranslationsRA2");
    strcat_s(Translations::pLanguage[2], "-Strings");
    strcat_s(Translations::pLanguage[3], "-Translations");

    if (!Translations::FADialog) {
        Translations::FADialog = MakeGameUnique<CINI>();
    }
    FString path = exePath;
    path += "\\FADialog.ini";
    Translations::FADialog->ClearAndLoad(path);

    FString stringTable;
    stringTable.Format("%s-StringTable", FinalAlertConfig::pLastRead);
    if (auto pSection = Translations::FADialog->GetSection(stringTable))
    {
        for (const auto& [key, value] : pSection->GetEntities())
        {
            FString value2 = value;
            value2.Replace("\\n", "\n");
            value2.Replace("\\t", "\t");
            value2.Replace("\\r", "\r");
            Translations::TranslateStringVariables(9, value2, __str(PROGRAM_TITLE));
            Translations::StringTable[atoi(key)] = value2;
        }
    }

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

static int hexCharToInt(wchar_t c) {
    if (c >= L'0' && c <= L'9') return c - L'0';
    if (c >= L'a' && c <= L'f') return 10 + (c - L'a');
    if (c >= L'A' && c <= L'F') return 10 + (c - L'A');
    return -1;
}

static void UnescapeSlashX(std::wstring& input) {
    std::wstring out;
    out.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '\\' && (i + 3) < input.size() && input[i + 1] == 'x') {
            int hi = hexCharToInt(input[i + 2]);
            int lo = hexCharToInt(input[i + 3]);
            if (hi >= 0 && lo >= 0) {
                unsigned char value = static_cast<unsigned char>((hi << 4) | lo);
                out.push_back(static_cast<wchar_t>(value));
                i += 3;
                continue;
            }
        }
        out.push_back(input[i]);
    }

    input = out;
}

char Translations::pLanguage[4][0x400];
FString Translations::CurrentTileSet;
std::map<HWND, int> Translations::DlgIdMap;
std::map<UINT, FString> Translations::StringTable;
std::map<int, FString> Translations::CustomTileSetNames;
std::unique_ptr<CINI, GameUniqueDeleter<CINI>> Translations::FADialog;
bool Translations::GetTranslationItem(const char* pLabelName, ppmfc::CString& ret)
{
    auto lang = CFinalSunApp::Instance->Language + "-";
    auto& falanguage = CINI::FALanguage();

    for (const auto& language : Translations::pLanguage)
    {
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

bool Translations::GetTranslationItem(const char* pLabelName, FString& ret)
{
    auto& falanguage = CINI::FALanguage();

    for (const auto& language : Translations::pLanguage)
    {
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

void Translations::TranslateStringVariables(int n, FString& text, const char* inserttext)
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

void Translations::TranslateDialog(HWND hWnd)
{
    auto itr = DlgIdMap.find(hWnd);
    if (itr != DlgIdMap.end())
    {
        int nDlgID = itr->second;
        ppmfc::CString section;
        section.Format("%s-%d", CFinalSunApp::Instance->Language, nDlgID);
        if (auto pSection = Translations::FADialog->GetSection(section))
        {
            for (const auto& [key, value] : pSection->GetEntities())
            {
                FString buffer = value;
                buffer.Replace("\\n", "\n");
                buffer.Replace("\\t", "\t");
                buffer.Replace("\\r", "\r");
                TranslateStringVariables(9, buffer, __str(PROGRAM_TITLE));
                auto wbuffer = STDHelpers::StringToWString(buffer);
                UnescapeSlashX(wbuffer);
                if (key == "Title")
                    ::SetWindowText(hWnd, value);
                else
                    ::SetDlgItemTextW(hWnd, (USHORT)atoi(key), wbuffer.c_str());
            }
        }
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    }
}

ppmfc::CString Translations::TranslateTileSet(int index)
{
    if (!CMapData::Instance->MapWidthPlusHeight)
        return "MISSING";

    if (index >= 10000)
    {
        auto itr = CustomTileSetNames.find(index);
        if (itr != CustomTileSetNames.end())
            return itr->second;
        else
            return "No Name";
    }

    ppmfc::CString setID;
    setID.Format("TileSet%04d", index);
    auto setName = CINI::CurrentTheater()->GetString(setID, "SetName", setID);
    ppmfc::CString result = TranslateOrDefault(setName, setName);

    auto lang = CFinalSunApp::Instance->Language + "-";
    auto theater = TheaterHelpers::GetCurrentSuffix();
    theater.MakeUpper();
    theater = "RenameID" + theater;
    if (auto pString = CINI::FALanguage().TryGetString(lang + theater, setID))
    {
        result = *pString;
    }
    else
    {
        if (auto pString = CINI::FALanguage().TryGetString(lang + "RenameID", setID))
        {
            result = *pString;
        }
    }

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
                    translated = StringtableLoader::QueryUIName(pair.second, true) + "(" + pair.second + ")";
                else
                    translated = StringtableLoader::QueryUIName(pair.second, true);

                src.Replace(pair.second, translated);
            }
        }
        else
        {
            for (auto& pair : countries)
            {
                if (ExtConfigs::BetterHouseNameTranslation)
                    translated = StringtableLoader::QueryUIName(pair.second, true) + "(" + pair.second + ")";
                else
                    translated = StringtableLoader::QueryUIName(pair.second, true);
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
    auto lang = CFinalSunApp::Instance->Language + "-";
    auto theater = TheaterHelpers::GetCurrentSuffix();
    theater.MakeUpper();
    theater = "RenameID" + theater;
    auto name = CINI::FALanguage().TryGetString(lang + theater, Translations::CurrentTileSet);
    if (!name) {
        name = CINI::FALanguage().TryGetString(lang + "RenameID", Translations::CurrentTileSet);
    }

    if (name) {
        R->EAX(name);
    }
    return 0;
}

DEFINE_HOOK(4F1620, CTerrainDlg_Update_SetOverlayName, 8)
{
    GET(int, index, ESI);
    auto lang = CFinalSunApp::Instance->Language + "-";
    auto theater = TheaterHelpers::GetCurrentSuffix();
    theater.MakeUpper();
    theater = "RenameID" + theater;
    const auto ovrID = Variables::RulesMap.GetValueAt("OverlayTypes", index);
    auto name = CINI::FALanguage().TryGetString(lang +  theater, ovrID);
    if (!name) {
        name = CINI::FALanguage().TryGetString(lang + "RenameID", ovrID);
    }
    
    if (name) {
        R->EAX(name);
    }
    return 0;
}

DEFINE_HOOK(412EF7, CBasic_UpdateStrings_End, 7)
{
    Translations::TranslateDialog(CFinalSunDlg::Instance->Basic.GetSafeHwnd());

    return 0;
}

DEFINE_HOOK(451283, CHouses_UpdateStrings_End, 7)
{
    Translations::TranslateDialog(CFinalSunDlg::Instance->Houses.GetSafeHwnd());

    return 0;
}

DEFINE_HOOK(499AFA, CMapD_UpdateStrings_End, 7)
{
    Translations::TranslateDialog(CFinalSunDlg::Instance->MapD.GetSafeHwnd());

    return 0;
}

DEFINE_HOOK(4DBC55, CSingleplayerSettings_UpdateStrings_End, 7)
{
    Translations::TranslateDialog(CFinalSunDlg::Instance->SingleplayerSettings.GetSafeHwnd());

    return 0;
}

DEFINE_HOOK(4DC917, CSpecialFlags_UpdateDialog_End, 7)
{
    Translations::TranslateDialog(CFinalSunDlg::Instance->SpecialFlags.GetSafeHwnd());

    return 0;
}