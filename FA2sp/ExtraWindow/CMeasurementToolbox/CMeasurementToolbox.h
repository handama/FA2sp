#pragma once

#include <vector>
#include "FA2PP.h"
#include "../../FA2sp/Helpers/FString.h"
#include "../../Helpers/MultimapHelper.h"
#include "../Common.h"

struct MapCoord;

class CMeasurementToolbox : public ppmfc::CDialog
{
public:
	static void ShowMeasurementToolbox();
	static void SetMeasurementToolbox(int X, int Y);
	static void ClearStatus();
	static void OnRightButtonDown();
	static void CancelPendingMeasurements();
	static CMeasurementToolbox* m_pMeasurementToolbox;
	static TransparencyHelper m_transparency;
protected:
enum Controls
	{
		TwoPointDistance = 1001,
		LiveDistance = 1002,
		ClearDistancePoints = 1003,
		LineSegment = 1004,
		PathDistance = 1005,
		PathTypeCombo = 1006,
		PathDistanceGroup = 1400,
		MovementZoneLabel = 1401,
		DestructibleOverlayCheck = 1402,
		DestructibleUnitsCheck = 1403,
		ClearPathDistance = 1404,
		SetSymmetryAxis = 1101,
		PlaceSymmetricPoint = 1102,
		ClearSymmetricPoints = 1103,
		SetCentralSymmetryCenter = 1201,
		PlaceCentralSymmetricPoint = 1202,
		ClearCentralSymmetricPoints = 1203,
		SetRadius = 1301,
		PlaceCircleCenter = 1302,
		PlaceCircle= 1304,
		ClearCircles = 1303,
	};

	CMeasurementToolbox(CWnd* pParent = NULL);
	virtual BOOL OnInitDialog();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DoDataExchange(ppmfc::CDataExchange* pDX);
	virtual void PostNcDestroy();
	virtual void OnCancel();
	virtual void OnClose();
	void OnClickTwoPointDistance();
	void OnClickLineSegment();
	void OnClickLiveDistance();
	void OnClickClearDistancePoints();
	void OnClickPathDistance();
	void OnSelectPathType();
	void OnClickDestructibleOverlay();
	void OnClickDestructibleUnits();
	void OnClickClearPathDistance();
	void OnClickOverlayOption();
	void OnClickObjectOption();
	void OnClickSetSymmetryAxis();
	void OnClickPlaceSymmetricPoint();
	void OnClickClearSymmetricPoints();
	void OnClickSetCentralSymmetryCenter();
	void OnClickPlaceCentralSymmetricPoint();
	void OnClickClearCentralSymmetricPoints();
	void OnEditSetRadius();
	void OnClickPlaceCircleCenter();
	void OnClickPlaceCircle();
	void OnClickClearCircles();
	static MapCoord GetAxialSymmetricPoint(const MapCoord& p);
	static MapCoord GetCentralSymmetricPoint(const MapCoord& p);
	ppmfc::CString m_radius;

	BOOL b_LiveDistance;
};
