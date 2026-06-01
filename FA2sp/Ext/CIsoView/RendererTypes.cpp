#include "RendererTypes.h"

using namespace Renderer;

const char* SmudgeType::IniSection = "SmudgeTypes";
const char* TerrainType::IniSection = "TerrainTypes";
const char* OverlayType::IniSection = "OverlayTypes";
const char* BuildingType::IniSection = "BuildingTypes";
const char* InfantryType::IniSection = "InfantryTypes";
const char* VehicleType::IniSection = "VehicleTypes";
const char* AircraftType::IniSection = "AircraftTypes";

void SmudgeType::Init(FString_view id)
{
    ID = id;
    Type = CLoadingExt::GameObjectType::Smudge;
    IsImageLoaded = CLoadingExt::IsImageLoaded(ID);
    if (!IsImageLoaded)
    {
        CLoadingExt::GetExtension()->LoadObjects(ID, Type);
        IsImageLoaded = true;
    }

    auto imageName = CLoadingExt::GetImageName(ID, 0);
    pImageData = CLoadingExt::GetImageDataFromMap(imageName);

    if (CLoadingExt::ObjectsNeedReloaded && ExtConfigs::DirectXRendering)
    {
        pImageData->GetTexture();
    }
}

void TerrainType::Init(FString_view id)
{
    ID = id;
    Type = CLoadingExt::GameObjectType::Terrain;

    IsImageLoaded = CLoadingExt::IsImageLoaded(ID);
    if (!IsImageLoaded)
    {
        CLoadingExt::GetExtension()->LoadObjects(ID, Type);
        IsImageLoaded = true;
    }

    IsTiberiumTree = Variables::RulesMap.GetBool(ID, "SpawnsTiberium");
    HasCustomPalette = CLoadingExt::CustomPaletteTerrains.find(ID) != CLoadingExt::CustomPaletteTerrains.end();

    auto imageName = CLoadingExt::GetImageName(ID, 0);
    pImageData = CLoadingExt::GetImageDataFromMap(imageName);

    if (CLoadingExt::ObjectsNeedReloaded && ExtConfigs::DirectXRendering)
    {
        pImageData->GetTexture();
    }

    if (ExtConfigs::InGameDisplay_Shadow)
    {
        auto shadowImageName = CLoadingExt::GetImageName(ID, 0, true);
        pShadowData = CLoadingExt::GetImageDataFromMap(shadowImageName);
        if (CLoadingExt::ObjectsNeedReloaded && ExtConfigs::DirectXRendering)
        {
            pShadowData->GetTexture();
        }
    }

    if (ExtConfigs::InGameDisplay_AlphaImage)
    {
        int avaFacings = CLoadingExt::GetAlphaImageFacing(ID);
        if (avaFacings > 0)
        {
            auto AIName = CLoadingExt::GetAlphaImageName(ID, 0, 0);
            pAlphaImageData = CLoadingExt::GetImageDataFromMap(AIName);
            if (CLoadingExt::ObjectsNeedReloaded && ExtConfigs::DirectXRendering)
            {
                pAlphaImageData->GetTexture(nullptr, true);
            }
        }
    }
}

void OverlayType::Init(WORD nOverlay)
{
    ID = Variables::RulesMap.GetValueAt("OverlayTypes", nOverlay);
    Type = CLoadingExt::GameObjectType::Overlay;
    OverlayIndex = nOverlay;
    TypeData = CMapDataExt::GetOverlayTypeData(nOverlay);

    IsImageLoaded = CLoadingExt::IsOverlayLoaded(ID);
    if (!IsImageLoaded)
    {
        CLoadingExt::GetExtension()->LoadOverlay(ID, nOverlay);
        IsImageLoaded = true;
    }

    int nMax = ExtConfigs::OverlayDataLimit;
    auto itr = CLoadingExt::OverlayDataLimits.find(nOverlay);
    if (itr != CLoadingExt::OverlayDataLimits.end())
        nMax = itr->second;
    nMax = std::min(ExtConfigs::OverlayDataLimit, nMax);

    for (int i = 0; i < nMax; ++i)
    {
        auto imageName = CLoadingExt::GetOverlayName(nOverlay, i);
        pImageData[i] = CLoadingExt::GetImageDataFromMap(imageName);

        if (CLoadingExt::ObjectsNeedReloaded && ExtConfigs::DirectXRendering
             && (!TypeData.Wall || !ExtConfigs::InGameDisplay_RemapableOverlay))
        {
            pImageData[i]->GetTexture();
        }

        if (ExtConfigs::InGameDisplay_Shadow)
        {
            auto shadowImageName = CLoadingExt::GetOverlayName(nOverlay, i, true);
            pShadowData[i] = CLoadingExt::GetImageDataFromMap(shadowImageName);

            if (CLoadingExt::ObjectsNeedReloaded  && ExtConfigs::DirectXRendering
                && (!TypeData.Wall || !ExtConfigs::InGameDisplay_RemapableOverlay))
            {
                pShadowData[i]->GetTexture();
            }
        }
    }
}

void TechnoType::InitTechnoAttachmentInfo()
{
    TechnoAttachmentInfo = CMapDataExt::GetTechnoAttachmentInfo(ID);
}

void BuildingType::Init(FString_view id)
{
    ID = id;
    Type = CLoadingExt::GameObjectType::Building;

    IsImageLoaded = CLoadingExt::IsImageLoaded(ID);
    if (!IsImageLoaded)
    {
        CLoadingExt::GetExtension()->LoadObjects(ID, Type);
        IsImageLoaded = true;
    }

    IsTerrainPalette = CMapDataExt::TerrainPaletteBuildings.find(ID) != CMapDataExt::TerrainPaletteBuildings.end();
    IsDamagedAsRubble = CMapDataExt::DamagedAsRubbleBuildings.find(ID) != CMapDataExt::DamagedAsRubbleBuildings.end();
    CanOccupyFire = Variables::RulesMap.GetBool(ID, "CanOccupyFire");
    LeaveRubble = Variables::RulesMap.GetBool(ID, "LeaveRubble");
    HasTurret = Variables::RulesMap.GetBool(ID, "Turret");
    Cloakable = Variables::RulesMap.GetBool(ID, "Cloakable");
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

    InitTechnoAttachmentInfo();
}

void VehicleType::Init(FString_view id)
{
    ID = id;
    Type = CLoadingExt::GameObjectType::Vehicle;

    IsImageLoaded = CLoadingExt::IsImageLoaded(ID);
    if (!IsImageLoaded)
    {
        CLoadingExt::GetExtension()->LoadObjects(ID, Type);
        IsImageLoaded = true;
    }

    ShouldUseDefaultImage = true;
    FacingCount = CLoadingExt::GetAvailableFacing(ID);
    for (int i = 0; i < FacingCount; ++i)
    {
        FString imageName = CLoadingExt::GetImageName(ID, i);
        pImageData[i] = CLoadingExt::GetImageDataFromMap(imageName);
        if (ImageDataClassSafe::IsVisibleImage(pImageData[i]))
            ShouldUseDefaultImage = false;

        if (ExtConfigs::InGameDisplay_Shadow)
        {
            imageName = CLoadingExt::GetImageName(ID, i, true);
            pShadowData[i] = CLoadingExt::GetImageDataFromMap(imageName);
        }
    }

    if (auto pValue = Variables::RulesMap.TryGetString(ID, "WaterImage"))
        WaterImage = Renderer::GetOrCreateVehicle(*pValue);
    
    if (auto pValue = Variables::RulesMap.TryGetString(ID, "Image.ConditionYellow"))
        ConditionYellowImage = Renderer::GetOrCreateVehicle(*pValue);
    
    if (auto pValue = Variables::RulesMap.TryGetString(ID, "Image.ConditionRed"))
        ConditionRedImage = Renderer::GetOrCreateVehicle(*pValue);
    
    if (auto pValue = Variables::RulesMap.TryGetString(ID, "WaterImage.ConditionYellow"))
        ConditionYellowWaterImage = Renderer::GetOrCreateVehicle(*pValue);
    
    if (auto pValue = Variables::RulesMap.TryGetString(ID, "WaterImage.ConditionRed"))
        ConditionRedWaterImage = Renderer::GetOrCreateVehicle(*pValue);
    
    if (auto pValue = Variables::RulesMap.TryGetString(ID, "UnloadingClass"))
        UnloadingImage = Renderer::GetOrCreateVehicle(*pValue);

    if (ExtConfigs::UseDefaultUnitImage)
        DefaultImage = Renderer::GetOrCreateVehicle("FA2DEFAULT_UNIT");

    IsHoveringUnit = Variables::RulesMap.GetString(ID, "SpeedType") == "Hover"
        && (Variables::RulesMap.GetString(ID, "Locomotor") == "Hover"
            || Variables::RulesMap.GetString(ID, "Locomotor") == "{4A582742-9839-11d1-B709-00A024DDAFD1}");
    Cloakable = Variables::RulesMap.GetBool(ID, "Cloakable");

    InitTechnoAttachmentInfo();
}

void AircraftType::Init(FString_view id)
{
    ID = id;
    Type = CLoadingExt::GameObjectType::Aircraft;

    IsImageLoaded = CLoadingExt::IsImageLoaded(ID);
    if (!IsImageLoaded)
    {
        CLoadingExt::GetExtension()->LoadObjects(ID, Type);
        IsImageLoaded = true;
    }

    ShouldUseDefaultImage = true;
    FacingCount = CLoadingExt::GetAvailableFacing(ID);
    for (int i = 0; i < FacingCount; ++i)
    {
        FString imageName = CLoadingExt::GetImageName(ID, i);
        pImageData[i] = CLoadingExt::GetImageDataFromMap(imageName);
        if (ImageDataClassSafe::IsVisibleImage(pImageData[i]))
            ShouldUseDefaultImage = false;
    }
 
    if (auto pValue = Variables::RulesMap.TryGetString(ID, "Image.ConditionYellow"))
        ConditionYellowImage = Renderer::GetOrCreateAircraft(*pValue);
    
    if (auto pValue = Variables::RulesMap.TryGetString(ID, "Image.ConditionRed"))
        ConditionRedImage = Renderer::GetOrCreateAircraft(*pValue);
    
    if (ExtConfigs::UseDefaultUnitImage)
        DefaultImage = Renderer::GetOrCreateAircraft("FA2DEFAULT_AIRCRAFT");

    Cloakable = Variables::RulesMap.GetBool(ID, "Cloakable");

    InitTechnoAttachmentInfo();
}

void InfantryType::Init(FString_view id)
{
    ID = id;
    Type = CLoadingExt::GameObjectType::Infantry;

    IsImageLoaded = CLoadingExt::IsImageLoaded(ID);
    if (!IsImageLoaded)
    {
        CLoadingExt::GetExtension()->LoadObjects(ID, Type);
        IsImageLoaded = true;
    }

    FacingCount = 8;
    IsDeployer = Variables::RulesMap.GetBool(ID, "Deployer");
    Swimable = CLoadingExt::SwimableInfantries.contains(ID);
    Cloakable = Variables::RulesMap.GetBool(ID, "Cloakable");
    
    ShouldUseDefaultImage = true;
    for (int i = 0; i < FacingCount; ++i)
    {
        FString imageName = CLoadingExt::GetImageName(ID, i);
        pImageData[i] = CLoadingExt::GetImageDataFromMap(imageName);
        if (ImageDataClassSafe::IsVisibleImage(pImageData[i]))
            ShouldUseDefaultImage = false;

        if (ExtConfigs::InGameDisplay_Shadow)
        {
            imageName = CLoadingExt::GetImageName(ID, i, true);
            pShadowData[i] = CLoadingExt::GetImageDataFromMap(imageName);
        }

        if (IsDeployer)
        {
            imageName = CLoadingExt::GetImageName(ID, i, false, true, false);
            pDeployImageData[i] = CLoadingExt::GetImageDataFromMap(imageName);

            if (!ImageDataClassSafe::IsValidImage(pDeployImageData[i]))
                pDeployImageData[i] = pImageData[i];

            if (ExtConfigs::InGameDisplay_Shadow)
            {
                imageName = CLoadingExt::GetImageName(ID, i, true, true, false);
                pDeployShadowData[i] = CLoadingExt::GetImageDataFromMap(imageName);

                if (!ImageDataClassSafe::IsValidImage(pDeployShadowData[i]))
                    pDeployShadowData[i] = pShadowData[i];
            }
        }

        if (Swimable)
        {
            imageName = CLoadingExt::GetImageName(ID, i, false, false, true);
            pWaterImageData[i] = CLoadingExt::GetImageDataFromMap(imageName);

            if (!ImageDataClassSafe::IsValidImage(pWaterImageData[i]))
                pWaterImageData[i] = pImageData[i];

            if (ExtConfigs::InGameDisplay_Shadow)
            {
                imageName = CLoadingExt::GetImageName(ID, i, true, false, true);
                pWaterShadowData[i] = CLoadingExt::GetImageDataFromMap(imageName);

                if (!ImageDataClassSafe::IsValidImage(pWaterShadowData[i]))
                    pWaterShadowData[i] = pShadowData[i];
            }
        }
    }

    if (ExtConfigs::UseDefaultUnitImage)
        DefaultImage = Renderer::GetOrCreateInfantry("FA2DEFAULT_INFANTRY");

    InitTechnoAttachmentInfo();
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

bool Renderer::OverlayType::IsBridge() const
{
    return OverlayIndex == 0x18 || OverlayIndex == 0x19 ||
        OverlayIndex == 0x3B || OverlayIndex == 0x3C ||
        OverlayIndex == 0xED || OverlayIndex == 0xEE ||
        (OverlayIndex >= 0x4A && OverlayIndex <= 0x65) ||
        (OverlayIndex >= 0xCD && OverlayIndex <= 0xEC);
}

bool Renderer::OverlayType::IsVisibleInMapRendererOrNormal() const
{
    return OverlayIndex != 0xffff && (!CIsoViewExt::RenderingMap
        || CIsoViewExt::RenderingMap
        && CIsoViewExt::MapRendererIgnoreObjects.find(ID)
        == CIsoViewExt::MapRendererIgnoreObjects.end());
}

std::vector<std::unique_ptr<ImageDataClassSafe>>* BuildingType::GetImageData(int rawFacing, int status, int forceFacing) const
{
    int nFacing = 0;
    if (FacingCount > 1)
    {
        nFacing = (FacingCount + 7 * FacingCount / 8 - (rawFacing * FacingCount / 256) % FacingCount) % FacingCount;
    }
    if (forceFacing > -1)
    {
        nFacing = forceFacing;
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

ImageDataClassSafe* Renderer::BuildingType::GetBundledImageData(int forceFacing)
{
    if (!pBundledImageData[forceFacing])
    {
        auto clips = GetImageData(0, 0, forceFacing);
        pBundledImageData[forceFacing] = std::move(CLoadingExt::BindClippedImages(*clips));
    }
    return pBundledImageData[forceFacing].get();
}

VehicleType* Renderer::VehicleType::GetAlteredType(const CUnitDataFS& obj, const LandType landType)
{
    VehicleType* pType = this;
    if (ExtConfigs::InGameDisplay_Water)
    {
        if ((landType == LandType::Water || landType == LandType::Beach) && obj.IsAboveGround != "1")
        {
            pType = WaterImage;
        }
    }
    if (ExtConfigs::InGameDisplay_Damage)
    {
        int HP = atoi(obj.Health);
        if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP)
        {
            pType = ConditionYellowImage;
        }
        if (static_cast<int>((CMapDataExt::ConditionRed + 0.001f) * 256) > HP)
        {
            pType = ConditionRedImage;
        }
        if (ExtConfigs::InGameDisplay_Water)
        {
            if ((landType == LandType::Water || landType == LandType::Beach) && obj.IsAboveGround != "1")
            {
                if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP)
                {
                    pType = ConditionYellowWaterImage;
                }
                if (static_cast<int>((CMapDataExt::ConditionRed + 0.001f) * 256) > HP)
                {
                    pType = ConditionRedWaterImage;
                }
            }
        }
    }

    if (ExtConfigs::InGameDisplay_Deploy && obj.Status == "Unload")
    {
        pType = UnloadingImage;
    }

    return pType;
}

ImageDataClassSafe* Renderer::VehicleType::GetImageData(const CUnitDataFS& obj, const LandType landType)
{
    auto pType = GetAlteredType(obj, landType);
    if (!pType)
        pType = this;

    if (DefaultImage && ExtConfigs::UseDefaultUnitImage && pType->ShouldUseDefaultImage)
        pType = DefaultImage;

    int nFacing = (atoi(obj.Facing) * pType->FacingCount / 256) % pType->FacingCount;

    return pType->pImageData[nFacing];
}

ImageDataClassSafe* Renderer::VehicleType::GetShadowData(const CUnitDataFS& obj, const LandType landType)
{
    auto pType = GetAlteredType(obj, landType);
    if (!pType)
        pType = this;

    if (DefaultImage && ExtConfigs::UseDefaultUnitImage && pType->ShouldUseDefaultImage)
        pType = DefaultImage;

    int nFacing = (atoi(obj.Facing) * pType->FacingCount / 256) % pType->FacingCount;

    return pType->pShadowData[nFacing];
}

ImageDataClassSafe* Renderer::VehicleType::GetTechnoAttachmentImageData(int nFacing, bool bShadow) const
{
    if (DefaultImage && ExtConfigs::UseDefaultUnitImage_TechnoAttachment && ShouldUseDefaultImage)
        return bShadow ? DefaultImage->pShadowData[nFacing] : DefaultImage->pImageData[nFacing];

    return bShadow ? pShadowData[nFacing] : pImageData[nFacing];
}

AircraftType* Renderer::AircraftType::GetAlteredType(const CAircraftDataFS& obj)
{
    AircraftType* pType = this;
    if (ExtConfigs::InGameDisplay_Damage)
    {
        int HP = atoi(obj.Health);
        if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP)
        {
            pType = ConditionYellowImage;
        }
        if (static_cast<int>((CMapDataExt::ConditionRed + 0.001f) * 256) > HP)
        {
            pType = ConditionRedImage;
        }
    }

    return pType;
}

ImageDataClassSafe* Renderer::AircraftType::GetImageData(const CAircraftDataFS& obj)
{
    auto pType = GetAlteredType(obj);
    if (!pType)
        pType = this;

    if (DefaultImage && ExtConfigs::UseDefaultUnitImage && pType->ShouldUseDefaultImage)
        pType = DefaultImage;

    int nFacing = (atoi(obj.Facing) * pType->FacingCount / 256) % pType->FacingCount;

    return pType->pImageData[nFacing];
}

ImageDataClassSafe* Renderer::AircraftType::GetTechnoAttachmentImageData(int nFacing) const
{
    if (DefaultImage && ExtConfigs::UseDefaultUnitImage_TechnoAttachment && ShouldUseDefaultImage)
        return DefaultImage->pImageData[nFacing];

    return pImageData[nFacing];
}

ImageDataClassSafe* Renderer::InfantryType::GetImageData(const CInfantryData& obj, const LandType landType) const
{
    int nFacing = 7 - (atoi(obj.Facing) / 32) % 8;
    if (DefaultImage && ExtConfigs::UseDefaultUnitImage && ShouldUseDefaultImage)
        return DefaultImage->pImageData[nFacing];

    bool bWater = ExtConfigs::InGameDisplay_Water && Swimable
        && (landType == LandType::Water || landType == LandType::Beach)
        && obj.IsAboveGround != "1";

    if (bWater)
        return pWaterImageData[nFacing];

    bool bDeploy = ExtConfigs::InGameDisplay_Deploy
        && obj.Status == "Unload" && IsDeployer;

    if (bDeploy)
        return pDeployImageData[nFacing];
    return pImageData[nFacing];
}

ImageDataClassSafe* Renderer::InfantryType::GetShadowData(const CInfantryData& obj, const LandType landType) const
{
    int nFacing = 7 - (atoi(obj.Facing) / 32) % 8;
    if (DefaultImage && ExtConfigs::UseDefaultUnitImage && ShouldUseDefaultImage)
        return DefaultImage->pShadowData[nFacing];

    bool bWater = ExtConfigs::InGameDisplay_Water && Swimable
        && (landType == LandType::Water || landType == LandType::Beach)
        && obj.IsAboveGround != "1";

    if (bWater)
        return pWaterShadowData[nFacing];

    bool bDeploy = ExtConfigs::InGameDisplay_Deploy
        && obj.Status == "Unload" && IsDeployer;

    if (bDeploy)
        return pDeployShadowData[nFacing];
    return pShadowData[nFacing];
}

ImageDataClassSafe* Renderer::InfantryType::GetTechnoAttachmentImageData(int nFacing, bool bShadow) const
{
    if (DefaultImage && ExtConfigs::UseDefaultUnitImage_TechnoAttachment && ShouldUseDefaultImage)
        return bShadow ? DefaultImage->pShadowData[nFacing] : DefaultImage->pImageData[nFacing];

    return bShadow ? pShadowData[nFacing] : pImageData[nFacing];
}

SmudgeType* Renderer::GetOrCreateSmudge(FString_view id)
{
    auto [itr, inserted] = SmudgeTypes.try_emplace(id);

    if (inserted)
        itr->second.Init(id);

    return &itr->second;
}

TerrainType* Renderer::GetOrCreateTerrain(FString_view id)
{
    auto [itr, inserted] = TerrainTypes.try_emplace(id);

    if (inserted)
        itr->second.Init(id);

    return &itr->second;
}

OverlayType* Renderer::GetOrCreateOverlay(WORD nOverlay)
{
    auto [itr, inserted] = OverlayTypes.try_emplace(nOverlay);

    if (inserted)
        itr->second.Init(nOverlay);

    return &itr->second;
}

BuildingType* Renderer::GetOrCreateBuilding(FString_view id)
{
    auto [itr, inserted] = BuildingTypes.try_emplace(id);

    if (inserted)
        itr->second.Init(id); 

    return &itr->second;
}

VehicleType* Renderer::GetOrCreateVehicle(FString_view id)
{
    auto [itr, inserted] = VehicleTypes.try_emplace(id);

    if (inserted)
        itr->second.Init(id);

    return &itr->second;
}

AircraftType* Renderer::GetOrCreateAircraft(FString_view id)
{
    auto [itr, inserted] = AircraftTypes.try_emplace(id);

    if (inserted)
        itr->second.Init(id);

    return &itr->second;
}

InfantryType* Renderer::GetOrCreateInfantry(FString_view id)
{
    auto [itr, inserted] = InfantryTypes.try_emplace(id);

    if (inserted)
        itr->second.Init(id);

    return &itr->second;
}

void Renderer::Building::Reload(short index)
{
    Visible = false;
    pType = nullptr;
    pRenderData = nullptr;

    if (index < 0 || index >= CMapDataExt::StructureIndexMap.size())
    {
        return;
    }

    auto StrINIIndex = CMapDataExt::StructureIndexMap[index];
    if (StrINIIndex < 0 
        || StrINIIndex >= CMapDataExt::BuildingDatasExt.size()
        || StrINIIndex >= CMapDataExt::BuildingRenderDatasFix.size())
        return;

    auto& data = CMapDataExt::GetBuildingDataFsFromMap(StrINIIndex);
    pObjectData = &data;
    pType = GetOrCreateBuilding(data.TypeID);
    pRenderData = &CMapDataExt::BuildingRenderDatasFix[StrINIIndex];
    pCellData = CMapData::Instance->TryGetCellAt(pRenderData->X, pRenderData->Y);
    HouseColor = pRenderData->HouseColor;

    if (CLoadingExt::ObjectsNeedReloaded)
    {
        auto pType = GetType();
        BGRStruct color(HouseColor);
        if (ExtConfigs::DirectXRendering)
        {
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

        for (int upgrade = 0; upgrade < pRenderData->PowerUpCount; ++upgrade)
        {
            const auto &upg = upgrade == 0 ? pRenderData->PowerUp1 : (upgrade == 1 ? pRenderData->PowerUp2 : pRenderData->PowerUp3);
            if (upg.GetLength() == 0)
                continue;

            auto pUpgType = Renderer::GetOrCreateBuilding(upg);
            auto pUpgData = pUpgType->GetBundledImageData(0);
            auto pUpgShadow = pUpgType->GetShadowData(0, 0);
            if (ExtConfigs::DirectXRendering)
            {
                if (ImageDataClassSafe::IsValidImage(pUpgData))
                {                   
                    auto newPal = PalettesManager::GetColoredPalette(pUpgData->pPalette, color);
                    pUpgData->GetBuildingColoredTextures(newPal, color);
                }

                if (ImageDataClassSafe::IsValidImage(pUpgShadow))
                {                   
                    pUpgShadow->GetTexture();
                }
            }
        }
    }

    if (!CIsoViewExt::DrawStructures)
        return;

    bool FilterVisible = false;
    bool RenderMapVisible = false;
    if (CIsoViewExt::DrawStructuresFilter && CViewObjectsExt::BuildingBrushDlgBF)
    {     
        auto CheckValue = [&](int nCheckBoxIdx, const ppmfc::CString& src, const ppmfc::CString& dst)
        {
            if (CViewObjectsExt::BuildingBrushBoolsBF[nCheckBoxIdx - 1300])
            {
                if (dst == src) return true;
                else return false;
            }
            return true;
        };

        const auto& filter = CViewObjectsExt::ObjectFilterB;
        if (filter.empty() || std::find(filter.begin(), filter.end(), data.TypeID) != filter.end())
        {
            if (CheckValue(1300, CViewObjectsExt::BuildingBrushDlgBF->CString_House, data.House) &&
                CheckValue(1301, CViewObjectsExt::BuildingBrushDlgBF->CString_HealthPoint, data.Health) &&
                CheckValue(1302, CViewObjectsExt::BuildingBrushDlgBF->CString_Direction, data.Facing) &&
                CheckValue(1303, CViewObjectsExt::BuildingBrushDlgBF->CString_Sellable, data.AISellable) &&
                CheckValue(1304, CViewObjectsExt::BuildingBrushDlgBF->CString_Rebuildable, data.AIRebuildable) &&
                CheckValue(1305, CViewObjectsExt::BuildingBrushDlgBF->CString_EnergySupport, data.PoweredOn) &&
                CheckValue(1306, CViewObjectsExt::BuildingBrushDlgBF->CString_UpgradeCount, data.Upgrades) &&
                CheckValue(1307, CViewObjectsExt::BuildingBrushDlgBF->CString_Spotlight, data.SpotLight) &&
                CheckValue(1308, CViewObjectsExt::BuildingBrushDlgBF->CString_Upgrade1, data.Upgrade1) &&
                CheckValue(1309, CViewObjectsExt::BuildingBrushDlgBF->CString_Upgrade2, data.Upgrade2) &&
                CheckValue(1310, CViewObjectsExt::BuildingBrushDlgBF->CString_Upgrade3, data.Upgrade3) &&
                CheckValue(1311, CViewObjectsExt::BuildingBrushDlgBF->CString_AIRepairs, data.AIRepairable) &&
                CheckValue(1312, CViewObjectsExt::BuildingBrushDlgBF->CString_ShowName, data.Nominal) &&
                CheckValue(1313, CViewObjectsExt::BuildingBrushDlgBF->CString_Tag, data.Tag))
                FilterVisible = true;
        }
    }
    else if (!CIsoViewExt::DrawStructuresFilter)
    {
        FilterVisible = true;
    }

    if (!CIsoViewExt::RenderingMap
        || CIsoViewExt::RenderingMap
        && CIsoViewExt::MapRendererIgnoreObjects.find(GetData()->TypeID)
        == CIsoViewExt::MapRendererIgnoreObjects.end())
        RenderMapVisible = true;

    Visible = FilterVisible && RenderMapVisible;
}

CBuildingDataFS* Renderer::Building::GetData()
{
    return static_cast<CBuildingDataFS*>(pObjectData);
}

bool Renderer::Object::IsVisible()
{
    return Visible;
}

COLORREF Renderer::Object::GetHouseColor()
{
    return HouseColor;
}

void Renderer::Vehicle::Reload(short index)
{
    Visible = false;
    pType = nullptr;

    if (index < 0 || index >= CMapDataExt::UnitDatasExt.size())
    {
        return;
    }

    auto& data = CMapDataExt::GetUnitDadaFsFromMap(index);
    pObjectData = &data;
    pType = GetOrCreateVehicle(data.TypeID);
    HouseColor = Miscs::GetColorRef(data.House);

    if (CLoadingExt::ObjectsNeedReloaded && ExtConfigs::DirectXRendering)
    {
        auto pType = GetType();
        BGRStruct color(HouseColor);
        for (int i = 0; i < pType->FacingCount; ++i)
        {
            auto pData = pType->GetImageData(data, LandType::Water);
            if (ImageDataClassSafe::IsValidImage(pData))
            {  
                auto newPal = PalettesManager::GetColoredPalette(pData->pPalette, color);
                pData->GetColoredTexture(newPal, color);                 
            }
            pData = pType->GetImageData(data, LandType::Clear13);
            if (ImageDataClassSafe::IsValidImage(pData))
            {  
                auto newPal = PalettesManager::GetColoredPalette(pData->pPalette, color);
                pData->GetColoredTexture(newPal, color);                 
            }

            pData = pType->GetShadowData(data, LandType::Water);
            if (ImageDataClassSafe::IsValidImage(pData))
            {                   
                pData->GetTexture();
            }
            pData = pType->GetShadowData(data, LandType::Clear13);
            if (ImageDataClassSafe::IsValidImage(pData))
            {                   
                pData->GetTexture();
            }
        }
    }

    if (!CIsoViewExt::DrawUnits)
        return;

    bool FilterVisible = false;
    bool RenderMapVisible = false;

    if (CIsoViewExt::DrawUnitsFilter && CViewObjectsExt::VehicleBrushDlgF)
    {
        auto CheckValue = [&](int nCheckBoxIdx, const ppmfc::CString& src, const ppmfc::CString& dst)
        {
            if (CViewObjectsExt::VehicleBrushBoolsF[nCheckBoxIdx - 1300])
            {
                if (dst == src) return true;
                else return false;
            }
            return true;
        };
        const auto& filter = CViewObjectsExt::ObjectFilterV;
        if (filter.empty() || std::find(filter.begin(), filter.end(), data.TypeID) != filter.end())
        {
            if (CheckValue(1300, CViewObjectsExt::VehicleBrushDlgF->CString_House, data.House) &&
                CheckValue(1301, CViewObjectsExt::VehicleBrushDlgF->CString_HealthPoint, data.Health) &&
                CheckValue(1302, CViewObjectsExt::VehicleBrushDlgF->CString_State, data.Status) &&
                CheckValue(1303, CViewObjectsExt::VehicleBrushDlgF->CString_Direction, data.Facing) &&
                CheckValue(1304, CViewObjectsExt::VehicleBrushDlgF->CString_VeteranLevel, data.VeterancyPercentage) &&
                CheckValue(1305, CViewObjectsExt::VehicleBrushDlgF->CString_Group, data.Group) &&
                CheckValue(1306, CViewObjectsExt::VehicleBrushDlgF->CString_OnBridge, data.IsAboveGround) &&
                CheckValue(1307, CViewObjectsExt::VehicleBrushDlgF->CString_FollowerID, data.FollowsIndex) &&
                CheckValue(1308, CViewObjectsExt::VehicleBrushDlgF->CString_AutoCreateNoRecruitable, data.AutoNORecruitType) &&
                CheckValue(1309, CViewObjectsExt::VehicleBrushDlgF->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType) &&
                CheckValue(1310, CViewObjectsExt::VehicleBrushDlgF->CString_Tag, data.Tag))
                FilterVisible = true;
        }
    }
    else if (!CIsoViewExt::DrawUnitsFilter)
    {
        FilterVisible = true;
    }

    if (!CIsoViewExt::RenderingMap
        || CIsoViewExt::RenderingMap
        && CIsoViewExt::MapRendererIgnoreObjects.find(GetData()->TypeID)
        == CIsoViewExt::MapRendererIgnoreObjects.end())
        RenderMapVisible = true;

    Visible = FilterVisible && RenderMapVisible;
}

CUnitDataFS* Renderer::Vehicle::GetData()
{
    return static_cast<CUnitDataFS*>(pObjectData);
}

void Renderer::Infantry::Reload(short index)
{
    Visible = false;
    pType = nullptr;

    if (index < 0 || index >= CMapDataExt::Instance->InfantryDatas.size())
    {
        return;
    }

    auto& data = CMapDataExt::GetInfantryDataFromMap(index);
    pObjectData = &data;
    pType = GetOrCreateInfantry(data.TypeID);
    HouseColor = Miscs::GetColorRef(data.House);

    if (CLoadingExt::ObjectsNeedReloaded && ExtConfigs::DirectXRendering)
    {
        auto pType = GetType();
        BGRStruct color(HouseColor);
        for (int i = 0; i < pType->FacingCount; ++i)
        {
            auto pData = pType->GetImageData(data, LandType::Water);
            if (ImageDataClassSafe::IsValidImage(pData))
            {  
                auto newPal = PalettesManager::GetColoredPalette(pData->pPalette, color);
                pData->GetColoredTexture(newPal, color);                 
            }
            pData = pType->GetImageData(data, LandType::Clear13);
            if (ImageDataClassSafe::IsValidImage(pData))
            {  
                auto newPal = PalettesManager::GetColoredPalette(pData->pPalette, color);
                pData->GetColoredTexture(newPal, color);                 
            }

            pData = pType->GetShadowData(data, LandType::Water);
            if (ImageDataClassSafe::IsValidImage(pData))
            {                   
                pData->GetTexture();
            }
            pData = pType->GetShadowData(data, LandType::Clear13);
            if (ImageDataClassSafe::IsValidImage(pData))
            {                   
                pData->GetTexture();
            }
        }
    }

    if (!CIsoViewExt::DrawInfantries)
        return;

    bool FilterVisible = false;
    bool RenderMapVisible = false;
    if (CIsoViewExt::DrawInfantriesFilter && CViewObjectsExt::InfantryBrushDlgF)
    {
        auto CheckValue = [&](int nCheckBoxIdx, const ppmfc::CString& src, const ppmfc::CString& dst)
        {
            if (CViewObjectsExt::InfantryBrushBoolsF[nCheckBoxIdx - 1300])
            {
                if (dst == src) return true;
                else return false;
            }
            return true;
        };

        const auto& filter = CViewObjectsExt::ObjectFilterI;
        if (filter.empty() || std::find(filter.begin(), filter.end(), data.TypeID) != filter.end())
        {
            if (CheckValue(1300, CViewObjectsExt::InfantryBrushDlgF->CString_House, data.House) &&
                CheckValue(1301, CViewObjectsExt::InfantryBrushDlgF->CString_HealthPoint, data.Health) &&
                CheckValue(1302, CViewObjectsExt::InfantryBrushDlgF->CString_State, data.Status) &&
                CheckValue(1303, CViewObjectsExt::InfantryBrushDlgF->CString_Direction, data.Facing) &&
                CheckValue(1304, CViewObjectsExt::InfantryBrushDlgF->CString_VerteranStatus, data.VeterancyPercentage) &&
                CheckValue(1305, CViewObjectsExt::InfantryBrushDlgF->CString_Group, data.Group) &&
                CheckValue(1306, CViewObjectsExt::InfantryBrushDlgF->CString_OnBridge, data.IsAboveGround) &&
                CheckValue(1307, CViewObjectsExt::InfantryBrushDlgF->CString_AutoCreateNoRecruitable, data.AutoNORecruitType) &&
                CheckValue(1308, CViewObjectsExt::InfantryBrushDlgF->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType) &&
                CheckValue(1309, CViewObjectsExt::InfantryBrushDlgF->CString_Tag, data.Tag))
                FilterVisible = true;
        }
    }
    else if (!CIsoViewExt::DrawInfantriesFilter)
    {
        FilterVisible = true;
    }

    if (!CIsoViewExt::RenderingMap
        || CIsoViewExt::RenderingMap
        && CIsoViewExt::MapRendererIgnoreObjects.find(GetData()->TypeID)
        == CIsoViewExt::MapRendererIgnoreObjects.end())
        RenderMapVisible = true;

    Visible = FilterVisible && RenderMapVisible;
}

CInfantryData* Renderer::Infantry::GetData()
{
    return static_cast<CInfantryData*>(pObjectData);
}

void Renderer::Infantry::OffsetInfantrySubcell(int& x, int& y)
{
    switch (atoi(GetData()->SubCell))
    {
    case 2:
        x += 15;
        y += 15;
        break;
    case 3:
        x -= 15;
        y += 15;
        break;
    case 4:
        y += 22;
        break;
    default:
        y += 15;
        break;
    }
}

void Renderer::Aircraft::Reload(short index)
{
    Visible = false;
    pType = nullptr;

    if (index < 0 || index >= CMapDataExt::AircraftDatasExt.size())
    {
        return;
    }

    auto& data = CMapDataExt::GetAircraftDataFsFromMap(index);
    pObjectData = &data;
    pType = GetOrCreateAircraft(data.TypeID);
    HouseColor = Miscs::GetColorRef(data.House);

    if (CLoadingExt::ObjectsNeedReloaded && ExtConfigs::DirectXRendering)
    {
        auto pType = GetType();
        BGRStruct color(HouseColor);
        for (int i = 0; i < pType->FacingCount; ++i)
        {
            auto pData = pType->GetImageData(data);
            if (ImageDataClassSafe::IsValidImage(pData))
            {  
                auto newPal = PalettesManager::GetColoredPalette(pData->pPalette, color);
                pData->GetColoredTexture(newPal, color);                 
            }
        }
    }

    if (!CIsoViewExt::DrawAircrafts)
        return;

    bool FilterVisible = false;
    bool RenderMapVisible = false;
    if (CIsoViewExt::DrawAircraftsFilter && CViewObjectsExt::AircraftBrushDlgF)
    {
        auto CheckValue = [&](int nCheckBoxIdx, const ppmfc::CString& src, const ppmfc::CString& dst)
        {
            if (CViewObjectsExt::AircraftBrushBoolsF[nCheckBoxIdx - 1300])
            {
                if (dst == src) return true;
                else return false;
            }
            return true;
        };
        const auto& filter = CViewObjectsExt::ObjectFilterA;
        if (filter.empty() || std::find(filter.begin(), filter.end(), data.TypeID) != filter.end())
        {
            if (CheckValue(1300, CViewObjectsExt::AircraftBrushDlgF->CString_House, data.House) &&
                CheckValue(1301, CViewObjectsExt::AircraftBrushDlgF->CString_HealthPoint, data.Health) &&
                CheckValue(1302, CViewObjectsExt::AircraftBrushDlgF->CString_Direction, data.Facing) &&
                CheckValue(1303, CViewObjectsExt::AircraftBrushDlgF->CString_Status, data.Status) &&
                CheckValue(1304, CViewObjectsExt::AircraftBrushDlgF->CString_VeteranLevel, data.VeterancyPercentage) &&
                CheckValue(1305, CViewObjectsExt::AircraftBrushDlgF->CString_Group, data.Group) &&
                CheckValue(1306, CViewObjectsExt::AircraftBrushDlgF->CString_AutoCreateNoRecruitable, data.AutoNORecruitType) &&
                CheckValue(1307, CViewObjectsExt::AircraftBrushDlgF->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType) &&
                CheckValue(1308, CViewObjectsExt::AircraftBrushDlgF->CString_Tag, data.Tag))
                FilterVisible = true;
        }
    }
    else if (!CIsoViewExt::DrawAircraftsFilter)
    {
        FilterVisible = true;
    }

    if (!CIsoViewExt::RenderingMap
        || CIsoViewExt::RenderingMap
        && CIsoViewExt::MapRendererIgnoreObjects.find(GetData()->TypeID)
        == CIsoViewExt::MapRendererIgnoreObjects.end())
        RenderMapVisible = true;

    Visible = FilterVisible && RenderMapVisible;
}

CAircraftDataFS* Renderer::Aircraft::GetData()
{
    return static_cast<CAircraftDataFS*>(pObjectData);
}

VehicleType* Renderer::Vehicle::GetType()
{
    return static_cast<VehicleType*>(pType);
}

InfantryType* Renderer::Infantry::GetType()
{
    return static_cast<InfantryType*>(pType);
}

AircraftType* Renderer::Aircraft::GetType()
{
    return static_cast<AircraftType*>(pType);
}

BuildingType* Renderer::Building::GetType()
{
    return static_cast<BuildingType*>(pType);
}

BuildingRenderData* Renderer::Building::GetRender()
{
    return pRenderData;
}

CellData * Renderer::Building::GetCellData()
{
    return pCellData;
}

bool Renderer::ObjectType::IsVisibleInMapRendererOrNormal() const
{
    return !CIsoViewExt::RenderingMap
        || CIsoViewExt::RenderingMap
        && CIsoViewExt::MapRendererIgnoreObjects.find(ID)
        == CIsoViewExt::MapRendererIgnoreObjects.end();
}
