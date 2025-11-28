#pragma once
#include <vector>
#include "FA2PP.H"
#include "../../Helpers/Translations.h"

class CMultiSelectionOptionDlg : public ppmfc::CDialog
{
public:
	CMultiSelectionOptionDlg(CWnd* pParent = NULL);
	BOOL Connected4;
	BOOL Connected8;
	BOOL SameTileSet;
	BOOL ConsiderLAT;
	BOOL SameHeight;
	BOOL SameBaiscHeight;

protected:
	virtual void DoDataExchange(ppmfc::CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
};
