#include "ArduinoInputController.h"
#include "PathEditor.h"
#include "mathUtility.h"
#include <cstdlib>
#include <algorithm>

ArduinoInputController::ArduinoInputController(const AccelerometerMapperConfig& config)
	: m_mapper(config)
{
}

bool ArduinoInputController::connect()
{
	if (m_serial.isConnected())
		return true;

	const char* configuredPort = std::getenv("AVTM_ACCEL_COM");
	const std::string portName = configuredPort && *configuredPort ? configuredPort : "COM3";
	unsigned int baudRate = 115200;
	if (const char* configuredBaud = std::getenv("AVTM_ACCEL_BAUD"))
	{
		char* end = nullptr;
		const unsigned long parsedBaud = std::strtoul(configuredBaud, &end, 10);
		if (end != configuredBaud && *end == '\0' && parsedBaud >= 1200 && parsedBaud <= 2000000)
			baudRate = static_cast<unsigned int>(parsedBaud);
	}

	std::string errorMessage;
	if (!m_serial.open(portName, baudRate, errorMessage))
	{
		m_statusMessage = errorMessage;
		return false;
	}
	m_mapper.beginCalibration();
	m_statusMessage = "Connected; hold still for calibration";
	return true;
}

void ArduinoInputController::disconnect()
{
	if (m_serial.isConnected())
	{
		m_serial.close();
		m_statusMessage = "Arduino disconnected";
	}
	m_recording = false;
	m_buttonWasPressed = false;
	m_pendingButtonPresses = 0;
}

void ArduinoInputController::toggleConnection()
{
	if (m_serial.isConnected())
		disconnect();
	else
		connect();
}

void ArduinoInputController::beginCalibration()
{
	m_mapper.beginCalibration();
	m_statusMessage = "Hold still; calibrating neutral";
}

void ArduinoInputController::resetCursor(const Vector3& referencePosition)
{
	m_mapper.setCursorPosition(referencePosition);
}

void ArduinoInputController::setBounds(float minX, float maxX, float minZ, float maxZ)
{
	m_mapper.setBounds(minX, maxX, minZ, maxZ);
}

void ArduinoInputController::startRecording(PathEditor& editor, double timestampSeconds)
{
	if (!m_serial.isConnected() || !m_serial.hasRecentSample(std::chrono::milliseconds(1000)))
	{
		m_statusMessage = "Connect Arduino and wait for valid samples";
		return;
	}
	if (!m_mapper.isCalibrated())
	{
		m_statusMessage = "Calibration incomplete; hold still or press G";
		return;
	}
	if (editor.getState() == EditorState::Recording)
		return;

	const Vector3& cursor = m_mapper.getCursorPosition();
	m_lastRecordTime = timestampSeconds;
	m_lastRecordedPosition = cursor;
	editor.setInputMethod("arduino-tilt");
	editor.beginStroke(cursor, timestampSeconds);
	m_recording = editor.getState() == EditorState::Recording;
	m_statusMessage = m_recording ? "Recording tilt-controlled path" : "Editor cannot start recording now";
}

void ArduinoInputController::stopRecording(PathEditor& editor)
{
	if (m_recording || editor.getState() == EditorState::Recording)
	{
		editor.endStroke();
	}
	m_recording = false;
}

void ArduinoInputController::toggleRecording(PathEditor& editor, double timestampSeconds)
{
	if (m_recording || editor.getState() == EditorState::Recording)
		stopRecording(editor);
	else
		startRecording(editor, timestampSeconds);
}

void ArduinoInputController::handleButtonPress(PathEditor& editor, double timestampSeconds)
{
	const EditorState state = editor.getState();
	if (state == EditorState::Recording)
	{
		stopRecording(editor);
	}
	else if (state == EditorState::Editing || state == EditorState::Preview)
	{
		startRecording(editor, timestampSeconds);
	}
}

void ArduinoInputController::update(float deltaTimeSeconds, double currentTimestampSeconds, PathEditor& editor)
{
	if (!m_serial.isConnected())
	{
		m_buttonWasPressed = false;
		if (m_recording)
		{
			stopRecording(editor);
			m_statusMessage = "Serial disconnected; recording stopped safely";
		}
		return;
	}

	std::vector<AccelerometerSample> samples;
	m_serial.poll(samples);
	for (const AccelerometerSample& sample : samples)
	{
		m_mapper.consumeSample(sample);
		if (sample.buttonPressed && !m_buttonWasPressed)
			++m_pendingButtonPresses;
		m_buttonWasPressed = sample.buttonPressed;
	}

	if (!m_serial.isConnected())
	{
		if (m_recording)
			stopRecording(editor);
		m_statusMessage = m_serial.getStatusMessage();
		return;
	}

	while (m_pendingButtonPresses > 0)
	{
		--m_pendingButtonPresses;
		handleButtonPress(editor, currentTimestampSeconds);
	}

	if (m_mapper.isCalibrated())
	{
		m_mapper.update(deltaTimeSeconds);
	}

	if (m_recording)
	{
		if (!m_serial.hasRecentSample(std::chrono::milliseconds(1000)))
		{
			stopRecording(editor);
			m_statusMessage = "Sensor data stale; recording stopped safely";
			return;
		}

		if (editor.getState() == EditorState::Recording)
		{
			const Vector3& cursor = m_mapper.getCursorPosition();
			const float moved = Distance(m_lastRecordedPosition, cursor);
			if (moved >= 0.8f || (currentTimestampSeconds - m_lastRecordTime >= 0.05 && moved > 0.05f))
			{
				editor.addSample(cursor, currentTimestampSeconds);
				m_lastRecordedPosition = cursor;
				m_lastRecordTime = currentTimestampSeconds;
			}
			if (editor.getRawSamples().size() >= 10000)
			{
				stopRecording(editor);
				m_statusMessage = "Sample limit reached; recording stopped";
			}
		}
		else
		{
			m_recording = false;
		}
	}

	if (m_mapper.isCalibrated() && m_serial.hasRecentSample(std::chrono::milliseconds(1000)) && !m_recording)
		m_statusMessage = "Calibrated; B starts at car, O returns to car";
	else if (!m_mapper.isCalibrated())
		m_statusMessage = "Calibrating neutral " + std::to_string(m_mapper.getCalibrationSamplesCollected()) + "/" +
			std::to_string(m_mapper.getCalibrationSamplesRequired());
	else if (!m_serial.hasRecentSample(std::chrono::milliseconds(1000)) && !m_recording)
		m_statusMessage = "Connected; waiting for fresh accelerometer data";
}
