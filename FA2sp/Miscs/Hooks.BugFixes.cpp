#include <Helpers/Macro.h>
#include <Drawing.h>
#include <CINI.h>
#include <CFinalSunDlg.h>
#include <CFinalSunApp.h>
#include <CMapData.h>
#include <CPalette.h>
#include <CObjectDatas.h>
#include <CTileTypeClass.h>
#include <CIsoView.h>
#include <CInputMessageBox.h>
#include <MFC/ppmfc_cstring.h>
#include "../FA2sp.h"
#include "../Helpers/STDHelpers.h"
#include <CPropertyInfantry.h>
#include "../Ext/CMapData/Body.h"

// FA2 will no longer automatically change the extension of map
DEFINE_HOOK(42700A, CFinalSunDlg_SaveMap_Extension, 9)
{
	return 0x42708D;
}

// Self explained nameing
DEFINE_HOOK(421B70, CFinalSunApp_InitInstance_NoEasyViewExplain, 5)
{
	CFinalSunApp::Instance->EasyMode = false;

	return 0x421EEB;
}

// Fix bug for incorrect color while drawing
DEFINE_HOOK(468760, Miscs_GetColor, 7)
{
	GET_STACK(const char*, pHouse, 0x4);
	GET_STACK(const char*, pColor, 0x8);

	FString color = "";
	if (pHouse)
		if (auto pStr = Variables::RulesMap.TryGetString(pHouse, "Color")) {
			color = *pStr;
		}

	if (pColor)
		color = pColor;

	auto itr = CMapDataExt::Colors.find(color);
	if (itr != CMapDataExt::Colors.end())
		R->EAX(itr->second);
	else
		R->EAX(0);
	
	return 0x468EEB;
}

// Fix the bug that up&down&left&right vk doesn't update the TileSetBrowserView
DEFINE_HOOK(422EA4, CFinalSunApp_ProcessMessageFilter_UpdateTileSetBrowserView_UpAndDown, 8)
{
	CFinalSunDlg::Instance->MyViewFrame.pTileSetBrowserFrame->View.SelectTileSet(
		(*CTileTypeClass::CurrentTileType)[CIsoView::CurrentCommand->Type].TileSet,
		false
	);

	return 0;
}

DEFINE_HOOK_AGAIN(422BF6, CFinalSunApp_ProcessMessageFilter_UpdateTileSetBrowserView_LeftAndRight, 7) // VirtualKey_Right
DEFINE_HOOK(422B95, CFinalSunApp_ProcessMessageFilter_UpdateTileSetBrowserView_LeftAndRight, 7) // VirtualKey_Left
{
	CFinalSunDlg::Instance->MyViewFrame.pTileSetBrowserFrame->View.RedrawWindow(
		nullptr, nullptr, 
		RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW
	);

	return 0;
}

// The original implement will lead to memory leak
DEFINE_HOOK(4564F0, CInputMessageBox_OnOK, 7)
{
	static char buffer[1024]{};
	static FString ReturnBuffer;

	GET(CInputMessageBox*, pThis, ECX);

	std::memset(buffer, 0, 1024);
	::GetWindowText(pThis->GetDlgItem(1047)->GetSafeHwnd(), buffer, 1023);
	ReturnBuffer = buffer;
	
	pThis->EndDialog(ReturnBuffer.GetLength() ? (int)ReturnBuffer.c_str() : (int)nullptr);

	return 0x4565A5;
}

// Rewrite SetOverlayAt to fix wrong credits on map bug
// if you undo the placement of some tiberium, and then
// move your mouse with previewed tiberium over the undo
// area, the bug happens
DEFINE_HOOK(4A16C0, CMapData_SetOverlayAt, 6)
{
	GET(CMapDataExt*, pThis, ECX);
	GET_STACK(int, dwPos, 0x4);
	GET_STACK(unsigned char, overlay, 0x8);

	pThis->SetNewOverlayAt(dwPos, overlay == 0xff ? 0xffff : overlay);

	return 0x4A17B6;
}

DEFINE_HOOK(4A2A10, CMapData_SetOverlayDataAt, 5)
{
	GET(CMapDataExt*, pThis, ECX);
	GET_STACK(int, dwPos, 0x4);
	GET_STACK(unsigned char, overlaydata, 0x8);

	int x = pThis->GetXFromCoordIndex(dwPos);
	int y = pThis->GetYFromCoordIndex(dwPos);
	int olyPos = y + x * 512;

	if (olyPos > 262144 || dwPos > pThis->CellDataCount) return 0x4A2A88;

	//auto& ovrl = pThis->Overlay[olyPos];
	//auto& ovrld = pThis->OverlayData[olyPos];

	auto& ovrl = pThis->CellDataExts[dwPos].NewOverlay;
	auto& ovrld = pThis->CellDatas[dwPos].OverlayData;

	if (CMapDataExt::IsOre(ovrl))
		return 0x4A2A88;

	pThis->OverlayData[olyPos] = overlaydata;
	pThis->CellDatas[dwPos].OverlayData = overlaydata;

	return 0x4A2A88;
}

DEFINE_HOOK(41B637, CCliffModifier_PlaceCliff, 8)
{
	GET(int, nTileToPlace, EAX);
	if (nTileToPlace < 0 || nTileToPlace >= CMapDataExt::TileDataCount)
		return 0x41B66E;
	return 0;
}
