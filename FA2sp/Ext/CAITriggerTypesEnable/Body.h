#pragma once

#include <CAITriggerTypesEnable.h>
#include "../FA2Expand.h"
#include "../../ExtraWindow/Common.h"

class NOVTABLE CAITriggerTypesEnableExt : public CAITriggerTypesEnable
{
public:
	typedef BOOL(*FuncT_PTM)(MSG* pMsg);

	static CAITriggerTypesEnable* Instance;
	static TransparencyHelper m_transparency;

	//hook function to replace in virtual function map
	BOOL PreTranslateMessageExt(MSG* pMsg);
	BOOL OnCommandExt(WPARAM wParam, LPARAM lParam);
	BOOL OnInitDialogExt();

	static void ProgramStartupInit();

	CAITriggerTypesEnableExt() {};
	~CAITriggerTypesEnableExt() {};

private:

};