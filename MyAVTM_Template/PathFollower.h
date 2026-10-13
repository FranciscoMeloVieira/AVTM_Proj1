#pragma once

#include "Path.h"
#include "Vector3.h"

struct PathFollowerConfig
{
    float minLookAhead = 8.0f;          // Minimum look-ahead distance in world units
    float maxLookAhead = 25.0f;         // Maximum look-ahead distance in world units
    float lookAheadGain = 0.6f;         // L = clamp(minLookAhead + lookAheadGain * abs(speed), min, max)
    float targetSpeed = 16.0f;          // Nominal cruise speed (world units/s)
    float turnSpeedReduction = 0.5f;    // Speed reduction factor in sharp turns (0.0 to 0.7)
    float maxSteeringAngle = 35.0f;     // Heading angle (degrees) for maximum steering input
    float steeringGain = 1.0f;          // Proportional gain on steering
    float progressSearchWindow = 35.0f; // Window distance around previous progress for local search
    float recoveryDistance = 25.0f;     // Deviation threshold triggering global recovery search
    float arrivalDistance = 25.0f;      // Distance from open path end to start decelerating
    float stopDistance = 3.0f;          // Distance from open path end to stop car
    float accelGain = 6.0f;             // Speed error scale for throttle request
    float brakeGain = 4.0f;             // Speed error scale for brake request
};

struct PathFollowOutput
{
    float steering = 0.0f; // Normalized [-1.0, 1.0] (positive = turn left, negative = turn right)
    float throttle = 0.0f; // Normalized [0.0, 1.0]
    float brake = 0.0f;    // Normalized [0.0, 1.0]
    bool finished = false; // True when open path endpoint is reached
};

struct CarState
{
    Vector3 position;
    Vector3 velocity;
    Vector3 forward;
    float speed = 0.0f;
};

class PathFollower
{
public:
    PathFollower();
    explicit PathFollower(const PathFollowerConfig& config);

    void setPath(const Path& path);
    void reset();

    PathFollowOutput update(const CarState& car, float deltaTime);

    // Configuration
    const PathFollowerConfig& getConfig() const { return m_config; }
    PathFollowerConfig& getConfig() { return m_config; }
    void setConfig(const PathFollowerConfig& config) { m_config = config; }

    // Telemetry & Diagnostics
    float getCurrentProgress() const { return m_currentProgress; }
    float getLookAheadDistance() const { return m_lookAheadDistance; }
    Vector3 getTargetPosition() const { return m_targetPosition; }
    Vector3 getClosestPosition() const { return m_closestPosition; }
    float getLateralDeviation() const { return m_lateralDeviation; }
    float getHeadingErrorDegrees() const { return m_headingErrorDegrees; }
    float getEffectiveTargetSpeed() const { return m_effectiveTargetSpeed; }
    const PathFollowOutput& getLastOutput() const { return m_lastOutput; }
    bool isFinished() const { return m_finished; }
    const Path& getPath() const { return m_path; }

    // Segment projection helper (exposed for testing)
    static bool projectPointOnSegment(
        const Vector3& p,
        const Vector3& a,
        const Vector3& b,
        Vector3& outClosest,
        float& outT,
        float& outDistance);

private:
    PathFollowerConfig m_config;
    Path m_path;

    float m_currentProgress = -1.0f; // Progress along path in cumulative distance (-1 = uninitialized)
    float m_lookAheadDistance = 0.0f;
    Vector3 m_targetPosition;
    Vector3 m_closestPosition;
    float m_lateralDeviation = 0.0f;
    float m_headingErrorDegrees = 0.0f;
    float m_effectiveTargetSpeed = 0.0f;
    PathFollowOutput m_lastOutput;
    bool m_finished = false;

    float findClosestProgress(const Vector3& pos, float searchWindow, Vector3& outClosestPoint, float& outDistance) const;
};
