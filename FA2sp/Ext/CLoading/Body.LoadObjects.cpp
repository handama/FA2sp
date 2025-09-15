#include "Body.h"

#include <CINI.h>
#include <CMapData.h>
#include <CMixFile.h>
#include <iostream>
#include <fstream>
#include "../../Miscs/VoxelDrawer.h"
#include "../../Miscs/Palettes.h"
#include "../../FA2sp.h"
#include "../../Algorithms/Matrix3D.h"
#include "../CMapData/Body.h"
#include "../CFinalSunDlg/Body.h"

std::vector<CLoadingExt::SHPUnionData> CLoadingExt::UnionSHP_Data[2];
std::vector<CLoadingExt::SHPUnionData> CLoadingExt::UnionSHPShadow_Data[2];
std::unordered_map<FString, CLoadingExt::ObjectType> CLoadingExt::ObjectTypes;
std::unordered_set<FString> CLoadingExt::LoadedObjects;
std::unordered_map<FString, int> CLoadingExt::AvailableFacings;
std::unordered_set<int> CLoadingExt::Ra2dotMixes;
unsigned char CLoadingExt::VXL_Data[0x10000] = {0};
unsigned char CLoadingExt::VXL_Shadow_Data[0x10000] = {0};
std::unordered_set<FString> CLoadingExt::LoadedOverlays;
int CLoadingExt::TallestBuildingHeight = 0;

std::unordered_map<FString, std::unique_ptr<ImageDataClassSafe>> CLoadingExt::CurrentFrameImageDataMap;
std::unordered_map<FString, std::unique_ptr<ImageDataClassSafe>> CLoadingExt::ImageDataMap;
std::unordered_map<FString, std::unique_ptr<ImageDataClassSurface>> CLoadingExt::SurfaceImageDataMap;

bool CLoadingExt::IsImageLoaded(const FString& name)
{
	auto itr = ImageDataMap.find(name);
	if (itr == ImageDataMap.end())
		return false;
	return itr->second->pImageBuffer != nullptr;
}

ImageDataClassSafe* CLoadingExt::GetImageDataFromMap(const FString& name)
{
	auto itr = ImageDataMap.find(name);
	if (itr == ImageDataMap.end())
	{
		auto ret = std::make_unique<ImageDataClassSafe>();
		auto [it, inserted] = CLoadingExt::ImageDataMap.emplace(name, std::move(ret));
		return it->second.get();
	}
	return itr->second.get();
}

ImageDataClassSafe* CLoadingExt::GetImageDataFromServer(const FString& name)
{
	if (ExtConfigs::LoadImageDataFromServer)
	{
		auto itr = CurrentFrameImageDataMap.find(name);
		if (itr == CurrentFrameImageDataMap.end())
		{
			auto ret = std::make_unique<ImageDataClassSafe>();
			RequestImageFromServer(name, *ret);
			auto [it, inserted] = CurrentFrameImageDataMap.emplace(name, std::move(ret));
			return it->second.get();
		}
		return itr->second.get();
	}
	return GetImageDataFromMap(name);
}

bool CLoadingExt::IsSurfaceImageLoaded(const FString& name)
{
	auto itr = SurfaceImageDataMap.find(name);
	if (itr == SurfaceImageDataMap.end())
		return false;
	return itr->second->lpSurface != nullptr;
}

ImageDataClassSurface* CLoadingExt::GetSurfaceImageDataFromMap(const FString& name)
{
	auto itr = SurfaceImageDataMap.find(name);
	if (itr == SurfaceImageDataMap.end())
	{
		auto ret = std::make_unique<ImageDataClassSurface>();
		auto [it, inserted] = CLoadingExt::SurfaceImageDataMap.emplace(name, std::move(ret));
		return it->second.get();
	}
	return itr->second.get();
}

int CLoadingExt::GetAvailableFacing(const FString& ID)
{
	auto itr = AvailableFacings.find(ID);
	if (itr == AvailableFacings.end())
		return 8;
	return itr->second;
}
FString CLoadingExt::GetImageName(FString ID, int nFacing, bool bShadow, bool bDeploy, bool bWater)
{
	FString ret;
	if (bShadow || bDeploy || bWater)
		ret.Format("%s%d\233%s%s%s", ID, nFacing, bDeploy ? "DEPLOY" : "", bWater ? "WATER" : "", bShadow ? "SHADOW" : "");
	else
		ret.Format("%s%d", ID, nFacing);
	return ret;
}

FString CLoadingExt::GetOverlayName(WORD ovr, BYTE ovrd, bool bShadow)
{
	FString ret;
	if (bShadow)
		ret.Format("OVRL\233%d_%dSHADOW", ovr, ovrd);
	else
		ret.Format("OVRL\233%d_%d", ovr, ovrd);
	return ret;
}

FString CLoadingExt::GetBuildingImageName(FString ID, int nFacing, int state, bool bShadow)
{
	FString ret;
	if (state == GBIN_DAMAGED)
	{
		if (bShadow)
			ret.Format("%s%d\233DAMAGEDSHADOW", ID, nFacing);
		else
			ret.Format("%s%d\233DAMAGED", ID, nFacing);
	}
	else if (state == GBIN_RUBBLE)
	{
		if (bShadow)
		{
			if (Variables::RulesMap.GetBool(ID, "LeaveRubble"))
				ret.Format("%s0\233RUBBLESHADOW", ID);
			else if (!ExtConfigs::HideNoRubbleBuilding)// use damaged art, save memory
				ret.Format("%s%d\233DAMAGEDSHADOW", ID, nFacing);
			else // hide rubble
				ret = "\233\144\241"; // invalid string to get it empty
		}
		else
		{
			if (Variables::RulesMap.GetBool(ID, "LeaveRubble"))
				ret.Format("%s0\233RUBBLE", ID);
			else if (!ExtConfigs::HideNoRubbleBuilding)// use damaged art, save memory
				ret.Format("%s%d\233DAMAGED", ID, nFacing);
			else // hide rubble
				ret = "\233\144\241"; // invalid string to get it empty
		}

	}
	else // GBIN_NORMAL
	{
		if (bShadow)
			ret.Format("%s%d\233SHADOW", ID, nFacing);
		else
			ret.Format("%s%d", ID, nFacing);
	}
	return ret;
}

CLoadingExt::ObjectType CLoadingExt::GetItemType(FString ID)
{
	if (ID == "")
		ObjectType::Unknown;
	if (ObjectTypes.size() == 0)
	{
		auto load = [](FString type, ObjectType e)
		{
			auto section = Variables::RulesMap.GetSection(type);
			for (auto& pair : section)
				ObjectTypes[pair.second] = e;
		};

		load("InfantryTypes", ObjectType::Infantry);
		load("VehicleTypes", ObjectType::Vehicle);
		load("AircraftTypes", ObjectType::Aircraft);
		load("BuildingTypes", ObjectType::Building);
		load("SmudgeTypes", ObjectType::Smudge);
		load("TerrainTypes", ObjectType::Terrain);
	}

	auto itr = ObjectTypes.find(ID);
	if (itr != ObjectTypes.end())
		return itr->second;
	return ObjectType::Unknown;
}

void CLoadingExt::LoadObjects(FString ID)
{
	if (ID == "")
		return;

    Logger::Debug("CLoadingExt::LoadObjects loading: %s\n", ID);
	if (!IsLoadingObjectView)
		LoadedObjects.insert(ID);

	auto eItemType = GetItemType(ID);
	switch (eItemType)
	{
	case CLoadingExt::ObjectType::Infantry:
		LoadInfantry(ID);
		break;
	case CLoadingExt::ObjectType::Terrain:
		LoadTerrainOrSmudge(ID, true);
		break;
	case CLoadingExt::ObjectType::Smudge:
		LoadTerrainOrSmudge(ID, false);
		break;
	case CLoadingExt::ObjectType::Vehicle:
	{
		LoadVehicleOrAircraft(ID);
		if (ExtConfigs::InGameDisplay_Deploy)
		{
			auto unloadingClass = Variables::RulesMap.GetString(ID, "UnloadingClass", ID);
			if (unloadingClass != ID)
			{
				LoadVehicleOrAircraft(unloadingClass);
			}
		}
		if (ExtConfigs::InGameDisplay_Water)
		{
			auto waterImage = Variables::RulesMap.GetString(ID, "WaterImage", ID);
			if (waterImage != ID)
			{
				LoadVehicleOrAircraft(waterImage);
			}
		}
		break;
	}
	case CLoadingExt::ObjectType::Aircraft:
		LoadVehicleOrAircraft(ID);
		break;
	case CLoadingExt::ObjectType::Building:
		LoadBuilding(ID);
		break;
	case CLoadingExt::ObjectType::Unknown:
	default:
		break;
	}
}

void CLoadingExt::ClearItemTypes()
{
	ObjectTypes.clear();
	LoadedObjects.clear();
	LoadedOverlays.clear();
	SwimableInfantries.clear();
	ImageDataMap.clear();
	AvailableFacings.clear();
	CMapDataExt::TerrainPaletteBuildings.clear();
	CMapDataExt::DamagedAsRubbleBuildings.clear();
	for (auto& data : SurfaceImageDataMap)
	{
		if (data.second->lpSurface)
		{
			data.second->lpSurface->Release();
		}
	}
	SurfaceImageDataMap.clear();
	Logger::Debug("CLoadingExt::Clearing Loaded Objects.\n");
	if (ExtConfigs::LoadImageDataFromServer)
	{
		CLoadingExt::SendRequestText("CLEAR_MAP");
	}
}

bool CLoadingExt::IsObjectLoaded(FString pRegName)
{
	return LoadedObjects.find(pRegName) != LoadedObjects.end();
}

bool CLoadingExt::IsOverlayLoaded(FString pRegName)
{
	return LoadedOverlays.find(pRegName) != LoadedOverlays.end();
}

FString CLoadingExt::GetTerrainOrSmudgeFileID(FString ID)
{
	FString ArtID = GetArtID(ID);
	FString ImageID = CINI::Art->GetString(ArtID, "Image", ArtID);

	return ImageID;
}

FString CLoadingExt::GetBuildingFileID(FString ID)
{
	FString ArtID = GetArtID(ID);
	FString ImageID = CINI::Art->GetString(ArtID, "Image", ArtID);

	FString backupID = ImageID;
	SetTheaterLetter(ImageID, ExtConfigs::NewTheaterType ? 1 : 0);

	FString validator = ImageID + ".SHP";
	int nMix = this->SearchFile(validator);
	if (!CLoading::HasFile(validator, nMix))
	{
		SetGenericTheaterLetter(ImageID);
		if (!ExtConfigs::UseStrictNewTheater)
		{
			validator = ImageID + ".SHP";
			nMix = this->SearchFile(validator);
			if (!CLoading::HasFile(validator, nMix))
				ImageID = backupID;
		}
	}
	return ImageID;
}

FString CLoadingExt::GetInfantryFileID(FString ID)
{
	FString ArtID = GetArtID(ID);

	FString ImageID = ArtID;

	if (ExtConfigs::ArtImageSwap)
		ImageID = CINI::Art->GetString(ArtID, "Image", ArtID);

	if (Variables::RulesMap.GetBool(ID, "AlternateTheaterArt"))
		ImageID += this->TheaterIdentifier;
	else if (Variables::RulesMap.GetBool(ID, "AlternateArcticArt"))
		if (this->TheaterIdentifier == 'A')
			ImageID += 'A';
	if (!CINI::Art->SectionExists(ImageID))
		ImageID = ArtID;

	return ImageID;
}

FString CLoadingExt::GetArtID(FString ID)
{
	return Variables::RulesMap.GetString(ID, "Image", ID);
}

FString CLoadingExt::GetVehicleOrAircraftFileID(FString ID)
{
	FString ArtID = GetArtID(ID);

	FString ImageID = ArtID;

	if (ExtConfigs::ArtImageSwap)
		ImageID = CINI::Art->GetString(ArtID, "Image", ArtID);

	return ImageID;
}

void CLoadingExt::LoadBuilding(FString ID)
{
	if (auto ppPowerUpBld = Variables::RulesMap.TryGetString(ID, "PowersUpBuilding")) // Early load
	{
		if (!CLoadingExt::IsObjectLoaded(*ppPowerUpBld))
			LoadBuilding(*ppPowerUpBld);
	}

	LoadBuilding_Normal(ID);
	if (IsLoadingObjectView)
		return;
	LoadBuilding_Damaged(ID);
	LoadBuilding_Rubble(ID);

	if (ExtConfigs::InGameDisplay_AlphaImage)
	{
		if (auto pAIFile = Variables::RulesMap.TryGetString(ID, "AlphaImage"))
		{
			auto AIDicName = *pAIFile + "\233ALPHAIMAGE";
			if (!CLoadingExt::IsObjectLoaded(AIDicName))
				LoadShp(AIDicName, *pAIFile + ".shp", "anim.pal", 0);
		}
	}
}

void CLoadingExt::LoadBuilding_Normal(FString ID)
{
	FString ArtID = GetArtID(ID);
	FString ImageID = GetBuildingFileID(ID);
	FString CurrentLoadingAnim;
	bool bHasShadow = !Variables::RulesMap.GetBool(ID, "NoShadow");
	int facings = ExtConfigs::ExtFacings ? 32 : 8;
	AvailableFacings[ID] = facings;

	FString PaletteName = CINI::Art->GetString(ArtID, "Palette", "unit");
	if (CINI::Art->GetBool(ArtID, "TerrainPalette"))
	{
		PaletteName = "iso\233NotAutoTinted";
		CMapDataExt::TerrainPaletteBuildings.insert(ID);
	}
	GetFullPaletteName(PaletteName);
	auto palette = PalettesManager::LoadPalette(PaletteName);

	auto loadBuildingFrameShape = [&](FString name, int nFrame = 0, int deltaX = 0, int deltaY = 0, bool shadow = false) -> bool
	{
		FString file = name + ".SHP";
		int nMix = SearchFile(file);
		// building can be displayed without the body
		if (!CLoading::HasFile(file, nMix))
			return true;

		ShapeHeader header;
		unsigned char* pBuffer;
		CMixFile::LoadSHP(file, nMix);
		CShpFile::GetSHPHeader(&header);
		if (header.FrameCount / 2 <= nFrame) {
			nFrame = 0;
		}
		CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &pBuffer, header);
		UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, false, 0, 0, true);

		if (shadow && ExtConfigs::InGameDisplay_Shadow)
		{
			CLoadingExt::LoadSHPFrameSafe(nFrame + header.FrameCount / 2, 1, &pBuffer, header);
			UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, true);
		}

		return true;
	};

	auto loadSingleFrameShape = [&](FString name, int nFrame = 0, int deltaX = 0, 
		int deltaY = 0, FString customPal = "", bool shadow = false, int forceNewTheater = -1) -> bool
	{
			bool applyNewTheater = CINI::Art->GetBool(name, "NewTheater");
			name = CINI::Art->GetString(name, "Image", name);
			applyNewTheater = CINI::Art->GetBool(name, "NewTheater", applyNewTheater);

			FString file = name + ".SHP";
			int nMix = SearchFile(file);
			int loadedMix = CLoadingExt::HasFileMix(file, nMix);
			// if anim file in RA2(MD).mix, always use NewTheater = yes
			if (Ra2dotMixes.find(loadedMix) != Ra2dotMixes.end())
			{
				applyNewTheater = true;
			}

			if (applyNewTheater || forceNewTheater == 1)
				SetTheaterLetter(file, ExtConfigs::NewTheaterType ? 1 : 0);
			nMix = SearchFile(file);
			if (!CLoading::HasFile(file, nMix))
			{
				SetGenericTheaterLetter(file);
				nMix = SearchFile(file);
				if (!CLoading::HasFile(file, nMix))
				{
					if (!ExtConfigs::UseStrictNewTheater)
					{
						auto searchNewTheater = [&nMix, this, &file](char t)
							{
								if (file.GetLength() >= 2)
									file.SetAt(1, t);
								nMix = SearchFile(file);
								return HasFile(file, nMix);
							};
						file = name + ".SHP";
						nMix = SearchFile(file);
						if (!CLoading::HasFile(file, nMix))
							if (!searchNewTheater('T'))
								if (!searchNewTheater('A'))
									if (!searchNewTheater('U'))
										if (!searchNewTheater('N'))
											if (!searchNewTheater('L'))
												if (!searchNewTheater('D'))
													return false;
					}
					else
					{
						return false;
					}
				}
			}

		ShapeHeader header;
		unsigned char* pBuffer;
		CMixFile::LoadSHP(file, nMix);
		CShpFile::GetSHPHeader(&header);
		if (header.FrameCount <= nFrame) {
			nFrame = 0;
		}
		CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &pBuffer, header);
		if (customPal != "")
		{
			if (auto thisPal = PalettesManager::LoadPalette(customPal))
			{
				std::vector<int> lookupTable = GeneratePalLookupTable(thisPal, palette);
				int counter = 0;
				for (int j = 0; j < header.Height; ++j)
				{
					for (int i = 0; i < header.Width; ++i)
					{
						unsigned char& ch = pBuffer[counter];
						ch = lookupTable[ch];
						counter++;
					}
				}
			}
		}

		UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, false,
			CINI::Art->GetInteger(ArtID, CurrentLoadingAnim + "ZAdjust"),
			CINI::Art->GetInteger(ArtID, CurrentLoadingAnim + "YSort"));

		if (shadow && ExtConfigs::InGameDisplay_Shadow)
		{
			CLoadingExt::LoadSHPFrameSafe(nFrame + header.FrameCount / 2, 1, &pBuffer, header);
			UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, true);
		}

		return true;
	};

	auto loadAnimFrameShape = [&](FString animkey, FString ignorekey)
	{
		CurrentLoadingAnim = animkey;
		if (auto pStr = CINI::Art->TryGetString(ArtID, animkey))
		{
			if (!CINI::FAData->GetBool(ignorekey, ID))
			{
				int nStartFrame = CINI::Art->GetInteger(*pStr, "LoopStart");
				FString customPal = "";
				if (!CINI::Art->GetBool(*pStr, "ShouldUseCellDrawer", true)) {
					customPal = CINI::Art->GetString(*pStr, "CustomPalette", "anim.pal");
					customPal.Replace("~~~", GetTheaterSuffix());
				}
				int deltaX = CINI::Art->GetInteger(*pStr, "XDrawOffset");
				int deltaY = CINI::Art->GetInteger(*pStr, "YDrawOffset");
				if (animkey.Find("ActiveAnim") != -1 || animkey == "IdleAnim")
				{
					deltaX += CINI::Art->GetInteger(ArtID, animkey + "X");
					deltaY += CINI::Art->GetInteger(ArtID, animkey + "Y");
				}

				loadSingleFrameShape(*pStr, nStartFrame, deltaX, deltaY, customPal, CINI::Art->GetBool(*pStr, "Shadow"));
			}
		}
	};

	if (auto ppPowerUpBld = Variables::RulesMap.TryGetString(ID, "PowersUpBuilding")) // Early load
	{
		if (!CLoadingExt::IsObjectLoaded(*ppPowerUpBld))
			LoadBuilding(*ppPowerUpBld);
	}

	int nBldStartFrame = CINI::Art->GetInteger(ArtID, "LoopStart", 0);

	FString AnimKeys[9] = 
	{	
		"IdleAnim",
		"ActiveAnim",
		"ActiveAnimTwo",
		"ActiveAnimThree",
		"ActiveAnimFour",
		"SuperAnim",
		"SuperAnimTwo",
		"SuperAnimThree",
		"SuperAnimFour" 
	};
	FString IgnoreKeys[9] =
	{
		"IgnoreIdleAnim",
		"IgnoreActiveAnim1",
		"IgnoreActiveAnim2",
		"IgnoreActiveAnim3",
		"IgnoreActiveAnim4",
		"IgnoreSuperAnim1",
		"IgnoreSuperAnim2",
		"IgnoreSuperAnim3",
		"IgnoreSuperAnim4"
	};
	
	loadBuildingFrameShape(ImageID, nBldStartFrame, 0, 0, bHasShadow && CINI::Art->GetBool(ArtID, "Shadow", true));
	for (int i = 0; i < 9; ++i)
	{
		loadAnimFrameShape(AnimKeys[i], IgnoreKeys[i]);
	}
	if (auto pStr = CINI::Art->TryGetString(ArtID, "BibShape")) {
		loadSingleFrameShape(*pStr, 0, 0, 0, "", false, 1);
	}

	FString DictName;

	unsigned char* pBuffer;
	int width, height;
	UnionSHP_GetAndClear(pBuffer, &width, &height, false, false, true);

	FString DictNameShadow;
	unsigned char* pBufferShadow{ 0 };
	int widthShadow, heightShadow;
	if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
		UnionSHP_GetAndClear(pBufferShadow, &widthShadow, &heightShadow, false, true);

	if (Variables::RulesMap.GetBool(ID, "Turret")) // Has turret
	{
		if (Variables::RulesMap.GetBool(ID, "TurretAnimIsVoxel"))
		{
			FString TurName = Variables::RulesMap.GetString(ID, "TurretAnim", ID + "tur");
			FString BarlName = ID + "barl";


			if (!VoxelDrawer::IsVPLLoaded())
				VoxelDrawer::LoadVPLFile("voxels.vpl");

			std::vector<unsigned char*> pTurImages, pBarlImages;
			pTurImages.resize(facings, nullptr);
			pBarlImages.resize(facings, nullptr);
			std::vector<VoxelRectangle> turrect, barlrect;
			turrect.resize(facings);
			barlrect.resize(facings);

			FString VXLName = BarlName + ".vxl";
			FString HVAName = BarlName + ".hva";
			if (VoxelDrawer::LoadVXLFile(VXLName))
			{
				if (VoxelDrawer::LoadHVAFile(HVAName))
				{
					for (int i = 0; i < facings; ++i)
					{
						bool result = VoxelDrawer::GetImageData((facings + 5 * facings / 8 - i) % facings, pBarlImages[i], barlrect[i]);
						if (!result)
							break;
					}
				}
			}

			VXLName = TurName + ".vxl";
			HVAName = TurName + ".hva";
			if (VoxelDrawer::LoadVXLFile(VXLName))
			{
				if (VoxelDrawer::LoadHVAFile(HVAName))
				{
					for (int i = 0; i < facings; ++i)
					{
						bool result = VoxelDrawer::GetImageData((facings + 5 * facings / 8 - i) % facings, pTurImages[i], turrect[i]);
						if (!result)
							break;
					}
				}
			}

			for (int i = 0; i < facings; ++i)
			{
				if (IsLoadingObjectView && i != 0)
					continue;
				auto pTempBuf = GameCreateArray<unsigned char>(width * height);
				memcpy_s(pTempBuf, width * height, pBuffer, width * height);
				UnionSHP_Add(pTempBuf, width, height);

				int deltaX = Variables::RulesMap.GetInteger(ID, "TurretAnimX", 0);
				int deltaY = Variables::RulesMap.GetInteger(ID, "TurretAnimY", 0);

				if (pTurImages[i])
				{
					FString pKey;

					pKey.Format("%sX%d", ID, (15 - i * 8 / facings) % 8);
					int turdeltaX = CINI::FAData->GetInteger("BuildingVoxelTurretsRA2", pKey);
					pKey.Format("%sY%d", ID, (15 - i * 8 / facings) % 8);
					int turdeltaY = CINI::FAData->GetInteger("BuildingVoxelTurretsRA2", pKey);

					VXL_Add(pTurImages[i], turrect[i].X + turdeltaX, turrect[i].Y + turdeltaY, turrect[i].W, turrect[i].H);
					CncImgFree(pTurImages[i]);

					if (pBarlImages[i])
					{
						pKey.Format("%sX%d", ID, (15 - i * 8 / facings) % 8);
						int barldeltaX = CINI::FAData->GetInteger("BuildingVoxelBarrelsRA2", pKey);
						pKey.Format("%sY%d", ID, (15 - i * 8 / facings) % 8);
						int barldeltaY = CINI::FAData->GetInteger("BuildingVoxelBarrelsRA2", pKey);

						VXL_Add(pBarlImages[i], barlrect[i].X + barldeltaX, barlrect[i].Y + barldeltaY, barlrect[i].W, barlrect[i].H);
						CncImgFree(pBarlImages[i]);
					}
				}

				int nW = 0x100, nH = 0x100;
				VXL_GetAndClear(pTurImages[i], &nW, &nH);

				UnionSHP_Add(pTurImages[i], 0x100, 0x100, deltaX, deltaY);

				unsigned char* pImage;
				int width1, height1;

				UnionSHP_GetAndClear(pImage, &width1, &height1);
				DictName.Format("%s%d", ID, i);
				SetImageDataSafe(pImage, DictName, width1, height1, palette);
			}

			if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
			{
				DictNameShadow.Format("%s%d\233SHADOW", ID, 0);
				SetImageDataSafe(pBufferShadow, DictNameShadow, widthShadow, heightShadow, &CMapDataExt::Palette_Shadow);
			}

			GameDeleteArray(pBuffer, width * height);
		}
		else //SHP anim
		{
			FString TurName = Variables::RulesMap.GetString(ID, "TurretAnim", ID + "tur");
			int nStartFrame = CINI::Art->GetInteger(TurName, "LoopStart");
			bool shadow = bHasShadow && CINI::Art->GetBool(TurName, "Shadow", true) && ExtConfigs::InGameDisplay_Shadow;
			for (int i = 0; i < facings; ++i)
			{
				if (IsLoadingObjectView && i != 0)
					continue;
				auto pTempBuf = GameCreateArray<unsigned char>(width * height);
				memcpy_s(pTempBuf, width * height, pBuffer, width * height);
				UnionSHP_Add(pTempBuf, width, height);

				if (shadow)
				{
					auto pTempBufShadow = GameCreateArray<unsigned char>(width * height);
					memcpy_s(pTempBufShadow, width * height, pBufferShadow, width * height);
					UnionSHP_Add(pTempBufShadow, width, height, 0, 0, false, true);
				}

				int deltaX = Variables::RulesMap.GetInteger(ID, "TurretAnimX", 0);
				int deltaY = Variables::RulesMap.GetInteger(ID, "TurretAnimY", 0);
				loadSingleFrameShape(CINI::Art->GetString(TurName, "Image", TurName),
					nStartFrame + i * 32 / facings, deltaX, deltaY, "", shadow);

				unsigned char* pImage;
				int width1, height1;
				UnionSHP_GetAndClear(pImage, &width1, &height1);

				DictName.Format("%s%d", ID, i);
				SetImageDataSafe(pImage, DictName, width1, height1, palette);

				if (shadow)
				{
					FString DictNameShadow;
					unsigned char* pImageShadow;
					int width1Shadow, height1Shadow;
					UnionSHP_GetAndClear(pImageShadow, &width1Shadow, &height1Shadow, false, true);
					DictNameShadow.Format("%s%d\233SHADOW", ID, i);
					SetImageDataSafe(pImageShadow, DictNameShadow, width1Shadow, height1Shadow, &CMapDataExt::Palette_Shadow);
				}
			}
			GameDelete(pBuffer);
			GameDelete(pBufferShadow);
		}
	}
	else // No turret
	{
		DictName.Format("%s%d", ID, 0);
		SetImageDataSafe(pBuffer, DictName, width, height, palette);
		if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
		{
			DictNameShadow.Format("%s%d\233SHADOW", ID, 0);
			SetImageDataSafe(pBufferShadow, DictNameShadow, widthShadow, heightShadow, &CMapDataExt::Palette_Shadow);
		}
	}
}

void CLoadingExt::LoadBuilding_Damaged(FString ID, bool loadAsRubble)
{
	FString ArtID = GetArtID(ID);
	FString ImageID = GetBuildingFileID(ID);
	FString CurrentLoadingAnim;
	bool bHasShadow = !Variables::RulesMap.GetBool(ID, "NoShadow");
	int facings = ExtConfigs::ExtFacings ? 32 : 8;
	AvailableFacings[ID] = facings;

	FString PaletteName = CINI::Art->GetString(ArtID, "Palette", "unit");
	if (CINI::Art->GetBool(ArtID, "TerrainPalette"))
	{
		PaletteName = "iso\233NotAutoTinted";
		CMapDataExt::TerrainPaletteBuildings.insert(ID);
	}
	GetFullPaletteName(PaletteName);
	auto palette = PalettesManager::LoadPalette(PaletteName);

	auto loadBuildingFrameShape = [&](FString name, int nFrame = 0, int deltaX = 0, int deltaY = 0, bool shadow = false) -> bool
	{
		FString file = name + ".SHP";
		int nMix = SearchFile(file);
		// building can be displayed without the body
		if (!CLoading::HasFile(file, nMix))
			return true;

		ShapeHeader header;
		unsigned char* pBuffer;
		CMixFile::LoadSHP(file, nMix);
		CShpFile::GetSHPHeader(&header);
		if (header.FrameCount / 2 <= nFrame) {
			nFrame = 0;
		}
		CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &pBuffer, header);

		UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, false, 0, 0, true);

		if (shadow && ExtConfigs::InGameDisplay_Shadow)
		{
			CLoadingExt::LoadSHPFrameSafe(nFrame + header.FrameCount / 2, 1, &pBuffer, header);
			UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, true);
		}

		return true;
	};

	auto loadSingleFrameShape = [&](FString name, int nFrame = 0, int deltaX = 0,
		int deltaY = 0, FString customPal = "", bool shadow = false, int forceNewTheater = -1) -> bool
	{
			bool applyNewTheater = CINI::Art->GetBool(name, "NewTheater");
			name = CINI::Art->GetString(name, "Image", name);
			applyNewTheater = CINI::Art->GetBool(name, "NewTheater", applyNewTheater);

			FString file = name + ".SHP";
			int nMix = SearchFile(file);
			int loadedMix = CLoadingExt::HasFileMix(file, nMix);
			// if anim file in RA2(MD).mix, always use NewTheater = yes
			if (Ra2dotMixes.find(loadedMix) != Ra2dotMixes.end())
			{
				applyNewTheater = true;
			}

			if (applyNewTheater || forceNewTheater == 1)
				SetTheaterLetter(file, ExtConfigs::NewTheaterType ? 1 : 0);
			nMix = SearchFile(file);
			if (!CLoading::HasFile(file, nMix))
			{
				SetGenericTheaterLetter(file);
				nMix = SearchFile(file);
				if (!CLoading::HasFile(file, nMix))
				{
					if (!ExtConfigs::UseStrictNewTheater)
					{
						auto searchNewTheater = [&nMix, this, &file](char t)
							{
								if (file.GetLength() >= 2)
									file.SetAt(1, t);
								nMix = SearchFile(file);
								return HasFile(file, nMix);
							};
						file = name + ".SHP";
						nMix = SearchFile(file);
						if (!CLoading::HasFile(file, nMix))
							if (!searchNewTheater('T'))
								if (!searchNewTheater('A'))
									if (!searchNewTheater('U'))
										if (!searchNewTheater('N'))
											if (!searchNewTheater('L'))
												if (!searchNewTheater('D'))
													return false;
					}
					else
					{
						return false;
					}
				}
			}

		ShapeHeader header;
		unsigned char* pBuffer;
		CMixFile::LoadSHP(file, nMix);
		CShpFile::GetSHPHeader(&header);
		if (header.FrameCount <= nFrame) {
			nFrame = 0;
		}
		CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &pBuffer, header);
		if (customPal != "")
		{
			if (auto thisPal = PalettesManager::LoadPalette(customPal))
			{
				std::vector<int> lookupTable = GeneratePalLookupTable(thisPal, palette);
				int counter = 0;
				for (int j = 0; j < header.Height; ++j)
				{
					for (int i = 0; i < header.Width; ++i)
					{
						unsigned char& ch = pBuffer[counter];
						ch = lookupTable[ch];
						counter++;
					}
				}
			}
		}


		UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, false,
			CINI::Art->GetInteger(ArtID, CurrentLoadingAnim + "ZAdjust"),
			CINI::Art->GetInteger(ArtID, CurrentLoadingAnim + "YSort"));

		if (shadow && ExtConfigs::InGameDisplay_Shadow)
		{
			CLoadingExt::LoadSHPFrameSafe(nFrame + header.FrameCount / 2, 1, &pBuffer, header);
			UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, true);
		}

		return true;
	};

	auto loadAnimFrameShape = [&](FString animkey, FString ignorekey)
	{
		FString damagedAnimkey = animkey + "Damaged";
		CurrentLoadingAnim = animkey;
		if (auto pStr = CINI::Art->TryGetString(ArtID, damagedAnimkey))
		{
			if (!CINI::FAData->GetBool(ignorekey, ID))
			{
				int nStartFrame = CINI::Art->GetInteger(*pStr, "LoopStart");
				FString customPal = "";
				if (!CINI::Art->GetBool(*pStr, "ShouldUseCellDrawer", true)) {
					customPal = CINI::Art->GetString(*pStr, "CustomPalette", "anim.pal");
					customPal.Replace("~~~", GetTheaterSuffix());
				}
				int deltaX = CINI::Art->GetInteger(*pStr, "XDrawOffset");
				int deltaY = CINI::Art->GetInteger(*pStr, "YDrawOffset");
				if (animkey.Find("ActiveAnim") != -1 || animkey == "IdleAnim")
				{
					deltaX += CINI::Art->GetInteger(ArtID, animkey + "X");
					deltaY += CINI::Art->GetInteger(ArtID, animkey + "Y");
				}

				loadSingleFrameShape(*pStr, nStartFrame, deltaX, deltaY, customPal, CINI::Art->GetBool(*pStr, "Shadow"));
			}
		}
		else if (auto pStr = CINI::Art->TryGetString(ArtID, animkey))
		{
			if (!CINI::FAData->GetBool(ignorekey, ID))
			{
				int nStartFrame = CINI::Art->GetInteger(*pStr, "LoopStart");
				FString customPal = "";
				if (!CINI::Art->GetBool(*pStr, "ShouldUseCellDrawer", true)) {
					customPal = CINI::Art->GetString(*pStr, "CustomPalette", "anim.pal");
					customPal.Replace("~~~", GetTheaterSuffix());
				}
				loadSingleFrameShape(*pStr, nStartFrame, 0, 0, customPal);
			}
		}
	};

	int nBldStartFrame = CINI::Art->GetInteger(ArtID, "LoopStart", 0) + 1;
	if (Variables::RulesMap.GetBool(ID, "Wall"))
		nBldStartFrame--;

	FString AnimKeys[9] =
	{
		"IdleAnim",
		"ActiveAnim",
		"ActiveAnimTwo",
		"ActiveAnimThree",
		"ActiveAnimFour",
		"SuperAnim",
		"SuperAnimTwo",
		"SuperAnimThree",
		"SuperAnimFour"
	};
	FString IgnoreKeys[9] =
	{
		"IgnoreIdleAnim",
		"IgnoreActiveAnim1",
		"IgnoreActiveAnim2",
		"IgnoreActiveAnim3",
		"IgnoreActiveAnim4",
		"IgnoreSuperAnim1",
		"IgnoreSuperAnim2",
		"IgnoreSuperAnim3",
		"IgnoreSuperAnim4"
	};
	loadBuildingFrameShape(ImageID, nBldStartFrame, 0, 0, bHasShadow&& CINI::Art->GetBool(ArtID, "Shadow", true));
	for (int i = 0; i < 9; ++i)
	{
		loadAnimFrameShape(AnimKeys[i], IgnoreKeys[i]);
	}
	if (auto pStr = CINI::Art->TryGetString(ArtID, "BibShape")) {
		loadSingleFrameShape(*pStr, 1, 0, 0, "", false, 1);
	}

	FString DictName;

	unsigned char* pBuffer;
	int width, height;
	UnionSHP_GetAndClear(pBuffer, &width, &height, false, false, true);

	FString DictNameShadow;
	unsigned char* pBufferShadow{ 0 };
	int widthShadow, heightShadow;
	if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
		UnionSHP_GetAndClear(pBufferShadow, &widthShadow, &heightShadow, false, true);

	if (Variables::RulesMap.GetBool(ID, "Turret")) // Has turret
	{
		if (Variables::RulesMap.GetBool(ID, "TurretAnimIsVoxel"))
		{
			FString TurName = Variables::RulesMap.GetString(ID, "TurretAnim", ID + "tur");
			FString BarlName = ID + "barl";


			if (!VoxelDrawer::IsVPLLoaded())
				VoxelDrawer::LoadVPLFile("voxels.vpl");

			std::vector<unsigned char*> pTurImages, pBarlImages;
			pTurImages.resize(facings, nullptr);
			pBarlImages.resize(facings, nullptr);
			std::vector<VoxelRectangle> turrect, barlrect;
			turrect.resize(facings);
			barlrect.resize(facings);

			FString VXLName = BarlName + ".vxl";
			FString HVAName = BarlName + ".hva";
			if (VoxelDrawer::LoadVXLFile(VXLName))
			{
				if (VoxelDrawer::LoadHVAFile(HVAName))
				{
					for (int i = 0; i < facings; ++i)
					{
						bool result = VoxelDrawer::GetImageData((facings + 5 * facings / 8 - i) % facings, pBarlImages[i], barlrect[i]);
						if (!result)
							break;
					}
				}
			}

			VXLName = TurName + ".vxl";
			HVAName = TurName + ".hva";
			if (VoxelDrawer::LoadVXLFile(VXLName))
			{
				if (VoxelDrawer::LoadHVAFile(HVAName))
				{
					for (int i = 0; i < facings; ++i)
					{
						bool result = VoxelDrawer::GetImageData((facings + 5 * facings / 8 - i) % facings, pTurImages[i], turrect[i]);
						if (!result)
							break;
					}
				}
			}

			for (int i = 0; i < facings; ++i)
			{
				auto pTempBuf = GameCreateArray<unsigned char>(width * height);
				memcpy_s(pTempBuf, width * height, pBuffer, width * height);
				UnionSHP_Add(pTempBuf, width, height);

				int deltaX = Variables::RulesMap.GetInteger(ID, "TurretAnimX", 0);
				int deltaY = Variables::RulesMap.GetInteger(ID, "TurretAnimY", 0);

				if (pTurImages[i])
				{
					FString pKey;

					pKey.Format("%sX%d", ID, (15 - i * 8 / facings) % 8);
					int turdeltaX = CINI::FAData->GetInteger("BuildingVoxelTurretsRA2", pKey);
					pKey.Format("%sY%d", ID, (15 - i * 8 / facings) % 8);
					int turdeltaY = CINI::FAData->GetInteger("BuildingVoxelTurretsRA2", pKey);

					VXL_Add(pTurImages[i], turrect[i].X + turdeltaX, turrect[i].Y + turdeltaY, turrect[i].W, turrect[i].H);
					CncImgFree(pTurImages[i]);

					if (pBarlImages[i])
					{
						pKey.Format("%sX%d", ID, (15 - i * 8 / facings) % 8);
						int barldeltaX = CINI::FAData->GetInteger("BuildingVoxelBarrelsRA2", pKey);
						pKey.Format("%sY%d", ID, (15 - i * 8 / facings) % 8);
						int barldeltaY = CINI::FAData->GetInteger("BuildingVoxelBarrelsRA2", pKey);

						VXL_Add(pBarlImages[i], barlrect[i].X + barldeltaX, barlrect[i].Y + barldeltaY, barlrect[i].W, barlrect[i].H);
						CncImgFree(pBarlImages[i]);
					}
				}

				int nW = 0x100, nH = 0x100;
				VXL_GetAndClear(pTurImages[i], &nW, &nH);

				UnionSHP_Add(pTurImages[i], 0x100, 0x100, deltaX, deltaY);

				unsigned char* pImage;
				int width1, height1;

				UnionSHP_GetAndClear(pImage, &width1, &height1);
				if (loadAsRubble)
					DictName.Format("%s%d\233RUBBLE", ID, i);
				else
					DictName.Format("%s%d\233DAMAGED", ID, i);
				SetImageDataSafe(pImage, DictName, width1, height1, palette);
			}

			if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
			{
				if (loadAsRubble)
					DictNameShadow.Format("%s%d\233RUBBLESHADOW", ID, 0);
				else
					DictNameShadow.Format("%s%d\233DAMAGEDSHADOW", ID, 0);
				SetImageDataSafe(pBufferShadow, DictNameShadow, widthShadow, heightShadow, &CMapDataExt::Palette_Shadow);
			}

			GameDeleteArray(pBuffer, width * height);
		}
		else //SHP anim
		{
			FString TurName = Variables::RulesMap.GetString(ID, "TurretAnim", ID + "tur");
			int nStartFrame = CINI::Art->GetInteger(TurName, "LoopStart");
			bool shadow = bHasShadow && CINI::Art->GetBool(TurName, "Shadow", true) && ExtConfigs::InGameDisplay_Shadow;
			for (int i = 0; i < facings; ++i)
			{
				auto pTempBuf = GameCreateArray<unsigned char>(width * height);
				memcpy_s(pTempBuf, width * height, pBuffer, width * height);
				UnionSHP_Add(pTempBuf, width, height);

				if (shadow)
				{
					auto pTempBufShadow = GameCreateArray<unsigned char>(width * height);
					memcpy_s(pTempBufShadow, width * height, pBufferShadow, width * height);
					UnionSHP_Add(pTempBufShadow, width, height, 0, 0, false, true);
				}

				int deltaX = Variables::RulesMap.GetInteger(ID, "TurretAnimX", 0);
				int deltaY = Variables::RulesMap.GetInteger(ID, "TurretAnimY", 0);
				loadSingleFrameShape(CINI::Art->GetString(TurName, "Image", TurName),
					nStartFrame + i * 32 / facings, deltaX, deltaY, "", shadow);

				unsigned char* pImage;
				int width1, height1;
				UnionSHP_GetAndClear(pImage, &width1, &height1);

				if (loadAsRubble)
					DictName.Format("%s%d\233RUBBLE", ID, i);
				else
					DictName.Format("%s%d\233DAMAGED", ID, i);
				SetImageDataSafe(pImage, DictName, width1, height1, palette);

				if (shadow)
				{
					FString DictNameShadow;
					unsigned char* pImageShadow;
					int width1Shadow, height1Shadow;
					UnionSHP_GetAndClear(pImageShadow, &width1Shadow, &height1Shadow, false, true);
					if (loadAsRubble)
						DictNameShadow.Format("%s%d\233RUBBLESHADOW", ID, i);
					else
						DictNameShadow.Format("%s%d\233DAMAGEDSHADOW", ID, i);
					SetImageDataSafe(pImageShadow, DictNameShadow, width1Shadow, height1Shadow, &CMapDataExt::Palette_Shadow);
				}
			}
			GameDelete(pBuffer);
			GameDelete(pBufferShadow);
		}
	}
	else // No turret
	{
		if (loadAsRubble)
			DictName.Format("%s%d\233RUBBLE", ID, 0);
		else
			DictName.Format("%s%d\233DAMAGED", ID, 0);
		SetImageDataSafe(pBuffer, DictName, width, height, palette);

		if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
		{
			if (loadAsRubble)
				DictNameShadow.Format("%s%d\233RUBBLESHADOW", ID, 0);
			else
				DictNameShadow.Format("%s%d\233DAMAGEDSHADOW", ID, 0);
			SetImageDataSafe(pBufferShadow, DictNameShadow, widthShadow, heightShadow, &CMapDataExt::Palette_Shadow);
		}
	}
}

void CLoadingExt::LoadBuilding_Rubble(FString ID)
{
	FString ArtID = GetArtID(ID);
	FString ImageID = GetBuildingFileID(ID);
	bool bHasShadow = !Variables::RulesMap.GetBool(ID, "NoShadow");
	FString PaletteName = "iso\233NotAutoTinted";
	PaletteName = CINI::Art->GetString(ArtID, "RubblePalette", PaletteName);
	GetFullPaletteName(PaletteName);
	auto pal = PalettesManager::LoadPalette(PaletteName);

	auto loadBuildingFrameShape = [&](FString name, int nFrame = 0, int deltaX = 0, int deltaY = 0, bool shadow = false) -> bool
	{
		FString file = name + ".SHP";
		int nMix = SearchFile(file);
		// building can be displayed without the body
		if (!CLoading::HasFile(file, nMix))
			return true;

		ShapeHeader header;
		unsigned char* pBuffer;
		CMixFile::LoadSHP(file, nMix);
		CShpFile::GetSHPHeader(&header);
		if (header.FrameCount / 2 <= nFrame) {
			return false;
		}
		CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &pBuffer, header);

		UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY);

		if (shadow && ExtConfigs::InGameDisplay_Shadow)
		{
			CLoadingExt::LoadSHPFrameSafe(nFrame + header.FrameCount / 2, 1, &pBuffer, header);
			UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, true);
		}

		return true;
	};

	auto loadSingleFrameShape = [&](FString name, int nFrame = 0, int deltaX = 0,
		int deltaY = 0, bool shadow = false, int forceNewTheater = -1) -> bool
	{
			bool applyNewTheater = CINI::Art->GetBool(name, "NewTheater");
			name = CINI::Art->GetString(name, "Image", name);
			applyNewTheater = CINI::Art->GetBool(name, "NewTheater", applyNewTheater);

			FString file = name + ".SHP";
			int nMix = SearchFile(file);
			int loadedMix = CLoadingExt::HasFileMix(file, nMix);
			// if anim file in RA2(MD).mix, always use NewTheater = yes
			if (Ra2dotMixes.find(loadedMix) != Ra2dotMixes.end())
			{
				applyNewTheater = true;
			}

			if (applyNewTheater || forceNewTheater == 1)
				SetTheaterLetter(file, ExtConfigs::NewTheaterType ? 1 : 0);
			nMix = SearchFile(file);
			if (!CLoading::HasFile(file, nMix))
			{
				SetGenericTheaterLetter(file);
				nMix = SearchFile(file);
				if (!CLoading::HasFile(file, nMix))
				{
					if (!ExtConfigs::UseStrictNewTheater)
					{
						auto searchNewTheater = [&nMix, this, &file](char t)
							{
								if (file.GetLength() >= 2)
									file.SetAt(1, t);
								nMix = SearchFile(file);
								return HasFile(file, nMix);
							};
						file = name + ".SHP";
						nMix = SearchFile(file);
						if (!CLoading::HasFile(file, nMix))
							if (!searchNewTheater('T'))
								if (!searchNewTheater('A'))
									if (!searchNewTheater('U'))
										if (!searchNewTheater('N'))
											if (!searchNewTheater('L'))
												if (!searchNewTheater('D'))
													return false;
					}
					else
					{
						return false;
					}
				}
			}

		ShapeHeader header;
		unsigned char* pBuffer;
		CMixFile::LoadSHP(file, nMix);
		CShpFile::GetSHPHeader(&header);
		if (header.FrameCount <= nFrame) {
			nFrame = 0;
		}
		CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &pBuffer, header);

		UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY);

		if (shadow && ExtConfigs::InGameDisplay_Shadow)
		{
			CLoadingExt::LoadSHPFrameSafe(nFrame + header.FrameCount / 2, 1, &pBuffer, header);
			UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, true);
		}

		return true;
	};

	auto loadAnimFrameShape = [&](FString animkey, FString ignorekey)
	{
		FString damagedAnimkey = animkey + "Damaged";
		if (auto pStr = CINI::Art->TryGetString(ArtID, damagedAnimkey))
		{
			if (!CINI::FAData->GetBool(ignorekey, ID))
			{
				int nStartFrame = CINI::Art->GetInteger(*pStr, "LoopStart");
				loadSingleFrameShape(*pStr, nStartFrame);
			}
		}
		else if (auto pStr = CINI::Art->TryGetString(ArtID, animkey))
		{
			if (!CINI::FAData->GetBool(ignorekey, ID))
			{
				int nStartFrame = CINI::Art->GetInteger(*pStr, "LoopStart");
				loadSingleFrameShape(*pStr, nStartFrame);
			}
		}
	};

	if (Variables::RulesMap.GetBool(ID, "LeaveRubble"))
	{
		int nBldStartFrame = CINI::Art->GetInteger(ArtID, "LoopStart", 0) + 3;
		if (loadBuildingFrameShape(ImageID, nBldStartFrame, 0, 0, bHasShadow && CINI::Art->GetBool(ArtID, "Shadow", true)))
		{
			unsigned char* pBuffer;
			int width, height;
			UnionSHP_GetAndClear(pBuffer, &width, &height);

			FString DictName = ID + "0\233RUBBLE";
			SetImageDataSafe(pBuffer, DictName, width, height, pal);

			if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
			{
				FString DictNameShadow;
				unsigned char* pBufferShadow{ 0 };
				int widthShadow, heightShadow;
				UnionSHP_GetAndClear(pBufferShadow, &widthShadow, &heightShadow, false, true);
				DictNameShadow.Format("%s%d\233RUBBLESHADOW", ID, 0);
				SetImageDataSafe(pBufferShadow, DictNameShadow, widthShadow, heightShadow, &CMapDataExt::Palette_Shadow);
			}
		}
		else
		{
			LoadBuilding_Damaged(ID, true);
			CMapDataExt::DamagedAsRubbleBuildings.insert(ID);
		}
	}
}

void CLoadingExt::LoadInfantry(FString ID)
{	
	FString ArtID = GetArtID(ID);
	FString ImageID = GetInfantryFileID(ID);
	bool bHasShadow = !Variables::RulesMap.GetBool(ID, "NoShadow");

	FString sequenceName = CINI::Art->GetString(ImageID, "Sequence");
	bool deployable = Variables::RulesMap.GetBool(ID, "Deployer") && CINI::Art->KeyExists(sequenceName, "Deployed");
	bool waterable = Variables::RulesMap.GetString(ID, "MovementZone") == "AmphibiousDestroyer" 
		&& CINI::Art->KeyExists(sequenceName, "Swim");
	FString frames = CINI::Art->GetString(sequenceName, "Guard", "0,1,1");
	int framesToRead[8];
	int frameStart, frameStep;
	sscanf_s(frames, "%d,%d,%d", &frameStart, &framesToRead[0], &frameStep);
	for (int i = 0; i < 8; ++i)
		framesToRead[i] = frameStart + i * frameStep;

	FString PaletteName = CINI::Art->GetString(ArtID, "Palette", "unit");
	GetFullPaletteName(PaletteName);
	auto pal = PalettesManager::LoadPalette(PaletteName);
	
	FString FileName = ImageID + ".shp";
	int nMix = this->SearchFile(FileName);
	if (CLoading::HasFile(FileName, nMix))
	{
		ShapeHeader header;
		unsigned char* FramesBuffers;
		CMixFile::LoadSHP(FileName, nMix);
		CShpFile::GetSHPHeader(&header);
		for (int i = 0; i < 8; ++i)
		{
			if (IsLoadingObjectView && i != 5)
				continue;

			CLoadingExt::LoadSHPFrameSafe(framesToRead[i], 1, &FramesBuffers, header);
			FString DictName;
			DictName.Format("%s%d", ID, i);
			// DictName.Format("%s%d", ImageID, i);
			SetImageDataSafe(FramesBuffers, DictName, header.Width, header.Height, pal);

			if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
			{
				FString DictNameShadow;
				unsigned char* pBufferShadow{ 0 };
				DictNameShadow.Format("%s%d\233SHADOW", ID, i);
				CLoadingExt::LoadSHPFrameSafe(framesToRead[i] + header.FrameCount / 2, 1, &pBufferShadow, header);
				SetImageDataSafe(pBufferShadow, DictNameShadow, header.Width, header.Height, &CMapDataExt::Palette_Shadow);
			}
		}

		if (ExtConfigs::InGameDisplay_Deploy && deployable)
		{
			FString framesDeploy = CINI::Art->GetString(sequenceName, "Deployed", "0,1,1");
			int framesToReadDeploy[8];
			int frameStartDeploy, frameStepDeploy;
			sscanf_s(framesDeploy, "%d,%d,%d", &frameStartDeploy, &framesToReadDeploy[0], &frameStepDeploy);
			for (int i = 0; i < 8; ++i)
				framesToReadDeploy[i] = frameStartDeploy + i * frameStepDeploy;
			unsigned char* FramesBuffersDeploy;
			for (int i = 0; i < 8; ++i)
			{
				CLoadingExt::LoadSHPFrameSafe(framesToReadDeploy[i], 1, &FramesBuffersDeploy, header);
				FString DictNameDeploy;
				DictNameDeploy.Format("%s%d\233DEPLOY", ID, i);
				SetImageDataSafe(FramesBuffersDeploy, DictNameDeploy, header.Width, header.Height, pal);

				if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
				{
					FString DictNameShadow;
					unsigned char* pBufferShadow{ 0 };
					DictNameShadow.Format("%s%d\233DEPLOYSHADOW", ID, i);
					CLoadingExt::LoadSHPFrameSafe(framesToReadDeploy[i] + header.FrameCount / 2, 1, &pBufferShadow, header);
					SetImageDataSafe(pBufferShadow, DictNameShadow, header.Width, header.Height, &CMapDataExt::Palette_Shadow);
				}
			}
		}
		
		if (ExtConfigs::InGameDisplay_Water && waterable)
		{
			FString framesWater = CINI::Art->GetString(sequenceName, "Swim", "0,1,1");
			int framesToReadWater[8];
			int frameStartWater, frameStepWater;
			sscanf_s(framesWater, "%d,%d,%d", &frameStartWater, &framesToReadWater[0], &frameStepWater);
			for (int i = 0; i < 8; ++i)
				framesToReadWater[i] = frameStartWater + i * frameStepWater;
			unsigned char* FramesBuffersWater;
			for (int i = 0; i < 8; ++i)
			{
				CLoadingExt::LoadSHPFrameSafe(framesToReadWater[i], 1, &FramesBuffersWater, header);
				FString DictNameWater;
				DictNameWater.Format("%s%d\233WATER", ID, i);
				SetImageDataSafe(FramesBuffersWater, DictNameWater, header.Width, header.Height, pal);

				if (ExtConfigs::InGameDisplay_Shadow)
				{
					FString DictNameShadow;
					unsigned char* pBufferShadow{ 0 };
					DictNameShadow.Format("%s%d\233WATERSHADOW", ID, i);
					CLoadingExt::LoadSHPFrameSafe(framesToReadWater[i] + header.FrameCount / 2, 1, &pBufferShadow, header);
					SetImageDataSafe(pBufferShadow, DictNameShadow, header.Width, header.Height, &CMapDataExt::Palette_Shadow);
				}
			}
		}
	}
}

void CLoadingExt::LoadTerrainOrSmudge(FString ID, bool terrain)
{
	FString ArtID = GetArtID(ID);
	FString ImageID = GetTerrainOrSmudgeFileID(ID);
	FString FileName = ImageID + this->GetFileExtension();
	int nMix = this->SearchFile(FileName);
	if (CLoading::HasFile(FileName, nMix))
	{
		ShapeHeader header;
		unsigned char* FramesBuffers[1];
		CMixFile::LoadSHP(FileName, nMix);
		CShpFile::GetSHPHeader(&header);
		CLoadingExt::LoadSHPFrameSafe(0, 1, &FramesBuffers[0], header);
		FString DictName;
		DictName.Format("%s%d", ID, 0);
		FString PaletteName = CINI::Art->GetString(ArtID, "Palette", "iso");
		if (!CINI::Art->KeyExists(ArtID, "Palette") && Variables::RulesMap.GetBool(ID, "SpawnsTiberium"))
		{
			PaletteName = "unitsno.pal";
		}
		PaletteName.MakeUpper();
		GetFullPaletteName(PaletteName);
		SetImageDataSafe(FramesBuffers[0], DictName, header.Width, header.Height, PalettesManager::LoadPalette(PaletteName));

		if (ExtConfigs::InGameDisplay_Shadow && terrain)
		{
			FString DictNameShadow;
			unsigned char* pBufferShadow[1];
			DictNameShadow.Format("%s%d\233SHADOW", ID, 0);
			CLoadingExt::LoadSHPFrameSafe(0 + header.FrameCount / 2, 1, &pBufferShadow[0], header);
			SetImageDataSafe(pBufferShadow[0], DictNameShadow, header.Width, header.Height, &CMapDataExt::Palette_Shadow);
		}

		if (ExtConfigs::InGameDisplay_AlphaImage && terrain)
		{
			if (auto pAIFile = Variables::RulesMap.TryGetString(ID, "AlphaImage"))
			{
				auto AIDicName = *pAIFile + "\233ALPHAIMAGE";
				if (!CLoadingExt::IsObjectLoaded(AIDicName))
					LoadShp(AIDicName, *pAIFile + ".shp", "anim.pal", 0);
			}
		}
	}
}

void CLoadingExt::LoadVehicleOrAircraft(FString ID)
{
	FString ArtID = GetArtID(ID);
	FString ImageID = GetVehicleOrAircraftFileID(ID);
	bool bHasTurret = Variables::RulesMap.GetBool(ID, "Turret");
	bool bHasShadow = !Variables::RulesMap.GetBool(ID, "NoShadow");
	int facings = ExtConfigs::ExtFacings ? 32 : 8;

	if (CINI::Art->GetBool(ArtID, "Voxel")) // As VXL
	{
		AvailableFacings[ID] = facings;
		FString FileName = ImageID + ".vxl";
		FString HVAName = ImageID + ".hva";

		if (!VoxelDrawer::IsVPLLoaded())
			VoxelDrawer::LoadVPLFile("voxels.vpl");

		FString PaletteName = CINI::Art->GetString(ArtID, "Palette", "unit");
		GetFullPaletteName(PaletteName);

		std::vector<unsigned char*> pImage, pTurretImage, pBarrelImage, pShadowImage;
		pImage.resize(facings, nullptr);
		pTurretImage.resize(facings, nullptr);
		pBarrelImage.resize(facings, nullptr);
		if (ExtConfigs::InGameDisplay_Shadow && bHasShadow)
			pShadowImage.resize(facings, nullptr);
		std::vector<VoxelRectangle> rect, turretrect, barrelrect, shadowrect;
		rect.resize(facings);
		turretrect.resize(facings);
		barrelrect.resize(facings);
		if (ExtConfigs::InGameDisplay_Shadow && bHasShadow)
			shadowrect.resize(facings);

		if (VoxelDrawer::LoadVXLFile(FileName))
		{
			if (VoxelDrawer::LoadHVAFile(HVAName))
			{
				for (int i = 0; i < facings; ++i)
				{
					int actFacing = (i + facings - 2 * facings / 8) % facings;
					bool result = false;
					if (ExtConfigs::InGameDisplay_Shadow && bHasShadow)
					{
						result = VoxelDrawer::GetImageData(actFacing, pImage[i], rect[i])
							&& VoxelDrawer::GetImageData(actFacing, pShadowImage[i], shadowrect[i], 0, 0, 0, true);
					}
					else
					{
						result = VoxelDrawer::GetImageData(actFacing, pImage[i], rect[i]);
					}
					if (!result)
						return;
				}
			}
		}

		if (bHasTurret)
		{
			int F, L, H;
			int s_count = sscanf_s(CINI::Art->GetString(ArtID, "TurretOffset", "0,0,0"), "%d,%d,%d", &F, &L, &H);
			if (s_count == 0) F = L = H = 0;
			else if (s_count == 1) L = H = 0;
			else if (s_count == 2) H = 0;

			FString turFileName = ImageID + "tur.vxl";
			FString turHVAName = ImageID + "tur.hva";
			if (VoxelDrawer::LoadVXLFile(turFileName))
			{
				if (VoxelDrawer::LoadHVAFile(turHVAName))
				{
					for (int i = 0; i < facings; ++i)
					{
						int actFacing = (i + facings - 2 * facings / 8) % facings;
						bool result = VoxelDrawer::GetImageData(actFacing, pTurretImage[i], turretrect[i], F, L, H);
						if (!result)
							break;
					}
				}
			}

			FString barlFileName = ImageID + "barl.vxl";
			FString barlHVAName = ImageID + "barl.hva";
			if (VoxelDrawer::LoadVXLFile(barlFileName))
			{
				if (VoxelDrawer::LoadHVAFile(barlHVAName))
				{
					for (int i = 0; i < facings; ++i)
					{
						int actFacing = (i + facings - 2 * facings / 8) % facings;
						bool result = VoxelDrawer::GetImageData(actFacing, pBarrelImage[i], barrelrect[i], F, L, H);
						if (!result)
							break;
					}
				}
			}

			for (int i = 0; i < facings; ++i)
			{
				if (IsLoadingObjectView && i != facings / 8 * 2)
					continue;
				FString DictName;
				DictName.Format("%s%d", ID, i);
				//DictName.Format("%s%d", ImageID, i);

				unsigned char* outBuffer;
				int outW = 0x100, outH = 0x100;

				if (pImage[i])
				{
					VXL_Add(pImage[i], rect[i].X, rect[i].Y, rect[i].W, rect[i].H);
					CncImgFree(pImage[i]);
				}
				FString pKey;
				if (pTurretImage[i])
				{
					pKey.Format("%sX%d", ID, i);
					int turdeltaX = CINI::FAData->GetInteger("VehicleVoxelTurretsRA2", pKey);
					pKey.Format("%sY%d", ID, i);
					int turdeltaY = CINI::FAData->GetInteger("VehicleVoxelTurretsRA2", pKey);
					VXL_Add(pTurretImage[i], turretrect[i].X + turdeltaX, turretrect[i].Y + turdeltaY, turretrect[i].W, turretrect[i].H);
					CncImgFree(pTurretImage[i]);

					if (pBarrelImage[i])
					{
						pKey.Format("%sX%d", ID, i);
						int barldeltaX = CINI::FAData->GetInteger("VehicleVoxelBarrelsRA2", pKey);
						pKey.Format("%sY%d", ID, i);
						int barldeltaY = CINI::FAData->GetInteger("VehicleVoxelBarrelsRA2", pKey);

						VXL_Add(pBarrelImage[i], barrelrect[i].X + barldeltaX, barrelrect[i].Y + barldeltaY, barrelrect[i].W, barrelrect[i].H);
						CncImgFree(pBarrelImage[i]);
					}
				}

				VXL_GetAndClear(outBuffer, &outW, &outH);

				SetImageDataSafe(outBuffer, DictName, outW, outH, PalettesManager::LoadPalette(PaletteName));
			}
		}
		else
		{
			for (int i = 0; i < facings; ++i)
			{
				if (IsLoadingObjectView && i != facings / 8 * 2)
					continue;
				FString DictName;
				DictName.Format("%s%d", ID, i);
				// DictName.Format("%s%d", ImageID, i);

				unsigned char* outBuffer;
				int outW = 0x100, outH = 0x100;

				VXL_Add(pImage[i], rect[i].X, rect[i].Y, rect[i].W, rect[i].H);
				delete[] pImage[i];
				VXL_GetAndClear(outBuffer, &outW, &outH);

				SetImageDataSafe(outBuffer, DictName, outW, outH, PalettesManager::LoadPalette(PaletteName));
			}
		}
		if (ExtConfigs::InGameDisplay_Shadow && bHasShadow)
			for (int i = 0; i < facings; ++i)
			{
				FString DictShadowName;
				DictShadowName.Format("%s%d\233SHADOW", ID, i);

				unsigned char* outBuffer;
				int outW = 0x100, outH = 0x100;

				VXL_Add(pShadowImage[i], shadowrect[i].X, shadowrect[i].Y, shadowrect[i].W, shadowrect[i].H, true);
				delete[] pShadowImage[i];
				VXL_GetAndClear(outBuffer, &outW, &outH, true);

				SetImageDataSafe(outBuffer, DictShadowName, outW, outH, &CMapDataExt::Palette_Shadow);
			}
	}
	else // As SHP
	{
		int facingCount = CINI::Art->GetInteger(ArtID, "Facings", 8);
		if (facingCount % 8 != 0)
			facingCount = (facingCount + 7) / 8 * 8;
		int targetFacings = ExtConfigs::ExtFacings ? facingCount : 8;
		AvailableFacings[ID] = targetFacings;
		std::vector<int> framesToRead(targetFacings);
		if (CINI::Art->KeyExists(ArtID, "StandingFrames"))
		{
			int nStartStandFrame = CINI::Art->GetInteger(ArtID, "StartStandFrame", 0);
			int nStandingFrames = CINI::Art->GetInteger(ArtID, "StandingFrames", 1);
			for (int i = 0; i < targetFacings; ++i)
				framesToRead[i] = nStartStandFrame + (i * facingCount / targetFacings) * nStandingFrames;
		}
		else
		{
			int nStartWalkFrame = CINI::Art->GetInteger(ArtID, "StartWalkFrame", 0);
			int nWalkFrames = CINI::Art->GetInteger(ArtID, "WalkFrames", 1);
			for (int i = 0; i < targetFacings; ++i) {
				framesToRead[i] = nStartWalkFrame + (i * facingCount / targetFacings) * nWalkFrames;
			}
		}

		std::rotate(framesToRead.begin(), framesToRead.begin() + 1 * targetFacings / 8, framesToRead.end());

		FString FileName = ImageID + ".shp";
		int nMix = this->SearchFile(FileName);
		if (CLoading::HasFile(FileName, nMix))
		{
			ShapeHeader header;
			unsigned char* FramesBuffers[2];
			unsigned char* FramesBuffersShadow[2];
			CMixFile::LoadSHP(FileName, nMix);
			CShpFile::GetSHPHeader(&header);
			for (int i = 0; i < targetFacings; ++i)
			{
				if (IsLoadingObjectView && i != targetFacings / 8 * 2)
					continue;
				CLoadingExt::LoadSHPFrameSafe(framesToRead[i], 1, &FramesBuffers[0], header);
				FString DictName;
				DictName.Format("%s%d", ID, i);
				// DictName.Format("%s%d", ImageID, i);
				FString PaletteName = CINI::Art->GetString(ArtID, "Palette", "unit");
				GetFullPaletteName(PaletteName);
				
				if (bHasTurret)
				{
					int F, L, H;
					int s_count = sscanf_s(CINI::Art->GetString(ArtID, "TurretOffset", "0,0,0"), "%d,%d,%d", &F, &L, &H);
					if (s_count == 0) F = L = H = 0;
					else if (s_count == 1) L = H = 0;
					else if (s_count == 2) H = 0;

					int nStartWalkFrame = CINI::Art->GetInteger(ArtID, "StartWalkFrame", 0);
					int nWalkFrames = CINI::Art->GetInteger(ArtID, "WalkFrames", 1);
					int turretFrameToRead;
					
					// turret start from 0 + WalkFrames * Facings, ignore StartWalkFrame
					// and always has 32 facings
					turretFrameToRead = facingCount * nWalkFrames + ((targetFacings / 8 + i) % targetFacings) * 32 / targetFacings;

					CLoadingExt::LoadSHPFrameSafe(turretFrameToRead, 1, &FramesBuffers[1], header);
					Matrix3D mat(F, L, H, i, targetFacings);

					UnionSHP_Add(FramesBuffers[0], header.Width, header.Height);
					UnionSHP_Add(FramesBuffers[1], header.Width, header.Height, mat.OutputX, mat.OutputY);
					unsigned char* outBuffer;
					int outW, outH;
					UnionSHP_GetAndClear(outBuffer, &outW, &outH);
					
					SetImageDataSafe(outBuffer, DictName, outW, outH, PalettesManager::LoadPalette(PaletteName));

					if (ExtConfigs::InGameDisplay_Shadow && bHasShadow)
					{
						FString DictNameShadow;
						DictNameShadow.Format("%s%d\233SHADOW", ID, i);
						CLoadingExt::LoadSHPFrameSafe(framesToRead[i] + header.FrameCount / 2, 1, &FramesBuffersShadow[0], header);
						CLoadingExt::LoadSHPFrameSafe(turretFrameToRead + header.FrameCount / 2, 1, &FramesBuffersShadow[1], header);
						UnionSHP_Add(FramesBuffersShadow[0], header.Width, header.Height, 0, 0, false, true);
						UnionSHP_Add(FramesBuffersShadow[1], header.Width, header.Height, mat.OutputX, mat.OutputY, false, true);
						unsigned char* outBufferShadow;
						int outWShadow, outHShadow;
						UnionSHP_GetAndClear(outBufferShadow, &outWShadow, &outHShadow, false, true);

						SetImageDataSafe(outBufferShadow, DictNameShadow, outWShadow, outHShadow, &CMapDataExt::Palette_Shadow);
					}
				}
				else
				{
					SetImageDataSafe(FramesBuffers[0], DictName, header.Width, header.Height, PalettesManager::LoadPalette(PaletteName));
					if (ExtConfigs::InGameDisplay_Shadow && bHasShadow)
					{
						FString DictNameShadow;
						DictNameShadow.Format("%s%d\233SHADOW", ID, i);
						CLoadingExt::LoadSHPFrameSafe(framesToRead[i] + header.FrameCount / 2, 1, &FramesBuffersShadow[0], header);
						SetImageDataSafe(FramesBuffersShadow[0], DictNameShadow, header.Width, header.Height, &CMapDataExt::Palette_Shadow);
					}
				}
			}
		}
	}
}

void CLoadingExt::SetImageDataSafe(unsigned char* pBuffer, FString NameInDict, int FullWidth, int FullHeight, Palette* pPal, bool toServer, bool clip)
{
	if (ExtConfigs::LoadImageDataFromServer && toServer)
	{
		ImageDataClassSafe tmp;
		SetImageDataSafe(pBuffer, &tmp, FullWidth, FullHeight, pPal);
		if (clip) TrimImageEdges(&tmp);
		CLoadingExt::SendImageToServer(NameInDict, &tmp);
	}
	else
	{
		auto pData = CLoadingExt::GetImageDataFromMap(NameInDict);
		SetImageDataSafe(pBuffer, pData, FullWidth, FullHeight, pPal);
		if (clip) TrimImageEdges(pData);
	}
}

void CLoadingExt::SetImageDataSafe(unsigned char* pBuffer, ImageDataClassSafe* pData, int FullWidth, int FullHeight, Palette* pPal)
{
	if (pData->pImageBuffer)
		pData->pImageBuffer = nullptr;
	if (pData->pPixelValidRanges)
		pData->pPixelValidRanges = nullptr;

	pData->pImageBuffer = std::unique_ptr<unsigned char[]>(new unsigned char[FullWidth * FullHeight]);
	std::memcpy(pData->pImageBuffer.get(), pBuffer, FullWidth * FullHeight);
	pData->FullHeight = FullHeight;
	pData->FullWidth = FullWidth;
	SetValidBufferSafe(pData, FullWidth, FullHeight);

	// Get available area
	int counter = 0;
	int validFirstX = FullWidth - 1;
	int validFirstY = FullHeight - 1;
	int validLastX = 0;
	int validLastY = 0;
	for (int j = 0; j < FullHeight; ++j)
	{
		for (int i = 0; i < FullWidth; ++i)
		{
			unsigned char ch = pBuffer[counter++];
			if (ch != 0)
			{
				if (i < validFirstX)
					validFirstX = i;
				if (j < validFirstY)
					validFirstY = j;
				if (i > validLastX)
					validLastX = i;
				if (j > validLastY)
					validLastY = j;
			}
		}
	}

	pData->ValidX = validFirstX;
	pData->ValidY = validFirstY;
	pData->ValidWidth = validLastX - validFirstX + 1;
	pData->ValidHeight = validLastY - validFirstY + 1;

	CLoadingExt::TallestBuildingHeight = std::max(CLoadingExt::TallestBuildingHeight, (int)pData->ValidHeight);
	CIsoViewExt::EXTRA_BORDER_BOTTOM = std::max(CLoadingExt::TallestBuildingHeight / 14, 25);

	pData->Flag = ImageDataFlag::SHP;
	pData->IsOverlay = false;
	pData->pPalette = pPal ? pPal : Palette::PALETTE_UNIT;

	GameDeleteArray(pBuffer, FullWidth * FullHeight);
}

void CLoadingExt::SetImageData(unsigned char* pBuffer, FString NameInDict, int FullWidth, int FullHeight, Palette* pPal)
{
	auto pData = ImageDataMapHelper::GetImageDataFromMap(NameInDict);
	SetImageData(pBuffer, pData, FullWidth, FullHeight, pPal);
}

void CLoadingExt::SetImageData(unsigned char* pBuffer, ImageDataClass* pData, int FullWidth, int FullHeight, Palette* pPal)
{
	if (pData->pImageBuffer)
		GameDeleteArray(pData->pImageBuffer, pData->FullWidth * pData->FullHeight);
	if (pData->pPixelValidRanges)
		GameDeleteArray(pData->pPixelValidRanges, pData->FullHeight);

	pData->pImageBuffer = pBuffer;
	pData->FullHeight = FullHeight;
	pData->FullWidth = FullWidth;
	SetValidBuffer(pData, FullWidth, FullHeight);

	// Get available area
	int counter = 0;
	int validFirstX = FullWidth - 1;
	int validFirstY = FullHeight - 1;
	int validLastX = 0;
	int validLastY = 0;
	for (int j = 0; j < FullHeight; ++j)
	{
		for (int i = 0; i < FullWidth; ++i)
		{
			unsigned char ch = pBuffer[counter++];
			if (ch != 0)
			{
				if (i < validFirstX)
					validFirstX = i;
				if (j < validFirstY)
					validFirstY = j;
				if (i > validLastX)
					validLastX = i;
				if (j > validLastY)
					validLastY = j;
			}
		}
	}

	pData->ValidX = validFirstX;
	pData->ValidY = validFirstY;
	pData->ValidWidth = validLastX - validFirstX + 1;
	pData->ValidHeight = validLastY - validFirstY + 1;

	CLoadingExt::TallestBuildingHeight = std::max(CLoadingExt::TallestBuildingHeight, (int)pData->ValidHeight);
	CIsoViewExt::EXTRA_BORDER_BOTTOM = std::max(CLoadingExt::TallestBuildingHeight / 14, 25);

	pData->Flag = ImageDataFlag::SHP;
	pData->IsOverlay = false;
	pData->pPalette = pPal ? pPal : Palette::PALETTE_UNIT;
}
// This function will shrink it to fit.
// Also will delete the origin buffer and create a new buffer.
void CLoadingExt::ShrinkSHP(unsigned char* pIn, int InWidth, int InHeight, unsigned char*& pOut, int* OutWidth, int* OutHeight)
{
	int counter = 0;
	int validFirstX = InWidth - 1;
	int validFirstY = InHeight - 1;
	int validLastX = 0;
	int validLastY = 0;
	for (int j = 0; j < InHeight; ++j)
	{
		for (int i = 0; i < InWidth; ++i)
		{
			unsigned char ch = pIn[counter++];
			if (ch != 0)
			{
				if (i < validFirstX)
					validFirstX = i;
				if (j < validFirstY)
					validFirstY = j;
				if (i > validLastX)
					validLastX = i;
				if (j > validLastY)
					validLastY = j;
			}
		}
	}

	counter = 0;
	*OutWidth = validLastX - validFirstX + 1;
	*OutHeight = validLastY - validFirstY + 1;
	pOut = GameCreateArray<unsigned char>(*OutWidth * *OutHeight);
	for (int j = 0; j < *OutHeight; ++j)
		memcpy_s(&pOut[j * *OutWidth], *OutWidth, &pIn[(j + validFirstY) * InWidth + validFirstX], *OutWidth);

	GameDeleteArray(pIn, InWidth * InHeight);
}

void CLoadingExt::UnionSHP_Add(unsigned char* pBuffer, int Width, int Height,
	int DeltaX, int DeltaY, bool UseTemp, bool bShadow, int ZAdjust, int YSort, bool MainBody)
{
	if (bShadow)
		UnionSHPShadow_Data[UseTemp].push_back(SHPUnionData{ pBuffer,Width,Height,DeltaX,DeltaY, ZAdjust, YSort, MainBody });
	else
		UnionSHP_Data[UseTemp].push_back(SHPUnionData{ pBuffer,Width,Height,DeltaX,DeltaY, ZAdjust, YSort, MainBody });
}

void CLoadingExt::UnionSHP_GetAndClear(unsigned char*& pOutBuffer, int* OutWidth,
	int* OutHeight, bool UseTemp, bool bShadow, bool bSort)
{
	// never calls it when UnionSHP_Data is empty
	auto& data = bShadow ? UnionSHPShadow_Data : UnionSHP_Data;
	if (data[UseTemp].size() == 1)
	{
		pOutBuffer = data[UseTemp][0].pBuffer;
		*OutWidth = data[UseTemp][0].Width;
		*OutHeight = data[UseTemp][0].Height;
		data[UseTemp].clear();
		return;
	}

	// For each shp, we make their center at the same point, this will give us proper result.
	int W = 0, H = 0;

	for (auto& data : data[UseTemp])
	{
		if (W < data.Width + 2 * abs(data.DeltaX)) W = data.Width + 2 * abs(data.DeltaX);
		if (H < data.Height + 2 * abs(data.DeltaY)) H = data.Height + 2 * abs(data.DeltaY);
	}

	// just make it work like unsigned char[W][H];
	pOutBuffer = GameCreateArray<unsigned char>(W * H);
	*OutWidth = W;
	*OutHeight = H;

	int ImageCenterX = W / 2;
	int ImageCenterY = H / 2;

	if (bSort && ExtConfigs::InGameDisplay_AnimAdjust)
	{
		std::vector<int> LastValidLine(data[UseTemp].size());
		int mainBodyIndex = -1;

		for (int i = 0; i < data[UseTemp].size(); ++i) {
			const auto& img = data[UseTemp][i];
			int nStartX = ImageCenterX - img.Width / 2 + img.DeltaX;
			int nStartY = ImageCenterY - img.Height / 2 + img.DeltaY;
			int lowestValidY = -1; 
			if (img.MainBody) mainBodyIndex = i;
			for (int j = img.Height - 1; j >= 0; --j) {
				for (int i = 0; i < img.Width; ++i) {
					if (img.pBuffer[j * img.Width + i] != 0) {
						lowestValidY = j;
						break;
					}
				}
				if (lowestValidY != -1) break; 
			}
			LastValidLine[i] = (lowestValidY);
		}

		if (mainBodyIndex > -1)
		{
			int mainLastLine = LastValidLine[mainBodyIndex];
			for (size_t i = 0; i < LastValidLine.size(); ++i) {
				LastValidLine[i] -= mainLastLine;
				LastValidLine[i] -= data[UseTemp][i].ZAdjust;
			}

			std::vector<size_t> indices(data[UseTemp].size());
			for (size_t i = 0; i < indices.size(); ++i) {
				indices[i] = i;
			}

			std::sort(indices.begin(), indices.end(),
				[&LastValidLine, &data, &UseTemp](size_t a, size_t b) {
				if (LastValidLine[a] != LastValidLine[b]) {
					return LastValidLine[a] < LastValidLine[b];
				}
				return data[UseTemp][a].MainBody < data[UseTemp][b].MainBody;
			});

			std::vector<SHPUnionData> sortedData(data[UseTemp].size());
			for (size_t i = 0; i < indices.size(); ++i) {
				sortedData[i] = data[UseTemp][indices[i]];
			}

			for (size_t i = 0; i < indices.size(); ++i) {
				data[UseTemp][i] = sortedData[i];
			}
		}
	}

	// Image[X][Y] <=> pOutBuffer[Y * W + X];
	for (auto& data : data[UseTemp])
	{
		int nStartX = ImageCenterX - data.Width / 2 + data.DeltaX;
		int nStartY = ImageCenterY - data.Height / 2 + data.DeltaY;

		for (int j = 0; j < data.Height; ++j)
			for (int i = 0; i < data.Width; ++i)
				if (auto nPalIdx = data.pBuffer[j * data.Width + i])
					pOutBuffer[(nStartY + j) * W + nStartX + i] = nPalIdx;

		GameDeleteArray(data.pBuffer, data.Width * data.Height);
	}

	data[UseTemp].clear();
}

void CLoadingExt::VXL_Add(unsigned char* pCache, int X, int Y, int Width, int Height, bool shadow)
{
	if (shadow)
	{
		for (int j = 0; j < Height; ++j)
			for (int i = 0; i < Width; ++i)
				if (auto ch = pCache[j * Width + i])
					VXL_Shadow_Data[(j + Y) * 0x100 + X + i] = ch;
	}
	else
	{
		for (int j = 0; j < Height; ++j)
			for (int i = 0; i < Width; ++i)
				if (auto ch = pCache[j * Width + i])
					VXL_Data[(j + Y) * 0x100 + X + i] = ch;
	}
}

void CLoadingExt::VXL_GetAndClear(unsigned char*& pBuffer, int* OutWidth, int* OutHeight, bool shadow)
{
	/* TODO : Save memory
	int validFirstX = 0x100 - 1;
	int validFirstY = 0x100 - 1;
	int validLastX = 0;
	int validLastY = 0;

	for (int j = 0; j < 0x100; ++j)
	{
		for (int i = 0; i < 0x100; ++i)
		{
			unsigned char ch = VXL_Data[j * 0x100 + i];
			if (ch != 0)
			{
				if (i < validFirstX)
					validFirstX = i;
				if (j < validFirstY)
					validFirstY = j;
				if (i > validLastX)
					validLastX = i;
				if (j > validLastY)
					validLastY = j;
			}
		}
	}
	*/
	if (shadow)
	{
		pBuffer = GameCreateArray<unsigned char>(0x10000);
		memcpy_s(pBuffer, 0x10000, VXL_Shadow_Data, 0x10000);
		memset(VXL_Shadow_Data, 0, 0x10000);
	}
	else
	{
		pBuffer = GameCreateArray<unsigned char>(0x10000);
		memcpy_s(pBuffer, 0x10000, VXL_Data, 0x10000);
		memset(VXL_Data, 0, 0x10000);
	}

}

void CLoadingExt::SetValidBuffer(ImageDataClass* pData, int Width, int Height)
{
	pData->pPixelValidRanges = GameCreateArray<ImageDataClass::ValidRangeData>(Height);
	for (int i = 0; i < Height; ++i)
	{
		int begin, end;
		this->GetSHPValidRange(pData->pImageBuffer, Width, i, &begin, &end);
		pData->pPixelValidRanges[i].First = begin;
		pData->pPixelValidRanges[i].Last = end;
	}
}

void CLoadingExt::SetValidBufferSafe(ImageDataClassSafe* pData, int Width, int Height)
{
	pData->pPixelValidRanges = std::unique_ptr<ImageDataClassSafe::ValidRangeData[]>(new ImageDataClassSafe::ValidRangeData[Height]);
	for (int i = 0; i < Height; ++i)
	{
		int begin, end;
		this->GetSHPValidRange(pData->pImageBuffer.get(), Width, i, &begin, &end);
		pData->pPixelValidRanges[i].First = begin;
		pData->pPixelValidRanges[i].Last = end;
	}
}

void CLoadingExt::TrimImageEdges(ImageDataClassSafe* pData)
{
	if (!pData || !pData->pImageBuffer || pData->FullWidth == 0 || pData->FullHeight == 0) return;

	const int oldW = pData->FullWidth;
	const int oldH = pData->FullHeight;
	unsigned char* buffer = pData->pImageBuffer.get();

	int minX = oldW - 1, minY = oldH - 1;
	int maxX = 0, maxY = 0;

	for (int y = 0; y < oldH; ++y)
	{
		for (int x = 0; x < oldW; ++x)
		{
			unsigned char px = buffer[y * oldW + x];
			if (px != 0)
			{
				if (x < minX) minX = x;
				if (y < minY) minY = y;
				if (x > maxX) maxX = x;
				if (y > maxY) maxY = y;
			}
		}
	}

	if (minX > maxX || minY > maxY)
		return;

	int validW = maxX - minX + 1;
	int validH = maxY - minY + 1;

	int leftSpace = minX;
	int rightSpace = oldW - 1 - maxX;
	int topSpace = minY;
	int bottomSpace = oldH - 1 - maxY;

	int cropLR = std::min(leftSpace, rightSpace);
	int cropTB = std::min(topSpace, bottomSpace);

	int newW = oldW - cropLR * 2;
	int newH = oldH - cropTB * 2;

	if (newW <= 0 || newH <= 0) return;

	std::unique_ptr<unsigned char[]> newBuffer(new unsigned char[newW * newH]);
	for (int y = 0; y < newH; ++y)
	{
		int srcY = y + cropTB;
		std::memcpy(&newBuffer[y * newW],
			&buffer[srcY * oldW + cropLR],
			newW);
	}

	pData->pImageBuffer = std::move(newBuffer);
	pData->FullWidth = newW;
	pData->FullHeight = newH;
	pData->ValidX = minX - cropLR;
	pData->ValidY = minY - cropTB;
	pData->ValidWidth = validW;
	pData->ValidHeight = validH;
	if (pData->pPixelValidRanges)
		pData->pPixelValidRanges = nullptr;
	SetValidBufferSafe(pData, newW, newH);
}

void CLoadingExt::SetTheaterLetter(FString& string, int mode)
{
	if (string.GetLength() < 2)
		return;

	if (this->TheaterIdentifier != 0)
	{
		if (mode == 1)
		{
			// Ares code here
			char c0 = string[0];
			char c1 = string[1] & ~0x20; // evil hack to uppercase
			if (isalpha(static_cast<unsigned char>(c0))) {
				if (c1 == 'A' || c1 == 'T') {
					string.SetAt(1, this->TheaterIdentifier);
				}
			}
		}
		else
		{
			// vanilla YR logic
			char c0 = string[0] & ~0x20;
			char c1 = string[1] & ~0x20;
			if (c0 == 'G' || c0 == 'N' || c0 == 'C' || c0 == 'Y') {
				if (c1 == 'A' || c1 == 'T') {
					string.SetAt(1, this->TheaterIdentifier);
				}
			}
		}
	}
}
void CLoadingExt::SetGenericTheaterLetter(FString& string)
{
	if (string.GetLength() < 2)
		return;

	string.SetAt(1, 'G');
}

int CLoadingExt::HasFileMix(FString filename, int nMix)
{
	FString filepath = CFinalSunApp::FilePath();
	filepath += filename;
	std::ifstream fin;
	fin.open(filepath, std::ios::in | std::ios::binary);
	if (fin.is_open())
	{
		fin.close();
		return -1;
	}
	size_t size = 0;
	auto data = ResourcePackManager::instance().getFileData(filename, &size);
	if (data && size > 0)
	{
		return -1;
	}

	if (ExtConfigs::ExtMixLoader)
	{
		auto& manager = MixLoader::Instance();
		int result = manager.QueryFileIndex(filename, nMix);
		if (result >= 0)
			return result;
	}

	if (nMix == -114)
	{
		nMix = CLoading::Instance->SearchFile(filename);
		if (CMixFile::HasFile(filename, nMix))
			return nMix;
		else
			return -2;
	}
	if (CMixFile::HasFile(filename, nMix))
		return nMix;

	return -2;
}

void CLoadingExt::GetFullPaletteName(FString& PaletteName)
{
	const int len = PaletteName.GetLength();
	if (len >= 4 &&
		PaletteName[len - 4] == '.' &&
		(PaletteName[len - 3] == 'p' || PaletteName[len - 3] == 'P') &&
		(PaletteName[len - 2] == 'a' || PaletteName[len - 2] == 'A') &&
		(PaletteName[len - 1] == 'l' || PaletteName[len - 1] == 'L'))
	{
		return;
	}

	switch (this->TheaterIdentifier)
	{
	case 'A':
		PaletteName += "sno.pal";
		return;
	case 'U':
		PaletteName += "urb.pal";
		return;
	case 'N':
		PaletteName += "ubn.pal";
		return;
	case 'D':
		PaletteName += "des.pal";
		return;
	case 'L':
		PaletteName += "lun.pal";
		return;
	case 'T':
	default:
		PaletteName += "tem.pal";
		return;
	}
}

int CLoadingExt::ColorDistance(const ColorStruct& color1, const ColorStruct& color2)
{
	int diffRed = color1.red - color2.red;
	int diffGreen = color1.green - color2.green;
	int diffBlue = color1.blue - color2.blue;
	return diffRed * diffRed + diffGreen * diffGreen + diffBlue * diffBlue;
}

std::vector<int> CLoadingExt::GeneratePalLookupTable(Palette* first, Palette* second)
{
	if (!first || !second) {
		std::vector<int> lookupTable;
		return lookupTable;
	}
	std::vector<int> lookupTable(256);

	lookupTable[0] = 0;
	for (int i = 1; i < 256; ++i) {
		int minDistance = std::numeric_limits<int>::max();
		int bestMatchIndex = 0;

		for (int j = 1; j < 256; ++j) {
			if (j >= 16 && j < 32) { // skip house colors
				continue;
			}
			int distance = ColorDistance(first->GetByteColor(i), second->GetByteColor(j));
			if (distance < minDistance) {
				minDistance = distance;
				bestMatchIndex = j;
			}
		}

		lookupTable[i] = bestMatchIndex;
	}

	return lookupTable;
}

void CLoadingExt::LoadSHPFrameSafe(int nFrame, int nFrameCount, unsigned char** ppBuffer, const ShapeHeader& header)
{
	if (nFrame < 0 || nFrame + nFrameCount - 1 >= header.FrameCount)
	{
		GameAllocator<BYTE> alloc;
		using AllocTraits = std::allocator_traits<GameAllocator<BYTE>>;
		// write an empty image
		for (int pic = 0; pic < nFrameCount; pic++) {
			size_t capacity = static_cast<size_t>(header.Width) * static_cast<size_t>(header.Height);
			if (header.Width > 0 && header.Height > 0 && capacity > 0) {
				ppBuffer[pic] = AllocTraits::allocate(alloc, capacity);
				std::fill_n(ppBuffer[pic], capacity, 0);
			}
			else {
				ppBuffer[pic] = nullptr;
			}
		}
		return;
	}

	CShpFile::LoadFrame(nFrame, nFrameCount, ppBuffer);
}

void CLoadingExt::LoadBitMap(FString ImageID, const CBitmap& cBitmap)
{
	auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
	auto pData = CLoadingExt::GetSurfaceImageDataFromMap(ImageID);
	pData->lpSurface = CIsoViewExt::BitmapToSurface(pIsoView->lpDD7, cBitmap);
	DDSURFACEDESC2 desc;
	memset(&desc, 0, sizeof(DDSURFACEDESC2));
	desc.dwSize = sizeof(DDSURFACEDESC2);
	desc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
	pData->lpSurface->GetSurfaceDesc(&desc);
	pData->FullWidth = desc.dwWidth;
	pData->FullHeight = desc.dwHeight;
	pData->Flag = ImageDataFlag::SurfaceData;
	CIsoView::SetColorKey(pData->lpSurface, RGB(255, 255, 255));
	LoadedObjects.insert(ImageID);
}

void CLoadingExt::LoadShp(FString ImageID, FString FileName, FString PalName, int nFrame, bool toServer)
{
	auto loadingExt = (CLoadingExt*)CLoading::Instance();
	loadingExt->GetFullPaletteName(PalName);
	if (auto pal = PalettesManager::LoadPalette(PalName))
	{
		int nMix = loadingExt->SearchFile(FileName);
		if (loadingExt->HasFile(FileName, nMix))
		{
			ShapeHeader header;
			unsigned char* FramesBuffers;
			CMixFile::LoadSHP(FileName, nMix);
			CShpFile::GetSHPHeader(&header);
			CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &FramesBuffers, header);
			loadingExt->SetImageDataSafe(FramesBuffers, ImageID, header.Width, header.Height, pal, toServer);
			LoadedObjects.insert(ImageID);
		}
	}
}

void CLoadingExt::LoadShp(FString ImageID, FString FileName, Palette* pPal, int nFrame, bool toServer)
{
	if (pPal)
	{
		auto loadingExt = (CLoadingExt*)CLoading::Instance();
		int nMix = loadingExt->SearchFile(FileName);
		if (loadingExt->HasFile(FileName, nMix))
		{
			ShapeHeader header;
			unsigned char* FramesBuffers;
			CMixFile::LoadSHP(FileName, nMix);
			CShpFile::GetSHPHeader(&header);
			CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &FramesBuffers, header);
			loadingExt->SetImageDataSafe(FramesBuffers, ImageID, header.Width, header.Height, pPal, toServer);
			LoadedObjects.insert(ImageID);
		}
	}
}

void CLoadingExt::LoadShpToSurface(FString ImageID, FString FileName, FString PalName, int nFrame)
{
	auto loadingExt = (CLoadingExt*)CLoading::Instance();
	loadingExt->GetFullPaletteName(PalName);
	if (auto pal = PalettesManager::LoadPalette(PalName))
	{
		int nMix = loadingExt->SearchFile(FileName);
		if (loadingExt->HasFile(FileName, nMix))
		{
			ShapeHeader header;
			unsigned char* FramesBuffers;
			CMixFile::LoadSHP(FileName, nMix);
			CShpFile::GetSHPHeader(&header);
			CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &FramesBuffers, header);

			CBitmap bitmap;
			if (bitmap.CreateBitmap(header.Width, header.Height, 1, 32, NULL))
			{
				CDC memDC;
				memDC.CreateCompatibleDC(NULL);
				CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

				LOGPALETTE* pLogPalette = (LOGPALETTE*)malloc(sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY));
				pLogPalette->palVersion = 0x300;
				pLogPalette->palNumEntries = 256;

				for (int i = 0; i < 256; i++)
				{
					pLogPalette->palPalEntry[i].peRed = pal->Data[i].R;
					pLogPalette->palPalEntry[i].peGreen = pal->Data[i].G;
					pLogPalette->palPalEntry[i].peBlue = pal->Data[i].B;
					pLogPalette->palPalEntry[i].peFlags = pal->Data[i].Zero;
				}
				CPalette paletteObj;
				paletteObj.CreatePalette(pLogPalette);
				free(pLogPalette);
				CPalette* pOldPalette = memDC.SelectPalette(&paletteObj, FALSE);
				memDC.RealizePalette();
				for (int y = 0; y < header.Height; y++)
				{
					for (int x = 0; x < header.Width; x++)
					{
						memDC.SetPixel(x, y, PALETTEINDEX(FramesBuffers[y * header.Width + x]));
					}
				}
				memDC.SelectPalette(pOldPalette, FALSE);
				memDC.SelectObject(pOldBitmap);

				auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
				auto pData = CLoadingExt::GetSurfaceImageDataFromMap(ImageID);
				pData->lpSurface = CIsoViewExt::BitmapToSurface(pIsoView->lpDD7, bitmap);

				DDSURFACEDESC2 desc;
				memset(&desc, 0, sizeof(DDSURFACEDESC2));
				desc.dwSize = sizeof(DDSURFACEDESC2);
				desc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
				pData->lpSurface->GetSurfaceDesc(&desc);
				pData->FullWidth = desc.dwWidth;
				pData->FullHeight = desc.dwHeight;
				pData->Flag = ImageDataFlag::SurfaceData;

				CIsoView::SetColorKey(pData->lpSurface, -1);
				LoadedObjects.insert(ImageID);
				memDC.DeleteDC();
			}	
		}
	}
}

void CLoadingExt::LoadShpToSurface(FString ImageID, unsigned char* pBuffer, int Width, int Height, Palette* pPal)
{
	auto loadingExt = (CLoadingExt*)CLoading::Instance();
	CBitmap bitmap;
	if (bitmap.CreateBitmap(Width, Height, 1, 32, NULL))
	{
		CDC memDC;
		memDC.CreateCompatibleDC(NULL);
		CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

		LOGPALETTE* pLogPalette = (LOGPALETTE*)malloc(sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY));
		pLogPalette->palVersion = 0x300;
		pLogPalette->palNumEntries = 256;

		for (int i = 0; i < 256; i++)
		{
			pLogPalette->palPalEntry[i].peRed = pPal->Data[i].R;
			pLogPalette->palPalEntry[i].peGreen = pPal->Data[i].G;
			pLogPalette->palPalEntry[i].peBlue = pPal->Data[i].B;
			pLogPalette->palPalEntry[i].peFlags = pPal->Data[i].Zero;
		}
		CPalette paletteObj;
		paletteObj.CreatePalette(pLogPalette);
		free(pLogPalette);
		CPalette* pOldPalette = memDC.SelectPalette(&paletteObj, FALSE);
		memDC.RealizePalette();
		for (int y = 0; y < Height; y++)
		{
			for (int x = 0; x < Width; x++)
			{
				memDC.SetPixel(x, y, PALETTEINDEX(pBuffer[y * Width + x]));
			}
		}
		memDC.SelectPalette(pOldPalette, FALSE);
		memDC.SelectObject(pOldBitmap);

		auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
		auto pData = CLoadingExt::GetSurfaceImageDataFromMap(ImageID);
		pData->lpSurface = CIsoViewExt::BitmapToSurface(pIsoView->lpDD7, bitmap);

		DDSURFACEDESC2 desc;
		memset(&desc, 0, sizeof(DDSURFACEDESC2));
		desc.dwSize = sizeof(DDSURFACEDESC2);
		desc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
		pData->lpSurface->GetSurfaceDesc(&desc);
		pData->FullWidth = desc.dwWidth;
		pData->FullHeight = desc.dwHeight;
		pData->Flag = ImageDataFlag::SurfaceData;

		CIsoView::SetColorKey(pData->lpSurface, -1);
		LoadedObjects.insert(ImageID);
		memDC.DeleteDC();
	}	
}

bool CLoadingExt::LoadShpToBitmap(ImageDataClassSafe* pData, CBitmap& outBitmap)
{
	auto loadingExt = (CLoadingExt*)CLoading::Instance();
	if (pData->FullWidth == 0 || pData->FullHeight == 0)
	{
		if (outBitmap.CreateBitmap(32, 32, 1, 32, NULL))
		{
			CDC dc;
			dc.CreateCompatibleDC(NULL);
			CBitmap* pOldBitmap = dc.SelectObject(&outBitmap);
			dc.FillSolidRect(0, 0, 32, 32, RGB(255, 0, 255));
			dc.SelectObject(pOldBitmap);
			dc.DeleteDC();
			return true;
		}
		return false;
	}
	if (outBitmap.CreateBitmap(pData->FullWidth, pData->FullHeight, 1, 32, NULL))
	{
		CDC memDC;
		memDC.CreateCompatibleDC(NULL);
		CBitmap* pOldBitmap = memDC.SelectObject(&outBitmap);

		LOGPALETTE* pLogPalette = (LOGPALETTE*)malloc(sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY));
		pLogPalette->palVersion = 0x300;
		pLogPalette->palNumEntries = 256;

		// magenta bg
		pLogPalette->palPalEntry[0].peRed = 255;
		pLogPalette->palPalEntry[0].peGreen = 0;
		pLogPalette->palPalEntry[0].peBlue = 255;
		pLogPalette->palPalEntry[0].peFlags = 0;
		for (int i = 1; i < 256; i++)
		{
			// black hack
			if (pData->pPalette->Data[i].R == 0 && pData->pPalette->Data[i].G == 0 && pData->pPalette->Data[i].B == 0)
			{
				pLogPalette->palPalEntry[i].peRed = 1;
				pLogPalette->palPalEntry[i].peGreen = 1;
				pLogPalette->palPalEntry[i].peBlue = 1;
				pLogPalette->palPalEntry[i].peFlags = pData->pPalette->Data[i].Zero;
				continue;
			}
			
			pLogPalette->palPalEntry[i].peRed = pData->pPalette->Data[i].R;
			pLogPalette->palPalEntry[i].peGreen = pData->pPalette->Data[i].G;
			pLogPalette->palPalEntry[i].peBlue = pData->pPalette->Data[i].B;
			pLogPalette->palPalEntry[i].peFlags = pData->pPalette->Data[i].Zero;
		}
		CPalette paletteObj;
		paletteObj.CreatePalette(pLogPalette);
		free(pLogPalette);
		CPalette* pOldPalette = memDC.SelectPalette(&paletteObj, FALSE);
		memDC.RealizePalette();
		for (int y = 0; y < pData->FullHeight; y++)
		{
			for (int x = 0; x < pData->FullWidth; x++)
			{
				memDC.SetPixel(x, y, PALETTEINDEX(pData->pImageBuffer[y * pData->FullWidth + x]));
			}
		}
		memDC.SelectPalette(pOldPalette, FALSE);
		memDC.SelectObject(pOldBitmap);
		memDC.DeleteDC();
		return true;
	}	
	return false;
}

bool CLoadingExt::LoadShpToBitmap(ImageDataClass* pData, CBitmap& outBitmap)
{
	auto loadingExt = (CLoadingExt*)CLoading::Instance();
	if (pData->FullWidth == 0 || pData->FullHeight == 0)
	{
		if (outBitmap.CreateBitmap(32, 32, 1, 32, NULL))
		{
			CDC dc;
			dc.CreateCompatibleDC(NULL);
			CBitmap* pOldBitmap = dc.SelectObject(&outBitmap);
			dc.FillSolidRect(0, 0, 32, 32, RGB(255, 0, 255));
			dc.SelectObject(pOldBitmap);
			dc.DeleteDC();
			return true;
		}
		return false;
	}
	if (outBitmap.CreateBitmap(pData->FullWidth, pData->FullHeight, 1, 32, NULL))
	{
		CDC memDC;
		memDC.CreateCompatibleDC(NULL);
		CBitmap* pOldBitmap = memDC.SelectObject(&outBitmap);

		LOGPALETTE* pLogPalette = (LOGPALETTE*)malloc(sizeof(LOGPALETTE) + 256 * sizeof(PALETTEENTRY));
		pLogPalette->palVersion = 0x300;
		pLogPalette->palNumEntries = 256;

		// magenta bg
		pLogPalette->palPalEntry[0].peRed = 255;
		pLogPalette->palPalEntry[0].peGreen = 0;
		pLogPalette->palPalEntry[0].peBlue = 255;
		pLogPalette->palPalEntry[0].peFlags = 0;
		for (int i = 1; i < 256; i++)
		{
			// black hack
			if (pData->pPalette->Data[i].R == 0 && pData->pPalette->Data[i].G == 0 && pData->pPalette->Data[i].B == 0)
			{
				pLogPalette->palPalEntry[i].peRed = 1;
				pLogPalette->palPalEntry[i].peGreen = 1;
				pLogPalette->palPalEntry[i].peBlue = 1;
				pLogPalette->palPalEntry[i].peFlags = pData->pPalette->Data[i].Zero;
				continue;
			}
			
			pLogPalette->palPalEntry[i].peRed = pData->pPalette->Data[i].R;
			pLogPalette->palPalEntry[i].peGreen = pData->pPalette->Data[i].G;
			pLogPalette->palPalEntry[i].peBlue = pData->pPalette->Data[i].B;
			pLogPalette->palPalEntry[i].peFlags = pData->pPalette->Data[i].Zero;
		}
		CPalette paletteObj;
		paletteObj.CreatePalette(pLogPalette);
		free(pLogPalette);
		CPalette* pOldPalette = memDC.SelectPalette(&paletteObj, FALSE);
		memDC.RealizePalette();
		for (int y = 0; y < pData->FullHeight; y++)
		{
			for (int x = 0; x < pData->FullWidth; x++)
			{
				memDC.SetPixel(x, y, PALETTEINDEX(pData->pImageBuffer[y * pData->FullWidth + x]));
			}
		}
		memDC.SelectPalette(pOldPalette, FALSE);
		memDC.SelectObject(pOldBitmap);
		memDC.DeleteDC();
		return true;
	}	
	return false;
}

bool CLoadingExt::SaveCBitmapToFile(CBitmap* pBitmap, const FString& filePath, COLORREF bgColor)
{
	if (!pBitmap) return false;

	BITMAP bmp;
	pBitmap->GetBitmap(&bmp);
	if (bmp.bmBitsPixel != 32) return false; 

	int width = bmp.bmWidth;
	int height = bmp.bmHeight;

	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height; 
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	HDC hDC = GetDC(nullptr);
	void* pDIBPixels = nullptr;
	HBITMAP hDIB = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pDIBPixels, nullptr, 0);
	ReleaseDC(nullptr, hDC);

	if (!hDIB || !pDIBPixels) return false;

	HDC hMemDC = CreateCompatibleDC(nullptr);
	SelectObject(hMemDC, hDIB);
	HDC hSrcDC = CreateCompatibleDC(nullptr);
	SelectObject(hSrcDC, *pBitmap);
	BitBlt(hMemDC, 0, 0, width, height, hSrcDC, 0, 0, SRCCOPY);
	DeleteDC(hMemDC);
	DeleteDC(hSrcDC);

	std::vector<DWORD> pixels(width * height);
	GetDIBits(GetDC(nullptr), hDIB, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);

	BYTE bgR = GetRValue(bgColor);
	BYTE bgG = GetGValue(bgColor);
	BYTE bgB = GetBValue(bgColor);
	for (int i = 0; i < width * height; ++i)
	{
		BYTE alpha = (pixels[i] >> 24) & 0xFF;
		if (pixels[i] == 0x00000000)
		{
			pixels[i] = (0x00 << 24) | (bgR << 16) | (bgG << 8) | bgB;
		}
		else
		{
			pixels[i] &= 0x00FFFFFF;
			pixels[i] |= 0xFF << 24;
		}
	}

	BITMAPFILEHEADER bmfHeader;
	DWORD dwBmpSize = width * height * 4;
	DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	bmfHeader.bfType = 0x4D42;  // 'BM'
	bmfHeader.bfSize = dwSizeofDIB;
	bmfHeader.bfReserved1 = 0;
	bmfHeader.bfReserved2 = 0;
	bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	HANDLE hFile = CreateFile(filePath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) return false;

	DWORD dwWritten;
	WriteFile(hFile, &bmfHeader, sizeof(BITMAPFILEHEADER), &dwWritten, nullptr);
	WriteFile(hFile, &bmi.bmiHeader, sizeof(BITMAPINFOHEADER), &dwWritten, nullptr);
	WriteFile(hFile, pixels.data(), dwBmpSize, &dwWritten, nullptr);

	CloseHandle(hFile);
	DeleteObject(hDIB);

	return true;
}

bool CLoadingExt::LoadBMPToCBitmap(const FString& filePath, CBitmap& outBitmap)
{
	HBITMAP hBmp = (HBITMAP)::LoadImage(nullptr, filePath,
		IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
	if (!hBmp)
		return false;

	outBitmap.Attach(hBmp);
	return true;
}

void CLoadingExt::LoadOverlay(FString pRegName, int nIndex)
{
	if (pRegName == "")
		return;

	CFinalSunDlg::LastSucceededOperation = 11;

	Logger::Debug("CLoadingExt::LoadOverlay loading: %s\n", pRegName);
	if (!IsLoadingObjectView)
		LoadedOverlays.insert(pRegName);

	FString ArtID;
	FString ImageID;
	FString filename;
	int hMix = 0;

	FString palName = "iso\233AutoTinted";
	auto const typeData = CMapDataExt::GetOverlayTypeData(nIndex);
	Palette* palette = nullptr;

	palName = "iso\233AutoTinted";
	GetFullPaletteName(palName);
	palette = PalettesManager::LoadPalette(palName);

	FString lpOvrlName = pRegName;
	FString::TrimIndex(lpOvrlName);

	bool isveinhole = CINI::Rules->GetBool(lpOvrlName,"IsVeinholeMonster");
	bool istiberium = CINI::Rules->GetBool(lpOvrlName, "Tiberium");
	bool isveins = CINI::Rules->GetBool(lpOvrlName, "IsVeins");

	ArtID = GetArtID(lpOvrlName);
	ImageID = CINI::Art->GetString(ArtID, "Image", ArtID);

	if (TheaterIdentifier == 'T') filename = ArtID + ".tem";
	if (TheaterIdentifier == 'A') filename = ArtID + ".sno";
	if (TheaterIdentifier == 'U') filename = ArtID + ".urb";
	if (TheaterIdentifier == 'N') filename = ArtID + ".ubn";
	if (TheaterIdentifier == 'L') filename = ArtID + ".lun";
	if (TheaterIdentifier == 'D') filename = ArtID + ".des";

	bool findFile = false;
	hMix = SearchFile(filename);
	findFile = HasFile(filename);

	if (!findFile)
	{
		auto searchNewTheater = [&findFile, &hMix, this, &filename](char t)
			{
				if (!findFile)
				{
					filename.SetAt(1, t);
					hMix = SearchFile(filename);
					findFile = HasFile(filename);
				}
			};

		filename = ArtID + ".shp";

		palName = "unit";
		GetFullPaletteName(palName);
		palette = PalettesManager::LoadPalette(palName);

		if (strlen(ArtID) >= 2)
		{		
			if (!findFile)
			{
				SetTheaterLetter(filename, ExtConfigs::NewTheaterType ? 1 : 0);
				hMix = SearchFile(filename);
				findFile = HasFile(filename);
			}
			searchNewTheater('G');
			searchNewTheater(ArtID[1]);
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
		else
		{
			hMix = SearchFile(filename);
			findFile = HasFile(filename);
		}
	}
	auto searchOtherTheater = [this, &hMix, &filename, &ArtID, &findFile, &palette](const char* theater)
		{
			if (!findFile)
			{
				filename = ArtID + "." + theater;
				hMix = SearchFile(filename);
				findFile = HasFile(filename);
				if (findFile)
				{
					FString palName;
					palName.Format("iso%s.pal", theater);
					palette = PalettesManager::LoadPalette(palName);
				}
			}
		};
	if (!findFile)
	{
		searchOtherTheater("tem");
		searchOtherTheater("sno");
		searchOtherTheater("urb");
		searchOtherTheater("ubn");
		searchOtherTheater("lun");
		searchOtherTheater("des");
	}
	if (istiberium || isveinhole || isveins)
		palette = PalettesManager::LoadPalette("temperat.pal");

	if (findFile)
	{
		auto customPal = typeData.CustomPaletteName;
		if (strlen(customPal))
		{
			GetFullPaletteName(customPal);
			if (auto customPalette = PalettesManager::LoadPalette(customPal))
			{
				palette = customPalette;
			}
		}

		ShapeHeader header;
		unsigned char* FramesBuffers;
		if (CMixFile::LoadSHP(filename, hMix))
		{
			CShpFile::GetSHPHeader(&header);
			int nCount = std::min(header.FrameCount, (short)60);

			for (int i = 0; i < nCount; ++i)
			{
				if (IsLoadingObjectView && i != CViewObjectsExt::InsertingOverlayData)
					continue;

				ShapeImageHeader imageHeader;
				CShpFile::GetSHPImageHeader(i, &imageHeader);

				if (imageHeader.Unknown == 0 && !CINI::FAData->GetBool("Debug", "IgnoreSHPImageHeadUnused"))
					continue;

				CLoadingExt::LoadSHPFrameSafe(i, 1, &FramesBuffers, header);
				FString DictName = GetOverlayName(nIndex, i);
				SetImageDataSafe(FramesBuffers, DictName, header.Width, header.Height, palette, true, false);

				if (ExtConfigs::InGameDisplay_Shadow && (i < header.FrameCount / 2))
				{
					FString DictNameShadow = GetOverlayName(nIndex, i, true);
					unsigned char* pBufferShadow{ 0 };
					CLoadingExt::LoadSHPFrameSafe(i + header.FrameCount / 2, 1, &pBufferShadow, header);
					SetImageDataSafe(pBufferShadow, DictNameShadow, header.Width, header.Height, &CMapDataExt::Palette_Shadow);
				}
			}
		}	
	}
}