#pragma once

#include "Vector3.h"
#include <cstddef>
#include <cstdint>
#include <string_view>

struct AccelerometerSample
{
	std::uint32_t timestampMilliseconds = 0;
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	bool buttonPressed = false;
};

class AccelerometerProtocol
{
public:
	static bool parseLine(std::string_view line, AccelerometerSample& outSample);
};

struct AccelerometerMapperConfig
{
	std::size_t calibrationSamples = 30;
	float filterAlpha = 0.15f;
	float deadZone = 0.08f;
	float fullScaleTilt = 0.35f;
	float sensitivity = 90.0f;
	float maximumSpeed = 120.0f;
	bool invertX = true;
	bool invertY = false;
};

class AccelerometerMapper
{
public:
	explicit AccelerometerMapper(const AccelerometerMapperConfig& config = {});

	void beginCalibration();
	void consumeSample(const AccelerometerSample& sample);
	void update(float deltaTimeSeconds);
	void setBounds(float minX, float maxX, float minZ, float maxZ);
	void setCursorPosition(const Vector3& position);
	void resetCursor();

	bool isCalibrated() const { return m_calibrated; }
	std::size_t getCalibrationSamplesCollected() const { return m_calibrationCount; }
	std::size_t getCalibrationSamplesRequired() const { return m_config.calibrationSamples; }
	const Vector3& getCursorPosition() const { return m_cursorPosition; }
	const AccelerometerSample& getNeutralBaseline() const { return m_neutral; }

private:
	AccelerometerMapperConfig m_config;
	AccelerometerSample m_neutral;
	AccelerometerSample m_filtered;
	Vector3 m_cursorPosition = Vector3(150.0f, 0.0f, 0.0f);
	float m_minX = -50.0f;
	float m_maxX = 350.0f;
	float m_minZ = -200.0f;
	float m_maxZ = 200.0f;
	std::size_t m_calibrationCount = 0;
	double m_calibrationX = 0.0;
	double m_calibrationY = 0.0;
	double m_calibrationZ = 0.0;
	bool m_calibrated = false;
	bool m_hasFilteredSample = false;

	float applyDeadZone(float value) const;
};
