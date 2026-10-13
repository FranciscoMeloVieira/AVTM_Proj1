#pragma once

#include "ScenegraphNode.h"
#include "Path.h"
#include "PathProcessor.h"
#include <vector>

class PathVisualizer : public EmptyNode
{
public:
    PathVisualizer();
    ~PathVisualizer() override = default;

    // Updates the displayed processed path waypoints
    void setPath(const Path& path);

    // Updates raw recorded samples (displayed in cyan/blue)
    void setRawSamples(const std::vector<RawPathSample>& rawSamples);
    void clearRawSamples();

    // Updates the current look-ahead target position
    void setTargetPosition(const Vector3& target);

    // Updates the current progress / closest point position
    void setProgressPosition(const Vector3& pos);
    void setCursorPosition(const Vector3& pos);
    void hideCursor();

    // Visibility toggles
    void setEnabled(bool enabled);
    bool isEnabled() const { return isVisible; }
    void hideMarkers();

private:
    MeshNode* m_targetMarker = nullptr;
    MeshNode* m_progressMarker = nullptr;
    MeshNode* m_startMarker = nullptr;
    MeshNode* m_endMarker = nullptr;
    MeshNode* m_cursorMarker = nullptr;

    InstancedObjectNode* m_waypointsNode = nullptr;
    InstancedObjectNode* m_rawSamplesNode = nullptr;

    void rebuildWaypoints(const Path& path);
};
