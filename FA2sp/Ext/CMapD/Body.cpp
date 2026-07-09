#include "Body.h"

CMapD* CMapDExt::Instance = nullptr;
TransparencyHelper CMapDExt::m_transparency;

void CMapDExt::ProgramStartupInit()
{
	RunTime::ResetMemoryContentAt(0x594A40, &CMapDExt::PreTranslateMessageExt);
	RunTime::ResetMemoryContentAt(0x594A28, &CMapDExt::OnCommandExt);
	RunTime::ResetMemoryContentAt(0x594A6C, &CMapDExt::OnInitDialogExt);
}

BOOL CMapDExt::PreTranslateMessageExt(MSG* pMsg)
{
	if (m_transparency.HandleMessage(GetSafeHwnd(), pMsg->message, pMsg->wParam, pMsg->lParam, "MapSettingsOpacity"))
		return TRUE;
	switch (pMsg->message) {

	default:
		break;
	}
	return this->ppmfc::CDialog::PreTranslateMessage(pMsg);
}

BOOL CMapDExt::OnCommandExt(WPARAM wParam, LPARAM lParam)
{
	WORD nID = LOWORD(wParam);
	int alpha = -1;
	if (nID == TransparencyHelper::IDM_OPAQUE)           alpha = 255;
	else if (nID == TransparencyHelper::IDM_NEAR_FULL)   alpha = 191;
	else if (nID == TransparencyHelper::IDM_HALF)        alpha = 128;
	else if (nID == TransparencyHelper::IDM_TRANSPARENT) alpha = 64;
	else if (nID == TransparencyHelper::IDM_FULL_TRANSPARENT) alpha = 1;

	if (alpha != -1) {
		m_transparency.ApplyTransparency(GetSafeHwnd(), alpha, "MapSettingsOpacity");
		return TRUE;
	}

	return this->ppmfc::CDialog::OnCommand(wParam, lParam);
}

BOOL CMapDExt::OnInitDialogExt()
{
	m_transparency.Init(GetSafeHwnd(), "MapSettingsOpacity");
	return this->ppmfc::CDialog::OnInitDialog();
}
