#include "Body.h"
#include <iostream>
#include <cmath>
#include "../../Source/CIsoView.h"
#include <random>
#include <chrono>
#include <numeric>
#include "../../Miscs/MultiSelection.h"
#include "../../FA2sp.h"
#include <queue>

static unsigned seed;
static float g_FlipRate = 0.10f;
// Hierarchical Worley noise with domain warping for organic clustered terrain.
// scale inverted: scale=1 -> largest clusters (~50 cells), scale=25 -> fine detail.
// Domain warping with bilinear-interpolated hash noise breaks Voronoi circularity.
// Detail level activates near coarse boundaries for varied fragmentation.
static int GetWorleyGroup(int cx, int cy, int scale, const std::vector<float>& cumulativeProbs, uint32_t baseSeed, float* outBoundaryRatio = nullptr)
{
    if (scale < 1) scale = 1;

    int numGroups = (int)cumulativeProbs.size();
    if (numGroups == 0) return 0;

    //--- Random rotation to prevent axis-aligned artifacts ---
    uint32_t angleSeed = baseSeed ^ 0xA5A5A5A5u;
    angleSeed = (angleSeed ^ (angleSeed >> 13)) * 1274126177u;
    angleSeed ^= (angleSeed >> 16);
    float angle = (angleSeed & 0x7FFFFFFFu) / float(0x7FFFFFFFu) * 2.0f * 3.14159265f;
    float cosA = cosf(angle);
    float sinA = sinf(angle);
    float rcx = (float)cx * cosA - (float)cy * sinA;
    float rcy = (float)cx * sinA + (float)cy * cosA;

    //--- Domain warping: smooth bilinear-interpolated hash to break Voronoi regularity ---
    // Warp spacing is ~1/3 of coarse spacing so warp varies smoothly across cells
    auto computeDomainWarp = [&](float baseSpacing) -> std::pair<float, float>
    {
        float warpSpacing = baseSpacing * 0.35f;
        if (warpSpacing < 3.0f) warpSpacing = 3.0f;

        float wgx = rcx / warpSpacing;
        float wgy = rcy / warpSpacing;
        int wgx0 = (int)floorf(wgx);
        int wgy0 = (int)floorf(wgy);
        float wfx = wgx - (float)wgx0;
        float wfy = wgy - (float)wgy0;

        // Smoothstep for organic interpolation
        float wsx = wfx * wfx * (3.0f - 2.0f * wfx);
        float wsy = wfy * wfy * (3.0f - 2.0f * wfy);

        auto getWarpVec = [&](int wx, int wy) -> std::pair<float, float>
        {
            uint32_t h = baseSeed ^ 0x77777777u;
            h ^= (uint32_t)(wx * 374761393u);
            h += (uint32_t)(wy * 668265263u);
            h = (h ^ (h >> 13)) * 1274126177u;
            h ^= (h >> 16);
            float vx = ((h & 0x7FFFFFFFu) / float(0x7FFFFFFFu) - 0.5f) * baseSpacing * 1.2f;
            float vy = (((h >> 16) & 0x7FFFFFFFu) / float(0x7FFFFFFFu) - 0.5f) * baseSpacing * 1.2f;
            return { vx, vy };
        };

        auto [w00x, w00y] = getWarpVec(wgx0, wgy0);
        auto [w10x, w10y] = getWarpVec(wgx0 + 1, wgy0);
        auto [w01x, w01y] = getWarpVec(wgx0, wgy0 + 1);
        auto [w11x, w11y] = getWarpVec(wgx0 + 1, wgy0 + 1);

        float wx = (1.0f - wsy) * ((1.0f - wsx) * w00x + wsx * w10x)
                 + wsy * ((1.0f - wsx) * w01x + wsx * w11x);
        float wy = (1.0f - wsy) * ((1.0f - wsx) * w00y + wsx * w10y)
                 + wsy * ((1.0f - wsx) * w01y + wsx * w11y);
        return { wx, wy };
    };

    float coarseSpacing = scale / 10.f;

    auto [warpX, warpY] = computeDomainWarp(coarseSpacing);
    float wrcx = rcx + warpX;
    float wrcy = rcy + warpY;

    //--- Utility: Warped Worley nearest-neighbor search ---
    auto findNearest = [&](float spacingF, uint32_t levelSeed,
        float qx, float qy,
        float& outDistSq, int& outGroup,
        bool findSecond, float& outDist2Sq, int& outGroup2) -> void
    {
        float jitter = spacingF * 0.50f;
        float perturbAmp = spacingF * 0.20f;
        float gx_f = qx / spacingF;
        float gy_f = qy / spacingF;
        int gx0 = (int)floorf(gx_f);
        int gy0 = (int)floorf(gy_f);

        outDistSq = FLT_MAX;
        outDist2Sq = FLT_MAX;
        outGroup = 0;
        outGroup2 = 0;

        for (int dx = -1; dx <= 2; dx++) {
            for (int dy = -1; dy <= 2; dy++) {
                int nx = gx0 + dx;
                int ny = gy0 + dy;

                uint32_t h = baseSeed ^ levelSeed;
                h ^= (uint32_t)(nx * 374761393u);
                h += (uint32_t)(ny * 668265263u);
                h = (h ^ (h >> 13)) * 1274126177u;
                h ^= (h >> 16);

                float jx = ((h & 0xFFFF) / 65535.0f) * 2.0f - 1.0f;
                float jy = (((h >> 16) & 0xFFFF) / 65535.0f) * 2.0f - 1.0f;

                float px = nx * spacingF + spacingF * 0.5f + jx * jitter;
                float py = ny * spacingF + spacingF * 0.5f + jy * jitter;

                float dxc = qx - px;
                float dyc = qy - py;
                float distSq = dxc * dxc + dyc * dyc;

                // Additive perturbation for rough edges
                if (spacingF > 2.0f) {
                    uint32_t ph = h ^ (uint32_t)(cx * 17 + cy * 31);
                    float perturb = ((ph & 0x7FFFFFFFu) / float(0x3FFFFFFFu) - 1.0f) * perturbAmp;
                    distSq += perturb;
                }

                uint32_t gh = h ^ 0x12345678u;
                gh = (gh ^ (gh >> 13)) * 1274126177u;
                gh ^= (gh >> 16);
                float groupRand = (gh & 0x7FFFFFFFu) / float(0x7FFFFFFFu);
                int group = 0;
                for (int g = 0; g < numGroups; g++) {
                    if (groupRand <= cumulativeProbs[g]) { group = g; break; }
                }

                if (distSq < outDistSq) {
                    if (findSecond) {
                        outDist2Sq = outDistSq;
                        outGroup2 = outGroup;
                    }
                    outDistSq = distSq;
                    outGroup = group;
                } else if (findSecond && distSq < outDist2Sq) {
                    outDist2Sq = distSq;
                    outGroup2 = group;
                }
            }
        }
    };

    //--- Phase 1: Coarse level on warped coordinates ---
    float coarseDistSq, coarseDist2Sq;
    int coarseGroup, coarseGroup2;
    findNearest(coarseSpacing, 0xCAFE0000u,
        wrcx, wrcy,
        coarseDistSq, coarseGroup, true, coarseDist2Sq, coarseGroup2);

    // Boundary detection via nearest/second-nearest ratio
    float boundaryRatio = coarseDistSq / coarseDist2Sq;

    // Interior: stable coarse region
    if (boundaryRatio < 0.70f) {
        if (outBoundaryRatio) *outBoundaryRatio = boundaryRatio;
        return coarseGroup;
    }

    //--- Phase 2: Detail level at boundaries for fragmentation ---
    float detailSpacing = 2.0f / (float)scale;
    if (detailSpacing < 2.0f) detailSpacing = 2.0f;
    if (detailSpacing > coarseSpacing * 0.3f) detailSpacing = coarseSpacing * 0.3f;

    // Detail uses original (non-warped) coords for truly fine detail
    float detailDistSq, detailDist2Sq;
    int detailGroup, detailGroup2;
    findNearest(detailSpacing, 0xDEAD0000u,
        rcx, rcy,
        detailDistSq, detailGroup, true, detailDist2Sq, detailGroup2);

    // Blend coarse ? detail near boundaries
    float detailInfluence = (boundaryRatio - 0.70f) / 0.30f;
    if (detailInfluence > 1.0f) detailInfluence = 1.0f;

    uint32_t mixHash = baseSeed;
    mixHash ^= (uint32_t)(cx * 1234567u + cy * 7654321u);
    mixHash = (mixHash ^ (mixHash >> 13)) * 1274126177u;
    mixHash ^= (mixHash >> 16);
    float mixRand = (mixHash & 0x7FFFFFFFu) / float(0x7FFFFFFFu);

    if (mixRand < detailInfluence) {
        if (outBoundaryRatio) *outBoundaryRatio = boundaryRatio;
        return detailGroup;
    }

    if (outBoundaryRatio) *outBoundaryRatio = boundaryRatio;
    return coarseGroup;
}

static int GetIndexByWeights(std::vector<float> weights) {
    float totalWeight = accumulate(weights.begin(), weights.end(), 0.0f);

    if (totalWeight > 1.0f) {
        for (float& weight : weights) {
            weight /= totalWeight; 
        }
        totalWeight = 1.0f;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    float randomValue = dis(gen); 

    if (totalWeight < 1.0f) {
        if (randomValue > totalWeight) {
            return -1; 
        }
    }

    float cumulativeWeight = 0.0f;
    for (size_t i = 0; i < weights.size(); ++i) {
        cumulativeWeight += weights[i];
        if (randomValue <= cumulativeWeight) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

static std::vector<int> getIgnoreTileSets(bool ignoreLandtypes)
{
    std::vector<int> roadSets;
    if (ignoreLandtypes)
        return roadSets;
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "PavedRoads", -1));
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "PavedRoadEnds", -1));
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "DirtRoadJunction", -1));
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "DirtRoadCurve", -1));
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "DirtRoadStraight", -1));
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "RoughGround", -1));
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "Medians", -1));
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "CliffRamps", -1));
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "SlopeSetPieces", -1));
    return roadSets;
}
static bool isLandTypeIgnored(int ttype, bool ignoreLandtypes)
{
    if (ignoreLandtypes)
        return false;
    switch (ttype)
    {
    case Rock7:
    case Rock8:
    case CliffRock:
    case Water:
    case Beach:
    case Railroad:
    case Tunnel:
        return true;
        break;
    default:
        break;
    }
    return false;
}

void CMapDataExt::CreateRandomGround(int TopX, int TopY, int BottomX, int BottomY, int scale, 
    std::vector<std::pair<std::vector<int>, float>> tiles,
    bool override, bool multiSelection, bool onlyClear, bool ignoreLandType)
{
    seed = (unsigned)std::chrono::system_clock::now().time_since_epoch().count();

    // Random global flip rate for this run: 0.05~0.15
    {
        uint32_t frs = (uint32_t)seed ^ 0xC0FFEEu;
        frs = (frs ^ (frs >> 13)) * 1274126177u;
        frs ^= (frs >> 16);
        g_FlipRate = 0.01f + ((frs & 0x7FFFFFFFu) / float(0x7FFFFFFFu)) * 0.05f;
    }

    std::vector<int> roadSets = getIgnoreTileSets(ignoreLandType);

    // used tile sets
    std::vector<int> tileindexes;

    // add clear tile if totalProbability < 1.0
    float totalProbability = 0.0f;
    for (const auto& tile : tiles) {
        totalProbability += tile.second;
        for (const auto& idx : tile.first) {
            tileindexes.push_back(idx);
        }
    }

    if (totalProbability < 1.0f) {
        tiles.push_back(std::make_pair(std::vector<int>{0xFFFF}, 1.0f - totalProbability));
    }

    // Build cumulative probabilities for Worley noise group assignment
    std::vector<float> cumulativeProbs;
    {
        float cumTotal = 0.0f;
        for (const auto& tile : tiles) {
            cumTotal += tile.second;
            cumulativeProbs.push_back(cumTotal);
        }
        if (cumTotal > 0.0f) {
            for (auto& p : cumulativeProbs) p /= cumTotal;
        }
    }

    auto tileNameHasShore = [&](int setIdx)
        {
            ppmfc::CString secName;
            secName.Format("TileSet%04d", setIdx);
            auto setName = CINI::CurrentTheater->GetString(secName, "SetName");
            setName.MakeLower();
            if (setName.Find("shore") != -1)
                return true;
            return false;
        };

    int numTiles = tiles.size();

    for (int i = TopX; i <= BottomX; ++i) {
        for (int j = TopY; j <= BottomY; ++j) {
            if (!CMapData::Instance->IsCoordInMap(i, j)) continue;
            CMapDataExt::CellDataExts[CMapData::Instance->GetCoordIndex(i, j)].AddRandomTile = false;
        }
    }

    for (int i = TopX; i <= BottomX; ++i) {
        for (int j = TopY; j <= BottomY; ++j) {
            if (!CMapData::Instance->IsCoordInMap(i, j)) continue;
            auto cell = CMapData::Instance->GetCellAt(i, j);
            if (CMapDataExt::IsHiddenCell(cell)) continue;
            if (multiSelection) {
                bool skip = true;
                for (const auto& coord : MultiSelection::SelectedCoords) {
                    if (coord.X == i && coord.Y == j)
                        skip = false;
                }
                if (skip)
                    continue;
            }

            if (std::find(tileindexes.begin(), tileindexes.end(), CMapDataExt::GetSafeTileIndex(cell->TileIndex)) == tileindexes.end()) {
                auto ttype = CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileBlockDatas[cell->TileSubIndex].TerrainType;
                if (isLandTypeIgnored(ttype, ignoreLandType)) {
                    continue;
                }
                if (std::find(roadSets.begin(), roadSets.end(), CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileSet) != roadSets.end()) {
                    continue;
                }
                auto tile = CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)];
                bool skip = false;
                for (int m = 0; m < tile.Width; m++)
                {
                    for (int n = 0; n < tile.Height; n++)
                    {
                        int subIdx = n * tile.Width + m;

                        if (CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileBlockDatas[subIdx].ImageData == NULL) {
                            continue;
                        }

                        auto ttype2 = CMapDataExt::GetExtension()->GetLandType(CMapDataExt::GetSafeTileIndex(cell->TileIndex), subIdx);
                        if (isLandTypeIgnored(ttype2, ignoreLandType)) {
                            skip = true;
                            break;
                        }
                    }
                }
                if (skip)
                    continue;

                if (CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileBlockDatas[cell->TileSubIndex].RampType != 0) continue;

                if (tileNameHasShore(CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileSet)) continue;
            }

            if (onlyClear) {
                CMapDataExt::GetExtension()->PlaceTileAt(i, j, 0);
                continue;
            }

            // Multi-scale Worley noise for clustered terrain distribution
            float boundaryRatio = 0.5f;
            int groupIdx = GetWorleyGroup(i, j, scale, cumulativeProbs, (uint32_t)seed, &boundaryRatio);

            // Edge-aware random flip (uniform group selection, no probability bias)
            {
                uint32_t flipSeed = (uint32_t)seed;
                flipSeed ^= (uint32_t)(i * 374761393u + j * 668265263u);
                flipSeed = (flipSeed ^ (flipSeed >> 13)) * 1274126177u;
                flipSeed ^= (flipSeed >> 16);
                float flipRand = (flipSeed & 0x7FFFFFFFu) / float(0x7FFFFFFFu);

                // Center: multiply flip rate by 0.20; Edge: full rate
                float flipRate = g_FlipRate * ((boundaryRatio > 0.70f) ? 1.0f : 0.20f);

                if (flipRand < flipRate) {
                    // Uniform random among all non-clear terrain groups
                    int lastNonClear = (int)tiles.size();
                    for (int t = lastNonClear - 1; t >= 0; t--) {
                        auto& tv = tiles[t].first;
                        if (!(tv.size() == 1 && tv[0] == 0xFFFF)) {
                            lastNonClear = t + 1;
                            break;
                        }
                    }
                    if (lastNonClear > 1) {
                        uint32_t rg = flipSeed ^ 0xBBBBBBBu;
                        rg = (rg ^ (rg >> 13)) * 1274126177u;
                        int newGroup = (int)((rg & 0x7FFFFFFFu) % (uint32_t)lastNonClear);
                        if (newGroup != groupIdx) {
                            groupIdx = newGroup;
                        }
                    }
                }
            }

            std::vector<int> tileIndexes = tiles[groupIdx].first;

            if (!override) {
                if (tileIndexes.size() == 1 && tileIndexes[0] == 0xFFFF) {
                    continue;
                }
            }

            CMapDataExt::GetExtension()->PlaceTileAt(i, j, STDHelpers::RandomSelectInt(tileIndexes), 1);
        }
    }
    if (!CFinalSunApp::Instance->DisableAutoShore) {
        CMapData::Instance->CreateShore(TopX - 1, TopY - 1, BottomX + 1, BottomY + 1);
    }
    if (!CFinalSunApp::Instance->DisableAutoLat) {
        for (int i = TopX - 2; i <= BottomX + 2; ++i) {
            for (int j = TopY - 2; j <= BottomY + 2; ++j) {
                if (!CMapData::Instance->IsCoordInMap(i, j))
                    continue;
                CMapDataExt::SmoothTileAt(i, j, true);
            }
        }
    }

}

void CMapDataExt::CreateRandomOverlay(int TopX, int TopY, 
    int BottomX, int BottomY, 
    std::vector<std::pair<std::vector<TerrainGeneratorOverlay>, float>> overlays,
    bool override, bool multiSelection, std::vector<MapCoord>& processedTiles, bool onlyClear, bool ignoreLandType)
{
    std::vector<float> weights;
    for (auto& ovr : overlays) {
        weights.push_back(ovr.second);
    }
    std::vector<int> roadSets = getIgnoreTileSets(ignoreLandType);
    for (int i = TopX; i <= BottomX; ++i) {
        for (int j = TopY; j <= BottomY; ++j) {
            if (!CMapData::Instance->IsCoordInMap(i, j)) continue;
            int pos = CMapData::Instance->GetCoordIndex(i, j);
            auto cell = CMapData::Instance->GetCellAt(pos);
            if (CMapDataExt::IsHiddenCell(cell)) continue;
            if (multiSelection) {
                bool skip = true;
                for (const auto& coord : MultiSelection::SelectedCoords) {
                    if (coord.X == i && coord.Y == j)
                        skip = false;
                }
                if (skip)
                    continue;
            }
            auto ttype = CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileBlockDatas[cell->TileSubIndex].TerrainType;

            if (isLandTypeIgnored(ttype, ignoreLandType)) {
                continue;
            }
            if (std::find(roadSets.begin(), roadSets.end(), CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileSet) != roadSets.end()) {
                continue;
            }
            if (override) {
                CMapDataExt::GetExtension()->SetNewOverlayAt(pos, 0xFFFF);
                CMapData::Instance->SetOverlayDataAt(pos, 0);
            }
            if (onlyClear) {
                CMapDataExt::GetExtension()->SetNewOverlayAt(pos, 0xFFFF);
                CMapData::Instance->SetOverlayDataAt(pos, 0);
                continue;
            }
            int randomIdx = GetIndexByWeights(weights);
            if (-1 < randomIdx && randomIdx < overlays.size()) {
                auto& overlayGroup = overlays[randomIdx].first;
                auto& overlay = overlayGroup[STDHelpers::RandomSelectInt(0, overlayGroup.size())];

                CMapDataExt::GetExtension()->SetNewOverlayAt(pos, overlay.Overlay);
                CMapData::Instance->SetOverlayDataAt(pos, STDHelpers::RandomSelectInt(overlay.AvailableOverlayData));
                auto&& data = CMapDataExt::GetOverlayTypeData(overlay.Overlay);
                if (data.TerrainRock || data.Rock)
                    processedTiles.push_back(MapCoord{ i, j });
            }
        }
    }
}

void CMapDataExt::CreateRandomTerrain(int TopX, int TopY,
    int BottomX, int BottomY,
    std::vector<std::pair<std::vector<FString>, float>> terrains,
    bool override, bool multiSelection, std::vector<MapCoord>& processedTiles, bool onlyClear, bool ignoreLandType)
{
    std::vector<float> weights;
    for (auto& ter : terrains) {
        weights.push_back(ter.second);
    }
    std::vector<int> roadSets = getIgnoreTileSets(ignoreLandType);
    for (int i = TopX; i <= BottomX; ++i) {
        for (int j = TopY; j <= BottomY; ++j) {
            if (!CMapData::Instance->IsCoordInMap(i, j)) continue;
            int pos = CMapData::Instance->GetCoordIndex(i, j);
            auto cell = CMapData::Instance->GetCellAt(pos);
            if (CMapDataExt::IsHiddenCell(cell)) continue;
            if (multiSelection) {
                bool skip = true;
                for (const auto& coord : MultiSelection::SelectedCoords) {
                    if (coord.X == i && coord.Y == j)
                        skip = false;
                }
                if (skip)
                    continue;
            }
            auto ttype = CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileBlockDatas[cell->TileSubIndex].TerrainType;

            if (isLandTypeIgnored(ttype, ignoreLandType)) {
                continue;
            }
            if (std::find(roadSets.begin(), roadSets.end(), CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileSet) != roadSets.end()) {
                continue;
            }
            if (override) {
                if (cell->Terrain > -1) {
                    CMapData::Instance->DeleteTerrainData(cell->Terrain);
                }
            }
            if (onlyClear) {
                if (cell->Terrain > -1) {
                    CMapData::Instance->DeleteTerrainData(cell->Terrain);
                }
                continue;
            }
            int randomIdx = GetIndexByWeights(weights);
            if (-1 < randomIdx && randomIdx < terrains.size()) {
                if (cell->Terrain > -1) {
                    CMapData::Instance->DeleteTerrainData(cell->Terrain);
                }
                if (std::find(processedTiles.begin(), processedTiles.end(), MapCoord{ i,j })
                    == processedTiles.end())
                {
                    auto& terrain = terrains[randomIdx].first;
                    CMapData::Instance->SetTerrainData(STDHelpers::RandomSelect(terrain), pos);
                }
            }
        }
    }
}

void CMapDataExt::CreateRandomSmudge(int TopX, int TopY,
    int BottomX, int BottomY, 
    std::vector<std::pair<std::vector<FString>, float>> smudges,
    bool override, bool multiSelection, bool onlyClear, bool ignoreLandType)
{
    std::vector<float> weights;
    for (auto& smu : smudges) {
        weights.push_back(smu.second);
    }
    std::vector<int> roadSets = getIgnoreTileSets(ignoreLandType);
    auto& rules = Variables::RulesMap;
    for (int i = TopX; i <= BottomX; ++i) {
        for (int j = TopY; j <= BottomY; ++j) {
            if (!CMapData::Instance->IsCoordInMap(i, j)) continue;
            int pos = CMapData::Instance->GetCoordIndex(i, j);
            auto cell = CMapData::Instance->GetCellAt(pos);
            if (CMapDataExt::IsHiddenCell(cell)) continue;
            if (multiSelection) {
                bool skip = true;
                for (const auto& coord : MultiSelection::SelectedCoords) {
                    if (coord.X == i && coord.Y == j)
                        skip = false;
                }
                if (skip)
                    continue;
            }
            auto ttype = CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileBlockDatas[cell->TileSubIndex].TerrainType;

            if (isLandTypeIgnored(ttype, ignoreLandType)) {
                continue;
            }
            if (std::find(roadSets.begin(), roadSets.end(), CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileSet) != roadSets.end()) {
                continue;
            }
            if (override) {
                if (cell->Smudge > -1) {
                    CMapData::Instance->DeleteSmudgeData(cell->Smudge);
                }
            }
            if (onlyClear) {
                if (cell->Smudge > -1) {
                    CMapData::Instance->DeleteSmudgeData(cell->Smudge);
                }
                continue;
            }
            int randomIdx = GetIndexByWeights(weights);
            if (-1 < randomIdx && randomIdx < smudges.size()) {
                auto& smudgeGroup = smudges[randomIdx].first;

                CSmudgeData smudge;
                smudge.X = j;
                smudge.Y = i;//opposite
                smudge.Flag = 0;
                smudge.TypeID = STDHelpers::RandomSelect(smudgeGroup).c_str();

                //check unplaceable tiles
                int width = rules.GetInteger(smudge.TypeID, "Width", 1);
                int height = rules.GetInteger(smudge.TypeID, "Height", 1);
                bool skip = false;
                for (int k = 0; k < width; k++)
                    for (int l = 0; l < height; l++)
                    {
                        if (!CMapData::Instance->IsCoordInMap(i + l, j + k)) continue;
                        auto cellCheck = CMapData::Instance->GetCellAt(i + l, j + k);

                        auto ttype = CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cellCheck->TileIndex)].TileBlockDatas[cellCheck->TileSubIndex].TerrainType;

                        if (isLandTypeIgnored(ttype, ignoreLandType)) {
                            skip = true;
                        }
                        if (CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cellCheck->TileIndex)].TileBlockDatas[cellCheck->TileSubIndex].RampType != 0) skip = true;
                    }

               
                //check overlapping
                if (!skip) {
                    for (auto& thisSmudge : CMapData::Instance->SmudgeDatas) {
                        if (thisSmudge.X <= 0 || thisSmudge.Y <= 0 || thisSmudge.Flag)
                            continue;
                        int thisWidth = rules.GetInteger(thisSmudge.TypeID, "Width", 1);
                        int thisHeight = rules.GetInteger(thisSmudge.TypeID, "Height", 1);
                        int thisX = thisSmudge.Y;
                        int thisY = thisSmudge.X;//opposite
                        for (int m = 0; m < thisWidth; m++)
                            for (int n = 0; n < thisHeight; n++)
                                for (int k = 0; k < width; k++)
                                    for (int l = 0; l < height; l++)
                                        if (thisY + m == j + k && thisX + n == i + l)
                                            skip = true;

                    }
                }
                if (!skip) {
                    if (cell->Smudge > -1) {
                        CMapData::Instance->DeleteSmudgeData(cell->Smudge);
                    }
                    CMapData::Instance->SetSmudgeData(&smudge);
                    CMapData::Instance->UpdateFieldSmudgeData(false);
                }
            }
        }
    }
}

static uint32_t g_NoiseSeed = 0;
static uint32_t g_NoiseSeed2 = 0;

static uint32_t GenerateRandomSeed()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis;
    return dis(gen);
}

static float HashNoise(int x, int y)
{
    uint32_t n = (uint32_t)x;
    n *= 374761393u;
    n += (uint32_t)y * 668265263u;
    n ^= g_NoiseSeed;
    n = (n ^ (n >> 13)) * 1274126177u;
    n ^= (n >> 16);
    return (n & 0x7fffffffu) / float(0x7fffffffu);
}

static float HashNoise2(int x, int y)
{
    uint32_t n = (uint32_t)x;
    n *= 374761393u;
    n += (uint32_t)y * 668265263u;
    n ^= g_NoiseSeed2;
    n = (n ^ (n >> 13)) * 1274126177u;
    n ^= (n >> 16);
    return (n & 0x7fffffffu) / float(0x7fffffffu);
}

static float Lerp(float a, float b, float t)
{
    return a + t * (b - a);
}

static float SmoothStep(float t)
{
    return t * t * (3.0f - 2.0f * t);
}

static float ValueNoise(float x, float y)
{
    int x0 = (int)floorf(x);
    int y0 = (int)floorf(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    float sx = SmoothStep(x - x0);
    float sy = SmoothStep(y - y0);

    float n00 = HashNoise(x0, y0);
    float n10 = HashNoise(x1, y0);
    float n01 = HashNoise(x0, y1);
    float n11 = HashNoise(x1, y1);

    float ix0 = Lerp(n00, n10, sx);
    float ix1 = Lerp(n01, n11, sx);

    return Lerp(ix0, ix1, sy);
}

static float ValueNoise2(float x, float y)
{
    int x0 = (int)floorf(x);
    int y0 = (int)floorf(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    float sx = SmoothStep(x - x0);
    float sy = SmoothStep(y - y0);

    float n00 = HashNoise2(x0, y0);
    float n10 = HashNoise2(x1, y0);
    float n01 = HashNoise2(x0, y1);
    float n11 = HashNoise2(x1, y1);

    float ix0 = Lerp(n00, n10, sx);
    float ix1 = Lerp(n01, n11, sx);

    return Lerp(ix0, ix1, sy);
}

static int GetCardinalLimit(bool steep)
{
    return steep ? 2 : 1;
}

static constexpr int DiagonalLimit = 1;

static std::unordered_map<MapCoord, int> ComputeDistanceToBoundary(
    const std::set<MapCoord>& region
)
{
    std::unordered_map<MapCoord, int> dist;
    std::queue<MapCoord> q;

    static const int dx[8] = { -1, 1, 0, 0, -1, 1, 1, -1 };
    static const int dy[8] = { 0, 0, -1, 1, -1, 1, -1, 1 };

    for (const auto& c : region)
    {
        bool isBorder = false;
        bool isSecondBorder = false;
        for (int i = 0; i < 4; i++)
        {
            MapCoord nb{ c.X + dx[i], c.Y + dy[i] };
            if (region.find(nb) == region.end())
            {
                isBorder = true;
                break;
            }
        }
        for (int i = 4; i < 8; i++)
        {
            MapCoord nb{ c.X + dx[i], c.Y + dy[i] };
            if (region.find(nb) == region.end())
            {
                isSecondBorder = true;
                break;
            }
        }

        if (isBorder)
        {
            dist[c] = 0;
            q.push(c);
        }
        if (isSecondBorder)
        {
            dist[c] = 1;
            q.push(c);
        }
    }

    while (!q.empty())
    {
        MapCoord cur = q.front();
        q.pop();

        int d = dist[cur];

        for (int i = 0; i < 4; i++)
        {
            MapCoord nb{ cur.X + dx[i], cur.Y + dy[i] };
            if (region.find(nb) == region.end())
                continue;

            if (!dist.count(nb))
            {
                dist[nb] = d + 1;
                q.push(nb);
            }
        }
    }

    return dist;
}

static void GetAllowedHeightRange(
    MapCoord coord,
    const std::unordered_map<MapCoord, int>& heights,
    const std::unordered_map<MapCoord, int>& boundaryDist,
    bool steep,
    int minHeight,
    int baseHeight,
    int maxHeight,
    int& outMin,
    int& outMax
)
{
    outMin = minHeight;
    outMax = maxHeight;

    int dist = 0;

    auto distIt = boundaryDist.find(coord);
    if (distIt != boundaryDist.end()) {
        dist = distIt->second;
        int boundaryMin = baseHeight - dist;
        int boundaryMax = baseHeight + dist;
        if (dist <= 1)
            boundaryMax = baseHeight;
        outMin = std::max(outMin, boundaryMin);
        outMax = std::min(outMax, boundaryMax);
    }

    if (outMin > outMax) {
        outMin = outMax = baseHeight;
    }
}

static void ProjectNoiseToValidHeights(
    const std::set<MapCoord>& region,
    std::unordered_map<MapCoord, int>& heights,
    const std::unordered_map<MapCoord, int>& expected,
    const std::unordered_map<MapCoord, int>& distToBoundary,
    bool steep,
    int minHeight,
    int baseHeight,
    int maxHeight,
    int relaxIterations
)
{
    //for (int iter = 0; iter < relaxIterations; iter++)
    //{
    for (auto& c : region)
    {
        int minH, maxH;
        GetAllowedHeightRange(
            c,
            heights,
            distToBoundary,
            steep,
            minHeight,
            baseHeight,
            maxHeight,
            minH,
            maxH
        );

        int target = expected.at(c);
        heights[c] = std::clamp(target, minH, maxH);
    }
    //}
}

static float ComputeLinearHeightOffset(
    const MapCoord& p,
    const MapCoord& start,
    const MapCoord& end,
    int startHeight,
    int endHeight
)
{
    float vx = float(end.X - start.X);
    float vy = float(end.Y - start.Y);

    float wx = float(p.X - start.X);
    float wy = float(p.Y - start.Y);

    float len2 = vx * vx + vy * vy;
    if (len2 < 1e-6f)
        return float(startHeight);

    float t = (wx * vx + wy * vy) / len2;
    t = std::clamp(t, 0.0f, 1.0f);

    float base = Lerp(float(startHeight), float(endHeight), t);

    float noise =
        HashNoise(p.X * 17, p.Y * 31) * 2.0f - 1.0f;

    const float jitterAmplitude = 2.0f;

    return base + noise * jitterAmplitude;
}

void CMapDataExt::GenerateNoiseSlopeTerrain(
    const std::set<MapCoord>& region,
    int minHeight,
    int baseHeight,
    int maxHeight,
    int minMarcoHeight,
    int maxMarcoHeight,
    bool steep,
    float frequency,
    float macroFrequency,
    int relaxIterations,
    MapCoord start,
    MapCoord end,
    int startHeight,
    int endHeight,
    bool avoidEdges
)
{
    if (region.empty()) return;

    g_NoiseSeed = GenerateRandomSeed();
    g_NoiseSeed2 = GenerateRandomSeed();

    std::unordered_map<MapCoord, int> expected;
    std::unordered_map<MapCoord, int> heights;

    for (auto& c : region)
    {
        float linearOffset = (start.X == 0 && start.Y == 0) ? 0.0f : ComputeLinearHeightOffset(
            c, start, end, startHeight, endHeight
        );

        float macroNoise =
            ValueNoise2(c.X * macroFrequency, c.Y * macroFrequency);

        float macroOffset = (macroNoise * 2.0f - 1.0f) * (maxMarcoHeight - minMarcoHeight);

        float n = ValueNoise(c.X * frequency, c.Y * frequency); 

        int hgt = minHeight 
            + (int)std::round(n * (maxHeight - minHeight)
            + linearOffset 
            + (macroFrequency == 0.0f ? 0 : macroOffset));

        expected[c] = hgt; 
        heights[c] = hgt;
    }

    minHeight = 0;
    maxHeight = 14;

    for (auto& [c, h] : expected)
    {
        h = std::clamp(h, minHeight, maxHeight);
    }
    for (auto& [c, h] : heights)
    {
        h = std::clamp(h, minHeight, maxHeight);
    }

    if (avoidEdges)
    {
        auto distToBoundary = ComputeDistanceToBoundary(region);
        ProjectNoiseToValidHeights(
            region,
            heights,
            expected,
            distToBoundary,
            steep,
            minHeight,
            baseHeight,
            maxHeight,
            relaxIterations
        );
    }

    for (auto& c : region)
    {
        auto cell = TryGetCellAt(c.X, c.Y);
        if (!cell) continue;

        cell->Height = std::clamp(heights[c], 0, 14);

        int idx = CMapData::Instance->GetCoordIndex(c.X, c.Y);
        CellDataExts[idx].Adjusted = true;
    }

    std::vector<int> ignoreList;
    for (int i = 0; i < CMapDataExt::CellDataExts.size(); ++i)
    {
        int x = CMapData::Instance->GetXFromCoordIndex(i);
        int y = CMapData::Instance->GetYFromCoordIndex(i);
        if (region.find({ x,y }) == region.end())
            ignoreList.push_back(i);
    }

    CMapDataExt::CheckCellLow(steep, 0, false, &ignoreList);
    CMapDataExt::CheckCellRise(steep, 0, false, &ignoreList);

    for (int i = 1; i < CMapDataExt::CellDataExts.size(); i++) // skip 0
    {
        if (CMapDataExt::CellDataExts[i].Adjusted)
        {
            int thisX = CMapData::Instance->GetXFromCoordIndex(i);
            int thisY = CMapData::Instance->GetYFromCoordIndex(i);
            int loops[3] = { 0, -1, 1 };
            for (int i : loops)
                for (int e : loops)
                {
                    int newX = thisX + i;
                    int newY = thisY + e;
                    int pos = CMapData::Instance->GetCoordIndex(newX, newY);
                    if (!CMapDataExt::IsCoordInFullMap(pos)) continue;
                    CMapDataExt::CellDataExts[pos].CreateSlope = true;
                }
        }
    }
    for (int i = 1; i < CMapDataExt::CellDataExts.size(); i++) // skip 0
    {
        if (CMapDataExt::CellDataExts[i].CreateSlope)
        {
            int thisX = CMapData::Instance->GetXFromCoordIndex(i);
            int thisY = CMapData::Instance->GetYFromCoordIndex(i);
            CMapDataExt::CreateSlopeAt(thisX, thisY);
        }
    }
    for (int i = 0; i < CMapDataExt::CellDataExts.size(); i++)
    {
        if (CMapDataExt::CellDataExts[i].CreateSlope || CMapDataExt::CellDataExts[i].Adjusted)
        {
            int thisX = CMapData::Instance->GetXFromCoordIndex(i);
            int thisY = CMapData::Instance->GetYFromCoordIndex(i);
            CMapData::Instance->UpdateMapPreviewAt(thisX, thisY);
        }
    }
}
