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
#include "../../ExtraWindow//CNewTaskforce/CNewTaskforce.h"
#include "../../ExtraWindow//CNewScript/CNewScript.h"
#include "../../ExtraWindow/CNewTrigger/CNewTrigger.h"
#include "../../ExtraWindow/CTerrainGenerator/CTerrainGenerator.h"
#include "../../Miscs/DialogStyle.h"

HWND CTileSetBrowserFrameExt::hTabCtrl = NULL;


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
			CTileManager::Create(this);
		else
			::ShowWindow(CTileManager::GetHandle(), SW_SHOW);
	}
}
void CTileSetBrowserFrameExt::OnBNSearchClicked()
{
	if (CObjectSearch::GetHandle() == NULL)
		CObjectSearch::Create(this);
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
		CTerrainGenerator::Create(this);
	else
	{
		::ShowWindow(CTerrainGenerator::GetHandle(), SW_SHOW);
		::SendMessage(CTerrainGenerator::GetHandle(), 114514, 0, 0);
	}
}

BOOL CTileSetBrowserFrameExt::PreTranslateMessageExt(MSG* pMsg)
{
	if (pMsg->message == WM_COMMAND)
	{
		auto nID = LOWORD(pMsg->wParam);
		auto nHi = HIWORD(pMsg->wParam);

		if (nID == (UINT)TriggerSort::MenuItem::AddTrigger)
		{
			if (IsWindowVisible(CNewTrigger::GetHandle()))
			{
				TriggerSort::CreateFromTriggerSort = true;
				TriggerSort::Instance.Menu_AddTrigger();
				CNewTrigger::OnClickNewTrigger();
				TriggerSort::CreateFromTriggerSort = false;
				return TRUE;
			}
		}
		if (nID == (UINT)TagSort::MenuItem::AddTrigger)
		{
			if (CFinalSunDlg::Instance->Tags && IsWindowVisible(CFinalSunDlg::Instance->Tags))
			{
				TagSort::Instance.Menu_AddTrigger();
				ppmfc::CString name;
				ppmfc::CString value;
				ppmfc::CString key = CMapDataExt::GetAvailableIndex();
				name = (TagSort::Instance.GetCurrentPrefix() + "New Tag").c_str();
				value.Format("0,%s,%s", name, CINI::CurrentDocument->GetKeyCount("Triggers") > 0 ?
					CINI::CurrentDocument->GetKeyAt("Triggers", 0).m_pchData : "01000000");
				CINI::CurrentDocument->WriteString("Tags", key, value);
				CFinalSunDlg::Instance->Tags.UpdateDialog();
				auto hTag = ::GetDlgItem(CFinalSunDlg::Instance->Tags, 1083);
				CComboBox& cTag = *(CComboBox*)CWnd::FromHandle(hTag);
				auto idx = cTag.FindStringExact(0, key + " (" + name + ")");
				if (idx != CB_ERR)
				{
					cTag.SetCurSel(idx);
					CFinalSunDlg::Instance->Tags.OnCBCurrentTagSelectedChanged();
				}
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
	if (pMsg->message == WM_KEYUP)
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
	if (pMsg->message == WM_LBUTTONUP)
	{
		if (pMsg->hwnd == this->DialogBar.GetDlgItem(6102)->GetSafeHwnd())
			this->OnBNTileManagerClicked();
	}	
	if (pMsg->message == WM_LBUTTONUP)
	{
		if (pMsg->hwnd == this->DialogBar.GetDlgItem(6250)->GetSafeHwnd())
			this->OnBNSearchClicked();
	}
	if (pMsg->message == WM_LBUTTONUP)
	{
		if (pMsg->hwnd == this->DialogBar.GetDlgItem(6251)->GetSafeHwnd())
			this->OnBNTerrainGeneratorClicked();
	}
	if (pMsg->hwnd == TriggerSort::Instance)
	{
		if (TriggerSort::Instance.OnMessage(pMsg))
			return TRUE;
	}
	if (pMsg->hwnd == TeamSort::Instance)
	{
		if (TeamSort::Instance.OnMessage(pMsg))
			return TRUE;
	}
	if (pMsg->hwnd == TaskforceSort::Instance)
	{
		if (TaskforceSort::Instance.OnMessage(pMsg))
			return TRUE;
	}
	if (pMsg->hwnd == ScriptSort::Instance)
	{
		if (ScriptSort::Instance.OnMessage(pMsg))
			return TRUE;
	}
	if (pMsg->hwnd == WaypointSort::Instance)
	{
		if (WaypointSort::Instance.OnMessage(pMsg))
			return TRUE;
	}
	if (pMsg->hwnd == TagSort::Instance)
	{
		if (TagSort::Instance.OnMessage(pMsg))
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
				break;
			case TabPage::TriggerSort:
				this->DialogBar.ShowWindow(SW_HIDE);
				this->View.ShowWindow(SW_HIDE);

				TeamSort::Instance.HideWindow();
				TaskforceSort::Instance.HideWindow();
				ScriptSort::Instance.HideWindow();
				TriggerSort::Instance.ShowWindow();
				WaypointSort::Instance.HideWindow();
				TagSort::Instance.HideWindow();

				if (IsWindowVisible(CNewTrigger::GetHandle()))
				{
					if (!TreeView_GetCount(TriggerSort::Instance.GetHwnd()))
						TriggerSort::Instance.LoadAllTriggers();
				}
				break;
			case TabPage::TeamSort:
				this->DialogBar.ShowWindow(SW_HIDE);
				this->View.ShowWindow(SW_HIDE);

				TeamSort::Instance.ShowWindow();
				TriggerSort::Instance.HideWindow();
				TaskforceSort::Instance.HideWindow();
				ScriptSort::Instance.HideWindow();
				WaypointSort::Instance.HideWindow();
				TagSort::Instance.HideWindow();
				if (IsWindowVisible(CNewTeamTypes::GetHandle()))
				{
					if (!TreeView_GetCount(TeamSort::Instance.GetHwnd()))
						TeamSort::Instance.LoadAllTriggers();
				}
				break;
			case TabPage::TaskforceSort:
				this->DialogBar.ShowWindow(SW_HIDE);
				this->View.ShowWindow(SW_HIDE);

				TeamSort::Instance.HideWindow();
				TriggerSort::Instance.HideWindow();
				TaskforceSort::Instance.ShowWindow();
				ScriptSort::Instance.HideWindow();
				WaypointSort::Instance.HideWindow();
				TagSort::Instance.HideWindow();
				if (IsWindowVisible(CNewTaskforce::GetHandle()) || IsWindowVisible(CNewTeamTypes::GetHandle()))
				{
					if (!TreeView_GetCount(TaskforceSort::Instance.GetHwnd()))
						TaskforceSort::Instance.LoadAllTriggers();
				}
				break;
			case TabPage::ScriptSort:
				this->DialogBar.ShowWindow(SW_HIDE);
				this->View.ShowWindow(SW_HIDE);

				TeamSort::Instance.HideWindow();
				TriggerSort::Instance.HideWindow();
				TaskforceSort::Instance.HideWindow();
				ScriptSort::Instance.ShowWindow();
				WaypointSort::Instance.HideWindow();
				TagSort::Instance.HideWindow();
				if (IsWindowVisible(CNewScript::GetHandle()) || IsWindowVisible(CNewTeamTypes::GetHandle()))
				{
					if (!TreeView_GetCount(ScriptSort::Instance.GetHwnd()))
						ScriptSort::Instance.LoadAllTriggers();
				}
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
				if (IsWindowVisible(CNewTeamTypes::GetHandle()) || CFinalSunDlg::Instance->TriggerFrame.m_hWnd || IsWindowVisible(CNewScript::GetHandle()))
				{
					if (!TreeView_GetCount(WaypointSort::Instance.GetHwnd()))
						WaypointSort::Instance.LoadAllTriggers();
				}
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
				if ( IsWindowVisible(CNewTrigger::GetHandle()) || CFinalSunDlg::Instance->Tags.m_hWnd)
				{
					if (!TreeView_GetCount(TagSort::Instance.GetHwnd()))
						TagSort::Instance.LoadAllTriggers();
				}
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
		pitem.pszText = FA2sp::Buffer.m_pchData;
		TabCtrl_InsertItem(this->hTabCtrl, i++, &pitem);
	};

	insertItem("Tiles && Overlays", "TabPages.TilePlacement");
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

}