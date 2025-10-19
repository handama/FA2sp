#include "CMapRendererDlg.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../Common.h"
#include "../../Ext/CIsoView/Body.h"

CMapRendererDlg::CMapRendererDlg(CWnd* pParent /*=NULL*/)
	: ppmfc::CDialog(329, pParent)
{
	b_LocalSize = FALSE;
	b_GameLayers = FALSE;
	n_Lighting = RendererLighting::Normal;
	b_DisplayInvisibleOverlay = FALSE;
	b_MarkStartPositions = FALSE;
	b_MarkOres = FALSE;
	b_IgnoreObjects = TRUE;
}

void CMapRendererDlg::DoDataExchange(ppmfc::CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Radio(pDX, 1001, b_LocalSize);
	DDX_Radio(pDX, 1003, b_GameLayers);
	DDX_Radio(pDX, 1005, n_Lighting);
	DDX_Check(pDX, 1010, b_DisplayInvisibleOverlay);
	DDX_Check(pDX, 1011, b_MarkStartPositions);
	DDX_Check(pDX, 1012, b_MarkOres);
	DDX_Check(pDX, 1013, b_IgnoreObjects);

	FString buffer;

	if (Translations::GetTranslationItem("OK", buffer))
		GetDlgItem(1)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("Cancel", buffer))
		GetDlgItem(2)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MapRendererDlgTips", buffer))
		GetDlgItem(1000)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MapRendererDlgRenderSize", buffer))
		GetDlgItem(5000)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MapRendererDlgRenderlayers", buffer))
		GetDlgItem(5001)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MapRendererDlgLocalsize", buffer))
		GetDlgItem(1001)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MapRendererDlgFullsize", buffer))
		GetDlgItem(1002)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MapRendererDlgIngame", buffer))
		GetDlgItem(1003)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MapRendererDlgCurrentlayers", buffer))
		GetDlgItem(1004)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MapRendererDlgLighting", buffer))
		GetDlgItem(5002)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MapRendererDlgCurrentLighting", buffer))
		GetDlgItem(1005)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MapRendererDlgNoneLighting", buffer))
		GetDlgItem(1006)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MapRendererDlgNormalLighting", buffer))
		GetDlgItem(1007)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MapRendererDlgLightningLighting", buffer))
		GetDlgItem(1008)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MapRendererDlgDominatorLighting", buffer))
		GetDlgItem(1009)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MapRendererDlgInvisibleOverlay", buffer))
		GetDlgItem(1010)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MapRendererDlgStartingPositions", buffer))
		GetDlgItem(1011)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MapRendererDlgResourceFields", buffer))
		GetDlgItem(1012)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MapRendererDlgIgnoreObjects", buffer))
		GetDlgItem(1013)->SetWindowTextA(buffer);
	
	if (Translations::GetTranslationItem("MapRendererDlgCaption", buffer))
		SetWindowTextA(buffer);
}

void CMapRendererDlg::OnOK()
{
	UpdateData(TRUE);
	CIsoViewExt::RenderCurrentLayers = b_GameLayers != 0;
	CIsoViewExt::RenderFullMap = b_LocalSize != 0;
	CIsoViewExt::RenderInvisibleOverlays = b_DisplayInvisibleOverlay;
	CIsoViewExt::RenderEmphasizeOres = b_MarkOres;
	CIsoViewExt::RenderMarkStartings = b_MarkStartPositions;
	CIsoViewExt::RenderLighing = (RendererLighting)n_Lighting;
	CIsoViewExt::RenderIgnoreObjects = b_IgnoreObjects;
	EndDialog(0);
}

void CMapRendererDlg::OnCancel()
{
	EndDialog(2);
}