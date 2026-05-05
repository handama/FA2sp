#include "RendererTypes.h"

using namespace Renderer;

const char* SmudgeType::IniSection = "SmudgeTypes";
const char* TerrainType::IniSection = "TerrainTypes";
const char* OverlayType::IniSection = "OverlayTypes";
const char* BuildingType::IniSection = "BuildingTypes";
const char* InfantryType::IniSection = "InfantryTypes";
const char* VehicleType::IniSection = "VehicleTypes";
const char* AircraftType::IniSection = "AircraftTypes";

SmudgeType::SmudgeType(FString_view id)
{
    ID = id;
    Type = CLoadingExt::GameObjectType::Smudge;
    IsImageLoaded = CLoadingExt::IsImageLoaded(ID);
    if (!IsImageLoaded)
        CLoadingExt::GetExtension()->LoadObjects(ID, Type);

    auto imageName = CLoadingExt::GetImageName(ID, 0);
    pImageData = CLoadingExt::GetImageDataFromMap(imageName);
}

TerrainType::TerrainType(FString_view id)
{
    ID = id;
    Type = CLoadingExt::GameObjectType::Terrain;

    IsImageLoaded = CLoadingExt::IsImageLoaded(ID);
    if (!IsImageLoaded)
        CLoadingExt::GetExtension()->LoadObjects(ID, Type);

    IsTiberiumTree = Variables::RulesMap.GetBool(ID, "SpawnsTiberium");
    HasCustomPalette = CLoadingExt::CustomPaletteTerrains.find(ID) != CLoadingExt::CustomPaletteTerrains.end();

    auto imageName = CLoadingExt::GetImageName(ID, 0);
    pImageData = CLoadingExt::GetImageDataFromMap(imageName);

    if (ExtConfigs::InGameDisplay_Shadow)
    {
        auto shadowImageName = CLoadingExt::GetImageName(ID, 0, true);
        pShadowData = CLoadingExt::GetImageDataFromMap(shadowImageName);
    }

    if (ExtConfigs::InGameDisplay_AlphaImage)
    {
        int avaFacings = CLoadingExt::GetAlphaImageFacing(ID);
        if (avaFacings > 0)
        {
            auto AIName = CLoadingExt::GetAlphaImageName(ID, 0, 0);
            pAlphaImageData = CLoadingExt::GetImageDataFromMap(AIName);
        }
    }
}

Renderer::OverlayType::OverlayType(WORD nOverlay)
{
    ID = Variables::RulesMap.GetValueAt("OverlayTypes", nOverlay);
    Type = CLoadingExt::GameObjectType::Overlay;
    OverlayIndex = nOverlay;
    //TypeData = CMapDataExt::GetOverlayTypeData(nOverlay);

    IsImageLoaded = CLoadingExt::IsOverlayLoaded(ID);
    if (!IsImageLoaded)
        CLoadingExt::GetExtension()->LoadOverlay(ID, nOverlay);

    int nMax = 256;
    auto itr = CLoadingExt::OverlayDataLimits.find(nOverlay);
    if (itr != CLoadingExt::OverlayDataLimits.end())
        nMax = itr->second;
    nMax = std::min(256, nMax);

    for (int i = 0; i < nMax; ++i)
    {
        auto imageName = CLoadingExt::GetOverlayName(nOverlay, i);
        pImageData[i] = CLoadingExt::GetImageDataFromMap(imageName);
        if (ExtConfigs::InGameDisplay_Shadow)
        {
            auto shadowImageName = CLoadingExt::GetOverlayName(nOverlay, i, true);
            pShadowData[i] = CLoadingExt::GetImageDataFromMap(shadowImageName);
        }
    }
}

Renderer::BuildingType::BuildingType(FString_view id)
{
    ID = id;
    Type = CLoadingExt::GameObjectType::Building;

    IsImageLoaded = CLoadingExt::IsImageLoaded(ID);
    if (!IsImageLoaded)
        CLoadingExt::GetExtension()->LoadObjects(ID, Type);

    IsTerrainPalette = CMapDataExt::TerrainPaletteBuildings.find(ID) != CMapDataExt::TerrainPaletteBuildings.end();
    IsDamagedAsRubble = CMapDataExt::DamagedAsRubbleBuildings.find(ID) != CMapDataExt::DamagedAsRubbleBuildings.end();
    CanOccupyFire = Variables::RulesMap.GetBool(ID, "CanOccupyFire");
    LeaveRubble = Variables::RulesMap.GetBool(ID, "LeaveRubble");
    HasTurret = Variables::RulesMap.GetBool(ID, "Turret");
    TurretAnimIsVoxel = Variables::RulesMap.GetBool(ID, "TurretAnimIsVoxel");
    TechLevel = Variables::RulesMap.GetInteger(ID, "TechLevel");
    FacingCount = CLoadingExt::GetAvailableFacing(ID);

    auto ArtID = CLoadingExt::GetArtID(ID);
    PowerUp1LocXX = CINI::Art->GetInteger(ArtID, "PowerUp1LocXX", 0);
    PowerUp1LocYY = CINI::Art->GetInteger(ArtID, "PowerUp1LocYY", 0);
    PowerUp2LocXX = CINI::Art->GetInteger(ArtID, "PowerUp2LocXX", 0);
    PowerUp2LocYY = CINI::Art->GetInteger(ArtID, "PowerUp2LocYY", 0);
    PowerUp3LocXX = CINI::Art->GetInteger(ArtID, "PowerUp3LocXX", 0);
    PowerUp3LocYY = CINI::Art->GetInteger(ArtID, "PowerUp3LocYY", 0);

    const int BuildingIdx = CMapDataExt::GetBuildingTypeIndex(ID);
    BuildingIndex = BuildingIdx;
    pDataExt = &CMapDataExt::BuildingDataExts[BuildingIdx];

    int aiFacings = CLoadingExt::GetAlphaImageFacing(ID);
    for (int i = 0; i < FacingCount; ++i)
    {
        FString imageName = CLoadingExt::GetBuildingImageName(ID, i, CLoadingExt::GBIN_NORMAL);
        pImageData[i] = &CLoadingExt::GetBuildingClipImageDataFromMap(imageName);

        imageName = CLoadingExt::GetBuildingImageName(ID, i, CLoadingExt::GBIN_DAMAGED);
        pDamagedImageData[i] = &CLoadingExt::GetBuildingClipImageDataFromMap(imageName);

        imageName = CLoadingExt::GetBuildingImageName(ID, i, CLoadingExt::GBIN_RUBBLE);
        pRubbleImageData[i] = &CLoadingExt::GetBuildingClipImageDataFromMap(imageName);

        if (ExtConfigs::InGameDisplay_Shadow)
        {
            imageName = CLoadingExt::GetBuildingImageName(ID, i, CLoadingExt::GBIN_NORMAL, true);
            pShadowData[i] = CLoadingExt::GetImageDataFromMap(imageName);

            imageName = CLoadingExt::GetBuildingImageName(ID, i, CLoadingExt::GBIN_DAMAGED, true);
            pDamagedShadowData[i] = CLoadingExt::GetImageDataFromMap(imageName);

            imageName = CLoadingExt::GetBuildingImageName(ID, i, CLoadingExt::GBIN_RUBBLE, true);
            pRubbleShadowData[i] = CLoadingExt::GetImageDataFromMap(imageName);
        }

        if (CanOccupyFire && TechLevel < 0)
        {
            imageName = CLoadingExt::GetBuildingImageName(ID, i, CLoadingExt::GBIN_GARRISONDAMAGED);
            pGarrisonDamagedImageData[i] = &CLoadingExt::GetBuildingClipImageDataFromMap(imageName);

            if (ExtConfigs::InGameDisplay_Shadow)
            {
                imageName = CLoadingExt::GetBuildingImageName(ID, i, CLoadingExt::GBIN_GARRISONDAMAGED, true);
                pGarrisonDamagedShadowData[i] = CLoadingExt::GetImageDataFromMap(imageName);
            }
        }

        if (ExtConfigs::InGameDisplay_AlphaImage && aiFacings > 0)
        {
            auto AIName = CLoadingExt::GetAlphaImageName(ID, i * 256 / FacingCount, aiFacings);
            pAlphaImageData[i] = CLoadingExt::GetImageDataFromMap(AIName);
        }
    }
}

ImageDataClassSafe* SmudgeType::GetImageData() const
{
    return pImageData;
}

ImageDataClassSafe* TerrainType::GetImageData() const
{
    return pImageData;
}

ImageDataClassSafe* TerrainType::GetShadowData() const
{
    return pShadowData;
}

ImageDataClassSafe* TerrainType::GetAlphaImageData() const
{
    return pAlphaImageData;
}

ImageDataClassSafe* OverlayType::GetImageData(BYTE nOverlayData) const
{
    return pImageData[nOverlayData];
}

ImageDataClassSafe* OverlayType::GetShadowData(BYTE nOverlayData) const
{
    return pShadowData[nOverlayData];
}

std::vector<std::unique_ptr<ImageDataClassSafe>>* BuildingType::GetImageData(int rawFacing, int status) const
{
    int nFacing = 0;
    if (FacingCount > 1)
    {
        nFacing = (FacingCount + 7 * FacingCount / 8 - (rawFacing * FacingCount / 256) % FacingCount) % FacingCount;
    }
    std::vector<std::unique_ptr<ImageDataClassSafe>>* ret = nullptr;
    switch (status)
    {
    case CLoadingExt::GBIN_NORMAL:
        ret = pImageData[nFacing];
        break;
    case CLoadingExt::GBIN_DAMAGED:
        ret = pDamagedImageData[nFacing];
        break;
    case CLoadingExt::GBIN_RUBBLE:
        ret = pRubbleImageData[nFacing];
        break;
    case CLoadingExt::GBIN_GARRISONDAMAGED:
        ret = pGarrisonDamagedImageData[nFacing];
        break;
    default:
        break;
    }

    static std::vector<std::unique_ptr<ImageDataClassSafe>> emptyVec = [] {
        std::vector<std::unique_ptr<ImageDataClassSafe>> v;

        auto p = std::make_unique<ImageDataClassSafe>();
        p->Flag = ImageDataFlag::SHP;
        p->IsOverlay = false;
        p->pPalette = Palette::PALETTE_UNIT;
        p->ClipOffsets.FullWidth = 0;
        p->ClipOffsets.LeftOffset = 0;

        v.push_back(std::move(p));
        return v;
        }();

    return ret ? ret : &emptyVec;
}

ImageDataClassSafe* BuildingType::GetShadowData(int nFacing, int status) const
{
    switch (status)
    {
    case CLoadingExt::GBIN_NORMAL:
        return pShadowData[nFacing];
    case CLoadingExt::GBIN_DAMAGED:
        return pDamagedShadowData[nFacing];
    case CLoadingExt::GBIN_RUBBLE:
        return pRubbleShadowData[nFacing];
    case CLoadingExt::GBIN_GARRISONDAMAGED:
        return pGarrisonDamagedShadowData[nFacing];
    default:
        break;
    }
    return pShadowData[nFacing];
}

ImageDataClassSafe* Renderer::BuildingType::GetAlphaImageData(int rawFacing) const
{
    return pAlphaImageData[rawFacing * FacingCount / 256];
}

SmudgeType* Renderer::GetOrCreateSmudge(FString_view id)
{
    auto [itr, inserted] = SmudgeTypes.try_emplace(id, id);
    return &itr->second;
}

TerrainType* Renderer::GetOrCreateTerrain(FString_view id)
{
    auto [itr, inserted] = TerrainTypes.try_emplace(id, id);
    return &itr->second;
}

OverlayType* Renderer::GetOrCreateOverlay(WORD nOverlay)
{
    auto [itr, inserted] = OverlayTypes.try_emplace(nOverlay, nOverlay);
    return &itr->second;
}

BuildingType* Renderer::GetOrCreateBuilding(FString_view id)
{
    auto [itr, inserted] = BuildingTypes.try_emplace(id, id);
    return &itr->second;
}