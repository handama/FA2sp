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

void OverlayType::Init(WORD nOverlay)
{
    ID = Variables::RulesMap.GetValueAt("OverlayTypes", nOverlay);
    Type = CLoadingExt::GameObjectType::Overlay;
    OverlayIndex = nOverlay;
    //TypeData = CMapDataExt::GetOverlayTypeData(nOverlay);

    IsImageLoaded = CLoadingExt::IsOverlayLoaded(ID);
    if (!IsImageLoaded)
    {
        CLoadingExt::GetExtension()->LoadOverlay(ID, nOverlay);
        IsImageLoaded = true;
    }

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

        imageName = CLoadingExt::GetImageName(ID, i, true);
        pShadowData[i] = CLoadingExt::GetImageDataFromMap(imageName);
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

    for (int i = 0; i < FacingCount; ++i)
    {
        FString imageName = CLoadingExt::GetImageName(ID, i);
        pImageData[i] = CLoadingExt::GetImageDataFromMap(imageName);
        if (ImageDataClassSafe::IsVisibleImage(pImageData[i]))
            ShouldUseDefaultImage = false;

        imageName = CLoadingExt::GetImageName(ID, i, true);
        pShadowData[i] = CLoadingExt::GetImageDataFromMap(imageName);

        if (IsDeployer)
        {
            imageName = CLoadingExt::GetImageName(ID, i, false, true, false);
            pDeployImageData[i] = CLoadingExt::GetImageDataFromMap(imageName);

            imageName = CLoadingExt::GetImageName(ID, i, true, true, false);
            pDeployShadowData[i] = CLoadingExt::GetImageDataFromMap(imageName);

            if (!ImageDataClassSafe::IsValidImage(pDeployImageData[i]))
                pDeployImageData[i] = pImageData[i];

            if (!ImageDataClassSafe::IsValidImage(pWaterImageData[i]))
                pWaterImageData[i] = pImageData[i];
        }

        if (Swimable)
        {
            imageName = CLoadingExt::GetImageName(ID, i, false, false, true);
            pWaterImageData[i] = CLoadingExt::GetImageDataFromMap(imageName);

            imageName = CLoadingExt::GetImageName(ID, i, true, false, true);
            pWaterShadowData[i] = CLoadingExt::GetImageDataFromMap(imageName);

            if (!ImageDataClassSafe::IsValidImage(pDeployShadowData[i]))
                pDeployShadowData[i] = pShadowData[i];

            if (!ImageDataClassSafe::IsValidImage(pWaterShadowData[i]))
                pWaterShadowData[i] = pShadowData[i];
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
