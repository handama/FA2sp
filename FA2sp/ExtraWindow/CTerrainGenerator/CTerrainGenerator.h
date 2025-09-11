#pragma once

#include <FA2PP.h>
#include <map>
#include <vector>
#include <string>
#include <regex>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/FString.h"
#include "../../Helpers/MultimapHelper.h"
#include "../Common.h"
#include <CLoading.h>
#include <Drawing.h>
#include <CFinalSunDlg.h>
#include <CObjectDatas.h>
#include <CMapData.h>
#include <CIsoView.h>
#include "../../Ext/CMapData/Body.h"
#include "../../Ext/CLoading/Body.h"

#define TERRAIN_GENERATOR_MAX 10
#define TERRAIN_GENERATOR_DISPLAY 5
#define TERRAIN_GENERATOR_PRECISION 6

class TerrainGeneratorPreset 
{
public:
    FString Name;
    FString ID;
    std::vector <FString> Theaters;
    std::vector<TerrainGeneratorGroup> TileSets;
    std::vector<TerrainGeneratorGroup> TerrainTypes;
    std::vector<TerrainGeneratorGroup> Overlays;
    std::vector<TerrainGeneratorGroup> Smudges;
    std::vector<FString> TileSetAvailableIndexesText;
    std::vector<FString> OverlayAvailableDataText;
    int Scale;

    static TerrainGeneratorPreset* create(FString sectionName, INISection* pSection)
    {
        return new TerrainGeneratorPreset(sectionName, pSection);
    }

    TerrainGeneratorPreset(FString sectionName, INISection* pSection)
    {
        ID = sectionName;
        Name = pSection->GetString("Name");
        for (auto& t : FString::SplitString(pSection->GetString("Theaters"))) {
            Theaters.push_back(t);
        }
        Scale = pSection->GetInteger("Scale", 25);
        FString key;
        for (auto i = 0; i < TERRAIN_GENERATOR_MAX; i++) {
            key.Format("%s%d", "TileSet", i);
            auto atoms = FString::SplitString(pSection->GetString(key));
            if (atoms.size() > 1) {
                TerrainGeneratorGroup group;
                group.Chance = std::atof(atoms[0]);
                int tileSet = atoi(atoms[1]);
                group.Items.push_back(STDHelpers::IntToString(tileSet, "%04d"));
                if (tileSet < CMapDataExt::TileSet_starts.size() - 1);
                int start = CMapDataExt::TileSet_starts[tileSet];
                int end = CMapDataExt::TileSet_starts[tileSet + 1];

                key += "AvailableIndexes";
                TileSetAvailableIndexesText.push_back(pSection->GetString(key));
                auto atomsIdx = FString::SplitString(TileSetAvailableIndexesText.back());
                if (atomsIdx.empty()) {
                    group.HasExtraIndex = false;
                    for (auto j = start; j < end; j++) {
                        group.AvailableTiles.push_back(j);
                    }
                }
                else {
                    group.HasExtraIndex = true;
                    for (auto& idx : atomsIdx) {
                        if (start + atoi(idx) < end) {
                            group.AvailableTiles.push_back(start + atoi(idx));
                        }
                    }
                }
                TileSets.push_back(group);
            }
        }
        for (auto i = 0; i < TERRAIN_GENERATOR_MAX; i++) {
            key.Format("%s%d", "TerrainType", i);
            auto atoms = FString::SplitString(pSection->GetString(key));
            if (atoms.size() > 1) {
                TerrainGeneratorGroup group;
                group.Chance = std::atof(atoms[0]);
                for (int j = 1; j < atoms.size(); j++) {
                    group.Items.push_back(atoms[j]);
                }
                TerrainTypes.push_back(group);
            }
        }
        for (auto i = 0; i < TERRAIN_GENERATOR_MAX; i++) {
            key.Format("%s%d", "Smudge", i);
            auto atoms = FString::SplitString(pSection->GetString(key));
            if (atoms.size() > 1) {
                TerrainGeneratorGroup group;
                group.Chance = std::atof(atoms[0]);
                for (int j = 1; j < atoms.size(); j++) {
                    group.Items.push_back(atoms[j]);
                }
                Smudges.push_back(group);
            }
        }
        for (auto i = 0; i < TERRAIN_GENERATOR_MAX; i++) {
            key.Format("%s%d", "Overlay", i);
            auto atoms = FString::SplitString(pSection->GetString(key));
            key += "AvailableData";
            OverlayAvailableDataText.push_back(pSection->GetString(key));
            auto atomsData = FString::SplitString(OverlayAvailableDataText.back());

            if (atoms.size() > 1) {
                TerrainGeneratorGroup group;
                for (auto& d : atomsData) {
                    group.OverlayItems.push_back(d);
                }
                group.Chance = std::atof(atoms[0]);
                for (int j = 1; j < atoms.size(); j++) {
                    TerrainGeneratorOverlay overlays;
                    overlays.Overlay = atoi(atoms[j]);
                    group.Items.push_back(STDHelpers::IntToString(overlays.Overlay));
                    if (overlays.Overlay < 0xFFFF)
                    {
                        CLoadingExt::GetExtension()->LoadOverlay(Variables::RulesMap.GetValueAt("OverlayTypes", overlays.Overlay), overlays.Overlay);

                        if (atomsData.empty()) {
                            group.HasExtraIndex = false;
                            for (int idx = 0; idx < 60; idx++)
                            {
                                auto imageName = CLoadingExt::GetOverlayName(overlays.Overlay, idx);
                                auto pic = CLoadingExt::GetImageDataFromServer(imageName);
                                if (pic && pic->pImageBuffer) {
                                    overlays.AvailableOverlayData.push_back(idx);
                                }
                            }
                        }
                        else {
                            group.HasExtraIndex = true;
                            for (int idx = 0; idx < 60; idx++)
                            {
                                auto imageName = CLoadingExt::GetOverlayName(overlays.Overlay, idx);
                                auto pic = CLoadingExt::GetImageDataFromServer(imageName);
                                if (pic && pic->pImageBuffer) {
                                    for (auto& data : atomsData) {
                                        if (idx == atoi(data)) {
                                            overlays.AvailableOverlayData.push_back(idx);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        group.HasExtraIndex = false;
                        overlays.AvailableOverlayData.push_back(0);
                    }              
                    group.Overlays.push_back(overlays);
                }
                Overlays.push_back(group);
            }
        }
    }
};

// A static window class
class CTerrainGenerator
{
public:
    enum Controls {
        Tab = 1000,
        Add = 1001,
        Name = 1003,
        Preset = 1005,
        Delete = 1006,
        SetRange = 1007,
        Apply = 1008,
        Clear = 1009,
        Override = 1010,
        Scale = 1013,
        Copy = 1014,
        TileSet1 = 2001,
        TileSet2 = 2007,
        TileSet3 = 2013,
        TileSet4 = 2019,
        TileSet5 = 2025,
        TileIndexes1 = 2003,
        TileIndexes2 = 2009,
        TileIndexes3 = 2015,
        TileIndexes4 = 2021,
        TileIndexes5 = 2027,
        TileChance1 = 2005,
        TileChance2 = 2011,
        TileChance3 = 2017,
        TileChance4 = 2023,
        TileChance5 = 2029,
        Overlay1 = 4001,
        Overlay2 = 4007,
        Overlay3 = 4013,
        Overlay4 = 4019,
        Overlay5 = 4025,
        OverlayData1 = 4003,
        OverlayData2 = 4009,
        OverlayData3 = 4015,
        OverlayData4 = 4021,
        OverlayData5 = 4027,
        OverlayChance1 = 4005,
        OverlayChance2 = 4011,
        OverlayChance3 = 4017,
        OverlayChance4 = 4023,
        OverlayChance5 = 4029,
        TerrainGroup1 = 3001,
        TerrainGroup2 = 3005,
        TerrainGroup3 = 3009,
        TerrainGroup4 = 3013,
        TerrainGroup5 = 3017,
        TerrainChance1 = 3003,
        TerrainChance2 = 3007,
        TerrainChance3 = 3011,
        TerrainChance4 = 3015,
        TerrainChance5 = 3019,
        SmudgeGroup1 = 5001,
        SmudgeGroup2 = 5005,
        SmudgeGroup3 = 5009,
        SmudgeGroup4 = 5013,
        SmudgeGroup5 = 5017,
        SmudgeChance1 = 5003,
        SmudgeChance2 = 5007,
        SmudgeChance3 = 5011,
        SmudgeChance4 = 5015,
        SmudgeChance5 = 5019,
    };

    static void Create(CTileSetBrowserFrame* pWnd);

    static HWND GetHandle()
    {
        return CTerrainGenerator::m_hwnd;
    }
    static bool OnEnterKeyDown(HWND& hWnd);
    static void OnSetRangeDone();

protected:
    static void Initialize(HWND& hWnd);
    static void Update(HWND& hWnd);
    static void Close(HWND& hWnd);

    static BOOL CALLBACK DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK DlgProcTab1(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK DlgProcTab2(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK DlgProcTab3(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK DlgProcTab4(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
    static void ShowTabPage(HWND hWnd, int tabIndex);
    static void AdjustTabPagePosition(HWND hTab, HWND hTabPage);

    static void OnSelchangePreset(bool edited = false, bool reload = true);
    static void OnSelchangeTileSet(int index, bool edited = false);
    static void OnEditchangeTerrain(int index);
    static void OnEditchangeSmudge(int index);
    static void OnEditchangeOverlay(int index);
    static void OnSeldropdownPreset(HWND& hWnd);
    static void OnCloseupCComboBox(HWND& hWnd, std::map<int, FString>& labels, bool isComboboxSelectOnly);

    static void OnClickSetRange();
    static void EnableWindows();
    static void OnClickApply(bool onlyClear = false);
    static void OnClickAdd();
    static void OnClickCopy();
    static void OnClickDelete(HWND& hWnd);
    static void SortPresets(const char* id = "");

    static void SaveAndReloadPreset();
    static FString DoubleToString(double value, int precision);

    static std::shared_ptr<TerrainGeneratorPreset> GetPreset(FString id) {
        auto it = TerrainGeneratorPresets.find(id);
        if (it != TerrainGeneratorPresets.end()) {
            return std::shared_ptr<TerrainGeneratorPreset>(it->second.get(), [](TerrainGeneratorPreset*) {});
        }
        return nullptr;
    }
    static void RemovePreset(FString id) {
        auto it = TerrainGeneratorPresets.find(id);
        if (it != TerrainGeneratorPresets.end()) {
            TerrainGeneratorPresets.erase(id);
        }
    }

private:
    static HWND m_hwnd;
    static CTileSetBrowserFrame* m_parent;
    static HWND hTab;
    static HWND hTab1Dlg;
    static HWND hTab2Dlg;
    static HWND hTab3Dlg;
    static HWND hTab4Dlg;
    static HWND hAdd;
    static HWND hName;
    static HWND hPreset;
    static HWND hDelete;
    static HWND hCopy;
    static HWND hOverride;
    static HWND hSetRange;
    static HWND hApply;
    static HWND hClear;
    static HWND hScale;
    static HWND hTileSet[TERRAIN_GENERATOR_DISPLAY];
    static HWND hTileIndexes[TERRAIN_GENERATOR_DISPLAY];
    static HWND hTileChance[TERRAIN_GENERATOR_DISPLAY];
    static HWND hOverlay[TERRAIN_GENERATOR_DISPLAY];
    static HWND hOverlayData[TERRAIN_GENERATOR_DISPLAY];
    static HWND hOverlayChance[TERRAIN_GENERATOR_DISPLAY];
    static HWND hTerrainGroup[TERRAIN_GENERATOR_DISPLAY];
    static HWND hTerrainChance[TERRAIN_GENERATOR_DISPLAY];
    static HWND hSmudgeGroup[TERRAIN_GENERATOR_DISPLAY];
    static HWND hSmudgeChance[TERRAIN_GENERATOR_DISPLAY];

    static std::map<int, FString> TileSetLabels[TERRAIN_GENERATOR_DISPLAY];
    static std::map<int, FString> OverlayLabels[TERRAIN_GENERATOR_DISPLAY];
    static std::map<int, FString> PresetLabels;
    static bool Autodrop;
    static bool DropNeedUpdate;
    static int CurrentPresetIndex;
    static int CurrentTabPage;
    static bool bOverride;
    static bool ProgrammaticallySettingText;

    static CINI& map;
    static MultimapHelper& rules;
    static std::shared_ptr<TerrainGeneratorPreset> CurrentPreset;
    static std::map<FString, std::shared_ptr<TerrainGeneratorPreset>> TerrainGeneratorPresets;
    static WNDPROC g_pOriginalTabPageProc;
    static LRESULT CALLBACK TabPageSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    static std::unique_ptr<CINI, GameUniqueDeleter<CINI>> ini;
    static MapCoord RangeFirstCell;
    static MapCoord RangeSecondCell;
    static bool UseMultiSelection;
};