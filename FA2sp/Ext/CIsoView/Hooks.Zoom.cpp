#include "Body.h"
#include "../../FA2sp.h"
#include <CMapData.h>
#include "../CMapData/Body.h"
#include "../CLoading/Body.h"

DEFINE_HOOK(45E880, CIsoView_MapCoord2ScreenCoord_Height, 5)
{
	if (CIsoViewExt::SkipMapScreenConvert)
		return 0x45E90C;

	return 0;
}

DEFINE_HOOK(476240, CIsoView_MapCoord2ScreenCoord_Flat, 5)
{
	if (CIsoViewExt::SkipMapScreenConvert)
		return 0x476297;

	return 0;
}

#define BACK_BUFFER_TO_PRIMARY(hook_addr, hook_name, hook_size, return_addr, special_draw) \
DEFINE_HOOK(hook_addr,hook_name,hook_size) \
{ \
	if (CIsoViewExt::RenderingMap) return return_addr; \
	auto pThis = CIsoViewExt::GetExtension();\
	CRect dr = CIsoViewExt::GetVisibleIsoViewRect();\
	if (special_draw >= 1 || CIsoViewExt::ScaledFactor == 1.0) {\
		if (special_draw > -1){\
			CIsoViewExt::SpecialDraw(CIsoViewExt::GetBackBuffer(), special_draw);\
		}\
		if (special_draw != 3)\
			CIsoViewExt::ReduceBrightness(CIsoViewExt::GetBackBuffer(), dr);\
		if (ExtConfigs::SecondScreenSupport) {\
			CRect drFixed = dr; \
			pThis->BltToWindow(pThis->m_hWnd,  CIsoViewExt::GetBackBuffer(), &dr, &drFixed);\
		}\
		else\
			pThis->lpDDPrimarySurface->Blt(&dr, CIsoViewExt::GetBackBuffer(), &dr, DDBLT_WAIT, 0);\
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
	if (ExtConfigs::SecondScreenSupport)\
		pThis->BltToWindow(pThis->m_hWnd,  CIsoViewExt::lpDDBackBufferZoomSurface, &dr, &dr);\
	else\
		pThis->lpDDPrimarySurface->Blt(&dr, CIsoViewExt::lpDDBackBufferZoomSurface, &dr, DDBLT_WAIT, 0);\
	return return_addr; \
}

BACK_BUFFER_TO_PRIMARY(459DEB, CIsoView_OnMouseMove_BackBufferToPrimary_Copy, 5, 0x459FE7, 2);
BACK_BUFFER_TO_PRIMARY(45CDF2, CIsoView_OnMouseMove_BackBufferToPrimary_Bridge, 6, 0x45D079, 3);
BACK_BUFFER_TO_PRIMARY(45AD6A, CIsoView_OnMouseMove_BackBufferToPrimary_Cursor, 9, 0x45AEF6, 1);
#undef BACK_BUFFER_TO_PRIMARY

DEFINE_HOOK(4572E1, CIsoView_OnMouseMove_BltTempBuffer, 6)
{
	auto pThis = CIsoView::GetInstance(); 
	CRect rect = CIsoViewExt::GetVisibleIsoViewRect();
	CIsoViewExt::GetBackBuffer()->Blt(&rect, pThis->lpDDTempBufferSurface, &rect, DDBLT_WAIT, 0);
	return 0x4572FC;
}

DEFINE_HOOK(460F00, CIsoView_ScreenCoord2MapCoord_Height, 7)
{
	GET_STACK(int*, X, 0x4);
	GET_STACK(int*, Y, 0x8);

	if (!CMapData::Instance->MapWidthPlusHeight
		|| !CTileTypeClass::Instance
		|| CIsoViewExt::SkipMapScreenConvert)
	{
		*X = 0;
		*Y = 0;
		return 0x4612EB;
	}

	auto pThis = CIsoView::GetInstance();
	CRect dr;
	GetWindowRect(pThis->GetSafeHwnd(), &dr);
	CIsoViewExt::AdaptRectForSecondScreen(&dr);
	*X += (*X - pThis->ViewPosition.x - dr.left) * (CIsoViewExt::ScaledFactor - 1.0);
	*Y += (*Y - pThis->ViewPosition.y - dr.top) * (CIsoViewExt::ScaledFactor - 1.0);
	return 0;
}

DEFINE_HOOK(460FF1, CIsoView_ScreenCoord2MapCoord_Height_IgnoreOutside, 8)
{
	GET(int, Y, EDI);
	GET(int, X, EBP);

	if (CMapData::Instance->IsCoordInMap(X, Y))
		return 0x461001;

	return 0x46126E;
}

DEFINE_HOOK(461167, CIsoView_ScreenCoord2MapCoord_Height_TileData, 6)
{
	if (!CMapData::Instance->MapWidthPlusHeight)
	{
		R->ECX(&CMapDataExt::TileData);
		return 0x46116D;
	}
	return 0;
}

DEFINE_HOOK(466890, CIsoView_ScreenCoord2MapCoord_Flat, 8)
{
	GET_STACK(int*, X, 0x4);
	GET_STACK(int*, Y, 0x8);

	auto pThis = CIsoView::GetInstance();
	CRect dr;
	pThis->GetWindowRect(&dr);
	CIsoViewExt::AdaptRectForSecondScreen(&dr);

	*X += (*X - pThis->ViewPosition.x - dr.left) * (CIsoViewExt::ScaledFactor - 1.0);
	*Y += (*Y - pThis->ViewPosition.y - dr.top) * (CIsoViewExt::ScaledFactor - 1.0);
	return 0;
}

DEFINE_HOOK(456F37, CIsoView_OnMouseMove_CursorScaled, 8)
{
	auto pThis = CIsoViewExt::GetExtension();
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
	if (ExtConfigs::SecondScreenSupport && CIsoViewExt::OnLButtonDown_CalledFromOnMouseMove)
	{
		CIsoViewExt::OnLButtonDown_CalledFromOnMouseMove = false;
	}
	else if (ExtConfigs::SecondScreenSupport)
	{
		R->Stack(0x8, R->Stack<int>(0x8) - GetSystemMetrics(SM_XVIRTUALSCREEN));
		R->Stack(0xC, R->Stack<int>(0xC) - GetSystemMetrics(SM_YVIRTUALSCREEN));
	}

	OnLButtonDown_pos.x = R->Stack<int>(0x8);
	OnLButtonDown_pos.y = R->Stack<int>(0xC);
	return 0;
}

DEFINE_HOOK(46133D, CIsoView_OnLButtonDown_Scaled_1, 9)
{
	auto coord = CIsoViewExt::GetExtension()->GetCurrentMapCoord(OnLButtonDown_pos);

	R->EDI(coord.X);
	R->Base(-0x50, coord.Y);
	R->Base(-0x80, coord.X);

	return 0x4615CE;
}

DEFINE_HOOK(4616F0, CIsoView_OnLButtonDown_Scaled_2, 6)
{
	auto coord = CIsoViewExt::GetExtension()->GetCurrentMapCoord(OnLButtonDown_pos);

	R->Base(-0x94, coord.X);
	R->Base(-0x84, coord.Y);
	R->EDI(coord.X);
	R->ESI(coord.Y);

	return 0x4616FC;
}

#define CIsoView_OnMouseMove_CallOnLButtonDown(hook_addr, hook_name, hook_size) \
DEFINE_HOOK(hook_addr,hook_name,hook_size) \
{ \
	CIsoViewExt::OnLButtonDown_CalledFromOnMouseMove = true;\
	return 0;\
}
CIsoView_OnMouseMove_CallOnLButtonDown(45B583, CallOnLButtonDown2, 7)
CIsoView_OnMouseMove_CallOnLButtonDown(45B5A3, CallOnLButtonDown3, 7)
CIsoView_OnMouseMove_CallOnLButtonDown(45BF40, CallOnLButtonDown4, 7)
#undef CIsoView_OnMouseMove_CallOnLButtonDown

#define CIsoView_OnLButtonDown_CallOnMouseMove(hook_addr, hook_name, hook_size) \
DEFINE_HOOK(hook_addr,hook_name,hook_size) \
{ \
	CIsoViewExt::OnMouseMove_CalledFromOnLButtonDown = true;\
	return 0;\
}
CIsoView_OnLButtonDown_CallOnMouseMove(4665D0, CallOnMouseMove1, 6)
CIsoView_OnLButtonDown_CallOnMouseMove(46684D, CallOnMouseMove2, 6)
#undef CIsoView_OnLButtonDown_CallOnMouseMove

static CPoint OnLButtonUp_pos;
DEFINE_HOOK(466970, CIsoView_OnLButtonUp_Update_Pos, 6)
{
	if (ExtConfigs::SecondScreenSupport)
	{
		R->Stack(0x8, R->Stack<int>(0x8) - GetSystemMetrics(SM_XVIRTUALSCREEN));
		R->Stack(0xC, R->Stack<int>(0xC) - GetSystemMetrics(SM_YVIRTUALSCREEN));
	}
	OnLButtonUp_pos.x = R->Stack<int>(0x8);
	OnLButtonUp_pos.y = R->Stack<int>(0xC);
	return 0;
}

DEFINE_HOOK(4669A8, CIsoView_OnLButtonUp_Scaled_1, A)
{
	auto coord = CIsoViewExt::GetExtension()->GetCurrentMapCoord(OnLButtonUp_pos);

	R->Stack(STACK_OFFS(0x214, 0x204), coord.X);
	R->EDI(coord.Y);

	return 0x466C6E;
}

DEFINE_HOOK(45AFFC, CIsoView_OnMouseMove_Drag_skip_dragFacing, 7)
{
	if (ExtConfigs::ExtFacings_DragPreview)
	{
		auto pIsoView = CIsoViewExt::GetExtension();
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
	CIsoViewExt::AdaptRectForSecondScreen(&r);

	int left = r.left;
	int top = r.top;

	int widthPx = r.right - r.left;
	int heightPx = r.bottom - r.top;

	int& height = CMapData::Instance->Size.Height;
	int& width = CMapData::Instance->Size.Width;

	if (pThis->ViewPosition.x < (height / 2 - 4 - left / 60) * 60)
		pThis->ViewPosition.x = (height / 2 - 4 - left / 60) * 60;

	double visibleRight = left + widthPx * CIsoViewExt::ScaledFactor * 0.8;

	if (pThis->ViewPosition.x + visibleRight > (height / 2 + width + 7) * 60)
		pThis->ViewPosition.x = (height / 2 + width + 7) * 60 - visibleRight;

	if (pThis->ViewPosition.y < (width / 2 - 10 - top / 30) * 30)
		pThis->ViewPosition.y = (width / 2 - 10 - top / 30) * 30;

	double visibleBottom = top + heightPx * CIsoViewExt::ScaledFactor * 0.8;

	if (pThis->ViewPosition.y + visibleBottom > (width / 2 + height + 6) * 30)
		pThis->ViewPosition.y = (width / 2 + height + 6) * 30 - visibleBottom;

	SetScrollPos(pThis->GetSafeHwnd(), SB_VERT, pThis->ViewPosition.y / 30 - width / 2 + 4, TRUE);
	SetScrollPos(pThis->GetSafeHwnd(), SB_HORZ, pThis->ViewPosition.x / 60 - height / 2 + 1, TRUE);

	return 0x476571;
}

DEFINE_HOOK(456DA0, CIsoView_OnMouseMove_FixPos, 8)
{
	if (ExtConfigs::SecondScreenSupport && !CIsoViewExt::OnMouseMove_CalledFromOnLButtonDown)
	{
		R->Stack(0x8, R->Stack<int>(0x8) - GetSystemMetrics(SM_XVIRTUALSCREEN));
		R->Stack(0xC, R->Stack<int>(0xC) - GetSystemMetrics(SM_YVIRTUALSCREEN));
	}
	CIsoViewExt::OnMouseMove_CalledFromOnLButtonDown = false;
	return 0;
}

DEFINE_HOOK(4763B0, CIsoView_OnRButtonDown_FixPos, 8)
{
	if (ExtConfigs::SecondScreenSupport)
	{
		R->Stack(0x8, R->Stack<int>(0x8) - GetSystemMetrics(SM_XVIRTUALSCREEN));
		R->Stack(0xC, R->Stack<int>(0xC) - GetSystemMetrics(SM_YVIRTUALSCREEN));
	}
	return 0;
}

DEFINE_HOOK(460DA0, CIsoView_OnLButtonDblClk_FixPos, 8)
{
	if (ExtConfigs::SecondScreenSupport)
	{
		R->Stack(0x8, R->Stack<int>(0x8) - GetSystemMetrics(SM_XVIRTUALSCREEN));
		R->Stack(0xC, R->Stack<int>(0xC) - GetSystemMetrics(SM_YVIRTUALSCREEN));
	}
	return 0;
}

DEFINE_HOOK(456CED, CIsoView_UpdateDialog, 9)
{
	CIsoViewExt::MoveToMapCoord(CMapData::Instance->MapWidthPlusHeight / 2, CMapData::Instance->MapWidthPlusHeight / 2);
	return 0x456D53;
}