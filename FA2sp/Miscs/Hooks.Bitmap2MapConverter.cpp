#include "../Ext/CMapData/Body.h"
#include "TheaterInfo.h"

DEFINE_HOOK(4138A0, CBitmap2MapConverter_Convert, 7)
{
	GET_STACK(HBITMAP, hBitmap, 0x4);
	GET_STACK(CMapData*, mapdata, 0x8);

	BITMAP bm;

	GetObject(hBitmap, sizeof(BITMAP), &bm);

	HBITMAP hUsed = hBitmap;

	if (bm.bmWidth + bm.bmHeight > 511)
	{
		float scalex = (float)bm.bmWidth / (float)bm.bmHeight;
		int neededheight, neededwidth;
		neededheight = 511.0f / (scalex + 1.0f);
		neededwidth = 511 - neededheight;

		hUsed = CreateCompatibleBitmap(GetDC(NULL), neededwidth, neededheight);
		HDC hDC = CreateCompatibleDC(GetDC(NULL));
		SelectObject(hDC, hUsed);
		HDC hSrcDC = CreateCompatibleDC(GetDC(NULL));
		SelectObject(hSrcDC, hBitmap);

		StretchBlt(hDC, 0, 0, neededwidth, neededheight, hSrcDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

		DeleteDC(hDC);
		DeleteDC(hSrcDC);

		GetObject(hUsed, sizeof(BITMAP), &bm);
	}

	HDC hDC;
	hDC = CreateCompatibleDC(GetDC(NULL));
	SelectObject(hDC, hUsed);


	srand(GetTickCount());

	int i;
	int e;
	int theater = 0;
	mapdata->CreateMap(bm.bmWidth, bm.bmHeight, CMapDataExt::BitmapImporterTheater, 0);

	// weirdly MapWidthPlusHeight not loaded yet
	int isosize = bm.bmWidth + bm.bmHeight;

	int waterSet = CINI::CurrentTheater->GetInteger("General", "WaterSet");
	if (CMapDataExt::BitmapImporterTheater == "LUNAR")
		waterSet = CINI::CurrentTheater->GetInteger("General", "PaveTile");
	int sandset = CINI::CurrentTheater->GetInteger("General", "GreenTile");
	int greenset = CINI::CurrentTheater->GetInteger("General", "RoughTile");

	int water_start = CMapDataExt::TileSet_starts[waterSet] + 8; // to 12
	if (CMapDataExt::BitmapImporterTheater == "LUNAR")
		water_start = CMapDataExt::TileSet_starts[waterSet];
	int sand_start = CMapDataExt::TileSet_starts[sandset];
	int green_start = CMapDataExt::TileSet_starts[greenset];

	for (i = 0; i < bm.bmWidth; i++)
	{
		for (e = 0; e < bm.bmHeight; e++)
		{
			COLORREF col = GetPixel(hDC, i, bm.bmHeight - e);

			int x = (i)+ mapdata->Size.Height + 1;
			int y = (e)+ mapdata->Size.Width;

			int xiso;
			int yiso;

			yiso = isosize - (y - x);
			xiso = isosize - (x + y);
			yiso -= mapdata->Size.Height;


			for (x = -1; x < 2; x++)
			{
				for (y = -1; y < 2; y++)
				{
					DWORD dwPos = xiso + x + (yiso + y) * isosize;

					if (dwPos > isosize * isosize) continue;

					CellData* fd = mapdata->GetCellAt(dwPos);

					int r = GetRValue(col);
					int g = GetGValue(col);
					int b = GetBValue(col);

					if (g > r && g > b)
					{
						if (theater != 1)
						{
							fd->TileIndex = 0;
							fd->TileSubIndex = 0;
						}
					}
					if (b > g && b > r)
					{
						if (CMapDataExt::BitmapImporterTheater != "LUNAR")
						{
							fd->TileIndex = STDHelpers::RandomSelectInt(TheaterInfo::CurrentSmallWaters);
						}
						else
						{
							fd->TileIndex = water_start;
						}

						fd->TileSubIndex = 0;
					}
					if (g > b + 25 && r > b + 25 && g > 120 && r > 120)
					{
						if (theater != 1)
						{
							fd->TileIndex = sand_start;
							fd->TileSubIndex = 0;
						}
					}
					if (b < 20 && r < 20 && g>20)
					{
						if (g < 140) // dark only

						{
							fd->TileIndex = green_start;
							fd->TileSubIndex = 0;
						}

					}

				}
			}
		}
	}
	
	// not necessary
	//mapdata->CreateShore(0, 0, isosize, isosize);
	for (int x = 0; x < isosize; x++)
	{
		for (int y = 0; y < isosize; y++)
		{
			CMapDataExt::SmoothTileAt(x, y);
		}
	}
	CMapDataExt::SmoothWater();
	
	DeleteDC(hDC);

	if (hUsed != hBitmap) DeleteObject(hUsed);

	return 0x4142CC;
}
