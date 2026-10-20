#include "PS4Controller.h"
#include <algorithm>
#include <cstdlib>

#ifdef _WIN32
#include <setupapi.h>
#include <hidsdi.h>
#include <hidpi.h>
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")
#endif

PS4Controller::PS4Controller() = default;

PS4Controller::~PS4Controller()
{
	close();
}

bool PS4Controller::open(std::string& outErrorMessage)
{
	close();

#ifdef _WIN32
	GUID hidGuid;
	HidD_GetHidGuid(&hidGuid);

	HDEVINFO deviceInfoSet = SetupDiGetClassDevsA(&hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (deviceInfoSet == INVALID_HANDLE_VALUE)
	{
		outErrorMessage = "Unable to query Windows HID devices (Win32 error " + std::to_string(GetLastError()) + ").";
		m_statusMessage = outErrorMessage;
		return false;
	}

	unsigned short targetVid = 0x054C; // Sony Interactive Entertainment
	if (const char* envVid = std::getenv("AVTM_DS4_VID"))
	{
		char* end = nullptr;
		const unsigned long v = std::strtoul(envVid, &end, 16);
		if (end != envVid)
			targetVid = static_cast<unsigned short>(v);
	}

	bool overridePid = false;
	unsigned short targetPid = 0;
	if (const char* envPid = std::getenv("AVTM_DS4_PID"))
	{
		char* end = nullptr;
		const unsigned long p = std::strtoul(envPid, &end, 16);
		if (end != envPid)
		{
			targetPid = static_cast<unsigned short>(p);
			overridePid = true;
		}
	}

	SP_DEVICE_INTERFACE_DATA interfaceData{};
	interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	HANDLE foundHandle = INVALID_HANDLE_VALUE;
	std::string matchedPath;

	for (DWORD index = 0; SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr, &hidGuid, index, &interfaceData); ++index)
	{
		DWORD detailSize = 0;
		SetupDiGetDeviceInterfaceDetailA(deviceInfoSet, &interfaceData, nullptr, 0, &detailSize, nullptr);
		if (detailSize == 0)
			continue;

		std::vector<BYTE> detailBuffer(detailSize);
		auto detailData = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_A>(detailBuffer.data());
		detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

		if (!SetupDiGetDeviceInterfaceDetailA(deviceInfoSet, &interfaceData, detailData, detailSize, nullptr, nullptr))
			continue;

		// Attempt open with read/write shared access
		HANDLE handle = CreateFileA(detailData->DevicePath,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr);

		if (handle == INVALID_HANDLE_VALUE)
		{
			// Fall back to read-only access
			handle = CreateFileA(detailData->DevicePath,
				GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				nullptr);
		}

		if (handle == INVALID_HANDLE_VALUE)
			continue;

		HIDD_ATTRIBUTES attrib{};
		attrib.Size = sizeof(HIDD_ATTRIBUTES);
		if (HidD_GetAttributes(handle, &attrib))
		{
			bool matches = (attrib.VendorID == targetVid);
			if (overridePid)
			{
				matches = matches && (attrib.ProductID == targetPid);
			}
			else
			{
				// Default DS4 Product IDs: 0x05C4 (v1), 0x09CC (v2), 0x0BA0 (Wireless Adapter)
				matches = matches && (attrib.ProductID == 0x05C4 || attrib.ProductID == 0x09CC ||
					attrib.ProductID == 0x0BA0 || attrib.ProductID == 0x081F);
			}

			if (matches)
			{
				foundHandle = handle;
				matchedPath = detailData->DevicePath;
				break;
			}
		}

		CloseHandle(handle);
	}

	SetupDiDestroyDeviceInfoList(deviceInfoSet);

	if (foundHandle == INVALID_HANDLE_VALUE)
	{
		outErrorMessage = "No Sony DualShock 4 controller detected via USB or Bluetooth.";
		m_statusMessage = outErrorMessage;
		return false;
	}

	m_deviceHandle = foundHandle;
	m_stopThread = false;
	m_connected = true;
	m_hasReceivedSample = false;
	m_statusMessage = "Connected to PS4 DualShock 4";
	outErrorMessage.clear();

	m_readerThread = std::thread(&PS4Controller::readerThreadFunc, this);
	return true;
#else
	outErrorMessage = "PS4 controller HID input is supported on Windows.";
	m_statusMessage = outErrorMessage;
	return false;
#endif
}

void PS4Controller::close()
{
#ifdef _WIN32
	m_stopThread = true;
	m_connected = false;

	if (m_deviceHandle != INVALID_HANDLE_VALUE)
	{
		CancelIoEx(m_deviceHandle, nullptr);
		CloseHandle(m_deviceHandle);
		m_deviceHandle = INVALID_HANDLE_VALUE;
	}

	if (m_readerThread.joinable())
	{
		m_readerThread.join();
	}
#endif

	m_connected = false;
	{
		std::lock_guard<std::mutex> lock(m_sampleMutex);
		m_sampleQueue.clear();
	}
	m_statusMessage = "PS4 controller disconnected";
}

#ifdef _WIN32
void PS4Controller::readerThreadFunc()
{
	std::vector<std::uint8_t> buffer(128);

	while (!m_stopThread && m_connected)
	{
		DWORD bytesRead = 0;
		if (!ReadFile(m_deviceHandle, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr))
		{
			const DWORD error = GetLastError();
			if (error == ERROR_DEVICE_NOT_CONNECTED || error == ERROR_INVALID_HANDLE ||
				error == ERROR_OPERATION_ABORTED || error == ERROR_GEN_FAILURE)
			{
				m_connected = false;
				m_statusMessage = "PS4 controller connection lost (Win32 error " + std::to_string(error) + ").";
				break;
			}
			++m_errorCount;
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
			continue;
		}

		if (bytesRead > 0)
		{
			PS4ControllerSample sample;
			if (PS4Parser::parseReport(buffer.data(), bytesRead, sample))
			{
				sample.timestampMilliseconds = static_cast<std::uint32_t>(GetTickCount());
				std::lock_guard<std::mutex> lock(m_sampleMutex);
				if (m_sampleQueue.size() < 512)
				{
					m_sampleQueue.push_back(sample);
					m_lastSampleTime = std::chrono::steady_clock::now();
					m_hasReceivedSample = true;
					m_statusMessage = "Receiving PS4 gyro data";
				}
				else
				{
					++m_errorCount;
				}
			}
		}
	}
}
#endif

void PS4Controller::poll(std::vector<PS4ControllerSample>& outSamples)
{
	{
		std::lock_guard<std::mutex> lock(m_sampleMutex);
		if (!m_sampleQueue.empty())
		{
			outSamples.insert(outSamples.end(), m_sampleQueue.begin(), m_sampleQueue.end());
			m_sampleQueue.clear();
		}
	}

	if (!m_connected && m_hasReceivedSample)
	{
		m_statusMessage = "PS4 controller disconnected";
	}
	else if (m_hasReceivedSample && !hasRecentSample(std::chrono::milliseconds(1000)))
	{
		m_statusMessage = "Connected; PS4 gyro data is stale";
	}
}

bool PS4Controller::hasRecentSample(std::chrono::milliseconds timeout) const
{
	return m_connected && m_hasReceivedSample &&
		(std::chrono::steady_clock::now() - m_lastSampleTime <= timeout);
}

long long PS4Controller::millisecondsSinceLastSample() const
{
	if (!m_hasReceivedSample)
		return -1;
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - m_lastSampleTime).count();
}

void PS4Controller::injectSample(const PS4ControllerSample& sample)
{
	std::lock_guard<std::mutex> lock(m_sampleMutex);
	m_sampleQueue.push_back(sample);
	m_lastSampleTime = std::chrono::steady_clock::now();
	m_hasReceivedSample = true;
	m_connected = true;
}

void PS4Controller::setSimulatedConnected(bool connected)
{
	m_connected = connected;
	if (!connected)
		m_statusMessage = "PS4 controller disconnected";
}
