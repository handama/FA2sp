#include "Body.h"

#include <Helpers/Macro.h>

#include "../../FA2sp.h"

#include <CFinalSunDlg.h>

DEFINE_HOOK(4B5460, CMapData_InitializeBuildingTypes, 7)
{
	GET(CMapDataExt*, pThis, ECX);
	GET_STACK(const char*, ID, 0x4);

	pThis->UpdateTypeDatas();
	if (ID)
		pThis->ProcessBuildingType(ID);

	return 0x4B6CF3;
}

DEFINE_HOOK(4A5089, CMapData_UpdateMapFieldData_Structures_CustomFoundation, 6)
{
	GET(int, BuildingIndex, ESI);
	GET_STACK(const int, X, STACK_OFFS(0x16C, 0x104));
	GET_STACK(const int, Y, STACK_OFFS(0x16C, 0x94));

	const auto& DataExt = CMapDataExt::BuildingDataExts[BuildingIndex];
	if (!DataExt.IsCustomFoundation())
	{
		for (int dy = 0; dy < DataExt.Width; ++dy)
		{
			for (int dx = 0; dx < DataExt.Height; ++dx)
			{
				const int x = X + dx;
				const int y = Y + dy;
				if (CMapData::Instance->GetCoordIndex(x, y) < CMapData::Instance->CellDataCount)
				{
					auto pCell = CMapData::Instance->GetCellAt(x, y);
					pCell->Structure = R->BX();
					pCell->TypeListIndex = BuildingIndex;
					CMapData::Instance->UpdateMapPreviewAt(x, y);
				}
			}
		}
	}
	else
	{
		for (const auto& block : *DataExt.Foundations)
		{
			const int x = X + block.Y;
			const int y = Y + block.X;
			if (CMapData::Instance->GetCoordIndex(x, y) < CMapData::Instance->CellDataCount)
			{
				auto pCell = CMapData::Instance->GetCellAt(x, y);
				pCell->Structure = R->BX();
				pCell->TypeListIndex = BuildingIndex;
				CMapData::Instance->UpdateMapPreviewAt(x, y);
			}
		}
	}

	return 0x4A57CD;
}