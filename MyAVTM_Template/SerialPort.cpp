#include "SerialPort.h"
#include <algorithm>
#include <array>
#include <cstdio>

SerialPort::~SerialPort()
{
	close();
}

bool SerialPort::open(const std::string& portName, unsigned int baudRate, std::string& errorMessage)
{
	close();
#ifdef _WIN32
	std::string deviceName = portName;
	if (deviceName.rfind("\\\\.\\", 0) != 0)
		deviceName = "\\\\.\\" + deviceName;

	m_handle = CreateFileA(deviceName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL, nullptr);
	if (m_handle == INVALID_HANDLE_VALUE)
	{
		errorMessage = "Unable to open " + portName + " (Win32 error " + std::to_string(GetLastError()) + ").";
		m_statusMessage = errorMessage;
		return false;
	}

	DCB settings{};
	settings.DCBlength = sizeof(settings);
	if (!GetCommState(m_handle, &settings))
	{
		errorMessage = "Unable to read serial settings (Win32 error " + std::to_string(GetLastError()) + ").";
		close();
		m_statusMessage = errorMessage;
		return false;
	}

	settings.BaudRate = baudRate;
	settings.ByteSize = 8;
	settings.Parity = NOPARITY;
	settings.StopBits = ONESTOPBIT;
	settings.fBinary = TRUE;
	settings.fParity = FALSE;
	settings.fOutxCtsFlow = FALSE;
	settings.fOutxDsrFlow = FALSE;
	settings.fDtrControl = DTR_CONTROL_ENABLE;
	settings.fDsrSensitivity = FALSE;
	settings.fOutX = FALSE;
	settings.fInX = FALSE;
	settings.fRtsControl = RTS_CONTROL_ENABLE;

	if (!SetCommState(m_handle, &settings))
	{
		errorMessage = "Unable to configure serial port (Win32 error " + std::to_string(GetLastError()) + ").";
		close();
		m_statusMessage = errorMessage;
		return false;
	}

	COMMTIMEOUTS timeouts{};
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	if (!SetCommTimeouts(m_handle, &timeouts) || !SetupComm(m_handle, 4096, 4096))
	{
		errorMessage = "Unable to configure serial buffering/timeouts (Win32 error " + std::to_string(GetLastError()) + ").";
		close();
		m_statusMessage = errorMessage;
		return false;
	}

	PurgeComm(m_handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
	m_connected = true;
	m_statusMessage = "Connected to " + portName + " at " + std::to_string(baudRate) + " baud";
	errorMessage.clear();
	return true;
#else
	errorMessage = "Arduino serial input is supported only on Windows.";
	m_statusMessage = errorMessage;
	return false;
#endif
}

void SerialPort::close()
{
#ifdef _WIN32
	if (m_handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_handle);
		m_handle = INVALID_HANDLE_VALUE;
	}
#endif
	m_connected = false;
	m_discardUntilNewline = false;
	m_lineLength = 0;
	m_hasReceivedSample = false;
	m_statusMessage = "Disconnected";
}

void SerialPort::poll(std::vector<AccelerometerSample>& samples)
{
#ifdef _WIN32
	if (!m_connected || m_handle == INVALID_HANDLE_VALUE)
		return;

	DWORD errors = 0;
	COMSTAT status{};
	if (!ClearCommError(m_handle, &errors, &status))
	{
		const std::string message = "Serial connection lost (Win32 error " + std::to_string(GetLastError()) + ").";
		close();
		m_statusMessage = message;
		return;
	}

	std::array<char, 256> input{};
	while (status.cbInQue > 0)
	{
		DWORD bytesRead = 0;
		const DWORD requested = std::min<DWORD>(status.cbInQue, static_cast<DWORD>(input.size()));
		if (!ReadFile(m_handle, input.data(), requested, &bytesRead, nullptr))
		{
			const std::string message = "Serial read failed (Win32 error " + std::to_string(GetLastError()) + ").";
			close();
			m_statusMessage = message;
			return;
		}
		if (bytesRead == 0)
			break;

		for (DWORD i = 0; i < bytesRead; ++i)
			processByte(input[i], samples);

		if (!ClearCommError(m_handle, &errors, &status))
		{
			const std::string message = "Serial connection lost (Win32 error " + std::to_string(GetLastError()) + ").";
			close();
			m_statusMessage = message;
			return;
		}
	}

	if (errors != 0)
	{
		++m_errorCount;
		ClearCommError(m_handle, &errors, &status);
	}
	if (m_hasReceivedSample && !hasRecentSample(std::chrono::milliseconds(1000)))
		m_statusMessage = "Connected; accelerometer data is stale.";
#endif
}

bool SerialPort::hasRecentSample(std::chrono::milliseconds timeout) const
{
	return m_connected && m_hasReceivedSample &&
		std::chrono::steady_clock::now() - m_lastSampleTime <= timeout;
}

long long SerialPort::millisecondsSinceLastSample() const
{
	if (!m_hasReceivedSample)
		return -1;
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - m_lastSampleTime).count();
}

void SerialPort::processByte(char value, std::vector<AccelerometerSample>& samples)
{
	if (value == '\n')
	{
		if (!m_discardUntilNewline && m_lineLength > 0)
		{
			AccelerometerSample sample;
			if (AccelerometerProtocol::parseLine(std::string_view(m_lineBuffer.data(), m_lineLength), sample))
			{
				if (samples.size() < 256)
				{
					samples.push_back(sample);
					m_lastSampleTime = std::chrono::steady_clock::now();
					m_hasReceivedSample = true;
					m_statusMessage = "Receiving accelerometer data";
				}
				else
				{
					++m_errorCount;
					m_statusMessage = "Accelerometer sample batch limit reached.";
				}
			}
			else
			{
				++m_errorCount;
				m_statusMessage = "Ignoring malformed accelerometer line.";
			}
		}
		m_lineLength = 0;
		m_discardUntilNewline = false;
		return;
	}

	if (m_discardUntilNewline)
		return;
	if (m_lineLength >= m_lineBuffer.size() - 1)
	{
		++m_errorCount;
		m_statusMessage = "Discarding overlong accelerometer line.";
		m_lineLength = 0;
		m_discardUntilNewline = true;
		return;
	}
	m_lineBuffer[m_lineLength++] = value;
}
