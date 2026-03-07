#include <Helpers/Macro.h>
#include <CCreateMap3B.h>
#include <CLoading.h>

#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/TheaterHelpers.h"
#include "../CMapData/Body.h"

DEFINE_HOOK(42CC84, CCreateMap3B_LoadMap, 5)
{
    CMapDataExt::IsImportingMap = true;
    return 0;
}
DEFINE_HOOK(42CCC8, CCreateMap3B_NoImportTrees, 5)
{
    for (int i = 0; i < CMapData::Instance->TerrainDatas.size(); ++i)
    {
        CMapData::Instance->DeleteTerrainData(i);
    }
    return 0x42CCDC;
}

DEFINE_HOOK(42CCE2, CCreateMap3B_NoImportUnits, 5)
{
    CINI::CurrentDocument->DeleteSection("Structures");
    CINI::CurrentDocument->DeleteSection("Units");
    CINI::CurrentDocument->DeleteSection("Aircraft");
    CINI::CurrentDocument->DeleteSection("Infantry");
    CMapDataExt::UpdateFieldStructureData_RedrawMinimap();
    CMapDataExt::UpdateFieldUnitData_RedrawMinimap();
    CMapDataExt::UpdateFieldAircraftData_RedrawMinimap();
    CMapDataExt::UpdateFieldInfantryData_RedrawMinimap();

    return 0x42CD5A;
}

