#include "Body.h"

#include "../../FA2sp.h"
#include "../CIsoView/Body.h"
#include "../CMapData/Body.h"
#include "../CFinalSunApp/Body.h"
#include <CMapData.h>
#include <CLoading.h>
#include <CInputMessageBox.h>
#include "../../Miscs/Palettes.h"
#include "../../ExtraWindow/CNewTeamTypes/CNewTeamTypes.h"
#include "../../ExtraWindow/CNewTaskforce/CNewTaskforce.h"
#include "../../ExtraWindow/CNewScript/CNewScript.h"
#include "../../ExtraWindow/CNewTrigger/CNewTrigger.h"
#include "../../ExtraWindow/CSearhReference/CSearhReference.h"
#include "../../ExtraWindow/CBatchTrigger/CBatchTrigger.h"
#include "../../ExtraWindow/CNewINIEditor/CNewINIEditor.h"
#include "../../ExtraWindow/CTriggerAnnotation/CTriggerAnnotation.h"
#include "../../ExtraWindow/CCsfEditor/CCsfEditor.h"
#include "../../ExtraWindow/CNewAITrigger/CNewAITrigger.h"
#include "../../ExtraWindow/CObjectSearch/CObjectSearch.h"
#include "../../ExtraWindow/CLuaConsole/CLuaConsole.h"
#include "../../ExtraWindow/CNewLocalVariables/CNewLocalVariables.h"
#include "../../ExtraWindow/CFA2spOptions/CFA2spOptions.h"
#include "../../Helpers/STDHelpers.h"

#include "../../Helpers/Translations.h"
#include "../CTileSetBrowserFrame/Body.h"
#include "../CLoading/Body.h"
#include "../../ExtraWindow/CSelectAutoShore/CSelectAutoShore.h"
#include <thread>
#include "../../Miscs/MultiSelection.h"
#include "../../Miscs/DialogStyle.h"
#include "../../Helpers/Helper.h"
#include "../../ExtraWindow/CMapRendererDlg/CMapRendererDlg.h"
#include <CUpdateProgress.h>
#include <filesystem>
#include "../../Miscs/SaveMap.h"
namespace fs = std::filesystem;

int CFinalSunDlgExt::CurrentLighting = 31000;
std::pair<FString, int> CFinalSunDlgExt::SearchObjectIndex ("", - 1);
int CFinalSunDlgExt::SearchObjectType = -1;
enum FindType { Aircraft = 0, Infantry, Structure, Unit };

void CFinalSunDlgExt::ProgramStartupInit()
{
	RunTime::ResetMemoryContentAt(0x5937E8, &CFinalSunDlgExt::PreTranslateMessageExt);
	RunTime::ResetMemoryContentAt(0x5937D0, &CFinalSunDlgExt::OnCommandExt);
}

class UpdateMenuHelper
{
public:
	UpdateMenuHelper(HMENU menu) { hMenu = menu; };
	~UpdateMenuHelper() {
		DarkTheme::EnableOwnerDrawMenu(hMenu);
	};
	HMENU hMenu;
};

BOOL CFinalSunDlgExt::OnCommandExt(WPARAM wParam, LPARAM lParam)
{
	WORD wmID = LOWORD(wParam);
	WORD wmMsg = HIWORD(wParam);

	HMENU hMenu = *this->GetMenu();
	auto updateMenu = UpdateMenuHelper(hMenu);
	auto SetLayerStatus = [this, &hMenu, wmID](int id, bool& param)
	{
		if (GetMenuState(hMenu, id, MF_BYCOMMAND) & MF_CHECKED)
		{
			param = false;
			CheckMenuItem(hMenu, id, MF_UNCHECKED);
		}
		else
		{
			param = true;
			CheckMenuItem(hMenu, id, MF_CHECKED);

		}
		if (wmID >= 30000 && wmID < 31000)
		::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
	};
	auto SetFilterStatus = [this, &hMenu, wmID](int id, bool& param)
		{
			if (GetMenuState(hMenu, id, MF_BYCOMMAND) & MF_CHECKED)
			{
				param = false;
				CheckMenuItem(hMenu, id, MF_UNCHECKED);
			}
			else
			{
				param = true;
				CheckMenuItem(hMenu, id, MF_CHECKED);


				const FString title = Translations::TranslateOrDefault(
					"ObjectFilterTitle", "Object Filter"
				);
				const FString message = Translations::TranslateOrDefault(
					"ObjectFilterMessage", "Please input Object ID(s):\n\nSeparate multiple objects with \",\". Leave it blank to display all.\nAfter confirmation, set object property filtering (optional)."
				);
				const FString message2 = Translations::TranslateOrDefault(
					"ObjectFilterMessageCT", "Please input Tag ID(s):\n\nSeparate multiple tags with \",\". You can only fill in the last digits."
				);
				ppmfc::CString result;
				if (wmID != 30020)
					result = CInputMessageBox::GetString(message, title);
				else
					result = CInputMessageBox::GetString(message2, title);

				if (STDHelpers::IsNullOrWhitespace(result))
					result = "";
				STDHelpers::TrimString(result);


				CViewObjectsExt::InitPropertyDlgFromProperty = true;
				if (wmID == 30020)
				{
					CViewObjectsExt::ObjectFilterCT.clear();
					CViewObjectsExt::ObjectFilterCT = FString::SplitString(result);
					if (STDHelpers::IsNullOrWhitespace(result))
						CheckMenuItem(hMenu, id, MF_UNCHECKED);

				}
				if (wmID == 30015)
				{
					CViewObjectsExt::ObjectFilterB.clear();
					CViewObjectsExt::ObjectFilterB = FString::SplitString(result);

					if (CViewObjectsExt::BuildingBrushDlgBF.get() == nullptr)
						CViewObjectsExt::BuildingBrushDlgBF = std::make_unique<CPropertyBuilding>(CFinalSunDlg::Instance->MyViewFrame.pIsoView);

					for (auto& v : CViewObjectsExt::BuildingBrushBools)
						v = false;

					CViewObjectsExt::BuildingBrushDlgBF->ppmfc::CDialog::DoModal();

					int index = 0;
					for (auto& v : CViewObjectsExt::BuildingBrushBools)
					{
						CViewObjectsExt::BuildingBrushBoolsBF[index] = v;
						index++;
					}
				}
				else if (wmID == 30016)
				{
					CViewObjectsExt::ObjectFilterI.clear();
					CViewObjectsExt::ObjectFilterI = FString::SplitString(result);

					if (CViewObjectsExt::InfantryBrushDlgF.get() == nullptr)
						CViewObjectsExt::InfantryBrushDlgF = std::make_unique<CPropertyInfantry>(CFinalSunDlg::Instance->MyViewFrame.pIsoView);

					for (auto& v : CViewObjectsExt::InfantryBrushBools)
						v = false;

					CViewObjectsExt::InfantryBrushDlgF->ppmfc::CDialog::DoModal();

					int index = 0;
					for (auto& v : CViewObjectsExt::InfantryBrushBools)
					{
						CViewObjectsExt::InfantryBrushBoolsF[index] = v;
						index++;
					}
				}
				else if (wmID == 30017)
				{
					CViewObjectsExt::ObjectFilterV.clear();
					CViewObjectsExt::ObjectFilterV = FString::SplitString(result);

					if (CViewObjectsExt::VehicleBrushDlgF.get() == nullptr)
						CViewObjectsExt::VehicleBrushDlgF = std::make_unique<CPropertyUnit>(CFinalSunDlg::Instance->MyViewFrame.pIsoView);

					for (auto& v : CViewObjectsExt::VehicleBrushBools)
						v = false;

					CViewObjectsExt::VehicleBrushDlgF->ppmfc::CDialog::DoModal();

					int index = 0;
					for (auto& v : CViewObjectsExt::VehicleBrushBools)
					{
						CViewObjectsExt::VehicleBrushBoolsF[index] = v;
						index++;
					}
				}
				else if (wmID == 30018)
				{
					CViewObjectsExt::ObjectFilterA.clear();
					CViewObjectsExt::ObjectFilterA = FString::SplitString(result);

					if (CViewObjectsExt::AircraftBrushDlgF.get() == nullptr)
						CViewObjectsExt::AircraftBrushDlgF = std::make_unique<CPropertyAircraft>(CFinalSunDlg::Instance->MyViewFrame.pIsoView);

					for (auto& v : CViewObjectsExt::AircraftBrushBools)
						v = false;

					CViewObjectsExt::AircraftBrushDlgF->ppmfc::CDialog::DoModal();

					int index = 0;
					for (auto& v : CViewObjectsExt::AircraftBrushBools)
					{
						CViewObjectsExt::AircraftBrushBoolsF[index] = v;
						index++;
					}
				}
				else if (wmID == 30019)
				{
					CViewObjectsExt::ObjectFilterBN.clear();
					CViewObjectsExt::ObjectFilterBN = FString::SplitString(result);

					if (CViewObjectsExt::BuildingBrushDlgBNF.get() == nullptr)
						CViewObjectsExt::BuildingBrushDlgBNF = std::make_unique<CPropertyBuilding>(CFinalSunDlg::Instance->MyViewFrame.pIsoView);

					for (auto& v : CViewObjectsExt::BuildingBrushBools)
						v = false;

					CViewObjectsExt::BuildingBrushDlgBNF->ppmfc::CDialog::DoModal();

					int index = 0;
					for (auto& v : CViewObjectsExt::BuildingBrushBools)
					{
						CViewObjectsExt::BuildingBrushBoolsBNF[index] = v;
						index++;
					}
				}


				CViewObjectsExt::InitPropertyDlgFromProperty = false;
			}

			if (wmID >= 30000 && wmID < 31000)
				::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
		};
	auto SetMenuStatusTrue = [this, &hMenu](int id, bool& param)
	{
		param = true;
		CheckMenuItem(hMenu, id, MF_CHECKED);
	};
	auto SetMenuStatusFalse = [this, &hMenu](int id, bool& param)
	{
		param = false;
		CheckMenuItem(hMenu, id, MF_UNCHECKED);
	};

	auto SetLightingStatus = [this, &hMenu](int id)
	{
		CheckMenuRadioItem(hMenu, 31000, 31003, id, MF_UNCHECKED);
		if (CFinalSunDlgExt::CurrentLighting != id)
		{
			CFinalSunDlgExt::CurrentLighting = id;
			LightingStruct::GetCurrentLighting();

			for (int i = 0; i < CMapData::Instance->MapWidthPlusHeight; i++) {
				for (int j = 0; j < CMapData::Instance->MapWidthPlusHeight; j++) {
					CMapData::Instance->UpdateMapPreviewAt(i, j);
				}
			}
			LightingSourceTint::CalculateMapLamps();

			this->MyViewFrame.Minimap.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
			::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
			auto tmp = CIsoView::CurrentCommand->Command;
			if (this->MyViewFrame.pTileSetBrowserFrame->View.CurrentMode == 1) {
				HWND hParent = this->MyViewFrame.pTileSetBrowserFrame->DialogBar.GetSafeHwnd();
				HWND hTileComboBox = ::GetDlgItem(hParent, 1366);
				::SendMessage(hParent, WM_COMMAND, MAKEWPARAM(1366, CBN_SELCHANGE), (LPARAM)hTileComboBox);
				CIsoView::CurrentCommand->Command = tmp;
			}
			else if (this->MyViewFrame.pTileSetBrowserFrame->View.CurrentMode == 2) {
				HWND hParent = this->MyViewFrame.pTileSetBrowserFrame->DialogBar.GetSafeHwnd();
				HWND hOverlayComboBox = ::GetDlgItem(hParent, 1367);
				::SendMessage(hParent, WM_COMMAND, MAKEWPARAM(1367, CBN_SELCHANGE), (LPARAM)hOverlayComboBox);
				CIsoView::CurrentCommand->Command = tmp;
			}
		}
	};

	auto changeBrushSize = [](int index)
	{
		auto pSection = CINI::FAData().GetSection("BrushSizes");
		if (pSection && pSection->GetEntities().size() > index && index >= 0)
		{
			auto pIsoView = (CIsoViewExt*)CIsoView::GetInstance();
			auto value = pSection->GetValueAt(index);
			auto bs = STDHelpers::SplitString(*value, 1, "x");
			pIsoView->BrushSizeX = atoi(bs[1]);
			pIsoView->BrushSizeY = atoi(bs[0]);
		}
	};

	auto isInChildWindow = [this]()
		{
			HWND hWnd = GetActiveWindow();
			if (hWnd == CNewAITrigger::GetHandle()) {
				return TRUE;
			}
			else if (hWnd == CNewINIEditor::GetHandle()) {
				return TRUE;
			}
			else if (hWnd == CNewINIEditor::GetImporter()) {
				return TRUE;
			}
			else if (hWnd == CNewScript::GetHandle()) {
				return TRUE;
			}
			else if (hWnd == CNewTaskforce::GetHandle()) {
				return TRUE;
			}
			else if (hWnd == CNewTeamTypes::GetHandle()) {
				return TRUE;
			}
			else if (hWnd == CTerrainGenerator::GetHandle()) {
				return TRUE;
			}
			else if (hWnd == CLuaConsole::GetHandle()) {
				return TRUE;
			}
			else if (hWnd == CCsfEditor::GetHandle()) {
				return TRUE;
			}
			else if (hWnd == CFA2spOptions::GetHandle()) {
				return TRUE;
			}
			else if (hWnd == CNewLocalVariables::GetHandle()) {
				return TRUE;
			}
			else if (hWnd == CSearhReference::GetHandle()) {
				return TRUE;
			}
			else if (hWnd == CBatchTrigger::GetHandle()) {
				return TRUE;
			}
			else if (hWnd == this->SingleplayerSettings.GetSafeHwnd()) {
				return TRUE;
			}
			else if (hWnd == this->AITriggerTypes.GetSafeHwnd()) {
				return TRUE;
			}
			else if (hWnd == this->AITriggerTypesEnable.GetSafeHwnd()) {
				return TRUE;
			}
			else if (hWnd == this->ScriptTypes.GetSafeHwnd()) {
				return TRUE;
			}
			else if (hWnd == this->TriggerFrame.GetSafeHwnd()) {
				return TRUE;
			}
			else if (hWnd == this->Tags.GetSafeHwnd()) {
				return TRUE;
			}
			else if (hWnd == this->TaskForce.GetSafeHwnd()) {
				return TRUE;
			}
			else if (hWnd == this->TeamTypes.GetSafeHwnd()) {
				return TRUE;
			}
			else if (hWnd == this->Houses.GetSafeHwnd()) {
				return TRUE;
			}
			else if (hWnd == this->SpecialFlags.GetSafeHwnd()) {
				return TRUE;
			}
			else if (hWnd == this->Lighting.GetSafeHwnd()) {
				return TRUE;
			}
			else if (hWnd == this->INIEditor.GetSafeHwnd()) {
				return TRUE;
			}		
			else if (hWnd == this->Basic.GetSafeHwnd()) {
				return TRUE;
			}	
			else if (hWnd == this->MapD.GetSafeHwnd()) {
				return TRUE;
			}
			for (int i = 0; i < TRIGGER_EDITOR_MAX_COUNT; ++i)
			{
				if (hWnd == CNewTrigger::Instance[i].GetHandle()) {
					return TRUE;
			}
			}
			return FALSE;
		};

	switch (wmID)
	{
	case 30000:
		SetLayerStatus(30000, CIsoViewExt::DrawStructures);
		return TRUE;
	case 30001:
		SetLayerStatus(30001, CIsoViewExt::DrawInfantries);
		return TRUE;
	case 30002:
		SetLayerStatus(30002, CIsoViewExt::DrawUnits);
		return TRUE;
	case 30003:
		SetLayerStatus(30003, CIsoViewExt::DrawAircrafts);
		return TRUE;
	case 30004:
		SetLayerStatus(30004, CIsoViewExt::DrawBasenodes);
		return TRUE;
	case 30005:
		SetLayerStatus(30005, CIsoViewExt::DrawWaypoints);
		return TRUE;
	case 30006:
		SetLayerStatus(30006, CIsoViewExt::DrawCelltags);
		return TRUE;
	case 30007:
		SetLayerStatus(30007, CIsoViewExt::DrawMoneyOnMap);
		return TRUE;
	case 30008:
		SetLayerStatus(30008, CIsoViewExt::DrawOverlays);
		return TRUE;
	case 30009:
		SetLayerStatus(30009, CIsoViewExt::DrawTerrains);
		return TRUE;
	case 30010:
		SetLayerStatus(30010, CIsoViewExt::DrawSmudges);
		return TRUE;
	case 30011:
		SetLayerStatus(30011, CIsoViewExt::DrawTubes);
		return TRUE;
	case 30012:
		SetLayerStatus(30012, CIsoViewExt::DrawBounds);
		return TRUE;	
	case 30013:
		SetLayerStatus(30013, CIsoViewExt::DrawBaseNodeIndex);
		return TRUE;
	case 30014:
		SetLayerStatus(30014, CIsoViewExt::RockCells);
		return TRUE;
	case 30015:
		SetFilterStatus(30015, CIsoViewExt::DrawStructuresFilter);
		return TRUE;
	case 30016:
		SetFilterStatus(30016, CIsoViewExt::DrawInfantriesFilter);
		return TRUE;
	case 30017:
		SetFilterStatus(30017, CIsoViewExt::DrawUnitsFilter);
		return TRUE;
	case 30018:
		SetFilterStatus(30018, CIsoViewExt::DrawAircraftsFilter);
		return TRUE;
	case 30019:
		SetFilterStatus(30019, CIsoViewExt::DrawBasenodesFilter);
		return TRUE;
	case 30020:
		SetFilterStatus(30020, CIsoViewExt::DrawCellTagsFilter);
		return TRUE;
	case 30021:
		SetLayerStatus(30021, CIsoViewExt::DrawVeterancy);
		return TRUE;
	case 30022:
		SetLayerStatus(30022, CIsoViewExt::DrawShadows);
		return TRUE;
	case 30023:
		SetLayerStatus(30023, CIsoViewExt::DrawAlphaImages);
		return TRUE;
	case 30024:
		SetLayerStatus(30024, CIsoViewExt::DrawAnnotations);
		return TRUE;
	case 30025:
		SetLayerStatus(30025, CIsoViewExt::DrawFires);
		return TRUE;
	case 30050:
		SetMenuStatusTrue(30000, CIsoViewExt::DrawStructures);
		SetMenuStatusTrue(30001, CIsoViewExt::DrawInfantries);
		SetMenuStatusTrue(30002, CIsoViewExt::DrawUnits);
		SetMenuStatusTrue(30003, CIsoViewExt::DrawAircrafts);
		SetMenuStatusTrue(30004, CIsoViewExt::DrawBasenodes);
		SetMenuStatusTrue(30005, CIsoViewExt::DrawWaypoints);
		SetMenuStatusTrue(30006, CIsoViewExt::DrawCelltags);
		SetMenuStatusTrue(30007, CIsoViewExt::DrawMoneyOnMap);
		SetMenuStatusTrue(30008, CIsoViewExt::DrawOverlays);
		SetMenuStatusTrue(30009, CIsoViewExt::DrawTerrains);
		SetMenuStatusTrue(30010, CIsoViewExt::DrawSmudges);
		SetMenuStatusTrue(30011, CIsoViewExt::DrawTubes);
		SetMenuStatusTrue(30012, CIsoViewExt::DrawBounds);
		SetMenuStatusTrue(30013, CIsoViewExt::DrawBaseNodeIndex);
		SetMenuStatusTrue(30021, CIsoViewExt::DrawVeterancy);
		SetMenuStatusTrue(30022, CIsoViewExt::DrawShadows);
		SetMenuStatusTrue(30023, CIsoViewExt::DrawAlphaImages);
		SetMenuStatusTrue(30024, CIsoViewExt::DrawAnnotations);
		SetMenuStatusTrue(30025, CIsoViewExt::DrawFires);
		SetMenuStatusFalse(30014, CIsoViewExt::RockCells);
		SetMenuStatusFalse(30015, CIsoViewExt::DrawStructuresFilter);
		SetMenuStatusFalse(30016, CIsoViewExt::DrawInfantriesFilter);
		SetMenuStatusFalse(30017, CIsoViewExt::DrawUnitsFilter);
		SetMenuStatusFalse(30018, CIsoViewExt::DrawAircraftsFilter);
		SetMenuStatusFalse(30019, CIsoViewExt::DrawBasenodesFilter);
		SetMenuStatusFalse(30020, CIsoViewExt::DrawCellTagsFilter);
		::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
		return TRUE;
	case 30052:
	{
		CIsoViewExt::Zoom(0.25);
		return TRUE;
	}
	case 30053:
	{
		CIsoViewExt::Zoom(-0.25);
		return TRUE;
	}
	case 30054:
	{
		auto pBrushSize = (ppmfc::CComboBox*)CFinalSunDlg::Instance->BrushSize.GetDlgItem(1377);
		int index = std::max(pBrushSize->GetCurSel() - 1, 0);
		pBrushSize->SetCurSel(index);
		changeBrushSize(index);
		return TRUE;
	}
	case 30055:
	{
		auto pBrushSize = (ppmfc::CComboBox*)CFinalSunDlg::Instance->BrushSize.GetDlgItem(1377);
		int index = std::min(pBrushSize->GetCurSel() + 1, pBrushSize->GetCount() - 1);
		pBrushSize->SetCurSel(index);
		changeBrushSize(index);
		return TRUE;
	}
	case 30056:
	{
		MultiSelection::Control_D_IsDown = true;
		MultiSelection::Clear();
		::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
		return TRUE;
	}
	case 31000:
	case 31001:
	case 31002:
	case 31003:
		SetLightingStatus(wmID);
		break;
	case 32000:
	case 32001:
	case 32002:
	case 32003:
	{
		HMENU hMenu = *this->GetMenu();
		if (GetMenuState(hMenu, wmID, MF_BYCOMMAND) & MF_CHECKED)
		{
			// set to false
			CIsoViewExt::AutoPropertyBrush[wmID - 32000] = false;
			CheckMenuItem(hMenu, wmID, MF_UNCHECKED);
		}
		else
		{
			// set to true
			CIsoViewExt::AutoPropertyBrush[wmID - 32000] = true;
			CheckMenuItem(hMenu, wmID, MF_CHECKED);
		}
		break;
	}
	case 33000:
	case 33001:
	case 33002:
	case 33003:
	case 33004:
	case 33005:
	case 33006:
	{
		const auto& url = CFinalSunAppExt::ExternalLinks[wmID - 33000].first;
		if (url.empty())
			break;
		std::string command = "cmd.exe /c start \"\" \"" + url + "\"";

		STARTUPINFOA si = { sizeof(si) };
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;

		PROCESS_INFORMATION pi;
		BOOL success = CreateProcessA(
			NULL,
			const_cast<LPSTR>(command.c_str()),
			NULL, NULL, FALSE,
			CREATE_NO_WINDOW,
			NULL, NULL,
			&si, &pi
		);

		if (success) {
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		else
		{
			if (!ShellExecute(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL))
			{
				ppmfc::CString pMessage = Translations::TranslateOrDefault("FailedToOpenLink",
					"Unable to open link, please try manually:");
				pMessage += " ";
				pMessage += url.c_str();
				MessageBox(pMessage);
			}
		}
		break;
	}
	case 34001:
		SetLayerStatus(34001, CIsoViewExt::PasteGround);
		return TRUE;
	case 34002:
		SetLayerStatus(34002, CIsoViewExt::PasteOverlays);
		return TRUE;
	case 34003:
		SetLayerStatus(34003, CIsoViewExt::PasteStructures);
		return TRUE;
	case 34004:
		SetLayerStatus(34004, CIsoViewExt::PasteInfantries);
		return TRUE;
	case 34005:
		SetLayerStatus(34005, CIsoViewExt::PasteUnits);
		return TRUE;
	case 34006:
		SetLayerStatus(34006, CIsoViewExt::PasteAircrafts);
		return TRUE;
	case 34007:
		SetLayerStatus(34007, CIsoViewExt::PasteTerrains);
		return TRUE;
	case 34008:
		SetLayerStatus(34008, CIsoViewExt::PasteSmudges);
		return TRUE;
	case 34051:
		SetLayerStatus(34051, CIsoViewExt::PasteOverriding);
		return TRUE;
	case 34052:
		SetLayerStatus(34052, CIsoViewExt::PasteShowOutline);
		return TRUE;
	case 34050:
		SetMenuStatusTrue(34001, CIsoViewExt::PasteGround);
		SetMenuStatusTrue(34002, CIsoViewExt::PasteOverlays);
		SetMenuStatusTrue(34003, CIsoViewExt::PasteStructures);
		SetMenuStatusTrue(34004, CIsoViewExt::PasteInfantries);
		SetMenuStatusTrue(34005, CIsoViewExt::PasteUnits);
		SetMenuStatusTrue(34006, CIsoViewExt::PasteAircrafts);
		SetMenuStatusTrue(34007, CIsoViewExt::PasteTerrains);
		SetMenuStatusTrue(34008, CIsoViewExt::PasteSmudges);
		return TRUE;
	case 34053:
		SetMenuStatusTrue(34001, CIsoViewExt::PasteGround);
		SetMenuStatusTrue(34002, CIsoViewExt::PasteOverlays);
		SetMenuStatusFalse(34003, CIsoViewExt::PasteStructures);
		SetMenuStatusFalse(34004, CIsoViewExt::PasteInfantries);
		SetMenuStatusFalse(34005, CIsoViewExt::PasteUnits);
		SetMenuStatusFalse(34006, CIsoViewExt::PasteAircrafts);
		SetMenuStatusFalse(34007, CIsoViewExt::PasteTerrains);
		SetMenuStatusFalse(34008, CIsoViewExt::PasteSmudges);
		return TRUE;
	case 40159:
	{
		SetLayerStatus(40159, ExtConfigs::TreeViewCameo_Display);
		((CViewObjectsExt*)(this->MyViewFrame.pViewObjects))->Redraw();

		CINI ini;
		ppmfc::CString path = CFinalSunAppExt::ExePathExt;
		path += "\\FinalAlert.ini";
		ini.ClearAndLoad(path);
		ini.WriteString("UserInterface", "ShowTreeViewCameo", ExtConfigs::TreeViewCameo_Display ? "1" : "0");
		ini.WriteToFile(path);

		return TRUE;
	}
	case 40163:
		SetLayerStatus(40163, CIsoViewExt::EnableDistanceRuler);
		CIsoViewExt::DistanceRuler.clear();
		return TRUE;
	default:
		break;
	}

	if (wmID >= 40140 && wmID < 40149)
	{
		auto& file = CFinalSunAppExt::RecentFilesExt[wmID - 40140];
		if (CLoading::IsFileExists(file.c_str()))
			this->LoadMap(file.c_str());
	}
	if (wmID == 40018 && CMapData::Instance->MapWidthPlusHeight)
	{
		ppmfc::CString buffer;
		buffer = CFinalSunApp::MapPath;
		if (CLoading::IsFileExists(buffer))
			this->LoadMap(CFinalSunApp::MapPath);
	}

	// object search
	if (wmID == 40134 || wmID == 40161)
	{
		auto pTSB = (CTileSetBrowserFrameExt*)CFinalSunDlg::Instance()->MyViewFrame.pTileSetBrowserFrame;
		pTSB->OnBNSearchClicked();
	}

	if (wmID == 40137 && CMapData::Instance->MapWidthPlusHeight)
	{
		auto pTSB = (CTileSetBrowserFrameExt*)CFinalSunDlg::Instance()->MyViewFrame.pTileSetBrowserFrame;
		pTSB->OnBNTileManagerClicked();
	}
	if (wmID == 40138)
	{
		if (CNewTeamTypes::GetHandle() == NULL)
			CNewTeamTypes::Create((CFinalSunDlg*)this);
		else
		{
			::SendMessage(CNewTeamTypes::GetHandle(), 114514, 0, 0);
		}

	}
	if (wmID == 40139)
	{
		if (CNewTaskforce::GetHandle() == NULL)
			CNewTaskforce::Create((CFinalSunDlg*)this);
		else
		{
			::SendMessage(CNewTaskforce::GetHandle(), 114514, 0, 0);
		}

	}
	if (wmID == 40150)
	{
		if (CNewScript::GetHandle() == NULL)
			CNewScript::Create((CFinalSunDlg*)this);
		else
		{
			::SendMessage(CNewScript::GetHandle(), 114514, 0, 0);
		}

	}
	if (wmID == 40151)
	{
		if (CNewTrigger::Instance[0].GetHandle() == NULL)
			CNewTrigger::Instance[0].Create((CFinalSunDlg*)this);
		else
		{
			::SendMessage(CNewTrigger::Instance[0].GetHandle(), 114514, 0, 0);
		}
	}
	if (wmID == 40168)
	{
		if (CBatchTrigger::GetHandle() == NULL)
			CBatchTrigger::Create((CFinalSunDlg*)this);
		else
		{
			::SendMessage(CBatchTrigger::GetHandle(), 114514, 0, 0);
		}
	}
	if (wmID == 40154)
	{
		if (CNewINIEditor::GetHandle() == NULL)
			CNewINIEditor::Create((CFinalSunDlg*)this);
		else
		{
			::SendMessage(CNewINIEditor::GetHandle(), 114514, 0, 0);
		}
	}
	if (wmID == 40155)
	{
		if (CCsfEditor::GetHandle() == NULL)
			CCsfEditor::Create((CFinalSunDlg*)this);
		else
		{
			::SendMessage(CCsfEditor::GetHandle(), 114514, 0, 0);
		}
	}
	if (wmID == 40156)
	{
		if (CNewAITrigger::GetHandle() == NULL)
			CNewAITrigger::Create((CFinalSunDlg*)this);
		else
		{
			::SendMessage(CNewAITrigger::GetHandle(), 114514, 0, 0);
		}
	}
	if (wmID == 40157)
	{
		const FString title = Translations::TranslateOrDefault(
			"Error", "Error"
		);
		const FString message = Translations::TranslateOrDefault(
			"SelectAutoShoreNotFound", "This theater does not have available shore options."
		);
		bool found = false;
		if (auto pSection = CINI::FAData->GetSection("AutoShoreTypes"))
		{
			for (const auto& type : pSection->GetEntities())
			{
				auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");
				if (STDHelpers::SplitString(type.second)[0] == thisTheater)
				{
					found = true;
				}
			}
		}
		if (found)
		{
			CSelectAutoShore dlg;
			dlg.DoModal();
			if (dlg.m_Combo != "" && !dlg.Groups.empty())
			{
				CMapDataExt::AutoShore_ShoreTileSet = dlg.Groups[atoi(dlg.m_Combo)].first;
				CMapDataExt::AutoShore_GreenTileSet = dlg.Groups[atoi(dlg.m_Combo)].second;
			}
		}
		else
		{
			::MessageBox(CFinalSunDlg::Instance()->MyViewFrame.pIsoView->m_hWnd, message, title, MB_ICONWARNING);
		}
	}
	if (wmID == 40158)
	{
		if (CLuaConsole::GetHandle() == NULL)
			CLuaConsole::Create((CFinalSunDlg*)this);
		else
		{
			::SendMessage(CLuaConsole::GetHandle(), 114514, 0, 0);
		}
	}
	if (wmID == 40160 && CMapData::Instance->MapWidthPlusHeight)
	{
		if (CNewLocalVariables::GetHandle() == NULL)
			CNewLocalVariables::Create((CFinalSunDlg*)this);
		else
		{
			::SendMessage(CNewLocalVariables::GetHandle(), 114514, 0, 0);
		}
	}
	if (wmID == 40162)
	{
		if (CFA2spOptions::GetHandle() == NULL)
			CFA2spOptions::Create((CFinalSunDlg*)this);
		else
		{
			::SendMessage(CFA2spOptions::GetHandle(), 114514, 0, 0);
		}
	}
	if (wmID == 40164)
	{
		if (CTriggerAnnotation::GetHandle() == NULL)
			CTriggerAnnotation::Create((CFinalSunDlg*)this);
		else
		{
			::SendMessage(CTriggerAnnotation::GetHandle(), 114514, 0, 0);
		}
	}
	if (wmID == 40165)
	{
		auto setLighting = [](int id)
		{
			if (CFinalSunDlgExt::CurrentLighting != id)
			{
				auto& pThis = CFinalSunDlg::Instance;
				CFinalSunDlgExt::CurrentLighting = id;
				LightingStruct::GetCurrentLighting();

				for (int i = 0; i < CMapData::Instance->MapWidthPlusHeight; i++) {
					for (int j = 0; j < CMapData::Instance->MapWidthPlusHeight; j++) {
						CMapData::Instance->UpdateMapPreviewAt(i, j);
					}
				}
				LightingSourceTint::CalculateMapLamps();

				pThis->MyViewFrame.Minimap.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
				::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
				auto tmp = CIsoView::CurrentCommand->Command;
				if (pThis->MyViewFrame.pTileSetBrowserFrame->View.CurrentMode == 1) {
					HWND hParent = pThis->MyViewFrame.pTileSetBrowserFrame->DialogBar.GetSafeHwnd();
					HWND hTileComboBox = ::GetDlgItem(hParent, 1366);
					::SendMessage(hParent, WM_COMMAND, MAKEWPARAM(1366, CBN_SELCHANGE), (LPARAM)hTileComboBox);
					CIsoView::CurrentCommand->Command = tmp;
				}
			}
		};
		auto renderMap = [&](FString path, bool batchProcess)
		{
			CIsoViewExt::InitGdiplus();
			CIsoViewExt::RenderingMap = true;
			if (batchProcess)
			{
				CMapData::Instance->LoadMap(path);
				FString str;
				str = Translations::TranslateOrDefault("MainDialogCaption", "%9");
				str += " (";
				str += path;
				str += ")";
				CFinalSunDlg::Instance->SetWindowText(str);
				strcpy_s(CFinalSunApp::MapPath(), path);
			}
			Gdiplus::Status result = Gdiplus::Status::AccessDenied;
			if (path.empty())
				path = CFinalSunAppExt::ExePathExt + "New map";
			size_t lastSlash = path.find_last_of("\\/");
			size_t lastDot = path.find_last_of('.');
			if (lastDot != std::string::npos && (lastSlash == std::string::npos || lastDot > lastSlash))
				path = path.substr(0, lastDot);
			path += CIsoViewExt::RenderSaveAsPNG ? ".png" : ".jpg";

			auto wpath = STDHelpers::StringToWString(path);
			int currentlighting = CFinalSunDlgExt::CurrentLighting;

			switch (CIsoViewExt::RenderLighing)
			{
			case None:
				setLighting(31000);
				break;
			case Normal:
				setLighting(31001);
				break;
			case LightningStorm:
				setLighting(31002);
				break;
			case Dominator:
				setLighting(31003);
				break;
			case Current:
			default:
				break;
			}

			auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");
			thisTheater.MakeUpper();
			if (thisTheater == "NEWURBAN")
				thisTheater = "UBN";
			FString theaterSuffix = thisTheater.Mid(0, 3);
			CIsoViewExt::MapRendererIgnoreObjects.clear();
			if (CIsoViewExt::RenderIgnoreObjects)
			{
				FString ignoreSection = "MapRendererIgnoreObjects";
				if (auto pSection = CINI::FAData->GetSection(ignoreSection))
				{
					for (auto& [_, ID] : pSection->GetEntities())
					{
						CIsoViewExt::MapRendererIgnoreObjects.insert(ID);
					}
				}
				if (auto pSection = CINI::FAData->GetSection(ignoreSection + theaterSuffix))
				{
					for (auto& [_, ID] : pSection->GetEntities())
					{
						CIsoViewExt::MapRendererIgnoreObjects.insert(ID);
					}
				}
			}
			if (!CIsoViewExt::RenderInvisibleInGame)
			{
				FString ignoreSection = "MapRendererIgnoreObjects";
				const auto&& buildings = Variables::RulesMap.GetSection("BuildingTypes");
				for (auto& [_, ID] : buildings)
				{
					if (Variables::RulesMap.GetBool(ID, "InvisibleInGame"))
						CIsoViewExt::MapRendererIgnoreObjects.insert(ID);
				}
				const auto& overlays = Variables::RulesMap.GetSection("OverlayTypes");
				for (auto& [_, ID] : overlays)
				{
					if (Variables::RulesMap.GetBool(ID, "IsRubble"))
						CIsoViewExt::MapRendererIgnoreObjects.insert(ID);
				}
			}

			auto pIsoView = CIsoView::GetInstance();

			int& height = CMapData::Instance->Size.Height;
			int& width = CMapData::Instance->Size.Width;
			int startX, startY, endX, endY;
			int startPointX, startPointY, endPointX, endPointY;
			if (CIsoViewExt::RenderFullMap)
			{
				startX = width - 1;
				startY = 0;
				endX = height;
				endY = width + height + 1;
			}
			else
			{
				int mpL = std::max(CMapData::Instance->LocalSize.Left, 0);
				int mpT = std::max(CMapData::Instance->LocalSize.Top, 0);
				int mpW = std::min(CMapData::Instance->LocalSize.Width, CMapData::Instance->Size.Width);
				int mpH = std::min(CMapData::Instance->LocalSize.Height, CMapData::Instance->Size.Height);

				startY = mpT + mpL - 2;
				startX = width + mpT - mpL - 3;

				endX = width - mpL - mpW + mpT - 3 + mpH + 4;
				endY = mpT + mpL + mpW - 2 + mpH + 4;
			}

			startPointX = startX;
			startPointY = startY;
			endPointX = endX;
			endPointY = endY;
			pIsoView->MapCoord2ScreenCoord_Flat(startPointX, startPointY);
			pIsoView->MapCoord2ScreenCoord_Flat(endPointX, endPointY);
			if (CIsoViewExt::RenderFullMap)
				endPointY -= 15;

			VEHGuard v(false);
			try {
				CIsoViewExt::pFullBitmap = new Bitmap(endPointX - startPointX, endPointY - startPointY, PixelFormat24bppRGB);
			}
			catch (const std::bad_alloc&) {
				if (!batchProcess)
				{
					const FString title = Translations::TranslateOrDefault(
						"Error", "Error"
					);
					const FString message = Translations::TranslateOrDefault(
						"AllocFullMapBitmapFailed", "Memory allocation failed, cannot render full map."
					);
					::MessageBox(CFinalSunDlg::Instance()->MyViewFrame.pIsoView->m_hWnd, message, title, MB_ICONWARNING);
				}
				CIsoViewExt::pFullBitmap = nullptr;
			}

			if (CIsoViewExt::pFullBitmap)
			{
				Graphics gInit(CIsoViewExt::pFullBitmap);
				gInit.Clear(Color(0, 0, 0, 0));

				CRect r;
				pIsoView->GetWindowRect(&r);

				CRect validRange;
				validRange.left = 30 * (height + width + startY - startX) - (r.right - r.left) / 2 - r.left;
				validRange.top = 15 * (startY + startX) - (r.bottom - r.top) / 2 - r.top;
				validRange.right = 30 * (height + width + endY - endX) - (r.right - r.left) / 2 - r.left;
				validRange.bottom = 15 * (endY + endX) - (r.bottom - r.top) / 2 - r.top;

				pIsoView->ViewPosition.y = validRange.top;

				int totalTileCount = ((validRange.right - validRange.left + r.Width()) / r.Width() + 1)
					* ((validRange.bottom - validRange.top + r.Height()) / r.Height() + 1) - 1;

				CUpdateProgress progress(
					Translations::TranslateOrDefault("MapRendererProgressText",
						"Rendering, please wait..."), NULL);
				progress.ShowWindow(SW_SHOW);
				progress.UpdateWindow();
				progress.ProgressBar.SetRange(0, totalTileCount + 1);
				progress.ProgressBar.SetPos(0);

				EnableScrollBar(pIsoView->GetSafeHwnd(), SB_BOTH, ESB_DISABLE_BOTH);

				int currentTile = 0;
				static int renderFailedCount;
				renderFailedCount = 0;
				while (pIsoView->ViewPosition.y < validRange.bottom + r.Height())
				{
					pIsoView->ViewPosition.x = validRange.left;
					while (pIsoView->ViewPosition.x < validRange.right + r.Width())
					{
						::SetScrollPos(pIsoView->GetSafeHwnd(), SB_VERT, pIsoView->ViewPosition.y / 30 - width / 2 + 4, TRUE);
						::SetScrollPos(pIsoView->GetSafeHwnd(), SB_HORZ, pIsoView->ViewPosition.x / 60 - height / 2 + 1, TRUE);
						CIsoViewExt::RenderTileSuccess = false;

						FString message;
						message.Format(Translations::TranslateOrDefault("MapRendererToolbarRendering",
							"Map Renderer: rendering tile (%d/%d)"), currentTile, totalTileCount);
						CIsoViewExt::SetStatusBarText(message);

						pIsoView->Draw();

						progress.ProgressBar.SetPos(currentTile);
						progress.ProgressBar.UpdateWindow();

						MSG msg;
						if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
						{
							TranslateMessage(&msg);
							DispatchMessage(&msg);
						}
						Sleep(1);

						if (CIsoViewExt::RenderTileSuccess || renderFailedCount >= 500) {
							pIsoView->ViewPosition.x += r.Width();
							currentTile++;
						}
						else {
							renderFailedCount++;
						}
					}
					pIsoView->ViewPosition.y += r.Height();
				}

				EnableScrollBar(pIsoView->GetSafeHwnd(), SB_BOTH, ESB_ENABLE_BOTH);

				FString message;
				message.Format(Translations::TranslateOrDefault("MapRendererToolbarSaving",
					"Map Renderer: saving png file to %s"), path);
				CIsoViewExt::SetStatusBarText(message);

				CLSID clsidEncoder;
				UINT num = 0, size = 0;
				GetImageEncodersSize(&num, &size);
				ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)malloc(size);
				GetImageEncoders(num, size, pImageCodecInfo);
				for (UINT i = 0; i < num; ++i)
				{
					if (wcscmp(pImageCodecInfo[i].MimeType, 
						CIsoViewExt::RenderSaveAsPNG ? L"image/png" : L"image/jpeg") == 0)
					{
						clsidEncoder = pImageCodecInfo[i].Clsid;
						break;
					}
				}
				free(pImageCodecInfo);

				ULONG quality = 80;
				EncoderParameters encoderParams{};
				encoderParams.Count = 1;
				encoderParams.Parameter[0].Guid = EncoderQuality;
				encoderParams.Parameter[0].Type = EncoderParameterValueTypeLong;
				encoderParams.Parameter[0].NumberOfValues = 1;
				encoderParams.Parameter[0].Value = &quality;

				result = CIsoViewExt::pFullBitmap->Save(wpath.c_str(), &clsidEncoder, 
					CIsoViewExt::RenderSaveAsPNG ? nullptr : &encoderParams);
				delete CIsoViewExt::pFullBitmap;
				CIsoViewExt::pFullBitmap = nullptr;
			}
			CIsoViewExt::RenderingMap = false;

			if (CIsoViewExt::RenderLighing != Current)
				setLighting(currentlighting);

			CIsoViewExt::MoveToMapCoord(CMapData::Instance->MapWidthPlusHeight / 2, CMapData::Instance->MapWidthPlusHeight / 2);

			if (result == Gdiplus::Status::Ok && !batchProcess)
			{
				FString templ = Translations::TranslateOrDefault(
					"MapRendererSuccess", "Saving output to"
				);
				templ += "\n%s";
				FString message;
				message.Format(templ, path);
				::MessageBox(CFinalSunDlg::Instance()->MyViewFrame.pIsoView->m_hWnd, message, "FA2sp", MB_ICONINFORMATION);
			}

			CIsoViewExt::pFullBitmap = nullptr;

		};

		CMapRendererDlg dlg;
		if (dlg.DoModal() != IDCANCEL)
		{
			bool batchProcess = false;
			if (!dlg.BatchPaths.empty())
			{
				dlg.BatchPaths.erase(
					std::remove_if(dlg.BatchPaths.begin(), dlg.BatchPaths.end(),
						[](const FString& p) {
					return !fs::exists(fs::path(p.c_str()));
				}),
					dlg.BatchPaths.end());
				batchProcess = !dlg.BatchPaths.empty();
			}

			if (!CMapData::Instance->MapWidthPlusHeight && !batchProcess)
				return TRUE;

			TempValueHolder<double> scaled(CIsoViewExt::ScaledFactor, 1.0);
			std::vector<std::unique_ptr<TempValueHolder<bool>>> holders;
			std::vector<std::unique_ptr<TempValueHolder<BOOL>>> holders2;

			if (!CIsoViewExt::RenderCurrentLayers)
			{
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawStructures, true);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawInfantries, true);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawUnits, true);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawAircrafts, true);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawBasenodes, false);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawWaypoints, false);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawCelltags, false);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawMoneyOnMap, false);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawOverlays, true);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawTerrains, true);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawSmudges, true);
				if (dlg.b_Tube)
					ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawTubes, true);
				else
					ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawTubes, false);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawBounds, false);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawVeterancy, true);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawShadows, true);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawAlphaImages, true);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawAnnotations, false);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawFires, true);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawBaseNodeIndex, false);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::RockCells, false);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawStructuresFilter, false);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawInfantriesFilter, false);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawUnitsFilter, false);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawAircraftsFilter, false);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawBasenodesFilter, false);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawCellTagsFilter, false);
				ADD_TEMP_HOLDER(holders, CIsoViewExt::EnableDistanceRuler, false);
				ADD_TEMP_HOLDER(holders, ExtConfigs::InGameDisplay_Cloakable, true);
				ADD_TEMP_HOLDER(holders2, CFinalSunApp::Instance->ShowBuildingCells, FALSE);
				ADD_TEMP_HOLDER(holders2, CFinalSunApp::Instance->FlatToGround, FALSE);
				ADD_TEMP_HOLDER(holders2, CFinalSunApp::Instance->FrameMode, FALSE);
			}
			else if (dlg.b_Tube)
			{
				ADD_TEMP_HOLDER(holders, CIsoViewExt::DrawTubes, true);
			}

			if (!batchProcess)
			{
				renderMap(CFinalSunApp::MapPath(), false);
			}
			else
			{
				for (const auto& p : dlg.BatchPaths)
				{
					renderMap(p, true);
				}
			}
		}
	}
	if (wmID == 40167)
	{
		if (!CMapData::Instance->MapWidthPlusHeight)
		{
			this->PlaySound(FASoundType::Error);
		}
		else
		{
			FString path(CFinalSunApp::MapPath);
			if (path != "")
			{
				MessageBeep(MB_ICONWARNING);
				SaveMapExt::SaveMapSilent(path);
			}
			else
			{
				this->SaveMapAs();
			}
		}
	}
	auto closeFA2Window = [this, &wmID](int wmID2, ppmfc::CDialog& dialog)
	{
		if (wmID2 == wmID)
		{
			if (dialog.m_hWnd != NULL)
			{
				dialog.EndDialog(NULL);
			}
		}
	};

	closeFA2Window(40040, this->MapD);
	closeFA2Window(40039, this->Houses);
	closeFA2Window(40036, this->Basic);
	closeFA2Window(40038, this->SpecialFlags);
	closeFA2Window(40043, this->Lighting);
	closeFA2Window(40048, this->AITriggerTypesEnable);
	closeFA2Window(40037, this->SingleplayerSettings);
	closeFA2Window(40042, this->Tags);

	if (wmID == 40152 && CMapData::Instance->MapWidthPlusHeight)
	{
		const FString title = Translations::TranslateOrDefault(
			"AutocreateLAT", "Autocreate LAT"
		);
		const FString message = Translations::TranslateOrDefault(
			"AutocreateLATmessage", "FA2 will recalculate LAT for the entire Map according to the rules of the game engine, which can be undone using the undo key. Do you want to continue?"
		);
		int result = ::MessageBox(CFinalSunDlg::Instance()->MyViewFrame.pIsoView->m_hWnd, message, title, MB_YESNO);

		if (result == IDYES)
		{
			CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);

			for (int x = 0; x < CMapData::Instance->MapWidthPlusHeight; x++)
				for (int y = 0; y < CMapData::Instance->MapWidthPlusHeight; y++)
					CMapDataExt::SmoothTileAt(x, y, true);

			::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
		}

	}
	if (wmID == 40153 && CMapData::Instance->MapWidthPlusHeight)
	{
		const FString title = Translations::TranslateOrDefault(
			"SmoothWater", "Smooth Water"
		);
		const FString message = Translations::TranslateOrDefault(
			"SmoothWatermessage", "FA2 will regenerate the water and eliminate fragmented terrain tiles, which can be undone using the undo key. Do you want to continue?"
		);
		int result = ::MessageBox(CFinalSunDlg::Instance()->MyViewFrame.pIsoView->m_hWnd, message, title, MB_YESNO);

		if (result == IDYES)
		{
			CMapData::Instance->SaveUndoRedoData(true, 0, 0, 0, 0);

			CMapDataExt::SmoothWater();

			::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
		}

	}
	//delete objects
	if (wmID == 40136 && CMapData::Instance->MapWidthPlusHeight)
	{
		if (isInChildWindow())
			return TRUE;

		CIsoView::CurrentCommand->Command = 0x2; // delete
		CIsoView::CurrentCommand->Type = 0;

		MessageBeep(MB_ICONWARNING);

		POINT ptScreen;
		::GetCursorPos(&ptScreen);
		::ScreenToClient(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, &ptScreen);
		LPARAM lParam = MAKELPARAM(ptScreen.x, ptScreen.y);
		WPARAM wParam = 0;
		::SendMessage(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, WM_MOUSEMOVE, wParam, lParam);
	}

	//F5
	if (wmID == 40053)
	{
		HWND hWnd = GetActiveWindow();
		if (hWnd == CNewAITrigger::GetHandle()) {
			::SendMessage(CNewTeamTypes::GetHandle(), 114514, 0, 0);
			return TRUE;
		}
		else if (hWnd == CNewINIEditor::GetHandle()) {
			::SendMessage(CNewINIEditor::GetHandle(), 114514, 0, 0);
			return TRUE;
		}
		else if (hWnd == CNewScript::GetHandle()) {
			::SendMessage(CNewScript::GetHandle(), 114514, 0, 0);
			return TRUE;
		}
		else if (hWnd == CNewTaskforce::GetHandle()) {
			::SendMessage(CNewTaskforce::GetHandle(), 114514, 0, 0);
			return TRUE;
		}
		else if (hWnd == CNewTeamTypes::GetHandle()) {
			::SendMessage(CNewTeamTypes::GetHandle(), 114514, 0, 0);
			return TRUE;
		}
		else if (hWnd == CTerrainGenerator::GetHandle()) {
			::SendMessage(CTerrainGenerator::GetHandle(), 114514, 0, 0);
			return TRUE;
		}
		else if (hWnd == CFA2spOptions::GetHandle()) {
			::SendMessage(CFA2spOptions::GetHandle(), 114514, 0, 0);
			return TRUE;
		}
		else if (hWnd == CNewLocalVariables::GetHandle()) {
			::SendMessage(CNewLocalVariables::GetHandle(), 114514, 0, 0);
			return TRUE;
		}
		else if (hWnd == CTriggerAnnotation::GetHandle()) {
			::SendMessage(CTriggerAnnotation::GetHandle(), 114514, 0, 0);
			return TRUE;
		}
		else if (hWnd == CCsfEditor::GetHandle()) {
			::SendMessage(CCsfEditor::GetHandle(), 114514, 0, 0);
			return TRUE;
		}
		else if (hWnd == CLuaConsole::GetHandle()) {
			::SendMessage(CLuaConsole::GetHandle(), 114514, 0, 0);
			return TRUE;
		}
		else if (hWnd == CBatchTrigger::GetHandle()) {
			::SendMessage(CBatchTrigger::GetHandle(), 114514, 0, 0);
			return TRUE;
		}
		else if (hWnd == CSearhReference::GetHandle()) {
			::SendMessage(CSearhReference::GetHandle(), 114514, 0, 0);
			return TRUE;
		}

		for (int i = 0; i < TRIGGER_EDITOR_MAX_COUNT; ++i)
		{
			if (hWnd == CNewTrigger::Instance[i].GetHandle()) {
				::SendMessage(CNewTrigger::Instance[i].GetHandle(), 114514, 0, 0);
				return TRUE;
			}
		}

		int newParam = 0;
		auto refreshFA2Window = [this, &hWnd, &newParam, wmMsg](int wmID2, ppmfc::CDialog& dialog)
			{
				if (dialog.m_hWnd == hWnd)
				{
					dialog.EndDialog(IDCANCEL);
					newParam = MAKELONG(wmID2, wmMsg);
				}
				return false;
			};

		refreshFA2Window(40040, this->MapD);
		refreshFA2Window(40039, this->Houses);
		refreshFA2Window(40036, this->Basic);
		refreshFA2Window(40038, this->SpecialFlags);
		refreshFA2Window(40043, this->Lighting);
		refreshFA2Window(40048, this->AITriggerTypesEnable);
		refreshFA2Window(40037, this->SingleplayerSettings);
		refreshFA2Window(40042, this->Tags);

		if (newParam != 0)
			return this->ppmfc::CDialog::OnCommand(newParam, lParam);

	}
	// CTRL+C CTRL+V CTRL+Z CTRL+Y CTRL+X
	if (wmID == 57634 || wmID == 57637 || wmID == 57643 || wmID == 57644 || wmID == 40166)
	{
		if (isInChildWindow())
			return TRUE;
	}
	CopyPaste::IsCutting = false;
	if (wmID == 40166)
	{
		// cutting also uses copying logic
		CopyPaste::IsCutting = true;
		auto newParam = MAKELONG(57634, wmMsg);
		return this->ppmfc::CDialog::OnCommand(newParam, lParam);
	}

	return this->ppmfc::CDialog::OnCommand(wParam, lParam);
}
bool CFinalSunDlgExt::CheckProperty_Vehicle(CUnitData data)
{
	if (!CViewObjectsExt::VehicleBrushDlg)
		return true;

	auto CheckValue = [&](int nCheckBoxIdx, ppmfc::CString& src, ppmfc::CString& dst)
		{
			if (CViewObjectsExt::VehicleBrushBools[nCheckBoxIdx - 1300])
			{
				if (dst == src) return true;
				else return false;
			}
			return true;
		};
	if (
		CheckValue(1300, CViewObjectsExt::VehicleBrushDlg->CString_House, data.House) &&
		CheckValue(1301, CViewObjectsExt::VehicleBrushDlg->CString_HealthPoint, data.Health) &&
		CheckValue(1302, CViewObjectsExt::VehicleBrushDlg->CString_State, data.Status) &&
		CheckValue(1303, CViewObjectsExt::VehicleBrushDlg->CString_Direction, data.Facing) &&
		CheckValue(1304, CViewObjectsExt::VehicleBrushDlg->CString_VeteranLevel, data.VeterancyPercentage) &&
		CheckValue(1305, CViewObjectsExt::VehicleBrushDlg->CString_Group, data.Group) &&
		CheckValue(1306, CViewObjectsExt::VehicleBrushDlg->CString_OnBridge, data.IsAboveGround) &&
		CheckValue(1307, CViewObjectsExt::VehicleBrushDlg->CString_FollowerID, data.FollowsIndex) &&
		CheckValue(1308, CViewObjectsExt::VehicleBrushDlg->CString_AutoCreateNoRecruitable, data.AutoNORecruitType) &&
		CheckValue(1309, CViewObjectsExt::VehicleBrushDlg->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType) &&
		CheckValue(1310, CViewObjectsExt::VehicleBrushDlg->CString_Tag, data.Tag)
		)
		return true;
	else
		return false;
}

bool CFinalSunDlgExt::CheckProperty_Building(CBuildingData data)
{
	if (!CViewObjectsExt::BuildingBrushDlg)
		return true;

	auto CheckValue = [&](int nCheckBoxIdx, ppmfc::CString& src, ppmfc::CString& dst)
		{
			if (CViewObjectsExt::BuildingBrushBools[nCheckBoxIdx - 1300])
			{
				if (dst == src) return true;
				else return false;
			}
			return true;
		};
	if (
		CheckValue(1300, CViewObjectsExt::BuildingBrushDlg->CString_House, data.House) &&
		CheckValue(1301, CViewObjectsExt::BuildingBrushDlg->CString_HealthPoint, data.Health) &&
		CheckValue(1302, CViewObjectsExt::BuildingBrushDlg->CString_Direction, data.Facing) &&
		CheckValue(1303, CViewObjectsExt::BuildingBrushDlg->CString_Sellable, data.AISellable) &&
		CheckValue(1304, CViewObjectsExt::BuildingBrushDlg->CString_Rebuildable, data.AIRebuildable) &&
		CheckValue(1305, CViewObjectsExt::BuildingBrushDlg->CString_EnergySupport, data.PoweredOn) &&
		CheckValue(1306, CViewObjectsExt::BuildingBrushDlg->CString_UpgradeCount, data.Upgrades) &&
		CheckValue(1307, CViewObjectsExt::BuildingBrushDlg->CString_Spotlight, data.SpotLight) &&
		CheckValue(1308, CViewObjectsExt::BuildingBrushDlg->CString_Upgrade1, data.Upgrade1) &&
		CheckValue(1309, CViewObjectsExt::BuildingBrushDlg->CString_Upgrade2, data.Upgrade2) &&
		CheckValue(1310, CViewObjectsExt::BuildingBrushDlg->CString_Upgrade3, data.Upgrade3) &&
		CheckValue(1311, CViewObjectsExt::BuildingBrushDlg->CString_AIRepairs, data.AIRepairable) &&
		CheckValue(1312, CViewObjectsExt::BuildingBrushDlg->CString_ShowName, data.Nominal) &&
		CheckValue(1313, CViewObjectsExt::BuildingBrushDlg->CString_Tag, data.Tag)
		)
	return true;
	else
		return false;
}

bool CFinalSunDlgExt::CheckProperty_Infantry(CInfantryData data)
{
	if (!CViewObjectsExt::InfantryBrushDlg)
		return true;

	auto CheckValue = [&](int nCheckBoxIdx, ppmfc::CString& src, ppmfc::CString& dst)
		{
			if (CViewObjectsExt::InfantryBrushBools[nCheckBoxIdx - 1300])
			{
				if (dst == src) return true;
				else return false;
			}
			return true;
		};
	if (
		CheckValue(1300, CViewObjectsExt::InfantryBrushDlg->CString_House, data.House) &&
		CheckValue(1301, CViewObjectsExt::InfantryBrushDlg->CString_HealthPoint, data.Health) &&
		CheckValue(1302, CViewObjectsExt::InfantryBrushDlg->CString_State, data.Status) &&
		CheckValue(1303, CViewObjectsExt::InfantryBrushDlg->CString_Direction, data.Facing) &&
		CheckValue(1304, CViewObjectsExt::InfantryBrushDlg->CString_VerteranStatus, data.VeterancyPercentage) &&
		CheckValue(1305, CViewObjectsExt::InfantryBrushDlg->CString_Group, data.Group) &&
		CheckValue(1306, CViewObjectsExt::InfantryBrushDlg->CString_OnBridge, data.IsAboveGround) &&
		CheckValue(1307, CViewObjectsExt::InfantryBrushDlg->CString_AutoCreateNoRecruitable, data.AutoNORecruitType) &&
		CheckValue(1308, CViewObjectsExt::InfantryBrushDlg->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType) &&
		CheckValue(1309, CViewObjectsExt::InfantryBrushDlg->CString_Tag, data.Tag)
		)
	return true;
	else
		return false;
}

bool CFinalSunDlgExt::CheckProperty_Aircraft(CAircraftData data)
{
	if (!CViewObjectsExt::AircraftBrushDlg)
		return true;

	auto CheckValue = [&](int nCheckBoxIdx, ppmfc::CString& src, ppmfc::CString& dst)
		{
			if (CViewObjectsExt::AircraftBrushBools[nCheckBoxIdx - 1300])
			{
				if (dst == src) return true;
				else return false;
			}
			return true;
		};
	if (
		CheckValue(1300, CViewObjectsExt::AircraftBrushDlg->CString_House, data.House) &&
		CheckValue(1301, CViewObjectsExt::AircraftBrushDlg->CString_HealthPoint, data.Health) &&
		CheckValue(1302, CViewObjectsExt::AircraftBrushDlg->CString_Direction, data.Facing) &&
		CheckValue(1303, CViewObjectsExt::AircraftBrushDlg->CString_Status, data.Status) &&
		CheckValue(1304, CViewObjectsExt::AircraftBrushDlg->CString_VeteranLevel, data.VeterancyPercentage) &&
		CheckValue(1305, CViewObjectsExt::AircraftBrushDlg->CString_Group, data.Group) &&
		CheckValue(1306, CViewObjectsExt::AircraftBrushDlg->CString_AutoCreateNoRecruitable, data.AutoNORecruitType) &&
		CheckValue(1307, CViewObjectsExt::AircraftBrushDlg->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType) &&
		CheckValue(1308, CViewObjectsExt::AircraftBrushDlg->CString_Tag, data.Tag)
		)
	return true;
	else
		return false;
}

BOOL CFinalSunDlgExt::PreTranslateMessageExt(MSG* pMsg)
{
	if (pMsg->message == WM_SYSKEYDOWN || pMsg->message == WM_SYSKEYUP) {
		if (pMsg->wParam == VK_MENU) {
			return TRUE;
		}
	}
	switch (pMsg->message)
	{
	//case WM_INITDIALOG:
	//	;
	//  SetWindowTheme(*this, L"DarkMode_Explorer", NULL);
	case WM_KEYDOWN:
		if (pMsg->wParam == VK_RETURN)
		{
			HWND hWnd = GetFocus();					// EDIT		COMBOBOX_DROPDOWN
			HWND hParent1 = ::GetParent(hWnd);		// WINDOW	COMBOBOX
			HWND hParent2 = ::GetParent(hParent1);	//			WINDOW
			ExtraWindow::bEnterSearch = true;
			if (hParent2 == CNewAITrigger::GetHandle()) {
				if (CNewAITrigger::OnEnterKeyDown(hParent1)) {
					ExtraWindow::bEnterSearch = false;
					return TRUE;
				}
			}
			else if (hWnd == ::GetDlgItem(CObjectSearch::GetHandle(), CObjectSearch::Input)) {
				::ShowWindow(CObjectSearch::GetHandle(), SW_SHOW);
				::SendMessage(CObjectSearch::GetHandle(), 114514, 0, 0);
				ExtraWindow::bEnterSearch = false;
				return TRUE;
			}
			else if (hParent1 == CNewINIEditor::GetHandle()) {
				if (CNewINIEditor::OnEnterKeyDown(hWnd)) {
					ExtraWindow::bEnterSearch = false;
					return TRUE;
				}
			}
			else if (hParent1 == CCsfEditor::GetHandle()) {
				if (CCsfEditor::OnEnterKeyDown(hWnd)) {
					ExtraWindow::bEnterSearch = false;
					return TRUE;
				}
			}
			else if (hParent2 == CNewScript::GetHandle()) {
				if (CNewScript::OnEnterKeyDown(hParent1)) {
					ExtraWindow::bEnterSearch = false;
					return TRUE;
				}
			}
			else if (hParent2 == CNewTaskforce::GetHandle()) {
				if (CNewTaskforce::OnEnterKeyDown(hParent1)) {
					ExtraWindow::bEnterSearch = false;
					return TRUE;
				}
			}
			else if (hParent2 == CNewTeamTypes::GetHandle()) {
				if (CNewTeamTypes::OnEnterKeyDown(hParent1)) {
					ExtraWindow::bEnterSearch = false;
					return TRUE;
				}
			}
			else if (hParent2 == CTerrainGenerator::GetHandle()) {
				if (CTerrainGenerator::OnEnterKeyDown(hParent1)) {
					ExtraWindow::bEnterSearch = false;
					return TRUE;
				}
			}
			for (int i = 0; i < TRIGGER_EDITOR_MAX_COUNT; ++i)
			{
				if (hParent2 == CNewTrigger::Instance[i].GetHandle()) {
					if (CNewTrigger::Instance[i].OnEnterKeyDown(hParent1)) {
						ExtraWindow::bEnterSearch = false;
						return TRUE;
					}
				}
			}
			ExtraWindow::bEnterSearch = false;
		}
		if (CIsoView::CurrentCommand->Command == 0x1E)
		{
			for (int i = 0; i < 10; ++i)
			{
				if (pMsg->wParam == i + '0' || pMsg->wParam == i + VK_NUMPAD0)
				{
					int ctIndex = CIsoView::CurrentCommand->Type;
					auto& info = CViewObjectsExt::TreeView_ConnectedTileMap[ctIndex];
					auto& tileSet = CViewObjectsExt::ConnectedTileSets[info.Index];
					if (tileSet.ToSetPress[i] > -1)
					{
						for (auto& [ctIndex2, info2] : CViewObjectsExt::TreeView_ConnectedTileMap)
						{
							if (info2.Index == tileSet.ToSetPress[i] && info2.Front == info.Front)
							{
								CIsoView::CurrentCommand->Type = ctIndex2;
							}
						}
					}
				}
			}
		}

		break;
	case WM_MOUSEWHEEL:
	{
		if (GetKeyState(VK_CONTROL) & 0x8000)
		{
			HWND hWnd = GetFocus();					// EDIT		COMBOBOX_DROPDOWN
			HWND hParent1 = ::GetParent(hWnd);		// WINDOW	COMBOBOX
			if (hParent1 != CNewINIEditor::GetHandle()
				&& hParent1 != CCsfEditor::GetHandle()
				&& hParent1 != CNewINIEditor::GetImporter()
				&& hParent1 != CLuaConsole::GetHandle()
				&& hParent1 != CTriggerAnnotation::GetHandle()
				&& !CViewObjectsExt::IsOpeningAnnotationDlg
				)
			{
				int zDelta = GET_WHEEL_DELTA_WPARAM(pMsg->wParam);
				if (zDelta < 0) {
					CIsoViewExt::Zoom(0.1);
				}
				else {
					CIsoViewExt::Zoom(-0.1);
				}
			}
		}
		break;
	}
	case WM_MBUTTONUP:
	{
		HWND hWnd = GetFocus();					// EDIT		COMBOBOX_DROPDOWN
		HWND hParent1 = ::GetParent(hWnd);		// WINDOW	COMBOBOX
		if (hParent1 != CNewINIEditor::GetHandle()
			&& hParent1 != CCsfEditor::GetHandle()
			&& hParent1 != CNewINIEditor::GetImporter()
			&& hParent1 != CLuaConsole::GetHandle()
			&& hParent1 != CTriggerAnnotation::GetHandle()
			&& !CViewObjectsExt::IsOpeningAnnotationDlg
			)
		{
			CIsoViewExt::Zoom(0.0);
		}
		break;
	}
	}
	return ppmfc::CDialog::PreTranslateMessage(pMsg);
}