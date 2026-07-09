#include "Body.h"

#include "../../Helpers/Translations.h"

#include <CINI.h>
#include <CLoading.h>
#include "../../Miscs/Palettes.h"
#include "../CFinalSunDlg/Body.h"
#include "../CLoading/Body.h"

TransparencyHelper CLightingExt::m_transparency;

void CLightingExt::ProgramStartupInit()
{
	RunTime::ResetMemoryContentAt(0x594788, &CLightingExt::PreTranslateMessageExt);
	RunTime::ResetMemoryContentAt(0x594770, &CLightingExt::OnCommandExt);
	RunTime::ResetMemoryContentAt(0x5947B4, &CLightingExt::OnInitDialogExt);
}

void CLightingExt::Translate()
{
	Translations::TranslateItem(this, "LightingTitle");
	Translations::TranslateItem(this, 1285, "LightingDesc");
	Translations::TranslateItem(this, 1298, "LightingNormal");
	Translations::TranslateItem(this, 1299, "LightingIonStorm");
	Translations::TranslateItem(this, 1320, "LightingDominator");

	// Normal
	Translations::TranslateItem(this, 1300, "LightingAmbient");
	Translations::TranslateItem(this, 1305, "LightingGreen");
	Translations::TranslateItem(this, 1304, "LightingRed");
	Translations::TranslateItem(this, 1306, "LightingBlue");
	Translations::TranslateItem(this, 1301, "LightingLevel");
	Translations::TranslateItem(this, 1315, "LightingGround");

	// IonStorm
	Translations::TranslateItem(this, 1302, "LightingAmbient");
	Translations::TranslateItem(this, 1308, "LightingGreen");
	Translations::TranslateItem(this, 1307, "LightingRed");
	Translations::TranslateItem(this, 1309, "LightingBlue");
	Translations::TranslateItem(this, 1303, "LightingLevel");
	Translations::TranslateItem(this, 1316, "LightingGround");

	// Dominator
	Translations::TranslateItem(this, 1310, "LightingAmbient");
	Translations::TranslateItem(this, 1311, "LightingGreen");
	Translations::TranslateItem(this, 1312, "LightingRed");
	Translations::TranslateItem(this, 1313, "LightingBlue");
	Translations::TranslateItem(this, 1314, "LightingLevel");
	Translations::TranslateItem(this, 1317, "LightingGround");

	// Others
	Translations::TranslateItem(this, 1318, "LightingNukeAmbientChangeRate");
	Translations::TranslateItem(this, 1319, "LightingDominatorAmbientChangeRate");
}

BOOL CLightingExt::PreTranslateMessageExt(MSG* pMsg)
{
	if (m_transparency.HandleMessage(GetSafeHwnd(), pMsg->message, pMsg->wParam, pMsg->lParam, "LightingOpacity"))
		return TRUE;
	if (pMsg->message == WM_KEYUP)
	{
		bool edited = false;
		auto process = [&pMsg, this, &edited](int nID, const char* pKey, bool edit = true) -> bool
		{
			auto pWnd = (ppmfc::CWnd*)this->GetDlgItem(nID);
			if (pMsg->hwnd == pWnd->m_hWnd)
			{
				ppmfc::CString buffer;
				pWnd->GetWindowText(buffer);

				double temp;
				if (sscanf_s(buffer, "%lf", &temp) == 1)
					buffer.Format("%.6f", temp);
				else
				{
					buffer = "0.000000";
					pWnd->SetWindowText(buffer);
				}
				if (CINI::CurrentDocument->GetString("Lighting", pKey) != buffer)
				{
					CINI::CurrentDocument->WriteString("Lighting", pKey, buffer);
					if (edit)
						edited = true;
				}
				return true;
			}
			return false;
		};

		if (!process(1068, "DominatorAmbient"))
		if (!process(1069, "DominatorGreen"))
		if (!process(1070, "DominatorRed"))
		if (!process(1071, "DominatorBlue"))
		if (!process(1072, "DominatorLevel"))
		if (!process(1073, "Ground"))
		if (!process(1074, "IonGround"))
		if (!process(1075, "DominatorGround"))
		if (!process(1076, "NukeAmbientChangeRate", false))
		if (!process(1077, "DominatorAmbientChangeRate", false))
		;

		if (edited && CFinalSunDlgExt::CurrentLighting != 31000)
		{
			LightingStruct::GetCurrentLighting();
			for (int i = 0; i < CMapData::Instance->MapWidthPlusHeight; i++) {
				for (int j = 0; j < CMapData::Instance->MapWidthPlusHeight; j++) {
					CMapData::Instance->UpdateMapPreviewAt(i, j);
				}
			}
			LightingSourceTint::CalculateMapLamps();

			CFinalSunDlg::Instance()->MyViewFrame.Minimap.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
			CFinalSunDlg::Instance()->MyViewFrame.pIsoView->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		}
	}
	return this->ppmfc::CDialog::PreTranslateMessage(pMsg);
}

BOOL CLightingExt::OnCommandExt(WPARAM wParam, LPARAM lParam)
{
	WORD nID = LOWORD(wParam);
	int alpha = -1;
	if (nID == TransparencyHelper::IDM_OPAQUE)           alpha = 255;
	else if (nID == TransparencyHelper::IDM_NEAR_FULL)   alpha = 191;
	else if (nID == TransparencyHelper::IDM_HALF)        alpha = 128;
	else if (nID == TransparencyHelper::IDM_TRANSPARENT) alpha = 64;
	else if (nID == TransparencyHelper::IDM_FULL_TRANSPARENT) alpha = 1;

	if (alpha != -1) {
		m_transparency.ApplyTransparency(GetSafeHwnd(), alpha, "LightingOpacity");
		return TRUE;
	}

	return this->ppmfc::CDialog::OnCommand(wParam, lParam);
}

BOOL CLightingExt::OnInitDialogExt(void)
{
	m_transparency.Init(GetSafeHwnd(), "LightingOpacity");
	Translate();
	return this->ppmfc::CDialog::OnInitDialog();
}