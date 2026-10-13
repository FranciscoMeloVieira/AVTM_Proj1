#pragma once

#include "AccelerometerInput.h"
#include <array>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

class SerialPort
{
public:
	SerialPort() = default;
	~SerialPort();

	SerialPort(const SerialPort&) = delete;
	SerialPort& operator=(const SerialPort&) = delete;

	bool open(const std::string& portName, unsigned int baudRate, std::string& errorMessage);
	void close();
	void poll(std::vector<AccelerometerSample>& samples);

	bool isConnected() const { return m_connected; }
	bool hasRecentSample(std::chrono::milliseconds timeout) const;
	long long millisecondsSinceLastSample() const;
	std::size_t getErrorCount() const { return m_errorCount; }
	const std::string& getStatusMessage() const { return m_statusMessage; }

private:
#ifdef _WIN32
	HANDLE m_handle = INVALID_HANDLE_VALUE;
#endif
	bool m_connected = false;
	bool m_discardUntilNewline = false;
	std::array<char, 129> m_lineBuffer{};
	std::size_t m_lineLength = 0;
	std::size_t m_errorCount = 0;
	std::chrono::steady_clock::time_point m_lastSampleTime{};
	bool m_hasReceivedSample = false;
	std::string m_statusMessage = "Disconnected";

	void processByte(char value, std::vector<AccelerometerSample>& samples);
};
