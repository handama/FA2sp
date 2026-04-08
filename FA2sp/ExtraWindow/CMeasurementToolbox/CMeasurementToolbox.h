#pragma once

#include <vector>
#include "FA2PP.h"
#include "../../FA2sp/Helpers/FString.h"
#include "../../Helpers/MultimapHelper.h"

struct MapCoord;

class CMeasurementToolbox : public ppmfc::CDialog
{
public:
	static void ShowMeasurementToolbox();
	static void SetMeasurementToolbox(int X, int Y);
	static void ClearStatus();
	static void OnRightButtonDown();
	static CMeasurementToolbox* m_pMeasurementToolbox;

protected:
	enum Controls
	{
		TwoPointDistance = 1001,
		LiveDistance = 1002,
		ClearDistancePoints = 1003,
		SetSymmetryAxis = 1101,
		PlaceSymmetricPoint = 1102,
		ClearSymmetricPoints = 1103,
		SetCentralSymmetryCenter = 1201,
		PlaceCentralSymmetricPoint = 1202,
		ClearCentralSymmetricPoints = 1203,
		SetRadius = 1301,
		PlaceCircleCenter = 1302,
		ClearCircles = 1303,
	};

	CMeasurementToolbox(CWnd* pParent = NULL);
	virtual BOOL OnInitDialog();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void DoDataExchange(ppmfc::CDataExchange* pDX);
	virtual void PostNcDestroy();
	virtual void OnCancel();
	virtual void OnClose();
	void OnClickTwoPointDistance();
	void OnClickLiveDistance();
	void OnClickClearDistancePoints();
	void OnClickSetSymmetryAxis();
	void OnClickPlaceSymmetricPoint();
	void OnClickClearSymmetricPoints();
	void OnClickSetCentralSymmetryCenter();
	void OnClickPlaceCentralSymmetricPoint();
	void OnClickClearCentralSymmetricPoints();
	void OnEditSetRadius();
	void OnClickPlaceCircle();
	void OnClickClearCircles();
	static MapCoord GetAxialSymmetricPoint(const MapCoord& p);
	static MapCoord GetCentralSymmetricPoint(const MapCoord& p);
	ppmfc::CString m_radius;

	BOOL b_LiveDistance;
};
