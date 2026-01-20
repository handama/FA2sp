#include "Body.h"

#include <Helpers/Macro.h>

#include <CMapData.h>
#include <CObjectDatas.h>
#include <CINI.h>
#include <Drawing.h>
#include <CFinalSunApp.h>
#include <CFinalSunDlg.h>
#include <CMixFile.h>
#include <CIsoView.h>
#include "../../FA2sp.h"
#include "../../Helpers/STDHelpers.h"
#include <filesystem>
#include <fstream>
#include "../../Helpers/Translations.h"
#include "../CFinalSunDlg/Body.h"
#include "../../ExtraWindow/CCsfEditor/CCsfEditor.h"
#include "../CMapData/Body.h"
#include "../../Miscs/Hooks.INI.h"

DEFINE_HOOK(4808A0, CLoading_LoadObjects, 5)
{
    GET(CLoadingExt*, pThis, ECX);
    REF_STACK(ppmfc::CString, pRegName, 0x4);

    pThis->CLoadingExt::LoadObjects(pRegName);

    return 0x486173;
}

DEFINE_HOOK(475450, GetObjectName, 7)
{
    GET_STACK(ppmfc::CString*, pRet, 0x4);
    GET_STACK(LPCSTR, ID, 0x8);
    GET_STACK(int, nFacing, 0xC);
    GET_STACK(DWORD, dwCaller, 0x0);

    const ppmfc::CString temp = CLoadingExt::GetImageName(ID, nFacing);

    switch (dwCaller)
    {
    case 0x473E63: // Infantry
    case 0x4737DA: // Aircraft
    case 0x4731AD: // Unit
    case 0x4730EA:
    case 0x4B5DF6: // Building
    case 0x4B607D:
    case 0x4B67A0:
    case 0x4B67C3:
    case 0x4B6C02:
    case 0x4B6C30:
    case 0x470EA1: // Powerups
    case 0x471036:
    case 0x4716C9:
    case 0x47187A:
    case 0x471E64:
    case 0x471FCD:
        new(pRet) ppmfc::CString(temp);
        R->EAX(pRet);
        return 0x47623D;
    default:
        return 0;
    }
}

DEFINE_HOOK_AGAIN(471028, CLoading_Draw_PowerUpFacingFix, 7)
DEFINE_HOOK_AGAIN(470E93, CLoading_Draw_PowerUpFacingFix, 7)
DEFINE_HOOK_AGAIN(471FBF, CLoading_Draw_PowerUpFacingFix, 7)
DEFINE_HOOK_AGAIN(471E56, CLoading_Draw_PowerUpFacingFix, 7)
DEFINE_HOOK_AGAIN(47186C, CLoading_Draw_PowerUpFacingFix, 7)
DEFINE_HOOK(4716BB, CLoading_Draw_PowerUpFacingFix, 7)
{
    R->Stack(0, 0);
    return 0;
}

DEFINE_HOOK(4B85D0, CFinalSunDlg_CreateMap_ClearCLoadingExtData, 5)
{
    CLoadingExt::ClearItemTypes();
    return 0;
}

DEFINE_HOOK(42CBFC, CFinalSunDlg_CreateMap_ClearCLoadingExtData2, 8)
{
    CLoadingExt::ClearItemTypes();
    return 0;
}

DEFINE_HOOK(49D2C0, CMapData_LoadMap_ClearCLoadingExtData, 5)
{
    CLoadingExt::ClearItemTypes();
    return 0;
}

DEFINE_HOOK(49D63A, CLoading_LoadMap_ReloadGame, 5)
{
    GET(const char*, mapPath, EDI);

    Logger::Debug("CLoading::LoadMap(): Loading %s\n", mapPath);
    CViewObjectsExt::InitializeOnUpdateEngine();
    CIsoView::CurrentCommand->Command = 0;

    if (ExtConfigs::ReloadGameFromMapFolder)
    {
        std::string buffer = std::string(mapPath);
        buffer = buffer.substr(0, buffer.find_last_of("\\") + 1);
        FString folder = buffer.c_str();

        std::string buffer2 = std::string(CFinalSunApp::FilePath);
        std::transform(buffer.begin(), buffer.end(), buffer.begin(), (int(*)(int))tolower);
        std::transform(buffer2.begin(), buffer2.end(), buffer2.begin(), (int(*)(int))tolower);

        if (buffer != buffer2)
        {
            if (std::filesystem::exists(buffer + "ra2.mix"))
            {
                FString pMessage = Translations::TranslateOrDefault("LoadFromOtherGameDirectory",
                    "You seem to have read a map from another game directory.\nClick 'OK' to reload the resources in this directory.");

                int text = 0;
                if (CIsoViewExt::RenderingMap)
                    text = IDOK;
                else
                    text = MessageBox(CFinalSunDlg::Instance->m_hWnd, pMessage, "FA2sp", MB_OKCANCEL | MB_ICONEXCLAMATION);

                if (text == IDOK)
                {
                        for (int i = 0; i < 2000; i++)
                            CMixFile::Close(i);

                        //reload all INIs
                        CINI::Rules().Release();
                        CINI::Art().Release();
                        //CINI::Turtorial().Release(); what's this
                        CINI::Sound().Release();
                        CINI::Eva().Release();
                        CINI::Theme().Release();
                        CINI::Ai().Release();
                        CINI::Temperate().Release();
                        CINI::Snow().Release();
                        CINI::Urban().Release();
                        CINI::NewUrban().Release();
                        CINI::Lunar().Release();
                        CINI::Desert().Release();
                        //CINI::FAData().Release();
                        //CINI::FALanguage().Release();
                        CINI::CurrentDocument().Release();

                        CLoading::Instance()->Release();
                        strcpy_s(CFinalSunApp::FilePath, 260, folder);

                        CLoadingExt::g_cache[0].clear();
                        CLoadingExt::g_cache[1].clear();
                        CLoadingExt::g_cacheTime[0].clear();
                        CLoadingExt::g_cacheTime[1].clear();
                        CLoadingExt::g_lastCleanup = 0;

                        Logger::Debug("CLoading::Load() Called. Reload all files from %s\n", folder);
                        CLoading::Instance()->Load();
                        CCsfEditor::NeedUpdate = true;
                }
            }
        }
    }

    INIIncludes::SkipBracketFix = true;
    CMapDataExt::IsLoadingMapFile = true;
    CMapDataExt::MapIniSectionSorting.clear();
    CMapDataExt::MapInlineComments.clear();
    CMapDataExt::MapFrontlineComments.clear();
    CMapDataExt::MapInsectionComments.clear();
    CMapDataExt::MapFrontsectionComments.clear();

    return 0;
}

// some maps use '[' for encryption
DEFINE_HOOK(49D64F, CMapData_LoadMap_SkipBracketFix, 6)
{
    INIIncludes::SkipBracketFix = false;
    CMapDataExt::IsLoadingMapFile = false;
    return 0;
}

DEFINE_HOOK(49D5CC, CLoading_LoadMap_CallMissingRelease, 5)
{
    GET(CLoading*, pThis, ESI);

    pThis->Release();
    pThis->InitializeDDraw();

    return 0;
}

DEFINE_HOOK(491FD4, CLoading_Release_SetImageDataToNullptr, 5)
{
    GET(char*, pNode, ESI); // Map node in fact
    ImageDataClass* pData = (ImageDataClass*)(pNode + 0xC + 0x4); // data = pNode->_Value.second
    
    if (pData->pImageBuffer)
    {
        GameDelete(pData->pImageBuffer);
        pData->pImageBuffer = nullptr;
    }
    if (pData->pPixelValidRanges)
    {
        GameDelete(pData->pPixelValidRanges);
        pData->pPixelValidRanges = nullptr;
    }

    return 0x491FF1;
}

DEFINE_HOOK(491D00, CLoading_Release_BackBufferZoom, 5)
{
    CLoadingExt::ClearItemTypes(!CIsoViewExt::ReInitializingDDraw);
    if (CIsoViewExt::lpDDBackBufferZoomSurface != NULL) CIsoViewExt::lpDDBackBufferZoomSurface->Release();
    return 0;
}

DEFINE_HOOK(48DBB0, CLoading_InitTMPs_ReadFolder, E)
{
    GET(int, nMIx, EAX);
    GET_STACK(const char*, lpFilename, STACK_OFFS(0x59C, 0x588));

    if (nMIx)
        return 0x48DC52;

    int result = CLoadingExt::GetExtension()->HasFileMix(lpFilename);
    if (result > -2)
        return 0x48DC52;

    if (CINI::CurrentTheater == &CINI::NewUrban)
        return 0x48DBC0;

    return 0x48DC03;
}

DEFINE_HOOK(48E5C5, CLoading_LoadTile_ReadFolder, 8)
{
    GET(LPCSTR, lpFilename, ESI);
    GET(unsigned int, nMix, EDI);
    GET(BOOL, oriResult, EAX);

    int result = CLoadingExt::GetExtension()->HasFileMix(lpFilename);
    if (result > -2)
    {
        R->EDI(0);
        return 0x48E5CD;
    }

    if (oriResult)
    {
        return 0x48E5CD;
    }

    return 0x48EDC8;
}

DEFINE_HOOK(525AF8, CLoading_SetCurrentTMP_ReadGameFolder, 8)
{
    GET_STACK(LPCSTR, lpFilename, STACK_OFFS(0x20, -0x4));
    DWORD dwSize;
    if (auto pBuffer = (unsigned char*)CLoadingExt::GetExtension()->ReadWholeFile(lpFilename, &dwSize))
    {
        CLoading::Instance->CurrentTMP->open(pBuffer, dwSize);
        return 0x525B77;
    }
    return 0;
}

DEFINE_HOOK(48EE60, CLoading_LoadOverlayGraphic, 7)
{
    GET(CLoadingExt*, pThis, ECX); 
    GET_STACK(LPCSTR, lpOvrlName, 0x4);
    GET_STACK(int, iOvrlNum, 0x8);

    pThis->CLoadingExt::LoadOverlay(lpOvrlName, iOvrlNum);

    return 0x4909FE;
}

