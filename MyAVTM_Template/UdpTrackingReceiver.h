#pragma once

// WinSock2 UDP receiver for MediaPipe hand tracking data sent by finger_tracker.py
// Runs a background thread; main thread calls poll() every frame.

#include <string>
#include <atomic>
#include <mutex>
#include <thread>

// -------------------------------------------------------------------------
// Shared tracking state produced by the UDP receiver
// -------------------------------------------------------------------------
struct UdpTrackingState
{
    bool  detected   = false;   // Hand visible
    bool  pinching   = false;   // Stable pinch gesture active
    float normX      = 0.5f;   // Normalised [0,1] horizontal position
    float normY      = 0.5f;   // Normalised [0,1] vertical position
    float pinchDist  = 1.0f;   // Raw landmark distance (debug)
};

// Commands forwarded from the Python hotkeys
enum class UdpCommand
{
    None,
    Confirm,   // C / Enter
    Clear,     // X
    Loop       // L
};

class UdpTrackingReceiver
{
public:
    static constexpr int  DEFAULT_PORT    = 9099;
    static constexpr float TIMEOUT_SEC   = 2.0f;  // Seconds before "no signal"

    explicit UdpTrackingReceiver(int port = DEFAULT_PORT);
    ~UdpTrackingReceiver();

    // Start / stop the background receive thread
    bool start();
    void stop();

    bool isRunning()   const { return m_running.load(); }

    // Returns true when UDP packets have arrived within TIMEOUT_SEC
    bool hasSignal()   const;

    // Thread-safe snapshot of the latest state
    UdpTrackingState getState() const;

    // Consume one pending command (returns None if queue empty); call every frame
    UdpCommand       popCommand();

    // Diagnostics
    int  getTotalPackets()  const { return m_totalPackets.load(); }
    int  getPort()          const { return m_port; }

private:
    void receiverThread();
    void parsePacket(const char* buf, int len);

    int  m_port;
    int  m_sock  = -1;   // SOCKET (cast for portability)

    std::thread        m_thread;
    std::atomic<bool>  m_running  { false };
    std::atomic<int>   m_totalPackets { 0 };

    mutable std::mutex    m_mutex;
    UdpTrackingState      m_state;
    UdpCommand            m_pendingCmd { UdpCommand::None };
    double                m_lastPacketTime = 0.0;
};
