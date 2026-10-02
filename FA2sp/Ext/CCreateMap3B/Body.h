#pragma once

#include <CCreateMap3B.h>
#include "../FA2Expand.h"

class NOVTABLE CCreateMap3BExt : public CCreateMap3B
{
public:
	typedef BOOL(*FuncT_PTM)(MSG* pMsg);

	static CCreateMap3B* Instance;

	//hook function to replace in virtual function map
	BOOL OnCommandExt(WPARAM wParam, LPARAM lParam);
	BOOL OnInitDialogExt();

	static void ProgramStartupInit();
	static ppmfc::CMenu* TheaterMenu;

	CCreateMap3BExt() {};
	~CCreateMap3BExt() {};

private:

};