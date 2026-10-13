#include "AccelerometerInput.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <system_error>

bool AccelerometerProtocol::parseLine(std::string_view line, AccelerometerSample& outSample)
{
	if (!line.empty() && line.back() == '\r')
		line.remove_suffix(1);
	if (line.size() > 128)
		return false;

	std::array<std::string_view, 6> fields;
	std::size_t fieldStart = 0;
	for (std::size_t i = 0; i < fields.size(); ++i)
	{
		const std::size_t separator = line.find(',', fieldStart);
		if (i < fields.size() - 1)
		{
			if (separator == std::string_view::npos)
				return false;
			fields[i] = line.substr(fieldStart, separator - fieldStart);
			fieldStart = separator + 1;
		}
		else
		{
			if (separator != std::string_view::npos)
				return false;
			fields[i] = line.substr(fieldStart);
		}
	}

	if (fields[0] != "ACCEL" || fields[1] != "1")
		return false;

	AccelerometerSample sample;
	auto timestampResult = std::from_chars(fields[2].data(), fields[2].data() + fields[2].size(), sample.timestampMilliseconds);
	if (timestampResult.ec != std::errc() || timestampResult.ptr != fields[2].data() + fields[2].size())
		return false;

	float* values[] = { &sample.x, &sample.y, &sample.z };
	for (std::size_t i = 0; i < 3; ++i)
	{
		auto result = std::from_chars(fields[i + 3].data(), fields[i + 3].data() + fields[i + 3].size(), *values[i]);
		if (result.ec != std::errc() || result.ptr != fields[i + 3].data() + fields[i + 3].size() ||
			!std::isfinite(*values[i]) || std::abs(*values[i]) > 16.0f)
			return false;
	}

	outSample = sample;
	return true;
}

AccelerometerMapper::AccelerometerMapper(const AccelerometerMapperConfig& config)
	: m_config(config)
{
	m_config.calibrationSamples = std::max<std::size_t>(1, m_config.calibrationSamples);
	m_config.filterAlpha = std::clamp(m_config.filterAlpha, 0.01f, 1.0f);
	m_config.deadZone = std::clamp(m_config.deadZone, 0.0f, 0.95f);
	m_config.fullScaleTilt = std::max(m_config.fullScaleTilt, m_config.deadZone + 0.001f);
	m_config.sensitivity = std::max(0.0f, m_config.sensitivity);
	m_config.maximumSpeed = std::max(0.0f, m_config.maximumSpeed);
}

void AccelerometerMapper::beginCalibration()
{
	m_calibrationCount = 0;
	m_calibrationX = 0.0;
	m_calibrationY = 0.0;
	m_calibrationZ = 0.0;
	m_calibrated = false;
	m_hasFilteredSample = false;
}

void AccelerometerMapper::consumeSample(const AccelerometerSample& sample)
{
	if (!m_calibrated)
	{
		m_calibrationX += sample.x;
		m_calibrationY += sample.y;
		m_calibrationZ += sample.z;
		++m_calibrationCount;
		if (m_calibrationCount >= m_config.calibrationSamples)
		{
			const double count = static_cast<double>(m_calibrationCount);
			m_neutral.timestampMilliseconds = sample.timestampMilliseconds;
			m_neutral.x = static_cast<float>(m_calibrationX / count);
			m_neutral.y = static_cast<float>(m_calibrationY / count);
			m_neutral.z = static_cast<float>(m_calibrationZ / count);
			m_filtered = m_neutral;
			m_calibrated = true;
			m_hasFilteredSample = true;
		}
		return;
	}

	if (!m_hasFilteredSample)
	{
		m_filtered = sample;
		m_hasFilteredSample = true;
		return;
	}

	const float alpha = m_config.filterAlpha;
	m_filtered.x += alpha * (sample.x - m_filtered.x);
	m_filtered.y += alpha * (sample.y - m_filtered.y);
	m_filtered.z += alpha * (sample.z - m_filtered.z);
	m_filtered.timestampMilliseconds = sample.timestampMilliseconds;
}

void AccelerometerMapper::update(float deltaTimeSeconds)
{
	if (!m_calibrated || !m_hasFilteredSample || !std::isfinite(deltaTimeSeconds) || deltaTimeSeconds <= 0.0f)
		return;

	const float dt = std::min(deltaTimeSeconds, 0.1f);
	float velocityX = m_config.sensitivity * applyDeadZone(m_filtered.y - m_neutral.y);
	float velocityZ = -m_config.sensitivity * applyDeadZone(m_filtered.x - m_neutral.x);
	if (m_config.invertX)
		velocityX = -velocityX;
	if (m_config.invertY)
		velocityZ = -velocityZ;
	velocityX = std::clamp(velocityX, -m_config.maximumSpeed, m_config.maximumSpeed);
	velocityZ = std::clamp(velocityZ, -m_config.maximumSpeed, m_config.maximumSpeed);

	m_cursorPosition.x = std::clamp(m_cursorPosition.x + velocityX * dt, m_minX, m_maxX);
	m_cursorPosition.z = std::clamp(m_cursorPosition.z + velocityZ * dt, m_minZ, m_maxZ);
}

void AccelerometerMapper::setBounds(float minX, float maxX, float minZ, float maxZ)
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

void AccelerometerMapper::setCursorPosition(const Vector3& position)
{
	if (!std::isfinite(position.x) || !std::isfinite(position.z))
		return;
	m_cursorPosition.x = std::clamp(position.x, m_minX, m_maxX);
	m_cursorPosition.y = 0.0f;
	m_cursorPosition.z = std::clamp(position.z, m_minZ, m_maxZ);
}

void AccelerometerMapper::resetCursor()
{
	m_cursorPosition.x = std::clamp(150.0f, m_minX, m_maxX);
	m_cursorPosition.z = std::clamp(0.0f, m_minZ, m_maxZ);
}

float AccelerometerMapper::applyDeadZone(float value) const
{
	const float magnitude = std::abs(value);
	if (magnitude <= m_config.deadZone)
		return 0.0f;
	const float scaled = std::min((magnitude - m_config.deadZone) /
		(m_config.fullScaleTilt - m_config.deadZone), 1.0f);
	return std::copysign(scaled, value);
}
