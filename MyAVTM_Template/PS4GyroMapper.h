#pragma once

#include "Vector3.h"
#include "PS4Parser.h"
#include <cstddef>

struct PS4GyroMapperConfig
{
	std::size_t calibrationSamples = 30; // Number of samples to collect for neutral calibration
	float filterAlpha = 0.20f;          // Exponential filter factor in [0.01, 1.0]
	float deadZone = 0.05f;             // Dead zone threshold
	float fullScaleMotion = 1.0f;       // Full scale threshold
	float sensitivity = 100.0f;         // Cursor sensitivity (units/sec)
	float maximumSpeed = 150.0f;        // Clamped maximum speed
	bool invertX = true;                // Invert roll axis (X cursor velocity)
	bool invertZ = false;               // Invert pitch axis (Z cursor velocity)
	bool useGyroRate = false;           // Use gyro angular rate if true, accelerometer tilt if false
};

class PS4GyroMapper
{
public:
	explicit PS4GyroMapper(const PS4GyroMapperConfig& config = {});

	void beginCalibration();
	void consumeSample(const PS4ControllerSample& sample);
	void update(float deltaTimeSeconds);
	void setBounds(float minX, float maxX, float minZ, float maxZ);
	void setCursorPosition(const Vector3& position);
	void resetCursor();

	bool isCalibrated() const { return m_calibrated; }
	std::size_t getCalibrationSamplesCollected() const { return m_calibrationCount; }
	std::size_t getCalibrationSamplesRequired() const { return m_config.calibrationSamples; }
	const Vector3& getCursorPosition() const { return m_cursorPosition; }

	float getNeutralPitch() const { return m_neutralPitch; }
	float getNeutralRoll() const { return m_neutralRoll; }
	float getFilteredPitch() const { return m_filteredPitch; }
	float getFilteredRoll() const { return m_filteredRoll; }

	float applyDeadZone(float value) const;

	const PS4GyroMapperConfig& getConfig() const { return m_config; }
	void setConfig(const PS4GyroMapperConfig& config) { m_config = config; }

private:
	PS4GyroMapperConfig m_config;

	float m_neutralPitch = 0.0f;
	float m_neutralRoll = 0.0f;
	float m_filteredPitch = 0.0f;
	float m_filteredRoll = 0.0f;

	Vector3 m_cursorPosition{ 150.0f, 0.0f, 0.0f };
	float m_minX = -50.0f;
	float m_maxX = 350.0f;
	float m_minZ = -200.0f;
	float m_maxZ = 200.0f;

	std::size_t m_calibrationCount = 0;
	double m_calibrationPitchSum = 0.0;
	double m_calibrationRollSum = 0.0;
	bool m_calibrated = false;
	bool m_hasFilteredSample = false;

	void extractMotion(const PS4ControllerSample& sample, float& outPitch, float& outRoll) const;
};
