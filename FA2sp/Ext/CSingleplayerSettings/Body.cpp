#include "Body.h"
#include "../../Miscs/Hooks.INI.h"

#include <CINI.h>
#include <string>

void CSingleplayerSettingsExt::ProgramStartupInit()
{
	//RunTime::ResetMemoryContentAt(0x596520, &CSingleplayerSettingsExt::OnCommandExt);
	RunTime::ResetMemoryContentAt(0x596538, &CSingleplayerSettingsExt::PreTranslateMessageExt);
}

//BOOL CSingleplayerSettingsExt::OnCommandExt(WPARAM wParam, LPARAM lParam)
//{
//	
//
//	return this->ppmfc::CDialog::OnCommand(wParam, lParam);
//}


BOOL CSingleplayerSettingsExt::PreTranslateMessageExt(MSG* pMsg)
{
	auto pWnd = (ppmfc::CWnd*)this->GetDlgItem(1400);
	if (pMsg->hwnd == pWnd->m_hWnd && pMsg->message == WM_LBUTTONDOWN)
	{
		auto process = [&pMsg, this](int nID, const char* pKey)// -> bool
		{
			auto pWnd = (ppmfc::CWnd*)this->GetDlgItem(nID);
			ppmfc::CString buffer;
			pWnd->GetWindowText(buffer);
			CINIExt::CurrentDocument->WriteString("Ranking", pKey, buffer);
		};
		auto processGeneral = [&pMsg, this](int nID, const char* pKey)// -> bool
		{
			auto pWnd = (ppmfc::CWnd*)this->GetDlgItem(nID);
			ppmfc::CString buffer;
			pWnd->GetWindowText(buffer);
			if (buffer == "")
				CINIExt::CurrentDocument->DeleteKey("General", pKey);
			else
				CINIExt::CurrentDocument->WriteString("General", pKey, buffer);
		};

		if (pMsg->hwnd == pWnd->m_hWnd)
		{
			process(1356, "ParTimeEasy");
			process(1357, "ParTimeMedium");
			process(1358, "ParTimeHard");
			process(1359, "OverParTitle");
			process(1360, "OverParMessage");
			process(1361, "UnderParTitle");
			process(1362, "UnderParMessage");
			processGeneral(1366, "CampaignMoneyDeltaEasy");
			processGeneral(1368, "CampaignMoneyDeltaHard");
			processGeneral(1370, "SpyMoneyStealPercent");
			processGeneral(1372, "TeamDelays");
			processGeneral(1374, "PrismSupportModifier");
			processGeneral(1376, "DefaultMirageDisguises");
		}
	}
	return this->ppmfc::CDialog::PreTranslateMessage(pMsg);
}