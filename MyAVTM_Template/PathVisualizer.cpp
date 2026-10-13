#include "PathVisualizer.h"
#include <GL/glew.h>
#include <cmath>

PathVisualizer::PathVisualizer()
{
    // Material for lookahead target marker: bright orange-red
    Material targetMaterial = {
        { 1.0f, 0.3f, 0.0f, 1.0f }, // Diffuse
        { 0.4f, 0.1f, 0.0f, 1.0f }, // Ambient
        { 0.8f, 0.8f, 0.8f, 1.0f }, // Specular
        { 0.9f, 0.3f, 0.0f, 1.0f }, // Emissive
        30.0f                       // Shininess
    };

    // Material for current car projection on path: bright yellow
    Material progressMaterial = {
        { 1.0f, 1.0f, 0.1f, 1.0f }, // Diffuse
        { 0.3f, 0.3f, 0.0f, 1.0f }, // Ambient
        { 0.5f, 0.5f, 0.5f, 1.0f }, // Specular
        { 0.8f, 0.8f, 0.0f, 1.0f }, // Emissive
        20.0f                       // Shininess
    };

    // Material for Start marker: bright green
    Material startMaterial = {
        { 0.1f, 1.0f, 0.2f, 1.0f }, // Diffuse
        { 0.1f, 0.4f, 0.1f, 1.0f }, // Ambient
        { 0.5f, 0.9f, 0.5f, 1.0f }, // Specular
        { 0.3f, 0.9f, 0.3f, 1.0f }, // Emissive
        20.0f                       // Shininess
    };

    // Material for End marker: bright red
    Material endMaterial = {
        { 1.0f, 0.1f, 0.1f, 1.0f }, // Diffuse
        { 0.4f, 0.0f, 0.0f, 1.0f }, // Ambient
        { 0.8f, 0.3f, 0.3f, 1.0f }, // Specular
        { 0.9f, 0.1f, 0.1f, 1.0f }, // Emissive
        20.0f                       // Shininess
    };

    // Target marker sphere (index 0)
    m_targetMarker = new MeshNode(2, 0, targetMaterial);
    m_targetMarker->setScale(1.8f, 1.8f, 1.8f);
    m_targetMarker->setAbsolutePosition(0.0f, -100.0f, 0.0f);
    addChild(m_targetMarker);

    // Progress marker sphere (index 1)
    m_progressMarker = new MeshNode(2, 0, progressMaterial);
    m_progressMarker->setScale(1.2f, 1.2f, 1.2f);
    m_progressMarker->setAbsolutePosition(0.0f, -100.0f, 0.0f);
    addChild(m_progressMarker);

    // Start marker sphere (index 2)
    m_startMarker = new MeshNode(2, 0, startMaterial);
    m_startMarker->setScale(2.2f, 2.2f, 2.2f);
    m_startMarker->setAbsolutePosition(0.0f, -100.0f, 0.0f);
    addChild(m_startMarker);

    // End marker sphere (index 3)
    m_endMarker = new MeshNode(2, 0, endMaterial);
    m_endMarker->setScale(2.2f, 2.2f, 2.2f);
    m_endMarker->setAbsolutePosition(0.0f, -100.0f, 0.0f);
    addChild(m_endMarker);

    Material cursorMaterial = {
        { 0.1f, 1.0f, 1.0f, 1.0f },
        { 0.0f, 0.3f, 0.3f, 1.0f },
        { 0.6f, 1.0f, 1.0f, 1.0f },
        { 0.1f, 0.8f, 0.8f, 1.0f },
        25.0f
    };
    m_cursorMarker = new MeshNode(2, 0, cursorMaterial);
    m_cursorMarker->setScale(2.5f, 2.5f, 2.5f);
    hideCursor();
    addChild(m_cursorMarker);
}

void PathVisualizer::rebuildWaypoints(const Path& path)
{
    // Clear any previous waypoint or raw nodes (keep markers at indices 0-4)
    while (children.size() > 5)
    {
        children.pop_back();
    }
    m_waypointsNode = nullptr;
    m_rawSamplesNode = nullptr;

    if (!path.valid || path.totalLength < 0.1f)
    {
        m_startMarker->setAbsolutePosition(0.0f, -100.0f, 0.0f);
        m_endMarker->setAbsolutePosition(0.0f, -100.0f, 0.0f);
        return;
    }

    // Set Start & End marker positions
    Vector3 startPos = path.points.front().position;
    startPos.y = 1.0f;
    m_startMarker->setAbsolutePosition(startPos.x, startPos.y, startPos.z);

    if (path.closedLoop)
    {
        m_endMarker->setAbsolutePosition(0.0f, -100.0f, 0.0f); // Hidden for closed loops
    }
    else
    {
        Vector3 endPos = path.points.back().position;
        endPos.y = 1.0f;
        m_endMarker->setAbsolutePosition(endPos.x, endPos.y, endPos.z);
    }

    // Sample waypoints along the path every 3.5 world units
    float spacing = 3.5f;
    int sampleCount = static_cast<int>(std::ceil(path.totalLength / spacing));
    if (sampleCount < 2) sampleCount = 2;

    std::vector<Vector3> samplePositions;
    samplePositions.reserve(sampleCount);

    for (int i = 0; i < sampleCount; ++i)
    {
        float dist = (path.totalLength / static_cast<float>(sampleCount)) * static_cast<float>(i);
        Vector3 pos = path.getPositionAtDistance(dist);
        pos.y = 0.5f; // Raised slightly above ground plane
        samplePositions.push_back(pos);
    }

    Material waypointMaterial = {
        { 0.1f, 0.9f, 0.2f, 1.0f },
        { 0.1f, 0.3f, 0.1f, 1.0f },
        { 0.3f, 0.3f, 0.3f, 1.0f },
        { 0.1f, 0.6f, 0.1f, 1.0f },
        10.0f
    };

    // Mesh ID 0 = cube, texMode = 0
    m_waypointsNode = new InstancedObjectNode(0, 0, waypointMaterial, samplePositions);
    m_waypointsNode->setScale(0.8f, 0.2f, 0.8f);
    addChild(m_waypointsNode);
}

void PathVisualizer::setPath(const Path& path)
{
    rebuildWaypoints(path);
}

void PathVisualizer::setRawSamples(const std::vector<RawPathSample>& rawSamples)
{
    if (rawSamples.empty())
    {
        clearRawSamples();
        return;
    }
    clearRawSamples();

    std::vector<Vector3> rawPositions;
    rawPositions.reserve(rawSamples.size());

    for (const auto& sample : rawSamples)
    {
        Vector3 p = sample.position;
        p.y = 0.4f; // Just below processed path markers
        rawPositions.push_back(p);
    }

    Material rawMaterial = {
        { 0.2f, 0.8f, 1.0f, 1.0f }, // Cyan
        { 0.1f, 0.3f, 0.5f, 1.0f },
        { 0.4f, 0.8f, 1.0f, 1.0f },
        { 0.2f, 0.7f, 1.0f, 1.0f },
        15.0f
    };

    m_rawSamplesNode = new InstancedObjectNode(0, 0, rawMaterial, rawPositions);
    m_rawSamplesNode->setScale(0.5f, 0.15f, 0.5f);
    addChild(m_rawSamplesNode);
}

void PathVisualizer::setCursorPosition(const Vector3& pos)
{
    if (m_cursorMarker)
        m_cursorMarker->setAbsolutePosition(pos.x, 2.5f, pos.z);
}

void PathVisualizer::hideCursor()
{
    if (m_cursorMarker)
        m_cursorMarker->setAbsolutePosition(0.0f, -100.0f, 0.0f);
}

void PathVisualizer::clearRawSamples()
{
    if (m_rawSamplesNode && !children.empty() && children.back().get() == m_rawSamplesNode)
    {
        children.pop_back();
    }
    m_rawSamplesNode = nullptr;
}

void PathVisualizer::setTargetPosition(const Vector3& target)
{
    if (m_targetMarker)
    {
        m_targetMarker->setAbsolutePosition(target.x, 1.2f, target.z);
    }
}

void PathVisualizer::setProgressPosition(const Vector3& pos)
{
    if (m_progressMarker)
    {
        m_progressMarker->setAbsolutePosition(pos.x, 0.9f, pos.z);
    }
}

void PathVisualizer::hideMarkers()
{
    if (m_targetMarker) m_targetMarker->setAbsolutePosition(0.0f, -100.0f, 0.0f);
    if (m_progressMarker) m_progressMarker->setAbsolutePosition(0.0f, -100.0f, 0.0f);
}

void PathVisualizer::setEnabled(bool enabled)
{
    setVisibility(enabled);
}
