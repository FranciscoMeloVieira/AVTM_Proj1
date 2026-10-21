#pragma once

#include "IPathInputController.h"
#include "PS4Controller.h"
#include "PS4GyroMapper.h"

class PS4GyroInputController : public IPathInputController
{
public:
	explicit PS4GyroInputController(const PS4GyroMapperConfig& config = {});
	~PS4GyroInputController() override = default;

	PathInputMode getMode() const override { return PathInputMode::PS4Gyro; }
	std::string getModeName() const override { return "PS4 DualShock 4 Gyro"; }
	std::string getStatusMessage() const override { return m_statusMessage; }

	bool isConnected() const override { return m_controller.isConnected(); }
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

	// Hardware button action queries
	bool consumeEnterEditorRequested() override;
	bool consumeConfirmRequested() override;
	bool consumeClearRequested() override;
	bool consumeRecalibrateRequested() override;
	bool consumeResetCursorRequested() override;

	// Accessors for diagnostics and unit testing
	PS4Controller& getController() { return m_controller; }
	const PS4Controller& getController() const { return m_controller; }
	PS4GyroMapper& getMapper() { return m_mapper; }
	const PS4GyroMapper& getMapper() const { return m_mapper; }

private:
	PS4Controller m_controller;
	PS4GyroMapper m_mapper;

	bool m_recording = false;
	std::string m_statusMessage = "PS4 controller disconnected";

	Vector3 m_recordingStartPosition{ 150.0f, 0.0f, 0.0f };
	Vector3 m_lastRecordedPosition{ 150.0f, 0.0f, 0.0f };
	double m_lastRecordTime = 0.0;

	// Button edge tracking
	bool m_lastButtonCross = false;

	// Action requests triggered by controller buttons
	bool m_enterEditorRequested = false;
	bool m_confirmRequested = false;

	void handleButtonEdges(const PS4ControllerSample& sample, PathEditor& editor, double timestampSeconds);
};
