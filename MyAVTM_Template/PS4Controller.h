#pragma once

#include "PS4Parser.h"
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

class PS4Controller
{
public:
	PS4Controller();
	~PS4Controller();

	PS4Controller(const PS4Controller&) = delete;
	PS4Controller& operator=(const PS4Controller&) = delete;

	bool open(std::string& outErrorMessage);
	void close();
	void poll(std::vector<PS4ControllerSample>& outSamples);

	bool isConnected() const { return m_connected.load(); }
	bool hasRecentSample(std::chrono::milliseconds timeout) const;
	long long millisecondsSinceLastSample() const;
	std::size_t getErrorCount() const { return m_errorCount; }
	const std::string& getStatusMessage() const { return m_statusMessage; }

	// Mock injection for unit testing without physical controller hardware
	void injectSample(const PS4ControllerSample& sample);
	void setSimulatedConnected(bool connected);

private:
#ifdef _WIN32
	HANDLE m_deviceHandle = INVALID_HANDLE_VALUE;
	std::thread m_readerThread;
	std::atomic<bool> m_stopThread{ false };
	void readerThreadFunc();
#endif

	std::atomic<bool> m_connected{ false };
	mutable std::mutex m_sampleMutex;
	std::vector<PS4ControllerSample> m_sampleQueue;
	std::chrono::steady_clock::time_point m_lastSampleTime{};
	bool m_hasReceivedSample = false;
	std::string m_statusMessage = "PS4 controller disconnected";
	std::size_t m_errorCount = 0;
};
