#pragma once

#include <FA2PP.h>

#include "../../MapGenerator/MapGenerator/MapGen.h"

// Feed RandomMapGenerator's data tables from FA2's live editor state.
//
// The generator wants one TileCellAttr per TMP sub-cell (terrainType / rampType
// / frame presence / radar colour / the cell's footprint and Y adjustment).
// FA2 already holds exactly that: CMapDataExt::TileData is indexed by tile
// index, and each CTileTypeClass carries Width/Height (the TMP's cell grid)
// plus one CTileBlockClass per sub-cell. So the whole table is a straight copy
// and nothing has to be read from disk or from the game MIX.
//
// Everything else the generator needs (rulesmd.ini / artmd.ini /
// temperatmd.ini) is read from the game folder through
// RandomMapGenerator::s_gameIniDir, so there is no second copy of those tables
// to keep in sync either.
class CMapGenDataFeed
{
public:
    // Route the generator's INI lookups through the INIs FA2 already parsed
    // (CINI::Rules / Art / CurrentTheater) instead of files on disk.
    static void InstallIniSource();

    // Convert CMapDataExt::TileData into the generator's per-tile-cell table.
    // `out[i]` corresponds to absolute tile index i.
    static bool BuildTileCellAttrs(std::vector<std::vector<TileCellAttr> >& out);

    // One-shot: build the table and switch the generator into host-data mode.
    static bool Apply(RandomMapGenerator& rmg);

    // "[Map] Theater" of the open document as the generator's theater index
    // (0 = temperate, 1 = snow), or -1 for a theater it does not model.
    static int CurrentTheaterIndex();
};
