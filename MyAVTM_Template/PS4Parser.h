#pragma once

#include <cstddef>
#include <cstdint>

struct PS4ControllerSample
{
	std::uint32_t timestampMilliseconds = 0;

	// Gyroscope angular rates (signed 16-bit decoded to radians/sec or normalized units)
	float pitchRate = 0.0f; // Gyro X: pitch motion
	float yawRate = 0.0f;   // Gyro Z: yaw motion
	float rollRate = 0.0f;  // Gyro Y: roll motion

	// Accelerometer linear acceleration in g's
	float accelX = 0.0f;
	float accelY = 0.0f;
	float accelZ = 0.0f;

	// Face buttons
	bool buttonCross = false;    // X button (Start/Stop recording)
	bool buttonCircle = false;   // Circle (Confirm path)
	bool buttonSquare = false;   // Square (Clear path)
	bool buttonTriangle = false; // Triangle (Reset cursor)

	// Shoulder & trigger buttons
	bool buttonL1 = false;
	bool buttonR1 = false;
	bool buttonL2 = false;
	bool buttonR2 = false;

	// Center & system buttons
	bool buttonShare = false;
	bool buttonOptions = false;  // Options (Recalibrate)
	bool buttonL3 = false;
	bool buttonR3 = false;
	bool buttonPS = false;
	bool buttonTouchpad = false;

	// D-pad (0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW, 8=neutral)
	std::uint8_t dpad = 8;
};

class PS4Parser
{
public:
	// Parses a raw HID report buffer (supports USB Report 0x01 and Bluetooth Report 0x11)
	static bool parseReport(const std::uint8_t* data, std::size_t size, PS4ControllerSample& outSample);

private:
	static std::int16_t readInt16LE(const std::uint8_t* p);
};
