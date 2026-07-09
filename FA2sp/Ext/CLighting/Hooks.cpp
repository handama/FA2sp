#include "Body.h"

#include <Helpers/Macro.h>
#include <CINI.h>
#include <CFinalSunDlg.h>
#include <CLoading.h>
#include "../../Miscs/Palettes.h"
#include "../CFinalSunDlg/Body.h"
#include "../CLoading/Body.h"
bool IsUpdatingDialog = false;

DEFINE_HOOK(432577, CFinalSunDlg_UpdateDialog_Translate_CLighting,5)
{
    GET(CFinalSunDlg*, pThis, ESI);

    if (pThis->Lighting.m_hWnd)
        reinterpret_cast<CLightingExt*>(&pThis->Lighting)->Translate();

    return 0;
}

DEFINE_HOOK(477920, CLighting_UpdateDialog_Init, 7)
{
    IsUpdatingDialog = true;

    return 0;
}

DEFINE_HOOK(478407, CLighting_UpdateDialog, 7)
{
    GET(CLighting*, pThis, ESI);

    pThis->SetDlgItemText(1068, CINI::CurrentDocument->GetString("Lighting", "DominatorAmbient", "0.850000"));
    pThis->SetDlgItemText(1069, CINI::CurrentDocument->GetString("Lighting", "DominatorGreen", "0.300000"));
    pThis->SetDlgItemText(1070, CINI::CurrentDocument->GetString("Lighting", "DominatorRed", "0.200000"));
    pThis->SetDlgItemText(1071, CINI::CurrentDocument->GetString("Lighting", "DominatorBlue", "0.000000"));
    pThis->SetDlgItemText(1072, CINI::CurrentDocument->GetString("Lighting", "DominatorLevel", "1.000000"));

    pThis->SetDlgItemText(1073, CINI::CurrentDocument->GetString("Lighting", "Ground", "0.080000"));
    pThis->SetDlgItemText(1074, CINI::CurrentDocument->GetString("Lighting", "IonGround", "0.000000"));
    pThis->SetDlgItemText(1075, CINI::CurrentDocument->GetString("Lighting", "DominatorGround", "0.000000"));

    pThis->SetDlgItemText(1076, CINI::CurrentDocument->GetString("Lighting", "NukeAmbientChangeRate", "1.500000"));
    pThis->SetDlgItemText(1077, CINI::CurrentDocument->GetString("Lighting", "DominatorAmbientChangeRate", "0.000000"));

    IsUpdatingDialog = false;

    return 0;
}

#define UPDATE_LIGHTING(hook_addr, hook_name) \
DEFINE_HOOK(hook_addr,hook_name,7) \
{ \
    if (!IsUpdatingDialog && CFinalSunDlgExt::CurrentLighting != 31000)\
    {\
        LightingStruct::GetCurrentLighting();\
		for (int i = 0; i < CMapData::Instance->MapWidthPlusHeight; i++) {\
            for (int j = 0; j < CMapData::Instance->MapWidthPlusHeight; j++) {\
                CMapData::Instance->UpdateMapPreviewAt(i, j);\
            }\
        }\
        LightingSourceTint::CalculateMapLamps();\
        CFinalSunDlg::Instance()->MyViewFrame.Minimap.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);\
        CFinalSunDlg::Instance()->MyViewFrame.pIsoView->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);\
    }\
    return 0;\
}

UPDATE_LIGHTING(479A07, CLighting_UpdateLighting_IonBlue);
UPDATE_LIGHTING(478F17, CLighting_UpdateLighting_Blue);
UPDATE_LIGHTING(4797D7, CLighting_UpdateLighting_IonGreen);
UPDATE_LIGHTING(478CE7, CLighting_UpdateLighting_Green);
UPDATE_LIGHTING(4795A4, CLighting_UpdateLighting_IonRed);
UPDATE_LIGHTING(478AB7, CLighting_UpdateLighting_Red);
UPDATE_LIGHTING(479377, CLighting_UpdateLighting_IonLevel);
UPDATE_LIGHTING(478887, CLighting_UpdateLighting_Level);
UPDATE_LIGHTING(479147, CLighting_UpdateLighting_IonAmbient);
UPDATE_LIGHTING(478657, CLighting_UpdateLighting_Ambient);
