#pragma once

#include "ScenegraphNode.h"
#include "Path.h"
#include <vector>

class PathVisualizer : public EmptyNode
{
public:
    PathVisualizer();
    ~PathVisualizer() override = default;

    // Updates the displayed path waypoints
    void setPath(const Path& path);

    // Updates the current look-ahead target position
    void setTargetPosition(const Vector3& target);

    // Updates the current progress / closest point position
    void setProgressPosition(const Vector3& pos);

    void setEnabled(bool enabled);
    bool isEnabled() const { return isVisible; }

private:
    MeshNode* m_targetMarker = nullptr;
    MeshNode* m_progressMarker = nullptr;
    InstancedObjectNode* m_waypointsNode = nullptr;

    void rebuildWaypoints(const Path& path);
};
