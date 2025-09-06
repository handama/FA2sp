#include "Body.h"

#include <Helpers/Macro.h>
#include "../../Helpers/Translations.h"
#include <CMapData.h>
#include <CFinalSunDlg.h>
#include <CMyViewFrame.h>
#include <CIsoView.h>
#include <CMinimap.h>
#include "../../Miscs/MultiSelection.h"

DEFINE_HOOK(41B0E0, CChangeMapSize_DoDataExchange, 6)
{
    GET(CChangeMapSize*, pThis, ECX);
    GET_STACK(ppmfc::CDataExchange*, pDX, 0x4);

	auto setDataValue = [&pDX](int index, int& value)
		{
            if (pDX->m_bSaveAndValidate)
            {
                ppmfc::CString strTemp;
                ppmfc::DDX_Text(pDX, index, strTemp);
    
                if (!strTemp.IsEmpty())
                {
                    value = atoi(strTemp);
                }
                else
                {
                    value = 0;
                }
            }
            else
            {
                ppmfc::CString strTemp;
                strTemp.Format("%d", value);
                ppmfc::DDX_Text(pDX, index, strTemp);
            }
		};
    
    setDataValue(1036, pThis->INT_Left);
    setDataValue(1037, pThis->INT_Top);
    setDataValue(1373, pThis->INT_Width);
    setDataValue(1374, pThis->INT_Height);

    return 0x41B148;
}

DEFINE_HOOK(499D56, CMapD_OnBNChangeMapSizeClicked_ValidCheck, 7)
{
    GET(CMapD*, pThis, ESI);
    REF_STACK(CChangeMapSize, cms, STACK_OFFS(0xB0, 0xAC));

	if (!CMapData::Instance->MapWidthPlusHeight) return 0x499E30;

    // remove warning since this function works normally
	//if (R->ESI<ppmfc::CDialog*>()->MessageBox(
	//	Translations::TranslateOrDefault("ChangeMapSizeWarn", 
	//		"It is highly recommended that you should use Starkku's MapTool to resize your map rather than"
	//		"Use FinalAlert2's map resizing, are you sure you want to continue?"),
	//	Translations::TranslateOrDefault("Warning", "Warning"),
	//	MB_YESNO | MB_ICONINFORMATION
	//) == IDYES)
	{
		CMapData::Instance->ResizeMap(
			cms.INT_Left,
			cms.INT_Top,
			cms.INT_Width,
			cms.INT_Height
		);
		CFinalSunDlg::Instance->MyViewFrame.pIsoView->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
		CFinalSunDlg::Instance->MyViewFrame.Minimap.Update();
		pThis->CString_Width.Format("%d", cms.INT_Width);
		pThis->CString_Height.Format("%d", cms.INT_Height);
		pThis->UpdateData(FALSE);
		pThis->Update();
        MultiSelection::Clear();
	}

    return 0x499E30;
}

DEFINE_HOOK(499CF7, CMapD_OnBNChangeMapLocalSizeClicked, 7)
{
    CFinalSunDlg::Instance->MyViewFrame.Minimap.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    return 0;
}