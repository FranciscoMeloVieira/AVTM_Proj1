#include "PathFollower.h"
#include <cmath>
#include <algorithm>
#include <limits>

PathFollower::PathFollower()
    : m_config()
{
}

PathFollower::PathFollower(const PathFollowerConfig& config)
    : m_config(config)
{
}

void PathFollower::setPath(const Path& path, bool seekToStart)
{
    m_path = path;
    reset();
    m_seekingStart = seekToStart && m_path.valid && m_path.points.size() >= 2;
}

void PathFollower::reset()
{
    m_currentProgress = -1.0f;
    m_lookAheadDistance = 0.0f;
    m_targetPosition = Vector3();
    m_closestPosition = Vector3();
    m_lateralDeviation = 0.0f;
    m_headingErrorDegrees = 0.0f;
    m_effectiveTargetSpeed = 0.0f;
    m_lastOutput = PathFollowOutput();
    m_finished = false;
    m_seekingStart = false;
}

bool PathFollower::projectPointOnSegment(
    const Vector3& p,
    const Vector3& a,
    const Vector3& b,
    Vector3& outClosest,
    float& outT,
    float& outDistance)
{
    float abX = b.x - a.x;
    float abZ = b.z - a.z;
    float lenSq = abX * abX + abZ * abZ;

    if (lenSq < 0.000001f)
    {
        outClosest = a;
        outT = 0.0f;
        outDistance = std::hypot(p.x - a.x, p.z - a.z);
        return false;
    }

    float apX = p.x - a.x;
    float apZ = p.z - a.z;
    float t = (apX * abX + apZ * abZ) / lenSq;
    outT = std::clamp(t, 0.0f, 1.0f);

    outClosest = Lerp(a, b, outT);
    outDistance = std::hypot(p.x - outClosest.x, p.z - outClosest.z);
    return true;
}

float PathFollower::findClosestProgress(
    const Vector3& pos,
    float searchWindow,
    Vector3& outClosestPoint,
    float& outDistance) const
{
    int segCount = m_path.getSegmentCount();
    if (segCount == 0)
    {
        outClosestPoint = pos;
        outDistance = 0.0f;
        return 0.0f;
    }

    float bestDistance = (std::numeric_limits<float>::max)();
    float bestProgress = 0.0f;
    Vector3 bestPoint;
    bool foundInWindow = false;

    // First pass: local window search if searchWindow > 0 and valid current progress
    if (searchWindow > 0.0f && m_currentProgress >= 0.0f)
    {
        for (int i = 0; i < segCount; ++i)
        {
            Vector3 a, b;
            float segLen = 0.0f;
            float startDist = 0.0f;
            m_path.getSegment(i, a, b, segLen, startDist);

            float segMidDist = startDist + segLen * 0.5f;
            float distDiff = std::abs(segMidDist - m_currentProgress);

            if (m_path.closedLoop)
            {
                distDiff = (std::min)(distDiff, m_path.totalLength - distDiff);
            }

            // Segment is considered candidate if within window + half segment length
            if (distDiff <= searchWindow + segLen * 0.5f)
            {
                Vector3 closest;
                float t = 0.0f;
                float dist = 0.0f;
                projectPointOnSegment(pos, a, b, closest, t, dist);

                if (dist < bestDistance)
                {
                    bestDistance = dist;
                    bestPoint = closest;
                    bestProgress = startDist + t * segLen;
                    foundInWindow = true;
                }
            }
        }
    }

    // Fallback pass: global search if local search failed or distance exceeds recovery threshold
    if (!foundInWindow || bestDistance > m_config.recoveryDistance)
    {
        bestDistance = (std::numeric_limits<float>::max)();

        for (int i = 0; i < segCount; ++i)
        {
            Vector3 a, b;
            float segLen = 0.0f;
            float startDist = 0.0f;
            m_path.getSegment(i, a, b, segLen, startDist);

            Vector3 closest;
            float t = 0.0f;
            float dist = 0.0f;
            projectPointOnSegment(pos, a, b, closest, t, dist);

            if (dist < bestDistance)
            {
                bestDistance = dist;
                bestPoint = closest;
                bestProgress = startDist + t * segLen;
            }
        }
    }

    outClosestPoint = bestPoint;
    outDistance = bestDistance;
    return bestProgress;
}

PathFollowOutput PathFollower::update(const CarState& car, float deltaTime)
{
    PathFollowOutput output;

    if (!m_path.valid || m_path.points.size() < 2)
    {
        output.finished = true;
        m_lastOutput = output;
        return output;
    }

    if (m_finished)
    {
        output.steering = 0.0f;
        output.throttle = 0.0f;
        output.brake = 0.0f;
        output.finished = true;
        m_lastOutput = output;
        return output;
    }

    // 1. Estimate progress on path
    Vector3 closestPoint;
    float lateralDist = 0.0f;

    if (m_seekingStart)
    {
        closestPoint = m_path.points.front().position;
        lateralDist = std::hypot(
            car.position.x - closestPoint.x,
            car.position.z - closestPoint.z);
        m_currentProgress = 0.0f;

        if (lateralDist <= m_config.stopDistance)
        {
            m_seekingStart = false;
        }
    }
    else if (m_currentProgress < 0.0f)
    {
        // Initial anchor search across full path
        m_currentProgress = findClosestProgress(car.position, 0.0f, closestPoint, lateralDist);
    }
    else
    {
        // Local search with recovery fallback
        m_currentProgress = findClosestProgress(car.position, m_config.progressSearchWindow, closestPoint, lateralDist);
    }

    m_closestPosition = closestPoint;
    m_lateralDeviation = lateralDist;

    // 2. Compute speed-aware look-ahead distance and target point
    float speed = std::abs(car.speed);
    m_lookAheadDistance = std::clamp(
        m_config.minLookAhead + m_config.lookAheadGain * speed,
        m_config.minLookAhead,
        m_config.maxLookAhead);

    float sTarget = m_currentProgress + m_lookAheadDistance;

    if (m_path.closedLoop)
    {
        sTarget = std::fmod(sTarget, m_path.totalLength);
        if (sTarget < 0.0f)
            sTarget += m_path.totalLength;
    }
    else
    {
        sTarget = (std::min)(sTarget, m_path.totalLength);
    }

    m_targetPosition = m_seekingStart
        ? m_path.points.front().position
        : m_path.getPositionAtDistance(sTarget);

    // 3. Calculate steering request
    Vector3 toTarget = Vector3(
        m_targetPosition.x - car.position.x,
        0.0f,
        m_targetPosition.z - car.position.z);

    float distToTarget = Length(toTarget);
    Vector3 targetDir = (distToTarget > 0.001f) ? (toTarget / distToTarget) : car.forward;

    Vector3 carFwd = Normalize(Vector3(car.forward.x, 0.0f, car.forward.z));
    if (Length(carFwd) < 0.001f)
    {
        carFwd = Vector3(0.0f, 0.0f, 1.0f);
    }

    // Signed angle in XZ driving plane:
    // Dot gives cosine, Cross Y component gives sine
    float dotVal = std::clamp(Dot(carFwd, targetDir), -1.0f, 1.0f);
    float crossY = carFwd.z * targetDir.x - carFwd.x * targetDir.z;
    float angleRad = std::atan2(crossY, dotVal);
    m_headingErrorDegrees = angleRad * 180.0f / 3.14159265f;

    float rawSteering = (m_headingErrorDegrees / m_config.maxSteeringAngle) * m_config.steeringGain;
    output.steering = std::clamp(rawSteering, -1.0f, 1.0f);

    // 4. Request speed, throttle, and brake
    if (!m_path.closedLoop)
    {
        float remainingDist = m_path.totalLength - m_currentProgress;

        if (remainingDist <= m_config.stopDistance && speed < 1.0f)
        {
            m_finished = true;
            output.steering = 0.0f;
            output.throttle = 0.0f;
            output.brake = 0.0f;
            output.finished = true;
            m_lastOutput = output;
            return output;
        }

        if (remainingDist < m_config.arrivalDistance)
        {
            float arrivalRatio = std::clamp(
                remainingDist / m_config.arrivalDistance, 0.0f, 1.0f);
            m_effectiveTargetSpeed = m_config.targetSpeed * arrivalRatio;
        }
        else
        {
            m_effectiveTargetSpeed = m_config.targetSpeed;
        }
    }
    else
    {
        m_effectiveTargetSpeed = m_config.targetSpeed;
    }

    // Curvature slowdown
    float turnFactor = std::abs(output.steering);
    m_effectiveTargetSpeed *= (1.0f - m_config.turnSpeedReduction * turnFactor);

    // Keep a minimum speed only for closed loops. Open paths must be able to stop.
    if (m_path.closedLoop)
    {
        m_effectiveTargetSpeed = (std::max)(m_effectiveTargetSpeed, 4.0f);
    }

    // Speed error control
    float speedError = m_effectiveTargetSpeed - car.speed;

    if (speedError > 0.5f)
    {
        // Need to accelerate
        output.throttle = std::clamp(speedError / m_config.accelGain, 0.2f, 1.0f);
        output.brake = 0.0f;
    }
    else if (speedError < -1.0f)
    {
        // Need to brake / decelerate
        output.throttle = 0.0f;
        output.brake = std::clamp((-speedError) / m_config.brakeGain, 0.2f, 1.0f);
    }
    else
    {
        // Coast / light maintenance throttle
        output.throttle = 0.15f;
        output.brake = 0.0f;
    }

    output.finished = false;
    m_lastOutput = output;
    return output;
}
