#pragma once
#include "Body.h"
#include "../CLoading/Body.h"
#include "../CMapData/Body.h"
#include "CTileTypeClass.h"

namespace Renderer
{
    constexpr int FACING_MAX = 32;
    constexpr int INFANTRY_FACING = 8;
    class ObjectType
    {
    public:
        CLoadingExt::GameObjectType Type = CLoadingExt::GameObjectType::Unknown;
        FString ID = "";
        bool IsImageLoaded = false;
        virtual bool IsVisibleInMapRendererOrNormal() const;
    };

    class SmudgeType : public ObjectType
    {
    public:
        SmudgeType() = default;
        void Init(FString_view id);
        ImageDataClassSafe* GetImageData() const;

        static const char* IniSection;

    private:
        ImageDataClassSafe* pImageData = nullptr;
    };

    class TerrainType : public ObjectType
    {
    public:
        bool IsTiberiumTree = false;
        bool HasCustomPalette = false;

        TerrainType() = default;
        void Init(FString_view id);
        ImageDataClassSafe* GetImageData() const;
        ImageDataClassSafe* GetShadowData() const;
        ImageDataClassSafe* GetAlphaImageData() const;

        static const char* IniSection;

    private:
        ImageDataClassSafe* pImageData = nullptr;
        ImageDataClassSafe* pShadowData = nullptr;
        ImageDataClassSafe* pAlphaImageData = nullptr;
    };

    class OverlayType : public ObjectType
    {
    public:
        WORD OverlayIndex = 0;
        OverlayTypeData TypeData{};

        OverlayType() = default;
        void Init(WORD nOverlay);
        ImageDataClassSafe* GetImageData(BYTE nOverlayData) const;
        ImageDataClassSafe* GetShadowData(BYTE nOverlayData) const;
        bool IsBridge() const;
        virtual bool IsVisibleInMapRendererOrNormal() const override;

        static const char* IniSection;

    private:
        ImageDataClassSafe* pImageData[256]{ nullptr };
        ImageDataClassSafe* pShadowData[256]{ nullptr };
    };

    class TechnoType : public ObjectType
    {
    public:
        std::vector<TechnoAttachment>* TechnoAttachmentInfo = nullptr;
        int FacingCount = 1;
        bool Cloakable = false;
        TechnoType() = default;
        void InitTechnoAttachmentInfo();
    };

    class BuildingType : public TechnoType
    {
    public:
        int BuildingIndex = -1;
        bool IsTerrainPalette = false;
        bool LeaveRubble = false;
        bool IsDamagedAsRubble = false;
        bool CanOccupyFire = false;
        bool HasTurret = false;
        bool TurretAnimIsVoxel = false;
        char TechLevel = 0;
        short PowerUp1LocXX = 0;
        short PowerUp1LocYY = 0;
        short PowerUp2LocXX = 0;
        short PowerUp2LocYY = 0;
        short PowerUp3LocXX = 0;
        short PowerUp3LocYY = 0;
        BuildingDataExt* pDataExt = nullptr;

        BuildingType() = default;
        void Init(FString_view id);
        std::vector<std::unique_ptr<ImageDataClassSafe>>* GetImageData(int rawFacing, int status, int forceFacing = -1) const;
        ImageDataClassSafe* GetShadowData(int nFacing, int status) const;
        ImageDataClassSafe* GetAlphaImageData(int rawFacing) const;
        ImageDataClassSafe* GetBundledImageData(int forceFacing);

        static const char* IniSection;

    private:
        std::vector<std::unique_ptr<ImageDataClassSafe>>* pImageData[FACING_MAX]{ nullptr };
        ImageDataClassSafe* pShadowData[FACING_MAX]{ nullptr };
        std::vector<std::unique_ptr<ImageDataClassSafe>>* pDamagedImageData[FACING_MAX]{ nullptr };
        ImageDataClassSafe* pDamagedShadowData[FACING_MAX]{ nullptr };
        std::vector<std::unique_ptr<ImageDataClassSafe>>* pRubbleImageData[FACING_MAX]{ nullptr };
        ImageDataClassSafe* pRubbleShadowData[FACING_MAX]{ nullptr };
        std::vector<std::unique_ptr<ImageDataClassSafe>>* pGarrisonDamagedImageData[FACING_MAX]{ nullptr };
        ImageDataClassSafe* pGarrisonDamagedShadowData[FACING_MAX]{ nullptr };
        ImageDataClassSafe* pAlphaImageData[FACING_MAX]{ nullptr };
        std::unique_ptr<ImageDataClassSafe> pBundledImageData[FACING_MAX]{ nullptr };
    };

    class InfantryType : public TechnoType
    {
    public:
        InfantryType* DefaultImage = nullptr;
        bool IsDeployer = false;
        bool Swimable = false;
        bool ShouldUseDefaultImage = false;

        InfantryType() = default;
        void Init(FString_view id);
        ImageDataClassSafe* GetImageData(const CInfantryData& obj, const LandType landType) const;
        ImageDataClassSafe* GetShadowData(const CInfantryData& obj, const LandType landType) const;
        ImageDataClassSafe* GetTechnoAttachmentImageData(int nFacing, bool bShadow) const;

        static const char* IniSection;

    private:
        ImageDataClassSafe* pImageData[INFANTRY_FACING]{ nullptr };
        ImageDataClassSafe* pShadowData[INFANTRY_FACING]{ nullptr };
        ImageDataClassSafe* pWaterImageData[INFANTRY_FACING]{ nullptr };
        ImageDataClassSafe* pWaterShadowData[INFANTRY_FACING]{ nullptr };
        ImageDataClassSafe* pDeployImageData[INFANTRY_FACING]{ nullptr };
        ImageDataClassSafe* pDeployShadowData[INFANTRY_FACING]{ nullptr };
    };

    class VehicleType : public TechnoType
    {
    public:
        VehicleType* WaterImage = nullptr;
        VehicleType* ConditionYellowImage = nullptr;
        VehicleType* ConditionRedImage = nullptr;
        VehicleType* ConditionYellowWaterImage = nullptr;
        VehicleType* ConditionRedWaterImage = nullptr;
        VehicleType* UnloadingImage = nullptr;
        VehicleType* DefaultImage = nullptr;
        bool IsHoveringUnit = false;
        bool ShouldUseDefaultImage = false;

        VehicleType() = default;
        void Init(FString_view id);
        ImageDataClassSafe* GetImageData(const CUnitDataFS& obj, const LandType landType);
        ImageDataClassSafe* GetShadowData(const CUnitDataFS& obj, const LandType landType);
        ImageDataClassSafe* GetTechnoAttachmentImageData(int nFacing, bool bShadow) const;

        static const char* IniSection;

    private:
        VehicleType* GetAlteredType(const CUnitDataFS& obj, const LandType landType);
        ImageDataClassSafe* pImageData[FACING_MAX]{ nullptr };
        ImageDataClassSafe* pShadowData[FACING_MAX]{ nullptr };
    };

    class AircraftType : public TechnoType
    {
    public:
        AircraftType* ConditionYellowImage = nullptr;
        AircraftType* ConditionRedImage = nullptr;
        AircraftType* DefaultImage = nullptr;
        bool ShouldUseDefaultImage = false;

        AircraftType() = default;
        void Init(FString_view id);
        ImageDataClassSafe* GetImageData(const CAircraftDataFS& obj);
        ImageDataClassSafe* GetTechnoAttachmentImageData(int nFacing) const;

        static const char* IniSection;

    private:
        AircraftType* GetAlteredType(const CAircraftDataFS& obj);
        ImageDataClassSafe* pImageData[FACING_MAX]{ nullptr };
    };

    class Object
    {
    protected:
        bool Visible = false;
        void* pObjectData = nullptr;
        ObjectType* pType = nullptr;
    public:
        bool IsVisible();
    };

    class Building : public Object
    {
    public:
        void Reload(short index);
        CBuildingDataFS* GetData();
        BuildingType* GetType();
        BuildingRenderData* GetRender();
        CellData* GetCellData();

    private:
        BuildingRenderData* pRenderData = nullptr;
        CellData* pCellData = nullptr;
    };

    class Vehicle : public Object
    {
    public:
        void Reload(short index);
        CUnitDataFS* GetData();
        VehicleType* GetType();
    };

    class Infantry : public Object
    {
    public:
        void Reload(short index);
        CInfantryData* GetData();
        void OffsetInfantrySubcell(int& x, int& y);
        InfantryType* GetType();
    };

    class Aircraft : public Object
    {
    public:
        void Reload(short index);
        CAircraftDataFS* GetData();
        AircraftType* GetType();
    };

    inline FHashMap<SmudgeType> SmudgeTypes;
    inline FHashMap<TerrainType> TerrainTypes;
    inline std::unordered_map<WORD, OverlayType> OverlayTypes;
    inline FHashMap<BuildingType> BuildingTypes;
    inline FHashMap<InfantryType> InfantryTypes;
    inline FHashMap<VehicleType> VehicleTypes;
    inline FHashMap<AircraftType> AircraftTypes;

    inline Building Buildings[SHRT_MAX + 1];
    inline Vehicle Vehicles[SHRT_MAX + 1];
    inline Infantry Infantries[SHRT_MAX + 1];
    inline Aircraft Aircrafts[SHRT_MAX + 1];

    SmudgeType* GetOrCreateSmudge(FString_view id);
    TerrainType* GetOrCreateTerrain(FString_view id);
    OverlayType* GetOrCreateOverlay(WORD nOverlay);
    BuildingType* GetOrCreateBuilding(FString_view id);
    VehicleType* GetOrCreateVehicle(FString_view id);
    AircraftType* GetOrCreateAircraft(FString_view id);
    InfantryType* GetOrCreateInfantry(FString_view id);
}

