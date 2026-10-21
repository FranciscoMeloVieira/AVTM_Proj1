#pragma once

#include "IPathInputController.h"
#include "AccelerometerInput.h"
#include "SerialPort.h"
#include <chrono>

class ArduinoInputController : public IPathInputController
{
public:
	explicit ArduinoInputController(const AccelerometerMapperConfig& config = {});
	~ArduinoInputController() override = default;

	PathInputMode getMode() const override { return PathInputMode::Arduino; }
	std::string getModeName() const override { return "Arduino Tilt"; }
	std::string getStatusMessage() const override { return m_statusMessage; }

	bool isConnected() const override { return m_serial.isConnected(); }
	bool connect() override;
	void disconnect() override;
	void toggleConnection() override;

	bool supportsCalibration() const override { return true; }
	bool isCalibrated() const override { return m_mapper.isCalibrated(); }
	void beginCalibration() override;
	std::size_t getCalibrationProgress() const override { return m_mapper.getCalibrationSamplesCollected(); }
	std::size_t getCalibrationRequired() const override { return m_mapper.getCalibrationSamplesRequired(); }

	bool hasCursor() const override { return true; }
	Vector3 getCursorPosition() const override { return m_mapper.getCursorPosition(); }
	void setCursorPosition(const Vector3& position) override { m_mapper.setCursorPosition(position); }
	void resetCursor(const Vector3& referencePosition) override;
	void setBounds(float minX, float maxX, float minZ, float maxZ) override;

	bool isRecording() const override { return m_recording; }
	void startRecording(PathEditor& editor, double timestampSeconds) override;
	void stopRecording(PathEditor& editor) override;
	void toggleRecording(PathEditor& editor, double timestampSeconds) override;

	void update(float deltaTimeSeconds, double currentTimestampSeconds, PathEditor& editor) override;

	bool consumeEnterEditorRequested() override;
	bool consumeConfirmRequested() override;

	// Accessors for diagnostics / tests
	SerialPort& getSerialPort() { return m_serial; }
	const SerialPort& getSerialPort() const { return m_serial; }
	AccelerometerMapper& getMapper() { return m_mapper; }
	const AccelerometerMapper& getMapper() const { return m_mapper; }

private:
	SerialPort m_serial;
	AccelerometerMapper m_mapper;
	bool m_recording = false;
	bool m_buttonWasPressed = false;
	unsigned int m_pendingButtonPresses = 0;
	std::string m_statusMessage = "Arduino disconnected";
	bool m_enterEditorRequested = false;
	bool m_confirmRequested = false;
	Vector3 m_recordingStartPosition{ 150.0f, 0.0f, 0.0f };
	Vector3 m_lastRecordedPosition{ 150.0f, 0.0f, 0.0f };
	double m_lastRecordTime = 0.0;

	void handleButtonPress(PathEditor& editor, double timestampSeconds);
};
