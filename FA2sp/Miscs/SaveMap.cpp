#include "SaveMap.h"

#include <Helpers/Macro.h>

#include <CINI.h>
#include <CFinalSunApp.h>
#include <CFinalSunDlg.h>
#include <CLoading.h>
#include <CMapData.h>

#include "../FA2sp.h"
#include "../FA2sp.Constants.h"

#include "../Algorithms/sha1.h"
#include "../Algorithms/base64.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <fstream>
#include <format>
#include <filesystem>
#include "FileWatcher.h"
#include "../Helpers/STDHelpers.h"
#include "../Algorithms/lcw.h"
#include "../Algorithms/lzo.h"
#include "../Helpers/Translations.h"
#include "../Ext/CMapData/Body.h"
#include "Palettes.h"
#include "Hooks.INI.h"
#include "../Ext/CLoading/Body.h"
#include "../ExtraWindow/CNewMMXSavingOptionsDlg/CNewMMXSavingOptionsDlg.h"
#include "../Ext/CFinalSunApp/Body.h"

std::optional<std::filesystem::file_time_type> SaveMapExt::SaveTime;



static std::string to_short_filename(const std::filesystem::path& p, const std::unordered_set<std::string>& existing) {
    FString stem = p.stem().string();
    stem.Replace(" ", "");
    std::string ext = p.extension().string();

    if (ext.size() > 4) {
        ext = ext.substr(0, 4);
    }

    if (stem.size() <= 8) {
        std::string candidate = stem + ext.c_str();
        if (!existing.count(candidate)) {
            return candidate;
        }
    }

    std::string base = stem.substr(0, 6);
    int counter = 1;

    while (true) {
        std::string candidate = base + "~" + std::to_string(counter) + ext;
        if (!existing.count(candidate)) {
            return candidate;
        }
        ++counter;
    }
}

DEFINE_HOOK(4D5505, CSaveOption_CTOR_DefaultValue, 0)
{
    int nValue = CMapData::Instance->IsMultiOnly() ? 
        ExtConfigs::SaveMap_DefaultPreviewOptionMP : 
        ExtConfigs::SaveMap_DefaultPreviewOptionSP
        ;

    R->EBX(std::clamp(nValue, 0, 2));
    
    return 0x4D550E;
}

// FA2 SaveMap is almost O(N^4), who wrote that?
DEFINE_HOOK(428D97, CFinalSunDlg_SaveMap, 7)
{
    GET(CINI*, pINI, EAX);
    GET_STACK(CFinalSunDlg*, pThis, STACK_OFFS(0x3F4, 0x36C));
    REF_STACK(ppmfc::CString, filepath, STACK_OFFS(0x3F4, -0x4));
    GET_STACK(int, previewOption, STACK_OFFS(0x3F4, 0x1AC));

    pThis->MyViewFrame.StatusBar.SetWindowText(Translations::TranslateOrDefault("SavingMap", "Saving map..."));
    pThis->MyViewFrame.StatusBar.UpdateWindow();

    SaveMapExt::SaveMap(pINI, pThis, filepath, previewOption, true, false);

    return 0x42A859;
}

DEFINE_HOOK(42B30F, CFinalSunDlg_SaveMap_SkipMapDTOR, 7)
{
    return 0x42B323;
}

DEFINE_HOOK(42B2AF, CFinalSunDlg_SaveMap_SkipDeleteFile, 7)
{
    return 0x42B2C2;
}

DEFINE_HOOK(42686A, CFinalSunDlg_SaveMap_SetDefaultExtension, 5)
{
    int defaultExtention = 1;
    
    if (ExtConfigs::SaveMap_OnlySaveMAP)
    {
        defaultExtention = 4;
    }
    else if (CMapData::Instance->IsMultiOnly() && CLoading::HasMdFile())
    {
        defaultExtention = 2;
    }
    else if (CMapData::Instance->IsMultiOnly() && !CLoading::HasMdFile())
    {
        defaultExtention = 3;
    }
    else if (!CMapData::Instance->IsMultiOnly())
    {
        defaultExtention = 4;
    }

    R->Stack<int>(STACK_OFFS(0x3CC, (0x280 + 0x14)), defaultExtention);

    return 0;
}

//ppmfc::CString filePath;
//DEFINE_HOOK(4268DC, CFinalSunDlg_SaveMap_RenameMapPath, 7)
//{
//    GET(CFinalSunDlg*, pThis, ECX);
//
//    filePath = CFinalSunApp::Instance().MapPath();
//
//    if (ExtConfigs::SaveMap_OnlySaveMAP)
//    {
//        int nExtIndex = filePath.ReverseFind('.');
//        if (nExtIndex == -1)
//            filePath += ".map";
//        else
//            filePath = filePath.Mid(0, nExtIndex) + ".map";
//    }
//    else if (ExtConfigs::SaveMap_MultiPlayOnlySaveYRM && CMapData::Instance->IsMultiOnly())
//    {
//        int nExtIndex = filePath.ReverseFind('.');
//        if (nExtIndex == -1)
//            filePath += ".yrm";
//        else
//            filePath = filePath.Mid(0, nExtIndex) + ".yrm";
//    }
//    else if (!CMapData::Instance->IsMultiOnly() && ExtConfigs::SaveMap_SinglePlayOnlySaveMAP)
//    {
//        int nExtIndex = filePath.ReverseFind('.');
//        if (nExtIndex == -1)
//            filePath += ".map";
//        else
//            filePath = filePath.Mid(0, nExtIndex) + ".map";
//    }
//
//    strcpy(CFinalSunApp::Instance().MapPath, filePath);
//
//    return 0;
//}
//
//DEFINE_HOOK(426921, CFinalSunDlg_SaveMap_RenameMapPath2, 6)
//{
//    R->Stack<LPCSTR>(STACK_OFFS(0x3CC, 0x3BC), filePath);
//    return 0;
//}

DEFINE_HOOK(42A8F5, CFinalSunDlg_SaveMap_ReplaceCopyFile, 7)
{
    REF_STACK(ppmfc::CString, filepath, STACK_OFFS(0x3F4, -0x4));

    std::ifstream fin;
    fin.open(filepath, std::ios::in | std::ios::binary);
    if (fin.is_open())
    {
        fin.close();

        if (!SaveMapExt::IsAutoSaving) {
            SaveMapExt::SaveTime = std::filesystem::last_write_time(filepath.m_pchData);
            FileWatcher::IsMapJustSaved = true;
            FileWatcher::IsSavingMap = false;
        }
        return 0x42A92D;
    }
    return 0x42A911;
}
 
DEFINE_HOOK(42B2EA, CFinalSunDlg_SaveMap_SkipStringDTOR, C)
{
    return 0x42B30F;
}

bool SaveMapExt::SaveMapSilent(FString filepath, bool panic)
{
    auto ini = &CINI::CurrentDocument;
    FString buffer;
    FString buffer2;

    if (!panic)
    {
        SaveMapExt::StopTimer();
        CIsoViewExt::SetStatusBarText(Translations::TranslateOrDefault("SavingMap", "Saving map..."));
        struct GetHeaderRect
        {
            static void Get(int& startx, int& starty, int& width, int& height)
            {
                JMP_STD(0x523DD0);
            }
        };
        if (CMapData::Instance->IsMultiOnly())
        {
            // Create [Header]
            int i;
            int wp_count = 0;
            int xw[8] = { 0,0,0,0,0,0,0,0 };
            int yw[8] = { 0,0,0,0,0,0,0,0 };
            for (i = 0; i < 8; i++)
            {
                buffer.Format("%d", i);
                if (ini->KeyExists("Waypoints", buffer))
                {
                    auto value = ini->GetString("Waypoints", buffer);
                    int x = atoi(value) / 1000;
                    int y = atoi(value) % 1000;
                    xw[wp_count] = (7680 * (y - x) / 256 + 15360) / 60;
                    yw[wp_count] = 3840 * (x + y + 1) / 256 / 30;
                    wp_count++;
                }
            }

            buffer.Format("%d", wp_count);
            ini->WriteString("Header", "NumberStartingPoints", buffer);

            for (i = 0; i < 8; i++)
            {
                buffer.Format("Waypoint%d", i + 1);
                buffer2.Format("%d,%d", xw[i], yw[i]);
                ini->WriteString("Header", buffer, buffer2);
            }

            int startx, starty, width, height;
            GetHeaderRect::Get(startx, starty, width, height);

            buffer.Format("%d", height);
            ini->WriteString("Header", "Height", buffer);
            buffer.Format("%d", width);
            ini->WriteString("Header", "Width", buffer);
            buffer.Format("%d", startx);
            ini->WriteString("Header", "StartX", buffer);
            buffer.Format("%d", starty);
            ini->WriteString("Header", "StartY", buffer);
        }

    }

    CMapData::Instance->UpdateINIFile(SaveMapFlag::UpdateMapFieldData);

    if (SaveMap(ini, CFinalSunDlg::Instance(), filepath, 2, false, panic))
    {
        if (!panic)
        {
            buffer = "Map saved as \"%1\"";
            Translations::GetTranslationItem("FileSaved", buffer);
            Translations::TranslateStringVariables(1, buffer, filepath);
            CIsoViewExt::SetStatusBarText(buffer);
        }
        SaveMapExt::ResetTimer();
        return true;
    }
    SaveMapExt::ResetTimer();
    return false;
}

bool SaveMapExt::SaveMap(CINI* pINI, CFinalSunDlg* pFinalSun, FString filepath, int previewOption, bool showDialog, bool panic)
{
    if (SaveMapExt::IsAutoSaving)
        previewOption = 2; //no preview to save time

    FileWatcher::IsSavingMap = true;

    ppmfc::CString buffer;
    buffer.Format("%d", pINI->GetInteger("FA2spVersionControl", "Version") + 1);
    pINI->WriteString("FA2spVersionControl", "Version", buffer);

    Logger::Raw("SaveMap : Now removing empty sections and keys.\n");
    std::vector<ppmfc::CString> sectionsToRemove;
    for (auto& section_pair : pINI->Dict)
    {
        buffer = section_pair.first;
        buffer.Trim();
        if (buffer.GetLength() == 0 || section_pair.second.GetEntities().size() == 0)
            sectionsToRemove.push_back(section_pair.first);

        std::vector<ppmfc::CString> keysToRemove;
        for (auto& key_pair : section_pair.second.GetEntities())
        {
            buffer = key_pair.first;
            buffer.Trim();
            if (buffer.GetLength() == 0)
                keysToRemove.push_back(key_pair.first);
        }

        for (auto& key : keysToRemove)
            pINI->DeleteKey(section_pair.first, key);

        if (section_pair.second.GetEntities().size() == 0)
            sectionsToRemove.push_back(section_pair.first);
    }
    for (auto& section : sectionsToRemove)
        pINI->DeleteSection(section);

    if (previewOption == 2)
    {
        // No preview / hidden preview.
        Logger::Raw("SaveMap : Generating a hidden map preview.\n");
        pINI->DeleteSection("Preview");
        pINI->DeleteSection("PreviewPack");
        pINI->WriteString("Preview", "Size", "0,0,1,1");
        pINI->WriteString("PreviewPack", "1", "BwADABQAAAARAAA=");
    }
    else if (previewOption == 0)
    {
        // Generate new preview.
        Logger::Raw("SaveMap : Generating a new map preview.\n");

        if (ExtConfigs::SaveMaps_BetterMapPreview && CMapData::Instance->IsMultiOnly())
        {
            auto image = std::unique_ptr<unsigned char[]>(new unsigned char[256 * 512 * 3] {0});
            auto imageLocal = std::unique_ptr<unsigned char[]>(new unsigned char[256 * 512 * 3] {0});

            auto safeColorBtye = [](int x)
            {
                if (x > 255)
                    x = 255;
                if (x < 0)
                    x = 0;
                return (byte)x;
            };
            auto heightExtraLight = [safeColorBtye](int rgb, int h, LightingStruct ret)
            {
                return safeColorBtye(rgb * (ret.Ambient - ret.Ground + ret.Level * h));
            };
            auto isSafePos = [](int x, int y)
            {
                int dPows = x * CMapData::Instance().MapWidthPlusHeight + y;
                if (dPows < CMapData::Instance().CellDataCount)
                    return true;
                return false;
            };
            auto getPos = [](int x, int y)
            {
                int dPows = x * CMapData::Instance().MapWidthPlusHeight + y;
                if (dPows < CMapData::Instance().CellDataCount)
                    return dPows;
                return 0;
            };

            std::vector<int[2]>playerLocation;

            pINI->DeleteSection("Preview");
            pINI->DeleteSection("PreviewPack");

            auto& map = CINI::CurrentDocument();
            auto thisTheater = map.GetString("Map", "Theater");

            auto tiledata = CMapDataExt::TileData;

            auto size = STDHelpers::SplitString(map.GetString("Map", "Size", "0,0,0,0"));
            auto lSize = STDHelpers::SplitString(map.GetString("Map", "LocalSize", "0,0,0,0"));

            int mapwidth = atoi(size[2]);
            int mapheight = atoi(size[3]);

            int mpL = atoi(lSize[0]);
            int mpT = atoi(lSize[1]);
            int mpW = atoi(lSize[2]);
            int mpH = atoi(lSize[3]);

            int lb = mpL * 2 - 1;
            int rb = (mpL + mpW) * 2 - 1;
            int tb = mpT - 2 - 2;
            int bb = mpT + mpH + 2 - 1;
            int lwidth = rb - lb + 1;
            int lheight = bb - tb + 1;


            auto& mapData = CMapData::Instance();

            ppmfc::CString pSize;
            pSize.Format("0,0,%d,%d", lwidth, lheight);
            pINI->WriteString("Preview", "Size", pSize);

            std::vector<MapCoord> PlayerLocations;

            for (auto& cell : CMapDataExt::CellDataExts)
            {
                cell.AroundPlayerLocation = false;
                cell.AroundHighBridge = false;
            }
            for (int i = 0; i < mapData.CellDataCount; i++)
            {
                CellDataExt& cellExt = CMapDataExt::CellDataExts[i];
                CellData& cell = mapData.CellDatas[i];
                int X = i / mapData.MapWidthPlusHeight;
                int Y = i % mapData.MapWidthPlusHeight;

                if (mapData.IsMultiOnly() && cell.Waypoint != -1)
                {
                    auto pSection = CINI::CurrentDocument->GetSection("Waypoints");
                    auto& pWP = *pSection->GetKeyAt(cell.Waypoint);
                    if (atoi(pWP) < 8)
                    {
                        bool found = false;
                        MapCoord pl;
                        pl.X = 0;
                        pl.Y = 0;

                        for (int y = 0; y < mapheight; y++)
                        {
                            for (int x = 0; x < mapwidth * 2; x++)
                            {
                                int dx = x;
                                int dy = y * 2 + x % 2;
                                int rx = (dx + dy) / 2 + 1;
                                int ry = dy - rx + mapwidth + 1;

                                if (rx == X && ry == Y)
                                {
                                    pl.X = x;
                                    pl.Y = y;
                                    found = true;
                                    break;
                                }
                            }
                            if (found)
                                break;
                        }

                        PlayerLocations.push_back(pl);
                    }
                }

                auto overlay = cell.Overlay;
                auto overlayD = cell.OverlayData;
                if (overlay == 24 || overlay == 25 || overlay == 237 || overlay == 238) //high bridge
                {
                    if (overlayD >= 0 && overlayD <= 8) //NW-SE
                    {
                        CMapDataExt::CellDataExts[getPos(X, Y)].AroundHighBridge = true;
                        CMapDataExt::CellDataExts[getPos(X, Y - 1)].AroundHighBridge = true;
                        CMapDataExt::CellDataExts[getPos(X, Y + 1)].AroundHighBridge = true;

                    }
                    else if (overlayD >= 9 && overlayD <= 17) //NE-SW
                    {
                        CMapDataExt::CellDataExts[getPos(X, Y)].AroundHighBridge = true;
                        CMapDataExt::CellDataExts[getPos(X - 1, Y)].AroundHighBridge = true;
                        CMapDataExt::CellDataExts[getPos(X + 1, Y)].AroundHighBridge = true;
                    }
                }
            }

            int index = 0;
            int index2 = 0;
            for (int y = 0; y < mapheight; y++)
            {
                for (int x = 0; x < mapwidth * 2; x++)
                {
                    int dx = x;
                    int dy = y * 2 + x % 2;
                    int rx = (dx + dy) / 2 + 1;
                    int ry = dy - rx + mapwidth + 1;

                    int dPows = rx * mapData.MapWidthPlusHeight + ry;
                    if (dPows < mapData.CellDataCount)
                    {
                        if (mapData.IsCoordInMap(ry, rx))
                        {
                            CellDataExt& cellExt = CMapDataExt::CellDataExts[dPows];
                            CellData& cell = mapData.CellDatas[dPows];
                            int tileIndex = cell.TileIndex;
                            if (tileIndex >= CMapDataExt::TileDataCount)
                                tileIndex = 0;
                            int tileSubIndex = cell.TileSubIndex;
                            if (tileSubIndex >= tiledata[tileIndex].TileBlockCount)
                                tileSubIndex = 0;

                            auto colorL = tiledata[tileIndex].TileBlockDatas[tileSubIndex].RadarColorLeft;
                            auto colorR = tiledata[tileIndex].TileBlockDatas[tileSubIndex].RadarColorRight;
                            RGBClass2 color;
                            //RadarColorLeft is BGR
                            color.R = colorL.B * (14 - cell.Height) / 14 + colorR.B * cell.Height / 14;
                            color.G = colorL.G * (14 - cell.Height) / 14 + colorR.G * cell.Height / 14;
                            color.B = colorL.R * (14 - cell.Height) / 14 + colorR.R * cell.Height / 14;

                            auto overlay = cellExt.NewOverlay;
                            auto overlayD = cell.OverlayData;
                            if (overlay != 0xFFFF)
                            {
                                auto radarColor = CMapDataExt::GetOverlayTypeData(overlay).RadarColor;
                                if (overlay == 100 || overlay == 101 || overlay == 231 || overlay == 232) //broken bridge
                                {
                                }
                                else if (overlay == 24 || overlay == 25 || overlay == 237 || overlay == 238) //high bridge
                                {
                                }
                                else
                                {
                                    color = RGB(radarColor.R, radarColor.G, radarColor.B);
                                }
                            }
                            if (cellExt.AroundHighBridge)
                                color = RGB(107, 109, 107);

                            int type = cell.TerrainType;
                            std::string name = Variables::RulesMap.GetValueAt("TerrainTypes", type).m_pchData;
                            if (!name.empty())
                            {
                                if (name.find("TREE") != std::string::npos)
                                    color = RGB(0, 194, 0);
                                else if (name.find("TIBTRE") != std::string::npos)
                                    color = RGB(10, 10, 10);
                                else
                                    color = RGB(69, 68, 69);
                            }

                            //no need to get house color
                            if (cell.Structure != -1 || cell.Infantry[0] != -1 || cell.Infantry[1] != -1 || cell.Infantry[2] != -1 || cell.Unit != -1 || cell.Aircraft != -1)
                                color = RGB(123, 125, 123);
                            if (cell.Structure != -1)
                            {
                                auto pSection = CINI::CurrentDocument->GetSection("Structures");
                                auto& pStr = *pSection->GetValueAt(cell.Structure);
                                auto atoms = STDHelpers::SplitString(pStr, 1);
                                auto& name = atoms[1];

                                if (Variables::RulesMap.GetBool(name, "NeedsEngineer"))
                                    color = RGB(215, 215, 215);
                            }

                            LightingStruct ret;
                            switch (CFinalSunDlgExt::CurrentLighting)
                            {
                            case 31001:
                            case 31002:
                            case 31003:
                                ret = LightingStruct::GetCurrentLighting();
                                break;
                            default:
                                ret.Red = 1.0f;
                                ret.Green = 1.0f;
                                ret.Blue = 1.0f;
                                ret.Ambient = 1.0f;
                                ret.Ground = 0.0f;
                                ret.Level = 0.0078125f;
                                break;
                            }

                            color.R = safeColorBtye(heightExtraLight(color.R, cell.Height, ret) * ret.Red);
                            color.G = safeColorBtye(heightExtraLight(color.G, cell.Height, ret) * ret.Green);
                            color.B = safeColorBtye(heightExtraLight(color.B, cell.Height, ret) * ret.Blue);

                            for (auto& pl : PlayerLocations)
                            {
                                if (pl.X - x <= 2 && pl.X - x >= -1 && pl.Y - y <= 2 && pl.Y - y >= -1)
                                    color = RGB(240, 0, 0);
                            }

                            byte r = (byte)color.R;
                            byte g = (byte)color.G;
                            byte b = (byte)color.B;

                            image[index++] = r;
                            image[index++] = g;
                            image[index++] = b;

                        }
                        else
                        {
                            image[index++] = 0;
                            image[index++] = 0;
                            image[index++] = 0;
                        }
                        //get localsize preview
                        if (x >= lb && x <= rb
                            && y >= tb && y <= bb)
                        {
                            imageLocal[index2++] = image[index - 3];
                            imageLocal[index2++] = image[index - 2];
                            imageLocal[index2++] = image[index - 1];
                        }
                    }
                }
            }
            auto data = lzo::compress(imageLocal.get(), sizeof(byte) * 3 * lwidth * lheight);
            data = base64::encode(data);
            pINI->WriteBase64String("PreviewPack", data.data(), data.length());
        }
        else
            CMapData::Instance->UpdateINIFile(SaveMapFlag::UpdatePreview);
    }
    else
    {
        // Do not update preview.
        Logger::Raw("SaveMap : Retaining current map preview.\n");
    }

    std::ofstream fout;
    fout.exceptions(std::ofstream::failbit | std::ofstream::badbit);

    bool saveAsUTF8 = CMapDataExt::IsUTF8File || ExtConfigs::UTF8Support_AlwaysSaveAsUTF8;

    VEHGuard v(false);
    std::filesystem::path p(filepath.c_str());
    FString ext = p.extension().string();

    CNewMMXSavingOptionsDlg dlg;
    ext.MakeLower();
    bool saveAsMMX = (ext == ".mmx" || ext == ".yro") && !SaveMapExt::IsAutoSaving;
    if (saveAsMMX)
    {
        auto oriName = pFinalSun->PKTHeader.GetString("MultiMaps", "1");
        dlg.m_Description = pFinalSun->PKTHeader.GetString(oriName, "Description");
        dlg.m_MinPlayers = pFinalSun->PKTHeader.GetString(oriName, "MinPlayers");
        dlg.m_Maxplayers = pFinalSun->PKTHeader.GetString(oriName, "MaxPlayers");

        if (showDialog && dlg.DoModal() == IDCANCEL) return false;

        pINI->WriteString("Basic", "Official", "Yes");

        std::unordered_set<std::string> used;
        filepath = (p.parent_path().string() + "\\" + to_short_filename(p, used)).c_str();
        p = std::filesystem::path(filepath.c_str());
    }

    Logger::Raw("SaveMap : Trying to save map to %s.\n", filepath);

    try
    {
        fout.open(filepath, std::ios::out | std::ios::trunc);
        if (fout.is_open())
        {
            pINI->DeleteSection("Digest");

            std::ostringstream oss;
            FString comments;

            if (ExtConfigs::SaveMap_FileEncodingComment)
            {
                comments += "; ";
                if (saveAsUTF8)
                    comments += Translations::TranslateOrDefault("SaveMap_FileEncodingComment1_UTF8", "This file is encoded as UTF8, please open it in this format");
                else
                    comments += Translations::TranslateOrDefault("SaveMap_FileEncodingComment1", "This file is encoded as ANSI/GBK, please open it in this format");

                comments += "\n";
                comments += "; ";
                comments += Translations::TranslateOrDefault("SaveMap_FileEncodingComment2", "If non ASCII characters (such as Chinese) are used");
                comments += "\n";
                comments += "; ";
                comments += Translations::TranslateOrDefault("SaveMap_FileEncodingComment3", "modifying the file with incorrect encoding will result in garbled characters");
                comments += "\n";
                comments += "\n";
            }

            comments += "; Map created with FinalAlert 2(tm) Mission Editor\n";
            comments += "; Get it at http://www.westwood.com\n";
            comments += "; note that all comments were truncated\n";
            comments += "\n";
            comments += "; This FA2 uses FA2sp created by secsome, modified by Handama & E1Elite\n";
            comments += "; Get the lastest dll at https://github.com/handama/FA2sp\n";
            comments += "; Current version : "  PRODUCT_STR  ", "  __str(HDM_PRODUCT_VERSION)  "\n\n";

            oss << comments;

            const char* includeSection = ExtConfigs::IncludeType ? "$Include" : "#include";
            auto pInclude = pINI->GetSection(includeSection);

            std::unique_ptr<CINIExt, GameUniqueDeleter<CINIExt>> includeIni;
            if (pInclude && ExtConfigs::AllowIncludes && !panic)
            {
                includeIni = MakeGameUnique<CINIExt>();
                FString buffer = " \n";

                std::queue<ppmfc::CString> currentIncludeInis;

                for (auto& pair : pInclude->GetEntities()) {
                    currentIncludeInis.push(pair.second);
                }
                includeIni->LoadINIExt((uint8_t*)buffer.data(), buffer.length(), nullptr, true, true, true, &currentIncludeInis);
            }

            auto saveSection = [&oss, &pInclude, &includeIni](INISection* pSection, FString sectionName)
            {
                bool hasInclude = includeIni && includeIni->SectionExists(sectionName);
                if (hasInclude)
                {
                    auto pIncludeSection = includeIni->GetSection(sectionName);
                    auto& keys = pIncludeSection->GetEntities();
                    bool wroteSection = false;
                    for (auto& pair : pSection->GetEntities())
                    {
                        auto itr = keys.find(pair.first);
                        if (itr == keys.end() || itr->second != pair.second)
                        {
                            if (!wroteSection)
                            {
                                oss << "[" << sectionName << "]\n";
                                wroteSection = true;
                            }
                            oss << pair.first << "=" << pair.second << "\n";
                        }
                    }
                    if (wroteSection)
                        oss << "\n";
                }
                else
                {
                    oss << "[" << sectionName << "]\n";
                    for (const auto& pair : pSection->GetEntities())
                        oss << pair.first << "=" << pair.second << "\n";
                    oss << "\n";
                }
            };

            if (!SaveMapExt::IsAutoSaving && ExtConfigs::SaveMap_PreserveINISorting)
            {
                if (!pINI->SectionExists("Header"))
                {
                    pINI->WriteString("Header", "NumberStartingPoints", "0");
                }
                for (const auto& sectionName : CMapDataExt::MapIniSectionSorting)
                {
                    if (sectionName == "Digest")
                        continue;
                    if (const auto pSection = pINI->GetSection(sectionName))
                    {
                        saveSection(pSection, sectionName);
                    }
                }
                for (auto& section : pINI->Dict)
                {
                    if (!strcmp(section.first, "Digest"))
                        continue;

                    auto it = std::find(CMapDataExt::MapIniSectionSorting.begin(), CMapDataExt::MapIniSectionSorting.end(), section.first);
                    if (it == CMapDataExt::MapIniSectionSorting.end())
                    {
                        saveSection(&section.second, section.first);
                    }
                }
            }
            else
            {
                // Add "Header" for single-player map to prevent loading error
                if (const auto pSection = pINI->GetSection("Header"))
                {
                    oss << "[Header]\n";
                    for (const auto& pair : pSection->GetEntities())
                        oss << pair.first << "=" << pair.second << "\n";
                    oss << "\n";
                }
                else if (!CMapData::Instance->IsMultiOnly())
                {
                    oss << "[Header]\n";
                    oss << "NumberStartingPoints" << "=" << "0" << "\n";
                    oss << "\n";
                }

                // Dirty fix: vanilla YR needs "Preview" and "PreviewPack" before "Map"
                // So we just put them at first.
                if (const auto pSection = pINI->GetSection("Preview"))
                {
                    oss << "[Preview]\n";
                    for (const auto& pair : pSection->GetEntities())
                        oss << pair.first << "=" << pair.second << "\n";
                    oss << "\n";
                }
                if (const auto pSection = pINI->GetSection("PreviewPack"))
                {
                    oss << "[PreviewPack]\n";
                    for (const auto& pair : pSection->GetEntities())
                        oss << pair.first << "=" << pair.second << "\n";
                    oss << "\n";
                }

                for (auto& section : pINI->Dict)
                {
                    if (!strcmp(section.first, "Preview")
                        || !strcmp(section.first, "PreviewPack")
                        || !strcmp(section.first, "Header")
                        || !strcmp(section.first, "Digest"))
                        continue;

                    saveSection(&section.second, section.first);
                }
            }

            // Generate the Digest
            unsigned char hash[20];
            const auto& hash_source = oss.str();
            SHA1::hash(hash, hash_source.data(), hash_source.length());

            char hash_value[64] = { 0 };
            sprintf_s(
                hash_value,
                "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                hash[0], hash[1], hash[2], hash[3], hash[4],
                hash[5], hash[6], hash[7], hash[8], hash[9],
                hash[10], hash[11], hash[12], hash[13], hash[14],
                hash[15], hash[16], hash[17], hash[18], hash[19]
            );
            Logger::Raw("SaveMap : Map SHA1 hash: %s\n", hash_value);

            // As sha1 hash length is only 20, the length of base64 result won't
            // go over the limitation of uublock's 70 per line. So only one row!
            oss << "[Digest]\n1=" << base64::encode(hash, 20) << "\n";

            if (saveAsMMX)
            {
                FString core = p.stem().string();
                FString MMX = core + ".map";
                FString PKT = core + ".pkt";

                auto itr = pFinalSun->PKTHeader.Dict.end();
                for (size_t i = 0, sz = pFinalSun->PKTHeader.Dict.size();
                    i < sz && itr != pFinalSun->PKTHeader.Dict.begin(); ++i) {
                    --itr;
                    itr->second.~INISection();
                    pFinalSun->PKTHeader.Dict.manual_erase(itr);
                }

                pFinalSun->PKTHeader.WriteString("MultiMaps", "1", core);
                pFinalSun->PKTHeader.WriteString(core, "Description", dlg.m_Description);
                pFinalSun->PKTHeader.WriteString(core, "CD", "2");
                pFinalSun->PKTHeader.WriteString(core, "MinPlayers", dlg.m_MinPlayers);
                pFinalSun->PKTHeader.WriteString(core, "MaxPlayers", dlg.m_Maxplayers);
                pFinalSun->PKTHeader.WriteString(core, "GameMode", pINI->GetString("Basic", "GameMode", "standard"));

                MixPacker mix;
                if (saveAsUTF8)
                {
                    FString output = oss.str();
                    output.toUTF8();
                    mix.Add(MMX, output.data(), output.size());
                }
                else
                {
                    mix.Add(MMX, oss.str().data(), oss.str().size());
                }

                std::ostringstream pkt;
                for (auto& section : pFinalSun->PKTHeader.Dict)
                {
                    pkt << "[" << section.first << "]\n";
                    for (const auto& pair : section.second.GetEntities())
                        pkt << pair.first << "=" << pair.second << "\n";
                    pkt << "\n";
                }
                mix.Add(PKT, pkt.str().data(), pkt.str().size());
                fout.close();
                if (mix.Pack(filepath))
                {
                    Logger::Raw("SaveMap : Successfully saved %u sections.\n", pINI->Dict.size());
                }
            }
            else
            {
                // Now just write the file
                if (saveAsUTF8)
                {
                    FString output = oss.str();
                    output.toUTF8();
                    fout << output;
                }
                else
                {
                    fout << oss.str();
                }
                fout.flush();
                fout.close();
                Logger::Raw("SaveMap : Successfully saved %u sections.\n", pINI->Dict.size());
            }
        }
        else
        {
            ppmfc::CString buffer;
            buffer.Format("Failed to create file %s.\n", filepath);
            Logger::Raw(buffer);
            buffer.Format(Translations::TranslateOrDefault("CannotCreateFile", "Cannot create file: %s.\n"), filepath);
            ::MessageBox(NULL, buffer, Translations::TranslateOrDefault("Error", "Error"), MB_OK | MB_ICONERROR);
            return false;
        }
    }
    catch (const std::ios_base::failure& e)
    {
        UNREFERENCED_PARAMETER(e);
        ppmfc::CString buffer;
        buffer.Format(
            Translations::TranslateOrDefault(
                "CannotCreateFile",
                "Cannot create file: %s.\n"),
            filepath
        );

        ::MessageBox(NULL, buffer,
            Translations::TranslateOrDefault("Error", "Error"),
            MB_OK | MB_ICONERROR);

        return false;
    }

    return true;
}

void SaveMapExt::ResetTimer()
{
    StopTimer();
    if (ExtConfigs::SaveMap_AutoSave_Interval >= 30)
    {
        if (Timer = SetTimer(NULL, NULL, 1000 * ExtConfigs::SaveMap_AutoSave_Interval, SaveMapCallback))
            Logger::Debug("Successfully created timer with ID = %p.\n", Timer);
        else
            Logger::Debug("Failed to create timer! Auto-save is currently unable to use!\n");
    }
}

void SaveMapExt::StopTimer()
{
    if (Timer != NULL)
    {
        KillTimer(NULL, Timer);
        Timer = NULL;
    }
}

void SaveMapExt::RemoveEarlySaves()
{
    if (ExtConfigs::SaveMap_AutoSave_MaxCount != -1)
    {
        struct FileTimeComparator
        {
            bool operator()(const FILETIME& a, const FILETIME& b) const { return CompareFileTime(&a, &b) == -1; }
        };

        std::map<FILETIME, ppmfc::CString, FileTimeComparator> m;
        auto mapName = CINI::CurrentDocument->GetString("Basic", "Name", "No Name");

        /*
        * Fix : Windows file name cannot begin with space and cannot have following characters:
        * \ / : * ? " < > |
        */
        for (int i = 0; i < mapName.GetLength(); ++i)
            if (mapName[i] == '\\' || mapName[i] == '/' || mapName[i] == ':' ||
                mapName[i] == '*' || mapName[i] == '?' || mapName[i] == '"' ||
                mapName[i] == '<' || mapName[i] == '>' || mapName[i] == '|'
                )
                mapName.SetAt(i, '-');

        const auto ext =
            !ExtConfigs::SaveMap_OnlySaveMAP && CMapData::Instance->IsMultiOnly() ?
            CLoading::HasMdFile() ?
            "yrm" :
            "mpr" :
            "map";

        ppmfc::CString buffer;
        buffer.Format("%s\\AutoSaves\\%s\\%s-*.%s",
            CFinalSunAppExt::ExePathExt,
            mapName,
            mapName,
            ext
        );

        WIN32_FIND_DATA Data;
        auto hFindData = FindFirstFile(buffer, &Data);
        while (hFindData != INVALID_HANDLE_VALUE)
        {
            m[Data.ftLastWriteTime] = Data.cFileName;
            if (!FindNextFile(hFindData, &Data))
                break;
        }

        int count = m.size() - ExtConfigs::SaveMap_AutoSave_MaxCount;
        if (count <= 0)
            return;

        auto itr = m.begin();
        while (count != 0)
        {
            buffer.Format("%s\\AutoSaves\\%s\\%s", CFinalSunAppExt::ExePathExt, mapName, itr->second);
            DeleteFile(buffer);
            ++itr;
            --count;
        }
    }
}

void CALLBACK SaveMapExt::SaveMapCallback(HWND hwnd, UINT message, UINT iTimerID, DWORD dwTime)
{
    if (!ExtConfigs::SaveMap_AutoSave)
    {
        StopTimer();
        return;
    }

    Logger::Debug("SaveMapCallback called, trying to auto save map. hwnd = %08X, message = %d, iTimerID = %d, dwTime = %d.\n",
        (int)hwnd, message, iTimerID, dwTime);

    if (!CMapData::Instance->MapWidthPlusHeight || !CMapData::Instance->FieldDataAllocated)
    {
        StopTimer();
        return;
    }
    if (CIsoView::GetInstance()->lpDDPrimarySurface->IsLost() != DD_OK)
    {
        StopTimer();
        return;
    }

    SYSTEMTIME time;
    GetLocalTime(&time);

    auto mapName = CINI::CurrentDocument->GetString("Basic", "Name", "No Name");

    /*
    * Fix : Windows file name cannot begin with space and cannot have following characters:
    * \ / : * ? " < > |
    */
    for (int i = 0; i < mapName.GetLength(); ++i)
        if (mapName[i] == '\\' || mapName[i] == '/' || mapName[i] == ':' ||
            mapName[i] == '*' || mapName[i] == '?' || mapName[i] == '"' ||
            mapName[i] == '<' || mapName[i] == '>' || mapName[i] == '|'
            )
            mapName.SetAt(i, '-');

    if (mapName == "")
        mapName = "Empty Name";

    const auto ext =
        !ExtConfigs::SaveMap_OnlySaveMAP && CMapData::Instance->IsMultiOnly() ?
        CLoading::HasMdFile() ?
        "yrm" :
        "mpr" :
        "map";

    ppmfc::CString buffer = CFinalSunAppExt::ExePathExt;
    buffer += "\\AutoSaves\\";
    CreateDirectory(buffer, nullptr);
    buffer += mapName;
    CreateDirectory(buffer, nullptr);

    buffer.Format("%s\\AutoSaves\\%s\\%s-%04d%02d%02d-%02d%02d%02d-%03d.%s",
        CFinalSunAppExt::ExePathExt,
        mapName,
        mapName,
        time.wYear, time.wMonth, time.wDay,
        time.wHour, time.wMinute, time.wSecond,
        time.wMilliseconds,
        ext
    );

    IsAutoSaving = true;
    SaveMapExt::SaveMapSilent(buffer);
    IsAutoSaving = false;

    RemoveEarlySaves();
}

bool SaveMapExt::IsAutoSaving = false;
UINT_PTR SaveMapExt::Timer = NULL;

DEFINE_HOOK(426E50, CFinalSunDlg_SaveMap_AutoSave_StopTimer, 7)
{
    SaveMapExt::StopTimer();
    return 0;
}

DEFINE_HOOK(42B3AC, CFinalSunDlg_SaveMap_AutoSave_ResetTimer, 7)
{
    SaveMapExt::ResetTimer();
    return 0;
}

DEFINE_HOOK(427949, CFinalSunDlg_SaveMap_AutoSave_SkipDialog, A)
{
    return SaveMapExt::IsAutoSaving ? 0x428CF6 : 0;
}

DEFINE_HOOK(42B294, CFinalSunDlg_SaveMap_AutoSave_SkipEditFilesMenu, 8)
{
    return SaveMapExt::IsAutoSaving ? 0x42B2AF : 0;
}

DEFINE_HOOK(437D84, CFinalSunDlg_LoadMap_StopTimer, 5)
{
    if (ExtConfigs::SaveMap_AutoSave)
        SaveMapExt::StopTimer();
    return 0;
}

DEFINE_HOOK(438D90, CFinalSunDlg_LoadMap_ResetTimer, 7)
{
    if (ExtConfigs::SaveMap_AutoSave && CMapData::Instance->MapWidthPlusHeight)
        SaveMapExt::ResetTimer();
    return 0;
}

DEFINE_HOOK(42CBE0, CFinalSunDlg_CreateMap_StopTimer, 5)
{
    if (ExtConfigs::SaveMap_AutoSave)
        SaveMapExt::StopTimer();
    return 0;
}

DEFINE_HOOK(42E18E, CFinalSunDlg_CreateMap_ResetTimer, 7)
{
    if (ExtConfigs::SaveMap_AutoSave && CMapData::Instance->MapWidthPlusHeight)
        SaveMapExt::ResetTimer();
    return 0;
}

DEFINE_HOOK(424959, CFinalSunDlg_OpenMap_SkipMMXCheck, 6)
{
    return 0x424C57;
}

DEFINE_HOOK(4375CE, CFinalSunDlg_OpenMap2_SkipMMXCheck, 6)
{
    return 0x4378DB;
}

DEFINE_HOOK(49D63A, CFinalSunDlg_LoadMap_HandleMMXFile, 5)
{
    GET(CINIExt*, ini, ESI);
    GET(const char*, pFileName, EDI);

    std::filesystem::path p(pFileName);
    FString ext = p.extension().string();
    ext.MakeLower();
    CMapDataExt::IsMMXFile = false;
    int mixIndex = -114;
    auto& manager = MixLoader::MMXHolder();
    if (ext == ".mmx" || ext == ".yro")
    {
        if (auto id = manager.LoadMixFile(pFileName))
        {
            CMapDataExt::IsMMXFile = true;
            mixIndex = id;
            Logger::Debug("CMapData::LoadMap(): Loaded %s file %s.\n", ext, pFileName);
        }
    }

    if (CMapDataExt::IsMMXFile)
    {
        auto loadFileInMix = [&manager](const char* filename, DWORD* pDwSize, int nMix) -> unsigned char*
        {
            size_t sizeM = 0;
            auto result = manager.LoadFile(filename, &sizeM, nMix);
            if (result && sizeM > 0)
            {
                auto pBuffer = GameCreateArray<unsigned char>(sizeM);
                memcpy(pBuffer, result.get(), sizeM);
                if (pDwSize)
                    *pDwSize = (DWORD)sizeM;
                return pBuffer;
            }
            return nullptr;
        };

        DWORD size = 0;
        if (auto file = loadFileInMix((p.stem().string() + ".map").c_str(), &size, mixIndex))
        {
            ini->LoadINIExt(file, size, nullptr, true, true, true);
            if (auto pkt = loadFileInMix((p.stem().string() + ".pkt").c_str(), &size, mixIndex))
            {
                ((CINIExt*)(&CFinalSunDlg::Instance->PKTHeader))->LoadINIExt(pkt, size, nullptr, true, true, false);
            }
            manager.Clear();
            return 0x49D644;
        }
        Logger::Raw("CMapData::LoadMap(): Failed to open %s, fallback to default parser.\n", p.stem().string() + ".map");
    }
    ini->ClearAndLoad(pFileName, 1);

    manager.Clear();
    return 0x49D644;
}
