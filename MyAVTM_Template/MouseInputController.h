#pragma once

#include "IPathInputController.h"

class MouseInputController : public IPathInputController
{
public:
	MouseInputController() = default;
	~MouseInputController() override = default;

	PathInputMode getMode() const override { return PathInputMode::Mouse; }
	std::string getModeName() const override { return "Mouse"; }
	std::string getStatusMessage() const override { return "Mouse active (drag to draw)"; }

	bool isConnected() const override { return true; }
	bool connect() override { return true; }
	void disconnect() override {}
	void toggleConnection() override {}

	bool supportsCalibration() const override { return false; }
	bool isCalibrated() const override { return true; }
	void beginCalibration() override {}
	std::size_t getCalibrationProgress() const override { return 0; }
	std::size_t getCalibrationRequired() const override { return 0; }

	bool hasCursor() const override { return false; }
	Vector3 getCursorPosition() const override { return m_lastMouseWorld; }
	void setCursorPosition(const Vector3& position) override { m_lastMouseWorld = position; }
	void resetCursor(const Vector3& referencePosition) override { m_lastMouseWorld = referencePosition; }
	void setBounds(float minX, float maxX, float minZ, float maxZ) override {}

	bool isRecording() const override { return m_recording; }
	void startRecording(PathEditor& editor, double timestampSeconds) override;
	void stopRecording(PathEditor& editor) override;
	void toggleRecording(PathEditor& editor, double timestampSeconds) override;

	void update(float deltaTimeSeconds, double currentTimestampSeconds, PathEditor& editor) override;

	void onMouseDown(int screenX, int screenY, int windowWidth, int windowHeight,
		double timestampSeconds, PathEditor& editor) override;
	void onMouseDrag(int screenX, int screenY, int windowWidth, int windowHeight,
		double timestampSeconds, PathEditor& editor) override;
	void onMouseUp(int screenX, int screenY, int windowWidth, int windowHeight,
		double timestampSeconds, PathEditor& editor) override;

private:
	Vector3 m_lastMouseWorld{ 150.0f, 0.0f, 0.0f };
	bool m_recording = false;
};
