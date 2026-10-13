#include "PathProcessorTests.h"
#include "PathProcessor.h"
#include "PathEditor.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <limits>

namespace {

bool approxEqual(float a, float b, float eps = 0.05f)
{
    return std::abs(a - b) <= eps;
}

bool approxEqual(const Vector3& a, const Vector3& b, float eps = 0.05f)
{
    return approxEqual(a.x, b.x, eps) && approxEqual(a.y, b.y, eps) && approxEqual(a.z, b.z, eps);
}

// 1. Coordinate tests
bool testCoordinateMapping()
{
    int winW = 640;
    int winH = 480;
    float viewSize = 200.0f;
    float camX = 150.0f;
    float camZ = 0.0f;

    // Center of screen (320, 240) -> should be (camX, 0, camZ)
    Vector3 centerWorld = PathEditor::screenToWorldOrtho(320, 240, winW, winH, viewSize, camX, camZ);
    assert(approxEqual(centerWorld.x, 150.0f));
    assert(approxEqual(centerWorld.y, 0.0f)); // Driving plane
    assert(approxEqual(centerWorld.z, 0.0f));

    // Top-center (320, 0) -> ndcZ = (0 - 240)/240 = -1.0 -> worldZ = -200
    Vector3 topWorld = PathEditor::screenToWorldOrtho(320, 0, winW, winH, viewSize, camX, camZ);
    assert(approxEqual(topWorld.x, 150.0f));
    assert(approxEqual(topWorld.z, -200.0f));

    // Bottom-center (320, 480) -> ndcZ = (480 - 240)/240 = +1.0 -> worldZ = +200
    Vector3 bottomWorld = PathEditor::screenToWorldOrtho(320, 480, winW, winH, viewSize, camX, camZ);
    assert(approxEqual(bottomWorld.x, 150.0f));
    assert(approxEqual(bottomWorld.z, +200.0f));

    // Track landmark: origin (0, 0, 0)
    // ndcX = (0 - 150) / 200 = -0.75
    // screenX = 320 + (-0.75 * 240) = 320 - 180 = 140
    // screenY = 240
    Vector3 originWorld = PathEditor::screenToWorldOrtho(140, 240, winW, winH, viewSize, camX, camZ);
    assert(approxEqual(originWorld.x, 0.0f));
    assert(approxEqual(originWorld.z, 0.0f));

    // Ray-ground intersection:
    Vector3 rayOrig(150.0f, 200.0f, 50.0f);
    Vector3 rayDir(0.0f, -1.0f, 0.0f); // Looking straight down
    Vector3 hit;
    bool intersected = PathEditor::intersectRayWithGroundPlane(rayOrig, rayDir, 0.0f, hit);
    assert(intersected);
    assert(approxEqual(hit, Vector3(150.0f, 0.0f, 50.0f)));

    // Parallel ray must be rejected
    Vector3 horizDir(1.0f, 0.0f, 0.0f);
    assert(!PathEditor::intersectRayWithGroundPlane(rayOrig, horizDir, 0.0f, hit));

    return true;
}

// 2. Recording tests
bool testRecordingAndFiltering()
{
    PathEditor editor;
    editor.enterEditMode();
    assert(editor.getState() == EditorState::Editing);

    // Single click / too-short stroke
    editor.beginStroke(Vector3(0.0f, 0.0f, 0.0f), 0.0);
    assert(editor.getState() == EditorState::Recording);
    editor.endStroke();
    assert(editor.getState() == EditorState::Preview);
    assert(!editor.getProcessedPath().valid); // Must reject single click

    // Clear and redraw
    editor.clear();
    assert(editor.getState() == EditorState::Editing);
    assert(!editor.hasSamples());

    // Dense stationary points should be filtered by minRawSampleDistance
    PathProcessor processor;
    std::vector<RawPathSample> dense;
    for (int i = 0; i < 50; ++i)
    {
        // 50 points all within 0.1m
        dense.push_back({ Vector3(0.0f + i * 0.001f, 0.0f, 0.0f), static_cast<double>(i) * 0.01 });
    }
    std::string err;
    std::vector<Vector3> filtered = processor.filterRawSamples(dense, err);
    // After filtering jitter, stationary cluster collapses to 1 sample, which is below minSampleCount (3)
    assert(filtered.empty());
    assert(!err.empty());

    return true;
}

// 3. Processing tests: Resampling & Smoothing
bool testResamplingAndSmoothing()
{
    PathProcessor processor;

    // Straight line of length 30 with irregular spacing
    std::vector<RawPathSample> raw = {
        { Vector3(0.0f, 0.0f, 0.0f), 0.0 },
        { Vector3(0.0f, 0.0f, 2.0f), 0.1 },
        { Vector3(0.0f, 0.0f, 15.0f), 0.5 },
        { Vector3(0.0f, 0.0f, 16.0f), 0.6 },
        { Vector3(0.0f, 0.0f, 30.0f), 1.0 }
    };

    Path openPath;
    std::string error;
    bool ok = processor.process(raw, false, openPath, error);
    assert(ok && openPath.valid);
    assert(!openPath.closedLoop);
    assert(approxEqual(openPath.totalLength, 30.0f));

    // Endpoints must be strictly preserved
    assert(approxEqual(openPath.points.front().position, Vector3(0.0f, 0.0f, 0.0f)));
    assert(approxEqual(openPath.points.back().position, Vector3(0.0f, 0.0f, 30.0f)));

    // Resampled spacing: each segment should be around 3.0m
    for (size_t i = 1; i < openPath.points.size(); ++i)
    {
        float d = Distance(openPath.points[i - 1].position, openPath.points[i].position);
        assert(d >= 2.0f && d <= 4.0f);
    }

    // Closed loop processing: square loop 40m
    std::vector<RawPathSample> squareRaw = {
        { Vector3(0.0f, 0.0f, 0.0f), 0.0 },
        { Vector3(0.0f, 0.0f, 10.0f), 0.2 },
        { Vector3(10.0f, 0.0f, 10.0f), 0.4 },
        { Vector3(10.0f, 0.0f, 0.0f), 0.6 }
    };

    Path loopPath;
    ok = processor.process(squareRaw, true, loopPath, error);
    assert(ok && loopPath.valid);
    assert(loopPath.closedLoop);
    assert(loopPath.totalLength >= 20.0f && loopPath.totalLength <= 40.0f);

    return true;
}

// 4. End-to-end Editor confirm and reset
bool testEditorWorkflow()
{
    PathEditor editor;
    editor.enterEditMode();

    // Draw a valid open stroke
    editor.beginStroke(Vector3(0.0f, 0.0f, 0.0f), 0.0);
    editor.addSample(Vector3(0.0f, 0.0f, 20.0f), 0.2);
    editor.addSample(Vector3(0.0f, 0.0f, 50.0f), 0.5);
    editor.addSample(Vector3(20.0f, 0.0f, 70.0f), 0.8);
    editor.endStroke();

    assert(editor.getState() == EditorState::Preview);
    assert(editor.getProcessedPath().valid);

    // Toggle closed loop
    editor.setClosedLoop(false);
    assert(!editor.getProcessedPath().closedLoop);

    // Confirm
    Path confirmed;
    bool confirmedOk = editor.confirmPath(confirmed);
    assert(confirmedOk && confirmed.valid);
    assert(editor.getState() == EditorState::Idle);

    return true;
}

} // namespace

bool RunPathProcessorTests()
{
    std::cout << "\n=== Running PathProcessor & PathEditor Unit Tests ===" << std::endl;

    assert(testCoordinateMapping());
    std::cout << "[PASS] Coordinate Mapping (Center, Corners, Ray-Plane Intersection)" << std::endl;

    assert(testRecordingAndFiltering());
    std::cout << "[PASS] Recording & Jitter Filtering (Single Click, Stationary Rejection)" << std::endl;

    assert(testResamplingAndSmoothing());
    std::cout << "[PASS] Uniform Resampling and Conservative Smoothing (Open & Closed)" << std::endl;

    assert(testEditorWorkflow());
    std::cout << "[PASS] Editor Workflow (Record -> Preview -> Toggle Loop -> Confirm)" << std::endl;

    std::cout << "=== All PathProcessor & PathEditor Unit Tests Passed! ===\n" << std::endl;
    return true;
}
