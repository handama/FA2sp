#include "Body.h"

#include "../../FA2sp.h"

#include <WindowsX.h>
#include <CPalette.h>

#include <CMapData.h>
#include <Drawing.h>
#include <CINI.h>

#include <ranges>
#include "../../Helpers/STDHelpers.h"
#include "../CMapData/Body.h"
#include "../../Helpers/Translations.h"
#include "../../Miscs/MultiSelection.h"
#include "../../Miscs/Palettes.h"
#include "../CLoading/Body.h"
#include <immintrin.h>
#include <mutex>

bool CIsoViewExt::DrawStructures = true;
bool CIsoViewExt::DrawInfantries = true;
bool CIsoViewExt::DrawUnits = true;
bool CIsoViewExt::DrawAircrafts = true;
bool CIsoViewExt::DrawBasenodes = true;
bool CIsoViewExt::DrawWaypoints = true;
bool CIsoViewExt::DrawCelltags = true;
bool CIsoViewExt::DrawMoneyOnMap = true;
bool CIsoViewExt::DrawOverlays = true;
bool CIsoViewExt::DrawTerrains = true;
bool CIsoViewExt::DrawSmudges = true;
bool CIsoViewExt::DrawTubes = true;
bool CIsoViewExt::DrawBounds = true;
bool CIsoViewExt::DrawVeterancy = true;
bool CIsoViewExt::DrawShadows = true;
bool CIsoViewExt::DrawAlphaImages = true;
bool CIsoViewExt::DrawBaseNodeIndex = true;
bool CIsoViewExt::RockCells = false;

bool CIsoViewExt::PasteStructures = false;
bool CIsoViewExt::PasteInfantries = false;
bool CIsoViewExt::PasteUnits = false;
bool CIsoViewExt::PasteAircrafts = false;
bool CIsoViewExt::PasteOverlays = true;
bool CIsoViewExt::PasteTerrains = false;
bool CIsoViewExt::PasteSmudges = false;
bool CIsoViewExt::PasteGround = true;
bool CIsoViewExt::PasteOverriding = false;
bool CIsoViewExt::PasteShowOutline = false;

bool CIsoViewExt::DrawStructuresFilter = false;
bool CIsoViewExt::DrawInfantriesFilter = false;
bool CIsoViewExt::DrawUnitsFilter = false;
bool CIsoViewExt::DrawAircraftsFilter = false;
bool CIsoViewExt::DrawBasenodesFilter = false;
bool CIsoViewExt::DrawCellTagsFilter = false;

bool CIsoViewExt::AutoPropertyBrush[4] = { false };
bool CIsoViewExt::IsPressingALT = false;
bool CIsoViewExt::IsPressingTube = false;
bool CIsoViewExt::EnableDistanceRuler = false;
bool CIsoViewExt::CliffBackAlt = false;
bool CIsoViewExt::HistoryRecord_IsHoldingLButton = false;
std::vector<MapCoord> CIsoViewExt::TubeNodes;
std::vector<MapCoord> CIsoViewExt::DistanceRuler;
ppmfc::CString CIsoViewExt::CurrentCellObjectHouse = "";
int CIsoViewExt::EXTRA_BORDER_BOTTOM = 25;
Cell3DLocation CIsoViewExt::CurrentDrawCellLocation;

float CIsoViewExt::drawOffsetX;
float CIsoViewExt::drawOffsetY;

LPDIRECTDRAWSURFACE7 CIsoViewExt::lpDDBackBufferZoomSurface;
double CIsoViewExt::ScaledFactor = 1.0;
double CIsoViewExt::ScaledMax = 1.5;
double CIsoViewExt::ScaledMin = 0.25;
UINT CIsoViewExt::nFlagsMove;
bool CIsoViewExt::LastCommand::requestSubpos = false;
CIsoViewExt::LastCommand CIsoViewExt::LastAltCommand;

COLORREF CIsoViewExt::CellHilightColors[16] = {
    RGB(255, 255, 255),	// level 0
    RGB(170, 0, 170),	// level 1
    RGB(0, 170, 170),	// level 2
    RGB(0, 170, 0),		// level 3
    RGB(90, 255, 90),	// level 4
    RGB(255, 255, 90),	// level 5
    RGB(255, 50, 50),	// level 6
    RGB(170, 85, 0),	// level 7
    RGB(170, 0, 0),		// level 8
    RGB(85, 255, 255),	// level 9
    RGB(80, 80, 255),	// level 10
    RGB(0, 0, 170),		// level 11
    RGB(0, 0, 0),		// level 12
    RGB(85,85 ,85),		// level 13
    RGB(170, 170, 170),	// level 14
    RGB(255, 255, 255)	// level 15
};

static bool TilePixels[1800] =
{
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false,
        false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false,
        false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false,
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false,
        false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false,
        false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
};

static BYTE alphaBlendTable[256][256];

constexpr float kFactor = 0.7f;
constexpr BYTE MakeValue(int i) {
    int v = static_cast<int>(i * kFactor);
    return v > 255 ? 255 : static_cast<BYTE>(v);
}
constexpr auto MakeBrightnessLUT() {
    std::array<BYTE, 256> lut{};
    for (int i = 0; i < 256; ++i) {
        lut[i] = MakeValue(i);
    }
    return lut;
}
constexpr std::array<BYTE, 256> BrightnessLUT = MakeBrightnessLUT();

void CIsoViewExt::InitAlphaTable() {
    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 256; j++) {
            alphaBlendTable[i][j] = static_cast<BYTE>((i * j) >> 8);
        }
    }
}

void CIsoViewExt::ProgramStartupInit()
{
    // RunTime::ResetMemoryContentAt(0x594518, CIsoViewExt::PreTranslateMessageExt);
}

void CIsoViewExt::ConfirmTube(bool addReverse)
{
    auto& nodes = CIsoViewExt::TubeNodes;
    if (nodes.size() < 2)
        return;

    auto GetOppositeFacing = [](int nFacing) -> char
        {
            switch (nFacing)
            {
            case FACING_NORTHEAST:
                return FACING_SOUTHWEST;

            case FACING_EAST:
                return FACING_WEST;

            case FACING_SOUTHEAST:
                return FACING_NORTHWEST;

            case FACING_SOUTH:
                return FACING_NORTH;

            case FACING_SOUTHWEST:
                return FACING_NORTHEAST;

            case FACING_WEST:
                return FACING_EAST;

            case FACING_NORTHWEST:
                return FACING_SOUTHEAST;

            case FACING_NORTH:
                return FACING_SOUTH;

            default:
            case FACING_INVALID:
                return FACING_INVALID;
            }
        };

    std::vector<int> AllDirections;
    for (int j = 0; j < nodes.size() - 1; ++j)
    {
        int x1, x2, y1, y2;
        x1 = nodes[j].X;
        y1 = nodes[j].Y;
        x2 = nodes[j + 1].X;
        y2 = nodes[j + 1].Y;
        auto path = CIsoViewExt::GetTubePath(x1, y1, x2, y2, j == 0);
        auto directions = CIsoViewExt::GetTubeDirections(path);
        AllDirections.insert(AllDirections.end(), directions.begin(), directions.end());
    }
    if (AllDirections.size() > 99)
    {
        FString pMessage = Translations::TranslateOrDefault("ErrorTooLongTube",
            "Cannot generate a too long tube!");
        ::MessageBox(NULL, pMessage, Translations::TranslateOrDefault("Error", "Error"), MB_OK);
        return;
    }
    CMapDataExt::MakeObjectRecord(ObjectRecord::RecordType::Tunnel);

    auto key = CINI::GetAvailableKey("Tubes");
    FString value;
    value.Format("%d,%d,%d,%d,%d", nodes.front().Y, nodes.front().X, AllDirections.front(), nodes.back().Y, nodes.back().X);
    for (int i = 0; i < AllDirections.size(); ++i)
    {
        FString direc;
        direc.Format(",%d", AllDirections[i]);
        value += direc;
    }
    value += ",-1";
    CINI::CurrentDocument->WriteString("Tubes", key, value);

    if (addReverse)
    {
        key = CINI::GetAvailableKey("Tubes");
        value.Format("%d,%d,%d,%d,%d", nodes.back().Y, nodes.back().X, GetOppositeFacing(AllDirections.back()), nodes.front().Y, nodes.front().X);
        for (int i = AllDirections.size() - 1; i >= 0; --i)
        {
            FString direc;
            direc.Format(",%d", GetOppositeFacing(AllDirections[i]));
            value += direc;
        }
        value += ",-1";
        CINI::CurrentDocument->WriteString("Tubes", key, value);
    }

    CMapData::Instance->UpdateFieldTubeData(false);
}

void CIsoViewExt::DrawLockedCellOutlineX(int X, int Y, int W, int H, COLORREF color, COLORREF colorX, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc, bool onlyX)
{
    X += 3;
    Y += 1;
    if (lpDesc->lpSurface == nullptr)
        return;

    RECT rect = CIsoViewExt::GetScaledWindowRect();

    auto lPitch = lpDesc->lPitch;
    auto nBytesPerPixel = *(int*)0x72A8C0;

    auto pRGB = (ColorStruct*)&color;
    BGRStruct ddColor;
    ddColor.R = pRGB->red;
    ddColor.G = pRGB->green;
    ddColor.B = pRGB->blue;

    auto pRGB2 = (ColorStruct*)&colorX;
    BGRStruct ddColor2;
    ddColor2.R = pRGB2->red;
    ddColor2.G = pRGB2->green;
    ddColor2.B = pRGB2->blue;

    auto DrawLine = [lPitch, nBytesPerPixel, ddColor, lpDesc, &rect](int X1, int Y1, int X2, int Y2)
        {
            int color = *(int*)&ddColor;

            if (X1 > X2)
            {
                std::swap(X1, X2);
                std::swap(Y1, Y2);
            }

            int dx = X2 - X1;
            int dy = Y2 - Y1;

            auto ptr = (unsigned char*)lpDesc->lpSurface + lPitch * Y1 + X1 * nBytesPerPixel;

            if (dy == 0)
            {
                for (int i = 0; i <= dx; ++i)
                {
                    memcpy(ptr, &ddColor, nBytesPerPixel);
                    ptr += nBytesPerPixel;
                }
            }
            else if (dx == 0)
            {
                int pitch = lPitch;
                if (dy < 0)
                {
                    pitch = -pitch;
                    dy = -dy;
                }

                for (int i = 0; i <= dy; ++i)
                {
                    memcpy(ptr, &ddColor, nBytesPerPixel);
                    ptr += pitch;
                }
            }
            else
            {
                int pitch = lPitch;
                if (dy < 0)
                {
                    pitch = -pitch;
                    dy = -dy;
                }

                int dx2 = 2 * dx;
                int dy2 = 2 * dy;

                if (dx > dy)
                {
                    int delta = dy2 - dx;
                    for (int i = 0; i < dx; ++i)
                    {
                        memcpy(ptr + i * nBytesPerPixel, &ddColor, nBytesPerPixel);
                        if (delta > 0)
                        {
                            ptr += pitch;
                            delta -= dx2;
                        }
                        delta += dy2;
                    }
                }
                else
                {
                    int delta = dx2 - dy;
                    int k = 0;

                    for (int i = 0; i < dy; ++i)
                    {
                        memcpy(ptr + k * nBytesPerPixel, &ddColor, nBytesPerPixel);
                        if (delta > 0)
                        {
                            ++k;
                            delta -= dy2;
                        }
                        delta += dx2;
                        ptr += pitch;
                    }
                }
            }
        };

    auto DrawLine2 = [lPitch, nBytesPerPixel, ddColor2, lpDesc, &rect](int X1, int Y1, int X2, int Y2)
        {
            int color = *(int*)&ddColor2;

            if (X1 > X2)
            {
                std::swap(X1, X2);
                std::swap(Y1, Y2);
            }

            int dx = X2 - X1;
            int dy = Y2 - Y1;

            auto ptr = (unsigned char*)lpDesc->lpSurface + lPitch * Y1 + X1 * nBytesPerPixel;

            if (dy == 0)
            {
                for (int i = 0; i <= dx; ++i)
                {
                    memcpy(ptr, &ddColor2, nBytesPerPixel);
                    ptr += nBytesPerPixel;
                }
            }
            else if (dx == 0)
            {
                int pitch = lPitch;
                if (dy < 0)
                {
                    pitch = -pitch;
                    dy = -dy;
                }

                for (int i = 0; i <= dy; ++i)
                {
                    memcpy(ptr, &ddColor2, nBytesPerPixel);
                    ptr += pitch;
                }
            }
            else
            {
                int pitch = lPitch;
                if (dy < 0)
                {
                    pitch = -pitch;
                    dy = -dy;
                }

                int dx2 = 2 * dx;
                int dy2 = 2 * dy;

                if (dx > dy)
                {
                    int delta = dy2 - dx;
                    for (int i = 0; i < dx; ++i)
                    {
                        memcpy(ptr + i * nBytesPerPixel, &ddColor2, nBytesPerPixel);
                        if (delta > 0)
                        {
                            ptr += pitch;
                            delta -= dx2;
                        }
                        delta += dy2;
                    }
                }
                else
                {
                    int delta = dx2 - dy;
                    int k = 0;

                    for (int i = 0; i < dy; ++i)
                    {
                        memcpy(ptr + k * nBytesPerPixel, &ddColor2, nBytesPerPixel);
                        if (delta > 0)
                        {
                            ++k;
                            delta -= dy2;
                        }
                        delta += dx2;
                        ptr += pitch;
                    }
                }
            }
        };
    auto ClipAndDrawLine2 = [&rect, DrawLine2](int X1, int Y1, int X2, int Y2)
        {
            auto encode = [&rect](int x, int y)
                {
                    int c = 0;
                    if (x < rect.left) c = c | 0x1;
                    else if (x > rect.right) c = c | 0x2;
                    if (y > rect.bottom) c = c | 0x4;
                    else if (y < rect.top) c = c | 0x8;
                    return c;
                };
            auto clip = [&rect, encode](int& X1, int& Y1, int& X2, int& Y2) -> bool
                {
                    int code1, code2, code;
                    int x = 0, y = 0;
                    code1 = encode(X1, Y1);
                    code2 = encode(X2, Y2);
                    while (code1 != 0 || code2 != 0)
                    {
                        if ((code1 & code2) != 0) return false;
                        code = code1;
                        if (code == 0) code = code2;
                        if ((0b1 & code) != 0)
                        {
                            x = rect.left;
                            y = Y1 + (Y2 - Y1) * (rect.left - X1) / (X2 - X1);
                        }
                        else if ((0b10 & code) != 0)
                        {
                            x = rect.right;
                            y = Y1 + (Y2 - Y1) * (rect.right - X1) / (X2 - X1);
                        }
                        else if ((0b100 & code) != 0)
                        {
                            y = rect.bottom;
                            x = X1 + (X2 - X1) * (rect.bottom - Y1) / (Y2 - Y1);
                        }
                        else if ((0b1000 & code) != 0)
                        {
                            y = rect.top;
                            x = X1 + (X2 - X1) * (rect.top - Y1) / (Y2 - Y1);
                        }
                        if (code == code1)
                        {
                            X1 = x;
                            Y1 = y;
                            code1 = encode(x, y);
                        }
                        else
                        {
                            X2 = x;
                            Y2 = y;
                            code2 = encode(x, y);
                        }
                    }
                    return true;
                };
            if (clip(X1, Y1, X2, Y2))
                DrawLine2(X1, Y1, X2, Y2);
        };
    auto ClipAndDrawLine = [&rect, DrawLine](int X1, int Y1, int X2, int Y2)
        {
            auto encode = [&rect](int x, int y)
                {
                    int c = 0;
                    if (x < rect.left) c = c | 0x1;
                    else if (x > rect.right) c = c | 0x2;
                    if (y > rect.bottom) c = c | 0x4;
                    else if (y < rect.top) c = c | 0x8;
                    return c;
                };
            auto clip = [&rect, encode](int& X1, int& Y1, int& X2, int& Y2) -> bool
                {
                    int code1, code2, code;
                    int x = 0, y = 0;
                    code1 = encode(X1, Y1);
                    code2 = encode(X2, Y2);
                    while (code1 != 0 || code2 != 0)
                    {
                        if ((code1 & code2) != 0) return false;
                        code = code1;
                        if (code == 0) code = code2;
                        if ((0b1 & code) != 0)
                        {
                            x = rect.left;
                            y = Y1 + (Y2 - Y1) * (rect.left - X1) / (X2 - X1);
                        }
                        else if ((0b10 & code) != 0)
                        {
                            x = rect.right;
                            y = Y1 + (Y2 - Y1) * (rect.right - X1) / (X2 - X1);
                        }
                        else if ((0b100 & code) != 0)
                        {
                            y = rect.bottom;
                            x = X1 + (X2 - X1) * (rect.bottom - Y1) / (Y2 - Y1);
                        }
                        else if ((0b1000 & code) != 0)
                        {
                            y = rect.top;
                            x = X1 + (X2 - X1) * (rect.top - Y1) / (Y2 - Y1);
                        }
                        if (code == code1)
                        {
                            X1 = x;
                            Y1 = y;
                            code1 = encode(x, y);
                        }
                        else
                        {
                            X2 = x;
                            Y2 = y;
                            code2 = encode(x, y);
                        }
                    }
                    return true;
                };
            if (clip(X1, Y1, X2, Y2))
                DrawLine(X1, Y1, X2, Y2);
        };

    int halfCellWidth = 30 * W;
    int quaterCellWidth = 15 * W;
    int fullCellHeight = 30 * H;
    int halfCellHeight = 15 * H;

    int y1 = Y - 30;
    int x1 = X + 30;

    int x2 = halfCellWidth + X + 30 - 2;
    int y2 = quaterCellWidth + y1 - 1;

    int x3 = halfCellWidth - fullCellHeight + X + 29;
    int y3 = halfCellHeight + quaterCellWidth + y1 - 1;

    int x4 = X - fullCellHeight + 29;
    int y4 = halfCellHeight + y1 - 1;

    y1 -= 1;
    x1 -= 1;
    int x1L = x1 + 1;
    int x3L = x3 + 1;
    int y1L = y1 - 1;
    int y3L = y3 + 1;

    int x4B = x4;
    int y4B = y4;
    int x2T = x2 + 2;
    int y2T = y2 + 1;
    x4 -= 1;

    //   1
    //  # #
    // 4   2
    //  # #
    //   3

    auto drawCellOutline = [&](int inneroffset)
        {
            ClipAndDrawLine(x1, y1 + inneroffset, x1, y3 - inneroffset);
            ClipAndDrawLine(x1 - 1, y1 + inneroffset, x1 - 1, y3 - inneroffset);
            ClipAndDrawLine(x4 + 2 * inneroffset, y4, x2 - 2 * inneroffset, y4);

            ClipAndDrawLine(x1, y1 + inneroffset, x2T - 2 * inneroffset, y2T);
            ClipAndDrawLine(x2 - 2 * inneroffset, y2, x3, y3 - inneroffset);
            ClipAndDrawLine(x3L, y3L - inneroffset, x4B + 2 * inneroffset, y4B);
            ClipAndDrawLine(x4 + 2 * inneroffset, y4, x1L, y1L + inneroffset);
        };
    

    auto drawCellOutline2 = [&](int inneroffset)
        {
            ClipAndDrawLine2(x1 + 1, y1 + 1 + inneroffset, x1 + 1, y3 - inneroffset + 1);
            ClipAndDrawLine2(x1 + 1 - 1, y1 + 1 + inneroffset, x1 + 1 - 1, y3 - inneroffset + 1);
            ClipAndDrawLine2(x4 + 1 + 2 * inneroffset, y4 + 1, x2 + 1 - 2 * inneroffset, y4 + 1);

            ClipAndDrawLine2(x1 + 1, y1 + 1 + inneroffset, x2T + 1 - 2 * inneroffset, y2T + 1);
            ClipAndDrawLine2(x2 + 1 - 2 * inneroffset, y2 + 1, x3 + 1, y3 - inneroffset + 1);
            ClipAndDrawLine2(x3L + 1, y3L + 1 - inneroffset, x4B + 1 + 2 * inneroffset, y4B + 1);
            ClipAndDrawLine2(x4 + 1 + 2 * inneroffset, y4 + 1, x1L + 1, y1L + inneroffset + 1);
        };

    if (onlyX)
    {
        ClipAndDrawLine(x1, y1, x1, y3);
        ClipAndDrawLine(x4, y4, x2 , y4);
    }
    else
    {
        drawCellOutline2(0);
        drawCellOutline2(-1);

        drawCellOutline(0);
        drawCellOutline(-1);
    }
}

void CIsoViewExt::DrawLockedCellOutline(int X, int Y, int W, int H, COLORREF color, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc, bool s1, bool s2, bool s3, bool s4)
{   
    X += 2;
    Y += 1;
    if (lpDesc->lpSurface == nullptr)
        return;

    RECT rect = CIsoViewExt::GetScaledWindowRect();

    auto lPitch = lpDesc->lPitch;
    auto nBytesPerPixel = *(int*)0x72A8C0;

    auto pRGB = (ColorStruct*)&color;
    BGRStruct ddColor;
    ddColor.R = pRGB->red;
    ddColor.G = pRGB->green;
    ddColor.B = pRGB->blue;

    auto DrawLine = [lPitch, nBytesPerPixel, ddColor, lpDesc, &rect](int X1, int Y1, int X2, int Y2)
    {
        int color = *(int*)&ddColor;

        if (X1 > X2) 
        {
            std::swap(X1, X2);
            std::swap(Y1, Y2);
        }

        int dx = X2 - X1;
        int dy = Y2 - Y1;

        auto ptr = (unsigned char*)lpDesc->lpSurface + lPitch * Y1 + X1 * nBytesPerPixel;

        if (dy == 0) 
        {
            for (int i = 0; i <= dx; ++i) 
            {
                memcpy(ptr, &ddColor, nBytesPerPixel);
                ptr += nBytesPerPixel;
            }
        }
        else if (dx == 0) 
        {
            int pitch = lPitch;
            if (dy < 0)
            {
                pitch = -pitch;
                dy = -dy;
            }

            for (int i = 0; i <= dy; ++i) 
            {
                memcpy(ptr, &ddColor, nBytesPerPixel);
                ptr += pitch;
            }
        }
        else 
        {
            int pitch = lPitch;
            if (dy < 0)
            {
                pitch = -pitch;
                dy = -dy;
            }

            int dx2 = 2 * dx;
            int dy2 = 2 * dy;

            if (dx > dy) 
            {
                int delta = dy2 - dx;
                for (int i = 0; i < dx; ++i) 
                {
                    memcpy(ptr + i * nBytesPerPixel, &ddColor, nBytesPerPixel);
                    if (delta > 0)
                    {
                        ptr += pitch;
                        delta -= dx2;
                    }
                    delta += dy2;
                }
            }
            else 
            {
                int delta = dx2 - dy;
                int k = 0;

                for (int i = 0; i < dy; ++i)
                {
                    memcpy(ptr + k * nBytesPerPixel, &ddColor, nBytesPerPixel);
                    if (delta > 0) 
                    {
                        ++k;
                        delta -= dy2;
                    }
                    delta += dx2;
                    ptr += pitch;
                }
            }
        }
    };
    auto ClipAndDrawLine = [&rect, DrawLine](int X1, int Y1, int X2, int Y2)
    {
        auto encode = [&rect](int x, int y)
        {
            int c = 0;
            if (x < rect.left) c = c | 0x1;
            else if (x > rect.right) c = c | 0x2;
            if (y > rect.bottom) c = c | 0x4;
            else if (y < rect.top) c = c | 0x8;
            return c;
        };
        auto clip = [&rect, encode](int& X1, int& Y1, int& X2, int& Y2) -> bool
        {
            int code1, code2, code;
            int x = 0, y = 0;
            code1 = encode(X1, Y1);
            code2 = encode(X2, Y2);
            while (code1 != 0 || code2 != 0)
            {
                if ((code1 & code2) != 0) return false;
                code = code1;
                if (code == 0) code = code2;
                if ((0b1 & code) != 0)
                {
                    x = rect.left;
                    y = Y1 + (Y2 - Y1) * (rect.left - X1) / (X2 - X1);
                }
                else if ((0b10 & code) != 0)
                {
                    x = rect.right;
                    y = Y1 + (Y2 - Y1) * (rect.right - X1) / (X2 - X1);
                }
                else if ((0b100 & code) != 0)
                {
                    y = rect.bottom;
                    x = X1 + (X2 - X1) * (rect.bottom - Y1) / (Y2 - Y1);
                }
                else if ((0b1000 & code) != 0)
                {
                    y = rect.top;
                    x = X1 + (X2 - X1) * (rect.top - Y1) / (Y2 - Y1);
                }
                if (code == code1) 
                {
                    X1 = x;
                    Y1 = y;
                    code1 = encode(x, y);
                }
                else 
                {
                    X2 = x;
                    Y2 = y;
                    code2 = encode(x, y);
                }
            }
            return true;
        };
        if (clip(X1, Y1, X2, Y2))
            DrawLine(X1, Y1, X2, Y2);
    };

    int halfCellWidth = 30 * W;
    int quaterCellWidth = 15 * W;
    int fullCellHeight = 30 * H;
    int halfCellHeight = 15 * H;

    int y1 = Y - 30;
    int x1 = X + 30;

    int x2 = halfCellWidth + X + 30;
    int y2 = quaterCellWidth + y1 - 2;

    int x3 = halfCellWidth - fullCellHeight + X + 29;
    int y3 = halfCellHeight + quaterCellWidth + y1 - 1;

    int x4 = X - fullCellHeight + 29;
    int y4 = halfCellHeight + y1 - 1;

    y1 -= 1;
    x1 -= 1;
    int x1L = x1 + 1;
    int x3L = x3 + 1;
    int y1L = y1 - 1;
    int y3L = y3 + 1;

    int x4B = x4;
    int y4B = y4;
    int x2T = x2;
    int y2T = y2 + 2;
    x4 -= 1;

    //   1
    //  # #
    // 4   2
    //  # #
    //   3


    if (s1)
        ClipAndDrawLine(x1, y1, x2T, y2T);
    if (s2)
        ClipAndDrawLine(x2, y2, x3, y3);
    if (s3)
        ClipAndDrawLine(x3L, y3L, x4B, y4B);
    if (s4)
        ClipAndDrawLine(x4, y4, x1L, y1L);

    
    // thicker
    if (!bUseDot)
    {
        if (s1)
            ClipAndDrawLine(x1 - 1, y1 + 2, x2T - 2, y2T + 1);
        if (s2)
            ClipAndDrawLine(x2 - 2, y2 - 1, x3 - 2, y3 - 1);
        if (s3)
            ClipAndDrawLine(x3L, y3L - 2, x4B + 1, y4B - 1);
        if (s4)
            ClipAndDrawLine(x4 + 4, y4, x1L + 2, y1L + 1);
        

        if (s1)
            ClipAndDrawLine(x1 - 1, y1 + 1, x2T - 2, y2T);
        if (s2)
            ClipAndDrawLine(x2 - 2, y2, x3 - 1, y3 - 1);
        if (s3)
            ClipAndDrawLine(x3L, y3L - 1, x4B + 1, y4B);
        if (s4)
            ClipAndDrawLine(x4 + 2, y4, x1L, y1L + 1);
    }

}

void CIsoViewExt::DrawLockedCellOutlinePaintCursor(int X, int Y, int height, COLORREF color, HDC hdc, HWND hwnd, bool useHeightColor)
{   
    X += 6 / CIsoViewExt::ScaledFactor - 6 + 2;
    Y += 3 / CIsoViewExt::ScaledFactor - 3 + 1;
    if (!hdc)
        return;
    if (!hwnd)
        return;

    CRect rect;
    auto pThis = CIsoView::GetInstance();
    pThis->GetWindowRect(&rect);

    COLORREF heightColor = color;
    if (useHeightColor)
    {
        heightColor = CIsoViewExt::CellHilightColors[height];
    }

    auto DrawLine = [hwnd, hdc, &color](int X1, int Y1, int X2, int Y2, int dashLen, int gapLen)
        {
            float dx = static_cast<float>(X2 - X1);
            float dy = static_cast<float>(Y2 - Y1);
            float lineLength = std::sqrt(dx * dx + dy * dy);

            float ux = dx / lineLength;
            float uy = dy / lineLength;

            float totalDrawn = 0.0f;

            HPEN hPen = CreatePen(PS_SOLID, 1, color);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

            while (totalDrawn < lineLength)
            {
                float startX = X1 + ux * totalDrawn;
                float startY = Y1 + uy * totalDrawn;

                float drawLength = std::min((float)dashLen, lineLength - totalDrawn);
                float endX = startX + ux * drawLength;
                float endY = startY + uy * drawLength;

                MoveToEx(hdc, static_cast<int>(startX + 0.5f), static_cast<int>(startY + 0.5f), NULL);
                LineTo(hdc, static_cast<int>(endX + 0.5f), static_cast<int>(endY + 0.5f));

                totalDrawn += drawLength + gapLen;
            }

            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
        };
    auto DrawLineInner = [hwnd, heightColor, hdc, &rect](int X1, int Y1, int X2, int Y2)
    {
        PAINTSTRUCT ps;
        HPEN hPen;
        HPEN hPenOld;
        BeginPaint(hwnd, &ps);
        hPen = CreatePen(PS_SOLID, 0, heightColor);
        hPenOld = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, X1, Y1, NULL);
        LineTo(hdc, X2, Y2);
        SelectObject(hdc, hPenOld);
        DeleteObject(hPen);
        EndPaint(hwnd, &ps);
        
    };
    auto ClipAndDrawLine = [&rect, DrawLine, DrawLineInner](int X1, int Y1, int X2, int Y2, int type)
    {
        auto encode = [&rect](int x, int y)
        {
            int c = 0;
            if (x < rect.left) c = c | 0x1;
            else if (x > rect.right) c = c | 0x2;
            if (y > rect.bottom) c = c | 0x4;
            else if (y < rect.top) c = c | 0x8;
            return c;
        };
        auto clip = [&rect, encode](int& X1, int& Y1, int& X2, int& Y2) -> bool
        {
            int code1, code2, code;
            int x = 0, y = 0;
            code1 = encode(X1, Y1);
            code2 = encode(X2, Y2);
            while (code1 != 0 || code2 != 0)
            {
                if ((code1 & code2) != 0) return false;
                code = code1;
                if (code == 0) code = code2;
                if ((0b1 & code) != 0)
                {
                    x = rect.left;
                    y = Y1 + (Y2 - Y1) * (rect.left - X1) / (X2 - X1);
                }
                else if ((0b10 & code) != 0)
                {
                    x = rect.right;
                    y = Y1 + (Y2 - Y1) * (rect.right - X1) / (X2 - X1);
                }
                else if ((0b100 & code) != 0)
                {
                    y = rect.bottom;
                    x = X1 + (X2 - X1) * (rect.bottom - Y1) / (Y2 - Y1);
                }
                else if ((0b1000 & code) != 0)
                {
                    y = rect.top;
                    x = X1 + (X2 - X1) * (rect.top - Y1) / (Y2 - Y1);
                }
                if (code == code1) 
                {
                    X1 = x;
                    Y1 = y;
                    code1 = encode(x, y);
                }
                else 
                {
                    X2 = x;
                    Y2 = y;
                    code2 = encode(x, y);
                }
            }
            return true;
        };
        if (clip(X1, Y1, X2, Y2))
        {
            if (type == 0)
                DrawLine(X1, Y1, X2, Y2, 1000, 0);
            else if (type == 1)
                DrawLineInner(X1, Y1, X2, Y2);
            else if (type == 2)
                DrawLine(X1, Y1, X2, Y2, std::max(4 / CIsoViewExt::ScaledFactor, 1.0), std::max(2 / CIsoViewExt::ScaledFactor, 1.0));
        }
           
    };

    double halfCellWidth = 30 / CIsoViewExt::ScaledFactor;
    double quaterCellWidth = 15 / CIsoViewExt::ScaledFactor;
    double fullCellHeight = 30 / CIsoViewExt::ScaledFactor;
    double halfCellHeight = 15 / CIsoViewExt::ScaledFactor;

    double y1 = Y - 30 / CIsoViewExt::ScaledFactor;
    double x1 = X + 30 / CIsoViewExt::ScaledFactor;

    double x2 = halfCellWidth + X + 30 / CIsoViewExt::ScaledFactor - 2;
    double y2 = quaterCellWidth + y1 - 1;

    double x3 = halfCellWidth - fullCellHeight + X + 30 / CIsoViewExt::ScaledFactor - 1;
    double y3 = halfCellHeight + quaterCellWidth + y1 - 1;

    double x4 = X - fullCellHeight + 30 / CIsoViewExt::ScaledFactor - 1;
    double y4 = halfCellHeight + y1 - 1;

    y1 -= 1;
    x1 -= 1;
    double x1L = x1 + 1;
    double x3L = x3 - 1;
    double y1L = y1 - 1;
    double y3L = y3;
    y3 += 1;
    x3 -= 2;
    double x4B = x4 - 2;
    double y4B = y4 - 1;
    double x2T = x2 + 2;
    double y2T = y2 + 1;

    //   1
    //  # #
    // 4   2
    //  # #
    //   3

    
    auto drawCellOutline = [&](int inneroffset, bool useheightcolor = false)
        {
            ClipAndDrawLine(x1, y1 + inneroffset, x2T - 2 * inneroffset, y2T, useheightcolor);
            ClipAndDrawLine(x2 - 2 * inneroffset, y2, x3, y3 - inneroffset, useheightcolor);
            ClipAndDrawLine(x3L, y3L - inneroffset, x4B + 2 * inneroffset, y4B, useheightcolor);
            ClipAndDrawLine(x4 + 2 * inneroffset, y4, x1L, y1L + inneroffset, useheightcolor);
        };
    drawCellOutline(0);
    drawCellOutline(1,true);
    if (CIsoViewExt::ScaledFactor < 0.76)
        drawCellOutline(2, true);
    if (CIsoViewExt::ScaledFactor < 0.31)
        drawCellOutline(3, true);

    if (useHeightColor)
    {
        drawCellOutline(-1);
        if (CIsoViewExt::ScaledFactor < 0.6)
            drawCellOutline(-2);
        if (CIsoViewExt::ScaledFactor < 0.31)
            drawCellOutline(-3);
    }

    color = ExtConfigs::CursorSelectionBound_HeightColor;
    auto drawHeightLine = [&](int offset)
        {
            ClipAndDrawLine(x2 + offset, y2, x2 + offset, y2 + height * 15 / CIsoViewExt::ScaledFactor, 2);
            ClipAndDrawLine(x4 - offset, y4, x4 - offset, y4 + height * 15 / CIsoViewExt::ScaledFactor, 2);
            ClipAndDrawLine(x3 + offset + 1, y3, x3 + offset + 1, y3 + height * 15 / CIsoViewExt::ScaledFactor, 2);
        };

    if (!CFinalSunApp::Instance->FlatToGround)
    {
        drawHeightLine(0);
        if (CIsoViewExt::ScaledFactor < 0.76)
            drawHeightLine(1);
        if (CIsoViewExt::ScaledFactor < 0.31)
            drawHeightLine(-1);
    }
}

void CIsoViewExt::DrawLockedCellOutlinePaint(int X, int Y, int W, int H, COLORREF color, bool bUseDot, HDC hdc, HWND hwnd, bool s1, bool s2, bool s3, bool s4)
{   
    if (!s1 && !s2 && !s3 && !s4)
        return;

    X += 6 / CIsoViewExt::ScaledFactor - 6 + 2;
    Y += 3 / CIsoViewExt::ScaledFactor - 3 + 1;
    if (!hdc)
        return;
    if (!hwnd)
        return;

    CRect rect;
    auto pThis = CIsoView::GetInstance();
    pThis->GetWindowRect(&rect);

    auto DrawLine = [hwnd, color, hdc, &rect](int X1, int Y1, int X2, int Y2)
    {
        PAINTSTRUCT ps;
        HPEN hPen;
        HPEN hPenOld;
        BeginPaint(hwnd, &ps);
        hPen = CreatePen(PS_SOLID, 0, color);
        hPenOld = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, X1, Y1, NULL);
        LineTo(hdc, X2, Y2);
        SelectObject(hdc, hPenOld);
        DeleteObject(hPen);
        EndPaint(hwnd, &ps);
        
    };
    auto ClipAndDrawLine = [&rect, DrawLine](int X1, int Y1, int X2, int Y2)
    {
        auto encode = [&rect](int x, int y)
        {
            int c = 0;
            if (x < rect.left) c = c | 0x1;
            else if (x > rect.right) c = c | 0x2;
            if (y > rect.bottom) c = c | 0x4;
            else if (y < rect.top) c = c | 0x8;
            return c;
        };
        auto clip = [&rect, encode](int& X1, int& Y1, int& X2, int& Y2) -> bool
        {
            int code1, code2, code;
            int x = 0, y = 0;
            code1 = encode(X1, Y1);
            code2 = encode(X2, Y2);
            while (code1 != 0 || code2 != 0)
            {
                if ((code1 & code2) != 0) return false;
                code = code1;
                if (code == 0) code = code2;
                if ((0b1 & code) != 0)
                {
                    x = rect.left;
                    y = Y1 + (Y2 - Y1) * (rect.left - X1) / (X2 - X1);
                }
                else if ((0b10 & code) != 0)
                {
                    x = rect.right;
                    y = Y1 + (Y2 - Y1) * (rect.right - X1) / (X2 - X1);
                }
                else if ((0b100 & code) != 0)
                {
                    y = rect.bottom;
                    x = X1 + (X2 - X1) * (rect.bottom - Y1) / (Y2 - Y1);
                }
                else if ((0b1000 & code) != 0)
                {
                    y = rect.top;
                    x = X1 + (X2 - X1) * (rect.top - Y1) / (Y2 - Y1);
                }
                if (code == code1) 
                {
                    X1 = x;
                    Y1 = y;
                    code1 = encode(x, y);
                }
                else 
                {
                    X2 = x;
                    Y2 = y;
                    code2 = encode(x, y);
                }
            }
            return true;
        };
        if (clip(X1, Y1, X2, Y2))
            DrawLine(X1, Y1, X2, Y2);
    };

    double halfCellWidth = 30 / CIsoViewExt::ScaledFactor * W;
    double quaterCellWidth = 15 / CIsoViewExt::ScaledFactor * W;
    double fullCellHeight = 30 / CIsoViewExt::ScaledFactor * H;
    double halfCellHeight = 15 / CIsoViewExt::ScaledFactor * H;

    double y1 = Y - 30 / CIsoViewExt::ScaledFactor;
    double x1 = X + 30 / CIsoViewExt::ScaledFactor;

    double x2 = halfCellWidth + X + 30 / CIsoViewExt::ScaledFactor - 2;
    double y2 = quaterCellWidth + y1 - 1;

    double x3 = halfCellWidth - fullCellHeight + X + 30 / CIsoViewExt::ScaledFactor - 1;
    double y3 = halfCellHeight + quaterCellWidth + y1 - 1;

    double x4 = X - fullCellHeight + 30 / CIsoViewExt::ScaledFactor - 1;
    double y4 = halfCellHeight + y1 - 1;

    y1 -= 1;
    x1 -= 1;
    double x1L = x1 + 1;
    double x3L = x3 - 1;
    double y1L = y1 - 1;
    double y3L = y3;
    y3 += 1;
    x3 -= 2;
    double x4B = x4 - 2;
    double y4B = y4 - 1;
    double x2T = x2 + 2;
    double y2T = y2 + 1;

    //   1
    //  # #
    // 4   2
    //  # #
    //   3

    auto drawCellOutline = [&](int inneroffset)
        {
            if (s1)
            ClipAndDrawLine(x1, y1 + inneroffset, x2T - 2 * inneroffset, y2T);
            if (s2)
            ClipAndDrawLine(x2 - 2 * inneroffset, y2, x3, y3 - inneroffset);
            if (s3)
            ClipAndDrawLine(x3L, y3L - inneroffset, x4B + 2 * inneroffset, y4B);
            if (s4)
            ClipAndDrawLine(x4 + 2 * inneroffset, y4, x1L, y1L + inneroffset);
        };

    drawCellOutline(0);
    drawCellOutline(-1);
    if (CIsoViewExt::ScaledFactor < 0.6)
        drawCellOutline(-2);
    if (CIsoViewExt::ScaledFactor < 0.31)
        drawCellOutline(-3);

}

void CIsoViewExt::DrawLine(int x1, int y1, int x2, int y2, COLORREF color, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc, bool bDashed)
{
    if (lpDesc->lpSurface == nullptr)
        return;

    RECT rect = CIsoViewExt::GetScaledWindowRect();

    auto lPitch = lpDesc->lPitch;
    auto nBytesPerPixel = *(int*)0x72A8C0;

    auto pRGB = (ColorStruct*)&color;
    BGRStruct ddColor;
    ddColor.R = pRGB->red;
    ddColor.G = pRGB->green;
    ddColor.B = pRGB->blue;

    auto DrawLine = [lPitch, nBytesPerPixel, ddColor, lpDesc, &rect, bDashed](int X1, int Y1, int X2, int Y2)
        {
            int color = *(int*)&ddColor;

            if (X1 > X2)
            {
                std::swap(X1, X2);
                std::swap(Y1, Y2);
            }

            int dx = X2 - X1;
            int dy = Y2 - Y1;

            auto ptr = (unsigned char*)lpDesc->lpSurface + lPitch * Y1 + X1 * nBytesPerPixel;

            const int dashOn = 3;
            const int dashOff = 3;
            const int dashPeriod = dashOn + dashOff;

            auto shouldDraw = [=](int step) {
                return !bDashed || (step % dashPeriod) < dashOn;
                };

            if (dy == 0)
            {
                for (int i = 0; i <= dx; ++i)
                {
                    if (shouldDraw(i))
                        memcpy(ptr, &ddColor, nBytesPerPixel);
                    ptr += nBytesPerPixel;
                }
            }
            else if (dx == 0)
            {
                int pitch = lPitch;
                if (dy < 0)
                {
                    pitch = -pitch;
                    dy = -dy;
                }

                for (int i = 0; i <= dy; ++i)
                {
                    if (shouldDraw(i))
                        memcpy(ptr, &ddColor, nBytesPerPixel);
                    ptr += pitch;
                }
            }
            else
            {
                int pitch = lPitch;
                if (dy < 0)
                {
                    pitch = -pitch;
                    dy = -dy;
                }

                int dx2 = 2 * dx;
                int dy2 = 2 * dy;

                if (dx > dy)
                {
                    int delta = dy2 - dx;
                    int yOffset = 0;
                    for (int i = 0; i <= dx; ++i)
                    {
                        if (shouldDraw(i))
                            memcpy(ptr + yOffset, &ddColor, nBytesPerPixel);
                        if (delta > 0)
                        {
                            yOffset += pitch;
                            delta -= dx2;
                        }
                        delta += dy2;
                        ptr += nBytesPerPixel;
                    }
                }
                else
                {
                    int delta = dx2 - dy;
                    int xOffset = 0;
                    for (int i = 0; i <= dy; ++i)
                    {
                        if (shouldDraw(i))
                            memcpy(ptr + xOffset * nBytesPerPixel, &ddColor, nBytesPerPixel);
                        if (delta > 0)
                        {
                            ++xOffset;
                            delta -= dy2;
                        }
                        delta += dx2;
                        ptr += pitch;
                    }
                }
            }
        };
    auto ClipAndDrawLine = [&rect, DrawLine](int X1, int Y1, int X2, int Y2)
        {
            auto encode = [&rect](int x, int y)
                {
                    int c = 0;
                    if (x < rect.left) c = c | 0x1;
                    else if (x > rect.right) c = c | 0x2;
                    if (y > rect.bottom) c = c | 0x4;
                    else if (y < rect.top) c = c | 0x8;
                    return c;
                };
            auto clip = [&rect, encode](int& X1, int& Y1, int& X2, int& Y2) -> bool
                {
                    int code1, code2, code;
                    int x = 0, y = 0;
                    code1 = encode(X1, Y1);
                    code2 = encode(X2, Y2);
                    while (code1 != 0 || code2 != 0)
                    {
                        if ((code1 & code2) != 0) return false;
                        code = code1;
                        if (code == 0) code = code2;
                        if ((0b1 & code) != 0)
                        {
                            x = rect.left;
                            y = Y1 + (Y2 - Y1) * (rect.left - X1) / (X2 - X1);
                        }
                        else if ((0b10 & code) != 0)
                        {
                            x = rect.right;
                            y = Y1 + (Y2 - Y1) * (rect.right - X1) / (X2 - X1);
                        }
                        else if ((0b100 & code) != 0)
                        {
                            y = rect.bottom;
                            x = X1 + (X2 - X1) * (rect.bottom - Y1) / (Y2 - Y1);
                        }
                        else if ((0b1000 & code) != 0)
                        {
                            y = rect.top;
                            x = X1 + (X2 - X1) * (rect.top - Y1) / (Y2 - Y1);
                        }
                        if (code == code1)
                        {
                            X1 = x;
                            Y1 = y;
                            code1 = encode(x, y);
                        }
                        else
                        {
                            X2 = x;
                            Y2 = y;
                            code2 = encode(x, y);
                        }
                    }
                    return true;
                };
            if (clip(X1, Y1, X2, Y2))
            {



                DrawLine(X1, Y1, X2, Y2);
            }
                
        };

    ClipAndDrawLine(x1, y1, x2, y2);
}

void CIsoViewExt::DrawLockedLines(const std::vector<std::pair<MapCoord, MapCoord>>& lines, int X, int Y, COLORREF color, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc)
{
    if (lpDesc->lpSurface == nullptr)
        return;

    RECT rect = CIsoViewExt::GetScaledWindowRect();

    auto lPitch = lpDesc->lPitch;
    auto nBytesPerPixel = *(int*)0x72A8C0;

    auto pRGB = (ColorStruct*)&color;
    BGRStruct ddColor;
    ddColor.R = pRGB->red;
    ddColor.G = pRGB->green;
    ddColor.B = pRGB->blue;

    auto DrawLine = [lPitch, nBytesPerPixel, ddColor, lpDesc, &rect](int X1, int Y1, int X2, int Y2)
    {
        int color = *(int*)&ddColor;

        if (X1 > X2)
        {
            std::swap(X1, X2);
            std::swap(Y1, Y2);
        }

        int dx = X2 - X1;
        int dy = Y2 - Y1;

        auto ptr = (unsigned char*)lpDesc->lpSurface + lPitch * Y1 + X1 * nBytesPerPixel;

        if (dy == 0)
        {
            for (int i = 0; i <= dx; ++i)
            {
                memcpy(ptr, &ddColor, nBytesPerPixel);
                ptr += nBytesPerPixel;
            }
        }
        else if (dx == 0)
        {
            int pitch = lPitch;
            if (dy < 0)
            {
                pitch = -pitch;
                dy = -dy;
            }

            for (int i = 0; i <= dy; ++i)
            {
                memcpy(ptr, &ddColor, nBytesPerPixel);
                ptr += pitch;
            }
        }
        else
        {
            int pitch = lPitch;
            if (dy < 0)
            {
                pitch = -pitch;
                dy = -dy;
            }

            int dx2 = 2 * dx;
            int dy2 = 2 * dy;

            if (dx > dy)
            {
                int delta = dy2 - dx;
                for (int i = 0; i < dx; ++i)
                {
                    memcpy(ptr + i * nBytesPerPixel, &ddColor, nBytesPerPixel);
                    if (delta > 0)
                    {
                        ptr += pitch;
                        delta -= dx2;
                    }
                    delta += dy2;
                }
            }
            else
            {
                int delta = dx2 - dy;
                int k = 0;

                for (int i = 0; i < dy; ++i)
                {
                    memcpy(ptr + k * nBytesPerPixel, &ddColor, nBytesPerPixel);
                    if (delta > 0)
                    {
                        ++k;
                        delta -= dy2;
                    }
                    delta += dx2;
                    ptr += pitch;
                }
            }
        }
    };
    auto ClipAndDrawLine = [&rect, DrawLine](int X1, int Y1, int X2, int Y2)
    {
        auto encode = [&rect](int x, int y)
        {
            int c = 0;
            if (x < rect.left) c = c | 0x1;
            else if (x > rect.right) c = c | 0x2;
            if (y > rect.bottom) c = c | 0x4;
            else if (y < rect.top) c = c | 0x8;
            return c;
        };
        auto clip = [&rect, encode](int& X1, int& Y1, int& X2, int& Y2) -> bool
        {
            int code1, code2, code;
            int x = 0, y = 0;
            code1 = encode(X1, Y1);
            code2 = encode(X2, Y2);
            while (code1 != 0 || code2 != 0)
            {
                if ((code1 & code2) != 0) return false;
                code = code1;
                if (code == 0) code = code2;
                if ((0b1 & code) != 0)
                {
                    x = rect.left;
                    y = Y1 + (Y2 - Y1) * (rect.left - X1) / (X2 - X1);
                }
                else if ((0b10 & code) != 0)
                {
                    x = rect.right;
                    y = Y1 + (Y2 - Y1) * (rect.right - X1) / (X2 - X1);
                }
                else if ((0b100 & code) != 0)
                {
                    y = rect.bottom;
                    x = X1 + (X2 - X1) * (rect.bottom - Y1) / (Y2 - Y1);
                }
                else if ((0b1000 & code) != 0)
                {
                    y = rect.top;
                    x = X1 + (X2 - X1) * (rect.top - Y1) / (Y2 - Y1);
                }
                if (code == code1)
                {
                    X1 = x;
                    Y1 = y;
                    code1 = encode(x, y);
                }
                else
                {
                    X2 = x;
                    Y2 = y;
                    code2 = encode(x, y);
                }
            }
            return true;
        };
        if (clip(X1, Y1, X2, Y2))
            DrawLine(X1, Y1, X2, Y2);
    };

    Y -= 30;
    X += 30;
    for (const auto& line : lines)
    {
        int x1 = X + line.first.X;
        int y1 = Y + line.first.Y;
        int x2 = X + line.second.X;
        int y2 = Y + line.second.Y;
        ClipAndDrawLine(x1, y1, x2, y2);
        //ClipAndDrawLine(x1, y1, x2, y2);
        if (!bUseDot)
        {
            ClipAndDrawLine(x1 + 1, y1, x2 + 1, y2);
            ClipAndDrawLine(x1 - 1, y1, x2 - 1, y2);
            ClipAndDrawLine(x1 + 2, y1, x2 + 2, y2);
            ClipAndDrawLine(x1 - 2, y1, x2 - 2, y2);
        }
    }
}

int CIsoViewExt::GetSelectedSubcellInfantryIdx(int X, int Y, bool getSubcel)
{
    if (CIsoViewExt::LastCommand::requestSubpos)
        return CIsoViewExt::LastAltCommand.Subpos;

    auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;

    auto currentMapCoord = pIsoView->StartCell;
    int pos;  
    if (X != -1 && Y != -1)
    {
        currentMapCoord.X = X;
        currentMapCoord.Y = Y;
        pos = CMapData::Instance().GetCoordIndex(X, Y);
    }

    else
        pos = CMapData::Instance().GetCoordIndex(currentMapCoord.X, currentMapCoord.Y);
    if (CMapDataExt::GetInfantryAt(pos) != -1 || getSubcel)
    {
        auto& mouse = pIsoView->MouseCurrentPosition;

        RECT rect;
        pIsoView->GetWindowRect(&rect);
        int mouseX = mouse.x + rect.left + pIsoView->ViewPosition.x;
        int mouseY = mouse.y + rect.top + pIsoView->ViewPosition.y;

        int cX = currentMapCoord.X, cY = currentMapCoord.Y;
        CIsoViewExt::MapCoord2ScreenCoord(cX, cY);
        int CellCenterX = cX + 36 / CIsoViewExt::ScaledFactor;
        int CellCenterY = cY - 12 / CIsoViewExt::ScaledFactor;
        FString tmp;

        auto getSubcellInf = [&](int subpos)
            {
                for (int i = 0; i < 3; i++)
                {
                    int idx = CMapData::Instance->CellDatas[pos].Infantry[i];
                    if (idx != -1)
                    {
                        CInfantryData infData;
                        CMapData::Instance->GetInfantryData(idx, infData);
                        if (atoi(infData.SubCell) == subpos)
                            return idx;
                    }
                }
                return -1;
            };

        int count = 0;
        for (int i = 0; i < 3; i++)
        {
            int idx = CMapData::Instance->CellDatas[pos].Infantry[i];
            if (idx != -1)
            {
                count++;
            }
        }
        if (count == 1 && !ExtConfigs::InfantrySubCell_Edit_Single && !getSubcel)
        {
             return CMapDataExt::GetInfantryAt(pos);
        }
        else
        {
            int xDistance = (mouseX - CellCenterX) * CIsoViewExt::ScaledFactor;
            if (xDistance >= -10 && xDistance <= 6)
            {
                if (getSubcel)
                {
                    if (ExtConfigs::InfantrySubCell_Edit_FixCenter && ExtConfigs::InfantrySubCell_GameDefault)
                        return 4;
                    else if (ExtConfigs::InfantrySubCell_Edit_FixCenter && !ExtConfigs::InfantrySubCell_GameDefault)
                        return 1;
                    else
                    {
                        if (mouseY - CellCenterY > 2)
                            return 4;
                        return 1;
                    }
                }
                     
                int idx = getSubcellInf(4);
                if (idx == -1)
                    idx = getSubcellInf(0);
                if (idx == -1)
                    idx = getSubcellInf(1);
                if (idx != -1)
                    return idx;
            }
            else if (xDistance < -10)
            {
                if (getSubcel)
                    return 3;
                int idx = getSubcellInf(3);
                if (idx != -1)
                    return idx;
            }
            else if (xDistance > 6)
            {
                if (getSubcel)
                    return 2;
                int idx = getSubcellInf(2);
                if (idx != -1)
                    return idx;
            }
        }
    }
    return -1;
}

void CIsoViewExt::DrawBitmap(FString filename, int X, int Y, LPDDSURFACEDESC2 lpDesc)
{
    this->BlitTransparentDesc(CLoadingExt::GetSurfaceImageDataFromMap(filename + ".bmp")->lpSurface, this->lpDDBackBufferSurface, lpDesc, X, Y, -1, -1);
}

void CIsoViewExt::DrawCelltag(int X, int Y, LPDDSURFACEDESC2 lpDesc)
{
    auto image = CLoadingExt::GetSurfaceImageDataFromMap("CELLTAG");
    this->BlitTransparentDesc(image->lpSurface, this->lpDDBackBufferSurface, lpDesc, X + 25 - image->FullWidth / 2, Y + 12 - image->FullHeight / 2, -1, -1);
}

void CIsoViewExt::DrawWaypointFlag(int X, int Y, LPDDSURFACEDESC2 lpDesc)
{
    auto image = CLoadingExt::GetSurfaceImageDataFromMap("FLAG");
    this->BlitTransparentDesc(image->lpSurface, this->lpDDBackBufferSurface, lpDesc, X + 5 + 25 - image->FullWidth / 2, Y + 12 - image->FullHeight / 2, -1, -1);
}

void CIsoViewExt::FillArea(int X, int Y, int ID, int Subtile, int oriX, int oriY, std::set<MapCoord>* selectedCoords)
{
    bool isFirstRun = false;
    std::unique_ptr<std::set<MapCoord>> recordCoords;
    if (!selectedCoords)
    {
        recordCoords = std::make_unique<std::set<MapCoord>>();
        selectedCoords = recordCoords.get();
        isFirstRun = true;
    }

    auto& map = CMapData::Instance;
    
    if (!map->IsCoordInMap(X, Y))
        return;
    auto cell = map->GetCellAt(X, Y);
    cell->Flag.NotAValidCell = TRUE;

    if (cell->IsHidden())
        return;

    if (MultiSelection::SelectedCoords.size() > 0 && MultiSelection::IsSelected(oriX, oriY)) {
        bool skip = true;
        for (const auto& coord : MultiSelection::SelectedCoords) {
            if (coord.X == X && coord.Y == Y) {
                skip = false;
                break;
            }
        }
        if (skip)
            return;
    }

    int tileIndex_cell = CMapDataExt::GetSafeTileIndex(cell->TileIndex);

    int mapwidth, mapheight;
    mapwidth = map->Size.Width;
    mapheight = map->Size.Height;

    //const int BlockWaters[3] = { 6 , 7, 13 };
    int iWaterSet = CINI::CurrentTheater->GetInteger("General", "WaterSet", -1);

    int i, e;
    for (i = -1; i < 2; i++)
    {
        for (e = -1; e < 2; e++)
        {
            if (abs(i) == abs(e)) continue;
            int cur_x, cur_y;
            cur_x = X + i;
            cur_y = Y + e;

            if (!map->IsCoordInMap(cur_x, cur_y))
                continue;

            auto cell2 = map->TryGetCellAt(cur_x, cur_y);

            if (cell2->Flag.NotAValidCell) continue;

            bool match = false;
            int tileIndex_cell2 = CMapDataExt::GetSafeTileIndex(cell2->TileIndex);

            match = tileIndex_cell2 == tileIndex_cell && cell2->TileSubIndex == cell->TileSubIndex;
            if (ExtConfigs::FillArea_ConsiderLAT && !match)
            {
                for (auto& latPair : CMapDataExt::Tile_to_lat)
                {
                    int iSmoothSet = latPair[0];
                    int iLatSet = latPair[1];

                    if (iLatSet >= 0 && iSmoothSet >= 0 && iSmoothSet < CMapDataExt::TileSet_starts.size() && iLatSet < CMapDataExt::TileSet_starts.size() &&
                        (CMapDataExt::TileData[tileIndex_cell2].TileSet == iSmoothSet || CMapDataExt::TileData[tileIndex_cell2].TileSet == iLatSet) &&
                        (CMapDataExt::TileData[tileIndex_cell].TileSet == iSmoothSet || CMapDataExt::TileData[tileIndex_cell].TileSet == iLatSet))
                    {
                        if (cell2->TileSubIndex == cell->TileSubIndex) {
                            match = true;
                            break;
                        }
                    }
                }

            }
            if (ExtConfigs::FillArea_ConsiderWater && !match)
            {
                if (CMapDataExt::TileData[tileIndex_cell2].TileSet == iWaterSet && CMapDataExt::TileData[tileIndex_cell].TileSet == iWaterSet)
                {
                    //bool notWaterBlock = true;
                    //for (int bw : BlockWaters)
                    //{
                    //    if (bw == tileIndex_cell2 - CMapDataExt::TileSet_starts[iWaterSet] || bw == tileIndex_cell - CMapDataExt::TileSet_starts[iWaterSet])
                    //        notWaterBlock = false;
                    //}
                    //if (notWaterBlock)
                        match = true;
                }
            }


            if (tileIndex_cell2 != ID && match)
            {
                FillArea(cur_x, cur_y, ID, Subtile, oriX, oriY, selectedCoords);
            }

        }
    }
    selectedCoords->insert(MapCoord{ X,Y });
    if (isFirstRun)
    {
        if (ID >= 0 && ID < CMapDataExt::TileDataCount)
        {
            std::vector<TilePlacement> placements;
            auto& tile = CMapDataExt::TileData[ID];
            int tileOriginX = oriY - (tile.Width - 1);
            int tileOriginY = oriX - (tile.Height - 1);
            for (const auto& coord : *selectedCoords)
            {
                int localY = ((coord.Y - tileOriginX) % tile.Width + tile.Width) % tile.Width;
                int localX = ((coord.X - tileOriginY) % tile.Height + tile.Height) % tile.Height;

                int subtileIndex = localY + localX * tile.Width;

                placements.push_back(TilePlacement{
                    (short)coord.X,
                    (short)coord.Y,
                    (short)subtileIndex
                    });
            }

            for (auto& coord : placements)
            {
                if (tile.TileBlockDatas[coord.SubtileIndex].ImageData != NULL)
                {
                    bool isBridge = (tile.TileSet == CMapDataExt::BridgeSet || tile.TileSet == CMapDataExt::WoodBridgeSet);
                    auto cell = CMapData::Instance->GetCellAt(coord.X, coord.Y);
                    cell->TileIndex = ID;
                    cell->TileSubIndex = coord.SubtileIndex;
                    cell->Flag.AltIndex = isBridge ? 0 : STDHelpers::RandomSelectInt(0, tile.AltTypeCount + 1);
                    CMapDataExt::GetExtension()->SetHeightAt(coord.X, coord.Y, cell->Height + tile.TileBlockDatas[coord.SubtileIndex].Height);
                    CMapData::Instance->UpdateMapPreviewAt(coord.X, coord.Y);
                }
            }

            if (!CFinalSunApp::Instance->DisableAutoLat)
            {
                std::set<MapCoord> editedLatCoords;
                for (const auto& p : *selectedCoords) {
                    editedLatCoords.insert({ p.X + 1, p.Y });
                    editedLatCoords.insert({ p.X - 1, p.Y });
                    editedLatCoords.insert({ p.X, p.Y + 1 });
                    editedLatCoords.insert({ p.X, p.Y - 1 });
                    editedLatCoords.insert({ p.X, p.Y });
                }
                for (const auto& p : editedLatCoords)
                {
                    CMapDataExt::SmoothTileAt(p.X, p.Y, true);
                }
            }
        }
        selectedCoords->clear();
    } 
}

void CIsoViewExt::BlitText(const std::wstring& text, COLORREF textColor, COLORREF bgColor,
    CIsoView* pThis, void* dst, const RECT& window, const DDBoundary& boundary,
    int x, int y, int fontSize, BYTE alpha, bool bold)
{
    int bpp = 4;
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    RECT textRect = { 0, 0, 1000, 1000 };
    HFONT hFont = CreateFontW(fontSize, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Cambria");
    HFONT oldFont = (HFONT)SelectObject(hdcMem, hFont);

    ::DrawTextW(hdcMem, text.c_str(), -1, &textRect, DT_CALCRECT | DT_LEFT | DT_TOP | DT_WORDBREAK);
    int swidth = textRect.right - textRect.left;
    int sheight = textRect.bottom - textRect.top;

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, swidth, sheight);
    SelectObject(hdcMem, hBitmap);

    SetBkColor(hdcMem, bgColor);
    SetTextColor(hdcMem, textColor);
    SetBkMode(hdcMem, OPAQUE);

    swidth = std::min(swidth, 1000);
    sheight = std::min(sheight, 1000);

    RECT fillRect = { 0, 0, swidth, sheight };
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    FillRect(hdcMem, &fillRect, hBrush);
    DeleteObject(hBrush);

    ::DrawTextW(hdcMem, text.c_str(), -1, &fillRect, DT_LEFT | DT_TOP | DT_WORDBREAK);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = swidth;
    bmi.bmiHeader.biHeight = -sheight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    auto tempPixels = new BYTE[swidth * sheight * bpp];
    GetDIBits(hdcMem, hBitmap, 0, sheight, tempPixels, &bmi, DIB_RGB_COLORS);

    int borderWidth = 3;
    int finalWidth = swidth + 2 * borderWidth;
    int finalHeight = sheight + 2 * borderWidth;

    auto src = new BGRStruct[finalWidth * finalHeight];

    BGRStruct bgbgr;
    bgbgr.B = (bgColor >> 16) & 0xFF;
    bgbgr.G = (bgColor >> 8) & 0xFF;
    bgbgr.R = bgColor & 0xFF;
    
    BGRStruct textbgr;
    textbgr.B = (textColor >> 16) & 0xFF;
    textbgr.G = (textColor >> 8) & 0xFF;
    textbgr.R = textColor & 0xFF;

    // bgColor border
    for (int y = 0; y < finalHeight; y++) {
        for (int x = 0; x < finalWidth; x++) {
            if (x <= 2 || x >= finalWidth - 3 || y <= 2 || y >= finalHeight - 3) {
                src[y * finalWidth + x] = bgbgr;
            }
        }
    }

    // textColor border
    for (int y = 0; y < finalHeight; y++) {
        for (int x = 0; x < finalWidth; x++) {
            if (x == 0 || x == finalWidth || y == 0 || y == finalHeight - 1) {
                src[y * finalWidth + x] = textbgr;
            }
        }
    }

    for (int y = 0; y < sheight; y++) {
        for (int x = 0; x < swidth; x++) {
            int srcIndex = (y + borderWidth) * finalWidth + (x + borderWidth);
            int tempIndex = (y * swidth + x) * 4;
            src[srcIndex] = BGRStruct(tempPixels[tempIndex + 0], // B
                tempPixels[tempIndex + 1], // G
                tempPixels[tempIndex + 2]);// R
        }
    }

    delete[] tempPixels;

    swidth = finalWidth;
    sheight = finalHeight;

    SelectObject(hdcMem, oldFont);
    DeleteObject(hFont);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    if (src == NULL || dst == NULL) {
        delete[] src;
        return;
    }
    if (x + swidth < window.left || y + sheight < window.top) {
        delete[] src;
        return;
    }
    if (x >= window.right || y >= window.bottom) {
        delete[] src;
        return;
    }

    RECT blrect;
    RECT srcRect;
    srcRect.left = 0;
    srcRect.top = 0;
    srcRect.right = swidth;
    srcRect.bottom = sheight;
    blrect.left = x;
    if (blrect.left < 0) {
        srcRect.left = 1 - blrect.left;
        //blrect.left=1;
    }
    blrect.top = y;
    if (blrect.top < 0) {
        srcRect.top = 1 - blrect.top;
        //blrect.top=1;
    }
    blrect.right = (x + swidth);
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        blrect.right = window.right;
    }
    blrect.bottom = (y + sheight);
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        blrect.bottom = window.bottom;
    }

    BGRStruct textBGR;
    auto pRGB = (ColorStruct*)&textColor;
    textBGR.R = pRGB->red;
    textBGR.G = pRGB->green;
    textBGR.B = pRGB->blue;

    int i, e;
    auto const surfaceEnd = (BYTE*)dst + boundary.dpitch * boundary.dwHeight;

    for (e = srcRect.top; e < srcRect.bottom; e++) {
        for (i = srcRect.left; i <= srcRect.right; i++) {
            if (blrect.left + i < 0) {
                continue;
            }
            
            const int spos = i + e * swidth;
            auto c = src[spos];
            auto dest = ((BYTE*)dst + (blrect.left + i) * bpp + (blrect.top + e) * boundary.dpitch);

            if (dest >= dst) {
                if (dest + bpp < surfaceEnd) {
                    if (alpha < 255 && c != textBGR)
                    {
                        BGRStruct oriColor = *(BGRStruct*)dest;
                        c.B = (c.B * alpha + oriColor.B * (255 - alpha)) / 255;
                        c.G = (c.G * alpha + oriColor.G * (255 - alpha)) / 255;
                        c.R = (c.R * alpha + oriColor.R * (255 - alpha)) / 255;
                    }
                    memcpy(dest, &c, bpp);
                }
            }
        }
    }
    delete[] src;
}

IDirectDrawSurface7* CIsoViewExt::BitmapToSurface(IDirectDraw7* pDD, const CBitmap& bitmap)
{
    BITMAP bm;
    GetObject(bitmap, sizeof(bm), &bm);

    DDSURFACEDESC2 desc = { 0 };
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    desc.dwWidth = bm.bmWidth;
    desc.dwHeight = bm.bmHeight;

    IDirectDrawSurface7* pSurface = nullptr;
    if (pDD->CreateSurface(&desc, &pSurface, nullptr) != DD_OK)
        return nullptr;

    pSurface->Restore();

    CDC bitmapDC;
    if (!bitmapDC.CreateCompatibleDC(nullptr)) {
        pSurface->Release();
        return nullptr;
    }
    bitmapDC.SelectObject(bitmap);

    HDC hSurfaceDC = nullptr;
    if (pSurface->GetDC(&hSurfaceDC) != DD_OK) {
        pSurface->Release();
        return nullptr;
    }

    CDC surfaceDC;
    surfaceDC.Attach(hSurfaceDC);

    auto success = surfaceDC.BitBlt(0, 0, bm.bmWidth, bm.bmHeight, &bitmapDC, 0, 0, SRCCOPY);
    surfaceDC.Detach();
    pSurface->ReleaseDC(hSurfaceDC);

    return pSurface;
}

void CIsoViewExt::BlitTransparent(LPDIRECTDRAWSURFACE7 pic, int x, int y, int width, int height, BYTE alpha, LPDIRECTDRAWSURFACE7 surface)
{
    // 0. 初始检查
    if (!pic || alpha == 0) {
        return;
    }

    // 1. 初始化常量和偏移
    const int X_OFFSET = 1;
    const int Y_OFFSET = -29;
    const int BPP = 4; // 假设 32 位颜色深度

    auto pThis = CIsoView::GetInstance();
    RECT windowRect;
    if (!surface) {
        windowRect = CIsoViewExt::GetScaledWindowRect();
        surface = pThis->lpDDBackBufferSurface;
    }
    else if (surface == pThis->lpDDBackBufferSurface) {
        windowRect = CIsoViewExt::GetScaledWindowRect();
    }
    else {
        pThis->GetWindowRect(&windowRect);
    }

    x += X_OFFSET;
    y += Y_OFFSET;

    // 2. 获取表面尺寸
    if (width == -1 || height == -1) {
        DDSURFACEDESC2 ddsd = { sizeof(DDSURFACEDESC2), DDSD_WIDTH | DDSD_HEIGHT };
        if (pic->GetSurfaceDesc(&ddsd) != DD_OK) {
            return;
        }
        width = ddsd.dwWidth;
        height = ddsd.dwHeight;
    }

    // 3. 边界检查（与原始代码一致）
    if (x + width < 0 || y + height < 0) {
        return;
    }
    if (x > windowRect.right || y > windowRect.bottom) {
        return;
    }

    // 4. 计算源和目标矩形（与原始代码一致）
    RECT srcRect = { 0, 0, width, height };
    RECT destRect = { x, y, x + width, y + height };
    if (destRect.left < 0) {
        srcRect.left = -destRect.left;
        destRect.left = 0;
    }
    if (destRect.top < 0) {
        srcRect.top = -destRect.top;
        destRect.top = 0;
    }
    if (destRect.right > windowRect.right) {
        srcRect.right = width - (destRect.right - windowRect.right);
        destRect.right = windowRect.right;
    }
    if (destRect.bottom > windowRect.bottom) {
        srcRect.bottom = height - (destRect.bottom - windowRect.bottom);
        destRect.bottom = windowRect.bottom;
    }

    // 5. 锁定表面
    DDSURFACEDESC2 destDesc = { sizeof(DDSURFACEDESC2) };
    DDSURFACEDESC2 srcDesc = { sizeof(DDSURFACEDESC2) };
    if (surface->Lock(NULL, &destDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL) != DD_OK) {
        return;
    }
    if (pic->Lock(NULL, &srcDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL) != DD_OK) {
        surface->Unlock(NULL);
        return;
    }

    // 6. 获取颜色键
    DDCOLORKEY colorKey;
    if (pic->GetColorKey(DDCKEY_SRCBLT, &colorKey) != DD_OK) {
        pic->Unlock(NULL);
        surface->Unlock(NULL);
        return;
    }
    DWORD colorKeyLow = colorKey.dwColorSpaceLowValue;
    DWORD colorKeyHigh = colorKey.dwColorSpaceHighValue;

    // 8. 计算基地址
    BYTE* destPixels = static_cast<BYTE*>(destDesc.lpSurface);
    BYTE* srcPixels = static_cast<BYTE*>(srcDesc.lpSurface);
    int destPitch = destDesc.lPitch;
    int srcPitch = srcDesc.lPitch;
    int maxDestX = destDesc.dwWidth;
    int maxDestY = destDesc.dwHeight;

    // 9. 像素绘制循环
    for (LONG row = 0; row < srcRect.bottom - srcRect.top; ++row) {
        LONG dy = destRect.top + row;
        if (dy < 0 || dy >= maxDestY) {
            continue;
        }

        BYTE* destLine = destPixels + dy * destPitch + destRect.left * BPP;
        BYTE* srcLine = srcPixels + row * srcPitch;
        for (LONG col = 0; col < srcRect.right - srcRect.left; ++col) {
            LONG dx = destRect.left + col;
            if (dx < 0 || dx >= maxDestX) {
                continue;
            }

            int srcIndex = col * BPP;
            int destIndex = col * BPP;
            DWORD srcColor = *reinterpret_cast<DWORD*>(srcLine + srcIndex);
            if (srcColor >= colorKeyLow && srcColor <= colorKeyHigh) {
                continue;
            }

            BYTE* destPtr = destLine + destIndex;
            if (destIndex >= 0 && destIndex < maxDestY * destPitch) {
                BYTE srcR = srcLine[srcIndex + 2];
                BYTE srcG = srcLine[srcIndex + 1];
                BYTE srcB = srcLine[srcIndex];
                BYTE destR = destPtr[2];
                BYTE destG = destPtr[1];
                BYTE destB = destPtr[0];

                // Alpha 混合使用预计算表
                destPtr[2] = alphaBlendTable[srcR][alpha] + alphaBlendTable[destR][255 - alpha];
                destPtr[1] = alphaBlendTable[srcG][alpha] + alphaBlendTable[destG][255 - alpha];
                destPtr[0] = alphaBlendTable[srcB][alpha] + alphaBlendTable[destB][255 - alpha];
            }
        }
    }

    // 10. 解锁表面
    pic->Unlock(NULL);
    surface->Unlock(NULL);
}

void CIsoViewExt::BlitTransparentDesc(LPDIRECTDRAWSURFACE7 pic, LPDIRECTDRAWSURFACE7 surface, DDSURFACEDESC2* pDestDesc,
    int x, int y, int width, int height, BYTE alpha)
{
    if (!pic || !pDestDesc || alpha == 0) {
        return;
    }

    const int X_OFFSET = 1;
    const int Y_OFFSET = -29;
    const int BPP = 4; 

    auto pThis = CIsoView::GetInstance();
    RECT windowRect;
    if (surface == pThis->lpDDBackBufferSurface) {
        windowRect = CIsoViewExt::GetScaledWindowRect();
    }
    else {
        pThis->GetWindowRect(&windowRect);
    }

    x += X_OFFSET;
    y += Y_OFFSET;

    if (width == -1 || height == -1) {
        DDSURFACEDESC2 ddsd = { sizeof(DDSURFACEDESC2), DDSD_WIDTH | DDSD_HEIGHT };
        if (pic->GetSurfaceDesc(&ddsd) != DD_OK) {
            return;
        }
        width = ddsd.dwWidth;
        height = ddsd.dwHeight;
    }

    if (x + width < 0 || y + height < 0) {
        return;
    }
    if (x > windowRect.right || y > windowRect.bottom) {
        return;
    }

    RECT srcRect = { 0, 0, width, height };
    RECT destRect = { x, y, x + width, y + height };
    if (destRect.left < 0) {
        srcRect.left = -destRect.left;
        destRect.left = 0;
    }
    if (destRect.top < 0) {
        srcRect.top = -destRect.top;
        destRect.top = 0;
    }
    if (destRect.right > windowRect.right) {
        srcRect.right = width - (destRect.right - windowRect.right);
        destRect.right = windowRect.right;
    }
    if (destRect.bottom > windowRect.bottom) {
        srcRect.bottom = height - (destRect.bottom - windowRect.bottom);
        destRect.bottom = windowRect.bottom;
    }

    DDSURFACEDESC2 srcDesc = { sizeof(DDSURFACEDESC2) };
    if (pic->Lock(NULL, &srcDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL) != DD_OK) {
        return;
    }

    DDCOLORKEY colorKey;
    if (pic->GetColorKey(DDCKEY_SRCBLT, &colorKey) != DD_OK) {
        pic->Unlock(NULL);
        return;
    }
    DWORD colorKeyLow = colorKey.dwColorSpaceLowValue;
    DWORD colorKeyHigh = colorKey.dwColorSpaceHighValue;

    BYTE* destPixels = static_cast<BYTE*>(pDestDesc->lpSurface);
    BYTE* srcPixels = static_cast<BYTE*>(srcDesc.lpSurface);
    int destPitch = pDestDesc->lPitch;
    int srcPitch = srcDesc.lPitch;
    int maxDestX = pDestDesc->dwWidth;
    int maxDestY = pDestDesc->dwHeight;

    for (LONG row = 0; row < srcRect.bottom - srcRect.top; ++row) {
        LONG dy = destRect.top + row;
        if (dy < 0 || dy >= maxDestY) {
            continue;
        }

        BYTE* destLine = destPixels + dy * destPitch + destRect.left * BPP;
        BYTE* srcLine = srcPixels + row * srcPitch;
        for (LONG col = 0; col < srcRect.right - srcRect.left; ++col) {
            LONG dx = destRect.left + col;
            if (dx < 0 || dx >= maxDestX) {
                continue;
            }

            int srcIndex = col * BPP;
            int destIndex = col * BPP;
            DWORD srcColor = *reinterpret_cast<DWORD*>(srcLine + srcIndex);
            if (srcColor >= colorKeyLow && srcColor <= colorKeyHigh) {
                continue;
            }

            BYTE* destPtr = destLine + destIndex;
            if (destIndex >= 0 && destIndex < maxDestY * destPitch) {
                BYTE srcR = srcLine[srcIndex + 2];
                BYTE srcG = srcLine[srcIndex + 1];
                BYTE srcB = srcLine[srcIndex];
                BYTE destR = destPtr[2];
                BYTE destG = destPtr[1];
                BYTE destB = destPtr[0];

                destPtr[2] = alphaBlendTable[srcR][alpha] + alphaBlendTable[destR][255 - alpha];
                destPtr[1] = alphaBlendTable[srcG][alpha] + alphaBlendTable[destG][255 - alpha];
                destPtr[0] = alphaBlendTable[srcB][alpha] + alphaBlendTable[destB][255 - alpha];
            }
        }
    }

    pic->Unlock(NULL);
}

void CIsoViewExt::BlitSHPTransparent(CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, ImageDataClass* pd, Palette* newPal, BYTE alpha, COLORREF houseColor, int extraLightType, bool remap)
{
    if (alpha == 0 || !pd || pd->Flag == ImageDataFlag::SurfaceData || !pd->pImageBuffer || !dst) {
        return;
    }

    const int X_OFFSET = 31;
    const int Y_OFFSET = -29;
    const int BPP = 4;

    x += X_OFFSET;
    y += Y_OFFSET;

    BYTE* src = static_cast<BYTE*>(pd->pImageBuffer);
    int swidth = pd->FullWidth;
    int sheight = pd->FullHeight;

    if (x + swidth < window.left || y + sheight < window.top) {
        return;
    }
    if (x >= window.right || y >= window.bottom) {
        return;
    }

    RECT srcRect = { 0, 0, swidth, sheight };
    RECT destRect = { x, y, x + swidth, y + sheight };
    if (destRect.left < 0) {
        srcRect.left = 1 - destRect.left;
    }
    if (destRect.top < 0) {
        srcRect.top = 1 - destRect.top;
    }
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        destRect.right = window.right;
    }
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        destRect.bottom = window.bottom;
    }

    if (!newPal) {
        newPal = pd->pPalette;
    }
    bool isMultiSelected = false;
    if (extraLightType == -10 || extraLightType >= 500) {
        isMultiSelected = MultiSelection::IsSelected(CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y);
        if (extraLightType >= 500) {
            int overlay = extraLightType - 500;
            if (overlay == 0x18 || overlay == 0x19 || // BRIDGE1, BRIDGE2
                overlay == 0x3B || overlay == 0x3C || // RAILBRDG1, RAILBRDG2
                overlay == 0xED || overlay == 0xEE || // BRIDGEB1, BRIDGEB2
                (overlay >= 0x4A && overlay <= 0x65) || // LOBRDG 1-28
                (overlay >= 0xCD && overlay <= 0xEC)) { // LOBRDGB 1-4
                isMultiSelected = MultiSelection::IsSelected(
                    CIsoViewExt::CurrentDrawCellLocation.X + 1,
                    CIsoViewExt::CurrentDrawCellLocation.Y + 1);
            }
        }
    }

    BGRStruct color;
    auto pRGB = reinterpret_cast<ColorStruct*>(&houseColor);
    color.R = pRGB->red;
    color.G = pRGB->green;
    color.B = pRGB->blue;
    if (LightingStruct::CurrentLighting != LightingStruct::NoLighting) {
        if (extraLightType >= 500) {
            newPal = PalettesManager::GetOverlayPalette(newPal, CIsoViewExt::CurrentDrawCellLocation, extraLightType - 500);
        }
        else {
            newPal = PalettesManager::GetObjectPalette(newPal, color, remap, CIsoViewExt::CurrentDrawCellLocation, false, extraLightType);
        }
    }
    else {
        newPal = PalettesManager::GetPalette(newPal, color, remap);
    }

    BYTE* srcBase = src;
    BYTE* destBase = static_cast<BYTE*>(dst) + destRect.top * boundary.dpitch + destRect.left * BPP;
    BYTE* surfaceEnd = static_cast<BYTE*>(dst) + boundary.dpitch * boundary.dwHeight;

    for (LONG row = srcRect.top; row < srcRect.bottom; ++row) {
        LONG left = pd->pPixelValidRanges[row].First;
        LONG right = pd->pPixelValidRanges[row].Last;
        if (left < srcRect.left) {
            left = srcRect.left;
        }
        if (right >= srcRect.right) {
            right = srcRect.right - 1;
        }
        if (left > right) {
            continue;
        }

        BYTE* srcPtr = srcBase + row * swidth + left;
        BYTE* destPtr = destBase + row * boundary.dpitch + left * BPP;
        for (LONG col = left; col <= right; ++col, ++srcPtr, destPtr += BPP) {
            if (destRect.left + col < 0) {
                continue;
            }

            BYTE pixelValue = *srcPtr;
            if (pixelValue && destPtr >= dst && destPtr + BPP <= surfaceEnd) {
                BGRStruct c = newPal->Data[pixelValue];
                if (alpha < 255) {
                    BGRStruct oriColor = *reinterpret_cast<BGRStruct*>(destPtr);
                    c.B = alphaBlendTable[c.B][alpha] + alphaBlendTable[oriColor.B][255 - alpha];
                    c.G = alphaBlendTable[c.G][alpha] + alphaBlendTable[oriColor.G][255 - alpha];
                    c.R = alphaBlendTable[c.R][alpha] + alphaBlendTable[oriColor.R][255 - alpha];
                }
                if (isMultiSelected) {
                    RGBClass* selColor = reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor);
                    c.B = (c.B * 2 + selColor->B) / 3;
                    c.G = (c.G * 2 + selColor->G) / 3;
                    c.R = (c.R * 2 + selColor->R) / 3;
                }
                memcpy(destPtr, &c, BPP);
            }
        }
    }
}

void CIsoViewExt::BlitSHPTransparent(CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, ImageDataClassSafe* pd, Palette* newPal, BYTE alpha, COLORREF houseColor, int extraLightType, bool remap)
{
    if (alpha == 0 || !pd || pd->Flag == ImageDataFlag::SurfaceData || !pd->pImageBuffer || !dst) {
        return;
    }

    const int X_OFFSET = 31;
    const int Y_OFFSET = -29;
    const int BPP = 4;

    x += X_OFFSET;
    y += Y_OFFSET;

    BYTE* src = static_cast<BYTE*>(pd->pImageBuffer.get());
    int swidth = pd->FullWidth;
    int sheight = pd->FullHeight;

    if (x + swidth < window.left || y + sheight < window.top) {
        return;
    }
    if (x >= window.right || y >= window.bottom) {
        return;
    }

    RECT srcRect = { 0, 0, swidth, sheight };
    RECT destRect = { x, y, x + swidth, y + sheight };
    if (destRect.left < 0) {
        srcRect.left = 1 - destRect.left;
    }
    if (destRect.top < 0) {
        srcRect.top = 1 - destRect.top;
    }
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        destRect.right = window.right;
    }
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        destRect.bottom = window.bottom;
    }

    if (!newPal) {
        newPal = pd->pPalette;
    }
    bool isMultiSelected = false;
    if (extraLightType == -10 || extraLightType >= 500) {
        isMultiSelected = MultiSelection::IsSelected(CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y);
        if (extraLightType >= 500) {
            int overlay = extraLightType - 500;
            if (overlay == 0x18 || overlay == 0x19 || // BRIDGE1, BRIDGE2
                overlay == 0x3B || overlay == 0x3C || // RAILBRDG1, RAILBRDG2
                overlay == 0xED || overlay == 0xEE || // BRIDGEB1, BRIDGEB2
                (overlay >= 0x4A && overlay <= 0x65) || // LOBRDG 1-28
                (overlay >= 0xCD && overlay <= 0xEC)) { // LOBRDGB 1-4
                isMultiSelected = MultiSelection::IsSelected(
                    CIsoViewExt::CurrentDrawCellLocation.X + 1,
                    CIsoViewExt::CurrentDrawCellLocation.Y + 1);
            }
        }
    }

    BGRStruct color;
    auto pRGB = reinterpret_cast<ColorStruct*>(&houseColor);
    color.R = pRGB->red;
    color.G = pRGB->green;
    color.B = pRGB->blue;
    if (LightingStruct::CurrentLighting != LightingStruct::NoLighting) {
        if (extraLightType >= 500) {
            newPal = PalettesManager::GetOverlayPalette(newPal, CIsoViewExt::CurrentDrawCellLocation, extraLightType - 500);
        }
        else if (extraLightType <= -100){
            // do not tint color
        }
        else {
            newPal = PalettesManager::GetObjectPalette(newPal, color, remap, CIsoViewExt::CurrentDrawCellLocation, false, extraLightType);
        }
    }
    else {
        newPal = PalettesManager::GetPalette(newPal, color, remap);
    }

    BYTE* srcBase = src;
    BYTE* destBase = static_cast<BYTE*>(dst) + destRect.top * boundary.dpitch + destRect.left * BPP;
    BYTE* surfaceEnd = static_cast<BYTE*>(dst) + boundary.dpitch * boundary.dwHeight;

    for (LONG row = srcRect.top; row < srcRect.bottom; ++row) {
        LONG left = pd->pPixelValidRanges[row].First;
        LONG right = pd->pPixelValidRanges[row].Last;
        if (left < srcRect.left) {
            left = srcRect.left;
        }
        if (right >= srcRect.right) {
            right = srcRect.right - 1;
        }
        if (left > right) {
            continue;
        }

        BYTE* srcPtr = srcBase + row * swidth + left;
        BYTE* destPtr = destBase + row * boundary.dpitch + left * BPP;
        for (LONG col = left; col <= right; ++col, ++srcPtr, destPtr += BPP) {
            if (destRect.left + col < 0) {
                continue;
            }

            BYTE pixelValue = *srcPtr;
            if (pixelValue && destPtr >= dst && destPtr + BPP <= surfaceEnd) {
                BGRStruct c = newPal->Data[pixelValue];
                if (alpha < 255) {
                    BGRStruct oriColor = *reinterpret_cast<BGRStruct*>(destPtr);
                    c.B = alphaBlendTable[c.B][alpha] + alphaBlendTable[oriColor.B][255 - alpha];
                    c.G = alphaBlendTable[c.G][alpha] + alphaBlendTable[oriColor.G][255 - alpha];
                    c.R = alphaBlendTable[c.R][alpha] + alphaBlendTable[oriColor.R][255 - alpha];
                }
                if (isMultiSelected) {
                    RGBClass* selColor = reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor);
                    c.B = (c.B * 2 + selColor->B) / 3;
                    c.G = (c.G * 2 + selColor->G) / 3;
                    c.R = (c.R * 2 + selColor->R) / 3;
                }
                memcpy(destPtr, &c, BPP);
            }
        }
    }
}

void CIsoViewExt::BlitSHPTransparent_Building(CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, ImageDataClassSafe* pd, Palette* newPal, BYTE alpha,
    COLORREF houseColor, COLORREF addOnColor, bool isRubble, bool isTerrain)
{
    if (alpha == 0 || !pd || pd->Flag == ImageDataFlag::SurfaceData || !pd->pImageBuffer || !dst) {
        return;
    }

    const int X_OFFSET = 31;
    const int Y_OFFSET = -29;
    const int BPP = 4; 

    x += X_OFFSET;
    y += Y_OFFSET;

    BYTE* src = static_cast<BYTE*>(pd->pImageBuffer.get());
    int swidth = pd->FullWidth;
    int sheight = pd->FullHeight;

    if (x + swidth < window.left || y + sheight < window.top) {
        return;
    }
    if (x >= window.right || y >= window.bottom) {
        return;
    }

    RECT srcRect = { 0, 0, swidth, sheight };
    RECT destRect = { x, y, x + swidth, y + sheight };
    if (destRect.left < 0) {
        srcRect.left = 1 - destRect.left;
    }
    if (destRect.top < 0) {
        srcRect.top = 1 - destRect.top;
    }
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        destRect.right = window.right;
    }
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        destRect.bottom = window.bottom;
    }

    if (!newPal) {
        newPal = pd->pPalette;
    }
    BGRStruct color;
    auto pRGB = reinterpret_cast<ColorStruct*>(&houseColor);
    color.R = pRGB->red;
    color.G = pRGB->green;
    color.B = pRGB->blue;
    if (LightingStruct::CurrentLighting == LightingStruct::NoLighting) {
        newPal = PalettesManager::GetPalette(newPal, color, !isTerrain && !isRubble);
    }
    else {
        newPal = PalettesManager::GetObjectPalette(newPal, color, !isTerrain && !isRubble,
            CIsoViewExt::CurrentDrawCellLocation, false, isRubble || isTerrain ? 4 : 3);
    }

    BYTE* srcBase = src;
    BYTE* destBase = static_cast<BYTE*>(dst) + destRect.top * boundary.dpitch + destRect.left * BPP;
    BYTE* surfaceEnd = static_cast<BYTE*>(dst) + boundary.dpitch * boundary.dwHeight;

    for (LONG row = srcRect.top; row < srcRect.bottom; ++row) {
        LONG left = pd->pPixelValidRanges[row].First;
        LONG right = pd->pPixelValidRanges[row].Last;
        if (left < srcRect.left) {
            left = srcRect.left;
        }
        if (right >= srcRect.right) {
            right = srcRect.right - 1;
        }
        if (left > right) {
            continue;
        }

        BYTE* srcPtr = srcBase + row * swidth + left;
        BYTE* destPtr = destBase + row * boundary.dpitch + left * BPP;
        for (LONG col = left; col <= right; ++col, ++srcPtr, destPtr += BPP) {
            if (destRect.left + col < 0) {
                continue;
            }

            BYTE pixelValue = *srcPtr;
            if (pixelValue && destPtr >= dst && destPtr + BPP <= surfaceEnd) {
                BGRStruct c = newPal->Data[pixelValue];
                if (alpha < 255) {
                    BGRStruct oriColor = *reinterpret_cast<BGRStruct*>(destPtr);
                    c.B = alphaBlendTable[c.B][alpha] + alphaBlendTable[oriColor.B][255 - alpha];
                    c.G = alphaBlendTable[c.G][alpha] + alphaBlendTable[oriColor.G][255 - alpha];
                    c.R = alphaBlendTable[c.R][alpha] + alphaBlendTable[oriColor.R][255 - alpha];
                }
                memcpy(destPtr, &c, BPP);
            }
        }
    }
}

void CIsoViewExt::BlitSHPTransparent_AlphaImage(CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, ImageDataClassSafe* pd)
{
    if (!pd || pd->Flag == ImageDataFlag::SurfaceData || !pd->pImageBuffer || !dst) {
        return;
    }

    const int X_OFFSET = 31;
    const int Y_OFFSET = -29;
    const int BPP = 4; 

    x += X_OFFSET;
    y += Y_OFFSET;

    BYTE* src = static_cast<BYTE*>(pd->pImageBuffer.get());
    int swidth = pd->FullWidth;
    int sheight = pd->FullHeight;

    if (x + swidth < window.left || y + sheight < window.top) {
        return;
    }
    if (x >= window.right || y >= window.bottom) {
        return;
    }

    RECT srcRect = { 0, 0, swidth, sheight };
    RECT destRect = { x, y, x + swidth, y + sheight };
    if (destRect.left < 0) {
        srcRect.left = 1 - destRect.left;
    }
    if (destRect.top < 0) {
        srcRect.top = 1 - destRect.top;
    }
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        destRect.right = window.right;
    }
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        destRect.bottom = window.bottom;
    }

    BYTE* srcBase = src;
    BYTE* destBase = static_cast<BYTE*>(dst) + destRect.top * boundary.dpitch + destRect.left * BPP;
    BYTE* surfaceEnd = static_cast<BYTE*>(dst) + boundary.dpitch * boundary.dwHeight;

    for (LONG row = srcRect.top; row < srcRect.bottom; ++row) {
        LONG left = srcRect.left;
        LONG right = srcRect.right - 1;
        if (left > right) {
            continue;
        }

        BYTE* srcPtr = srcBase + row * swidth + left;
        BYTE* destPtr = destBase + row * boundary.dpitch + left * BPP;
        for (LONG col = left; col <= right; ++col, ++srcPtr, destPtr += BPP) {
            if (destRect.left + col < 0) {
                continue;
            }

            BYTE pixelValue = *srcPtr;
            if (pixelValue != 127 && destPtr >= dst && destPtr + BPP <= surfaceEnd) {
                BGRStruct oriColor = *reinterpret_cast<BGRStruct*>(destPtr);
                BGRStruct c;
                c.B = std::min(static_cast<int>(oriColor.B * pixelValue * 2 / 256), 255);
                c.G = std::min(static_cast<int>(oriColor.G * pixelValue * 2 / 256), 255);
                c.R = std::min(static_cast<int>(oriColor.R * pixelValue * 2 / 256), 255);
                memcpy(destPtr, &c, BPP);
            }
        }
    }
}

void CIsoViewExt::BlitSHPTransparent(LPDDSURFACEDESC2 lpDesc, int x, int y, ImageDataClass* pd, Palette* newPal, BYTE alpha, COLORREF houseColor)
{  
    auto pThis = CIsoView::GetInstance();
    RECT window = CIsoViewExt::GetScaledWindowRect();
    DDBoundary boundary{ lpDesc->dwWidth, lpDesc->dwHeight, lpDesc->lPitch };
    CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary, x, y, pd, newPal, alpha, houseColor);
}

void CIsoViewExt::BlitSHPTransparent(LPDDSURFACEDESC2 lpDesc, int x, int y, ImageDataClassSafe* pd, Palette* newPal, BYTE alpha, COLORREF houseColor)
{  
    auto pThis = CIsoView::GetInstance();
    RECT window = CIsoViewExt::GetScaledWindowRect();
    DDBoundary boundary{ lpDesc->dwWidth, lpDesc->dwHeight, lpDesc->lPitch };
    CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary, x, y, pd, newPal, alpha, houseColor);
}

void CIsoViewExt::BlitTerrain(CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, CTileBlockClass* subTile, Palette* pal, BYTE alpha)
{
    if (alpha == 0 || !subTile || !subTile->HasValidImage || !subTile->ImageData || !dst || !subTile->pPixelValidRanges) {
        return;
    }

    const int TILE_WIDTH = 60;
    const int TILE_HEIGHT = 30;
    const int X_OFFSET = 61;
    const int Y_OFFSET = 1;
    const int BPP = 4;

    x += X_OFFSET;
    y += Y_OFFSET;

    int swidth = subTile->BlockWidth;
    int sheight = subTile->BlockHeight;
    RECT srcRect = { 0, 0, swidth, sheight };
    RECT destRect = { x, y, x + swidth, y + sheight };

    srcRect.left = 0;
    srcRect.top = 0;
    srcRect.right = swidth;
    srcRect.bottom = sheight;
    if (destRect.left < 0) {
        srcRect.left = 1 - destRect.left;
    }
    if (destRect.top < 0) {
        srcRect.top = 1 - destRect.top;
    }
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        destRect.right = window.right;
    }
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        destRect.bottom = window.bottom;
    }

    if (srcRect.right <= srcRect.left || srcRect.bottom <= srcRect.top) {
        return;
    }

    Palette* newPal = pal;
    BGRStruct color;
    if (LightingStruct::CurrentLighting != LightingStruct::NoLighting) {
        newPal = PalettesManager::GetObjectPalette(pal, color, false, CIsoViewExt::CurrentDrawCellLocation);
    }
    bool multiSelected = MultiSelection::IsSelected(CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y);

    BYTE* srcBase = static_cast<BYTE*>(subTile->ImageData);
    BYTE* destBase = static_cast<BYTE*>(dst) + destRect.top * boundary.dpitch + destRect.left * BPP;
    BYTE* surfaceEnd = static_cast<BYTE*>(dst) + boundary.dpitch * boundary.dwHeight;

    for (LONG row = srcRect.top; row < srcRect.bottom; ++row) {
        LONG left = std::max((LONG)subTile->pPixelValidRanges[row].First, srcRect.left);
        LONG right = std::min((LONG)subTile->pPixelValidRanges[row].Last, srcRect.right - 1);
        if (left > right) {
            continue;
        }

        BYTE* srcPtr = srcBase + row * swidth + left;
        BYTE* destPtr = destBase + row * boundary.dpitch + left * BPP;

        for (LONG col = left; col <= right; ++col, ++srcPtr, destPtr += BPP) {
            if (ExtConfigs::FlatToGroundHideExtra && CFinalSunApp::Instance->FlatToGround) {
                int posInTile = col + subTile->XMinusExX + (row + subTile->YMinusExY) * TILE_WIDTH;
                if (col + subTile->XMinusExX < 0 ||
                    col + subTile->XMinusExX >= TILE_WIDTH ||
                    row + subTile->YMinusExY < 0 ||
                    row + subTile->YMinusExY >= TILE_HEIGHT ||
                    !TilePixels[posInTile]) {
                    continue;
                }
            }

            BYTE pixelValue = *srcPtr;
            if (pixelValue && destPtr + BPP <= surfaceEnd) {
                BGRStruct c = newPal->Data[pixelValue];

                if (alpha < 255) {
                    BGRStruct oriColor = *reinterpret_cast<BGRStruct*>(destPtr);
                    c.B = alphaBlendTable[c.B][alpha] + alphaBlendTable[oriColor.B][255 - alpha];
                    c.G = alphaBlendTable[c.G][alpha] + alphaBlendTable[oriColor.G][255 - alpha];
                    c.R = alphaBlendTable[c.R][alpha] + alphaBlendTable[oriColor.R][255 - alpha];
                }

                if (multiSelected) {
                    RGBClass* selColor = reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor);
                    c.B = (c.B * 2 + selColor->B) / 3;
                    c.G = (c.G * 2 + selColor->G) / 3;
                    c.R = (c.R * 2 + selColor->R) / 3;
                }

                memcpy(destPtr, &c, BPP);
            }
        }
    }
}

void CIsoViewExt::ScaleBitmap(CBitmap* pBitmap, int maxSize, COLORREF bgColor, bool removeHalo, bool trim)
{
    if (!pBitmap || maxSize <= 0) return;

    BITMAP bmpInfo = {};
    pBitmap->GetBitmap(&bmpInfo);
    int srcW = bmpInfo.bmWidth;
    int srcH = bmpInfo.bmHeight;

    if (bmpInfo.bmWidth == maxSize && bmpInfo.bmHeight == maxSize)
        return;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = srcW;
    bmi.bmiHeader.biHeight = -srcH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<DWORD> srcPixels(srcW * srcH);
    {
        HDC hdc = GetDC(NULL);
        GetDIBits(hdc, (HBITMAP)(*pBitmap), 0, srcH, srcPixels.data(), &bmi, DIB_RGB_COLORS);
        ReleaseDC(NULL, hdc);
    }

    int left = srcW, right = 0, top = srcH, bottom = 0;
    DWORD bgRGB = RGB(GetBValue(bgColor), GetGValue(bgColor), GetRValue(bgColor));

    if (trim)
    {
        for (int y = 0; y < srcH; ++y)
        {
            for (int x = 0; x < srcW; ++x)
            {
                DWORD px = srcPixels[y * srcW + x];
                COLORREF pxColor = RGB(px & 0xFF, (px >> 8) & 0xFF, (px >> 16) & 0xFF);
                if (pxColor != bgRGB)
                {
                    if (x < left) left = x;
                    if (x > right) right = x;
                    if (y < top) top = y;
                    if (y > bottom) bottom = y;
                }
            }
        }
    }
    else
    {
        left = 0;
        top = 0;
        right = srcW;
        bottom = srcH;
    }

    bool empty = false;
    if (left > right || top > bottom)
    {
        empty = true;
        left = 0;
        top = 0;
        right = srcW;
        bottom = srcH;
    }

    int cropW = right - left + 1;
    int cropH = bottom - top + 1;
    float scale = std::min((float)maxSize / cropW, (float)maxSize / cropH);
    int newW = int(cropW * scale);
    int newH = int(cropH * scale);
    int offsetX = (maxSize - newW) / 2;
    int offsetY = (maxSize - newH) / 2;

    bmi.bmiHeader.biWidth = maxSize;
    bmi.bmiHeader.biHeight = -maxSize;

    void* pDstBits = nullptr;
    HBITMAP hNewBmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pDstBits, NULL, 0);
    if (!hNewBmp || !pDstBits) return;

    DWORD* dst = (DWORD*)pDstBits;

    if (empty)
    {
        DWORD bgARGB = 0xFF000000 | (GetRValue(bgColor) << 16) | (GetGValue(bgColor) << 8) | GetBValue(bgColor);
        std::fill(dst, dst + maxSize * maxSize, bgARGB);
        pBitmap->DeleteObject();
        pBitmap->Attach(hNewBmp);
        return;
    }

    DWORD bgARGB = 0xFF000000 | (GetRValue(bgColor) << 16) | (GetGValue(bgColor) << 8) | GetBValue(bgColor);
    std::fill(dst, dst + maxSize * maxSize, bgARGB);

    for (int y = 0; y < newH; ++y)
    {
        for (int x = 0; x < newW; ++x)
        {
            float fx = left + x / scale;
            float fy = top + y / scale;

            int x0 = (int)fx;
            int y0 = (int)fy;
            int x1 = std::min(x0 + 1, srcW - 1);
            int y1 = std::min(y0 + 1, srcH - 1);

            x0 = std::clamp(x0, 0, srcW - 1);
            x1 = std::clamp(x1, 0, srcW - 1);
            y0 = std::clamp(y0, 0, srcH - 1);
            y1 = std::clamp(y1, 0, srcH - 1);

            float dx = fx - x0;
            float dy = fy - y0;

            DWORD c00 = srcPixels[y0 * srcW + x0];
            DWORD c10 = srcPixels[y0 * srcW + x1];
            DWORD c01 = srcPixels[y1 * srcW + x0];
            DWORD c11 = srcPixels[y1 * srcW + x1];

            auto extractRGB = [](DWORD c) {
                return std::tuple<int, int, int>(
                    c & 0xFF,
                    (c >> 8) & 0xFF,
                    (c >> 16) & 0xFF
                );
                };

            COLORREF cBG = bgRGB;

            auto isBG = [=](DWORD c) {
                COLORREF pxColor = RGB(c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF);
                return pxColor == cBG;
                };

            int r = 0, g = 0, b = 0;
            float totalWeight = 0.0f;

            auto blend = [&](DWORD color, float weight) {
                if (!isBG(color))
                {
                    int cr, cg, cb;
                    std::tie(cb, cg, cr) = extractRGB(color);
                    r += int(cr * weight);
                    g += int(cg * weight);
                    b += int(cb * weight);
                    totalWeight += weight;
                }
                };

            blend(c00, (1 - dx) * (1 - dy));
            blend(c10, dx * (1 - dy));
            blend(c01, (1 - dx) * dy);
            blend(c11, dx * dy);

            DWORD result = bgARGB;
            if (totalWeight > 0.0f)
            {
                r = int(r / totalWeight);
                g = int(g / totalWeight);
                b = int(b / totalWeight);
                result = 0xFF000000 | (r << 16) | (g << 8) | b;
            }

            int dxDst = offsetX + x;
            int dyDst = offsetY + y;
            dst[dyDst * maxSize + dxDst] = result;
        }
    }

    if (removeHalo)
    {
        for (int i = 0; i < maxSize * maxSize; ++i)
        {
            DWORD& px = dst[i];
            COLORREF c = RGB(px & 0xFF, (px >> 8) & 0xFF, (px >> 16) & 0xFF);
            if (c == bgRGB)
            {
                px = 0x00000000 | (GetRValue(bgColor) << 16) | (GetGValue(bgColor) << 8) | GetBValue(bgColor);
            }
        }
    }
    pBitmap->DeleteObject();
    pBitmap->Attach(hNewBmp);
    return;
}

void CIsoViewExt::MaskShadowPixels(const RECT& window, int x, int y, ImageDataClassSafe* pd, std::vector<char>& mask)
{
    if (!pd || !pd->pImageBuffer || !pd->pPixelValidRanges || mask.empty()) {
        return;
    }

    const int X_OFFSET = 31;
    const int Y_OFFSET = -29;
    x += X_OFFSET;
    y += Y_OFFSET;

    BYTE* src = static_cast<BYTE*>(pd->pImageBuffer.get());
    int swidth = pd->FullWidth;
    int sheight = pd->FullHeight;

    if (x + swidth < window.left || y + sheight < window.top) {
        return;
    }
    if (x >= window.right || y >= window.bottom) {
        return;
    }

    RECT srcRect = { 0, 0, swidth, sheight };
    RECT destRect = { x, y, x + swidth, y + sheight };
    if (destRect.left < 0) {
        srcRect.left = 1 - destRect.left;
    }
    if (destRect.top < 0) {
        srcRect.top = 1 - destRect.top;
    }
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        destRect.right = window.right;
    }
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        destRect.bottom = window.bottom;
    }

    const int maskWidth = window.right - window.left;
    const int maskHeight = window.bottom - window.top;
    if (mask.size() < static_cast<size_t>(maskWidth * maskHeight)) {
        return;
    }

    BYTE* srcBase = src;
    for (LONG row = srcRect.top; row < srcRect.bottom; ++row) {
        LONG left = std::max(static_cast<LONG>(pd->pPixelValidRanges[row].First), srcRect.left);
        LONG right = std::min(static_cast<LONG>(pd->pPixelValidRanges[row].Last), srcRect.right - 1);
        if (left > right) {
            continue;
        }

        BYTE* srcPtr = srcBase + row * swidth + left;
        LONG destXBase = destRect.left + left;

        for (LONG col = left; col <= right; ++col, ++srcPtr) {
            BYTE pixelValue = *srcPtr;
            if (!pixelValue) {
                continue;
            }

            LONG localX = destXBase + (col - left) - window.left;
            LONG localY = destRect.top + row - window.top;

            if (localX >= 0 && localX < maskWidth && localY >= 0 && localY < maskHeight) {
                size_t maskIndex = static_cast<size_t>(localY * maskWidth + localX);
                mask[maskIndex] = 1;
            }
        }
    }
}

void CIsoViewExt::DrawShadowMask(void* dst, const DDBoundary& boundary, const RECT& window, const std::vector<byte>& mask)
{
    if (!dst || mask.empty()) {
        return;
    }

    const int BPP = 4;
    const BGRStruct SHADOW_COLOR = { 0, 0, 0 };
    const BYTE ALPHA = 128;
    const int maskWidth = window.right - window.left;
    const int maskHeight = window.bottom - window.top;
    const int width = boundary.dwWidth;
    const int height = boundary.dwHeight;

    if (mask.size() < static_cast<size_t>(maskWidth * maskHeight)) {
        return;
    }

    BYTE* base = static_cast<BYTE*>(dst);
    BYTE* surfaceEnd = base + boundary.dpitch * height;

    for (LONG y = 0; y < maskHeight; ++y) {
        LONG dy = window.top + y;
        if (dy < 0 || dy >= height) {
            continue;
        }

        BYTE* destLine = base + dy * boundary.dpitch + window.left * BPP;
        for (LONG x = 0; x < maskWidth; ++x) {
            BYTE count = mask[y * maskWidth + x];
            if (count == 0) {
                continue;
            }

            LONG dx = window.left + x;
            if (dx < 0 || dx >= width) {
                continue;
            }

            BYTE* dest = destLine + x * BPP;
            if (dest + BPP > surfaceEnd) {
                continue;
            }

            BGRStruct oriColor = *reinterpret_cast<BGRStruct*>(dest);
            BGRStruct blended = oriColor;

            BYTE keep = alphaBlendTable[255][255 - ALPHA];
            BYTE blend = 255 - keep;
            blended.R = alphaBlendTable[SHADOW_COLOR.R][blend] + alphaBlendTable[oriColor.R][keep];
            blended.G = alphaBlendTable[SHADOW_COLOR.G][blend] + alphaBlendTable[oriColor.G][keep];
            blended.B = alphaBlendTable[SHADOW_COLOR.B][blend] + alphaBlendTable[oriColor.B][keep];

            memcpy(dest, &blended, BPP);
        }
    }
}

std::vector<MapCoord> CIsoViewExt::GetTubePath(int x1, int y1, int x2, int y2, bool first)
{
    std::vector<MapCoord> path;
    int x = x1, y = y1;
    path.emplace_back(x, y);

    while (x != x2 || y != y2) {
        int dx = x2 - x;
        int dy = y2 - y;

        int step_x = (dx == 0 ? 0 : (dx > 0 ? 1 : -1));
        int step_y = (dy == 0 ? 0 : (dy > 0 ? 1 : -1));

        x += step_x;
        y += step_y;

        path.emplace_back(x, y);
    }
    int size = path.size();
    if (size > 2)
    {
        if (first && !(path[size - 1].X != x2 && path[size - 1].Y != y2 || path[size - 2].X != x2 && path[size - 2].Y != y2))
        {
            if (abs(x1 - x2) >= abs(y1 - y2))
            {
                if (x1 >= x2)
                {
                    path = GetTubePath(x1 - 2, y1, x2, y2, false);
                    path.insert(path.begin(), { x1 - 1,y1 });
                    path.insert(path.begin(), { x1,y1 });
                }
                else
                {
                    path = GetTubePath(x1 + 2, y1, x2, y2, false);
                    path.insert(path.begin(), { x1 + 1,y1 });
                    path.insert(path.begin(), { x1,y1 });
                }
            }
            else
            {
                if (y1 >= y2)
                {
                    path = GetTubePath(x1, y1 - 2, x2, y2, false);
                    path.insert(path.begin(), { x1,y1 - 1 });
                    path.insert(path.begin(), { x1,y1 });
                }
                else
                {
                    path = GetTubePath(x1, y1 + 2, x2, y2, false);
                    path.insert(path.begin(), { x1,y1 + 1 });
                    path.insert(path.begin(), { x1,y1 });
                }
            }
            return path;
        }
    }
    return path;
}

std::vector<int> CIsoViewExt::GetTubeDirections(const std::vector<MapCoord>& path)
{
    std::vector<int> directions;

    for (size_t i = 1; i < path.size(); ++i) {
        int dx = path[i].X - path[i - 1].X;
        int dy = path[i].Y - path[i - 1].Y;

        int dirCode = -1;
        if (dx == -1 && dy == 0) dirCode = 0;
        else if (dx == -1 && dy == 1) dirCode = 1;
        else if (dx == 0 && dy == 1) dirCode = 2;
        else if (dx == 1 && dy == 1) dirCode = 3;
        else if (dx == 1 && dy == 0) dirCode = 4;
        else if (dx == 1 && dy == -1) dirCode = 5;
        else if (dx == 0 && dy == -1) dirCode = 6;
        else if (dx == -1 && dy == -1) dirCode = 7;

        directions.push_back(dirCode);
    }

    return directions;
}

std::vector<MapCoord> CIsoViewExt::GetPathFromDirections(int x0, int y0, const std::vector<int>& directions)
{
    std::vector<MapCoord> path;
    path.emplace_back(x0, y0);

    const int dx[8] = { -1, -1,  0,  1, 1, 1,  0, -1 };
    const int dy[8] = { 0,  1,  1,  1, 0, -1, -1, -1 };

    int x = x0, y = y0;

    for (int dir : directions) {
        x += dx[dir];
        y += dy[dir];
        path.emplace_back(x, y);
    }

    return path;
}

RECT CIsoViewExt::GetScaledWindowRect()
{
    CRect rect;
    auto pThis = CIsoView::GetInstance();
    pThis->GetWindowRect(&rect);
    rect.right += rect.Width() * (CIsoViewExt::ScaledFactor - 1.0);
    rect.bottom += rect.Height() * (CIsoViewExt::ScaledFactor - 1.0);
    return rect;
}

void CIsoViewExt::ReduceBrightness(IDirectDrawSurface7* pSurface, const RECT& rc)
{
    if (!ExtConfigs::EnableDarkMode || !ExtConfigs::EnableDarkMode_DimMap || !pSurface) return;

    DDSURFACEDESC2 ddsd = { sizeof(ddsd) };
    HRESULT hr = pSurface->Lock(const_cast<RECT*>(&rc), &ddsd, DDLOCK_WAIT, nullptr);
    if (FAILED(hr)) return;

    BYTE* pBits = static_cast<BYTE*>(ddsd.lpSurface);
    if (!pBits) {
        pSurface->Unlock(nullptr);
        return;
    }

    for (LONG y = 0; y < (rc.bottom - rc.top); ++y) {
        DWORD* pLine = reinterpret_cast<DWORD*>(pBits + y * ddsd.lPitch);
        for (LONG x = 0; x < (rc.right - rc.left); ++x) {
            DWORD c = pLine[x];
            BYTE r = (c >> 16) & 0xFF;
            BYTE g = (c >> 8) & 0xFF;
            BYTE b = c & 0xFF;
            r = BrightnessLUT[r];
            g = BrightnessLUT[g];
            b = BrightnessLUT[b];
            pLine[x] = (c & 0xFF000000) | (r << 16) | (g << 8) | b;
        }
    }

    pSurface->Unlock(nullptr);
}


void CIsoViewExt::MapCoord2ScreenCoord(int& X, int& Y, int flatMode)
{
    CRect rect;
    auto pThis = CIsoView::GetInstance();
    pThis->GetWindowRect(&rect);
    if (flatMode == 0)
        pThis->MapCoord2ScreenCoord(X, Y);
    else if (flatMode == 1)
        pThis->MapCoord2ScreenCoord_Flat(X, Y);
    else
        pThis->MapCoord2ScreenCoord_Height(X, Y);
    X = (X - pThis->ViewPosition.x - rect.left) / CIsoViewExt::ScaledFactor + pThis->ViewPosition.x + rect.left;
    Y = (Y - pThis->ViewPosition.y - rect.top) / CIsoViewExt::ScaledFactor + pThis->ViewPosition.y + rect.top;
}

bool CIsoViewExt::StretchCopySurfaceBilinear(
    LPDIRECTDRAWSURFACE7 srcSurface, CRect srcRect,
    LPDIRECTDRAWSURFACE7 dstSurface, CRect dstRect)
{
    if (!ExtConfigs::DDrawScalingBilinear ||
        (ExtConfigs::DDrawScalingBilinear_OnlyShrink && ScaledFactor < 1)) {
        dstSurface->Blt(&dstRect, srcSurface, &srcRect, DDBLT_WAIT, 0);
        return true;
    }

    DDSURFACEDESC2 srcDesc = { sizeof(DDSURFACEDESC2) };
    DDSURFACEDESC2 dstDesc = { sizeof(DDSURFACEDESC2) };

    if (srcSurface->Lock(NULL, &srcDesc, DDLOCK_READONLY | DDLOCK_WAIT, NULL) != DD_OK)
        return false;
    if (dstSurface->Lock(NULL, &dstDesc, DDLOCK_WAIT, NULL) != DD_OK) {
        srcSurface->Unlock(NULL);
        return false;
    }

    uint8_t* srcBits = (uint8_t*)srcDesc.lpSurface;
    uint8_t* dstBits = (uint8_t*)dstDesc.lpSurface;
    int srcPitch = srcDesc.lPitch;
    int dstPitch = dstDesc.lPitch;

    int srcW = srcRect.Width();
    int srcH = srcRect.Height();
    int dstW = dstRect.Width();
    int dstH = dstRect.Height();

    struct LookupCache {
        int w = 0, h = 0;
        std::vector<int> srcX0, srcX1, srcY0, srcY1;
        std::vector<uint8_t> fx, fy;
        double factor = 0.0;
    };
    static LookupCache cache;
    static std::mutex cacheMutex;

    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (cache.factor != CIsoViewExt::ScaledFactor || cache.w != dstW || cache.h != dstH) {
            cache.factor = CIsoViewExt::ScaledFactor;
            cache.w = dstW;
            cache.h = dstH;
            cache.srcX0.resize(dstW);
            cache.srcX1.resize(dstW);
            cache.fx.resize(dstW);
            cache.srcY0.resize(dstH);
            cache.srcY1.resize(dstH);
            cache.fy.resize(dstH);

            for (int x = 0; x < dstW; ++x) {
                float u = (x + 0.5f) * srcW / (float)dstW - 0.5f;
                int ix = (int)floor(u);
                float fx = u - ix;
                ix = std::clamp(ix, 0, srcW - 1);
                cache.srcX0[x] = ix;
                cache.srcX1[x] = std::min(ix + 1, srcW - 1);
                cache.fx[x] = (uint8_t)(fx * 255.0f + 0.5f);
            }
            for (int y = 0; y < dstH; ++y) {
                float v = (y + 0.5f) * srcH / (float)dstH - 0.5f;
                int iy = (int)floor(v);
                float fy = v - iy;
                iy = std::clamp(iy, 0, srcH - 1);
                cache.srcY0[y] = iy;
                cache.srcY1[y] = std::min(iy + 1, srcH - 1);
                cache.fy[y] = (uint8_t)(fy * 255.0f + 0.5f);
            }
        }
    }

    auto& srcX0 = cache.srcX0;
    auto& srcX1 = cache.srcX1;
    auto& fxTable = cache.fx;
    auto& srcY0 = cache.srcY0;
    auto& srcY1 = cache.srcY1;
    auto& fyTable = cache.fy;

    for (int y = 0; y < dstH; ++y) {
        int sy0 = srcY0[y];
        int sy1 = srcY1[y];
        uint8_t fy = fyTable[y];

        uint8_t* dstRow = dstBits + (dstRect.top + y) * dstPitch + dstRect.left * 4;
        uint8_t* row0 = srcBits + (srcRect.top + sy0) * srcPitch + srcRect.left * 4;
        uint8_t* row1 = srcBits + (srcRect.top + sy1) * srcPitch + srcRect.left * 4;

        int x = 0;

        if (ExtConfigs::AVX2_Support)
        {
            for (; x + 7 < dstW; x += 8) {
                __m256i outPix;
                uint32_t out[8];
                for (int i = 0; i < 8; i++) {
                    int sx0 = srcX0[x + i];
                    int sx1 = srcX1[x + i];
                    uint8_t fx = fxTable[x + i];

                    uint32_t c00 = *(uint32_t*)(row0 + sx0 * 4);
                    uint32_t c10 = *(uint32_t*)(row0 + sx1 * 4);
                    uint32_t c01 = *(uint32_t*)(row1 + sx0 * 4);
                    uint32_t c11 = *(uint32_t*)(row1 + sx1 * 4);

                    auto interp = [&](int c00, int c10, int c01, int c11) {
                        int r00 = (c00 >> 16) & 0xFF, g00 = (c00 >> 8) & 0xFF, b00 = c00 & 0xFF;
                        int r10 = (c10 >> 16) & 0xFF, g10 = (c10 >> 8) & 0xFF, b10 = c10 & 0xFF;
                        int r01 = (c01 >> 16) & 0xFF, g01 = (c01 >> 8) & 0xFF, b01 = c01 & 0xFF;
                        int r11 = (c11 >> 16) & 0xFF, g11 = (c11 >> 8) & 0xFF, b11 = c11 & 0xFF;

                        int rTop = ((r00 * (255 - fx)) + (r10 * fx)) >> 8;
                        int gTop = ((g00 * (255 - fx)) + (g10 * fx)) >> 8;
                        int bTop = ((b00 * (255 - fx)) + (b10 * fx)) >> 8;

                        int rBot = ((r01 * (255 - fx)) + (r11 * fx)) >> 8;
                        int gBot = ((g01 * (255 - fx)) + (g11 * fx)) >> 8;
                        int bBot = ((b01 * (255 - fx)) + (b11 * fx)) >> 8;

                        int r = ((rTop * (255 - fy)) + (rBot * fy)) >> 8;
                        int g = ((gTop * (255 - fy)) + (gBot * fy)) >> 8;
                        int b = ((bTop * (255 - fy)) + (bBot * fy)) >> 8;
                        return (r << 16) | (g << 8) | b;
                    };
                    out[i] = interp(c00, c10, c01, c11);
                }
                outPix = _mm256_loadu_si256((__m256i*)out);
                _mm256_storeu_si256((__m256i*)(dstRow + x * 4), outPix);
            }
        } 
        else
        {
            for (; x + 3 < dstW; x += 4) {
                __m128i outPix;
                uint32_t out[4];
                for (int i = 0; i < 4; i++) {
                    int sx0 = srcX0[x + i];
                    int sx1 = srcX1[x + i];
                    uint8_t fx = fxTable[x + i];

                    uint32_t c00 = *(uint32_t*)(row0 + sx0 * 4);
                    uint32_t c10 = *(uint32_t*)(row0 + sx1 * 4);
                    uint32_t c01 = *(uint32_t*)(row1 + sx0 * 4);
                    uint32_t c11 = *(uint32_t*)(row1 + sx1 * 4);

                    auto interp = [&](int c00, int c10, int c01, int c11) {
                        int r00 = (c00 >> 16) & 0xFF, g00 = (c00 >> 8) & 0xFF, b00 = c00 & 0xFF;
                        int r10 = (c10 >> 16) & 0xFF, g10 = (c10 >> 8) & 0xFF, b10 = c10 & 0xFF;
                        int r01 = (c01 >> 16) & 0xFF, g01 = (c01 >> 8) & 0xFF, b01 = c01 & 0xFF;
                        int r11 = (c11 >> 16) & 0xFF, g11 = (c11 >> 8) & 0xFF, b11 = c11 & 0xFF;

                        int rTop = ((r00 * (255 - fx)) + (r10 * fx)) >> 8;
                        int gTop = ((g00 * (255 - fx)) + (g10 * fx)) >> 8;
                        int bTop = ((b00 * (255 - fx)) + (b10 * fx)) >> 8;

                        int rBot = ((r01 * (255 - fx)) + (r11 * fx)) >> 8;
                        int gBot = ((g01 * (255 - fx)) + (g11 * fx)) >> 8;
                        int bBot = ((b01 * (255 - fx)) + (b11 * fx)) >> 8;

                        int r = ((rTop * (255 - fy)) + (rBot * fy)) >> 8;
                        int g = ((gTop * (255 - fy)) + (gBot * fy)) >> 8;
                        int b = ((bTop * (255 - fy)) + (bBot * fy)) >> 8;
                        return (r << 16) | (g << 8) | b;
                    };
                    out[i] = interp(c00, c10, c01, c11);
                }
                outPix = _mm_loadu_si128((__m128i*)out);
                _mm_storeu_si128((__m128i*)(dstRow + x * 4), outPix);
            }
        }
        
        for (; x < dstW; ++x) {
            int sx0 = srcX0[x];
            int sx1 = srcX1[x];
            uint8_t fx = fxTable[x];
            uint32_t c00 = *(uint32_t*)(row0 + sx0 * 4);
            uint32_t c10 = *(uint32_t*)(row0 + sx1 * 4);
            uint32_t c01 = *(uint32_t*)(row1 + sx0 * 4);
            uint32_t c11 = *(uint32_t*)(row1 + sx1 * 4);

            int r00 = (c00 >> 16) & 0xFF, g00 = (c00 >> 8) & 0xFF, b00 = c00 & 0xFF;
            int r10 = (c10 >> 16) & 0xFF, g10 = (c10 >> 8) & 0xFF, b10 = c10 & 0xFF;
            int r01 = (c01 >> 16) & 0xFF, g01 = (c01 >> 8) & 0xFF, b01 = c01 & 0xFF;
            int r11 = (c11 >> 16) & 0xFF, g11 = (c11 >> 8) & 0xFF, b11 = c11 & 0xFF;

            int rTop = ((r00 * (255 - fx)) + (r10 * fx)) >> 8;
            int gTop = ((g00 * (255 - fx)) + (g10 * fx)) >> 8;
            int bTop = ((b00 * (255 - fx)) + (b10 * fx)) >> 8;

            int rBot = ((r01 * (255 - fx)) + (r11 * fx)) >> 8;
            int gBot = ((g01 * (255 - fx)) + (g11 * fx)) >> 8;
            int bBot = ((b01 * (255 - fx)) + (b11 * fx)) >> 8;

            int r = ((rTop * (255 - fy)) + (rBot * fy)) >> 8;
            int g = ((gTop * (255 - fy)) + (gBot * fy)) >> 8;
            int b = ((bTop * (255 - fy)) + (bBot * fy)) >> 8;
            *(uint32_t*)(dstRow + x * 4) = (r << 16) | (g << 8) | b;
        }
    }

    srcSurface->Unlock(NULL);
    dstSurface->Unlock(NULL);
    return true;
}

void CIsoViewExt::DrawCreditOnMap(HDC hDC)
{
    auto pThis = CIsoView::GetInstance();
    CRect rect;
    pThis->GetWindowRect(&rect);
    int leftIndex = 0;
    int lineHeight = ExtConfigs::DisplayTextSize + 2;
    ::SetBkMode(hDC, OPAQUE);
    ::SetBkColor(hDC, RGB(0xFF, 0xFF, 0xFF));
    SetTextAlign(hDC, TA_LEFT);
    if (CIsoViewExt::DrawMoneyOnMap)
    {
        FString buffer;
        buffer.Format(Translations::TranslateOrDefault("MoneyOnMap", "Credits On Map: %d"), CMapData::Instance->MoneyCount);
        ::TextOut(hDC, rect.left + 10, rect.top + 10 + lineHeight * leftIndex++, buffer, buffer.GetLength());

        if (ExtConfigs::EnableMultiSelection)
        {
            if (MultiSelection::SelectedCoords.size())
            {
                int nCount = 0;
                auto pExt = CMapDataExt::GetExtension();
                pExt->InitOreValue();
                MultiSelection::ApplyForEach(
                    [&nCount, pExt](CellData& cell, CellDataExt& cellExt) {
                        nCount += pExt->GetOreValue(cellExt.NewOverlay, cell.OverlayData);
                    }
                );

                buffer.Format(Translations::TranslateOrDefault("MoneyOnMap.MultiSelection",
                    "MultiSelection Enabled. Selected Credits: %d"), nCount);
                FString buffer2;
                buffer2.Format(Translations::TranslateOrDefault("MoneyOnMap.MultiSelectionCoords",
                    ", Selected Tiles: %d"), MultiSelection::SelectedCoords.size());
                buffer += buffer2;
                ::TextOut(hDC, rect.left + 10, rect.top + 10 + lineHeight * leftIndex++, buffer, buffer.GetLength());
            }
        }
    }
    if (CFinalSunApp::Instance().FlatToGround)
    {
        FString buffer;
        buffer.Format(Translations::TranslateOrDefault("FlatToGroundModeEnabled", "2D Mode Enabled"));
        ::TextOut(hDC, rect.left + 10, rect.top + 10 + lineHeight * leftIndex++, buffer, buffer.GetLength());
    }
}

void CIsoViewExt::DrawDistanceRuler(HDC hDC)
{
    int fontSize = ExtConfigs::DisplayTextSize;
    if (CIsoViewExt::ScaledFactor < 0.75)
        fontSize += 2;
    if (CIsoViewExt::ScaledFactor < 0.5)
        fontSize += 2;
    if (CIsoViewExt::ScaledFactor < 0.3)
        fontSize += 2;
    int lineHeight = fontSize + 2;
    if (!CIsoViewExt::DistanceRuler.empty())
    {
        for (int i = 0; i < CIsoViewExt::DistanceRuler.size(); ++i)
        {
            int x1 = CIsoViewExt::DistanceRuler[i].X;
            int y1 = CIsoViewExt::DistanceRuler[i].Y;
            int x2, y2 = 0;
            if (i == CIsoViewExt::DistanceRuler.size() - 1)
            {
                auto pIsoView = CIsoView::GetInstance();
                auto point = pIsoView->GetCurrentMapCoord(pIsoView->MouseCurrentPosition);
                x2 = point.X;
                y2 = point.Y;
                if (!CMapData::Instance->IsCoordInMap(x2, y2))
                    return;
            }
            else
            {
                x2 = CIsoViewExt::DistanceRuler[i + 1].X;
                y2 = CIsoViewExt::DistanceRuler[i + 1].Y;
            }
            MapCoord coord1 = { x1,y1 };
            MapCoord coord2 = { x2,y2 };
            double distance = sqrt((coord1.X - coord2.X) * (coord1.X - coord2.X) + (coord1.Y - coord2.Y) * (coord1.Y - coord2.Y));
            FString buffer;
            CIsoViewExt::MapCoord2ScreenCoord(x1, y1);
            CIsoViewExt::MapCoord2ScreenCoord(x2, y2);
            int drawX = x2 - CIsoViewExt::drawOffsetX + 30;
            int drawY = y2 - CIsoViewExt::drawOffsetY - 15;
            if (distance > 0.1)
            {
                CIsoViewExt::DrawLineHDC(hDC, x1, y1, x2, y2, ExtConfigs::DistanceRuler_Color, 2);
                int j = 1;
                std::ostringstream oss;
                oss.precision(2);
                oss << std::fixed << distance;
                buffer.Format(Translations::TranslateOrDefault("DistanceRuler.Distance", "Distance: %s"), oss.str().c_str());
                ::TextOut(hDC, drawX, drawY + lineHeight * j++, buffer, buffer.GetLength());
                buffer.Format(Translations::TranslateOrDefault("DistanceRuler.Coordinate", "XY: %d, %d, ΔXY: %d, %d"),
                    coord2.Y, coord2.X, coord2.Y - coord1.Y, coord2.X - coord1.X);
                ::TextOut(hDC, drawX, drawY + lineHeight * j++, buffer, buffer.GetLength());
            }
            if (i == 0)
            {
                drawX = x1 - CIsoViewExt::drawOffsetX + 30;
                drawY = y1 - CIsoViewExt::drawOffsetY - 15;
                buffer.Format(Translations::TranslateOrDefault("DistanceRuler.InitCoordinate", "XY: %d, %d"),
                    coord1.Y, coord1.X);
                ::TextOut(hDC, drawX, drawY + lineHeight * 1, buffer, buffer.GetLength());
            }
        }
    }
}

CRect CIsoViewExt::GetVisibleIsoViewRect()
{
    auto pThis = CIsoView::GetInstance();
    CRect rect;
    pThis->GetWindowRect(&rect);
    CRect screenRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    CRect destRect;
    destRect.IntersectRect(&rect, &screenRect);
    return destRect;
}

void CIsoViewExt::SpecialDraw(LPDIRECTDRAWSURFACE7 surface, int specialDraw)
{
    switch (specialDraw)
    {
    case 0:
    {
        auto pThis = CIsoView::GetInstance();
        CRect rect = CIsoViewExt::GetVisibleIsoViewRect();
        pThis->lpDDTempBufferSurface->Blt(&rect, surface, &rect, DDBLT_WAIT, 0);
        if (pThis->IsScrolling)
        {
            auto point = pThis->MoveCenterPosition;
            point.x += rect.left - 16 - 18;
            point.y += rect.top + 14 - 12;
            auto cursor = CLoadingExt::GetSurfaceImageDataFromMap("scrollcursor.bmp");
            CIsoViewExt::BlitTransparent(cursor->lpSurface, point.x, point.y, -1, -1, 255, surface);
        }

        HDC hDC;
        surface->GetDC(&hDC);
        int fontSize = ExtConfigs::DisplayTextSize;
        if (CIsoViewExt::ScaledFactor < 0.75)
            fontSize += 2;
        if (CIsoViewExt::ScaledFactor < 0.5)
            fontSize += 2;
        if (CIsoViewExt::ScaledFactor < 0.3)
            fontSize += 2;
        HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Cambria");
        HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

        if (EnableDistanceRuler)
        {
            DrawDistanceRuler(hDC);
        }
        DrawCreditOnMap(hDC);

        SelectObject(hDC, hOldFont);
        DeleteObject(hFont);
        surface->ReleaseDC(hDC);

        break;
    }
    case 1:
    {
        HDC hDC;
        surface->GetDC(&hDC);
        int fontSize = ExtConfigs::DisplayTextSize;
        if (CIsoViewExt::ScaledFactor < 0.75)
            fontSize += 2;
        if (CIsoViewExt::ScaledFactor < 0.5)
            fontSize += 2;
        if (CIsoViewExt::ScaledFactor < 0.3)
            fontSize += 2;
        HFONT hFont = CreateFont(fontSize, 0, 0, 0,  FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Cambria");
        HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

        if (EnableDistanceRuler)
        {
            DrawDistanceRuler(hDC);
        }
        DrawMouseMove(hDC);
        DrawCreditOnMap(hDC);

        SelectObject(hDC, hOldFont);
        DeleteObject(hFont);

        surface->ReleaseDC(hDC);
        break;
    }
    case 2:
    {
        HDC hDC;
        surface->GetDC(&hDC);
        int fontSize = ExtConfigs::DisplayTextSize;
        if (CIsoViewExt::ScaledFactor < 0.75)
            fontSize += 2;
        if (CIsoViewExt::ScaledFactor < 0.5)
            fontSize += 2;
        if (CIsoViewExt::ScaledFactor < 0.3)
            fontSize += 2;
        HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Cambria");
        HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

        if (EnableDistanceRuler)
        {
            DrawDistanceRuler(hDC);
        }
        DrawCopyBound(hDC);
        DrawCreditOnMap(hDC);

        SelectObject(hDC, hOldFont);
        DeleteObject(hFont);
        surface->ReleaseDC(hDC);
        break;
    }
    case 3:
    {
        HDC hDC;
        surface->GetDC(&hDC);

        if (EnableDistanceRuler)
        {
            int fontSize = ExtConfigs::DisplayTextSize;
            if (CIsoViewExt::ScaledFactor < 0.75)
                fontSize += 2;
            if (CIsoViewExt::ScaledFactor < 0.5)
                fontSize += 2;
            if (CIsoViewExt::ScaledFactor < 0.3)
                fontSize += 2;
            HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE, "Cambria");
            HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

            DrawDistanceRuler(hDC);

            SelectObject(hDC, hOldFont);
            DeleteObject(hFont);
        }
        DrawBridgeLine(hDC);
        surface->ReleaseDC(hDC);
        break;
    }
    case 4:
    {
        if (EnableDistanceRuler)
        {
            HDC hDC;
            surface->GetDC(&hDC);
            int fontSize = ExtConfigs::DisplayTextSize;
            if (CIsoViewExt::ScaledFactor < 0.75)
                fontSize += 2;
            if (CIsoViewExt::ScaledFactor < 0.5)
                fontSize += 2;
            if (CIsoViewExt::ScaledFactor < 0.3)
                fontSize += 2;
            HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE, "Cambria");
            HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

            DrawDistanceRuler(hDC);

            SelectObject(hDC, hOldFont);
            DeleteObject(hFont);
            surface->ReleaseDC(hDC);
        }
        break;
    }
    default:
        break;
    }
}

void CIsoViewExt::MoveToMapCoord(int X, int Y)
{
    if (!CMapData::Instance->IsCoordInMap(X, Y))
        return;

    auto pThis = CIsoView::GetInstance();
    int nMapCoord = CMapData::Instance->GetCoordIndex(X, Y);
    RECT rect = GetScaledWindowRect();
    int x = 30 * (CMapData::Instance->MapWidthPlusHeight + X - Y) - (rect.right - rect.left) / 2 - rect.left;
    int y = 15 * (Y + X) - CMapData::Instance->CellDatas[nMapCoord].Height - (rect.bottom - rect.top) / 2 - rect.top;
    pThis->MoveTo(x, y);
    pThis->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    CFinalSunDlg::Instance->MyViewFrame.Minimap.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}

void CIsoViewExt::Zoom(double offset)
{
    if (CMapData::Instance->MapWidthPlusHeight)
    {
        auto pThis = CIsoView::GetInstance();
        double scaledOld = CIsoViewExt::ScaledFactor;
        CRect oldRect = GetScaledWindowRect();
        if (offset == 0.0)
        {
            CIsoViewExt::ScaledFactor = 1.0;
        }
        else
        {
            CIsoViewExt::ScaledFactor += offset;
            CIsoViewExt::ScaledFactor = std::min(CIsoViewExt::ScaledMax, CIsoViewExt::ScaledFactor);
            CIsoViewExt::ScaledFactor = std::max(CIsoViewExt::ScaledMin, CIsoViewExt::ScaledFactor);
        }
        if (abs(CIsoViewExt::ScaledFactor - 1.0) <= 0.06)
            CIsoViewExt::ScaledFactor = 1.0;
        if (scaledOld != CIsoViewExt::ScaledFactor)
        {
            CRect newRect = GetScaledWindowRect();
            CRect oriRect;
            pThis->GetWindowRect(&oriRect);
            double mousePosX;
            double mousePosY;
            mousePosX = static_cast<double>(pThis->MouseCurrentPosition.x) / oriRect.Width();
            mousePosY = static_cast<double>(pThis->MouseCurrentPosition.y) / oriRect.Height();

            pThis->ViewPosition.x += (oldRect.Width() - newRect.Width()) * mousePosX;
            pThis->ViewPosition.y += (oldRect.Height() - newRect.Height()) * mousePosY;
            pThis->MoveTo(pThis->ViewPosition.x, pThis->ViewPosition.y);
            pThis->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            CFinalSunDlg::Instance->MyViewFrame.Minimap.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        }
    }
}

void CIsoViewExt::DrawMultiMapCoordBorders(HDC hDC, const std::vector<MapCoord>& coords, COLORREF color)
{
    auto pThis = (CIsoViewExt*)CIsoView::GetInstance();
    for (const auto& mc : coords)
    {
        if (!CMapDataExt::IsCoordInFullMap(mc.X, mc.Y))
            continue;

        int x = mc.X;
        int y = mc.Y;
        CIsoViewExt::MapCoord2ScreenCoord(x, y);
        int drawX = x - CIsoViewExt::drawOffsetX;
        int drawY = y - CIsoViewExt::drawOffsetY;

        bool s1 = true;
        bool s2 = true;
        bool s3 = true;
        bool s4 = true;

        for (auto& coord : coords)
        {
            if (!CMapDataExt::IsCoordInFullMap(coord.X, coord.Y))
                continue;

            if (coord.X == mc.X - 1 && coord.Y == mc.Y)
            {
                s1 = false;
            }
            if (coord.X == mc.X + 1 && coord.Y == mc.Y)
            {
                s3 = false;
            }
            if (coord.X == mc.X && coord.Y == mc.Y - 1)
            {
                s4 = false;
            }

            if (coord.X == mc.X && coord.Y == mc.Y + 1)
            {
                s2 = false;
            }
        }
        pThis->DrawLockedCellOutlinePaint(drawX, drawY, 1, 1, color, false, hDC, pThis->m_hWnd, s1, s2, s3, s4);
    }
}

void CIsoViewExt::DrawMultiMapCoordBorders(LPDDSURFACEDESC2 lpDesc, const std::vector<MapCoord>& coords, COLORREF color)
{
    auto pThis = (CIsoViewExt*)CIsoView::GetInstance();
    for (const auto& mc : coords)
    {
        if (!CMapDataExt::IsCoordInFullMap(mc.X, mc.Y))
            continue;

        int x = mc.X;
        int y = mc.Y;
        CIsoView::MapCoord2ScreenCoord(x, y);
        int drawX = x - CIsoViewExt::drawOffsetX;
        int drawY = y - CIsoViewExt::drawOffsetY;

        bool s1 = true;
        bool s2 = true;
        bool s3 = true;
        bool s4 = true;

        for (auto& coord : coords)
        {
            if (!CMapDataExt::IsCoordInFullMap(coord.X, coord.Y))
                continue;

            if (coord.X == mc.X - 1 && coord.Y == mc.Y)
            {
                s1 = false;
            }
            if (coord.X == mc.X + 1 && coord.Y == mc.Y)
            {
                s3 = false;
            }
            if (coord.X == mc.X && coord.Y == mc.Y - 1)
            {
                s4 = false;
            }

            if (coord.X == mc.X && coord.Y == mc.Y + 1)
            {
                s2 = false;
            }
        }
        pThis->DrawLockedCellOutline(drawX, drawY, 1, 1, color, false, false, lpDesc, s1, s2, s3, s4);
    }
}

void CIsoViewExt::DrawMultiMapCoordBorders(LPDDSURFACEDESC2 lpDesc, const std::set<MapCoord>& coords, COLORREF color)
{
    auto pThis = (CIsoViewExt*)CIsoView::GetInstance();
    for (const auto& mc : coords)
    {
        if (!CMapDataExt::IsCoordInFullMap(mc.X, mc.Y))
            continue;

        int x = mc.X;
        int y = mc.Y;
        CIsoView::MapCoord2ScreenCoord(x, y);
        int drawX = x - CIsoViewExt::drawOffsetX;
        int drawY = y - CIsoViewExt::drawOffsetY;

        bool s1 = true;
        bool s2 = true;
        bool s3 = true;
        bool s4 = true;

        for (auto& coord : coords)
        {
            if (!CMapDataExt::IsCoordInFullMap(coord.X, coord.Y))
                continue;

            if (coord.X == mc.X - 1 && coord.Y == mc.Y)
            {
                s1 = false;
            }
            if (coord.X == mc.X + 1 && coord.Y == mc.Y)
            {
                s3 = false;
            }
            if (coord.X == mc.X && coord.Y == mc.Y - 1)
            {
                s4 = false;
            }

            if (coord.X == mc.X && coord.Y == mc.Y + 1)
            {
                s2 = false;
            }
        }
        pThis->DrawLockedCellOutline(drawX, drawY, 1, 1, color, false, false, lpDesc, s1, s2, s3, s4);
    }
}

void CIsoViewExt::DrawLineHDC(HDC hDC, int x1, int y1, int x2, int y2, int color, int size)
{
    auto pThis = (CIsoViewExt*)CIsoView::GetInstance();
    x1 += 36 / CIsoViewExt::ScaledFactor - 6;
    x2 += 36 / CIsoViewExt::ScaledFactor - 6;
    y1 -= 12.5 / CIsoViewExt::ScaledFactor + 2.5;
    y2 -= 12.5 / CIsoViewExt::ScaledFactor + 2.5;
    PAINTSTRUCT ps;
    HPEN hPen;
    HPEN hPenOld;
    BeginPaint(pThis->m_hWnd, &ps);
    hPen = CreatePen(PS_SOLID, CIsoViewExt::ScaledFactor < 0.61 ? (2 + size) : size, color);
    hPenOld = (HPEN)SelectObject(hDC, hPen);
    MoveToEx(hDC, x1 - CIsoViewExt::drawOffsetX, y1 - CIsoViewExt::drawOffsetY, NULL);
    LineTo(hDC, x2 - CIsoViewExt::drawOffsetX, y2 - CIsoViewExt::drawOffsetY);
    SelectObject(hDC, hPenOld);
    DeleteObject(hPen);
    EndPaint(pThis->m_hWnd, &ps);
}

std::vector<MapCoord> CIsoViewExt::GetLinePoints(MapCoord mc1, MapCoord mc2)
{
    std::vector<MapCoord> points;

    int dx = std::abs(mc2.X - mc1.X);
    int dy = -std::abs(mc2.Y - mc1.Y);
    int sx = (mc1.X < mc2.X) ? 1 : -1;
    int sy = (mc1.Y < mc2.Y) ? 1 : -1;
    int err = dx + dy; // error value

    while (true) {
        points.emplace_back(mc1.X, mc1.Y);

        if (mc1.X == mc2.X && mc1.Y == mc2.Y)
            break;

        int e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            mc1.X += sx;
        }
        if (e2 <= dx) {
            err += dx;
            mc1.Y += sy;
        }
    }
    return points;
}

std::vector<MapCoord> CIsoViewExt::GetLineRectangles(MapCoord start, MapCoord end, int width, int height)
{
    auto isOverlap = [](const MapCoord& a, const MapCoord& b, int w, int h) {
        return !(a.X + w <= b.X || b.X + w <= a.X ||
            a.Y + h <= b.Y || b.Y + h <= a.Y);
        };

    std::vector<MapCoord> placedRects;

    int x1 = start.X;
    int y1 = start.Y;
    int x2 = end.X;
    int y2 = end.Y;

    int dx = std::abs(x2 - x1);
    int dy = -std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx + dy;

    while (true) {
        MapCoord current{ x1, y1 };

        bool overlap = false;
        for (const auto& rect : placedRects) {
            if (isOverlap(current, rect, width, height)) {
                overlap = true;
                break;
            }
        }

        if (!overlap) {
            placedRects.push_back(current);
        }

        if (x1 == x2 && y1 == y2)
            break;

        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }

    return placedRects;
}

BOOL CIsoViewExt::PreTranslateMessageExt(MSG* pMsg)
{
    return CIsoView::PreTranslateMessage(pMsg);
}

BOOL CIsoViewExt::OnMouseWheelExt(UINT Flags, short zDelta, CPoint point)
{
    return TRUE;
}
