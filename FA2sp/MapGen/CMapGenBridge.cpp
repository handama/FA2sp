#include "CMapGenBridge.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

#include "../Logger.h"
#include "../Ext/CMapData/Body.h"
#include "CMapGenDataFeed.h"

namespace
{
    // FA2's cell coordinates are the transpose of the generator's: the
    // generator packs [Terrain] keys as X + 1000*Y (X in the low digits) while
    // FA2 decodes x = value / 1000 (X in the high digits). Both are internally
    // correct, so exporting has to swap the axes. Everything else in this file
    // works in FA2 coordinates and goes through these two helpers.
    constexpr bool kGenXYTransposed = true;

    inline void Fa2ToGen(int ax, int ay, int& gx, int& gy)
    {
        if (kGenXYTransposed) { gx = ay; gy = ax; }
        else                  { gx = ax; gy = ay; }
    }

    // Inverse of Fa2ToGen, undoing the region offset as well: the generator was
    // asked for a map whose local origin sits at (-dx, -dy).
    inline void GenToFa2(int gx, int gy, int dx, int dy, int& ax, int& ay)
    {
        if (kGenXYTransposed) { ax = gy + dy; ay = gx + dx; }
        else                  { ax = gx + dx; ay = gy + dy; }
    }

    // dx / dy are offsets in the generator's own coordinate space.
    inline MapCell* GenCellAt(RandomMapGenerator& rmg, int ax, int ay, int dx, int dy)
    {
        int gx = 0, gy = 0;
        Fa2ToGen(ax, ay, gx, gy);
        return rmg.GetCellAt(gx - dx, gy - dy);
    }

    inline int CeilDiv2(int value)
    {
        return (value + 1) / 2;
    }

    // The generator names every structure it places "Neutral House", but a map
    // only accepts a house its own [Houses] section actually lists. Pick the
    // closest match that exists:
    //   section empty        -> "Neutral"
    //   has "Neutral House"  -> "Neutral House"
    //   has "Neutral"        -> "Neutral"
    //   otherwise            -> the first entry
    //
    // [Houses] is index=name (0=Americans, 1=Neutral House, ...) and FA2 treats
    // the name as the house identity - it looks up the [<name>] section for the
    // per-house data - so the comparison is on the entries' values.
    FString ResolveHouseName(CINI& ini)
    {
        const char* kNeutralHouse = "Neutral House";
        const char* kNeutral = "Neutral";
        const FString fallback = kNeutral;

        INISection* pSection = ini.GetSection("Houses");
        if (pSection == nullptr || pSection->GetEntities().empty())
            return fallback;

        bool hasNeutralHouse = false;
        bool hasNeutral = false;
        FString first;

        for (const auto& entry : pSection->GetEntities())
        {
            const char* name = entry.second.GetString();
            if (name == nullptr || name[0] == '\0')
                continue;

            if (first.empty())
                first = name;

            if (_stricmp(name, kNeutralHouse) == 0)
                hasNeutralHouse = true;
            else if (_stricmp(name, kNeutral) == 0)
                hasNeutral = true;
        }

        if (hasNeutralHouse)
            return FString(kNeutralHouse);
        if (hasNeutral)
            return FString(kNeutral);
        return first.empty() ? fallback : first;
    }
}

MapGenError CMapGenBridge::Run(const MapGenRequest& req,
                               int ax1, int ay1, int ax2, int ay2)
{
    if (CMapData::Instance->MapNotLoaded)
        return MapGenError::NoMap;

    const int theater = CMapGenDataFeed::CurrentTheaterIndex();
    if (theater < 0)
        return MapGenError::Theater;

    if (CMapDataExt::TileData == nullptr || CMapDataExt::TileDataCount <= 0)
        return MapGenError::NoTileData;

    // ---- selection -> generator extent ------------------------------------
    int dx = 0;
    int dy = 0;
    int mapWidth = 0;    // W'
    int mapHeight = 0;   // H'

    if (req.wholeMap)
    {
        mapWidth = CMapData::Instance->Size.Width;
        mapHeight = CMapData::Instance->Size.Height;
        dx = 0;
        dy = 0;

        // The generator sizes its slot array from the map (see slotStride_),
        // so the only limit is the editor's own w+h < 2048.
        if (mapWidth <= 0 || mapHeight <= 0 || mapWidth + mapHeight > 2048)
            return MapGenError::RegionTooLarge;
    }
    else
    {
        if (ax1 > ax2) std::swap(ax1, ax2);
        if (ay1 > ay2) std::swap(ay1, ay2);

        const int w = ax2 - ax1 + 1;
        const int h = ay2 - ay1 + 1;
        if (w < kMinRegionSide || h < kMinRegionSide)
            return MapGenError::RegionTooSmall;

        // In generator space the rectangle is [ay1..ay2] x [ax1..ax2].
        const int gw = h;
        const int gh = w;

        // Pick the smallest diamond that strictly contains the rectangle:
        //   W' < a+b                 (a = b = k, k = W'/2 + 1)
        //   (a+gw-1)+(b+gh-1) <= W'+2H'
        // and W' >= max(gw, gh) so |x-y| < W' holds everywhere inside it.
        mapWidth = std::max(gw, gh);
        const int k = mapWidth / 2 + 1;
        mapHeight = CeilDiv2(2 * k + gw + gh - 2 - mapWidth);
        if (mapHeight < 1)
            mapHeight = 1;

        // Editor limit: w + h stays below 2048, matching MapSizeExt.
        if (mapWidth + mapHeight > 2048)
            return MapGenError::RegionTooLarge;

        dx = ay1 - k;
        dy = ax1 - k;
    }

    // ---- run the generator ------------------------------------------------
    RandomMapGenerator rmg;
    if (!CMapGenDataFeed::Apply(rmg))
        return MapGenError::NoTileData;

    // The ore slider drives both knobs the generator has: the tiberium total
    // (AddTiberium spreads MinTiberium + 0.2*oreDensity*(Max-Min)) and the
    // 0..3 control that also decides whether gems spawn. Pinning min == max
    // turns the slider into a direct ore budget; the 55 is halved because a
    // full slider at the old scale was far too much ore to play on.
    const int orePercent = std::clamp(req.oreAmount, 0, 100);

    RMGSettings settings = rmg.GetSettings();
    settings.MinTiberium = orePercent * 55 / 2;
    settings.MaxTiberium = orePercent * 55 / 2;
    settings.VegetationMinimums.assign(5, std::clamp(req.vegetation, 0, 100));
    settings.VegetationMaximums.assign(5, std::clamp(req.vegetation, 0, 100));
    rmg.SetSettings(settings);   // must precede RollGlobalOptions()

    MapGenConfig cfg = MapGenConfig();
    cfg.landType = req.landType;
    cfg.theater = theater;
    cfg.timeOfDay = 1;
    cfg.sizeSlider = 1;          // unused: overrideMapWidth/Height replace CalcMapSize
    cfg.playerCount = 2;         // unused by the terrain pipeline
    cfg.oreDensity = std::clamp(orePercent * 3 / 100, 0, 3);
    cfg.multiplayer = true;      // only the .map writer reads this
    cfg.overrideMapWidth = mapWidth;
    cfg.overrideMapHeight = mapHeight;

    // Both seeds are used verbatim - 0 is a perfectly valid seed, and it is
    // what the retail build always used for the map's own Randomizer.
    const unsigned globalSeed = req.globalSeed;

    cfg.global = rmg.RollGlobalOptions(globalSeed);
    cfg.randomSeed = static_cast<unsigned>(cfg.global.seed04C);

    // The map's own Randomizer. Vanilla pins this to the constant 0 (0x58b770),
    // which is why the island skeleton never moved; wiring the second seed box
    // here is what makes the shape follow the seed the user picked.
    cfg.mapRngSeed = req.mapSeed;
    cfg.waterAmount = cfg.global.waterAmount;

    Logger::Debug("[MapGen] globalSeed=%08X mapSeed=%08X land=%d water=%d ext=%dx%d\n",
                  globalSeed, cfg.mapRngSeed, static_cast<int>(cfg.landType),
                  cfg.waterAmount, mapWidth, mapHeight);

    if (!rmg.GenerateMapBody(cfg))
        return MapGenError::GenFailed;

    Logger::Debug("[MapGen] theater tiles: water=%d shore=%d green=%d\n",
                  rmg.GetWaterTileIndexDebug(),
                  rmg.GetShorePiecesDebug(),
                  rmg.GetGreenTileDebug());

    // The stage order is the retail one; see WinMain.cpp:496-624. It must not
    // be reordered - each stage's RNG draws depend on the previous ones.
    if (cfg.landType == RmLandType::Inland || cfg.landType == RmLandType::Mountainous)
    {
        if (rmg.GetWaterAmount() != 0)
            rmg.GenerateSpecialTerrain();
    }
    else
    {
        rmg.GenerateTerrain();
    }

    rmg.DecorateWaterTiles();
    rmg.InitRegions();
    rmg.MakeRegions();
    rmg.RecalculateCellAttributes();
    rmg.CreateStartingPoints();      // consumes RNG; its waypoints are dropped
    rmg.AddTechBuildings();
    rmg.AddTiberium();
    rmg.RecalculateCellAttributes();
    rmg.RecalculateCellAttributes();
    rmg.CreateHills();
    rmg.CreateLATs();
    rmg.RecalculateCellAttributes();
    rmg.PruneTerrainTrees();
    rmg.Cleanup();
    rmg.Done();

    // ---- write it into the document --------------------------------------
    if (req.wholeMap)
    {
        const int side = CMapData::Instance->Size.Width + CMapData::Instance->Size.Height;
        ax1 = 1; ay1 = 1;
        ax2 = side; ay2 = side;
    }

    // Everything in the selection is wiped before writing: tiles get replaced
    // right after, but terrain objects, smudges, structures and overlays from
    // the old map would otherwise survive on top of the new ground.
    CMapDataExt& map = *CMapDataExt::GetExtension();

    const int recordType = ObjectRecord::RecordType::Terrain
                         | ObjectRecord::RecordType::Smudge
                         | ObjectRecord::RecordType::Building
                         | ObjectRecord::RecordType::Unit
                         | ObjectRecord::RecordType::Aircraft
                         | ObjectRecord::RecordType::Infantry;
    CMapDataExt::MakeMixedRecord(ax1 - 4, ay1 - 4, ax2 + 4, ay2 + 4, recordType);

    // Clear the selection. The object arrays - not the cells - are what ends
    // up in the saved map, so they are swept by coordinate: a cell that lost
    // its Terrain/Structure index can still leave an orphaned entry behind,
    // and that is exactly what made a second Generate leave the old rocks and
    // buildings sitting under the new terrain.
    //
    // Descending, because DeleteXxxData closes the gap in the array and the
    // remaining entries shift down.
    auto InRect = [ax1, ay1, ax2, ay2](int x, int y)
    {
        return x >= ax1 && x <= ax2 && y >= ay1 && y <= ay2;
    };

    for (int i = static_cast<int>(map.TerrainDatas.size()) - 1; i >= 0; --i)
    {
        const CTerrainData& item = map.TerrainDatas[i];
        if (InRect(item.X, item.Y))
            map.DeleteTerrainData(i);
    }

    for (int i = static_cast<int>(map.SmudgeDatas.size()) - 1; i >= 0; --i)
    {
        const CSmudgeData& item = map.SmudgeDatas[i];
        if (InRect(item.X, item.Y))
            map.DeleteSmudgeData(i);
    }

    for (int i = static_cast<int>(map.InfantryDatas.size()) - 1; i >= 0; --i)
    {
        const CInfantryData& item = map.InfantryDatas[i];
        if (InRect(std::atoi(item.X.GetString()), std::atoi(item.Y.GetString())))
            map.DeleteInfantryData(i);
    }

    for (int i = static_cast<int>(map.UnitDatas.size()) - 1; i >= 0; --i)
    {
        const CUnitData& item = map.UnitDatas[i];
        if (InRect(std::atoi(item.X.GetString()), std::atoi(item.Y.GetString())))
            map.DeleteUnitData(i);
    }

    // Structures and aircraft have no exposed array, so the cell is the only
    // handle - exactly how FA2sp's own delete hook does it. No range check:
    // BuildingDatas is never populated, so bounding by its size silently
    // rejected every index.
    std::vector<int> doomedBuildings;
    std::vector<int> doomedAircraft;
    for (int ay = ay1; ay <= ay2; ++ay)
    {
        for (int ax = ax1; ax <= ax2; ++ax)
        {
            if (!map.IsCoordInMap(ax, ay))
                continue;
            CellData* cell = map.GetCellAt(ax, ay);
            if (cell == nullptr)
                continue;

            if (cell->Structure > -1)
                doomedBuildings.push_back(cell->Structure);
            if (cell->Aircraft > -1)
                doomedAircraft.push_back(cell->Aircraft);
        }
    }

    auto PurgeDescending = [](std::vector<int>& list, auto&& erase)
    {
        std::sort(list.begin(), list.end());
        list.erase(std::unique(list.begin(), list.end()), list.end());
        for (auto it = list.rbegin(); it != list.rend(); ++it)
        {
            if (*it > -1)
                erase(*it);
        }
    };

    PurgeDescending(doomedBuildings, [&map](int index) { map.DeleteBuildingData(index); });
    PurgeDescending(doomedAircraft, [&map](int index) { map.DeleteAircraftData(index); });

    // Whatever the engine left pointing at a removed entry gets dropped here,
    // together with the overlays the new terrain replaces.
    for (int ay = ay1; ay <= ay2; ++ay)
    {
        for (int ax = ax1; ax <= ax2; ++ax)
        {
            if (!map.IsCoordInMap(ax, ay))
                continue;
            CellData* cell = map.GetCellAt(ax, ay);
            if (cell == nullptr)
                continue;

            cell->Structure = -1;
            cell->Terrain = -1;
            cell->Smudge = -1;
            cell->Unit = -1;
            cell->Aircraft = -1;
            cell->Infantry[0] = -1;
            cell->Infantry[1] = -1;
            cell->Infantry[2] = -1;

            if (cell->Overlay != 0xFF)
                map.SetNewOverlayAt(ax, ay, 0xFFFF);
        }
    }

    int written = 0;
    std::vector<int> oreCells;   // cells that actually received an ore overlay

    for (int ay = ay1; ay <= ay2; ++ay)
    {
        for (int ax = ax1; ax <= ax2; ++ax)
        {
            if (!map.IsCoordInMap(ax, ay))
                continue;

            MapCell* src = GenCellAt(rmg, ax, ay, dx, dy);
            if (src == nullptr)
                continue;

            CellData* dst = map.GetCellAt(ax, ay);

            // MapCell::Height is the TMP sub-cell, MapCell::Level is the ground
            // level. FA2 stores them the other way round in CellData
            // (TileSubIndex / Height) - do not "fix" this pairing.
            // 0xFFFF is the engine's "no tile" marker and has to survive: going
            // through GetSafeTileIndex would clamp it to 0, which is a real
            // tile, so the empty cells turned into solid ground.
            const int srcTile = src->IsoTileTypeIndex;
            dst->TileIndex = (srcTile == 0xFFFF)
                ? static_cast<unsigned short>(0xFFFF)
                : static_cast<unsigned short>(CMapDataExt::GetSafeTileIndex(srcTile) & 0xFFFF);
            dst->TileIndexHiPart = 0;
            dst->TileSubIndex = static_cast<unsigned char>(src->Height & 0xFF);
            dst->Height = static_cast<unsigned char>(std::clamp(src->Level, 0, 14));
            dst->IceGrowth = 0;

            if (req.withOre && src->OverlayTypeIndex > 0)
            {
                const unsigned ovr = static_cast<unsigned>(src->OverlayTypeIndex);
                const unsigned char ovrd = static_cast<unsigned char>(src->OverlayData);

                const int pos = map.GetCoordIndex(ax, ay);
                const int olyPos = ay + ax * CMapDataExt::X_PLUS_Y_LIMIT;

                if (olyPos < static_cast<int>(CMapDataExt::NewOverlay.size()) && pos < map.CellDataCount)
                {
                    CMapDataExt::NewOverlay[static_cast<size_t>(olyPos)] = static_cast<WORD>(ovr);
                    CMapDataExt::NewOverlayData[static_cast<size_t>(olyPos)] = ovrd;
                    CMapDataExt::CellDataExts[static_cast<size_t>(pos)].NewOverlay = static_cast<WORD>(ovr);

                    dst->Overlay = static_cast<unsigned char>(std::min<unsigned>(ovr, 0xFF));
                    dst->OverlayData = ovrd;
                    map.AddTiberium(static_cast<unsigned char>(std::min<unsigned>(ovr, 0xFF)), ovrd);
                    oreCells.push_back(pos);
                }
            }

            map.UpdateMapPreviewAt(ax, ay);
            ++written;
        }
    }

    // ---- ore smoothing ----------------------------------------------------
    // Only the cells that actually received ore: running this over every
    // overlay would also reshape rocks and walls. A separate pass is needed
    // because SmoothTiberium looks at the eight neighbours, and inside the
    // write loop half of them still hold the old map's ore.
    for (int pos : oreCells)
        map.SmoothTiberium(pos);

    // ---- objects ----------------------------------------------------------
    if (req.withTrees)
    {
        for (const MapTerrainObject& tree : rmg.GetTerrainObjects())
        {
            if (tree.typeName == nullptr)
                continue;

            int ax = 0, ay = 0;
            GenToFa2(tree.coords.X, tree.coords.Y, dx, dy, ax, ay);
            if (!map.IsCoordInMap(ax, ay))
                continue;
            if (ax < ax1 || ax > ax2 || ay < ay1 || ay > ay2)
                continue;

            map.SetTerrainData(tree.typeName, map.GetCoordIndex(ax, ay));
        }
    }

    // [TEMP DIAG] Which tile indices actually landed on the map, and how the
    // selected rect splits between water / shore / land. Remove once the water
    // and shoreline generation looks right.
    {
        std::map<int, int> histogram;
        int waterCells = 0;
        int waterLevelCells = 0;
        int raisedCells = 0;
        int mismatched = 0;
        int sampleX = -1;
        int sampleY = -1;
        int sampleSrcTile = 0;
        int sampleSrcSub = 0;
        int sampleDstTile = 0;
        int sampleDstSub = 0;
        const int waterBase = rmg.GetWaterTileIndexDebug();

        for (int ay = ay1; ay <= ay2; ++ay)
        {
            for (int ax = ax1; ax <= ax2; ++ax)
            {
                if (!map.IsCoordInMap(ax, ay))
                    continue;
                MapCell* cell = GenCellAt(rmg, ax, ay, dx, dy);
                if (cell == nullptr)
                    continue;
                ++histogram[cell->IsoTileTypeIndex];
                if (cell->IsoTileTypeIndex >= waterBase
                    && cell->IsoTileTypeIndex < waterBase + 14)
                    ++waterCells;
                if (cell->Level > 0)
                    ++raisedCells;
                if (cell->IsoTileTypeIndex == 0xFFFF)
                    ++waterLevelCells;

                // Does what we wrote match what the generator holds?
                CellData* written = map.GetCellAt(ax, ay);
                if (written != nullptr)
                {
                    const unsigned short wantTile =
                        static_cast<unsigned short>(cell->IsoTileTypeIndex & 0xFFFF);
                    const unsigned char wantSub =
                        static_cast<unsigned char>(cell->Height & 0xFF);
                    if (written->TileIndex != wantTile || written->TileSubIndex != wantSub)
                    {
                        if (mismatched == 0)
                        {
                            sampleX = ax;
                            sampleY = ay;
                            sampleSrcTile = wantTile;
                            sampleSrcSub = wantSub;
                            sampleDstTile = written->TileIndex;
                            sampleDstSub = written->TileSubIndex;
                        }
                        ++mismatched;
                    }
                }
            }
        }

        std::vector<std::pair<int, int> > top(histogram.begin(), histogram.end());
        std::sort(top.begin(), top.end(),
                  [](const std::pair<int, int>& a, const std::pair<int, int>& b)
                  {
                      return a.second > b.second;
                  });

        std::string dump;
        char one[64];
        for (size_t i = 0; i < top.size() && i < 12; ++i)
        {
            sprintf_s(one, "%d x%d ", top[i].first, top[i].second);
            dump += one;
        }

        Logger::Debug("[MapGen] tiles: %s| water(%d..%d)=%d raised=%d empty=%d mismatch=%d@%d,%d(src %d/%d vs dst %d/%d)\n",
                      dump.c_str(), waterBase, waterBase + 13,
                      waterCells, raisedCells, waterLevelCells,
                      mismatched, sampleX, sampleY,
                      sampleSrcTile, sampleSrcSub, sampleDstTile, sampleDstSub);
    }

    if (req.withTech)
    {
        // Whatever the generator called the owner, the map has the final say:
        // an owner it does not list in [Houses] produces a structure FA2 cannot
        // resolve (and the game drops it on load).
        const FString houseName = ResolveHouseName(map.INI);

        for (const MapStructure& bld : rmg.GetStructures())
        {
            if (bld.typeName == nullptr)
                continue;

            int ax = 0, ay = 0;
            GenToFa2(bld.coords.X, bld.coords.Y, dx, dy, ax, ay);
            if (!map.IsCoordInMap(ax, ay))
                continue;
            if (ax < ax1 || ax > ax2 || ay < ay1 || ay > ay2)
                continue;

            map.SetBuildingData(nullptr, bld.typeName, houseName,
                                 map.GetCoordIndex(ax, ay), "");
        }
    }

    // Terrain objects live in the [Terrain] section, so the field data has to
    // be rebuilt; structures / units / infantry / aircraft go through their own
    // rebuilds (the same set FA2sp's own paste and undo paths use). The minimap
    // is redrawn by the caller once all of them are done.
    map.UpdateFieldTerrainData(false);
    CMapDataExt::UpdateFieldStructureData_Optimized();
    map.UpdateFieldUnitData(false);
    map.UpdateFieldInfantryData(false);
    map.UpdateFieldAircraftData(false);

    Logger::Debug("[MapGen] wrote %d cells\n", written);
    return MapGenError::None;
}
