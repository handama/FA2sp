#include "CTerrainGenerator.h"
#include "../../FA2sp.h"
#include <Miscs/Miscs.h>
#include "../CObjectSearch/CObjectSearch.h"
#include "../../Ext/CTileSetBrowserFrame/TabPages/TeamSort.h"
#include "../../Ext/CTileSetBrowserFrame/TabPages/TaskForceSort.h"
#include "../CNewScript/CNewScript.h"
#include <numeric>
#include "../CSearhReference/CSearhReference.h"
#include "../../Miscs/MultiSelection.h"
#include "../../Miscs/DialogStyle.h"

HWND CTerrainGenerator::m_hwnd;
CTileSetBrowserFrame* CTerrainGenerator::m_parent;
CINI& CTerrainGenerator::map = CINI::CurrentDocument;
std::unique_ptr<CINI, GameUniqueDeleter<CINI>> CTerrainGenerator::ini = nullptr;
MultimapHelper& CTerrainGenerator::rules = Variables::RulesMap;
HWND CTerrainGenerator::hTab;
HWND CTerrainGenerator::hTab1Dlg;
HWND CTerrainGenerator::hTab2Dlg;
HWND CTerrainGenerator::hTab3Dlg;
HWND CTerrainGenerator::hTab4Dlg;
HWND CTerrainGenerator::hAdd;
HWND CTerrainGenerator::hName;
HWND CTerrainGenerator::hPreset;
HWND CTerrainGenerator::hDelete;
HWND CTerrainGenerator::hCopy;
HWND CTerrainGenerator::hOverride;
HWND CTerrainGenerator::hSetRange;
HWND CTerrainGenerator::hApply;
HWND CTerrainGenerator::hClear;
HWND CTerrainGenerator::hScale;
HWND CTerrainGenerator::hTileSet[TERRAIN_GENERATOR_DISPLAY];
HWND CTerrainGenerator::hTileIndexes[TERRAIN_GENERATOR_DISPLAY];
HWND CTerrainGenerator::hTileChance[TERRAIN_GENERATOR_DISPLAY];
HWND CTerrainGenerator::hOverlay[TERRAIN_GENERATOR_DISPLAY];
HWND CTerrainGenerator::hOverlayData[TERRAIN_GENERATOR_DISPLAY];
HWND CTerrainGenerator::hOverlayChance[TERRAIN_GENERATOR_DISPLAY];
HWND CTerrainGenerator::hTerrainGroup[TERRAIN_GENERATOR_DISPLAY];
HWND CTerrainGenerator::hTerrainChance[TERRAIN_GENERATOR_DISPLAY];
HWND CTerrainGenerator::hSmudgeGroup[TERRAIN_GENERATOR_DISPLAY];
HWND CTerrainGenerator::hSmudgeChance[TERRAIN_GENERATOR_DISPLAY];
std::map<int, FString> CTerrainGenerator::TileSetLabels[TERRAIN_GENERATOR_DISPLAY];
std::map<int, FString> CTerrainGenerator::OverlayLabels[TERRAIN_GENERATOR_DISPLAY];
std::map<int, FString> CTerrainGenerator::PresetLabels;
bool CTerrainGenerator::Autodrop;
bool CTerrainGenerator::DropNeedUpdate;
bool CTerrainGenerator::bOverride;
bool CTerrainGenerator::ProgrammaticallySettingText;
int CTerrainGenerator::CurrentPresetIndex;
int CTerrainGenerator::CurrentTabPage;
MapCoord CTerrainGenerator::RangeFirstCell;
MapCoord CTerrainGenerator::RangeSecondCell;
bool CTerrainGenerator::UseMultiSelection;

std::map<FString, std::shared_ptr<TerrainGeneratorPreset>> CTerrainGenerator::TerrainGeneratorPresets;
std::shared_ptr<TerrainGeneratorPreset> CTerrainGenerator::CurrentPreset;

WNDPROC CTerrainGenerator::g_pOriginalTabPageProc = nullptr;
LRESULT CALLBACK CTerrainGenerator::TabPageSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DarkTheme::MyCallWindowProcA(g_pOriginalTabPageProc, hWnd, uMsg, wParam, lParam);
}

void CTerrainGenerator::Create(CTileSetBrowserFrame* pWnd)
{
    m_parent = pWnd;
    m_hwnd = CreateDialog(
        static_cast<HINSTANCE>(FA2sp::hInstance),
        MAKEINTRESOURCE(314),
        pWnd->DialogBar.GetSafeHwnd(),
        CTerrainGenerator::DlgProc
    );

    if (m_hwnd)
        ShowWindow(m_hwnd, SW_SHOW);
    else
    {
        Logger::Error("Failed to create CTerrainGenerator.\n");
        m_parent = NULL;
        return;
    }

}

void CTerrainGenerator::Initialize(HWND& hWnd)
{
    hTab = GetDlgItem(hWnd, Controls::Tab);
    if (ExtConfigs::EnableDarkMode)
        ::SetWindowSubclass(hTab, DarkTheme::TabCtrlSubclassProc, 0, 0);
    hTab1Dlg = CreateDialog(static_cast<HINSTANCE>(FA2sp::hInstance), MAKEINTRESOURCE(315), hTab, DlgProcTab1);
    hTab2Dlg = CreateDialog(static_cast<HINSTANCE>(FA2sp::hInstance), MAKEINTRESOURCE(316), hTab, DlgProcTab2);
    hTab3Dlg = CreateDialog(static_cast<HINSTANCE>(FA2sp::hInstance), MAKEINTRESOURCE(317), hTab, DlgProcTab3);
    hTab4Dlg = CreateDialog(static_cast<HINSTANCE>(FA2sp::hInstance), MAKEINTRESOURCE(318), hTab, DlgProcTab4);
    if (ExtConfigs::EnableDarkMode)
    {
        SetWindowTheme(hTab, L"DarkMode_Explorer", NULL);
        g_pOriginalTabPageProc = (WNDPROC)GetWindowLongPtr(hTab1Dlg, GWLP_WNDPROC);
        if (g_pOriginalTabPageProc)
        {
            SetWindowLongPtr(hTab1Dlg, GWLP_WNDPROC, (LONG_PTR)TabPageSubclassProc);
        }
        g_pOriginalTabPageProc = (WNDPROC)GetWindowLongPtr(hTab2Dlg, GWLP_WNDPROC);
        if (g_pOriginalTabPageProc)
        {
            SetWindowLongPtr(hTab2Dlg, GWLP_WNDPROC, (LONG_PTR)TabPageSubclassProc);
        }
        g_pOriginalTabPageProc = (WNDPROC)GetWindowLongPtr(hTab3Dlg, GWLP_WNDPROC);
        if (g_pOriginalTabPageProc)
        {
            SetWindowLongPtr(hTab3Dlg, GWLP_WNDPROC, (LONG_PTR)TabPageSubclassProc);
        }
        g_pOriginalTabPageProc = (WNDPROC)GetWindowLongPtr(hTab4Dlg, GWLP_WNDPROC);
        if (g_pOriginalTabPageProc)
        {
            SetWindowLongPtr(hTab4Dlg, GWLP_WNDPROC, (LONG_PTR)TabPageSubclassProc);
        }
    }

    TCITEM tie;
    tie.mask = TCIF_TEXT;
    
    FString tabText = _T(Translations::TranslateOrDefault("CTerrainGenerator.Tiles", "Tiles"));
    tie.pszText = tabText.GetBuffer();
    TabCtrl_InsertItem(hTab, 0, &tie);
    
    FString tabText2 = _T(Translations::TranslateOrDefault("CTerrainGenerator.TerrainTypes", "Terrain Types"));
    tie.pszText = tabText2.GetBuffer();
    TabCtrl_InsertItem(hTab, 1, &tie);
    
    FString tabText3 = _T(Translations::TranslateOrDefault("CTerrainGenerator.Overlays", "Overlays"));
    tie.pszText = tabText3.GetBuffer();
    TabCtrl_InsertItem(hTab, 2, &tie);
    
    FString tabText4 = _T(Translations::TranslateOrDefault("CTerrainGenerator.Smudges", "Smudges"));
    tie.pszText = tabText4.GetBuffer();
    TabCtrl_InsertItem(hTab, 3, &tie);

    tabText.ReleaseBuffer();
    tabText2.ReleaseBuffer();
    tabText3.ReleaseBuffer();
    tabText4.ReleaseBuffer();
    
    FString buffer;
    if (Translations::GetTranslationItem("CTerrainGenerator.Title", buffer))
        SetWindowText(hWnd, buffer);

    auto Translate = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName, HWND parent)
        {
            HWND hTarget = parent == NULL ? GetDlgItem(hWnd, nIDDlgItem) : GetDlgItem(parent, nIDDlgItem);
            if (Translations::GetTranslationItem(pLabelName, buffer))
                SetWindowText(hTarget, buffer);
        };
    auto TranslateOrDefault = [&hWnd, &buffer](int nIDDlgItem, const char* pLabelName, const char* pDefault)
        {
            HWND hTarget = GetDlgItem(hWnd, nIDDlgItem);
            SetWindowText(hTarget, Translations::TranslateOrDefault(pLabelName, pDefault));
        };

    TranslateOrDefault(1011, "CTerrainGenerator.Description", 
        "First create or select a preset, then click \"Set Range\" to select a rectangular on the map. MultiSelection is also supported."
        " Click \"Apply\" to generate the terrain. Click \"Clear\" to clear the objects in the corresponding"
        " tabs in the range. Check \"Override\" to perform a clear each time you click Apply."
        " \"Scale\"controls the fineness of the generated tile, smaller means more fragmented tiles,"
        "the recommended value is around 30. Hidden cells will be skipped.");

    Translate(2000, "CTerrainGenerator.TileSet1", hTab1Dlg);
    Translate(2006, "CTerrainGenerator.TileSet2", hTab1Dlg);
    Translate(2012, "CTerrainGenerator.TileSet3", hTab1Dlg);
    Translate(2018, "CTerrainGenerator.TileSet4", hTab1Dlg);
    Translate(2024, "CTerrainGenerator.TileSet5", hTab1Dlg);
    Translate(2002, "CTerrainGenerator.TileIndexes", hTab1Dlg);
    Translate(2008, "CTerrainGenerator.TileIndexes", hTab1Dlg);
    Translate(2014, "CTerrainGenerator.TileIndexes", hTab1Dlg);
    Translate(2020, "CTerrainGenerator.TileIndexes", hTab1Dlg);
    Translate(2026, "CTerrainGenerator.TileIndexes", hTab1Dlg);
    Translate(2004, "CTerrainGenerator.Chance", hTab1Dlg);
    Translate(2010, "CTerrainGenerator.Chance", hTab1Dlg);
    Translate(2016, "CTerrainGenerator.Chance", hTab1Dlg);
    Translate(2022, "CTerrainGenerator.Chance", hTab1Dlg);
    Translate(2028, "CTerrainGenerator.Chance", hTab1Dlg);

    Translate(4000, "CTerrainGenerator.Overlay1", hTab3Dlg);
    Translate(4006, "CTerrainGenerator.Overlay2", hTab3Dlg);
    Translate(4012, "CTerrainGenerator.Overlay3", hTab3Dlg);
    Translate(4018, "CTerrainGenerator.Overlay4", hTab3Dlg);
    Translate(4024, "CTerrainGenerator.Overlay5", hTab3Dlg);
    Translate(4002, "CTerrainGenerator.OverlayIndexes", hTab3Dlg);
    Translate(4008, "CTerrainGenerator.OverlayIndexes", hTab3Dlg);
    Translate(4014, "CTerrainGenerator.OverlayIndexes", hTab3Dlg);
    Translate(4020, "CTerrainGenerator.OverlayIndexes", hTab3Dlg);
    Translate(4026, "CTerrainGenerator.OverlayIndexes", hTab3Dlg);
    Translate(4004, "CTerrainGenerator.Chance", hTab3Dlg);
    Translate(4010, "CTerrainGenerator.Chance", hTab3Dlg);
    Translate(4016, "CTerrainGenerator.Chance", hTab3Dlg);
    Translate(4022, "CTerrainGenerator.Chance", hTab3Dlg);
    Translate(4028, "CTerrainGenerator.Chance", hTab3Dlg);

    Translate(3000, "CTerrainGenerator.Terrain1", hTab2Dlg);
    Translate(3004, "CTerrainGenerator.Terrain2", hTab2Dlg);
    Translate(3008, "CTerrainGenerator.Terrain3", hTab2Dlg);
    Translate(3012, "CTerrainGenerator.Terrain4", hTab2Dlg);
    Translate(3016, "CTerrainGenerator.Terrain5", hTab2Dlg);
    Translate(3002, "CTerrainGenerator.Chance", hTab2Dlg);
    Translate(3006, "CTerrainGenerator.Chance", hTab2Dlg);
    Translate(3010, "CTerrainGenerator.Chance", hTab2Dlg);
    Translate(3014, "CTerrainGenerator.Chance", hTab2Dlg);
    Translate(3018, "CTerrainGenerator.Chance", hTab2Dlg);

    Translate(5000, "CTerrainGenerator.Smudge1", hTab4Dlg);
    Translate(5004, "CTerrainGenerator.Smudge2", hTab4Dlg);
    Translate(5008, "CTerrainGenerator.Smudge3", hTab4Dlg);
    Translate(5012, "CTerrainGenerator.Smudge4", hTab4Dlg);
    Translate(5016, "CTerrainGenerator.Smudge5", hTab4Dlg);
    Translate(5002, "CTerrainGenerator.Chance", hTab4Dlg);
    Translate(5006, "CTerrainGenerator.Chance", hTab4Dlg);
    Translate(5010, "CTerrainGenerator.Chance", hTab4Dlg);
    Translate(5014, "CTerrainGenerator.Chance", hTab4Dlg);
    Translate(5018, "CTerrainGenerator.Chance", hTab4Dlg);

    Translate(1001, "CTerrainGenerator.Add", NULL);
    Translate(1002, "CTerrainGenerator.Name", NULL);
    Translate(1004, "CTerrainGenerator.Preset", NULL);
    Translate(1006, "CTerrainGenerator.Delete", NULL);
    Translate(1007, "CTerrainGenerator.SetRange", NULL);
    Translate(1008, "CTerrainGenerator.Apply", NULL);
    Translate(1009, "CTerrainGenerator.Clear", NULL);
    Translate(1010, "CTerrainGenerator.Override", NULL);
    Translate(1012, "CTerrainGenerator.Scale", NULL);
    Translate(1014, "CTerrainGenerator.Copy", NULL);

    hAdd = GetDlgItem(hWnd, Controls::Add);
    hName = GetDlgItem(hWnd, Controls::Name);
    hPreset = GetDlgItem(hWnd, Controls::Preset);
    hDelete = GetDlgItem(hWnd, Controls::Delete);
    hCopy = GetDlgItem(hWnd, Controls::Copy);
    hOverride = GetDlgItem(hWnd, Controls::Override);
    hSetRange = GetDlgItem(hWnd, Controls::SetRange);
    hApply = GetDlgItem(hWnd, Controls::Apply);
    hClear = GetDlgItem(hWnd, Controls::Clear);
    hScale = GetDlgItem(hWnd, Controls::Scale);
    hTileSet[0] = GetDlgItem(hTab1Dlg, Controls::TileSet1);
    hTileSet[1] = GetDlgItem(hTab1Dlg, Controls::TileSet2);
    hTileSet[2] = GetDlgItem(hTab1Dlg, Controls::TileSet3);
    hTileSet[3] = GetDlgItem(hTab1Dlg, Controls::TileSet4);
    hTileSet[4] = GetDlgItem(hTab1Dlg, Controls::TileSet5);
    hTileIndexes[0] = GetDlgItem(hTab1Dlg, Controls::TileIndexes1);
    hTileIndexes[1] = GetDlgItem(hTab1Dlg, Controls::TileIndexes2);
    hTileIndexes[2] = GetDlgItem(hTab1Dlg, Controls::TileIndexes3);
    hTileIndexes[3] = GetDlgItem(hTab1Dlg, Controls::TileIndexes4);
    hTileIndexes[4] = GetDlgItem(hTab1Dlg, Controls::TileIndexes5);
    hTileChance[0] = GetDlgItem(hTab1Dlg, Controls::TileChance1);
    hTileChance[1] = GetDlgItem(hTab1Dlg, Controls::TileChance2);
    hTileChance[2] = GetDlgItem(hTab1Dlg, Controls::TileChance3);
    hTileChance[3] = GetDlgItem(hTab1Dlg, Controls::TileChance4);
    hTileChance[4] = GetDlgItem(hTab1Dlg, Controls::TileChance5);
    hOverlay[0] = GetDlgItem(hTab3Dlg, Controls::Overlay1);
    hOverlay[1] = GetDlgItem(hTab3Dlg, Controls::Overlay2);
    hOverlay[2] = GetDlgItem(hTab3Dlg, Controls::Overlay3);
    hOverlay[3] = GetDlgItem(hTab3Dlg, Controls::Overlay4);
    hOverlay[4] = GetDlgItem(hTab3Dlg, Controls::Overlay5);
    hOverlayData[0] = GetDlgItem(hTab3Dlg, Controls::OverlayData1);
    hOverlayData[1] = GetDlgItem(hTab3Dlg, Controls::OverlayData2);
    hOverlayData[2] = GetDlgItem(hTab3Dlg, Controls::OverlayData3);
    hOverlayData[3] = GetDlgItem(hTab3Dlg, Controls::OverlayData4);
    hOverlayData[4] = GetDlgItem(hTab3Dlg, Controls::OverlayData5);
    hOverlayChance[0] = GetDlgItem(hTab3Dlg, Controls::OverlayChance1);
    hOverlayChance[1] = GetDlgItem(hTab3Dlg, Controls::OverlayChance2);
    hOverlayChance[2] = GetDlgItem(hTab3Dlg, Controls::OverlayChance3);
    hOverlayChance[3] = GetDlgItem(hTab3Dlg, Controls::OverlayChance4);
    hOverlayChance[4] = GetDlgItem(hTab3Dlg, Controls::OverlayChance5);
    hTerrainGroup[0] = GetDlgItem(hTab2Dlg, Controls::TerrainGroup1);
    hTerrainGroup[1] = GetDlgItem(hTab2Dlg, Controls::TerrainGroup2);
    hTerrainGroup[2] = GetDlgItem(hTab2Dlg, Controls::TerrainGroup3);
    hTerrainGroup[3] = GetDlgItem(hTab2Dlg, Controls::TerrainGroup4);
    hTerrainGroup[4] = GetDlgItem(hTab2Dlg, Controls::TerrainGroup5);
    hTerrainChance[0] = GetDlgItem(hTab2Dlg, Controls::TerrainChance1);
    hTerrainChance[1] = GetDlgItem(hTab2Dlg, Controls::TerrainChance2);
    hTerrainChance[2] = GetDlgItem(hTab2Dlg, Controls::TerrainChance3);
    hTerrainChance[3] = GetDlgItem(hTab2Dlg, Controls::TerrainChance4);
    hTerrainChance[4] = GetDlgItem(hTab2Dlg, Controls::TerrainChance5);
    hSmudgeGroup[0] = GetDlgItem(hTab4Dlg, Controls::SmudgeGroup1);
    hSmudgeGroup[1] = GetDlgItem(hTab4Dlg, Controls::SmudgeGroup2);
    hSmudgeGroup[2] = GetDlgItem(hTab4Dlg, Controls::SmudgeGroup3);
    hSmudgeGroup[3] = GetDlgItem(hTab4Dlg, Controls::SmudgeGroup4);
    hSmudgeGroup[4] = GetDlgItem(hTab4Dlg, Controls::SmudgeGroup5);
    hSmudgeChance[0] = GetDlgItem(hTab4Dlg, Controls::SmudgeChance1);
    hSmudgeChance[1] = GetDlgItem(hTab4Dlg, Controls::SmudgeChance2);
    hSmudgeChance[2] = GetDlgItem(hTab4Dlg, Controls::SmudgeChance3);
    hSmudgeChance[3] = GetDlgItem(hTab4Dlg, Controls::SmudgeChance4);
    hSmudgeChance[4] = GetDlgItem(hTab4Dlg, Controls::SmudgeChance5);

    bOverride = true;
    ProgrammaticallySettingText = false;

    ShowTabPage(hWnd, 0);
    Update(hWnd);
}

void CTerrainGenerator::Update(HWND& hWnd)
{
    DropNeedUpdate = false;
    CurrentPreset = nullptr;
    TerrainGeneratorPresets.clear();
    HWND hParent = m_parent->DialogBar.GetSafeHwnd();
    HWND hTileComboBox = GetDlgItem(hParent, 1366);
    //HWND hOverlayComboBox = GetDlgItem(hParent, 1367);
    int nTileCount = SendMessage(hTileComboBox, CB_GETCOUNT, NULL, NULL);
    //int nOverlayCount = SendMessage(hOverlayComboBox, CB_GETCOUNT, NULL, NULL);
    char buffer[512] = { 0 };

    while (SendMessage(hTileSet[0], CB_DELETESTRING, 0, NULL) != CB_ERR);
    while (SendMessage(hTileSet[1], CB_DELETESTRING, 0, NULL) != CB_ERR);
    while (SendMessage(hTileSet[2], CB_DELETESTRING, 0, NULL) != CB_ERR);
    while (SendMessage(hTileSet[3], CB_DELETESTRING, 0, NULL) != CB_ERR);
    while (SendMessage(hTileSet[4], CB_DELETESTRING, 0, NULL) != CB_ERR);
    if (nTileCount > 0) {
        int idx;
        for (idx = 0; idx < nTileCount; ++idx)
        {
            SendMessage(hTileComboBox, CB_GETLBTEXT, idx, (LPARAM)(LPCSTR)buffer);
            SendMessage(hTileSet[0], CB_INSERTSTRING, idx, (LPARAM)(LPCSTR)buffer);
            SendMessage(hTileSet[1], CB_INSERTSTRING, idx, (LPARAM)(LPCSTR)buffer);
            SendMessage(hTileSet[2], CB_INSERTSTRING, idx, (LPARAM)(LPCSTR)buffer);
            SendMessage(hTileSet[3], CB_INSERTSTRING, idx, (LPARAM)(LPCSTR)buffer);
            SendMessage(hTileSet[4], CB_INSERTSTRING, idx, (LPARAM)(LPCSTR)buffer);
        }
        SendMessage(hTileSet[0], CB_INSERTSTRING, idx, (LPARAM)(LPCSTR)"<none>");
        SendMessage(hTileSet[1], CB_INSERTSTRING, idx, (LPARAM)(LPCSTR)"<none>");
        SendMessage(hTileSet[2], CB_INSERTSTRING, idx, (LPARAM)(LPCSTR)"<none>");
        SendMessage(hTileSet[3], CB_INSERTSTRING, idx, (LPARAM)(LPCSTR)"<none>");
        SendMessage(hTileSet[4], CB_INSERTSTRING, idx, (LPARAM)(LPCSTR)"<none>");
    }
    SendMessage(hOverride, BM_SETCHECK, bOverride, 0);

    if (!ini) {
        ini = MakeGameUnique<CINI>();
    }
    FString path = CFinalSunApp::ExePath();
    path += "\\TerrainGenerator.ini";
    ini->ClearAndLoad(path);
    auto itr = ini->Dict.begin();
    for (size_t i = 0, sz = ini->Dict.size(); i < sz; ++i, ++itr)
    {
        std::shared_ptr<TerrainGeneratorPreset> preset(TerrainGeneratorPreset::create(itr->first, &itr->second));
        auto currentTheater = map.GetString("Map", "Theater");
        bool skip = true;
        for (const auto& t : preset->Theaters) {
            if (t == currentTheater)
                skip = false;
        }
        if (skip)
            continue;
        TerrainGeneratorPresets[preset->ID] = std::move(preset);
    }

    SortPresets();

    int count = SendMessage(hPreset, CB_GETCOUNT, NULL, NULL);
    if (CurrentPresetIndex < 0)
        CurrentPresetIndex = 0;
    if (CurrentPresetIndex > count - 1)
        CurrentPresetIndex = count - 1;
    SendMessage(hPreset, CB_SETCURSEL, CurrentPresetIndex, NULL);
    Autodrop = false;
    OnSelchangePreset();
}

void CTerrainGenerator::Close(HWND& hWnd)
{
    RangeFirstCell.X = -1;
    RangeFirstCell.Y = -1;
    RangeSecondCell.X = -1;
    RangeSecondCell.Y = -1;
    UseMultiSelection = false;
    if (CIsoView::CurrentCommand->Command == 0x1F)
        CIsoView::CurrentCommand->Command = 0x0;
    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);

    ini = nullptr;

    EndDialog(hWnd, NULL);

    CTerrainGenerator::m_hwnd = NULL;
    CTerrainGenerator::m_parent = NULL;

}

BOOL CALLBACK CTerrainGenerator::DlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        CTerrainGenerator::Initialize(hWnd);
        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::Preset:
            if (CODE == CBN_SELCHANGE)
                OnSelchangePreset();
            else if (CODE == CBN_DROPDOWN)
                OnSeldropdownPreset(hWnd);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangePreset(true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hPreset, PresetLabels, true);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::Name:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hName, buffer, 511);
                CurrentPreset->Name = buffer;
                DropNeedUpdate = true;
                SendMessage(hPreset, CB_DELETESTRING, CurrentPresetIndex, NULL);
                SendMessage(hPreset, CB_INSERTSTRING, CurrentPresetIndex, 
                    (LPARAM)(LPCSTR)ExtraWindow::FormatTriggerDisplayName(CurrentPreset->ID, CurrentPreset->Name));
                SendMessage(hPreset, CB_SETCURSEL, CurrentPresetIndex, NULL);
                SaveAndReloadPreset();
            }
            break;
        case Controls::Scale:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hScale, buffer, 511);
                CurrentPreset->Scale = atoi(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::Override:
            if (CODE == BN_CLICKED)
                bOverride =  SendMessage(hOverride, BM_GETCHECK, 0, 0);
            break;
        case Controls::SetRange:
            if (CODE == BN_CLICKED)
                OnClickSetRange();
            break;
        case Controls::Apply:
            if (CODE == BN_CLICKED)
                OnClickApply();
            break;
        case Controls::Add:
            if (CODE == BN_CLICKED)
                OnClickAdd();
            break;
        case Controls::Delete:
            if (CODE == BN_CLICKED)
                OnClickDelete(hWnd);
            break;
        case Controls::Copy:
            if (CODE == BN_CLICKED)
                OnClickCopy();
            break;
        case Controls::Clear:
            if (CODE == BN_CLICKED)
                OnClickApply(true);
            break;
        default:
            break;
        }
        break;
    }
    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;

        if (pnmh->idFrom == Controls::Tab && pnmh->code == TCN_SELCHANGE)
        {
            int nSel = TabCtrl_GetCurSel(pnmh->hwndFrom);
            ShowTabPage(hWnd, nSel);
        }
    }
    break;
    case WM_CLOSE:
    {
        CTerrainGenerator::Close(hWnd);
        return TRUE;
    }
    case 114514: // used for update
    {
        Update(hWnd);
        return TRUE;
    }

    }

    // Process this message through default handler
    return FALSE;
}

BOOL CALLBACK CTerrainGenerator::DlgProcTab1(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::TileSet1:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTileSet(0);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeTileSet(0, true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hTileSet[0], TileSetLabels[0], false);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::TileSet2:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTileSet(1);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeTileSet(1, true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hTileSet[1], TileSetLabels[2], false);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::TileSet3:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTileSet(2);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeTileSet(2, true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hTileSet[2], TileSetLabels[2], false);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::TileSet4:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTileSet(3);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeTileSet(3, true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hTileSet[3], TileSetLabels[3], false);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::TileSet5:
            if (CODE == CBN_SELCHANGE)
                OnSelchangeTileSet(4);
            else if (CODE == CBN_EDITCHANGE)
                OnSelchangeTileSet(4, true);
            else if (CODE == CBN_CLOSEUP)
                OnCloseupCComboBox(hTileSet[4], TileSetLabels[4], false);
            else if (CODE == CBN_SELENDOK)
                ExtraWindow::bComboLBoxSelected = true;
            break;
        case Controls::TileChance1:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hTileChance[0], buffer, 511);
                CurrentPreset->TileSets[0].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::TileChance2:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hTileChance[1], buffer, 511);
                CurrentPreset->TileSets[1].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::TileChance3:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hTileChance[2], buffer, 511);
                CurrentPreset->TileSets[2].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::TileChance4:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hTileChance[3], buffer, 511);
                CurrentPreset->TileSets[3].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::TileChance5:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hTileChance[4], buffer, 511);
                CurrentPreset->TileSets[4].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::TileIndexes1:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hTileIndexes[0], buffer, 511);
                FString text = buffer;
                auto atoms = FString::SplitString(text);
                if (!atoms.empty()) {
                    CurrentPreset->TileSetAvailableIndexesText[0] = buffer;
                    CurrentPreset->TileSets[0].HasExtraIndex = true;
                }
                else {
                    CurrentPreset->TileSetAvailableIndexesText[0] = "";
                    CurrentPreset->TileSets[0].HasExtraIndex = false;
                }
                SaveAndReloadPreset();
            }
            break;
        case Controls::TileIndexes2:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hTileIndexes[1], buffer, 511);
                FString text = buffer;
                auto atoms = FString::SplitString(text);
                if (!atoms.empty()) {
                    CurrentPreset->TileSetAvailableIndexesText[1] = buffer;
                    CurrentPreset->TileSets[1].HasExtraIndex = true;
                }
                else {
                    CurrentPreset->TileSetAvailableIndexesText[1] = "";
                    CurrentPreset->TileSets[1].HasExtraIndex = false;
                }
                SaveAndReloadPreset();
            }
            break;
        case Controls::TileIndexes3:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hTileIndexes[2], buffer, 511);
                FString text = buffer;
                auto atoms = FString::SplitString(text);
                if (!atoms.empty()) {
                    CurrentPreset->TileSetAvailableIndexesText[2] = buffer;
                    CurrentPreset->TileSets[2].HasExtraIndex = true;
                }
                else {
                    CurrentPreset->TileSetAvailableIndexesText[2] = "";
                    CurrentPreset->TileSets[2].HasExtraIndex = false;
                }
                SaveAndReloadPreset();
            }
            break;
        case Controls::TileIndexes4:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hTileIndexes[3], buffer, 511);
                FString text = buffer;
                auto atoms = FString::SplitString(text);
                if (!atoms.empty()) {
                    CurrentPreset->TileSetAvailableIndexesText[3] = buffer;
                    CurrentPreset->TileSets[3].HasExtraIndex = true;
                }
                else {
                    CurrentPreset->TileSetAvailableIndexesText[3] = "";
                    CurrentPreset->TileSets[3].HasExtraIndex = false;
                }
                SaveAndReloadPreset();
            }
            break;
        case Controls::TileIndexes5:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hTileIndexes[4], buffer, 511);
                FString text = buffer;
                auto atoms = FString::SplitString(text);
                if (!atoms.empty()) {
                    CurrentPreset->TileSetAvailableIndexesText[4] = buffer;
                    CurrentPreset->TileSets[4].HasExtraIndex = true;
                }
                else {
                    CurrentPreset->TileSetAvailableIndexesText[4] = "";
                    CurrentPreset->TileSets[4].HasExtraIndex = false;
                }
                SaveAndReloadPreset();
            }
            break;
        default:
            break;
        }
        break;
    }
    break;
    case WM_CLOSE:
    {
        return TRUE;
    }
    case 114514: // used for update
    {
        return TRUE;
    }

    }
    return FALSE;
}

BOOL CALLBACK CTerrainGenerator::DlgProcTab2(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::TerrainChance1:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hTerrainChance[0], buffer, 511);
                CurrentPreset->TerrainTypes[0].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::TerrainChance2:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hTerrainChance[1], buffer, 511);
                CurrentPreset->TerrainTypes[1].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::TerrainChance3:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hTerrainChance[2], buffer, 511);
                CurrentPreset->TerrainTypes[2].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::TerrainChance4:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hTerrainChance[3], buffer, 511);
                CurrentPreset->TerrainTypes[3].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::TerrainChance5:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hTerrainChance[4], buffer, 511);
                CurrentPreset->TerrainTypes[4].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::TerrainGroup1:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                OnEditchangeTerrain(0);
            }
            break;
        case Controls::TerrainGroup2:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                OnEditchangeTerrain(1);
            }
            break;
        case Controls::TerrainGroup3:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                OnEditchangeTerrain(2);
            }
            break;
        case Controls::TerrainGroup4:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                OnEditchangeTerrain(3);
            }
            break;
        case Controls::TerrainGroup5:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                OnEditchangeTerrain(4);
            }
            break;
        default:
            break;
        }
        break;
    }
    break;
    case WM_CLOSE:
    {
        return TRUE;
    }
    case 114514: // used for update
    {
        return TRUE;
    }

    }
    return FALSE;
}

BOOL CALLBACK CTerrainGenerator::DlgProcTab3(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::OverlayChance1:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hOverlayChance[0], buffer, 511);
                CurrentPreset->Overlays[0].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::OverlayChance2:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hOverlayChance[1], buffer, 511);
                CurrentPreset->Overlays[1].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::OverlayChance3:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hOverlayChance[2], buffer, 511);
                CurrentPreset->Overlays[2].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::OverlayChance4:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hOverlayChance[3], buffer, 511);
                CurrentPreset->Overlays[3].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::OverlayChance5:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hOverlayChance[4], buffer, 511);
                CurrentPreset->Overlays[4].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::Overlay1:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                OnEditchangeOverlay(0);
            }
            break;
        case Controls::Overlay2:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                OnEditchangeOverlay(1);
            }
            break;
        case Controls::Overlay3:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                OnEditchangeOverlay(2);
            }
            break;
        case Controls::Overlay4:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                OnEditchangeOverlay(3);
            }
            break;
        case Controls::Overlay5:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                OnEditchangeOverlay(4);
            }
            break;
        case Controls::OverlayData1:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hOverlayData[0], buffer, 511);
                FString text = buffer;
                auto atoms = FString::SplitString(text);
                if (!atoms.empty()) {
                    CurrentPreset->OverlayAvailableDataText[0] = buffer;
                    CurrentPreset->Overlays[0].HasExtraIndex = true;
                }
                else {
                    CurrentPreset->OverlayAvailableDataText[0] = "";
                    CurrentPreset->Overlays[0].HasExtraIndex = false;
                }
                SaveAndReloadPreset();
            }
            break;
        case Controls::OverlayData2:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hOverlayData[1], buffer, 511);
                FString text = buffer;
                auto atoms = FString::SplitString(text);
                if (!atoms.empty()) {
                    CurrentPreset->OverlayAvailableDataText[1] = buffer;
                    CurrentPreset->Overlays[1].HasExtraIndex = true;
                }
                else {
                    CurrentPreset->OverlayAvailableDataText[1] = "";
                    CurrentPreset->Overlays[1].HasExtraIndex = false;
                }
                SaveAndReloadPreset();
            }
            break;
        case Controls::OverlayData3:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hOverlayData[2], buffer, 511);
                FString text = buffer;
                auto atoms = FString::SplitString(text);
                if (!atoms.empty()) {
                    CurrentPreset->OverlayAvailableDataText[2] = buffer;
                    CurrentPreset->Overlays[2].HasExtraIndex = true;
                }
                else {
                    CurrentPreset->OverlayAvailableDataText[2] = "";
                    CurrentPreset->Overlays[2].HasExtraIndex = false;
                }
                SaveAndReloadPreset();
            }
            break;
        case Controls::OverlayData4:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hOverlayData[3], buffer, 511);
                FString text = buffer;
                auto atoms = FString::SplitString(text);
                if (!atoms.empty()) {
                    CurrentPreset->OverlayAvailableDataText[3] = buffer;
                    CurrentPreset->Overlays[3].HasExtraIndex = true;
                }
                else {
                    CurrentPreset->OverlayAvailableDataText[3] = "";
                    CurrentPreset->Overlays[3].HasExtraIndex = false;
                }
                SaveAndReloadPreset();
            }
            break;
        case Controls::OverlayData5:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hOverlayData[4], buffer, 511);
                FString text = buffer;
                auto atoms = FString::SplitString(text);
                if (!atoms.empty()) {
                    CurrentPreset->OverlayAvailableDataText[4] = buffer;
                    CurrentPreset->Overlays[4].HasExtraIndex = true;
                }
                else {
                    CurrentPreset->OverlayAvailableDataText[4] = "";
                    CurrentPreset->Overlays[4].HasExtraIndex = false;
                }
                SaveAndReloadPreset();
            }
            break;
        default:
            break;
        }
        break;
    }
    break;
    case WM_CLOSE:
    {
        return TRUE;
    }
    case 114514: // used for update
    {
        return TRUE;
    }

    }
    return FALSE;
}

BOOL CALLBACK CTerrainGenerator::DlgProcTab4(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        return TRUE;
    }
    case WM_COMMAND:
    {
        WORD ID = LOWORD(wParam);
        WORD CODE = HIWORD(wParam);
        switch (ID)
        {
        case Controls::SmudgeChance1:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hSmudgeChance[0], buffer, 511);
                CurrentPreset->Smudges[0].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::SmudgeChance2:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hSmudgeChance[1], buffer, 511);
                CurrentPreset->Smudges[1].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::SmudgeChance3:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hSmudgeChance[2], buffer, 511);
                CurrentPreset->Smudges[2].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::SmudgeChance4:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hSmudgeChance[3], buffer, 511);
                CurrentPreset->Smudges[3].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::SmudgeChance5:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                char buffer[512]{ 0 };
                GetWindowText(hSmudgeChance[4], buffer, 511);
                CurrentPreset->Smudges[4].Chance = atof(buffer);
                SaveAndReloadPreset();
            }
            break;
        case Controls::SmudgeGroup1:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                OnEditchangeSmudge(0);
            }
            break;
        case Controls::SmudgeGroup2:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                OnEditchangeSmudge(1);
            }
            break;
        case Controls::SmudgeGroup3:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                OnEditchangeSmudge(2);
            }
            break;
        case Controls::SmudgeGroup4:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                OnEditchangeSmudge(3);
            }
            break;
        case Controls::SmudgeGroup5:
            if (CODE == EN_CHANGE && CurrentPreset && !ProgrammaticallySettingText)
            {
                OnEditchangeSmudge(4);
            }
            break;
        default:
            break;
        }
        break;
    }
    break;
    case WM_CLOSE:
    {
        return TRUE;
    }
    case 114514: // used for update
    {
        return TRUE;
    }

    }
    return FALSE;
}

void CTerrainGenerator::OnSeldropdownPreset(HWND& hWnd)
{
    if (Autodrop)
    {
        Autodrop = false;
        return;
    }

    if (!DropNeedUpdate)
        return;

    DropNeedUpdate = false;

    FString id = "";
    if (CurrentPreset) {
        id = CurrentPreset->ID;
    }
    SortPresets(id);
}

void CTerrainGenerator::OnSelchangePreset(bool edited, bool reload)
{
    ProgrammaticallySettingText = true;
    char buffer[512]{ 0 };

    if (edited && (SendMessage(hPreset, CB_GETCOUNT, NULL, NULL) > 0 || !PresetLabels.empty()))
    {
        Autodrop = true;
        ExtraWindow::OnEditCComboBox(hPreset, PresetLabels);
        ProgrammaticallySettingText = false;
        return;
    }
    CurrentPresetIndex = SendMessage(hPreset, CB_GETCURSEL, NULL, NULL);
    if (CurrentPresetIndex < 0 || CurrentPresetIndex >= SendMessage(hPreset, CB_GETCOUNT, NULL, NULL))
    {
        CurrentPresetIndex = -1;
        CurrentPreset = nullptr;

        for (auto idx = 0; idx < TERRAIN_GENERATOR_DISPLAY; idx++) {
            SendMessage(hTileSet[idx], CB_SETCURSEL, CB_ERR, NULL);
            SendMessage(hOverlay[idx], WM_SETTEXT, 0, (LPARAM)"");
            SendMessage(hTileIndexes[idx], WM_SETTEXT, 0, (LPARAM)"");
            SendMessage(hTileChance[idx], WM_SETTEXT, 0, (LPARAM)"");
            SendMessage(hOverlayData[idx], WM_SETTEXT, 0, (LPARAM)"");
            SendMessage(hOverlayChance[idx], WM_SETTEXT, 0, (LPARAM)"");
            SendMessage(hTerrainGroup[idx], WM_SETTEXT, 0, (LPARAM)"");
            SendMessage(hTerrainChance[idx], WM_SETTEXT, 0, (LPARAM)"");
            SendMessage(hSmudgeChance[idx], WM_SETTEXT, 0, (LPARAM)"");
            SendMessage(hSmudgeGroup[idx], WM_SETTEXT, 0, (LPARAM)"");
            EnableWindow(hTileSet[idx], FALSE);
            EnableWindow(hTileIndexes[idx], FALSE);
            EnableWindow(hTileChance[idx], FALSE);
            EnableWindow(hOverlay[idx], FALSE);
            EnableWindow(hOverlayData[idx], FALSE);
            EnableWindow(hOverlayChance[idx], FALSE);
            EnableWindow(hTerrainGroup[idx], FALSE);
            EnableWindow(hTerrainChance[idx], FALSE);
            EnableWindow(hSmudgeGroup[idx], FALSE);
            EnableWindow(hSmudgeChance[idx], FALSE);
        }
        SendMessage(hName, WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hScale, WM_SETTEXT, 0, (LPARAM)"");
        ProgrammaticallySettingText = false;
        return;
    }

    SendMessage(hPreset, CB_GETLBTEXT, CurrentPresetIndex, (LPARAM)buffer);
    FString id = buffer;
    FString::TrimIndex(id);
    CurrentPreset = GetPreset(id);
    if (!CurrentPreset) {
        ProgrammaticallySettingText = false;
        return;
    }

    SendMessage(hScale, WM_SETTEXT, 0, (LPARAM)STDHelpers::IntToString(CurrentPreset->Scale).m_pchData);
    SendMessage(hName, WM_SETTEXT, 0, (LPARAM)CurrentPreset->Name);

    for (auto idx = 0; idx < TERRAIN_GENERATOR_DISPLAY; idx++) {
        SendMessage(hTileSet[idx], CB_SETCURSEL, CB_ERR, NULL);
        SendMessage(hOverlay[idx], WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hTileIndexes[idx], WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hTileChance[idx], WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hOverlayData[idx], WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hOverlayChance[idx], WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hTerrainGroup[idx], WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hTerrainChance[idx], WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hSmudgeChance[idx], WM_SETTEXT, 0, (LPARAM)"");
        SendMessage(hSmudgeGroup[idx], WM_SETTEXT, 0, (LPARAM)"");
    }

    for (auto idx = 0; idx < CurrentPreset->TileSets.size() && idx < TERRAIN_GENERATOR_DISPLAY; idx++) {
        auto& group = CurrentPreset->TileSets[idx];
        int ccbIdx = ExtraWindow::FindCBStringExactStart(hTileSet[idx], group.Items.front());
        if (ccbIdx != CB_ERR)
        {
            SendMessage(hTileSet[idx], CB_SETCURSEL, ccbIdx, NULL);
        }
        else
        {
            SendMessage(hTileSet[idx], CB_SETCURSEL, -1, NULL);
            SendMessage(hTileSet[idx], WM_SETTEXT, 0, (LPARAM)group.Items.front());
        }
        if (group.HasExtraIndex) {
            SendMessage(hTileIndexes[idx], WM_SETTEXT, 0, (LPARAM)CurrentPreset->TileSetAvailableIndexesText[idx]);
        }
        SendMessage(hTileChance[idx], WM_SETTEXT, 0, (LPARAM)DoubleToString(group.Chance, TERRAIN_GENERATOR_PRECISION));
    }
    for (auto idx = 0; idx < CurrentPreset->Overlays.size() && idx < TERRAIN_GENERATOR_DISPLAY; idx++) {
        auto& group = CurrentPreset->Overlays[idx];

        FString text = "";
        for (auto& ovr : group.Overlays) {
                text += STDHelpers::IntToString(ovr.Overlay) + ",";
        }
        SendMessage(hOverlay[idx], WM_SETTEXT, 0, (LPARAM)text.Mid(0, text.GetLength() - 1));
        if (group.HasExtraIndex) {
            SendMessage(hOverlayData[idx], WM_SETTEXT, 0, (LPARAM)CurrentPreset->OverlayAvailableDataText[idx]);
        }
        SendMessage(hOverlayChance[idx], WM_SETTEXT, 0, (LPARAM)DoubleToString(group.Chance, TERRAIN_GENERATOR_PRECISION));
    }
    for (auto idx = 0; idx < CurrentPreset->TerrainTypes.size() && idx < TERRAIN_GENERATOR_DISPLAY; idx++) {
        auto& group = CurrentPreset->TerrainTypes[idx];

        FString text = "";
        for (auto& obj : group.Items) {
                text += obj + ",";
        }
        SendMessage(hTerrainGroup[idx], WM_SETTEXT, 0, (LPARAM)text.Mid(0, text.GetLength() - 1));
        SendMessage(hTerrainChance[idx], WM_SETTEXT, 0, (LPARAM)DoubleToString(group.Chance, TERRAIN_GENERATOR_PRECISION));
    }
    for (auto idx = 0; idx < CurrentPreset->Smudges.size() && idx < TERRAIN_GENERATOR_DISPLAY; idx++) {
        auto& group = CurrentPreset->Smudges[idx];

        FString text = "";
        for (auto& obj : group.Items) {
                text += obj + ",";
        }
        SendMessage(hSmudgeGroup[idx], WM_SETTEXT, 0, (LPARAM)text.Mid(0, text.GetLength() - 1));
        SendMessage(hSmudgeChance[idx], WM_SETTEXT, 0, (LPARAM)DoubleToString(group.Chance, TERRAIN_GENERATOR_PRECISION));
    }

    EnableWindows();
    DropNeedUpdate = false;
    ProgrammaticallySettingText = false;
}

void CTerrainGenerator::OnSelchangeTileSet(int index, bool edited)
{
    if (index < 0 || TERRAIN_GENERATOR_DISPLAY <= index || !CurrentPreset) return;
    auto& hwnd = hTileSet[index];
    auto& label = TileSetLabels[index];

    int curSel = SendMessage(hwnd, CB_GETCURSEL, NULL, NULL);
    FString text;
    char buffer[512]{ 0 };
    if (edited && (SendMessage(hwnd, CB_GETCOUNT, NULL, NULL) > 0 || !label.empty()))
    {
        ExtraWindow::OnEditCComboBox(hwnd, label);
    }

    if (curSel >= 0 && curSel < SendMessage(hwnd, CB_GETCOUNT, NULL, NULL))
    {
        SendMessage(hwnd, CB_GETLBTEXT, curSel, (LPARAM)buffer);
        text = buffer;
    }
    if (edited)
    {
        GetWindowText(hwnd, buffer, 511);
        text = buffer;
    }

    if (!text)
        return;

    FString::TrimIndex(text);
    if (text == "")
        text = "<none>";

    text.Replace(",", "");

    if (text == "<none>") {
        if (!CurrentPreset->TileSets.empty() && index < CurrentPreset->TileSets.size()) {
            CurrentPreset->TileSets.erase(CurrentPreset->TileSets.begin() + index);
            CurrentPreset->TileSetAvailableIndexesText.erase(CurrentPreset->TileSetAvailableIndexesText.begin() + index);
        }
        SaveAndReloadPreset();
        OnSelchangePreset();
    }
    else
    {
        if (index == CurrentPreset->TileSets.size()) {
            TerrainGeneratorGroup group;
            group.Items.push_back(STDHelpers::IntToString(atoi(text), "%04d"));
            CurrentPreset->TileSets.push_back(group);
            CurrentPreset->TileSetAvailableIndexesText.push_back("");
            SendMessage(hTileChance[index], WM_SETTEXT, NULL, (LPARAM)"0.0");
        }
        else {
            CurrentPreset->TileSets[index].Items[0] = STDHelpers::IntToString(atoi(text), "%04d");
        }
        SaveAndReloadPreset();
    }
}

void CTerrainGenerator::OnEditchangeTerrain(int index)
{
    if (index < 0 || TERRAIN_GENERATOR_DISPLAY <= index || !CurrentPreset) return;
    auto& hwnd = hTerrainGroup[index];
    char buffer[512]{ 0 };
    GetWindowText(hwnd, buffer, 511);
    FString text = buffer;
    text.Trim();
    auto atoms = FString::SplitString(text);
    if (atoms.empty()) {
        if (!CurrentPreset->TerrainTypes.empty() && index < CurrentPreset->TerrainTypes.size()){
            CurrentPreset->TerrainTypes.erase(CurrentPreset->TerrainTypes.begin() + index);
        }
        SaveAndReloadPreset();
        OnSelchangePreset();
    }
    else {
        if (index == CurrentPreset->TerrainTypes.size()) {
            TerrainGeneratorGroup group;
            for (auto& t : atoms) {
                t.Trim();
                group.Items.push_back(t);
            }
            CurrentPreset->TerrainTypes.push_back(group);
            SendMessage(hTerrainChance[index], WM_SETTEXT, NULL, (LPARAM)"0.0");
        }
        else {
            CurrentPreset->TerrainTypes[index].Items.clear();
            for (auto& t : atoms) {
                t.Trim();
                CurrentPreset->TerrainTypes[index].Items.push_back(t);
            } 
        }
        SaveAndReloadPreset();
    }
}

void CTerrainGenerator::OnEditchangeOverlay(int index)
{
    if (index < 0 || TERRAIN_GENERATOR_DISPLAY <= index || !CurrentPreset) return;
    auto& hwnd = hOverlay[index];
    char buffer[512]{ 0 };
    GetWindowText(hwnd, buffer, 511);
    FString text = buffer;
    text.Trim();
    auto atoms = FString::SplitString(text);
    if (atoms.empty()) {
        if (!CurrentPreset->Overlays.empty() && index < CurrentPreset->Overlays.size()) {
            CurrentPreset->Overlays.erase(CurrentPreset->Overlays.begin() + index);
            CurrentPreset->OverlayAvailableDataText.erase(CurrentPreset->OverlayAvailableDataText.begin() + index);
        }
        SaveAndReloadPreset();
        OnSelchangePreset();
    }
    else {
        if (index == CurrentPreset->Overlays.size()) {
            TerrainGeneratorGroup group;
            for (auto& t : atoms) {
                t.Trim();
                TerrainGeneratorOverlay ovr;
                ovr.Overlay = atoi(t);
                group.Overlays.push_back(ovr);
            }
            CurrentPreset->Overlays.push_back(group);
            CurrentPreset->OverlayAvailableDataText.push_back("");
            SendMessage(hOverlayChance[index], WM_SETTEXT, NULL, (LPARAM)"0.0");
        }
        else {
            CurrentPreset->Overlays[index].Overlays.clear();
            for (auto& t : atoms) {
                t.Trim();
                TerrainGeneratorOverlay ovr;
                ovr.Overlay = atoi(t);
                CurrentPreset->Overlays[index].Overlays.push_back(ovr);
            } 
        }
        SaveAndReloadPreset();
    }
}

void CTerrainGenerator::OnEditchangeSmudge(int index)
{
    if (index < 0 || TERRAIN_GENERATOR_DISPLAY <= index || !CurrentPreset) return;
    auto& hwnd = hSmudgeGroup[index];
    char buffer[512]{ 0 };
    GetWindowText(hwnd, buffer, 511);
    FString text = buffer;
    text.Trim();
    auto atoms = FString::SplitString(text);
    if (atoms.empty()) {
        if (!CurrentPreset->Smudges.empty() && index < CurrentPreset->Smudges.size()) {
            CurrentPreset->Smudges.erase(CurrentPreset->Smudges.begin() + index);
        }
        SaveAndReloadPreset();
        OnSelchangePreset();
    }
    else {
        if (index == CurrentPreset->Smudges.size()) {
            TerrainGeneratorGroup group;
            for (auto& t : atoms) {
                t.Trim();
                group.Items.push_back(t);
            }
            CurrentPreset->Smudges.push_back(group);
            SendMessage(hSmudgeChance[index], WM_SETTEXT, NULL, (LPARAM)"0.0");
        }
        else {
            CurrentPreset->Smudges[index].Items.clear();
            for (auto& t : atoms) {
                t.Trim();
                CurrentPreset->Smudges[index].Items.push_back(t);
            } 
        }
        SaveAndReloadPreset();
    }
}

void CTerrainGenerator::OnClickSetRange()
{
    RangeFirstCell.X = -1;
    RangeFirstCell.Y = -1;
    RangeSecondCell.X = -1;
    RangeSecondCell.Y = -1;
    auto pIsoView = CFinalSunDlg::Instance->MyViewFrame.pIsoView;
    if (MultiSelection::SelectedCoords.size() > 0) {
        //UseMultiSelection = true;
        //::RedrawWindow(pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
    }
    else {
        UseMultiSelection = false;
        CIsoView::CurrentCommand->Command = 0x1F;
        CIsoView::CurrentCommand->Type = 0;
        CIsoView::CurrentCommand->Param = 0;
        pIsoView->CurrentCellObjectIndex = -1;
        pIsoView->CurrentCellObjectType = -1;
    }
}

void CTerrainGenerator::OnSetRangeDone()
{
    CIsoView::CurrentCommand->Command = 0x0;
    CIsoView::CurrentCommand->Type = 0;
    CIsoView::CurrentCommand->Param = 0;
    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
}

void CTerrainGenerator::OnClickAdd()
{
    for (int i = 0; i < 9999; i++) {
        FString ID;
        ID.Format("%03d", i);
        if (!ini->SectionExists(ID)) {
            ini->WriteString(ID, "Name", "New Preset");
            ini->WriteString(ID, "Scale", "25");
            ini->WriteString(ID, "Theaters", map.GetString("Map", "Theater"));

            std::shared_ptr<TerrainGeneratorPreset> preset(TerrainGeneratorPreset::create(ID, ini->GetSection(ID)));

            SendMessage(hPreset, CB_SETCURSEL, 
                SendMessage(hPreset, CB_ADDSTRING, 0, 
                    (LPARAM)(LPCSTR)ExtraWindow::FormatTriggerDisplayName(ID, preset->Name))
                , NULL);
            TerrainGeneratorPresets[ID] = std::move(preset);
            CurrentPreset = GetPreset(ID);
            SaveAndReloadPreset();
            SortPresets(ID);
            OnSelchangePreset();
            break;
        }
    }

}

void CTerrainGenerator::OnClickCopy()
{
    if (!CurrentPreset) return;
    for (int i = 0; i < 9999; i++) {
        FString ID;
        ID.Format("%03d", i);
        if (!ini->SectionExists(ID)) {
            std::shared_ptr<TerrainGeneratorPreset> preset(TerrainGeneratorPreset::create(CurrentPreset->ID, ini->GetSection(CurrentPreset->ID)));
            preset->ID = ID;
            preset->Name = ExtraWindow::GetCloneName(preset->Name);
            
            SendMessage(hPreset, CB_SETCURSEL, 
                SendMessage(hPreset, CB_ADDSTRING, 0, 
                    (LPARAM)(LPCSTR)ExtraWindow::FormatTriggerDisplayName(preset->ID, preset->Name))
                , NULL);
            TerrainGeneratorPresets[ID] = std::move(preset);
            CurrentPreset = GetPreset(ID);
            SaveAndReloadPreset();
            SortPresets(ID);
            OnSelchangePreset();
            break;
        }
    }

}

void CTerrainGenerator::OnClickDelete(HWND& hWnd)
{
    if (!CurrentPreset) return;
    int result = MessageBox(hWnd,
        Translations::TranslateOrDefault("CTerrainGeneratorDelWarn", "Are you sure that you want to delete the selected preset?"),
        Translations::TranslateOrDefault("CTerrainGeneratorDelTitle", "Delete preset"), MB_YESNO);

    if (result == IDNO)
        return;

    FString id = CurrentPreset->ID;
    ini->DeleteSection(id);
    CurrentPreset = nullptr;
    RemovePreset(id);
    SendMessage(hPreset, CB_DELETESTRING, CurrentPresetIndex, NULL);
    if (CurrentPresetIndex >= SendMessage(hPreset, CB_GETCOUNT, NULL, NULL))
        CurrentPresetIndex--;
    if (CurrentPresetIndex < 0)
        CurrentPresetIndex = 0;
    SendMessage(hPreset, CB_SETCURSEL, CurrentPresetIndex, NULL);
    FString path = CFinalSunApp::ExePath();
    path += "\\TerrainGenerator.ini";
    ini->WriteToFile(path);
    OnSelchangePreset();
}

void CTerrainGenerator::OnClickApply(bool onlyClear)
{
    if (!CurrentPreset) return;
    if (!MultiSelection::SelectedCoords.empty()) UseMultiSelection = true;

    if ((RangeFirstCell.X < 0 || RangeSecondCell.X < 0) && !UseMultiSelection) return;
    int x1, x2, y1, y2;
    if (RangeFirstCell.X < RangeSecondCell.X) {
        x1 = RangeFirstCell.X;
        x2 = RangeSecondCell.X;
    }
    else {
        x2 = RangeFirstCell.X;
        x1 = RangeSecondCell.X;
    }
    if (RangeFirstCell.Y < RangeSecondCell.Y) {
        y1 = RangeFirstCell.Y;
        y2 = RangeSecondCell.Y;
    }
    else {
        y2 = RangeFirstCell.Y;
        y1 = RangeSecondCell.Y;
    }
    if (UseMultiSelection) {
        x1 = INT_MAX;
        y1 = INT_MAX;
        x2 = 0;
        y2 = 0;
        for (const auto& coord : MultiSelection::SelectedCoords) {
            if (coord.X <= x1)
                x1 = coord.X;
            if (coord.X >= x2)
                x2 = coord.X;
            if (coord.Y <= y1)
                y1 = coord.Y;
            if (coord.Y >= y2)
                y2 = coord.Y;
        }
    }
    std::vector<std::pair<std::vector<FString>, float>> smudges;
    for (const auto& group : CurrentPreset->Smudges) {
        smudges.push_back(std::make_pair(group.Items, group.Chance));
    }
    std::vector<std::pair<std::vector<FString>, float>> terrains;
    for (const auto& group : CurrentPreset->TerrainTypes) {
        terrains.push_back(std::make_pair(group.Items, group.Chance));
    }

    int recordType = 0;
    if (!terrains.empty() && !onlyClear || (onlyClear && CurrentTabPage == 1))
        recordType |= ObjectRecord::RecordType::Terrain;
    if (!smudges.empty() && !onlyClear || (onlyClear && CurrentTabPage == 3))
        recordType |= ObjectRecord::RecordType::Smudge;

    CMapDataExt::MakeMixedRecord(x1 - 4, y1 - 4, x2 + 5, y2 + 5, recordType);

    std::vector<std::pair<std::vector<int>, float>> tiles;
    for (const auto& group : CurrentPreset->TileSets) {
        tiles.push_back(std::make_pair(group.AvailableTiles, group.Chance));  
    }
    if (!tiles.empty() && !onlyClear || (onlyClear && CurrentTabPage == 0)) {
        CMapDataExt::CreateRandomGround(x1, y1, x2, y2, CurrentPreset->Scale, tiles, bOverride, UseMultiSelection, onlyClear);
    }

    std::vector<std::pair<std::vector<TerrainGeneratorOverlay>, float>> overlays;
    for (const auto& group : CurrentPreset->Overlays) {
        overlays.push_back(std::make_pair(group.Overlays, group.Chance));
    }
    if (!overlays.empty() && !onlyClear || (onlyClear && CurrentTabPage == 2)) {
        CMapDataExt::CreateRandomOverlay(x1, y1, x2, y2, overlays, bOverride, UseMultiSelection, onlyClear);
    }

    if (!terrains.empty() && !onlyClear || (onlyClear && CurrentTabPage == 1)) {
        CMapDataExt::CreateRandomTerrain(x1, y1, x2, y2, terrains, bOverride, UseMultiSelection, onlyClear);
    }

    if (!smudges.empty() && !onlyClear || (onlyClear && CurrentTabPage == 3)) {
        CMapDataExt::CreateRandomSmudge(x1, y1, x2, y2, smudges, bOverride, UseMultiSelection, onlyClear);
    }

    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
}

void CTerrainGenerator::OnCloseupCComboBox(HWND& hWnd, std::map<int, FString>& labels, bool isComboboxSelectOnly)
{
    if (!ExtraWindow::OnCloseupCComboBox(hWnd, labels, isComboboxSelectOnly))
    {
        if (hWnd == hPreset)
        {
            OnSelchangePreset();
        }
    }
}
void CTerrainGenerator::SaveAndReloadPreset()
{
    if (!CurrentPreset) return;
    FString id = CurrentPreset->ID;
    FString path = CFinalSunApp::ExePath();
    path += "\\TerrainGenerator.ini";
    ini->WriteString(id, "Name", CurrentPreset->Name);
    ini->WriteString(id, "Scale", STDHelpers::IntToString(CurrentPreset->Scale));
    FString theaters = "";
    for (const auto& t : CurrentPreset->Theaters) {
        theaters += t + ",";
    }
    ini->WriteString(id, "Theaters", theaters.Mid(0, theaters.GetLength() - 1));
    for (auto idx = 0; idx < TERRAIN_GENERATOR_MAX; idx++) {
        auto& group = CurrentPreset->TileSets[idx];
        FString key;
        key.Format("TileSet%d", idx);
        if (idx < CurrentPreset->TileSets.size()) {
            FString value;
            value.Format("%s,%s", DoubleToString(group.Chance, TERRAIN_GENERATOR_PRECISION), STDHelpers::IntToString(atoi(group.Items[0])));
            ini->WriteString(id, key, value);
            key += "AvailableIndexes";
            if (group.HasExtraIndex) { 
                ini->WriteString(id, key, CurrentPreset->TileSetAvailableIndexesText[idx]);
            }
            else {
                ini->DeleteKey(id, key);
            }
        }
        else {
            ini->DeleteKey(id, key);
            key += "AvailableIndexes";
            ini->DeleteKey(id, key);
        }

    } 
    for (auto idx = 0; idx < TERRAIN_GENERATOR_MAX; idx++) {
        auto& group = CurrentPreset->TerrainTypes[idx];
        FString key;
        key.Format("TerrainType%d", idx);
        if (idx < CurrentPreset->TerrainTypes.size()) {
            FString value;
            value.Format("%s", DoubleToString(group.Chance, TERRAIN_GENERATOR_PRECISION));
            for (int i = 0; i < group.Items.size(); i++) {
                value += ",";
                value += group.Items[i];
            }
            ini->WriteString(id, key, value);
        }
        else {
            ini->DeleteKey(id, key);
        }
    }
    for (auto idx = 0; idx < TERRAIN_GENERATOR_MAX; idx++) {
        auto& group = CurrentPreset->Smudges[idx];
        FString key;
        key.Format("Smudge%d", idx);
        if (idx < CurrentPreset->Smudges.size()) {
            FString value;
            value.Format("%s", DoubleToString(group.Chance, TERRAIN_GENERATOR_PRECISION));
            for (int i = 0; i < group.Items.size(); i++) {
                value += ",";
                value += group.Items[i];
            }
            ini->WriteString(id, key, value);
        }
        else {
            ini->DeleteKey(id, key);
        }
    }
    for (auto idx = 0; idx < TERRAIN_GENERATOR_MAX; idx++) {
        auto& group = CurrentPreset->Overlays[idx];
        FString key;
        key.Format("Overlay%d", idx);
        if (idx < CurrentPreset->Overlays.size()) {
            FString value;
            value.Format("%s", DoubleToString(group.Chance, TERRAIN_GENERATOR_PRECISION));
            for (int i = 0; i < group.Overlays.size(); i++) {
                value += ",";
                value += STDHelpers::IntToString(group.Overlays[i].Overlay);
            }
            ini->WriteString(id, key, value);
            key += "AvailableData";
            if (group.HasExtraIndex) {
                ini->WriteString(id, key, CurrentPreset->OverlayAvailableDataText[idx]);
            }
            else {
                ini->DeleteKey(id, key);
            }
        }
        else {
            ini->DeleteKey(id, key);
            key += "AvailableData";
            ini->DeleteKey(id, key);
        }

    }

    ini->WriteToFile(path);
    CurrentPreset = nullptr;
    RemovePreset(id);
    std::shared_ptr<TerrainGeneratorPreset> preset(TerrainGeneratorPreset::create(id, ini->GetSection(id)));
    TerrainGeneratorPresets[id] = std::move(preset);
    CurrentPreset = GetPreset(id);
    EnableWindows();
}

FString CTerrainGenerator::DoubleToString(double value, int precision)
{
    FString ret;
    std::ostringstream oss;
    oss.precision(precision);
    oss << std::fixed << value;
    std::string result = oss.str();
    size_t dotPos = result.find('.'); 
    if (dotPos != std::string::npos) {
        size_t lastNonZero = result.find_last_not_of('0');
        if (lastNonZero != std::string::npos && lastNonZero > dotPos) {
            result.erase(lastNonZero + 1); 
        }
        if (result.back() == '.') {
            result.pop_back();
        }
    }
    ret = result.c_str();
    return ret;
}

void CTerrainGenerator::EnableWindows()
{
    for (auto idx = 0; idx < TERRAIN_GENERATOR_DISPLAY; idx++) {
        if (idx < CurrentPreset->TileSets.size() + 1) {
            EnableWindow(hTileSet[idx], TRUE);
        }
        else {
            EnableWindow(hTileSet[idx], FALSE);
        }
        if (idx < CurrentPreset->TileSets.size()) {
            EnableWindow(hTileIndexes[idx], TRUE);
            EnableWindow(hTileChance[idx], TRUE);
        }
        else {
            EnableWindow(hTileIndexes[idx], FALSE);
            EnableWindow(hTileChance[idx], FALSE);
        }
    }
    for (auto idx = 0; idx < TERRAIN_GENERATOR_DISPLAY; idx++) {
        if (idx < CurrentPreset->Overlays.size() + 1) {
            EnableWindow(hOverlay[idx], TRUE);
        }
        else {
            EnableWindow(hOverlay[idx], FALSE);
        }
        if (idx < CurrentPreset->Overlays.size()) {
            EnableWindow(hOverlayData[idx], TRUE);
            EnableWindow(hOverlayChance[idx], TRUE);
        }
        else {
            EnableWindow(hOverlayData[idx], FALSE);
            EnableWindow(hOverlayChance[idx], FALSE);
        }
    }
    for (auto idx = 0; idx < TERRAIN_GENERATOR_DISPLAY; idx++) {
        if (idx < CurrentPreset->TerrainTypes.size() + 1) {
            EnableWindow(hTerrainGroup[idx], TRUE);
        }
        else {
            EnableWindow(hTerrainGroup[idx], FALSE);
        }
        if (idx < CurrentPreset->TerrainTypes.size()) {
            EnableWindow(hTerrainChance[idx], TRUE);
        }
        else {
            EnableWindow(hTerrainChance[idx], FALSE);
        }
    }
    for (auto idx = 0; idx < TERRAIN_GENERATOR_DISPLAY; idx++) {
        if (idx < CurrentPreset->Smudges.size() + 1) {
            EnableWindow(hSmudgeGroup[idx], TRUE);
        }
        else {
            EnableWindow(hSmudgeGroup[idx], FALSE);
        }
        if (idx < CurrentPreset->Smudges.size()) {
            EnableWindow(hSmudgeChance[idx], TRUE);
        }
        else {
            EnableWindow(hSmudgeChance[idx], FALSE);
        }
    }
}

void CTerrainGenerator::ShowTabPage(HWND hWnd, int tabIndex)
{
    CurrentTabPage = tabIndex;
    AdjustTabPagePosition(hTab, hTab1Dlg);
    AdjustTabPagePosition(hTab, hTab2Dlg);
    AdjustTabPagePosition(hTab, hTab3Dlg);
    AdjustTabPagePosition(hTab, hTab4Dlg);
    switch (tabIndex)
    {
    case 0:
        ShowWindow(hTab1Dlg, SW_SHOW);
        ShowWindow(hTab2Dlg, SW_HIDE);
        ShowWindow(hTab3Dlg, SW_HIDE);
        ShowWindow(hTab4Dlg, SW_HIDE);
        break;
    case 1:
        ShowWindow(hTab1Dlg, SW_HIDE);
        ShowWindow(hTab2Dlg, SW_SHOW);
        ShowWindow(hTab3Dlg, SW_HIDE);
        ShowWindow(hTab4Dlg, SW_HIDE);
        break;
    case 2:
        ShowWindow(hTab1Dlg, SW_HIDE);
        ShowWindow(hTab2Dlg, SW_HIDE);
        ShowWindow(hTab3Dlg, SW_SHOW);
        ShowWindow(hTab4Dlg, SW_HIDE);
        break;
    case 3:
        ShowWindow(hTab1Dlg, SW_HIDE);
        ShowWindow(hTab2Dlg, SW_HIDE);
        ShowWindow(hTab3Dlg, SW_HIDE);
        ShowWindow(hTab4Dlg, SW_SHOW);
        break;
    }
}

void CTerrainGenerator::AdjustTabPagePosition(HWND hTab, HWND hTabPage)
{
    RECT rcTab;
    GetClientRect(hTab, &rcTab);
    
    TabCtrl_AdjustRect(hTab, FALSE, &rcTab);
    
    SetWindowPos(hTabPage, nullptr, rcTab.left, rcTab.top,
        rcTab.right - rcTab.left, rcTab.bottom - rcTab.top,
        SWP_NOZORDER | SWP_NOACTIVATE);
}


void CTerrainGenerator::SortPresets(const char* id)
{
    while (SendMessage(hPreset, CB_DELETESTRING, 0, NULL) != CB_ERR);
    std::vector<FString> labels;

    for (const auto& [id, preset] : TerrainGeneratorPresets) {
        labels.push_back(ExtraWindow::FormatTriggerDisplayName(id, preset->Name));
    }
    std::sort(labels.begin(), labels.end(), ExtraWindow::SortLabels);

    for (size_t i = 0; i < labels.size(); ++i) {
        SendMessage(hPreset, CB_INSERTSTRING, i, (LPARAM)(LPCSTR)labels[i]);
    }
    if (strcmp(id, "") != 0) {
        CurrentPresetIndex = ExtraWindow::FindCBStringExactStart(hPreset, id);
        SendMessage(hPreset, CB_SETCURSEL, CurrentPresetIndex, NULL);
    }
}

bool CTerrainGenerator::OnEnterKeyDown(HWND& hWnd)
{
    if (hWnd == hPreset)
        OnSelchangePreset(true);
    else if (hWnd == hTileSet[0])
        OnSelchangeTileSet(0, true);
    else if (hWnd == hTileSet[1])
        OnSelchangeTileSet(1, true);
    else if (hWnd == hTileSet[2])
        OnSelchangeTileSet(2, true);
    else if (hWnd == hTileSet[3])
        OnSelchangeTileSet(3, true);
    else if (hWnd == hTileSet[4])
        OnSelchangeTileSet(4, true);
    else
        return false;
    return true;
}