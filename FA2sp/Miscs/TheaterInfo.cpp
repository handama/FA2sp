#include "TheaterInfo.h"

#include <Helpers/Macro.h>

#include <CTileTypeClass.h>
#include <CLoading.h>
#include <CMapData.h>
#include <CINI.h>

#include <vector>

#include "../Helpers/STDHelpers.h"
#include "../FA2sp.h"
#include "../Ext/CMapData/Body.h"

const char* TheaterInfo::GetInfoSection()
{
	switch (CLoading::Instance->TheaterIdentifier)
	{
	case 'A':
		return "SnowInfo";
	case 'U':
		return "UrbanInfo";
	case 'D':
		return "DesertInfo";
	case 'L':
		return "LunarInfo";
	case 'N':
		return "NewUrbanInfo";
	case 'T':
	default:
		return "TemperateInfo";
	}
}

void TheaterInfo::UpdateTheaterInfo()
{
	CurrentInfoHasCliff2 = false;
	CurrentInfo.clear();
	ppmfc::CString pSection = GetInfoSection();
	ppmfc::CString buffer = CINI::FAData->GetString(pSection, "Morphables");
	ppmfc::CString buffer2 = CINI::FAData->GetString(pSection, "Ramps");
	auto SplitM = STDHelpers::SplitString(buffer);
	auto SplitR = STDHelpers::SplitString(buffer2);
	int i = 0;
	for (int i = 0; i < std::min(SplitM.size(), SplitR.size()); ++i)
	{
		auto& str = SplitM[i];
		auto& str2 = SplitR[i];
		auto morphableSlides = STDHelpers::SplitString(str, 1, "#");
		auto morphableTileset = atoi(morphableSlides[0]);
		auto morphableIndex = morphableSlides[1].IsEmpty() ? 0 : atoi(morphableSlides[1]);

		if (!CMapDataExt::IsValidTileSet(morphableTileset) || !CMapDataExt::IsValidTileSet(atoi(str2)))
			continue;

		CurrentInfo.emplace_back(InfoStruct());
		CurrentInfo.back().Morphable = morphableTileset;
		CurrentInfo.back().MorphableIndex = CMapDataExt::TileSet_starts[CurrentInfo.back().Morphable] + morphableIndex;
		CurrentInfo.back().Ramp = atoi(str2);
		CurrentInfo.back().RampIndex = CMapDataExt::TileSet_starts[CurrentInfo.back().Ramp];
	}
	auto cliff2 = CINI::FAData->GetInteger(pSection, "Cliffs2", 1919810);
	auto cliffWater2 = CINI::FAData->GetInteger(pSection, "CliffsWater2", 1919810);
	if (CMapDataExt::IsValidTileSet(cliff2) && CMapDataExt::IsValidTileSet(cliffWater2))
	{
		CMapData::Cliff2 = cliff2;
		CMapData::Cliffs2Count = CMapDataExt::TileSet_starts[cliff2];
		CMapData::CliffWaters2 = cliffWater2;
		CurrentInfoHasCliff2 = true;
	}

	CurrentInfoNonMorphable.clear();
	// Forward compatibility
	ppmfc::CString pSections[2] = { pSection, pSection + "2" };
	for (int k = 0; k < 2; ++k)
	{
		buffer = CINI::FAData->GetString(pSections[i], "AddTiles");
		for (auto& str : STDHelpers::SplitString(buffer))
		{
			auto morphableSlides = STDHelpers::SplitString(str, 1, "#");
			auto morphableTileset = atoi(morphableSlides[0]);
			auto morphableIndex = morphableSlides[1].IsEmpty() ? 0 : atoi(morphableSlides[1]);
			if (!CMapDataExt::IsValidTileSet(morphableTileset))
				continue;

			CurrentInfoNonMorphable.emplace_back(InfoStruct());
			CurrentInfoNonMorphable.back().Morphable = morphableTileset;
			CurrentInfoNonMorphable.back().MorphableIndex = CMapDataExt::TileSet_starts[CurrentInfoNonMorphable.back().Morphable] + morphableIndex;
		}
	}
}
std::vector<InfoStruct> TheaterInfo::CurrentInfo;
std::vector<InfoStruct> TheaterInfo::CurrentInfoNonMorphable;
std::set<int> TheaterInfo::CurrentBigWaters;
std::set<int> TheaterInfo::CurrentSmallWaters;
bool TheaterInfo::CurrentInfoHasCliff2 = false;

DEFINE_HOOK(49D121, CMapData_UpdateINIFile_UpdateTheaterInfos, 7)
{
	TheaterInfo::UpdateTheaterInfo();
	return 0;
}
//
//DEFINE_HOOK(45BE1D, CIsoView_OnMouseMove_TheaterInfo, 5)
//{
//	if (ExtConfigs::ExtraRaiseGroundTerrainSupport)
//	{
//		GET(CTileTypeClass*, pTile, EBP);
//		GET(const int, nRampType, EBX);
//		GET(int, nRampIndex, EDI);
//		GET_STACK(unsigned int, nIndex, STACK_OFFS(0x3D544, 0x3D530));
//
//		if (nRampType == -1)
//		{
//			for (auto& info : TheaterInfo::CurrentInfo)
//				if (info.Ramp == pTile->TileSet || info.Morphable == pTile->TileSet)
//					if (pTile->Morphable)
//					{
//						CMapData::Instance->SetTileAt(nIndex, info.MorphableIndex);
//						break;
//					}
//		}
//
//		if (pTile->Morphable && nRampType != -1)
//		{
//			for (auto& info : TheaterInfo::CurrentInfo)
//				if (info.Ramp == pTile->TileSet || info.Morphable == pTile->TileSet)
//				{
//					nRampIndex = info.RampIndex;
//					break;
//				}
//
//			CMapData::Instance->SetTileAt(nIndex, nRampIndex + nRampType - 1);
//		}	return 0x45BEAF;
//	}
//	return 0;
//}
//
//DEFINE_HOOK(45E0CB, CMapData_sub_45D090_TheaterInfo_1, 6)
//{
//	if (ExtConfigs::ExtraRaiseGroundTerrainSupport)
//	{
//		GET_STACK(CTileTypeClass*, pTile, STACK_OFFS(0xF8, 0xAC));
//		GET_STACK(const int, nRampType, STACK_OFFS(0xF8, 0xE0));
//
//		enum { LABEL_290 = 0x45E7A6, HasInfo = 0x45E117 };
//
//		if (nRampType != -1 || !pTile->Morphable)
//			return LABEL_290;
//
//		for (auto& info : TheaterInfo::CurrentInfo)
//			if (info.Ramp == pTile->TileSet || info.Morphable == pTile->TileSet)
//			{
//				R->EAX(info.MorphableIndex);
//				return HasInfo; // SetTileAt
//			}
//
//		return LABEL_290;
//	}
//	return 0;
//}
//
//// LABEL_290
//DEFINE_HOOK(45E7A6, CMapData_sub_45D090_TheaterInfo_2, 7)
//{
//	if (ExtConfigs::ExtraRaiseGroundTerrainSupport)
//	{
//		GET_STACK(CTileTypeClass*, pTile, STACK_OFFS(0xF8, 0xAC));
//		GET_STACK(const int, nRampType, STACK_OFFS(0xF8, 0xE0));
//		GET_STACK(int, nRampIndex, STACK_OFFS(0xF8, 0x20));
//		GET_STACK(const int, nIndex, STACK_OFFS(0xF8, -0x4));
//
//		if (pTile->Morphable && nRampType != -1)
//		{
//			nRampIndex = R->Stack<int>(STACK_OFFS(0xF8, 0x20));
//			for (auto& info : TheaterInfo::CurrentInfo)
//				if (info.Ramp == pTile->TileSet || info.Morphable == pTile->TileSet)
//				{
//					nRampIndex = info.RampIndex;
//					break;
//				}
//
//			CMapData::Instance->SetTileAt(nIndex, nRampIndex + nRampType - 1);
//		}
//
//		return 0x45E801;
//	}
//	return 0;
//}

// this was originally disabled
//
//DEFINE_HOOK(46BB36, CMapData_sub_46AB30_TheaterInfo, A)
//{
//	GET(CTileTypeClass*, pTile, EBP);
//	GET(const int, nRampType, EBX);
//	GET(int, nRampIndex, EDI);
//	GET(const int, nIndex, ESI);
//
//	enum { LABEL_320 = 0x46BBC2 };
//
//	if (nRampType == -1)
//	{
//		for (auto& info : TheaterInfo::CurrentInfo)
//			if (info.Ramp == pTile->TileSet || info.Morphable == pTile->TileSet)
//			{
//				if (!pTile->Morphable)
//					return LABEL_320;
//				CMapData::Instance->sub_416550(nIndex, info.MorphableIndex);
//				break;
//			}
//	}
//
//	if (pTile->Morphable && nRampType != -1)
//	{
//		for (auto& info : TheaterInfo::CurrentInfo)
//			if (info.Ramp == pTile->TileSet || info.Morphable == pTile->TileSet)
//			{
//				nRampIndex = info.RampIndex;
//				break;
//			}
//
//		CMapData::Instance->sub_416550(nIndex, nRampIndex + nRampType - 1);
//	}
//
//	return LABEL_320;
//}

//DEFINE_HOOK_AGAIN(4662E8, CIsoView_OnLButtonDown_TheaterInfo, 5)
//DEFINE_HOOK_AGAIN(465BBF, CIsoView_OnLButtonDown_TheaterInfo, 5)
//DEFINE_HOOK_AGAIN(46547C, CIsoView_OnLButtonDown_TheaterInfo, 5)
//DEFINE_HOOK(4649D0, CIsoView_OnLButtonDown_TheaterInfo, 5)
//{
//	if (ExtConfigs::ExtraRaiseGroundTerrainSupport)
//	{
//		GET(CTileTypeClass*, pTile, EDI);
//		GET(int, nRampType, ECX);
//		GET(int, nRampIndex, EBX);
//
//		GET_BASE(const int, nIndex, R->Origin() == 0x46547C || R->Origin() == 0x4649D0 ? 0x10 : 0x14);
//
//		if (nRampType == -1)
//		{
//			for (auto& info : TheaterInfo::CurrentInfo)
//				if (info.Ramp == pTile->TileSet || info.Morphable == pTile->TileSet)
//					if (pTile->Morphable)
//					{
//						CMapData::Instance->SetTileAt(nIndex, info.MorphableIndex);
//						nRampType = R->Base<int>(0x8);
//						break;
//					}
//		}
//
//		if (pTile->Morphable && nRampType != -1)
//		{
//			for (auto& info : TheaterInfo::CurrentInfo)
//				if (info.Ramp == pTile->TileSet || info.Morphable == pTile->TileSet)
//				{
//					nRampIndex = info.RampIndex;
//					break;
//				}
//
//			CMapData::Instance->SetTileAt(nIndex, nRampIndex + nRampType - 1);
//		}
//
//		return R->Origin() + 0x91;
//	}
//	return 0;
//}
//
