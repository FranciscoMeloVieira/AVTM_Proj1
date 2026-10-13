#include "CameraCapture.h"
#include <iostream>
#include <chrono>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objbase.h>

CameraCapture::CameraCapture()
{
}

CameraCapture::~CameraCapture()
{
    stop();
}

bool CameraCapture::start(int cameraIndex, int width, int height)
{
    stop();

    m_cameraIndex = cameraIndex;
    m_width = width;
    m_height = height;
    m_isOpened.store(false);
    m_running.store(true);

    m_captureThread = std::thread(&CameraCapture::captureLoop, this);

    // Wait briefly up to 1 second for thread to attempt opening camera
    for (int i = 0; i < 20; ++i)
    {
        if (m_isOpened.load())
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return m_isOpened.load();
}

void CameraCapture::stop()
{
    if (m_running.load())
    {
        m_running.store(false);
        if (m_captureThread.joinable())
        {
            m_captureThread.join();
        }
    }
    m_isOpened.store(false);
}

void CameraCapture::captureLoop()
{
    // Initialize COM for DirectShow / Media Foundation on this worker thread
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    try
    {
        std::cout << "[CameraCapture] Opening camera " << m_cameraIndex << " via CAP_DSHOW..." << std::endl;
        bool opened = m_cap.open(m_cameraIndex, cv::CAP_DSHOW);
        if (!opened)
        {
            std::cout << "[CameraCapture] CAP_DSHOW not available, trying CAP_MSMF..." << std::endl;
            opened = m_cap.open(m_cameraIndex, cv::CAP_MSMF);
        }
        if (!opened)
        {
            std::cout << "[CameraCapture] CAP_MSMF not available, trying CAP_ANY..." << std::endl;
            opened = m_cap.open(m_cameraIndex, cv::CAP_ANY);
        }

        if (opened && m_cap.isOpened())
        {
            m_cap.set(cv::CAP_PROP_FRAME_WIDTH, m_width);
            m_cap.set(cv::CAP_PROP_FRAME_HEIGHT, m_height);
            m_cap.set(cv::CAP_PROP_FPS, 30);
            m_isOpened.store(true);
            std::cout << "[CameraCapture] Camera " << m_cameraIndex << " opened successfully." << std::endl;
        }
        else
        {
            std::cerr << "[CameraCapture] Note: Camera " << m_cameraIndex
                      << " not accessible. Finger tracking offline (mouse editor & AI paths remain active)." << std::endl;
            m_isOpened.store(false);
        }

        while (m_running.load() && m_isOpened.load())
        {
            cv::Mat frame;
            if (m_cap.read(frame) && !frame.empty())
            {
                std::lock_guard<std::mutex> lock(m_frameMutex);
                m_latestFrame = frame;
                m_hasNewFrame.store(true);
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }

        if (m_cap.isOpened())
        {
            m_cap.release();
            std::cout << "[CameraCapture] Camera released cleanly." << std::endl;
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "[CameraCapture] Exception opening camera: " << ex.what() << std::endl;
        m_isOpened.store(false);
    }
    catch (...)
    {
        std::cerr << "[CameraCapture] Unknown exception opening camera." << std::endl;
        m_isOpened.store(false);
    }

    if (SUCCEEDED(hr))
    {
        CoUninitialize();
    }
}

bool CameraCapture::getLatestFrame(cv::Mat& outFrame)
{
    std::lock_guard<std::mutex> lock(m_frameMutex);
    if (m_latestFrame.empty())
    {
        return false;
    }
    outFrame = m_latestFrame.clone();
    m_hasNewFrame.store(false);
    return true;
}
