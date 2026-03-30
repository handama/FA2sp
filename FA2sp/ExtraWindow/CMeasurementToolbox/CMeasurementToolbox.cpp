#include "CMeasurementToolbox.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../Common.h"
#include "../../Ext/CLoading/Body.h"
#include <CFinalSunApp.h>
#include "../../Ext/CIsoView/Body.h"

CMeasurementToolbox* CMeasurementToolbox::m_pMeasurementToolbox = nullptr;

void CMeasurementToolbox::ShowMeasurementToolbox()
{
	if (m_pMeasurementToolbox == nullptr || !::IsWindow(m_pMeasurementToolbox->m_hWnd))
	{
		m_pMeasurementToolbox = new CMeasurementToolbox();   

		if (!m_pMeasurementToolbox->Create(339, CFinalSunDlg::Instance)) 
		{
			CIsoViewExt::EnableOtherMeasurementTools = false;
			delete m_pMeasurementToolbox;
			m_pMeasurementToolbox = nullptr;
			Logger::Error("Failed to create CMeasurementToolbox.\n");
			return;
		}
	}
	CIsoViewExt::EnableOtherMeasurementTools = true;
	m_pMeasurementToolbox->ShowWindow(SW_SHOW);    
	::SetWindowPos(m_pMeasurementToolbox->GetSafeHwnd(), HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

CMeasurementToolbox::CMeasurementToolbox(CWnd* pParent /*=NULL*/)
	: ppmfc::CDialog(339, pParent)
{
    b_LiveDistance = false;
}

BOOL CMeasurementToolbox::OnInitDialog()
{
	CDialog::OnInitDialog();
	FString buffer;

	auto translate = [&buffer, this](int nItem, const char* lpLabel)
	{
		if (Translations::GetTranslationItem(lpLabel, buffer))
			GetDlgItem(nItem)->SetWindowTextA(buffer);
	};

	translate(900, "MeasurementToolbox.DistanceRuler");
	translate(901, "MeasurementToolbox.AxialSymmetry");
	translate(902, "MeasurementToolbox.CentralSymmetry");
	translate(TwoPointDistance, "MeasurementToolbox.TwoPointDistance");
	translate(LiveDistance, "MeasurementToolbox.LiveDistance");
	translate(ClearDistancePoints, "MeasurementToolbox.ClearDistancePoints");
	translate(SetSymmetryAxis, "MeasurementToolbox.SetSymmetryAxis");
	translate(PlaceSymmetricPoint, "MeasurementToolbox.PlaceSymmetricPoint");
	translate(ClearSymmetricPoints, "MeasurementToolbox.ClearSymmetricPoints");
	translate(SetCentralSymmetryCenter, "MeasurementToolbox.SetCentralSymmetryCenter");
	translate(PlaceCentralSymmetricPoint, "MeasurementToolbox.PlaceCentralSymmetricPoint");
	translate(ClearCentralSymmetricPoints, "MeasurementToolbox.ClearCentralSymmetricPoints");

	if (Translations::GetTranslationItem("MeasurementToolboxCaption", buffer))
		SetWindowTextA(buffer);

	return TRUE;
}

BOOL CMeasurementToolbox::OnCommand(WPARAM wParam, LPARAM lParam)
{
    WORD nID = LOWORD(wParam);
    WORD nNotify = HIWORD(wParam);

    if (nNotify == BN_CLICKED)
    {
		switch (nID)
		{
		case CMeasurementToolbox::TwoPointDistance:	
			OnClickTwoPointDistance();
			break;
		case CMeasurementToolbox::LiveDistance:
			OnClickLiveDistance();
			break;
		case CMeasurementToolbox::ClearDistancePoints:
			OnClickClearDistancePoints();
			break;
		case CMeasurementToolbox::SetSymmetryAxis:
			OnClickSetSymmetryAxis();
			break;
		case CMeasurementToolbox::PlaceSymmetricPoint:
			OnClickPlaceSymmetricPoint();
			break;
		case CMeasurementToolbox::ClearSymmetricPoints:
			OnClickClearSymmetricPoints();
			break;
		case CMeasurementToolbox::SetCentralSymmetryCenter:
			OnClickSetCentralSymmetryCenter();
			break;
		case CMeasurementToolbox::PlaceCentralSymmetricPoint:
			OnClickPlaceCentralSymmetricPoint();
			break;
		case CMeasurementToolbox::ClearCentralSymmetricPoints:
			OnClickClearCentralSymmetricPoints();
			break;
		default:
			break;
		}
    }

    return CDialog::OnCommand(wParam, lParam);
}

MapCoord CMeasurementToolbox::GetAxialSymmetricPoint(const MapCoord& p)
{
	auto& axis = CIsoViewExt::AxialSymmetryLine;

	double x1 = axis[0].X;
	double y1 = axis[0].Y;
	double x2 = axis[1].X;
	double y2 = axis[1].Y;

	double x = p.X;
	double y = p.Y;

	double dx = x2 - x1;
	double dy = y2 - y1;

	double len2 = dx * dx + dy * dy;
	if (len2 == 0.0)
	{
		return p;
	}

	double apx = x - x1;
	double apy = y - y1;

	double t = (apx * dx + apy * dy) / len2;

	double qx = x1 + t * dx;
	double qy = y1 + t * dy;

	double sx = 2 * qx - x;
	double sy = 2 * qy - y;

	MapCoord result{};
	result.X = static_cast<int>(std::round(sx));
	result.Y = static_cast<int>(std::round(sy));

	return result;
}

MapCoord CMeasurementToolbox::GetCentralSymmetricPoint(const MapCoord& p)
{
	const auto& c = CIsoViewExt::CentralSymmetryCenter;

	double sx = 2.0 * c.X - p.X;
	double sy = 2.0 * c.Y - p.Y;

	MapCoord result{};
	result.X = static_cast<int>(std::round(sx));
	result.Y = static_cast<int>(std::round(sy));

	return result;
}

void CMeasurementToolbox::OnClickTwoPointDistance()
{
	CIsoView::CurrentCommand->Command = 0x26;
	CIsoView::CurrentCommand->Type = MeasurementTypes::TwoPointDistance;

	::SendMessage(GetDlgItem(LiveDistance)->GetSafeHwnd(), BM_SETCHECK, BST_UNCHECKED, 0);
	CIsoViewExt::LiveDistanceRuler.clear();
	CIsoViewExt::EnableLiveDistanceRuler = false;
}

void CMeasurementToolbox::SetMeasurementToolbox(int X, int Y)
{
	if (CIsoView::CurrentCommand->Type == MeasurementTypes::TwoPointDistance)
	{
		if (CIsoViewExt::TwoPointDistance.empty() || CIsoViewExt::TwoPointDistance.back()[1] != MapCoord{ 0, 0 })
		{
			CIsoViewExt::TwoPointDistance.push_back({ MapCoord{ 0, 0 }, MapCoord{ 0, 0 } });
		}
		if (CIsoViewExt::TwoPointDistance.back()[0] == MapCoord{ 0, 0 })
		{
			CIsoViewExt::TwoPointDistance.back()[0] = { X,Y };
		}
		else if (CIsoViewExt::TwoPointDistance.back()[0] != MapCoord{ X,Y })
		{
			CIsoViewExt::TwoPointDistance.back()[1] = { X,Y };
		}
	}
	else if (CIsoView::CurrentCommand->Type == MeasurementTypes::SetSymmetryAxis)
	{
		if (CIsoViewExt::AxialSymmetryLine[1] != MapCoord{ 0, 0 })
		{
			CIsoViewExt::AxialSymmetryLine[0] = MapCoord{ 0, 0 };
			CIsoViewExt::AxialSymmetryLine[1] = MapCoord{ 0, 0 };
		}
		if (CIsoViewExt::AxialSymmetryLine[0] == MapCoord{ 0, 0 })
		{
			CIsoViewExt::AxialSymmetryLine[0] = { X,Y };
			CIsoViewExt::AxialSymmetricPoints.clear();
			::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		}
		else if (CIsoViewExt::AxialSymmetryLine[0] != MapCoord{ X,Y })
		{
			CIsoViewExt::AxialSymmetryLine[1] = { X,Y };
		}
	}
	else if (CIsoView::CurrentCommand->Type == MeasurementTypes::SetCentralSymmetryCenter)
	{
		CIsoViewExt::CentralSymmetryCenter = MapCoord{ X, Y };
		CIsoViewExt::CentralSymmetricPoints.clear();
		::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	}
	else if (CIsoView::CurrentCommand->Type == MeasurementTypes::PlaceSymmetricPoint)
	{
		if (CIsoViewExt::AxialSymmetryLine[0] != MapCoord{ 0, 0 }
			&& CIsoViewExt::AxialSymmetryLine[1] != MapCoord{ 0, 0 })
		{
			MapCoord mc1 = { X,Y };
			auto mc2 = GetAxialSymmetricPoint(mc1);
			CIsoViewExt::AxialSymmetricPoints.push_back(std::make_pair(mc1, mc2));
			::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		}
	}
	else if (CIsoView::CurrentCommand->Type == MeasurementTypes::PlaceCentralSymmetricPoint)
	{
		if (CIsoViewExt::CentralSymmetryCenter != MapCoord{ 0, 0 })
		{
			MapCoord mc1 = { X,Y };
			auto mc2 = GetCentralSymmetricPoint(mc1);
			CIsoViewExt::CentralSymmetricPoints.push_back(std::make_pair(mc1, mc2));
			::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		}

	}
}

void CMeasurementToolbox::OnClickLiveDistance()
{
	CIsoViewExt::EnableLiveDistanceRuler = ::SendMessage(GetDlgItem(LiveDistance)->GetSafeHwnd(), BM_GETCHECK, 0, 0);
	if (CIsoViewExt::EnableLiveDistanceRuler)
	{
		if (CIsoView::CurrentCommand->Command == 0x26)
		{
			CIsoView::CurrentCommand->Command = 0x0;
			CIsoView::CurrentCommand->Type = 0;
		}
	}
	else
	{
		CIsoViewExt::LiveDistanceRuler.clear();
	}
}

void CMeasurementToolbox::OnClickClearDistancePoints()
{
	CIsoViewExt::LiveDistanceRuler.clear();
	CIsoViewExt::EnableLiveDistanceRuler = false;
	CIsoViewExt::TwoPointDistance.clear();
	::SendMessage(GetDlgItem(LiveDistance)->GetSafeHwnd(), BM_SETCHECK, BST_UNCHECKED, 0);
	::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

void CMeasurementToolbox::OnClickSetSymmetryAxis()
{
	CIsoView::CurrentCommand->Command = 0x26;
	CIsoView::CurrentCommand->Type = MeasurementTypes::SetSymmetryAxis;
}

void CMeasurementToolbox::OnClickPlaceSymmetricPoint()
{
	CIsoView::CurrentCommand->Command = 0x26;
	CIsoView::CurrentCommand->Type = MeasurementTypes::PlaceSymmetricPoint;
}

void CMeasurementToolbox::OnClickClearSymmetricPoints()
{
	CIsoViewExt::AxialSymmetricPoints.clear();
	CIsoViewExt::AxialSymmetryLine[0] = { 0,0 };
	CIsoViewExt::AxialSymmetryLine[1] = { 0,0 };
	::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

void CMeasurementToolbox::OnClickSetCentralSymmetryCenter()
{
	CIsoView::CurrentCommand->Command = 0x26;
	CIsoView::CurrentCommand->Type = MeasurementTypes::SetCentralSymmetryCenter;
}

void CMeasurementToolbox::OnClickPlaceCentralSymmetricPoint()
{
	CIsoView::CurrentCommand->Command = 0x26;
	CIsoView::CurrentCommand->Type = MeasurementTypes::PlaceCentralSymmetricPoint;
}

void CMeasurementToolbox::OnClickClearCentralSymmetricPoints()
{
	CIsoViewExt::CentralSymmetricPoints.clear();
	CIsoViewExt::CentralSymmetryCenter = { 0,0 };
	::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

void CMeasurementToolbox::PostNcDestroy()
{
	ClearStatus();

	delete this;
	m_pMeasurementToolbox = nullptr;
}

void CMeasurementToolbox::OnClose()
{
	ClearStatus();
	DestroyWindow();
}

void CMeasurementToolbox::OnCancel()
{
	OnClose();
}

void CMeasurementToolbox::ClearStatus()
{
	CIsoView::CurrentCommand->Command = 0x0;
	CIsoView::CurrentCommand->Type = 0;
	CIsoViewExt::LiveDistanceRuler.clear();
	CIsoViewExt::EnableLiveDistanceRuler = false;
	CIsoViewExt::EnableOtherMeasurementTools = false;
	CIsoViewExt::TwoPointDistance.clear();
	CIsoViewExt::AxialSymmetryLine[0] = MapCoord{ 0,0 };
	CIsoViewExt::AxialSymmetryLine[1] = MapCoord{ 0,0 };
	CIsoViewExt::CentralSymmetryCenter = MapCoord{ 0,0 };
	CIsoViewExt::AxialSymmetricPoints.clear();
	CIsoViewExt::CentralSymmetricPoints.clear();
}

void CMeasurementToolbox::OnRightButtonDown()
{
	if (CIsoView::CurrentCommand->Type == MeasurementTypes::TwoPointDistance)
	{
		if (!CIsoViewExt::TwoPointDistance.empty())
		{
			if (CIsoViewExt::TwoPointDistance.back()[0] != MapCoord{ 0,0 }
				&& CIsoViewExt::TwoPointDistance.back()[1] == MapCoord{ 0,0 })
			{
				CIsoViewExt::TwoPointDistance.back()[0] = MapCoord{ 0,0 };
				CIsoViewExt::TwoPointDistance.back()[1] = MapCoord{ 0,0 };
			}
		}
	}
	if (CIsoView::CurrentCommand->Type == MeasurementTypes::SetSymmetryAxis)
	{
		if (CIsoViewExt::AxialSymmetryLine[0] != MapCoord{ 0,0 }
			&& CIsoViewExt::AxialSymmetryLine[1] == MapCoord{ 0,0 })
		{
			CIsoViewExt::AxialSymmetryLine[0] = MapCoord{ 0,0 };
			CIsoViewExt::AxialSymmetryLine[1] = MapCoord{ 0,0 };
		}
	}

	CIsoView::CurrentCommand->Command = 0x0;
	CIsoView::CurrentCommand->Type = 0;
}