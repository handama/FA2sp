#include "HeightGenerator.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <stdexcept>
#include <random>

template<typename T>
std::vector<T> GetModes(const std::vector<T>& vec)
{
    std::unordered_map<T, int> count;
    int maxCount = 0;

    for (const auto& x : vec)
        maxCount = std::max(maxCount, ++count[x]);

    std::vector<T> result;
    for (const auto& [value, cnt] : count)
        if (cnt == maxCount)
            result.push_back(value);

    return result;
}
void HeightGenerator::reset() {
    N = 0;
    edgeCount = 0;
    roughEdges = 0;
    coords.clear();
    coord2idx.clear();
    adjOffset.clear();
    adjFlat.clear();
    edgeMaxDiff.clear();
    edgeIgnoreConstraint.clear();
    height.clear();
    isPreset.clear();
    low.clear();
    high.clear();
    freeNodes.clear();
    std::random_device rd;
    rng.seed(rd());
}
	
void HeightGenerator::buildGraph(const std::set<VertexHeight>& points) {
    N = static_cast<int>(points.size());
    coords.reserve(N);

    // Temporary adjacency list, flattened after construction
    std::vector<std::vector<int>> tmpAdj(N);
    for (auto& a : tmpAdj) a.reserve(4);

    int idx = 0;
    for (const auto& p : points) {
        coords.push_back({p.X, p.Y});
        coord2idx[{p.X, p.Y}] = idx++;
    }

    // 4-connected neighbors
    const int dx[] = { 1, -1, 0, 0 };
    const int dy[] = { 0, 0, 1, -1 };
    edgeCount = 0;
    for (int i = 0; i < N; ++i) {
        int x = coords[i].X;
        int y = coords[i].Y;
        for (int k = 0; k < 4; ++k) {
            MapCoord neighbor{ x + dx[k], y + dy[k] };
            auto it = coord2idx.find(neighbor);
            if (it != coord2idx.end()) {
                int j = it->second;
                if (i < j) { // Each undirected edge added only once
                    tmpAdj[i].push_back(j);
                    tmpAdj[j].push_back(i);
                    edgeCount++;
                }
            }
        }
    }

    // Flatten to adjOffset + adjFlat
    adjOffset.resize(N + 1);
    adjOffset[0] = 0;
    for (int i = 0; i < N; ++i) {
        adjOffset[i + 1] = adjOffset[i] + static_cast<int>(tmpAdj[i].size());
    }
    adjFlat.resize(adjOffset[N]);
    for (int i = 0; i < N; ++i) {
        std::copy(tmpAdj[i].begin(), tmpAdj[i].end(), adjFlat.begin() + adjOffset[i]);
    }
    edgeMaxDiff.assign(adjFlat.size(), 1);
    edgeIgnoreConstraint.assign(adjFlat.size(), 0);

    // Mark edges where both adjacent cells are non-morphable ¡ª height constraint is ignored
    auto isCellMorphable = [](CellData* cell) -> bool {
        int tileIndex = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
        return CMapDataExt::TileData[tileIndex].Morphable;
    };
    for (int u = 0; u < N; ++u) {
        int x = coords[u].X;
        int y = coords[u].Y;
        for (int ei = adjOffset[u]; ei < adjOffset[u + 1]; ++ei) {
            int v = adjFlat[ei];
            if (u >= v) continue; // each undirected edge once
            int dx = coords[v].X - x;
            int dy = coords[v].Y - y;
            CellData *c1 = nullptr, *c2 = nullptr;
            if (dx != 0) { // horizontal edge
                int cx = (dx > 0) ? x : x - 1;
                c1 = CMapDataExt::TryGetCellAt(cx, y - 1);
                c2 = CMapDataExt::TryGetCellAt(cx, y);
            } else { // vertical edge
                int cy = (dy > 0) ? y : y - 1;
                c1 = CMapDataExt::TryGetCellAt(x - 1, cy);
                c2 = CMapDataExt::TryGetCellAt(x, cy);
            }
            if (!isCellMorphable(c1) && !isCellMorphable(c2)) {
                edgeIgnoreConstraint[ei] = 1;
                // Find and mark reverse edge
                for (int re = adjOffset[v]; re < adjOffset[v + 1]; ++re) {
                    if (adjFlat[re] == u) {
                        edgeIgnoreConstraint[re] = 1;
                        break;
                    }
                }
            }
        }
    }
}

void HeightGenerator::applyPresets(const std::set<VertexHeight>& presets, int minH, int maxH) {
    isPreset.assign(N, 0);
    height.assign(N, minH); // Temporary fill, will be overwritten later

    for (const auto& p : presets) {
        MapCoord coord{ p.X, p.Y };
        auto it = coord2idx.find(coord);
        if (it == coord2idx.end()) {
            Logger::Raw("Preset point out of bounds: %d, %d\n", p.X, p.Y);
            continue; // Presets not in the point set are skipped directly
        }
        // Clamp height to valid range [minH, maxH]
        int h = p.Height;
        if (h < minH) h = minH;
        else if (h > maxH) h = maxH;
        int idx = it->second;
        isPreset[idx] = 1;
        height[idx] = h;
    }

    // Build free node list (non-preset nodes), for direct random sampling in SA inner loop
    freeNodes.clear();
    freeNodes.reserve(N);
    for (int i = 0; i < N; ++i) {
        if (!isPreset[i]) freeNodes.push_back(i);
    }
}

bool HeightGenerator::propagateConstraints(int minH, int maxH) {
    low.assign(N, minH);
    high.assign(N, maxH);
    std::queue<int> q;

    // Preset point ranges fixed to their own heights
    for (int i = 0; i < N; ++i) {
        if (isPreset[i]) {
            low[i] = high[i] = height[i];
            q.push(i);
        }
    }

    while (!q.empty()) {
        int u = q.front();
        q.pop();
        for (int ei = adjOffset[u]; ei < adjOffset[u + 1]; ++ei) {
            if (edgeIgnoreConstraint[ei]) continue;
            int v = adjFlat[ei];
            // h[v] must be within [h[u]-1, h[u]+1]
            int newLow  = std::max(low[v], low[u] - 1);
            int newHigh = std::min(high[v], high[u] + 1);
            if (newLow > newHigh) {
                return false; // Empty range, no solution
            }
            if (newLow > low[v] || newHigh < high[v]) {
                low[v] = newLow;
                high[v] = newHigh;
                q.push(v);
            }
        }
    }
    return true;
}

// Progressive relaxation: when presets conflict, increase edge maxDiff step by step
// until all constraints are satisfied. Preset heights are never modified.
void HeightGenerator::propagateConstraintsRelaxed(int minH, int maxH) {
    const int maxRelax = maxH - minH;
    edgeMaxDiff.assign(adjFlat.size(), 1);

    for (int iter = 0; iter < maxRelax * N; ++iter) {
        // Reset bounds from scratch each iteration
        low.assign(N, minH);
        high.assign(N, maxH);
        for (int i = 0; i < N; ++i) {
            if (isPreset[i]) {
                low[i] = high[i] = height[i];
            }
        }

        std::queue<int> q;
        std::vector<char> inQ(N, 0);
        for (int i = 0; i < N; ++i) {
            if (isPreset[i]) {
                q.push(i);
                inQ[i] = 1;
            }
        }

        bool hasConflict = false;

        while (!q.empty()) {
            int u = q.front(); q.pop();
            inQ[u] = 0;
            for (int ei = adjOffset[u]; ei < adjOffset[u + 1]; ++ei) {
                if (edgeIgnoreConstraint[ei]) continue;
                int v = adjFlat[ei];
                int diff = edgeMaxDiff[ei];
                int newLow = std::max(low[v], low[u] - diff);
                int newHigh = std::min(high[v], high[u] + diff);

                if (newLow > newHigh) {
                    // Conflict: increase diff on both directions
                    if (diff < maxRelax) {
                        edgeMaxDiff[ei] = diff + 1;
                        // Find and update reverse edge
                        for (int re = adjOffset[v]; re < adjOffset[v + 1]; ++re) {
                            if (adjFlat[re] == u) {
                                edgeMaxDiff[re] = diff + 1;
                                break;
                            }
                        }
                        hasConflict = true;
                    }
                    continue;
                }

                if (newLow > low[v] || newHigh < high[v]) {
                    low[v] = newLow;
                    high[v] = newHigh;
                    if (!inQ[v]) {
                        q.push(v);
                        inQ[v] = 1;
                    }
                }
            }
        }

        if (!hasConflict) break;
    }
}

// Constraint propagation with "preset ranges": preset nodes' range is [initLow, initHigh],
// other nodes use [minH, maxH]. Returns feasibility and outputs converged bounds loOut/hiOut.
bool HeightGenerator::propagateBoundedRanges(
    const std::vector<int>& presetIdx,
    const std::vector<int>& initLow,
    const std::vector<int>& initHigh,
    int minH, int maxH,
    std::vector<int>& loOut,
    std::vector<int>& hiOut) const
{
    loOut.assign(N, minH);
    hiOut.assign(N, maxH);
    for (size_t k = 0; k < presetIdx.size(); ++k) {
        loOut[presetIdx[k]] = initLow[k];
        hiOut[presetIdx[k]] = initHigh[k];
    }
    std::queue<int> q;
    std::vector<char> inQ(N, 0);
    for (int idx : presetIdx) { q.push(idx); inQ[idx] = 1; }
    while (!q.empty()) {
        int u = q.front(); q.pop(); inQ[u] = 0;
        for (int ei = adjOffset[u]; ei < adjOffset[u + 1]; ++ei) {
            int v = adjFlat[ei];
            int nlo = std::max(loOut[v], loOut[u] - 1);
            int nhi = std::min(hiOut[v], hiOut[u] + 1);
            if (nlo > nhi) return false;
            if (nlo > loOut[v] || nhi < hiOut[v]) {
                loOut[v] = nlo;
                hiOut[v] = nhi;
                if (!inQ[v]) { q.push(v); inQ[v] = 1; }
            }
        }
    }
    return true;
}

// Called when propagateConstraints detects infeasibility: adjusts preset heights with minimal
// modification to make the overall constraints feasible.
void HeightGenerator::relaxPresets(int minH, int maxH) {
    std::vector<int> presetIdx;
    for (int i = 0; i < N; ++i) if (isPreset[i]) presetIdx.push_back(i);
    const int P = static_cast<int>(presetIdx.size());
    if (P <= 1) return; // Single point or no presets won't conflict

    std::vector<int> origH(P);
    for (int i = 0; i < P; ++i) origH[i] = height[presetIdx[i]];

    auto buildRanges = [&](int t, std::vector<int>& il, std::vector<int>& ih) {
        il.resize(P);
        ih.resize(P);
        for (int i = 0; i < P; ++i) {
            int l = origH[i] - t; if (l < minH) l = minH;
            int r = origH[i] + t; if (r > maxH) r = maxH;
            il[i] = l;
            ih[i] = r;
        }
    };

    std::vector<int> il, ih, lo, hi;
    int loT = 0;
    int hiT = maxH - minH;

    // Invariant: hiT is always feasible (preset range fully open to [minH, maxH], no tightening)
    buildRanges(hiT, il, ih);
    if (!propagateBoundedRanges(presetIdx, il, ih, minH, maxH, lo, hi)) {
        // Theoretically unreachable; defensive fallback: unify all presets to minH
        for (int i = 0; i < N; ++i) if (isPreset[i]) height[i] = minH;
        return;
    }
    while (loT < hiT) {
        int mid = (loT + hiT) / 2;
        buildRanges(mid, il, ih);
        if (propagateBoundedRanges(presetIdx, il, ih, minH, maxH, lo, hi)) {
            hiT = mid;
        } else {
            loT = mid + 1;
        }
    }
    // Recompute ranges with minimal t
    buildRanges(loT, il, ih);
    propagateBoundedRanges(presetIdx, il, ih, minH, maxH, lo, hi);

    for (int i = 0; i < P; ++i) {
        height[presetIdx[i]] = lo[presetIdx[i]];
    }
}

int HeightGenerator::countRoughEdges() const {
    int count = 0;
    for (int u = 0; u < N; ++u) {
        for (int ei = adjOffset[u]; ei < adjOffset[u + 1]; ++ei) {
            int v = adjFlat[ei];
            if (u < v) { // Each edge counted only once
                if (edgeIgnoreConstraint[ei]) continue; // Unconstrained edge
                if (edgeMaxDiff[ei] > 1) continue; // Relaxed edge, never counts as rough
                int diff = height[u] - height[v];
                if (diff < 0) diff = -diff;
                if (diff == 1) count++;
            }
        }
    }
    return count;
}

inline double fast_exp_neg(double x) {
    if (x > 20.0) return 0.0;
    if (x < 1e-4) return 1.0 - x;
    double x2 = x * x;
    return 1.0 / (1.0 + x + 0.5 * x2 + 0.16666666666666666 * x2 * x);
}

void HeightGenerator::simulatedAnnealing(double targetRatio, int minH, int maxH) {
    if (edgeCount == 0) return;

    const int freeCnt = static_cast<int>(freeNodes.size());
    if (freeCnt == 0) return; // All nodes are presets, no adjustment needed

    const double invEdgeCount = 1.0 / edgeCount;
    const double invRngMax = 1.0 / static_cast<double>(rng.max());
    const unsigned int rngMod = static_cast<unsigned int>(freeCnt);

    int currentRough = roughEdges;
    const int targetRough = static_cast<int>(targetRatio * edgeCount + 0.5);

    auto roughHit = [&]() {
        int d = currentRough - targetRough;
        return (d < 0 ? -d : d) <= 1;
    };

    double T = 1.0;
    const double T_min = 1e-6;
    const double T_lock = 1e-3;     
    const double alpha = 0.98;      
    const int iterPerT = 5 * N;     

    double diff = static_cast<double>(currentRough) * invEdgeCount - targetRatio;
    double currentE = diff * diff;

    bool hitTarget = false;

    while (T > T_min) {
        for (int it = 0; it < iterPerT; ++it) {
            // Random vertex + random single direction (proposal distribution consistent with original algorithm, maintains unbiasedness)
            int v = freeNodes[rng() % rngMod];
            int newH = height[v] + ((rng() & 1u) ? 1 : -1);
            if (newH < low[v] || newH > high[v]) continue;

            int oldH = height[v];
            int deltaRough = 0;
            bool feasible = true;
            for (int ei = adjOffset[v]; ei < adjOffset[v + 1]; ++ei) {
                if (edgeIgnoreConstraint[ei]) continue;
                int u = adjFlat[ei];
                int hu = height[u];
                int maxDiff = edgeMaxDiff[ei];
                int oldDiff = oldH - hu;
                if (oldDiff < 0) oldDiff = -oldDiff;
                int newDiff = newH - hu;
                if (newDiff < 0) newDiff = -newDiff;
                if (newDiff > maxDiff) { feasible = false; break; }
                if (maxDiff == 1) {
                    if (oldDiff == 1 && newDiff == 0) deltaRough--;
                    else if (oldDiff == 0 && newDiff == 1) deltaRough++;
                }
            }
            if (!feasible) continue;

            int newRough = currentRough + deltaRough;
            double newRatio = static_cast<double>(newRough) * invEdgeCount;
            diff = newRatio - targetRatio;
            double newE = diff * diff;
            double deltaE = newE - currentE;

            // Metropolis criterion: accept if deltaE < 0, otherwise accept with probability
            if (deltaE < 0 || (rng() * invRngMax) < fast_exp_neg(deltaE / T)) {
                height[v] = newH;
                currentRough = newRough;
                currentE = newE;
                // Only stop on target hit after temperature is low enough (sufficient mixing)
                if (T <= T_lock && roughHit()) { hitTarget = true; break; }
            }
        }
        if (hitTarget) break;
        T *= alpha;
    }
    roughEdges = currentRough;
}

std::set<VertexHeight> HeightGenerator::generateHeights(
    const std::set<VertexHeight>& points,
    double roughnessRatio,
    int minH,
    int maxH,
    const std::set<VertexHeight>& presets,
    bool bPreserveAnchorHeights)
{
    reset();

    buildGraph(points);
    applyPresets(presets, minH, maxH);

    if (bPreserveAnchorHeights) {
        // Presets are fixed ¡ª use progressive relaxation to resolve conflicts
        // by increasing edge maxDiff instead of modifying preset heights
        propagateConstraintsRelaxed(minH, maxH);
    } else {
        if (!propagateConstraints(minH, maxH)) {
            // Preset constraint conflict: adjust preset heights with minimal modification, then re-propagate
            relaxPresets(minH, maxH);
            propagateConstraints(minH, maxH);
        }
    }

    // Initial solution takes the lower bound of each point's feasible range
    height = low;
    roughEdges = countRoughEdges();

    simulatedAnnealing(roughnessRatio, minH, maxH);

    std::set<VertexHeight> result;
    for (int i = 0; i < N; ++i) {
        result.insert(VertexHeight{(short)coords[i].X, (short)coords[i].Y, (char)height[i]});
    }
    return result;
}

std::set<MapCoord> HeightGenerator::GetBoundaryCoords(const std::set<MapCoord>& coords) {
    std::set<MapCoord> result;
    for (const auto& p : coords) {
        // Check if all four-direction neighbors are in the set
        bool hasUp    = coords.find({p.X, p.Y + 1}) != coords.end();
        bool hasDown  = coords.find({p.X, p.Y - 1}) != coords.end();
        bool hasLeft  = coords.find({p.X - 1, p.Y}) != coords.end();
        bool hasRight = coords.find({p.X + 1, p.Y}) != coords.end();

        // If any neighbor is missing, this point is a boundary point
        if (!(hasUp && hasDown && hasLeft && hasRight)) {
            result.insert(p);
        }
    }
    return result;
}

std::set<MapCoord> HeightGenerator::GetInnerCoords(const std::set<MapCoord>& coords)
{
    std::set<MapCoord> result;
    for (const auto& p : coords) {
        // Check if all four-direction neighbors are in the set
        bool hasUp    = coords.find({p.X, p.Y + 1}) != coords.end();
        bool hasDown  = coords.find({p.X, p.Y - 1}) != coords.end();
        bool hasLeft  = coords.find({p.X - 1, p.Y}) != coords.end();
        bool hasRight = coords.find({p.X + 1, p.Y}) != coords.end();

        // Inner point: all four neighbors must be present
        if (hasUp && hasDown && hasLeft && hasRight) {
            result.insert(p);
        }
    }
    return result;
}

std::set<VertexHeight> HeightGenerator::GetBoundaryVertices(const std::set<MapCoord>& coords) {
    std::set<VertexHeight> result;
    // Collect all four vertices of each cell (consistent with VertexType convention)
    std::set<VertexHeight> allVertices;
    for (const auto& c : coords) {
        allVertices.insert({c.X,     c.Y    });     // Top
        allVertices.insert({c.X,     c.Y + 1});     // Right
        allVertices.insert({c.X + 1, c.Y + 1});     // Bottom
        allVertices.insert({c.X + 1, c.Y    });     // Left
    }
    // If any adjacent cell is missing, this vertex is a boundary vertex
    for (const auto& v : allVertices) {
        bool hasBR = coords.find({v.X,     v.Y    }) != coords.end();
        bool hasTR = coords.find({v.X,     v.Y - 1}) != coords.end();
        bool hasTL = coords.find({v.X - 1, v.Y - 1}) != coords.end();
        bool hasBL = coords.find({v.X - 1, v.Y    }) != coords.end();

        if (!(hasBR && hasTR && hasTL && hasBL)) {
			CellData* cells[4];
            cells[0] = CMapDataExt::TryGetCellAt(v.X - 1, v.Y - 1); 
            cells[1] = CMapDataExt::TryGetCellAt(v.X - 1, v.Y);
            cells[2] = CMapDataExt::TryGetCellAt(v.X, v.Y); 
            cells[3] = CMapDataExt::TryGetCellAt(v.X, v.Y - 1);
            std::vector<int> heights;
            heights.reserve(4);
			for (int i = 0; i < 4; ++i)
            {
                auto cell = cells[i];
                int h = cell->Height;
                const auto& tileData = CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)];
                if (tileData.TileBlockCount > cell->TileSubIndex)
                {
                    auto type = int(tileData.TileBlockDatas[cell->TileSubIndex].RampType) - 1;
                    if (type >= 0 && type < RampTypes.size())
                    {
                        auto rampType = RampTypes.at(type);
                        switch (VertexType(i))
                        {	
                        case VertexType_Top:
                            h += rampType.bottom;
                            break;
                        case VertexType_Right:
                            h += rampType.left;
                            break;
                        case VertexType_Bottom:
                            h += rampType.top;
                            break;
                        case VertexType_Left:
                            h += rampType.right;
                            break;	
                        default:
                            break;
                        }
                    }
                }
                heights.push_back(h);
            }

			auto modes = GetModes(heights);
			result.insert({v.X, v.Y, *std::min_element(modes.begin(), modes.end())});
        }
    }
    return result;
}
