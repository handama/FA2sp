#include "../CFinalSunDlg/Body.h"
#include "../../Algorithms/Matrix3D.h"
#include "../../ExtraWindow/CTerrainGenerator/CTerrainGenerator.h"
#include "../../Helpers/Translations.h"
#include "../../Miscs/Hooks.INI.h"
#include "../../Miscs/MultiSelection.h"
#include "../../Miscs/Palettes.h"
#include "../CFinalSunApp/Body.h"
#include "../CIsoView/Body.h"
#include "../CIsoView/DirectXCore.h"
#include "../CLoading/Body.h"
#include "../CMapData/Body.h"
#include "RendererTypes.h"
#include <CLoading.h>
#include <CPalette.h>
#include <Drawing.h>
#include <Helpers/Macro.h>
#include <Miscs/Miscs.h>
#include <atomic>
#include <chrono>
#include <codecvt>
#include <functional>
#include <thread>
#include <CUpdateProgress.h>

static CRect window;
static MapCoord VisibleCoordTL;
static MapCoord VisibleCoordBR;
static ppmfc::CPoint ViewPosition;
using DrawCall = std::function<void()>;

std::unordered_set<short> CIsoViewExt::VisibleStructures;
std::unordered_set<short> CIsoViewExt::VisibleInfantries;
std::unordered_set<short> CIsoViewExt::VisibleUnits;
std::unordered_set<short> CIsoViewExt::VisibleAircrafts;
FHashSet CIsoViewExt::MapRendererIgnoreObjects;
std::vector<EditedMarks> CIsoViewExt::DrawEditedMarks;

struct CellInfo
{
	int X, Y;
	int screenX, screenY;
	int pos;
	bool isInMap;
	CellData *cell;
	CellDataExt *cellExt;
	bool aroundRedrawCell;
};

static std::vector<std::pair<MapCoord, FString>> WaypointsToDraw;
static std::vector<std::pair<MapCoord, FString>> OverlayTextsToDraw;
static std::vector<std::pair<MapCoord, FString>> TerrainTextsToDraw;
static std::vector<std::pair<MapCoord, FString>> SmudgeTextsToDraw;
static std::vector<std::pair<MapCoord, FString>> BaseNodeTextsToDraw;
static std::vector<std::pair<MapCoord, DrawBuildings>> BuildingsToDraw;
static std::vector<std::pair<MapCoord, ImageDataClassSafe *>> AlphaImagesToDraw;
static std::vector<std::pair<MapCoord, ImageDataClassSafe *>> FiresToDraw;
static std::vector<Veterancy> DrawVeterancies;
static std::vector<CellInfo> visibleCells;
static bool DrawnBuildings[SHRT_MAX + 1];
static std::vector<BaseNodeDataExt> DrawnBaseNodes;
static WORD coordToIndex[512][512];
static std::vector<char> shadowMask_Building_Infantry;
static std::vector<char> shadowMask_Terrain;
static std::vector<char> shadowMask_Overlay;
static std::vector<byte> shadowMask;
static std::vector<byte> shadowHeightMask;
static std::vector<int> cellHeightMask;
static std::vector<char> objectOverlapMask;
static std::vector<MapCoord> RedrawCoords;
static std::vector<ppmfc::CString *> Celltags;
static std::vector<const ppmfc::CString *> Waypoints;

#define EXTRA_BORDER 15

inline static bool IsCoordInWindow(int X, int Y)
{
	return X + Y > VisibleCoordTL.X + VisibleCoordTL.Y - EXTRA_BORDER &&
		   X + Y < VisibleCoordBR.X + VisibleCoordBR.Y + CIsoViewExt::EXTRA_BORDER_BOTTOM &&
		   X > Y + VisibleCoordBR.X - VisibleCoordBR.Y - EXTRA_BORDER - 8 &&
		   X < Y + VisibleCoordTL.X - VisibleCoordTL.Y + EXTRA_BORDER;
}

inline static bool IsCoordInWindowButOnBottom(int X, int Y)
{
	return X + Y > VisibleCoordTL.X + VisibleCoordTL.Y - EXTRA_BORDER &&
		   X + Y > VisibleCoordBR.X + VisibleCoordBR.Y + CIsoViewExt::EXTRA_BORDER_BOTTOM &&
		   X > Y + VisibleCoordBR.X - VisibleCoordBR.Y - EXTRA_BORDER - 8 &&
		   X < Y + VisibleCoordTL.X - VisibleCoordTL.Y + EXTRA_BORDER;
}

inline static void GetUnitImageID(FString &ImageID, const CUnitDataFS &obj, const LandType &landType)
{
	if (ExtConfigs::InGameDisplay_Water)
	{
		if ((landType == LandType::Water || landType == LandType::Beach) && obj.IsAboveGround != "1")
		{
			ImageID = Variables::RulesMap.GetString(obj.TypeID, "WaterImage", obj.TypeID);
		}
	}
	if (ExtConfigs::InGameDisplay_Damage)
	{
		int HP = atoi(obj.Health);
		if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP)
		{
			ImageID = Variables::RulesMap.GetString(obj.TypeID, "Image.ConditionYellow", ImageID);
		}
		if (static_cast<int>((CMapDataExt::ConditionRed + 0.001f) * 256) > HP)
		{
			ImageID = Variables::RulesMap.GetString(obj.TypeID, "Image.ConditionRed", ImageID);
		}
		if (ExtConfigs::InGameDisplay_Water)
		{
			if ((landType == LandType::Water || landType == LandType::Beach) && obj.IsAboveGround != "1")
			{
				if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP)
				{
					ImageID = Variables::RulesMap.GetString(obj.TypeID, "WaterImage.ConditionYellow", ImageID);
				}
				if (static_cast<int>((CMapDataExt::ConditionRed + 0.001f) * 256) > HP)
				{
					ImageID = Variables::RulesMap.GetString(obj.TypeID, "WaterImage.ConditionRed", ImageID);
				}
			}
		}
	}
	// UnloadingClass is prior
	if (ExtConfigs::InGameDisplay_Deploy && obj.Status == "Unload")
	{
		ImageID = Variables::RulesMap.GetString(obj.TypeID, "UnloadingClass", ImageID);
	}
}

struct RecursionGuard
{
	FSet &stack;
	const FString &id;

	RecursionGuard(FSet &s, const FString &i)
		: stack(s), id(i)
	{
		stack.insert(id);
	}

	~RecursionGuard()
	{
		stack.erase(id);
	}
};

static void DrawTechnoAttachments(
	DrawCall originalDraw,
	FSet &recursionStack,
	Renderer::TechnoType *pType,
	int oriFacing,
	CellData *cell,
	LPVOID lpSurface,
	DDBoundary &boundary,
	int displayX,
	int displayY,
	COLORREF color,
	bool isShadow)
{
	if (auto infosOri = pType->TechnoAttachmentInfo)
	{
		if (recursionStack.contains(pType->ID))
			return;

		RecursionGuard guard(recursionStack, pType->ID);

		auto pThis = CIsoView::GetInstance();

		auto infos = *infosOri;
		auto calcGroupAndY = [&](const TechnoAttachment &a, int &outY) -> int
		{
			int ParentFacings = pType->FacingCount;
			int parentFacing = (oriFacing * ParentFacings / 256) % ParentFacings;
			Matrix3D mat(a.F, a.L, 0, parentFacing, ParentFacings);

			outY = mat.OutputY;

			if (a.YSortPosition == TechnoAttachment::YSortPosition::Bottom ||
				(a.YSortPosition == TechnoAttachment::YSortPosition::Default && outY < 0))
			{
				return 0;
			}

			if (a.YSortPosition == TechnoAttachment::YSortPosition::Default)
			{
				return 1;
			}

			// YSortPosition == Top
			return 2;
		};

		std::stable_sort(
			infos.begin(),
			infos.end(),
			[&](const TechnoAttachment &a, const TechnoAttachment &b)
			{
				int yA, yB;
				int gA = calcGroupAndY(a, yA);
				int gB = calcGroupAndY(b, yB);

				if (gA != gB)
					return gA < gB;

				return yA < yB;
			});

		auto firstGroupEnd = infos.end();
		for (auto it = infos.begin(); it != infos.end(); ++it)
		{
			int y;
			int g = calcGroupAndY(*it, y);

			if (g >= 1 && firstGroupEnd == infos.end())
				firstGroupEnd = it;
		}

		auto eParentType = pType->Type;
		int oriParentFacing = oriFacing;
		std::size_t redrawIndex = std::distance(infos.begin(), firstGroupEnd);

		if (eParentType == CLoadingExt::GameObjectType::Building)
		{
			auto pBuildingType = static_cast<Renderer::BuildingType *>(pType);
			const auto &DataExt = *pBuildingType->pDataExt;
			displayX += (DataExt.RealHeight - DataExt.RealHeight) * 30 / 2;
			displayY += (DataExt.RealWidth + DataExt.RealHeight - 2) * 15 / 2 + 15;
		}
		for (int i = 0; i < infos.size(); ++i)
		{
			const auto &info = infos[i];
			if (recursionStack.contains(info.ID))
				continue;

			if (eParentType == CLoadingExt::GameObjectType::Building && !info.IsOnTurret)
			{
				oriFacing = 0;
			}
			else
			{
				oriFacing = oriParentFacing;
			}

			if (redrawIndex > 0 && i == redrawIndex)
			{
				originalDraw();
			}

			auto eItemType = CLoadingExt::GetExtension()->GetItemType(info.ID);
			switch (eItemType)
			{
			case CLoadingExt::GameObjectType::Infantry:
			{
				auto pInfantry = Renderer::GetOrCreateInfantry(info.ID);
				int facings = 8;
				int ParentFacings = pType->FacingCount;
				int parentFacing = (oriFacing * ParentFacings / 256) % ParentFacings;
				int newFacing = (7 - (oriFacing + info.RotationAdjust) / 32 + facings) % facings;

				auto pData = pInfantry->GetTechnoAttachmentImageData(newFacing, isShadow);

				if (!isShadow)
				{
					Matrix3D mat(info.F, info.L, info.H, parentFacing, ParentFacings);

					auto draw = [&]
					{
						if (ImageDataClassSafe::IsValidImage(pData))
						{
							if (ExtConfigs::DirectXRendering)
							{
								CIsoViewExt::DirectXNormal(
									displayX - pData->FullWidth / 2 + mat.OutputX + info.DeltaX,
									displayY - pData->FullHeight / 2 + mat.OutputY + info.DeltaY,
									pData, NULL,
									ExtConfigs::InGameDisplay_Cloakable && pInfantry->Cloakable ? 0.5f : 1.0f, color, 1, true);
							}
							else
							{
								CIsoViewExt::BlitSHPTransparent(pThis, lpSurface, window, boundary,
																displayX - pData->FullWidth / 2 + mat.OutputX + info.DeltaX,
																displayY - pData->FullHeight / 2 + mat.OutputY + info.DeltaY,
																pData, NULL,
																ExtConfigs::InGameDisplay_Cloakable && pInfantry->Cloakable ? 128 : 255, color, 1, true);
							}
						}
					};

					draw();

					if (CIsoViewExt::DrawVeterancy)
					{
						auto &veter = DrawVeterancies.emplace_back();
						veter.X = displayX + mat.OutputX + info.DeltaX;
						veter.Y = displayY + mat.OutputY + info.DeltaY;
						veter.VP = 0;
						veter.ID = info.ID;
					}

					DrawTechnoAttachments(draw, recursionStack, pInfantry, oriFacing + info.RotationAdjust, cell, lpSurface, boundary,
										  displayX + mat.OutputX + info.DeltaX, displayY + mat.OutputY + info.DeltaY, color, isShadow);
				}
			}
			break;
			case CLoadingExt::GameObjectType::Vehicle:
			{
				auto pVehicle = Renderer::GetOrCreateVehicle(info.ID);
				int facings = pVehicle->FacingCount;
				int ParentFacings = pType->FacingCount;
				int parentFacing = (oriFacing * ParentFacings / 256) % ParentFacings;
				int additionalFacing = (info.RotationAdjust * facings / 256) % facings;
				int newFacing = (parentFacing * facings / ParentFacings + additionalFacing) % facings;

				auto pData = pVehicle->GetTechnoAttachmentImageData(newFacing, isShadow);

				if (!isShadow)
				{
					Matrix3D mat(info.F, info.L, info.H, parentFacing, ParentFacings);
					auto draw = [&]
					{
						if (ImageDataClassSafe::IsValidImage(pData))
						{
							if (ExtConfigs::DirectXRendering)
							{
								CIsoViewExt::DirectXNormal(
									displayX - pData->FullWidth / 2 + mat.OutputX + info.DeltaX,
									displayY - pData->FullHeight / 2 + mat.OutputY + info.DeltaY,
									pData, NULL,
									ExtConfigs::InGameDisplay_Cloakable && pVehicle->Cloakable ? 0.5f : 1.0f, color, 0, true);
							}
							else
							{
								CIsoViewExt::BlitSHPTransparent(pThis, lpSurface, window, boundary,
																displayX - pData->FullWidth / 2 + mat.OutputX + info.DeltaX,
																displayY - pData->FullHeight / 2 + mat.OutputY + info.DeltaY,
																pData, NULL,
																ExtConfigs::InGameDisplay_Cloakable && pVehicle->Cloakable ? 128 : 255, color, 0, true);
							}
						}
					};

					draw();

					if (CIsoViewExt::DrawVeterancy)
					{
						auto &veter = DrawVeterancies.emplace_back();
						veter.X = displayX + mat.OutputX + info.DeltaX;
						veter.Y = displayY + mat.OutputY + info.DeltaY;
						veter.VP = 0;
						veter.ID = info.ID;
					}

					DrawTechnoAttachments(draw, recursionStack, pVehicle, oriFacing + info.RotationAdjust, cell, lpSurface, boundary,
										  displayX + mat.OutputX + info.DeltaX, displayY + mat.OutputY + info.DeltaY, color, isShadow);
				}
				else if (isShadow && pData->pImageBuffer && !(ExtConfigs::InGameDisplay_Cloakable && pVehicle->Cloakable))
				{
					// shadow always on the ground
					Matrix3D mat(info.F, info.L, 0, parentFacing, ParentFacings);
					if (ExtConfigs::DirectXRendering)
					{
						CIsoViewExt::DirectXShadow(
							displayX - pData->FullWidth / 2 + mat.OutputX + info.DeltaX,
							displayY - pData->FullHeight / 2 + mat.OutputY + info.DeltaY,
							pData, cell ? static_cast<byte>(cell->Height) : 0xFF);
					}
					else
					{
						CIsoViewExt::BlitSHPTransparent(pThis, lpSurface, window, boundary,
														displayX - pData->FullWidth / 2 + mat.OutputX + info.DeltaX,
														displayY - pData->FullHeight / 2 + mat.OutputY + info.DeltaY, pData, NULL, 128);
					}

					DrawTechnoAttachments([] {}, recursionStack, pVehicle, oriFacing + info.RotationAdjust, cell, lpSurface, boundary,
										  displayX + mat.OutputX + info.DeltaX, displayY + mat.OutputY + info.DeltaY, color, isShadow);
				}
			}
			break;
			case CLoadingExt::GameObjectType::Aircraft:
			{
				if (!isShadow)
				{
					auto pAircraft = Renderer::GetOrCreateAircraft(info.ID);
					int facings = pAircraft->FacingCount;
					int ParentFacings = pType->FacingCount;
					int parentFacing = (oriFacing * ParentFacings / 256) % ParentFacings;
					int additionalFacing = (info.RotationAdjust * facings / 256) % facings;
					int newFacing = (parentFacing * facings / ParentFacings + additionalFacing) % facings;

					auto pData = pAircraft->GetTechnoAttachmentImageData(newFacing);

					Matrix3D mat(info.F, info.L, info.H, parentFacing, ParentFacings);
					auto draw = [&]
					{
						if (ImageDataClassSafe::IsValidImage(pData))
						{
							if (ExtConfigs::DirectXRendering)
							{
								CIsoViewExt::DirectXNormal(
									displayX - pData->FullWidth / 2 + mat.OutputX + info.DeltaX,
									displayY - pData->FullHeight / 2 + mat.OutputY + info.DeltaY,
									pData, NULL,
									ExtConfigs::InGameDisplay_Cloakable && pAircraft->Cloakable ? 0.5f : 1.0f, color, 2, true);
							}
							else
							{
								CIsoViewExt::BlitSHPTransparent(pThis, lpSurface, window, boundary,
																displayX - pData->FullWidth / 2 + mat.OutputX + info.DeltaX,
																displayY - pData->FullHeight / 2 + mat.OutputY + info.DeltaY,
																pData, NULL,
																ExtConfigs::InGameDisplay_Cloakable && pAircraft->Cloakable ? 128 : 255, color, 2, true);
							}
						}
					};

					draw();

					if (CIsoViewExt::DrawVeterancy)
					{
						auto &veter = DrawVeterancies.emplace_back();
						veter.X = displayX + mat.OutputX + info.DeltaX;
						veter.Y = displayY + mat.OutputY + info.DeltaY;
						veter.VP = 0;
						veter.ID = info.ID;
					}

					DrawTechnoAttachments(draw, recursionStack, pAircraft, oriFacing + info.RotationAdjust, cell, lpSurface, boundary,
										  displayX + mat.OutputX + info.DeltaX, displayY + mat.OutputY + info.DeltaY, color, isShadow);
				}
			}
			break;
			case CLoadingExt::GameObjectType::Building:
			{
				auto pBuilding = Renderer::GetOrCreateBuilding(info.ID);
				int facings = pBuilding->FacingCount;
				int ParentFacings = pType->FacingCount;
				int parentFacing = (oriFacing * ParentFacings / 256) % ParentFacings;

				int newFacing = 0;
				if (facings > 1)
				{
					newFacing = (facings + 7 * facings / 8 -
								 ((oriFacing + info.RotationAdjust) * facings / 256) % facings) %
								facings;
				}

				if (!isShadow)
				{
					auto clips = pBuilding->GetImageData(0, 0, newFacing);
					for (auto &pBldData : *clips)
					{
						if (pBldData)
						{
							auto ArtID = CLoadingExt::GetArtID(info.ID);
							Matrix3D mat(info.F, info.L, info.H, parentFacing, ParentFacings);
							auto draw = [&]
							{
								if (pBldData->pImageBuffer)
								{
									if (ExtConfigs::DirectXRendering)
									{
										CIsoViewExt::DirectXBuilding(
											displayX - pBldData->ClipOffsets.FullWidth / 2 +
												pBldData->ClipOffsets.LeftOffset + mat.OutputX + info.DeltaX,
											displayY - pBldData->FullHeight / 2 + mat.OutputY + info.DeltaY, pBldData.get(),
											NULL, ExtConfigs::InGameDisplay_Cloakable && pBuilding->Cloakable ? 128 : 255,
											color, false, pBuilding->IsTerrainPalette);
									}
									else
									{
										CIsoViewExt::BlitSHPTransparent_Building(pThis, lpSurface, window, boundary,
																				 displayX - pBldData->ClipOffsets.FullWidth / 2 +
																					 pBldData->ClipOffsets.LeftOffset + mat.OutputX + info.DeltaX,
																				 displayY - pBldData->FullHeight / 2 + mat.OutputY + info.DeltaY, pBldData.get(),
																				 NULL, ExtConfigs::InGameDisplay_Cloakable && pBuilding->Cloakable ? 128 : 255,
																				 color, false, pBuilding->IsTerrainPalette);
									}
								}
							};

							draw();

							if (CIsoViewExt::DrawVeterancy)
							{
								const auto &DataExt = *pBuilding->pDataExt;
								auto &veter = DrawVeterancies.emplace_back();
								veter.X = displayX + mat.OutputX + info.DeltaX + (DataExt.RealWidth - DataExt.RealHeight) * 30 / 2;
								veter.Y = displayY + mat.OutputY + info.DeltaY + (DataExt.RealWidth + DataExt.RealHeight - 2) * 15 / 2;
								veter.VP = 0;
								veter.ID = info.ID;
							}

							DrawTechnoAttachments(draw, recursionStack, pBuilding, oriFacing + info.RotationAdjust, cell, lpSurface, boundary,
												  displayX + mat.OutputX + info.DeltaX, displayY + mat.OutputY + info.DeltaY, color, isShadow);
						}
					}
				}
			}
			break;
			case CLoadingExt::GameObjectType::Unknown:
			default:
				break;
			}

			if (redrawIndex == infos.size() && i == infos.size() - 1)
			{
				originalDraw();
			}
		}
	}
}

static void InitAllObjects()
{
	CMapDataExt::RefreshAllWindows();

	if (!ExtConfigs::LoadObjectsOnInit)
		return;

	auto keepAlive = []()
	{
		MSG msg;
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		Sleep(1);
	};

	std::set<WORD> overlays;
	std::vector<BaseNodeDataExt*> baseNodes;
	for (auto& cellExt: CMapDataExt::CellDataExts)
	{
		if (cellExt.NewOverlay != 0xffff)
		{
			overlays.insert(cellExt.NewOverlay);
		}
		for (auto &node : cellExt.BaseNodes)
		{
			baseNodes.push_back(&node);
		}
	}
	int count = CMapDataExt::BuildingDatasExt.size() 
	+ CMapDataExt::UnitDatasExt.size()
	+ CMapDataExt::AircraftDatasExt.size()
	+ CMapData::Instance->InfantryDatas.size()
	+ CMapData::Instance->TerrainDatas.size()
	+ CMapData::Instance->SmudgeDatas.size()
	+ overlays.size() + baseNodes.size();

	int currentPos = 0;

	CUpdateProgress progress(
		Translations::TranslateOrDefault("InitAllObjectsProgressText",
			"Loading objects, please wait..."), NULL);
	progress.ShowWindow(SW_SHOW);
	progress.UpdateWindow();
	progress.ProgressBar.SetRange(0, count);
	progress.ProgressBar.SetPos(0);

	for (int i = 0; i < CMapDataExt::BuildingDatasExt.size(); ++i)
	{
		Renderer::Buildings[i].Reload(i);	
		progress.ProgressBar.SetPos(++currentPos);
		progress.ProgressBar.UpdateWindow();
	}
	keepAlive();
	for (int i = 0; i < CMapDataExt::UnitDatasExt.size(); ++i)
	{
		Renderer::Vehicles[i].Reload(i);
		progress.ProgressBar.SetPos(++currentPos);
		progress.ProgressBar.UpdateWindow();
	}
	keepAlive();
	for (int i = 0; i < CMapDataExt::AircraftDatasExt.size(); ++i)
	{
		Renderer::Aircrafts[i].Reload(i);
		progress.ProgressBar.SetPos(++currentPos);
		progress.ProgressBar.UpdateWindow();
	}
	keepAlive();
	for (int i = 0; i < CMapData::Instance->InfantryDatas.size(); ++i)
	{
		Renderer::Infantries[i].Reload(i);
		progress.ProgressBar.SetPos(++currentPos);
		progress.ProgressBar.UpdateWindow();
	}
	keepAlive();
	for (int i = 0; i < CMapData::Instance->TerrainDatas.size(); ++i)
	{
		auto &obj = CMapData::Instance->TerrainDatas[i].TypeID;
		auto pType = Renderer::GetOrCreateTerrain(obj);
		progress.ProgressBar.SetPos(++currentPos);
		progress.ProgressBar.UpdateWindow();
	}
	keepAlive();
	for (int i = 0; i < CMapData::Instance->SmudgeDatas.size(); ++i)
	{
		auto &obj = CMapData::Instance->SmudgeDatas[i].TypeID;
		auto pType = Renderer::GetOrCreateSmudge(obj);
		progress.ProgressBar.SetPos(++currentPos);
		progress.ProgressBar.UpdateWindow();
	}
	keepAlive();
	for (auto nOvr : overlays)
	{
		auto pType = Renderer::GetOrCreateOverlay(nOvr);
		progress.ProgressBar.SetPos(++currentPos);
		progress.ProgressBar.UpdateWindow();
	}
	keepAlive();
	for (auto& node : baseNodes)
	{
		auto pType = Renderer::GetOrCreateBuilding(node->ID);
		if (ExtConfigs::DirectXRendering)
		{
			auto HouseColor = Miscs::GetColorRef(node->House);
			BGRStruct color(HouseColor);
			bool bunker = pType->CanOccupyFire && pType->TechLevel < 0;
			for (int i = 0; i < pType->FacingCount; ++i)
			{
				for (int j = 0; j < (bunker ? 4 : 3); ++j)
				{
					auto clips = pType->GetImageData(i, j);
					for (auto &pData : *clips)
					{
						if (ImageDataClassSafe::IsValidImage(pData.get()))
						{                   
							auto newPal = PalettesManager::GetColoredPalette(pData->pPalette, color);
							pData->GetBuildingColoredTextures(newPal, color);
						}
					}
	
					auto pData = pType->GetShadowData(i, j);
					if (ImageDataClassSafe::IsValidImage(pData))
					{                   
						pData->GetTexture();
					}
				}
			}
		}
	}
}

static void DrawMap()
{
	auto pThis = CIsoViewExt::GetExtension();
	auto pMap = CMapDataExt::GetExtension();
	auto pFinalSunDlg = CFinalSunDlg::Instance();

	if (ExtConfigs::DirectXRendering)
		pThis->g_pSP->BeginFrame();

	// sanity checks
	{
		if (pMap->MapNotLoaded)
			return;

		if (pThis->CancelDraw)
		{
			pThis->CancelDraw = false;
			return;
		}

		if (pThis->lpDDPrimarySurface == NULL || pThis->IsInitializing || pMap->TileData == NULL || pMap->TileDataCount == 0 || (*CTileTypeClass::Instance) == NULL || (*CTileTypeClass::InstanceCount) == 0 || pMap->MapWidthPlusHeight == 0)
			return;
	}

	DDSURFACEDESC2 ddsd{};
	LPDIRECTDRAWSURFACE7 lpSurface = pThis->GetBackBuffer();

	if (!ExtConfigs::DirectXRendering)
	{
		DDBLTFX fx;
		memset(&fx, 0, sizeof(DDBLTFX));
		fx.dwSize = sizeof(DDBLTFX);
		fx.dwFillColor = CIsoViewExt::RenderingMap ? RGB(0, 0, 0) : (ExtConfigs::EnableDarkMode ? RGB(32, 32, 32) : RGB(255, 255, 255));
	
		lpSurface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &fx);
		ddsd.dwSize = sizeof(DDSURFACEDESC2);
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
		lpSurface->GetSurfaceDesc(&ddsd);
		lpSurface->Lock(NULL, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_NOSYSLOCK, NULL);
		
		if (pThis->lpDDPrimarySurface->IsLost() != DD_OK || ddsd.lpSurface == NULL)
		{
			pThis->PrimarySurfaceLost();
			return;
		}
	}

	DDBoundary boundary{ddsd.dwWidth, ddsd.dwHeight, ddsd.lPitch};

	// clear static containers, init some game logics
	{
		if (CLoadingExt::ObjectsNeedReloaded)
		{
			InitAllObjects();
			CLoadingExt::ObjectsNeedReloaded = false;
		}

		PalettesManager::CalculatedObjectPaletteFiles.clear();
		CIsoViewExt::VisibleStructures.clear();
		CIsoViewExt::VisibleInfantries.clear();
		CIsoViewExt::VisibleUnits.clear();
		CIsoViewExt::VisibleAircrafts.clear();
		CLoadingExt::CurrentFrameImageDataMap.clear();
		WaypointsToDraw.clear();
		OverlayTextsToDraw.clear();
		SmudgeTextsToDraw.clear();
		TerrainTextsToDraw.clear();
		BuildingsToDraw.clear();
		AlphaImagesToDraw.clear();
		FiresToDraw.clear();
		BaseNodeTextsToDraw.clear();
		DrawVeterancies.clear();
		visibleCells.clear();
		memset(coordToIndex, 0, sizeof(coordToIndex));
		memset(DrawnBuildings, 0, sizeof(DrawnBuildings));
		DrawnBaseNodes.clear();
		RedrawCoords.clear();
		Celltags.clear();
		Waypoints.clear();
		if (CIsoViewExt::DrawCelltags)
			if (auto pSection = CINI::CurrentDocument->GetSection("CellTags"))
			{
				Celltags.reserve(pSection->GetEntities().size());
				for (auto &[key, value] : pSection->GetEntities())
					Celltags.push_back(&value);
			}
		if (CIsoViewExt::DrawWaypoints)
			if (auto pSection = CINI::CurrentDocument->GetSection("Waypoints"))
			{
				Waypoints.reserve(pSection->GetEntities().size());
				for (auto &[key, value] : pSection->GetEntities())
					Waypoints.push_back(&key);
			}

		if (PalettesManager::NeedReloadLighting)
		{
			LightingStruct::GetCurrentLighting();
			PalettesManager::NeedReloadLighting = false;
		}
		if (CMapDataExt::Init_OpenMinimap)
		{
			CFinalSunDlg::Instance->MyViewFrame.Minimap.Update();
			CMapDataExt::Init_OpenMinimap = false;
		}

		std::erase_if(CLoadingExt::IFVTurrets, [](auto &pair)
					  {
			auto& ifv = pair.first;
			auto& tur = pair.second;

			if (CLoadingExt::GetIFVTurretIndex(ifv) != tur)
			{
				CLoadingExt::LoadedObjects.erase(ifv);
				return true;
			}
			return false; });
		std::erase_if(CLoadingExt::InitialOccupiedBuildings, [](auto &building)
					  {
			if (!CLoadingExt::IsPreOccupiedBunker(building))
			{
				CLoadingExt::LoadedObjects.erase(building);
				return true;
			}
			return false; });

		if (INIIncludes::MapINIWarn)
		{
			if (!CINI::CurrentDocument->GetBool("FA2spVersionControl", "MapIncludeWarned"))
			{
				int result = MessageBox(CIsoView::GetInstance()->GetSafeHwnd(),
										Translations::TranslateOrDefault("MapIncludeWarningMessage",
																		 "This map contains include INIs. All key value pairs in the INIs will not be saved to the map, nor will be saved to the INIs.\n"
																		 "If you click 'OK', this warning will no longer pop up in this map."),
										Translations::TranslateOrDefault("Warning", "Warning"),
										MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION);

				if (result == IDOK)
					CINI::CurrentDocument->WriteBool("FA2spVersionControl", "MapIncludeWarned", true);
			}
			INIIncludes::MapINIWarn = false;
		}
	}

	// init window positions
	{
		RECT rect;
		::GetClientRect(pThis->GetSafeHwnd(), &rect);
		POINT topLeft = {rect.left, rect.top};
		::ClientToScreen(pThis->GetSafeHwnd(), &topLeft);
		double offsetX = 0.016795436849483363 * topLeft.x - 4.664099013466316;
		double offsetY = 0.03362306232114938 * topLeft.y - 2.4360168849787662;
		ViewPosition = pThis->ViewPosition;
		pThis->ViewPosition.x += offsetX;
		pThis->ViewPosition.y += offsetY;

		pThis->GetWindowRect(&window);
		CIsoViewExt::AdaptRectForSecondScreen(&window);

		double scale = CIsoViewExt::ScaledFactor;
		if (scale < 0.9)
			scale += 0.1;
		if (scale < 0.7)
			scale += 0.1;
		if (scale < 0.5)
			scale += 0.1;
		window.right += window.Width() * (scale - 1.0);
		if (scale < 1.0)
			scale = 1.0;
		window.bottom += window.Height() * (scale - 1.0);

		VisibleCoordTL.X = window.left + pThis->ViewPosition.x;
		VisibleCoordTL.Y = window.top + pThis->ViewPosition.y;
		VisibleCoordBR.X = window.right + pThis->ViewPosition.x;
		VisibleCoordBR.Y = window.bottom + pThis->ViewPosition.y;
		pThis->ScreenCoord2MapCoord_Flat(VisibleCoordTL.X, VisibleCoordTL.Y);
		pThis->ScreenCoord2MapCoord_Flat(VisibleCoordBR.X, VisibleCoordBR.Y);
		if (VisibleCoordBR.X < 0 || VisibleCoordBR.Y < 0)
		{
			VisibleCoordBR.X = CMapData::Instance->Size.Width;
			VisibleCoordBR.Y = CMapData::Instance->MapWidthPlusHeight + 1;
		}
		if (VisibleCoordTL.X < 0 || VisibleCoordTL.Y < 0)
		{
			VisibleCoordTL.X = CMapData::Instance->Size.Width;
			VisibleCoordTL.Y = 0;
		}
	}

	pFinalSunDlg->LastSucceededOperation = 100;

	int DrawOffsetX = pThis->ViewPosition.x;
	int DrawOffsetY = pThis->ViewPosition.y;

	if (ExtConfigs::DirectXRendering)
	{
		DrawOffsetX += window.left;
		DrawOffsetY += window.top;
	}

	CIsoViewExt::drawOffsetX = DrawOffsetX;
	CIsoViewExt::drawOffsetY = DrawOffsetY;

	HDC hDC;
	lpSurface->GetDC(&hDC);

	auto isCoordInFullMap = [](int X, int Y)
	{
		if (!ExtConfigs::DisplayObjectsOutside)
			return CMapData::Instance->IsCoordInMap(X, Y);

		return X >= 0 && Y >= 0 &&
			   X < CMapData::Instance->MapWidthPlusHeight &&
			   Y < CMapData::Instance->MapWidthPlusHeight;
	};

	for (int XplusY = VisibleCoordTL.X + VisibleCoordTL.Y - EXTRA_BORDER;
		 XplusY < VisibleCoordBR.X + VisibleCoordBR.Y + CIsoViewExt::EXTRA_BORDER_BOTTOM;
		 XplusY++)
	{
		for (int X = 0; X < XplusY; X++)
		{
			int Y = XplusY - X;
			if (!IsCoordInWindow(X, Y) || !isCoordInFullMap(X, Y))
				continue;
			int pos = CMapData::Instance->GetCoordIndex(X, Y);
			int screenX = X, screenY = Y;
			CIsoView::MapCoord2ScreenCoord(screenX, screenY);
			screenX -= DrawOffsetX;
			screenY -= DrawOffsetY;
			auto cell = CMapData::Instance->GetCellAt(pos);
			auto &cellExt = CMapDataExt::CellDataExts[pos];
			coordToIndex[X][Y] = (WORD)visibleCells.size();
			visibleCells.push_back({X, Y, screenX, screenY, pos,
									CMapData::Instance->IsCoordInMap(X, Y),
									cell,
									&cellExt,
									false});

			cell->Flag.RedrawTerrain = false;
			cellExt.BuildingRenderParts.clear();
			cellExt.BaseNodeRenderParts.clear();

			if (cell->Structure > -1)
			{
				Renderer::Buildings[cell->Structure].Reload(cell->Structure);
			}

			if (cell->Unit > -1)
			{
				Renderer::Vehicles[cell->Unit].Reload(cell->Unit);
			}
			if (cell->Aircraft > -1)
			{
				Renderer::Aircrafts[cell->Aircraft].Reload(cell->Aircraft);
			}
			for (int i = 0; i < 3; ++i)
			{
				if (cell->Infantry[i] > -1)
				{
					Renderer::Infantries[cell->Infantry[i]].Reload(cell->Infantry[i]);
				}
			}
		}
	}

	auto isCellHidden = [](CellData *pCell)
	{
		return pCell->IsHidden() && (!CIsoViewExt::RenderingMap || CIsoViewExt::RenderingMap && CIsoViewExt::RenderCurrentLayers);
	};

	auto isCloakable = [](Renderer::TechnoType *pType)
	{
		return ExtConfigs::InGameDisplay_Cloakable && pType->Cloakable;
	};

	auto getTileVirtualHeight = [](CellData *cell) -> int
	{
		int altImage = cell->Flag.AltIndex;
		int tileIndex = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
		int tileSubIndex = cell->TileSubIndex;
		if (CFinalSunApp::Instance->FrameMode)
		{
			if (CMapDataExt::TileData[tileIndex].FrameModeIndex < CMapDataExt::TileDataCount)
			{
				tileIndex = CMapDataExt::TileData[tileIndex].FrameModeIndex;
			}
			else
			{
				tileIndex = CMapDataExt::TileSet_starts[CMapDataExt::HeightBase] + cell->Height;
				tileSubIndex = 0;
			}
		}
		tileIndex = CMapDataExt::GetSafeTileIndex(tileIndex);

		CTileTypeClass tile = CMapDataExt::TileData[tileIndex];
		int tileSet = tile.TileSet;
		if (tile.AltTypeCount)
		{
			if (altImage > 0)
			{
				altImage = altImage < tile.AltTypeCount ? altImage : tile.AltTypeCount;
				tile = tile.AltTypes[altImage - 1];
			}
		}
		if (tileSubIndex < tile.TileBlockCount && tile.TileBlockDatas[tileSubIndex].ImageData != NULL)
		{
			auto &subTile = tile.TileBlockDatas[tileSubIndex];
			return cell->Height - subTile.YMinusExY / 15;
		}
		return cell->Height;
	};

	bool shadow = CIsoViewExt::DrawShadows && ExtConfigs::InGameDisplay_Shadow;

	if (!ExtConfigs::DirectXRendering)
	{
		int shadowMask_width = window.right - window.left;
		int shadowMask_height = window.bottom - window.top;
		int shadowMask_size = shadowMask_width * shadowMask_height;

		shadowMask_Building_Infantry.assign(shadowMask_size, 0);
		shadowMask_Terrain.assign(shadowMask_size, 0);
		shadowMask_Overlay.assign(shadowMask_size, 0);
		shadowMask.assign(shadowMask_size, 0);

		if (ExtConfigs::PreciseDepthCalculation)
		{
			shadowHeightMask.assign(shadowMask_size, 0);
			cellHeightMask.assign(shadowMask_size, 0);
			objectOverlapMask.assign(shadowMask_size, CHAR_MIN);
		}
	}

	// loop1: tiles
	for (auto &info : visibleCells)
	{
		if (!info.isInMap)
			continue;
		auto &X = info.X;
		auto &Y = info.Y;
		auto &cell = info.cell;
		auto &cellExt = info.cellExt;
		auto &screenX = info.screenX;
		auto &screenY = info.screenY;

		CIsoViewExt::CurrentDrawCellLocation.X = X;
		CIsoViewExt::CurrentDrawCellLocation.Y = Y;
		CIsoViewExt::CurrentDrawCellLocation.Height = cell->Height;

		int altImage = cell->Flag.AltIndex;
		int tileIndex = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
		int tileSetOri = CMapDataExt::TileData[tileIndex].TileSet;
		int tileSubIndex = cell->TileSubIndex;

		int virtualHeight = cell->Height;
		if (CFinalSunApp::Instance->FrameMode)
		{
			if (CMapDataExt::TileData[tileIndex].FrameModeIndex < CMapDataExt::TileDataCount)
			{
				tileIndex = CMapDataExt::TileData[tileIndex].FrameModeIndex;
			}
			else
			{
				tileIndex = CMapDataExt::TileSet_starts[CMapDataExt::HeightBase] + cell->Height;
				tileSubIndex = 0;
			}
		}
		tileIndex = CMapDataExt::GetSafeTileIndex(tileIndex);

		CTileTypeClass tile = CMapDataExt::TileData[tileIndex];
		int tileSet = tile.TileSet;
		if (tile.AltTypeCount)
		{
			if (altImage > 0)
			{
				altImage = altImage < tile.AltTypeCount ? altImage : tile.AltTypeCount;
				tile = tile.AltTypes[altImage - 1];
			}
		}

		if (tileSubIndex < tile.TileBlockCount && tile.TileBlockDatas[tileSubIndex].ImageData != NULL)
		{
			auto &subTile = tile.TileBlockDatas[tileSubIndex];
			virtualHeight = cell->Height - subTile.YMinusExY / 15;
		}

		// bridge hack
		if (tileSetOri == CMapDataExt::BridgeSet || tileSetOri == CMapDataExt::WoodBridgeSet)
		{
			int relativeIdx = cell->TileIndex - CMapDataExt::TileSet_starts[tileSetOri];
			if ((relativeIdx == 0 || relativeIdx == 1) && tileSubIndex != 1)
			{
				virtualHeight = cell->Height;
			}
			else if ((relativeIdx == 3 || relativeIdx == 4) && tileSubIndex != 5)
			{
				virtualHeight = cell->Height;
			}
			else if (11 > relativeIdx && relativeIdx >= 6)
			{
				virtualHeight = cell->Height;
			}
			else if (16 > relativeIdx && relativeIdx >= 11)
			{
				virtualHeight = cell->Height;
			}
		}
		auto &heightSet = CMapDataExt::NoHeightRedrawTileSets;
		if (heightSet.find(tileSetOri) != heightSet.end())
		{
			virtualHeight = cell->Height;
		}

		for (int i = 1; i <= 2 + virtualHeight - cell->Height + cell->Height / 2; i++)
		{
			if (CMapData::Instance->IsCoordInMap(X - i, Y - i))
			{
				auto blockedCell = CMapData::Instance->GetCellAt(X - i, Y - i);
				int blockedHeight = blockedCell->Height; //(getTileVirtualHeight(blockedCell));
				if (virtualHeight - blockedHeight >= 2 * i || i == 1 && blockedCell->Flag.RedrawTerrain && virtualHeight > blockedHeight)
					cell->Flag.RedrawTerrain = true;
			}
		}
		for (int i = 0; i <= 2 + virtualHeight - cell->Height + cell->Height / 2; i++)
		{
			if (CMapData::Instance->IsCoordInMap(X - i - 1, Y - i))
			{
				auto blockedCell = CMapData::Instance->GetCellAt(X - i - 1, Y - i);
				if (blockedCell->Flag.RedrawTerrain && virtualHeight - blockedCell->Height >= 2 * std::max(1, i))
					cell->Flag.RedrawTerrain = true;
			}
			if (CMapData::Instance->IsCoordInMap(X - i, Y - i - 1))
			{
				auto blockedCell = CMapData::Instance->GetCellAt(X - i, Y - i - 1);
				if (blockedCell->Flag.RedrawTerrain && virtualHeight - blockedCell->Height >= 2 * std::max(1, i))
					cell->Flag.RedrawTerrain = true;
			}
		}

		if (!cell->Flag.RedrawTerrain || CFinalSunApp::Instance->FlatToGround)
		{
			if (tileSubIndex < tile.TileBlockCount && tile.TileBlockDatas[tileSubIndex].ImageData != NULL)
			{
				auto &subTile = tile.TileBlockDatas[tileSubIndex];
				int x = screenX;
				int y = screenY;
				x -= 60;
				y -= 30;

				if (subTile.HasValidImage)
				{
					if (ExtConfigs::DirectXRendering)
					{
						CIsoViewExt::DirectXTerrain(x, y,
													&subTile, isCellHidden(cell) ? 0.5f : 1.0f, cell->Height);
					}
					else
					{
						Palette *pal = CMapDataExt::TileSetPalettes[tileSet];
						CIsoViewExt::BlitTerrain(pThis, ddsd.lpSurface, window, boundary,
													x + subTile.XMinusExX, y + subTile.YMinusExY, &subTile, pal,
													isCellHidden(cell) ? 128 : 255, nullptr, nullptr, cell->Height, &cellHeightMask, tileSetOri);
					}

					auto &cellExt = CMapDataExt::CellDataExts[CMapData::Instance->GetCoordIndex(X, Y)];
					cellExt.HasAnim = false;
					if (CMapDataExt::TileAnimations.find(tileIndex) != CMapDataExt::TileAnimations.end())
					{
						auto &tileAnim = CMapDataExt::TileAnimations[tileIndex];
						if (tileAnim.AttachedSubTile == tileSubIndex)
						{
							cellExt.HasAnim = true;
						}
					}

					if (CMapDataExt::RedrawExtraTileSets.find(tileSet) != CMapDataExt::RedrawExtraTileSets.end())
						RedrawCoords.push_back(MapCoord{X, Y});
				}
			}
		}
		if (cell->Flag.RedrawTerrain && !CFinalSunApp::Instance->FlatToGround)
		{
			for (int i = 1; i <= 2; i++)
			{
				if (CMapData::Instance->IsCoordInMap(X + i, Y + i))
				{
					auto nextCell = CMapData::Instance->GetCellAt(X + i, Y + i);
					int altImage = nextCell->Flag.AltIndex;
					int tileIndex = CMapDataExt::GetSafeTileIndex(nextCell->TileIndex);
					int tileSubIndex = nextCell->TileSubIndex;
					CTileBlockClass *subTile = nullptr;
					if (!nextCell->Flag.RedrawTerrain)
					{
						if (CFinalSunApp::Instance->FrameMode)
						{
							if (CMapDataExt::TileData[tileIndex].FrameModeIndex < CMapDataExt::TileDataCount)
							{
								tileIndex = CMapDataExt::TileData[tileIndex].FrameModeIndex;
							}
							else
							{
								tileIndex = CMapDataExt::TileSet_starts[CMapDataExt::HeightBase] + nextCell->Height;
								tileSubIndex = 0;
							}
						}
						tileIndex = CMapDataExt::GetSafeTileIndex(tileIndex);

						CTileTypeClass *tile = &CMapDataExt::TileData[tileIndex];
						int tileSet = tile->TileSet;
						if (tile->AltTypeCount)
						{
							if (altImage > 0)
							{
								altImage = altImage < tile->AltTypeCount ? altImage : tile->AltTypeCount;
								tile = &tile->AltTypes[altImage - 1];
							}
						}
						if (tileSubIndex < tile->TileBlockCount)
							subTile = &tile->TileBlockDatas[tileSubIndex];
						else
							continue;
					}
					else
						continue;
					if (subTile &&
						-subTile->YMinusExY - 30 * i - (cell->Height - nextCell->Height) * 15 >= 0) // tile blocks with extra image above themselves
					{
						nextCell->Flag.RedrawTerrain = true;

						for (int j = 1; j <= 2; j++)
						{
							if (CMapData::Instance->IsCoordInMap(X + i + j, Y + i + j))
							{
								auto nextNextCell = CMapData::Instance->GetCellAt(X + i + j, Y + i + j);
								if (nextNextCell->Height - nextCell->Height >= 2 * j || j == 1 && nextNextCell->Height > nextCell->Height)
									nextNextCell->Flag.RedrawTerrain = true;
							}
							if (CMapData::Instance->IsCoordInMap(X + i + j + 1, Y + i + j))
							{
								auto nextNextCell = CMapData::Instance->GetCellAt(X + i + j + 1, Y + i + j);
								if (nextNextCell->Height - nextCell->Height >= 2 * j || j == 1 && nextNextCell->Height > nextCell->Height)
									nextNextCell->Flag.RedrawTerrain = true;
							}
							if (CMapData::Instance->IsCoordInMap(X + i + j, Y + i + j + 1))
							{
								auto nextNextCell = CMapData::Instance->GetCellAt(X + i + j, Y + i + j + 1);
								if (nextNextCell->Height - nextCell->Height >= 2 * j || j == 1 && nextNextCell->Height > nextCell->Height)
									nextNextCell->Flag.RedrawTerrain = true;
							}
						}
						continue;
					}
				}
			}
		}

		if (cell->Flag.RedrawTerrain)
		{
			for (int dx = -3; dx <= 3; ++dx)
			{
				for (int dy = -3; dy <= 3; ++dy)
				{
					if (dx + dy > 0) 
						continue;
					if (!isCoordInFullMap(X + dx, Y + dy))
						continue;
				
					int nx = X + dx;
					int ny = Y + dy;
					int neighborIndex = coordToIndex[nx][ny];
					auto &neighbor = visibleCells[neighborIndex];
					neighbor.aroundRedrawCell = true;
				}
			}
		}
	}
	for (const auto &coord : RedrawCoords)
	{
		const int &X = coord.X;
		const int &Y = coord.Y;
		const auto cell = CMapData::Instance->GetCellAt(X, Y);

		CIsoViewExt::CurrentDrawCellLocation.X = X;
		CIsoViewExt::CurrentDrawCellLocation.Y = Y;
		CIsoViewExt::CurrentDrawCellLocation.Height = cell->Height;

		int altImage = cell->Flag.AltIndex;
		int tileIndex = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
		int tileSubIndex = cell->TileSubIndex;

		if (CFinalSunApp::Instance->FrameMode)
		{
			if (CMapDataExt::TileData[tileIndex].FrameModeIndex < CMapDataExt::TileDataCount)
			{
				tileIndex = CMapDataExt::TileData[tileIndex].FrameModeIndex;
			}
			else
			{
				tileIndex = CMapDataExt::TileSet_starts[CMapDataExt::HeightBase] + cell->Height;
				tileSubIndex = 0;
			}
		}
		tileIndex = CMapDataExt::GetSafeTileIndex(tileIndex);

		CTileTypeClass tile = CMapDataExt::TileData[tileIndex];
		int tileSet = tile.TileSet;
		if (tile.AltTypeCount)
		{
			if (altImage > 0)
			{
				altImage = altImage < tile.AltTypeCount ? altImage : tile.AltTypeCount;
				tile = tile.AltTypes[altImage - 1];
			}
		}
		if (tileSubIndex < tile.TileBlockCount && tile.TileBlockDatas[tileSubIndex].ImageData != NULL)
		{
			auto &subTile = tile.TileBlockDatas[tileSubIndex];
			int x = X;
			int y = Y;
			CIsoView::MapCoord2ScreenCoord(x, y);
			x -= DrawOffsetX;
			y -= DrawOffsetY;
			x -= 60;
			y -= 30;

			if (subTile.HasValidImage)
			{
				auto &dataExt = CMapDataExt::TileBlockDataExt[&subTile];
				auto pData = dataExt.pExtraImage;
				if (ImageDataClassSafe::IsVisibleImage(pData))
				{
					if (ExtConfigs::DirectXRendering)
					{
						CIsoViewExt::DirectXTerrain(x, y,
							&subTile, isCellHidden(cell) ? 0.5f : 1.0f, cell->Height, true);
					}
					else
					{
						CIsoViewExt::BlitSHPTransparent(pThis, ddsd.lpSurface, window, boundary,
														x + 30 + dataExt.ExtraOffset.x,
														y + 30 + dataExt.ExtraOffset.y,
														pData, nullptr, isCellHidden(cell) ? 128 : 255, -2, -10);
					}
				}
			}
		}
	}

	// loop2: smudges
	for (const auto &info : visibleCells)
	{
		auto &X = info.X;
		auto &Y = info.Y;
		auto &pos = info.pos;
		auto &cell = info.cell;
		auto &cellExt = info.cellExt;
		auto &x = info.screenX;
		auto &y = info.screenY;

		CIsoViewExt::CurrentDrawCellLocation.X = X;
		CIsoViewExt::CurrentDrawCellLocation.Y = Y;
		CIsoViewExt::CurrentDrawCellLocation.Height = cell->Height;

		// smudges
		if (cell->Smudge != -1 && cell->Smudge < CMapData::Instance->SmudgeDatas.size() && CIsoViewExt::DrawSmudges && (info.isInMap || ExtConfigs::DisplayObjectsOutside))
		{
			auto &obj = CMapData::Instance->SmudgeDatas[cell->Smudge].TypeID;
			auto pType = Renderer::GetOrCreateSmudge(obj);
			if (pType->IsVisibleInMapRendererOrNormal())
			{
				auto pData = pType->GetImageData();
				if (ImageDataClassSafe::IsVisibleImage(pData))
				{
					if (ExtConfigs::DirectXRendering)
					{
						CIsoViewExt::DirectXNormal(
							x - pData->FullWidth / 2,
							y - pData->FullHeight / 2,
							pData, nullptr, 1.0f, 0, -1, false, 0xff, false);
					}
					else
					{
						CIsoViewExt::BlitSHPTransparent(pThis, ddsd.lpSurface, window, boundary,
														x - pData->FullWidth / 2, y - pData->FullHeight / 2, pData, NULL, 255, 0, -1, false);
					}
				}
				else
				{
					SmudgeTextsToDraw.push_back(std::make_pair(MapCoord{X, Y}, obj));
				}
			}
		}
	}

	// loop3: shadows
	for (const auto &info : visibleCells)
	{
		if (!info.isInMap && !ExtConfigs::DisplayObjectsOutside)
			continue;
		auto &X = info.X;
		auto &Y = info.Y;
		auto &pos = info.pos;
		auto &cell = info.cell;
		auto &cellExt = info.cellExt;
		auto &x = info.screenX;
		auto &y = info.screenY;

		CIsoViewExt::CurrentDrawCellLocation.X = X;
		CIsoViewExt::CurrentDrawCellLocation.Y = Y;
		CIsoViewExt::CurrentDrawCellLocation.Height = cell->Height;

		if (cell->Waypoint != -1 && CIsoViewExt::DrawWaypoints)
		{
			WaypointsToDraw.push_back(std::make_pair(MapCoord{x, y},
													 cell->Waypoint < Waypoints.size() ? Waypoints[cell->Waypoint]->GetString() : ""));
		}
		if (cell->Structure > -1)
		{
			if (!DrawnBuildings[cell->Structure])
			{
				DrawnBuildings[cell->Structure] = true;
				auto pType = Renderer::Buildings[cell->Structure].GetType();
				auto &objRender = *Renderer::Buildings[cell->Structure].GetRender();
				const auto &DataExt = *pType->pDataExt;

				const int HP = objRender.Strength;
				int status = CLoadingExt::GBIN_NORMAL;
				if (pType->CanOccupyFire && pType->TechLevel < 0)
				{
					if (HP == 0)
						status = CLoadingExt::GBIN_RUBBLE;
					else if (static_cast<int>((CMapDataExt::ConditionRed + 0.001f) * 256) > HP)
						status = CLoadingExt::GBIN_DAMAGED;
					else if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP)
						status = CLoadingExt::GBIN_GARRISONDAMAGED;
				}
				else
				{
					if (HP == 0)
						status = CLoadingExt::GBIN_RUBBLE;
					else if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP)
						status = CLoadingExt::GBIN_DAMAGED;
				}

				int x1 = objRender.X;
				int y1 = objRender.Y;
				CIsoView::MapCoord2ScreenCoord(x1, y1);
				x1 -= DrawOffsetX;
				y1 -= DrawOffsetY;

				MapCoord buildingOrigin{objRender.X, objRender.Y};
				if (!IsCoordInWindow(objRender.X, objRender.Y))
				{
					buildingOrigin.X = X;
					buildingOrigin.Y = Y;
				}

				auto &clips = *pType->GetImageData(objRender.Facing, status);

				Palette *pPal = nullptr;
				BGRStruct color;
				auto pRGB = reinterpret_cast<ColorStruct *>(&objRender.HouseColor);
				color.R = pRGB->red;
				color.G = pRGB->green;
				color.B = pRGB->blue;

				auto isRubble = status == CLoadingExt::GBIN_RUBBLE &&
								!pType->IsDamagedAsRubble && (pType->LeaveRubble || ExtConfigs::HideNoRubbleBuilding);
				auto isTerrain = pType->IsTerrainPalette;

				if (!ExtConfigs::DirectXRendering)
				{
					if (LightingStruct::CurrentLighting == LightingStruct::NoLighting)
					{
						pPal = PalettesManager::GetPalette(clips[0]->pPalette, color, !isTerrain && !isRubble);
					}
					else
					{
						pPal = PalettesManager::GetObjectPalette(clips[0]->pPalette, color, !isTerrain && !isRubble,
																 {objRender.X, objRender.Y, CMapDataExt::TryGetCellAt(objRender.X, objRender.Y)->Height},
																 false, isRubble || isTerrain ? 4 : 3);
					}
				}
				int partCount = std::min(DataExt.BottomCoords.size(), clips.size());
				for (int i = 0; i < partCount; ++i)
				{
					auto pData = clips[i].get();

					auto &coord = DataExt.BottomCoords[i];
					MapCoord coordInMap = {objRender.X + coord.X, objRender.Y + coord.Y};

					while (IsCoordInWindowButOnBottom(coordInMap.X, coordInMap.Y))
					{
						coordInMap.X--;
						coordInMap.Y--;
						if (IsCoordInWindow(coordInMap.X, coordInMap.Y))
						{
							break;
						}
					}
					if (!IsCoordInWindow(coordInMap.X, coordInMap.Y))
					{
						coordInMap.X = X;
						coordInMap.Y = Y;
					}
					CellDataExt *cellExt = nullptr;
					bool objectsOnBuilding = false;
					if (!DataExt.IsCustomFoundation())
					{
						for (int dy = 0; dy < DataExt.Width; ++dy)
						{
							for (int dx = 0; dx < DataExt.Height; ++dx)
							{
								const int x = objRender.X + dx;
								const int y = objRender.Y + dy;
								auto cell = CMapData::Instance->TryGetCellAt(x, y);
								if (cell->Aircraft > -1 || cell->Unit > -1 || cell->Infantry[0] > -1 || cell->Infantry[1] > -1 || cell->Infantry[2] > -1)
								{
									objectsOnBuilding = true;
									break;
								}
							}
						}
					}
					else
					{
						for (const auto &block : *DataExt.Foundations)
						{
							const int x = objRender.X + block.Y;
							const int y = objRender.Y + block.X;
							auto cell = CMapData::Instance->TryGetCellAt(x, y);
							if (cell->Aircraft > -1 || cell->Unit > -1 || cell->Infantry[0] > -1 || cell->Infantry[1] > -1 || cell->Infantry[2] > -1)
							{
								objectsOnBuilding = true;
								break;
							}
						}
					}

					if (objectsOnBuilding)
					{
						cellExt = &CMapDataExt::CellDataExts
									  [CMapData::Instance->GetCoordIndex(buildingOrigin.X, buildingOrigin.Y)];
					}
					else if (isCoordInFullMap(coordInMap.X, coordInMap.Y))
					{
						cellExt = &CMapDataExt::CellDataExts
									  [CMapData::Instance->GetCoordIndex(coordInMap.X, coordInMap.Y)];
					}
					else
					{
						bool found = false;
						for (int dy = 0; dy < DataExt.Width; ++dy)
						{
							for (int dx = 0; dx < DataExt.Height; ++dx)
							{
								const int x = objRender.X + dx;
								const int y = objRender.Y + dy;
								if (isCoordInFullMap(x, y))
								{
									found = true;
									cellExt = &CMapDataExt::CellDataExts
												  [CMapData::Instance->GetCoordIndex(x, y)];
								}
								if (found)
									break;
							}
							if (found)
								break;
						}
					}

					if (cellExt)
					{
						bool hasFire = status == CLoadingExt::GBIN_DAMAGED;
						if (pType->CanOccupyFire && pType->TechLevel > -1 && static_cast<int>((CMapDataExt::ConditionRed + 0.001f) * 256) <= HP)
							hasFire = false;

						cellExt->BuildingRenderParts.push_back({(short)i,
																i == DataExt.Width - 1,
																hasFire,
																x1 - pData->ClipOffsets.FullWidth / 2 + pData->ClipOffsets.LeftOffset,
																y1,
																status,
																pData,
																pPal,
																&Renderer::Buildings[cell->Structure]});
					}
				}

				if (shadow && CIsoViewExt::DrawStructures && !isCloakable(pType))
				{
					int nFacing = 0;
					int FacingCount = pType->FacingCount;
					if (pType->HasTurret && !pType->TurretAnimIsVoxel)
					{
						nFacing = (FacingCount + 7 * FacingCount / 8 - (objRender.Facing * FacingCount / 256) % FacingCount) % FacingCount;
					}
					auto pData = pType->GetShadowData(nFacing, status);
					if (ImageDataClassSafe::IsValidImage(pData))
					{
						if (ExtConfigs::DirectXRendering)
						{
							CIsoViewExt::DirectXShadow(x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, cell->Height);
						}
						else
						{
							CIsoViewExt::MaskShadowPixels(window,
														  x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData,
														  shadowMask_Building_Infantry,
														  shadowHeightMask, cell->Height);
						}
					}

					FSet drawn;
					DrawTechnoAttachments([] {}, drawn, pType, FacingCount == 1 ? 0 : objRender.Facing,
										  cell, ddsd.lpSurface, boundary,
										  x1, y1, 0xffffff, true);

					for (int upgrade = 0; upgrade < objRender.PowerUpCount; ++upgrade)
					{
						const auto &upg = upgrade == 0 ? objRender.PowerUp1 : (upgrade == 1 ? objRender.PowerUp2 : objRender.PowerUp3);
						const auto &upgXX = upgrade == 0 ? pType->PowerUp1LocXX : (upgrade == 1 ? pType->PowerUp2LocXX : pType->PowerUp3LocXX);
						const auto &upgYY = upgrade == 0 ? pType->PowerUp1LocYY : (upgrade == 1 ? pType->PowerUp2LocYY : pType->PowerUp3LocYY);
						if (upg.GetLength() == 0)
							continue;

						auto pUpgType = Renderer::GetOrCreateBuilding(upg);
						auto pUpgData = pUpgType->GetShadowData(0, 0);
						if (ImageDataClassSafe::IsValidImage(pUpgData))
						{
							int x1 = x;
							int y1 = y;
							x1 += upgXX;
							y1 += upgYY;

							if (ExtConfigs::DirectXRendering)
							{
								CIsoViewExt::DirectXShadow(
									x1 - pUpgData->FullWidth / 2,
									y1 - pUpgData->FullHeight / 2,
									pUpgData, cell->Height);
							}
							else
							{
								CIsoViewExt::MaskShadowPixels(window,
															  x1 - pUpgData->FullWidth / 2, y1 - pUpgData->FullHeight / 2,
															  pUpgData, shadowMask_Building_Infantry,
															  shadowHeightMask, cell->Height);
							}
						}
					}
				}
			}
		}
		if (info.isInMap || ExtConfigs::DisplayObjectsOutside)
		{
			if (!cellExt->BaseNodes.empty())
			{
				for (auto &node : cellExt->BaseNodes)
				{
					if (std::find(DrawnBaseNodes.begin(), DrawnBaseNodes.end(), node) == DrawnBaseNodes.end())
					{
						DrawnBaseNodes.push_back(node);
						if (CIsoViewExt::RenderingMap && CIsoViewExt::MapRendererIgnoreObjects.find(node.ID) != CIsoViewExt::MapRendererIgnoreObjects.end())
							continue;

						if (CIsoViewExt::DrawBasenodesFilter && CViewObjectsExt::BuildingBrushDlgBNF)
						{
							const auto &filter = CViewObjectsExt::ObjectFilterBN;
							auto CheckValue = [&](int nCheckBoxIdx, ppmfc::CString &src, const ppmfc::CString &dst)
							{
								if (CViewObjectsExt::BuildingBrushBoolsBNF[nCheckBoxIdx - 1300])
								{
									if (dst == src)
										return true;
									else
										return false;
								}
								return true;
							};
							if (filter.empty() || std::find(filter.begin(), filter.end(), node.ID) != filter.end())
							{
								if (!CheckValue(1300, CViewObjectsExt::BuildingBrushDlgBNF->CString_House, node.House))
									continue;
							}
							else
							{
								continue;
							}
						}

						auto pType = Renderer::GetOrCreateBuilding(node.ID);
						const auto &DataExt = *pType->pDataExt;

						int x1 = node.X;
						int y1 = node.Y;
						CIsoView::MapCoord2ScreenCoord(x1, y1);
						x1 -= DrawOffsetX;
						y1 -= DrawOffsetY;

						FString text;
						text.Format("%03d", node.BasenodeID);
						BaseNodeTextsToDraw.push_back(std::make_pair( MapCoord{x1 + 30, y1 - 15}, text));

						MapCoord buildingOrigin{node.X, node.Y};
						if (!IsCoordInWindow(node.X, node.Y))
						{
							buildingOrigin.X = X;
							buildingOrigin.Y = Y;
						}

						auto &clips = *pType->GetImageData(224, 0);
						Palette *pPal = nullptr;
						BGRStruct color;
						auto houseColor = Miscs::GetColorRef(node.House);
						auto pRGB = reinterpret_cast<ColorStruct *>(&houseColor);
						color.R = pRGB->red;
						color.G = pRGB->green;
						color.B = pRGB->blue;

						auto isTerrain = pType->IsTerrainPalette;

						if (ExtConfigs::DirectXRendering)
						{
							pPal = nullptr;
						}
						else
						{
							if (LightingStruct::CurrentLighting == LightingStruct::NoLighting)
							{
								pPal = PalettesManager::GetPalette(clips[0]->pPalette, color, !isTerrain);
							}
							else
							{
								pPal = PalettesManager::GetObjectPalette(clips[0]->pPalette, color, !isTerrain,
																		 {(short)node.X, (short)node.Y, CMapDataExt::TryGetCellAt(node.X, node.Y)->Height},
																		 false, isTerrain ? 4 : 3);
							}
						}

						for (int i = 0; i < std::min(DataExt.BottomCoords.size(), clips.size()); ++i)
						{
							auto pData = clips[i].get();
							auto &coord = DataExt.BottomCoords[i];
							MapCoord coordInMap = {node.X + coord.X, node.Y + coord.Y};

							while (IsCoordInWindowButOnBottom(coordInMap.X, coordInMap.Y))
							{
								coordInMap.X--;
								coordInMap.Y--;
								if (IsCoordInWindow(coordInMap.X, coordInMap.Y))
								{
									break;
								}
							}
							if (!IsCoordInWindow(coordInMap.X, coordInMap.Y))
							{
								coordInMap.X = X;
								coordInMap.Y = Y;
							}
							CellDataExt *cellExt = nullptr;
							bool objectsOnBuilding = false;
							if (!DataExt.IsCustomFoundation())
							{
								for (int dy = 0; dy < DataExt.Width; ++dy)
								{
									for (int dx = 0; dx < DataExt.Height; ++dx)
									{
										const int x = node.X + dx;
										const int y = node.Y + dy;
										auto cell = CMapData::Instance->TryGetCellAt(x, y);
										if (cell->Aircraft > -1 || cell->Unit > -1 || cell->Infantry[0] > -1 || cell->Infantry[1] > -1 || cell->Infantry[2] > -1)
										{
											objectsOnBuilding = true;
											break;
										}
									}
								}
							}
							else
							{
								for (const auto &block : *DataExt.Foundations)
								{
									const int x = node.X + block.Y;
									const int y = node.Y + block.X;
									auto cell = CMapData::Instance->TryGetCellAt(x, y);
									if (cell->Aircraft > -1 || cell->Unit > -1 || cell->Infantry[0] > -1 || cell->Infantry[1] > -1 || cell->Infantry[2] > -1)
									{
										objectsOnBuilding = true;
										break;
									}
								}
							}
							if (objectsOnBuilding)
							{
								cellExt = &CMapDataExt::CellDataExts
											  [CMapData::Instance->GetCoordIndex(buildingOrigin.X, buildingOrigin.Y)];
							}
							else if (isCoordInFullMap(coordInMap.X, coordInMap.Y))
							{
								cellExt = &CMapDataExt::CellDataExts
											  [CMapData::Instance->GetCoordIndex(coordInMap.X, coordInMap.Y)];
							}
							else
							{
								bool found = false;
								for (int dy = 0; dy < DataExt.Width; ++dy)
								{
									for (int dx = 0; dx < DataExt.Height; ++dx)
									{
										const int x = node.X + dx;
										const int y = node.Y + dy;
										if (isCoordInFullMap(x, y))
										{
											found = true;
											cellExt = &CMapDataExt::CellDataExts
														  [CMapData::Instance->GetCoordIndex(x, y)];
										}
										if (found)
											break;
									}
									if (found)
										break;
								}
							}
							if (cellExt)
								cellExt->BaseNodeRenderParts.push_back({(short)i,
																		x1 - pData->ClipOffsets.FullWidth / 2 + pData->ClipOffsets.LeftOffset,
																		y1,
																		pType->BuildingIndex,
																		pData,
																		pPal,
																		&node,
																		pType,
																		houseColor});
						}
					}
				}
			}
		}
		for (int i = 0; i < 3 && shadow; i++)
		{
			if (cell->Infantry[i] > -1 && Renderer::Infantries[cell->Infantry[i]].IsVisible())
			{
				auto &obj = *Renderer::Infantries[cell->Infantry[i]].GetData();
				auto pType = Renderer::Infantries[cell->Infantry[i]].GetType();
				if (!isCloakable(pType))
				{
					auto pData = pType->GetShadowData(obj, CMapDataExt::GetLandType(cell->TileIndex, cell->TileSubIndex));

					int x1 = x;
					int y1 = y;
					Renderer::Infantries[cell->Infantry[i]].OffsetInfantrySubcell(x1, y1);

					if (ExtConfigs::InGameDisplay_Bridge && obj.IsAboveGround == "1")
						y1 -= 60;

					if (ImageDataClassSafe::IsValidImage(pData))
					{
						if (ExtConfigs::DirectXRendering)
						{
							CIsoViewExt::DirectXShadow(x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, cell->Height);
						}
						else
						{
							CIsoViewExt::MaskShadowPixels(window,
														  x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData,
														  shadowMask_Building_Infantry,
														  shadowHeightMask, cell->Height);
						}
					}
					FSet drawn;
					DrawTechnoAttachments([] {}, drawn, pType, atoi(obj.Facing),
										  cell, ddsd.lpSurface, boundary,
										  x1, y1, 0xffffff, true);
				}
			}
		}
		if (shadow && cell->Unit > -1 && Renderer::Vehicles[cell->Unit].IsVisible())
		{
			auto &obj = *Renderer::Vehicles[cell->Unit].GetData();
			auto pType = Renderer::Vehicles[cell->Unit].GetType();
			if (!isCloakable(pType))
			{
				auto pData = pType->GetShadowData(obj, CMapDataExt::GetLandType(cell->TileIndex, cell->TileSubIndex));

				int x1 = x;
				int y1 = y;

				if (ExtConfigs::InGameDisplay_Bridge && obj.IsAboveGround == "1")
					y1 -= 60;

				if (ImageDataClassSafe::IsValidImage(pData))
				{
					// units are special, they overlap with each other
					if (ExtConfigs::DirectXRendering)
					{
						CIsoViewExt::DirectXShadow(
							x1 - pData->FullWidth / 2,
							y1 - pData->FullHeight / 2 + 15,
							pData, cell->Height, true);
					}
					else
					{
						CIsoViewExt::BlitSHPTransparent(pThis, ddsd.lpSurface, window, boundary,
														x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2 + 15, pData, NULL, 128);
					}
				}

				FSet drawn;
				DrawTechnoAttachments([] {}, drawn, pType, atoi(obj.Facing),
									  cell, ddsd.lpSurface, boundary,
									  x1, y1 + 15, 0xffffff, true);
			}
		}
		if (shadow && cell->Aircraft > -1 && Renderer::Aircrafts[cell->Aircraft].IsVisible())
		{
			auto &obj = *Renderer::Aircrafts[cell->Aircraft].GetData();
			auto pType = Renderer::Aircrafts[cell->Aircraft].GetType();
			if (!isCloakable(pType) && pType->TechnoAttachmentInfo)
			{
				FSet drawn;
				DrawTechnoAttachments([] {}, drawn, pType, atoi(obj.Facing),
									  cell, ddsd.lpSurface, boundary,
									  x, y + 15, 0xffffff, true);
			}
		}
		if (shadow && cell->Terrain != -1 && cell->Terrain < CMapData::Instance->TerrainDatas.size() && CIsoViewExt::DrawTerrains)
		{
			auto &obj = CMapData::Instance->TerrainDatas[cell->Terrain].TypeID;
			auto pType = Renderer::GetOrCreateTerrain(obj);
			if (pType->IsVisibleInMapRendererOrNormal())
			{
				auto pData = pType->GetShadowData();
				if (ImageDataClassSafe::IsVisibleImage(pData))
				{
					int x1 = x;
					int y1 = y;

					if (ExtConfigs::DirectXRendering)
					{
						CIsoViewExt::DirectXShadow(
							x1 - pData->FullWidth / 2,
							y1 - pData->FullHeight / 2 + (pType->IsTiberiumTree ? -1 : 15),
							pData, cell->Height);
					}
					else
					{
						CIsoViewExt::MaskShadowPixels(window,
													  x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2 + (pType->IsTiberiumTree ? -1 : 15),
													  pData, shadowMask_Terrain,
													  shadowHeightMask, cell->Height);
					}
				}
			}
		}
		if (shadow && cellExt->NewOverlay != 0xFFFF && CIsoViewExt::DrawOverlays)
		{
			auto pType = Renderer::GetOrCreateOverlay(cellExt->NewOverlay);
			if (pType->IsVisibleInMapRendererOrNormal())
			{
				auto pData = pType->GetShadowData(cell->OverlayData);
				if (ImageDataClassSafe::IsVisibleImage(pData))
				{
					int x1 = x;
					int y1 = y;

					y1 += CIsoViewExt::GetOverlayDrawOffset(cellExt->NewOverlay, cell->OverlayData);

					if (ExtConfigs::DirectXRendering)
					{
						CIsoViewExt::DirectXShadow(x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, cell->Height);
					}
					else
					{
						CIsoViewExt::MaskShadowPixels(window,
													  x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, shadowMask_Overlay,
													  shadowHeightMask, cell->Height);
					}
				}
			}
		}
	}
	if (!ExtConfigs::DirectXRendering && shadow)
	{
		for (size_t i = 0; i < shadowMask.size(); ++i)
		{
			shadowMask[i] += shadowMask_Building_Infantry[i];
			shadowMask[i] += shadowMask_Terrain[i];
			shadowMask[i] += shadowMask_Overlay[i];
		}
		CIsoViewExt::DrawShadowMask(ddsd.lpSurface, boundary, window, shadowMask, shadowHeightMask, cellHeightMask);
	}

	// loop4: objects
	memset(DrawnBuildings, 0, sizeof(DrawnBuildings));
	DrawnBaseNodes.clear();

	for (const auto &info : visibleCells)
	{
		auto &X = info.X;
		auto &Y = info.Y;
		auto &pos = info.pos;
		auto &cell = info.cell;
		auto &cellExt = info.cellExt;
		auto &x = info.screenX;
		auto &y = info.screenY;

		CIsoViewExt::CurrentDrawCellLocation.X = X;
		CIsoViewExt::CurrentDrawCellLocation.Y = Y;
		CIsoViewExt::CurrentDrawCellLocation.Height = cell->Height;

		// tiles
		if (info.isInMap)
		{
			int altImage = cell->Flag.AltIndex;
			int tileIndex = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
			int tileSubIndex = cell->TileSubIndex;
			if (tileIndex < CMapDataExt::TileDataCount)
			{
				auto drawTerrainAnim = [&pThis, &ddsd, &boundary, &cell, &isCellHidden](int tileIndex, int tileSubIndex, int x, int y)
				{
					if (CMapDataExt::TileAnimations.find(tileIndex) != CMapDataExt::TileAnimations.end())
					{
						auto &tileAnim = CMapDataExt::TileAnimations[tileIndex];
						if (tileAnim.AttachedSubTile == tileSubIndex)
						{
							auto pData = CLoadingExt::GetImageDataFromMap(tileAnim.ImageName);

							if (ImageDataClassSafe::IsVisibleImage(pData))
							{
								if (ExtConfigs::DirectXRendering)
								{
									CIsoViewExt::DirectXNormal(
										x - pData->FullWidth / 2 + tileAnim.XOffset,
										y - pData->FullHeight / 2 + tileAnim.YOffset + 15,
										pData, NULL, isCellHidden(cell) ? 0.5f : 1.0f, -2, -10);
								}
								else
								{
									CIsoViewExt::BlitSHPTransparent(pThis, ddsd.lpSurface, window, boundary,
																	x - pData->FullWidth / 2 + tileAnim.XOffset,
																	y - pData->FullHeight / 2 + tileAnim.YOffset + 15,
																	pData, NULL, isCellHidden(cell) ? 128 : 255, -2, -10);
								}
							}
						}
					}
				};

				if (cell->Flag.RedrawTerrain && !CFinalSunApp::Instance->FlatToGround)
				{
					int tileSetOri = CMapDataExt::TileData[tileIndex].TileSet;
					if (CFinalSunApp::Instance->FrameMode)
					{
						if (CMapDataExt::TileData[tileIndex].FrameModeIndex < CMapDataExt::TileDataCount)
						{
							tileIndex = CMapDataExt::TileData[tileIndex].FrameModeIndex;
						}
						else
						{
							tileIndex = CMapDataExt::TileSet_starts[CMapDataExt::HeightBase] + cell->Height;
							tileSubIndex = 0;
						}
					}
					tileIndex = CMapDataExt::GetSafeTileIndex(tileIndex);

					CTileTypeClass tile = CMapDataExt::TileData[tileIndex];
					int tileSet = tile.TileSet;
					if (tile.AltTypeCount)
					{
						if (altImage > 0)
						{
							altImage = altImage < tile.AltTypeCount ? altImage : tile.AltTypeCount;
							tile = tile.AltTypes[altImage - 1];
						}
					}
					if (tileSubIndex < tile.TileBlockCount && tile.TileBlockDatas[tileSubIndex].ImageData != NULL)
					{
						auto &subTile = tile.TileBlockDatas[tileSubIndex];
						int x1 = x;
						int y1 = y;
						x1 -= 60;
						y1 -= 30;


						if (subTile.HasValidImage)
						{
							Palette *pal = CMapDataExt::TileSetPalettes[tileSet];

							if (ExtConfigs::DirectXRendering)
							{
								CIsoViewExt::DirectXTerrain(x1, y1,
															&subTile, isCellHidden(cell) ? 0.5f : 1.0f,
															cell->Height);										
													
							}
							else
							{
								CIsoViewExt::BlitTerrain(pThis, ddsd.lpSurface, window, boundary,
														 x1 + subTile.XMinusExX, y1 + subTile.YMinusExY, &subTile, pal,
														 isCellHidden(cell) ? 128 : 255,
														 shadow ? &shadowMask : nullptr,
														 shadow ? &shadowHeightMask : nullptr,
														 cell->Height + (subTile.YMinusExY < 0 ? ((subTile.YMinusExY + 15) / -30) : 0),
														 nullptr, tileSetOri, &objectOverlapMask);
							}

							if (CMapDataExt::RedrawExtraTileSets.contains(tileSet))
							{
								auto &dataExt = CMapDataExt::TileBlockDataExt[&subTile];
								auto pData = dataExt.pExtraImage;
								if (ImageDataClassSafe::IsVisibleImage(pData))
								{
									if (ExtConfigs::DirectXRendering)
									{
										CIsoViewExt::DirectXTerrain(x1, y1,
											&subTile, isCellHidden(cell) ? 0.5f : 1.0f, cell->Height, true);
									}
									else
									{
										CIsoViewExt::BlitSHPTransparent(pThis, ddsd.lpSurface, window, boundary,
																		x1 + 30 + dataExt.ExtraOffset.x,
																		y1 + 30 + dataExt.ExtraOffset.y,
																		pData, pal, isCellHidden(cell) ? 128 : 255, -2, -10);
									}
								}
							}
							drawTerrainAnim(tileIndex, tileSubIndex, x1 + 60, y1 + 30);
						}
					}
				}
				else
				{
					if (cellExt->HasAnim)
					{
						if (CFinalSunApp::Instance->FrameMode)
						{
							if (CMapDataExt::TileData[tileIndex].FrameModeIndex < CMapDataExt::TileDataCount)
							{
								tileIndex = CMapDataExt::TileData[tileIndex].FrameModeIndex;
							}
							else
							{
								tileIndex = CMapDataExt::TileSet_starts[CMapDataExt::HeightBase] + cell->Height;
								tileSubIndex = 0;
							}
						}
						tileIndex = CMapDataExt::GetSafeTileIndex(tileIndex);
						drawTerrainAnim(tileIndex, tileSubIndex, x, y);
					}
				}
			}
		}

		// smudges in redrawn tiles
		if (cell->Smudge != -1 && cell->Smudge < CMapData::Instance->SmudgeDatas.size() && CIsoViewExt::DrawSmudges && cell->Flag.RedrawTerrain && !CFinalSunApp::Instance->FlatToGround && (info.isInMap || ExtConfigs::DisplayObjectsOutside))
		{
			auto &obj = CMapData::Instance->SmudgeDatas[cell->Smudge].TypeID;
			auto pType = Renderer::GetOrCreateSmudge(obj);
			if (pType->IsVisibleInMapRendererOrNormal())
			{
				auto pData = pType->GetImageData();
				if (ImageDataClassSafe::IsVisibleImage(pData))
				{
					if (ExtConfigs::DirectXRendering)
					{
						CIsoViewExt::DirectXNormal(
							x - pData->FullWidth / 2,
							y - pData->FullHeight / 2,
							pData, NULL, 1.0f, 0, -1, false, 
							info.aroundRedrawCell ? cell->Height : 0xFF);
					}
					else
					{
						CIsoViewExt::BlitSHPTransparent(pThis, ddsd.lpSurface, window, boundary,
														x - pData->FullWidth / 2, y - pData->FullHeight / 2, pData, NULL, 255, 0, -1, false,
														info.aroundRedrawCell ? &objectOverlapMask : nullptr);
					}
				}
			}
		}

		// overlays
		int nextPos = CMapData::Instance->GetCoordIndex(X + 1, Y + 1);
		if (nextPos >= CMapData::Instance->CellDataCount)
			nextPos = 0;
		auto cellNext = CMapData::Instance->GetCellAt(nextPos);
		auto &cellNextExt = CMapDataExt::CellDataExts[nextPos];
		if ((cellExt->NewOverlay != 0xFFFF || cellNextExt.NewOverlay != 0xFFFF) && CIsoViewExt::DrawOverlays && (info.isInMap || ExtConfigs::DisplayObjectsOutside))
		{
			auto pNextType = Renderer::GetOrCreateOverlay(cellNextExt.NewOverlay);
			auto pType = Renderer::GetOrCreateOverlay(cellExt->NewOverlay);
			if (pNextType->IsVisibleInMapRendererOrNormal())
			{
				if (pNextType->IsBridge())
				{
					auto pData = pNextType->GetImageData(cellNext->OverlayData);

					if (!ImageDataClassSafe::IsVisibleImage(pData))
					{
						if (ExtConfigs::DisplayBridgeOverlay ||
							!(cellNextExt.NewOverlay >= 0x4a && cellNextExt.NewOverlay <= 0x65) &&
								!(cellNextExt.NewOverlay >= 0xcd && cellNextExt.NewOverlay <= 0xec))
						{
							char cd[10];
							cd[0] = '0';
							cd[1] = 'x';
							_itoa(cellNextExt.NewOverlay, cd + 2, 16);
							OverlayTextsToDraw.push_back(std::make_pair(MapCoord{X + 1, Y + 1}, cd));
						}
					}
					else
					{
						int x1 = x;
						int y1 = y;

						y1 += CIsoViewExt::GetOverlayDrawOffset(cellNextExt.NewOverlay, cellNext->OverlayData);
						y1 += 30;
						if (!CFinalSunApp::Instance->FlatToGround)
							y1 -= (cellNext->Height - cell->Height) * 15;

						CIsoViewExt::CurrentDrawCellLocation.X++;
						CIsoViewExt::CurrentDrawCellLocation.Y++;
						CIsoViewExt::CurrentDrawCellLocation.Height = cellNext->Height;

						if (ExtConfigs::DirectXRendering)
						{
							CIsoViewExt::DirectXOverlay(
								x1 - pData->FullWidth / 2,
								y1 - pData->FullHeight / 2,
								pData, pNextType, cellNext, &cellNextExt, info.aroundRedrawCell);
						}
						else
						{
							CIsoViewExt::BlitSHPTransparent(pThis, ddsd.lpSurface, window, boundary,
															x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, NULL, 255, 0, 500 + cellNextExt.NewOverlay, false);
						}

						CIsoViewExt::CurrentDrawCellLocation.X--;
						CIsoViewExt::CurrentDrawCellLocation.Y--;
						CIsoViewExt::CurrentDrawCellLocation.Height = cell->Height;
					}
				}
			}
			if (pType->IsVisibleInMapRendererOrNormal())
			{
				if (!pType->IsBridge())
				{
					auto pData = pType->GetImageData(cell->OverlayData);

					if (!ImageDataClassSafe::IsVisibleImage(pData))
					{
						if (ExtConfigs::DisplayBridgeOverlay ||
							!(cellExt->NewOverlay >= 0x4a && cellExt->NewOverlay <= 0x65) &&
								!(cellExt->NewOverlay >= 0xcd && cellExt->NewOverlay <= 0xec))
						{
							char cd[10];
							cd[0] = '0';
							cd[1] = 'x';
							_itoa(cellExt->NewOverlay, cd + 2, 16);
							OverlayTextsToDraw.push_back(std::make_pair(MapCoord{X, Y}, cd));
						}
					}
					else
					{
						int x1 = x;
						int y1 = y;

						y1 += CIsoViewExt::GetOverlayDrawOffset(cellExt->NewOverlay, cell->OverlayData);

						if (ExtConfigs::DirectXRendering)
						{
							CIsoViewExt::DirectXOverlay(
								x1 - pData->FullWidth / 2,
								y1 - pData->FullHeight / 2,
								pData, pType, cell, cellExt, info.aroundRedrawCell);
						}
						else
						{
							CIsoViewExt::BlitSHPTransparent(pThis, ddsd.lpSurface, window, boundary,
															x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, NULL, 255, 0, 500 + cellExt->NewOverlay, false);
						}
					}
				}
			}
		}

		// terrains
		if (cell->Terrain != -1 && cell->Terrain < CMapData::Instance->TerrainDatas.size() && CIsoViewExt::DrawTerrains && (info.isInMap || ExtConfigs::DisplayObjectsOutside))
		{
			auto &obj = CMapData::Instance->TerrainDatas[cell->Terrain].TypeID;
			auto pType = Renderer::GetOrCreateTerrain(obj);
			if (pType->IsVisibleInMapRendererOrNormal())
			{
				auto pData = pType->GetImageData();

				if (ImageDataClassSafe::IsVisibleImage(pData))
				{
					if (ExtConfigs::DirectXRendering)
					{
						CIsoViewExt::DirectXNormal(
							x - pData->FullWidth / 2,
							y - pData->FullHeight / 2 + (pType->IsTiberiumTree ? -1 : 15),
							pData, NULL, 1.0f, 0, pType->IsTiberiumTree ? 6 : (pType->HasCustomPalette ? 5 : -1), false, info.aroundRedrawCell ? cell->Height : 0xFF);
					}
					else
					{
						CIsoViewExt::BlitSHPTransparent(pThis, ddsd.lpSurface, window, boundary,
														x - pData->FullWidth / 2, y - pData->FullHeight / 2 + (pType->IsTiberiumTree ? -1 : 15),
														pData, NULL, 255, 0, pType->IsTiberiumTree ? 6 : (pType->HasCustomPalette ? 5 : -1), false,
														info.aroundRedrawCell ? &objectOverlapMask : nullptr);
					}
				}
				else
				{
					TerrainTextsToDraw.push_back(std::make_pair(MapCoord{X, Y}, obj));
				}
				if (ExtConfigs::InGameDisplay_AlphaImage && CIsoViewExt::DrawAlphaImages)
				{
					auto pAIData = pType->GetAlphaImageData();
					if (ImageDataClassSafe::IsVisibleImage(pAIData))
					{
						AlphaImagesToDraw.push_back(
							std::make_pair(MapCoord{x - pAIData->FullWidth / 2,
													y - pAIData->FullHeight / 2 + (pType->IsTiberiumTree ? 0 : 12)},
										   pAIData));
					}
				}
			}
		}

		// buildings
		for (const auto &part : cellExt->BuildingRenderParts)
		{
			auto pBuilding = part.pBuilding;
			auto pType = pBuilding->GetType();
			const auto &objRender = *pBuilding->GetRender();
			const auto &DataExt = *pType->pDataExt;
			short index = pBuilding - Renderer::Buildings;
			bool firstDraw = !DrawnBuildings[index];
			DrawnBuildings[index] = true;
			CIsoViewExt::CurrentDrawCellLocation.X = objRender.X;
			CIsoViewExt::CurrentDrawCellLocation.Y = objRender.Y;
			CIsoViewExt::CurrentDrawCellLocation.Height = CMapDataExt::TryGetCellAt(objRender.X, objRender.Y)->Height;

			int x1 = objRender.X;
			int y1 = objRender.Y;
			CIsoView::MapCoord2ScreenCoord(x1, y1);
			x1 -= DrawOffsetX;
			y1 -= DrawOffsetY;
			if (firstDraw && CFinalSunApp::Instance->ShowBuildingCells)
			{
				if (ExtConfigs::DirectXRendering)
				{
					if (DataExt.IsCustomFoundation())
						pThis->DirectXDrawLockedLines(*DataExt.LinesToDraw, x1, y1, objRender.HouseColor, false);
					else
						pThis->DirectXDrawLockedCellOutline(x1, y1, DataExt.Width, DataExt.Height, objRender.HouseColor, false);
				}
				else
				{
					if (DataExt.IsCustomFoundation())
						pThis->DrawLockedLines(*DataExt.LinesToDraw, x1, y1, objRender.HouseColor, false, false, &ddsd);
					else
						pThis->DrawLockedCellOutline(x1, y1, DataExt.Width, DataExt.Height, objRender.HouseColor, false, false, &ddsd);
				}
			}

			if (CIsoViewExt::DrawStructures && pBuilding->IsVisible())
			{
				if (ImageDataClassSafe::IsValidImage(part.pData))
				{
					if (ExtConfigs::DirectXRendering)
					{
						auto isRubble = part.Status == CLoadingExt::GBIN_RUBBLE &&
										!pType->IsDamagedAsRubble && (pType->LeaveRubble || ExtConfigs::HideNoRubbleBuilding);
						auto isTerrain = pType->IsTerrainPalette;
						CIsoViewExt::DirectXBuilding(part.DrawX, part.DrawY - part.pData->FullHeight / 2,
													 part.pData, part.pPal, isCloakable(pType) ? 0.5f : 1.0f,
													 objRender.HouseColor, isRubble, isTerrain, 
													 info.aroundRedrawCell ? pBuilding->GetCellData()->Height : 0xFF);
					}
					else
					{
						CIsoViewExt::BlitSHPTransparent_Building(pThis, ddsd.lpSurface, window, boundary,
																 part.DrawX, part.DrawY - part.pData->FullHeight / 2,
																 part.pData, part.pPal, isCloakable(pType) ? 128 : 255);
					}
					if (part.IsBottom)
					{
						auto draw = [&]
						{
							auto &clips = *pType->GetImageData(objRender.Facing, part.Status);
							for (auto &pData : clips)
							{
								if (ImageDataClassSafe::IsValidImage(pData.get()))
								{
									if (ExtConfigs::DirectXRendering)
									{
										CIsoViewExt::DirectXBuilding(
											x1 - pData->ClipOffsets.FullWidth / 2 + pData->ClipOffsets.LeftOffset,
											y1 - pData->FullHeight / 2, pData.get(),
											NULL, isCloakable(pType) ? 0.5f : 1.0f,
											objRender.HouseColor, false, pType->IsTerrainPalette, 
											info.aroundRedrawCell ? pBuilding->GetCellData()->Height : 0xFF);
									}
									else
									{
										CIsoViewExt::BlitSHPTransparent_Building(pThis, ddsd.lpSurface, window, boundary,
																				 x1 - pData->ClipOffsets.FullWidth / 2 + pData->ClipOffsets.LeftOffset,
																				 y1 - pData->FullHeight / 2, pData.get(), NULL, isCloakable(pType) ? 128 : 255,
																				 objRender.HouseColor, false, pType->IsTerrainPalette);
									}
								}
							}
						};

						FSet drawn;
						DrawTechnoAttachments(draw, drawn, pType, pType->FacingCount <= 1 ? 0 : objRender.Facing,
											  cell, ddsd.lpSurface, boundary,
											  x1, y1,
											  objRender.HouseColor, false);
					}
				}
				else if (part.IsBottom)
				{
					FSet drawn;
					DrawTechnoAttachments([] {}, drawn, pType, pType->FacingCount <= 1 ? 0 : objRender.Facing,
										  cell, ddsd.lpSurface, boundary,
										  x1, y1,
										  objRender.HouseColor, false);
				}

				if (part.IsBottom)
				{
					for (int upgrade = 0; upgrade < objRender.PowerUpCount; ++upgrade)
					{
						const auto &upg = upgrade == 0 ? objRender.PowerUp1 : (upgrade == 1 ? objRender.PowerUp2 : objRender.PowerUp3);
						const auto &upgXX = upgrade == 0 ? pType->PowerUp1LocXX : (upgrade == 1 ? pType->PowerUp2LocXX : pType->PowerUp3LocXX);
						const auto &upgYY = upgrade == 0 ? pType->PowerUp1LocYY : (upgrade == 1 ? pType->PowerUp2LocYY : pType->PowerUp3LocYY);

						if (upg.GetLength() == 0)
							continue;

						auto pUpgType = Renderer::GetOrCreateBuilding(upg);
						auto pUpgData = pUpgType->GetBundledImageData(0);
						if (ImageDataClassSafe::IsValidImage(pUpgData))
						{
							int x2 = x1;
							int y2 = y1;
							x2 += upgXX;
							y2 += upgYY;

							if (ExtConfigs::DirectXRendering)
							{
								CIsoViewExt::DirectXBuilding(
									x2 - pUpgData->FullWidth / 2,
									y2 - pUpgData->FullHeight / 2,
									pUpgData, NULL,
									isCloakable(pType) ? 0.5f : 1.0f,
									objRender.HouseColor, false, pType->IsTerrainPalette, info.aroundRedrawCell ? cell->Height : 0xFF);
							}
							else
							{
								CIsoViewExt::BlitSHPTransparent_Building(pThis, ddsd.lpSurface, window, boundary,
																		 x2 - pUpgData->FullWidth / 2, y2 - pUpgData->FullHeight / 2, pUpgData, NULL, isCloakable(pType) ? 128 : 255,
																		 objRender.HouseColor, false, pType->IsTerrainPalette);
							}
						}
					}
				}

				if (firstDraw && CIsoViewExt::DrawVeterancy)
				{
					auto &veter = DrawVeterancies.emplace_back();
					veter.X = x1 + (DataExt.RealWidth - DataExt.RealHeight) * 30 / 2;
					veter.Y = y1 + (DataExt.RealWidth + DataExt.RealHeight - 2) * 15 / 2;
					veter.VP = 0;
					veter.ID = objRender.ID;
				}

				if (firstDraw && ExtConfigs::InGameDisplay_AlphaImage && CIsoViewExt::DrawAlphaImages && objRender.poweredOn)
				{
					auto pAIData = pType->GetAlphaImageData(objRender.Facing);
					if (ImageDataClassSafe::IsValidImage(pAIData))
					{
						AlphaImagesToDraw.push_back(
							std::make_pair(
								MapCoord{
									x1 - pAIData->FullWidth / 2 + (DataExt.RealWidth - DataExt.RealHeight) * 30 / 2,
									y1 - pAIData->FullHeight / 2 + (DataExt.RealWidth + DataExt.RealHeight) * 15 / 2},
								pAIData));
					}
				}

				if (firstDraw && CIsoViewExt::DrawFires && part.hasFire && DataExt.DamageFireOffsets.size() > 0)
				{
					for (int i = 0; i < DataExt.DamageFireOffsets.size(); ++i)
					{
						if (cellExt->DamagedFires[i] < 0) break;
						const auto &fire = CLoadingExt::DamageFires[cellExt->DamagedFires[i]].get();
						if (ImageDataClassSafe::IsValidImage(fire))
						{
							FiresToDraw.push_back(std::make_pair(
								MapCoord{x1 - fire->FullWidth / 2 + DataExt.DamageFireOffsets[i].x,
										 y1 - fire->FullHeight / 2 + DataExt.DamageFireOffsets[i].y},
								fire));
						}
					}
				}
			}
		}

		// nodes
		for (const auto &part : cellExt->BaseNodeRenderParts)
		{
			const auto &DataExt = CMapDataExt::BuildingDataExts[part.INIIndex];
			bool firstDraw = std::find(DrawnBaseNodes.begin(), DrawnBaseNodes.end(), *part.Data) == DrawnBaseNodes.end();
			DrawnBaseNodes.push_back(*part.Data);

			int x1 = part.Data->X;
			int y1 = part.Data->Y;
			CIsoView::MapCoord2ScreenCoord(x1, y1);
			x1 -= DrawOffsetX;
			y1 -= DrawOffsetY;

			bool strOverlap = false;
			if (!DataExt.IsCustomFoundation())
			{
				for (int dy = 0; dy < DataExt.Width; ++dy)
				{
					for (int dx = 0; dx < DataExt.Height; ++dx)
					{
						const int x = part.Data->X + dx;
						const int y = part.Data->Y + dy;
						int pos = CMapData::Instance->GetCoordIndex(x, y);
						if (pos < CMapDataExt::CellDataExts.size())
						{
							auto &cellExt = CMapDataExt::CellDataExts[pos];
							for (const auto &[_, type] : cellExt.Structures)
							{
								if (type == part.INIIndex)
									strOverlap = true;
							}
						}
					}
				}
			}
			else
			{
				for (const auto &block : *DataExt.Foundations)
				{
					const int x = part.Data->X + block.Y;
					const int y = part.Data->Y + block.X;
					int pos = CMapData::Instance->GetCoordIndex(x, y);
					if (pos < CMapDataExt::CellDataExts.size())
					{
						auto &cellExt = CMapDataExt::CellDataExts[pos];
						for (const auto &[_, type] : cellExt.Structures)
						{
							if (type == part.INIIndex)
								strOverlap = true;
						}
					}
				}
			}

			if (firstDraw && (!CIsoViewExt::RenderingMap || CIsoViewExt::RenderingMap && CIsoViewExt::RenderCurrentLayers) && (CFinalSunApp::Instance->ShowBuildingCells || strOverlap) && (CIsoViewExt::DrawBasenodes || CFinalSunApp::Instance->ShowBuildingCells))
			{
				if (ExtConfigs::DirectXRendering)
				{
					if (DataExt.IsCustomFoundation())
					{
						pThis->DirectXDrawLockedLines(*DataExt.LinesToDraw, x1, y1, part.HouseColor, true);
						pThis->DirectXDrawLockedLines(*DataExt.LinesToDraw, x1 + 1, y1, part.HouseColor, true);
					}
					else
					{
						pThis->DirectXDrawLockedCellOutline(x1, y1, DataExt.Width, DataExt.Height, part.HouseColor, true);
						pThis->DirectXDrawLockedCellOutline(x1 + 1, y1, DataExt.Width, DataExt.Height, part.HouseColor, true);
					}
				}
				else
				{
					if (DataExt.IsCustomFoundation())
					{
						pThis->DrawLockedLines(*DataExt.LinesToDraw, x1, y1, part.HouseColor, true, false, &ddsd);
						pThis->DrawLockedLines(*DataExt.LinesToDraw, x1 + 1, y1, part.HouseColor, true, false, &ddsd);
					}
					else
					{
						pThis->DrawLockedCellOutline(x1, y1, DataExt.Width, DataExt.Height, part.HouseColor, true, false, &ddsd);
						pThis->DrawLockedCellOutline(x1 + 1, y1, DataExt.Width, DataExt.Height, part.HouseColor, true, false, &ddsd);
					}
				}
			}

			if (CIsoViewExt::DrawBasenodes)
			{
				if (ImageDataClassSafe::IsValidImage(part.pData))
				{
					if (ExtConfigs::DirectXRendering)
					{
						CIsoViewExt::DirectXBaseNode(
							part.DrawX, part.DrawY - part.pData->FullHeight / 2,
							part.pData, nullptr, 0.5f, part.HouseColor);
					}
					else
					{
						CIsoViewExt::BlitSHPTransparent_Building(pThis, ddsd.lpSurface, window, boundary,
																 part.DrawX, part.DrawY - part.pData->FullHeight / 2, part.pData, part.pPal, 128);
					}
				}
				if (firstDraw && CIsoViewExt::DrawVeterancy)
				{
					auto &veter = DrawVeterancies.emplace_back();
					veter.X = x1 + (DataExt.RealWidth - DataExt.RealHeight) * 30 / 2;
					veter.Y = y1 + (DataExt.RealWidth + DataExt.RealHeight - 2) * 15 / 2;
					veter.VP = 0;
					veter.ID = part.Data->ID;
					veter.Transp = true;
				}
			}
		}

		CIsoViewExt::CurrentDrawCellLocation.X = X;
		CIsoViewExt::CurrentDrawCellLocation.Y = Y;
		CIsoViewExt::CurrentDrawCellLocation.Height = cell->Height;

		// units
		if (cell->Unit > -1 && Renderer::Vehicles[cell->Unit].IsVisible() && (info.isInMap || ExtConfigs::DisplayObjectsOutside))
		{
			auto &obj = *Renderer::Vehicles[cell->Unit].GetData();
			auto pType = Renderer::Vehicles[cell->Unit].GetType();
			auto pData = pType->GetImageData(obj, CMapDataExt::GetLandType(cell->TileIndex, cell->TileSubIndex));

			bool HoveringUnit = ExtConfigs::InGameDisplay_Hover && pType->IsHoveringUnit;
			auto color = Renderer::Vehicles[cell->Unit].GetHouseColor();

			if (ImageDataClassSafe::IsValidImage(pData))
			{
				if (ExtConfigs::InGameDisplay_Bridge && obj.IsAboveGround == "1" && !CIsoViewExt::RenderingMap)
				{
					if(ExtConfigs::DirectXRendering)
					{
						pThis->DrawLineRawDirectX(x + 30, y + 15 - (HoveringUnit ? 10 : 0) - 60 - 30,
						x + 30, y + 15 - 30, ExtConfigs::CursorSelectionBound_HeightColor,
							false, true, 1, false);
					}
					else
					{
						pThis->DrawLine(x + 30, y + 15 - (HoveringUnit ? 10 : 0) - 60 - 30,
						x + 30, y + 15 - 30, ExtConfigs::CursorSelectionBound_HeightColor,
						false, false, &ddsd, window, true);

					}
				}

				auto draw = [&]
				{
					if (ExtConfigs::DirectXRendering)
					{
						CIsoViewExt::DirectXNormal(
							x - pData->FullWidth / 2,
							y - pData->FullHeight / 2 + 15 - (HoveringUnit ? 10 : 0) -
								(ExtConfigs::InGameDisplay_Bridge && obj.IsAboveGround == "1" ? 60 : 0),
							pData, NULL, isCloakable(pType) ? 0.5f : 1.0f, color, 0, true, info.aroundRedrawCell ? cell->Height : 0xFF);
					}
					else
					{
						CIsoViewExt::BlitSHPTransparent(pThis, ddsd.lpSurface, window, boundary,
														x - pData->FullWidth / 2,
														y - pData->FullHeight / 2 + 15 - (HoveringUnit ? 10 : 0) -
															(ExtConfigs::InGameDisplay_Bridge && obj.IsAboveGround == "1" ? 60 : 0),
														pData, NULL, isCloakable(pType) ? 128 : 255, color, 0, true,
														info.aroundRedrawCell ? &objectOverlapMask : nullptr);
					}
				};
				draw();

				if (CIsoViewExt::DrawVeterancy)
				{
					auto &veter = DrawVeterancies.emplace_back();
					int VP = atoi(obj.VeterancyPercentage);
					veter.X = x;
					veter.Y = y - (HoveringUnit ? 10 : 0) - (ExtConfigs::InGameDisplay_Bridge && obj.IsAboveGround == "1" ? 60 : 0);
					veter.VP = VP;
					veter.ID = obj.TypeID;
				}

				FSet drawn;
				DrawTechnoAttachments(draw, drawn, pType, atoi(obj.Facing),
									  cell, ddsd.lpSurface, boundary,
									  x, y + 15 - (HoveringUnit ? 10 : 0) - (ExtConfigs::InGameDisplay_Bridge && obj.IsAboveGround == "1" ? 60 : 0),
									  color, false);
			}
			else
			{
				FSet drawn;
				DrawTechnoAttachments([] {}, drawn, pType, atoi(obj.Facing),
									  cell, ddsd.lpSurface, boundary,
									  x, y + 15 - (HoveringUnit ? 10 : 0) - (ExtConfigs::InGameDisplay_Bridge && obj.IsAboveGround == "1" ? 60 : 0),
									  color, false);
			}
		}

		// aircrafts
		if (cell->Aircraft > -1 && Renderer::Aircrafts[cell->Aircraft].IsVisible() && (info.isInMap || ExtConfigs::DisplayObjectsOutside))
		{
			auto &obj = *Renderer::Aircrafts[cell->Aircraft].GetData();
			auto pType = Renderer::Aircrafts[cell->Aircraft].GetType();
			auto pData = pType->GetImageData(obj);

			auto color = Renderer::Aircrafts[cell->Aircraft].GetHouseColor();
			if (ImageDataClassSafe::IsValidImage(pData))
			{
				auto draw = [&]
				{
					if (ExtConfigs::DirectXRendering)
					{
						CIsoViewExt::DirectXNormal(
							x - pData->FullWidth / 2, y - pData->FullHeight / 2 + 15,
							pData, NULL, isCloakable(pType) ? 0.5f : 1.0f, color, 2, true, info.aroundRedrawCell ? cell->Height : 0xFF);
					}
					else
					{
						CIsoViewExt::BlitSHPTransparent(pThis, ddsd.lpSurface, window, boundary,
														x - pData->FullWidth / 2, y - pData->FullHeight / 2 + 15, pData, NULL,
														isCloakable(pType) ? 128 : 255, color, 2, true,
														info.aroundRedrawCell ? &objectOverlapMask : nullptr);
					}
				};
				draw();

				if (CIsoViewExt::DrawVeterancy)
				{
					auto &veter = DrawVeterancies.emplace_back();
					int VP = atoi(obj.VeterancyPercentage);
					veter.X = x;
					veter.Y = y;
					veter.VP = VP;
					veter.ID = obj.TypeID;
				}

				FSet drawn;
				DrawTechnoAttachments(draw, drawn, pType, atoi(obj.Facing),
									  cell, ddsd.lpSurface, boundary,
									  x, y + 15,
									  color, false);
			}
			else
			{
				FSet drawn;
				DrawTechnoAttachments([] {}, drawn, pType, atoi(obj.Facing),
									  cell, ddsd.lpSurface, boundary,
									  x, y + 15,
									  color, false);
			}
		}

		// infantries
		for (int i = 2; i >= 0; --i)
		{
			if (cell->Infantry[i] > -1 && Renderer::Infantries[cell->Infantry[i]].IsVisible() && (info.isInMap || ExtConfigs::DisplayObjectsOutside))
			{
				auto &obj = *Renderer::Infantries[cell->Infantry[i]].GetData();
				auto pType = Renderer::Infantries[cell->Infantry[i]].GetType();
				auto pData = pType->GetImageData(obj, CMapDataExt::GetLandType(cell->TileIndex, cell->TileSubIndex));

				int x1 = x;
				int y1 = y;
				Renderer::Infantries[cell->Infantry[i]].OffsetInfantrySubcell(x1, y1);

				if (ExtConfigs::InGameDisplay_Bridge && obj.IsAboveGround == "1")
					y1 -= 60;

				if (ExtConfigs::InGameDisplay_Bridge && obj.IsAboveGround == "1" && !CIsoViewExt::RenderingMap)
				{
					if(ExtConfigs::DirectXRendering)
					{
						pThis->DrawLineRawDirectX(x1 + 30, y1 - 30,
							x1 + 30, y1 + 60 - 30, ExtConfigs::CursorSelectionBound_HeightColor,
							false, true, 1, false);
					}
					else
					{
						pThis->DrawLine(x1 + 30, y1 - 30,
							x1 + 30, y1 + 60 - 30, ExtConfigs::CursorSelectionBound_HeightColor,
							false, false, &ddsd, window, true);
					}
				}

				auto color = Renderer::Infantries[cell->Infantry[i]].GetHouseColor();
				if (ImageDataClassSafe::IsValidImage(pData))
				{
					auto draw = [&]
					{
						if (ExtConfigs::DirectXRendering)
						{
							CIsoViewExt::DirectXNormal(
								x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2,
								pData, NULL, isCloakable(pType) ? 0.5f : 1.0f, color, 1, true, info.aroundRedrawCell ? cell->Height : 0xFF);
						}
						else
						{
							CIsoViewExt::BlitSHPTransparent(pThis, ddsd.lpSurface, window, boundary,
															x1 - pData->FullWidth / 2, y1 - pData->FullHeight / 2, pData, NULL,
															isCloakable(pType) ? 128 : 255, color, 1, true,
															info.aroundRedrawCell ? &objectOverlapMask : nullptr);
						}
					};
					draw();

					if (CIsoViewExt::DrawVeterancy)
					{
						auto &veter = DrawVeterancies.emplace_back();
						int VP = atoi(obj.VeterancyPercentage);
						veter.X = x1 - 5;
						veter.Y = y1 - 4 - 15;
						veter.VP = VP;
						veter.ID = obj.TypeID;
					}

					FSet drawn;
					DrawTechnoAttachments(draw, drawn, pType, atoi(obj.Facing),
										  cell, ddsd.lpSurface, boundary,
										  x1, y1,
										  color, false);
				}
				else
				{
					FSet drawn;
					DrawTechnoAttachments([] {}, drawn, pType, atoi(obj.Facing),
										  cell, ddsd.lpSurface, boundary,
										  x1, y1,
										  color, false);
				}
			}
		}
	}

	for (const auto &fire : FiresToDraw)
	{
		if (ExtConfigs::DirectXRendering)
		{
			CIsoViewExt::DirectXNormal(
				fire.first.X, fire.first.Y,
				fire.second, NULL, 1.0f, 0, -100, false);
		}
		else
		{
			CIsoViewExt::BlitSHPTransparent(pThis, ddsd.lpSurface, window, boundary,
											fire.first.X, fire.first.Y,
											fire.second, NULL, 255, 0, -100, false);
		}
	}

	for (const auto &ai : AlphaImagesToDraw)
	{
		if (ExtConfigs::DirectXRendering)
		{
			CIsoViewExt::DirectXAlphaImage(ai.first.X, ai.first.Y, ai.second);
		}
		else
		{
			CIsoViewExt::BlitSHPTransparent_AlphaImage(pThis, ddsd.lpSurface, window, boundary,
													   ai.first.X, ai.first.Y, ai.second);
		}
	}

	if (CIsoViewExt::DrawVeterancy)
	{
		const char *InsigniaVeteran = "FA2spInsigniaVeteran";
		const char *InsigniaElite = "FA2spInsigniaElite";
		auto veteran = CLoadingExt::GetImageDataFromMap(InsigniaVeteran);
		auto elite = CLoadingExt::GetImageDataFromMap(InsigniaElite);

		for (auto &dv : DrawVeterancies)
		{
			ImageDataClassSafe *pImage = nullptr;
			auto insignia = CLoadingExt::GetInsignia(dv.ID);
			if (dv.VP >= 200)
			{
				pImage = elite;
				if (!insignia.Elite.IsEmpty())
					pImage = CLoadingExt::GetImageDataFromMap(insignia.Elite);
			}
			else if (dv.VP >= 100)
			{
				pImage = veteran;
				if (!insignia.Veteran.IsEmpty())
					pImage = CLoadingExt::GetImageDataFromMap(insignia.Veteran);
			}
			else
			{
				if (!insignia.Rookie.IsEmpty())
					pImage = CLoadingExt::GetImageDataFromMap(insignia.Rookie);
			}
			if (pImage)
			{
				if (ExtConfigs::DirectXRendering)
				{
					CIsoViewExt::DirectXNormal(
						dv.X - pImage->FullWidth / 2 + 10, dv.Y + 21 - pImage->FullHeight / 2,
						pImage, 0, dv.Transp ? 0.5f : 1.0f, 0, -100, false);
				}
				else
				{
					CIsoViewExt::BlitSHPTransparent(pThis, ddsd.lpSurface, window, boundary,
													dv.X - pImage->FullWidth / 2 + 10, dv.Y + 21 - pImage->FullHeight / 2,
													pImage, 0, dv.Transp ? 128 : 255, 0, -100, false);
				}
			}
		}
	}

	if ((CIsoView::CurrentCommand->Command == 0x17 ||
		 CIsoView::CurrentCommand->Command == 0x25) &&
		CIsoViewExt::DrawPropertyBrushMark)
	{
		for (auto &dv : CIsoViewExt::DrawEditedMarks)
		{
			int x1 = dv.X, y1 = dv.Y;
			CIsoView::MapCoord2ScreenCoord(x1, y1);
			x1 -= DrawOffsetX;
			y1 -= DrawOffsetY;
			switch (dv.subPos)
			{
			case 2:
				x1 += 15;
				y1 += 15;
				break;
			case 3:
				x1 -= 15;
				y1 += 15;
				break;
			case 4:
				y1 += 22;
				break;
			default:
				y1 += 15;
				break;
			}
			if (ExtConfigs::DirectXRendering)
			{
				pThis->DirectXBitmap(x1 + 30, y1 - 15, "PROPERTY_MARK");
			}
			else if (auto image = CLoadingExt::GetSurfaceImageDataFromMap("PROPERTY_MARK"))
			{
				pThis->BlitTransparentDesc(image->lpSurface,
										   lpSurface, &ddsd,
										   x1 - image->FullWidth / 2 + 30,
										   y1 - image->FullHeight / 2 - 15, -1, -1,
										   255);
			}
		}
	}

	if (CIsoViewExt::DrawTubes)
	{
		for (const auto &tube : CMapDataExt::Tubes)
		{
			int color = tube.PositiveFacing ? RGB(255, 0, 0) : RGB(0, 0, 255);
			int height = std::min(CMapDataExt::TryGetCellAt(tube.StartCoord.X, tube.StartCoord.Y)->Height,
								  CMapDataExt::TryGetCellAt(tube.EndCoord.X, tube.EndCoord.Y)->Height);
			height *= 15;
			if (CFinalSunApp::Instance->FlatToGround)
				height = 0;
			for (int i = 0; i < tube.PathCoords.size() - 1; ++i)
			{
				int x1, x2, y1, y2;
				x1 = tube.PathCoords[i].X;
				y1 = tube.PathCoords[i].Y;
				x2 = tube.PathCoords[i + 1].X;
				y2 = tube.PathCoords[i + 1].Y;
				CIsoView::MapCoord2ScreenCoord_Flat(x1, y1);
				CIsoView::MapCoord2ScreenCoord_Flat(x2, y2);
				x1 -= DrawOffsetX;
				y1 -= DrawOffsetY;
				x2 -= DrawOffsetX;
				y2 -= DrawOffsetY;
				if (tube.PositiveFacing)
				{
					x1 += 1;
					y1 += 1;
					x2 += 1;
					y2 += 1;
				}
				else
				{
					x1 -= 1;
					y1 -= 1;
					x2 -= 1;
					y2 -= 1;
				}
				if(ExtConfigs::DirectXRendering)
				{
					pThis->DrawLineRawDirectX(x1 + 30, y1 - 15 - height, x2 + 30, y2 - 15 - height, color, true, false, 1, false);
				}
				else
				{
					pThis->DrawLine(x1 + 30, y1 - 15 - height, x2 + 30, y2 - 15 - height, color, false, false, &ddsd, window);
				}
			}
			int x1, x2, y1, y2;
			x1 = tube.StartCoord.X;
			y1 = tube.StartCoord.Y;
			x2 = tube.EndCoord.X;
			y2 = tube.EndCoord.Y;
			CIsoView::MapCoord2ScreenCoord_Flat(x1, y1);
			CIsoView::MapCoord2ScreenCoord_Flat(x2, y2);
			x1 -= DrawOffsetX;
			y1 -= DrawOffsetY;
			x2 -= DrawOffsetX;
			y2 -= DrawOffsetY;
			if (tube.PositiveFacing)
			{
				x1 += 1;
				y1 += 1;
				x2 += 1;
				y2 += 1;
			}
			else
			{
				x1 -= 1;
				y1 -= 1;
				x2 -= 1;
				y2 -= 1;
			}
			if(ExtConfigs::DirectXRendering)
			{			
				pThis->DirectXDrawLockedCellOutline(x1, y1 - height, 1, 1, color, true);
				pThis->DirectXDrawLockedCellOutlineX(x2, y2 - height, 1, 1, color, color, true, true);
			}
			else
			{
				pThis->DrawLockedCellOutline(x1, y1 - height, 1, 1, color, true, false, &ddsd);
				pThis->DrawLockedCellOutlineX(x2, y2 - height, 1, 1, color, color, false, false, &ddsd, true);
			}
		}
	}
	if (CIsoViewExt::RockCells)
	{
		auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");
		auto Map = &CMapData::Instance();
		for (int i = 0; i < Map->CellDataCount; i++)
		{
			auto cell = &Map->CellDatas[i];
			auto &cellExt = CMapDataExt::CellDataExts[i];
			int x = i % Map->MapWidthPlusHeight;
			int y = i / Map->MapWidthPlusHeight;
			int tileIndex = cell->TileIndex;
			if (tileIndex == 65535)
				tileIndex = 0;

			if (CMapDataExt::TileData && tileIndex < CMapDataExt::TileDataCount && cell->TileSubIndex < CMapDataExt::TileData[tileIndex].TileBlockCount)
			{
				auto ttype = CMapDataExt::TileData[tileIndex].TileBlockDatas[cell->TileSubIndex].TerrainType;
				if (ttype == 0x7 || ttype == 0x8 || ttype == 0xf ||
					(cellExt.NewOverlay == 0xFFFF ? false : CMapDataExt::GetOverlayTypeData(cellExt.NewOverlay).TerrainRock))
				{
					CIsoView::MapCoord2ScreenCoord(x, y);
					int drawX = x - DrawOffsetX;
					int drawY = y - DrawOffsetY;
					if(ExtConfigs::DirectXRendering)
					{		
						pThis->DirectXDrawLockedCellOutlineX(drawX, drawY, 1, 1, RGB(255, 0, 0), RGB(40, 0, 0), false);
					}
					else
					{
						pThis->DrawLockedCellOutlineX(drawX, drawY, 1, 1, RGB(255, 0, 0), RGB(40, 0, 0), true, false, &ddsd);
					}
				}
			}
		}
	}
	if (CTerrainGenerator::RangeFirstCell.X > -1 && CTerrainGenerator::RangeSecondCell.X > -1 && (!CIsoViewExt::RenderingMap || CIsoViewExt::RenderingMap && CIsoViewExt::RenderCurrentLayers))
	{
		if (MultiSelection::SelectedCoords.empty())
		{
			int X = CTerrainGenerator::RangeFirstCell.X, Y = CTerrainGenerator::RangeFirstCell.Y;
			int XW = abs(CTerrainGenerator::RangeSecondCell.X - CTerrainGenerator::RangeFirstCell.X) + 1;
			int YW = abs(CTerrainGenerator::RangeSecondCell.Y - CTerrainGenerator::RangeFirstCell.Y) + 1;
			if (X > CTerrainGenerator::RangeSecondCell.X)
				X = CTerrainGenerator::RangeSecondCell.X;
			if (Y > CTerrainGenerator::RangeSecondCell.Y)
				Y = CTerrainGenerator::RangeSecondCell.Y;

			std::vector<MapCoord> coords;
			for (int i = X; i < X + XW; i++)
			{
				for (int j = Y; j < Y + YW; j++)
				{
					coords.push_back({i, j});
				}
			}
			if(ExtConfigs::DirectXRendering)
			{		
				CIsoViewExt::DirectXDrawMultiMapCoordBorders(coords, ExtConfigs::TerrainGeneratorColor, 0, 0, false);
			}
			else
			{
				CIsoViewExt::DrawMultiMapCoordBorders(&ddsd, coords, ExtConfigs::TerrainGeneratorColor);
			}
		}
		else
		{
			CTerrainGenerator::RangeFirstCell.X = -1;
			CTerrainGenerator::RangeFirstCell.Y = -1;
			CTerrainGenerator::RangeSecondCell.X = -1;
			CTerrainGenerator::RangeSecondCell.Y = -1;
		}
	}

	if (ExtConfigs::DirectXRendering)
	{
		for (const auto &info : visibleCells)
		{
			if (!info.isInMap && !ExtConfigs::DisplayObjectsOutside)
				continue;
			auto &X = info.X;
			auto &Y = info.Y;
			auto &pos = info.pos;
			auto &cell = info.cell;
			auto &cellExt = info.cellExt;
			auto &x = info.screenX;
			auto &y = info.screenY;

			auto drawCellTagImage = [&](const ppmfc::CString &id)
			{
				auto itr = CMapDataExt::CustomCelltagColors.find(id);
				if (itr != CMapDataExt::CustomCelltagColors.end())
				{
					auto pTexture = CLoadingExt::DirectXGetOrLoadFlagOrCelltagFromMap(itr->second, false);
					CIsoViewExt::DirectXFlagOrCelltag(x + 29, y + 13, pTexture,
						 ExtConfigs::DrawCelltagTranslucent ? 0.5f : 1.0f);
				}
				else
				{
					CIsoViewExt::DirectXBitmap(x + 29, y + 13, "CELLTAG",
						ExtConfigs::DrawCelltagTranslucent ? 0.5f : 1.0f);
				}
			};

			auto drawWaypointImage = [&](const ppmfc::CString &id)
			{
				auto itr = CMapDataExt::CustomWaypointColors.find(id);
				if (itr != CMapDataExt::CustomWaypointColors.end())
				{
					auto pTexture = CLoadingExt::DirectXGetOrLoadFlagOrCelltagFromMap(itr->second, true);
					CIsoViewExt::DirectXFlagOrCelltag(x + 30, y + 12, pTexture);
				}
				else
				{
					CIsoViewExt::DirectXBitmap(x + 30, y + 12, "FLAG");
				}
			};

			if (cell->CellTag > -1 && cell->CellTag < Celltags.size())
			{
				auto id = Celltags[cell->CellTag];
				if (id)
				{
					if (CIsoViewExt::DrawCellTagsFilter && !CViewObjectsExt::ObjectFilterCT.empty() && !id->IsEmpty())
					{
						for (auto &name : CViewObjectsExt::ObjectFilterCT)
						{
							if (name == *id)
							{
								drawCellTagImage(*id);
								break;
							}
							if (STDHelpers::IsNumber(name))
							{
								int n = atoi(name);
								if (n < 1000000)
								{
									FString buffer;
									buffer.Format("%08d", n + 1000000);
									if (buffer == *id)
									{
										drawCellTagImage(*id);
										break;
									}
								}
							}
						}
					}
					else					
						drawCellTagImage(*id);
				}
			}

			if (cell->Waypoint > -1 && cell->Waypoint < Waypoints.size())
			{
				auto id = Waypoints[cell->Waypoint];
				if (id)
					drawWaypointImage(*id);
			}

			if (CIsoViewExt::DrawAnnotations && CMapDataExt::HasAnnotation(pos))
				CIsoViewExt::DirectXBitmap(x + 30, y + 11, "ANNOTATION");
		}
	}
	else
	{
		// celltag, waypoint, annotation
		DDSURFACEDESC2 CellTagDesc = {sizeof(DDSURFACEDESC2)};
		DDCOLORKEY CellTagColorKey{0, 0};
		auto CellTagImage = CLoadingExt::GetSurfaceImageDataFromMap("CELLTAG");
		bool CellTagLocked = CellTagImage &&
							 CellTagImage->lpSurface->Lock(NULL, &CellTagDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL) == DD_OK;
		if (CellTagImage)
			CellTagImage->lpSurface->GetColorKey(DDCKEY_SRCBLT, &CellTagColorKey);

		DDSURFACEDESC2 WaypointDesc = {sizeof(DDSURFACEDESC2)};
		DDCOLORKEY WaypointColorKey{0, 0};
		auto WaypointImage = CLoadingExt::GetSurfaceImageDataFromMap("FLAG");
		bool WaypointLocked = WaypointImage &&
							  WaypointImage->lpSurface->Lock(NULL, &WaypointDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL) == DD_OK;
		if (WaypointImage)
			WaypointImage->lpSurface->GetColorKey(DDCKEY_SRCBLT, &WaypointColorKey);

		DDSURFACEDESC2 AnnotationDesc = {sizeof(DDSURFACEDESC2)};
		DDCOLORKEY AnnotationColorKey{0, 0};
		auto AnnotationImage = CLoadingExt::GetSurfaceImageDataFromMap("ANNOTATION");
		bool AnnotationLocked = AnnotationImage &&
								AnnotationImage->lpSurface->Lock(NULL, &AnnotationDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL) == DD_OK;
		if (AnnotationImage)
			AnnotationImage->lpSurface->GetColorKey(DDCKEY_SRCBLT, &AnnotationColorKey);

		for (const auto &info : visibleCells)
		{
			if (!info.isInMap && !ExtConfigs::DisplayObjectsOutside)
				continue;
			auto &X = info.X;
			auto &Y = info.Y;
			auto &pos = info.pos;
			auto &cell = info.cell;
			auto &cellExt = info.cellExt;
			auto &x = info.screenX;
			auto &y = info.screenY;

			auto drawCellTagImage = [&](const ppmfc::CString &id)
			{
				auto itr = CMapDataExt::CustomCelltagColors.find(id);
				if (itr != CMapDataExt::CustomCelltagColors.end())
				{
					auto image = CLoadingExt::GetOrLoadFlagOrCelltagFromMap(itr->second, false);
					pThis->BlitTransparentDesc(image->lpSurface,
											   lpSurface, &ddsd,
											   x + 29 - image->FullWidth / 2,
											   y + 13 - image->FullHeight / 2, -1, -1,
											   ExtConfigs::DrawCelltagTranslucent ? 128 : 255);
				}
				else
				{
					pThis->BlitTransparentDescNoLock(CellTagImage->lpSurface,
													 lpSurface, &ddsd, CellTagDesc, CellTagColorKey,
													 x + 29 - CellTagImage->FullWidth / 2,
													 y + 13 - CellTagImage->FullHeight / 2, -1, -1,
													 ExtConfigs::DrawCelltagTranslucent ? 128 : 255);
				}
			};

			auto drawWaypointImage = [&](const ppmfc::CString &id)
			{
				auto itr = CMapDataExt::CustomWaypointColors.find(id);
				if (itr != CMapDataExt::CustomWaypointColors.end())
				{
					auto image = CLoadingExt::GetOrLoadFlagOrCelltagFromMap(itr->second, true);
					pThis->BlitTransparentDesc(image->lpSurface,
											   lpSurface, &ddsd,
											   x + 30 - WaypointImage->FullWidth / 2,
											   y + 12 - WaypointImage->FullHeight / 2);
				}
				else
				{
					pThis->BlitTransparentDescNoLock(WaypointImage->lpSurface,
													 lpSurface, &ddsd, WaypointDesc, WaypointColorKey,
													 x + 30 - WaypointImage->FullWidth / 2,
													 y + 12 - WaypointImage->FullHeight / 2, -1, -1);
				}
			};

			if (CellTagLocked && cell->CellTag > -1 && cell->CellTag < Celltags.size())
			{
				auto id = Celltags[cell->CellTag];
				if (id)
				{
					if (CIsoViewExt::DrawCellTagsFilter && !CViewObjectsExt::ObjectFilterCT.empty() && !id->IsEmpty())
					{
						for (auto &name : CViewObjectsExt::ObjectFilterCT)
						{
							if (name == *id)
							{
								drawCellTagImage(*id);
								break;
							}
							if (STDHelpers::IsNumber(name))
							{
								int n = atoi(name);
								if (n < 1000000)
								{
									FString buffer;
									buffer.Format("%08d", n + 1000000);
									if (buffer == *id)
									{
										drawCellTagImage(*id);
										break;
									}
								}
							}
						}
					}
					else
						drawCellTagImage(*id);
				}
			}

			if (WaypointLocked && cell->Waypoint > -1 && cell->Waypoint < Waypoints.size())
			{
				auto id = Waypoints[cell->Waypoint];
				if (id)
					drawWaypointImage(*id);
			}

			if (AnnotationLocked && CIsoViewExt::DrawAnnotations && CMapDataExt::HasAnnotation(pos))
				pThis->BlitTransparentDescNoLock(AnnotationImage->lpSurface,
												 lpSurface, &ddsd, AnnotationDesc, AnnotationColorKey,
												 x + 5,
												 y - 2, -1, -1);
		}

		if (CellTagImage && CellTagLocked)
			CellTagImage->lpSurface->Unlock(NULL);

		if (WaypointImage && WaypointLocked)
			WaypointImage->lpSurface->Unlock(NULL);

		if (AnnotationImage && AnnotationLocked)
			AnnotationImage->lpSurface->Unlock(NULL);
	}

	auto &cellDataExt = CMapDataExt::CellDataExt_FindCell;
	if (cellDataExt.drawCell)
	{
		int x = cellDataExt.X;
		int y = cellDataExt.Y;

		CIsoView::MapCoord2ScreenCoord(x, y);

		int drawX = x - DrawOffsetX;
		int drawY = y - DrawOffsetY;

		pThis->DrawBitmap("target", drawX - 20, drawY - 11, &ddsd);
	}

	if (!CIsoViewExt::RenderingMap || CIsoViewExt::RenderingMap && CIsoViewExt::RenderInvisibleInGame)
	{
		SetBkMode(hDC, TRANSPARENT);
		SetTextAlign(hDC, TA_CENTER);
		SetTextColor(hDC, RGB(0, 0, 0));

		TextParams param;
		param.SetFont("MS Sans Serif").SetFontSize(14).SetBold().SetAlwaysOnTop()
		.SetAlignCenter().SetColor(ShapeColor::FromCOLORREF(RGB(0, 0, 0))).SetPadding(0, 0);

		if (CIsoViewExt::DrawOverlays)
		{
			for (const auto &[coord, index] : OverlayTextsToDraw)
			{
				if (IsCoordInWindow(coord.X, coord.Y))
				{
					MapCoord mc = coord;
					CIsoView::MapCoord2ScreenCoord(mc.X, mc.Y);
					int drawX = mc.X - DrawOffsetX + 30;
					int drawY = mc.Y - DrawOffsetY - 25;
					if (ExtConfigs::DirectXRendering)
					{
						pThis->g_pTR->DrawTexts(drawX, drawY, index, param);
					}
					else
					{
						TextOut(hDC, drawX, drawY, index, strlen(index));
					}
				}
			}
		}
		if (CIsoViewExt::DrawTerrains)
		{
			for (const auto &[coord, index] : TerrainTextsToDraw)
			{
				if (IsCoordInWindow(coord.X, coord.Y))
				{
					MapCoord mc = coord;
					CIsoView::MapCoord2ScreenCoord(mc.X, mc.Y);
					int drawX = mc.X - DrawOffsetX + 30;
					int drawY = mc.Y - DrawOffsetY - 25;
					if (ExtConfigs::DirectXRendering)
					{
						pThis->g_pTR->DrawTexts(drawX, drawY, index, param);
					}
					else
					{
						TextOut(hDC, drawX, drawY, index, strlen(index));
					}
				}
			}
		}
		if (CIsoViewExt::DrawSmudges)
		{
			for (const auto &[coord, index] : SmudgeTextsToDraw)
			{
				if (IsCoordInWindow(coord.X, coord.Y))
				{
					MapCoord mc = coord;
					CIsoView::MapCoord2ScreenCoord(mc.X, mc.Y);
					int drawX = mc.X - DrawOffsetX + 30;
					int drawY = mc.Y - DrawOffsetY - 25;
					if (ExtConfigs::DirectXRendering)
					{
						pThis->g_pTR->DrawTexts(drawX, drawY, index, param);
					}
					else
					{
						TextOut(hDC, drawX, drawY, index, strlen(index));
					}
				}
			}
		}
	}

	if (CIsoViewExt::DrawBaseNodeIndex)
	{		
		TextParams param;
		param.SetFont("MS Sans Serif").SetFontSize(14).SetBold().SetAlwaysOnTop().
		SetColor(ShapeColor::FromCOLORREF(ExtConfigs::BaseNodeIndex_Color)).
		SetAlignCenter().SetPadding(0, 0);
		SetTextColor(hDC, ExtConfigs::BaseNodeIndex_Color);
		if (ExtConfigs::BaseNodeIndex_Background)
		{
			SetBkMode(hDC, OPAQUE);
			SetBkColor(hDC, ExtConfigs::BaseNodeIndex_Background_Color);
			param.SetBgColor(ShapeColor::FromCOLORREF(ExtConfigs::BaseNodeIndex_Background_Color));
		}
		else
			SetBkMode(hDC, TRANSPARENT);
		SetTextAlign(hDC, TA_CENTER);

		for (const auto &[coord, index] : BaseNodeTextsToDraw)
		{
			if (ExtConfigs::DirectXRendering)
			{
				pThis->g_pTR->DrawTexts(coord.X, coord.Y, index, param);
			}
			else
			{
				TextOut(hDC, coord.X, coord.Y, index, strlen(index));
			}
		}
	}

	if (CIsoViewExt::DrawWaypoints)
	{
		TextParams param;
		SetTextColor(hDC, ExtConfigs::Waypoint_Color);
		if (ExtConfigs::Waypoint_Background)
		{
			SetBkMode(hDC, OPAQUE);
			SetBkColor(hDC, ExtConfigs::Waypoint_Background_Color);
			param.SetBgColor(ShapeColor::FromCOLORREF(ExtConfigs::Waypoint_Background_Color));
		}
		else
			SetBkMode(hDC, TRANSPARENT);
		SetTextAlign(hDC, TA_CENTER);

		param.SetFont("MS Sans Serif").SetFontSize(14).SetBold().SetAlwaysOnTop().
		SetAlignCenter().SetColor(ShapeColor::FromCOLORREF(ExtConfigs::Waypoint_Color)).SetPadding(0, 0);

		for (const auto &[coord, index] : WaypointsToDraw)
		{
			int drawX = coord.X + 30 + ExtConfigs::Waypoint_Text_ExtraOffset.x;
			int drawY = coord.Y - 15 + ExtConfigs::Waypoint_Text_ExtraOffset.y;

			if (ExtConfigs::DirectXRendering)
			{
				pThis->g_pTR->DrawTexts(drawX, drawY, index, param);
			}
			else
			{
				TextOut(hDC, drawX, drawY, index, strlen(index));
			}
		}
	}

	SetTextAlign(hDC, TA_LEFT);
	SetTextColor(hDC, RGB(0, 0, 0));

	if (CIsoViewExt::DrawAnnotations)
	{
		if (auto pSection = CINI::CurrentDocument->GetSection("Annotations"))
		{
			for (const auto &[key, value] : pSection->GetEntities())
			{
				auto pos = atoi(key);
				int x = pos / 1000;
				int y = pos % 1000;
				if (!IsCoordInWindow(x, y) || (!ExtConfigs::DisplayObjectsOutside && !CMapData::Instance->IsCoordInMap(x, y)))
					continue;
				CIsoView::MapCoord2ScreenCoord(x, y);
				x -= DrawOffsetX;
				y -= DrawOffsetY;
				x += 23;
				y -= 15;
				auto atoms = FString::SplitString(value, 6);
				int fontSize = std::min(100, atoi(atoms[0]));
				fontSize = std::max(10, fontSize);
				bool bold = STDHelpers::IsTrue(atoms[1]);
				bool folded = STDHelpers::IsTrue(atoms[2]);
				auto textColor = STDHelpers::HexStringToColorRefRGB(atoms[3]);
				auto bgColor = STDHelpers::HexStringToColorRefRGB(atoms[4]);

				FString text = atoms[5];
				for (int i = 6; i < atoms.size() - 1; i++)
				{
					text += ",";
					text += atoms[i];
				}
				text.Replace("\\n", "\n");

				if (ExtConfigs::DirectXRendering)
				{
					if (folded)
					{
						int maxChars = 3;
						size_t bytePos = 0;
						int charCount = 0;

						while (bytePos < text.length() && charCount < maxChars)
						{
							unsigned char c = static_cast<unsigned char>(text[bytePos]);
							if (IsDBCSLeadByte(c) && bytePos + 1 < text.length())
								bytePos += 2;
							else
								bytePos += 1;
							++charCount;
						}
						if (bytePos < text.length())
						{
							text = text.substr(0, bytePos);

							char toRemove = '\n';
							text.erase(std::remove(text.begin(), text.end(), toRemove), text.end());

							text += "...";
						}
						if (fontSize > 18)
							fontSize = 18;
					}

					TextParams param;
					if (folded ? false : bold)
						param.SetBold();
					param.SetFontSize(fontSize).SetAlwaysOnTop().
					SetColor(ShapeColor::FromCOLORREF(textColor)).
					SetBgColor(ShapeColor::FromRGBA(GetRValue(bgColor), GetGValue(bgColor), GetBValue(bgColor), 128)).
					SetBorder().SetPadding(3, 3);

					pThis->g_pTR->DrawTexts(x, y, text, param);
				}
				else
				{
					auto result = STDHelpers::StringToWString(text);

					if (folded)
					{
						int count = 3;
						if (count < result.length() - 1)
						{
							if (IS_HIGH_SURROGATE(result[count - 1]) && IS_LOW_SURROGATE(result[count]))
							{
								count--;
							}
							result = result.substr(0, count);
							wchar_t toRemove = L'\n';
							result.erase(std::remove(result.begin(), result.end(), toRemove), result.end());
							result += L"...";
						}
						if (fontSize > 18)
							fontSize = 18;
					}
					CIsoViewExt::BlitText(result, textColor, bgColor,
										  pThis, ddsd.lpSurface, window, boundary, x, y, fontSize, 128, folded ? false : bold);
				}
			}
		}
	}

	if (CIsoViewExt::PasteShowOutline && CIsoView::CurrentCommand->Command == 21 && !CopyPaste::PastedCoords.empty() && !CopyPaste::CopyWholeMap && (!CIsoViewExt::RenderingMap || CIsoViewExt::RenderingMap && CIsoViewExt::RenderCurrentLayers))
	{
		if (ExtConfigs::DirectXRendering)
		{
			CIsoViewExt::DirectXDrawMultiMapCoordBorders(CopyPaste::PastedCoords, ExtConfigs::CopySelectionBound_Color, false);
		}
		else
		{
			CIsoViewExt::DrawMultiMapCoordBorders(&ddsd, CopyPaste::PastedCoords, ExtConfigs::CopySelectionBound_Color);
		}
	}
	// line tool
	auto &command = pThis->LastAltCommand;
	if ((GetKeyState(VK_MENU) & 0x8000) && command.isSame() && (!CIsoViewExt::RenderingMap || CIsoViewExt::RenderingMap && CIsoViewExt::RenderCurrentLayers) && CIsoView::CurrentCommand->Command != 4)
	{
		auto point = pThis->GetCurrentMapCoord(pThis->MouseCurrentPosition);
		auto mapCoords = pThis->GetLinePoints({command.X, command.Y}, {point.X, point.Y});
		if (ExtConfigs::DirectXRendering)
		{
			CIsoViewExt::DirectXDrawMultiMapCoordBorders(mapCoords, ExtConfigs::CursorSelectionBound_Color, 0, 0, false);
		}
		else
		{
			CIsoViewExt::DrawMultiMapCoordBorders(&ddsd, mapCoords, ExtConfigs::CursorSelectionBound_Color);
		}
	}

	if (CIsoViewExt::RenderingMap)
	{
		int &height = CMapData::Instance->Size.Height;
		int &width = CMapData::Instance->Size.Width;
		int startX, startY;
		if (CIsoViewExt::RenderFullMap)
		{
			startX = width - 1;
			startY = 0;
		}
		else
		{
			const int &mapwidth = CMapData::Instance->Size.Width;
			const int &mapheight = CMapData::Instance->Size.Height;
			const int &mpL = CMapData::Instance->LocalSize.Left;
			const int &mpT = CMapData::Instance->LocalSize.Top;
			const int &mpW = CMapData::Instance->LocalSize.Width;
			const int &mpH = CMapData::Instance->LocalSize.Height;

			startY = mpT + mpL - 2;
			startX = mapwidth + mpT - mpL - 3;
		}
		pThis->MapCoord2ScreenCoord_Flat(startX, startY);

		RECT r;
		pThis->GetWindowRect(&r);
		pThis->AdaptRectForSecondScreen(&r);

		int pngPosX = r.left + pThis->ViewPosition.x - startX - 4;
		int pngPosY = r.top + pThis->ViewPosition.y - startY - 3 + (CIsoViewExt::RenderFullMap ? 0 : 15);

		if (CIsoViewExt::BlitDDSurfaceRectToBitmap(
				hDC,
				boundary,
				r,
				pngPosX, pngPosY))
			CIsoViewExt::RenderTileSuccess = true;
	}

	if (CIsoViewExt::DrawBounds)
	{
		const int &mapwidth = CMapData::Instance->Size.Width;
		const int &mapheight = CMapData::Instance->Size.Height;

		const int &mpL = CMapData::Instance->LocalSize.Left;
		const int &mpT = CMapData::Instance->LocalSize.Top;
		const int &mpW = CMapData::Instance->LocalSize.Width;
		const int &mpH = CMapData::Instance->LocalSize.Height;

		// blue bound
		{
			int y1 = mpT + mpL - 2;
			int x1 = mapwidth + mpT - mpL - 3;

			int y4 = mpT + mpL + mpW - 2 + mpH + 4;
			int x4 = mapwidth - mpL - mpW + mpT - 3 + mpH + 4;

			CIsoView::MapCoord2ScreenCoord_Flat(x1, y1);
			CIsoView::MapCoord2ScreenCoord_Flat(x4, y4);

			x1 -= DrawOffsetX;
			x4 -= DrawOffsetX;
			y1 -= DrawOffsetY + 15;
			y4 -= DrawOffsetY + 15;

			if (y4 > y1 && x4 > x1)
			{
				if (ExtConfigs::DirectXRendering)
				{
					LineParams param;
					param.SetThickness(5.0f).SetColor(ShapeColor::FromRGBA(0, 0, 255)).SetAntiAlias(false);

					pThis->g_pSP->DrawLine(x1 - 3, y1, x4 + 3, y1, param);
					pThis->g_pSP->DrawLine(x4, y1, x4, y4, param);
					pThis->g_pSP->DrawLine(x1 - 3, y4, x4 + 3, y4, param);
					pThis->g_pSP->DrawLine(x1, y4, x1, y1, param);

					// thin blue bound on top
					if (y1 + 75 < y4)
					{
						param.SetThickness(1.0f);
						pThis->g_pSP->DrawLine(x1 - 1, y1 + 75, x4 + 2, y1 + 75, param);
					}
				}
				else
				{
					pThis->DrawLine(x1 - 1, y1, x4 + 2, y1, RGB(0, 0, 255), false, false, &ddsd, window, false, 5);
					pThis->DrawLine(x4, y1, x4, y4, RGB(0, 0, 255), false, false, &ddsd, window, false, 5);
					pThis->DrawLine(x1 - 1, y4, x4 + 2, y4, RGB(0, 0, 255), false, false, &ddsd, window, false, 5);
					pThis->DrawLine(x1, y4, x1, y1, RGB(0, 0, 255), false, false, &ddsd, window, false, 5);

					// thin blue bound on top
					if (y1 + 75 < y4)
						pThis->DrawLine(x1 - 1, y1 + 75, x4 + 2, y1 + 75, RGB(0, 0, 255), false, false, &ddsd, window, false, 1);
				}
			}
		}
		// red bound
		{
			int y1 = 1;
			int x1 = mapwidth;

			int y4 = mapheight + mapwidth - 1;
			int x4 = mapheight;

			CIsoView::MapCoord2ScreenCoord_Flat(x1, y1);
			CIsoView::MapCoord2ScreenCoord_Flat(x4, y4);

			x1 -= DrawOffsetX - 30;
			x4 -= DrawOffsetX - 30;
			y1 -= DrawOffsetY + 15;
			y4 -= DrawOffsetY + 15;

			if (ExtConfigs::DirectXRendering)
			{
				LineParams param;
				param.SetThickness(5.0f).SetColor(ShapeColor::FromRGBA(255, 0, 0)).SetAntiAlias(false);

				pThis->g_pSP->DrawLine(x1 - 3, y1, x4 + 3, y1, param);
				pThis->g_pSP->DrawLine(x4, y1, x4, y4, param);
				pThis->g_pSP->DrawLine(x1 - 3, y4, x4 + 3, y4, param);
				pThis->g_pSP->DrawLine(x1, y4, x1, y1, param);
			}
			else
			{
				pThis->DrawLine(x1 - 1, y1, x4 + 2, y1, RGB(255, 0, 0), false, false, &ddsd, window, false, 5);
				pThis->DrawLine(x4, y1, x4, y4, RGB(255, 0, 0), false, false, &ddsd, window, false, 5);
				pThis->DrawLine(x1 - 1, y4, x4 + 2, y4, RGB(255, 0, 0), false, false, &ddsd, window, false, 5);
				pThis->DrawLine(x1, y4, x1, y1, RGB(255, 0, 0), false, false, &ddsd, window, false, 5);
			}
		}
	}

	lpSurface->ReleaseDC(hDC);
	lpSurface->Unlock(NULL);

	pThis->ViewPosition = ViewPosition;

	if (ExtConfigs::DirectXRendering)
	{
		CIsoViewExt::SpecialDrawDirectX(0);
		pThis->g_pDX->Render();
		pThis->g_pSP->BeginFrame();

		if (CIsoViewExt::RenderingMap)
		{
			int &height = CMapData::Instance->Size.Height;
			int &width = CMapData::Instance->Size.Width;
			int startX, startY;
			if (CIsoViewExt::RenderFullMap)
			{
				startX = width - 1;
				startY = 0;
			}
			else
			{
				const int &mapwidth = CMapData::Instance->Size.Width;
				const int &mapheight = CMapData::Instance->Size.Height;
				const int &mpL = CMapData::Instance->LocalSize.Left;
				const int &mpT = CMapData::Instance->LocalSize.Top;
				const int &mpW = CMapData::Instance->LocalSize.Width;
				const int &mpH = CMapData::Instance->LocalSize.Height;

				startY = mpT + mpL - 2;
				startX = mapwidth + mpT - mpL - 3;
			}
			pThis->MapCoord2ScreenCoord_Flat(startX, startY);

			RECT r;
			pThis->GetWindowRect(&r);
			pThis->AdaptRectForSecondScreen(&r);

			int pngPosX = r.left + pThis->ViewPosition.x - startX - 4;
			int pngPosY = r.top + pThis->ViewPosition.y - startY - 3 + (CIsoViewExt::RenderFullMap ? 0 : 15);

			// Read DirectX offscreen texture to the full map bitmap
			auto pDX = pThis->g_pDX.get();
			if (auto pOffscreenTex = pDX->GetOffscreenTexture())
			{
				auto pDevice = pDX->GetDevice();
				auto pContext = pDX->GetContext();

				D3D11_TEXTURE2D_DESC texDesc;
				pOffscreenTex->GetDesc(&texDesc);

				// Use client area rect as source â€? the offscreen texture starts at (0,0)
				// in client coordinates, NOT at the window screen position.
				int clientW = pDX->GetClientWidth();
				int clientH = pDX->GetClientHeight();
				int srcLeft = 0, srcTop = 0;
				int srcW = clientW, srcH = clientH;

				// Clip to offscreen texture bounds
				if (srcLeft + srcW > (int)texDesc.Width) srcW = texDesc.Width - srcLeft;
				if (srcTop + srcH > (int)texDesc.Height) srcH = texDesc.Height - srcTop;

				// Handle negative destination (left/top edge of map):
				// offset source rect and reduce copy size accordingly.
				if (pngPosX < 0) { srcLeft += (-pngPosX); srcW += pngPosX; pngPosX = 0; }
				if (pngPosY < 0) { srcTop += (-pngPosY); srcH += pngPosY; pngPosY = 0; }

				if (srcW > 0 && srcH > 0)
				{
					int bmpW = CIsoViewExt::pFullBitmap->GetWidth();
					int bmpH = CIsoViewExt::pFullBitmap->GetHeight();
					if (pngPosX + srcW > bmpW) srcW = bmpW - pngPosX;
					if (pngPosY + srcH > bmpH) srcH = bmpH - pngPosY;

					if (srcW > 0 && srcH > 0)
					{
						D3D11_TEXTURE2D_DESC stagingDesc = {};
						stagingDesc.Width = texDesc.Width;
						stagingDesc.Height = texDesc.Height;
						stagingDesc.MipLevels = 1;
						stagingDesc.ArraySize = 1;
						stagingDesc.Format = texDesc.Format;
						stagingDesc.SampleDesc.Count = 1;
						stagingDesc.Usage = D3D11_USAGE_STAGING;
						stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

						Microsoft::WRL::ComPtr<ID3D11Texture2D> pStaging;
						if (SUCCEEDED(pDevice->CreateTexture2D(&stagingDesc, nullptr, &pStaging)))
						{
							pContext->CopyResource(pStaging.Get(), pOffscreenTex);

							D3D11_MAPPED_SUBRESOURCE mapped;
							if (SUCCEEDED(pContext->Map(pStaging.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
							{
								Gdiplus::BitmapData bitmapData;
								Gdiplus::Rect bmpRect(pngPosX, pngPosY, srcW, srcH);
								if (CIsoViewExt::pFullBitmap->LockBits(&bmpRect, Gdiplus::ImageLockModeWrite,
																	   PixelFormat24bppRGB, &bitmapData) == Gdiplus::Ok)
								{
									const BYTE* srcRow = (const BYTE*)mapped.pData + srcTop * mapped.RowPitch + srcLeft * 4;
									BYTE* dstRow = (BYTE*)bitmapData.Scan0;

									for (LONG y = 0; y < srcH; ++y)
									{
										const BYTE* src = srcRow;
										BYTE* dst = dstRow;
										for (LONG x = 0; x < srcW; ++x)
										{
											// DXGI_FORMAT_R8G8B8A8_UNORM -> GDI+ 24bppRGB (BGR)
											dst[0] = src[2]; // B
											dst[1] = src[1]; // G
											dst[2] = src[0]; // R
											src += 4;
											dst += 3;
										}
										srcRow += mapped.RowPitch;
										dstRow += bitmapData.Stride;
									}

									CIsoViewExt::pFullBitmap->UnlockBits(&bitmapData);
									CIsoViewExt::RenderTileSuccess = true;
								}
								pContext->Unmap(pStaging.Get(), 0);
							}
						}
					}
				}
			}
		}

		return;
	}

	if (CIsoViewExt::RenderingMap)
		return;
	CRect dr = CIsoViewExt::GetVisibleIsoViewRect();
	if (CIsoViewExt::ScaledFactor == 1.0)
	{
		CIsoViewExt::SpecialDraw(CIsoViewExt::GetBackBuffer(), 0);
		CIsoViewExt::ReduceBrightness(CIsoViewExt::GetBackBuffer(), dr);
		if (ExtConfigs::SecondScreenSupport)
		{

			CRect drFixed = dr;
			pThis->BltToWindow(pThis->m_hWnd, CIsoViewExt::GetBackBuffer(), &dr, &drFixed);
		}
		else
			pThis->lpDDPrimarySurface->Blt(&dr, CIsoViewExt::GetBackBuffer(), &dr, DDBLT_WAIT, 0);
	}
	else
	{
		CRect backDr;
		backDr = dr;
		backDr.right += backDr.Width() * (CIsoViewExt::ScaledFactor - 1.0);
		backDr.bottom += backDr.Height() * (CIsoViewExt::ScaledFactor - 1.0);
		CIsoViewExt::StretchCopySurfaceBilinear(pThis->lpDDBackBufferSurface, backDr,
												CIsoViewExt::lpDDBackBufferZoomSurface, dr);
		CIsoViewExt::SpecialDraw(CIsoViewExt::lpDDBackBufferZoomSurface, 0);
		CIsoViewExt::ReduceBrightness(CIsoViewExt::lpDDBackBufferZoomSurface, dr);
		if (ExtConfigs::SecondScreenSupport)
			pThis->BltToWindow(pThis->m_hWnd, CIsoViewExt::lpDDBackBufferZoomSurface, &dr, &dr);
		else
			pThis->lpDDPrimarySurface->Blt(&dr, CIsoViewExt::lpDDBackBufferZoomSurface, &dr, DDBLT_WAIT, 0);
	}
}

DEFINE_HOOK(468690, CIsoView_OnSize, A)
{
	GET(CIsoViewExt *, pThis, ECX);
	if (pThis->g_pDX)
		pThis->g_pDX->OnResize(pThis->GetSafeHwnd());
	return 0;
}

DEFINE_HOOK(4686A2, CIsoView_OnSize_Clipper, 6)
{
	GET(CIsoViewExt *, pThis, ECX);

	if (ExtConfigs::DirectXRendering)
		return 0x4686BD;
	return 0;
}

DEFINE_HOOK(46DE00, CIsoView_Draw, 7)
{
	auto start = std::chrono::high_resolution_clock::now();
	static float smoothedFps = (float)CFinalSunAppExt::ScreenRefreshRate;

	DrawMap();

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	auto count = duration.count();

	float currentFps;
	if (count == 0)
		currentFps = (float)CFinalSunAppExt::ScreenRefreshRate;
	else
		currentFps = 1000.0f / count;

	smoothedFps = smoothedFps * 0.85f + currentFps * 0.15f;

	if (smoothedFps < 10.0f)
		smoothedFps = 10.0f;
	if (smoothedFps > 300.0f)
		smoothedFps = 300.0f;

	CFinalSunAppExt::ScreenRefreshRate = (int)smoothedFps;

	return 0x47519D;
}
