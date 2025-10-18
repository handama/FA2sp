#pragma once

#include <vector>
#include "FA2PP.h"
#include "../../FA2sp/Helpers/FString.h"
#include "../../Helpers/MultimapHelper.h"

class CMapRendererDlg : public ppmfc::CDialog
{
public:
	CMapRendererDlg(CWnd* pParent = NULL);
	BOOL b_LocalSize;
	BOOL b_GameLayers;

protected:
	virtual void DoDataExchange(ppmfc::CDataExchange* pDX);
	virtual void OnOK();
	virtual void OnCancel();
};
