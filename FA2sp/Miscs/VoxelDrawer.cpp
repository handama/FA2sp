#include "VoxelDrawer.h"

#include <CLoading.h>

void VoxelDrawer::Initalize()
{
    CncImgCreate();
    CncImgSetMaxFacing(32);
    CncImgSetLightingSource(ExtConfigs::LightingSource[0], ExtConfigs::LightingSource[1], ExtConfigs::LightingSource[2]);
}

void VoxelDrawer::Finalize()
{
    CncImgRelease();
}

bool VoxelDrawer::LoadVPLFile(FString name)
{
    bool result = false;
    DWORD dwSize;
    if (auto pBuffer = (unsigned char*)CLoading::Instance->ReadWholeFile(name, &dwSize))
    {
        result = CncImgLoadVPLFile(pBuffer);
        GameDeleteArray(pBuffer, dwSize);
    }
    return result;
}

bool VoxelDrawer::LoadVXLFile(FString name)
{
    bool result = false;
    DWORD dwSize;
    if (auto pBuffer = (unsigned char*)CLoading::Instance->ReadWholeFile(name, &dwSize))
    {
        if (CncImgIsVXLLoaded())
            CncImgClearCurrentVXL();
        result = CncImgLoadVXLFile(pBuffer);
        GameDeleteArray(pBuffer, dwSize);
    }
    return result;
}

bool VoxelDrawer::LoadHVAFile(FString name)
{
    bool result = false;
    DWORD dwSize;
    if (auto pBuffer = (unsigned char*)CLoading::Instance->ReadWholeFile(name, &dwSize))
    {
        result = CncImgLoadHVAFile(pBuffer);
        GameDeleteArray(pBuffer, dwSize);
    }
    return result;
}

bool VoxelDrawer::GetImageData(unsigned int nFacing, unsigned char*& pBuffer, int& width,
    int& height, int& x, int& y, const int F, const int L, const int H, bool Shadow, int fireAngle, bool mainBody)
{
    const unsigned int nIndex = ExtConfigs::ExtFacings ? nFacing : nFacing * 4;
    CncImgPrepareVXLCache(nIndex, F, L, H, fireAngle, mainBody && !ExtConfigs::InGameDisplay_Shadow_OnGround);
    if (Shadow)
        CncImgGetShadowImageFrame(nIndex, &width, &height, &x, &y);
    else
        CncImgGetImageFrame(nIndex, &width, &height, &x, &y);
    if (width < 0 || height < 0)
        return false;
    if (Shadow)
        return CncImgGetShadowImageData(nIndex, &pBuffer);
    return CncImgGetImageData(nIndex, &pBuffer);
}

bool VoxelDrawer::GetImageData(unsigned int nFacing, unsigned char*& pBuffer, VoxelRectangle& rect,
    const int F, const int L, const int H, bool Shadow, int fireAngle, bool mainBody)
{
    return GetImageData(nFacing, pBuffer, rect.W, rect.H, rect.X, rect.Y, F, L, H, Shadow, fireAngle, mainBody);
}

bool VoxelDrawer::IsVPLLoaded()
{
    return CncImgIsVPLLoaded();
}
