#include "Body.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Miscs/TheaterInfo.h"
#include "../../FA2sp.h"
#include <CINI.h>
#include <CMapData.h>
#include <CIsoView.h>
#include <CTileTypeClass.h>
#include <CLoading.h>
#include "../CTileSetBrowserFrame/Body.h"
#include "../CMapData/Body.h"
#include "../../Miscs/MultiSelection.h"

std::unordered_map<int, ConnectedTileInfo> CViewObjectsExt::TreeView_ConnectedTileMap;
std::vector<ConnectedTileSet> CViewObjectsExt::ConnectedTileSets;
int CViewObjectsExt::CurrentConnectedTileType;

MapCoord CViewObjectsExt::CliffConnectionCoord;
std::vector<MapCoord> CViewObjectsExt::CliffConnectionCoordRecords;
int CViewObjectsExt::CliffConnectionTile;
int CViewObjectsExt::CliffConnectionHeight;
int CViewObjectsExt::CliffConnectionHeightAdjust;
ConnectedTiles CViewObjectsExt::LastPlacedCT;
int CViewObjectsExt::LastTempPlacedCTIndex;
int CViewObjectsExt::LastTempFacing;
std::vector<ConnectedTiles> CViewObjectsExt::LastPlacedCTRecords;
ConnectedTiles CViewObjectsExt::ThisPlacedCT;
int CViewObjectsExt::LastCTTile;
int CViewObjectsExt::LastSuccessfulIndex;
int CViewObjectsExt::NextCTHeightOffset;
int CViewObjectsExt::LastSuccessfulHeightOffset;
bool CViewObjectsExt::LastSuccessfulOpposite;
bool CViewObjectsExt::IsUsingTXCliff = false;
bool CViewObjectsExt::PlaceConnectedTile_Start = false;
bool CViewObjectsExt::PlaceConnectedTile_AutoConnect = false;
bool CViewObjectsExt::HeightChanged;
bool CViewObjectsExt::IsInPlaceCliff_OnMouseMove;
std::vector<int> CViewObjectsExt::LastCTTileRecords;
std::vector<int> CViewObjectsExt::LastHeightRecords;

// ===================== AutoConnect virtual placement =====================
// While planning a path, tile writes are redirected into an in-memory segment
// buffer instead of touching the real map. The planned path is applied to the
// map only as a preview (fully revertible) and committed on the second click.

namespace AutoConnect
{
    // snapshot of the mutable placement state, used to roll back a rejected
    // trial segment or to restart planning from the session start point
    struct SegmentState
    {
        MapCoord Anchor;
        int Height;
        ConnectedTiles LastCT;
        ConnectedTiles ThisCT;          // used by long-distance continuation; must survive rejected trials
        int LastCTTile;
        int CliffTile;                  // current preview tile; must survive rejected trials
        bool PlaceStart;                // first-tile start compensation must survive rejected trials
        int LastSuccessfulIndex;
        bool LastSuccessfulOpposite;
        int LastSuccessfulHeightOffset;
        int NextCTHeightOffset;
        bool HeightChanged;
        int HeightAdjust;
        int LastTempPlacedCTIndex;
        int LastTempFacing;
    };

    struct Segment
    {
        std::map<int, CellData> Cells;                  // dwpos -> new cell value (tile body)
        std::map<int, CellData> FixCells;               // dwpos -> junction patch cell value
        std::map<int, std::pair<int, int>> Overlays;    // dwpos -> {overlay, overlayData}
        SegmentState Before;
    };

    // one committed placement batch: an AutoConnect path or a manual single
    // segment (the result of one left-click commit)
    struct CommittedBatch
    {
        std::map<int, CellData> Cells;                  // tile bodies
        std::map<int, CellData> FixCells;               // junction patches
        std::map<int, std::pair<int, int>> Overlays;    // overlays
    };

    struct Session
    {
        bool Active = false;                // start point confirmed, target expected
        bool Previewing = false;            // preview currently applied to the map
        MapCoord Start;
        MapCoord Target;
        int LastPreviewPos = -1;            // dedupe mousemove replanning
        SegmentState SessionBefore;
        std::vector<Segment> Segments;
        std::map<int, CellData> MergedCells;            // tile bodies of all accepted segments
        std::map<int, CellData> MergedFixCells;         // junction patches of all accepted segments
        std::map<int, CellData> OldCells;               // real values before preview applied
        std::map<int, std::pair<int, int>> OldOverlays; // real {overlay, overlayData} before preview applied

        // Persisted across chained AutoConnect batches within one placement
        // flow. Used so later batches also avoid cells already committed by
        // earlier batches.
        std::vector<CommittedBatch> CommittedBatches;           // per-batch history (for undo)
        std::map<int, CellData> CommittedCells;                 // aggregate tile bodies of all committed batches
        std::map<int, CellData> CommittedFixCells;              // aggregate junction patches of all committed batches
        std::map<int, std::pair<int, int>> CommittedOverlays;   // aggregate overlays of all committed batches
        CommittedBatch PendingManualBatch;                      // scratch buffer for a single manual placement
    };

    Session g_Session;
    bool g_VirtualPlacing = false;
    Segment* g_pCurrentSegment = nullptr;
    CommittedBatch* g_pCurrentManualBatch = nullptr;

    // ===================== Closure (head-to-tail) planning =====================
    // The connection state of a placed tile is fully described by the triple
    // (LastPlacedCT.Index, LastPlacedCT.Opposite, CliffConnectionCoord). If the
    // last tile of a closing path ends with the same triple as the flow's very
    // first tile, the loop is closed: the next tile would land exactly on the
    // first tile, i.e. the first and the last tile fully overlap.
    bool ClosureFirstRecorded = false;              // the first tile has been captured
    bool ClosureFirstPending = true;                // the flow's real first tile is not locked yet; virtual plans only stage it
    MapCoord ClosureFlowStart{ -1, -1 };            // flow start anchor P
    int ClosureFirstIndex = -1;                     // first tile triple: tile index
    bool ClosureFirstOpposite = false;              // first tile triple: opposite
    MapCoord ClosureFirstCoord{ -1, -1 };           // first tile triple: anchor after it
    int ClosureFirstVariant = -1;                   // concrete tile of the first placement
    std::map<int, CellData> ClosureFirstCellData;   // cells written by the first placement
    std::map<int, std::pair<int, int>> ClosureFirstOverlays; // overlays of the first placement

    // closure search runtime state
    MapCoord ClosureCursor{ -1, -1 };
    MapCoord ClosureRealCursor{ -1, -1 };  // the real mouse cursor; survives DFS mutation of ClosureCursor
    int ClosureTargetIndex = -1;
    bool ClosureTargetOpposite = false;
    MapCoord ClosureTargetCoord{ -1, -1 };
    int ClosureMaxSteps = 0;
    std::set<int> ClosureFirstBodyCells;    // positions of the first tile's body cells only
    int g_ClosureBudget = 0;                // remaining placement attempts for the whole closure search

    // ---- tuning knobs (adjust freely) ----
    int ClosureMaxBacktrack = 10;           // max k: how many base-plan segments may be dropped; -1 = unlimited
    double ClosurePruneStep = 4.0;          // per-tile anchor advance assumed for distance pruning (hard upper bound)
    int ClosureBudgetLimit = 10000;         // max virtual placements per PlanClosure call
    int ClosureBaseRetries = 3;             // how many times to re-roll the base plan after a failed closure search; 0 = single base
    int ClosureSectorWidth = 1;             // candidate direction sector around the ideal one; 0 = ideal direction only
    int ClosureBeamWidth = 6;               // beam width: best states kept per layer; 1 = pure greedy

    // forced-choice hooks for the closure search. Normal mode behaves exactly
    // like the original random pick; the search drives the enumeration through
    // these so every tile choice can be tried deterministically.
    int g_ForcedIndex = -1;         // force one of the random tile choices
    int g_ForcedVariant = -1;       // force the concrete tile variant
    bool g_ForcedHonored = false;   // the forced index was actually applied
    int g_EnumCount = 0;            // candidates exposed by the last random point
    int g_EnumList[2] = { -1, -1 };
    int g_EnumPicked = -1;          // candidate the last random point returned
    bool g_ClosurePlanActive = false; // the current preview is a closure plan

    // pick from a candidate list, honouring the forced index and exposing the
    // alternatives so the closure search can enumerate all choices
    static int PickIndex(std::vector<int>& choices)
    {
        g_EnumCount = (int)choices.size();
        for (int i = 0; i < 2 && i < (int)choices.size(); ++i)
            g_EnumList[i] = choices[i];
        if (g_ForcedIndex >= 0)
        {
            for (int c : choices)
            {
                if (c == g_ForcedIndex)
                {
                    g_ForcedHonored = true;
                    g_EnumPicked = c;
                    return c;
                }
            }
        }
        int picked = STDHelpers::RandomSelectInt(choices);
        g_EnumPicked = picked;
        return picked;
    }

    // pick the concrete tile variant, honouring the forced variant
    static int PickVariant(std::vector<int>& tiles, int index)
    {
        if (g_ForcedVariant >= 0)
        {
            for (int t : tiles)
            {
                if (t == g_ForcedVariant)
                    return t;
            }
        }
        return STDHelpers::RandomSelectInt(tiles, true, index);
    }

    static void CaptureState(SegmentState& s)
    {
        s.Anchor = CViewObjectsExt::CliffConnectionCoord;
        s.Height = CViewObjectsExt::CliffConnectionHeight;
        s.LastCT = CViewObjectsExt::LastPlacedCT;
        s.ThisCT = CViewObjectsExt::ThisPlacedCT;
        s.LastCTTile = CViewObjectsExt::LastCTTile;
        s.CliffTile = CViewObjectsExt::CliffConnectionTile;
        s.PlaceStart = CViewObjectsExt::PlaceConnectedTile_Start;
        s.LastSuccessfulIndex = CViewObjectsExt::LastSuccessfulIndex;
        s.LastSuccessfulOpposite = CViewObjectsExt::LastSuccessfulOpposite;
        s.LastSuccessfulHeightOffset = CViewObjectsExt::LastSuccessfulHeightOffset;
        s.NextCTHeightOffset = CViewObjectsExt::NextCTHeightOffset;
        s.HeightChanged = CViewObjectsExt::HeightChanged;
        s.HeightAdjust = CViewObjectsExt::CliffConnectionHeightAdjust;
        s.LastTempPlacedCTIndex = CViewObjectsExt::LastTempPlacedCTIndex;
        s.LastTempFacing = CViewObjectsExt::LastTempFacing;
    }

    static void RestoreState(const SegmentState& s)
    {
        CViewObjectsExt::CliffConnectionCoord = s.Anchor;
        CViewObjectsExt::CliffConnectionHeight = s.Height;
        CViewObjectsExt::LastPlacedCT = s.LastCT;
        CViewObjectsExt::ThisPlacedCT = s.ThisCT;
        CViewObjectsExt::LastCTTile = s.LastCTTile;
        CViewObjectsExt::CliffConnectionTile = s.CliffTile;
        CViewObjectsExt::PlaceConnectedTile_Start = s.PlaceStart;
        CViewObjectsExt::LastSuccessfulIndex = s.LastSuccessfulIndex;
        CViewObjectsExt::LastSuccessfulOpposite = s.LastSuccessfulOpposite;
        CViewObjectsExt::LastSuccessfulHeightOffset = s.LastSuccessfulHeightOffset;
        CViewObjectsExt::NextCTHeightOffset = s.NextCTHeightOffset;
        CViewObjectsExt::HeightChanged = s.HeightChanged;
        CViewObjectsExt::CliffConnectionHeightAdjust = s.HeightAdjust;
        CViewObjectsExt::LastTempPlacedCTIndex = s.LastTempPlacedCTIndex;
        CViewObjectsExt::LastTempFacing = s.LastTempFacing;
    }

    static void MergeCommittedBatch(const CommittedBatch& batch)
    {
        auto& S = g_Session;
        for (auto& [pos, cd] : batch.Cells)
            S.CommittedCells[pos] = cd;
        for (auto& [pos, cd] : batch.FixCells)
            S.CommittedFixCells[pos] = cd;
        for (auto& [pos, ov] : batch.Overlays)
            S.CommittedOverlays[pos] = ov;
    }

    static void ClearCommitted()
    {
        auto& S = g_Session;
        S.CommittedBatches.clear();
        S.CommittedCells.clear();
        S.CommittedFixCells.clear();
        S.CommittedOverlays.clear();
        S.PendingManualBatch = CommittedBatch{};
        g_pCurrentManualBatch = nullptr;
    }

    static void RebuildCommitted()
    {
        auto& S = g_Session;
        S.CommittedCells.clear();
        S.CommittedFixCells.clear();
        S.CommittedOverlays.clear();
        for (auto& batch : S.CommittedBatches)
            MergeCommittedBatch(batch);
    }

    static bool PopLastCommittedBatch()
    {
        auto& S = g_Session;
        if (S.CommittedBatches.empty())
            return false;
        S.CommittedBatches.pop_back();
        RebuildCommitted();
        return true;
    }
}

// single interception point for every connected-tile cell write inside
// PlaceConnectedTile_OnMouseMove: in virtual mode the write goes to the
// current trial segment buffer, otherwise it behaves exactly like before.
// isFix marks junction patch writes (dwposFix series); they are collected
// separately so their overlap can be checked independently.
static void AutoConnect_WriteCell(std::map<int, CellData>& tmpCellDatas, CellData* cellDatas,
    int dwpos, int tileIndex, int tileSubIndex, int newHeight, bool isFix = false)
{
    int altCount = CMapDataExt::TileData[tileIndex].AltTypeCount;
    if (newHeight > 14) newHeight = 14;
    if (newHeight < 0) newHeight = 0;

    if (AutoConnect::g_VirtualPlacing && AutoConnect::g_pCurrentSegment)
    {
        CellData v = cellDatas[dwpos]; // keep untouched fields of the real cell
        v.TileIndex = tileIndex;
        v.TileSubIndex = tileSubIndex;
        v.Flag.AltIndex = STDHelpers::RandomSelectInt(0, altCount + 1);
        v.Height = newHeight;
        if (isFix)
            AutoConnect::g_pCurrentSegment->FixCells[dwpos] = v;
        else
            AutoConnect::g_pCurrentSegment->Cells[dwpos] = v;
        return;
    }

    tmpCellDatas[dwpos] = cellDatas[dwpos];
    cellDatas[dwpos].TileIndex = tileIndex;
    cellDatas[dwpos].TileSubIndex = tileSubIndex;
    cellDatas[dwpos].Flag.AltIndex = STDHelpers::RandomSelectInt(0, altCount + 1);
    cellDatas[dwpos].Height = newHeight;

    if (AutoConnect::g_pCurrentManualBatch)
    {
        if (isFix)
            AutoConnect::g_pCurrentManualBatch->FixCells[dwpos] = cellDatas[dwpos];
        else
            AutoConnect::g_pCurrentManualBatch->Cells[dwpos] = cellDatas[dwpos];
    }
}


void CViewObjectsExt::ConnectedTile_Initialize()
{
    TreeView_ConnectedTileMap.clear();
    CurrentConnectedTileType = -1;
    ConnectedTileSets.clear();
    auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");
    if (CMapDataExt::TileData)
    {
        std::string path = CFinalSunApp::Instance->ExePath();
        path += "\\ConnectedTileDrawer.ini";

        CINI ini;
        ini.ClearAndLoad(path.c_str());


        if (auto pSection = ini.GetSection("ConnectedTiles"))
        {
            for (auto& pair : pSection->GetEntities())
            {
                ConnectedTileSet cts;
                if (auto pSection2 = ini.GetSection(pair.second))
                {
                    cts.StartTile = ini.GetInteger(pair.second, "StartTile");
                    cts.Allowed = false;
                    auto allowedTheaters = STDHelpers::SplitString(ini.GetString(pair.second, "AllowedTheaters"));
                    auto transed = FinalAlertConfig::Language + "-" + "Name";
                    if (auto pName = ini.TryGetString(pair.second, transed))
                        cts.Name = *pName;
                    else
                        cts.Name = ini.GetString(pair.second, "Name");
                    cts.SetName = pair.second;
                    auto type = ini.GetString(pair.second, "Type");
                    auto sptype = ini.GetString(pair.second, "SpecialType");
                    cts.WaterCliff = ini.GetBool(pair.second, "WaterCliff");
                    if (type == "Cliff")
                        cts.Type = ConnectedTileSetTypes::Cliff;
                    else if (type == "CityCliff")
                        cts.Type = ConnectedTileSetTypes::CityCliff;
                    else if (type == "IceCliff")
                        cts.Type = ConnectedTileSetTypes::IceCliff;
                    else if (type == "DirtRoad")
                        cts.Type = ConnectedTileSetTypes::DirtRoad;
                    else if (type == "Highway")
                        cts.Type = ConnectedTileSetTypes::Highway;
                    else if (type == "Shore")
                        cts.Type = ConnectedTileSetTypes::Shore;
                    else if (type == "PaveShore")
                        cts.Type = ConnectedTileSetTypes::PaveShore;
                    else if (type == "SpecialPaveShore")
                        cts.Type = ConnectedTileSetTypes::SpecialPaveShore;
                    else if (type == "CityDirtRoad")
                        cts.Type = ConnectedTileSetTypes::CityDirtRoad;
                    else if (type == "RailRoad")
                        cts.Type = ConnectedTileSetTypes::RailRoad;

                    if (sptype == "SnowSnow")
                        cts.SpecialType = ConnectedTileSetSpecialTypes::SnowSnow;
                    else if (sptype == "SnowStone")
                        cts.SpecialType = ConnectedTileSetSpecialTypes::SnowStone;
                    else if (sptype == "StoneStone")
                        cts.SpecialType = ConnectedTileSetSpecialTypes::StoneStone;
                    else if (sptype == "StoneSnow")
                        cts.SpecialType = ConnectedTileSetSpecialTypes::StoneSnow;
                    else if (sptype == "SnowWater")
                        cts.SpecialType = ConnectedTileSetSpecialTypes::SnowWater;
                    else if (sptype == "StoneWater")
                        cts.SpecialType = ConnectedTileSetSpecialTypes::StoneWater;
                    else
                        cts.SpecialType = -1;

                    cts.IsTXCityCliff = false;

                    switch (cts.Type)
                    {
                    case ConnectedTileSetTypes::Highway:
                        cts.AutoWeaveAmplitude = 2.5;
                        cts.AutoSingleVariantPenalty = false;
                        cts.AutoBacktrackSteps = 5;
                        break;
                    case ConnectedTileSetTypes::PaveShore:
                    case ConnectedTileSetTypes::SpecialPaveShore:
                    case ConnectedTileSetTypes::RailRoad:
                        cts.AutoWeaveAmplitude = 1.5;
                        cts.AutoSingleVariantPenalty = false;
                        cts.AutoBacktrackSteps = 5;
                        break;
                    default:
                        cts.AutoWeaveAmplitude = 3.0;
                        cts.AutoSingleVariantPenalty = true;
                        cts.AutoBacktrackSteps = 7;
                        break;
                    }
                    if (auto pValue = ini.TryGetString(pair.second, "AutoWeaveAmplitude"))
                        cts.AutoWeaveAmplitude = std::max(0.0, atof(*pValue));
                    if (auto pValue = ini.TryGetString(pair.second, "AutoSingleVariantPenalty"))
                        cts.AutoSingleVariantPenalty = *pValue == "yes"
                            || *pValue == "true" || *pValue == "1";
                    if (auto pValue = ini.TryGetString(pair.second, "AutoBacktrackSteps"))
                        cts.AutoBacktrackSteps = std::clamp(atoi(*pValue), 0, 10);

                    for (auto& t : allowedTheaters)
                    {
                        std::string t1 = (std::string)t;
                        std::transform(t1.begin(), t1.end(), t1.begin(), [](unsigned char c) {return std::tolower(c); });
                        std::string t2 = (std::string)thisTheater;
                        std::transform(t2.begin(), t2.end(), t2.begin(), [](unsigned char c) {return std::tolower(c); });
                        if (t1 == t2)
                            cts.Allowed = true;
                    }

                    for (int i = 0; i < 100; i++)
                    {
                        FString buffer;
                        buffer.Format("%s.%d", pair.second, i);
                        if (ini.SectionExists(buffer))
                        {
                            ConnectedTiles ct;

                            auto cp0 = STDHelpers::SplitString(ini.GetString(buffer, "ConnectionPoint0"), 1);
                            ct.ConnectionPoint0.X = atoi(cp0[1]);
                            ct.ConnectionPoint0.Y = atoi(cp0[0]);
                            auto cp1 = STDHelpers::SplitString(ini.GetString(buffer, "ConnectionPoint1"), 1);
                            ct.ConnectionPoint1.X = atoi(cp1[1]);
                            ct.ConnectionPoint1.Y = atoi(cp1[0]);

                            ct.Direction0 = ini.GetInteger(buffer, "ConnectionPoint0.Direction");
                            ct.Direction1 = ini.GetInteger(buffer, "ConnectionPoint1.Direction");

                            ct.Side0 = ini.GetString(buffer, "ConnectionPoint0.Side") == "Back";
                            ct.Side1 = ini.GetString(buffer, "ConnectionPoint1.Side") == "Back";

                            ct.Index = i;

                            ct.AdditionalOffset = ini.GetInteger(buffer, "AdditionalOffset");
                            ct.HeightAdjust = ini.GetInteger(buffer, "HeightAdjust");

                            auto tileIndices = STDHelpers::SplitString(ini.GetString(buffer, "TileIndices"));
                            for (auto& ti : tileIndices)
                            {
                                ct.TileIndices.push_back(atoi(ti) + ct.AdditionalOffset);
                            }

                            cts.ConnectedTile.push_back(ct);
                        }
                        else
                            break;
                    }

                }
                CViewObjectsExt::ConnectedTileSets.push_back(cts);
            }

            for (auto& cts : CViewObjectsExt::ConnectedTileSets)
            {
                FString key;
                for (int i = 0; i < 10; ++i)
                {
                    cts.ToSetPress[i] = -1;
                    key.Format("ToSet.Press%d", i);
                    if (auto pVaule = ini.TryGetString(cts.SetName, key))
                    {
                        int j = 0;
                        for (auto& cts2 : CViewObjectsExt::ConnectedTileSets)
                        {
                            if (cts2.SetName == *pVaule)
                            {
                                cts.ToSetPress[i] = j;
                                break;
                            }
                            j++;
                        }
                    }
                }
            }
        }
    }
}

void CViewObjectsExt::Redraw_ConnectedTile(CViewObjectsExt* pThis)
{
    int index = 0;
    HTREEITEM& hCT = ExtNodes[Root_Cliff];
    if (hCT == NULL)    return;

    HTREEITEM hCliff = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_Cliff", -1, hCT);
    HTREEITEM hCliffLand = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_CliffLand", -1, hCliff);
    HTREEITEM hCliffWater = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_CliffWater", -1, hCliff);

    HTREEITEM hCliffLandSnowSnow = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_CliffTXSnowSnow", -1, hCliffLand);
    HTREEITEM hCliffLandSnowStone = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_CliffTXSnowStone", -1, hCliffLand);
    HTREEITEM hCliffLandStoneSnow = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_CliffTXStoneSnow", -1, hCliffLand);
    HTREEITEM hCliffLandStoneStone = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_CliffTXStoneStone", -1, hCliffLand);
    HTREEITEM hCliffSnowWater = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_CliffTXSnowWater", -1, hCliffWater);
    HTREEITEM hCliffStoneWater = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_CliffTXStoneWater", -1, hCliffWater);

    HTREEITEM hDirtRoad = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_Road", -1, hCT);
    HTREEITEM hShore = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_Shore", -1, hCT);
    HTREEITEM hHighway = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_PavedRoad", -1, hCT);
    HTREEITEM hSpecial = pThis == nullptr ? NULL : pThis->InsertTranslatedString("CT_Special", -1, hCT);

    HTREEITEM hRailroad = pThis == nullptr ? NULL : pThis->InsertTranslatedString("Tracks", -1, hCT);

    std::vector<HTREEITEM> subNodes;
    subNodes.push_back(hCliffLandSnowSnow);
    subNodes.push_back(hCliffLandSnowStone);
    subNodes.push_back(hCliffLandStoneSnow);
    subNodes.push_back(hCliffLandStoneStone);
    subNodes.push_back(hCliffSnowWater);
    subNodes.push_back(hCliffStoneWater);
    subNodes.push_back(hCliffLand);
    subNodes.push_back(hCliffWater);
    subNodes.push_back(hDirtRoad);
    subNodes.push_back(hShore);
    subNodes.push_back(hHighway);
    subNodes.push_back(hSpecial);
    subNodes.push_back(hCliff);
    subNodes.push_back(hRailroad);

    auto thisTheater = CINI::CurrentDocument().GetString("Map", "Theater");

    int i = -1;
    for (auto& ct : ConnectedTileSets)
    {
        i++;
        if (ct.Allowed)
        {
            if (ct.ConnectedTile.empty()) continue;
            int firstTileIndex = ct.ConnectedTile.front().TileIndices[0] + ct.StartTile;
            int lastTileIndex = ct.ConnectedTile.back().TileIndices[0] + ct.StartTile;

            if (ct.Type == ConnectedTileSetTypes::CityCliff)
            {
                // 29 & 30 are special diagonals in TX
                if (ct.ConnectedTile.size() > 30)
                {
                    int lastTileIndexTX = ct.ConnectedTile[30].TileIndices[0] + ct.StartTile;
                    if (lastTileIndexTX < CMapDataExt::TileDataCount)
                    {
                        int lastTilesetTX = CMapDataExt::TileData[lastTileIndexTX].TileSet;
                        FString buffer;
                        buffer.Format("TileSet%04d", lastTilesetTX);

                        auto exist = CINI::CurrentTheater->GetBool(buffer, "AllowToPlace", true);
                        auto exist2 = CINI::CurrentTheater->GetString(buffer, "FileName", "");
                        if (exist && strcmp(exist2, "") != 0)
                        {
                            ct.IsTXCityCliff = true;
                        }
                    }
                }
                if (!ct.IsTXCityCliff && ct.ConnectedTile.size() > 28)
                    lastTileIndex = ct.ConnectedTile[28].TileIndices[0] + ct.StartTile;
            }

            if (ct.Type != ConnectedTileSetTypes::RailRoad)
            {
                if (firstTileIndex > CMapDataExt::TileDataCount || lastTileIndex > CMapDataExt::TileDataCount)
                    continue;

                int firstTileset = CMapDataExt::TileData[firstTileIndex].TileSet;
                int lastTileset = CMapDataExt::TileData[lastTileIndex].TileSet;
                if (!CMapDataExt::IsValidTileSet(firstTileset) || !CMapDataExt::IsValidTileSet(lastTileset))
                    continue;
            }

            ConnectedTileInfo info{};
            info.Index = i;
            info.Front = true;
            ConnectedTileInfo info2{};
            info2.Index = i;
            info2.Front = false;

            switch (ct.Type)
            {
            case ConnectedTileSetTypes::Cliff:
            case ConnectedTileSetTypes::CityCliff:
            case ConnectedTileSetTypes::IceCliff:
                if (ct.WaterCliff)
                {
                    if (ct.SpecialType < 0)
                    {
                        TreeView_ConnectedTileMap[index] = info;
                        if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hCliffWater);
                        index++;
                        TreeView_ConnectedTileMap[index] = info2;
                        if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hCliffWater);
                        index++;
                    }
                    else
                    {

                        if (ct.SpecialType == SnowWater)
                        {
                            TreeView_ConnectedTileMap[index] = info;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hCliffSnowWater);
                            index++;
                            TreeView_ConnectedTileMap[index] = info2;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hCliffSnowWater);
                            index++;
                        }
                        else if (ct.SpecialType == StoneWater)
                        {
                            TreeView_ConnectedTileMap[index] = info;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hCliffStoneWater);
                            index++;
                            TreeView_ConnectedTileMap[index] = info2;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hCliffStoneWater);
                            index++;
                        }

                    }
                }
                else
                {
                    if (ct.SpecialType < 0)
                    {
                        TreeView_ConnectedTileMap[index] = info;
                        if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hCliffLand);
                        index++;
                        TreeView_ConnectedTileMap[index] = info2;
                        if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hCliffLand);
                        index++;
                    }
                    else
                    {
                        if (ct.SpecialType == SnowSnow)
                        {
                            TreeView_ConnectedTileMap[index] = info;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hCliffLandSnowSnow);
                            index++;
                            TreeView_ConnectedTileMap[index] = info2;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hCliffLandSnowSnow);
                            index++;
                        }
                        else if (ct.SpecialType == SnowStone)
                        {
                            TreeView_ConnectedTileMap[index] = info;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hCliffLandSnowStone);
                            index++;
                            TreeView_ConnectedTileMap[index] = info2;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hCliffLandSnowStone);
                            index++;
                        }
                        else if (ct.SpecialType == StoneSnow)
                        {
                            TreeView_ConnectedTileMap[index] = info;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hCliffLandStoneSnow);
                            index++;
                            TreeView_ConnectedTileMap[index] = info2;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hCliffLandStoneSnow);
                            index++;
                        }
                        else if (ct.SpecialType == StoneStone)
                        {
                            TreeView_ConnectedTileMap[index] = info;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hCliffLandStoneStone);
                            index++;
                            TreeView_ConnectedTileMap[index] = info2;
                            if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hCliffLandStoneStone);
                            index++;
                        }

                    }

                }
                break;
            case ConnectedTileSetTypes::DirtRoad:
            case ConnectedTileSetTypes::CityDirtRoad:
                TreeView_ConnectedTileMap[index] = info;
                if (pThis) pThis->InsertString(ct.Name, Const_ConnectedTile + index, hDirtRoad);
                index++;
                break;
            case ConnectedTileSetTypes::Highway:
                TreeView_ConnectedTileMap[index] = info;
                if (pThis) pThis->InsertString(ct.Name, Const_ConnectedTile + index, hHighway);
                index++;
                break;
            case ConnectedTileSetTypes::RailRoad:
                TreeView_ConnectedTileMap[index] = info;
                if (pThis) pThis->InsertString(ct.Name, Const_ConnectedTile + index, hRailroad);
                index++;
                break;
            case ConnectedTileSetTypes::Shore:
            case ConnectedTileSetTypes::PaveShore:
                TreeView_ConnectedTileMap[index] = info;
                if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hShore);
                index++;
                TreeView_ConnectedTileMap[index] = info2;
                if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hShore);
                index++;
                break;
            case ConnectedTileSetTypes::SpecialPaveShore:
                TreeView_ConnectedTileMap[index] = info;
                if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Front", " (Front)"), Const_ConnectedTile + index, hSpecial);
                index++;
                TreeView_ConnectedTileMap[index] = info2;
                if (pThis) pThis->InsertString(ct.Name + Translations::TranslateOrDefault("ConnectedTile.Back", " (Back)"), Const_ConnectedTile + index, hSpecial);
                index++;
                break;
            default:
                break;
            }
        }
    }

    if (pThis)
    {
        for (auto& subnode : subNodes)
        {
            if (!pThis->GetTreeCtrl().ItemHasChildren(subnode))
                pThis->GetTreeCtrl().DeleteItem(subnode);
        }
    }
}

void CViewObjectsExt::PlaceConnectedTile_OnMouseMove(int X, int Y, bool place)
{
    if (!CMapDataExt::IsCoordInFullMap(X, Y))
        return;
    CViewObjectsExt::IsInPlaceCliff_OnMouseMove = true;

    auto handleExit = []()
    {
        CViewObjectsExt::NextCTHeightOffset = 0;
        CViewObjectsExt::IsInPlaceCliff_OnMouseMove = false;
    };

    if (CViewObjectsExt::CliffConnectionCoord.X == -1 && CViewObjectsExt::CliffConnectionCoord.Y == -1)
    {
        handleExit();
        return;
    }

    if (CViewObjectsExt::CliffConnectionHeight < 0)
        CViewObjectsExt::CliffConnectionHeight = 0;
    if (CViewObjectsExt::CliffConnectionHeight > 14)
        CViewObjectsExt::CliffConnectionHeight = 14;

    auto& mapData = CMapData::Instance();
    auto cellDatas = mapData.CellDatas;
    std::vector<int> cliffConnectionTiles;
    bool forceFront = false;
    bool forceBack = false;

    ConnectedTileSet tileSet;
    bool UrbanCliff = false;
    bool IceCliff = false;
    bool cityRoad = false;
    bool railRoad = false;

    int subPos = 0;

    int dwposFix = -1;
    int dwposFix2 = -1;
    std::map <int, CellData> tmpCellDatas;
    auto thisTile = CMapDataExt::TileData[0];

    MapCoord cursor;
    cursor.X = X; cursor.Y = Y;
    int facing = CMapDataExt::GetFacing(CViewObjectsExt::CliffConnectionCoord, cursor);

    int xx = CViewObjectsExt::CliffConnectionCoord.X - cursor.X;
    int yy = CViewObjectsExt::CliffConnectionCoord.Y - cursor.Y;
    double distance = sqrt(xx * xx + yy * yy);

    int distanceX = abs(xx);
    int distanceY = abs(yy);

    int SmallDistance = 5;
    int LargeDistance = 9;

    int offsetConnectX = 0;
    int offsetConnectY = 0;
    int offsetPlaceX = 0;
    int offsetPlaceY = 0;

    int MultiPlaceDirection = -1;
    bool thisTileHeightOffest = false;
    bool opposite = false;

    auto getOppositeDirection = [](int dir)
    {
        if (dir > 7 || dir < 0)
            return 0;
        if (dir == 0)
            return 4;
        if (dir == 1)
            return 5;
        if (dir == 2)
            return 6;
        if (dir == 3)
            return 7;
        if (dir == 4)
            return 0;
        if (dir == 5)
            return 1;
        if (dir == 6)
            return 2;
        if (dir == 7)
            return 3;
        return 0;
    };

    if (CIsoView::CurrentCommand->Type >= 9000)
    {
        handleExit();
        return;
    }

    int ctIndex = CIsoView::CurrentCommand->Type;
    CurrentConnectedTileType = TreeView_ConnectedTileMap[ctIndex].Index;
    tileSet = CViewObjectsExt::ConnectedTileSets[CurrentConnectedTileType];

    switch (tileSet.Type)
    {
    case ConnectedTileSetTypes::Cliff:
        if (TreeView_ConnectedTileMap[ctIndex].Front && CViewObjectsExt::LastPlacedCT.Index == -1)
        {
            forceFront = true;
        }
        else if (!TreeView_ConnectedTileMap[ctIndex].Front && CViewObjectsExt::LastPlacedCT.Index == -1)
        {
            forceBack = true;
        }
        break;
    case ConnectedTileSetTypes::CityCliff:
        if (TreeView_ConnectedTileMap[ctIndex].Front && CViewObjectsExt::LastPlacedCT.Index == -1)
        {
            forceFront = true;
        }
        else if (!TreeView_ConnectedTileMap[ctIndex].Front && CViewObjectsExt::LastPlacedCT.Index == -1)
        {
            forceBack = true;
        }
        UrbanCliff = true;
        break;
    case ConnectedTileSetTypes::IceCliff:
        if (TreeView_ConnectedTileMap[ctIndex].Front && CViewObjectsExt::LastPlacedCT.Index == -1)
        {
            forceFront = true;
        }
        else if (!TreeView_ConnectedTileMap[ctIndex].Front && CViewObjectsExt::LastPlacedCT.Index == -1)
        {
            forceBack = true;
        }
        IceCliff = true;
        break;
    case ConnectedTileSetTypes::DirtRoad:
        break;
    case ConnectedTileSetTypes::CityDirtRoad:
        cityRoad = true;
        break;
    case ConnectedTileSetTypes::Highway:
        break;
    case ConnectedTileSetTypes::RailRoad:
        railRoad = true;
        break;
    case ConnectedTileSetTypes::Shore:
        if (TreeView_ConnectedTileMap[ctIndex].Front)
        {
            forceFront = true;
        }
        else
        {
            forceBack = true;
        }
        break;
    case ConnectedTileSetTypes::PaveShore:
    case ConnectedTileSetTypes::SpecialPaveShore:
        if (TreeView_ConnectedTileMap[ctIndex].Front)
        {
            forceFront = true;
        }
        else
        {
            forceBack = true;
        }
        break;
    default:
        break;
    }

    //very stupid code
    if (tileSet.Type == ConnectedTileSetTypes::Cliff || tileSet.Type == ConnectedTileSetTypes::CityCliff || tileSet.Type == ConnectedTileSetTypes::IceCliff)
    {
        if (!tileSet.Name)
        {
            handleExit();
            return;
        }

        int index = -1;

        if (NULL == CMapDataExt::TileData)
        {
            handleExit();
            return;
        }

        if (place && !AutoConnect::g_VirtualPlacing)
        {
            mapData.SaveUndoRedoData(true, CViewObjectsExt::CliffConnectionCoord.X - 4, CViewObjectsExt::CliffConnectionCoord.Y - 4,
                CViewObjectsExt::CliffConnectionCoord.X + 4, CViewObjectsExt::CliffConnectionCoord.Y + 4);
        }

        //        7 
        //     6     0
        //  5           1
        //     4     2
        //        3

        int loop = 0;
        while (cliffConnectionTiles.empty())
        {
            loop++;
            if (facing == 1)
            {
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22)
                {
                    facing = 3;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    facing = 3;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18)
                {
                    facing = 0;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17)
                {
                    facing = 7;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30)
                {
                    facing = 2;
                    continue;
                }
                if (!forceFront && (forceBack
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Index == 16
                    || CViewObjectsExt::LastPlacedCT.Index == 17
                    || CViewObjectsExt::LastPlacedCT.Index == 18
                    || CViewObjectsExt::LastPlacedCT.Index == 21
                    || CViewObjectsExt::LastPlacedCT.Index == 28
                    || CViewObjectsExt::LastPlacedCT.Index == 22
                    || CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1))
                {
                    if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                    {
                        offsetPlaceY -= 1;
                    }

                    std::vector<int> backCornet1;
                    std::vector<int> backCornet2;
                    backCornet1.push_back(14);
                    backCornet1.push_back(15);
                    backCornet2.push_back(17);
                    backCornet2.push_back(18);

                    offsetPlaceY += 1;
                    offsetConnectX -= 1;

                    index = AutoConnect::PickIndex(backCornet1);
                    if (distance < SmallDistance)
                        index = 16;

                    if (!UrbanCliff)
                    {
                        if (CViewObjectsExt::LastPlacedCT.Index == 12
                            || CViewObjectsExt::LastPlacedCT.Index == 13
                            || CViewObjectsExt::LastPlacedCT.Index == 27
                            || CViewObjectsExt::LastPlacedCT.Index == 14
                            || CViewObjectsExt::LastPlacedCT.Index == 25
                            || CViewObjectsExt::LastPlacedCT.Index == 15
                            || CViewObjectsExt::LastPlacedCT.Index == 16)
                        {
                            if (distance > LargeDistance)
                            {
                                if (place && !AutoConnect::g_VirtualPlacing)
                                    index = CViewObjectsExt::ThisPlacedCT.Index;
                                else
                                    index = AutoConnect::PickIndex(backCornet2);
                            }
                        }

                    }
                    else if (UrbanCliff && index != 16)
                    {
                        index = 25;
                        if (tileSet.ConnectedTile[index].TileIndices[0] < 0)
                        {
                            index = AutoConnect::PickIndex(backCornet1);
                        }
                    }

                    if (CViewObjectsExt::LastPlacedCT.Index == 7
                        || CViewObjectsExt::LastPlacedCT.Index == 8
                        || CViewObjectsExt::LastPlacedCT.Index == 9
                        || CViewObjectsExt::LastPlacedCT.Index == 10
                        || CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Index == 30
                        || CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        index = 18;
                        offsetPlaceX -= 1;
                    }


                    if (index == 17 || index == 18)
                    {
                        offsetPlaceX += 1;
                        offsetConnectY -= 1;
                    }

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else
                {

                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        facing = 7;
                        continue;
                    }

                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9)
                    {
                        facing = 2;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6)
                    {
                        facing = 2;
                        continue;
                    }

                    if (CViewObjectsExt::LastPlacedCT.Index == 7
                        || CViewObjectsExt::LastPlacedCT.Index == 8
                        || CViewObjectsExt::LastPlacedCT.Index == 9
                        || CViewObjectsExt::LastPlacedCT.Index == 10
                        || CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Index == 11)
                        offsetPlaceY += 1;
                    if (CViewObjectsExt::LastPlacedCT.Index == 12
                        || CViewObjectsExt::LastPlacedCT.Index == 27
                        || CViewObjectsExt::LastPlacedCT.Index == 13)
                        offsetPlaceX -= 1;

                    offsetConnectY += 1;
                    offsetConnectX -= 1;
                    index = 5;
                    if (distance < SmallDistance)
                        index = 6;
                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
            }
            else if (facing == 2)
            {
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22)
                {
                    facing = 1;
                    continue;
                }
                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18)
                {
                    facing = 0;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22)
                {
                    facing = 3;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    facing = 3;
                    continue;
                }
                if ((CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18) && !tileSet.IsTXCityCliff)
                {
                    facing = 1;
                    continue;
                }
                if (!forceFront && (forceBack || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16))
                {
                    if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                    {
                        offsetPlaceX -= 1;
                        offsetPlaceY -= 1;
                    }

                    offsetPlaceX += 1;
                    offsetPlaceY += 1;
                    offsetConnectX -= 1;

                    if (CViewObjectsExt::LastPlacedCT.Index == 5
                        || CViewObjectsExt::LastPlacedCT.Index == 6
                        || CViewObjectsExt::LastPlacedCT.Index == 7
                        || CViewObjectsExt::LastPlacedCT.Index == 8
                        || CViewObjectsExt::LastPlacedCT.Index == 9
                        || CViewObjectsExt::LastPlacedCT.Index == 10
                        || CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Index == 30
                        || CViewObjectsExt::LastPlacedCT.Index == 11)
                        offsetPlaceX -= 1;

                    index = 12;
                    if (distance < SmallDistance)
                        index = 13;

                    if (distance > LargeDistance && UrbanCliff)
                    {
                        index = 27;
                        offsetConnectY += 1;
                        if (tileSet.ConnectedTile[index].TileIndices[0] < 0)
                        {
                            index = 12;
                            offsetConnectY -= 1;
                        }
                    }
                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else
                {
                    bool txcliff = false;
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27)
                    {
                        if (!tileSet.IsTXCityCliff)
                        {
                            facing = 1;
                            continue;
                        }
                        else
                        {
                            txcliff = true;
                        }
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18)
                    {
                        facing = 0;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1)
                    {
                        facing = 1;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        facing = 7;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        if (!tileSet.IsTXCityCliff)
                        {
                            facing = 1;
                            continue;
                        }
                        else
                        {
                            txcliff = true;
                        }
                    }

                    offsetConnectY += 1;
                    index = 3;
                    if (distance < SmallDistance)
                        index = 4;

                    if ((CViewObjectsExt::LastPlacedCT.Index == 5 || CViewObjectsExt::LastPlacedCT.Index == 6) && distance > LargeDistance && tileSet.IsTXCityCliff)
                    {
                        offsetPlaceY -= 1;
                        index = 30;
                    }

                    if (txcliff)
                    {
                        index = 30;
                    }

                    if (index == 30 && (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18))
                    {
                        offsetPlaceX -= 1;
                        offsetPlaceY -= 1;
                    }

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
            }
            else if (facing == 3)
            {
                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22)
                {
                    facing = 1;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18)
                {
                    facing = 1;
                    continue;
                }
                if (!forceFront && (forceBack || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11))
                {
                    if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                    {
                        offsetPlaceY -= 1;
                    }
                    offsetPlaceY += 1;
                    offsetConnectX += 1;
                    index = 10;
                    if (distance < SmallDistance)
                        index = 11;
                    if (distance > LargeDistance)
                        index = 9;

                    if (index == 9)
                    {
                        offsetPlaceY -= 1;
                        if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                        {
                            offsetPlaceY += 1;
                        }
                    }

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    offsetPlaceY += 1;
                    offsetPlaceX += 1;
                    offsetConnectX += 1;

                    index = 10;
                    if (distance < SmallDistance)
                        index = 11;

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                    {
                        offsetPlaceY -= 1;
                    }
                    offsetConnectY += 1;
                    index = 0;
                    if (distance < SmallDistance)
                        index = 1;

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else
                {
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1)
                    {
                        facing = 1;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        facing = 5;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        facing = 1;
                        continue;
                    }

                    if (CViewObjectsExt::LastPlacedCT.Index == 5
                        || CViewObjectsExt::LastPlacedCT.Index == 6
                        || CViewObjectsExt::LastPlacedCT.Index == 3
                        || CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Index == 30
                        || CViewObjectsExt::LastPlacedCT.Index == 4
                        || CViewObjectsExt::LastPlacedCT.Index == 0
                        || CViewObjectsExt::LastPlacedCT.Index == 1
                        || CViewObjectsExt::LastPlacedCT.Index == 2)
                        offsetPlaceX += 1;
                    offsetConnectY += 1;
                    index = 0;
                    if (distance < SmallDistance)
                        index = 1;
                    if (distance > LargeDistance)
                        index = 2;

                    if (index == 2)
                    {
                        offsetPlaceX -= 1;
                        if (forceFront && CViewObjectsExt::PlaceConnectedTile_Start)
                        {
                            offsetPlaceX += 1;
                        }
                    }

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }

            }
            else if (facing == 4)
            {
                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18)
                {
                    facing = 6;
                    continue;
                }
                if ((CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22) && !tileSet.IsTXCityCliff)
                {
                    facing = 5;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18)
                {
                    facing = 5;
                    continue;
                }

                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    facing = 3;
                    continue;
                }

                if (!forceFront && (forceBack || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Index == 16))
                {
                    if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                    {
                        offsetPlaceX -= 1;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Index != 21
                        && CViewObjectsExt::LastPlacedCT.Index != 22
                        && CViewObjectsExt::LastPlacedCT.Index != 28
                        && CViewObjectsExt::LastPlacedCT.Index != 14
                        && CViewObjectsExt::LastPlacedCT.Index != 15
                        && CViewObjectsExt::LastPlacedCT.Index != 25
                        && CViewObjectsExt::LastPlacedCT.Index != 16)
                        offsetPlaceX += 1;
                    offsetConnectX += 1;

                    index = 21;
                    if (distance < SmallDistance)
                        index = 22;

                    if (distance > LargeDistance && UrbanCliff)
                    {
                        index = 28;
                        if (tileSet.ConnectedTile[index].TileIndices[0] < 0)
                        {
                            index = 21;
                        }
                    }

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else
                {
                    bool txcliff = false;
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                    {
                        facing = 3;
                        continue;
                    }
                    if ((CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22))
                    {
                        if (!tileSet.IsTXCityCliff)
                        {
                            facing = 1;
                            continue;
                        }
                        else
                        {
                            txcliff = true;
                        }
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        if (!tileSet.IsTXCityCliff)
                        {
                            facing = 5;
                            continue;
                        }
                        else
                        {
                            txcliff = true;
                        }
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9)
                    {
                        facing = 1;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        facing = 5;
                        continue;
                    }
                    offsetConnectX += 1;
                    index = 7;
                    if (distance < SmallDistance)
                        index = 8;

                    if ((CViewObjectsExt::LastPlacedCT.Index == 5 || CViewObjectsExt::LastPlacedCT.Index == 6) && distance > LargeDistance && tileSet.IsTXCityCliff)
                    {
                        offsetPlaceX -= 1;
                        opposite = true;
                        index = 30;
                    }

                    if (txcliff)
                    {
                        opposite = true;
                        index = 30;
                    }

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
            }
            else if (facing == 5)
            {
                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18)
                {
                    facing = 6;
                    continue;
                }
                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    facing = 3;
                    continue;
                }

                if (!forceFront && (forceBack || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4
                    || CViewObjectsExt::LastPlacedCT.Index == 12
                    || CViewObjectsExt::LastPlacedCT.Index == 13
                    || CViewObjectsExt::LastPlacedCT.Index == 27
                    || CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Index == 16
                    || CViewObjectsExt::LastPlacedCT.Index == 17
                    || CViewObjectsExt::LastPlacedCT.Index == 18
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22
                    || CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11))
                {
                    if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                    {
                        offsetPlaceY += 1;
                    }
                    opposite = true;

                    std::vector<int> backCornet1;
                    std::vector<int> backCornet2;
                    backCornet1.push_back(14);
                    backCornet1.push_back(15);
                    backCornet2.push_back(17);
                    backCornet2.push_back(18);

                    offsetPlaceY -= 1;
                    offsetConnectX += 1;

                    index = AutoConnect::PickIndex(backCornet1);
                    if (distance < SmallDistance)
                        index = 16;
                    if (!UrbanCliff)
                    {
                        if (CViewObjectsExt::LastPlacedCT.Index == 14
                            || CViewObjectsExt::LastPlacedCT.Index == 15
                            || CViewObjectsExt::LastPlacedCT.Index == 25
                            || CViewObjectsExt::LastPlacedCT.Index == 16
                            || CViewObjectsExt::LastPlacedCT.Index == 21
                            || CViewObjectsExt::LastPlacedCT.Index == 28
                            || CViewObjectsExt::LastPlacedCT.Index == 22)
                        {
                            if (distance > LargeDistance)
                            {
                                if (place && !AutoConnect::g_VirtualPlacing)
                                    index = CViewObjectsExt::ThisPlacedCT.Index;
                                else
                                    index = AutoConnect::PickIndex(backCornet2);
                            }

                        }

                    }
                    else if (UrbanCliff && index != 16)
                    {
                        index = 25;
                        if (tileSet.ConnectedTile[index].TileIndices[0] < 0)
                        {
                            index = AutoConnect::PickIndex(backCornet1);
                        }
                    }

                    if (CViewObjectsExt::LastPlacedCT.Index == 0
                        || CViewObjectsExt::LastPlacedCT.Index == 1
                        || CViewObjectsExt::LastPlacedCT.Index == 2
                        || CViewObjectsExt::LastPlacedCT.Index == 3
                        || CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        index = 18;
                        offsetPlaceX += 1;
                    }

                    if (CViewObjectsExt::LastPlacedCT.Index == 10
                        || CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        offsetPlaceX += 1;
                        offsetPlaceY += 1;
                    }

                    if (index == 17 || index == 18)
                    {
                        offsetPlaceY += 1;
                        offsetConnectX -= 1;
                    }

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else
                {
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        facing = 4;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9)
                    {
                        facing = 7;
                        continue;
                    }
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6)
                    {
                        facing = 7;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Index == 0
                        || CViewObjectsExt::LastPlacedCT.Index == 1
                        || CViewObjectsExt::LastPlacedCT.Index == 2
                        || CViewObjectsExt::LastPlacedCT.Index == 3
                        || CViewObjectsExt::LastPlacedCT.Index == 4
                        || CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Index == 21
                        || CViewObjectsExt::LastPlacedCT.Index == 28
                        || CViewObjectsExt::LastPlacedCT.Index == 22)
                        offsetPlaceX += 1;
                    opposite = true;
                    offsetConnectY -= 1;
                    offsetConnectX += 1;
                    index = 5;
                    if (distance < SmallDistance)
                        index = 6;
                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
            }
            else if (facing == 6)
            {
                bool txcliff = false;
                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17)
                {
                    facing = 0;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9)
                {
                    facing = 7;
                    continue;
                }

                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9)
                {
                    if (!tileSet.IsTXCityCliff)
                    {
                        facing = 5;
                        continue;
                    }
                    else
                    {
                        txcliff = true;
                    }

                }

                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    facing = 3;
                    continue;
                }

                if (!forceFront && (forceBack || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11
                    || CViewObjectsExt::LastPlacedCT.Index == 12
                    || CViewObjectsExt::LastPlacedCT.Index == 13
                    || CViewObjectsExt::LastPlacedCT.Index == 27
                    || CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Index == 16
                    || CViewObjectsExt::LastPlacedCT.Index == 17
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22))
                {
                    if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                    {
                        offsetPlaceY += 1;
                    }
                    opposite = true;

                    if (CViewObjectsExt::LastPlacedCT.Index == 10
                        || CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        offsetPlaceX += 1;
                    }
                    else
                        offsetPlaceY -= 1;

                    index = 12;
                    if (distance < SmallDistance)
                        index = 13;
                    if (distance > LargeDistance && UrbanCliff && CViewObjectsExt::LastPlacedCT.Index != 28)
                    {
                        index = 27;
                        offsetPlaceY -= 1;
                        if (tileSet.ConnectedTile[index].TileIndices[0] < 0)
                        {
                            index = 12;
                            offsetPlaceY += 1;
                        }
                    }
                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else
                {
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        if (!tileSet.IsTXCityCliff)
                        {
                            facing = 5;
                            continue;
                        }
                        else
                        {
                            txcliff = true;
                        }
                    }
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        facing = 4;
                        continue;
                    }
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6)
                    {
                        facing = 7;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Index == 5
                        || CViewObjectsExt::LastPlacedCT.Index == 6)
                        offsetPlaceX -= 1;
                    opposite = true;
                    offsetConnectY -= 1;
                    index = 3;
                    if (distance < SmallDistance)
                        index = 4;
                    if ((CViewObjectsExt::LastPlacedCT.Index == 5 || CViewObjectsExt::LastPlacedCT.Index == 6) && distance > LargeDistance && tileSet.IsTXCityCliff)
                    {
                        offsetPlaceX += 1;
                        index = 29;
                    }

                    if (txcliff)
                    {
                        index = 29;
                    }
                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
            }
            else if (facing == 7)
            {
                if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17)
                {
                    facing = 0;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22)
                {
                    facing = 6;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    facing = 6;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17)
                {
                    facing = 6;
                    continue;
                }
                if (!forceFront && (forceBack || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18))
                {
                    if (CViewObjectsExt::LastPlacedCT.Index == 5
                        || CViewObjectsExt::LastPlacedCT.Index == 6)
                        offsetPlaceY -= 1;
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27)
                    {
                        offsetPlaceX -= 1;
                        offsetPlaceY -= 1;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Index == 18)
                    {
                        offsetPlaceX -= 1;
                        offsetPlaceY -= 1;
                    }

                    opposite = true;
                    offsetConnectX -= 1;
                    offsetConnectY -= 1;
                    index = 10;
                    if (distance < SmallDistance)
                        index = 11;
                    if (distance > LargeDistance)
                        index = 9;

                    if (index == 9)
                        offsetConnectY += 1;

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else
                {
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                    {
                        facing = 0;
                        continue;
                    }

                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        facing = 5;
                        continue;
                    }
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        facing = 1;
                        continue;
                    }
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9)
                    {
                        facing = 5;
                        continue;
                    }

                    if (CViewObjectsExt::LastPlacedCT.Index == 5
                        || CViewObjectsExt::LastPlacedCT.Index == 6)
                        offsetPlaceX -= 1;
                    opposite = true;
                    offsetConnectX -= 1;
                    offsetConnectY -= 1;
                    index = 0;
                    if (distance < SmallDistance)
                        index = 1;
                    if (distance > LargeDistance)
                        index = 2;

                    if (index == 2)
                        offsetConnectX += 1;

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }

            }
            else if (facing == 0)
            {
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22)
                {
                    facing = 6;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16)
                {
                    facing = 6;
                    continue;
                }
                if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17)
                {
                    facing = 6;
                    continue;
                }

                if (!forceFront && (forceBack || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                    || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                    || CViewObjectsExt::LastPlacedCT.Index == 21
                    || CViewObjectsExt::LastPlacedCT.Index == 28
                    || CViewObjectsExt::LastPlacedCT.Index == 22
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 17
                    || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 18
                    ))
                {
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1)
                    {
                        facing = 1;
                        continue;
                    }
                    if (forceBack && CViewObjectsExt::PlaceConnectedTile_Start)
                    {
                        offsetPlaceY -= 1;
                    }
                    opposite = true;
                    offsetConnectX -= 1;
                    offsetConnectY -= 1;
                    offsetPlaceY += 1;
                    index = 21;
                    if (distance < SmallDistance)
                        index = 22;

                    if (distance > LargeDistance && UrbanCliff && CViewObjectsExt::LastPlacedCT.Index != 27)
                    {
                        index = 28;
                        if (tileSet.ConnectedTile[index].TileIndices[0] < 0)
                        {
                            index = 21;
                        }
                    }

                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
                else
                {
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11)
                    {
                        facing = 5;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 29
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        facing = 7;
                        continue;
                    }

                    bool txcliff = false;
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4)
                    {
                        if (!tileSet.IsTXCityCliff)
                        {
                            facing = 1;
                            continue;
                        }
                        else
                        {
                            txcliff = true;
                        }

                    }
                    if (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                        || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9)
                    {
                        facing = 2;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6)
                    {
                        facing = 2;
                        continue;
                    }
                    if (CViewObjectsExt::LastPlacedCT.Index == 18)
                    {
                        offsetPlaceX -= 1;
                        offsetPlaceY -= 1;
                    }

                    if (CViewObjectsExt::LastPlacedCT.Index == 5
                        || CViewObjectsExt::LastPlacedCT.Index == 6)
                        offsetPlaceY -= 1;
                    if (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                        || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13)
                    {
                        offsetPlaceX -= 1;
                        offsetPlaceY -= 1;
                    }
                    opposite = true;
                    offsetConnectX -= 1;
                    index = 7;
                    if (distance < SmallDistance)
                        index = 8;

                    if ((CViewObjectsExt::LastPlacedCT.Index == 5 || CViewObjectsExt::LastPlacedCT.Index == 6) && distance > LargeDistance && tileSet.IsTXCityCliff)
                    {
                        opposite = false;
                        offsetPlaceY += 1;
                        index = 29;
                    }

                    if (txcliff)
                    {
                        opposite = false;
                        index = 29;
                    }
                    for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                    {
                        cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                    }
                }
            }
            else
            {
                handleExit();
                return;
            }

            if (index < 0)
            {
                handleExit();
                return;
            }

            if (loop > 3)
            {
                handleExit();
                return;
            }
        }

        CViewObjectsExt::ThisPlacedCT = tileSet.ConnectedTile[index];
        if (!place)
        {
            CViewObjectsExt::LastTempPlacedCTIndex = index;
            CViewObjectsExt::LastTempFacing = facing;
            CViewObjectsExt::CliffConnectionTile = AutoConnect::PickVariant(cliffConnectionTiles, index);
        }
        else
        {
            if (index != CViewObjectsExt::LastTempPlacedCTIndex || facing != CViewObjectsExt::LastTempFacing)
            {
                CViewObjectsExt::CliffConnectionTile = AutoConnect::PickVariant(cliffConnectionTiles, index);
            }
            CViewObjectsExt::LastTempPlacedCTIndex = -1;
            CViewObjectsExt::LastTempFacing = -1;
        }

        if (opposite)
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y;
        }
        else
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y;
        }

        thisTile = CMapDataExt::TileData[CViewObjectsExt::CliffConnectionTile];

        if (IceCliff
            && (index == 5 || index == 6)
            && (CViewObjectsExt::LastPlacedCT.Index == 5
                || CViewObjectsExt::LastPlacedCT.Index == 6
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[25].TileIndices[0] + tileSet.StartTile;
            if (index == 6)
                offsetY -= 1;

            offsetX += 1;
            offsetY += 1;
            dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;
            if (opposite)
            {
                if (index == 6)
                {
                    offsetX -= 1;
                    offsetY += 1;
                }

                dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }

            if (dwposFix2 < 0 || dwposFix2 >= mapData.CellDataCount)
                dwposFix2 = 0;

            auto thisTileFix = CMapDataExt::TileData[idxFix];
            AutoConnect_WriteCell(tmpCellDatas, cellDatas, dwposFix2, idxFix, 0,
                CViewObjectsExt::CliffConnectionHeight + (thisTileFix.TileBlockCount == 0 ? 0 : thisTileFix.TileBlockDatas[0].Height), true);
        }
        else if (IceCliff
            && (opposite && (index == 0
                || index == 1
                || index == 2
                || index == 3
                || index == 4
                || index == 7
                || index == 8
                || index == 9
                || index == 10
                || index == 11))
            &&
            (CViewObjectsExt::LastPlacedCT.Index == 5
                || CViewObjectsExt::LastPlacedCT.Index == 6)
            )
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[25].TileIndices[0] + tileSet.StartTile;
            if (index == 6)
                offsetY -= 1;

            offsetX += 1;
            offsetY += 1;
            dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;
            if (opposite)
            {
                if (index == 6)
                {
                    offsetX -= 1;
                    offsetY += 1;
                }

                dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }
            if (dwposFix2 < 0 || dwposFix2 >= mapData.CellDataCount)
                dwposFix2 = 0;

            auto thisTileFix = CMapDataExt::TileData[idxFix];
            AutoConnect_WriteCell(tmpCellDatas, cellDatas, dwposFix2, idxFix, 0,
                CViewObjectsExt::CliffConnectionHeight + (thisTileFix.TileBlockCount == 0 ? 0 : thisTileFix.TileBlockDatas[0].Height), true);
        }

        if ((index == 5 || index == 6)
            && (CViewObjectsExt::LastPlacedCT.Index == 5
                || CViewObjectsExt::LastPlacedCT.Index == 6
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                || CViewObjectsExt::LastPlacedCT.Index == 29
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[20].TileIndices[0] + tileSet.StartTile;
            if (index == 6)
                offsetY -= 1;
            dwposFix = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;
            if (opposite)
            {
                if (index == 6)
                {
                    offsetX -= 1;
                    offsetY += 1;
                }

                dwposFix = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }
            if (dwposFix < 0 || dwposFix >= mapData.CellDataCount)
                dwposFix = 0;

            auto thisTileFix = CMapDataExt::TileData[idxFix];
            AutoConnect_WriteCell(tmpCellDatas, cellDatas, dwposFix, idxFix, 0,
                CViewObjectsExt::CliffConnectionHeight + (thisTileFix.TileBlockCount == 0 ? 0 : thisTileFix.TileBlockDatas[0].Height), true);
        }
        else if ((index == 29)
            && (CViewObjectsExt::LastPlacedCT.Index == 5
                || CViewObjectsExt::LastPlacedCT.Index == 6))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[20].TileIndices[0] + tileSet.StartTile;
            offsetY -= 1;
            dwposFix = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;
            if (opposite)
            {
                offsetX -= 1;
                offsetY += 1;
                dwposFix = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }
            if (dwposFix < 0 || dwposFix >= mapData.CellDataCount)
                dwposFix = 0;

            auto thisTileFix = CMapDataExt::TileData[idxFix];
            AutoConnect_WriteCell(tmpCellDatas, cellDatas, dwposFix, idxFix, 0,
                CViewObjectsExt::CliffConnectionHeight + (thisTileFix.TileBlockCount == 0 ? 0 : thisTileFix.TileBlockDatas[0].Height), true);
        }
        else if ((index == 0 && !opposite || index == 1 && !opposite || index == 2 && !opposite || index == 3 && !opposite || index == 4 && !opposite || index == 21 && !opposite || index == 22 && !opposite || index == 28 && !opposite)
            && (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[20].TileIndices[0] + tileSet.StartTile;
            if (index == 4)
            {
                offsetY -= 1;
            }
            if (index == 2)
            {
                offsetX += 1;
            }
            if (index == 1)
            {
                offsetX -= 1;
                offsetY -= 1;
            }
            if (index == 22)
            {
                offsetX -= 1;
                offsetY -= 1;
            }
            if (index == 21)
            {
                offsetY -= 1;
            }

            dwposFix = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;

            if (opposite)
            {
                dwposFix = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }
            if (dwposFix < 0 || dwposFix >= mapData.CellDataCount)
                dwposFix = 0;
            auto thisTileFix = CMapDataExt::TileData[idxFix];
            AutoConnect_WriteCell(tmpCellDatas, cellDatas, dwposFix, idxFix, 0,
                CViewObjectsExt::CliffConnectionHeight + (thisTileFix.TileBlockCount == 0 ? 0 : thisTileFix.TileBlockDatas[0].Height), true);
        }
        else if ((index == 7 || index == 8 || index == 9 || index == 10 || index == 11 || index == 12 || index == 13 || index == 27)
            && (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[20].TileIndices[0] + tileSet.StartTile;

            if (index == 8)
            {
                offsetX -= 1;
            }
            if (index == 9)
            {
                offsetY += 1;
            }
            if (index == 11)
            {
                offsetX -= 1;
                offsetY -= 1;
            }
            if (index == 12)
            {
                offsetX -= 1;
            }
            if (index == 13)
            {
                offsetX -= 1;
                offsetY -= 1;
            }
            if (index == 27)
            {
                offsetY -= 1;
            }

            dwposFix = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;

            if (opposite)
            {
                dwposFix = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }
            if (dwposFix < 0 || dwposFix >= mapData.CellDataCount)
                dwposFix = 0;
            auto thisTileFix = CMapDataExt::TileData[idxFix];
            AutoConnect_WriteCell(tmpCellDatas, cellDatas, dwposFix, idxFix, 0,
                CViewObjectsExt::CliffConnectionHeight + (thisTileFix.TileBlockCount == 0 ? 0 : thisTileFix.TileBlockDatas[0].Height), true);
        }

        if (((index == 5 || index == 6 || index == 7 && opposite || index == 8 && opposite || index == 9 && opposite || index == 10 && opposite || index == 11 && opposite || index == 30 && !opposite)
            && (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13))
            || (index == 18
                && !CViewObjectsExt::LastPlacedCT.Opposite && (CViewObjectsExt::LastPlacedCT.Index == 7
                    || CViewObjectsExt::LastPlacedCT.Index == 8
                    || CViewObjectsExt::LastPlacedCT.Index == 9
                    || CViewObjectsExt::LastPlacedCT.Index == 10
                    || CViewObjectsExt::LastPlacedCT.Index == 11))
            || CViewObjectsExt::LastPlacedCT.Index == 18
            && opposite && (index == 7
                || index == 8
                || index == 9
                || index == 10
                || index == 11))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[24].TileIndices[0] + tileSet.StartTile;
            if (index == 5)
            {
                offsetX += 1;
            }
            if (index == 6)
            {
                offsetX += 1;
                offsetY -= 1;
            }
            if (index == 30)
            {
                offsetX += 1;
                offsetY += 1;
            }
            if (index == 9)
            {
                offsetX += 1;
            }
            if (index == 10)
            {
                offsetX += 1;
            }
            if (index == 11)
            {
                offsetX += 1;
            }
            if (index == 7 || index == 8)
            {
                offsetX += 1;
            }


            dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;

            if (opposite)
            {
                dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;

            }
            if (dwposFix2 < 0 || dwposFix2 >= mapData.CellDataCount)
                dwposFix2 = 0;

            auto thisTileFix = CMapDataExt::TileData[idxFix];
            AutoConnect_WriteCell(tmpCellDatas, cellDatas, dwposFix2, idxFix, 0,
                CViewObjectsExt::CliffConnectionHeight + (thisTileFix.TileBlockCount == 0 ? 0 : thisTileFix.TileBlockDatas[0].Height), true);
        }
        else if ((index == 27 && !opposite || index == 10 && !opposite || index == 11 && !opposite || index == 12 && !opposite || index == 13 && !opposite)
            && (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 7
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 8
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 9
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 10
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 11))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[24].TileIndices[0] + tileSet.StartTile;

            if (index == 11)
            {
                offsetY -= 1;
            }
            if (index == 10)
            {
                offsetX += 1;
            }
            if (index == 13)
            {
                offsetY -= 1;
            }
            if (index == 27)
            {
                offsetY -= 1;
                offsetX += 1;
            }

            dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;

            if (opposite)
            {
                dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;

            }
            if (dwposFix2 < 0 || dwposFix2 >= mapData.CellDataCount)
                dwposFix2 = 0;

            auto thisTileFix = CMapDataExt::TileData[idxFix];
            AutoConnect_WriteCell(tmpCellDatas, cellDatas, dwposFix2, idxFix, 0,
                CViewObjectsExt::CliffConnectionHeight + (thisTileFix.TileBlockCount == 0 ? 0 : thisTileFix.TileBlockDatas[0].Height), true);
        }

        if (((index == 0 || index == 1 || index == 21 && !opposite || index == 22 && !opposite || index == 28 && !opposite
            || index == 2 && opposite || index == 3 && opposite || index == 4 && opposite || index == 5 && opposite || index == 6 && opposite)
            && (CViewObjectsExt::LastPlacedCT.Index == 0
                || CViewObjectsExt::LastPlacedCT.Index == 1
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 2
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 3
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 4
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 30
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 5
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 6
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22))
            || index == 18
            && !CViewObjectsExt::LastPlacedCT.Opposite && (CViewObjectsExt::LastPlacedCT.Index == 0
                || CViewObjectsExt::LastPlacedCT.Index == 1
                || CViewObjectsExt::LastPlacedCT.Index == 2
                || CViewObjectsExt::LastPlacedCT.Index == 3
                || CViewObjectsExt::LastPlacedCT.Index == 4)
            || CViewObjectsExt::LastPlacedCT.Index == 18
            && opposite && (index == 0
                || index == 1
                || index == 2
                || index == 3
                || index == 4)
            || index == 30 && opposite &&
            (CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 0
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 1
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[23].TileIndices[0] + tileSet.StartTile;
            if (index == 1 || index == 22)
            {
                offsetX -= 1;
            }
            if (index == 28)
            {
                offsetY += 1;
            }
            if (index == 30)
            {
                offsetX += 1;
                offsetY += 1;
            }
            if (index == 0)
            {
                offsetY += 1;
            }

            dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;
            if (opposite)
            {
                if (index == 1)
                {
                    offsetX += 1;
                    offsetY += 1;
                }
                if (index == 2 || index == 3 || index == 4 || index == 5 || index == 6)
                {
                    offsetY += 1;
                }
                if (index == 6)
                {
                    offsetX -= 1;
                }
                dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }
            if (dwposFix2 < 0 || dwposFix2 >= mapData.CellDataCount)
                dwposFix2 = 0;
            auto thisTileFix = CMapDataExt::TileData[idxFix];
            AutoConnect_WriteCell(tmpCellDatas, cellDatas, dwposFix2, idxFix, 0,
                CViewObjectsExt::CliffConnectionHeight + (thisTileFix.TileBlockCount == 0 ? 0 : thisTileFix.TileBlockDatas[0].Height), true);
        }
        if ((index == 14 && !opposite || index == 15 && !opposite || index == 25 && !opposite || index == 16 && !opposite || index == 21 && opposite || index == 22 && opposite || index == 28 && opposite)
            && (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 12
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 13
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 27
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[19].TileIndices[0] + tileSet.StartTile;
            if (index == 25 || CViewObjectsExt::LastPlacedCT.Index == 25)
                idxFix = tileSet.ConnectedTile[26].TileIndices[0] + tileSet.StartTile;

            if (index == 14 || index == 15 || index == 25)
            {
                offsetX += 1;
                offsetY += 1;
            }
            if (index == 16)
            {
                offsetX += 1;
            }
            if (index == 21 || index == 22 || index == 28)
            {
                offsetX += 1;
            }
            dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;

            if (opposite)
            {
                dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }
            if (dwposFix2 < 0 || dwposFix2 >= mapData.CellDataCount)
                dwposFix2 = 0;

            auto thisTileFix = CMapDataExt::TileData[idxFix];
            AutoConnect_WriteCell(tmpCellDatas, cellDatas, dwposFix2, idxFix, 0,
                CViewObjectsExt::CliffConnectionHeight + (thisTileFix.TileBlockCount == 0 ? 0 : thisTileFix.TileBlockDatas[0].Height), true);
        }
        else if ((index == 14 && opposite || index == 15 && opposite || index == 25 && opposite || index == 16 && opposite || index == 12 && opposite || index == 13 && opposite || index == 27 && opposite)
            && (!CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 21
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 22
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 28
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 14
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 15
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 25
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Index == 16))
        {
            int offsetX = 0;
            int offsetY = 0;
            int idxFix = tileSet.ConnectedTile[19].TileIndices[0] + tileSet.StartTile;
            if (index == 25 || CViewObjectsExt::LastPlacedCT.Index == 25)
                idxFix = tileSet.ConnectedTile[26].TileIndices[0] + tileSet.StartTile;

            if (index == 14 || index == 15 || index == 25)
            {
                offsetX += 1;
                offsetY += 1;
            }
            if (index == 16)
            {
                offsetY += 1;
            }
            if (index == 12 || index == 13)
            {
                offsetY += 1;
            }
            if (index == 27)
            {
                offsetY += 2;
            }

            dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + offsetX;

            if (opposite)
            {
                dwposFix2 = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY + offsetY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX + offsetX;
            }
            if (dwposFix2 < 0 || dwposFix2 >= mapData.CellDataCount)
                dwposFix2 = 0;

            auto thisTileFix = CMapDataExt::TileData[idxFix];
            AutoConnect_WriteCell(tmpCellDatas, cellDatas, dwposFix2, idxFix, 0,
                CViewObjectsExt::CliffConnectionHeight + (thisTileFix.TileBlockCount == 0 ? 0 : thisTileFix.TileBlockDatas[0].Height), true);
        }

    }
    else if (tileSet.Type == ConnectedTileSetTypes::DirtRoad || tileSet.Type == ConnectedTileSetTypes::CityDirtRoad) //much smarter
    {
        SmallDistance = 3;
        int MiddleDistance = 5;
        int MiddleDistanceHorizontal = 6;
        LargeDistance = 7;

        if (!tileSet.Name)
        {
            handleExit();
            return;
        }

        int index = -1;

        if (NULL == CMapDataExt::TileData)
        {
            handleExit();
            return;
        }

        auto getSuitableBendy = [&tileSet, &getOppositeDirection, &opposite, &cityRoad](int lastDirection, int direction)
        {
            for (int i = 0; i < 24; i++)
            {
                if (cityRoad && i == 6)
                    continue; //buggy tile
                if (tileSet.ConnectedTile[i].Direction0 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction1 == direction)
                {
                    opposite = false;
                    return i;
                }

                if (tileSet.ConnectedTile[i].Direction1 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction0 == direction)
                {
                    opposite = true;
                    return i;
                }
            }
            return -1;
        };

        //        7 
        //     6     0
        //  5           1
        //     4     2
        //        3

        int loop = 0;
        while (cliffConnectionTiles.empty())
        {
            loop++;
            if (CViewObjectsExt::LastPlacedCT.Index == -1
                || !CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Direction1 == facing
                || CViewObjectsExt::LastPlacedCT.Opposite && CViewObjectsExt::LastPlacedCT.Direction0 == facing)
            {
                if (facing == 1)
                {
                    //offsetPlaceY += 1;
                    //offsetConnectX -= 1;

                    index = 26;
                    if (distance < SmallDistance)
                        index = 27;
                    if (distance > MiddleDistanceHorizontal)
                        index = 24;
                    if ((distance > MiddleDistanceHorizontal) && cityRoad)
                        index = 25;
                }
                else if (facing == 2)
                {
                    if (distance < SmallDistance)
                        index = 33;
                    else if (distance < MiddleDistance)
                        index = 32;
                    else if (distance < LargeDistance)
                        index = 31;
                    else
                        index = 30;
                }
                else if (facing == 3)
                {
                    index = 37;
                    if (distance < SmallDistance)
                        index = 38;
                    if (distance > MiddleDistanceHorizontal)
                        index = 36;
                }
                else if (facing == 4)
                {
                    if (distance < SmallDistance)
                        index = 44;
                    else if (distance < MiddleDistance)
                        index = 43;
                    else if (distance < LargeDistance)
                        index = 42;
                    else
                        index = 41;
                }
                else if (facing == 5)
                {
                    opposite = true;
                    index = 26;
                    if (distance < SmallDistance)
                        index = 27;
                    if (distance > MiddleDistanceHorizontal)
                        index = 24;
                    if ((distance > MiddleDistanceHorizontal) && cityRoad)
                        index = 25;
                }
                else if (facing == 6)
                {
                    opposite = true;
                    if (distance < SmallDistance)
                        index = 33;
                    else if (distance < MiddleDistance)
                        index = 32;
                    else if (distance < LargeDistance)
                        index = 31;
                    else
                        index = 30;
                }
                else if (facing == 7)
                {
                    opposite = true;
                    index = 37;
                    if (distance < SmallDistance)
                        index = 38;
                    if (distance > MiddleDistanceHorizontal)
                        index = 36;
                }
                else if (facing == 0)
                {
                    opposite = true;
                    if (distance < SmallDistance)
                        index = 44;
                    else if (distance < MiddleDistance)
                        index = 43;
                    else if (distance < LargeDistance)
                        index = 42;
                    else
                        index = 41;
                }
                else
                {
                    handleExit();
                    return;
                }
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 1 && facing == 0 && distance > LargeDistance && !cityRoad)
            {
                index = 29;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 1 && facing == 2 && distance > LargeDistance && !cityRoad)
            {
                index = 28;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 2 && facing == 1 && distance > LargeDistance && !cityRoad)
            {
                index = 35;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 2 && facing == 3 && distance > LargeDistance && !cityRoad)
            {
                index = 34;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 3 && facing == 2 && distance > LargeDistance && !cityRoad)
            {
                index = 39;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 3 && facing == 4 && distance > LargeDistance && !cityRoad)
            {
                index = 40;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 4 && facing == 3 && distance > LargeDistance && !cityRoad)
            {
                index = 46;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 4 && facing == 5 && distance > LargeDistance && !cityRoad)
            {
                index = 45;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 5 && facing == 4 && distance > LargeDistance && !cityRoad)
            {
                opposite = true;
                index = 29;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 5 && facing == 6 && distance > LargeDistance && !cityRoad)
            {
                opposite = true;
                index = 28;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 6 && facing == 5 && distance > LargeDistance && !cityRoad)
            {
                opposite = true;
                index = 35;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 6 && facing == 7 && distance > LargeDistance && !cityRoad)
            {
                opposite = true;
                index = 34;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 7 && facing == 6 && distance > LargeDistance && !cityRoad)
            {
                opposite = true;
                index = 39;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 7 && facing == 0 && distance > LargeDistance && !cityRoad)
            {
                opposite = true;
                index = 40;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 0 && facing == 7 && distance > LargeDistance && !cityRoad)
            {
                opposite = true;
                index = 46;
            }
            else if (CViewObjectsExt::LastPlacedCT.GetNextDirection() == 0 && facing == 1 && distance > LargeDistance && !cityRoad)
            {
                opposite = true;
                index = 45;
            }
            else
            {
                index = getSuitableBendy(CViewObjectsExt::LastPlacedCT.GetNextDirection(), facing);
            }

            if (CViewObjectsExt::NextCTHeightOffset > 0)
            {
                if (index == 30 || index == 31 || index == 32 || index == 33)
                {
                    if (opposite)
                        index = 49;
                    else
                        index = 47;
                }
                else if (index == 41 || index == 42 || index == 43 || index == 44)
                {
                    if (opposite)
                        index = 50;
                    else
                        index = 48;
                }
            }
            else if (CViewObjectsExt::NextCTHeightOffset < 0)
            {
                if (index == 30 || index == 31 || index == 32 || index == 33)
                {
                    if (opposite)
                        index = 47;
                    else
                        index = 49;
                }
                else if (index == 41 || index == 42 || index == 43 || index == 44)
                {
                    if (opposite)
                        index = 48;
                    else
                        index = 50;
                }
            }
            if (index >= 47)
                thisTileHeightOffest = true;


            if (CViewObjectsExt::LastSuccessfulIndex == -1 && index == -1)
            {
                handleExit();
                return;
            }

            if (index < 0)
            {
                if (getOppositeDirection(CViewObjectsExt::LastPlacedCT.GetNextDirection()) != tileSet.ConnectedTile[CViewObjectsExt::LastSuccessfulIndex].GetThisDirection(CViewObjectsExt::LastSuccessfulOpposite))
                {
                    handleExit();
                    return;
                }

                opposite = CViewObjectsExt::LastSuccessfulOpposite;
                index = CViewObjectsExt::LastSuccessfulIndex;
                if (CViewObjectsExt::LastSuccessfulHeightOffset != 0 || tileSet.ConnectedTile[index].HeightAdjust != 0)
                {
                    handleExit();
                    return;
                }
            }
            else
            {
                CViewObjectsExt::LastSuccessfulOpposite = opposite;
                CViewObjectsExt::LastSuccessfulIndex = index;
            }

            if (!tileSet.ConnectedTile[index].TileIndices.empty())
                for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                {
                    cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                }

            if (loop > 3)
            {
                handleExit();
                return;
            }
        }

        if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 1)
        {
            offsetConnectX -= 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 2)
        {
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 3)
        {
            offsetConnectX += 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 4)
        {
            offsetConnectX += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 5)
        {
            offsetConnectX += 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 6)
        {
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 7)
        {
            offsetConnectX -= 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 0)
        {
            offsetConnectX -= 1;
        }

        if (CViewObjectsExt::PlaceConnectedTile_Start)
        {
            if (facing == 1)
            {
                offsetPlaceY += 1;
                offsetPlaceX -= 1;
            }
            else if (facing == 2)
            {
                offsetPlaceX += 1;
            }
            else if (facing == 3)
            {
                offsetPlaceY += 1;
                offsetPlaceX += 1;
            }
            else if (facing == 6)
            {
                offsetPlaceX += 1;
            }
        }


        CViewObjectsExt::ThisPlacedCT = tileSet.ConnectedTile[index];
        if (!place)
        {
            CViewObjectsExt::LastTempPlacedCTIndex = index;
            CViewObjectsExt::LastTempFacing = facing;
            CViewObjectsExt::CliffConnectionTile = AutoConnect::PickVariant(cliffConnectionTiles, index);
        }
        else
        {
            if (index != CViewObjectsExt::LastTempPlacedCTIndex || facing != CViewObjectsExt::LastTempFacing)
            {
                CViewObjectsExt::CliffConnectionTile = AutoConnect::PickVariant(cliffConnectionTiles, index);
            }
            CViewObjectsExt::LastTempPlacedCTIndex = -1;
            CViewObjectsExt::LastTempFacing = -1;
        }


        if (opposite)
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y;
        }
        else
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y;
        }

        thisTile = CMapDataExt::TileData[CViewObjectsExt::CliffConnectionTile];


    }
    else if (tileSet.Type == ConnectedTileSetTypes::Shore)
    {
        SmallDistance = 4;
        LargeDistance = 7;

        if (!tileSet.Name)
        {
            handleExit();
            return;
        }

        int index = -1;

        if (NULL == CMapDataExt::TileData)
        {
            handleExit();
            return;
        }

        auto getSuitableBendy = [&tileSet, &getOppositeDirection, &opposite, &distance, &SmallDistance, &LargeDistance](bool lastSide, int lastDirection, int direction)
        {
            bool met = false;
            for (int i = 0; i < 24; i++)
            {
                if (tileSet.ConnectedTile[i].Direction0 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction1 == direction && tileSet.ConnectedTile[i].Side0 == lastSide)
                {
                    met = true;
                    opposite = false;
                }
                else if (tileSet.ConnectedTile[i].Direction1 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction0 == direction && tileSet.ConnectedTile[i].Side1 == lastSide)
                {
                    met = true;
                    opposite = true;
                }
                if (met)
                {
                    if (i == 0 && distance < SmallDistance)
                        i = 1;
                    else if (i == 5 && distance < SmallDistance)
                        i = 6;
                    else if (i == 10 && distance < SmallDistance)
                        i = 11;
                    else if (i == 15 && distance < SmallDistance)
                        i = 16;
                    return i;
                }
            }
            return -1;
        };

        auto getSuitableBendy1357 = [&tileSet, &getOppositeDirection, &opposite, &distance, &SmallDistance, &LargeDistance](bool lastSide, int lastDirection, int direction)
        {
            if (direction == 1 || direction == 5)
                for (int i = 0; i < 24; i++)
                {
                    if (i != 3 && i != 7 && i != 12 && i != 18)
                        continue;

                    if (((direction == 1 && i == 3)
                        || (direction == 5 && i == 7)
                        || (direction == 1 && i == 12)
                        || (direction == 5 && i == 18))
                        && tileSet.ConnectedTile[i].Direction0 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Side0 == lastSide)
                    {
                        if (tileSet.ConnectedTile.size() < 25)
                            return -1;

                        opposite = false;

                        if (direction == 5 && i == 7 && distance >= SmallDistance)
                        {
                            opposite = true;
                            i = 24;
                        }
                        if (direction == 5 && i == 24 && distance > LargeDistance)
                        {
                            opposite = true;
                            i = 25;
                        }

                        return i;
                    }
                    else if (((direction == 5 && i == 3)
                        || (direction == 1 && i == 7)
                        || (direction == 5 && i == 12)
                        || (direction == 1 && i == 18))
                        && tileSet.ConnectedTile[i].Direction1 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Side1 == lastSide)
                    {
                        if (tileSet.ConnectedTile.size() < 25)
                            return -1;

                        opposite = true;

                        if (direction == 1 && i == 7 && distance >= SmallDistance)
                        {
                            opposite = false;
                            i = 24;
                        }
                        if (direction == 1 && i == 24 && distance > LargeDistance)
                        {
                            opposite = false;
                            i = 25;
                        }

                        return i;
                    }

                }
            if (direction == 3 || direction == 7)
                for (int i = 0; i < 24; i++)
                {
                    if (i != 2 && i != 8 && i != 13 && i != 17)
                        continue;

                    if (direction == 3 && tileSet.ConnectedTile[i].Direction0 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Side0 == lastSide)
                    {
                        opposite = false;
                        return i;
                    }
                    else if (direction == 7 && tileSet.ConnectedTile[i].Direction1 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Side1 == lastSide)
                    {
                        opposite = true;
                        return i;
                    }

                }
            return -1;
        };

        //        7 
        //     6     0
        //  5           1
        //     4     2
        //        3

        int loop = 0;
        while (cliffConnectionTiles.empty())
        {
            loop++;
            if (CViewObjectsExt::LastPlacedCT.Index == -1 && forceFront)
            {
                if (facing == 1)
                {
                    index = 3;
                }
                else if (facing == 2)
                {
                    index = 0;
                    if (distance < SmallDistance)
                        index = 1;
                }
                else if (facing == 3)
                {
                    index = 2;
                }
                else if (facing == 4)
                {
                    index = 5;
                    if (distance < SmallDistance)
                        index = 6;
                }
                else if (facing == 5)
                {
                    opposite = true;
                    index = 3;
                }
                else if (facing == 6)
                {
                    opposite = true;
                    index = 0;
                    if (distance < SmallDistance)
                        index = 1;
                }
                else if (facing == 7)
                {
                    opposite = true;
                    index = 2;
                }
                else if (facing == 0)
                {
                    opposite = true;
                    index = 5;
                    if (distance < SmallDistance)
                        index = 6;
                }
                else
                {
                    handleExit();
                    return;
                }
            }
            else if (CViewObjectsExt::LastPlacedCT.Index == -1 && forceBack)
            {
                if (facing == 1)
                {
                    index = 12;
                }
                else if (facing == 2)
                {
                    index = 10;
                    if (distance < SmallDistance)
                        index = 11;
                }
                else if (facing == 3)
                {
                    index = 13;
                }
                else if (facing == 4)
                {
                    index = 15;
                    if (distance < SmallDistance)
                        index = 16;
                }
                else if (facing == 5)
                {
                    opposite = true;
                    index = 12;
                }
                else if (facing == 6)
                {
                    opposite = true;
                    index = 10;
                    if (distance < SmallDistance)
                        index = 11;
                }
                else if (facing == 7)
                {
                    opposite = true;
                    index = 13;
                }
                else if (facing == 0)
                {
                    opposite = true;
                    index = 15;
                    if (distance < SmallDistance)
                        index = 16;
                }
                else
                {
                    handleExit();
                    return;
                }
            }
            else if (facing == 1 || facing == 3 || facing == 5 || facing == 7)
            {
                index = getSuitableBendy1357(CViewObjectsExt::LastPlacedCT.GetNextSide(), CViewObjectsExt::LastPlacedCT.GetNextDirection(), facing);
            }
            else
            {
                index = getSuitableBendy(CViewObjectsExt::LastPlacedCT.GetNextSide(), CViewObjectsExt::LastPlacedCT.GetNextDirection(), facing);
            }

            if (CViewObjectsExt::LastSuccessfulIndex == -1 && index == -1)
            {
                handleExit();
                return;
            }


            if (index < 0)
            {
                if (getOppositeDirection(CViewObjectsExt::LastPlacedCT.GetNextDirection()) != tileSet.ConnectedTile[CViewObjectsExt::LastSuccessfulIndex].GetThisDirection(CViewObjectsExt::LastSuccessfulOpposite))
                {
                    handleExit();
                    return;
                }

                opposite = CViewObjectsExt::LastSuccessfulOpposite;
                index = CViewObjectsExt::LastSuccessfulIndex;
                if (CViewObjectsExt::LastSuccessfulHeightOffset != 0 || tileSet.ConnectedTile[index].HeightAdjust != 0)
                {
                    handleExit();
                    return;
                }
            }
            else
            {
                CViewObjectsExt::LastSuccessfulOpposite = opposite;
                CViewObjectsExt::LastSuccessfulIndex = index;
            }


            if (!tileSet.ConnectedTile[index].TileIndices.empty())
                for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                {
                    cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                }

            if (loop > 3)
            {
                handleExit();
                return;
            }
        }

        if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 1)
        {
            offsetConnectX -= 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 2)
        {
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 3)
        {
            offsetConnectX += 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 4)
        {
            offsetConnectX += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 5)
        {
            offsetConnectX += 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 6)
        {
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 7)
        {
            offsetConnectX -= 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 0)
        {
            offsetConnectX -= 1;
        }

        CViewObjectsExt::ThisPlacedCT = tileSet.ConnectedTile[index];
        if (!place)
        {
            CViewObjectsExt::LastTempPlacedCTIndex = index;
            CViewObjectsExt::LastTempFacing = facing;
            CViewObjectsExt::CliffConnectionTile = AutoConnect::PickVariant(cliffConnectionTiles, index);
        }
        else
        {
            if (index != CViewObjectsExt::LastTempPlacedCTIndex || facing != CViewObjectsExt::LastTempFacing)
            {
                CViewObjectsExt::CliffConnectionTile = AutoConnect::PickVariant(cliffConnectionTiles, index);
            }
            CViewObjectsExt::LastTempPlacedCTIndex = -1;
            CViewObjectsExt::LastTempFacing = -1;
        }


        if (opposite)
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y;
        }
        else
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y;
        }

        thisTile = CMapDataExt::TileData[CViewObjectsExt::CliffConnectionTile];
    }
    else if (tileSet.Type == ConnectedTileSetTypes::PaveShore || tileSet.Type == ConnectedTileSetTypes::SpecialPaveShore)
    {
        facing = CMapDataExt::GetFacing4(CViewObjectsExt::CliffConnectionCoord, cursor);
        SmallDistance = 3;
        LargeDistance = 5;


        if (!tileSet.Name)
        {
            handleExit();
            return;
        }

        int index = -1;

        if (NULL == CMapDataExt::TileData)
        {
            handleExit();
            return;
        }

        auto getSuitableBendy = [&tileSet, &getOppositeDirection, &opposite, &distance, &SmallDistance, &LargeDistance](bool lastSide, int lastDirection, int direction)
        {
            bool met = false;
            for (int i = 0; i < 12; i++)
            {
                if (tileSet.ConnectedTile[i].Direction0 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction1 == direction && tileSet.ConnectedTile[i].Side0 == lastSide)
                {
                    met = true;
                    opposite = false;
                }
                else if (tileSet.ConnectedTile[i].Direction1 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction0 == direction && tileSet.ConnectedTile[i].Side1 == lastSide)
                {
                    met = true;
                    opposite = true;
                }
                if (met)
                {
                    return i;
                }
            }
            return -1;
        };

        //        7 
        //     6     0
        //  5           1
        //     4     2
        //        3

        int loop = 0;
        while (cliffConnectionTiles.empty())
        {
            loop++;
            if (CViewObjectsExt::LastPlacedCT.Index == -1 && forceFront)
            {
                if (facing == 2)
                {
                    index = 4;
                }
                else if (facing == 4)
                {
                    index = 2;
                }
                else if (facing == 6)
                {
                    opposite = true;
                    index = 4;
                }
                else if (facing == 0)
                {
                    opposite = true;
                    index = 2;
                }
                else
                {
                    handleExit();
                    return;
                }
            }
            else if (CViewObjectsExt::LastPlacedCT.Index == -1 && forceBack)
            {
                if (facing == 2)
                {
                    index = 0;
                }
                else if (facing == 4)
                {
                    index = 6;
                }
                else if (facing == 6)
                {
                    opposite = true;
                    index = 0;
                }
                else if (facing == 0)
                {
                    opposite = true;
                    index = 6;
                }
                else
                {
                    handleExit();
                    return;
                }
            }
            else
            {
                index = getSuitableBendy(CViewObjectsExt::LastPlacedCT.GetNextSide(), CViewObjectsExt::LastPlacedCT.GetNextDirection(), facing);
            }

            if (CViewObjectsExt::LastSuccessfulIndex == -1 && index == -1)
            {
                handleExit();
                return;
            }

            if (index < 0)
            {
                if (getOppositeDirection(CViewObjectsExt::LastPlacedCT.GetNextDirection()) != tileSet.ConnectedTile[CViewObjectsExt::LastSuccessfulIndex].GetThisDirection(CViewObjectsExt::LastSuccessfulOpposite))
                {
                    handleExit();
                    return;
                }

                opposite = CViewObjectsExt::LastSuccessfulOpposite;
                index = CViewObjectsExt::LastSuccessfulIndex;
                if (CViewObjectsExt::LastSuccessfulHeightOffset != 0 || tileSet.ConnectedTile[index].HeightAdjust != 0)
                {
                    handleExit();
                    return;
                }
            }
            else
            {
                CViewObjectsExt::LastSuccessfulOpposite = opposite;
                CViewObjectsExt::LastSuccessfulIndex = index;
            }


            if (!tileSet.ConnectedTile[index].TileIndices.empty())
                for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                {
                    cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                }

            if (loop > 3)
            {
                handleExit();
                return;
            }
        }

        if (index == 0 || index == 2 || index == 4 || index == 6)
            MultiPlaceDirection = facing;

        if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 1)
        {
            offsetConnectX -= 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 2)
        {
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 3)
        {
            offsetConnectX += 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 4)
        {
            offsetConnectX += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 5)
        {
            offsetConnectX += 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 6)
        {
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 7)
        {
            offsetConnectX -= 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 0)
        {
            offsetConnectX -= 1;
        }

        CViewObjectsExt::ThisPlacedCT = tileSet.ConnectedTile[index];
        if (!place)
        {
            CViewObjectsExt::LastTempPlacedCTIndex = index;
            CViewObjectsExt::LastTempFacing = facing;
            CViewObjectsExt::CliffConnectionTile = AutoConnect::PickVariant(cliffConnectionTiles, index);
        }
        else
        {
            if (index != CViewObjectsExt::LastTempPlacedCTIndex || facing != CViewObjectsExt::LastTempFacing)
            {
                CViewObjectsExt::CliffConnectionTile = AutoConnect::PickVariant(cliffConnectionTiles, index);
            }
            CViewObjectsExt::LastTempPlacedCTIndex = -1;
            CViewObjectsExt::LastTempFacing = -1;
        }


        if (opposite)
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y;
        }
        else
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y;
        }

        thisTile = CMapDataExt::TileData[CViewObjectsExt::CliffConnectionTile];
    }
    else if (tileSet.Type == ConnectedTileSetTypes::Highway)
    {
        facing = CMapDataExt::GetFacing4(CViewObjectsExt::CliffConnectionCoord, cursor);
        SmallDistance = 3;
        LargeDistance = 5;

        if (!tileSet.Name)
        {
            handleExit();
            return;
        }

        int index = -1;

        if (NULL == CMapDataExt::TileData)
        {
            handleExit();
            return;
        }

        auto getSuitableBendy = [&tileSet, &getOppositeDirection, &opposite, &distance, &SmallDistance, &LargeDistance](bool lastSide, int lastDirection, int direction)
        {
            bool met = false;
            for (int i = 0; i < 6; i++)
            {
                if (tileSet.ConnectedTile[i].Direction0 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction1 == direction && tileSet.ConnectedTile[i].Side0 == lastSide)
                {
                    met = true;
                    opposite = false;
                }
                else if (tileSet.ConnectedTile[i].Direction1 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction0 == direction && tileSet.ConnectedTile[i].Side1 == lastSide)
                {
                    met = true;
                    opposite = true;
                }
                if (met)
                {
                    return i;
                }
            }
            return -1;
        };

        //        7 
        //     6     0
        //  5           1
        //     4     2
        //        3

        int loop = 0;
        while (cliffConnectionTiles.empty())
        {
            loop++;
            if (CViewObjectsExt::LastPlacedCT.Index == -1)
            {
                if (facing == 2)
                {
                    index = 0;
                }
                else if (facing == 4)
                {
                    index = 1;
                }
                else if (facing == 6)
                {
                    opposite = true;
                    index = 0;
                }
                else if (facing == 0)
                {
                    opposite = true;
                    index = 1;
                }
                else
                {
                    handleExit();
                    return;
                }
            }
            else
            {
                index = getSuitableBendy(CViewObjectsExt::LastPlacedCT.GetNextSide(), CViewObjectsExt::LastPlacedCT.GetNextDirection(), facing);
            }

            if (CViewObjectsExt::NextCTHeightOffset > 0)
            {
                if (index == 0)
                {
                    if (opposite)
                        index = 8;
                    else
                        index = 6;
                }
                else if (index == 1)
                {
                    if (opposite)
                        index = 9;
                    else
                        index = 7;
                }
            }
            else if (CViewObjectsExt::NextCTHeightOffset < 0)
            {
                if (index == 0)
                {
                    if (opposite)
                        index = 6;
                    else
                        index = 8;
                }
                else if (index == 1)
                {
                    if (opposite)
                        index = 7;
                    else
                        index = 9;
                }
            }
            if (index >= 6)
                thisTileHeightOffest = true;

            if (CViewObjectsExt::LastSuccessfulIndex == -1 && index == -1)
            {
                handleExit();
                return;
            }

            if (index < 0)
            {
                if (getOppositeDirection(CViewObjectsExt::LastPlacedCT.GetNextDirection()) != tileSet.ConnectedTile[CViewObjectsExt::LastSuccessfulIndex].GetThisDirection(CViewObjectsExt::LastSuccessfulOpposite))
                {
                    handleExit();
                    return;
                }

                opposite = CViewObjectsExt::LastSuccessfulOpposite;
                index = CViewObjectsExt::LastSuccessfulIndex;
                if (CViewObjectsExt::LastSuccessfulHeightOffset != 0 || tileSet.ConnectedTile[index].HeightAdjust != 0)
                {
                    handleExit();
                    return;
                }
            }
            else
            {
                CViewObjectsExt::LastSuccessfulOpposite = opposite;
                CViewObjectsExt::LastSuccessfulIndex = index;
            }


            if (!tileSet.ConnectedTile[index].TileIndices.empty())
                for (auto ti : tileSet.ConnectedTile[index].TileIndices)
                {
                    cliffConnectionTiles.push_back(ti + tileSet.StartTile);
                }

            if (loop > 3)
            {
                handleExit();
                return;
            }
        }

        if (index == 0 || index == 1)
            MultiPlaceDirection = facing;

        if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 1)
        {
            offsetConnectX -= 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 2)
        {
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 3)
        {
            offsetConnectX += 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 4)
        {
            offsetConnectX += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 5)
        {
            offsetConnectX += 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 6)
        {
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 7)
        {
            offsetConnectX -= 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 0)
        {
            offsetConnectX -= 1;
        }

        CViewObjectsExt::ThisPlacedCT = tileSet.ConnectedTile[index];
        if (!place)
        {
            CViewObjectsExt::LastTempPlacedCTIndex = index;
            CViewObjectsExt::LastTempFacing = facing;
            CViewObjectsExt::CliffConnectionTile = AutoConnect::PickVariant(cliffConnectionTiles, index);
        }
        else
        {
            if (index != CViewObjectsExt::LastTempPlacedCTIndex || facing != CViewObjectsExt::LastTempFacing)
            {
                CViewObjectsExt::CliffConnectionTile = AutoConnect::PickVariant(cliffConnectionTiles, index);
            }
            CViewObjectsExt::LastTempPlacedCTIndex = -1;
            CViewObjectsExt::LastTempFacing = -1;
        }


        if (opposite)
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y;
        }
        else
        {
            offsetConnectX -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X;
            offsetConnectY -= CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y;
        }

        thisTile = CMapDataExt::TileData[CViewObjectsExt::CliffConnectionTile];
    }
    else if (tileSet.Type == ConnectedTileSetTypes::RailRoad)
    {
        int index = -1;
        auto getSuitableBendy = [&tileSet, &getOppositeDirection, &opposite, &distance, &SmallDistance, &LargeDistance](int lastDirection, int direction)
        {
            bool met = false;
            for (int i = 0; i < tileSet.ConnectedTile.size(); i++)
            {
                if (tileSet.ConnectedTile[i].Direction0 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction1 == direction)
                {
                    met = true;
                    opposite = false;
                }
                else if (tileSet.ConnectedTile[i].Direction1 == getOppositeDirection(lastDirection) && tileSet.ConnectedTile[i].Direction0 == direction)
                {
                    met = true;
                    opposite = true;
                }
                if (met)
                {
                    return i;
                }
            }
            return -1;
        };
        if (CViewObjectsExt::LastPlacedCT.Index == -1)
        {
            if (facing == 0)
            {
                index = 2;
            }
            else if (facing == 1)
            {
                index = 1;
            }
            else if (facing == 2)
            {
                index = 3;
            }
            else if (facing == 3)
            {
                opposite = true;
                index = 0;
            }
            else if (facing == 4)
            {
                opposite = true;
                index = 2;
            }
            else if (facing == 5)
            {
                opposite = true;
                index = 1;
            }
            else if (facing == 6)
            {
                opposite = true;
                index = 3;
            }
            else if (facing == 7)
            {
                index = 0;
            }
        }
        else
        {
            index = getSuitableBendy(CViewObjectsExt::LastPlacedCT.GetNextDirection(), facing);
        }

        if (index < 0)
        {
            if (getOppositeDirection(CViewObjectsExt::LastPlacedCT.GetNextDirection()) != tileSet.ConnectedTile[CViewObjectsExt::LastSuccessfulIndex].GetThisDirection(CViewObjectsExt::LastSuccessfulOpposite))
            {
                handleExit();
                return;
            }

            opposite = CViewObjectsExt::LastSuccessfulOpposite;
            index = CViewObjectsExt::LastSuccessfulIndex;
            if (CViewObjectsExt::LastSuccessfulHeightOffset != 0 || tileSet.ConnectedTile[index].HeightAdjust != 0)
            {
                handleExit();
                return;
            }
        }
        else
        {
            CViewObjectsExt::LastSuccessfulOpposite = opposite;
            CViewObjectsExt::LastSuccessfulIndex = index;
        }

        if (!tileSet.ConnectedTile[index].TileIndices.empty())
        {
            CViewObjectsExt::CliffConnectionTile = tileSet.ConnectedTile[index].TileIndices[0] + tileSet.StartTile;
        }
        else
        {
            handleExit();
            return;
        }
        CViewObjectsExt::ThisPlacedCT = tileSet.ConnectedTile[index];

        if (getOppositeDirection(CViewObjectsExt::LastPlacedCT.GetNextDirection(false))
            == CViewObjectsExt::LastPlacedCT.GetNextDirection(true)) // straight track
        {
            if (CViewObjectsExt::LastPlacedCT.Index == CViewObjectsExt::ThisPlacedCT.Index)
            {
                if (CViewObjectsExt::LastPlacedCT.GetNextDirection() != facing)
                {
                    handleExit();
                    return;
                }
            }
        }

        if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 1)
        {
            offsetConnectX -= 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 2)
        {
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 3)
        {
            offsetConnectX += 1;
            offsetConnectY += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 4)
        {
            offsetConnectX += 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 5)
        {
            offsetConnectX += 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 6)
        {
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 7)
        {
            offsetConnectX -= 1;
            offsetConnectY -= 1;
        }
        else if (tileSet.ConnectedTile[index].GetNextDirection(opposite) == 0)
        {
            offsetConnectX -= 1;
        }

        std::map<int, byte> tmpOverlayDatas;
        std::map<int, word> tmpOverlays;
        if (0 <= CViewObjectsExt::CliffConnectionTile &&
            (!ExtConfigs::ExtOverlays && CViewObjectsExt::CliffConnectionTile < 256)
            || (ExtConfigs::ExtOverlays && CViewObjectsExt::CliffConnectionTile < 65536))
        {
            int repeat = 1;
            if (facing == 0 && index == 2)
            {
                repeat = std::max(abs(CViewObjectsExt::CliffConnectionCoord.X - cursor.X), 1);
                for (int i = 0; i < repeat; ++i)
                {
                    int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX - i;
                    if (pos >= 0 && pos < CMapData::Instance->CellDataCount)
                    {
                        tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                        tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                    }
                }
            }
            else if (facing == 4 && index == 2)
            {
                repeat = std::max(abs(CViewObjectsExt::CliffConnectionCoord.X - cursor.X), 1);
                for (int i = 0; i < repeat; ++i)
                {
                    int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + i;
                    if (pos >= 0 && pos < CMapData::Instance->CellDataCount)
                    {
                        tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                        tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                    }
                }
            }
            else if (facing == 2 && index == 3)
            {
                repeat = std::max(abs(CViewObjectsExt::CliffConnectionCoord.Y - cursor.Y), 1);
                for (int i = 0; i < repeat; ++i)
                {
                    int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + i) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX;
                    if (pos >= 0 && pos < CMapData::Instance->CellDataCount)
                    {
                        tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                        tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                    }
                }
            }
            else if (facing == 6 && index == 3)
            {
                repeat = std::max(abs(CViewObjectsExt::CliffConnectionCoord.Y - cursor.Y), 1);
                for (int i = 0; i < repeat; ++i)
                {
                    int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY - i) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX;
                    if (pos >= 0 && pos < CMapData::Instance->CellDataCount)
                    {
                        tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                        tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                    }
                }
            }
            else if (facing == 1 && index == 1)
            {
                repeat = std::max(abs(((CViewObjectsExt::CliffConnectionCoord.X - CViewObjectsExt::CliffConnectionCoord.Y)
                    - (cursor.X - cursor.Y)) / 2), 1);
                for (int i = 0; i < repeat; ++i)
                {
                    int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + i) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX - i;
                    if (pos >= 0 && pos < CMapData::Instance->CellDataCount)
                    {
                        tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                        tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                    }
                }
            }
            else if (facing == 5 && index == 1)
            {
                repeat = std::max(abs(((CViewObjectsExt::CliffConnectionCoord.X - CViewObjectsExt::CliffConnectionCoord.Y)
                    - (cursor.X - cursor.Y)) / 2), 1);
                for (int i = 0; i < repeat; ++i)
                {
                    int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY - i) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + i;
                    if (pos >= 0 && pos < CMapData::Instance->CellDataCount)
                    {
                        tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                        tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                    }
                }
            }
            else if (facing == 7 && index == 0)
            {
                repeat = std::max(abs(((CViewObjectsExt::CliffConnectionCoord.X + CViewObjectsExt::CliffConnectionCoord.Y)
                    - (cursor.X + cursor.Y)) / 2), 1);
                for (int i = 0; i < repeat; ++i)
                {
                    int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY - i) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX - i;
                    if (pos >= 0 && pos < CMapData::Instance->CellDataCount)
                    {
                        tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                        tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                    }
                }
            }
            else if (facing == 3 && index == 0)
            {
                repeat = std::max(abs(((CViewObjectsExt::CliffConnectionCoord.X + CViewObjectsExt::CliffConnectionCoord.Y)
                    - (cursor.X + cursor.Y)) / 2), 1);
                for (int i = 0; i < repeat; ++i)
                {
                    int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY + i) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX + i;
                    if (pos >= 0 && pos < CMapData::Instance->CellDataCount)
                    {
                        tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                        tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                    }
                }
            }
            else
            {
                int pos = (CliffConnectionCoord.Y - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX;
                if (pos >= 0 && pos < CMapData::Instance->CellDataCount)
                {
                    tmpOverlayDatas[pos] = CMapData::Instance->GetCellAt(pos)->OverlayData;
                    tmpOverlays[pos] = CMapDataExt::CellDataExts[pos].NewOverlay;
                }
            }

            if (place && !AutoConnect::g_VirtualPlacing)
            {
                int l = CMapData::Instance->MapWidthPlusHeight; int t = CMapData::Instance->MapWidthPlusHeight; int r = 0; int b = 0;
                for (const auto& [pos, _] : tmpOverlays)
                {
                    int x = CMapData::Instance->GetXFromCoordIndex(pos);
                    int y = CMapData::Instance->GetYFromCoordIndex(pos);
                    if (x < l) l = x;
                    if (y < t) t = y;
                    if (x > r) r = x;
                    if (y > b) b = y;
                }
                // expand 1 size for ore
                mapData.SaveUndoRedoData(true,
                    l - 1,
                    t - 1,
                    r + 2,
                    b + 2
                );
            }

            if (AutoConnect::g_VirtualPlacing && AutoConnect::g_pCurrentSegment)
            {
                // virtual placement: collect overlay writes into the trial segment
                for (const auto& [pos, _] : tmpOverlays)
                {
                    AutoConnect::g_pCurrentSegment->Overlays[pos] = { CViewObjectsExt::CliffConnectionTile, 0 };
                }
            }
            else
            {
                for (const auto& [pos, _] : tmpOverlays)
                {
                    CMapDataExt::GetExtension()->SetNewOverlayAt(pos, CViewObjectsExt::CliffConnectionTile);
                    CMapData::Instance->SetOverlayDataAt(pos, 0);
                    if (AutoConnect::g_pCurrentManualBatch)
                        AutoConnect::g_pCurrentManualBatch->Overlays[pos] = { CViewObjectsExt::CliffConnectionTile, 0 };
                }
            }

            offsetConnectX *= repeat;
            offsetConnectY *= repeat;
            if (!AutoConnect::g_VirtualPlacing)
                ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);

            if (place)
            {
                if (CViewObjectsExt::PlaceConnectedTile_Start)
                    CViewObjectsExt::PlaceConnectedTile_Start = false;
                if (!AutoConnect::g_VirtualPlacing)
                {
                    CViewObjectsExt::LastPlacedCTRecords.push_back(CViewObjectsExt::LastPlacedCT);
                    CViewObjectsExt::CliffConnectionCoordRecords.push_back(CViewObjectsExt::CliffConnectionCoord);
                    CViewObjectsExt::LastCTTileRecords.push_back(CViewObjectsExt::CliffConnectionTile);
                    CViewObjectsExt::LastHeightRecords.push_back(CViewObjectsExt::CliffConnectionHeight);
                }
                CViewObjectsExt::LastCTTile = CViewObjectsExt::CliffConnectionTile;

                CViewObjectsExt::LastPlacedCT = CViewObjectsExt::ThisPlacedCT;
                CViewObjectsExt::LastPlacedCT.Opposite = opposite;
                CViewObjectsExt::NextCTHeightOffset = 0;

                if (opposite)
                {
                    CViewObjectsExt::CliffConnectionCoord.X += offsetConnectX + offsetPlaceX + CViewObjectsExt::LastPlacedCT.ConnectionPoint0.X;
                    CViewObjectsExt::CliffConnectionCoord.Y += offsetConnectY + offsetPlaceY + CViewObjectsExt::LastPlacedCT.ConnectionPoint0.Y;
                }
                else
                {
                    CViewObjectsExt::CliffConnectionCoord.X += offsetConnectX + offsetPlaceX + CViewObjectsExt::LastPlacedCT.ConnectionPoint1.X;
                    CViewObjectsExt::CliffConnectionCoord.Y += offsetConnectY + offsetPlaceY + CViewObjectsExt::LastPlacedCT.ConnectionPoint1.Y;
                }

                if (!AutoConnect::g_VirtualPlacing && AutoConnect::ClosureFirstPending)
                {
                    // a real manual placement locks the flow's first tile,
                    // overriding any staged virtual plan
                    AutoConnect::ClosureFirstRecorded = true;
                    AutoConnect::ClosureFirstPending = false;
                    AutoConnect::ClosureFirstIndex = CViewObjectsExt::LastPlacedCT.Index;
                    AutoConnect::ClosureFirstOpposite = CViewObjectsExt::LastPlacedCT.Opposite;
                    AutoConnect::ClosureFirstCoord = CViewObjectsExt::CliffConnectionCoord;
                    AutoConnect::ClosureFirstVariant = CViewObjectsExt::CliffConnectionTile;
                    if (AutoConnect::g_pCurrentManualBatch)
                    {
                        AutoConnect::ClosureFirstCellData.clear();
                        AutoConnect::ClosureFirstOverlays.clear();
                        AutoConnect::ClosureFirstBodyCells.clear();
                        for (auto& [pos, cd] : AutoConnect::g_pCurrentManualBatch->Cells)
                        {
                            AutoConnect::ClosureFirstCellData[pos] = cd;
                            AutoConnect::ClosureFirstBodyCells.insert(pos);
                        }
                        for (auto& [pos, cd] : AutoConnect::g_pCurrentManualBatch->FixCells)
                            AutoConnect::ClosureFirstCellData[pos] = cd;
                        for (auto& [pos, ov] : AutoConnect::g_pCurrentManualBatch->Overlays)
                            AutoConnect::ClosureFirstOverlays[pos] = ov;
                    }
                }
            }
            else
            {
                for (const auto& [pos, ovr] : tmpOverlays)
                {
                    CMapDataExt::GetExtension()->SetNewOverlayAt(pos, ovr);
                }
                for (const auto& [pos, overd] : tmpOverlayDatas)
                {
                    CMapData::Instance->SetOverlayDataAt(pos, overd);
                }
            }
        }

        handleExit();
        return;
    }

    if (CViewObjectsExt::NextCTHeightOffset <= 0 && !thisTileHeightOffest)
    {
        CViewObjectsExt::HeightChanged = false;
        CViewObjectsExt::CliffConnectionHeightAdjust = 0;
    }

    if (CViewObjectsExt::NextCTHeightOffset < 0 && thisTileHeightOffest)
    {
        CViewObjectsExt::CliffConnectionHeightAdjust = -1;
        CViewObjectsExt::HeightChanged = true;
    }

    if (CViewObjectsExt::LastPlacedCT.GetNextHeightOffset() > 0)
    {
        CViewObjectsExt::CliffConnectionHeightAdjust = 1;
        CViewObjectsExt::HeightChanged = true;
    }

    if (CViewObjectsExt::CliffConnectionHeightAdjust == 1 && thisTileHeightOffest && CViewObjectsExt::NextCTHeightOffset < 0)
    {
        CViewObjectsExt::CliffConnectionHeightAdjust = 0;
        CViewObjectsExt::HeightChanged = true;
    }

    if (CViewObjectsExt::LastPlacedCT.GetNextHeightOffset() == 0 && !thisTileHeightOffest)
        CViewObjectsExt::CliffConnectionHeightAdjust = 0;

    int thisTileHeight = thisTile.Height;
    int thisTileWidth = thisTile.Width;
    int HorizontalLoop = 1;

    if (MultiPlaceDirection == 0 || MultiPlaceDirection == 4)
    {
        if (opposite)
            offsetPlaceX -= thisTileHeight * (distanceX - 1);
        else
            offsetConnectX += thisTileHeight * (distanceX - 1);
        thisTileHeight *= distanceX;
    }
    if (MultiPlaceDirection == 2 || MultiPlaceDirection == 6)
    {
        if (opposite)
            offsetPlaceY -= thisTileWidth * (distanceY - 1);
        else
            offsetConnectY += thisTileWidth * (distanceY - 1);
        HorizontalLoop = distanceY / thisTileWidth;
    }


    if (place && !AutoConnect::g_VirtualPlacing
        && tileSet.Type != ConnectedTileSetTypes::Cliff && tileSet.Type != ConnectedTileSetTypes::CityCliff && tileSet.Type != ConnectedTileSetTypes::IceCliff)
    {
        if (MultiPlaceDirection == 0)
            mapData.SaveUndoRedoData(true,
                CViewObjectsExt::CliffConnectionCoord.X - thisTileHeight - 1,
                CViewObjectsExt::CliffConnectionCoord.Y - thisTile.Width - 1,
                CViewObjectsExt::CliffConnectionCoord.X + thisTile.Height + 1,
                CViewObjectsExt::CliffConnectionCoord.Y + thisTile.Width + 1
            );
        else if (MultiPlaceDirection == 4)
            mapData.SaveUndoRedoData(true,
                CViewObjectsExt::CliffConnectionCoord.X - thisTile.Height - 1,
                CViewObjectsExt::CliffConnectionCoord.Y - thisTile.Width - 1,
                CViewObjectsExt::CliffConnectionCoord.X + thisTileHeight + 1,
                CViewObjectsExt::CliffConnectionCoord.Y + thisTile.Width + 1
            );
        else if (MultiPlaceDirection == 2)
            mapData.SaveUndoRedoData(true,
                CViewObjectsExt::CliffConnectionCoord.X - thisTile.Height - 1,
                CViewObjectsExt::CliffConnectionCoord.Y - thisTile.Width - 1,
                CViewObjectsExt::CliffConnectionCoord.X + thisTile.Height + 1,
                CViewObjectsExt::CliffConnectionCoord.Y + thisTile.Width * HorizontalLoop + 1
            );
        else if (MultiPlaceDirection == 6)
            mapData.SaveUndoRedoData(true,
                CViewObjectsExt::CliffConnectionCoord.X - thisTile.Height - 1,
                CViewObjectsExt::CliffConnectionCoord.Y - thisTile.Width * HorizontalLoop - 1,
                CViewObjectsExt::CliffConnectionCoord.X + thisTile.Height + 1,
                CViewObjectsExt::CliffConnectionCoord.Y + thisTile.Width + 1
            );
        else
            mapData.SaveUndoRedoData(true,
                CViewObjectsExt::CliffConnectionCoord.X - thisTile.Height - 1,
                CViewObjectsExt::CliffConnectionCoord.Y - thisTile.Width - 1,
                CViewObjectsExt::CliffConnectionCoord.X + thisTile.Height + 1,
                CViewObjectsExt::CliffConnectionCoord.Y + thisTile.Width + 1
            );
    }
    for (int k = 0; k < HorizontalLoop; k++)
    {
        for (int i = 0; i < thisTileHeight; i++)
        {
            for (int j = 0 + thisTileWidth * k; j < thisTileWidth * (k + 1); j++)
            {
                if (thisTile.TileBlockDatas[subPos].ImageData != NULL)
                {
                    int dwpos, x, y;
                    if (opposite)
                    {
                        x = CliffConnectionCoord.X + i - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX;
                        y = CliffConnectionCoord.Y + j - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY;

                    }
                    else
                    {
                        x = CliffConnectionCoord.X + i - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX;
                        y = CliffConnectionCoord.Y + j - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY;
                    }
                    if (CMapDataExt::IsCoordInFullMap(x, y))
                    {
                        dwpos = y * mapData.MapWidthPlusHeight + x;
                        AutoConnect_WriteCell(tmpCellDatas, cellDatas, dwpos, CViewObjectsExt::CliffConnectionTile, subPos,
                            CViewObjectsExt::CliffConnectionHeight + thisTile.TileBlockDatas[subPos].Height + CViewObjectsExt::CliffConnectionHeightAdjust);
                    }
                }
                subPos++;

                if (subPos >= thisTile.TileBlockCount)
                    subPos = 0;
            }
        }
    }
    if (!AutoConnect::g_VirtualPlacing)
        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);

    if (!place)
    {
        //undo
        subPos = 0;
        if (dwposFix > -1)
        {
            if (dwposFix >= 0 && dwposFix < mapData.CellDataCount && tmpCellDatas.find(dwposFix) != tmpCellDatas.end())
            {
                cellDatas[dwposFix] = tmpCellDatas[dwposFix];
            }
        }
        if (dwposFix2 > -1)
        {
            if (dwposFix2 >= 0 && dwposFix2 < mapData.CellDataCount && tmpCellDatas.find(dwposFix2) != tmpCellDatas.end())
            {
                cellDatas[dwposFix2] = tmpCellDatas[dwposFix2];
            }
        }
        for (int k = 0; k < HorizontalLoop; k++)
        {
            for (int i = 0; i < thisTileHeight; i++)
            {
                for (int j = 0 + thisTileWidth * k; j < thisTileWidth * (k + 1); j++)
                {
                    if (thisTile.TileBlockDatas[subPos].ImageData != NULL)
                    {
                        int dwpos, x, y;
                        if (opposite)
                        {
                            x = CliffConnectionCoord.X + i - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX;
                            y = CliffConnectionCoord.Y + j - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY;

                        }
                        else
                        {
                            x = CliffConnectionCoord.X + i - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX;
                            y = CliffConnectionCoord.Y + j - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY;
                        }
                        if (CMapDataExt::IsCoordInFullMap(x, y))
                        {
                            dwpos = y * mapData.MapWidthPlusHeight + x;
                            if (tmpCellDatas.find(dwpos) != tmpCellDatas.end())
                            {
                                cellDatas[dwpos] = tmpCellDatas[dwpos];
                            }
                        }
                    }
                    subPos++;

                    if (subPos >= thisTile.TileBlockCount)
                        subPos = 0;
                }
            }
        }
    }

    if (place)
    {
        //update mapPreview
        if (CViewObjectsExt::PlaceConnectedTile_Start)
            CViewObjectsExt::PlaceConnectedTile_Start = false;
        subPos = 0;
        int idx = 0;
        if (!AutoConnect::g_VirtualPlacing)
        {
            if (dwposFix > -1)
            {
                CMapData::Instance->UpdateMapPreviewAt(dwposFix % mapData.MapWidthPlusHeight, dwposFix / mapData.MapWidthPlusHeight);
                idx++;
            }
            if (dwposFix2 > -1)
            {
                CMapData::Instance->UpdateMapPreviewAt(dwposFix2 % mapData.MapWidthPlusHeight, dwposFix2 / mapData.MapWidthPlusHeight);
                idx++;
            }
            for (int k = 0; k < HorizontalLoop; k++)
            {
                for (int i = 0; i < thisTileHeight; i++)
                {
                    for (int j = 0 + thisTileWidth * k; j < thisTileWidth * (k + 1); j++)
                    {
                        if (thisTile.TileBlockDatas[subPos].ImageData != NULL)
                        {
                            int dwpos;
                            if (opposite)
                            {
                                dwpos = (CliffConnectionCoord.Y + j - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.Y + offsetPlaceY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X + i - CViewObjectsExt::ThisPlacedCT.ConnectionPoint1.X + offsetPlaceX;
                            }
                            else
                            {
                                dwpos = (CliffConnectionCoord.Y + j - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.Y + offsetPlaceY) * mapData.MapWidthPlusHeight + CliffConnectionCoord.X + i - CViewObjectsExt::ThisPlacedCT.ConnectionPoint0.X + offsetPlaceX;
                            }
                            CMapData::Instance->UpdateMapPreviewAt(dwpos % mapData.MapWidthPlusHeight, dwpos / mapData.MapWidthPlusHeight);
                            idx++;
                        }
                        subPos++;

                        if (subPos >= thisTile.TileBlockCount)
                            subPos = 0;
                    }
                }
            }
        }


        if (!AutoConnect::g_VirtualPlacing)
        {
            CViewObjectsExt::LastPlacedCTRecords.push_back(CViewObjectsExt::LastPlacedCT);
            CViewObjectsExt::CliffConnectionCoordRecords.push_back(CViewObjectsExt::CliffConnectionCoord);
            CViewObjectsExt::LastCTTileRecords.push_back(CViewObjectsExt::CliffConnectionTile);
            CViewObjectsExt::LastHeightRecords.push_back(CViewObjectsExt::CliffConnectionHeight);
        }
        CViewObjectsExt::LastCTTile = CViewObjectsExt::CliffConnectionTile;

        CViewObjectsExt::LastPlacedCT = CViewObjectsExt::ThisPlacedCT;
        CViewObjectsExt::LastPlacedCT.Opposite = opposite;
        CViewObjectsExt::LastSuccessfulHeightOffset = CViewObjectsExt::CliffConnectionHeightAdjust;
        CViewObjectsExt::NextCTHeightOffset = 0;
        CViewObjectsExt::HeightChanged = false;

        CViewObjectsExt::CliffConnectionHeight += CViewObjectsExt::CliffConnectionHeightAdjust;

        if (opposite)
        {
            CViewObjectsExt::CliffConnectionCoord.X += offsetConnectX + offsetPlaceX + CViewObjectsExt::LastPlacedCT.ConnectionPoint0.X;
            CViewObjectsExt::CliffConnectionCoord.Y += offsetConnectY + offsetPlaceY + CViewObjectsExt::LastPlacedCT.ConnectionPoint0.Y;
        }
        else
        {
            CViewObjectsExt::CliffConnectionCoord.X += offsetConnectX + offsetPlaceX + CViewObjectsExt::LastPlacedCT.ConnectionPoint1.X;
            CViewObjectsExt::CliffConnectionCoord.Y += offsetConnectY + offsetPlaceY + CViewObjectsExt::LastPlacedCT.ConnectionPoint1.Y;
        }

        if (!AutoConnect::g_VirtualPlacing && AutoConnect::ClosureFirstPending)
        {
            // a real manual placement locks the flow's first tile,
            // overriding any staged virtual plan
            AutoConnect::ClosureFirstRecorded = true;
            AutoConnect::ClosureFirstPending = false;
            AutoConnect::ClosureFirstIndex = CViewObjectsExt::LastPlacedCT.Index;
            AutoConnect::ClosureFirstOpposite = CViewObjectsExt::LastPlacedCT.Opposite;
            AutoConnect::ClosureFirstCoord = CViewObjectsExt::CliffConnectionCoord;
            AutoConnect::ClosureFirstVariant = CViewObjectsExt::CliffConnectionTile;
            if (AutoConnect::g_pCurrentManualBatch)
            {
                AutoConnect::ClosureFirstCellData.clear();
                AutoConnect::ClosureFirstOverlays.clear();
                AutoConnect::ClosureFirstBodyCells.clear();
                for (auto& [pos, cd] : AutoConnect::g_pCurrentManualBatch->Cells)
                {
                    AutoConnect::ClosureFirstCellData[pos] = cd;
                    AutoConnect::ClosureFirstBodyCells.insert(pos);
                }
                for (auto& [pos, cd] : AutoConnect::g_pCurrentManualBatch->FixCells)
                    AutoConnect::ClosureFirstCellData[pos] = cd;
                for (auto& [pos, ov] : AutoConnect::g_pCurrentManualBatch->Overlays)
                    AutoConnect::ClosureFirstOverlays[pos] = ov;
            }
        }
    }
    CViewObjectsExt::IsInPlaceCliff_OnMouseMove = false;
    return;
}

namespace AutoConnect
{
    // 8-direction unit vectors matching CMapDataExt::GetFacing
    // 0:(-1,0) 1:(-1,1) 2:(0,1) 3:(1,1) 4:(1,0) 5:(1,-1) 6:(0,-1) 7:(-1,-1)
    static const int DirX[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
    static const int DirY[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

    static double Dist(const MapCoord& a, const MapCoord& b)
    {
        double dx = (double)(b.X - a.X);
        double dy = (double)(b.Y - a.Y);
        return sqrt(dx * dx + dy * dy);
    }

    // signed lateral offset of p from the start->target line, in cells
    static double LateralOffset(const MapCoord& start, const MapCoord& target, const MapCoord& p)
    {
        double ex = (double)(target.X - start.X);
        double ey = (double)(target.Y - start.Y);
        double len = sqrt(ex * ex + ey * ey);
        if (len < 0.001)
            return 0.0;
        // cross product / length
        return ((double)(p.X - start.X) * ey - (double)(p.Y - start.Y) * ex) / len;
    }

    // the connected-tile set currently selected in the object browser, or null
    static ConnectedTileSet* GetCurrentTileSet()
    {
        if (CIsoView::CurrentCommand->Command != 0x1E)
            return nullptr;
        int ctIndex = CIsoView::CurrentCommand->Type;
        auto it = CViewObjectsExt::TreeView_ConnectedTileMap.find(ctIndex);
        if (it == CViewObjectsExt::TreeView_ConnectedTileMap.end())
            return nullptr;
        auto& info = it->second;
        if (info.Index < 0 || info.Index >= (int)CViewObjectsExt::ConnectedTileSets.size())
            return nullptr;
        return &CViewObjectsExt::ConnectedTileSets[info.Index];
    }

    static bool IsFourDirectionType()
    {
        auto pSet = GetCurrentTileSet();
        if (!pSet)
            return false;
        return pSet->Type == CViewObjectsExt::ConnectedTileSetTypes::Highway
            || pSet->Type == CViewObjectsExt::ConnectedTileSetTypes::PaveShore
            || pSet->Type == CViewObjectsExt::ConnectedTileSetTypes::SpecialPaveShore;
    }

    static bool IsRailRoadType()
    {
        auto pSet = GetCurrentTileSet();
        return pSet && pSet->Type == CViewObjectsExt::ConnectedTileSetTypes::RailRoad;
    }

    // build ordered direction candidates for the next segment; the ordering
    // implements the weaving behaviour: near the start->target line the path
    // keeps drifting to its current side, past the soft limit it starts
    // turning back, and past the hard limit returning is forced
    static std::vector<int> BuildCandidates(const MapCoord& start, const MapCoord& anchor,
        const MapCoord& target, bool fourDir, int lastDir, int lastDirRun,
        int lastVel, double soft, double hard)
    {
        std::vector<int> candidates;

        int ideal, s1, s2; // s1/s2: the two off-axis side options
        if (fourDir)
        {
            // 4-direction tile sets: only axis directions produce valid pieces;
            // wobble alternates between the axis direction and its perpendiculars
            ideal = CMapDataExt::GetFacing4(anchor, target); // 0/2/4/6
            s1 = (ideal + 2) % 8;
            s2 = (ideal + 6) % 8;
        }
        else
        {
            ideal = CMapDataExt::GetFacing(anchor, target);
            s1 = (ideal + 1) % 8;
            s2 = (ideal + 7) % 8;
        }

        // probe which side option reduces the lateral offset from the line
        double lat = LateralOffset(start, target, anchor);
        double lat1 = LateralOffset(start, target, { anchor.X + DirX[s1], anchor.Y + DirY[s1] });
        double lat2 = LateralOffset(start, target, { anchor.X + DirX[s2], anchor.Y + DirY[s2] });
        int home = (fabs(lat1) <= fabs(lat2)) ? s1 : s2; // back towards the line
        int away = (home == s1) ? s2 : s1;

        double alat = fabs(lat);
        if (alat >= hard)
        {
            // too far off the line: steer back
            candidates.push_back(home);
            candidates.push_back(ideal);
        }
        else if (alat >= soft)
        {
            // begin the turn back, but not always immediately
            if (STDHelpers::RandomSelectInt(0, 9) < 7)
            {
                candidates.push_back(home);
                candidates.push_back(ideal);
                candidates.push_back(away);
            }
            else
            {
                candidates.push_back(ideal);
                candidates.push_back(home);
                candidates.push_back(away);
            }
        }
        else
        {
            // near the line: keep drifting through it in the current lateral
            // direction so the path weaves back and forth; flip every now and
            // then so the weave never settles into a repeating pattern
            int lead;
            if (lastVel > 0)
                lead = (lat1 > lat2) ? s1 : s2;
            else if (lastVel < 0)
                lead = (lat1 > lat2) ? s2 : s1;
            else
                lead = (STDHelpers::RandomSelectInt(0, 1) == 0) ? s1 : s2;
            if (STDHelpers::RandomSelectInt(0, 9) < 2)
                lead = (lead == s1) ? s2 : s1; // occasional pattern break

            if (STDHelpers::RandomSelectInt(0, 9) < 6)
            {
                candidates.push_back(lead);
                candidates.push_back(ideal);
            }
            else
            {
                candidates.push_back(ideal);
                candidates.push_back(lead);
            }
            candidates.push_back((lead == s1) ? s2 : s1);
        }

        // repeat penalty: if the last two segments went the same way, deprioritize it
        if (lastDir >= 0 && lastDirRun >= 2)
        {
            for (size_t i = 0; i < candidates.size(); ++i)
            {
                if (candidates[i] == lastDir && i < candidates.size() - 1)
                {
                    candidates.erase(candidates.begin() + i);
                    candidates.push_back(lastDir);
                    break;
                }
            }
        }
        return candidates;
    }

    // try to place one virtual segment towards 'dir'; returns true when the
    // segment was accepted (cells collected, anchor advanced); forbiddenSingleCT
    // rejects a segment that would repeat the given 1-variant tile
    static bool TrySegment(int dir, const MapCoord& target, int segLenMin, int segLenMax,
        Session& S, int forbiddenSingleCT = -1, double weaveAmplitude = 3.0,
        bool allowDetour = false)
    {
        int ax = CViewObjectsExt::CliffConnectionCoord.X;
        int ay = CViewObjectsExt::CliffConnectionCoord.Y;

        MapCoord anchor{ ax, ay };
        double d = Dist(anchor, target);
        int len = segLenMin + STDHelpers::RandomSelectInt(0, segLenMax - segLenMin);
        if ((double)len > d)
            len = (int)d;
        if (len < 1)
            len = 1;

        MapCoord vt{ ax + DirX[dir] * len, ay + DirY[dir] * len };
        if (!CMapDataExt::IsCoordInFullMap(vt.X, vt.Y))
        {
            vt = target; // fall back to a direct shot
            if (Dist(anchor, vt) < 0.5)
                return false;
        }

        Segment seg;
        CaptureState(seg.Before);

        AutoConnect::g_pCurrentSegment = &seg;
        AutoConnect::g_VirtualPlacing = true;
        CViewObjectsExt::PlaceConnectedTile_OnMouseMove(vt.X, vt.Y, true);
        AutoConnect::g_VirtualPlacing = false;
        AutoConnect::g_pCurrentSegment = nullptr;

        // rejected inside the placement logic (no suitable tile)?
        if (CViewObjectsExt::CliffConnectionCoord.X == ax && CViewObjectsExt::CliffConnectionCoord.Y == ay)
        {
            RestoreState(seg.Before);
            return false;
        }

        MapCoord newAnchor = CViewObjectsExt::CliffConnectionCoord;

        // The placement must actually move in the requested direction. Some
        // fallback paths (e.g. highway/rail with no reverse tile) otherwise
        // "succeed" by continuing straight in the old direction, which makes
        // AutoConnect run away from the target before turning around.
        int deltaX = newAnchor.X - ax;
        int deltaY = newAnchor.Y - ay;
        if (deltaX * DirX[dir] + deltaY * DirY[dir] < 0)
        {
            RestoreState(seg.Before);
            return false;
        }

        // Also detect the highway/rail "no suitable bend" fallback: if the
        // previous tile continues in one direction but we asked for another,
        // and the placed tile is the same index, it actually just repeated the
        // old straight instead of turning.
        if (seg.Before.LastCT.Index >= 0)
        {
            int currentDir = seg.Before.LastCT.GetNextDirection();
            if (currentDir != dir && CViewObjectsExt::LastPlacedCT.Index == seg.Before.LastCT.Index)
            {
                RestoreState(seg.Before);
                return false;
            }
        }

        // must make progress towards the target; detour segments may temporarily
        // move away from it, but only by a bounded amount so the path cannot
        // run arbitrarily far in the wrong direction
        double newDist = Dist(newAnchor, target);
        if (newDist >= d)
        {
            if (!allowDetour || newDist >= d + 3.0)
            {
                RestoreState(seg.Before);
                return false;
            }
        }

        // visual repetition guard: a 1-variant piece may not follow itself
        if (forbiddenSingleCT != -1)
        {
            auto& placedCT = CViewObjectsExt::LastPlacedCT;
            if (placedCT.TileIndices.size() == 1 && placedCT.TileIndices[0] == forbiddenSingleCT)
            {
                RestoreState(seg.Before);
                return false;
            }
        }

        // spindle corridor: the allowed lateral offset from the start->target
        // line is widest halfway and tight near both ends, so the path weaves
        // around the line instead of bulging to one side; scaled by the tile
        // set's weave amplitude
        double traveled = Dist(S.Start, anchor);
        double allowed = std::min(weaveAmplitude * 2.0, weaveAmplitude * 0.8 + 0.12 * std::min(traveled, d));
        double lateral = fabs(LateralOffset(S.Start, target, newAnchor));
        if (lateral > allowed && !allowDetour)
        {
            RestoreState(seg.Before);
            return false;
        }

        // overlap check across both the current batch and previously committed
        // batches: neither tile bodies nor junction patches may land on a cell
        // an earlier segment already wrote.
        auto cellTaken = [&](int pos)
        {
            return S.MergedCells.find(pos) != S.MergedCells.end()
                || S.MergedFixCells.find(pos) != S.MergedFixCells.end()
                || S.CommittedCells.find(pos) != S.CommittedCells.end()
                || S.CommittedFixCells.find(pos) != S.CommittedFixCells.end();
        };
        for (auto& [pos, _] : seg.Cells)
        {
            if (cellTaken(pos))
            {
                RestoreState(seg.Before);
                return false;
            }
        }
        for (auto& [pos, _] : seg.FixCells)
        {
            if (seg.Cells.find(pos) != seg.Cells.end() || cellTaken(pos))
            {
                RestoreState(seg.Before);
                return false;
            }
        }
        for (auto& [pos, _] : seg.Overlays)
        {
            bool taken = false;
            for (auto& prevSeg : S.Segments)
            {
                if (prevSeg.Overlays.find(pos) != prevSeg.Overlays.end())
                {
                    taken = true;
                    break;
                }
            }
            if (!taken && S.CommittedOverlays.find(pos) != S.CommittedOverlays.end())
                taken = true;
            if (taken)
            {
                RestoreState(seg.Before);
                return false;
            }
        }

        // accepted
        if (seg.Before.PlaceStart && !ClosureFirstRecorded)
        {
            // capture the flow's very first tile: its connection triple plus
            // the exact cells it wrote, so a closing path can land on it
            ClosureFirstRecorded = true;
            ClosureFirstIndex = CViewObjectsExt::LastPlacedCT.Index;
            ClosureFirstOpposite = CViewObjectsExt::LastPlacedCT.Opposite;
            ClosureFirstCoord = CViewObjectsExt::CliffConnectionCoord;
            ClosureFirstVariant = CViewObjectsExt::CliffConnectionTile;
            ClosureFirstCellData.clear();
            ClosureFirstOverlays.clear();
            ClosureFirstBodyCells.clear();
            for (auto& [pos, cd] : seg.Cells)
            {
                ClosureFirstCellData[pos] = cd;
                ClosureFirstBodyCells.insert(pos);
            }
            for (auto& [pos, cd] : seg.FixCells)
                ClosureFirstCellData[pos] = cd;
            for (auto& [pos, ov] : seg.Overlays)
                ClosureFirstOverlays[pos] = ov;
        }
        for (auto& [pos, cd] : seg.Cells)
            S.MergedCells[pos] = cd;
        for (auto& [pos, cd] : seg.FixCells)
            S.MergedFixCells[pos] = cd;
        S.Segments.push_back(std::move(seg));
        return true;
    }

    // plan a fresh path from the session start point to (X, Y)
    static void PlanSession(int X, int Y)
    {
        g_ClosurePlanActive = false;
        auto& S = g_Session;
        S.Target = { X, Y };
        S.Segments.clear();
        S.MergedCells.clear();
        S.MergedFixCells.clear();

        // restart planning from the session start state
        RestoreState(S.SessionBefore);

        // The plan is rebuilt from scratch on every mouse move, and the first
        // tile is re-rolled each time. Any previously captured "first tile"
        // data belongs to a discarded plan, so drop it while this plan still
        // starts the flow (later chained batches keep the flow's first tile).
        if (CViewObjectsExt::PlaceConnectedTile_Start)
        {
            ClosureFirstRecorded = false;
            ClosureFirstCellData.clear();
            ClosureFirstOverlays.clear();
            ClosureFirstBodyCells.clear();
        }

        bool fourDir = IsFourDirectionType();
        bool rail = IsRailRoadType();

        // per-type planning parameters (INI-overridable)
        auto pSet = GetCurrentTileSet();
        double amp = pSet ? pSet->AutoWeaveAmplitude : 3.0;
        bool singlePenalty = !pSet || pSet->AutoSingleVariantPenalty;
        int backtrackSteps = pSet ? pSet->AutoBacktrackSteps : 5;

        int segLenMin = 4, segLenMax = 8;
        if (fourDir) { segLenMin = 3; segLenMax = 5; }     // short runs make the wobble visible
        else if (rail) { segLenMin = 4; segLenMax = 6; }

        double d0 = Dist(S.Start, S.Target);
        int maxIter = (int)(d0 * 3.0) + 64;

        int lastDir = -1;
        if (CViewObjectsExt::LastPlacedCT.Index >= 0)
            lastDir = CViewObjectsExt::LastPlacedCT.GetNextDirection();
        int lastDirRun = 0;
        int lastVel = 0;              // lateral drift direction of the last segment
        int lastSingleCT = -1;        // tile index when the last segment placed a 1-variant piece
        int backtrackBudget = 3;      // total backtrack attempts per plan
        int detourBudget = 8;         // temporary non-progress/corridor-recovery segments allowed for U-turns/detours

        // weave amplitude: tight for short paths, wider for long ones
        double soft = std::clamp(amp * 0.5 + d0 * 0.02, amp * 0.5, amp);
        double hard = soft * 2.0;

        for (int iter = 0; iter < maxIter; ++iter)
        {
            MapCoord anchor = CViewObjectsExt::CliffConnectionCoord;
            double d = Dist(anchor, S.Target);
            if (d <= 2.0)
                break;

            // forced straight convergence near the target
            bool forceStraight = (d <= 8.0);

            std::vector<int> candidates;
            if (forceStraight)
            {
                if (fourDir)
                    candidates.push_back(CMapDataExt::GetFacing4(anchor, S.Target));
                else
                    candidates.push_back(CMapDataExt::GetFacing(anchor, S.Target));
            }
            else
            {
                candidates = BuildCandidates(S.Start, anchor, S.Target, fourDir,
                    lastDir, lastDirRun, lastVel, soft, hard);
            }

            double latBefore = LateralOffset(S.Start, S.Target, anchor);

            bool accepted = false;
            int chosenDir = -1;
            // pass 0 avoids repeating the last 1-variant piece; pass 1 drops
            // that penalty so reachability always wins over variety
            for (int pass = 0; pass < 2 && !accepted; ++pass)
            {
                int forbidCT = (singlePenalty && pass == 0) ? lastSingleCT : -1;
                for (int dir : candidates)
                {
                    if (TrySegment(dir, S.Target, segLenMin, segLenMax, S, forbidCT, amp))
                    {
                        accepted = true;
                        chosenDir = dir;
                        break;
                    }
                }
            }

            if (!accepted && detourBudget > 0)
            {
                // normal forward progress is impossible (e.g. target is behind us).
                // allow a short detour: try the current heading and side directions
                // with relaxed distance/corridor checks so the path can loop back.
                std::vector<int> detourCandidates;
                auto addDir = [&detourCandidates](int dir)
                {
                    for (int c : detourCandidates)
                    {
                        if (c == dir)
                            return;
                    }
                    detourCandidates.push_back(dir);
                };
                int ideal = fourDir
                    ? CMapDataExt::GetFacing4(anchor, S.Target)
                    : CMapDataExt::GetFacing(anchor, S.Target);
                // Walk along the shorter arc from the current heading to the
                // target direction, trying the target-side first. This makes the
                // detour turn toward the target as soon as possible instead of
                // continuing straight in the old direction.
                if (lastDir >= 0 && lastDir != ideal)
                {
                    int diff = (ideal - lastDir + 8) % 8;
                    int revStep = (diff <= 4) ? -1 : 1;
                    int cur = ideal;
                    while (cur != lastDir)
                    {
                        addDir(cur);
                        cur = (cur + revStep + 8) % 8;
                    }
                }
                addDir(ideal);
                if (lastDir >= 0)
                    addDir(lastDir);
                // Last resort: any remaining direction.
                for (int d = 0; d < 8; ++d)
                    addDir(d);

                for (int dir : detourCandidates)
                {
                    if (TrySegment(dir, S.Target, segLenMin, segLenMax, S, -1, amp, true))
                    {
                        accepted = true;
                        chosenDir = dir;
                        // Only consume detour budget when the segment actually
                        // moves away from the target. Corridor-recovery segments
                        // that still make progress should not exhaust the budget.
                        if (Dist(CViewObjectsExt::CliffConnectionCoord, S.Target) >= d)
                            detourBudget--;
                        break;
                    }
                }
            }

            if (!accepted)
            {
                // dead end: drop the last few segments and retry from there;
                // the reroll usually finds a different continuation
                if (backtrackSteps <= 0 || backtrackBudget <= 0 || S.Segments.empty())
                    break; // keep the segments placed so far

                int drop = std::min(backtrackSteps, (int)S.Segments.size());
                auto& keep = S.Segments[S.Segments.size() - drop];
                RestoreState(keep.Before);
                S.Segments.resize(S.Segments.size() - drop);
                if (S.Segments.empty() && CViewObjectsExt::PlaceConnectedTile_Start)
                {
                    // the first tile itself was dropped: the next reroll will
                    // place a new one, so its recorded data must be re-captured
                    ClosureFirstRecorded = false;
                    ClosureFirstCellData.clear();
                    ClosureFirstOverlays.clear();
                    ClosureFirstBodyCells.clear();
                }

                S.MergedCells.clear();
                S.MergedFixCells.clear();
                for (auto& seg : S.Segments)
                {
                    for (auto& [pos, cd] : seg.Cells)
                        S.MergedCells[pos] = cd;
                    for (auto& [pos, cd] : seg.FixCells)
                        S.MergedFixCells[pos] = cd;
                }

                // weave/repeat trackers start fresh after the backtrack
                if (CViewObjectsExt::LastPlacedCT.Index >= 0)
                    lastDir = CViewObjectsExt::LastPlacedCT.GetNextDirection();
                else
                    lastDir = -1;
                lastDirRun = 0;
                lastVel = 0;
                detourBudget = 8;
                auto& placedCT = CViewObjectsExt::LastPlacedCT;
                lastSingleCT = (placedCT.TileIndices.size() == 1) ? placedCT.TileIndices[0] : -1;
                backtrackBudget--;
                continue;
            }

            // track the lateral drift for the weaving logic
            double latAfter = LateralOffset(S.Start, S.Target, CViewObjectsExt::CliffConnectionCoord);
            if (latAfter > latBefore + 0.01)
                lastVel = 1;
            else if (latAfter < latBefore - 0.01)
                lastVel = -1;

            // track the placed piece for the 1-variant repeat penalty
            auto& placedCT = CViewObjectsExt::LastPlacedCT;
            lastSingleCT = (placedCT.TileIndices.size() == 1) ? placedCT.TileIndices[0] : -1;

            if (chosenDir == lastDir)
                lastDirRun++;
            else
            {
                lastDir = chosenDir;
                lastDirRun = 1;
            }
        }
    }

    // write the planned path onto the map (preview or commit), backing up
    // the real cell values the first time each position is touched; segments
    // are replayed in planning order (junction patch first, tile body second)
    // so the result matches a manual placement sequence exactly
    static void ApplyToMap(bool backup)
    {
        auto& S = g_Session;
        auto& mapData = CMapData::Instance();

        auto writeCell = [&](int pos, const CellData& cd)
        {
            if (pos < 0 || pos >= mapData.CellDataCount)
                return;
            if (backup)
                S.OldCells.emplace(pos, mapData.CellDatas[pos]);
            mapData.CellDatas[pos] = cd;
        };

        for (auto& seg : S.Segments)
        {
            for (auto& [pos, cd] : seg.FixCells)
                writeCell(pos, cd);
            for (auto& [pos, cd] : seg.Cells)
                writeCell(pos, cd);
        }
        for (auto& seg : S.Segments)
        {
            for (auto& [pos, ov] : seg.Overlays)
            {
                if (pos < 0 || pos >= mapData.CellDataCount)
                    continue;
                if (backup)
                {
                    if (S.OldOverlays.find(pos) == S.OldOverlays.end())
                    {
                        S.OldOverlays.emplace(pos, std::make_pair(
                            (int)CMapDataExt::CellDataExts[pos].NewOverlay,
                            (int)mapData.CellDatas[pos].OverlayData));
                    }
                }
                CMapDataExt::GetExtension()->SetNewOverlayAt(pos, ov.first);
                mapData.SetOverlayDataAt(pos, ov.second);
            }
        }
    }

    // revert the preview: restore overlays first, then whole cells
    static void RevertFromMap()
    {
        auto& S = g_Session;
        auto& mapData = CMapData::Instance();

        for (auto& [pos, ov] : S.OldOverlays)
        {
            if (pos < 0 || pos >= mapData.CellDataCount)
                continue;
            CMapDataExt::GetExtension()->SetNewOverlayAt(pos, ov.first);
            mapData.SetOverlayDataAt(pos, ov.second);
        }
        for (auto& [pos, cd] : S.OldCells)
        {
            if (pos < 0 || pos >= mapData.CellDataCount)
                continue;
            mapData.CellDatas[pos] = cd;
        }
    }

    // ===================== Closure planning =====================

    static bool ClosureTripleMatches()
    {
        return CViewObjectsExt::LastPlacedCT.Index == ClosureTargetIndex
            && CViewObjectsExt::LastPlacedCT.Opposite == ClosureTargetOpposite
            && CViewObjectsExt::CliffConnectionCoord.X == ClosureTargetCoord.X
            && CViewObjectsExt::CliffConnectionCoord.Y == ClosureTargetCoord.Y;
    }

    static void RebuildMerged(Session& S)
    {
        S.MergedCells.clear();
        S.MergedFixCells.clear();
        for (auto& seg : S.Segments)
        {
            for (auto& [pos, cd] : seg.Cells)
                S.MergedCells[pos] = cd;
            for (auto& [pos, cd] : seg.FixCells)
                S.MergedFixCells[pos] = cd;
        }
    }

    static void MergeSegment(Session& S, const Segment& seg)
    {
        for (auto& [pos, cd] : seg.Cells)
            S.MergedCells[pos] = cd;
        for (auto& [pos, cd] : seg.FixCells)
            S.MergedFixCells[pos] = cd;
    }

    static void UnmergeSegment(Session& S, const Segment& seg)
    {
        for (auto& [pos, cd] : seg.Cells)
            S.MergedCells.erase(pos);
        for (auto& [pos, cd] : seg.FixCells)
            S.MergedFixCells.erase(pos);
    }

    // overlap validation for one closure extension step. Only the final step
    // may overlap the flow's first tile: it is meant to land exactly on it.
    static bool ValidClosureStep(const Segment& seg, bool finalStep)
    {
        auto& S = g_Session;
        auto cellTaken = [&](int pos)
        {
            return S.MergedCells.find(pos) != S.MergedCells.end()
                || S.MergedFixCells.find(pos) != S.MergedFixCells.end()
                || S.CommittedCells.find(pos) != S.CommittedCells.end()
                || S.CommittedFixCells.find(pos) != S.CommittedFixCells.end();
        };
        auto relaxed = [&](int pos)
        {
            return finalStep && (ClosureFirstCellData.find(pos) != ClosureFirstCellData.end()
                || ClosureFirstOverlays.find(pos) != ClosureFirstOverlays.end());
        };

        for (auto& [pos, _] : seg.Cells)
        {
            if (cellTaken(pos) && !relaxed(pos))
                return false;
        }
        for (auto& [pos, _] : seg.FixCells)
        {
            if (cellTaken(pos) && !relaxed(pos))
                return false;
        }
        for (auto& [pos, _] : seg.Overlays)
        {
            bool taken = false;
            for (auto& prevSeg : S.Segments)
            {
                if (prevSeg.Overlays.find(pos) != prevSeg.Overlays.end())
                {
                    taken = true;
                    break;
                }
            }
            if (!taken && S.CommittedOverlays.find(pos) != S.CommittedOverlays.end())
                taken = true;
            if (taken && !relaxed(pos))
                return false;
        }
        return true;
    }

    // make the final closure tile bit-identical to the flow's first tile
    static void RestoreFirstTileData(Segment& seg)
    {
        for (auto& [pos, cd] : seg.Cells)
        {
            auto it = ClosureFirstCellData.find(pos);
            if (it != ClosureFirstCellData.end())
                cd = it->second;
        }
        for (auto& [pos, cd] : seg.FixCells)
        {
            auto it = ClosureFirstCellData.find(pos);
            if (it != ClosureFirstCellData.end())
                cd = it->second;
        }
        for (auto& [pos, ov] : seg.Overlays)
        {
            auto it = ClosureFirstOverlays.find(pos);
            if (it != ClosureFirstOverlays.end())
                ov = it->second;
        }
    }

    // the final closure tile must occupy exactly the same cells as the flow's
    // first tile: that is what "fully overlapping" means. A triple match alone
    // can leave the footprints shifted (e.g. by the first tile's start
    // compensation), which would look like a mismatched piece.
    static bool ClosureFootprintMatches(const Segment& seg)
    {
        if (seg.Cells.size() != ClosureFirstBodyCells.size())
            return false;
        for (auto& [pos, _] : seg.Cells)
        {
            if (ClosureFirstBodyCells.find(pos) == ClosureFirstBodyCells.end())
                return false;
        }
        return true;
    }

    // one virtual extension placement honouring the current forced-choice state
    static bool ClosurePlace(Segment& seg, MapCoord anchor0)
    {
        if (--g_ClosureBudget < 0)
            return false;
        CaptureState(seg.Before);
        g_EnumCount = 0;
        g_pCurrentSegment = &seg;
        g_VirtualPlacing = true;
        CViewObjectsExt::PlaceConnectedTile_OnMouseMove(ClosureCursor.X, ClosureCursor.Y, true);
        g_VirtualPlacing = false;
        g_pCurrentSegment = nullptr;
        return CViewObjectsExt::CliffConnectionCoord != anchor0;
    }

    // build the virtual cursor candidates for one step: the real cursor, the
    // first tile's anchor, and the anchor offset along a narrow sector of
    // directions around the ideal one (towards the target), at two distances
    // (near = small/medium tile variants, far = large ones). The placement
    // logic derives facing and distance from the cursor, so each candidate
    // makes it pick a different tile/direction. Candidates are ordered by
    // distance to the target so the search tries the most promising first.
    static int BuildClosureCursors(MapCoord anchor0, MapCoord* out, int max)
    {
        static const int DirX[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
        static const int DirY[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
        static const int Lens[2] = { 4, 10 };

        int n = 0;
        auto add = [&](int x, int y)
        {
            if (n >= max)
                return;
            for (int i = 0; i < n; ++i)
            {
                if (out[i].X == x && out[i].Y == y)
                    return;
            }
            out[n++] = MapCoord{ x, y };
        };

        int ideal = CMapDataExt::GetFacing(anchor0, ClosureTargetCoord);
        for (int w = 0; w <= ClosureSectorWidth; ++w)
        {
            for (int s = -1; s <= 1; s += 2)
            {
                if (w == 0 && s == -1)
                    continue;
                int dir = (ideal + w * s + 8) % 8;
                for (int l = 0; l < 2; ++l)
                    add(anchor0.X + DirX[dir] * Lens[l], anchor0.Y + DirY[dir] * Lens[l]);
            }
        }

        add(ClosureRealCursor.X, ClosureRealCursor.Y);
        add(ClosureTargetCoord.X, ClosureTargetCoord.Y);

        std::sort(out, out + n, [](const MapCoord& a, const MapCoord& b)
        {
            return Dist(a, ClosureTargetCoord) < Dist(b, ClosureTargetCoord);
        });
        return n;
    }

    // one beam state: a complete placement state plus the extension segments
    // placed so far, with its heuristic score
    struct ClosureBeamEntry
    {
        SegmentState State;
        std::vector<Segment> Segments;
        double Score;
        double G;
    };

    // beam search: layer by layer, place one tile per state, keep only the
    // ClosureBeamWidth states closest to the target (heuristic f = 0.7*g + h).
    // The path may end as soon as a placed tile matches the flow's first tile.
    // On success S.Segments holds the closing path and the state is at the
    // seam; on failure the caller must restore the k-round state.
    static bool ClosureBeamSearch(const SegmentState& kState, int baseCount)
    {
        auto& S = g_Session;

        std::vector<ClosureBeamEntry> beam;
        ClosureBeamEntry seed;
        seed.State = kState;
        seed.G = 0.0;
        seed.Score = Dist(kState.Anchor, ClosureTargetCoord);
        beam.push_back(std::move(seed));

        for (int depth = 0; depth < ClosureMaxSteps; ++depth)
        {
            std::vector<ClosureBeamEntry> next;

            for (auto& entry : beam)
            {
                if (g_ClosureBudget <= 0)
                    break;

                RestoreState(entry.State);
                S.Segments.resize(baseCount);
                for (auto& seg : entry.Segments)
                    S.Segments.push_back(seg);
                RebuildMerged(S);

                MapCoord anchor0 = CViewObjectsExt::CliffConnectionCoord;
                SegmentState stepBefore;
                CaptureState(stepBefore);

                if (Dist(anchor0, ClosureTargetCoord) > (ClosureMaxSteps - depth) * ClosurePruneStep)
                    continue;

                MapCoord cursors[16];
                int cursorCount = BuildClosureCursors(anchor0, cursors, 16);

                for (int c = 0; c < cursorCount; ++c)
                {
                    ClosureCursor = cursors[c];

                    // try one index branch: default, or a forced alternative
                    auto tryBranch = [&](int forcedIndex, bool requireHonored) -> int
                    {
                        RestoreState(stepBefore);
                        Segment seg;
                        g_ForcedIndex = forcedIndex;
                        g_ForcedVariant = ClosureFirstVariant;
                        if (!ClosurePlace(seg, anchor0))
                            return 0;
                        if (requireHonored && !g_ForcedHonored)
                            return 0;

                        if (ClosureTripleMatches())
                        {
                            // this tile is the seam: it must overlap the first
                            // tile exactly
                            if (!ClosureFootprintMatches(seg))
                                return 0;
                            RestoreFirstTileData(seg);
                            if (!ValidClosureStep(seg, true))
                                return 0;
                            MergeSegment(S, seg);
                            S.Segments.push_back(seg);
                            return 2;
                        }

                        if (!ValidClosureStep(seg, false))
                            return 0;

                        ClosureBeamEntry ne;
                        CaptureState(ne.State);
                        ne.Segments = entry.Segments;
                        ne.Segments.push_back(std::move(seg));
                        double g2 = entry.G + Dist(anchor0, CViewObjectsExt::CliffConnectionCoord);
                        ne.G = g2;
                        ne.Score = 0.7 * g2 + Dist(CViewObjectsExt::CliffConnectionCoord, ClosureTargetCoord);
                        next.push_back(std::move(ne));
                        return 1;
                    };

                    int r = tryBranch(-1, false);
                    if (r == 2)
                        return true;

                    // alternative branches: the random choice points expose at
                    // most two candidates; try the one the default run did not
                    // pick
                    int picked = g_EnumPicked;
                    for (int i = 0; i < g_EnumCount && i < 2; ++i)
                    {
                        int alt = g_EnumList[i];
                        if (alt < 0 || alt == picked)
                            continue;
                        int r2 = tryBranch(alt, true);
                        if (r2 == 2)
                            return true;
                    }
                }
            }

            if (next.empty())
                return false;

            std::sort(next.begin(), next.end(), [](const ClosureBeamEntry& a, const ClosureBeamEntry& b)
            {
                return a.Score < b.Score;
            });
            if ((int)next.size() > ClosureBeamWidth)
                next.resize(ClosureBeamWidth);
            beam = std::move(next);
        }

        return false;
    }

    // plan a closing path. The default plan's tail is the "base state": first
    // try extending 2 tiles from it, then backtrack one tile and extend 3,
    // then backtrack two and extend 4, ... until the whole base plan has been
    // consumed. Falls back to the base plan when no closing arrangement exists.
    static bool PlanClosure(int X, int Y)
    {
        auto& S = g_Session;

        ClosureCursor = { X, Y };
        ClosureRealCursor = { X, Y };

        // outer layer: when the closure search over a base plan fails, re-roll
        // a fresh base plan (the default generation is random) and search
        // again, up to ClosureBaseRetries times. The last generated base plan
        // is kept as the final fallback.
        SegmentState lastBaseState;
        std::vector<Segment> lastBaseSegments;

        for (int attempt = 0; attempt <= ClosureBaseRetries; ++attempt)
        {
            // per-attempt budget: every base re-roll gets a fresh allowance
            g_ClosureBudget = ClosureBudgetLimit;

            // rebuild the base plan fresh: it is the "default generation" the
            // closure search modifies
            PlanSession(X, Y);

            CaptureState(lastBaseState);
            lastBaseSegments = S.Segments;

            // the target is the first tile of the current plan; re-planning
            // re-rolls it, so refresh the closure target for this attempt
            ClosureTargetIndex = ClosureFirstIndex;
            ClosureTargetOpposite = ClosureFirstOpposite;
            ClosureTargetCoord = ClosureFirstCoord;

            // the default plan already closes the loop
            if (ClosureTripleMatches())
            {
                g_ClosurePlanActive = true;
                return true;
            }

            int n = (int)S.Segments.size();
            int kMax = (ClosureMaxBacktrack < 0) ? n : std::min(n, ClosureMaxBacktrack);
            for (int k = 0; k <= kMax; ++k)
            {
                if (g_ClosureBudget <= 0)
                    break;
                if (k > 0)
                {
                    auto& keep = S.Segments[S.Segments.size() - 1];
                    RestoreState(keep.Before);
                    S.Segments.pop_back();
                    RebuildMerged(S);
                }

                ClosureMaxSteps = 2 + k;
                SegmentState kState;
                CaptureState(kState);
                int baseCount = (int)S.Segments.size();
                bool solved = ClosureBeamSearch(kState, baseCount);
                if (!solved)
                {
                    RestoreState(kState);
                    S.Segments.resize(baseCount);
                    RebuildMerged(S);
                }
                if (solved)
                {
                    g_ClosurePlanActive = true;
                    return true;
                }
            }
        }

        // no closure solution across all base re-rolls: keep the last base plan
        RestoreState(lastBaseState);
        S.Segments = std::move(lastBaseSegments);
        RebuildMerged(S);
        g_ClosurePlanActive = false;
        return false;
    }

}

bool CViewObjectsExt::AutoConnect_PreviewActive()
{
    return AutoConnect::g_Session.Previewing;
}

void CViewObjectsExt::AutoConnect_UpdatePreview(int X, int Y)
{
    if (!CMapDataExt::IsCoordInFullMap(X, Y))
        return;
    auto& S = AutoConnect::g_Session;

    // no active session (e.g. right after a commit): open one anchored at the
    // current placement state so chained segments keep their live preview
    if (!S.Active)
    {
        if (CViewObjectsExt::CliffConnectionCoord.X < 0
            || CViewObjectsExt::CliffConnectionCoord.Y < 0)
            return;
        S.Active = true;
        S.Previewing = false;
        S.Start = CViewObjectsExt::CliffConnectionCoord;
        S.LastPreviewPos = -1;
        S.Segments.clear();
        S.MergedCells.clear();
        S.MergedFixCells.clear();
        S.OldCells.clear();
        S.OldOverlays.clear();
        CViewObjectsExt::LastTempPlacedCTIndex = -1;
        CViewObjectsExt::LastTempFacing = -1;
        CViewObjectsExt::CliffConnectionTile = -1;
        AutoConnect::CaptureState(S.SessionBefore);
    }

    // closure mode: the cursor is within 2 cells of the flow's first tile, so the
    // plan is adjusted so its last tile lands exactly on the first tile
    bool closureActive = AutoConnect::ClosureFirstRecorded
        && AutoConnect::Dist(MapCoord{ X, Y }, AutoConnect::ClosureFlowStart) <= 3.0;

    if (!closureActive)
    {
        // too close to the session start for a meaningful path; the anchor itself
        // may sit at the virtual path end after planning, so the distance must be
        // measured from the fixed session start point instead
        int ddx = X - S.Start.X, ddy = Y - S.Start.Y;
        if (sqrt((double)(ddx * ddx + ddy * ddy)) <= 2.0)
        {
            if (S.Previewing)
            {
                AutoConnect::RevertFromMap();
                S.OldCells.clear();
                S.OldOverlays.clear();
                S.Previewing = false;
                S.LastPreviewPos = -1;
                AutoConnect::g_ClosurePlanActive = false;
                ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
            }
            return;
        }
    }

    int dwpos = Y * CMapData::Instance->MapWidthPlusHeight + X;
    if (S.Previewing && S.LastPreviewPos == dwpos)
        return; // same cell, nothing to do

    // revert the previous preview before replanning
    if (S.Previewing)
    {
        AutoConnect::RevertFromMap();
        S.OldCells.clear();
        S.OldOverlays.clear();
        S.Previewing = false;
    }

    if (closureActive)
        AutoConnect::PlanClosure(X, Y);
    else
        AutoConnect::PlanSession(X, Y);
    S.LastPreviewPos = dwpos;

    if (S.Segments.empty())
        return; // nothing planned; the map stays untouched

    AutoConnect::ApplyToMap(true);
    S.Previewing = true;
    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
}

bool CViewObjectsExt::AutoConnect_OnClick(int X, int Y)
{
    auto& S = AutoConnect::g_Session;

    // closure mode: a click within 2 cells of the flow's first tile commits the
    // closing path instead of cancelling the session
    bool closureClick = AutoConnect::ClosureFirstRecorded
        && AutoConnect::Dist(MapCoord{ X, Y }, AutoConnect::ClosureFlowStart) <= 2.0;

    if (!closureClick)
    {
        // measure from the fixed session start: after a preview the anchor sits at
        // the virtual path end, so a click on the target would otherwise be
        // misread as a "close" click and fall back to a single-segment placement
        MapCoord base = S.Active ? S.Start : CViewObjectsExt::CliffConnectionCoord;
        int cdx = X - base.X, cdy = Y - base.Y;
        if (sqrt((double)(cdx * cdx + cdy * cdy)) <= 2.0)
        {
            if (S.Active)
                CViewObjectsExt::AutoConnect_Cancel(); // close click cancels the session
            return false; // caller falls back to the classic single-segment placement
        }
    }

    // (re)plan to the click point; UpdatePreview auto-opens the session when
    // none is active (chained placement right after a commit) and keeps the
    // already previewed path when the mouse cell hasn't changed
    CViewObjectsExt::AutoConnect_UpdatePreview(X, Y);

    if (!S.Previewing || S.Segments.empty())
        return true; // nothing plannable; session stays open for the next click

    // Never commit a partial path that did not actually reach the target,
    // unless the preview is a successful closure plan: it ends on the flow's
    // first tile by design, which may sit a little past the cursor
    if (AutoConnect::Dist(CViewObjectsExt::CliffConnectionCoord, S.Target) > 2.0
        && !(closureClick && AutoConnect::g_ClosurePlanActive))
    {
        return true; // keep the preview; the next mouse move will replan
    }

    {
        // commit: revert -> precise undo snapshot -> re-apply
        AutoConnect::RevertFromMap();
        S.OldCells.clear();
        S.OldOverlays.clear();
        S.Previewing = false;

        auto& mapData = CMapData::Instance();

        // precise undo rectangle around every touched position
        int l = mapData.MapWidthPlusHeight, t = mapData.MapWidthPlusHeight, r = 0, b = 0;
        auto expand = [&](int pos)
        {
            int x = pos % mapData.MapWidthPlusHeight;
            int y = pos / mapData.MapWidthPlusHeight;
            if (x < l) l = x;
            if (y < t) t = y;
            if (x > r) r = x;
            if (y > b) b = y;
        };
        for (auto& [pos, _] : S.MergedCells)
            expand(pos);
        for (auto& [pos, _] : S.MergedFixCells)
            expand(pos);
        for (auto& seg : S.Segments)
            for (auto& [pos, _] : seg.Overlays)
                expand(pos);

        mapData.SaveUndoRedoData(true, l - 2, t - 2, r + 2, b + 2);

        AutoConnect::ApplyToMap(false);

        for (auto& [pos, _] : S.MergedCells)
        {
            int x = pos % mapData.MapWidthPlusHeight;
            int y = pos / mapData.MapWidthPlusHeight;
            mapData.UpdateMapPreviewAt(x, y);
        }
        for (auto& [pos, _] : S.MergedFixCells)
        {
            int x = pos % mapData.MapWidthPlusHeight;
            int y = pos / mapData.MapWidthPlusHeight;
            mapData.UpdateMapPreviewAt(x, y);
        }
        for (auto& seg : S.Segments)
        {
            for (auto& [pos, _] : seg.Overlays)
            {
                int x = pos % mapData.MapWidthPlusHeight;
                int y = pos / mapData.MapWidthPlusHeight;
                mapData.UpdateMapPreviewAt(x, y);
            }
        }

        // one undo step for the whole path: the 4 record stacks hold the
        // session-start state, the native undo restores the map rectangle
        CViewObjectsExt::LastPlacedCTRecords.push_back(S.SessionBefore.LastCT);
        CViewObjectsExt::CliffConnectionCoordRecords.push_back(S.SessionBefore.Anchor);
        CViewObjectsExt::LastCTTileRecords.push_back(S.SessionBefore.LastCTTile);
        CViewObjectsExt::LastHeightRecords.push_back(S.SessionBefore.Height);

        // committing an AutoConnect batch locks the staged first tile as real
        AutoConnect::ClosureFirstPending = false;

        // Keep this committed batch in the cross-batch avoidance history.
        // It must be stored before Segments/Merged* are cleared.
        AutoConnect::CommittedBatch batch;
        batch.Cells = S.MergedCells;
        batch.FixCells = S.MergedFixCells;
        for (auto& seg : S.Segments)
        {
            for (auto& [pos, ov] : seg.Overlays)
                batch.Overlays[pos] = ov;
        }
        AutoConnect::MergeCommittedBatch(batch);
        S.CommittedBatches.push_back(std::move(batch));

        // state stays at the path end for chained sessions
        S.Active = false;
        S.Segments.clear();
        S.MergedCells.clear();
        S.MergedFixCells.clear();
        S.LastPreviewPos = -1;

        // A closed loop has been committed: reset the placement flow so the next
        // click starts a brand-new flow. The undo record stacks AND the first
        // tile closure record are intentionally kept: undoing returns to the
        // exact state the closure was committed from, and the closure mode
        // stays usable from there (it is only cleared by a fresh start click).
        if (closureClick && AutoConnect::g_ClosurePlanActive)
        {
            CViewObjectsExt::CliffConnectionCoord.X = -1;
            CViewObjectsExt::CliffConnectionCoord.Y = -1;
            CViewObjectsExt::CliffConnectionHeight = -1;
            CViewObjectsExt::LastCTTile = -1;
            CViewObjectsExt::CliffConnectionTile = -1;
            CViewObjectsExt::LastPlacedCT.Index = -1;
            CViewObjectsExt::ThisPlacedCT.Index = -1;
            CViewObjectsExt::LastTempPlacedCTIndex = -1;
            CViewObjectsExt::LastTempFacing = -1;
            CViewObjectsExt::PlaceConnectedTile_Start = false;
            AutoConnect::ClearCommitted();
        }

        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
        return true;
    }
}

void CViewObjectsExt::AutoConnect_Cancel()
{
    auto& S = AutoConnect::g_Session;
    if (S.Previewing)
    {
        AutoConnect::RevertFromMap();
        S.OldCells.clear();
        S.OldOverlays.clear();
        S.Previewing = false;
        ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
    }
    if (S.Active)
    {
        // roll the placement state back to the session start point
        AutoConnect::RestoreState(S.SessionBefore);
        S.Active = false;
    }
    S.Segments.clear();
    S.MergedCells.clear();
    S.MergedFixCells.clear();
    S.LastPreviewPos = -1;
}

void CViewObjectsExt::AutoConnect_UndoLastBatch()
{
    // Called from the connected-tile undo path after AutoConnect_Cancel().
    // Removes the last committed batch (AutoConnect or manual single segment)
    // from the cross-batch avoidance history so subsequent batches do not
    // keep avoiding cells that were just undone.
    if (AutoConnect::PopLastCommittedBatch()
        && AutoConnect::g_Session.CommittedBatches.empty())
    {
        // the flow's very first tile was undone: restart closure tracking
        AutoConnect::ClosureFirstRecorded = false;
        AutoConnect::ClosureFirstPending = true;
        AutoConnect::ClosureFirstCellData.clear();
        AutoConnect::ClosureFirstOverlays.clear();
        AutoConnect::ClosureFirstBodyCells.clear();
    }
}

void CViewObjectsExt::AutoConnect_ResetHistory()
{
    // Called when the user explicitly leaves/resets the AutoConnect flow,
    // e.g. right-click or switching away from the connected-tile tool, so
    // stale committed-cell data cannot leak into a brand-new flow.
    AutoConnect::ClearCommitted();
}

void CViewObjectsExt::PlaceConnectedTile_OnLButtonDown(int X, int Y)
{
    if (!CMapDataExt::IsCoordInFullMap(X, Y))
        return;
    auto& mapData = CMapData::Instance();
    auto cellDatas = mapData.CellDatas;

    if (CViewObjectsExt::CliffConnectionCoord.X == -1
        || CViewObjectsExt::CliffConnectionCoord.Y == -1
        || CViewObjectsExt::CliffConnectionHeight == -1)
    {
        CViewObjectsExt::CliffConnectionCoord.X = X;
        CViewObjectsExt::CliffConnectionCoord.Y = Y;
        CViewObjectsExt::CliffConnectionCoordRecords.clear();
        auto dwpos = Y * mapData.MapWidthPlusHeight + X;
        auto& cell = cellDatas[dwpos];
        int tileIndex = CMapDataExt::GetSafeTileIndex(cell.TileIndex);
        int tileSubIndex = CMapDataExt::GetSafeSubTileIndex(cell.TileIndex, cell.TileSubIndex);
        auto internalHeight = CMapDataExt::TileData[tileIndex].TileBlockDatas[tileSubIndex].Height;
        CViewObjectsExt::CliffConnectionHeight = cellDatas[dwpos].Height - internalHeight;
        CViewObjectsExt::CliffConnectionHeight = std::clamp(CViewObjectsExt::CliffConnectionHeight, 0, 14);
        CViewObjectsExt::PlaceConnectedTile_Start = true;
        CViewObjectsExt::LastTempPlacedCTIndex = -1;
        CViewObjectsExt::LastTempFacing = -1;
        CViewObjectsExt::CliffConnectionTile = -1;

        // Any new placement flow starts with an empty cross-batch history,
        // regardless of whether AutoConnect is currently enabled.
        AutoConnect::ClearCommitted();

        // A fresh flow: reset the closure tracking and remember the start anchor.
        AutoConnect::ClosureFirstRecorded = false;
        AutoConnect::ClosureFirstPending = true;
        AutoConnect::ClosureFlowStart = { X, Y };
        AutoConnect::ClosureFirstCellData.clear();
        AutoConnect::ClosureFirstOverlays.clear();
        AutoConnect::ClosureFirstBodyCells.clear();

        if (CViewObjectsExt::PlaceConnectedTile_AutoConnect)
        {
            // open a new virtual session anchored here
            auto& S = AutoConnect::g_Session;
            S.Active = true;
            S.Previewing = false;
            S.Start = { X, Y };
            S.LastPreviewPos = -1;
            S.Segments.clear();
            S.MergedCells.clear();
            S.MergedFixCells.clear();
            S.OldCells.clear();
            S.OldOverlays.clear();
            AutoConnect::CaptureState(S.SessionBefore);
        }
        return;
    }

    if (CViewObjectsExt::PlaceConnectedTile_AutoConnect)
    {
        // commit the previewed path (or plan+commit when no preview is up);
        // a close click on the session start cancels it and returns false
        if (CViewObjectsExt::AutoConnect_OnClick(X, Y))
            return;
        // fall through to the classic single-segment placement
    }

    // Manual single-segment placement. It does not need to avoid anything
    // itself, but it must still be recorded in the committed history so a
    // later AutoConnect session can avoid the cells it occupied.
    auto& S = AutoConnect::g_Session;
    S.PendingManualBatch = AutoConnect::CommittedBatch{};
    AutoConnect::g_pCurrentManualBatch = &S.PendingManualBatch;
    CViewObjectsExt::PlaceConnectedTile_OnMouseMove(X, Y, true);
    AutoConnect::g_pCurrentManualBatch = nullptr;

    if (!S.PendingManualBatch.Cells.empty()
        || !S.PendingManualBatch.FixCells.empty()
        || !S.PendingManualBatch.Overlays.empty())
    {
        AutoConnect::MergeCommittedBatch(S.PendingManualBatch);
        S.CommittedBatches.push_back(std::move(S.PendingManualBatch));
    }
}
