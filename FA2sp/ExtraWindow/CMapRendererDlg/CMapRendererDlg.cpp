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
}

void CMapRendererDlg::DoDataExchange(ppmfc::CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Radio(pDX, 1001, b_LocalSize);
	DDX_Radio(pDX, 1003, b_GameLayers);

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
	
	if (Translations::GetTranslationItem("MapRendererDlgCaption", buffer))
		SetWindowTextA(buffer);
}

void CMapRendererDlg::OnOK()
{
	UpdateData(TRUE);
	CIsoViewExt::RenderCurrentLayers = b_GameLayers != 0;
	CIsoViewExt::RenderFullMap = b_LocalSize != 0;
	EndDialog(0);
}

void CMapRendererDlg::OnCancel()
{
	EndDialog(2);
}