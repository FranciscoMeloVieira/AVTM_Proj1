#include "PathVisualizer.h"
#include <GL/glew.h>
#include <cmath>

PathVisualizer::PathVisualizer()
{
    // Material for path waypoints: luminous green
    Material waypointMaterial = {
        { 0.1f, 0.9f, 0.2f, 1.0f }, // Diffuse
        { 0.1f, 0.3f, 0.1f, 1.0f }, // Ambient
        { 0.3f, 0.3f, 0.3f, 1.0f }, // Specular
        { 0.1f, 0.6f, 0.1f, 1.0f }, // Emissive
        10.0f                       // Shininess
    };

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

    // Target marker sphere (mesh ID 2 = sphere, texMode = 0)
    m_targetMarker = new MeshNode(2, 0, targetMaterial);
    m_targetMarker->setScale(1.8f, 1.8f, 1.8f);
    m_targetMarker->setAbsolutePosition(0.0f, -100.0f, 0.0f); // Hidden initially
    addChild(m_targetMarker);

    // Progress marker sphere (mesh ID 2 = sphere, texMode = 0)
    m_progressMarker = new MeshNode(2, 0, progressMaterial);
    m_progressMarker->setScale(1.2f, 1.2f, 1.2f);
    m_progressMarker->setAbsolutePosition(0.0f, -100.0f, 0.0f); // Hidden initially
    addChild(m_progressMarker);
}

void PathVisualizer::rebuildWaypoints(const Path& path)
{
    // Clear any previous waypoint nodes (keep markers at index 0 and 1)
    while (children.size() > 2)
    {
        children.pop_back();
    }
    m_waypointsNode = nullptr;

    if (!path.valid || path.totalLength < 0.1f)
        return;

    // Sample waypoints along the path every 4 world units
    float spacing = 4.0f;
    int sampleCount = static_cast<int>(std::ceil(path.totalLength / spacing));
    if (sampleCount < 2) sampleCount = 2;

    std::vector<Vector3> samplePositions;
    samplePositions.reserve(sampleCount);

    for (int i = 0; i < sampleCount; ++i)
    {
        float dist = (path.totalLength / static_cast<float>(sampleCount)) * static_cast<float>(i);
        Vector3 pos = path.getPositionAtDistance(dist);
        pos.y = 0.5f; // Slightly above ground so always visible
        samplePositions.push_back(pos);
    }

    Material waypointMaterial = {
        { 0.1f, 0.9f, 0.2f, 1.0f },
        { 0.1f, 0.3f, 0.1f, 1.0f },
        { 0.3f, 0.3f, 0.3f, 1.0f },
        { 0.1f, 0.6f, 0.1f, 1.0f },
        10.0f
    };

    // Mesh ID 0 = cube, texMode = 0 (no texture)
    m_waypointsNode = new InstancedObjectNode(0, 0, waypointMaterial, samplePositions);
    m_waypointsNode->setScale(0.8f, 0.2f, 0.8f);
    addChild(m_waypointsNode);
}

void PathVisualizer::setPath(const Path& path)
{
    rebuildWaypoints(path);
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

void PathVisualizer::setEnabled(bool enabled)
{
    setVisibility(enabled);
}
