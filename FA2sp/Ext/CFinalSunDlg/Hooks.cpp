#include "Body.h"
#include <Helpers/Macro.h>
#include "../../Helpers/Translations.h"
#include "../../Helpers/TheaterHelpers.h"
#include <CInputMessageBox.h>
#include <CFinalSunApp.h>
#include <CMapData.h>
#include <algorithm>
#include "../CIsoView/Body.h"
#include "../../Miscs/DialogStyle.h"
#include "../CFinalSunApp/Body.h"
#include "../../ExtraWindow/CTerrainGenerator/CTerrainGenerator.h"
#include "../../Miscs/MultiSelection.h"
#include "../../ExtraWindow/CLuaConsole/CLuaConsole.h"

DEFINE_HOOK(424654, CFinalSunDlg_OnInitDialog_SetMenuItemStateByDefault, 7)
{
    GET(CFinalSunDlg*, pThis, ESI);

    auto pMenu = pThis->GetMenu();
    if (ExtConfigs::EnableDarkMode)
    {
        DarkTheme::EnableOwnerDrawMenu(*pMenu);
        DarkTheme::InitializeMenuOverlay(pThis->GetSafeHwnd());
    }

    pMenu->CheckMenuItem(30000, MF_CHECKED);
    pMenu->CheckMenuItem(30001, MF_CHECKED);
    pMenu->CheckMenuItem(30002, MF_CHECKED);
    pMenu->CheckMenuItem(30003, MF_CHECKED);
    pMenu->CheckMenuItem(30004, MF_CHECKED);
    pMenu->CheckMenuItem(30005, MF_CHECKED);
    pMenu->CheckMenuItem(30006, MF_CHECKED);
    pMenu->CheckMenuItem(30007, MF_CHECKED);
    pMenu->CheckMenuItem(30008, MF_CHECKED);
    pMenu->CheckMenuItem(30009, MF_CHECKED);
    pMenu->CheckMenuItem(30010, MF_CHECKED);
    pMenu->CheckMenuItem(30011, MF_CHECKED);
    pMenu->CheckMenuItem(30012, MF_CHECKED);
    pMenu->CheckMenuItem(30021, MF_CHECKED);

    pMenu->CheckMenuRadioItem(31000, 31003, CFinalSunDlgExt::CurrentLighting, MF_CHECKED);
    return 0;
}

DEFINE_HOOK(432304, CFinalSunDlg_Update_LayersVisibility, 5)
{
    GET(ppmfc::CMenu*, pMenu, ESI);

    auto SetItemCheckStatus = [&pMenu](int id, bool& param)
    {
        pMenu->CheckMenuItem(id, param ? MF_CHECKED : MF_UNCHECKED);
    };

    SetItemCheckStatus(30000, CIsoViewExt::DrawStructures);
    SetItemCheckStatus(30001, CIsoViewExt::DrawInfantries);
    SetItemCheckStatus(30002, CIsoViewExt::DrawUnits);
    SetItemCheckStatus(30003, CIsoViewExt::DrawAircrafts);
    SetItemCheckStatus(30004, CIsoViewExt::DrawBasenodes);
    SetItemCheckStatus(30005, CIsoViewExt::DrawWaypoints);
    SetItemCheckStatus(30006, CIsoViewExt::DrawCelltags);
    SetItemCheckStatus(30007, CIsoViewExt::DrawMoneyOnMap);
    SetItemCheckStatus(30008, CIsoViewExt::DrawOverlays);
    SetItemCheckStatus(30009, CIsoViewExt::DrawTerrains);
    SetItemCheckStatus(30010, CIsoViewExt::DrawSmudges);
    SetItemCheckStatus(30011, CIsoViewExt::DrawTubes);
    SetItemCheckStatus(30012, CIsoViewExt::DrawBounds);
    SetItemCheckStatus(30021, CIsoViewExt::DrawVeterancy);
    SetItemCheckStatus(30013, CIsoViewExt::DrawBaseNodeIndex);
    SetItemCheckStatus(30014, CIsoViewExt::RockCells);
    SetItemCheckStatus(30022, CIsoViewExt::DrawShadows);
    SetItemCheckStatus(30023, CIsoViewExt::DrawAlphaImages);
    SetItemCheckStatus(30015, CIsoViewExt::DrawStructuresFilter);
    SetItemCheckStatus(30016, CIsoViewExt::DrawInfantriesFilter);
    SetItemCheckStatus(30017, CIsoViewExt::DrawUnitsFilter);
    SetItemCheckStatus(30018, CIsoViewExt::DrawAircraftsFilter);
    SetItemCheckStatus(30019, CIsoViewExt::DrawBasenodesFilter);
    SetItemCheckStatus(30020, CIsoViewExt::DrawCellTagsFilter);

    SetItemCheckStatus(34001, CIsoViewExt::PasteGround);
    SetItemCheckStatus(34002, CIsoViewExt::PasteOverlays);
    SetItemCheckStatus(34003, CIsoViewExt::PasteStructures);
    SetItemCheckStatus(34004, CIsoViewExt::PasteInfantries);
    SetItemCheckStatus(34005, CIsoViewExt::PasteUnits);
    SetItemCheckStatus(34006, CIsoViewExt::PasteAircrafts);
    SetItemCheckStatus(34007, CIsoViewExt::PasteTerrains);
    SetItemCheckStatus(34008, CIsoViewExt::PasteSmudges);
    SetItemCheckStatus(34051, CIsoViewExt::PasteOverriding);
    SetItemCheckStatus(34052, CIsoViewExt::PasteShowOutline);
    SetItemCheckStatus(40159, ExtConfigs::TreeViewCameo_Display);
    SetItemCheckStatus(40163, CIsoViewExt::EnableDistanceRuler);

    SetItemCheckStatus(32000, CIsoViewExt::AutoPropertyBrush[0]);
    SetItemCheckStatus(32001, CIsoViewExt::AutoPropertyBrush[1]);
    SetItemCheckStatus(32002, CIsoViewExt::AutoPropertyBrush[2]);
    SetItemCheckStatus(32003, CIsoViewExt::AutoPropertyBrush[3]);

    pMenu->CheckMenuRadioItem(31000, 31003, CFinalSunDlgExt::CurrentLighting, MF_CHECKED);

    return 0;
}

DEFINE_HOOK(432380, CFinalSunDlg_Update_RecentFiles, A)
{
    GET(CMenu*, pMenu, ESI);

    for (size_t i = 0; i < CFinalSunAppExt::RecentFilesExt.size(); ++i)
    {
        if (CFinalSunAppExt::RecentFilesExt[i].length())
            pMenu->GetSubMenu(0)->InsertMenu(10 + i, MF_BYPOSITION, 40140 + i, CFinalSunAppExt::RecentFilesExt[i].c_str());
    }

    R->EDI(::CheckMenuItem);

    DarkTheme::EnableOwnerDrawMenu(*pMenu);
    return 0x432442;
}

DEFINE_HOOK(43209D, CFinalSunDlg_Update_TranslateMenuItems, A)
{
    // GET_STACK(CFinalSunDlg*, pThis, STACK_OFFS(0x60, 0x3C));
    // auto pMenu = pThis->GetMenu();

    GET(CMenu*, pMenu, ESI);

    auto translateMenuItem = [&pMenu](int id, ppmfc::CString pSrcName)
    {
        ppmfc::CString buffer;
        if (Translations::GetTranslationItem(pSrcName, buffer))
        {
            buffer.Replace("\\t", "\t");
            pMenu->ModifyMenu(id, MF_BYCOMMAND, id, buffer);
        }
    };

    auto translateSubMenu = [&pMenu](int pos, ppmfc::CString pSrcName)
    {
        ppmfc::CString buffer;
        if (Translations::GetTranslationItem(pSrcName, buffer))
        {
            pMenu->ModifyMenu(pos, MF_BYPOSITION, pos, buffer);
        }
    };

	ppmfc::CString menuName = "";
	for (int i = 0; i < 20; i++)
	{
		menuName.Format("Menu.Name.%d", i);
	    translateSubMenu(i, menuName);
	}

    translateMenuItem(57600, "Menu.File.New");
    translateMenuItem(40001, "Menu.File.Open");
    translateMenuItem(57603, "Menu.File.Save");
    translateMenuItem(40002, "Menu.File.SaveAs");
    translateMenuItem(40025, "Menu.File.CheckMap");
    // translateMenuItem(40017, "Menu.File.RunGame");
    translateMenuItem(40018, "Menu.File.Reopen");
    translateMenuItem(40003, "Menu.File.Quit");

    translateMenuItem(57643, "Menu.Edit.Undo");
    translateMenuItem(57644, "Menu.Edit.Redo");
    translateMenuItem(57634, "Menu.Edit.Copy");
    translateMenuItem(40109, "Menu.Edit.CopyWholeMap");
    translateMenuItem(57637, "Menu.Edit.Paste");
    translateMenuItem(40110, "Menu.Edit.PasteCentered");
    translateMenuItem(40040, "Menu.Edit.Map");
    translateMenuItem(40036, "Menu.Edit.Basic");
    translateMenuItem(40038, "Menu.Edit.SpecialFlags");
    translateMenuItem(40043, "Menu.Edit.Lighting");
    translateMenuItem(40039, "Menu.Edit.Houses");
    translateMenuItem(40151, "Menu.Edit.TriggerEditor");
    translateMenuItem(40042, "Menu.Edit.TagEditor");
    translateMenuItem(40139, "Menu.Edit.Taskforces");
    translateMenuItem(40150, "Menu.Edit.Scripts");
    translateMenuItem(40138, "Menu.Edit.Teams");
    translateMenuItem(40156, "Menu.Edit.AITriggers");
    translateMenuItem(40048, "Menu.Edit.AITriggerEnable");
    translateMenuItem(40160, "Menu.Edit.LocalVariables");
    translateMenuItem(40037, "Menu.Edit.SingleplayerSettings");
    translateMenuItem(40154, "Menu.Edit.INIEditor");
    translateMenuItem(40155, "Menu.Edit.CSFEditor");

    translateMenuItem(40053, "Menu.Terrain.RaiseGround");
    translateMenuItem(40054, "Menu.Terrain.LowerGround");
    translateMenuItem(40064, "Menu.Terrain.FlattenGround");
    translateMenuItem(40065, "Menu.Terrain.HideTileset");
    translateMenuItem(40066, "Menu.Terrain.ShowEveryTileset");
    translateMenuItem(40072, "Menu.Terrain.HideSingleField");
    translateMenuItem(40071, "Menu.Terrain.ShowAllFields");
    translateMenuItem(40062, "Menu.Terrain.RaiseSingleTile");
    translateMenuItem(40063, "Menu.Terrain.LowerSingleTile");

    translateMenuItem(40077, "Menu.MapTools.ChangeMapHeight");
    translateMenuItem(40096, "Menu.MapTools.AutoCreateShores");
    translateMenuItem(40152, "Menu.MapTools.AutoCreateLAT");
    translateMenuItem(40153, "Menu.MapTools.SmoothenWater");
    translateMenuItem(40085, "Menu.MapTools.AutoLevelUsingCliffs");
    translateMenuItem(40092, "Menu.MapTools.PaintCliffFront");
    translateMenuItem(40093, "Menu.MapTools.PaintCliffBack");
    translateMenuItem(40131, "Menu.MapTools.SearchObject");
    translateMenuItem(40132, "Menu.MapTools.SearchNextObject");
    translateMenuItem(40133, "Menu.MapTools.SearchWaypoint");
    translateMenuItem(40134, "Menu.MapTools.GlobalSearch");
    translateMenuItem(40135, "Menu.MapTools.ToolScripts");
    translateMenuItem(40136, "Menu.MapTools.DeleteObjects");
    translateMenuItem(40163, "Menu.MapTools.DistanceRuler");
    translateMenuItem(40158, "Menu.Options.LuaScriptConsole");
    translateMenuItem(40159, "Menu.Options.ShowObjectViewCameo");

    translateMenuItem(40004, "Menu.Options.Settings");
    translateMenuItem(40024, "Menu.Options.ShowMinimap");
    translateMenuItem(40023, "Menu.Options.Easymode");
    translateMenuItem(40118, "Menu.Options.Sounds");
    translateMenuItem(40123, "Menu.Options.ShowBuildingOutline");
    translateMenuItem(40104, "Menu.Options.DisableAutoShore");
    translateMenuItem(40105, "Menu.Options.DisableAutoLAT");
    translateMenuItem(40120, "Menu.Options.DisableSlopeCorrection");
    translateMenuItem(40157, "Menu.Options.SelectAutoShoreType");
    translateMenuItem(40162, "Menu.Options.Preferences");

    translateMenuItem(57670, "Menu.Help.Manual");
    translateMenuItem(40006, "Menu.Help.Info");
    translateMenuItem(40075, "Menu.Help.Credits");
    translateMenuItem(40022, "Menu.Help.TipOfTheDay");

    translateMenuItem(30000, "Menu.Display.Structures");
    translateMenuItem(30001, "Menu.Display.Infantries");
    translateMenuItem(30002, "Menu.Display.Units");
    translateMenuItem(30003, "Menu.Display.Aircrafts");
    translateMenuItem(30004, "Menu.Display.Basenodes");
    translateMenuItem(30005, "Menu.Display.Waypoints");
    translateMenuItem(30006, "Menu.Display.Celltags");
    translateMenuItem(30007, "Menu.Display.MoneyOnMap");
    translateMenuItem(30008, "Menu.Display.Overlays");
    translateMenuItem(30009, "Menu.Display.Terrains");
    translateMenuItem(30010, "Menu.Display.Smudges");
    translateMenuItem(30011, "Menu.Display.Tubes");
    translateMenuItem(30012, "Menu.Display.Bounds");
    translateMenuItem(30013, "Menu.Display.BasenodeIndex");
    translateMenuItem(30014, "Menu.Display.RockCells");
    translateMenuItem(30022, "Menu.Display.Shadows");
    translateMenuItem(30023, "Menu.Display.AlphaImages");

    translateMenuItem(30015, "Menu.Display.StructuresFilter");
    translateMenuItem(30016, "Menu.Display.InfantriesFilter");
    translateMenuItem(30017, "Menu.Display.UnitsFilter");
    translateMenuItem(30018, "Menu.Display.AircraftsFilter");
    translateMenuItem(30019, "Menu.Display.BasenodesFilter");
    translateMenuItem(30020, "Menu.Display.CellTagsFilter");
    translateMenuItem(30021, "Menu.Display.Veterancy");
    translateMenuItem(30050, "Menu.Display.ShowAll");

    translateMenuItem(31000, "Menu.Lighting.None");
    translateMenuItem(31001, "Menu.Lighting.Normal");
    translateMenuItem(31002, "Menu.Lighting.Lightning");
    translateMenuItem(31003, "Menu.Lighting.Dominator");

    translateMenuItem(32000, "Menu.PropertyBrush.AutoAircraft");
    translateMenuItem(32001, "Menu.PropertyBrush.AutoBuilding");
    translateMenuItem(32002, "Menu.PropertyBrush.AutoInfantry");
    translateMenuItem(32003, "Menu.PropertyBrush.AutoVehicle");

    translateMenuItem(33000, "Menu.Website.FA2spGithubPage");
    translateMenuItem(33001, "Menu.Website.FA2spHEGithubPage");
    translateMenuItem(33002, "Menu.Website.PhobosDocumentationPage");
    translateMenuItem(33003, "Menu.Website.PPMForumMainPage");
    translateMenuItem(33004, "Menu.Website.ModEncMainPage");
    translateMenuItem(33005, "Menu.Website.RA2MapGuideChinese");

    translateMenuItem(34001, "Menu.Paste.Ground");
    translateMenuItem(34002, "Menu.Paste.Overlays");
    translateMenuItem(34003, "Menu.Paste.Structures");
    translateMenuItem(34004, "Menu.Paste.Infantries");
    translateMenuItem(34005, "Menu.Paste.Units");
    translateMenuItem(34006, "Menu.Paste.Aircrafts");
    translateMenuItem(34007, "Menu.Paste.Terrains");
    translateMenuItem(34008, "Menu.Paste.Smudges");
    translateMenuItem(34050, "Menu.Paste.All");
    translateMenuItem(34051, "Menu.Paste.Overriding");
    translateMenuItem(34052, "Menu.Paste.ShowOutline");


    if (!ExtConfigs::InGameDisplay_Shadow) {
        CMenu* pSubMenu = pMenu->GetSubMenu(5);
        if (pSubMenu != nullptr) {
            pSubMenu->DeleteMenu(30022, MF_BYCOMMAND);
        }
    }
    if (!ExtConfigs::InGameDisplay_AlphaImage) {
        CMenu* pSubMenu = pMenu->GetSubMenu(5);
        if (pSubMenu != nullptr) {
            pSubMenu->DeleteMenu(30023, MF_BYCOMMAND);
        }
    }

    return 0x432304;
}

DEFINE_HOOK(432010, CFinalSunDlg_Update_EasyModeDisable, 7)
{
    CFinalSunApp::Instance->EasyMode = false;
    return 0;
}

#pragma warning(disable : 4834)
#pragma warning(disable : 6031)

DEFINE_HOOK(436EE0, CFinalSunDlg_AddToRecentFile, 7)
{
    REF_STACK(ppmfc::CString, lpPath, 0x4);

    std::string filepath = lpPath.m_pchData;
    auto& recentfiles = CFinalSunAppExt::RecentFilesExt;
    std::vector<std::string> sortedrecentfiles;
    auto itr = std::find_if(recentfiles.begin(), recentfiles.end(),
        [filepath](std::string& s) {return _strcmpi(s.c_str(), filepath.c_str()) == 0; }
    );
    if (itr == recentfiles.end()) // doesn't have this file
    {
        sortedrecentfiles.push_back(filepath);

        // Remove useless files
        for (auto& file : recentfiles)
        {
            if (file.length() > 0)
            {
                auto it = std::find_if(sortedrecentfiles.begin(), sortedrecentfiles.end(),
                    [file](std::string& s) {return _strcmpi(s.c_str(), file.c_str()) == 0; }
                );
                if (it == sortedrecentfiles.end()) // no duplicate file
                    sortedrecentfiles.push_back(file);
            }
        }
        recentfiles = sortedrecentfiles;

        CINI ini;
        std::string path = CFinalSunApp::Instance->ExePath();
        path += "\\FinalAlert.ini";
        ini.ClearAndLoad(path.c_str());

        for (size_t i = 0; i < recentfiles.size(); ++i)
            ini.WriteString("Files", std::format("{0:d}", i).c_str(), recentfiles[i].c_str());

        ini.WriteToFile(path.c_str());
    }

    return 0x437453;
}

DEFINE_HOOK(4340F0, CFinalSunDlg_Tools_ChangeMapHeight, 7)
{
    GET(CFinalSunDlg*, pThis, ECX);

    if (CMapData::Instance->MapWidthPlusHeight)
    {
        pThis->PlaySound(CFinalSunDlg::FASoundType::Normal);

        ppmfc::CString lpTitle = "StrChangeHeightCap";
        ppmfc::CString lpMessage = "StrChangeHeight";
        Translations::GetTranslationItem(lpTitle, lpTitle);
        Translations::GetTranslationItem(lpMessage, lpMessage);
        lpMessage.Replace("%1", "-14");
        lpMessage.Replace("%2", "14");

        int nDelta = 0;
        if (sscanf_s(CInputMessageBox::GetString(lpMessage, lpTitle), "%d", &nDelta) == 1 && nDelta >= -14 && nDelta <= 14)
        {
            for (int i = 0; i < CMapData::Instance->CellDataCount; ++i)
            {
                CMapData::Instance->CellDatas[i].Height =
                    std::clamp(CMapData::Instance->CellDatas[i].Height + nDelta, 0, 14);
            }

            pThis->MyViewFrame.pIsoView->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        }
        else
            pThis->PlaySound(CFinalSunDlg::FASoundType::Error);
    }
    else
        pThis->PlaySound(CFinalSunDlg::FASoundType::Error);

    return 0x434135;
}

DEFINE_HOOK(42D736, CFinalSunDlg_NewMap_Theater, 5)
{
    GET_STACK(int, theaterIndex, STACK_OFFS(0x161C, 0x15D4));

    auto theaters = TheaterHelpers::GetEnabledTheaterNames();
    auto theaterName = theaterIndex < theaters.size() ? theaters[theaterIndex] : ppmfc::CString("");
    char* pBuffer = new char[theaterName.GetLength() + 1];
    strcpy(pBuffer, theaterName);

    R->ECX(pBuffer);

    return 0;
}

DEFINE_HOOK(435290, CFinalSunDlg_OnMaptoolsAutolevel_ChangeTo2DMode, 5)
{
    GET(CFinalSunDlg*, pThis, ECX);

    if (CMapData::Instance->MapWidthPlusHeight)
    {
        pThis->PlaySound(CFinalSunDlg::FASoundType::Normal);
        CFinalSunApp::Instance().FlatToGround = !CFinalSunApp::Instance().FlatToGround;
        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);

    }
    else
            pThis->PlaySound(CFinalSunDlg::FASoundType::Error);


    return 0x4352D1;
}

DEFINE_HOOK(435450, CFinalSunDlg_OnMaptoolsAutocreateshores, 5)
{
    GET(CFinalSunDlg*, pThis, ECX);

    if (CMapData::Instance->MapWidthPlusHeight)
    {
        pThis->PlaySound(CFinalSunDlg::FASoundType::Normal);

        ppmfc::CString pMessage = Translations::TranslateOrDefault("AutocreateShoresMessage",
            "Warning: This tool may damage existing tiles.\n"
            "This tool will recalculate and generate shores across the map. After that, you should check the map to avoid connection errors. You can use Edit->undo to undo this action.");

        int result = MessageBox(pThis->MyViewFrame.pIsoView->m_hWnd, pMessage, Translations::TranslateOrDefault("AutocreateShoresTitle", "Autocreate Shores"), MB_YESNO | MB_ICONEXCLAMATION);

        if (result == IDYES)
        {
            auto& Map = CMapData::Instance();
            Map.SaveUndoRedoData(true, 0, 0, 0, 0);

            Map.CreateShore(0, 0, Map.MapWidthPlusHeight, Map.MapWidthPlusHeight);
            ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
        }
    }
    else
        pThis->PlaySound(CFinalSunDlg::FASoundType::Error);


    return 0x4354D5;
}

DEFINE_HOOK(4F5F30, CToolSettingsBar_OnShowWindow, 8)
{
    GET(CBrushSize*, pThis, ECX);
    auto pBrushSize = (ppmfc::CComboBox*)pThis->GetDlgItem(1377);
    pBrushSize->DeleteAllStrings();

    auto pSection = CINI::FAData().GetSection("BrushSizes");
    if (pSection)
    {
        for (auto& pair : pSection->GetEntities())
        {
            pBrushSize->AddString(pair.second);
        }
    }

    pBrushSize->SetCurSel(0);
    pBrushSize->UpdateData(FALSE);
    return 0x4F5F42;
}

DEFINE_HOOK(4F5E21, CToolSettingsBar_OnSelchangeBrushsize, 5)
{
    GET(int, curSel, ESI);

    // use game XY, not FA2 XY
    int bx = 1;
    int by = 1;

    auto pSection = CINI::FAData().GetSection("BrushSizes");
    if (pSection && pSection->GetEntities().size() > curSel)
    {
        auto value = pSection->GetValueAt(curSel);
        auto bs = STDHelpers::SplitString(*value, 1, "x");
        bx = atoi(bs[0]);
        by = atoi(bs[1]);
    }

    R->EAX(by);
    R->ECX(bx);

    return 0x4F5E9B;
}

DEFINE_HOOK(433C68, CFinalSunDlg_OnTerrainHeightenground_SkipBrushChange, 9)
{
    if (ExtConfigs::SkipBrushSizeChangeOnTools || CIsoView::CurrentCommand->Command == FACurrentCommand::HeightenGround)
        return 0x433CA5;

    return 0x433C71;
}
DEFINE_HOOK(433CD8, CFinalSunDlg_OnTerrainLowerground_SkipBrushChange, 9)
{
    if (ExtConfigs::SkipBrushSizeChangeOnTools || CIsoView::CurrentCommand->Command == FACurrentCommand::LowerGround)
        return 0x433D15;
    return 0x433CE1;
}
DEFINE_HOOK(433E28, CFinalSunDlg_OnTerrainFlatten_SkipBrushChange, 9)
{
    if (ExtConfigs::SkipBrushSizeChangeOnTools || CIsoView::CurrentCommand->Command == FACurrentCommand::MakeTerrainFlat)
        return 0x433E65;
    return 0x433E31;
}

DEFINE_HOOK(4F4BF4, CTipDlg_SkipTipOfTheDay, 7)
{
    if (ExtConfigs::SkipTipsOfTheDay)
        R->EAX(1);
    return 0;
}

static WORD StatusBar_Overlay = 0xFFFF;
DEFINE_HOOK(46A13B, CFinalSunDlg_StatusBar_Overlay, 6)
{
    GET(int, pos, EAX);
    StatusBar_Overlay = CMapDataExt::GetExtension()->GetOverlayAt(pos);
    R->EAX(StatusBar_Overlay);
    return 0x46A14B;
}

DEFINE_HOOK(46A1AD, CFinalSunDlg_StatusBar_Overlay_2, 5)
{
    R->EAX(StatusBar_Overlay);
    return 0x46A1B2;
}

DEFINE_HOOK(43533D, CFinalSunDlg_OnMaptoolsFrontcliff, 9)
{
    int cliff = CINI::CurrentTheater->GetInteger("General", "CliffSet", 1919810);
    if (CMapDataExt::IsValidTileSet(cliff))
        return 0x435350;
    return 0;
}

DEFINE_HOOK(4353BD, CFinalSunDlg_OnMaptoolsBackcliff, 9)
{
    int cliff = CINI::CurrentTheater->GetInteger("General", "CliffSet", 1919810);
    if (CMapDataExt::IsValidTileSet(cliff))
        return 0x4353D0;
    return 0;
}

DEFINE_HOOK(45EAF0, CIsoView_OnRButtonUp_CancelDistanceRuler, 6)
{
    if (!CIsoView::GetInstance()->IsScrolling)
        CIsoViewExt::DistanceRuler.clear();
    return 0;
}

DEFINE_HOOK(45EBB1, CIsoView_OnRButtonUp_CancelTreeViewSelection, 6)
{
    if (!CViewObjectsExt::NeedChangeTreeViewSelect)
        return 0x45EBC5;

    auto hWnd = CFinalSunDlg::Instance->MyViewFrame.pViewObjects->m_hWnd;
    HTREEITEM hSelectedItem = TreeView_GetSelection(hWnd);
    HTREEITEM hParent = TreeView_GetParent(hWnd, hSelectedItem);
    HTREEITEM hPrevSibling = TreeView_GetPrevSibling(hWnd, hSelectedItem);
    if (hParent != NULL)
        TreeView_SelectItem(hWnd, hParent);
    else if (hPrevSibling != NULL) {
        TreeView_SelectItem(hWnd, hPrevSibling);
    }
    else {
        HTREEITEM hRoot = TreeView_GetRoot(hWnd);
        if (hRoot != NULL)
            TreeView_SelectItem(hWnd, hRoot);
    }
    if (!CopyPaste::PastedCoords.empty())
    {
        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
    CopyPaste::PastedCoords.clear();
    if (CIsoView::CurrentCommand->Command == 0x1B)
    {
        CIsoView::CurrentCommand->Command = 0x0;
        CIsoView::CurrentCommand->Type = 0;
    }   
    if (CIsoView::CurrentCommand->Command == 0x1F) {
        CTerrainGenerator::RangeFirstCell.X = -1;
        CTerrainGenerator::RangeFirstCell.Y = -1;
        CTerrainGenerator::RangeSecondCell.X = -1;
        CTerrainGenerator::RangeSecondCell.Y = -1;
        CIsoView::CurrentCommand->Command = 0x0;
        CIsoView::CurrentCommand->Type = 0;
    }  
    if (CViewObjectsExt::MoveBaseNode_SelectedObj.X > -1) {
        CViewObjectsExt::MoveBaseNode_SelectedObj.House = "";
        CViewObjectsExt::MoveBaseNode_SelectedObj.ID = "";
        CViewObjectsExt::MoveBaseNode_SelectedObj.Key = "";
        CViewObjectsExt::MoveBaseNode_SelectedObj.X = -1;
        CViewObjectsExt::MoveBaseNode_SelectedObj.Y = -1;
        CIsoView::CurrentCommand->Command = 0x0;
        CIsoView::CurrentCommand->Type = 0;
        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
    if (CIsoViewExt::IsPressingTube)
    {
        CIsoViewExt::IsPressingTube = false;
        CIsoViewExt::TubeNodes.clear();
    }
    if (CIsoView::CurrentCommand->Command == 0x23)
    {
        CLuaConsole::Lua.collect_garbage();
        CLuaConsole::applyingScript = false;
        CIsoView::CurrentCommand->Command = 0x0;
        CIsoView::CurrentCommand->Type = 0;
    }
    CIsoViewExt::LastAltCommand.reset();

    return 0x45EBC5;
}

DEFINE_HOOK(4326C0, CFinalSunDlg_QuitProgram_BeforeDialog, 5)
{
    ExtConfigs::IsQuitingProgram = true;
    return 0;
}

DEFINE_HOOK(4327A1, CFinalSunDlg_QuitProgram_AfterDialog, 5)
{
    ExtConfigs::FileWatcher = false;

    auto& minimap = CFinalSunDlg::Instance->MyViewFrame.Minimap;
    if (minimap.GetSafeHwnd()) {
        minimap.DestroyWindow();
    }
    
    return 0;
}

DEFINE_HOOK(432829, CFinalSunDlg_QuitProgram_End, 5)
{
    ExtConfigs::IsQuitingProgram = false;
    return 0;
}
