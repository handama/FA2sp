#include "Body.h"
#include "../../ExtraWindow/CNewEasterEgg/CNewEasterEgg.h"

CCredits* CCreditsExt::Instance = nullptr;

void CCreditsExt::ProgramStartupInit()
{
	RunTime::ResetMemoryContentAt(0x592810, &CCreditsExt::PreTranslateMessageExt);
}

BOOL CCreditsExt::PreTranslateMessageExt(MSG* pMsg)
{
	if (this->DrawHappyFace && pMsg->message == WM_LBUTTONDOWN)
	{
		if (CNewEasterEgg::GetHandle() == NULL)
			CNewEasterEgg::Create((CFinalSunDlg*)this);
		return TRUE;
	}
	return this->ppmfc::CDialog::PreTranslateMessage(pMsg);
}