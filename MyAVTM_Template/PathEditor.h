#pragma once

#include "Vector3.h"
#include "Path.h"
#include "PathProcessor.h"
#include <vector>
#include <string>

enum class EditorState
{
    Idle,       // Normal driving mode
    Editing,    // Path edit mode active, ready to draw
    Recording,  // Currently dragging mouse and recording stroke
    Preview     // Stroke ended, previewing raw samples and processed path
};

struct PathEditorStats
{
    std::string inputMethod = "mouse";
    bool trackReferenceVisible = true;
    int rawSampleCount = 0;
    int processedPointCount = 0;
    float pathLength = 0.0f;
    double drawingDurationSeconds = 0.0;
    int redrawCount = 0;
    bool isClosedLoop = false;
};

class PathEditor
{
public:
    PathEditor();
    explicit PathEditor(const PathProcessorConfig& config);

    // State machine transitions
    void enterEditMode();
    void exitEditMode();
    EditorState getState() const { return m_state; }

    // Mouse drawing actions
    void beginStroke(const Vector3& worldPos, double timestampSeconds);
    void addSample(const Vector3& worldPos, double timestampSeconds);
    void endStroke();
    void clear();

    // Editor actions while in Preview/Editing
    void toggleClosedLoop();
    void setClosedLoop(bool closed);
    bool isClosedLoop() const { return m_closedLoop; }

    void toggleSmoothing();
    bool isSmoothingEnabled() const { return m_processor.getConfig().enableSmoothing; }

    void toggleTrackReference();
    void setTrackReferenceVisible(bool visible);
    bool isTrackReferenceVisible() const { return m_trackReferenceVisible; }

    // Process and confirm
    bool processCurrentStroke();
    bool confirmPath(Path& outPath);

    // Accessors
    bool hasSamples() const { return !m_rawSamples.empty(); }
    const std::vector<RawPathSample>& getRawSamples() const { return m_rawSamples; }
    const Path& getProcessedPath() const { return m_processedPath; }
    const std::string& getLastErrorMessage() const { return m_lastErrorMessage; }
    const PathEditorStats& getStats() const { return m_stats; }
    PathProcessor& getProcessor() { return m_processor; }
    void setInputMethod(const std::string& inputMethod) { m_stats.inputMethod = inputMethod; }

    // Screen-to-world mapping for overhead orthographic camera
    static Vector3 screenToWorldOrtho(
        int screenX, int screenY,
        int windowWidth, int windowHeight,
        float orthoViewSize,
        float cameraX, float cameraZ);

    // General ray-plane intersection for arbitrary camera
    static bool intersectRayWithGroundPlane(
        const Vector3& rayOrigin,
        const Vector3& rayDir,
        float planeY,
        Vector3& outIntersection);

private:
    EditorState m_state = EditorState::Idle;
    std::vector<RawPathSample> m_rawSamples;
    Path m_processedPath;
    PathProcessor m_processor;
    std::string m_lastErrorMessage;

    bool m_closedLoop = false;
    bool m_trackReferenceVisible = true;   // Track visible by default
    double m_strokeStartTime = 0.0;
    PathEditorStats m_stats;

    void logStats() const;
};
