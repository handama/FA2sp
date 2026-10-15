#include "Body.h"

#include <Helpers/Macro.h>

#include <CMapData.h>
#include "../CMapData/Body.h"
#include "../CFinalSunDlg/Body.h"

DEFINE_HOOK(4CED70, CMapValidator_DoValidator, 7)
{
	GET(CMapValidatorExt*, pThis, ECX);

	int result = pThis->DoValidator();

	R->EAX(result);
	return 0x4D1A2C; // the function's only retn (the /GS + SEH epilogue cannot be reused here)
}

DEFINE_HOOK(4CEA50, CMapValidator_CTOR, 7)
{
	if (auto pSection = CINI::FAData->GetSection("StructureOverlappingCheckIgnores"))
	{
		for (const auto& [key, value] : pSection->GetEntities())
			CMapValidatorExt::StructureOverlappingIgnores.emplace(std::string{value.GetString()});
	}

	return 0;
}

DEFINE_HOOK(426A60, CMapValidator_DTOR, 7)
{
	CMapValidatorExt::StructureOverlappingIgnores.clear();

	return 0;
}

DEFINE_HOOK(4C8200, CMapValidator_IsYRMap_Overlay, 7)
{
	int yroverlay = CINI::FAData->GetInteger("YROverlay", "Begin", 243);
	for (const auto& cellExt : CMapDataExt::CellDataExts)
	{
		if (cellExt.NewOverlay >= yroverlay && cellExt.NewOverlay != 0xFFFF)
		{
			R->EAX(1);
			return 0x4C9C42;
		}
	}
	return 0;
}