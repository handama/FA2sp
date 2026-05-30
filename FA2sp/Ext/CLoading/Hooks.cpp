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
#include "../CIsoView/DirectXCore.h"
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

	FString fileName = Filename;
	if (!CLoadingExt::NotFoundFiles.contains(fileName))
	{
		if (ExtConfigs::ExtMixLoader)
		{
			auto& manager = MixLoader::Instance();
			auto result = manager.QueryFileIndex(fileName);
			if (result >= 0)
			{
#ifndef NDEBUG
				Logger::Debug("[SearchFile] %s - %d\n", fileName, result);
#endif
				R->EAX(result);
				return 0x48AA63;
			}
		}
		else
		{
			for (int i = 0; i < CMixFile::ArraySize; ++i)
			{
				auto& mix = CMixFile::Array[i];
				if (!mix.is_open())
					break;
				if (CMixFile::HasFile(fileName, i + 1))
				{
#ifndef NDEBUG
					Logger::Debug("[SearchFile] %s - %d\n", fileName, i + 1);
#endif
					R->EAX(i + 1);
					return 0x48AA63;
				}
			}
		}
		CLoadingExt::NotFoundFiles.insert(fileName);
	}

#ifndef NDEBUG
	Logger::Debug("[SearchFile] %s - NOT FOUND\n", fileName);
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
	auto loadInternalorExternalBitmap = [](const char* imageID, int resource, const char* newFile = nullptr)
		{
			std::string pic = CFinalSunAppExt::ExePathExt;
			pic += "\\pics\\";
			pic += newFile;
			if (fs::exists(pic))
			{
				CBitmap bmp;
				if (CLoadingExt::LoadBMPToCBitmap(pic, bmp))
					CLoadingExt::LoadBitMap(imageID, bmp);
			}
			else
			{
				HBITMAP hBmp = (HBITMAP)LoadImage(static_cast<HINSTANCE>(FA2sp::hInstance), MAKEINTRESOURCE(resource),
					IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
				CBitmap cBitmap;
				cBitmap.Attach(hBmp);
				CLoadingExt::LoadBitMap(imageID, cBitmap);
			}
		};
	loadInternalorExternalBitmap("ANNOTATION", 1001, "annotation.bmp");
	loadInternalorExternalBitmap("FLAG", 1023, "waypoint.bmp");
	loadInternalorExternalBitmap("CELLTAG", 1024, "celltag.bmp");
	loadInternalorExternalBitmap("PROPERTY_MARK", 1036, "property_mark.bmp");

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
					{
						if (!ExtConfigs::DirectXRendering && entry.path().filename().string() == "scrollcursor.bmp")
						{
							BITMAP bm = {};
							bmp.GetBitmap(&bm);
							int maxSize = std::max(bm.bmWidth, bm.bmHeight);
							CIsoViewExt::ScaleBitmap(&bmp, maxSize * CFinalSunAppExt::ProgramScaleFactor, RGB(255, 255, 255), true, false);
						}
						CLoadingExt::LoadBitMap(entry.path().filename().string(), bmp);
					}
				}
			}
		}
	}

	return 0;
}

static bool hasExtraImage = false;
static int width, height;
static int tileIndex = -1;
static int subTileIndex = -1;
static byte altCount[256];
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

		CMapDataExt::TileBlockExtraOffsets[{tileIndex, subTileIndex, altCount[subTileIndex]}]
			= std::make_pair( POINT{ currentTMP->x_extra - currentTMP->x, currentTMP->y_extra - currentTMP->y },
				POINT{ currentTMP->cx_extra, currentTMP->cy_extra }
		);

		auto loadingExt = (CLoadingExt*)CLoading::Instance();
		FString ImageID;
		ImageID.Format("EXTRAIMAGE\233%d\233%d\233%d", tileIndex, subTileIndex, altCount[subTileIndex]);
		loadingExt->SetImageDataSafe(extra_image, ImageID, currentTMP->cx_extra, currentTMP->cy_extra, &CMapDataExt::Palette_ISO, false);
		CLoadingExt::LoadedObjects.insert(ImageID);
		altCount[subTileIndex]++;
	}
	return 0;
}

DEFINE_HOOK(48C3D0, CLoading_InitTMPs_InitTileData, 7)
{
	CMapDataExt::InitializeTileDataInfo();
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