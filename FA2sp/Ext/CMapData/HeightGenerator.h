#pragma once
#include "Body.h"

#include <set>
#include <map>
#include <unordered_map>
#include <vector>
#include <random>
#include <stdexcept>
#include <functional>

// Height generator class
class HeightGenerator {
public:
    // Main entry: returns the generated height for each point
    std::set<VertexHeight> generateHeights(
        const std::set<VertexHeight>& points,
        double roughnessRatio,
        int minH,
        int maxH,
        const std::set<VertexHeight>& presets);

    static std::set<MapCoord> GetBoundaryCoords(const std::set<MapCoord>& coords);
    static std::set<MapCoord> GetInnerCoords(const std::set<MapCoord>& coords);
    static std::set<VertexHeight> GetBoundaryVertices(const std::set<MapCoord>& coords);

private:
    int N = 0;
    int edgeCount = 0;
    int roughEdges = 0;
    std::vector<MapCoord> coords;
    std::unordered_map<MapCoord, int> coord2idx;
    std::vector<int> adjOffset;
    std::vector<int> adjFlat;
    std::vector<int> height;
    std::vector<char> isPreset;   
    std::vector<int> low;
    std::vector<int> high;
    std::vector<int> freeNodes;

    std::minstd_rand rng;

    void reset();
    void buildGraph(const std::set<VertexHeight>& points);
    void applyPresets(const std::set<VertexHeight>& presets, int minH, int maxH);
    bool propagateConstraints(int minH, int maxH);
    bool propagateBoundedRanges(const std::vector<int>& presetIdx,
                                const std::vector<int>& initLow,
                                const std::vector<int>& initHigh,
                                int minH, int maxH,
                                std::vector<int>& loOut,
                                std::vector<int>& hiOut) const;
    void relaxPresets(int minH, int maxH);
    int countRoughEdges() const;
    void simulatedAnnealing(double targetRatio, int minH, int maxH);
};

