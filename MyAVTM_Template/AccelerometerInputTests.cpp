#include "AccelerometerInputTests.h"
#include "AccelerometerInput.h"
#include "PathEditor.h"
#include <cassert>
#include <cmath>
#include <iostream>

namespace {

bool near(float left, float right, float tolerance = 0.001f)
{
	return std::abs(left - right) <= tolerance;
}

bool testProtocolParsing()
{
	AccelerometerSample sample;
	assert(AccelerometerProtocol::parseLine("ACCEL,1,12345,0.125,-0.250,0.980\r", sample));
	assert(sample.timestampMilliseconds == 12345);
	assert(near(sample.x, 0.125f));
	assert(near(sample.y, -0.25f));
	assert(near(sample.z, 0.98f));

	assert(!AccelerometerProtocol::parseLine("ACCEL,2,12345,0,0,1", sample));
	assert(!AccelerometerProtocol::parseLine("ACCEL,1,12345,0,0", sample));
	assert(!AccelerometerProtocol::parseLine("ACCEL,1,12345,0,0,1,extra", sample));
	assert(!AccelerometerProtocol::parseLine("ACCEL,1,12345,nan,0,1", sample));
	assert(!AccelerometerProtocol::parseLine("ACCEL,1,12345,17,0,1", sample));
	return true;
}

bool testCalibrationFilteringAndBounds()
{
	AccelerometerMapperConfig config;
	config.calibrationSamples = 3;
	config.filterAlpha = 1.0f;
	config.sensitivity = 100.0f;
	config.maximumSpeed = 50.0f;
	AccelerometerMapper mapper(config);

	mapper.beginCalibration();
	mapper.consumeSample({ 0, 0.1f, 0.2f, 0.95f });
	mapper.consumeSample({ 20, 0.1f, 0.2f, 0.95f });
	assert(!mapper.isCalibrated());
	mapper.consumeSample({ 40, 0.1f, 0.2f, 0.95f });
	assert(mapper.isCalibrated());
	assert(mapper.getCalibrationSamplesCollected() == 3);

	mapper.consumeSample({ 60, 0.12f, 0.22f, 0.95f });
	mapper.update(0.1f);
	assert(near(mapper.getCursorPosition().x, 150.0f));
	assert(near(mapper.getCursorPosition().z, 0.0f));

	mapper.consumeSample({ 80, 0.1f, 0.3f, 0.95f });
	mapper.update(0.1f);
	assert(mapper.getCursorPosition().x < 150.0f);
	assert(near(mapper.getCursorPosition().z, 0.0f));

	mapper.setCursorPosition(Vector3(150.0f, 0.0f, 0.0f));
	mapper.consumeSample({ 100, 0.1f, 0.1f, 0.95f });
	mapper.update(0.1f);
	assert(mapper.getCursorPosition().x > 150.0f);
	assert(near(mapper.getCursorPosition().z, 0.0f));

	mapper.setCursorPosition(Vector3(150.0f, 0.0f, 0.0f));
	mapper.consumeSample({ 120, 0.3f, 0.2f, 0.95f });
	mapper.update(0.1f);
	assert(near(mapper.getCursorPosition().x, 150.0f));
	assert(mapper.getCursorPosition().z < 0.0f);

	mapper.setCursorPosition(Vector3(150.0f, 0.0f, 0.0f));
	mapper.consumeSample({ 140, -0.1f, 0.2f, 0.95f });
	mapper.update(0.1f);
	assert(near(mapper.getCursorPosition().x, 150.0f));
	assert(mapper.getCursorPosition().z > 0.0f);

	mapper.setBounds(149.0f, 151.0f, -1.0f, 1.0f);
	mapper.consumeSample({ 160, 2.0f, 0.2f, 0.95f });
	mapper.update(0.1f);
	mapper.update(0.1f);
	assert(mapper.getCursorPosition().x <= 151.0f);
	assert(mapper.getCursorPosition().z >= -1.0f);
	mapper.setCursorPosition(Vector3(200.0f, 10.0f, 20.0f));
	assert(near(mapper.getCursorPosition().x, 151.0f));
	assert(near(mapper.getCursorPosition().y, 0.0f));
	assert(near(mapper.getCursorPosition().z, 1.0f));
	mapper.resetCursor();
	assert(near(mapper.getCursorPosition().x, 150.0f));
	assert(near(mapper.getCursorPosition().z, 0.0f));
	return true;
}

bool testSharedPathEditorPipeline()
{
	PathEditor editor;
	editor.enterEditMode();
	editor.setInputMethod("arduino-tilt");
	editor.beginStroke(Vector3(0.0f, 0.0f, 0.0f), 0.0);
	editor.addSample(Vector3(0.0f, 0.0f, 20.0f), 0.2);
	editor.addSample(Vector3(20.0f, 0.0f, 40.0f), 0.4);
	editor.endStroke();

	assert(editor.getProcessedPath().valid);
	assert(editor.getStats().inputMethod == "arduino-tilt");
	return true;
}

}

bool RunAccelerometerInputTests()
{
	std::cout << "\n=== Running Accelerometer Input Tests ===" << std::endl;
	assert(testProtocolParsing());
	std::cout << "[PASS] Protocol parsing and validation" << std::endl;
	assert(testCalibrationFilteringAndBounds());
	std::cout << "[PASS] Calibration, dead zone, tilt mapping, and cursor bounds" << std::endl;
	assert(testSharedPathEditorPipeline());
	std::cout << "[PASS] Sensor samples use the shared PathEditor pipeline" << std::endl;
	return true;
}
