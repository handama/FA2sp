#include "Body.h"
#include "../../ExtraWindow/CNewEasterEgg/CNewEasterEgg.h"

CCredits* CCreditsExt::Instance = nullptr;

void CCreditsExt::ProgramStartupInit()
{
	RunTime::ResetMemoryContentAt(0x592810, &CCreditsExt::PreTranslateMessageExt);
}

static int loop = -1;
BOOL CCreditsExt::PreTranslateMessageExt(MSG* pMsg)
{
	if (this->DrawHappyFace && pMsg->message == WM_LBUTTONUP)
	{
		if (loop == -1)
		{
			loop = STDHelpers::RandomSelectInt(0, 2);
		}
		if (loop % 2 == 0)
		{
			if (CChineseChess::GetHandle() == NULL)
				CChineseChess::Create(CFinalSunDlg::Instance);
			loop++;
		}
		else
		{
			if (CGoBang::GetHandle() == NULL)
				CGoBang::Create(CFinalSunDlg::Instance);
			loop++;
		}
		return TRUE;
	}
	else if (this->DrawHappyFace && pMsg->message == WM_LBUTTONDOWN)
	{
		return TRUE;
	}
	return this->ppmfc::CDialog::PreTranslateMessage(pMsg);
}