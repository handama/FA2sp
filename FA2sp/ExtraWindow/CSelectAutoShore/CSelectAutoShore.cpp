#include "CSelectAutoShore.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../Common.h"
#include "../../Ext/CMapData/Body.h"

CSelectAutoShore::CSelectAutoShore(CWnd* pParent) : ppmfc::CDialog(319, pParent)
{
	Groups.clear();
	m_Combo = "";
}

void CSelectAutoShore::DoDataExchange(ppmfc::CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_CBString(pDX, 1002, m_Combo);

	FString buffer;

	if (Translations::GetTranslationItem("OK", buffer))
		GetDlgItem(1)->SetWindowTextA(buffer);
	
	if (Translations::GetTranslationItem("CSelectAutoShoreCap", buffer))
		SetWindowTextA(buffer);	

	if (Translations::GetTranslationItem("CSelectAutoShoreDesc", buffer))
		GetDlgItem(1001)->SetWindowTextA(buffer);
}


BOOL CSelectAutoShore::OnInitDialog()
{
	ppmfc::CDialog::OnInitDialog();

	CComboBox* box = (CComboBox*)GetDlgItem(1002);

	if (auto pSection = CINI::FAData->GetSection("AutoShoreTypes"))
	{
		int index = 0;
		auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");
		for (const auto& type : pSection->GetEntities())
		{
			auto atoms = STDHelpers::SplitString(type.second, 3);
			if (atoms[0] == thisTheater)
			{
				int shore = -1;
				int green = -1;
				if (!STDHelpers::IsNumber(atoms[2]))
				{
					shore = CINI::CurrentTheater->GetInteger("General", atoms[2], -1);
				}
				else
				{
					shore = atoi(atoms[2]);
				}
				if (!STDHelpers::IsNumber(atoms[3]))
				{
					green = CINI::CurrentTheater->GetInteger("General", atoms[3], -1);
				}
				else
				{
					green = atoi(atoms[3]);
				}
				if (shore >= 0 && CMapDataExt::IsValidTileSet(shore))
				{
					ppmfc::CString text;
					text.Format("%d - %s", index++, Translations::TranslateOrDefault(atoms[1], atoms[1]));
					Groups.push_back(std::make_pair(shore, green));
					box->AddString(text);
				}
			}
		}
	}

	box->SetCurSel(0);
	return TRUE;  
}

void CSelectAutoShore::OnOK()
{
	UpdateData(TRUE);
	STDHelpers::TrimIndex(m_Combo);
	EndDialog(0);
}

void CSelectAutoShore::OnCancel()
{
	m_Combo = "";
	EndDialog(0);
}