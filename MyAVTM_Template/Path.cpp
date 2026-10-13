#include "Path.h"
#include <cmath>
#include <algorithm>

void Path::clear()
{
    name.clear();
    points.clear();
    closedLoop = false;
    valid = false;
    totalLength = 0.0f;
}

bool Path::buildFromPoints(const std::vector<Vector3>& rawPositions, bool isClosed, const std::string& pathName)
{
    clear();
    name = pathName;
    closedLoop = isClosed;

    if (rawPositions.size() < 2)
    {
        valid = false;
        return false;
    }

    // Filter consecutive duplicates and validate finite coordinates
    std::vector<Vector3> filtered;
    filtered.reserve(rawPositions.size());

    for (const auto& p : rawPositions)
    {
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
        {
            valid = false;
            return false;
        }

        if (filtered.empty())
        {
            filtered.push_back(p);
        }
        else
        {
            float d = Distance(filtered.back(), p);
            if (d > 0.01f) // Filter out duplicates closer than 1cm
            {
                filtered.push_back(p);
            }
        }
    }

    // For closed loops, if the last point is identical to the first, remove it
    if (isClosed && filtered.size() > 2)
    {
        float d = Distance(filtered.front(), filtered.back());
        if (d <= 0.01f)
        {
            filtered.pop_back();
        }
    }

    size_t minPoints = isClosed ? 3 : 2;
    if (filtered.size() < minPoints)
    {
        valid = false;
        return false;
    }

    // Build cumulative distances
    points.resize(filtered.size());
    points[0].position = filtered[0];
    points[0].cumulativeDistance = 0.0f;

    float runningDist = 0.0f;
    for (size_t i = 1; i < filtered.size(); ++i)
    {
        float segLen = Distance(filtered[i - 1], filtered[i]);
        runningDist += segLen;
        points[i].position = filtered[i];
        points[i].cumulativeDistance = runningDist;
    }

    if (isClosed)
    {
        float closingLen = Distance(filtered.back(), filtered.front());
        totalLength = runningDist + closingLen;
    }
    else
    {
        totalLength = runningDist;
    }

    if (totalLength < 0.01f)
    {
        valid = false;
        return false;
    }

    valid = true;
    return true;
}

int Path::getSegmentCount() const
{
    if (!valid || points.size() < 2)
        return 0;
    return closedLoop ? static_cast<int>(points.size()) : static_cast<int>(points.size()) - 1;
}

bool Path::getSegment(int index, Vector3& outA, Vector3& outB, float& outSegLength, float& outStartDist) const
{
    int count = getSegmentCount();
    if (index < 0 || index >= count)
        return false;

    int n = static_cast<int>(points.size());
    int nextIdx = (index + 1) % n;

    outA = points[index].position;
    outB = points[nextIdx].position;
    outStartDist = points[index].cumulativeDistance;

    if (index == n - 1 && closedLoop)
    {
        outSegLength = totalLength - outStartDist;
    }
    else
    {
        outSegLength = points[nextIdx].cumulativeDistance - outStartDist;
    }

    return true;
}

Vector3 Path::getPositionAtDistance(float distance) const
{
    if (!valid || points.empty())
        return Vector3();

    if (points.size() == 1)
        return points[0].position;

    float s = distance;
    if (closedLoop)
    {
        s = std::fmod(s, totalLength);
        if (s < 0.0f)
            s += totalLength;
    }
    else
    {
        s = std::clamp(s, 0.0f, totalLength);
    }

    int segCount = getSegmentCount();
    for (int i = 0; i < segCount; ++i)
    {
        Vector3 a, b;
        float segLen = 0.0f;
        float startDist = 0.0f;
        getSegment(i, a, b, segLen, startDist);

        float endDist = startDist + segLen;
        if (s >= startDist && (s <= endDist || i == segCount - 1))
        {
            if (segLen <= 0.0001f)
                return a;

            float t = (s - startDist) / segLen;
            t = std::clamp(t, 0.0f, 1.0f);
            return Lerp(a, b, t);
        }
    }

    return points.back().position;
}

Vector3 Path::getTangentAtDistance(float distance) const
{
    if (!valid || points.size() < 2)
        return Vector3(0.0f, 0.0f, 1.0f);

    float s = distance;
    if (closedLoop)
    {
        s = std::fmod(s, totalLength);
        if (s < 0.0f)
            s += totalLength;
    }
    else
    {
        s = std::clamp(s, 0.0f, totalLength);
    }

    int segCount = getSegmentCount();
    for (int i = 0; i < segCount; ++i)
    {
        Vector3 a, b;
        float segLen = 0.0f;
        float startDist = 0.0f;
        getSegment(i, a, b, segLen, startDist);

        float endDist = startDist + segLen;
        if (s >= startDist && (s <= endDist || i == segCount - 1))
        {
            Vector3 dir = b - a;
            dir.y = 0.0f;
            return Normalize(dir);
        }
    }

    Vector3 lastDir = points.back().position - points[points.size() - 2].position;
    lastDir.y = 0.0f;
    return Normalize(lastDir);
}

Path Path::createTrackLoopPath()
{
    // Closed circuit matching the Cheerios boundaries track centerline
    // Note: Y coordinate is 0.0f (driving plane)
    std::vector<Vector3> waypoints = {
        // Main straight (starts at origin where car spawns)
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 40.0f },
        { 0.0f, 0.0f, 80.0f },
        { 0.0f, 0.0f, 120.0f },
        { 0.0f, 0.0f, 160.0f },
        // Turn 1 (Right turn to top straight)
        { 4.0f, 0.0f, 178.0f },
        { 14.0f, 0.0f, 186.0f },
        { 26.0f, 0.0f, 188.0f },
        // Top straight
        { 60.0f, 0.0f, 188.0f },
        { 120.0f, 0.0f, 188.0f },
        { 180.0f, 0.0f, 188.0f },
        { 240.0f, 0.0f, 188.0f },
        { 280.0f, 0.0f, 188.0f },
        // Turn 2 (Right turn heading down)
        { 294.0f, 0.0f, 184.0f },
        { 300.0f, 0.0f, 174.0f },
        // Straight 3
        { 300.0f, 0.0f, 140.0f },
        { 300.0f, 0.0f, 110.0f },
        // Turn 3 (Right turn into middle straight)
        { 296.0f, 0.0f, 96.0f },
        { 288.0f, 0.0f, 88.0f },
        // Straight 4
        { 240.0f, 0.0f, 88.0f },
        { 180.0f, 0.0f, 88.0f },
        { 130.0f, 0.0f, 88.0f },
        // Turn 4 (Left turn into chicane)
        { 110.0f, 0.0f, 86.0f },
        { 102.0f, 0.0f, 78.0f },
        { 100.0f, 0.0f, 66.0f },
        // Straight 5 (Chicane straight)
        { 100.0f, 0.0f, 30.0f },
        { 100.0f, 0.0f, 0.0f },
        { 100.0f, 0.0f, -30.0f },
        { 100.0f, 0.0f, -48.0f },
        // Turn 5 (Right turn leaving chicane)
        { 104.0f, 0.0f, -58.0f },
        { 114.0f, 0.0f, -64.0f },
        // Straight 6
        { 150.0f, 0.0f, -64.0f },
        { 190.0f, 0.0f, -64.0f },
        { 232.0f, 0.0f, -64.0f },
        // Turn 6 (Right turn heading back down)
        { 246.0f, 0.0f, -70.0f },
        { 252.0f, 0.0f, -80.0f },
        // Straight 7
        { 252.0f, 0.0f, -110.0f },
        { 252.0f, 0.0f, -140.0f },
        { 252.0f, 0.0f, -170.0f },
        // Turn 7 (Right turn into bottom straight)
        { 248.0f, 0.0f, -182.0f },
        { 236.0f, 0.0f, -188.0f },
        // Bottom straight
        { 190.0f, 0.0f, -188.0f },
        { 140.0f, 0.0f, -188.0f },
        { 90.0f, 0.0f, -188.0f },
        { 40.0f, 0.0f, -188.0f },
        { 18.0f, 0.0f, -188.0f },
        // Turn 8 (Right turn back to main straight)
        { 6.0f, 0.0f, -184.0f },
        { 0.0f, 0.0f, -172.0f },
        { 0.0f, 0.0f, -120.0f },
        { 0.0f, 0.0f, -60.0f }
    };

    Path path;
    path.buildFromPoints(waypoints, true, "Full Track Circuit (Closed Loop)");
    return path;
}

Path Path::createStraightTestPath()
{
    // Open path: straight acceleration test along main straight
    std::vector<Vector3> waypoints = {
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 30.0f },
        { 0.0f, 0.0f, 60.0f },
        { 0.0f, 0.0f, 90.0f },
        { 0.0f, 0.0f, 120.0f },
        { 0.0f, 0.0f, 150.0f },
        { 0.0f, 0.0f, 175.0f }
    };

    Path path;
    path.buildFromPoints(waypoints, false, "Straight Line (Open Path)");
    return path;
}

Path Path::createTurnTestPath()
{
    // Open path: straight into 90-degree right turn
    std::vector<Vector3> waypoints = {
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 40.0f },
        { 0.0f, 0.0f, 80.0f },
        { 0.0f, 0.0f, 120.0f },
        { 0.0f, 0.0f, 150.0f },
        { 4.0f, 0.0f, 174.0f },
        { 14.0f, 0.0f, 184.0f },
        { 28.0f, 0.0f, 188.0f },
        { 60.0f, 0.0f, 188.0f },
        { 100.0f, 0.0f, 188.0f }
    };

    Path path;
    path.buildFromPoints(waypoints, false, "Straight + 90 Deg Curve (Open Path)");
    return path;
}
