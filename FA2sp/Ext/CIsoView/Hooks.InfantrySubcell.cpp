#include "Body.h"
#include "../../FA2sp.h"
#include "../CMapData/Body.h"

DEFINE_HOOK(466E07, CIsoView_OnLButtonUp_DragInfantrySubcell, 5)
{
	if (!ExtConfigs::InfantrySubCell_Edit || !ExtConfigs::InfantrySubCell_Edit_Drag)
		return 0;

	GET_STACK(UINT, nFlags, 0x218);
	GET_STACK(int, screenCoordX, 0x21C);
	GET_STACK(int, screenCoordY, 0x220);
	CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Infantry);
	auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
	CInfantryData inf;
	CMapData::Instance->GetInfantryData(pIsoView->CurrentCellObjectIndex, inf);
	CPoint point(screenCoordX, screenCoordY);
	auto newMapCoord = pIsoView->GetCurrentMapCoord(point);
	inf.X.Format("%d", newMapCoord.X);
	inf.Y.Format("%d", newMapCoord.Y);
	inf.SubCell.Format("%d", CIsoViewExt::GetSelectedSubcellInfantryIdx(newMapCoord.X, newMapCoord.Y, true));
	if (nFlags != MK_SHIFT)
		CMapData::Instance->DeleteInfantryData(pIsoView->CurrentCellObjectIndex);

	CMapData::Instance->SetInfantryData(&inf, nullptr, nullptr, 0, -1);

	return 0x467682;
}

static int GetInfantryCountAt(DWORD dwPos)
{
	int i;
	int sc = 0;
	for (i = 0; i < 3; i++)
	{
		if (CMapData::Instance->CellDatas[dwPos].Infantry[i] > -1) sc++;
	}

	return sc;
}

DEFINE_HOOK(46BD99, CIsoView_PlaceCurrentObjectAt_PlaceInfantrySubcell, 5)
{
	if (!ExtConfigs::InfantrySubCell_Edit || !ExtConfigs::InfantrySubCell_Edit_Place)
		return 0;

	GET_STACK(int, X, 0x98);
	GET_STACK(int, Y, 0x9C);

	if (GetInfantryCountAt(X + Y * CMapData::Instance->MapWidthPlusHeight) >= 3)
		return 0x46D48C;

	CInfantryData infantry;

	infantry.Status = ExtConfigs::DefaultInfantryProperty.Status;
	infantry.Tag = ExtConfigs::DefaultInfantryProperty.Tag;
	infantry.Facing = ExtConfigs::DefaultInfantryProperty.Facing;
	infantry.VeterancyPercentage = ExtConfigs::DefaultInfantryProperty.VeterancyPercentage;
	infantry.Group = ExtConfigs::DefaultInfantryProperty.Group;
	infantry.IsAboveGround = ExtConfigs::DefaultInfantryProperty.IsAboveGround;
	infantry.AutoNORecruitType = ExtConfigs::DefaultInfantryProperty.AutoNORecruitType;
	infantry.AutoYESRecruitType = ExtConfigs::DefaultInfantryProperty.AutoYESRecruitType;
	infantry.Health = ExtConfigs::DefaultInfantryProperty.Health;

	infantry.House = CIsoView::CurrentHouse();
	infantry.SubCell.Format("%d", CIsoViewExt::GetSelectedSubcellInfantryIdx(X, Y, true));
	infantry.TypeID = CIsoView::CurrentCommand->ObjectID;
	infantry.X.Format("%d", X);
	infantry.Y.Format("%d", Y);

	CMapData::Instance->SetInfantryData(&infantry, nullptr, nullptr, 0, -1);

	return 0x46D48C;
}

DEFINE_HOOK(45B4BA, CIsoView_OnMouseMove_DrawSubcellDragLine_X, 9)
{
	auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
	if (pIsoView->CurrentCellObjectType == 0)
	{
		CInfantryData infData;
		CMapData::Instance->GetInfantryData(pIsoView->CurrentCellObjectIndex, infData);
		int offsetX = 0;
		switch (atoi(infData.SubCell))
		{
		case 2:
			offsetX = 18;
			break;
		case 3:
			offsetX = -12;
			break;
		case 0:
		case 1:
		case 4:
			offsetX = 3;
			break;
		}
		R->ECX(R->ECX() + offsetX);
	}

	return 0;
}

DEFINE_HOOK(45B491, CIsoView_OnMouseMove_DrawSubcellDragLine_Y, 6)
{
	auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
	if (pIsoView->CurrentCellObjectType == 0)
	{
		CInfantryData infData;
		CMapData::Instance->GetInfantryData(pIsoView->CurrentCellObjectIndex, infData);
		int offsetY = 0;
		switch (atoi(infData.SubCell))
		{
		case 2:
		case 3:
			break;
		case 0:
		case 1:
			break;
		case 4:
			offsetY = 7;
			break;
		}
		R->EAX(R->EAX() + offsetY);
	}
	return 0;
}

DEFINE_HOOK(45B4D4, CIsoView_OnMouseMove_DrawSubcellDragLine_Target, 9)
{
	int offsetX = 0;
	int offsetY = 0;

	if (ExtConfigs::InfantrySubCell_Edit && ExtConfigs::InfantrySubCell_Edit_Drag)
	{
		auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
		if (pIsoView->CurrentCellObjectType == 0)
		{
			auto point = pIsoView->GetCurrentMapCoord(CIsoView::GetInstance()->MouseCurrentPosition);

			switch (CIsoViewExt::GetSelectedSubcellInfantryIdx(point.X, point.Y, true))
			{
			case 2:
				offsetX = 15;
				break;
			case 3:
				offsetX = -15;
				break;
			case 0:
			case 1:
				break;
			case 4:
				offsetY = 8;
				break;
			}
		}
	}

	R->ECX(R->ECX() + offsetX + 5);
	R->EAX(R->EAX() + offsetY - 3); // make line in the center
	return 0;
}
