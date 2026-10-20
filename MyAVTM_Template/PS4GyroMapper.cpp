#include "PS4GyroMapper.h"
#include <algorithm>
#include <cmath>

PS4GyroMapper::PS4GyroMapper(const PS4GyroMapperConfig& config)
	: m_config(config)
{
	m_config.calibrationSamples = std::max<std::size_t>(1, m_config.calibrationSamples);
	m_config.filterAlpha = std::clamp(std::isfinite(m_config.filterAlpha) ? m_config.filterAlpha : 0.20f, 0.01f, 1.0f);
	m_config.deadZone = std::clamp(m_config.deadZone, 0.0f, 0.95f);
	m_config.fullScaleMotion = std::max(m_config.fullScaleMotion, m_config.deadZone + 0.001f);
	m_config.sensitivity = std::max(0.0f, m_config.sensitivity);
	m_config.maximumSpeed = std::max(0.0f, m_config.maximumSpeed);
}

void PS4GyroMapper::beginCalibration()
{
	m_calibrationCount = 0;
	m_calibrationPitchSum = 0.0;
	m_calibrationRollSum = 0.0;
	m_calibrated = false;
	m_hasFilteredSample = false;
}

void PS4GyroMapper::extractMotion(const PS4ControllerSample& sample, float& outPitch, float& outRoll) const
{
	if (m_config.useGyroRate)
	{
		outPitch = sample.pitchRate;
		outRoll = sample.rollRate;
	}
	else
	{
		const float denom = std::sqrt(sample.accelX * sample.accelX + sample.accelZ * sample.accelZ);
		outPitch = std::atan2(-sample.accelY, (denom > 0.0001f) ? denom : 0.0001f);
		outRoll = std::atan2(sample.accelX, sample.accelZ);
	}
}

void PS4GyroMapper::consumeSample(const PS4ControllerSample& sample)
{
	float pitch = 0.0f;
	float roll = 0.0f;
	extractMotion(sample, pitch, roll);

	if (!std::isfinite(pitch) || !std::isfinite(roll))
		return;

	if (!m_calibrated)
	{
		m_calibrationPitchSum += pitch;
		m_calibrationRollSum += roll;
		++m_calibrationCount;

		if (m_calibrationCount >= m_config.calibrationSamples)
		{
			const double count = static_cast<double>(m_calibrationCount);
			m_neutralPitch = static_cast<float>(m_calibrationPitchSum / count);
			m_neutralRoll = static_cast<float>(m_calibrationRollSum / count);
			m_filteredPitch = m_neutralPitch;
			m_filteredRoll = m_neutralRoll;
			m_calibrated = true;
			m_hasFilteredSample = true;
		}
		return;
	}

	if (!m_hasFilteredSample)
	{
		m_filteredPitch = pitch;
		m_filteredRoll = roll;
		m_hasFilteredSample = true;
		return;
	}

	const float alpha = m_config.filterAlpha;
	m_filteredPitch += alpha * (pitch - m_filteredPitch);
	m_filteredRoll += alpha * (roll - m_filteredRoll);
}

void PS4GyroMapper::update(float deltaTimeSeconds)
{
	if (!m_calibrated || !m_hasFilteredSample || !std::isfinite(deltaTimeSeconds) || deltaTimeSeconds <= 0.0f)
		return;

	const float dt = std::min(deltaTimeSeconds, 0.1f);
	float velX = m_config.sensitivity * applyDeadZone(m_filteredRoll - m_neutralRoll);
	float velZ = m_config.sensitivity * applyDeadZone(m_filteredPitch - m_neutralPitch);

	if (m_config.invertX)
		velX = -velX;
	if (m_config.invertZ)
		velZ = -velZ;

	velX = std::clamp(velX, -m_config.maximumSpeed, m_config.maximumSpeed);
	velZ = std::clamp(velZ, -m_config.maximumSpeed, m_config.maximumSpeed);

	m_cursorPosition.x = std::clamp(m_cursorPosition.x + velX * dt, m_minX, m_maxX);
	m_cursorPosition.z = std::clamp(m_cursorPosition.z + velZ * dt, m_minZ, m_maxZ);
}

void PS4GyroMapper::setBounds(float minX, float maxX, float minZ, float maxZ)
{
	if (!std::isfinite(minX) || !std::isfinite(maxX) || !std::isfinite(minZ) || !std::isfinite(maxZ) ||
		minX > maxX || minZ > maxZ)
		return;

	m_minX = minX;
	m_maxX = maxX;
	m_minZ = minZ;
	m_maxZ = maxZ;
	m_cursorPosition.x = std::clamp(m_cursorPosition.x, m_minX, m_maxX);
	m_cursorPosition.z = std::clamp(m_cursorPosition.z, m_minZ, m_maxZ);
}

void PS4GyroMapper::setCursorPosition(const Vector3& position)
{
	if (!std::isfinite(position.x) || !std::isfinite(position.z))
		return;

	m_cursorPosition.x = std::clamp(position.x, m_minX, m_maxX);
	m_cursorPosition.y = 0.0f;
	m_cursorPosition.z = std::clamp(position.z, m_minZ, m_maxZ);
}

void PS4GyroMapper::resetCursor()
{
	m_cursorPosition.x = std::clamp(150.0f, m_minX, m_maxX);
	m_cursorPosition.y = 0.0f;
	m_cursorPosition.z = std::clamp(0.0f, m_minZ, m_maxZ);
}

float PS4GyroMapper::applyDeadZone(float value) const
{
	const float magnitude = std::abs(value);
	if (magnitude <= m_config.deadZone)
		return 0.0f;

	const float range = m_config.fullScaleMotion - m_config.deadZone;
	const float scaled = (range > 0.0001f) ? std::min((magnitude - m_config.deadZone) / range, 1.0f) : 1.0f;
	const float smoothScaled = scaled * scaled * (3.0f - 2.0f * scaled);
	return std::copysign(smoothScaled, value);
}
