#include "PS4ControllerInputTests.h"
#include "PS4Parser.h"
#include "PS4GyroMapper.h"
#include "PS4GyroInputController.h"
#include "MouseInputController.h"
#include "ArduinoInputController.h"
#include "PathEditor.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace {

bool approxEqual(float left, float right, float tolerance = 0.005f)
{
	return std::abs(left - right) <= tolerance;
}

void writeInt16LE(std::uint8_t* dest, std::int16_t value)
{
	dest[0] = static_cast<std::uint8_t>(value & 0xFF);
	dest[1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
}

bool testUSBReportParsing()
{
	std::vector<std::uint8_t> report(64, 0);
	report[0] = 0x01; // USB Report ID

	// Byte 5: D-pad=0 (North), Cross=1 (bit 5, 0x20)
	report[5] = 0x00 | 0x20;

	// Byte 6: L1=1 (0x01), Options=1 (0x20)
	report[6] = 0x01 | 0x20;

	// Byte 7: PS=1 (0x01)
	report[7] = 0x01;

	// Gyro: Pitch = 1024 (1.0 rad/s), Yaw = -512 (-0.5 rad/s), Roll = 2048 (2.0 rad/s)
	writeInt16LE(&report[13], 1024);
	writeInt16LE(&report[15], -512);
	writeInt16LE(&report[17], 2048);

	// Accel: AccelX = 4096 (0.5g), AccelZ = 8192 (1.0g), AccelY = -4096 (-0.5g)
	writeInt16LE(&report[19], 4096);
	writeInt16LE(&report[21], 8192);
	writeInt16LE(&report[23], -4096);

	PS4ControllerSample sample;
	assert(PS4Parser::parseReport(report.data(), report.size(), sample));

	assert(sample.buttonCross);
	assert(!sample.buttonCircle);
	assert(!sample.buttonSquare);
	assert(!sample.buttonTriangle);
	assert(sample.buttonL1);
	assert(!sample.buttonR1);
	assert(sample.buttonOptions);
	assert(!sample.buttonShare);
	assert(sample.buttonPS);
	assert(!sample.buttonTouchpad);
	assert(sample.dpad == 0);

	assert(approxEqual(sample.pitchRate, 1.0f));
	assert(approxEqual(sample.yawRate, -0.5f));
	assert(approxEqual(sample.rollRate, 2.0f));

	assert(approxEqual(sample.accelX, 0.5f));
	assert(approxEqual(sample.accelZ, 1.0f));
	assert(approxEqual(sample.accelY, -0.5f));

	// Test alternate face buttons
	report[5] = 0x10 | 0x40 | 0x80; // Square, Circle, Triangle pressed, Cross released
	assert(PS4Parser::parseReport(report.data(), report.size(), sample));
	assert(!sample.buttonCross);
	assert(sample.buttonCircle);
	assert(sample.buttonSquare);
	assert(sample.buttonTriangle);

	return true;
}

bool testBluetoothReportParsing()
{
	std::vector<std::uint8_t> report(78, 0);
	report[0] = 0x11; // Bluetooth Report ID
	report[1] = 0xC0; // BT header flag
	report[2] = 0x00;

	// Byte 7: D-pad=4 (South), Cross=1 (0x20), Square=1 (0x10)
	report[7] = 0x04 | 0x20 | 0x10;

	// Byte 8: R1=1 (0x02), Share=1 (0x10)
	report[8] = 0x02 | 0x10;

	// Byte 9: Touchpad=1 (0x02)
	report[9] = 0x02;

	// Gyro: Pitch = -1024 (-1.0 rad/s), Yaw = 0, Roll = 1536 (1.5 rad/s)
	writeInt16LE(&report[15], -1024);
	writeInt16LE(&report[17], 0);
	writeInt16LE(&report[19], 1536);

	// Accel: AccelX = 0, AccelZ = 8192 (1.0g), AccelY = 0
	writeInt16LE(&report[21], 0);
	writeInt16LE(&report[23], 8192);
	writeInt16LE(&report[25], 0);

	PS4ControllerSample sample;
	assert(PS4Parser::parseReport(report.data(), report.size(), sample));

	assert(sample.buttonCross);
	assert(sample.buttonSquare);
	assert(!sample.buttonCircle);
	assert(!sample.buttonTriangle);
	assert(sample.buttonR1);
	assert(!sample.buttonL1);
	assert(sample.buttonShare);
	assert(!sample.buttonOptions);
	assert(sample.buttonTouchpad);
	assert(!sample.buttonPS);
	assert(sample.dpad == 4);

	assert(approxEqual(sample.pitchRate, -1.0f));
	assert(approxEqual(sample.yawRate, 0.0f));
	assert(approxEqual(sample.rollRate, 1.5f));

	assert(approxEqual(sample.accelX, 0.0f));
	assert(approxEqual(sample.accelZ, 1.0f));
	assert(approxEqual(sample.accelY, 0.0f));

	return true;
}

bool testMalformedReportRejection()
{
	PS4ControllerSample sample;
	// Null pointer
	assert(!PS4Parser::parseReport(nullptr, 64, sample));

	// Zero length
	std::uint8_t dummy = 0x01;
	assert(!PS4Parser::parseReport(&dummy, 0, sample));

	// Truncated USB report
	std::vector<std::uint8_t> truncatedUSB(63, 0);
	truncatedUSB[0] = 0x01;
	assert(!PS4Parser::parseReport(truncatedUSB.data(), truncatedUSB.size(), sample));

	// Truncated Bluetooth report
	std::vector<std::uint8_t> truncatedBT(77, 0);
	truncatedBT[0] = 0x11;
	assert(!PS4Parser::parseReport(truncatedBT.data(), truncatedBT.size(), sample));

	// Unknown Report ID
	std::vector<std::uint8_t> unknown(64, 0);
	unknown[0] = 0x05;
	assert(!PS4Parser::parseReport(unknown.data(), unknown.size(), sample));

	return true;
}

bool testGyroMapperCalibrationAndFiltering()
{
	PS4GyroMapperConfig config;
	config.calibrationSamples = 4;
	config.filterAlpha = 0.5f;
	config.deadZone = 0.05f;
	config.sensitivity = 100.0f;
	config.maximumSpeed = 150.0f;
	config.invertX = false;
	config.invertZ = false;
	config.useGyroRate = true;

	PS4GyroMapper mapper(config);
	assert(!mapper.isCalibrated());

	// Feed 3 samples: not calibrated yet
	PS4ControllerSample s;
	s.pitchRate = 0.10f;
	s.rollRate = 0.20f;
	mapper.consumeSample(s);
	mapper.consumeSample(s);
	mapper.consumeSample(s);
	assert(!mapper.isCalibrated());
	assert(mapper.getCalibrationSamplesCollected() == 3);

	// 4th sample: completes calibration
	s.pitchRate = 0.10f;
	s.rollRate = 0.20f;
	mapper.consumeSample(s);
	assert(mapper.isCalibrated());
	assert(mapper.getCalibrationSamplesCollected() == 4);
	assert(approxEqual(mapper.getNeutralPitch(), 0.10f));
	assert(approxEqual(mapper.getNeutralRoll(), 0.20f));

	// Feed sample equal to neutral: filtered sample equals neutral
	mapper.consumeSample(s);
	assert(approxEqual(mapper.getFilteredPitch(), 0.10f));
	assert(approxEqual(mapper.getFilteredRoll(), 0.20f));

	// Test exponential filtering: step from 0.10 to 0.30 with alpha=0.5
	s.pitchRate = 0.30f;
	s.rollRate = 0.20f;
	mapper.consumeSample(s);
	// Expected filtered pitch = 0.10 + 0.5 * (0.30 - 0.10) = 0.20
	assert(approxEqual(mapper.getFilteredPitch(), 0.20f));

	// Re-calibration reset
	mapper.beginCalibration();
	assert(!mapper.isCalibrated());
	assert(mapper.getCalibrationSamplesCollected() == 0);

	return true;
}

bool testGyroMapperDeadZoneVelocityAndBounds()
{
	PS4GyroMapperConfig config;
	config.calibrationSamples = 2;
	config.filterAlpha = 1.0f; // Instant pass-through for predictable testing
	config.deadZone = 0.10f;
	config.fullScaleMotion = 1.0f;
	config.sensitivity = 100.0f;
	config.maximumSpeed = 50.0f;
	config.invertX = false;
	config.invertZ = false;
	config.useGyroRate = true;

	PS4GyroMapper mapper(config);
	mapper.setBounds(100.0f, 200.0f, -50.0f, 50.0f);
	mapper.setCursorPosition(Vector3(150.0f, 0.0f, 0.0f));

	// Calibrate neutral to zero
	PS4ControllerSample zero;
	zero.pitchRate = 0.0f;
	zero.rollRate = 0.0f;
	mapper.consumeSample(zero);
	mapper.consumeSample(zero);
	assert(mapper.isCalibrated());

	// Test dead zone: deviation 0.05 is below deadZone 0.10 -> velocity is 0
	PS4ControllerSample subDeadZone;
	subDeadZone.pitchRate = 0.05f;
	subDeadZone.rollRate = -0.05f;
	mapper.consumeSample(subDeadZone);
	mapper.update(0.1f);
	assert(approxEqual(mapper.getCursorPosition().x, 150.0f));
	assert(approxEqual(mapper.getCursorPosition().z, 0.0f));

	// Test motion above dead zone: positive roll moves +X
	PS4ControllerSample rollRight;
	rollRight.pitchRate = 0.0f;
	rollRight.rollRate = 0.5f;
	mapper.consumeSample(rollRight);
	mapper.update(0.1f);
	assert(mapper.getCursorPosition().x > 150.0f);
	assert(approxEqual(mapper.getCursorPosition().z, 0.0f));

	// Test pitch motion: positive pitch moves +Z
	mapper.setCursorPosition(Vector3(150.0f, 0.0f, 0.0f));
	PS4ControllerSample pitchUp;
	pitchUp.pitchRate = 0.5f;
	pitchUp.rollRate = 0.0f;
	mapper.consumeSample(pitchUp);
	mapper.update(0.1f);
	assert(approxEqual(mapper.getCursorPosition().x, 150.0f));
	assert(mapper.getCursorPosition().z > 0.0f);

	// Test bounds clamping: drive cursor towards max bounds
	PS4ControllerSample fullTilt;
	fullTilt.pitchRate = 2.0f;
	fullTilt.rollRate = 2.0f;
	for (int i = 0; i < 30; ++i)
	{
		mapper.consumeSample(fullTilt);
		mapper.update(0.1f);
	}
	assert(mapper.getCursorPosition().x <= 200.0f);
	assert(mapper.getCursorPosition().z <= 50.0f);

	// Test reset cursor
	mapper.resetCursor();
	assert(approxEqual(mapper.getCursorPosition().x, 150.0f));
	assert(approxEqual(mapper.getCursorPosition().z, 0.0f));

	return true;
}

bool testGyroMapperSustainedTiltMotion()
{
	PS4GyroMapperConfig config;
	config.calibrationSamples = 2;
	config.filterAlpha = 1.0f;

	PS4GyroMapper mapper(config);
	PS4ControllerSample neutral;
	neutral.accelZ = 1.0f;
	mapper.consumeSample(neutral);
	mapper.consumeSample(neutral);
	assert(mapper.isCalibrated());

	PS4ControllerSample tilted = neutral;
	tilted.accelY = -0.5f;
	tilted.accelZ = std::sqrt(0.75f);
	mapper.consumeSample(tilted);
	mapper.update(0.016f);
	const float firstTiltPosition = mapper.getCursorPosition().z;

	// The cursor keeps moving while the controller remains tilted, even without new gyro motion.
	mapper.update(0.016f);
	const float secondTiltPosition = mapper.getCursorPosition().z;
	assert(!approxEqual(firstTiltPosition, secondTiltPosition));

	mapper.consumeSample(neutral);
	mapper.update(0.016f);
	const float neutralPosition = mapper.getCursorPosition().z;
	mapper.update(0.016f);
	assert(approxEqual(mapper.getCursorPosition().z, neutralPosition));

	return true;
}

bool testPS4XButtonEditorCycleAndAutoFollow()
{
	PS4GyroMapperConfig config;
	config.calibrationSamples = 2;
	config.filterAlpha = 1.0f;

	PS4GyroInputController controller(config);
	PathEditor editor;

	// Inject 2 neutral calibration samples
	PS4ControllerSample calSample;
	calSample.pitchRate = 0.0f;
	calSample.rollRate = 0.0f;
	controller.getController().injectSample(calSample);
	controller.getController().injectSample(calSample);
	controller.update(0.016f, 1.0, editor);
	assert(controller.isCalibrated());

	// First press of X enters path editing.
	PS4ControllerSample pressX = calSample;
	pressX.buttonCross = true;
	controller.getController().injectSample(pressX);
	controller.update(0.016f, 1.1, editor);
	assert(controller.consumeEnterEditorRequested());
	assert(!controller.isRecording());
	assert(editor.getState() == EditorState::Idle);
	editor.enterEditMode();
	const Vector3 carPosition(40.0f, 0.0f, 60.0f);
	controller.resetCursor(carPosition);

	// The cursor can move while editing, but the next stroke starts at the car.
	PS4ControllerSample releaseX = calSample;
	controller.getController().injectSample(releaseX);
	controller.update(0.016f, 1.15, editor);
	controller.setCursorPosition(Vector3(170.0f, 0.0f, 30.0f));
	controller.getController().injectSample(pressX);
	controller.update(0.016f, 1.2, editor);

	assert(controller.isRecording());
	assert(editor.getState() == EditorState::Recording);
	assert(editor.getStats().inputMethod == "ps4-gyro");
	assert(approxEqual(editor.getRawSamples().front().position.x, carPosition.x));
	assert(approxEqual(editor.getRawSamples().front().position.z, carPosition.z));

	// Add enough movement for the stroke processor to accept the path.
	editor.addSample(Vector3(160.0f, 0.0f, 20.0f), 1.25);
	editor.addSample(Vector3(180.0f, 0.0f, 40.0f), 1.27);

	// Releasing X continues recording.
	controller.getController().injectSample(releaseX);
	controller.update(0.016f, 1.28, editor);
	assert(controller.isRecording());
	assert(editor.getState() == EditorState::Recording);

	// Pressing X again finishes the path and requests automatic following.
	controller.getController().injectSample(pressX);
	controller.update(0.016f, 1.3, editor);

	assert(!controller.isRecording());
	assert(editor.getState() == EditorState::Preview);
	assert(editor.getProcessedPath().valid);
	assert(controller.consumeConfirmRequested());
	assert(!controller.consumeConfirmRequested());

	// Other face buttons do not trigger path workflow actions.
	PS4ControllerSample circlePress = calSample;
	circlePress.buttonCircle = true;
	controller.getController().injectSample(circlePress);
	controller.update(0.016f, 1.4, editor);
	assert(!controller.consumeConfirmRequested());

	PS4ControllerSample squarePress = calSample;
	squarePress.buttonSquare = true;
	controller.getController().injectSample(squarePress);
	controller.update(0.016f, 1.5, editor);
	assert(!controller.consumeClearRequested());

	PS4ControllerSample trianglePress = calSample;
	trianglePress.buttonTriangle = true;
	controller.getController().injectSample(trianglePress);
	controller.update(0.016f, 1.6, editor);
	assert(!controller.consumeResetCursorRequested());

	return true;
}

bool testDisconnectHandlingDuringRecording()
{
	PS4GyroMapperConfig config;
	config.calibrationSamples = 2;
	PS4GyroInputController controller(config);
	PathEditor editor;
	editor.enterEditMode();

	// Calibrate
	PS4ControllerSample sample;
	sample.pitchRate = 0.0f;
	sample.rollRate = 0.0f;
	controller.getController().injectSample(sample);
	controller.getController().injectSample(sample);
	controller.update(0.016f, 1.0, editor);
	assert(controller.isCalibrated());

	// Start recording
	controller.startRecording(editor, 1.0);
	assert(controller.isRecording());
	assert(editor.getState() == EditorState::Recording);

	// Add a motion sample
	controller.setCursorPosition(Vector3(160.0f, 0.0f, 20.0f));
	controller.update(0.016f, 1.2, editor);

	// Disconnect controller mid-recording
	controller.getController().setSimulatedConnected(false);
	controller.update(0.016f, 1.3, editor);

	// Recording must stop safely, editor transitions to Preview, samples preserved
	assert(!controller.isRecording());
	assert(editor.getState() == EditorState::Preview);
	assert(editor.hasSamples());
	assert(controller.getStatusMessage().find("disconnected") != std::string::npos);

	return true;
}

bool testUnifiedIPathInputControllerPipeline()
{
	// Test that Mouse, Arduino, and PS4 Gyro all implement IPathInputController
	// and produce identical RawPathSample data into PathEditor and PathProcessor
	std::vector<std::unique_ptr<IPathInputController>> controllers;
	controllers.push_back(std::make_unique<MouseInputController>());
	controllers.push_back(std::make_unique<ArduinoInputController>());
	controllers.push_back(std::make_unique<PS4GyroInputController>());

	assert(controllers[0]->getMode() == PathInputMode::Mouse);
	assert(controllers[1]->getMode() == PathInputMode::Arduino);
	assert(controllers[2]->getMode() == PathInputMode::PS4Gyro);

	// Test feeding strokes from all 3 controllers into PathEditor
	for (auto& ctrl : controllers)
	{
		PathEditor editor;
		editor.enterEditMode();

		editor.setInputMethod(ctrl->getModeName());
		editor.beginStroke(Vector3(0.0f, 0.0f, 0.0f), 0.0);
		editor.addSample(Vector3(0.0f, 0.0f, 20.0f), 0.2);
		editor.addSample(Vector3(20.0f, 0.0f, 40.0f), 0.4);
		editor.endStroke();

		assert(editor.getState() == EditorState::Preview);
		assert(editor.getProcessedPath().valid);
		assert(editor.getProcessedPath().points.size() > 2);
		assert(editor.getStats().inputMethod == ctrl->getModeName());
	}

	return true;
}

} // anonymous namespace

bool RunPS4ControllerInputTests()
{
	std::cout << "\n=== Running PS4 DualShock 4 Gyro Input Tests ===" << std::endl;

	assert(testUSBReportParsing());
	std::cout << "[PASS] PS4 USB Report 0x01 parsing (gyro, accel, buttons)" << std::endl;

	assert(testBluetoothReportParsing());
	std::cout << "[PASS] PS4 Bluetooth Report 0x11 parsing (gyro, accel, buttons)" << std::endl;

	assert(testMalformedReportRejection());
	std::cout << "[PASS] Malformed and truncated HID report rejection" << std::endl;

	assert(testGyroMapperCalibrationAndFiltering());
	std::cout << "[PASS] Neutral calibration and exponential low-pass filtering" << std::endl;

	assert(testGyroMapperDeadZoneVelocityAndBounds());
	std::cout << "[PASS] Dead zone thresholding, velocity mapping, and bounds clamping" << std::endl;

	assert(testGyroMapperSustainedTiltMotion());
	std::cout << "[PASS] Sustained cursor motion while tilted and stopping at neutral" << std::endl;

	assert(testPS4XButtonEditorCycleAndAutoFollow());
	std::cout << "[PASS] PS4 X editor cycle and automatic follow request" << std::endl;

	assert(testDisconnectHandlingDuringRecording());
	std::cout << "[PASS] Robust disconnect handling and recording safety" << std::endl;

	assert(testUnifiedIPathInputControllerPipeline());
	std::cout << "[PASS] Unified IPathInputController pipeline across Mouse, Arduino, and PS4 Gyro" << std::endl;

	std::cout << "=== All PS4 Controller Input Tests Passed! ===\n" << std::endl;
	return true;
}
