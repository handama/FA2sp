#pragma once

#include <vector>
#include "FA2PP.h"
#include "../../FA2sp/Helpers/FString.h"
#include "../../Helpers/MultimapHelper.h"

class CNewTipsOfTheDay : public ppmfc::CDialog
{
public:
	static void ShowNewTipsOfTheDay();

protected:
	CNewTipsOfTheDay(CWnd* pParent = NULL);
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(ppmfc::CDataExchange* pDX);
	virtual void OnOK();
	virtual void OnCancel();
	virtual void OnClose();
	virtual void PostNcDestroy();
	static CNewTipsOfTheDay* m_pNewTipsOfTheDayDlg;
	ppmfc::CStatic m_staticPic;
	ppmfc::CStatic m_staticText;
	BOOL b_ShowAtStartup;
	std::vector<FString> m_Tips;
	int m_CurrentPos;
};
