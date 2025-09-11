#include "Body.h"
#include "../../FA2sp.h"
#include <CMapData.h>
#include "../CMapData/Body.h"

/*
DEFINE_HOOK(45E880, CIsoView_MapCoord2ScreenCoord_Height, 5)
{
	GET_STACK(int*, X, 0x4);
	GET_STACK(int*, Y, 0x8);
	*X = std::max(0, *X);
	*Y = std::max(0, *Y);
	*X = std::min(CMapData::Instance->MapWidthPlusHeight, *X);
	*Y = std::min(CMapData::Instance->MapWidthPlusHeight, *Y);
	return 0;
}

DEFINE_HOOK(476240, CIsoView_MapCoord2ScreenCoord_Flat, 5)
{
	GET_STACK(int*, X, 0x4);
	GET_STACK(int*, Y, 0x8);
	*X = std::max(0, *X);
	*Y = std::max(0, *Y);
	*X = std::min(CMapData::Instance->MapWidthPlusHeight, *X);
	*Y = std::min(CMapData::Instance->MapWidthPlusHeight, *Y);
	return 0;
}
*/

#define BACK_BUFFER_TO_PRIMARY(hook_addr, hook_name, hook_size, return_addr, special_draw) \
DEFINE_HOOK(hook_addr,hook_name,hook_size) \
{ \
	auto pThis = CIsoView::GetInstance();\
	CRect dr = CIsoViewExt::GetVisibleIsoViewRect();\
	if (special_draw >= 1 || abs(CIsoViewExt::ScaledFactor - 1.0) <= 0.01) {\
		if (special_draw > -1){\
			CIsoViewExt::SpecialDraw(pThis->lpDDBackBufferSurface, special_draw);\
		}\
		CIsoViewExt::ReduceBrightness(pThis->lpDDBackBufferSurface, dr);\
		pThis->lpDDPrimarySurface->Blt(&dr, pThis->lpDDBackBufferSurface, &dr, DDBLT_WAIT, 0);\
		return return_addr; \
	}\
	CRect backDr;\
	backDr = dr;\
	backDr.right += backDr.Width() * (CIsoViewExt::ScaledFactor - 1.0);\
	backDr.bottom += backDr.Height() * (CIsoViewExt::ScaledFactor - 1.0);\
	CIsoViewExt::StretchCopySurfaceBilinear(pThis->lpDDBackBufferSurface, backDr,\
		CIsoViewExt::lpDDBackBufferZoomSurface, dr);\
	CIsoViewExt::SpecialDraw(CIsoViewExt::lpDDBackBufferZoomSurface, special_draw);\
	CIsoViewExt::ReduceBrightness(CIsoViewExt::lpDDBackBufferZoomSurface, dr);\
	pThis->lpDDPrimarySurface->Blt(&dr, CIsoViewExt::lpDDBackBufferZoomSurface, &dr, DDBLT_WAIT, 0);\
	return return_addr; \
}

BACK_BUFFER_TO_PRIMARY(475150, CIsoView_Draw_BackBufferToPrimary, 6, 0x47517B, 0);
BACK_BUFFER_TO_PRIMARY(459DEB, CIsoView_OnMouseMove_BackBufferToPrimary_Copy, 5, 0x459FE7, 2);
BACK_BUFFER_TO_PRIMARY(45CDF2, CIsoView_OnMouseMove_BackBufferToPrimary_Bridge, 6, 0x45D079, 3);
BACK_BUFFER_TO_PRIMARY(45AD6A, CIsoView_OnMouseMove_BackBufferToPrimary_Cursor, 9, 0x45AEF6, 1);
#undef BACK_BUFFER_TO_PRIMARY

DEFINE_HOOK(4572E1, CIsoView_OnMouseMove_BltTempBuffer, 6)
{
	auto pThis = CIsoView::GetInstance(); 
	CRect rect = CIsoViewExt::GetVisibleIsoViewRect();
	pThis->lpDDBackBufferSurface->Blt(&rect, pThis->lpDDTempBufferSurface, &rect, DDBLT_WAIT, 0);
	return 0x4572FC;
}

DEFINE_HOOK(4750CF, CIsoView_Draw_SkipScroll_SkipTempBufferBlt, 6)
{
	return 0x475150;
}

DEFINE_HOOK(460F00, CIsoView_ScreenCoord2MapCoord_Height, 7)
{
	GET_STACK(int*, X, 0x4);
	GET_STACK(int*, Y, 0x8);

	auto pThis = CIsoView::GetInstance();
	CRect dr;
	GetWindowRect(pThis->GetSafeHwnd(), &dr);
	*X += (*X - pThis->ViewPosition.x - dr.left) * (CIsoViewExt::ScaledFactor - 1.0);
	*Y += (*Y - pThis->ViewPosition.y - dr.top) * (CIsoViewExt::ScaledFactor - 1.0);
	return 0;
}

DEFINE_HOOK(466890, CIsoView_ScreenCoord2MapCoord_Flat, 8)
{
	GET_STACK(int*, X, 0x4);
	GET_STACK(int*, Y, 0x8);

	auto pThis = CIsoView::GetInstance();
	CRect dr;
	pThis->GetWindowRect(&dr);
	*X += (*X - pThis->ViewPosition.x - dr.left) * (CIsoViewExt::ScaledFactor - 1.0);
	*Y += (*Y - pThis->ViewPosition.y - dr.top) * (CIsoViewExt::ScaledFactor - 1.0);
	return 0;
}

DEFINE_HOOK(456F37, CIsoView_OnMouseMove_CursorScaled, 8)
{
	auto pThis = CIsoView::GetInstance();
	auto coord = pThis->GetCurrentMapCoord(pThis->MouseCurrentPosition);

	R->EBX(coord.X);
	R->ESI(coord.Y);
	R->Stack(STACK_OFFS(0x3D528, 0x3D4F0), coord.X);
	R->Stack(STACK_OFFS(0x3D528, 0x3D4F8), coord.Y);

	return 0x457207;
}

static CPoint OnLButtonDown_pos;
DEFINE_HOOK(4612F0, CIsoView_OnLButtonDown_Update_Pos, 5)
{
	OnLButtonDown_pos.x = R->Stack<UINT>(0x8);
	OnLButtonDown_pos.y = R->Stack<UINT>(0xC);
	return 0;
}

DEFINE_HOOK(46133D, CIsoView_OnLButtonDown_Scaled_1, 9)
{
	auto coord = CIsoView::GetInstance()->GetCurrentMapCoord(OnLButtonDown_pos);

	R->EDI(coord.X);
	R->Base(-0x50, coord.Y);
	R->Base(-0x80, coord.X);

	return 0x4615CE;
}

static CPoint OnLButtonUp_pos;
DEFINE_HOOK(466970, CIsoView_OnLButtonUp_Update_Pos, 6)
{
	OnLButtonUp_pos.x = R->Stack<UINT>(0x8);
	OnLButtonUp_pos.y = R->Stack<UINT>(0xC);
	return 0;
}

DEFINE_HOOK(4669A8, CIsoView_OnLButtonUp_Scaled_1, A)
{
	auto coord = CIsoView::GetInstance()->GetCurrentMapCoord(OnLButtonUp_pos);

	R->Stack(STACK_OFFS(0x214, 0x204), coord.X);
	R->EDI(coord.Y);

	return 0x466C6E;
}

DEFINE_HOOK(45AFFC, CIsoView_OnMouseMove_Drag_skip_dragFacing, 7)
{
	if (ExtConfigs::ExtFacings_DragPreview)
	{
		auto pIsoView = (CIsoViewExt*)CIsoView::GetInstance();
		auto point = pIsoView->GetCurrentMapCoord(pIsoView->MouseCurrentPosition);
		if (CIsoView::CurrentCommand->Command == 0
			&& (GetKeyState(VK_CONTROL) & 0x8000)
			&& CIsoView::GetInstance()->CurrentCellObjectIndex >= 0
			&& CIsoView::GetInstance()->CurrentCellObjectType >= 0
			&& CIsoView::GetInstance()->CurrentCellObjectType <= 3)
		{
			CViewObjectsExt::ApplyDragFacing(point.X, point.Y);
		}
	}

	return 0x45CD6D;
}

DEFINE_HOOK(4751FA, CIsoView_BltToBackBuffer_Scaled, 7)
{
	GET_STACK(CRect, viewRect, STACK_OFFS(0xC0, 0x8C));

	if (CIsoViewExt::ScaledFactor > 1.0)
	{
		viewRect.right += viewRect.Width() * (CIsoViewExt::ScaledFactor - 1.0);
		viewRect.bottom += viewRect.Height() * (CIsoViewExt::ScaledFactor - 1.0);
	}

	R->Stack(STACK_OFFS(0xC0, 0x8C), viewRect);

	return 0;
}

DEFINE_HOOK(476419, CIsoView_MoveTo, 7)
{
	auto pThis = CIsoView::GetInstance();

	RECT r;
	pThis->GetWindowRect(&r);

	int& height = CMapData::Instance->Size.Height;
	int& width = CMapData::Instance->Size.Width;

	if (pThis->ViewPosition.x < (height / 2 - 4 - r.left / 60) * 60)
		pThis->ViewPosition.x = (height / 2 - 4 - r.left / 60) * 60;
	if (pThis->ViewPosition.x + r.right * CIsoViewExt::ScaledFactor * 0.8 > (height / 2 + width + 7) * 60)
		pThis->ViewPosition.x = (height / 2 + width + 7) * 60 - r.right * CIsoViewExt::ScaledFactor * 0.8;
	if (pThis->ViewPosition.y < (width / 2 - 10 - r.top / 30) * 30)
		pThis->ViewPosition.y = (width / 2 - 10 - r.top / 30) * 30;
	if (pThis->ViewPosition.y + r.bottom * CIsoViewExt::ScaledFactor * 0.8 > (width / 2 + height + 6) * 30)
		pThis->ViewPosition.y = (width / 2 + height + 6) * 30 - r.bottom * CIsoViewExt::ScaledFactor * 0.8;

	SetScrollPos(pThis->GetSafeHwnd(), SB_VERT, pThis->ViewPosition.y / 30 - width / 2 + 4, TRUE);
	SetScrollPos(pThis->GetSafeHwnd(), SB_HORZ, pThis->ViewPosition.x / 60 - height / 2 + 1, TRUE);

	return 0x476571;
}
