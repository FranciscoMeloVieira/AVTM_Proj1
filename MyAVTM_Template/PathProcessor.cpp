#include "PathProcessor.h"
#include <cmath>
#include <algorithm>

PathProcessor::PathProcessor()
    : m_config()
{
}

PathProcessor::PathProcessor(const PathProcessorConfig& config)
    : m_config(config)
{
}

std::vector<Vector3> PathProcessor::filterRawSamples(
    const std::vector<RawPathSample>& raw,
    std::string& outError) const
{
    outError.clear();
    std::vector<Vector3> filtered;

    if (raw.size() < static_cast<size_t>(m_config.minSampleCount))
    {
        outError = "Path has too few raw samples (draw a longer stroke).";
        return filtered;
    }

    filtered.reserve(raw.size());

    for (const auto& sample : raw)
    {
        const Vector3& p = sample.position;

        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
        {
            outError = "Non-finite coordinates detected in raw input.";
            filtered.clear();
            return filtered;
        }

        Vector3 groundPoint(p.x, 0.0f, p.z);

        if (filtered.empty())
        {
            filtered.push_back(groundPoint);
        }
        else
        {
            float dist = Distance(filtered.back(), groundPoint);
            if (dist >= m_config.minRawSampleDistance)
            {
                filtered.push_back(groundPoint);
            }
        }
    }

    if (filtered.size() < static_cast<size_t>(m_config.minSampleCount))
    {
        outError = "Stroke too short or stationary after filtering jitter.";
        filtered.clear();
        return filtered;
    }

    return filtered;
}

std::vector<Vector3> PathProcessor::resample(
    const std::vector<Vector3>& points,
    bool closedLoop) const
{
    if (points.size() < 2)
        return points;

    // Compute cumulative segment distances
    std::vector<float> cumDist;
    cumDist.reserve(points.size() + 1);
    cumDist.push_back(0.0f);

    float totalLen = 0.0f;
    for (size_t i = 1; i < points.size(); ++i)
    {
        totalLen += Distance(points[i - 1], points[i]);
        cumDist.push_back(totalLen);
    }

    float closingLen = 0.0f;
    if (closedLoop)
    {
        closingLen = Distance(points.back(), points.front());
        totalLen += closingLen;
    }

    if (totalLen < 0.01f)
        return points;

    float spacing = (std::max)(m_config.resampleSpacing, 0.5f);
    int numSamples = static_cast<int>(std::round(totalLen / spacing));

    if (closedLoop)
    {
        if (numSamples < 3) numSamples = 3;
    }
    else
    {
        if (numSamples < 2) numSamples = 2;
    }

    std::vector<Vector3> resampled;
    resampled.reserve(numSamples + 1);

    auto sampleAtDistance = [&](float targetDist) -> Vector3 {
        if (closedLoop)
        {
            targetDist = std::fmod(targetDist, totalLen);
            if (targetDist < 0.0f) targetDist += totalLen;
        }
        else
        {
            targetDist = std::clamp(targetDist, 0.0f, totalLen);
        }

        // Search segments
        size_t segCount = closedLoop ? points.size() : points.size() - 1;
        for (size_t i = 0; i < segCount; ++i)
        {
            float segStart = cumDist[i];
            float segLen = (i == points.size() - 1 && closedLoop)
                ? closingLen
                : (cumDist[i + 1] - segStart);
            float segEnd = segStart + segLen;

            if (targetDist >= segStart && (targetDist <= segEnd || i == segCount - 1))
            {
                if (segLen <= 0.0001f)
                    return points[i];

                float t = std::clamp((targetDist - segStart) / segLen, 0.0f, 1.0f);
                const Vector3& a = points[i];
                const Vector3& b = (i == points.size() - 1 && closedLoop) ? points[0] : points[i + 1];
                return Lerp(a, b, t);
            }
        }
        return points.back();
    };

    if (closedLoop)
    {
        float step = totalLen / static_cast<float>(numSamples);
        for (int i = 0; i < numSamples; ++i)
        {
            resampled.push_back(sampleAtDistance(i * step));
        }
    }
    else
    {
        float step = totalLen / static_cast<float>(numSamples - 1);
        for (int i = 0; i < numSamples; ++i)
        {
            resampled.push_back(sampleAtDistance(i * step));
        }
        // Pin exact first and last
        resampled.front() = points.front();
        resampled.back() = points.back();
    }

    return resampled;
}

std::vector<Vector3> PathProcessor::smooth(
    const std::vector<Vector3>& points,
    bool closedLoop) const
{
    if (!m_config.enableSmoothing || points.size() < 3 || m_config.smoothingPasses <= 0)
        return points;

    std::vector<Vector3> current = points;
    std::vector<Vector3> next = current;
    float w = std::clamp(m_config.smoothingWeight, 0.0f, 0.5f);
    size_t n = current.size();

    for (int pass = 0; pass < m_config.smoothingPasses; ++pass)
    {
        if (closedLoop)
        {
            for (size_t i = 0; i < n; ++i)
            {
                size_t prevIdx = (i + n - 1) % n;
                size_t nextIdx = (i + 1) % n;
                Vector3 avgNeighbor = (current[prevIdx] + current[nextIdx]) * 0.5f;
                next[i] = current[i] * (1.0f - w) + avgNeighbor * w;
            }
        }
        else
        {
            // Pinned endpoints
            next[0] = current[0];
            next[n - 1] = current[n - 1];

            for (size_t i = 1; i < n - 1; ++i)
            {
                Vector3 avgNeighbor = (current[i - 1] + current[i + 1]) * 0.5f;
                next[i] = current[i] * (1.0f - w) + avgNeighbor * w;
            }
        }
        current = next;
    }

    return current;
}

bool PathProcessor::process(
    const std::vector<RawPathSample>& rawSamples,
    bool closedLoop,
    Path& outPath,
    std::string& outErrorMessage) const
{
    outPath.clear();
    outErrorMessage.clear();

    // 1. Filter raw samples
    std::vector<Vector3> filtered = filterRawSamples(rawSamples, outErrorMessage);
    if (filtered.empty())
    {
        return false;
    }

    // 2. Uniform Resampling
    std::vector<Vector3> resampled = resample(filtered, closedLoop);
    if (resampled.size() < static_cast<size_t>(closedLoop ? 3 : 2))
    {
        outErrorMessage = "Resampling resulted in insufficient points.";
        return false;
    }

    // 3. Conservative Smoothing
    std::vector<Vector3> smoothed = smooth(resampled, closedLoop);

    // 4. Build Canonical Path
    std::string name = closedLoop ? "Drawn Path (Closed Loop)" : "Drawn Path (Open)";
    if (!outPath.buildFromPoints(smoothed, closedLoop, name))
    {
        outErrorMessage = "Failed to build canonical path structure.";
        return false;
    }

    // 5. Validate Total Path Length
    if (outPath.totalLength < m_config.minPathLength)
    {
        char msg[128];
        std::snprintf(msg, sizeof(msg), "Path total length (%.1fm) is below minimum (%.1fm).", outPath.totalLength, m_config.minPathLength);
        outErrorMessage = msg;
        outPath.clear();
        return false;
    }

    return true;
}
