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
#include "../CIsoView/DirectXCore.h"
#include "../../Helpers/Translations.h"
#include "../../Miscs/MultiSelection.h"
#include "../../Miscs/Palettes.h"
#include "../CLoading/Body.h"
#include <immintrin.h>
#include <mutex>
#include "../../Miscs/TheaterInfo.h"
#include <stack>
#include "../../ExtraWindow/CNewScript/CNewScript.h"

Bitmap* CIsoViewExt::pFullBitmap = nullptr;
bool CIsoViewExt::SkipMapScreenConvert = false;
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
bool CIsoViewExt::DrawAnnotations = true;
bool CIsoViewExt::DrawFires = true;
bool CIsoViewExt::RockCells = false;
bool CIsoViewExt::DrawPropertyBrushMark = false;

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
bool CIsoViewExt::RenderingMap = false;
bool CIsoViewExt::RenderFullMap = false;
bool CIsoViewExt::RenderCurrentLayers = false;
bool CIsoViewExt::RenderTileSuccess = false;
bool CIsoViewExt::RenderInvisibleInGame = false;
bool CIsoViewExt::RenderEmphasizeOres = false;
bool CIsoViewExt::RenderMarkStartings = false;
bool CIsoViewExt::RenderIgnoreObjects = false;
bool CIsoViewExt::RenderSaveAsPNG = false;
bool CIsoViewExt::EnableAutoTrack = false;
RendererLighting CIsoViewExt::RenderLighing = RendererLighting::Current;

bool CIsoViewExt::AutoPropertyBrush[4] = { false };
bool CIsoViewExt::IsPressingALT = false;
bool CIsoViewExt::IsPressingTube = false;
bool CIsoViewExt::EnableLiveDistanceRuler = false;
bool CIsoViewExt::EnableOtherMeasurementTools = false;
std::vector<TwoPointStruct> CIsoViewExt::TwoPointDistance{};
MapCoord CIsoViewExt::AxialSymmetryLine[2]{};
MapCoord CIsoViewExt::CentralSymmetryCenter{};
std::vector<std::pair<MapCoord, MapCoord>> CIsoViewExt::AxialSymmetricPoints;
std::vector<std::pair<MapCoord, MapCoord>> CIsoViewExt::CentralSymmetricPoints;
std::vector<std::pair<MapCoord, float>> CIsoViewExt::Circles;
float CIsoViewExt::CircleRadius;
bool CIsoViewExt::DrawScriptPath = false;
bool CIsoViewExt::ReInitializingDDraw = false;
bool CIsoViewExt::CliffBackAlt = false;
bool CIsoViewExt::OnLButtonDown_CalledFromOnMouseMove = false;
bool CIsoViewExt::OnMouseMove_CalledFromOnLButtonDown = false;
bool CIsoViewExt::HistoryRecord_IsHoldingLButton = false;
std::vector<MapCoord> CIsoViewExt::TubeNodes;
std::vector<MapCoord> CIsoViewExt::LiveDistanceRuler;
std::vector<MapCoord> CIsoViewExt::ScriptPath;
FString CIsoViewExt::CurrentCellObjectHouse = "";
int CIsoViewExt::EXTRA_BORDER_BOTTOM = 25;
Cell3DLocation CIsoViewExt::CurrentDrawCellLocation;
std::unordered_map<TextCacheKey, TextCacheEntry, TextCacheHasher> CIsoViewExt::textCache;

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

constexpr float cellLength = 42.426407f;
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

void CIsoViewExt::InitGdiplus()
{
    static bool initialized = false;
    if (!initialized) {
        GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken;
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        initialized = true;
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
    const auto nBytesPerPixel = 4;

    unsigned char* base = (unsigned char*)lpDesc->lpSurface;
    unsigned char* end = base + lPitch * lpDesc->dwHeight;
    auto SafeWritePixel = [&](unsigned char* p, BGRStruct& ddColor)
    {
        if (p < base || p + nBytesPerPixel > end)
            return;
        memcpy(p, &ddColor, nBytesPerPixel);
    };

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

    auto DrawLine = [&SafeWritePixel, lPitch, nBytesPerPixel, lpDesc, &rect](int X1, int Y1, int X2, int Y2, BGRStruct& ddColor)
        {
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
                    SafeWritePixel(ptr, ddColor);
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
                    SafeWritePixel(ptr, ddColor);
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
                        SafeWritePixel(ptr + i * nBytesPerPixel, ddColor);
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
                        SafeWritePixel(ptr + k * nBytesPerPixel, ddColor); 
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

    auto ClipAndDrawLine = [&rect, DrawLine](int X1, int Y1, int X2, int Y2, BGRStruct& ddColor)
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
                DrawLine(X1, Y1, X2, Y2, ddColor);
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
            ClipAndDrawLine(x1, y1 + inneroffset, x1, y3 - inneroffset, ddColor);
            ClipAndDrawLine(x1 - 1, y1 + inneroffset, x1 - 1, y3 - inneroffset, ddColor);
            ClipAndDrawLine(x4 + 2 * inneroffset, y4, x2 - 2 * inneroffset, y4, ddColor);

            ClipAndDrawLine(x1, y1 + inneroffset, x2T - 2 * inneroffset, y2T, ddColor);
            ClipAndDrawLine(x2 - 2 * inneroffset, y2, x3, y3 - inneroffset, ddColor);
            ClipAndDrawLine(x3L, y3L - inneroffset, x4B + 2 * inneroffset, y4B, ddColor);
            ClipAndDrawLine(x4 + 2 * inneroffset, y4, x1L, y1L + inneroffset, ddColor);
        };
    

    auto drawCellOutline2 = [&](int inneroffset)
        {
            ClipAndDrawLine(x1 + 1, y1 + 1 + inneroffset, x1 + 1, y3 - inneroffset + 1, ddColor2);
            ClipAndDrawLine(x1 + 1 - 1, y1 + 1 + inneroffset, x1 + 1 - 1, y3 - inneroffset + 1, ddColor2);
            ClipAndDrawLine(x4 + 1 + 2 * inneroffset, y4 + 1, x2 + 1 - 2 * inneroffset, y4 + 1, ddColor2);

            ClipAndDrawLine(x1 + 1, y1 + 1 + inneroffset, x2T + 1 - 2 * inneroffset, y2T + 1, ddColor2);
            ClipAndDrawLine(x2 + 1 - 2 * inneroffset, y2 + 1, x3 + 1, y3 - inneroffset + 1, ddColor2);
            ClipAndDrawLine(x3L + 1, y3L + 1 - inneroffset, x4B + 1 + 2 * inneroffset, y4B + 1, ddColor2);
            ClipAndDrawLine(x4 + 1 + 2 * inneroffset, y4 + 1, x1L + 1, y1L + inneroffset + 1, ddColor2);
        };

    if (onlyX)
    {
        ClipAndDrawLine(x1, y1, x1, y3, ddColor);
        ClipAndDrawLine(x4, y4, x2 , y4, ddColor);
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
    const auto nBytesPerPixel = 4;

    unsigned char* base = (unsigned char*)lpDesc->lpSurface;
    unsigned char* end = base + lPitch * lpDesc->dwHeight;
    auto SafeWritePixel = [&](unsigned char* p, BGRStruct& ddColor)
    {
        if (p < base || p + nBytesPerPixel > end)
            return;
        memcpy(p, &ddColor, nBytesPerPixel);
    };

    auto pRGB = (ColorStruct*)&color;
    BGRStruct ddColor;
    ddColor.R = pRGB->red;
    ddColor.G = pRGB->green;
    ddColor.B = pRGB->blue;

    auto DrawLine = [&SafeWritePixel, lPitch, nBytesPerPixel, &ddColor, lpDesc, &rect](int X1, int Y1, int X2, int Y2)
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
                SafeWritePixel(ptr, ddColor);
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
                SafeWritePixel(ptr, ddColor);
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
                    SafeWritePixel(ptr + i * nBytesPerPixel, ddColor);
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
                    SafeWritePixel(ptr + k * nBytesPerPixel, ddColor);
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
    AdaptRectForSecondScreen(&rect);

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
        HPEN hPen;
        HPEN hPenOld;
        hPen = CreatePen(PS_SOLID, 0, heightColor);
        hPenOld = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, X1, Y1, NULL);
        LineTo(hdc, X2, Y2);
        SelectObject(hdc, hPenOld);
        DeleteObject(hPen);
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

void CIsoViewExt::DrawEllipsePaint(int X, int Y, int majorRadius, COLORREF color, HDC hdc, const RECT& rect, int width)
{
    if (!hdc)
        return;

    X += 30 / CIsoViewExt::ScaledFactor;
    Y -= 15 / CIsoViewExt::ScaledFactor;
    X += 6 / CIsoViewExt::ScaledFactor - 6 + 2;
    Y += 3 / CIsoViewExt::ScaledFactor - 3;

    majorRadius /= CIsoViewExt::ScaledFactor;

    int cx = X - CIsoViewExt::drawOffsetX;
    int cy = Y - CIsoViewExt::drawOffsetY;

    int left = cx - majorRadius;
    int right = cx + majorRadius;
    int top = cy - majorRadius / 2;
    int bottom = cy + majorRadius / 2;

    RECT ellipseRect = { left, top, right, bottom };
    RECT tmp;
    if (!IntersectRect(&tmp, &ellipseRect, &rect))
    {
        return;
    }

    HPEN hPen = CreatePen(PS_SOLID, width, color);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    int savedDC = SaveDC(hdc);

    HRGN rgn = CreateRectRgnIndirect(&rect);
    SelectClipRgn(hdc, rgn);

    Ellipse(hdc, left, top, right, bottom);

    RestoreDC(hdc, savedDC);

    DeleteObject(rgn);
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);
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
    AdaptRectForSecondScreen(&rect);

    auto DrawLine = [hwnd, color, hdc, &rect](int X1, int Y1, int X2, int Y2)
    {
        HPEN hPen;
        HPEN hPenOld;
        hPen = CreatePen(PS_SOLID, 0, color);
        hPenOld = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, X1, Y1, NULL);
        LineTo(hdc, X2, Y2);
        SelectObject(hdc, hPenOld);
        DeleteObject(hPen);
        
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

void CIsoViewExt::DrawLine(
    int x1, int y1, int x2, int y2,
    COLORREF color,
    bool bUseDot,
    bool bUsePrimary,
    LPDDSURFACEDESC2 lpDesc,
    const RECT& rect,
    bool bDashed,
    int nThickness)
{
    if (!lpDesc || !lpDesc->lpSurface)
        return;

    const int surfW = (int)lpDesc->dwWidth;
    const int surfH = (int)lpDesc->dwHeight;
    const int pitch = lpDesc->lPitch;
    const int bpp = 4;

    ColorStruct* pRGB = (ColorStruct*)&color;
    BGRStruct ddColor{ pRGB->blue, pRGB->green, pRGB->red };

    unsigned char* base = (unsigned char*)lpDesc->lpSurface;
    unsigned char* end = base + pitch * surfH;

    auto SafeWritePixel = [&](unsigned char* p)
    {
        if (p < base || p + bpp > end)
            return;
        memcpy(p, &ddColor, bpp);
    };

    auto DrawThinLine = [&](int X1, int Y1, int X2, int Y2)
    {
        int dx = abs(X2 - X1);
        int dy = abs(Y2 - Y1);

        int sx = (X1 < X2) ? 1 : -1;
        int sy = (Y1 < Y2) ? 1 : -1;

        int err = dx - dy;

        const int dashOn = 3, dashOff = 3;
        const int dashPeriod = dashOn + dashOff;
        int stepCount = 0;

        while (true)
        {
            if (!bDashed || (stepCount % dashPeriod) < dashOn)
            {
                if ((unsigned)X1 < (unsigned)surfW &&
                    (unsigned)Y1 < (unsigned)surfH)
                {
                    unsigned char* p =
                        base + Y1 * pitch + X1 * bpp;
                    SafeWritePixel(p);
                }
            }

            if (X1 == X2 && Y1 == Y2)
                break;

            int e2 = err * 2;
            if (e2 > -dy)
            {
                err -= dy;
                X1 += sx;
            }
            if (e2 < dx)
            {
                err += dx;
                Y1 += sy;
            }
            ++stepCount;
        }
    };

    auto ClipAndDrawLine = [&](int& X1, int& Y1, int& X2, int& Y2) -> bool
    {
        auto encode = [&](int x, int y)
        {
            int c = 0;
            if (x < rect.left)   c |= 1;
            if (x > rect.right)  c |= 2;
            if (y < rect.top)    c |= 8;
            if (y > rect.bottom) c |= 4;
            return c;
        };

        int c1 = encode(X1, Y1);
        int c2 = encode(X2, Y2);

        while (c1 || c2)
        {
            if (c1 & c2)
                return false;

            int c = c1 ? c1 : c2;
            int x = 0, y = 0;

            if (c & 1)
            {
                x = rect.left;
                y = Y1 + (Y2 - Y1) * (rect.left - X1) / (X2 - X1);
            }
            else if (c & 2)
            {
                x = rect.right;
                y = Y1 + (Y2 - Y1) * (rect.right - X1) / (X2 - X1);
            }
            else if (c & 8)
            {
                y = rect.top;
                x = X1 + (X2 - X1) * (rect.top - Y1) / (Y2 - Y1);
            }
            else if (c & 4)
            {
                y = rect.bottom;
                x = X1 + (X2 - X1) * (rect.bottom - Y1) / (Y2 - Y1);
            }

            if (c == c1)
            {
                X1 = x; Y1 = y;
                c1 = encode(X1, Y1);
            }
            else
            {
                X2 = x; Y2 = y;
                c2 = encode(X2, Y2);
            }
        }

        DrawThinLine(X1, Y1, X2, Y2);
        return true;
    };

    if (nThickness <= 1)
    {
        int tx1 = x1, ty1 = y1;
        int tx2 = x2, ty2 = y2;
        ClipAndDrawLine(tx1, ty1, tx2, ty2);
        return;
    }

    int dx = x2 - x1;
    int dy = y2 - y1;
    float len = sqrtf((float)(dx * dx + dy * dy));
    if (len < 1e-6f)
        return;

    float nx = -dy / len;
    float ny = dx / len;
    int half = (nThickness - 1) / 2;

    for (int i = -half; i <= half; ++i)
    {
        float offset = (float)i + 0.5f;
        int ox = (int)(nx * offset);
        int oy = (int)(ny * offset);

        int tx1 = x1 + ox;
        int ty1 = y1 + oy;
        int tx2 = x2 + ox;
        int ty2 = y2 + oy;

        ClipAndDrawLine(tx1, ty1, tx2, ty2);
    }
}

void CIsoViewExt::DrawLockedLines(const std::vector<std::pair<MapCoord, MapCoord>>& lines, int X, int Y, COLORREF color, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc)
{
    if (lpDesc->lpSurface == nullptr)
        return;

    RECT rect = CIsoViewExt::GetScaledWindowRect();

    auto lPitch = lpDesc->lPitch;
    const auto nBytesPerPixel = 4;

    unsigned char* base = (unsigned char*)lpDesc->lpSurface;
    unsigned char* end = base + lPitch * lpDesc->dwHeight;
    auto SafeWritePixel = [&](unsigned char* p, BGRStruct& ddColor)
    {
        if (p < base || p + nBytesPerPixel > end)
            return;
        memcpy(p, &ddColor, nBytesPerPixel);
    };

    auto pRGB = (ColorStruct*)&color;
    BGRStruct ddColor;
    ddColor.R = pRGB->red;
    ddColor.G = pRGB->green;
    ddColor.B = pRGB->blue;

    auto DrawLine = [&SafeWritePixel, lPitch, nBytesPerPixel, &ddColor, lpDesc, &rect](int X1, int Y1, int X2, int Y2)
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
                SafeWritePixel(ptr, ddColor);
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
                SafeWritePixel(ptr, ddColor);
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
                    SafeWritePixel(ptr + i * nBytesPerPixel, ddColor);
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
                    SafeWritePixel(ptr + k * nBytesPerPixel, ddColor);
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
        //AdaptRectForSecondScreen(&rect);
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
    this->BlitTransparentDesc(CLoadingExt::GetSurfaceImageDataFromMap(filename + ".bmp")->lpSurface, GetBackBuffer(), lpDesc, X, Y, -1, -1);
}

void CIsoViewExt::DrawCelltag(int X, int Y, LPDDSURFACEDESC2 lpDesc)
{
    auto image = CLoadingExt::GetSurfaceImageDataFromMap("CELLTAG");
    this->BlitTransparentDesc(image->lpSurface, GetBackBuffer(), lpDesc, X + 25 - image->FullWidth / 2, Y + 12 - image->FullHeight / 2, -1, -1);
}

void CIsoViewExt::DrawWaypointFlag(int X, int Y, LPDDSURFACEDESC2 lpDesc)
{
    auto image = CLoadingExt::GetSurfaceImageDataFromMap("FLAG");
    this->BlitTransparentDesc(image->lpSurface, GetBackBuffer(), lpDesc, X + 5 + 25 - image->FullWidth / 2, Y + 12 - image->FullHeight / 2, -1, -1);
}
void CIsoViewExt::GetSameConnectedCells(int X, int Y, int oriX, int oriY, std::set<MapCoord>* selectedCoords)
{
    if (!selectedCoords) return;

    auto& map = CMapData::Instance;
    if (!map->IsCoordInMap(X, Y)) return;

    std::stack<MapCoord> stk;
    stk.push({ X, Y });

    while (!stk.empty())
    {
        auto cur = stk.top(); stk.pop();
        int x = cur.X;
        int y = cur.Y;

        auto cell = map->TryGetCellAt(x, y);
        if (cell->Flag.NotAValidCell) continue;
        cell->Flag.NotAValidCell = TRUE;

        if (cell->IsHidden()) continue;

        if (MultiSelection::SelectedCoords.size() > 0 && MultiSelection::IsSelected(oriX, oriY)) {
            bool skip = true;
            for (const auto& coord : MultiSelection::SelectedCoords) {
                if (coord.X == x && coord.Y == y) {
                    skip = false;
                    break;
                }
            }
            if (skip) continue;
        }

        selectedCoords->insert({ x, y });

        int tileIndex_cell = CMapDataExt::GetSafeTileIndex(cell->TileIndex);

        static const int dx[4] = { -1, 1, 0, 0 };
        static const int dy[4] = { 0, 0, -1, 1 };

        for (int k = 0; k < 4; k++)
        {
            int nx = x + dx[k];
            int ny = y + dy[k];

            if (!map->IsCoordInMap(nx, ny))
                continue;

            auto cell2 = map->TryGetCellAt(nx, ny);
            if (cell2->Flag.NotAValidCell)
                continue;

            bool match = false;

            int tileIndex_cell2 = CMapDataExt::GetSafeTileIndex(cell2->TileIndex);
            match = tileIndex_cell2 == tileIndex_cell && cell2->TileSubIndex == cell->TileSubIndex;

            if (ExtConfigs::FillArea_ConsiderLAT && !match) {
                for (const auto& latInfo : CMapDataExt::Tile_to_lat)
                {
                    int iSmoothSet = latInfo.SmoothSet;
                    int iLatSet = latInfo.LatSet;

                    if (iLatSet >= 0 && iSmoothSet >= 0 &&
                        iSmoothSet < CMapDataExt::TileSet_starts.size() &&
                        iLatSet < CMapDataExt::TileSet_starts.size() &&
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

            if (ExtConfigs::FillArea_ConsiderWater && !match) {
                if (CMapDataExt::TileData[tileIndex_cell2].TileSet == CMapDataExt::WaterSet &&
                    CMapDataExt::TileData[tileIndex_cell].TileSet == CMapDataExt::WaterSet)
                {
                    match = true;
                }
            }

            if (match) {
                stk.push({ nx, ny });
            }
        }
    }
}

void CIsoViewExt::FillArea(int X, int Y, int ID, int Subtile, int oriX, int oriY)
{
    std::unique_ptr<std::set<MapCoord>> recordCoords = std::make_unique<std::set<MapCoord>>();
    auto& map = CMapData::Instance;
    for (int i = 0; i < map->CellDataCount; ++i)
    {
        auto& cell = map->CellDatas[i];
        cell.Flag.NotAValidCell = FALSE;
    }
    GetSameConnectedCells(X, Y, oriX, oriY, recordCoords.get());
   
    if (ID >= 0 && ID < CMapDataExt::TileDataCount)
    {
        std::map<int, std::vector<TilePlacement>> placements;
        auto& tile = CMapDataExt::TileData[ID];
        int tileOriginX = oriY - (tile.Width - 1);
        int tileOriginY = oriX - (tile.Height - 1);
        for (const auto& coord : *recordCoords)
        {
            int localY = ((coord.Y - tileOriginX) % tile.Width + tile.Width) % tile.Width;
            int localX = ((coord.X - tileOriginY) % tile.Height + tile.Height) % tile.Height;

            int groupY = coord.Y - localY;
            int groupX = coord.X - localX;

            int subtileIndex = localY + localX * tile.Width;

            placements[groupY * 1000 + groupX].push_back(TilePlacement{
                (short)coord.X,
                (short)coord.Y,
                (short)subtileIndex
                });
        }

        for (auto& [_, groups] : placements)
        {
            int index = CIsoView::CurrentCommand->Param == 1 ? GetRandomTileIndex() : 0;
            for (auto& coord : groups)
            {
                if (tile.TileBlockDatas[coord.SubtileIndex].ImageData != NULL)
                {
                    bool isBridge = (tile.TileSet == CMapDataExt::BridgeSet || tile.TileSet == CMapDataExt::WoodBridgeSet);
                    auto cell = CMapData::Instance->GetCellAt(coord.X, coord.Y);
                    if (CIsoView::CurrentCommand->Param == 1)
                    {                      
                        auto& tile = CMapDataExt::TileData[index];
                        if (tile.TileBlockDatas[coord.SubtileIndex].ImageData != NULL)
                        {
                            cell->TileIndex = index;
                            cell->TileSubIndex = coord.SubtileIndex;
                            cell->Flag.AltIndex = isBridge ? 0 : STDHelpers::RandomSelectInt(0, tile.AltTypeCount + 1);
                            CMapDataExt::GetExtension()->SetHeightAt(coord.X, coord.Y, cell->Height + tile.TileBlockDatas[coord.SubtileIndex].Height);
                        }
                    }
                    else
                    {
                        cell->TileIndex = ID;
                        cell->TileSubIndex = coord.SubtileIndex;
                        cell->Flag.AltIndex = isBridge ? 0 : STDHelpers::RandomSelectInt(0, tile.AltTypeCount + 1);
                        CMapDataExt::GetExtension()->SetHeightAt(coord.X, coord.Y, cell->Height + tile.TileBlockDatas[coord.SubtileIndex].Height);
                    }
                    CMapData::Instance->UpdateMapPreviewAt(coord.X, coord.Y);
                }
            }
        }

        if (!CFinalSunApp::Instance->DisableAutoLat)
        {
            std::set<MapCoord> editedLatCoords;
            for (const auto& p : *recordCoords) {
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
    recordCoords->clear();
}

void CIsoViewExt::BlitText(const std::wstring& text, COLORREF textColor, COLORREF bgColor,
    CIsoView* pThis, void* dst, const RECT& window, const DDBoundary& boundary,
    int x, int y, int fontSize, BYTE alpha, bool bold)
{
    const int bpp = 4;

    TextCacheKey key{ text, textColor, bgColor, fontSize, bold };
    auto it = textCache.find(key);

    if (it == textCache.end()) {
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

        std::vector<BGRStruct> src(finalWidth * finalHeight);

        BGRStruct bgbgr;
        bgbgr.B = (bgColor >> 16) & 0xFF;
        bgbgr.G = (bgColor >> 8) & 0xFF;
        bgbgr.R = bgColor & 0xFF;

        BGRStruct textbgr;
        textbgr.B = (textColor >> 16) & 0xFF;
        textbgr.G = (textColor >> 8) & 0xFF;
        textbgr.R = textColor & 0xFF;

        // bgColor border
        for (int yy = 0; yy < finalHeight; yy++) {
            for (int xx = 0; xx < finalWidth; xx++) {
                if (xx <= 2 || xx >= finalWidth - 3 || yy <= 2 || yy >= finalHeight - 3) {
                    src[yy * finalWidth + xx] = bgbgr;
                }
            }
        }

        // textColor border
        for (int yy = 0; yy < finalHeight; yy++) {
            for (int xx = 0; xx < finalWidth; xx++) {
                if (xx == 0 || xx == finalWidth || yy == 0 || yy == finalHeight - 1) {
                    src[yy * finalWidth + xx] = textbgr;
                }
            }
        }

        for (int yy = 0; yy < sheight; yy++) {
            for (int xx = 0; xx < swidth; xx++) {
                int srcIndex = (yy + borderWidth) * finalWidth + (xx + borderWidth);
                int tempIndex = (yy * swidth + xx) * bpp;
                src[srcIndex] = BGRStruct(tempPixels[tempIndex + 0], // B
                    tempPixels[tempIndex + 1], // G
                    tempPixels[tempIndex + 2]);// R
            }
        }

        delete[] tempPixels;

        SelectObject(hdcMem, oldFont);
        DeleteObject(hFont);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);

        TextCacheEntry entry;
        entry.width = finalWidth;
        entry.height = finalHeight;
        entry.pixels = std::move(src);

        textCache[key] = std::move(entry);
        it = textCache.find(key);
    }

    auto& entry = it->second;
    auto& src = entry.pixels;
    int swidth = entry.width;
    int sheight = entry.height;

    if (dst == NULL) {
        return;
    }
    if (x + swidth < window.left || y + sheight < window.top) {
        return;
    }
    if (x >= window.right || y >= window.bottom) {
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
    }
    blrect.top = y;
    if (blrect.top < 0) {
        srcRect.top = 1 - blrect.top;
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

bool CIsoViewExt::SaveImageDataToBMP(ImageDataClassSafe* pd,const char* outputPath)
{
    if (!pd || !pd->pImageBuffer || pd->Flag == ImageDataFlag::SurfaceData)
        return false;

    const int BPP = 4;

    int width = pd->FullWidth;
    int height = pd->FullHeight;

    if (width <= 0 || height <= 0)
        return false;

    int pitch = ((width * BPP + 3) & ~3);
    size_t bufferSize = static_cast<size_t>(pitch) * height;

    std::unique_ptr<BYTE[]> pixels(new BYTE[bufferSize]());
    if (!pixels)
        return false;

    Palette* newPal = pd->pPalette;
    if (!newPal)
        return false;

    RGBClass oreColor{};
    bool isEmphasizingOre = false;
    BYTE oreOpacity = 0;

    BYTE* src = static_cast<BYTE*>(pd->pImageBuffer.get());
    int sw = pd->FullWidth;

    for (size_t i = 0; i < bufferSize; i += 4) {
        pixels[i + 0] = 255;
        pixels[i + 1] = 0;
        pixels[i + 2] = 255;
        pixels[i + 3] = 255;
    }

    for (int row = 0; row < height; ++row)
    {
        LONG left = pd->pPixelValidRanges[row].First;
        LONG right = pd->pPixelValidRanges[row].Last;

        if (left > right)
            continue;

        left = std::max(left, 0L);
        right = std::min(right, static_cast<LONG>(width) - 1);

        BYTE* srcPtr = src + row * sw + left;

        BYTE* destRow = pixels.get() + row * pitch + left * BPP;

        for (LONG col = left; col <= right; ++col, ++srcPtr, destRow += BPP)
        {
            BYTE idx = *srcPtr;
            if (idx == 0) continue;

            BGRStruct c = newPal->Data[idx];

            destRow[0] = c.B;
            destRow[1] = c.G;
            destRow[2] = c.R;
            destRow[3] = 255;
        }
    }

    BITMAPFILEHEADER bfh = {};
    BITMAPINFOHEADER bih = {};

    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = -height; 
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = static_cast<DWORD>(bufferSize);
    bih.biXPelsPerMeter = 2835;
    bih.biYPelsPerMeter = 2835;

    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + bih.biSizeImage;

    FILE* fp = fopen(outputPath, "wb");
    if (!fp)
        return false;

    fwrite(&bfh, 1, sizeof(bfh), fp);
    fwrite(&bih, 1, sizeof(bih), fp);
    fwrite(pixels.get(), 1, bufferSize, fp);
    fclose(fp);

    return true;
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

bool CIsoViewExt::LoadAndScaleToBitmap(const ImageDataView* pData,
    CBitmap& outBitmap,
    int maxSize,
    COLORREF bgColor,
    bool trim,
    bool removeHalo)
{
    if (!pData || maxSize <= 0)
        return false;

    int srcW = pData->FullWidth;
    int srcH = pData->FullHeight;

    if (srcW <= 0 || srcH <= 0)
        return false;

    int left = srcW, right = 0, top = srcH, bottom = 0;

    auto isBG = [&](BYTE idx) { return idx == 0; };

    if (trim)
    {
        for (int y = 0; y < srcH; ++y)
        {
            for (int x = 0; x < srcW; ++x)
            {
                BYTE idx = pData->pImageBuffer[y * srcW + x];
                if (!isBG(idx))
                {
                    left = MIN(left, x);
                    right = MAX(right, x);
                    top = MIN(top, y);
                    bottom = MAX(bottom, y);
                }
            }
        }
    }
    else
    {
        left = 0; top = 0; right = srcW - 1; bottom = srcH - 1;
    }

    if (left > right || top > bottom)
    {
        left = 0; top = 0; right = srcW - 1; bottom = srcH - 1;
    }

    int cropW = right - left + 1;
    int cropH = bottom - top + 1;

    float scale = MIN((float)maxSize / cropW, (float)maxSize / cropH);

    int newW = int(cropW * scale);
    int newH = int(cropH * scale);

    int offsetX = (maxSize - newW) / 2;
    int offsetY = (maxSize - newH) / 2;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = maxSize;
    bmi.bmiHeader.biHeight = -maxSize;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HBITMAP hBmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    if (!hBmp || !pBits)
        return false;

    DWORD* dst = (DWORD*)pBits;

    DWORD transparentRGB =
        (GetRValue(bgColor) << 16) |
        (GetGValue(bgColor) << 8) |
        GetBValue(bgColor);

    std::fill(dst, dst + maxSize * maxSize, transparentRGB);

    std::vector<int> x0(newW), x1(newW);
    std::vector<float> dx(newW);

    for (int x = 0; x < newW; ++x)
    {
        float fx = left + x / scale;
        int ix = (int)fx;

        x0[x] = std::clamp(ix, 0, srcW - 1);
        x1[x] = std::clamp(ix + 1, 0, srcW - 1);
        dx[x] = fx - ix;
    }

    for (int y = 0; y < newH; ++y)
    {
        float fy = top + y / scale;
        int iy = (int)fy;

        int y0 = std::clamp(iy, 0, srcH - 1);
        int y1 = std::clamp(iy + 1, 0, srcH - 1);
        float dy = fy - iy;

        DWORD* dstRow = dst + (offsetY + y) * maxSize + offsetX;

        for (int x = 0; x < newW; ++x)
        {
            BYTE i00 = pData->pImageBuffer[y0 * srcW + x0[x]];
            BYTE i10 = pData->pImageBuffer[y0 * srcW + x1[x]];
            BYTE i01 = pData->pImageBuffer[y1 * srcW + x0[x]];
            BYTE i11 = pData->pImageBuffer[y1 * srcW + x1[x]];

            float w00 = (1 - dx[x]) * (1 - dy);
            float w10 = dx[x] * (1 - dy);
            float w01 = (1 - dx[x]) * dy;
            float w11 = dx[x] * dy;

            float r = 0, g = 0, b = 0, total = 0;

            auto sample = [&](BYTE idx, float w)
            {
                if (idx == 0) return;

                auto& p = pData->pPalette->Data[idx];
                r += p.R * w;
                g += p.G * w;
                b += p.B * w;
                total += w;
            };

            sample(i00, w00);
            sample(i10, w10);
            sample(i01, w01);
            sample(i11, w11);

            if (total > 0.0f)
            {
                int ir = int(r / total);
                int ig = int(g / total);
                int ib = int(b / total);

                dstRow[x] = (ir << 16) | (ig << 8) | ib;
            }
        }
    }

    if (removeHalo)
    {
        for (int i = 0; i < maxSize * maxSize; ++i)
        {
            if (dst[i] == transparentRGB)
                continue;

            int r = (dst[i] >> 16) & 0xFF;
            int g = (dst[i] >> 8) & 0xFF;
            int b = dst[i] & 0xFF;

            if (abs(r - 255) < 2 && g < 2 && abs(b - 255) < 2)
                dst[i] = transparentRGB;
        }
    }

    outBitmap.DeleteObject();
    outBitmap.Attach(hBmp);

    return true;
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
    AdaptRectForSecondScreen(&rect);
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

int CIsoViewExt::GetRandomTileIndex()
{
    if (CViewObjectsExt::PlacingRandomTile >= 0)
    {
        if (CIsoView::CurrentCommand->Command != 10 && CIsoView::CurrentCommand->Param != 1)
        {
            CViewObjectsExt::PlacingRandomTile = -1;
            return 0;
        }
        std::vector<int> randomList;

        if (auto pSection = CINI::FAData().GetSection("PlaceRandomTileList"))
        {
            if (auto pSection2 = CINI::FAData().GetSection(*pSection->GetValueAt(CViewObjectsExt::PlacingRandomTile)))
            {
                for (auto& pKey : pSection2->GetEntities())
                {
                    if (pKey.first.Find("Name") < 0 && pKey.first != "AllowedTheater")
                    {
                        int tile = atoi(pKey.second);
                        if (tile >= CMapDataExt::TileDataCount)
                            return 0;
                        randomList.push_back(tile);
                    }
                }
            }
        }
        return STDHelpers::RandomSelectInt(randomList);
    }
    else // random water
    {
        return STDHelpers::RandomSelectInt(TheaterInfo::CurrentBigWaters);
    }
    return 0;
}

void CIsoViewExt::MapCoord2ScreenCoord(int& X, int& Y, int flatMode)
{
    CRect rect;
    auto pThis = CIsoView::GetInstance();
    pThis->GetWindowRect(&rect);
    AdaptRectForSecondScreen(&rect);
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

        if (ExtConfigs::AVX2_Support) [[likely]]
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
    AdaptRectForSecondScreen(&rect);
    int leftIndex = 0;
    int fontSize = ExtConfigs::DisplayTextSize;
    if (CIsoViewExt::ScaledFactor < 0.3)
        fontSize += 6;
    else if (CIsoViewExt::ScaledFactor < 0.5)
        fontSize += 4;
    else if (CIsoViewExt::ScaledFactor < 0.75)
        fontSize += 2;
    int lineHeight = fontSize + 2;
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
        if (CFinalSunApp::Instance().FlatToGround)
        {
            FString buffer;
            buffer.Format(Translations::TranslateOrDefault("FlatToGroundModeEnabled", "2D Mode Enabled"));
            ::TextOut(hDC, rect.left + 10, rect.top + 10 + lineHeight * leftIndex++, buffer, buffer.GetLength());
        }
        if (CIsoViewExt::ScaledFactor != 1.0)
        {
            FString buffer;
            buffer.Format(Translations::TranslateOrDefault("ScaledFactorText", "Zoom: %.02fx, Middle-click to reset"), 1.0 / CIsoViewExt::ScaledFactor);
            ::TextOut(hDC, rect.left + 10, rect.top + 10 + lineHeight * leftIndex++, buffer, buffer.GetLength());
        }
    }
}

void CIsoViewExt::DrawDistanceRuler(HDC hDC, const RECT& rect)
{
    int fontSize = ExtConfigs::DisplayTextSize;
    if (CIsoViewExt::ScaledFactor < 0.75)
        fontSize += 2;
    if (CIsoViewExt::ScaledFactor < 0.5)
        fontSize += 2;
    if (CIsoViewExt::ScaledFactor < 0.3)
        fontSize += 2;
    int lineHeight = fontSize + 2;
    if (!CIsoViewExt::LiveDistanceRuler.empty())
    {
        for (int i = 0; i < CIsoViewExt::LiveDistanceRuler.size(); ++i)
        {
            int x1 = CIsoViewExt::LiveDistanceRuler[i].X;
            int y1 = CIsoViewExt::LiveDistanceRuler[i].Y;
            int x2, y2 = 0;
            if (i == CIsoViewExt::LiveDistanceRuler.size() - 1)
            {
                auto pIsoView = CIsoViewExt::GetExtension();
                auto point = pIsoView->GetCurrentMapCoord(pIsoView->MouseCurrentPosition);
                x2 = point.X;
                y2 = point.Y;
                if (!CMapData::Instance->IsCoordInMap(x2, y2))
                    return;
            }
            else
            {
                x2 = CIsoViewExt::LiveDistanceRuler[i + 1].X;
                y2 = CIsoViewExt::LiveDistanceRuler[i + 1].Y;
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
                CIsoViewExt::DrawLineHDC(hDC, x1, y1, x2, y2, ExtConfigs::DistanceRuler_Color, rect, 2);
                int j = 1;
                std::ostringstream oss;
                oss.precision(2);
                oss << std::fixed << distance;
                buffer.Format(Translations::TranslateOrDefault("DistanceRuler.Distance", "Distance: %s"), oss.str().c_str());
                TextOutClipped(hDC, drawX, drawY + lineHeight * j++, buffer, buffer.GetLength(), rect);
                buffer.Format(Translations::TranslateOrDefault("DistanceRuler.Coordinate", "XY: %d, %d, ¦¤XY: %d, %d"),
                    coord2.Y, coord2.X, coord2.Y - coord1.Y, coord2.X - coord1.X);
                TextOutClipped(hDC, drawX, drawY + lineHeight * j++, buffer, buffer.GetLength(), rect);
            }
            if (i == 0)
            {
                drawX = x1 - CIsoViewExt::drawOffsetX + 30;
                drawY = y1 - CIsoViewExt::drawOffsetY - 15;
                buffer.Format(Translations::TranslateOrDefault("DistanceRuler.InitCoordinate", "XY: %d, %d"),
                    coord1.Y, coord1.X);
                TextOutClipped(hDC, drawX, drawY + lineHeight * 1, buffer, buffer.GetLength(), rect);
            }
        }
    }
}

void CIsoViewExt::DrawScriptPaths(HDC hDC, const RECT& rect)
{
    for (int i = 1; i < ScriptPath.size(); ++i)
    {
        auto& coord1 = ScriptPath[i - 1];
        auto& coord2 = ScriptPath[i];

        int x1 = coord1.X;
        int y1 = coord1.Y;
        CIsoViewExt::MapCoord2ScreenCoord(x1, y1);
        int x2 = coord2.X;
        int y2 = coord2.Y;
        CIsoViewExt::MapCoord2ScreenCoord(x2, y2);
        CIsoViewExt::DrawArrowHDC(hDC, x1, y1, x2, y2, ExtConfigs::DistanceRuler_Color, rect, 2);
    }
}

void CIsoViewExt::DrawOtherMeasurementTools(HDC hDC, const RECT& rect)
{
    int fontSize = ExtConfigs::DisplayTextSize;
    if (CIsoViewExt::ScaledFactor < 0.3)
        fontSize += 6;
    else if (CIsoViewExt::ScaledFactor < 0.5)
        fontSize += 4;
    else if (CIsoViewExt::ScaledFactor < 0.75)
        fontSize += 2;
    int lineHeight = fontSize + 2;
    auto reversedColor = RGB(
        255 - GetRValue(ExtConfigs::DistanceRuler_Color),
        255 - GetGValue(ExtConfigs::DistanceRuler_Color),
        255 - GetBValue(ExtConfigs::DistanceRuler_Color)
    );
    auto pIsoView = CIsoViewExt::GetExtension();
    for (auto& twoPoints : TwoPointDistance)
    {
        if (twoPoints.Point1 != MapCoord{ 0,0 })
        {
            int j = 0;
            int x1 = twoPoints.Point1.X;
            int y1 = twoPoints.Point1.Y;
            FString buffer;
            CIsoViewExt::MapCoord2ScreenCoord(x1, y1);
            int drawX = x1 - CIsoViewExt::drawOffsetX + 30;
            int drawY = y1 - CIsoViewExt::drawOffsetY - 15;
            if (twoPoints.drawText)
            {
                buffer.Format(Translations::TranslateOrDefault("DistanceRuler.InitCoordinate", "XY: %d, %d"),
                    twoPoints.Point1.Y, twoPoints.Point1.X);
                TextOutClipped(hDC, drawX, drawY + lineHeight * j++, buffer, buffer.GetLength(), rect);
            }

            auto coord2 = twoPoints.Point2;
            if (twoPoints.Point2 == MapCoord{ 0,0 })
            {
                auto point = pIsoView->GetCurrentMapCoord(pIsoView->MouseCurrentPosition);
                coord2 = point;

                if (!CMapData::Instance->IsCoordInMap(coord2.X, coord2.Y))
                    continue;
            }

            j = 0;
            int x2 = coord2.X;
            int y2 = coord2.Y;
            CIsoViewExt::MapCoord2ScreenCoord(x2, y2);
            drawX = x2 - CIsoViewExt::drawOffsetX + 30;
            drawY = y2 - CIsoViewExt::drawOffsetY - 15;
            double distance = sqrt((
                twoPoints.Point1.X - coord2.X)
                * (twoPoints.Point1.X - coord2.X)
                + (twoPoints.Point1.Y - coord2.Y)
                * (twoPoints.Point1.Y - coord2.Y));
            CIsoViewExt::DrawLineHDC(hDC, x1, y1, x2, y2, ExtConfigs::DistanceRuler_Color, rect, 2);
            if (twoPoints.drawText)
            {
                std::ostringstream oss;
                oss.precision(2);
                oss << std::fixed << distance;
                buffer.Format(Translations::TranslateOrDefault("DistanceRuler.Distance", "Distance: %s"), oss.str().c_str());
                TextOutClipped(hDC, drawX, drawY + lineHeight * j++, buffer, buffer.GetLength(), rect);
                buffer.Format(Translations::TranslateOrDefault("DistanceRuler.Coordinate", "XY: %d, %d, ¦¤XY: %d, %d"),
                    coord2.Y, coord2.X,
                    coord2.Y - twoPoints.Point1.Y, coord2.X - twoPoints.Point1.X);
                TextOutClipped(hDC, drawX, drawY + lineHeight * j++, buffer, buffer.GetLength(), rect);
            }
        }
    }
    for (auto& [mc, radius] : CIsoViewExt::Circles)
    {
        int drawX = mc.X;
        int drawY = mc.Y;
        CIsoViewExt::MapCoord2ScreenCoord(drawX, drawY);
        float rad = radius * cellLength;
        CIsoViewExt::DrawDashLineHDC(hDC, drawX, drawY, drawX + rad / CIsoViewExt::ScaledFactor, drawY, reversedColor, rect, 1);
        pIsoView->DrawEllipsePaint(drawX, drawY, rad, ExtConfigs::DistanceRuler_Color, hDC, rect, CIsoViewExt::ScaledFactor < 0.61 ? 4 : 2);
    }
    if (AxialSymmetryLine[0] != MapCoord{ 0,0 })
    {
        int j = 0;
        int x1 = AxialSymmetryLine[0].X;
        int y1 = AxialSymmetryLine[0].Y;
        CIsoViewExt::MapCoord2ScreenCoord(x1, y1);
        auto coord2 = AxialSymmetryLine[1];
        if (AxialSymmetryLine[1] == MapCoord{ 0,0 })
        {
            auto point = pIsoView->GetCurrentMapCoord(pIsoView->MouseCurrentPosition);
            coord2 = point;
        }
        if (CMapData::Instance->IsCoordInMap(coord2.X, coord2.Y))
        {
            j = 0;
            int x2 = coord2.X;
            int y2 = coord2.Y;
            CIsoViewExt::MapCoord2ScreenCoord(x2, y2);
            CIsoViewExt::DrawDashLineHDC(hDC, x1, y1, x2, y2, ExtConfigs::DistanceRuler_Color, rect, 3);
        }
    }
    int i = 0;
    for (auto& [mc1, mc2] : AxialSymmetricPoints)
    {
        if (!CMapData::Instance->IsCoordInMap(mc1.X, mc1.Y) || !CMapData::Instance->IsCoordInMap(mc2.X, mc2.Y))
            continue;
        FString buffer;
        int x1 = mc1.X;
        int y1 = mc1.Y;
        CIsoViewExt::MapCoord2ScreenCoord(x1, y1);
        int drawX1 = x1 - CIsoViewExt::drawOffsetX + 26 / CIsoViewExt::ScaledFactor - 6;
        int drawY1 = y1 - CIsoViewExt::drawOffsetY - 20 / CIsoViewExt::ScaledFactor - 3;
        int x2 = mc2.X;
        int y2 = mc2.Y;
        CIsoViewExt::MapCoord2ScreenCoord(x2, y2);
        int drawX2 = x2 - CIsoViewExt::drawOffsetX + 26 / CIsoViewExt::ScaledFactor - 6;
        int drawY2 = y2 - CIsoViewExt::drawOffsetY - 20 / CIsoViewExt::ScaledFactor - 3;
        CIsoViewExt::DrawDashLineHDC(hDC, x1, y1, x2, y2, reversedColor, rect, 1);
        buffer.Format("A%d", i);
        TextOutClipped(hDC, drawX1, drawY1, buffer, buffer.GetLength(), rect);
        buffer.Format("B%d", i);
        TextOutClipped(hDC, drawX2, drawY2, buffer, buffer.GetLength(), rect);
        ++i;
    }
    i = 0;
    for (auto& [mc1, mc2] : CentralSymmetricPoints)
    {
        if (!CMapData::Instance->IsCoordInMap(mc1.X, mc1.Y) || !CMapData::Instance->IsCoordInMap(mc2.X, mc2.Y))
            continue;
        FString buffer;
        int x1 = mc1.X;
        int y1 = mc1.Y;
        CIsoViewExt::MapCoord2ScreenCoord(x1, y1);
        int drawX1 = x1 - CIsoViewExt::drawOffsetX + 26 / CIsoViewExt::ScaledFactor - 6;
        int drawY1 = y1 - CIsoViewExt::drawOffsetY - 20 / CIsoViewExt::ScaledFactor - 3;
        int x2 = mc2.X;
        int y2 = mc2.Y;
        CIsoViewExt::MapCoord2ScreenCoord(x2, y2);
        int drawX2 = x2 - CIsoViewExt::drawOffsetX + 26 / CIsoViewExt::ScaledFactor - 6;
        int drawY2 = y2 - CIsoViewExt::drawOffsetY - 20 / CIsoViewExt::ScaledFactor - 3;
        CIsoViewExt::DrawDashLineHDC(hDC, x1, y1, x2, y2, reversedColor, rect, 1);
        buffer.Format("A%d", i);
        TextOutClipped(hDC, drawX1, drawY1, buffer, buffer.GetLength(), rect);
        buffer.Format("B%d", i);
        TextOutClipped(hDC, drawX2, drawY2, buffer, buffer.GetLength(), rect);
        ++i;
    }
    if (CentralSymmetryCenter != MapCoord{ 0,0 })
    {
        FString buffer(Translations::TranslateOrDefault("MeasurementToolbox.CentralSymmetryCenter", "Center"));
        int x = CentralSymmetryCenter.X;
        int y = CentralSymmetryCenter.Y;
        CIsoViewExt::MapCoord2ScreenCoord(x, y);
        int drawX = x - CIsoViewExt::drawOffsetX + 20 / CIsoViewExt::ScaledFactor - 6;
        int drawY = y - CIsoViewExt::drawOffsetY - 20 / CIsoViewExt::ScaledFactor - 3;
        TextOutClipped(hDC, drawX, drawY, buffer, buffer.GetLength(), rect);
    }
}

CRect CIsoViewExt::GetVisibleIsoViewRect()
{
    auto pThis = CIsoView::GetInstance(); 
    CRect rect; 
    pThis->GetWindowRect(&rect);
    AdaptRectForSecondScreen(&rect);

    if (ExtConfigs::SecondScreenSupport)
    {
        int vw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int vh = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        CRect virtualRect(0, 0, vw, vh);

        CRect destRect;
        destRect.IntersectRect(&rect, &virtualRect);

        return destRect;
    }

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
            point.x += rect.left - 16 - 18 + GetSystemMetrics(SM_XVIRTUALSCREEN);
            point.y += rect.top + 14 - 12 + GetSystemMetrics(SM_YVIRTUALSCREEN);
            auto cursor = CLoadingExt::GetSurfaceImageDataFromMap("scrollcursor.bmp");
            CIsoViewExt::BlitTransparent(cursor->lpSurface, point.x, point.y, -1, -1, 255, surface);
        }

        HDC hDC;
        surface->GetDC(&hDC);
        int fontSize = ExtConfigs::DisplayTextSize;
        if (CIsoViewExt::ScaledFactor < 0.3)
            fontSize += 6;
        else if (CIsoViewExt::ScaledFactor < 0.5)
            fontSize += 4;
        else if (CIsoViewExt::ScaledFactor < 0.75)
            fontSize += 2;
        HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Cambria");
        HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

        if (EnableLiveDistanceRuler)
        {
            DrawDistanceRuler(hDC, rect);
        }
        if (EnableOtherMeasurementTools)
        {
            DrawOtherMeasurementTools(hDC, rect);
        }
        if (DrawScriptPath)
        {
            DrawScriptPaths(hDC, rect);
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
        if (CIsoViewExt::ScaledFactor < 0.3)
            fontSize += 6;
        else if (CIsoViewExt::ScaledFactor < 0.5)
            fontSize += 4;
        else if (CIsoViewExt::ScaledFactor < 0.75)
            fontSize += 2;
        HFONT hFont = CreateFont(fontSize, 0, 0, 0,  FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Cambria");
        HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

        RECT rect;
        auto pThis = CIsoView::GetInstance();
        pThis->GetWindowRect(&rect);
        AdaptRectForSecondScreen(&rect);
        if (EnableLiveDistanceRuler)
        {
            DrawDistanceRuler(hDC, rect);
        }
        if (EnableOtherMeasurementTools)
        {
            DrawOtherMeasurementTools(hDC, rect);
        }
        if (DrawScriptPath)
        {
            DrawScriptPaths(hDC, rect);
        }
        DrawMouseMove(hDC, rect);
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
        if (CIsoViewExt::ScaledFactor < 0.3)
            fontSize += 6;
        else if (CIsoViewExt::ScaledFactor < 0.5)
            fontSize += 4;
        else if (CIsoViewExt::ScaledFactor < 0.75)
            fontSize += 2;
        HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Cambria");
        HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

        RECT rect;
        auto pThis = CIsoView::GetInstance();
        pThis->GetWindowRect(&rect);
        AdaptRectForSecondScreen(&rect);
        if (EnableLiveDistanceRuler)
        {
            DrawDistanceRuler(hDC, rect);
        }
        if (EnableOtherMeasurementTools)
        {
            DrawOtherMeasurementTools(hDC, rect);
        }
        if (DrawScriptPath)
        {
            DrawScriptPaths(hDC, rect);
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

        if (EnableLiveDistanceRuler || EnableOtherMeasurementTools)
        {
            int fontSize = ExtConfigs::DisplayTextSize;
            if (CIsoViewExt::ScaledFactor < 0.3)
                fontSize += 6;
            else if (CIsoViewExt::ScaledFactor < 0.5)
                fontSize += 4;
            else if (CIsoViewExt::ScaledFactor < 0.75)
                fontSize += 2;
            HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE, "Cambria");
            HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

            RECT rect;
            auto pThis = CIsoView::GetInstance();
            pThis->GetWindowRect(&rect);
            AdaptRectForSecondScreen(&rect);
            if (EnableLiveDistanceRuler)
                DrawDistanceRuler(hDC, rect);
            if (EnableOtherMeasurementTools)
                DrawOtherMeasurementTools(hDC, rect);
            if (DrawScriptPath)
                DrawScriptPaths(hDC, rect);

            SelectObject(hDC, hOldFont);
            DeleteObject(hFont);
        }
        DrawBridgeLine(hDC);
        surface->ReleaseDC(hDC);
        break;
    }
    case 4:
    {
        if (EnableLiveDistanceRuler || EnableOtherMeasurementTools)
        {
            HDC hDC;
            surface->GetDC(&hDC);
            int fontSize = ExtConfigs::DisplayTextSize;
            if (CIsoViewExt::ScaledFactor < 0.3)
                fontSize += 6;
            else if (CIsoViewExt::ScaledFactor < 0.5)
                fontSize += 4;
            else if (CIsoViewExt::ScaledFactor < 0.75)
                fontSize += 2;
            HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE, "Cambria");
            HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

            RECT rect;
            auto pThis = CIsoView::GetInstance();
            pThis->GetWindowRect(&rect);
            AdaptRectForSecondScreen(&rect);
            if (EnableLiveDistanceRuler)
                DrawDistanceRuler(hDC, rect);
            if (EnableOtherMeasurementTools)
                DrawOtherMeasurementTools(hDC, rect);
            if (DrawScriptPath)
                DrawScriptPaths(hDC, rect);

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
    RECT rect = GetScaledWindowRect();
    int x = 30 * (CMapData::Instance->MapWidthPlusHeight + Y - X) - (rect.right - rect.left) / 2 - rect.left;
    int y = 15 * (Y + X) - CMapData::Instance->TryGetCellAt(X, Y)->Height - (rect.bottom - rect.top) / 2 - rect.top;
    pThis->MoveTo(x, y);
    pThis->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    CFinalSunDlg::Instance->MyViewFrame.Minimap.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}

void CIsoViewExt::Zoom(double offset)
{
    if (CMapData::Instance->MapWidthPlusHeight)
    {
        auto pThis = CIsoViewExt::GetExtension();
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
            AdaptRectForSecondScreen(&oriRect);
            double mousePosX;
            double mousePosY;
            if (ExtConfigs::SecondScreenSupport)
            {
                mousePosX = static_cast<double>(pThis->MouseCurrentPosition.x + GetSystemMetrics(SM_XVIRTUALSCREEN)) / oriRect.Width();
                mousePosY = static_cast<double>(pThis->MouseCurrentPosition.y + GetSystemMetrics(SM_YVIRTUALSCREEN)) / oriRect.Height();
            }
            else
            {
                mousePosX = static_cast<double>(pThis->MouseCurrentPosition.x) / oriRect.Width();
                mousePosY = static_cast<double>(pThis->MouseCurrentPosition.y) / oriRect.Height();
            }

            pThis->ViewPosition.x += (oldRect.Width() - newRect.Width()) * mousePosX;
            pThis->ViewPosition.y += (oldRect.Height() - newRect.Height()) * mousePosY;
            pThis->MoveTo(pThis->ViewPosition.x, pThis->ViewPosition.y);

            if(pThis->g_pDX)
                pThis->g_pDX->SetZoomOut(CIsoViewExt::ScaledFactor);

            pThis->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            CFinalSunDlg::Instance->MyViewFrame.Minimap.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        }
    }
}

void CIsoViewExt::DrawMultiMapCoordBorders(HDC hDC, const std::vector<MapCoord>& coords, COLORREF color, int offsetX, int offsetY)
{
    auto pThis = static_cast<CIsoViewExt*>(CIsoView::GetInstance());

    auto MakeCoordKey = [](int x, int y)
    {
        return (static_cast<uint32_t>(x) << 16) | static_cast<uint16_t>(y);
    };

    std::unordered_set<uint32_t> coordSet;
    coordSet.reserve(coords.size());

    for (const auto& mc : coords)
    {
        if (CMapDataExt::IsCoordInFullMap(mc.X, mc.Y))
        {
            coordSet.insert(MakeCoordKey(mc.X, mc.Y));
        }
    }

    for (const auto& mc : coords)
    {
        if (!CMapDataExt::IsCoordInFullMap(mc.X, mc.Y))
            continue;

        int x = mc.X;
        int y = mc.Y;
        CIsoViewExt::MapCoord2ScreenCoord(x, y);

        int drawX = x - CIsoViewExt::drawOffsetX + offsetX;
        int drawY = y - CIsoViewExt::drawOffsetY + offsetY;

        bool s1 = true;
        bool s2 = true;
        bool s3 = true;
        bool s4 = true;

        if (coordSet.count(MakeCoordKey(mc.X - 1, mc.Y))) s1 = false;
        if (coordSet.count(MakeCoordKey(mc.X + 1, mc.Y))) s3 = false;
        if (coordSet.count(MakeCoordKey(mc.X, mc.Y + 1))) s2 = false;
        if (coordSet.count(MakeCoordKey(mc.X, mc.Y - 1))) s4 = false;

        pThis->DrawLockedCellOutlinePaint(drawX, drawY, 1, 1, color, false, hDC, pThis->m_hWnd, s1, s2, s3, s4);
    }
}

void CIsoViewExt::DrawMultiMapCoordBorders(LPDDSURFACEDESC2 lpDesc, const std::vector<MapCoord>& coords, COLORREF color)
{
    auto pThis = static_cast<CIsoViewExt*>(CIsoView::GetInstance());

    auto MakeCoordKey = [](int x, int y)
    {
        return (static_cast<uint32_t>(x) << 16) | static_cast<uint16_t>(y);
    };

    std::unordered_set<uint32_t> coordSet;
    coordSet.reserve(coords.size());

    for (const auto& mc : coords)
    {
        if (CMapDataExt::IsCoordInFullMap(mc.X, mc.Y))
        {
            coordSet.insert(MakeCoordKey(mc.X, mc.Y));
        }
    }

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

        if (coordSet.count(MakeCoordKey(mc.X - 1, mc.Y))) s1 = false;
        if (coordSet.count(MakeCoordKey(mc.X + 1, mc.Y))) s3 = false;
        if (coordSet.count(MakeCoordKey(mc.X, mc.Y + 1))) s2 = false;
        if (coordSet.count(MakeCoordKey(mc.X, mc.Y - 1))) s4 = false;

        pThis->DrawLockedCellOutline(
            drawX, drawY,
            1, 1,
            color,
            false, false,
            lpDesc,
            s1, s2, s3, s4
        );
    }
}

void CIsoViewExt::DrawMultiMapCoordBorders(LPDDSURFACEDESC2 lpDesc, const std::set<MapCoord>& coords, COLORREF color)
{
    auto pThis = static_cast<CIsoViewExt*>(CIsoView::GetInstance());

    auto MakeCoordKey = [](int x, int y)
    {
        return (static_cast<uint32_t>(x) << 16) | static_cast<uint16_t>(y);
    };

    std::unordered_set<uint32_t> coordSet;
    coordSet.reserve(coords.size());

    for (const auto& mc : coords)
    {
        if (CMapDataExt::IsCoordInFullMap(mc.X, mc.Y))
        {
            coordSet.insert(MakeCoordKey(mc.X, mc.Y));
        }
    }

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

        if (coordSet.count(MakeCoordKey(mc.X - 1, mc.Y))) s1 = false;
        if (coordSet.count(MakeCoordKey(mc.X + 1, mc.Y))) s3 = false;
        if (coordSet.count(MakeCoordKey(mc.X, mc.Y + 1))) s2 = false;
        if (coordSet.count(MakeCoordKey(mc.X, mc.Y - 1))) s4 = false;

        pThis->DrawLockedCellOutline(drawX, drawY, 1, 1, color, false, false, lpDesc, s1, s2, s3, s4);
    }
}

void CIsoViewExt::TextOutClipped(HDC hdc, int x, int y, const char* text, int len, const RECT& rect)
{
    if (!hdc || !text || len <= 0)
        return;

    if (x < rect.left || x > rect.right ||
        y < rect.top || y > rect.bottom)
    {
        return;
    }

    ::TextOut(hdc, x, y, text, len);
}

bool CIsoViewExt::ClipLineToRect(int& x1, int& y1, int& x2, int& y2, const RECT& rect)
{
    auto encode = [&rect](int x, int y)
    {
        int c = 0;
        if (x < rect.left)        c |= 1; 
        else if (x > rect.right)  c |= 2; 
        if (y > rect.bottom)      c |= 4; 
        else if (y < rect.top)    c |= 8; 
        return c;
    };

    int code1 = encode(x1, y1);
    int code2 = encode(x2, y2);

    while (code1 || code2)
    {
        if (code1 & code2)
            return false;

        int code = code1 ? code1 : code2;

        int x = 0, y = 0;

        int dx = x2 - x1;
        int dy = y2 - y1;

        if ((code & 1) && dx != 0)
        {
            x = rect.left;
            y = y1 + dy * (rect.left - x1) / dx;
        }
        else if ((code & 2) && dx != 0)
        {
            x = rect.right;
            y = y1 + dy * (rect.right - x1) / dx;
        }
        else if ((code & 4) && dy != 0) 
        {
            y = rect.bottom;
            x = x1 + dx * (rect.bottom - y1) / dy;
        }
        else if ((code & 8) && dy != 0) 
        {
            y = rect.top;
            x = x1 + dx * (rect.top - y1) / dy;
        }
        else
        {
            return false;
        }

        if (code == code1)
        {
            x1 = x;
            y1 = y;
            code1 = encode(x1, y1);
        }
        else
        {
            x2 = x;
            y2 = y;
            code2 = encode(x2, y2);
        }
    }

    return true;
}

void CIsoViewExt::DrawLineHDC(HDC hDC, int x1, int y1, int x2, int y2, int color, const RECT& rect, int size)
{
    x1 += 36 / CIsoViewExt::ScaledFactor - 6;
    x2 += 36 / CIsoViewExt::ScaledFactor - 6;
    y1 -= 12.5 / CIsoViewExt::ScaledFactor + 2.5;
    y2 -= 12.5 / CIsoViewExt::ScaledFactor + 2.5;

    x1 -= CIsoViewExt::drawOffsetX;
    y1 -= CIsoViewExt::drawOffsetY;
    x2 -= CIsoViewExt::drawOffsetX;
    y2 -= CIsoViewExt::drawOffsetY;

    if (!ClipLineToRect(x1, y1, x2, y2, rect))
        return;

    HPEN hPen = CreatePen(
        PS_SOLID,
        CIsoViewExt::ScaledFactor < 0.61 ? (2 + size) : size,
        color
    );

    HPEN hOld = (HPEN)SelectObject(hDC, hPen);

    MoveToEx(hDC, x1, y1, NULL);
    LineTo(hDC, x2, y2);

    SelectObject(hDC, hOld);
    DeleteObject(hPen);
}

void CIsoViewExt::DrawArrowHDC(HDC hDC, int x1, int y1, int x2, int y2, int color, const RECT& rect, int size)
{
    x1 += 36 / CIsoViewExt::ScaledFactor - 6;
    x2 += 36 / CIsoViewExt::ScaledFactor - 6;
    y1 -= 12.5 / CIsoViewExt::ScaledFactor + 2.5;
    y2 -= 12.5 / CIsoViewExt::ScaledFactor + 2.5;

    int sx1 = x1 - CIsoViewExt::drawOffsetX;
    int sy1 = y1 - CIsoViewExt::drawOffsetY;
    int sx2 = x2 - CIsoViewExt::drawOffsetX;
    int sy2 = y2 - CIsoViewExt::drawOffsetY;

    int cx1 = sx1, cy1 = sy1;
    int cx2 = sx2, cy2 = sy2;

    if (!ClipLineToRect(cx1, cy1, cx2, cy2, rect))
        return;

    HPEN hPen = CreatePen(
        PS_SOLID,
        CIsoViewExt::ScaledFactor < 0.61 ? (2 + size) : size,
        color
    );
    HPEN hOld = (HPEN)SelectObject(hDC, hPen);

    MoveToEx(hDC, cx1, cy1, NULL);
    LineTo(hDC, cx2, cy2);

    double dx = sx2 - sx1;
    double dy = sy2 - sy1;
    double len = sqrt(dx * dx + dy * dy);

    if (len > 0.0001)
    {
        double ux = dx / len;
        double uy = dy / len;

        double arrowLen = 10.0 / CIsoViewExt::ScaledFactor;
        double arrowWidth = 5.0 / CIsoViewExt::ScaledFactor;

        double px = -uy;
        double py = ux;

        POINT pts[3];

        pts[0].x = sx2;
        pts[0].y = sy2;

        pts[1].x = (LONG)(sx2 - ux * arrowLen + px * arrowWidth);
        pts[1].y = (LONG)(sy2 - uy * arrowLen + py * arrowWidth);

        pts[2].x = (LONG)(sx2 - ux * arrowLen - px * arrowWidth);
        pts[2].y = (LONG)(sy2 - uy * arrowLen - py * arrowWidth);

        if (PtInRect(&rect, pts[0]) || PtInRect(&rect, pts[1]) || PtInRect(&rect, pts[2]))
        {
            HBRUSH hBrush = CreateSolidBrush(color);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hDC, hBrush);

            Polygon(hDC, pts, 3);

            SelectObject(hDC, hOldBrush);
            DeleteObject(hBrush);
        }
    }

    SelectObject(hDC, hOld);
    DeleteObject(hPen);
}

void CIsoViewExt::DrawDashLineHDC(HDC hDC, int x1, int y1, int x2, int y2, int color, const RECT& rect, int size)
{
    x1 += 36 / CIsoViewExt::ScaledFactor - 6;
    x2 += 36 / CIsoViewExt::ScaledFactor - 6;
    y1 -= 12.5 / CIsoViewExt::ScaledFactor + 2.5;
    y2 -= 12.5 / CIsoViewExt::ScaledFactor + 2.5;

    int sx1 = x1 - CIsoViewExt::drawOffsetX;
    int sy1 = y1 - CIsoViewExt::drawOffsetY;
    int sx2 = x2 - CIsoViewExt::drawOffsetX;
    int sy2 = y2 - CIsoViewExt::drawOffsetY;

    if (!ClipLineToRect(sx1, sy1, sx2, sy2, rect))
        return;

    LOGBRUSH lb = {};
    lb.lbStyle = BS_SOLID;
    lb.lbColor = color;

    DWORD style[] = { 6, 4 };

    HPEN hPen = ExtCreatePen(
        PS_GEOMETRIC | PS_USERSTYLE | PS_ENDCAP_FLAT,
        size,
        &lb,
        2,
        style
    );

    HPEN hOld = (HPEN)SelectObject(hDC, hPen);

    MoveToEx(hDC, sx1, sy1, NULL);
    LineTo(hDC, sx2, sy2);

    SelectObject(hDC, hOld);
    DeleteObject(hPen);
}

std::vector<MapCoord> CIsoViewExt::GetLinePoints(MapCoord mc1, MapCoord mc2)
{
    std::vector<MapCoord> points;

    if (mc1.X == 0 && mc1.Y == 0 || mc2.X == 0 && mc2.Y == 0)
        return points;

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

bool CIsoViewExt::BlitDDSurfaceRectToBitmap(
    HDC hDC,
    const DDBoundary& boundary,
    const RECT& srcRect,
    int dstX,
    int dstY)
{
    if (!hDC || !pFullBitmap)
        return false;

    RECT rc = srcRect;

    if (srcRect.right < 0 || srcRect.bottom < 0)
    {
        Sleep(50);
        return false;
    }

    if (rc.left < 0)
        rc.left = 0;
    if (rc.top < 0)
        rc.top = 0;
    if (rc.right > (int)boundary.dwWidth)
        rc.right = boundary.dwWidth;
    if (rc.bottom > (int)boundary.dwHeight)
        rc.bottom = boundary.dwHeight;


    auto pIsoView = CIsoView::GetInstance();
    int& height = CMapData::Instance->Size.Height;
    int& width = CMapData::Instance->Size.Width;
    int endX, endY;
    int startOffsetX = width - 1;
    int startOffsetY = 0;

    pIsoView->MapCoord2ScreenCoord_Flat(startOffsetX, startOffsetY);

    if (CIsoViewExt::RenderFullMap)
    {
        endX = height;
        endY = width + height + 1;
    }
    else
    {
        const int& mapwidth = CMapData::Instance->Size.Width;
        const int& mapheight = CMapData::Instance->Size.Height;
        const int& mpL = CMapData::Instance->LocalSize.Left;
        const int& mpT = CMapData::Instance->LocalSize.Top;
        const int& mpW = CMapData::Instance->LocalSize.Width;
        const int& mpH = CMapData::Instance->LocalSize.Height;

        endX = mapwidth - mpL - mpW + mpT - 3 + mpH + 4;
        endY = mpT + mpL + mpW - 2 + mpH + 4;
    }

    pIsoView->MapCoord2ScreenCoord_Flat(endX, endY);
    endX -= startOffsetX;
    endY -= startOffsetY;

    if (rc.right - rc.left + dstX > endX)
        rc.right = endX - dstX + rc.left;
    if (rc.bottom + dstY - rc.top > endY)
        rc.bottom = endY - dstY + rc.top;

    if (rc.right <= rc.left || rc.bottom <= rc.top)
        return true;

    int srcW = rc.right - rc.left;
    int srcH = rc.bottom - rc.top;

    int finalW = dstX + srcW;
    int finalH = dstY + srcH;

    if (finalW > (int)pFullBitmap->GetWidth())
        srcW -= finalW - pFullBitmap->GetWidth();

    if (finalH > (int)pFullBitmap->GetHeight())
        srcH -= finalH - pFullBitmap->GetHeight();

    if (srcW <= 0 || srcH <= 0)
        return true;

    Graphics g(pFullBitmap);
    HDC hdcTarget = g.GetHDC();
    BOOL bOK = BitBlt(hdcTarget, dstX, dstY, srcW, srcH, hDC, rc.left, rc.top, SRCCOPY);
    g.ReleaseHDC(hdcTarget);

    return bOK == TRUE;
}

int CIsoViewExt::GetOverlayDrawOffset(WORD nOverlay, BYTE nOverlayData)
{
    auto type = CMapDataExt::GetOverlayTypeData(nOverlay);
    if (nOverlay == 0xA7) 
    {
        return -49;
    }
    else if (type.Veins)
    {
        return -1;
    }
    else if (
        nOverlay != 0x18 && nOverlay != 0x19 && // BRIDGE1, BRIDGE2
        nOverlay != 0x3B && nOverlay != 0x3C && // RAILBRDG1, RAILBRDG2
        nOverlay != 0xED && nOverlay != 0xEE // BRIDGEB1, BRIDGEB2
        )
    {
        if (nOverlay >= 0x27 && nOverlay <= 0x36) // Tracks
            return 14;
        else if (type.RailRoad)
            return 14;
        else if (type.Wall)
            return 3;
        else if (type.Crate)
            return 3;
        else if (type.Tiberium)
            return 3;
        else
            return 15;
    }
    else
    {
        if (nOverlayData >= 0x9 && nOverlayData <= 0x11)
            return -16;
        else
            return -1;
    }
    return 15;
}

void CIsoViewExt::SetStatusBarText(const char* text)
{
    if (text && strlen(text) > 0)
    {
        ::SendMessage(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd, 0x401, 0, (LPARAM)text);
        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
        ::UpdateWindow(CFinalSunDlg::Instance->MyViewFrame.StatusBar.m_hWnd);
    }
}

void CIsoViewExt::PlaceTileOnMouse(int x, int y, int nFlags, bool recordHistory)
{
    auto Map = CMapDataExt::GetExtension();

    if (CIsoView::CurrentCommand->Type < CUSTOM_TILE_START)
    {
        int i, e, f, n;
        int p = 0;
        auto tileData = CMapDataExt::TileData[CIsoView::CurrentCommand->Type];
        auto cell = Map->TryGetCellAt(x, y);

        int width = tileData.Height;
        int height = tileData.Width;
        int pos = x - width + 1 + (y - height + 1) * Map->MapWidthPlusHeight;
        int startheight = cell->Height + CIsoView::CurrentCommand->Height;
        int ground = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
        int subGround = CMapDataExt::GetSafeSubTileIndex(cell->TileIndex, cell->TileSubIndex);

        startheight -= CMapDataExt::TileData[ground].TileBlockDatas[subGround].Height;

        if (recordHistory)
            Map->SaveUndoRedoData(TRUE, x - width - 4,
                y - height - 4,
                x - width + this->BrushSizeX * width + 7,
                y - height + this->BrushSizeY * height + 7);
        int ori_x = x - width + 1;
        int ori_y = y - height + 1;

        for (f = 0; f < this->BrushSizeX; f++)
        {
            for (n = 0; n < this->BrushSizeY; n++)
            {
                int tile = CIsoView::CurrentCommand->Type;

                if (CIsoView::CurrentCommand->Param == 1)
                {
                    tile = CIsoViewExt::GetRandomTileIndex();
                }

                p = 0;
                for (i = 0; i < tileData.Height; i++)
                {
                    for (e = 0; e < tileData.Width; e++)
                    {
                        if (tileData.TileBlockDatas[p].ImageData != NULL)
                        {
                            int my_x = ori_x + f * width + i;
                            int my_y = ori_y + n * height + e;
                            if (!Map->IsCoordInMap(my_x, my_y))
                            {
                                p++;
                                continue;
                            }
                            auto cell = Map->GetCellAt(my_x, my_y);

                            if (!(ExtConfigs::PlaceTileSkipHide && cell->IsHidden()))
                            {
                                Map->SetHeightAt(my_x, my_y,
                                    startheight + tileData.TileBlockDatas[p].Height);

                                auto tileSet = tileData.TileSet;
                                bool isBridge = (tileSet == CMapDataExt::BridgeSet || tileSet == CMapDataExt::WoodBridgeSet);

                                cell->TileIndex = tile;
                                cell->TileSubIndex = p;
                                cell->Flag.AltIndex = isBridge ? 0 : STDHelpers::RandomSelectInt(0, tileData.AltTypeCount + 1);
                                CMapData::Instance->UpdateMapPreviewAt(my_x, my_y);
                            }
                        }
                        p++;
                    }
                }
            }
        }

        if (!((nFlags & MK_CONTROL) && (nFlags & MK_SHIFT)))
        {
            if (!CFinalSunApp::Instance->DisableAutoShore)
                Map->CreateShore(x - width - 2, y - height - 2,
                    x - width + tileData.Height * this->BrushSizeX + 5,
                    y - height + tileData.Width * this->BrushSizeY + 5, FALSE);
            if (!CFinalSunApp::Instance->DisableAutoLat)
            {
                for (f = 0; f < this->BrushSizeX; f++)
                {
                    for (n = 0; n < this->BrushSizeY; n++)
                    {
                        p = 0;
                        for (i = -1; i < tileData.Height + 1; i++)
                        {
                            for (e = -1; e < tileData.Width + 1; e++)
                            {
                                int my_x = ori_x + f * width + i;
                                int my_y = ori_y + n * height + e;
                                if (!Map->IsCoordInMap(my_x, my_y))
                                {
                                    continue;
                                }
                                Map->SmoothTileAt(my_x, my_y);
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        int i, e, f, n;
        int p = 0;
        auto tileData = CMapDataExt::GetCustomTile(CIsoView::CurrentCommand->Type);
        auto cell = Map->TryGetCellAt(x, y);

        int width = tileData->Height;
        int height = tileData->Width;
        int pos = x - width + 1 + (y - height + 1) * Map->MapWidthPlusHeight;
        int startheight = cell->Height + CIsoView::CurrentCommand->Height;
        int ground = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
        int subGround = CMapDataExt::GetSafeSubTileIndex(cell->TileIndex, cell->TileSubIndex);

        startheight -= CMapDataExt::TileData[ground].TileBlockDatas[subGround].Height;

        if (recordHistory)
            Map->SaveUndoRedoData(TRUE, x - width - 4,
                y - height - 4,
                x - width + this->BrushSizeX * width + 7,
                y - height + this->BrushSizeY * height + 7);
        int ori_x = x - width + 1;
        int ori_y = y - height + 1;

        for (f = 0; f < this->BrushSizeX; f++)
        {
            for (n = 0; n < this->BrushSizeY; n++)
            {
                p = 0;
                for (i = 0; i < tileData->Height; i++)
                {
                    for (e = 0; e < tileData->Width; e++)
                    {
                        auto& tile = tileData->TileBlockDatas[p];
                        auto block = tile.GetTileBlock();
                        if (block && block->ImageData != NULL)
                        {
                            int my_x = ori_x + f * width + i;
                            int my_y = ori_y + n * height + e;
                            if (!Map->IsCoordInMap(my_x, my_y))
                            {
                                p++;
                                continue;
                            }
                            auto cell = Map->GetCellAt(my_x, my_y);

                            if (!(ExtConfigs::PlaceTileSkipHide && cell->IsHidden()))
                            {
                                auto tileData = CMapDataExt::TileData[tile.TileIndex];
                                auto tileSet = tileData.TileSet;
                                bool isBridge = (tileSet == CMapDataExt::BridgeSet || tileSet == CMapDataExt::WoodBridgeSet);

                                Map->SetHeightAt(my_x, my_y,
                                    startheight + tile.GetHeight());

                                cell->TileIndex = tile.TileIndex;
                                cell->TileSubIndex = tile.SubTileIndex;
                                cell->Flag.AltIndex = isBridge ? 0 : STDHelpers::RandomSelectInt(0, tileData.AltTypeCount + 1);

                                CMapData::Instance->UpdateMapPreviewAt(my_x, my_y);
                            }
                        }
                        p++;
                    }
                }
            }
        }

        if (!((nFlags & MK_CONTROL) && (nFlags & MK_SHIFT)))
        {
            if (!CFinalSunApp::Instance->DisableAutoShore)
                Map->CreateShore(x - width - 2, y - height - 2,
                    x - width + tileData->Height * this->BrushSizeX + 5,
                    y - height + tileData->Width * this->BrushSizeY + 5, FALSE);

            if (!CFinalSunApp::Instance->DisableAutoLat)
            {
                for (f = 0; f < this->BrushSizeX; f++)
                {
                    for (n = 0; n < this->BrushSizeY; n++)
                    {
                        p = 0;
                        for (i = -1; i < tileData->Height + 1; i++)
                        {
                            for (e = -1; e < tileData->Width + 1; e++)
                            {
                                int my_x = ori_x + f * width + i;
                                int my_y = ori_y + n * height + e;
                                if (!Map->IsCoordInMap(my_x, my_y))
                                {
                                    continue;
                                }
                                Map->SmoothTileAt(my_x, my_y);
                            }
                        }
                    }
                }
            }
        }
    }
}

ImageDataView CIsoViewExt::MakeImageDataView(ImageDataClassSafe* p, Palette* pPal)
{
    return {
        p->FullWidth,
        p->FullHeight,
        p->pImageBuffer.get(),
        p->pOpacity.get(),
        pPal ? pPal :p->pPalette,
        ImageDataView::ImageDataViewType::ImageDataSafe,
        p
    };
}

ImageDataView CIsoViewExt::MakeImageDataView(ImageDataClass* p, Palette* pPal)
{
    return {
        p->FullWidth,
        p->FullHeight,
        p->pImageBuffer,
        nullptr,
        pPal ? pPal : p->pPalette,
        ImageDataView::ImageDataViewType::ImageData,
        p
    };
}

ImageDataView CIsoViewExt::MakeImageDataView(CTileBlockClass* p, Palette* pPal)
{
    return {
        p->BlockWidth,
        p->BlockHeight,
        p->ImageData,
        nullptr,
        pPal,
        ImageDataView::ImageDataViewType::TileBlockData,
        p
    };
}

BOOL CIsoViewExt::PreTranslateMessageExt(MSG* pMsg)
{
    return CIsoView::PreTranslateMessage(pMsg);
}

BOOL CIsoViewExt::OnMouseWheelExt(UINT Flags, short zDelta, CPoint point)
{
    return TRUE;
}
