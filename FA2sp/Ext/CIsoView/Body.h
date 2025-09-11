#pragma once

#include <FA2PP.h>

#include <CIsoView.h>
#include "../FA2Expand.h"
#include "../CFinalSunDlg/Body.h"
#include <unordered_map>
#include <unordered_set>
#include <set>

struct CellData;
class ImageDataClassSafe;

struct Cell3DLocation
{
    short X;
    short Y;
    short Height;
};

struct DDBoundary
{
    unsigned dwWidth{};
    unsigned dwHeight{};
    long dpitch{};
};

struct DrawBuildings
{
    short index;
    short x;
    short y;
    short buildingIndex;
};

struct DrawVeterancy
{
    int X;
    int Y;
    int VP;
};

struct TilePlacement
{
    short X;
    short Y;
    short SubtileIndex;
};

class NOVTABLE CIsoViewExt : public CIsoView
{
public:
    static void ProgramStartupInit();

    BOOL PreTranslateMessageExt(MSG* pMsg);

    BOOL OnMouseWheelExt(UINT Flags, short zDelta, CPoint pt);

    void DrawLockedCellOutline(int X, int Y, int W, int H, COLORREF color, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc, bool s1 = true, bool s2 = true, bool s3 = true, bool s4 = true);
    void DrawLockedCellOutlineX(int X, int Y, int W, int H, COLORREF color, COLORREF colorX, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc, bool onlyX = false);
    void DrawLine(int x1, int y1, int x2, int y2, COLORREF color, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc, bool bDashed = false);
    void DrawLockedLines(const std::vector<std::pair<MapCoord, MapCoord>>& lines, int X, int Y, COLORREF color, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc);
    void DrawCelltag(int X, int Y, LPDDSURFACEDESC2 lpDesc);
    void DrawBitmap(FString filename, int X, int Y, LPDDSURFACEDESC2 lpDesc);
    void DrawWaypointFlag(int X, int Y, LPDDSURFACEDESC2 lpDesc);

    void ConfirmTube(bool addReverse = true);

    void DrawLockedCellOutlinePaint(int X, int Y, int W, int H, COLORREF color, bool bUseDot, HDC hdc, HWND hwnd, bool s1 = true, bool s2 = true, bool s3 = true, bool s4 = true);
    void DrawLockedCellOutlinePaintCursor(int X, int Y, int height, COLORREF color, HDC hdc, HWND hwnd, bool useHeightColor);
    static int GetSelectedSubcellInfantryIdx(int X = -1, int Y = -1, bool getSubcell = false);
    static void FillArea(int X, int Y, int ID, int Subtile, int oriX, int oriY, std::set<MapCoord>* = nullptr);
    static IDirectDrawSurface7* BitmapToSurface(IDirectDraw7* pDD, const CBitmap& bitmap);
    static void BlitTransparent(LPDIRECTDRAWSURFACE7 pic, int x, int y, int width = -1, int height = -1, BYTE alpha = 255, LPDIRECTDRAWSURFACE7 surface = nullptr);
    static void BlitTransparentDesc(LPDIRECTDRAWSURFACE7 pic, LPDIRECTDRAWSURFACE7 surface, DDSURFACEDESC2* pDestDesc,
        int x, int y, int width = -1, int height = -1, BYTE alpha = 255);
    static void BlitSHPTransparent(LPDDSURFACEDESC2 lpDesc, int x, int y, ImageDataClass* pd, Palette* newPal = NULL, BYTE alpha = 255, COLORREF houseColor = -1);
    static void BlitSHPTransparent(LPDDSURFACEDESC2 lpDesc, int x, int y, ImageDataClassSafe* pd, Palette* newPal = NULL, BYTE alpha = 255, COLORREF houseColor = -1);
    static void BlitSHPTransparent(CIsoView* pThis, void* dst, const RECT& window,
        const DDBoundary& boundary, int x, int y, ImageDataClass* pd, Palette* newPal = NULL, BYTE alpha = 255, COLORREF houseColor = -1, int extraLightType = -1, bool remap = false);
    static void BlitSHPTransparent(CIsoView* pThis, void* dst, const RECT& window,
        const DDBoundary& boundary, int x, int y, ImageDataClassSafe* pd, Palette* newPal = NULL, BYTE alpha = 255, COLORREF houseColor = -1, int extraLightType = -1, bool remap = false);
    static void BlitSHPTransparent_Building(CIsoView* pThis, void* dst, const RECT& window,
        const DDBoundary& boundary, int x, int y, ImageDataClassSafe* pd, Palette* newPal = NULL,
        BYTE alpha = 255, COLORREF houseColor = -1, COLORREF addOnColor = -1, bool isRubble = false, bool isTerrain = false);
    static void BlitSHPTransparent_AlphaImage(CIsoView* pThis, void* dst, const RECT& window,
        const DDBoundary& boundary, int x, int y, ImageDataClassSafe* pd);
    static void BlitTerrain(CIsoView* pThis, void* dst, const RECT& window,
        const DDBoundary& boundary, int x, int y, CTileBlockClass* subTile, Palette* pal, BYTE alpha = 255);
    static void BlitText(const std::wstring& text, COLORREF textColor, COLORREF bgColor,
        CIsoView* pThis, void* dst, const RECT& window, const DDBoundary& boundary,
        int x, int y, int fontSize = 20, BYTE alpha = 255, bool bold = false);
    static void MaskShadowPixels(const RECT& window, int x, int y, ImageDataClassSafe* pd, std::vector<char>& mask);
    static void DrawShadowMask(void* dst, const DDBoundary& boundary, const RECT& window, const std::vector<byte>& mask);
    static void ScaleBitmap(CBitmap* pBitmap, int maxSize, COLORREF bgColor, bool removeHalo = true, bool trim = true);
    static std::vector<MapCoord> GetTubePath(int x1, int y1, int x2, int y2, bool first = true);
    static std::vector<int> GetTubeDirections(const std::vector<MapCoord>& path);
    static std::vector<MapCoord> GetPathFromDirections(int x0, int y0, const std::vector<int>& directions);
    static RECT GetScaledWindowRect();
    static void ReduceBrightness(IDirectDrawSurface7* pSurface, const RECT& rc);

    // flatMode 0 = auto, 1 = flat, 2 = height
    static void MapCoord2ScreenCoord(int& X, int& Y, int flatMode = 0);
    static void DrawMouseMove(HDC hDC);
    static void DrawCopyBound(HDC hDC);
    static void DrawBridgeLine(HDC hDC);
    static void DrawLineHDC(HDC hDC, int x1, int y1, int x2, int y2, int color, int size = 0);
    static void DrawMultiMapCoordBorders(HDC hDC, const std::vector<MapCoord>& coords, COLORREF color);
    static void DrawMultiMapCoordBorders(LPDDSURFACEDESC2 lpDesc, const std::vector<MapCoord>& coords, COLORREF color);
    static void DrawMultiMapCoordBorders(LPDDSURFACEDESC2 lpDesc, const std::set<MapCoord>& coords, COLORREF color);
    static bool StretchCopySurfaceBilinear(LPDIRECTDRAWSURFACE7 srcSurface, CRect srcRect, LPDIRECTDRAWSURFACE7 dstSurface, CRect dstRect);
    static void SpecialDraw(LPDIRECTDRAWSURFACE7 surface, int specialDraw);
    static CRect GetVisibleIsoViewRect();
    static void DrawCreditOnMap(HDC hDC);
    static void DrawDistanceRuler(HDC hDC);
    static void MoveToMapCoord(int X, int Y);
    static void Zoom(double offset);
    static std::vector<MapCoord> GetLinePoints(MapCoord mc1, MapCoord mc2);
    static std::vector<MapCoord> GetLineRectangles(MapCoord start, MapCoord end, int width, int height);
    static void InitAlphaTable();

    static bool DrawStructures;
    static bool DrawInfantries;
    static bool DrawUnits;
    static bool DrawAircrafts;
    static bool DrawBasenodes;
    static bool DrawWaypoints;
    static bool DrawCelltags;
    static bool DrawMoneyOnMap;
    static bool DrawOverlays;
    static bool DrawTerrains;
    static bool DrawSmudges;
    static bool DrawTubes;
    static bool DrawBounds;
    static bool DrawVeterancy;
    static bool DrawShadows;
    static bool DrawAlphaImages;
    static bool DrawBaseNodeIndex;
    static bool RockCells;

    static bool PasteStructures;
    static bool PasteInfantries;
    static bool PasteUnits;
    static bool PasteAircrafts;
    static bool PasteOverlays;
    static bool PasteTerrains;
    static bool PasteSmudges;
    static bool PasteGround;
    static bool PasteOverriding;
    static bool PasteShowOutline;

    static bool DrawStructuresFilter;
    static bool DrawInfantriesFilter;
    static bool DrawUnitsFilter;
    static bool DrawAircraftsFilter;
    static bool DrawBasenodesFilter;
    static bool DrawCellTagsFilter;

    static bool AutoPropertyBrush[4];

    static COLORREF CellHilightColors[16];
    static float drawOffsetX;
    static float drawOffsetY;
    static Cell3DLocation CurrentDrawCellLocation;

    static std::unordered_set<short> VisibleStructures;
    static std::unordered_set<short> VisibleInfantries;
    static std::unordered_set<short> VisibleUnits;
    static std::unordered_set<short> VisibleAircrafts;
    
    static bool IsPressingALT;
    static bool IsPressingTube;
    static std::vector<MapCoord> TubeNodes;
    static ppmfc::CString CurrentCellObjectHouse;
    static int EXTRA_BORDER_BOTTOM;

    static LPDIRECTDRAWSURFACE7 lpDDBackBufferZoomSurface;
    static double ScaledFactor;
    static double ScaledMax;
    static double ScaledMin;

    static UINT nFlagsMove;

    static std::vector<MapCoord> DistanceRuler;
    static bool EnableDistanceRuler;

    static bool CliffBackAlt;
    static bool HistoryRecord_IsHoldingLButton;

    struct LastCommand
    {
        int Command = -1;
        int Type;
        int Param;
        int Overlay;
        int OverlayData;
        int Height;
        ppmfc::CString ObjectID;
        int X;
        int Y;
        int Subpos;
        static bool requestSubpos;

        void reset()
        {
            Command = -1;
        }

        void record(int x, int y)
        {
            Command = CIsoView::CurrentCommand->Command;
            Type = CIsoView::CurrentCommand->Type;
            Param = CIsoView::CurrentCommand->Param;
            Overlay = CIsoView::CurrentCommand->Overlay;
            OverlayData = CIsoView::CurrentCommand->OverlayData;
            Height = CIsoView::CurrentCommand->Height;
            ObjectID = CIsoView::CurrentCommand->ObjectID;
            X = x;
            Y = y;
            requestSubpos = false;
            Subpos = CIsoViewExt::GetSelectedSubcellInfantryIdx(x, y, true);
        }

        bool isValidCommand()
        {
            return
                Command == 1 || Command == 10 || Command == 22;
        }

        bool isSame()
        {
            if (CViewObjectsExt::PlacingRandomRock >= 0)
            {
                return
                    isValidCommand() &&
                    Command == CIsoView::CurrentCommand->Command &&
                    Type == CIsoView::CurrentCommand->Type &&
                    Param == CIsoView::CurrentCommand->Param &&
                    OverlayData == CIsoView::CurrentCommand->OverlayData &&
                    Height == CIsoView::CurrentCommand->Height &&
                    ObjectID == CIsoView::CurrentCommand->ObjectID;
            }
            else if (CViewObjectsExt::PlacingRandomSmudge >= 0 
                || CViewObjectsExt::PlacingRandomTerrain >= 0
                || CViewObjectsExt::PlacingRandomInfantry >= 0
                || CViewObjectsExt::PlacingRandomVehicle >= 0
                || CViewObjectsExt::PlacingRandomStructure >= 0
                || CViewObjectsExt::PlacingRandomAircraft >= 0
                )
            {
                return
                    isValidCommand() &&
                    Command == CIsoView::CurrentCommand->Command &&
                    Type == CIsoView::CurrentCommand->Type &&
                    Param == CIsoView::CurrentCommand->Param &&
                    Overlay == CIsoView::CurrentCommand->Overlay &&
                    OverlayData == CIsoView::CurrentCommand->OverlayData &&
                    Height == CIsoView::CurrentCommand->Height;
            }
            return
                isValidCommand() &&
                Command == CIsoView::CurrentCommand->Command &&
                Type == CIsoView::CurrentCommand->Type &&
                Param == CIsoView::CurrentCommand->Param &&
                Overlay == CIsoView::CurrentCommand->Overlay &&
                OverlayData == CIsoView::CurrentCommand->OverlayData &&
                Height == CIsoView::CurrentCommand->Height &&
                ObjectID == CIsoView::CurrentCommand->ObjectID;
        }
    };

    static LastCommand LastAltCommand;

};