#include "FingerTracker.h"
#include <cmath>
#include <algorithm>
#include <iostream>

FingerTracker::FingerTracker()
{
}

void FingerTracker::reset()
{
    m_hasPrevPoint = false;
    m_consecutivePinchFrames = 0;
    m_consecutiveOpenFrames = 0;
    m_stablePinching = false;
    m_trail.clear();
}

bool FingerTracker::process(const cv::Mat& rawFrame, FingerTrackingResult& result, cv::Mat& debugFrame)
{
    if (rawFrame.empty())
    {
        result.detected = false;
        return false;
    }

    // 1. Mirror horizontally for intuitive user interaction
    cv::Mat flipped;
    cv::flip(rawFrame, flipped, 1);
    debugFrame = flipped.clone();

    int frameW = flipped.cols;
    int frameH = flipped.rows;
    result.frameWidth = frameW;
    result.frameHeight = frameH;

    // 2. Color segmentation (YCrCb + HSV)
    cv::Mat ycrcb, hsv;
    cv::cvtColor(flipped, ycrcb, cv::COLOR_BGR2YCrCb);
    cv::cvtColor(flipped, hsv, cv::COLOR_BGR2HSV);

    // YCrCb skin mask
    cv::Mat maskYCrCb;
    cv::inRange(ycrcb, cv::Scalar(0, 133, 77), cv::Scalar(255, 173, 127), maskYCrCb);

    // HSV skin mask
    cv::Mat maskHSV1, maskHSV2, maskHSV;
    cv::inRange(hsv, cv::Scalar(0, 30, 60), cv::Scalar(25, 255, 255), maskHSV1);
    cv::inRange(hsv, cv::Scalar(165, 30, 60), cv::Scalar(180, 255, 255), maskHSV2);
    cv::bitwise_or(maskHSV1, maskHSV2, maskHSV);

    // Combined skin mask
    cv::Mat skinMask;
    cv::bitwise_and(maskYCrCb, maskHSV, skinMask);

    // Morphological cleanup: remove small speckles and close palm gaps
    cv::Mat kernelOpen = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::Mat kernelClose = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7));
    cv::morphologyEx(skinMask, skinMask, cv::MORPH_OPEN, kernelOpen);
    cv::morphologyEx(skinMask, skinMask, cv::MORPH_CLOSE, kernelClose);
    cv::GaussianBlur(skinMask, skinMask, cv::Size(3, 3), 0);

    // 3. Find largest contour (the hand)
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(skinMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    double maxArea = 0.0;
    int maxIdx = -1;
    for (size_t i = 0; i < contours.size(); ++i)
    {
        double area = cv::contourArea(contours[i]);
        if (area > maxArea)
        {
            maxArea = area;
            maxIdx = static_cast<int>(i);
        }
    }

    // Require minimum hand area
    if (maxIdx < 0 || maxArea < 3500.0)
    {
        result.detected = false;
        result.isPinching = false;
        m_consecutivePinchFrames = 0;
        m_consecutiveOpenFrames++;
        if (m_consecutiveOpenFrames >= 3)
        {
            m_stablePinching = false;
        }

        // Overlay status on debug frame
        cv::putText(debugFrame, "NO HAND DETECTED", cv::Point(30, 60),
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 165, 255), 2);
        cv::putText(debugFrame, "Show hand to camera to draw", cv::Point(30, 95),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(200, 200, 200), 1);
        return false;
    }

    const auto& handContour = contours[maxIdx];
    cv::Rect handRect = cv::boundingRect(handContour);
    result.handRect = handRect;

    // 4. Palm center & radius via Distance Transform on hand mask ROI
    cv::Mat handRoi = skinMask(handRect);
    cv::Mat distMap;
    cv::distanceTransform(handRoi, distMap, cv::DIST_L2, 5);

    double maxDistVal = 0.0;
    cv::Point maxDistLoc;
    cv::minMaxLoc(distMap, nullptr, &maxDistVal, nullptr, &maxDistLoc);

    cv::Point2f palmCenter(static_cast<float>(handRect.x + maxDistLoc.x),
                           static_cast<float>(handRect.y + maxDistLoc.y));
    float palmRadius = static_cast<float>(maxDistVal);
    if (palmRadius < 15.0f) palmRadius = 25.0f;

    // 5. Convex Hull & Fingertip candidates
    std::vector<cv::Point> hullPoints;
    std::vector<int> hullIndices;
    cv::convexHull(handContour, hullPoints, false);
    cv::convexHull(handContour, hullIndices, false, false);

    std::vector<cv::Point2f> fingerCandidates;
    float minTipDistFromPalm = palmRadius * 1.25f;

    for (const auto& hp : hullPoints)
    {
        cv::Point2f p(static_cast<float>(hp.x), static_cast<float>(hp.y));
        float d = static_cast<float>(cv::norm(p - palmCenter));
        // Must be sufficiently far from palm and generally above the wrist
        if (d > minTipDistFromPalm && p.y < palmCenter.y + palmRadius * 0.4f)
        {
            // Cluster nearby points (within 25 pixels)
            bool duplicate = false;
            for (auto& existing : fingerCandidates)
            {
                if (cv::norm(p - existing) < 25.0)
                {
                    if (p.y < existing.y) existing = p; // Keep the higher tip
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate)
            {
                fingerCandidates.push_back(p);
            }
        }
    }

    // Sort candidate tips by Y (topmost first)
    std::sort(fingerCandidates.begin(), fingerCandidates.end(),
              [](const cv::Point2f& a, const cv::Point2f& b) { return a.y < b.y; });

    cv::Point2f indexTip;
    cv::Point2f thumbTip;
    cv::Point2f activeDrawPoint;
    bool rawPinching = false;
    float pinchDist = 0.0f;

    if (fingerCandidates.empty())
    {
        // Fallback: topmost point of contour
        cv::Point topPt = handContour[0];
        for (const auto& pt : handContour)
        {
            if (pt.y < topPt.y) topPt = pt;
        }
        indexTip = cv::Point2f(static_cast<float>(topPt.x), static_cast<float>(topPt.y));
        activeDrawPoint = indexTip;
        rawPinching = false;
    }
    else if (fingerCandidates.size() == 1)
    {
        // Only one prominent tip visible. When index and thumb pinch together tightly,
        // they merge into a single contour peak.
        indexTip = fingerCandidates[0];
        activeDrawPoint = indexTip;
        // If hand area is compact or tip is prominent, treat as pinch
        rawPinching = true;
        pinchDist = 0.0f;
    }
    else
    {
        // Two or more tips: topmost is index, find thumb (widest angle or closest secondary tip)
        indexTip = fingerCandidates[0];

        // Choose thumb as the next candidate with greatest horizontal deviation or nearest tip
        size_t thumbIdx = 1;
        float bestDist = static_cast<float>(cv::norm(indexTip - fingerCandidates[1]));
        for (size_t i = 2; i < fingerCandidates.size(); ++i)
        {
            float d = static_cast<float>(cv::norm(indexTip - fingerCandidates[i]));
            if (d < bestDist)
            {
                bestDist = d;
                thumbIdx = i;
            }
        }
        thumbTip = fingerCandidates[thumbIdx];
        pinchDist = static_cast<float>(cv::norm(indexTip - thumbTip));

        // Threshold scaled to palm radius or fixed pixel distance
        float dynamicPinchThreshold = std::max(m_pinchThresholdPixels, palmRadius * 0.70f);
        if (pinchDist <= dynamicPinchThreshold)
        {
            rawPinching = true;
            activeDrawPoint = (indexTip + thumbTip) * 0.5f;
        }
        else
        {
            rawPinching = false;
            activeDrawPoint = indexTip; // Preview with index fingertip
        }
    }

    // 6. Temporal hysteresis to eliminate flickering
    // Temporal hysteresis to reduce flicker. Reduced required frames for faster response.
    if (rawPinching)
    {
        m_consecutivePinchFrames++;
        m_consecutiveOpenFrames = 0;
        if (m_consecutivePinchFrames >= 1)
        {
            m_stablePinching = true;
        }
    }
    else
    {
        m_consecutiveOpenFrames++;
        m_consecutivePinchFrames = 0;
        if (m_consecutiveOpenFrames >= 1)
        {
            m_stablePinching = false;
        }
    }

    // 7. Exponential moving average (EMA) smoothing for coordinates
    if (!m_hasPrevPoint)
    {
        m_smoothedPoint = activeDrawPoint;
        m_hasPrevPoint = true;
    }
    else
    {
        const float alpha = 0.75f; // Fast and responsive with smooth jitter removal
        m_smoothedPoint.x = alpha * activeDrawPoint.x + (1.0f - alpha) * m_smoothedPoint.x;
        m_smoothedPoint.y = alpha * activeDrawPoint.y + (1.0f - alpha) * m_smoothedPoint.y;
    }

    // 8. Fill result structure
    result.detected = true;
    result.isPinching = m_stablePinching;
    result.pinchPoint = m_smoothedPoint;
    result.normalizedPos = cv::Point2f(m_smoothedPoint.x / static_cast<float>(frameW),
                                       m_smoothedPoint.y / static_cast<float>(frameH));
    result.indexTip = indexTip;
    result.thumbTip = thumbTip;
    result.pinchDistance = pinchDist;
    result.confidence = 1.0f;

    // 9. Draw rich visualization onto debug frame
    // Draw hand contour
    std::vector<std::vector<cv::Point>> drawContoursList = { handContour };
    cv::drawContours(debugFrame, drawContoursList, 0, cv::Scalar(0, 220, 120), 2);

    // Draw palm center circle
    cv::circle(debugFrame, palmCenter, static_cast<int>(palmRadius), cv::Scalar(255, 140, 0), 1);
    cv::circle(debugFrame, palmCenter, 4, cv::Scalar(255, 140, 0), -1);

    // Draw finger candidate tips
    for (const auto& tip : fingerCandidates)
    {
        cv::circle(debugFrame, tip, 5, cv::Scalar(255, 200, 0), -1);
    }

    // Draw line between thumb and index if both tracked
    if (thumbTip.x > 0.0f && thumbTip.y > 0.0f)
    {
        cv::Scalar lineCol = m_stablePinching ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0);
        cv::line(debugFrame, indexTip, thumbTip, lineCol, 2);
    }

    // Draw recorded path trail on OpenCV window
    if (m_trail.size() > 1)
    {
        for (size_t i = 1; i < m_trail.size(); ++i)
        {
            cv::line(debugFrame, m_trail[i - 1], m_trail[i], cv::Scalar(0, 255, 255), 2);
        }
    }

    // Draw cursor marker at tracked position
    if (m_stablePinching)
    {
        // PINCHING (DRAWING): Red pulsing marker
        cv::circle(debugFrame, m_smoothedPoint, 14, cv::Scalar(0, 50, 255), -1);
        cv::circle(debugFrame, m_smoothedPoint, 22, cv::Scalar(0, 50, 255), 2);
        cv::line(debugFrame, cv::Point2f(m_smoothedPoint.x - 28, m_smoothedPoint.y),
                 cv::Point2f(m_smoothedPoint.x + 28, m_smoothedPoint.y), cv::Scalar(0, 50, 255), 2);
        cv::line(debugFrame, cv::Point2f(m_smoothedPoint.x, m_smoothedPoint.y - 28),
                 cv::Point2f(m_smoothedPoint.x, m_smoothedPoint.y + 28), cv::Scalar(0, 50, 255), 2);

        cv::putText(debugFrame, "PINCH: DRAWING",
                    cv::Point(static_cast<int>(m_smoothedPoint.x) + 25, static_cast<int>(m_smoothedPoint.y) - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 50, 255), 2);
    }
    else
    {
        // HOVERING (PREVIEW): Bright green marker
        cv::circle(debugFrame, m_smoothedPoint, 10, cv::Scalar(0, 255, 0), -1);
        cv::circle(debugFrame, m_smoothedPoint, 18, cv::Scalar(0, 255, 0), 2);
        cv::line(debugFrame, cv::Point2f(m_smoothedPoint.x - 24, m_smoothedPoint.y),
                 cv::Point2f(m_smoothedPoint.x + 24, m_smoothedPoint.y), cv::Scalar(0, 255, 0), 2);
        cv::line(debugFrame, cv::Point2f(m_smoothedPoint.x, m_smoothedPoint.y - 24),
                 cv::Point2f(m_smoothedPoint.x, m_smoothedPoint.y + 24), cv::Scalar(0, 255, 0), 2);

        cv::putText(debugFrame, "HOVER: PREVIEW",
                    cv::Point(static_cast<int>(m_smoothedPoint.x) + 25, static_cast<int>(m_smoothedPoint.y) - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 255, 0), 2);
    }

    // Top instruction banner
    cv::rectangle(debugFrame, cv::Rect(0, 0, frameW, 65), cv::Scalar(20, 20, 20), -1);
    cv::putText(debugFrame, "OPENCV FINGER PATH DRAWING", cv::Point(15, 25),
                cv::FONT_HERSHEY_SIMPLEX, 0.65, cv::Scalar(0, 240, 255), 2);
    cv::putText(debugFrame, "[Pinch]: Draw  |  [C / Enter]: Confirm & Drive  |  [X]: Clear  |  [L]: Loop",
                cv::Point(15, 52), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(220, 220, 220), 1);

    // Bottom diagnostic info
    std::string statusStr = m_stablePinching ? "STATUS: PINCH (RECORDING)" : "STATUS: HOVER (PREVIEW)";
    cv::putText(debugFrame, statusStr, cv::Point(15, frameH - 15),
                cv::FONT_HERSHEY_SIMPLEX, 0.5,
                m_stablePinching ? cv::Scalar(0, 80, 255) : cv::Scalar(0, 255, 0), 2);

    return true;
}
