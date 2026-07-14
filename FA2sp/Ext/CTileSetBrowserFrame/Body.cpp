#include "Body.h"

#include <FA2PP.h>

#include <CMapData.h>
#include <CFinalSunDlg.h>
#include "../../ExtraWindow/CTileManager/CTileManager.h"

#include "../../FA2sp.h"
//#include "../CTriggerFrame/Body.h"

#include "TabPages/TriggerSort.h"
#include "TabPages/TeamSort.h"

#include "../../Helpers/Translations.h"
#include "TabPages/TaskForceSort.h"
#include "TabPages/ScriptSort.h"
#include "TabPages/WaypointSort.h"
#include "TabPages/TagSort.h"
#include "../../ExtraWindow/CObjectSearch/CObjectSearch.h"
#include "../../ExtraWindow//CNewTeamTypes/CNewTeamTypes.h"
#include "../../ExtraWindow//CNewTag/CNewTag.h"
#include "../../ExtraWindow//CNewTaskforce/CNewTaskforce.h"
#include "../../ExtraWindow//CNewScript/CNewScript.h"
#include "../../ExtraWindow/CNewTrigger/CNewTrigger.h"
#include "../../ExtraWindow/CTerrainGenerator/CTerrainGenerator.h"
#include "../../Miscs/DialogStyle.h"
#include "TabPages/GridObjectViewer.h"

HWND CTileSetBrowserFrameExt::hTabCtrl = NULL;
bool CTileSetBrowserFrameExt::TerrainDlgLoaded = true;
CTileSetBrowserView* CTileSetBrowserFrameExt::TileSetBrowserView_Instance = nullptr;
float CTileSetBrowserFrameExt::TileSetBrowserViewScaledFactor = 1.0f;
float CTileSetBrowserFrameExt::OverlayBrowserViewScaledFactor = 1.0f;
float CTileSetBrowserFrameExt::GridObjectViewerScaledFactor = 1.0f;

void CTileSetBrowserFrameExt::ProgramStartupInit()
{
	RunTime::ResetMemoryContentAt(0x597458, &CTileSetBrowserFrameExt::PreTranslateMessageExt);
	RunTime::ResetMemoryContentAt(0x597444, &CTileSetBrowserFrameExt::OnNotifyExt);
	RunTime::ResetMemoryContentAt(0x597440, &CTileSetBrowserFrameExt::OnCommandExt);
}

void CTileSetBrowserFrameExt::OnBNTileManagerClicked()
{
	if (CMapData::Instance->MapWidthPlusHeight)
	{
		if (CTileManager::GetHandle() == NULL)
			CTileManager::Create(CFinalSunDlg::Instance);
		else
			::ShowWindow(CTileManager::GetHandle(), SW_SHOW);
	}
}
void CTileSetBrowserFrameExt::OnBNSearchClicked()
{
	if (CObjectSearch::GetHandle() == NULL)
		CObjectSearch::Create(CFinalSunDlg::Instance);
	else
	{
		::ShowWindow(CObjectSearch::GetHandle(), SW_SHOW);
		::SendMessage(CObjectSearch::GetHandle(), 114514, 0, 0);
	}


}
void CTileSetBrowserFrameExt::OnBNTerrainGeneratorClicked()
{
	if (!CMapData::Instance->MapWidthPlusHeight) return;
	if (CTerrainGenerator::GetHandle() == NULL)
		CTerrainGenerator::Create(CFinalSunDlg::Instance);
	else
	{
		::ShowWindow(CTerrainGenerator::GetHandle(), SW_SHOW);
		::SendMessage(CTerrainGenerator::GetHandle(), 114514, 0, 0);
	}
}

void CTileSetBrowserFrameExt::RefreshWindows()
{
	PostMessage(CFinalSunDlg::Instance->MyViewFrame.pTileSetBrowserFrame->GetSafeHwnd(), 114514, 0, 0);
}

BOOL CTileSetBrowserFrameExt::PreTranslateMessageExt(MSG* pMsg)
{
	if (pMsg->message == WM_COMMAND)
	{
		auto nID = LOWORD(pMsg->wParam);
		auto nHi = HIWORD(pMsg->wParam);

		if (nID == (UINT)TriggerSort::MenuItem::AddTrigger)
		{
			if (IsWindowVisible(CNewTrigger::GetFirstValidInstance().GetHandle()))
			{
				TriggerSort::CreateFromTriggerSort = true;
				TriggerSort::Instance.Menu_AddTrigger();
				CNewTrigger::GetFirstValidInstance().OnClickNewTrigger();
				TriggerSort::CreateFromTriggerSort = false;
				return TRUE;
			}
		}
		if (nID == (UINT)TagSort::MenuItem::AddTrigger)
		{
			if (CFinalSunDlg::Instance->Tags && IsWindowVisible(CFinalSunDlg::Instance->Tags))
			{
				TagSort::CreateFromTagSort = true;
				TagSort::Instance.Menu_AddTrigger();
				CNewTag::OnClickNewTag();
				TagSort::CreateFromTagSort = false;
				return TRUE;
			}
		}
		if (nID == (UINT)TeamSort::MenuItem::AddTrigger)
		{
			if (IsWindowVisible(CNewTeamTypes::GetHandle()))
			{
				TeamSort::CreateFromTeamSort = true;
				TeamSort::Instance.Menu_AddTrigger();
				CNewTeamTypes::OnClickNewTeam();
				TeamSort::CreateFromTeamSort = false;
				return TRUE;
			}
		}
		if (nID == (UINT)ScriptSort::MenuItem::AddTrigger)
		{
			if (IsWindowVisible(CNewScript::GetHandle()))
			{
				ScriptSort::CreateFromScriptSort = true;
				ScriptSort::Instance.Menu_AddTrigger();
				CNewScript::OnClickNewScript();
				ScriptSort::CreateFromScriptSort = false;
				return TRUE;
			}
		}
		if (nID == (UINT)TaskforceSort::MenuItem::AddTrigger)
		{
			if (IsWindowVisible(CNewTaskforce::GetHandle()))
			{
				TaskforceSort::CreateFromTaskForceSort = true;
				TaskforceSort::Instance.Menu_AddTrigger();
				CNewTaskforce::OnClickNewTaskforce();
				TaskforceSort::CreateFromTaskForceSort = false;
				return TRUE;
			}
		}
		if (nID == (UINT)TriggerSort::MenuItem::Refresh)
		{
			if (TriggerSort::Instance.IsVisible())
			{
				TriggerSort::Instance.LoadAllTriggers();

			}
			if (TagSort::Instance.IsVisible())
			{
				TagSort::Instance.LoadAllTriggers();

			}
			return TRUE;
		}		
		if (nID == (UINT)TeamSort::MenuItem::Refresh)// || nID == (UINT)TaskforceSort::MenuItem::Refresh || nID == (UINT)ScriptSort::MenuItem::Refresh || nID == (UINT)WaypointSort::MenuItem::Refresh)
		{
			if (TeamSort::Instance.IsVisible())
				TeamSort::Instance.LoadAllTriggers();
			if (TaskforceSort::Instance.IsVisible())
				TaskforceSort::Instance.LoadAllTriggers();
			if (ScriptSort::Instance.IsVisible())
				ScriptSort::Instance.LoadAllTriggers();
			if (WaypointSort::Instance.IsVisible())
				WaypointSort::Instance.LoadAllTriggers();
			if (TriggerSort::Instance.IsVisible())
				TriggerSort::Instance.LoadAllTriggers();
			if (TagSort::Instance.IsVisible())
				TagSort::Instance.LoadAllTriggers();

			return TRUE;
		}

	}
	else if (pMsg->message == WM_KEYUP)
	{
		if (pMsg->wParam == VK_F5)
		{
			if (TeamSort::Instance.IsVisible())
				TeamSort::Instance.LoadAllTriggers();
			if (TaskforceSort::Instance.IsVisible())
				TaskforceSort::Instance.LoadAllTriggers();
			if (ScriptSort::Instance.IsVisible())
				ScriptSort::Instance.LoadAllTriggers();
			if (WaypointSort::Instance.IsVisible())
				WaypointSort::Instance.LoadAllTriggers();
			if (TriggerSort::Instance.IsVisible())
				TriggerSort::Instance.LoadAllTriggers();
			if (TagSort::Instance.IsVisible())
				TagSort::Instance.LoadAllTriggers();

			return TRUE;
		}

	}	
	else if (pMsg->message == WM_LBUTTONUP)
	{
		if (pMsg->hwnd == this->DialogBar.GetDlgItem(6102)->GetSafeHwnd())
			this->OnBNTileManagerClicked();
		else if (pMsg->hwnd == this->DialogBar.GetDlgItem(6250)->GetSafeHwnd())
			this->OnBNSearchClicked();
		else if (pMsg->hwnd == this->DialogBar.GetDlgItem(6251)->GetSafeHwnd())
			this->OnBNTerrainGeneratorClicked();
	}
	else if (GetKeyState(VK_CONTROL) & 0x8000 && pMsg->message == WM_MOUSEWHEEL || pMsg->message ==  WM_MBUTTONUP)
	{		 
		float* target = nullptr;
		if (::IsWindowVisible(View) && View.CurrentMode == 1)
			target = &TileSetBrowserViewScaledFactor; 
		else if (::IsWindowVisible(View) && View.CurrentMode == 2)
			target = &OverlayBrowserViewScaledFactor; 
		else if (::IsWindowVisible(GridObjectViewer::Instance))
			target = &GridObjectViewerScaledFactor; 

		if (target)
		{
			CINI fa2;
			FString path = CFinalSunAppExt::ExePathExt;
			path += "\\FinalAlert.ini";
			fa2.ClearAndLoad(path);				
			if (pMsg->message ==  WM_MBUTTONUP)
			{
				*target = 1.0f;
			}
			else
			{
				int zDelta = GET_WHEEL_DELTA_WPARAM(pMsg->wParam);
				if (zDelta < 0) {
					*target -= 0.1f;
					*target = std::clamp(*target, 0.25f, 4.0f);
				}
				else {
					*target += 0.1f;
					*target = std::clamp(*target, 0.25f, 4.0f);
				}
			}
			std::ostringstream oss;
            oss.precision(3);
            oss << std::fixed << *target;
			if (target == &TileSetBrowserViewScaledFactor)
			{
				fa2.WriteString("UserInterface", "TileSetBrowserViewScaledFactor", oss.str().c_str());
				auto tmp = CIsoView::CurrentCommand->Command;
				HWND hParent = DialogBar.GetSafeHwnd();
				HWND hTileComboBox = ::GetDlgItem(hParent, 1366);
				::SendMessage(hParent, WM_COMMAND, MAKEWPARAM(1366, CBN_SELCHANGE), (LPARAM)hTileComboBox);
				CIsoView::CurrentCommand->Command = tmp;
			}
			else if (target == &OverlayBrowserViewScaledFactor)
			{
				fa2.WriteString("UserInterface", "OverlayBrowserViewScaledFactor", oss.str().c_str());
				auto tmp = CIsoView::CurrentCommand->Command;
				HWND hParent = DialogBar.GetSafeHwnd();
				HWND hOverlayComboBox = ::GetDlgItem(hParent, 1367);
				::SendMessage(hParent, WM_COMMAND, MAKEWPARAM(1367, CBN_SELCHANGE), (LPARAM)hOverlayComboBox);
				CIsoView::CurrentCommand->Command = tmp;
			}
			else if (target == &GridObjectViewerScaledFactor)
			{
				fa2.WriteString("UserInterface", "GridObjectViewerScaledFactor", oss.str().c_str());
				GridObjectViewer::Instance.UpdateImages();
			}
			fa2.WriteToFile(path);
		}
	}
	else if (pMsg->message == 114514)
	{
		if (GridObjectViewer::Instance.IsVisible())
		{	
			InvalidateRect(GridObjectViewer::Instance.GetControl(), NULL, TRUE);
			InvalidateRect(GridObjectViewer::Instance.GetView(), NULL, TRUE);
		}
		else if (::IsWindowVisible(this->DialogBar))
		{	
			InvalidateRect(this->DialogBar, NULL, TRUE);
			InvalidateRect(this->View, NULL, TRUE);
		}
	}
	else if (pMsg->hwnd == TriggerSort::Instance)
	{
		if (TriggerSort::Instance.OnMessage(pMsg))
			return TRUE;
	}
	else if (pMsg->hwnd == TeamSort::Instance)
	{
		if (TeamSort::Instance.OnMessage(pMsg))
			return TRUE;
	}
	else if (pMsg->hwnd == TaskforceSort::Instance)
	{
		if (TaskforceSort::Instance.OnMessage(pMsg))
			return TRUE;
	}
	else if (pMsg->hwnd == ScriptSort::Instance)
	{
		if (ScriptSort::Instance.OnMessage(pMsg))
			return TRUE;
	}
	else if (pMsg->hwnd == WaypointSort::Instance)
	{
		if (WaypointSort::Instance.OnMessage(pMsg))
			return TRUE;
	}
	else if (pMsg->hwnd == TagSort::Instance)
	{
		if (TagSort::Instance.OnMessage(pMsg))
			return TRUE;
	}
	else if (pMsg->hwnd == GridObjectViewer::Instance)
	{
		if (GridObjectViewer::Instance.OnMessage(pMsg))
			return TRUE;
	}	

	return this->ppmfc::CFrameWnd::PreTranslateMessage(pMsg);
}

BOOL CTileSetBrowserFrameExt::OnNotifyExt(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	LPNMHDR lpNmhdr = reinterpret_cast<LPNMHDR>(lParam);
	if (lpNmhdr->hwndFrom == this->hTabCtrl)
	{
		switch (lpNmhdr->code)
		{
		case TCN_SELCHANGE:
			switch (static_cast<TabPage>(TabCtrl_GetCurSel(this->hTabCtrl)))
			{
			default:
			case TabPage::TilesetBrowser:
				this->DialogBar.ShowWindow(SW_SHOW);
				this->View.ShowWindow(SW_SHOW);

				TriggerSort::Instance.HideWindow();
				TeamSort::Instance.HideWindow();
				TaskforceSort::Instance.HideWindow();
				ScriptSort::Instance.HideWindow();
				WaypointSort::Instance.HideWindow();
				TagSort::Instance.HideWindow();
				GridObjectViewer::Instance.HideWindow();
				break;
			case TabPage::TriggerSort:
				this->DialogBar.ShowWindow(SW_HIDE);
				this->View.ShowWindow(SW_HIDE);

				TeamSort::Instance.HideWindow();
				TaskforceSort::Instance.HideWindow();
				ScriptSort::Instance.HideWindow();
				TriggerSort::Instance.ShowWindow();
				TriggerSort::Instance.LoadAllTriggers();
				WaypointSort::Instance.HideWindow();
				TagSort::Instance.HideWindow();
				GridObjectViewer::Instance.HideWindow();
				break;
			case TabPage::TeamSort:
				this->DialogBar.ShowWindow(SW_HIDE);
				this->View.ShowWindow(SW_HIDE);

				TeamSort::Instance.ShowWindow();
				TeamSort::Instance.LoadAllTriggers();
				TriggerSort::Instance.HideWindow();
				TaskforceSort::Instance.HideWindow();
				ScriptSort::Instance.HideWindow();
				WaypointSort::Instance.HideWindow();
				TagSort::Instance.HideWindow();
				GridObjectViewer::Instance.HideWindow();
				break;
			case TabPage::TaskforceSort:
				this->DialogBar.ShowWindow(SW_HIDE);
				this->View.ShowWindow(SW_HIDE);

				TeamSort::Instance.HideWindow();
				TriggerSort::Instance.HideWindow();
				TaskforceSort::Instance.ShowWindow();
				TaskforceSort::Instance.LoadAllTriggers();
				ScriptSort::Instance.HideWindow();
				WaypointSort::Instance.HideWindow();
				TagSort::Instance.HideWindow();
				GridObjectViewer::Instance.HideWindow();
				break;
			case TabPage::ScriptSort:
				this->DialogBar.ShowWindow(SW_HIDE);
				this->View.ShowWindow(SW_HIDE);

				TeamSort::Instance.HideWindow();
				TriggerSort::Instance.HideWindow();
				TaskforceSort::Instance.HideWindow();
				ScriptSort::Instance.ShowWindow();
				ScriptSort::Instance.LoadAllTriggers();
				WaypointSort::Instance.HideWindow();
				TagSort::Instance.HideWindow();
				GridObjectViewer::Instance.HideWindow();
				break;
			case TabPage::WaypointSort:
				this->DialogBar.ShowWindow(SW_HIDE);
				this->View.ShowWindow(SW_HIDE);

				TeamSort::Instance.HideWindow();
				TriggerSort::Instance.HideWindow();
				TaskforceSort::Instance.HideWindow();
				ScriptSort::Instance.HideWindow();
				TagSort::Instance.HideWindow();
				WaypointSort::Instance.ShowWindow();
				WaypointSort::Instance.LoadAllTriggers();
				GridObjectViewer::Instance.HideWindow();
				break;
			case TabPage::TagSort:
				this->DialogBar.ShowWindow(SW_HIDE);
				this->View.ShowWindow(SW_HIDE);

				TeamSort::Instance.HideWindow();
				TriggerSort::Instance.HideWindow();
				TaskforceSort::Instance.HideWindow();
				ScriptSort::Instance.HideWindow();
				WaypointSort::Instance.HideWindow();
				TagSort::Instance.ShowWindow();
				TagSort::Instance.LoadAllTriggers();
				GridObjectViewer::Instance.HideWindow();
				break;
			case TabPage::GridObjectViewer:
				this->DialogBar.ShowWindow(SW_HIDE);
				this->View.ShowWindow(SW_HIDE);

				TeamSort::Instance.HideWindow();
				TriggerSort::Instance.HideWindow();
				TaskforceSort::Instance.HideWindow();
				ScriptSort::Instance.HideWindow();
				WaypointSort::Instance.HideWindow();
				TagSort::Instance.HideWindow();
				GridObjectViewer::Instance.ShowWindow();
				break;
			}

			return TRUE;
		default:
			break;
		}
	}
	else if (lpNmhdr->hwndFrom == TriggerSort::Instance)
	{
		if (TriggerSort::Instance.OnNotify(reinterpret_cast<LPNMTREEVIEW>(lpNmhdr)))
			return TRUE;
	}
	else if (lpNmhdr->hwndFrom == TeamSort::Instance)
	{
		if (TeamSort::Instance.OnNotify(reinterpret_cast<LPNMTREEVIEW>(lpNmhdr)))
			return TRUE;
	}
	else if (lpNmhdr->hwndFrom == TaskforceSort::Instance)
	{
		if (TaskforceSort::Instance.OnNotify(reinterpret_cast<LPNMTREEVIEW>(lpNmhdr)))
			return TRUE;
	}
	else if (lpNmhdr->hwndFrom == ScriptSort::Instance)
	{
		if (ScriptSort::Instance.OnNotify(reinterpret_cast<LPNMTREEVIEW>(lpNmhdr)))
			return TRUE;
	}
	else if (lpNmhdr->hwndFrom == WaypointSort::Instance)
	{
		if (WaypointSort::Instance.OnNotify(reinterpret_cast<LPNMTREEVIEW>(lpNmhdr)))
			return TRUE;
	}
	else if (lpNmhdr->hwndFrom == TagSort::Instance)
	{
		if (TagSort::Instance.OnNotify(reinterpret_cast<LPNMTREEVIEW>(lpNmhdr)))
			return TRUE;
	}


	return this->ppmfc::CFrameWnd::OnNotify(wParam, lParam, pResult);
}

BOOL CTileSetBrowserFrameExt::OnCommandExt(WPARAM wParam, LPARAM lParam)
{
	return this->ppmfc::CFrameWnd::OnCommand(wParam, lParam);
}

void CTileSetBrowserFrameExt::InitTabControl()
{
	RECT rect;
	this->GetClientRect(&rect);

	this->hTabCtrl = CreateWindowEx(0, WC_TABCONTROL,
		nullptr, TCS_FIXEDWIDTH | WS_CHILD | WS_VISIBLE,
		rect.left + 2, rect.top + 2, rect.right - 4, rect.bottom - 4,
		*this, NULL, (HINSTANCE)FA2sp::hInstance, nullptr);

	if (ExtConfigs::EnableDarkMode)
		::SetWindowSubclass(this->hTabCtrl, DarkTheme::TabCtrlSubclassProc, 0, 0);

	::SendMessage(this->hTabCtrl, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), NULL);
	::ShowWindow(this->hTabCtrl, SW_SHOW);
	
	::SetWindowPos(this->hTabCtrl, *this, NULL, NULL, NULL, NULL, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);

	int i = 0;
	auto insertItem = [&](const char* lpszDefault, const char* lpszTranslate)
	{
		TCITEM pitem;
		pitem.mask = TCIF_TEXT;
		FA2sp::Buffer = lpszDefault;
		Translations::GetTranslationItem(lpszTranslate, FA2sp::Buffer);
		std::string text = FA2sp::Buffer.GetString();
		pitem.pszText = (char*)text.c_str();
		TabCtrl_InsertItem(this->hTabCtrl, i++, &pitem);
	};

	insertItem("Tiles && Overlays", "TabPages.TilePlacement");
	insertItem("Object Viewer", "TabPages.GridObjectViewer");
	insertItem("Trigger Sort", "TabPages.TriggerSort");
	insertItem("Tag Sort", "TabPages.TagSort");
	insertItem("Team Sort", "TabPages.TeamSort");
	insertItem("Taskforce Sort", "TabPages.TaskforceSort");
	insertItem("Script Sort", "TabPages.ScriptSort");
	insertItem("Wayponit Sort", "TabPages.WaypointSort");
	
	// Create the pages
	TriggerSort::Instance.Create(hTabCtrl);
	TriggerSort::Instance.HideWindow();
	
	TagSort::Instance.Create(hTabCtrl);
	TagSort::Instance.HideWindow();

	TeamSort::Instance.Create(hTabCtrl);
	TeamSort::Instance.HideWindow();

	TaskforceSort::Instance.Create(hTabCtrl);
	TaskforceSort::Instance.HideWindow();

	ScriptSort::Instance.Create(hTabCtrl);
	ScriptSort::Instance.HideWindow();
	
	WaypointSort::Instance.Create(hTabCtrl);
	WaypointSort::Instance.HideWindow();
	
	GridObjectViewer::Instance.Create(hTabCtrl);
	GridObjectViewer::Instance.HideWindow();
}