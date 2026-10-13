#include "PathFollowerTests.h"
#include "Path.h"
#include "PathFollower.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <limits>

namespace {

bool approxEqual(float a, float b, float eps = 0.01f)
{
    return std::abs(a - b) <= eps;
}

bool approxEqual(const Vector3& a, const Vector3& b, float eps = 0.01f)
{
    return approxEqual(a.x, b.x, eps) && approxEqual(a.y, b.y, eps) && approxEqual(a.z, b.z, eps);
}

bool testSegmentProjection()
{
    Vector3 a(0.0f, 0.0f, 0.0f);
    Vector3 b(0.0f, 0.0f, 10.0f);

    Vector3 closest;
    float t = 0.0f;
    float dist = 0.0f;

    // 1. Point on segment
    Vector3 pOn(0.0f, 0.0f, 5.0f);
    PathFollower::projectPointOnSegment(pOn, a, b, closest, t, dist);
    assert(approxEqual(closest, pOn));
    assert(approxEqual(t, 0.5f));
    assert(approxEqual(dist, 0.0f));

    // 2. Point beside segment
    Vector3 pBeside(3.0f, 0.0f, 5.0f);
    PathFollower::projectPointOnSegment(pBeside, a, b, closest, t, dist);
    assert(approxEqual(closest, Vector3(0.0f, 0.0f, 5.0f)));
    assert(approxEqual(t, 0.5f));
    assert(approxEqual(dist, 3.0f));

    // 3. Point before segment
    Vector3 pBefore(0.0f, 0.0f, -4.0f);
    PathFollower::projectPointOnSegment(pBefore, a, b, closest, t, dist);
    assert(approxEqual(closest, a));
    assert(approxEqual(t, 0.0f));
    assert(approxEqual(dist, 4.0f));

    // 4. Point beyond segment
    Vector3 pBeyond(0.0f, 0.0f, 16.0f);
    PathFollower::projectPointOnSegment(pBeyond, a, b, closest, t, dist);
    assert(approxEqual(closest, b));
    assert(approxEqual(t, 1.0f));
    assert(approxEqual(dist, 6.0f));

    return true;
}

bool testCumulativeDistanceAndInterpolation()
{
    Path path;
    std::vector<Vector3> pts = {
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 10.0f },
        { 10.0f, 0.0f, 10.0f }
    };

    bool ok = path.buildFromPoints(pts, false, "TestPath");
    assert(ok && path.valid);
    assert(approxEqual(path.totalLength, 20.0f));
    assert(approxEqual(path.points[0].cumulativeDistance, 0.0f));
    assert(approxEqual(path.points[1].cumulativeDistance, 10.0f));
    assert(approxEqual(path.points[2].cumulativeDistance, 20.0f));

    Vector3 mid1 = path.getPositionAtDistance(5.0f);
    assert(approxEqual(mid1, Vector3(0.0f, 0.0f, 5.0f)));

    Vector3 mid2 = path.getPositionAtDistance(15.0f);
    assert(approxEqual(mid2, Vector3(5.0f, 0.0f, 10.0f)));

    return true;
}

bool testClosedLoopWraparound()
{
    Path path;
    // Square loop of side length 10
    std::vector<Vector3> pts = {
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 10.0f },
        { 10.0f, 0.0f, 10.0f },
        { 10.0f, 0.0f, 0.0f }
    };

    bool ok = path.buildFromPoints(pts, true, "LoopPath");
    assert(ok && path.valid && path.closedLoop);
    assert(approxEqual(path.totalLength, 40.0f));

    // Test wraparound: 45.0m should equal 5.0m
    Vector3 at5 = path.getPositionAtDistance(5.0f);
    Vector3 at45 = path.getPositionAtDistance(45.0f);
    assert(approxEqual(at5, at45));

    // Negative distance should wrap around properly
    Vector3 atNeg5 = path.getPositionAtDistance(-5.0f);
    Vector3 at35 = path.getPositionAtDistance(35.0f);
    assert(approxEqual(atNeg5, at35));

    return true;
}

bool testOpenPathEndpoint()
{
    Path path;
    std::vector<Vector3> pts = {
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 20.0f }
    };
    path.buildFromPoints(pts, false, "OpenPath");

    // Sampling past end should clamp to endpoint
    Vector3 past = path.getPositionAtDistance(50.0f);
    assert(approxEqual(past, Vector3(0.0f, 0.0f, 20.0f)));

    // Test controller endpoint arrival
    PathFollower follower;
    follower.setPath(path);

    CarState car;
    car.position = { 0.0f, 0.0f, 19.5f }; // Within stop distance (3.0m)
    car.forward = { 0.0f, 0.0f, 1.0f };
    car.velocity = { 0.0f, 0.0f, 0.2f };  // Speed < 1.0m/s
    car.speed = 0.2f;

    PathFollowOutput out = follower.update(car, 0.016f);
    assert(out.finished == true);
    assert(approxEqual(out.brake, 1.0f));
    assert(approxEqual(out.throttle, 0.0f));

    return true;
}

bool testZeroLengthAndDuplicatePoints()
{
    Path path;
    // Contains duplicate points
    std::vector<Vector3> pts = {
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 10.0f }
    };

    bool ok = path.buildFromPoints(pts, false, "DupPath");
    assert(ok && path.valid);
    assert(path.points.size() == 2);
    assert(approxEqual(path.totalLength, 10.0f));

    return true;
}

bool testSteeringSignConvention()
{
    // Car at (0,0,0) facing +Z (0,0,1)
    CarState car;
    car.position = { 0.0f, 0.0f, 0.0f };
    car.forward = { 0.0f, 0.0f, 1.0f };
    car.velocity = { 0.0f, 0.0f, 10.0f };
    car.speed = 10.0f;

    // 1. Path curving LEFT (+X in this game's driving plane)
    Path leftPath;
    leftPath.buildFromPoints({ { 0.0f, 0.0f, 0.0f }, { 10.0f, 0.0f, 20.0f } }, false);
    PathFollower followerLeft;
    followerLeft.setPath(leftPath);
    PathFollowOutput outLeft = followerLeft.update(car, 0.016f);

    // Left target MUST produce positive steering (+1.0 max) to match 'a' key
    assert(outLeft.steering > 0.1f);

    // 2. Path curving RIGHT (-X in this game's driving plane)
    Path rightPath;
    rightPath.buildFromPoints({ { 0.0f, 0.0f, 0.0f }, { -10.0f, 0.0f, 20.0f } }, false);
    PathFollower followerRight;
    followerRight.setPath(rightPath);
    PathFollowOutput outRight = followerRight.update(car, 0.016f);

    // Right target MUST produce negative steering (-1.0 max) to match 'd' key
    assert(outRight.steering < -0.1f);

    return true;
}

bool testInvalidPathRejection()
{
    // Fewer than 2 points
    Path p1;
    bool ok1 = p1.buildFromPoints({ { 0.0f, 0.0f, 0.0f } }, false);
    assert(!ok1 && !p1.valid);

    // Non-finite values
    float inf = std::numeric_limits<float>::infinity();
    Path p2;
    bool ok2 = p2.buildFromPoints({ { 0.0f, 0.0f, 0.0f }, { inf, 0.0f, 10.0f } }, false);
    assert(!ok2 && !p2.valid);

    // Zero length
    Path p3;
    bool ok3 = p3.buildFromPoints({ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }, false);
    assert(!ok3 && !p3.valid);

    return true;
}

bool testRecoveryAfterDisplacement()
{
    Path path;
    path.buildFromPoints({
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 50.0f },
        { 0.0f, 0.0f, 100.0f }
    }, false);

    PathFollower follower;
    follower.setPath(path);

    // First update at start
    CarState car;
    car.position = { 0.0f, 0.0f, 5.0f };
    car.forward = { 0.0f, 0.0f, 1.0f };
    car.velocity = { 0.0f, 0.0f, 5.0f };
    car.speed = 5.0f;
    follower.update(car, 0.016f);
    assert(approxEqual(follower.getCurrentProgress(), 5.0f));

    // Sudden massive displacement (e.g. knocked back to z = 85.0m)
    car.position = { 2.0f, 0.0f, 85.0f };
    follower.update(car, 0.016f);

    // Follower must recover and track near 85.0m, not stay stuck at 5.0m
    assert(approxEqual(follower.getCurrentProgress(), 85.0f, 1.0f));

    return true;
}

} // namespace

bool RunPathFollowerTests()
{
    std::cout << "\n=== Running Path & PathFollower Unit Tests ===" << std::endl;

    assert(testSegmentProjection());
    std::cout << "[PASS] Segment Projection (on, beside, before, beyond)" << std::endl;

    assert(testCumulativeDistanceAndInterpolation());
    std::cout << "[PASS] Cumulative Distance and Target Interpolation" << std::endl;

    assert(testClosedLoopWraparound());
    std::cout << "[PASS] Closed-loop Wraparound" << std::endl;

    assert(testOpenPathEndpoint());
    std::cout << "[PASS] Open-path Endpoint and Arrival Handling" << std::endl;

    assert(testZeroLengthAndDuplicatePoints());
    std::cout << "[PASS] Zero-length & Duplicate Points Filtering" << std::endl;

    assert(testSteeringSignConvention());
    std::cout << "[PASS] Steering Sign Convention (Left -> positive, Right -> negative)" << std::endl;

    assert(testInvalidPathRejection());
    std::cout << "[PASS] Invalid Path Rejection (finite, length, count)" << std::endl;

    assert(testRecoveryAfterDisplacement());
    std::cout << "[PASS] Global Recovery After Large Displacement" << std::endl;

    std::cout << "=== All Path & PathFollower Unit Tests Passed Successfully! ===\n" << std::endl;
    return true;
}
