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
#include "../CFinalSunApp/Body.h"
namespace fs = std::filesystem;

DEFINE_HOOK(486B00, CLoading_InitMixFiles, 7)
{
	GET(CLoadingExt*, pThis, ECX);

	bool result = pThis->InitMixFilesFix();
	R->EAX(result);

	return 0x48A26A;
}

DEFINE_HOOK(48A650, CLoading_SearchFile, 6)
{
	GET(CLoadingExt*, pThis, ECX);
	GET_STACK(const char*, Filename, 0x4);
	GET_STACK(unsigned char*, pTheaterType, 0x8);

	if (ExtConfigs::ExtMixLoader)
	{
		auto& manager = MixLoader::Instance();
		auto result = manager.QueryFileIndex(Filename);
		if (result >= 0)
		{
			R->EAX(result);
			return 0x48AA63;
		}
	}

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
	loadInternalBitmap("PROPERTY_MARK", 1036);

	std::string pics = CFinalSunAppExt::ExePathExt;
	pics += "\\pics";
	if (fs::exists(pics) && fs::is_directory(pics))
	{
		for (const auto& entry : fs::directory_iterator(pics)) {
			if (fs::is_regular_file(entry.status())) {
				if (entry.path().extension() == ".bmp")
				{
					CBitmap bmp;
					if (CLoadingExt::LoadBMPToCBitmap(entry.path().string(), bmp))
						CLoadingExt::LoadBitMap(entry.path().filename().string(), bmp);
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
				if (CLoadingExt::IsSurfaceImageLoaded(New))
				{
					auto image_new = CLoadingExt::GetSurfaceImageDataFromMap(New);
					image_ori->lpSurface->Release();
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
	replace("PROPERTY_MARK", "property_mark.bmp");

	return 0;
}

static bool hasExtraImage = false;
static int width, height;
static int tileIndex = -1;
static int subTileIndex = -1;
static int altCount[100];
static t_tmp_image_header* currentTMP = nullptr;
static byte* tmp_file_image = nullptr;

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
	currentTMP = R->EAX<t_tmp_image_header*>();
	return 0;
}

DEFINE_HOOK(52D04F, CLoading_DrawTMP_GetImage, 6)
{
	tmp_file_image = R->EDI<byte*>();
	if (R->EAX() >= 0)
	{
		R->EDX(R->EDX() + R->EAX());
	}

	return 0x52D055;
}

DEFINE_HOOK(52D098, CLoading_DrawTMP_5, 5)
{
	if (hasExtraImage)
	{
		int size = currentTMP->cx_extra * currentTMP->cy_extra;
		byte* extra_image = GameCreateArray<byte>(size);
		memcpy(extra_image, tmp_file_image, size);

		auto loadingExt = (CLoadingExt*)CLoading::Instance();
		FString ImageID;
		ImageID.Format("EXTRAIMAGE\233%d%d%d", tileIndex, subTileIndex, altCount[subTileIndex]);

		CLoadingExt::TileExtraOffsets[
			CLoadingExt::GetTileIdentifier(tileIndex, subTileIndex, altCount[subTileIndex])]
			= MapCoord{ currentTMP->x_extra - currentTMP->x, currentTMP->y_extra - currentTMP->y };

		Palette* pal = &CMapDataExt::Palette_ISO;
		if (CMapDataExt::TileData && tileIndex < CMapDataExt::TileDataCount 
			&& CMapDataExt::TileData[tileIndex].TileSet < CMapDataExt::TileSetPalettes.size())
		{
			pal = CMapDataExt::TileSetPalettes[CMapDataExt::TileData[tileIndex].TileSet];
		}
		loadingExt->SetImageDataSafe(extra_image, ImageID, currentTMP->cx_extra, currentTMP->cy_extra, pal, false);
		CLoadingExt::LoadedObjects.insert(ImageID);
		altCount[subTileIndex]++;
	}
	return 0;
}

DEFINE_HOOK(48E541, CLoading_InitTMPs_LoadTileData, 7)
{
	CMapDataExt::InitializeTileData();
	return 0;
}

DEFINE_HOOK(491D86, CLoading_Release_TheaterIsDesert, 5)
{
	R->ESI(5);
	return 0x491D8B;
}

DEFINE_HOOK(491E96, CLoading_Release_SetTileDataType, 5)
{
	GET(int, TheaterType, ESI);
	switch (TheaterType)
	{
	case 0:
		CTileTypeClass::Instance = &CTileTypeInfo::Temperate->Datas;
		CTileTypeClass::InstanceCount = &CTileTypeInfo::Temperate->Count;
		break;
	case 1:
		CTileTypeClass::Instance = &CTileTypeInfo::Snow->Datas;
		CTileTypeClass::InstanceCount = &CTileTypeInfo::Snow->Count;
		break;
	case 2:
		CTileTypeClass::Instance = &CTileTypeInfo::Urban->Datas;
		CTileTypeClass::InstanceCount = &CTileTypeInfo::Urban->Count;
		break;
	case 3:
		CTileTypeClass::Instance = &CTileTypeInfo::NewUrban->Datas;
		CTileTypeClass::InstanceCount = &CTileTypeInfo::NewUrban->Count;
		break;
	case 4:
		CTileTypeClass::Instance = &CTileTypeInfo::Lunar->Datas;
		CTileTypeClass::InstanceCount = &CTileTypeInfo::Lunar->Count;
		break;
	case 5:
		CTileTypeClass::Instance = &CTileTypeInfo::Desert->Datas;
		CTileTypeClass::InstanceCount = &CTileTypeInfo::Desert->Count;
		break;
	default:
		CTileTypeClass::Instance = &CTileTypeInfo::Temperate->Datas;
		CTileTypeClass::InstanceCount = &CTileTypeInfo::Temperate->Count;
		break;
	}
	if (CMapDataExt::TileData)
		delete[] CMapDataExt::TileData;
	CMapDataExt::TileData = nullptr;
	Logger::Debug("CLoading::Release() called.\n");
	return 0x491F36;
}