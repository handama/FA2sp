#include "Body.h"

CAITriggerTypesEnable* CAITriggerTypesEnableExt::Instance = nullptr;
TransparencyHelper CAITriggerTypesEnableExt::m_transparency;

void CAITriggerTypesEnableExt::ProgramStartupInit()
{
	RunTime::ResetMemoryContentAt(0x591C70, &CAITriggerTypesEnableExt::PreTranslateMessageExt);
	RunTime::ResetMemoryContentAt(0x591C58, &CAITriggerTypesEnableExt::OnCommandExt);
	RunTime::ResetMemoryContentAt(0x591C9C, &CAITriggerTypesEnableExt::OnInitDialogExt);
}

BOOL CAITriggerTypesEnableExt::PreTranslateMessageExt(MSG* pMsg)
{
	if (m_transparency.HandleMessage(GetSafeHwnd(), pMsg->message, pMsg->wParam, pMsg->lParam, "AITriggerTypesEnableOpacity"))
		return TRUE;
	switch (pMsg->message) {

	default:
		break;
	}
	return this->ppmfc::CDialog::PreTranslateMessage(pMsg);
}

BOOL CAITriggerTypesEnableExt::OnCommandExt(WPARAM wParam, LPARAM lParam)
{
	WORD nID = LOWORD(wParam);
	int alpha = -1;
	if (nID == TransparencyHelper::IDM_OPAQUE)           alpha = 255;
	else if (nID == TransparencyHelper::IDM_NEAR_FULL)   alpha = 191;
	else if (nID == TransparencyHelper::IDM_HALF)        alpha = 128;
	else if (nID == TransparencyHelper::IDM_TRANSPARENT) alpha = 64;
	else if (nID == TransparencyHelper::IDM_FULL_TRANSPARENT) alpha = 1;

	if (alpha != -1) {
		m_transparency.ApplyTransparency(GetSafeHwnd(), alpha, "AITriggerTypesEnableOpacity");
		return TRUE;
	}

	return this->ppmfc::CDialog::OnCommand(wParam, lParam);
}

BOOL CAITriggerTypesEnableExt::OnInitDialogExt()
{
	m_transparency.Init(GetSafeHwnd(), "AITriggerTypesEnableOpacity");
	return this->ppmfc::CDialog::OnInitDialog();
}
