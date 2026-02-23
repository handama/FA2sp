#include "Body.h"

#include <CINI.h>
#include <CMixFile.h>
#include <iostream>
#include <fstream>
#include "../../Miscs/VoxelDrawer.h"
#include "../../Miscs/Palettes.h"
#include "../../FA2sp.h"
#include "../../Algorithms/Matrix3D.h"
#include "../CMapData/Body.h"
#include "../CFinalSunDlg/Body.h"
#include <random>
#include <chrono>

std::vector<CLoadingExt::SHPUnionData> CLoadingExt::UnionSHP_Data[2];
std::vector<CLoadingExt::SHPUnionData> CLoadingExt::UnionSHPShadow_Data[2];
std::unordered_map<FString, CLoadingExt::ObjectType> CLoadingExt::ObjectTypes;
std::unordered_set<FString> CLoadingExt::LoadedObjects;
std::unordered_set<FString> CLoadingExt::LoadedSurfaceObjects;
std::unordered_set<FString> CLoadingExt::CustomPaletteTerrains;
std::unordered_map<FString, int> CLoadingExt::AvailableFacings;
std::unordered_set<int> CLoadingExt::Ra2dotMixes;
unsigned char CLoadingExt::VXL_Data[0x10000] = {0};
unsigned char CLoadingExt::VXL_Shadow_Data[0x10000] = {0};
bool CLoadingExt::DrawTurretShadow = false;
std::unordered_set<FString> CLoadingExt::LoadedOverlays;
std::unordered_map<FString, InsigniaGrid> CLoadingExt::LoadedInsignias;
int CLoadingExt::TallestBuildingHeight = 0;

std::unordered_map<std::string, std::vector<unsigned char>> CLoadingExt::g_cache[2];
std::unordered_map<std::string, uint64_t> CLoadingExt::g_cacheTime[2];
uint64_t CLoadingExt::g_lastCleanup = 0;

std::unordered_map<FString, std::unique_ptr<ImageDataClassSafe>> CLoadingExt::CurrentFrameImageDataMap;
std::unordered_map<FString, std::unique_ptr<ImageDataClassSafe>> CLoadingExt::ImageDataMap;
std::unordered_map<FString, std::vector<std::unique_ptr<ImageDataClassSafe>>> CLoadingExt::BuildingClipsImageDataMap;
std::unordered_map<FString, std::unique_ptr<ImageDataClassSurface>> CLoadingExt::SurfaceImageDataMap;
std::map<COLORREF, std::unique_ptr<ImageDataClassSurface>> CLoadingExt::CustomFlagMap;
std::map<COLORREF, std::unique_ptr<ImageDataClassSurface>> CLoadingExt::CustomCelltagMap;
std::vector<std::unique_ptr<ImageDataClassSafe>> CLoadingExt::DamageFires;
std::map<unsigned int, MapCoord> CLoadingExt::TileExtraOffsets;
unsigned int CLoadingExt::RandomFireSeed = 0;

bool CLoadingExt::IsImageLoaded(const FString& name)
{
	auto itr = ImageDataMap.find(name);
	if (itr == ImageDataMap.end())
		return false;
	return itr->second->pImageBuffer != nullptr;
}

ImageDataClassSafe* CLoadingExt::GetImageDataFromMap(const FString& name, 
	ObjectType type, int facing, int totalFacings, bool shadow, bool* isDefault)
{
	auto itr = ImageDataMap.find(name);
	if (ExtConfigs::UseDefaultUnitImage &&
		(itr == ImageDataMap.end() || itr != ImageDataMap.end() && !itr->second->pImageBuffer) &&
		name.Find("FA2DEFAULT_") == -1)
	{
		if (type == ObjectType::Infantry)
		{
			if (isDefault) *isDefault = true;
			const auto& imageName = CLoadingExt::GetImageName("FA2DEFAULT_INFANTRY", facing, shadow);
			return GetImageDataFromMap(imageName);
		}
		else if (type == ObjectType::Vehicle)
		{
			if (isDefault) *isDefault = true;
			int newFacing = facing * 32 / totalFacings;
			const auto& imageName = CLoadingExt::GetImageName("FA2DEFAULT_UNIT", newFacing, shadow);
			return GetImageDataFromMap(imageName);
		}
		else if (type == ObjectType::Aircraft)
		{
			if (isDefault) *isDefault = true;
			int newFacing = facing * 32 / totalFacings;
			const auto& imageName = CLoadingExt::GetImageName("FA2DEFAULT_AIRCRAFT", newFacing, shadow);
			return GetImageDataFromMap(imageName);
		}
	}
	if (itr == ImageDataMap.end())
	{
		auto ret = std::make_unique<ImageDataClassSafe>();
		auto [it, inserted] = CLoadingExt::ImageDataMap.emplace(name, std::move(ret));
		return it->second.get();
	}
	return itr->second.get();
}

std::vector<std::unique_ptr<ImageDataClassSafe>>& CLoadingExt::GetBuildingClipImageDataFromMap(const FString& name)
{
	auto itr = BuildingClipsImageDataMap.find(name);
	if (itr == BuildingClipsImageDataMap.end())
	{
		auto pEmpty = std::make_unique<ImageDataClassSafe>();
		pEmpty->Flag = ImageDataFlag::SHP;
		pEmpty->IsOverlay = false;
		pEmpty->pPalette = Palette::PALETTE_UNIT;
		pEmpty->ClipOffsets.FullWidth = 0;
		pEmpty->ClipOffsets.LeftOffset = 0;

		auto& inserted = BuildingClipsImageDataMap[name];
		inserted.push_back(std::move(pEmpty));
		
		return inserted;
	}
	return itr->second;
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

FString CLoadingExt::GetImageName(const FString& ID, int nFacing, bool bShadow, bool bDeploy, bool bWater)
{
	FString ret;
	if (bShadow || bDeploy || bWater)
		ret.Format("%s\233%d\233%s%s%s", ID, nFacing, bDeploy ? "DEPLOY" : "", bWater ? "WATER" : "", bShadow ? "SHADOW" : "");
	else
		ret.Format("%s\233%d", ID, nFacing);
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
			ret.Format("%s\233%d\233DAMAGEDSHADOW", ID, nFacing);
		else
			ret.Format("%s\233%d\233DAMAGED", ID, nFacing);
	}
	else if (state == GBIN_RUBBLE)
	{
		if (bShadow)
		{
			if (Variables::RulesMap.GetBool(ID, "LeaveRubble"))
				ret.Format("%s\2330\233RUBBLESHADOW", ID);
			else if (!ExtConfigs::HideNoRubbleBuilding)// use damaged art, save memory
				ret.Format("%s\233%d\233DAMAGEDSHADOW", ID, nFacing);
			else // hide rubble
				ret = "\233\144\241"; // invalid string to get it empty
		}
		else
		{
			if (Variables::RulesMap.GetBool(ID, "LeaveRubble"))
				ret.Format("%s\2330\233RUBBLE", ID);
			else if (!ExtConfigs::HideNoRubbleBuilding)// use damaged art, save memory
				ret.Format("%s\233%d\233DAMAGED", ID, nFacing);
			else // hide rubble
				ret = "\233\144\241"; // invalid string to get it empty
		}

	}
	else // GBIN_NORMAL
	{
		if (bShadow)
			ret.Format("%s\233%d\233SHADOW", ID, nFacing);
		else
			ret.Format("%s\233%d", ID, nFacing);
	}
	return ret;
}

CLoadingExt::ObjectType CLoadingExt::GetItemType(FString ID)
{
	if (ID == "")
		return ObjectType::Unknown;
	else if (ID == "FA2DEFAULT_INFANTRY")
		return ObjectType::Infantry;
	else if (ID == "FA2DEFAULT_UNIT")
		return ObjectType::Vehicle;
	else if (ID == "FA2DEFAULT_AIRCRAFT")
		return ObjectType::Aircraft;
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

void CLoadingExt::LoadObjects(const FString& ID)
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

void CLoadingExt::ClearItemTypes(bool releaseNonsurfaces)
{
	if (releaseNonsurfaces)
	{
		ObjectTypes.clear();
		LoadedObjects.clear();
		LoadedOverlays.clear();
		SwimableInfantries.clear();
		ImageDataMap.clear();
		AvailableFacings.clear();
		CustomPaletteTerrains.clear();
		CMapDataExt::TerrainPaletteBuildings.clear();
		CMapDataExt::DamagedAsRubbleBuildings.clear();
		CMapDataExt::BuildingTypes.clear();
		BuildingClipsImageDataMap.clear();
		LoadedInsignias.clear();
		Logger::Debug("CLoadingExt: Clearing loaded objects.\n");
	}							    
	else {						    
		Logger::Debug("CLoadingExt: Clearing loaded objects with surfaces.\n");
	}
	LoadedSurfaceObjects.clear();
	CIsoViewExt::textCache.clear();
	for (auto& data : SurfaceImageDataMap)
		if (data.second->lpSurface)
			data.second->lpSurface->Release();
	for (auto& data : CustomFlagMap)
		if (data.second->lpSurface)
			data.second->lpSurface->Release();
	for (auto& data : CustomCelltagMap)
		if (data.second->lpSurface)
			data.second->lpSurface->Release();
	SurfaceImageDataMap.clear();
	CustomFlagMap.clear();
	CustomCelltagMap.clear();
}

bool CLoadingExt::IsObjectLoaded(const FString& pRegName)
{
	return LoadedObjects.find(pRegName) != LoadedObjects.end();
}

bool CLoadingExt::IsSurfaceObjectLoaded(const FString& pRegName)
{
	return LoadedSurfaceObjects.find(pRegName) != LoadedSurfaceObjects.end();
}

bool CLoadingExt::IsOverlayLoaded(const FString& pRegName)
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
	if (!HasFile(validator, nMix))
	{
		SetGenericTheaterLetter(ImageID);
		if (!ExtConfigs::UseStrictNewTheater)
		{
			validator = ImageID + ".SHP";
			nMix = this->SearchFile(validator);
			if (!HasFile(validator, nMix))
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

void CLoadingExt::ClipAndLoadBuilding(FString ID, FString ImageID, unsigned char* pBuffer,
	int width, int height, Palette* palette, unsigned char*& pAlphaBuffer)
{
	auto& ret = CLoadingExt::GetBuildingClipImageDataFromMap(ImageID);
	ret.clear();
	int idx = CMapDataExt::GetBuildingTypeIndex(ID);
	auto& DataExt = CMapDataExt::GetExtension()->BuildingDataExts[idx];
	CLoadingExt::TrimImageEdges(pBuffer, width, height, &pAlphaBuffer);
	int parts = DataExt.BottomCoords.size();
	if (parts == 1)
	{
		auto pData = SetBuildingImageDataSafe(pBuffer, ImageID, width, height, palette, pAlphaBuffer);
		pData->ClipOffsets.FullWidth = width;
		pData->ClipOffsets.LeftOffset = 0;
	}
	else
	{
		int bottomIdx = DataExt.Width - 1;
		std::vector<int> witdhs(parts, 0);
		std::vector<int> alphaWitdhs(parts, 0);
		std::vector<unsigned char*> pSplit(parts, 0);
		std::vector<unsigned char*> pAlphaSplit(parts, 0);
		int left = 0;
		for (int i = 0; i < parts; ++i)
		{
			auto& coord = DataExt.BottomCoords[i];
			int right = (coord.Y - coord.X) * 30 + width / 2;
			if (i >= bottomIdx)
				right += 30;
			if (i == parts - 1)
				right = width;
			right = std::max(0, right);
			pSplit[i] = ClipImageHorizontal(pBuffer, width, height, left, right, witdhs[i]);
			pAlphaSplit[i] = ClipImageHorizontal(pAlphaBuffer, width, height, left, right, alphaWitdhs[i]);
			auto pData = SetBuildingImageDataSafe(pSplit[i], ImageID, witdhs[i], height, palette, pAlphaSplit[i]);
			if (pAlphaSplit[i])
				GameDeleteArray(pAlphaSplit[i], alphaWitdhs[i] * height);
			pData->ClipOffsets.FullWidth = width;
			pData->ClipOffsets.LeftOffset = left;
			left = right;
		}
		GameDeleteArray(pBuffer, width * height);
	}
}

static bool IsPreOccupiedBunker(const FString& ID)
{
	bool isBunker = Variables::RulesMap.GetBool(ID, "CanBeOccupied");
	if (!isBunker) return false;

	FString firstInf;
	auto types = STDHelpers::SplitString(Variables::RulesMap.GetString(ID, "InitialPayload.Types"));
	if (types.empty()) return false;
	
	firstInf = types[0];

	auto eItemType = CLoadingExt::GetExtension()->GetItemType(firstInf);
	if (eItemType != CLoadingExt::ObjectType::Infantry) return false;

	return true;
}

static FString GetFinalLoopAnim(const FString& image)
{
	int loopCount = CINI::Art->GetInteger(image, "LoopCount", 1);
	if (loopCount < 0) return image;

	if (auto next = CINI::Art->TryGetString(image, "Next"))
	{
		return GetFinalLoopAnim(next->m_pchData);
	}
	return image;
}

void CLoadingExt::LoadBuilding(FString ID)
{
	if (IsLoadingObjectView)
	{
		LoadBuilding_Normal(ID);
		return;
	}

	LoadBuilding_Normal(ID);
	LoadBuilding_Damaged(ID);
	LoadBuilding_Rubble(ID);

	LoadInsignia(ID);
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
	bool bHasShadow = !Variables::RulesMap.GetBool(ID, "NoShadow") && CINI::Art->GetBool(ArtID, "Shadow", true);
	int facings = 
		(Variables::RulesMap.GetBool(ID, "TurretAnimIsVoxel") 
		|| Variables::RulesMap.GetBool(ID, "Turret")) ? (ExtConfigs::ExtFacings ? 32 : 8) : 1;
	AvailableFacings[ID] = facings;
	bool isPreOccupiedBunker = IsPreOccupiedBunker(ID);
	Palette* pMixedPal = nullptr;

	FString PaletteName = CINI::Art->GetString(ArtID, "Palette", "unit");
	if (CINI::Art->GetBool(ArtID, "TerrainPalette"))
	{
		PaletteName = "iso\233NotAutoTinted";
		CMapDataExt::TerrainPaletteBuildings.insert(ID);
	}
	GetFullPaletteName(PaletteName);
	auto palette = PalettesManager::LoadPalette(PaletteName);
	auto mainPalette = palette;

	auto loadBuildingFrameShape = [&](FString name, int nFrame = 0, int deltaX = 0, int deltaY = 0, bool shadow = false) -> bool
	{
		FString file = name + ".SHP";
		int nMix = SearchFile(file);
		// building can be displayed without the body
		if (!HasFile(file, nMix))
			return true;

		if (!CMixFile::LoadSHP(file, nMix))
			return true;

		ShapeHeader header;
		unsigned char* pBuffer;
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
		int deltaY = 0, FString customPal = "", bool shadow = false, int forceNewTheater = -1,
		bool isBib = false) -> bool
	{
		bool applyNewTheater = CINI::Art->GetBool(name, "NewTheater");
		int opacity = 255 - CINI::Art->GetInteger(name, "Translucency") * 256 / 100;
		opacity = std::clamp(opacity, 0, 255);
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
		if (!HasFile(file, nMix))
		{
			SetGenericTheaterLetter(file);
			nMix = SearchFile(file);
			if (!HasFile(file, nMix))
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
					if (!HasFile(file, nMix))
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
		if (!CMixFile::LoadSHP(file, nMix))
			return false;
		CShpFile::GetSHPHeader(&header);
		if (header.FrameCount <= nFrame) {
			nFrame = 0;
		}
		CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &pBuffer, header);

		Palette* thisPal = nullptr;
		bool hasNewPal = pMixedPal != nullptr;
		if (customPal != "")
		{
			thisPal = PalettesManager::LoadPalette(customPal);
			if (thisPal && !hasNewPal)
			{
				pMixedPal = CreateBalancedPalette(palette, thisPal);
				RemapImagePalette(pBuffer, header.Width, header.Height, thisPal, pMixedPal, false);
				for (auto& added : UnionSHP_Data[0])
				{
					RemapImagePalette(added.pBuffer, added.Width, added.Height, palette, pMixedPal, true);
				}
				palette = pMixedPal;
			}
		}
		if (hasNewPal)
		{
			RemapImagePalette(pBuffer, header.Width, header.Height, thisPal ? thisPal : mainPalette, pMixedPal, true);
		}

		CurrentLoadingAnim.Replace("Damaged", "");
		CurrentLoadingAnim.Replace("Garrisoned", "");
		UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, false,
			isBib ? 1000000000 : CINI::Art->GetInteger(ArtID, CurrentLoadingAnim + "ZAdjust"),
			isBib ? -1000000000 : CINI::Art->GetInteger(ArtID, CurrentLoadingAnim + "YSort"), false, opacity, thisPal == nullptr);

		if (shadow && ExtConfigs::InGameDisplay_Shadow)
		{
			CLoadingExt::LoadSHPFrameSafe(nFrame + header.FrameCount / 2, 1, &pBuffer, header);
			UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, true);
		}

		return true;
	};

	auto loadAnimFrameShape = [&](FString animkey, FString ignorekey = "")
	{
		CurrentLoadingAnim = animkey;
		if (auto pStr = CINI::Art->TryGetString(ArtID, animkey))
		{
			if (ignorekey.IsEmpty() || !CINI::FAData->GetBool(ignorekey, ID))
			{
				auto anim = GetFinalLoopAnim(*pStr);
				int nStartFrame = CINI::Art->GetInteger(anim, "LoopStart");
				FString customPal = "";
				if (!CINI::Art->GetBool(anim, "ShouldUseCellDrawer", true)) {
					customPal = CINI::Art->GetString(anim, "CustomPalette", "anim.pal");
					GetFullPaletteName(customPal);
				}
				int deltaX = CINI::Art->GetInteger(anim, "XDrawOffset");
				int deltaY = CINI::Art->GetInteger(anim, "YDrawOffset");
				if (animkey.Find("ActiveAnim") != -1 || animkey == "IdleAnim")
				{
					deltaX += CINI::Art->GetInteger(ArtID, animkey + "X");
					deltaY += CINI::Art->GetInteger(ArtID, animkey + "Y");
				}

				loadSingleFrameShape(anim, nStartFrame, deltaX, deltaY, customPal, CINI::Art->GetBool(anim, "Shadow"));
			}
		}
	};

	if (auto ppPowerUpBld = Variables::RulesMap.TryGetString(ID, "PowersUpBuilding")) // Early load
	{
		if (!CLoadingExt::IsObjectLoaded(*ppPowerUpBld))
			LoadBuilding(*ppPowerUpBld);
	}

	int nBldStartFrame = CINI::Art->GetInteger(ArtID, "LoopStart", 0) + (isPreOccupiedBunker ? 2 : 0);

	if (Variables::RulesMap.GetBool(ID, "Gate"))
	{
		nBldStartFrame = 0;
	}

	FString AnimKeys[9] = 
	{	
		"IdleAnim",
		isPreOccupiedBunker ? "ActiveAnimGarrisoned": "ActiveAnim",
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
	if (auto pStr = CINI::Art->TryGetString(ArtID, "BibShape")) {
		loadSingleFrameShape(*pStr, 0, 0, 0, "", bHasShadow, 1, true);
	}
	loadBuildingFrameShape(ImageID, nBldStartFrame, 0, 0, bHasShadow);
	for (int i = 0; i < 9; ++i)
	{
		loadAnimFrameShape(AnimKeys[i], IgnoreKeys[i]);
	}

	FString DictName;

	unsigned char* pBuffer = nullptr;
	int width, height;
	bool hasTranspaernt = false;
	unsigned char* pOpacityBuffer = nullptr;
	for (auto& data : UnionSHP_Data[0])
		if (data.Opacity != 255) 
			hasTranspaernt = true;

	UnionSHP_GetAndClear(pBuffer, &width, &height, false, false, true,
		hasTranspaernt ? &pOpacityBuffer : nullptr, palette);

	FString DictNameShadow;
	unsigned char* pBufferShadow{ 0 };
	int widthShadow, heightShadow;
	if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
		UnionSHP_GetAndClear(pBufferShadow, &widthShadow, &heightShadow, false, true);

	if (Variables::RulesMap.GetBool(ID, "TurretAnimIsVoxel")) // Voxel turret
	{
		FString TurName = Variables::RulesMap.GetString(ID, "TurretAnim", ID + "tur");
		FString BarlName = ID + "barl";
		bool hasBarl = false;
		int fireAngle = Variables::RulesMap.GetInteger(ID, "FireAngle", 10);

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
				hasBarl = true;
				for (int i = 0; i < facings; ++i)
				{
					bool result = VoxelDrawer::GetImageData((facings + 5 * facings / 8 - i) % facings,
						pBarlImages[i], barlrect[i], 0, 0, 0, false, fireAngle);
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
				TurName.MakeLower();
				auto nameContainsTur = TurName.Find("tur");
				for (int i = 0; i < facings; ++i)
				{
					bool result = VoxelDrawer::GetImageData((facings + 5 * facings / 8 - i) % facings,
						pTurImages[i], turrect[i], 0, 0, 0, false,
						(hasBarl || nameContainsTur > 0) ? 0 : fireAngle);
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

				bool barrelInFront = IsBarrelInFront((7 * facings / 8 - i + facings) % facings, facings);

				if (barrelInFront)
				{
					VXL_Add(pTurImages[i], turrect[i].X + turdeltaX, turrect[i].Y + turdeltaY, turrect[i].W, turrect[i].H);
					CncImgFree(pTurImages[i]);
				}

				if (pBarlImages[i])
				{
					pKey.Format("%sX%d", ID, (15 - i * 8 / facings) % 8);
					int barldeltaX = CINI::FAData->GetInteger("BuildingVoxelBarrelsRA2", pKey);
					pKey.Format("%sY%d", ID, (15 - i * 8 / facings) % 8);
					int barldeltaY = CINI::FAData->GetInteger("BuildingVoxelBarrelsRA2", pKey);

					VXL_Add(pBarlImages[i], barlrect[i].X + barldeltaX, barlrect[i].Y + barldeltaY, barlrect[i].W, barlrect[i].H);
					CncImgFree(pBarlImages[i]);
				}

				if (!barrelInFront)
				{
					VXL_Add(pTurImages[i], turrect[i].X + turdeltaX, turrect[i].Y + turdeltaY, turrect[i].W, turrect[i].H);
					CncImgFree(pTurImages[i]);
				}
			}

			int nW = 0x100, nH = 0x100;
			VXL_GetAndClear(pTurImages[i], &nW, &nH);

			UnionSHP_Add(pTurImages[i], 0x100, 0x100, deltaX, deltaY);

			unsigned char* pImage;
			int width1, height1;

			unsigned char* pAlphaImage = nullptr;
			if (hasTranspaernt)
			{
				pAlphaImage = GameCreateArray<unsigned char>(width * height);
				memcpy(pAlphaImage, pOpacityBuffer, width * height);
			}
			UnionSHP_GetAndClear(pImage, &width1, &height1, false, false, false, 
				hasTranspaernt ? &pAlphaImage : nullptr, palette);
			DictName.Format("%s\233%d", ID, i);
			ClipAndLoadBuilding(ID, DictName, pImage, width1, height1, palette, pAlphaImage);
			if (pAlphaImage)
				GameDeleteArray(pAlphaImage, width * height);
		}

		if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
		{
			DictNameShadow.Format("%s\233%d\233SHADOW", ID, 0);
			SetImageDataSafe(pBufferShadow, DictNameShadow, widthShadow, heightShadow, &CMapDataExt::Palette_Shadow);
		}

		GameDeleteArray(pBuffer, width * height);
	} 
	else if (Variables::RulesMap.GetBool(ID, "Turret")) // Shape turret
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
			unsigned char* pAlphaImage = nullptr;
			if (hasTranspaernt)
			{
				pAlphaImage = GameCreateArray<unsigned char>(width * height);
				memcpy(pAlphaImage, pOpacityBuffer, width * height);
			}
			UnionSHP_GetAndClear(pImage, &width1, &height1, false, false, false,
				hasTranspaernt ? &pAlphaImage : nullptr, palette);

			DictName.Format("%s\233%d", ID, i);
			ClipAndLoadBuilding(ID, DictName, pImage, width1, height1, palette, pAlphaImage);
			if (pAlphaImage)
				GameDeleteArray(pAlphaImage, width * height);

			if (shadow)
			{
				FString DictNameShadow;
				unsigned char* pImageShadow;
				int width1Shadow, height1Shadow;
				UnionSHP_GetAndClear(pImageShadow, &width1Shadow, &height1Shadow, false, true);
				DictNameShadow.Format("%s\233%d\233SHADOW", ID, i);
				SetImageDataSafe(pImageShadow, DictNameShadow, width1Shadow, height1Shadow, &CMapDataExt::Palette_Shadow);
			}
		}
		GameDelete(pBuffer);
		GameDelete(pBufferShadow);
	}
	else // No turret
	{
		DictName.Format("%s\233%d", ID, 0);
		ClipAndLoadBuilding(ID, DictName, pBuffer, width, height, palette, pOpacityBuffer);
		if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
		{
			DictNameShadow.Format("%s\233%d\233SHADOW", ID, 0);
			SetImageDataSafe(pBufferShadow, DictNameShadow, widthShadow, heightShadow, &CMapDataExt::Palette_Shadow);
		}
	}
	if (pOpacityBuffer)
		GameDelete(pOpacityBuffer);
}

void CLoadingExt::LoadBuilding_Damaged(FString ID, bool loadAsRubble)
{
	FString ArtID = GetArtID(ID);
	FString ImageID = GetBuildingFileID(ID);
	FString CurrentLoadingAnim;
	bool bHasShadow = !Variables::RulesMap.GetBool(ID, "NoShadow") && CINI::Art->GetBool(ArtID, "Shadow", true);
	int facings =
		(Variables::RulesMap.GetBool(ID, "TurretAnimIsVoxel")
			|| Variables::RulesMap.GetBool(ID, "Turret")) ? (ExtConfigs::ExtFacings ? 32 : 8) : 1;
	AvailableFacings[ID] = facings;
	bool isPreOccupiedBunker = IsPreOccupiedBunker(ID);
	int techLevel = Variables::RulesMap.GetInteger(ID, "TechLevel");
	Palette* pMixedPal = nullptr;

	FString PaletteName = CINI::Art->GetString(ArtID, "Palette", "unit");
	if (CINI::Art->GetBool(ArtID, "TerrainPalette"))
	{
		PaletteName = "iso\233NotAutoTinted";
		CMapDataExt::TerrainPaletteBuildings.insert(ID);
	}
	GetFullPaletteName(PaletteName);
	auto palette = PalettesManager::LoadPalette(PaletteName);
	auto mainPalette = palette;

	auto loadBuildingFrameShape = [&](FString name, int nFrame = 0, int deltaX = 0, int deltaY = 0, bool shadow = false) -> bool
	{
		FString file = name + ".SHP";
		int nMix = SearchFile(file);
		// building can be displayed without the body
		if (!HasFile(file, nMix))
			return true;

		if (!CMixFile::LoadSHP(file, nMix))
			return true;

		ShapeHeader header;
		unsigned char* pBuffer;
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
		int deltaY = 0, FString customPal = "", bool shadow = false, int forceNewTheater = -1,
		bool isBib = false) -> bool
	{
			bool applyNewTheater = CINI::Art->GetBool(name, "NewTheater");
			int opacity = 255 - CINI::Art->GetInteger(name, "Translucency") * 256 / 100;
			opacity = std::clamp(opacity, 0, 255);
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
			if (!HasFile(file, nMix))
			{
				SetGenericTheaterLetter(file);
				nMix = SearchFile(file);
				if (!HasFile(file, nMix))
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
						if (!HasFile(file, nMix))
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
		if (!CMixFile::LoadSHP(file, nMix))
			return false;
		CShpFile::GetSHPHeader(&header);
		if (header.FrameCount <= nFrame) {
			nFrame = 0;
		}
		CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &pBuffer, header);

		Palette* thisPal = nullptr;
		bool hasNewPal = pMixedPal != nullptr;
		if (customPal != "")
		{
			thisPal = PalettesManager::LoadPalette(customPal);
			if (thisPal && !hasNewPal)
			{
				pMixedPal = CreateBalancedPalette(palette, thisPal);
				RemapImagePalette(pBuffer, header.Width, header.Height, thisPal, pMixedPal, false);
				for (auto& added : UnionSHP_Data[0])
				{
					RemapImagePalette(added.pBuffer, added.Width, added.Height, palette, pMixedPal, true);
				}
				palette = pMixedPal;
			}
		}
		if (hasNewPal)
		{
			RemapImagePalette(pBuffer, header.Width, header.Height, thisPal ? thisPal : mainPalette, pMixedPal, true);
		}

		CurrentLoadingAnim.Replace("Damaged", "");
		CurrentLoadingAnim.Replace("Garrisoned", "");
		UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, false,
			isBib ? 1000000000 : CINI::Art->GetInteger(ArtID, CurrentLoadingAnim + "ZAdjust"),
			isBib ? -1000000000 : CINI::Art->GetInteger(ArtID, CurrentLoadingAnim + "YSort"), false, opacity, thisPal == nullptr);

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
				auto anim = GetFinalLoopAnim(*pStr);
				int nStartFrame = CINI::Art->GetInteger(anim, "LoopStart");
				FString customPal = "";
				if (!CINI::Art->GetBool(anim, "ShouldUseCellDrawer", true)) {
					customPal = CINI::Art->GetString(anim, "CustomPalette", "anim.pal");
					GetFullPaletteName(customPal);
				}
				int deltaX = CINI::Art->GetInteger(anim, "XDrawOffset");
				int deltaY = CINI::Art->GetInteger(anim, "YDrawOffset");
				if (animkey.Find("ActiveAnim") != -1 || animkey == "IdleAnim")
				{
					deltaX += CINI::Art->GetInteger(ArtID, animkey + "X");
					deltaY += CINI::Art->GetInteger(ArtID, animkey + "Y");
				}

				loadSingleFrameShape(anim, nStartFrame, deltaX, deltaY, customPal, CINI::Art->GetBool(anim, "Shadow"));
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
					GetFullPaletteName(customPal);
				}
				loadSingleFrameShape(*pStr, nStartFrame, 0, 0, customPal);
			}
		}
	};

	int nBldStartFrame = CINI::Art->GetInteger(ArtID, "LoopStart", 0) + 1 
		+ ((isPreOccupiedBunker && techLevel > -1) ? 2 : 0);
	if (Variables::RulesMap.GetBool(ID, "Wall"))
	{
		nBldStartFrame--;
	}
	else if (Variables::RulesMap.GetBool(ID, "Gate"))
	{
		nBldStartFrame = CINI::Art->GetInteger(ArtID, "GateStages", 0) + 1;
	}

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
	if (auto pStr = CINI::Art->TryGetString(ArtID, "BibShape")) {
		loadSingleFrameShape(*pStr, 1, 0, 0, "", bHasShadow, 1, true);
	}
	loadBuildingFrameShape(ImageID, nBldStartFrame, 0, 0, bHasShadow);
	for (int i = 0; i < 9; ++i)
	{
		loadAnimFrameShape(AnimKeys[i], IgnoreKeys[i]);
	}

	FString DictName;

	unsigned char* pBuffer = nullptr;
	int width, height;
	bool hasTranspaernt = false;
	unsigned char* pOpacityBuffer = nullptr;
	for (auto& data : UnionSHP_Data[0])
		if (data.Opacity != 255)
			hasTranspaernt = true;

	UnionSHP_GetAndClear(pBuffer, &width, &height, false, false, true,
		hasTranspaernt ? &pOpacityBuffer : nullptr, palette);

	FString DictNameShadow;
	unsigned char* pBufferShadow{ 0 };
	int widthShadow, heightShadow;
	if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
		UnionSHP_GetAndClear(pBufferShadow, &widthShadow, &heightShadow, false, true);

	if (Variables::RulesMap.GetBool(ID, "TurretAnimIsVoxel")) // Voxel turret
	{
		FString TurName = Variables::RulesMap.GetString(ID, "TurretAnim", ID + "tur");
		FString BarlName = ID + "barl";
		int fireAngle = Variables::RulesMap.GetInteger(ID, "FireAngle", 10);
		bool hasBarl = false;

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
				hasBarl = true;
				for (int i = 0; i < facings; ++i)
				{
					bool result = VoxelDrawer::GetImageData((facings + 5 * facings / 8 - i) % facings,
						pBarlImages[i], barlrect[i], 0, 0, 0, false, fireAngle);
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
				TurName.MakeLower();
				auto nameContainsTur = TurName.Find("tur");
				for (int i = 0; i < facings; ++i)
				{
					bool result = VoxelDrawer::GetImageData((facings + 5 * facings / 8 - i) % facings,
						pTurImages[i], turrect[i], 0, 0, 0, false,
						(hasBarl || nameContainsTur > 0) ? 0 : fireAngle);
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

				bool barrelInFront = IsBarrelInFront((7 * facings / 8 - i + facings) % facings, facings);

				if (barrelInFront)
				{
					VXL_Add(pTurImages[i], turrect[i].X + turdeltaX, turrect[i].Y + turdeltaY, turrect[i].W, turrect[i].H);
					CncImgFree(pTurImages[i]);
				}

				if (pBarlImages[i])
				{
					pKey.Format("%sX%d", ID, (15 - i * 8 / facings) % 8);
					int barldeltaX = CINI::FAData->GetInteger("BuildingVoxelBarrelsRA2", pKey);
					pKey.Format("%sY%d", ID, (15 - i * 8 / facings) % 8);
					int barldeltaY = CINI::FAData->GetInteger("BuildingVoxelBarrelsRA2", pKey);

					VXL_Add(pBarlImages[i], barlrect[i].X + barldeltaX, barlrect[i].Y + barldeltaY, barlrect[i].W, barlrect[i].H);
					CncImgFree(pBarlImages[i]);
				}

				if (!barrelInFront)
				{
					VXL_Add(pTurImages[i], turrect[i].X + turdeltaX, turrect[i].Y + turdeltaY, turrect[i].W, turrect[i].H);
					CncImgFree(pTurImages[i]);
				}
			}

			int nW = 0x100, nH = 0x100;
			VXL_GetAndClear(pTurImages[i], &nW, &nH);

			UnionSHP_Add(pTurImages[i], 0x100, 0x100, deltaX, deltaY);

			unsigned char* pImage;
			int width1, height1;
			unsigned char* pAlphaImage = nullptr;
			if (hasTranspaernt)
			{
				pAlphaImage = GameCreateArray<unsigned char>(width * height);
				memcpy(pAlphaImage, pOpacityBuffer, width * height);
			}

			UnionSHP_GetAndClear(pImage, &width1, &height1, false, false, false,
				hasTranspaernt ? &pAlphaImage : nullptr, palette);
			if (loadAsRubble)
				DictName.Format("%s\233%d\233RUBBLE", ID, i);
			else
				DictName.Format("%s\233%d\233DAMAGED", ID, i);
			ClipAndLoadBuilding(ID, DictName, pImage, width1, height1, palette, pAlphaImage);

			if (pAlphaImage)
				GameDeleteArray(pAlphaImage, width * height);
		}

		if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
		{
			if (loadAsRubble)
				DictNameShadow.Format("%s\233%d\233RUBBLESHADOW", ID, 0);
			else
				DictNameShadow.Format("%s\233%d\233DAMAGEDSHADOW", ID, 0);
			SetImageDataSafe(pBufferShadow, DictNameShadow, widthShadow, heightShadow, &CMapDataExt::Palette_Shadow);
		}

		GameDeleteArray(pBuffer, width * height);
	}
	else if (Variables::RulesMap.GetBool(ID, "Turret")) // Shape turret
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
			unsigned char* pAlphaImage = nullptr;
			if (hasTranspaernt)
			{
				pAlphaImage = GameCreateArray<unsigned char>(width * height);
				memcpy(pAlphaImage, pOpacityBuffer, width * height);
			}
			UnionSHP_GetAndClear(pImage, &width1, &height1, false, false, false,
				hasTranspaernt ? &pAlphaImage : nullptr, palette);

			if (loadAsRubble)
				DictName.Format("%s\233%d\233RUBBLE", ID, i);
			else
				DictName.Format("%s\233%d\233DAMAGED", ID, i); 
			ClipAndLoadBuilding(ID, DictName, pImage, width1, height1, palette, pAlphaImage);
			if (pAlphaImage)
				GameDeleteArray(pAlphaImage, width * height);

			if (shadow)
			{
				FString DictNameShadow;
				unsigned char* pImageShadow;
				int width1Shadow, height1Shadow;
				UnionSHP_GetAndClear(pImageShadow, &width1Shadow, &height1Shadow, false, true);
				if (loadAsRubble)
					DictNameShadow.Format("%s\233%d\233RUBBLESHADOW", ID, i);
				else
					DictNameShadow.Format("%s\233%d\233DAMAGEDSHADOW", ID, i);
				SetImageDataSafe(pImageShadow, DictNameShadow, width1Shadow, height1Shadow, &CMapDataExt::Palette_Shadow);
			}
		}
		GameDelete(pBuffer);
		GameDelete(pBufferShadow);

	}
	else // No turret
	{
		if (loadAsRubble)
			DictName.Format("%s\233%d\233RUBBLE", ID, 0);
		else
			DictName.Format("%s\233%d\233DAMAGED", ID, 0);
		ClipAndLoadBuilding(ID, DictName, pBuffer, width, height, palette, pOpacityBuffer);

		if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
		{
			if (loadAsRubble)
				DictNameShadow.Format("%s\233%d\233RUBBLESHADOW", ID, 0);
			else
				DictNameShadow.Format("%s\233%d\233DAMAGEDSHADOW", ID, 0);
			SetImageDataSafe(pBufferShadow, DictNameShadow, widthShadow, heightShadow, &CMapDataExt::Palette_Shadow);
		}
	}
	if (pOpacityBuffer)
		GameDelete(pOpacityBuffer);
}

void CLoadingExt::LoadBuilding_Rubble(FString ID)
{
	FString ArtID = GetArtID(ID);
	FString ImageID = GetBuildingFileID(ID);
	bool bHasShadow = !Variables::RulesMap.GetBool(ID, "NoShadow") && CINI::Art->GetBool(ArtID, "Shadow", true);
	FString PaletteName = "iso\233NotAutoTinted";
	PaletteName = CINI::Art->GetString(ArtID, "RubblePalette", PaletteName);
	GetFullPaletteName(PaletteName);
	auto pal = PalettesManager::LoadPalette(PaletteName);

	auto loadBuildingFrameShape = [&](FString name, int nFrame = 0, int deltaX = 0, int deltaY = 0, bool shadow = false) -> bool
	{
		FString file = name + ".SHP";
		int nMix = SearchFile(file);
		// building can be displayed without the body
		if (!HasFile(file, nMix))
			return true;

		if (!CMixFile::LoadSHP(file, nMix))
			return true;

		ShapeHeader header;
		unsigned char* pBuffer;
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
		int deltaY = 0, bool shadow = false, int forceNewTheater = -1,
		bool isBib = false) -> bool
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
			if (!HasFile(file, nMix))
			{
				SetGenericTheaterLetter(file);
				nMix = SearchFile(file);
				if (!HasFile(file, nMix))
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
						if (!HasFile(file, nMix))
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
		if (!CMixFile::LoadSHP(file, nMix))
			return false;
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

	if (Variables::RulesMap.GetBool(ID, "LeaveRubble"))
	{
		int nBldStartFrame = CINI::Art->GetInteger(ArtID, "LoopStart", 0) + 3;
		if (loadBuildingFrameShape(ImageID, nBldStartFrame, 0, 0, bHasShadow))
		{
			unsigned char* pBuffer;
			int width, height;
			UnionSHP_GetAndClear(pBuffer, &width, &height);

			FString DictName = ID + "\2330\233RUBBLE";
			unsigned char* tmp = nullptr;
			ClipAndLoadBuilding(ID, DictName, pBuffer, width, height, pal, tmp);

			if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
			{
				FString DictNameShadow;
				unsigned char* pBufferShadow{ 0 };
				int widthShadow, heightShadow;
				UnionSHP_GetAndClear(pBufferShadow, &widthShadow, &heightShadow, false, true);
				DictNameShadow.Format("%s\233%d\233RUBBLESHADOW", ID, 0);
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
	LoadInsignia(ID);
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
	if (HasFile(FileName, nMix))
	{
		ShapeHeader header;
		unsigned char* FramesBuffers;
		if (!CMixFile::LoadSHP(FileName, nMix))
			return;

		CShpFile::GetSHPHeader(&header);
		for (int i = 0; i < 8; ++i)
		{
			if (IsLoadingObjectView && i != 5)
				continue;

			CLoadingExt::LoadSHPFrameSafe(framesToRead[i], 1, &FramesBuffers, header);
			FString DictName;
			DictName.Format("%s\233%d", ID, i);
			SetImageDataSafe(FramesBuffers, DictName, header.Width, header.Height, pal);

			if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
			{
				FString DictNameShadow;
				unsigned char* pBufferShadow{ 0 };
				DictNameShadow.Format("%s\233%d\233SHADOW", ID, i);
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
				DictNameDeploy.Format("%s\233%d\233DEPLOY", ID, i);
				SetImageDataSafe(FramesBuffersDeploy, DictNameDeploy, header.Width, header.Height, pal);

				if (bHasShadow && ExtConfigs::InGameDisplay_Shadow)
				{
					FString DictNameShadow;
					unsigned char* pBufferShadow{ 0 };
					DictNameShadow.Format("%s\233%d\233DEPLOYSHADOW", ID, i);
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
				DictNameWater.Format("%s\233%d\233WATER", ID, i);
				SetImageDataSafe(FramesBuffersWater, DictNameWater, header.Width, header.Height, pal);

				if (ExtConfigs::InGameDisplay_Shadow)
				{
					FString DictNameShadow;
					unsigned char* pBufferShadow{ 0 };
					DictNameShadow.Format("%s\233%d\233WATERSHADOW", ID, i);
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
	if (HasFile(FileName, nMix))
	{
		ShapeHeader header;
		unsigned char* FramesBuffers[1];
		if (!CMixFile::LoadSHP(FileName, nMix))
			return;
		CShpFile::GetSHPHeader(&header);
		CLoadingExt::LoadSHPFrameSafe(0, 1, &FramesBuffers[0], header);
		FString DictName;
		DictName.Format("%s\233%d", ID, 0);
		FString PaletteName = CINI::Art->GetString(ArtID, "Palette", "iso");
		if (!CINI::Art->KeyExists(ArtID, "Palette") && Variables::RulesMap.GetBool(ID, "SpawnsTiberium"))
		{
			PaletteName = "unitsno.pal";
		}
		if (CINI::Art->KeyExists(ArtID, "Palette") || Variables::RulesMap.GetBool(ID, "SpawnsTiberium"))
		{
			CustomPaletteTerrains.insert(ID);
		}
		PaletteName.MakeUpper();
		GetFullPaletteName(PaletteName);
		SetImageDataSafe(FramesBuffers[0], DictName, header.Width, header.Height, PalettesManager::LoadPalette(PaletteName));

		if (ExtConfigs::InGameDisplay_Shadow && terrain)
		{
			FString DictNameShadow;
			unsigned char* pBufferShadow[1];
			DictNameShadow.Format("%s\233%d\233SHADOW", ID, 0);
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

InsigniaGrid CLoadingExt::GetInsignia(const FString& ID)
{
	auto itr = LoadedInsignias.find(ID);
	if (itr != LoadedInsignias.end())
	{
		return itr->second;
	}
	return {};
}

void CLoadingExt::LoadInsignia(FString ID)
{
	const char* PaletteName = "palette.pal";

	auto InsigniaRookie = Variables::RulesMap.GetString(ID, "Insignia.Rookie");
	auto InsigniaVeteran = Variables::RulesMap.GetString(ID, "Insignia.Veteran");
	auto InsigniaElite = Variables::RulesMap.GetString(ID, "Insignia.Elite");
	int InsigniaIndex = Variables::RulesMap.GetInteger(ID, "InsigniaFrame");

	InsigniaGrid ret;

	if (!InsigniaRookie.IsEmpty())
	{
		InsigniaRookie += ".shp";
		ret.Rookie = ID + "\233Insignia.Rookie";
		CLoadingExt::LoadShp(ret.Rookie, InsigniaRookie, PaletteName,
			Variables::RulesMap.GetInteger(ID, "InsigniaFrame.Rookie", InsigniaIndex));
	}
	if (!InsigniaVeteran.IsEmpty())
	{
		InsigniaVeteran += ".shp";
		ret.Veteran = ID + "\233Insignia.Veteran";
		CLoadingExt::LoadShp(ret.Veteran, InsigniaVeteran, PaletteName,
			Variables::RulesMap.GetInteger(ID, "InsigniaFrame.Veteran", InsigniaIndex));
	}
	if (!InsigniaElite.IsEmpty())
	{
		InsigniaElite += ".shp";
		ret.Elite = ID + "\233Insignia.Elite";
		CLoadingExt::LoadShp(ret.Elite, InsigniaElite, PaletteName,
			Variables::RulesMap.GetInteger(ID, "InsigniaFrame.Elite", InsigniaIndex));
	}
	if (!ret.Rookie.IsEmpty() || !ret.Veteran.IsEmpty() || !ret.Elite.IsEmpty())
	{
		LoadedInsignias[ID] = ret;
	}
}

static int GetIFVTurretIndex(const char* infantrySection, const char* ifvSection)
{
	if (!infantrySection || !ifvSection || !*infantrySection || !*ifvSection)
		return -1;

	int ifvMode = Variables::RulesMap.GetInteger(infantrySection, "IFVMode", 0);
	if (ifvMode < 0)
		return -1;

	int weaponSlot = ifvMode + 1; 
	if (weaponSlot > 128) 
		return -1;

	bool isGunner = Variables::RulesMap.GetBool(ifvSection, "Gunner", false);

	if (!isGunner)
		return -1;

	// ares mode
	char keyName[64];
	_snprintf_s(keyName, sizeof(keyName), "WeaponTurretIndex%d", weaponSlot);

	int turretIndex = Variables::RulesMap.GetInteger(ifvSection, keyName, -1);
	if (turretIndex >= 0)
		return turretIndex;

	const char* prefixList[] = {
		"Normal", "Repair", "MachineGun", "Sniper", "Explode", "TerroristExplode",
		"Chrono", "Pistol", "BrainBlast", "Flak", "Shock", "RadCannon", "Cow",
		"Initiate", "Virus", "YuriPrime", "Guardian",
		nullptr
	};

	const char* foundPrefix = nullptr;

	for (int i = 0; prefixList[i] != nullptr; ++i)
	{
		const char* prefix = prefixList[i];
		char weaponKey[64];
		_snprintf_s(weaponKey, sizeof(weaponKey), "%sTurretWeapon", prefix);

		int value = Variables::RulesMap.GetInteger(ifvSection, weaponKey, -1);
		if (value == ifvMode)
		{
			foundPrefix = prefix;
			break;
		}
	}

	if (!foundPrefix)
		return -1; 

	char indexKey[64];
	_snprintf_s(indexKey, sizeof(indexKey), "%sTurretIndex", foundPrefix);

	turretIndex = Variables::RulesMap.GetInteger(ifvSection, indexKey, -1);
	if (turretIndex < 0)
		return -1;

	return turretIndex;
}

void CLoadingExt::LoadVehicleOrAircraft(FString ID)
{
	LoadInsignia(ID);
	FString ArtID = GetArtID(ID);
	FString ImageID = GetVehicleOrAircraftFileID(ID);
	bool bHasTurret = Variables::RulesMap.GetBool(ID, "Turret");
	bool bHasShadow = !Variables::RulesMap.GetBool(ID, "NoShadow");
	int facings = ExtConfigs::ExtFacings ? 32 : 8;
	bool turretShadow = bHasShadow && CINI::Art->GetBool(ArtID, "TurretShadow", DrawTurretShadow);
	int ifvTurIndex = -1;
	if (Variables::RulesMap.GetBool(ID, "Gunner"))
	{
		auto types = STDHelpers::SplitString(Variables::RulesMap.GetString(ID, "InitialPayload.Types"));
		if (!types.empty())
		{
			ifvTurIndex = GetIFVTurretIndex(types[0], ID);
		}
	}

	if (CINI::Art->GetBool(ArtID, "Voxel")) // As VXL
	{
		AvailableFacings[ID] = facings;
		FString FileName = ImageID + ".vxl";
		FString HVAName = ImageID + ".hva";

		if (!VoxelDrawer::IsVPLLoaded())
			VoxelDrawer::LoadVPLFile("voxels.vpl");

		FString PaletteName = CINI::Art->GetString(ArtID, "Palette", "unit");
		GetFullPaletteName(PaletteName);

		std::vector<unsigned char*> pImage, pTurretImage, pBarrelImage;
		std::vector<unsigned char*> pShadowImage, pShadowTurretImage, pShadowBarrelImage;
		pImage.resize(facings, nullptr);
		pTurretImage.resize(facings, nullptr);
		pBarrelImage.resize(facings, nullptr);
		if (ExtConfigs::InGameDisplay_Shadow && bHasShadow)
		{
			pShadowImage.resize(facings, nullptr);
			pShadowTurretImage.resize(facings, nullptr);
			pShadowBarrelImage.resize(facings, nullptr);
		}
		std::vector<VoxelRectangle> rect, turretrect, barrelrect;
		std::vector<VoxelRectangle> shadowrect, shadowturretrect, shadowbarrelrect;
		rect.resize(facings);
		turretrect.resize(facings);
		barrelrect.resize(facings);
		if (ExtConfigs::InGameDisplay_Shadow && bHasShadow)
		{
			shadowrect.resize(facings);
			shadowturretrect.resize(facings);
			shadowbarrelrect.resize(facings);
		}

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

			int AddiBarlL = CINI::Art->GetInteger(ArtID, "BarrelOffset", 0);
			int TotalTurretCount = CINI::Art->GetInteger(ArtID, "ExtraTurretCount", 0) + 1;
			int ExtraBarlCount = CINI::Art->GetInteger(ArtID, "ExtraBarrelCount", 0);
			bool BarrelOverTurret = CINI::Art->GetBool(ArtID, "BarrelOverTurret");

			std::vector<int> extraF, extraL, extraH;
			extraF.resize(TotalTurretCount);
			extraL.resize(TotalTurretCount);
			extraH.resize(TotalTurretCount);
			extraF[0] = F;
			extraL[0] = L;
			extraH[0] = H;
			for (int k = 1; k < TotalTurretCount; ++k)
			{
				int F = 0, L = 0, H = 0;
				FString key;
				key.Format("ExtraTurretOffset%d", k - 1);
				int s_count = sscanf_s(CINI::Art->GetString(ArtID, key, "0,0,0"), "%d,%d,%d", &F, &L, &H);
				if (s_count == 0) F = L = H = 0;
				else if (s_count == 1) L = H = 0;
				else if (s_count == 2) H = 0;
				extraF[k] = F;
				extraL[k] = L;
				extraH[k] = H;
			}

			FString turFileName = ImageID + "tur.vxl";
			FString turHVAName = ImageID + "tur.hva";
			if (ifvTurIndex >= 0)
			{
				turFileName.Format("%stur%d.vxl", ImageID, ifvTurIndex);
				turHVAName.Format("%stur%d.hva", ImageID, ifvTurIndex);
			}
			if (VoxelDrawer::LoadVXLFile(turFileName))
			{
				if (VoxelDrawer::LoadHVAFile(turHVAName))
				{
					for (int i = 0; i < facings; ++i)
					{
						int actFacing = (i + facings - 2 * facings / 8) % facings;
						bool result = false;
						if (ExtConfigs::InGameDisplay_Shadow && bHasShadow && turretShadow)
						{
							result = VoxelDrawer::GetImageData(actFacing, pTurretImage[i], turretrect[i], F, L, H)
								&& VoxelDrawer::GetImageData(actFacing, pShadowTurretImage[i], shadowturretrect[i], 
									F, L, 0, true, 0, false);
						}
						else
						{
							result = VoxelDrawer::GetImageData(actFacing, pTurretImage[i], turretrect[i], F, L, H);
						}
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
						bool result = false;
						if (ExtConfigs::InGameDisplay_Shadow && bHasShadow && turretShadow)
						{
							result = VoxelDrawer::GetImageData(actFacing, pBarrelImage[i], barrelrect[i],
								F, L, H, false, Variables::RulesMap.GetInteger(ID, "FireAngle", 10))
								&& VoxelDrawer::GetImageData(actFacing, pShadowBarrelImage[i], shadowbarrelrect[i],
									F, L, 0, true, Variables::RulesMap.GetInteger(ID, "FireAngle", 10), false);
						}
						else
						{
							result = VoxelDrawer::GetImageData(actFacing, pBarrelImage[i], barrelrect[i],
								F, L, H, false, Variables::RulesMap.GetInteger(ID, "FireAngle", 10));
						}

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
				DictName.Format("%s\233%d", ID, i);

				unsigned char* outBuffer;
				int outW = 0x100, outH = 0x100;

				if (pImage[i])
				{
					VXL_Add(pImage[i], rect[i].X, rect[i].Y, rect[i].W, rect[i].H);
					CncImgFree(pImage[i]);
				}
				FString pKey;

				pKey.Format("%sX%d", ID, i);
				int turdeltaX = CINI::FAData->GetInteger("VehicleVoxelTurretsRA2", pKey);
				pKey.Format("%sY%d", ID, i);
				int turdeltaY = CINI::FAData->GetInteger("VehicleVoxelTurretsRA2", pKey);
				pKey.Format("%sX%d", ID, i);
				int barldeltaX = CINI::FAData->GetInteger("VehicleVoxelBarrelsRA2", pKey);
				pKey.Format("%sY%d", ID, i);
				int barldeltaY = CINI::FAData->GetInteger("VehicleVoxelBarrelsRA2", pKey);

				bool barrelInFront = BarrelOverTurret || IsBarrelInFront(i, facings);

				for (int k = 0; k < TotalTurretCount; ++k)
				{
					int exF = extraF[k] - F, exL = extraL[k] - L, exH = extraH[k] - H;
					Matrix3D turretOffset(exF, exL, exH, i, facings);

					if (barrelInFront)
					{
						if (pTurretImage[i])
							VXL_Add(pTurretImage[i], 
								turretrect[i].X + turdeltaX + turretOffset.OutputX,
								turretrect[i].Y + turdeltaY + turretOffset.OutputY,
								turretrect[i].W, turretrect[i].H);
					}

					if (pBarrelImage[i])
					{
						Matrix3D mat(exF, exL + AddiBarlL, exH, i, facings);
						VXL_Add(pBarrelImage[i],
							barrelrect[i].X + barldeltaX + mat.OutputX + turretOffset.OutputX,
							barrelrect[i].Y + barldeltaY + mat.OutputY + turretOffset.OutputY,
							barrelrect[i].W, barrelrect[i].H);
						for (int j = 0; j < ExtraBarlCount; ++j)
						{
							FString key;
							key.Format("ExtraBarrelOffset%d", j);
							int AddiBarlL = CINI::Art->GetInteger(ArtID, key, 0);
							Matrix3D mat(exF, exL + AddiBarlL, exH, i, facings);
							VXL_Add(pBarrelImage[i],
								barrelrect[i].X + barldeltaX + mat.OutputX + turretOffset.OutputX,
								barrelrect[i].Y + barldeltaY + mat.OutputY + turretOffset.OutputY,
								barrelrect[i].W, barrelrect[i].H);
						}
					}

					if (!barrelInFront)
					{
						if (pTurretImage[i])
							VXL_Add(pTurretImage[i],
								turretrect[i].X + turdeltaX + turretOffset.OutputX,
								turretrect[i].Y + turdeltaY + turretOffset.OutputY,
								turretrect[i].W, turretrect[i].H);
					}

					if (ExtConfigs::InGameDisplay_Shadow && bHasShadow && turretShadow)
					{
						if (pShadowTurretImage[i])
						{
							VXL_Add(pShadowTurretImage[i],
								shadowturretrect[i].X + turdeltaX + turretOffset.OutputX,
								shadowturretrect[i].Y + turdeltaY + turretOffset.OutputY,
								shadowturretrect[i].W, shadowturretrect[i].H, true);
						}
						if (pShadowBarrelImage[i])
						{
							VXL_Add(pShadowBarrelImage[i],
								shadowbarrelrect[i].X + barldeltaX + turretOffset.OutputX,
								shadowbarrelrect[i].Y + barldeltaY + turretOffset.OutputY, 
								shadowbarrelrect[i].W, shadowbarrelrect[i].H, true);
							for (int j = 0; j < ExtraBarlCount; ++j)
							{
								FString key;
								key.Format("ExtraBarrelOffset%d", j);
								int AddiBarlL = CINI::Art->GetInteger(ArtID, key, 0);
								Matrix3D mat(exF, exL + AddiBarlL, exH, i, facings);
								VXL_Add(pShadowBarrelImage[i],
									shadowbarrelrect[i].X + barldeltaX + mat.OutputX + turretOffset.OutputX,
									shadowbarrelrect[i].Y + barldeltaY + mat.OutputY + turretOffset.OutputY,
									shadowbarrelrect[i].W, shadowbarrelrect[i].H, true);
							}
						}
					}
				}

				if (ExtConfigs::InGameDisplay_Shadow && bHasShadow && turretShadow && pShadowBarrelImage[i])
					CncImgFree(pShadowBarrelImage[i]);
				if (ExtConfigs::InGameDisplay_Shadow && bHasShadow && turretShadow && pShadowTurretImage[i])
					CncImgFree(pShadowTurretImage[i]);
				if (pTurretImage[i])
					CncImgFree(pTurretImage[i]);
				if (pBarrelImage[i])
					CncImgFree(pBarrelImage[i]);

				VXL_GetAndClear(outBuffer, &outW, &outH);
				SetImageDataSafe(outBuffer, DictName, outW, outH, PalettesManager::LoadPalette(PaletteName));

				if (ExtConfigs::InGameDisplay_Shadow && bHasShadow)
				{
					FString DictShadowName;
					DictShadowName.Format("%s\233%d\233SHADOW", ID, i);

					unsigned char* outBuffer;
					int outW = 0x100, outH = 0x100;

					if (pShadowImage[i])
					{
						VXL_Add(pShadowImage[i], shadowrect[i].X, shadowrect[i].Y, shadowrect[i].W, shadowrect[i].H, true);
						CncImgFree(pShadowImage[i]);
					}
					VXL_GetAndClear(outBuffer, &outW, &outH, true);

					SetImageDataSafe(outBuffer, DictShadowName, outW, outH, &CMapDataExt::Palette_Shadow);

					if (Variables::RulesMap.GetBool(ID, "JumpJet")
						|| Variables::RulesMap.GetBool(ID, "BalloonHover")
						|| Variables::RulesMap.GetBool(ID, "BalloonHover")
						|| Variables::RulesMap.GetString(ID, "Locomotor") == "{92612C46-F71F-11d1-AC9F-006008055BB5}"
						|| Variables::RulesMap.GetString(ID, "Locomotor") == "Jumpjet"
						)
					{
						auto pData = CLoadingExt::GetImageDataFromMap(DictShadowName);
						CLoadingExt::ScaleImageHalf(pData);
					}
				}
			}
		}
		else
		{
			for (int i = 0; i < facings; ++i)
			{
				if (IsLoadingObjectView && i != facings / 8 * 2)
					continue;
				FString DictName;
				DictName.Format("%s\233%d", ID, i);

				unsigned char* outBuffer;
				int outW = 0x100, outH = 0x100;

				VXL_Add(pImage[i], rect[i].X, rect[i].Y, rect[i].W, rect[i].H);
				CncImgFree(pImage[i]);
				VXL_GetAndClear(outBuffer, &outW, &outH);

				SetImageDataSafe(outBuffer, DictName, outW, outH, PalettesManager::LoadPalette(PaletteName));

				if (ExtConfigs::InGameDisplay_Shadow && bHasShadow)
				{
					FString DictShadowName;
					DictShadowName.Format("%s\233%d\233SHADOW", ID, i);

					unsigned char* outBuffer;
					int outW = 0x100, outH = 0x100;

					VXL_Add(pShadowImage[i], shadowrect[i].X, shadowrect[i].Y, shadowrect[i].W, shadowrect[i].H, true);
					CncImgFree(pShadowImage[i]);
					VXL_GetAndClear(outBuffer, &outW, &outH, true);

					SetImageDataSafe(outBuffer, DictShadowName, outW, outH, &CMapDataExt::Palette_Shadow);

					if (Variables::RulesMap.GetBool(ID, "JumpJet")
						|| Variables::RulesMap.GetBool(ID, "BalloonHover")
						|| Variables::RulesMap.GetBool(ID, "BalloonHover")
						|| Variables::RulesMap.GetString(ID, "Locomotor") == "{92612C46-F71F-11d1-AC9F-006008055BB5}"
						|| Variables::RulesMap.GetString(ID, "Locomotor") == "Jumpjet"
						)
					{
						auto pData = CLoadingExt::GetImageDataFromMap(DictShadowName);
						CLoadingExt::ScaleImageHalf(pData);
					}
				}
			}
		}
	}
	else // As SHP
	{
		int facingCount = CINI::Art->GetInteger(ArtID, "Facings", 8);
		if (facingCount < 8)
		{
			facingCount = 1;
		}
		else if (facingCount % 8 != 0)
			facingCount = (facingCount + 7) / 8 * 8;
		int targetFacings = ExtConfigs::ExtFacings ? facingCount : 8;
		if (facingCount == 1)
			targetFacings = 1;
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

		std::vector<unsigned char*> pBarrelImage;
		std::vector<unsigned char*> pShadowBarrelImage;
		std::vector<VoxelRectangle> barrelrect;
		std::vector<VoxelRectangle> shadowbarrelrect;

		FString barlFileName = ImageID + "barl.vxl";
		FString barlHVAName = ImageID + "barl.hva";

		if (!VoxelDrawer::IsVPLLoaded())
			VoxelDrawer::LoadVPLFile("voxels.vpl");

		bool hasVoxelBarl = bHasTurret && VoxelDrawer::LoadVXLFile(barlFileName) && VoxelDrawer::LoadHVAFile(barlHVAName);
		if (hasVoxelBarl)
		{
			pBarrelImage.resize(targetFacings, nullptr);
			barrelrect.resize(targetFacings);
			if (ExtConfigs::InGameDisplay_Shadow && bHasShadow)
			{
				pShadowBarrelImage.resize(targetFacings, nullptr);
				shadowbarrelrect.resize(targetFacings);
			}
		}

		std::rotate(framesToRead.begin(), framesToRead.begin() + 1 * targetFacings / 8, framesToRead.end());

		FString FileName = ImageID + ".shp";
		FString FileNameTurret = ImageID + "tur.shp";
		int nMix = this->SearchFile(FileName);
		if (HasFile(FileName, nMix))
		{
			ShapeHeader header{};
			ShapeHeader headerTurret{};
			unsigned char* FramesBuffers[1]{ 0 };
			unsigned char* FramesBuffersShadow[1]{ 0 };
			unsigned char* FramesBuffersTurret[32]{ 0 };
			unsigned char* FramesBuffersTurretShadow[32]{ 0 };
			FString PaletteName = CINI::Art->GetString(ArtID, "Palette", "unit");
			GetFullPaletteName(PaletteName);

			int nMixTur = this->SearchFile(FileNameTurret);
			bool bUseTurrentFile = HasFile(FileNameTurret, nMixTur);
			if (bHasTurret)
			{
				ShapeHeader* currentHeader = nullptr;
				if (bUseTurrentFile && CMixFile::LoadSHP(FileNameTurret, nMixTur))
				{
					CShpFile::GetSHPHeader(&headerTurret);
					currentHeader = &headerTurret;
				}
				else if (!bUseTurrentFile && CMixFile::LoadSHP(FileName, nMix))
				{
					CShpFile::GetSHPHeader(&header);
					currentHeader = &header;
				}

				if (currentHeader)
				{
					for (int i = 0; i < targetFacings; ++i)
					{
						if (IsLoadingObjectView && i != targetFacings / 8 * 2)
							continue;

						int nStartWalkFrame = CINI::Art->GetInteger(ArtID, "StartWalkFrame", 0);
						int nWalkFrames = CINI::Art->GetInteger(ArtID, "WalkFrames", 1);
						int turretFrameToRead, turrentFacing;

						// turret start from 0 + WalkFrames * Facings, ignore StartWalkFrame
						// and always has 32 facings
						turrentFacing = (((targetFacings / 8 + i) % targetFacings) * 32 / targetFacings) % 32;
						turretFrameToRead = bUseTurrentFile ? turrentFacing : (facingCount * nWalkFrames + turrentFacing);

						CLoadingExt::LoadSHPFrameSafe(turretFrameToRead,
							1, &FramesBuffersTurret[turrentFacing], *currentHeader);

						if (ExtConfigs::InGameDisplay_Shadow && bHasShadow)
							CLoadingExt::LoadSHPFrameSafe(turretFrameToRead + currentHeader->FrameCount / 2,
								1, &FramesBuffersTurretShadow[turrentFacing], *currentHeader);
					}

					if (bUseTurrentFile)
					{
						CMixFile::LoadSHP(FileName, nMix);
						CShpFile::GetSHPHeader(&header);
					}
				}
			}
			else
			{
				CMixFile::LoadSHP(FileName, nMix);
				CShpFile::GetSHPHeader(&header);
			}

			for (int i = 0; i < targetFacings; ++i)
			{
				if (IsLoadingObjectView && i != targetFacings / 8 * 2)
					continue;

				CLoadingExt::LoadSHPFrameSafe(framesToRead[i], 1, &FramesBuffers[0], header);

				FString DictName;
				DictName.Format("%s\233%d", ID, i);

				if (bHasTurret)
				{
					int F, L, H;
					int s_count = sscanf_s(CINI::Art->GetString(ArtID, "TurretOffset", "0,0,0"), "%d,%d,%d", &F, &L, &H);
					if (s_count == 0) F = L = H = 0;
					else if (s_count == 1) L = H = 0;
					else if (s_count == 2) H = 0;

					int nStartWalkFrame = CINI::Art->GetInteger(ArtID, "StartWalkFrame", 0);
					int nWalkFrames = CINI::Art->GetInteger(ArtID, "WalkFrames", 1);
					int turrentFacing;

					turrentFacing = (((targetFacings / 8 + i) % targetFacings) * 32 / targetFacings) % 32;

					bool barrelInFront = IsBarrelInFront(i, targetFacings);
					if (hasVoxelBarl)
					{
						int barlFacing = (i * facings / targetFacings + facings - 2 * facings / 8) % facings;
						if (ExtConfigs::InGameDisplay_Shadow && bHasShadow && turretShadow)
						{
							hasVoxelBarl = VoxelDrawer::GetImageData(barlFacing, pBarrelImage[i], barrelrect[i],
								F, L, H, false, Variables::RulesMap.GetInteger(ID, "FireAngle", 10))
								&& VoxelDrawer::GetImageData(barlFacing, pShadowBarrelImage[i], shadowbarrelrect[i],
									F, L, 0, true, Variables::RulesMap.GetInteger(ID, "FireAngle", 10), false);
						}
						else
						{
							hasVoxelBarl = VoxelDrawer::GetImageData(barlFacing, pBarrelImage[i], barrelrect[i],
								F, L, H, false, Variables::RulesMap.GetInteger(ID, "FireAngle", 10));
						}
					}

					unsigned char* outBufferVxl = nullptr;
					int outWVxl = 0x100, outHVxl = 0x100;
					if (hasVoxelBarl)
					{
						if (pBarrelImage[i])
						{
							VXL_Add(pBarrelImage[i], barrelrect[i].X, barrelrect[i].Y, barrelrect[i].W, barrelrect[i].H);
							CncImgFree(pBarrelImage[i]);
							VXL_GetAndClear(outBufferVxl, &outWVxl, &outHVxl);
						}

						if (ExtConfigs::InGameDisplay_Shadow && bHasShadow && turretShadow && pShadowBarrelImage[i])
						{
							unsigned char* outBuffer = nullptr;
							int outW = 0x100, outH = 0x100;
							VXL_Add(pShadowBarrelImage[i], shadowbarrelrect[i].X,
								shadowbarrelrect[i].Y, shadowbarrelrect[i].W, shadowbarrelrect[i].H, true);
							CncImgFree(pShadowBarrelImage[i]);
							VXL_GetAndClear(outBuffer, &outW, &outH, true);
							UnionSHP_Add(outBuffer, outW, outH, 0, 0, false, true);
						}
					}


					Matrix3D mat(F, L, H, i, targetFacings);

					UnionSHP_Add(FramesBuffers[0], header.Width, header.Height);

					if (outBufferVxl && !barrelInFront)
						UnionSHP_Add(outBufferVxl, outWVxl, outHVxl, 0, 0);

					UnionSHP_Add(FramesBuffersTurret[turrentFacing],
						bUseTurrentFile ? headerTurret.Width : header.Width,
						bUseTurrentFile ? headerTurret.Height : header.Height,
						mat.OutputX, mat.OutputY);

					if (outBufferVxl && barrelInFront)
						UnionSHP_Add(outBufferVxl, outWVxl, outHVxl, 0, 0);

					unsigned char* outBuffer;
					int outW, outH;
					UnionSHP_GetAndClear(outBuffer, &outW, &outH);

					SetImageDataSafe(outBuffer, DictName, outW, outH, PalettesManager::LoadPalette(PaletteName));

					if (ExtConfigs::InGameDisplay_Shadow && bHasShadow)
					{
						FString DictNameShadow;
						DictNameShadow.Format("%s\233%d\233SHADOW", ID, i);
						CLoadingExt::LoadSHPFrameSafe(framesToRead[i] + header.FrameCount / 2, 1, &FramesBuffersShadow[0], header);
						UnionSHP_Add(FramesBuffersShadow[0], header.Width, header.Height, 0, 0, false, true);
						UnionSHP_Add(FramesBuffersTurretShadow[turrentFacing],
							bUseTurrentFile ? headerTurret.Width : header.Width,
							bUseTurrentFile ? headerTurret.Height : header.Height,
							mat.OutputX, mat.OutputY, false, true);
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
						DictNameShadow.Format("%s\233%d\233SHADOW", ID, i);
						CLoadingExt::LoadSHPFrameSafe(framesToRead[i] + header.FrameCount / 2, 1, &FramesBuffersShadow[0], header);
						SetImageDataSafe(FramesBuffersShadow[0], DictNameShadow, header.Width, header.Height, &CMapDataExt::Palette_Shadow);
					}
				}
			}
		}
		else
		{
			// no image
			AvailableFacings[ID] = facings;
		}
	}
}

void CLoadingExt::SetImageDataSafe(unsigned char* pBuffer, FString NameInDict, int FullWidth, int FullHeight, Palette* pPal, bool clip)
{
	auto pData = CLoadingExt::GetImageDataFromMap(NameInDict);
	SetImageDataSafe(pBuffer, pData, FullWidth, FullHeight, pPal);
	if (clip) TrimImageEdges(pData, pPal == &CMapDataExt::Palette_Shadow);
}

ImageDataClassSafe* CLoadingExt::SetBuildingImageDataSafe(unsigned char* pBuffer, FString NameInDict,
	int FullWidth, int FullHeight, Palette* pPal, unsigned char* pAlphaBuffer)
{
	auto& ret = CLoadingExt::GetBuildingClipImageDataFromMap(NameInDict);
	ret.emplace_back(std::make_unique<ImageDataClassSafe>());
	auto pData = ret.back().get();
	if (pBuffer)
	{
		SetImageDataSafe(pBuffer, pData, FullWidth, FullHeight, pPal);
		if (pAlphaBuffer)
		{
			pData->pOpacity = std::unique_ptr<unsigned char[]>(new unsigned char[FullWidth * FullHeight]);
			std::memcpy(pData->pOpacity.get(), pAlphaBuffer, FullWidth * FullHeight);
		}
	}
	else
	{
		pData->Flag = ImageDataFlag::SHP;
		pData->IsOverlay = false;
		pData->pPalette = pPal ? pPal : Palette::PALETTE_UNIT;
	}
	return pData;
}

void CLoadingExt::SetImageDataSafe(unsigned char* pBuffer, ImageDataClassSafe* pData, int FullWidth, int FullHeight, Palette* pPal)
{
	if (pData->pImageBuffer)
		pData->pImageBuffer = nullptr;
	if (pData->pPixelValidRanges)
		pData->pPixelValidRanges = nullptr;

	pData->Flag = ImageDataFlag::SHP;
	pData->IsOverlay = false;
	pData->pPalette = pPal ? pPal : Palette::PALETTE_UNIT;

	if (!pBuffer)
	{
		pData->FullHeight = 0;
		pData->FullWidth = 0;
		return;
	}

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
	int DeltaX, int DeltaY, bool UseTemp, bool bShadow, int ZAdjust, int YSort,
	bool MainBody, unsigned char Opacity, bool Remapable)
{
	if (bShadow && pBuffer)
		UnionSHPShadow_Data[UseTemp].push_back(SHPUnionData{ pBuffer,Width,Height,DeltaX,DeltaY, ZAdjust, YSort, MainBody, Opacity, Remapable });
	else if (pBuffer)
		UnionSHP_Data[UseTemp].push_back(SHPUnionData{ pBuffer,Width,Height,DeltaX,DeltaY, ZAdjust, YSort, MainBody, Opacity, Remapable });
}

void CLoadingExt::UnionSHP_GetAndClear(unsigned char*& pOutBuffer,
	int* OutWidth, int* OutHeight, bool UseTemp, bool bShadow, bool bSort, unsigned char** pOutAlphaBuffer, Palette* pPal)
{
	auto& data = bShadow ? UnionSHPShadow_Data : UnionSHP_Data;
	if (data[UseTemp].size() == 0)
	{
		pOutBuffer = nullptr;
		*OutWidth = 0;
		*OutHeight = 0;
		return;
	}
	if (data[UseTemp].size() == 1)
	{
		pOutBuffer = data[UseTemp][0].pBuffer;
		*OutWidth = data[UseTemp][0].Width;
		*OutHeight = data[UseTemp][0].Height;
		if (pOutAlphaBuffer && !*pOutAlphaBuffer)
		{
			size_t pixelCount = (size_t)data[UseTemp][0].Width * data[UseTemp][0].Height;
			unsigned char* alpha = GameCreateArray<unsigned char>(pixelCount);
			memset(alpha, data[UseTemp][0].Opacity, pixelCount);
			*pOutAlphaBuffer = alpha;
		}

		data[UseTemp].clear();
		return;
	}

	// For each shp, we make their center at the same point, this will give us proper result.
	int W = 0, H = 0;

	for (auto& img : data[UseTemp])
	{
		if (W < img.Width + 2 * abs(img.DeltaX)) W = img.Width + 2 * abs(img.DeltaX);
		if (H < img.Height + 2 * abs(img.DeltaY)) H = img.Height + 2 * abs(img.DeltaY);
	}

	// just make it work like unsigned char[W][H];
	pOutBuffer = GameCreateArray<unsigned char>(W * H); 
	memset(pOutBuffer, 0, W * H);

	unsigned char* alphaBuffer = nullptr;
	unsigned char* alphaTempBuffer = nullptr;
	if (pOutAlphaBuffer)
	{
		if (!*pOutAlphaBuffer)
		{
			alphaBuffer = GameCreateArray<unsigned char>(W * H);
			memset(alphaBuffer, 0, W * H);
			*pOutAlphaBuffer = alphaBuffer;
		}
		else
		{
			alphaTempBuffer = *pOutAlphaBuffer;
			alphaBuffer = GameCreateArray<unsigned char>(W * H);
			memset(alphaBuffer, 0, W * H);
			*pOutAlphaBuffer = alphaBuffer;
		}
	}

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
	bool lastImgRemapable = true;
	for (const auto& img : data[UseTemp])
	{
		int nStartX = ImageCenterX - img.Width / 2 + img.DeltaX;
		int nStartY = ImageCenterY - img.Height / 2 + img.DeltaY;

		unsigned char layerOpacity = img.Opacity;

		for (int j = 0; j < img.Height; ++j)
		{
			int dstY = nStartY + j;
			if (dstY < 0 || dstY >= H) continue;

			for (int i = 0; i < img.Width; ++i)
			{
				unsigned char srcPal = img.pBuffer[j * img.Width + i];
				if (srcPal == 0) continue;

				//srcPal = 16;

				int dstX = nStartX + i;
				if (dstX < 0 || dstX >= W) continue;

				size_t dstIdx = (size_t)dstY * W + dstX;

				unsigned char& dstPal = pOutBuffer[dstIdx];

				if (alphaBuffer)
				{
					if (alphaTempBuffer)
					{
						alphaBuffer[dstIdx] = alphaTempBuffer[j * img.Width + i];
					}
					else
					{
						unsigned int a_dst = alphaBuffer[dstIdx];
						unsigned int a_src = layerOpacity;

						unsigned int a_out = a_src + (a_dst * (255 - a_src)) / 255;

						if (a_out > 255) a_out = 255;

						alphaBuffer[dstIdx] = (unsigned char)a_out;
					}
				}

				if (dstPal == 0 || layerOpacity == 255)
				{
					dstPal = srcPal;
				}
				else
				{
					bool srcPlayer = (srcPal >= 16 && srcPal <= 31 && img.Remapable);
					bool dstPlayer = (dstPal >= 16 && dstPal <= 31 && lastImgRemapable);

					if (srcPlayer || dstPlayer)
					{
						if (srcPlayer && dstPlayer)
						{
							float a_src = layerOpacity / 255.0f;
							float mixed = (1.0f - a_src) * dstPal + a_src * srcPal;
							int newIdx = (int)(mixed + 0.5f);
							if (newIdx < 16) newIdx = 16;
							if (newIdx > 31) newIdx = 31;
							dstPal = (unsigned char)newIdx;
						}
						else if (srcPlayer)
						{
							dstPal = srcPal;
						}
					}
					else
					{
						BGRStruct bg = pPal->Data[dstPal]; 
						float bg_r = bg.R, bg_g = bg.G, bg_b = bg.B;

						BGRStruct fg = pPal->Data[srcPal];
						float fg_r = fg.R, fg_g = fg.G, fg_b = fg.B;

						float a_fg = layerOpacity / 255.0f;
						float a_bg = 1.0f - a_fg;

						float mix_r = a_bg * bg_r + a_fg * fg_r;
						float mix_g = a_bg * bg_g + a_fg * fg_g;
						float mix_b = a_bg * bg_b + a_fg * fg_b;

						unsigned char best = 0;
						double minDist = 1e30;

						for (int k = 0; k < 256; ++k)
						{
							if (k >= 16 && k <= 31) continue; 

							BGRStruct c = pPal->Data[k];
							double dr = mix_r - c.R;
							double dg = mix_g - c.G;
							double db = mix_b - c.B;
							double dist = dr * dr + dg * dg + db * db;

							if (dist < minDist)
							{
								minDist = dist;
								best = (unsigned char)k;
							}
						}

						dstPal = best;
					}
				}
			}
		}

		GameDeleteArray(img.pBuffer, img.Width * img.Height);
		if (alphaTempBuffer)
		{
			GameDeleteArray(alphaTempBuffer, img.Width* img.Height);
			alphaTempBuffer = nullptr;
		}

		lastImgRemapable = img.Remapable;
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

void CLoadingExt::ScaleImageHalf(ImageDataClassSafe* pData)
{
	if (!pData || !pData->pImageBuffer ||
		pData->FullWidth <= 1 || pData->FullHeight <= 1)
		return;

	const int oldW = pData->FullWidth;
	const int oldH = pData->FullHeight;
	const unsigned char* src = pData->pImageBuffer.get();

	const int newW = oldW / 2;
	const int newH = oldH / 2;

	if (newW <= 0 || newH <= 0)
		return;

	std::unique_ptr<unsigned char[]> newBuffer(
		new unsigned char[newW * newH]
	);

	auto pick2x2 = [](
		const unsigned char* src, int w, int x, int y)
	{
		unsigned char p;

		p = src[y * w + x];
		if (p) return p;

		p = src[y * w + x + 1];
		if (p) return p;

		p = src[(y + 1) * w + x];
		if (p) return p;

		return src[(y + 1) * w + x + 1];
	};

	for (int y = 0; y < newH; ++y)
	{
		const int srcY = y * 2;
		for (int x = 0; x < newW; ++x)
		{
			const int srcX = x * 2;
			newBuffer[y * newW + x] =
				pick2x2(src, oldW, srcX, srcY);
		}
	}

	pData->pImageBuffer = std::move(newBuffer);

	pData->FullWidth = newW;
	pData->FullHeight = newH;

	pData->ValidX = pData->ValidX / 2;
	pData->ValidY = pData->ValidY / 2;

	pData->ValidWidth = (pData->ValidWidth + 1) / 2;
	pData->ValidHeight = (pData->ValidHeight + 1) / 2;

	if (pData->pPixelValidRanges)
		pData->pPixelValidRanges.reset();

	SetValidBufferSafe(pData, newW, newH);
}

void CLoadingExt::TrimImageEdges(ImageDataClassSafe* pData, bool shadow)
{
	if (!pData || !pData->pImageBuffer || pData->FullWidth == 0 || pData->FullHeight == 0) return;

	auto invalidateImage = [&pData]()
	{
		pData->pImageBuffer = nullptr;
		pData->pPixelValidRanges = nullptr;
		pData->FullWidth = 0;
		pData->FullHeight = 0;
		pData->ValidX = 0;
		pData->ValidY = 0;
		pData->ValidWidth = 0;
		pData->ValidHeight = 0;
	};

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
	{
		invalidateImage();
		return;
	}

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

	if (newW <= 0 || newH <= 0)
	{
		invalidateImage();
		return;
	}

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

void CLoadingExt::TrimImageEdges(unsigned char*& pBuffer, int& width, int& height, unsigned char** pSecondBuffer)
{
	if (!pBuffer || width <= 0 || height <= 0)
		return;

	const int oldW = width;
	const int oldH = height;

	int minX = oldW - 1, minY = oldH - 1;
	int maxX = 0, maxY = 0;

	for (int y = 0; y < oldH; ++y)
	{
		const unsigned char* row = pBuffer + y * oldW;
		for (int x = 0; x < oldW; ++x)
		{
			if (row[x] != 0)
			{
				if (x < minX) minX = x;
				if (y < minY) minY = y;
				if (x > maxX) maxX = x;
				if (y > maxY) maxY = y;
			}
		}
	}

	if (minX > maxX || minY > maxY)
	{
		return;
	}

	int leftSpace = minX;
	int rightSpace = oldW - 1 - maxX;
	int topSpace = minY;
	int bottomSpace = oldH - 1 - maxY;

	int cropLR = std::min(leftSpace, rightSpace);
	int cropTB = std::min(topSpace, bottomSpace);

	int newW = oldW - cropLR * 2;
	int newH = oldH - cropTB * 2;

	if (newW <= 0 || newH <= 0)
		return;

	unsigned char* newBuffer = GameCreateArray<unsigned char>(newW * newH);
	unsigned char* newSecondBuffer = nullptr;
	if (pSecondBuffer && *pSecondBuffer)
	{
		newSecondBuffer = GameCreateArray<unsigned char>(newW * newH);
	}

	for (int y = 0; y < newH; ++y)
	{
		int srcY = y + cropTB;
		std::memcpy(
			newBuffer + y * newW,
			pBuffer + srcY * oldW + cropLR,
			newW
		);
		if (pSecondBuffer && *pSecondBuffer)
		{
			std::memcpy(
				newSecondBuffer + y * newW,
				*pSecondBuffer + srcY * oldW + cropLR,
				newW
			);
		}
	}

	GameDeleteArray(pBuffer, oldW * oldH);
	if (pSecondBuffer && *pSecondBuffer)
	{
		GameDeleteArray(*pSecondBuffer, oldW * oldH);
		*pSecondBuffer = newSecondBuffer;
	}
	pBuffer = newBuffer;

	width = newW;
	height = newH;
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
	FString filepath;
	std::ifstream fin;

	filepath = CFinalSunApp::ExePath();
	filepath += "Resources\\HighPriority\\";
	filepath += filename;
	fin.open(filepath, std::ios::in | std::ios::binary);
	if (!fin.is_open())
	{
		filepath = CFinalSunApp::FilePath();
		filepath += filename;
		fin.open(filepath, std::ios::in | std::ios::binary);
	}

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
	}
	if (CMixFile::HasFile(filename, nMix))
		return nMix;

	filepath = CFinalSunApp::ExePath();
	filepath += "Resources\\LowPriority\\";
	filepath += filename;
	fin.open(filepath, std::ios::in | std::ios::binary);
	if (fin.is_open())
	{
		fin.close();
		return -1;
	}

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
		PaletteName.Replace("~~~", GetTheaterSuffix());
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

int CLoadingExt::ColorDistance(const BGRStruct& color1, const BGRStruct& color2)
{
	int diffRed = color1.R - color2.R;
	int diffGreen = color1.G - color2.G;
	int diffBlue = color1.B - color2.B;
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

struct ColorWithCount {
	BGRStruct color;
	int count = 1; 
};

bool CLoadingExt::AreColorsVeryClose(const BGRStruct& a, const BGRStruct& b) {
	int d = ColorDistance(a, b);
	return d <= 3 * 3; 
}

Palette* CLoadingExt::CreateBalancedPalette(const Palette* palA, const Palette* palB)
{
	Palette* result = GameCreate<Palette>();
	PalettesManager::CalculatedMixedPalettes.push_back(result);

	result->Data[0] = BGRStruct(0, 0, 0);

	for (int i = 16; i < 32; ++i) {
		result->Data[i] = palA->Data[i];
	}

	struct ColorNode {
		BGRStruct color;
		int count;
		float hue;
	};

	std::vector<ColorNode> colors;
	colors.reserve(512);

	auto add_palette = [&](const Palette* pal) {
		for (int i = 1; i < 256; ++i) {
			if (i >= 16 && i < 32) continue;

			bool found = false;
			for (auto& c : colors) {
				if (AreColorsVeryClose(c.color, pal->Data[i])) {
					c.count++;
					found = true;
					break;
				}
			}

			if (!found) {
				BGRStruct col = pal->Data[i];

				float r = col.R / 255.f;
				float g = col.G / 255.f;
				float b = col.B / 255.f;

				float maxc = std::max({ r, g, b });
				float minc = std::min({ r, g, b });
				float delta = maxc - minc;

				float hue = 0.f;
				if (delta > 0.0001f) {
					if (maxc == r)
						hue = fmodf((g - b) / delta, 6.f);
					else if (maxc == g)
						hue = (b - r) / delta + 2.f;
					else
						hue = (r - g) / delta + 4.f;
					hue *= 60.f;
					if (hue < 0) hue += 360.f;
				}

				colors.push_back({ col, 1, hue });
			}
		}
	};

	add_palette(palA);
	add_palette(palB);

	if (colors.empty())
		return result;

	const int target = 224;

	std::sort(colors.begin(), colors.end(),
		[](const ColorNode& a, const ColorNode& b) {
		return a.hue < b.hue;
	});

	int totalWeight = 0;
	for (auto& c : colors)
		totalWeight += c.count;

	std::vector<BGRStruct> selected;
	selected.reserve(target);

	int cumulative = 0;
	int idx = 0;

	for (int i = 0; i < target; ++i) {

		int threshold = (int)((long long)totalWeight * i / target);

		while (idx < (int)colors.size() - 1 &&
			cumulative + colors[idx].count <= threshold)
		{
			cumulative += colors[idx].count;
			idx++;
		}

		selected.push_back(colors[idx].color);
	}

	int write_idx = 1;
	for (auto& c : selected) {
		if (write_idx >= 16 && write_idx < 32)
			write_idx = 32;
		if (write_idx >= 256)
			break;

		result->Data[write_idx++] = c;
	}

	return result;
}

void CLoadingExt::RemapImagePalette(
	unsigned char* pBuffer,
	int width,
	int height,
	const Palette* oldPalette,
	const Palette* newPalette, 
	bool remapable)
{
	if (!pBuffer || !oldPalette || !newPalette) return;

	int lookup[256]{};

	lookup[0] = 0;

	if (remapable)
		for (int i = 16; i < 32; ++i) {
			lookup[i] = i; 
		}

	for (int i = 1; i < 256; ++i) {
		if (remapable && i >= 16 && i < 32) continue;

		const BGRStruct& target = oldPalette->Data[i];
		int minDist = INT_MAX;
		int best = i; 

		for (int j = 1; j < 256; ++j) {
			if (j >= 16 && j < 32) continue;
			int dist = ColorDistance(target, newPalette->Data[j]);
			if (dist < minDist) {
				minDist = dist;
				best = j;
			}
		}

		lookup[i] = best;
	}

	const int total = width * height;
	for (int i = 0; i < total; ++i) {
		unsigned char oldIdx = pBuffer[i];
		pBuffer[i] = static_cast<unsigned char>(lookup[oldIdx]);
	}
}

void CLoadingExt::LoadSHPFrameSafe(int nFrame, int nFrameCount, unsigned char** ppBuffer, const ShapeHeader& header)
{
	if (nFrame < 0 || nFrame + nFrameCount - 1 >= header.FrameCount || header.Width == 0 && header.Height == 0)
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
	LoadedSurfaceObjects.insert(ImageID);
}

bool CLoadingExt::ReplaceBitmapColor(
	CBitmap& bitmap,
	COLORREF oldColor,
	COLORREF newColor
)
{
	if (bitmap.GetSafeHandle() == nullptr)
		return false;

	BITMAP bm;
	if (!bitmap.GetBitmap(&bm))
		return false;

	if (bm.bmBitsPixel != 4 && bm.bmBitsPixel != 8 &&
		bm.bmBitsPixel != 16 && bm.bmBitsPixel != 24 && bm.bmBitsPixel != 32)
	{
		return false;
	}

	CDC memDC;
	if (!memDC.CreateCompatibleDC(nullptr))
		return false;

	CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);
	if (!pOldBitmap)
	{
		memDC.DeleteDC();
		return false;
	}

	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = bm.bmWidth;
	bmi.bmiHeader.biHeight = -bm.bmHeight;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	std::vector<BYTE> buffer(bm.bmWidth * bm.bmHeight * 4);
	if (GetDIBits(memDC.GetSafeHdc(),
		(HBITMAP)bitmap.GetSafeHandle(),
		0, bm.bmHeight,
		buffer.data(),
		&bmi,
		DIB_RGB_COLORS) == 0)
	{
		memDC.SelectObject(pOldBitmap);
		memDC.DeleteDC();
		return false;
	}

	BYTE oldR = GetRValue(oldColor);
	BYTE oldG = GetGValue(oldColor);
	BYTE oldB = GetBValue(oldColor);

	BYTE newR = GetRValue(newColor);
	BYTE newG = GetGValue(newColor);
	BYTE newB = GetBValue(newColor);

	for (int y = 0; y < bm.bmHeight; ++y)
	{
		for (int x = 0; x < bm.bmWidth; ++x)
		{
			BYTE* pixel = &buffer[(y * bm.bmWidth + x) * 4];

			BYTE b = pixel[0];
			BYTE g = pixel[1];
			BYTE r = pixel[2];

			if (r == oldR && g == oldG && b == oldB)
			{
				pixel[0] = newB;
				pixel[1] = newG;
				pixel[2] = newR;
			}
		}
	}

	SetDIBits(memDC.GetSafeHdc(),
		(HBITMAP)bitmap.GetSafeHandle(),
		0, bm.bmHeight,
		buffer.data(),
		&bmi,
		DIB_RGB_COLORS);

	memDC.SelectObject(pOldBitmap);
	memDC.DeleteDC();

	return true;
}

void CLoadingExt::LoadShp(FString ImageID, FString FileName, FString PalName, int nFrame)
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
			if (!CMixFile::LoadSHP(FileName, nMix))
				return;
			CShpFile::GetSHPHeader(&header);
			CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &FramesBuffers, header);
			loadingExt->SetImageDataSafe(FramesBuffers, ImageID, header.Width, header.Height, pal);
			LoadedObjects.insert(ImageID);
		}
	}
}

void CLoadingExt::LoadFires(const ppmfc::CString& FileName)
{
	auto loadingExt = (CLoadingExt*)CLoading::Instance();
	if (auto pal = PalettesManager::LoadPalette("anim.pal"))
	{
		int nMix = loadingExt->SearchFile(FileName);
		if (loadingExt->HasFile(FileName, nMix))
		{
			ShapeHeader header;
			unsigned char* FramesBuffers;
			if (!CMixFile::LoadSHP(FileName, nMix))
				return;
			CShpFile::GetSHPHeader(&header);
			for (int i = 0; i < header.FrameCount; ++i)
			{
				loadingExt->LoadSHPFrameSafe(i, 1, &FramesBuffers, header);
				auto pData = std::make_unique<ImageDataClassSafe>();
				loadingExt->SetImageDataSafe(FramesBuffers, pData.get(), header.Width, header.Height, pal);
				DamageFires.push_back(std::move(pData));
			}
		}
	}
}

std::vector<ImageDataClassSafe*> CLoadingExt::GetRandomFire(const MapCoord& coord, int number)
{
	if (DamageFires.empty()) return {};
	if (number == 0) return {};

	size_t h1 = std::hash<int>{}(coord.X);
	size_t h2 = std::hash<int>{}(coord.Y);
	auto hash = h1 ^ (h2 << 1);
	std::mt19937 rng(static_cast<unsigned int>(hash ^ RandomFireSeed));

	std::vector<ImageDataClassSafe*> shuffled;
	for (const auto& f : DamageFires)
		shuffled.push_back(f.get());

	std::shuffle(shuffled.begin(), shuffled.end(), rng);

	std::vector<ImageDataClassSafe*> result;
	result.reserve(number);

	if (shuffled.size() >= number) {
		result.insert(result.end(), shuffled.begin(), shuffled.begin() + number);
	}
	else {
		while (result.size() < number) {
			size_t remain = number - result.size();
			if (remain >= shuffled.size())
				result.insert(result.end(), shuffled.begin(), shuffled.end());
			else
				result.insert(result.end(), shuffled.begin(), shuffled.begin() + remain);
		}
	}
	return result;
}

bool CLoadingExt::IsBarrelInFront(int curFacing, int totFacing)
{
	if (curFacing >= 0 && curFacing < totFacing / 8)
		return false;
	else if (curFacing >= totFacing / 8 * 5 && curFacing < totFacing)
		return false;
	return true;
}

void CLoadingExt::LoadShp(FString ImageID, FString FileName, Palette* pPal, int nFrame)
{
	if (pPal)
	{
		auto loadingExt = (CLoadingExt*)CLoading::Instance();
		int nMix = loadingExt->SearchFile(FileName);
		if (loadingExt->HasFile(FileName, nMix))
		{
			ShapeHeader header;
			unsigned char* FramesBuffers;
			if (!CMixFile::LoadSHP(FileName, nMix))
				return;
			CShpFile::GetSHPHeader(&header);
			CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &FramesBuffers, header);
			loadingExt->SetImageDataSafe(FramesBuffers, ImageID, header.Width, header.Height, pPal);
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
				LoadedSurfaceObjects.insert(ImageID);
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
		LoadedSurfaceObjects.insert(ImageID);
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

unsigned char* CLoadingExt::ClipImageHorizontal(
	const unsigned char* pBuffer,
	int width,
	int height,
	int cutLeft,
	int cutRight,
	int& outWidth
)
{
	if (!pBuffer)
	{
		outWidth = 0;
		return nullptr;
	}

	cutLeft = std::max(0, cutLeft);
	cutRight = std::min(width, cutRight);

	if (cutLeft >= cutRight)
	{
		outWidth = 0;
		return nullptr;
	}

	outWidth = cutRight - cutLeft;

	size_t newSize = size_t(outWidth) * height;
	unsigned char* clipped = GameCreateArray<unsigned char>(newSize);

	for (int y = 0; y < height; ++y)
	{
		const unsigned char* src = pBuffer + y * width + cutLeft;
		unsigned char* dst = clipped + y * outWidth;

		std::memcpy(dst, src, outWidth);
	}

	return clipped;
}

std::unique_ptr<ImageDataClassSafe> CLoadingExt::BindClippedImages(const std::vector<std::unique_ptr<ImageDataClassSafe>>& imgs)
{
	if (imgs.empty())
		return nullptr;

	int totalWidth = 0;
	int maxHeight = 0;

	for (auto& img : imgs)
	{
		if (!img) continue;
		totalWidth += img->FullWidth;
		maxHeight = std::max<int>(maxHeight, img->FullHeight);
	}

	auto result = std::make_unique<ImageDataClassSafe>();
	result->FullWidth = totalWidth;
	result->FullHeight = maxHeight;
	result->pImageBuffer = std::unique_ptr<unsigned char[]>(
		new unsigned char[totalWidth * maxHeight]
	);

	std::memset(result->pImageBuffer.get(), 0, totalWidth * maxHeight);

	int offsetX = 0;

	for (auto& img : imgs)
	{
		if (!img || !img->pImageBuffer) continue;

		unsigned char* src = img->pImageBuffer.get();
		unsigned char* dst = result->pImageBuffer.get();

		for (int y = 0; y < img->FullHeight; ++y)
		{
			int dstIndex = y * totalWidth + offsetX;
			int srcIndex = y * img->FullWidth;
			std::memcpy(dst + dstIndex, src + srcIndex, img->FullWidth);
		}

		offsetX += img->FullWidth;
	}

	result->ValidX = 0;
	result->ValidY = 0;
	result->ValidWidth = totalWidth;
	result->ValidHeight = maxHeight;

	result->pPixelValidRanges = std::unique_ptr<ImageDataClassSafe::ValidRangeData[]>(
		new ImageDataClassSafe::ValidRangeData[maxHeight]
	);

	for (int y = 0; y < maxHeight; ++y)
	{
		ImageDataClassSafe::ValidRangeData& vr = result->pPixelValidRanges[y];

		vr.First = totalWidth - 1;
		vr.Last = 0;

		unsigned char* row = result->pImageBuffer.get() + y * totalWidth;
		for (int x = 0; x < totalWidth; ++x)
		{
			if (row[x] != 0)
			{
				vr.First = std::min<int>(vr.First, x);
				vr.Last = std::max<int>(vr.Last, x);
			}
		}
		if (vr.First > vr.Last)
		{
			vr.First = totalWidth - 1;
			vr.Last = 0;
		}
	}

	result->Flag = imgs[0]->Flag;
	result->IsOverlay = imgs[0]->IsOverlay;
	result->pPalette = imgs[0]->pPalette;
	result->BuildingFlag = imgs[0]->BuildingFlag;

	return result;
}

void CLoadingExt::LoadOverlay(const FString& pRegName, int nIndex)
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

	bool isveinhole = Variables::RulesMap.GetBool(lpOvrlName,"IsVeinholeMonster");
	bool istiberium = Variables::RulesMap.GetBool(lpOvrlName, "Tiberium");
	bool isveins = Variables::RulesMap.GetBool(lpOvrlName, "IsVeins");
	bool iswall = Variables::RulesMap.GetBool(lpOvrlName, "Wall");

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

	auto pCellAnim = Variables::RulesMap.GetString(lpOvrlName, "CellAnim");
	FString CellAnimImageID;
	if (pCellAnim != "")
	{
		CellAnimImageID = CINI::Art->GetString(pCellAnim, "Image", pCellAnim);
		if (iswall)
		{
			CellAnimImageID = "";
		}
	}

	auto loadSingleFrameShape = [&](FString name, int nFrame = 0, int deltaX = 0,
		int deltaY = 0, FString customPal = "", bool shadow = false, int forceNewTheater = -1) -> bool
	{
		FString CurrentLoadingAnim;
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
		if (!HasFile(file, nMix))
		{
			SetGenericTheaterLetter(file);
			nMix = SearchFile(file);
			if (!HasFile(file, nMix))
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
					if (!HasFile(file, nMix))
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
		if (!CMixFile::LoadSHP(file, nMix))
			return false;
		CShpFile::GetSHPHeader(&header);
		if (header.FrameCount <= nFrame) {
			nFrame = 0;
		}
		CLoadingExt::LoadSHPFrameSafe(nFrame, 1, &pBuffer, header);
		
		UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, false);

		if (shadow && ExtConfigs::InGameDisplay_Shadow)
		{
			CLoadingExt::LoadSHPFrameSafe(nFrame + header.FrameCount / 2, 1, &pBuffer, header);
			UnionSHP_Add(pBuffer, header.Width, header.Height, deltaX, deltaY, false, true);
		}

		return true;
	};

	auto loadAnimFrameShape = [&](bool shadow)
	{
		int nStartFrame = CINI::Art->GetInteger(CellAnimImageID, "LoopStart");
		int deltaX = CINI::Art->GetInteger(CellAnimImageID, "XDrawOffset");
		int deltaY = CINI::Art->GetInteger(CellAnimImageID, "YDrawOffset");
		loadSingleFrameShape(CellAnimImageID, nStartFrame, deltaX, deltaY, "", shadow);
	};

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

		unsigned char* pBuffer[2]{ 0 };
		int width = 1, height = 1;
		bool cellAnimShadow = CINI::Art->GetBool(CellAnimImageID, "Shadow") && ExtConfigs::InGameDisplay_Shadow;
		if (!CellAnimImageID.empty())
		{
			loadAnimFrameShape(cellAnimShadow);
			UnionSHP_GetAndClear(pBuffer[0], &width, &height, false, false);
			if (cellAnimShadow)
			{
				UnionSHP_GetAndClear(pBuffer[1], &width, &height, false, true);
			}
		}

		ShapeHeader header;
		unsigned char* FramesBuffers[2]{ 0 };
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

				FString DictName = GetOverlayName(nIndex, i);

				CLoadingExt::LoadSHPFrameSafe(i, 1, &FramesBuffers[0], header);

				if (ExtConfigs::InGameDisplay_Shadow && (i < header.FrameCount / 2))
				{
					CLoadingExt::LoadSHPFrameSafe(i + header.FrameCount / 2, 1, &FramesBuffers[1], header);
				}

				if (!CellAnimImageID.empty())
				{
					int offset = -60;

					offset += CIsoViewExt::GetOverlayDrawOffset(nIndex, i);

					// use CellAnim palette instead
					FString customPal = CINI::Art->GetString(CellAnimImageID, "CustomPalette", "anim.pal");
					Palette* cellAnimPal = nullptr;
					if (istiberium)
						customPal = "unit~~~.pal";
					GetFullPaletteName(customPal);
					if (customPal != "")
					{
						if (istiberium)
						{
							ppmfc::CString type;
							if (nIndex >= RIPARIUS_BEGIN && nIndex <= RIPARIUS_END)
								type = "Riparius";
							else if (nIndex >= CRUENTUS_BEGIN && nIndex <= CRUENTUS_END)
								type = "Cruentus";
							else if (nIndex >= VINIFERA_BEGIN && nIndex <= VINIFERA_END)
								type = "Vinifera";
							else if (nIndex >= ABOREUS_BEGIN && nIndex <= ABOREUS_END)
								type = "Aboreus";
							else
								type = "Riparius";

							auto color = Miscs::GetColorRef(type);
							BGRStruct c;
							c.R = GetRValue(color);
							c.G = GetGValue(color);
							c.B = GetBValue(color);
							cellAnimPal = PalettesManager::LoadTiberiumCellAnimPalette(c, customPal);
						}
						else
							cellAnimPal = PalettesManager::LoadPalette(customPal);

						if (cellAnimPal)
						{
							std::vector<int> lookupTable = GeneratePalLookupTable(palette, cellAnimPal);
							int counter = 0;
							for (int j = 0; j < header.Height; ++j)
							{
								for (int i = 0; i < header.Width; ++i)
								{
									unsigned char& ch = FramesBuffers[0][counter];
									ch = lookupTable[ch];
									counter++;
								}
							}
						}
					}

					unsigned char* TempBuffers[2]{ 0 };
					unsigned char* pOutBuffers[2]{ 0 };
					TempBuffers[0] = GameCreateArray<BYTE>(width * height);
					memcpy(TempBuffers[0], pBuffer[0], width * height);
					if (FramesBuffers[0])
					{
						UnionSHP_Add(FramesBuffers[0], header.Width, header.Height, 0, 0, false, false);
					}
					else
					{
						FramesBuffers[0] = GameCreateArray<BYTE>(1);
						FramesBuffers[0][0] = 0;
						UnionSHP_Add(FramesBuffers[0], 1, 1, 0, 0, false, false);
					}
					if (TempBuffers[0])
						UnionSHP_Add(TempBuffers[0], width, height, 0, -offset, false, false);

					int widthAll, heightAll;
					UnionSHP_GetAndClear(pOutBuffers[0], &widthAll, &heightAll, false, false);
					SetImageDataSafe(pOutBuffers[0], DictName, widthAll, heightAll, cellAnimPal ? cellAnimPal : palette, false);

					if (cellAnimShadow)
					{
						TempBuffers[1] = GameCreateArray<BYTE>(width * height);
						memcpy(TempBuffers[1], pBuffer[1], width * height);
						if (FramesBuffers[1])
						{
							UnionSHP_Add(FramesBuffers[1], header.Width, header.Height, 0, 0, false, true);
						}
						else
						{
							FramesBuffers[1] = GameCreateArray<BYTE>(1);
							FramesBuffers[1][0] = 0;
							UnionSHP_Add(FramesBuffers[1], 1, 1, 0, 0, false, true);
						}
						if (TempBuffers[1])
							UnionSHP_Add(TempBuffers[1], width, height, 0, -offset, false, true);

						int widthAll, heightAll;
						FString DictNameShadow = GetOverlayName(nIndex, i, true);
						UnionSHP_GetAndClear(pOutBuffers[1], &widthAll, &heightAll, false, true);
						SetImageDataSafe(pOutBuffers[1], DictNameShadow, widthAll, heightAll, &CMapDataExt::Palette_Shadow);
					}
				}
				else
				{
					SetImageDataSafe(FramesBuffers[0], DictName, header.Width, header.Height, palette, false);
					if (ExtConfigs::InGameDisplay_Shadow && (i < header.FrameCount / 2))
					{
						FString DictNameShadow = GetOverlayName(nIndex, i, true);
						SetImageDataSafe(FramesBuffers[1], DictNameShadow, header.Width, header.Height, &CMapDataExt::Palette_Shadow);
					}
				}
			}
		}

		GameDeleteArray(pBuffer[0], width * height);
		if (cellAnimShadow)
		{
			GameDeleteArray(pBuffer[1], width * height);
		}
	}
}

ImageDataClassSurface* CLoadingExt::GetOrLoadFlagOrCelltagFromMap(COLORREF newColor, bool IsFlag)
{
	auto& map = IsFlag ? CustomFlagMap : CustomCelltagMap;
	auto itr = map.find(newColor);
	if (itr == map.end())
	{
		auto ret = std::make_unique<ImageDataClassSurface>();

		HBITMAP hBmp = (HBITMAP)LoadImage(static_cast<HINSTANCE>(FA2sp::hInstance), MAKEINTRESOURCE(IsFlag ? 1023 : 1024),
			IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		CBitmap cBitmap;
		cBitmap.Attach(hBmp);
		auto r = ReplaceBitmapColor(cBitmap, 
			IsFlag ? (COLORREF)ExtConfigs::DisplayColor_Waypoint 
			: (COLORREF)ExtConfigs::DisplayColor_Celltag,
			newColor);

		auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
		ret->lpSurface = CIsoViewExt::BitmapToSurface(pIsoView->lpDD7, cBitmap);
		DDSURFACEDESC2 desc;
		memset(&desc, 0, sizeof(DDSURFACEDESC2));
		desc.dwSize = sizeof(DDSURFACEDESC2);
		desc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
		ret->lpSurface->GetSurfaceDesc(&desc);
		ret->FullWidth = desc.dwWidth;
		ret->FullHeight = desc.dwHeight;
		ret->Flag = ImageDataFlag::SurfaceData;
		CIsoView::SetColorKey(ret->lpSurface, RGB(255, 255, 255));

		auto [it, inserted] = map.emplace(newColor, std::move(ret));
		return it->second.get();

	}
	return itr->second.get();
}

void* CLoadingExt::ReadWholeFile(const char* filename, DWORD* pDwSize, bool fa2path)
{
	using Clock = std::chrono::steady_clock;
	const uint64_t nowMs =
		std::chrono::duration_cast<std::chrono::milliseconds>(
			Clock::now().time_since_epoch()).count();

	constexpr uint64_t CACHE_TTL_MS = 500;
	constexpr uint64_t CLEANUP_INTERVAL_MS = 1000;

	auto it = g_cache[fa2path].find(filename);
	if (it != g_cache[fa2path].end())
	{
		uint64_t lastUsed = g_cacheTime[fa2path][filename];
		const auto& src = it->second;
		auto pBuffer = GameCreateArray<unsigned char>(src.size());
		memcpy(pBuffer, src.data(), src.size());
		if (pDwSize)
			*pDwSize = (DWORD)src.size();

		g_cacheTime[fa2path][filename] = nowMs;

		if (nowMs - lastUsed > CACHE_TTL_MS)
		{
			g_cache[fa2path].erase(it);
			g_cacheTime[fa2path].erase(filename);
		}

		return pBuffer;
	}

	FString filepath;
	std::ifstream fin;

	auto readFile = [&](const std::string& path) -> std::vector<unsigned char>
	{
		std::ifstream f(path, std::ios::in | std::ios::binary);
		if (!f.is_open())
			return {};

		f.seekg(0, std::ios::end);
		int size = (int)f.tellg();
		if (size <= 0)
			return {};

		f.seekg(0, std::ios::beg);
		std::vector<unsigned char> buffer(size);
		f.read((char*)buffer.data(), size);
		return buffer;
	};

	std::vector<unsigned char> loadedData;

	if (fa2path)
	{
		filepath = CFinalSunApp::ExePath();
		filepath += filename;
		loadedData = readFile(filepath.c_str());
	}
	else
	{
		filepath = CFinalSunApp::ExePath();
		filepath += "Resources\\HighPriority\\";
		filepath += filename;
		loadedData = readFile(filepath.c_str());

		if (loadedData.empty())
		{
			filepath = CFinalSunApp::FilePath();
			filepath += filename;
			loadedData = readFile(filepath.c_str());
		}
	}

	if (loadedData.empty())
	{
		size_t size = 0;
		auto data = ResourcePackManager::instance().getFileData(filename, &size);
		if (data && size > 0)
			loadedData.assign(data.get(), data.get() + size);
	}

	if (loadedData.empty() && ExtConfigs::ExtMixLoader)
	{
		auto& manager = MixLoader::Instance();
		size_t sizeM = 0;
		auto result = manager.LoadFile(filename, &sizeM);
		if (result && sizeM > 0)
			loadedData.assign(result.get(), result.get() + sizeM);
	}

	if (loadedData.empty())
	{
		auto nMix = CLoading::Instance->SearchFile(filename);
		if (CMixFile::HasFile(filename, nMix))
		{
			Ccc_file file(true);
			file.open(filename, CMixFile::Array[nMix - 1]);
			loadedData.assign(
				(unsigned char*)file.get_data(),
				(unsigned char*)file.get_data() + file.get_size());
			file.close();
		}
	}

	if (loadedData.empty())
	{
		filepath = CFinalSunApp::ExePath();
		filepath += "Resources\\LowPriority\\";
		filepath += filename;
		loadedData = readFile(filepath.c_str());
	}

	if (loadedData.empty())
		return nullptr;

	g_cache[fa2path][filename] = loadedData;
	g_cacheTime[fa2path][filename] = nowMs;

	if (nowMs - g_lastCleanup > CLEANUP_INTERVAL_MS)
	{
		for (auto it2 = g_cacheTime[fa2path].begin(); it2 != g_cacheTime[fa2path].end();)
		{
			if (nowMs - it2->second > CACHE_TTL_MS)
			{
				g_cache[fa2path].erase(it2->first);
				it2 = g_cacheTime[fa2path].erase(it2);
			}
			else
			{
				++it2;
			}
		}
		g_lastCleanup = nowMs;
	}

	auto pBuffer = GameCreateArray<unsigned char>(loadedData.size());
	memcpy(pBuffer, loadedData.data(), loadedData.size());
	if (pDwSize)
		*pDwSize = (DWORD)loadedData.size();

	return pBuffer;
}

bool CLoadingExt::HasFile(ppmfc::CString filename, int nMix)
{
	FString filepath;
	std::ifstream fin;

	filepath = CFinalSunApp::ExePath();
	filepath += "Resources\\HighPriority\\";
	filepath += filename;
	fin.open(filepath, std::ios::in | std::ios::binary);
	if (!fin.is_open())
	{
		filepath = CFinalSunApp::FilePath();
		filepath += filename;
		fin.open(filepath, std::ios::in | std::ios::binary);
	}

	if (fin.is_open())
	{
		fin.close();
		return true;
	}

	size_t size = 0;
	auto data = ResourcePackManager::instance().getFileData(filename.m_pchData, &size);
	if (data && size > 0)
	{
		return true;
	}

	if (ExtConfigs::ExtMixLoader)
	{
		auto& manager = MixLoader::Instance();
		int result = manager.QueryFileIndex(filename.m_pchData, nMix);
		if (result >= 0)
			return true;
	}

	if (nMix == -114)
	{
		nMix = CLoading::Instance->SearchFile(filename);
		if (CMixFile::HasFile(filename, nMix))
			return true;
	}
	if (CMixFile::HasFile(filename, nMix))
		return true;

	filepath = CFinalSunApp::ExePath();
	filepath += "Resources\\LowPriority\\";
	filepath += filename;
	fin.open(filepath, std::ios::in | std::ios::binary);
	if (fin.is_open())
	{
		fin.close();
		return true;
	}
	return false;
}

