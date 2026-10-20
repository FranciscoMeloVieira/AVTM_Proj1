#pragma once

#include "Vector3.h"
#include <cstddef>
#include <string>

class PathEditor;

enum class PathInputMode
{
	Mouse,
	Arduino,
	PS4Gyro
};

class IPathInputController
{
public:
	virtual ~IPathInputController() = default;

	// Mode and identification
	virtual PathInputMode getMode() const = 0;
	virtual std::string getModeName() const = 0;
	virtual std::string getStatusMessage() const = 0;

	// Connection lifecycle
	virtual bool isConnected() const = 0;
	virtual bool connect() = 0;
	virtual void disconnect() = 0;
	virtual void toggleConnection() = 0;

	// Calibration
	virtual bool supportsCalibration() const = 0;
	virtual bool isCalibrated() const = 0;
	virtual void beginCalibration() = 0;
	virtual std::size_t getCalibrationProgress() const = 0;
	virtual std::size_t getCalibrationRequired() const = 0;

	// Cursor management (for motion/sensor controllers)
	virtual bool hasCursor() const = 0;
	virtual Vector3 getCursorPosition() const = 0;
	virtual void setCursorPosition(const Vector3& position) = 0;
	virtual void resetCursor(const Vector3& referencePosition) = 0;
	virtual void setBounds(float minX, float maxX, float minZ, float maxZ) = 0;

	// Recording lifecycle
	virtual bool isRecording() const = 0;
	virtual void startRecording(PathEditor& editor, double timestampSeconds) = 0;
	virtual void stopRecording(PathEditor& editor) = 0;
	virtual void toggleRecording(PathEditor& editor, double timestampSeconds) = 0;

	// Per-frame update
	virtual void update(float deltaTimeSeconds, double currentTimestampSeconds, PathEditor& editor) = 0;

	// Mouse event forwarding (for MouseInputController)
	virtual void onMouseDown(int screenX, int screenY, int windowWidth, int windowHeight,
		double timestampSeconds, PathEditor& editor) {}
	virtual void onMouseDrag(int screenX, int screenY, int windowWidth, int windowHeight,
		double timestampSeconds, PathEditor& editor) {}
	virtual void onMouseUp(int screenX, int screenY, int windowWidth, int windowHeight,
		double timestampSeconds, PathEditor& editor) {}

	// Hardware button action triggers (e.g. PS4 face buttons)
	virtual bool consumeConfirmRequested() { return false; }
	virtual bool consumeClearRequested() { return false; }
	virtual bool consumeRecalibrateRequested() { return false; }
	virtual bool consumeResetCursorRequested() { return false; }
};
