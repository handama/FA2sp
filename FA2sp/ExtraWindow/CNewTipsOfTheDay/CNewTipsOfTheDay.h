#pragma once

#include <vector>
#include "FA2PP.h"
#include "../../FA2sp/Helpers/FString.h"
#include "../../Helpers/MultimapHelper.h"
#include "../Common.h"

class CNewTipsOfTheDay : public ppmfc::CDialog
{
public:
	static void ShowNewTipsOfTheDay();
	static CNewTipsOfTheDay* m_pNewTipsOfTheDayDlg;

protected:
	CNewTipsOfTheDay(CWnd* pParent = NULL);
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(ppmfc::CDataExchange* pDX);
	virtual void OnOK();
	virtual void OnCancel();
	virtual void OnClose();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void PostNcDestroy();
	static TransparencyHelper m_transparency;
	void OnPrevious();
	ppmfc::CStatic m_staticPic;
	ppmfc::CStatic m_staticText;
	BOOL b_ShowAtStartup;
	std::vector<FString> m_Tips;
	int m_CurrentPos;
};
