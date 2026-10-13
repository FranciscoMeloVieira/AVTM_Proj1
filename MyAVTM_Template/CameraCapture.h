#pragma once

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <thread>
#include <mutex>
#include <atomic>

class CameraCapture
{
public:
    CameraCapture();
    ~CameraCapture();

    bool start(int cameraIndex = 0, int width = 640, int height = 480);
    void stop();

    bool isOpened() const { return m_isOpened.load(); }
    bool getLatestFrame(cv::Mat& outFrame);
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    void captureLoop();

    cv::VideoCapture m_cap;
    std::thread m_captureThread;
    std::mutex m_frameMutex;
    cv::Mat m_latestFrame;
    std::atomic<bool> m_running{ false };
    std::atomic<bool> m_isOpened{ false };
    std::atomic<bool> m_hasNewFrame{ false };
    int m_width = 640;
    int m_height = 480;
    int m_cameraIndex = 0;
};
