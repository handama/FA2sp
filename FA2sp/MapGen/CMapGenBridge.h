#pragma once

#include <FA2PP.h>

#include "../../MapGenerator/MapGenerator/MapGen.h"

// Parameters collected by the "Map Generator" tab of the terrain generator.
struct MapGenRequest
{
    RmLandType  landType = RmLandType::Continent;

    // Two independent seeds, each with its own roll button in the dialog.
    //   globalSeed  drives RollGlobalOptions: water amount, vegetation, ore
    //               layout, region sizing and the other map-wide knobs.
    //   mapSeed     drives the map's own Randomizer, i.e. the terrain body -
    //               the island skeleton and the mountain ranges.
    // Both are used verbatim, 0 included.
    unsigned    globalSeed = 0;
    unsigned    mapSeed = 0;

    bool        wholeMap = false;     // ignore the selection, regenerate everything
    bool        withOre = true;
    bool        withTrees = true;
    bool        withTech = true;

    int         oreAmount = 50;       // 0..100 slider
    int         vegetation = 60;      // 0..100 slider
};

// Why Run() refused or failed - the caller turns this into a translated string.
enum class MapGenError
{
    None = 0,
    NoMap,          // no document loaded
    Theater,        // theater is not Temperate / Snow
    NoTileData,     // CMapDataExt::TileData is empty
    RegionTooSmall, // selection below kMinRegionSide
    RegionTooLarge, // selection would overflow the generator's 512 stride
    GenFailed,      // GenerateMapBody refused the configuration
};

// Runs the random map generator and writes the result straight into CMapData.
//
// The generator reproduces the retail RMG, so it always produces a complete
// diamond map of its own. To paint a sub-region we size that diamond so the
// selection fits strictly inside it, then copy only the selected cells across.
// The two coordinate systems are transposed (see kGenXYTransposed below), which
// is isolated in GenCellAt / GenToFa2.
class CMapGenBridge
{
public:
    // Generate and write into the FA2 rectangle [ax1..ax2] x [ay1..ay2].
    // `wholeMap` in the request makes the rectangle irrelevant.
    static MapGenError Run(const MapGenRequest& req,
                           int ax1, int ay1, int ax2, int ay2);

    // Smallest selection the generator can work with, in cells.
    static constexpr int kMinRegionSide = 40;
};
