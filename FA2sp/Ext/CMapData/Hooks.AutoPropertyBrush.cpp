#include <Helpers/Macro.h>

#include "../CIsoView/Body.h"
#include "../CFinalSunDlg/Body.h"
#include "../../FA2sp.h"

DEFINE_HOOK(4B026A, CMapData_SetAircraftData_AutoPropertyBrush, 5)
{
	REF_STACK(CAircraftData, data, STACK_OFFS(0x160, 0x150));
	GET_STACK(CAircraftData*, pData, STACK_OFFS(0x160, -0x4));

	if (!pData)
	{
		data.Health = ExtConfigs::DefaultAircraftProperty.Health;
		data.Facing = ExtConfigs::DefaultAircraftProperty.Facing;
		data.Status = ExtConfigs::DefaultAircraftProperty.Status;
		data.Tag = ExtConfigs::DefaultAircraftProperty.Tag;
		data.VeterancyPercentage = ExtConfigs::DefaultAircraftProperty.VeterancyPercentage;
		data.Group = ExtConfigs::DefaultAircraftProperty.Group;
		data.AutoNORecruitType = ExtConfigs::DefaultAircraftProperty.AutoNORecruitType;
		data.AutoYESRecruitType = ExtConfigs::DefaultAircraftProperty.AutoYESRecruitType;
	}
	if (!(CIsoView::CurrentCommand->Command == 1 && CIsoView::CurrentCommand->Type == 7)
		&& !CIsoView::GetInstance()->Drag
		&& CIsoView::CurrentCommand->Command != 21)
	{
		if (CViewObjectsExt::PlacingRandomAircraft < 0 && CIsoView::CurrentCommand->Command == 1 && CIsoView::CurrentCommand->Type == 3)
			data.Facing.Format("%d", CIsoViewExt::AutoPropertyBrushFacing[0]);
		if (CIsoViewExt::AutoPropertyBrush[0])
			CViewObjectsExt::ApplyPropertyBrush_Aircraft(data);
	}

	return 0;
}

DEFINE_HOOK(4B0D7B, CMapData_SetUnitData_AutoPropertyBrush, 5)
{
	REF_STACK(CUnitData, data, STACK_OFFS(0x178, 0x168));
	GET_STACK(CUnitData*, pData, STACK_OFFS(0x178, -0x4));
	if (!pData)
	{
		data.Health = ExtConfigs::DefaultUnitProperty.Health;
		data.Facing = ExtConfigs::DefaultUnitProperty.Facing;
		data.Status = ExtConfigs::DefaultUnitProperty.Status;
		data.Tag = ExtConfigs::DefaultUnitProperty.Tag;
		data.VeterancyPercentage = ExtConfigs::DefaultUnitProperty.VeterancyPercentage;
		data.Group = ExtConfigs::DefaultUnitProperty.Group;
		data.IsAboveGround = ExtConfigs::DefaultUnitProperty.IsAboveGround;
		data.FollowsIndex = ExtConfigs::DefaultUnitProperty.FollowsIndex;
		data.AutoNORecruitType = ExtConfigs::DefaultUnitProperty.AutoNORecruitType;
		data.AutoYESRecruitType = ExtConfigs::DefaultUnitProperty.AutoYESRecruitType;
	}
	if (!(CIsoView::CurrentCommand->Command == 1 && CIsoView::CurrentCommand->Type == 7)
		&& !CIsoView::GetInstance()->Drag
		&& CIsoView::CurrentCommand->Command != 21)
	{
		if (CViewObjectsExt::PlacingRandomVehicle < 0 && CIsoView::CurrentCommand->Command == 1 && CIsoView::CurrentCommand->Type == 4)
			data.Facing.Format("%d", CIsoViewExt::AutoPropertyBrushFacing[3]);
		if (CIsoViewExt::AutoPropertyBrush[3])
			CViewObjectsExt::ApplyPropertyBrush_Vehicle(data);
	}
	return 0;
}
