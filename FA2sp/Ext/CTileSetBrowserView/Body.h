#pragma once

#include <CTileSetBrowserView.h>
#include "../FA2Expand.h"

struct CustomTile;

class NOVTABLE CTileSetBrowserViewExt : public CTileSetBrowserView
{
public:

	static void ProgramStartupInit();

	//
	// Ext Functions
	//

	BOOL OnInitDialogExt();
	BOOL PreTranslateMessageExt(MSG* pMsg);

	CTileSetBrowserViewExt() {};
	~CTileSetBrowserViewExt() {};

	// Functional Functions
	void OnBNTileManagerClicked();

	// Render a tile (standard or custom) into an offscreen DirectDraw surface.
	static LPDIRECTDRAWSURFACE7 RenderTile(int iTileIndex);

	// Size helpers shared with Hooks.cpp.
	static int GetAddedHeight(int tileIndex);
	static int GetAddedWidth(int tileIndex);
	static void GetCustomTileSize(const CustomTile* tileData, int& width, int& height);

private:

};