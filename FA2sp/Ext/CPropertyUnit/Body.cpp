#include "Body.h"

#include "../CFinalSunDlg/Body.h"

#include "../../Helpers/Translations.h"
#include "../CMapData/Body.h"

bool uTheFirst = true;
int uNode = NULL;

void CPropertyUnitExt::ProgramStartupInit()
{
	RunTime::ResetMemoryContentAt(0x5985D0, &CPropertyUnitExt::PreTranslateMessageExt);
	RunTime::ResetMemoryContentAt(0x5985B8, &CPropertyUnitExt::OnCommandExt);
}

BOOL CPropertyUnitExt::PreTranslateMessageExt(MSG* pMsg)
{
#define DECLARE_BST(id) \
if(pMsg->hwnd == this->GetDlgItem(id)->GetSafeHwnd()) \
{ \
	bool tmp = ::SendMessage(::GetDlgItem(*this, id), BM_GETCHECK, 0, 0) == BST_CHECKED; \
	::SendMessage(::GetDlgItem(*this, id), BM_SETCHECK, tmp ? BST_UNCHECKED : BST_CHECKED, 0); \
	CViewObjectsExt::VehicleBrushBools[id-1300] = tmp == false; \
}
	if (pMsg->message == WM_LBUTTONUP)
	{
		DECLARE_BST(1300)
	else DECLARE_BST(1301)
else DECLARE_BST(1302)
	else DECLARE_BST(1303)
	else DECLARE_BST(1304)
	else DECLARE_BST(1305)
	else DECLARE_BST(1306)
	else DECLARE_BST(1307)
	else DECLARE_BST(1308)
	else DECLARE_BST(1309)
	else DECLARE_BST(1310)
	}

#undef DECLARE_BST

	return this->ppmfc::CDialog::PreTranslateMessage(pMsg);
}

BOOL CPropertyUnitExt::OnCommandExt(WPARAM wParam, LPARAM lParam)
{
	WORD wmID = LOWORD(wParam);
	WORD wmMsg = HIWORD(wParam);

	if (wmID == 1082)
	{
		StatusProc(wmMsg, lParam);
	}
	if (wmID == 1 || wmID == 2)
	{
		uTheFirst = true;
	}

	return this->ppmfc::CDialog::OnCommand(wParam, lParam);
}

void CPropertyUnitExt::StatusUpdate(LPARAM lParam)
{
	HWND hStatusComboBox = reinterpret_cast<HWND>(lParam);

	if (uTheFirst)
	{
		uNode = ::SendMessage(hStatusComboBox, CB_GETCURSEL, 0, 0);
		uTheFirst = false;
	}

	while (::SendMessage(hStatusComboBox, CB_DELETESTRING, 0, NULL) != CB_ERR);

	for (int i = 0; i < CMapDataExt::TechnoStates.size(); ++i)
	{
		const auto& state = CMapDataExt::TechnoStates[i];
		FString key = "FootClassStatus.";
		key += state;
		::SendMessage(hStatusComboBox, CB_INSERTSTRING, i,
			static_cast<LPARAM>(Translations::TranslateOrDefault(key, state)));
	}
	::SendMessage(hStatusComboBox, CB_SETCURSEL, uNode, 0);


	return;
}

void CPropertyUnitExt::StatusProc(WORD nCode, LPARAM lParam)
{
	HWND hStatusComboBox = reinterpret_cast<HWND>(lParam);
	if (::SendMessage(hStatusComboBox, CB_GETCOUNT, NULL, NULL) <= 0)
		return;
	switch (nCode)
	{
	case CBN_DROPDOWN:
		StatusUpdate(lParam);
		break;
	//case CBN_SELCHANGE:
	case CBN_DBLCLK:
	case CBN_CLOSEUP:
		CurrentState(hStatusComboBox, ::SendMessage(hStatusComboBox, CB_GETCURSEL, NULL, NULL));
		break;
	default:
		break;
	}
	return;
}

void CPropertyUnitExt::CurrentState(HWND nComboBox, int Node)
{
	if (Node < CMapDataExt::TechnoStates.size() && Node >= 0)
	{
		::SendMessage(nComboBox, CB_SETCURSEL, 
			::SendMessage(nComboBox, CB_INSERTSTRING, CMapDataExt::TechnoStates.size(),
				CMapDataExt::TechnoStates[Node]), 0);
		uNode = Node;
	}
	return;
}
