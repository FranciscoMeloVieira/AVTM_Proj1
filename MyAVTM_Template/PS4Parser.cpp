#include "PS4Parser.h"
#include <cmath>

std::int16_t PS4Parser::readInt16LE(const std::uint8_t* p)
{
	const std::uint16_t u = static_cast<std::uint16_t>(p[0]) |
		(static_cast<std::uint16_t>(p[1]) << 8);
	return static_cast<std::int16_t>(u);
}

bool PS4Parser::parseReport(const std::uint8_t* data, std::size_t size, PS4ControllerSample& outSample)
{
	if (!data || size < 1)
		return false;

	const std::uint8_t reportId = data[0];
	PS4ControllerSample sample;

	if (reportId == 0x01)
	{
		// USB Input Report 0x01 (minimum 64 bytes)
		if (size < 64)
			return false;

		// Face buttons & D-pad in byte 5
		const std::uint8_t b5 = data[5];
		sample.dpad = b5 & 0x0F;
		sample.buttonSquare = (b5 & 0x10) != 0;
		sample.buttonCross = (b5 & 0x20) != 0;
		sample.buttonCircle = (b5 & 0x40) != 0;
		sample.buttonTriangle = (b5 & 0x80) != 0;

		// Shoulders, Options, Share in byte 6
		const std::uint8_t b6 = data[6];
		sample.buttonL1 = (b6 & 0x01) != 0;
		sample.buttonR1 = (b6 & 0x02) != 0;
		sample.buttonL2 = (b6 & 0x04) != 0;
		sample.buttonR2 = (b6 & 0x08) != 0;
		sample.buttonShare = (b6 & 0x10) != 0;
		sample.buttonOptions = (b6 & 0x20) != 0;
		sample.buttonL3 = (b6 & 0x40) != 0;
		sample.buttonR3 = (b6 & 0x80) != 0;

		// PS, Touchpad click in byte 7
		const std::uint8_t b7 = data[7];
		sample.buttonPS = (b7 & 0x01) != 0;
		sample.buttonTouchpad = (b7 & 0x02) != 0;

		// Gyroscope data at bytes 13-18 (int16_t little-endian)
		const std::int16_t rawPitch = readInt16LE(&data[13]);
		const std::int16_t rawYaw = readInt16LE(&data[15]);
		const std::int16_t rawRoll = readInt16LE(&data[17]);

		// Accelerometer data at bytes 19-24 (int16_t little-endian)
		const std::int16_t rawAccelX = readInt16LE(&data[19]);
		const std::int16_t rawAccelZ = readInt16LE(&data[21]);
		const std::int16_t rawAccelY = readInt16LE(&data[23]);

		// Standard DS4 gyro sensitivity: ~1024 LSB per rad/s (approx 16-17 LSB per deg/s)
		sample.pitchRate = static_cast<float>(rawPitch) / 1024.0f;
		sample.yawRate = static_cast<float>(rawYaw) / 1024.0f;
		sample.rollRate = static_cast<float>(rawRoll) / 1024.0f;

		// Standard DS4 accelerometer sensitivity: 8192 LSB per 1g
		sample.accelX = static_cast<float>(rawAccelX) / 8192.0f;
		sample.accelY = static_cast<float>(rawAccelY) / 8192.0f;
		sample.accelZ = static_cast<float>(rawAccelZ) / 8192.0f;

		outSample = sample;
		return true;
	}
	else if (reportId == 0x11)
	{
		// Bluetooth Input Report 0x11 (minimum 78 bytes)
		if (size < 78)
			return false;

		// Face buttons & D-pad in byte 7
		const std::uint8_t b7 = data[7];
		sample.dpad = b7 & 0x0F;
		sample.buttonSquare = (b7 & 0x10) != 0;
		sample.buttonCross = (b7 & 0x20) != 0;
		sample.buttonCircle = (b7 & 0x40) != 0;
		sample.buttonTriangle = (b7 & 0x80) != 0;

		// Shoulders, Options, Share in byte 8
		const std::uint8_t b8 = data[8];
		sample.buttonL1 = (b8 & 0x01) != 0;
		sample.buttonR1 = (b8 & 0x02) != 0;
		sample.buttonL2 = (b8 & 0x04) != 0;
		sample.buttonR2 = (b8 & 0x08) != 0;
		sample.buttonShare = (b8 & 0x10) != 0;
		sample.buttonOptions = (b8 & 0x20) != 0;
		sample.buttonL3 = (b8 & 0x40) != 0;
		sample.buttonR3 = (b8 & 0x80) != 0;

		// PS, Touchpad click in byte 9
		const std::uint8_t b9 = data[9];
		sample.buttonPS = (b9 & 0x01) != 0;
		sample.buttonTouchpad = (b9 & 0x02) != 0;

		// Gyroscope data at bytes 15-20 (int16_t little-endian)
		const std::int16_t rawPitch = readInt16LE(&data[15]);
		const std::int16_t rawYaw = readInt16LE(&data[17]);
		const std::int16_t rawRoll = readInt16LE(&data[19]);

		// Accelerometer data at bytes 21-26 (int16_t little-endian)
		const std::int16_t rawAccelX = readInt16LE(&data[21]);
		const std::int16_t rawAccelZ = readInt16LE(&data[23]);
		const std::int16_t rawAccelY = readInt16LE(&data[25]);

		sample.pitchRate = static_cast<float>(rawPitch) / 1024.0f;
		sample.yawRate = static_cast<float>(rawYaw) / 1024.0f;
		sample.rollRate = static_cast<float>(rawRoll) / 1024.0f;

		sample.accelX = static_cast<float>(rawAccelX) / 8192.0f;
		sample.accelY = static_cast<float>(rawAccelY) / 8192.0f;
		sample.accelZ = static_cast<float>(rawAccelZ) / 8192.0f;

		outSample = sample;
		return true;
	}

	return false;
}
