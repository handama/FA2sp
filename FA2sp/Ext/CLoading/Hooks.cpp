#include "Body.h"

#include <CFinalSunApp.h>
#include <CMixFile.h>
#include <CLoading.h>
#include <CINI.h>
#include <Drawing.h>
#include <string>
#include "..\..\Miscs\Palettes.h"
#include "..\CMapData\Body.h"
#include "../CIsoView/Body.h"
#include <filesystem>
namespace fs = std::filesystem;

DEFINE_HOOK(486B00, CLoading_InitMixFiles, 7)
{
	GET(CLoadingExt*, pThis, ECX);

	bool result = pThis->InitMixFilesFix();
	R->EAX(result);

	return 0x48A26A;
}


DEFINE_HOOK(48FCE1, CLoading_LoadOverlayGraphic_SearchFile, 8)
{
	GET(int, hMix, ESI);
	GET_STACK(const char*, lpBuffer, STACK_OFFS(0x1C4, 0x1B4));
	if (hMix != NULL)
		return 0x4900B0;

	ppmfc::CString filepath = CFinalSunApp::FilePath();
	filepath += lpBuffer;
	std::ifstream fin;
	fin.open(filepath, std::ios::in | std::ios::binary);
	if (fin.is_open())
	{
		fin.close();
		R->ESI(1919810);
		return 0x4900B0;
	}
	size_t size = 0;
	auto data = ResourcePackManager::instance().getFileData(lpBuffer, &size);
	if (data && size > 0)
	{
		R->ESI(1919810);
		return 0x4900B0;
	}
	return 0x48FCE9;
}

DEFINE_HOOK(48FDA0, CLoading_LoadOverlayGraphic_NewTheaterFix, 8)
{
	GET(int, hMix, ESI);
	GET(CLoadingExt*, pThis, ECX);
	GET_STACK(const char*, lpBuffer, STACK_OFFS(0x1C4, 0x1B4));
	GET_STACK(const char*, artID, STACK_OFFS(0x1C4, 0x1AC));
	if (hMix != NULL) 
		return 0x48FE68;
	ppmfc::CString filename = lpBuffer;
	hMix = pThis->SearchFile(filename);
	if (hMix == NULL) 
		hMix = 1919810;
	bool findFile = false;
	findFile = pThis->HasFile(filename);
	auto searchNewTheater = [&findFile, &hMix, &pThis, &filename](char t)
		{
			if (!findFile)
			{
				filename.SetAt(1, t);
				hMix = pThis->SearchFile(filename);
				if (hMix == NULL)
					hMix = 1919810;
				findFile = pThis->HasFile(filename);
			}
		};

	if (CINI::Art->GetBool(artID, "NewTheater") && strlen(artID) >= 2)
	{
		searchNewTheater('G');
		searchNewTheater(artID[1]);
		if (!ExtConfigs::UseStrictNewTheater)
		{
			searchNewTheater('T');
			searchNewTheater('A');
			searchNewTheater('U');
			searchNewTheater('N');
			searchNewTheater('L');
			searchNewTheater('D');
		}
	}
	
	if (findFile)
	{
		R->ESI(hMix);
		R->Stack(STACK_OFFS(0x1C4, 0x1B4), filename);
		return 0x48FE68;
	}

	return 0x48FDA8;
}

DEFINE_HOOK(48A650, CLoading_SearchFile, 6)
{
	GET(CLoadingExt*, pThis, ECX);
	GET_STACK(const char*, Filename, 0x4);
	GET_STACK(unsigned char*, pTheaterType, 0x8);
	
	for (int i = 0; i < CMixFile::ArraySize; ++i)
	{
		auto& mix = CMixFile::Array[i];
		if (!mix.is_open())
			break;
		if (CMixFile::HasFile(Filename, i + 1))
		{
#ifndef NDEBUG
			Logger::Debug("[SearchFile] %s - %d\n", Filename, i + 1);
#endif
			R->EAX(i + 1);
			return 0x48AA63;
		}
	}

#ifndef NDEBUG
	Logger::Debug("[SearchFile] %s - NOT FOUND\n", Filename);
#endif
	R->EAX(0);
	return 0x48AA63;
}

DEFINE_HOOK(49DD64, CMapData_LoadMap_InitMixFiles_Removal, 5)
{
	CLoading::Instance->CSCBuiltby.~CStatic();
	CLoading::Instance->CSCLoading.~CStatic();
	CLoading::Instance->CSCVersion.~CStatic();
	CLoading::Instance->CPCProgress.~CProgressCtrl();
	struct CLoadingHelper
	{
		void DTOR()
		{
			JMP_STD(0x551A1D);
		}
	};
	reinterpret_cast<CLoadingHelper*>(CLoading::Instance())->DTOR();
	return 0x49DD74;
}

DEFINE_HOOK(4B8CFC, CMapData_CreateMap_InitMixFiles_Removal, 5)
{
	CLoading::Instance->CSCBuiltby.~CStatic();
	CLoading::Instance->CSCLoading.~CStatic();
	CLoading::Instance->CSCVersion.~CStatic();
	CLoading::Instance->CPCProgress.~CProgressCtrl();
	struct CLoadingHelper
	{
		void DTOR()
		{
			JMP_STD(0x551A1D);
		}
	};
	reinterpret_cast<CLoadingHelper*>(CLoading::Instance())->DTOR();
	return 0x4B8D0C;
}

DEFINE_HOOK(4903F3, CLoading_DrawOverlay_Palette, 7)
{
	GET(CLoadingExt*, pThis, EDI);
	REF_STACK(ImageDataClass, pDrawData, STACK_OFFS(0x1C8, 0xC8));
	GET_STACK(int, nOverlayIndex, STACK_OFFS(0x1C8, -0x8));

	if (!CMapData::Instance->MapWidthPlusHeight) {
		return 0;
	}

	if (nOverlayIndex >= 0 && nOverlayIndex < 256)
	{
		auto const& typeData = CMapDataExt::OverlayTypeDatas[nOverlayIndex];

		if (typeData.Wall)
		{
			auto palName = typeData.WallPaletteName;
			pThis->GetFullPaletteName(palName);
			if (auto pal = PalettesManager::LoadPalette(palName)) {
				BGRStruct houseColor{ 0,0,255 };
				auto lightingPal = PalettesManager::GetPalette(pal, houseColor);
				pDrawData.pPalette = lightingPal;
			}
		}
	}

	return 0;
}

DEFINE_HOOK(48E541, CLoading_InitTMPs_UpdateTileDatas, 7)
{
	auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");
	if (thisTheater == "TEMPERATE")
	{
		CMapDataExt::TileData = CTileTypeInfo::Temperate().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::Temperate().Count;
	}
	if (thisTheater == "SNOW")
	{
		CMapDataExt::TileData = CTileTypeInfo::Snow().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::Snow().Count;
	}
	if (thisTheater == "URBAN")
	{
		CMapDataExt::TileData = CTileTypeInfo::Urban().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::Urban().Count;
	}
	if (thisTheater == "NEWURBAN")
	{
		CMapDataExt::TileData = CTileTypeInfo::NewUrban().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::NewUrban().Count;
	}
	if (thisTheater == "LUNAR")
	{
		CMapDataExt::TileData = CTileTypeInfo::Lunar().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::Lunar().Count;
	}
	if (thisTheater == "DESERT")
	{
		CMapDataExt::TileData = CTileTypeInfo::Desert().Datas;
		CMapDataExt::TileDataCount = CTileTypeInfo::Desert().Count;
	}

	return 0;
}

DEFINE_HOOK(48E970, CLoading_LoadTile_SkipTranspInsideCheck, 6)
{
	return 0x48EA44;
}

DEFINE_HOOK(47AB50, CLoading_InitPics_LoadDLLBitmaps, 7)
{
	auto loadInternalBitmap = [](const char* imageID, int resource)
		{
			HBITMAP hBmp = (HBITMAP)LoadImage(static_cast<HINSTANCE>(FA2sp::hInstance), MAKEINTRESOURCE(resource),
				IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
			CBitmap cBitmap;
			cBitmap.Attach(hBmp);
			CLoadingExt::LoadBitMap(imageID, cBitmap);
		};
	loadInternalBitmap("annotation.bmp", 1001);
	loadInternalBitmap("FLAG", 1023);
	loadInternalBitmap("CELLTAG", 1024);

	std::string pics = CFinalSunApp::ExePath();
	pics += "\\pics";
	if (fs::exists(pics) && fs::is_directory(pics))
	{
		for (const auto& entry : fs::directory_iterator(pics)) {
			if (fs::is_regular_file(entry.status())) {
				if (entry.path().extension() == ".bmp")
				{
					CBitmap bmp;
					if (CLoadingExt::LoadBMPToCBitmap(entry.path().c_str(), bmp))
						CLoadingExt::LoadBitMap(entry.path().filename().c_str(), bmp);
				}
			}
		}
	}

	return 0;
}

DEFINE_HOOK(47FA2D, CLoading_InitPics_End_LoadDLLBitmaps, 7)
{
	auto replace = [](const char* Ori, const char* New)
		{
			auto image_ori = CLoadingExt::GetSurfaceImageDataFromMap(Ori);
			if (image_ori->lpSurface)
			{
				if (CLoadingExt::IsImageLoaded(New))
				{
					auto image_new = CLoadingExt::GetSurfaceImageDataFromMap(New);
					image_ori->lpSurface = image_new->lpSurface;
				}
				DDSURFACEDESC2 ddsd;
				memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
				ddsd.dwSize = sizeof(DDSURFACEDESC2);
				ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
				image_ori->lpSurface->GetSurfaceDesc(&ddsd);
				image_ori->FullWidth = ddsd.dwWidth;
				image_ori->FullHeight = ddsd.dwHeight;
			}
		};

	replace("CELLTAG", "celltag.bmp");
	replace("FLAG", "waypoint.bmp");
	replace("FLAG", "waypoint.bmp");

	return 0;
}

static bool hasExtraImage = false;
static int width, height;
static int tileIndex = -1;
static int subTileIndex = -1;
static int altCount[100];
static byte* image = nullptr;

DEFINE_HOOK(48E580, CLoading_LoadTile_Init, 7)
{
	if (R->Stack<int>(0x10) != tileIndex)
	{
		std::memset(altCount, 0, sizeof(altCount));
	}
	tileIndex = R->Stack<int>(0x10);
	return 0;
}

DEFINE_HOOK(52CE30, CLoading_DrawTMP_1, 5)
{
	subTileIndex = R->Stack<int>(0x8);
	hasExtraImage = false;
	return 0;
}

DEFINE_HOOK(52CE78, CLoading_DrawTMP_2, 6)
{
	hasExtraImage = true;
	return 0;
}

DEFINE_HOOK(52CEEB, CLoading_DrawTMP_3, 7)
{
	if (hasExtraImage)
	{
		width = R->EDX();
		height = R->ECX();
	}
	return 0;
}

DEFINE_HOOK(52D047, CLoading_DrawTMP_4, 8)
{
	if (hasExtraImage)
	{
		if (image)
		{
			delete[] image;
		}
		int size = width * height;
		image = new byte[size];
		byte* image2 = R->EDX<byte*>();
		for (int i = 0; i < size; ++i) {
			image[i] = image2[i];
		}
	}
	return 0;
}

DEFINE_HOOK(52D098, CLoading_DrawTMP_5, 5)
{
	if (hasExtraImage)
	{
		byte* image2 = R->ESI<byte*>();
		int size = width * height;
		byte* diff = GameCreateArray<byte>(size);

		for (int i = 0; i < size; ++i) {
			diff[i] = (image[i] != image2[i]) ? image2[i] : 0;
		}

		auto loadingExt = (CLoadingExt*)CLoading::Instance();
		ppmfc::CString ImageID;
		ImageID.Format("EXTRAIMAGE\233%d%d%d", tileIndex, subTileIndex, altCount[subTileIndex]);
		loadingExt->SetImageDataSafe(diff, ImageID, width, height, Palette::PALETTE_ISO);
		CLoadingExt::LoadedObjects.insert(ImageID);
		altCount[subTileIndex]++;
	}
	return 0;
}

