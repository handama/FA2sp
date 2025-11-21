#include "CMultiSelectionOptionDlg.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../Common.h"

CMultiSelectionOptionDlg::CMultiSelectionOptionDlg(CWnd* pParent /*=NULL*/)
	: ppmfc::CDialog(323, pParent)
{
	Connected4 = TRUE;
	Connected8 = FALSE;
	SameTileSet = TRUE;
	ConsiderLAT = TRUE;
	SameHeight = FALSE;
	SameBaiscHeight = FALSE;
}

void CMultiSelectionOptionDlg::DoDataExchange(ppmfc::CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, 1001, Connected4);
	DDX_Check(pDX, 1006, Connected8);
	DDX_Check(pDX, 1002, SameTileSet);
	DDX_Check(pDX, 1003, ConsiderLAT);
	DDX_Check(pDX, 1004, SameHeight);
	DDX_Check(pDX, 1005, SameBaiscHeight);

	FString buffer;

	if (Translations::GetTranslationItem("OK", buffer))
		GetDlgItem(1)->SetWindowTextA(buffer);

	if (Translations::GetTranslationItem("MultiSelectionOptionConnected", buffer))
		GetDlgItem(1001)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MultiSelectionOptionConnected8Ways", buffer))
		GetDlgItem(1006)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MultiSelectionOptionSameTileSet", buffer))
		GetDlgItem(1002)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MultiSelectionOptionConsiderLAT", buffer))
		GetDlgItem(1003)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MultiSelectionOptionSameHeight", buffer))
		GetDlgItem(1004)->SetWindowTextA(buffer);
	if (Translations::GetTranslationItem("MultiSelectionOptionSameBaiscHeight", buffer))
		GetDlgItem(1005)->SetWindowTextA(buffer);


	if (Translations::GetTranslationItem("MultiSelectionOptionCaption", buffer))
		SetWindowTextA(buffer);
}


BOOL CMultiSelectionOptionDlg::OnInitDialog()
{
	ppmfc::CDialog::OnInitDialog();

	if (Connected4) ::SendMessage(GetDlgItem(1001)->GetSafeHwnd(), BM_SETCHECK, BST_CHECKED, 0);
	if (Connected8) ::SendMessage(GetDlgItem(1006)->GetSafeHwnd(), BM_SETCHECK, BST_CHECKED, 0);
	if (SameTileSet) ::SendMessage(GetDlgItem(1002)->GetSafeHwnd(), BM_SETCHECK, BST_CHECKED, 0);
	if (ConsiderLAT) ::SendMessage(GetDlgItem(1003)->GetSafeHwnd(), BM_SETCHECK, BST_CHECKED, 0);
	if (SameHeight) ::SendMessage(GetDlgItem(1004)->GetSafeHwnd(), BM_SETCHECK, BST_CHECKED, 0);
	if (SameBaiscHeight) ::SendMessage(GetDlgItem(1005)->GetSafeHwnd(), BM_SETCHECK, BST_CHECKED, 0);

	return TRUE;  
}

void CMultiSelectionOptionDlg::OnOK()
{
	UpdateData(TRUE);
	EndDialog(0);
}

void CMultiSelectionOptionDlg::OnCancel()
{
	EndDialog(0);
}
