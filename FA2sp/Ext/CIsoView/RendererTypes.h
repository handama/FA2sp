#pragma once
#include "Body.h"
#include "../CLoading/Body.h"
#include "../CMapData/Body.h"

namespace Renderer
{
    constexpr int FACING_MAX = 32;
    class ObjectType
    {
    public:
        CLoadingExt::GameObjectType Type = CLoadingExt::GameObjectType::Unknown;
        FString ID = "";
        bool IsImageLoaded = false;
    };

    class SmudgeType : public ObjectType
    {
    public:
        SmudgeType(FString_view id);
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

        TerrainType(FString_view id);
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
        //OverlayTypeData TypeData{};

        OverlayType(WORD nOverlay);
        ImageDataClassSafe* GetImageData(BYTE nOverlayData) const;
        ImageDataClassSafe* GetShadowData(BYTE nOverlayData) const;

        static const char* IniSection;

    private:
        ImageDataClassSafe* pImageData[256]{ nullptr };
        ImageDataClassSafe* pShadowData[256]{ nullptr };
    };

    class BuildingType : public ObjectType
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
        int FacingCount = 1;
        short PowerUp1LocXX = 0;
        short PowerUp1LocYY = 0;
        short PowerUp2LocXX = 0;
        short PowerUp2LocYY = 0;
        short PowerUp3LocXX = 0;
        short PowerUp3LocYY = 0;
        BuildingDataExt* pDataExt = nullptr;

        BuildingType(FString_view id);
        std::vector<std::unique_ptr<ImageDataClassSafe>>* GetImageData(int rawFacing, int status) const;
        ImageDataClassSafe* GetShadowData(int nFacing, int status) const;
        ImageDataClassSafe* GetAlphaImageData(int rawFacing) const;

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
    };
    class InfantryType : public ObjectType
    {

        static const char* IniSection;
    };
    class VehicleType : public ObjectType
    {

        static const char* IniSection;
    };
    class AircraftType : public ObjectType
    {

        static const char* IniSection;
    };

    inline FHashMap<SmudgeType> SmudgeTypes;
    inline FHashMap<TerrainType> TerrainTypes;
    inline std::unordered_map<WORD, OverlayType> OverlayTypes;
    inline FHashMap<BuildingType> BuildingTypes;
    inline FHashMap<InfantryType> InfantryTypes;
    inline FHashMap<VehicleType> VehicleTypes;
    inline FHashMap<AircraftType> AircraftTypes;

    SmudgeType* GetOrCreateSmudge(FString_view id);
    TerrainType* GetOrCreateTerrain(FString_view id);
    OverlayType* GetOrCreateOverlay(WORD nOverlay);
    BuildingType* GetOrCreateBuilding(FString_view id);
}

