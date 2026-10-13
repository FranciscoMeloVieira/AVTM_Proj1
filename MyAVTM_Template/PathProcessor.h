#pragma once

#include "Vector3.h"
#include "Path.h"
#include <vector>
#include <string>

struct RawPathSample
{
    Vector3 position;
    double timestampSeconds = 0.0;
};

struct PathProcessorConfig
{
    float minRawSampleDistance = 0.8f;   // Distance threshold to filter jittery or stationary mouse events
    float resampleSpacing = 3.0f;        // Desired uniform spacing along the path in world units
    bool enableSmoothing = true;         // Moving-average smoothing enabled
    float smoothingWeight = 0.35f;       // Smoothing blend weight [0.0, 0.5]
    int smoothingPasses = 2;             // Number of smoothing iterations
    float minPathLength = 5.0f;          // Minimum total length required for a valid path
    int minSampleCount = 3;              // Minimum distinct points required
};

class PathProcessor
{
public:
    PathProcessor();
    explicit PathProcessor(const PathProcessorConfig& config);

    // Processes raw samples through validation, filtering, uniform resampling,
    // and conservative smoothing, producing a canonical Path.
    bool process(
        const std::vector<RawPathSample>& rawSamples,
        bool closedLoop,
        Path& outPath,
        std::string& outErrorMessage) const;

    const PathProcessorConfig& getConfig() const { return m_config; }
    PathProcessorConfig& getConfig() { return m_config; }
    void setConfig(const PathProcessorConfig& config) { m_config = config; }

    // Individual pipeline stages (exposed for unit testing and fine control)
    std::vector<Vector3> filterRawSamples(const std::vector<RawPathSample>& raw, std::string& outError) const;
    std::vector<Vector3> resample(const std::vector<Vector3>& points, bool closedLoop) const;
    std::vector<Vector3> smooth(const std::vector<Vector3>& points, bool closedLoop) const;

private:
    PathProcessorConfig m_config;
};
