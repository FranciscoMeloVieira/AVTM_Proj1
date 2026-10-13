#include "PathEditor.h"
#include <iostream>
#include <cmath>

PathEditor::PathEditor()
    : m_processor()
{
}

PathEditor::PathEditor(const PathProcessorConfig& config)
    : m_processor(config)
{
}

void PathEditor::enterEditMode()
{
    m_state = hasSamples() ? EditorState::Preview : EditorState::Editing;
    m_lastErrorMessage.clear();
    std::cout << "[PathEditor] Entered Edit Mode. State: "
              << (m_state == EditorState::Preview ? "PREVIEW" : "EDITING") << std::endl;
}

void PathEditor::exitEditMode()
{
    m_state = EditorState::Idle;
    std::cout << "[PathEditor] Exited Edit Mode." << std::endl;
}

void PathEditor::beginStroke(const Vector3& worldPos, double timestampSeconds)
{
    if (m_state != EditorState::Editing && m_state != EditorState::Preview)
        return;

    if (!m_rawSamples.empty())
    {
        m_stats.redrawCount++;
        m_rawSamples.clear();
        m_processedPath.clear();
    }

    m_state = EditorState::Recording;
    m_strokeStartTime = timestampSeconds;
    m_lastErrorMessage.clear();

    RawPathSample sample;
    sample.position = Vector3(worldPos.x, 0.0f, worldPos.z);
    sample.timestampSeconds = timestampSeconds;
    m_rawSamples.push_back(sample);
}

void PathEditor::addSample(const Vector3& worldPos, double timestampSeconds)
{
    if (m_state != EditorState::Recording)
        return;

    Vector3 groundPos(worldPos.x, 0.0f, worldPos.z);

    // Only add if not identical to last recorded sample
    if (m_rawSamples.empty() || Distance(m_rawSamples.back().position, groundPos) > 0.05f)
    {
        RawPathSample sample;
        sample.position = groundPos;
        sample.timestampSeconds = timestampSeconds;
        m_rawSamples.push_back(sample);
    }
}

void PathEditor::endStroke()
{
    if (m_state != EditorState::Recording)
        return;

    m_state = EditorState::Preview;

    if (!m_rawSamples.empty())
    {
        m_stats.drawingDurationSeconds = m_rawSamples.back().timestampSeconds - m_strokeStartTime;
    }

    processCurrentStroke();
}

void PathEditor::clear()
{
    m_rawSamples.clear();
    m_processedPath.clear();
    m_lastErrorMessage.clear();
    m_state = EditorState::Editing;
    std::cout << "[PathEditor] Path cleared." << std::endl;
}

void PathEditor::toggleClosedLoop()
{
    m_closedLoop = !m_closedLoop;
    std::cout << "[PathEditor] Closed Loop: " << (m_closedLoop ? "ENABLED" : "DISABLED") << std::endl;
    if (hasSamples())
    {
        processCurrentStroke();
    }
}

void PathEditor::setClosedLoop(bool closed)
{
    m_closedLoop = closed;
    if (hasSamples())
    {
        processCurrentStroke();
    }
}

void PathEditor::toggleSmoothing()
{
    auto cfg = m_processor.getConfig();
    cfg.enableSmoothing = !cfg.enableSmoothing;
    m_processor.setConfig(cfg);
    std::cout << "[PathEditor] Smoothing: " << (cfg.enableSmoothing ? "ENABLED" : "DISABLED") << std::endl;
    if (hasSamples())
    {
        processCurrentStroke();
    }
}

void PathEditor::toggleTrackReference()
{
    m_trackReferenceVisible = !m_trackReferenceVisible;
    std::cout << "[PathEditor] Track Reference: " << (m_trackReferenceVisible ? "VISIBLE" : "HIDDEN (BLANK)") << std::endl;
}

void PathEditor::setTrackReferenceVisible(bool visible)
{
    m_trackReferenceVisible = visible;
}

bool PathEditor::processCurrentStroke()
{
    m_lastErrorMessage.clear();

    if (m_rawSamples.size() < 2)
    {
        m_lastErrorMessage = "Stroke is too short (at least 2 samples required).";
        m_processedPath.clear();
        return false;
    }

    bool success = m_processor.process(m_rawSamples, m_closedLoop, m_processedPath, m_lastErrorMessage);

    if (success)
    {
        m_stats.rawSampleCount = static_cast<int>(m_rawSamples.size());
        m_stats.processedPointCount = static_cast<int>(m_processedPath.points.size());
        m_stats.pathLength = m_processedPath.totalLength;
        m_stats.isClosedLoop = m_closedLoop;
        m_stats.trackReferenceVisible = m_trackReferenceVisible;
        logStats();
    }
    else
    {
        std::cout << "[PathEditor] Processing error: " << m_lastErrorMessage << std::endl;
    }

    return success;
}

bool PathEditor::confirmPath(Path& outPath)
{
    if (!m_processedPath.valid)
    {
        m_lastErrorMessage = "Cannot confirm invalid path. Please redraw or adjust stroke.";
        return false;
    }

    outPath = m_processedPath;
    m_state = EditorState::Idle;

    std::cout << "[PathEditor] Path confirmed! Handing to path follower: "
              << outPath.name << " (Length: " << outPath.totalLength << "m, Points: "
              << outPath.points.size() << ")" << std::endl;

    return true;
}

void PathEditor::logStats() const
{
    std::cout << "[PathEditor Stats] Raw samples: " << m_stats.rawSampleCount
              << " | Processed points: " << m_stats.processedPointCount
              << " | Length: " << m_stats.pathLength << "m"
              << " | Closed: " << (m_stats.isClosedLoop ? "YES" : "NO")
              << " | View: " << (m_stats.trackReferenceVisible ? "Track" : "Blank")
              << " | Redraws: " << m_stats.redrawCount << std::endl;
}

Vector3 PathEditor::screenToWorldOrtho(
    int screenX, int screenY,
    int windowWidth, int windowHeight,
    float orthoViewSize,
    float cameraX, float cameraZ)
{
    float halfW = windowWidth * 0.5f;
    float halfH = windowHeight * 0.5f;
    if (halfH <= 0.001f) halfH = 1.0f;

    // In CameraFixedOrtho:
    // Eye = (cameraX, 200, cameraZ), Target = (cameraX, 0, cameraZ), Up = (0, 0, -1)
    // Left-to-right on screen corresponds to +X in world space.
    // Top-to-bottom on screen corresponds to +Z in world space.
    float ndcX = (static_cast<float>(screenX) - halfW) / halfH;
    float ndcZ = (static_cast<float>(screenY) - halfH) / halfH;

    float worldX = cameraX + ndcX * orthoViewSize;
    float worldZ = cameraZ + ndcZ * orthoViewSize;

    return Vector3(worldX, 0.0f, worldZ);
}

bool PathEditor::intersectRayWithGroundPlane(
    const Vector3& rayOrigin,
    const Vector3& rayDir,
    float planeY,
    Vector3& outIntersection)
{
    if (std::abs(rayDir.y) < 0.00001f)
        return false; // Ray parallel to ground plane

    float t = (planeY - rayOrigin.y) / rayDir.y;
    if (t < 0.0f)
        return false; // Intersection behind ray origin

    outIntersection = rayOrigin + rayDir * t;
    return true;
}
