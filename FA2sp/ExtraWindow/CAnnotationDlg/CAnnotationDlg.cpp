#include "CAnnotationDlg.h"
#include "../../Helpers/STDHelpers.h"
#include "../Common.h"

CAnnotationDlg::CAnnotationDlg(CWnd* pParent /*=NULL*/)
	: ppmfc::CDialog(322, pParent)
{
	m_Text = "";
	m_FontSize = "20";
	m_TextColor = "0x000000";
	m_BgColor = "0xFFFFFF";
	m_Bold = FALSE;
	m_nInitTimer = NULL;
}

void CAnnotationDlg::DoDataExchange(ppmfc::CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, 1000, m_Text);
	DDX_Text(pDX, 1002, m_FontSize);
	//DDX_Text(pDX, 1004, m_TextColor);
	//DDX_Text(pDX, 1006, m_BgColor);
	DDX_Check(pDX, 1007, m_Bold);

	FString buffer;

	if (Translations::GetTranslationItem("OK", buffer))
		GetDlgItem(1)->SetWindowTextA(buffer);

	if (Translations::GetTranslationItem("AnnotationFontSize", buffer))
		GetDlgItem(1001)->SetWindowTextA(buffer);

	if (Translations::GetTranslationItem("AnnotationTextColor", buffer))
		GetDlgItem(1003)->SetWindowTextA(buffer);

	if (Translations::GetTranslationItem("AnnotationBackgroundColor", buffer))
		GetDlgItem(1005)->SetWindowTextA(buffer);

	if (Translations::GetTranslationItem("AnnotationBold", buffer))
		GetDlgItem(1007)->SetWindowTextA(buffer);

	if (Translations::GetTranslationItem("AnnotationCaption", buffer))
		SetWindowTextA(buffer);
}

BOOL CAnnotationDlg::OnInitDialog()
{
	ppmfc::CDialog::OnInitDialog();

	GetDlgItem(1002)->SetWindowTextA(m_FontSize);
	m_Text.Replace("\\n", "\n");
	GetDlgItem(1000)->SetWindowTextA(m_Text);
	if (m_Bold)
		::SendMessage(GetDlgItem(1007)->GetSafeHwnd(), BM_SETCHECK, BST_CHECKED, 0);
	m_nInitTimer = SetTimer(GetSafeHwnd(), 1, 20, nullptr);

	if (ExtConfigs::EnableDarkMode)
	{
		::SendMessage(GetDlgItem(1000)->GetSafeHwnd(), EM_SETBKGNDCOLOR, (WPARAM)FALSE, (LPARAM)RGB(32, 32, 32));
		CHARFORMAT cf = { 0 };
		cf.cbSize = sizeof(cf);
		cf.dwMask = CFM_COLOR;
		cf.crTextColor = RGB(220, 220, 220);
		::SendMessage(GetDlgItem(1000)->GetSafeHwnd(), EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
	}

	return TRUE;  
}

void CAnnotationDlg::OnOK()
{
	UpdateData(TRUE);
	EndDialog(0);

	//CDialog::OnOK();
}

void CAnnotationDlg::OnCancel()
{
	m_Text = "";
	EndDialog(0);
	//CDialog::OnCancel();
}
