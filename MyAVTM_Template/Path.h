#pragma once

#include "Vector3.h"
#include <vector>
#include <string>

struct PathPoint
{
    Vector3 position;
    float cumulativeDistance = 0.0f; // Distance from start of path along polyline
};

struct Path
{
    std::string name;
    std::vector<PathPoint> points;
    bool closedLoop = false;
    bool valid = false;
    float totalLength = 0.0f;

    void clear();

    // Builds a path from raw 3D positions (projected onto the driving plane if desired).
    // Filters out duplicate or near-identical consecutive points and validates length.
    bool buildFromPoints(const std::vector<Vector3>& rawPositions, bool isClosed, const std::string& pathName = "Path");

    // Segment querying
    int getSegmentCount() const;
    bool getSegment(int index, Vector3& outA, Vector3& outB, float& outSegLength, float& outStartDist) const;

    // Interpolates position along the path given distance along path
    Vector3 getPositionAtDistance(float distance) const;

    // Returns forward tangent direction at given distance
    Vector3 getTangentAtDistance(float distance) const;

    // Factory functions for predefined test paths (Stage 1)
    static Path createTrackLoopPath();
    static Path createStraightTestPath();
    static Path createTurnTestPath();
};
