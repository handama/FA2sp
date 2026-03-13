#pragma once

#include <CMinimap.h>
#include "../FA2Expand.h"

class NOVTABLE CMinimapExt : public CMinimap
{
public:
	typedef BOOL(*FuncT_PTM)(MSG* pMsg);

	static LRESULT CALLBACK MinimapWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static WNDPROC g_pfnOriginalMinimapProc;
	static double ASPECT_RATIO;
	static int InitWidth; 
	static double CurrentScale;

	CMinimapExt() {};
	~CMinimapExt() {};

private:

};