#include <Helpers/Macro.h>
#include <CCreateMap3A.h>
#include <CLoading.h>

#include "../CMapData/Body.h"

#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/TheaterHelpers.h"

DEFINE_HOOK(4D2E80, CCreateMap3A_OnInitDialog, 5)
{
    GET(CCreateMap3A*, pThis, ECX);

    pThis->ppmfc::CDialog::OnInitDialog();
    auto pCBTheater = (ppmfc::CComboBox*)pThis->GetDlgItem(1046);

    for (auto& str : TheaterHelpers::GetEnabledTheaterNames())
    {
        pCBTheater->AddString(TheaterHelpers::GetTranslatedName(str));
    }

    pThis->TheaterIndex = 0;
    pThis->DefaultHeight = 0;
    pThis->MapWidth = pThis->MapHeight = 80;

    pThis->ppmfc::CDialog::UpdateData(false);

    R->AL(true);

    return 0x4D2F3D;
}

DEFINE_HOOK(4D2F40, CCreateMap3A_OnOK, 5)
{
    GET(CCreateMap3A*, pThis, ECX);

    pThis->UpdateData();

    constexpr int maxWidthPlusHeight = CMapDataExt::MAX_MAP_WIDTH_PLUS_HEIGHT;
    constexpr int maxSide = maxWidthPlusHeight - 1;

    ppmfc::CString pMessage = Translations::TranslateOrDefault("CreateMap.SizeLimit1",
        "The width and height of the map must be between 1 and 2047.");
    ppmfc::CString pMessage2 = Translations::TranslateOrDefault("CreateMap.SizeLimit2",
        "Map width plus height cannot exceed 2048.");

    if (pThis->MapWidth > maxSide || pThis->MapWidth < 1 || pThis->MapHeight > maxSide || pThis->MapHeight < 1)
        ::MessageBox(NULL, pMessage, Translations::TranslateOrDefault("Error", "Error"), MB_OK);
    else if (pThis->MapWidth + pThis->MapHeight > maxWidthPlusHeight)
        ::MessageBox(NULL, pMessage2, Translations::TranslateOrDefault("Error", "Error"), MB_OK);
    else
        pThis->ppmfc::CDialog::OnOK();

    return 0x4D2FA7;
}