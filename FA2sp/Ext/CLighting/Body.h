#pragma once

#include <CLighting.h>
#include "../FA2Expand.h"
#include "../../ExtraWindow/Common.h"

class NOVTABLE CLightingExt : public CLighting
{
public:
	static TransparencyHelper m_transparency;

	//hook function to replace in virtual function map
	BOOL PreTranslateMessageExt(MSG* pMsg);
	BOOL OnCommandExt(WPARAM wParam, LPARAM lParam);
	BOOL OnInitDialogExt(void);

	static void ProgramStartupInit();

	void Translate();

	CLightingExt() {};
	~CLightingExt() {};

private:

};