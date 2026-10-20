#include "PS4GyroInputController.h"
#include "PathEditor.h"
#include "mathUtility.h"

PS4GyroInputController::PS4GyroInputController(const PS4GyroMapperConfig& config)
	: m_mapper(config)
{
}

bool PS4GyroInputController::connect()
{
	std::string err;
	if (m_controller.open(err))
	{
		m_mapper.beginCalibration();
		m_statusMessage = "Connected; hold still for neutral calibration";
		return true;
	}
	m_statusMessage = err;
	return false;
}

void PS4GyroInputController::disconnect()
{
	m_controller.close();
	m_recording = false;
	m_lastButtonCross = false;
	m_lastButtonCircle = false;
	m_lastButtonSquare = false;
	m_lastButtonTriangle = false;
	m_lastButtonOptions = false;
	m_lastButtonShare = false;
	m_statusMessage = "PS4 controller disconnected";
}

void PS4GyroInputController::toggleConnection()
{
	if (m_controller.isConnected())
		disconnect();
	else
		connect();
}

void PS4GyroInputController::beginCalibration()
{
	m_mapper.beginCalibration();
	m_statusMessage = "Hold still; calibrating controller neutral";
}

void PS4GyroInputController::resetCursor(const Vector3& referencePosition)
{
	m_mapper.setCursorPosition(referencePosition);
}

void PS4GyroInputController::setBounds(float minX, float maxX, float minZ, float maxZ)
{
	m_mapper.setBounds(minX, maxX, minZ, maxZ);
}

void PS4GyroInputController::startRecording(PathEditor& editor, double timestampSeconds)
{
	if (!m_controller.isConnected() || !m_controller.hasRecentSample(std::chrono::milliseconds(1000)))
	{
		m_statusMessage = "Connect PS4 controller and wait for valid data";
		return;
	}
	if (!m_mapper.isCalibrated())
	{
		m_statusMessage = "Calibration incomplete; hold still or press G/Options";
		return;
	}
	if (editor.getState() == EditorState::Recording)
		return;

	const Vector3& cursor = m_mapper.getCursorPosition();
	m_lastRecordTime = timestampSeconds;
	m_lastRecordedPosition = cursor;
	editor.setInputMethod("ps4-gyro");
	editor.beginStroke(cursor, timestampSeconds);
	m_recording = editor.getState() == EditorState::Recording;
	m_statusMessage = m_recording ? "Recording gyro-controlled path" : "Editor cannot start recording now";
}

void PS4GyroInputController::stopRecording(PathEditor& editor)
{
	if (m_recording || editor.getState() == EditorState::Recording)
	{
		editor.endStroke();
	}
	m_recording = false;
}

void PS4GyroInputController::toggleRecording(PathEditor& editor, double timestampSeconds)
{
	if (m_recording || editor.getState() == EditorState::Recording)
		stopRecording(editor);
	else
		startRecording(editor, timestampSeconds);
}

void PS4GyroInputController::handleButtonEdges(const PS4ControllerSample& sample, PathEditor& editor, double timestampSeconds)
{
	// PS4 X button (Cross): Start / Stop recording (replaces Arduino D4 / keyboard B)
	if (sample.buttonCross && !m_lastButtonCross)
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

	// Circle button: Confirm path (equivalent to Enter / Space / N)
	if (sample.buttonCircle && !m_lastButtonCircle)
	{
		m_confirmRequested = true;
	}

	// Square button: Clear path (equivalent to C)
	if (sample.buttonSquare && !m_lastButtonSquare)
	{
		m_clearRequested = true;
	}

	// Triangle button: Reset cursor (equivalent to O)
	if (sample.buttonTriangle && !m_lastButtonTriangle)
	{
		m_resetCursorRequested = true;
	}

	// Options / Share buttons: Recalibrate (equivalent to G)
	if ((sample.buttonOptions && !m_lastButtonOptions) || (sample.buttonShare && !m_lastButtonShare))
	{
		beginCalibration();
	}

	m_lastButtonCross = sample.buttonCross;
	m_lastButtonCircle = sample.buttonCircle;
	m_lastButtonSquare = sample.buttonSquare;
	m_lastButtonTriangle = sample.buttonTriangle;
	m_lastButtonOptions = sample.buttonOptions;
	m_lastButtonShare = sample.buttonShare;
}

void PS4GyroInputController::update(float deltaTimeSeconds, double currentTimestampSeconds, PathEditor& editor)
{
	// Disconnect handling
	if (!m_controller.isConnected())
	{
		m_lastButtonCross = false;
		m_lastButtonCircle = false;
		m_lastButtonSquare = false;
		m_lastButtonTriangle = false;
		m_lastButtonOptions = false;
		m_lastButtonShare = false;

		if (m_recording)
		{
			stopRecording(editor);
			m_statusMessage = "PS4 controller disconnected; recording stopped safely";
		}
		return;
	}

	std::vector<PS4ControllerSample> samples;
	m_controller.poll(samples);

	for (const PS4ControllerSample& sample : samples)
	{
		m_mapper.consumeSample(sample);
		handleButtonEdges(sample, editor, currentTimestampSeconds);
	}

	if (!m_controller.isConnected())
	{
		if (m_recording)
			stopRecording(editor);
		m_statusMessage = m_controller.getStatusMessage();
		return;
	}

	if (m_mapper.isCalibrated())
	{
		m_mapper.update(deltaTimeSeconds);
	}

	if (m_recording)
	{
		if (!m_controller.hasRecentSample(std::chrono::milliseconds(1000)))
		{
			stopRecording(editor);
			m_statusMessage = "PS4 gyro data stale; recording stopped safely";
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

	if (m_mapper.isCalibrated() && m_controller.hasRecentSample(std::chrono::milliseconds(1000)) && !m_recording)
		m_statusMessage = "Calibrated; X starts at car, Triangle returns to car";
	else if (!m_mapper.isCalibrated())
		m_statusMessage = "Calibrating neutral " + std::to_string(m_mapper.getCalibrationSamplesCollected()) + "/" +
			std::to_string(m_mapper.getCalibrationSamplesRequired());
	else if (!m_controller.hasRecentSample(std::chrono::milliseconds(1000)) && !m_recording)
		m_statusMessage = "Connected; waiting for fresh PS4 gyro data";
}

bool PS4GyroInputController::consumeConfirmRequested()
{
	const bool req = m_confirmRequested;
	m_confirmRequested = false;
	return req;
}

bool PS4GyroInputController::consumeClearRequested()
{
	const bool req = m_clearRequested;
	m_clearRequested = false;
	return req;
}

bool PS4GyroInputController::consumeRecalibrateRequested()
{
	const bool req = m_recalibrateRequested;
	m_recalibrateRequested = false;
	return req;
}

bool PS4GyroInputController::consumeResetCursorRequested()
{
	const bool req = m_resetCursorRequested;
	m_resetCursorRequested = false;
	return req;
}
