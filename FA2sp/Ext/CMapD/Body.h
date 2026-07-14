#pragma once

#include <CMapD.h>
#include "../FA2Expand.h"
#include "../../ExtraWindow/Common.h"

class NOVTABLE CMapDExt : public CMapD
{
public:
	typedef BOOL(*FuncT_PTM)(MSG* pMsg);

	static CMapD* Instance;
	static TransparencyHelper m_transparency;

	//hook function to replace in virtual function map
	BOOL PreTranslateMessageExt(MSG* pMsg);
	BOOL OnCommandExt(WPARAM wParam, LPARAM lParam);
	BOOL OnInitDialogExt();

	static void ProgramStartupInit();

	CMapDExt() {};
	~CMapDExt() {};

private:

};