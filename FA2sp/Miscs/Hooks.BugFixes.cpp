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

DEFINE_HOOK(555D97, CString_AllocBuffer, 7)
{
	if (!ExtConfigs::StringBufferStackAllocation)
	{
		if (R->ESI() > R->EDI())
			return 0x555DA8;
		else
		{
			R->ECX(0x8862A8);
			return 0x555D9E;
		}
	}

	return 0x555DD8;
}

DEFINE_HOOK(555DFE, CString_FreeData, 6)
{
	if (!ExtConfigs::StringBufferStackAllocation)
		return 0;

	GET(LPVOID, lpMem, ECX);
	FAMemory::Deallocate(lpMem);
	return 0x555E45;
}

// FA2 will no longer automatically change the extension of map
DEFINE_HOOK(42700A, CFinalSunDlg_SaveMap_Extension, 9)
{
	return 0x42708D;
}

// Extend Undo/Redo limit
DEFINE_HOOK(4BBAB8, CMapData_SaveUndoRedoData_SizeLimit, 6)
{
	++CMapData::Instance->UndoRedoCurrentDataIndex;
	++CMapData::Instance->UndoRedoDataCount;

	R->ESI(CMapData::Instance->UndoRedoDataCount);

	if (CMapData::Instance->UndoRedoDataCount <= ExtConfigs::UndoRedoLimit)
		return 0x4BBBB7;

	R->EDX(CMapData::Instance->UndoRedoData);
	CMapData::Instance->UndoRedoDataCount = ExtConfigs::UndoRedoLimit;
	CMapData::Instance->UndoRedoCurrentDataIndex = ExtConfigs::UndoRedoLimit - 1;
	return 0x4BBAF7;
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

	ppmfc::CString color = "";
	if (pHouse)
		if (auto pStr = Variables::Rules.TryGetString(pHouse, "Color")) {
			color = *pStr;
		}

	if (pColor)
		color = pColor;

	HSVClass hsv{ 0,0,0 };
	if (!color.IsEmpty())
		if (auto const pValue = CINI::Rules->TryGetString("Colors", color)) {
			sscanf_s(*pValue, "%hhu,%hhu,%hhu", &hsv.H, &hsv.S, &hsv.V);
		}
			

	RGBClass rgb;
	if (!ExtConfigs::UseRGBHouseColor)
		rgb = hsv;
	else
		rgb = { hsv.H,hsv.S,hsv.V };

	R->EAX<int>(rgb);

	return 0x468EEB;
}

DEFINE_HOOK(4A3306, CMapData_Update_InfantrySubCell, 5)//UpdateInfantry
{
	GET(int, spp, ECX);
	if (spp >= 4)
	{
		return 0x4A335D;
	}
	if (spp == 3)
	{
		R->ECX(0);
	}
	return 0x4A330B;
}
DEFINE_HOOK(4A7C74, CMapData_Update_InfantrySubCell2, 5)//DeleteInfantry
{
	GET(int, pos, EAX);
	if (pos > 0)
		pos--;
	if (pos == 3)
		pos = 0;
	R->EAX(pos);

	return 0x4A7C79;
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
	static ppmfc::CString ReturnBuffer;

	GET(CInputMessageBox*, pThis, ECX);

	pThis->GetDlgItem(1047)->GetWindowText(ReturnBuffer);
	
	pThis->EndDialog(ReturnBuffer.GetLength() ? (int)ReturnBuffer.m_pchData : (int)nullptr);

	return 0x4565A5;
}

DEFINE_HOOK(4C76C6, CMapData_ResizeMap_PositionFix_SmudgeAndBasenode, 5)
{
	GET_STACK(int, XOFF, STACK_OFFS(0x1C4, 0x19C));
	GET_STACK(int, YOFF, STACK_OFFS(0x1C4, 0x194));

	ppmfc::CString buffer;

	{
		std::vector<std::tuple<ppmfc::CString, ppmfc::CString, int, int>> smudges;
		for (size_t i = 0; i < CMapData::Instance->SmudgeDatas.size();++i)
		{
			const auto& data = CMapData::Instance->SmudgeDatas[i];
			buffer.Format("%d", i);
			smudges.emplace_back(buffer, data.TypeID, data.X + XOFF, data.Y + YOFF);
		}
		
		CMapData::Instance->INI.DeleteSection("Smudge");
		if (auto pSection = CMapData::Instance->INI.AddSection("Smudge"))
		{
			for (const auto& [key, id, x, y] : smudges)
			{
				if (!CMapData::Instance->IsCoordInMap(x, y))
					continue;

				buffer.Format("%s,%d,%d,0", id, x, y);
				CMapData::Instance->INI.WriteString(pSection, key, buffer);
			}
		}
	}
	CMapData::Instance->UpdateFieldSmudgeData(false);

	for (const auto& [_, house] : Variables::Rules.GetSection("Houses"))
	{
		if (auto pSection = CMapData::Instance->INI.GetSection(house))
		{
			const int nodeCount = CMapData::Instance->INI.GetInteger(pSection, "NodeCount");

			std::vector<std::tuple<ppmfc::CString, ppmfc::CString, int, int>> nodes;
			for (int i = 0; i < nodeCount; ++i)
			{
				buffer.Format("%03d", i);
				const auto value = CMapData::Instance->INI.GetString(pSection, buffer);
				const auto splits = STDHelpers::SplitString(value);
				nodes.emplace_back(buffer, splits[0], atoi(splits[1]) + XOFF, atoi(splits[2]) + YOFF);
			}

			for (auto& [key, id, x, y] : nodes)
			{
				if (!CMapData::Instance->IsCoordInMap(x, y))
				{
					x = 0;
					y = 0;
				}

				buffer.Format("%s,%d,%d", id, x, y);
				// CMapData::Instance->INI.DeleteKey(pSection, key); // useless
				CMapData::Instance->INI.WriteString(pSection, key, buffer);
			}
		}
	}
	CMapData::Instance->UpdateFieldBasenodeData(false);

	return 0;
}

DEFINE_HOOK(4FF70A, CTriggerEventsDlg_OnSelchangeParameter_FixFor23, 5)
{
	GET_STACK(ppmfc::CString, Code, STACK_OFFS(0xB0, 0x74));

	if (atoi(Code) == 2)
		return 0x4FF71B;

	return 0x4FF71E;
}

// Rewrite SetOverlayAt to fix wrong credits on map bug
// if you undo the placement of some tiberium, and then
// move your mouse with previewed tiberium over the undo
// area, the bug happens
DEFINE_HOOK(4A16C0, CMapData_SetOverlayAt, 6)
{
	GET(CMapData*, pThis, ECX);
	GET_STACK(int, dwPos, 0x4);
	GET_STACK(unsigned char, overlay, 0x8);

	int x = pThis->GetXFromCoordIndex(dwPos);
	int y = pThis->GetYFromCoordIndex(dwPos);
	int olyPos = y + x * 512;

	if (olyPos > 262144 || dwPos > pThis->CellDataCount) return 0x4A17B6;

	// here is the problem
	// auto& ovrl = pThis->Overlay[olyPos];
	// auto& ovrld = pThis->OverlayData[olyPos];

	auto& ovrl = pThis->CellDatas[dwPos].Overlay;
	auto& ovrld = pThis->CellDatas[dwPos].OverlayData;

	pThis->DeleteTiberium(ovrl, ovrld);

	pThis->Overlay[olyPos] = overlay;
	pThis->OverlayData[olyPos] = 0;
	pThis->CellDatas[dwPos].Overlay = overlay;
	pThis->CellDatas[dwPos].OverlayData = 0;

	// auto& ovrl2 = pThis->Overlay[olyPos];
	// auto& ovrld2 = pThis->OverlayData[olyPos];
	auto& ovrl2 = pThis->CellDatas[dwPos].Overlay;
	auto& ovrld2 = pThis->CellDatas[dwPos].OverlayData;
	pThis->AddTiberium(ovrl2, ovrld2);

	int i, e;
	for (i = -1; i < 2; i++)
		for (e = -1; e < 2; e++)
			if (pThis->IsCoordInMap(x + i, y + e))
				pThis->SmoothTiberium(pThis->GetCoordIndex(x + i, y + e));


	pThis->UpdateMapPreviewAt(x, y);

	return 0x4A17B6;
}

DEFINE_HOOK(4A2A10, CMapData_SetOverlayDataAt, 5)
{
	GET(CMapData*, pThis, ECX);
	GET_STACK(int, dwPos, 0x4);
	GET_STACK(unsigned char, overlaydata, 0x8);

	int x = pThis->GetXFromCoordIndex(dwPos);
	int y = pThis->GetYFromCoordIndex(dwPos);
	int olyPos = y + x * 512;

	if (olyPos > 262144 || dwPos > pThis->CellDataCount) return 0x4A2A88;

	//auto& ovrl = pThis->Overlay[olyPos];
	//auto& ovrld = pThis->OverlayData[olyPos];

	auto& ovrl = pThis->CellDatas[dwPos].Overlay;
	auto& ovrld = pThis->CellDatas[dwPos].OverlayData;

	if (CMapDataExt::IsOre(ovrl))
		return 0x4A2A88;

	pThis->OverlayData[olyPos] = overlaydata;
	pThis->CellDatas[dwPos].OverlayData = overlaydata;

	return 0x4A2A88;
}



