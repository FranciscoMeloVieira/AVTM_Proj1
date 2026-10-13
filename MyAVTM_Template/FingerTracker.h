#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/geometry.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <vector>

struct FingerTrackingResult
{
    bool detected = false;
    bool isPinching = false;
    cv::Point2f pinchPoint;       // in pixels (on horizontally flipped frame)
    cv::Point2f normalizedPos;    // in [0, 1] x [0, 1] (0=left, 1=right, 0=top, 1=bottom)
    cv::Point2f indexTip;         // in pixels
    cv::Point2f thumbTip;         // in pixels
    float pinchDistance = 0.0f;
    float confidence = 0.0f;
    cv::Rect handRect;
    int frameWidth = 640;
    int frameHeight = 480;
};

class FingerTracker
{
public:
    FingerTracker();

    bool process(const cv::Mat& rawFrame, FingerTrackingResult& result, cv::Mat& debugFrame);

    void setPinchThreshold(float thresholdPixels) { m_pinchThresholdPixels = thresholdPixels; }
    float getPinchThreshold() const { return m_pinchThresholdPixels; }

    void addTrailPoint(const cv::Point2f& pt) { m_trail.push_back(pt); }
    void clearTrail() { m_trail.clear(); }

    void reset();

private:
    // Raised default pinch threshold to make pinch detection more permissive
    float m_pinchThresholdPixels = 80.0f;
    cv::Point2f m_smoothedPoint{ 0.0f, 0.0f };
    bool m_hasPrevPoint = false;

    // Hysteresis for pinch detection
    int m_consecutivePinchFrames = 0;
    int m_consecutiveOpenFrames = 0;
    bool m_stablePinching = false;

    // Visual trail of drawn points on camera feed
    std::vector<cv::Point2f> m_trail;
};
