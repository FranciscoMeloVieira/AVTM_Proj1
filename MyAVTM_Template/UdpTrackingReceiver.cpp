// UdpTrackingReceiver.cpp
// Receives MediaPipe hand-tracking UDP packets from finger_tracker.py
// Protocol:
//   Data  : "DET,PINCH,X,Y,DIST\n"  (where DET/PINCH are 0 or 1)
//   Command: "CMD:CONFIRM\n" | "CMD:CLEAR\n" | "CMD:LOOP\n"

#include "UdpTrackingReceiver.h"

// ---- Windows networking --------------------------------------------------
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
// --------------------------------------------------------------------------

#include <iostream>
#include <cstring>
#include <cstdio>
#include <chrono>

static double nowSec()
{
    using namespace std::chrono;
    return duration_cast<duration<double>>(
        steady_clock::now().time_since_epoch()).count();
}

// -------------------------------------------------------------------------
UdpTrackingReceiver::UdpTrackingReceiver(int port)
    : m_port(port)
{
}

UdpTrackingReceiver::~UdpTrackingReceiver()
{
    stop();
}

// -------------------------------------------------------------------------
bool UdpTrackingReceiver::start()
{
    if (m_running.load())
        return true;

    // Init WinSock
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::cerr << "[UDP] WSAStartup failed: " << WSAGetLastError() << std::endl;
        return false;
    }

    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET)
    {
        std::cerr << "[UDP] socket() failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    // Non-blocking
    u_long nb = 1;
    ioctlsocket(s, FIONBIO, &nb);

    // Allow socket reuse
    int reuse = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(static_cast<u_short>(m_port));

    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
    {
        std::cerr << "[UDP] bind() failed on port " << m_port
                  << ": " << WSAGetLastError() << std::endl;
        closesocket(s);
        WSACleanup();
        return false;
    }

    m_sock = static_cast<int>(s);
    m_running.store(true);

    m_thread = std::thread(&UdpTrackingReceiver::receiverThread, this);
    std::cout << "[UDP] Listening on port " << m_port
              << " for MediaPipe hand-tracking data..." << std::endl;
    return true;
}

// -------------------------------------------------------------------------
void UdpTrackingReceiver::stop()
{
    if (!m_running.load())
        return;

    m_running.store(false);

    if (m_sock >= 0)
    {
        closesocket(static_cast<SOCKET>(m_sock));
        m_sock = -1;
    }

    if (m_thread.joinable())
        m_thread.join();

    WSACleanup();
    std::cout << "[UDP] Receiver stopped." << std::endl;
}

// -------------------------------------------------------------------------
bool UdpTrackingReceiver::hasSignal() const
{
    std::lock_guard<std::mutex> lk(m_mutex);
    return (nowSec() - m_lastPacketTime) < TIMEOUT_SEC;
}

// -------------------------------------------------------------------------
UdpTrackingState UdpTrackingReceiver::getState() const
{
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_state;
}

// -------------------------------------------------------------------------
UdpCommand UdpTrackingReceiver::popCommand()
{
    std::lock_guard<std::mutex> lk(m_mutex);
    UdpCommand cmd = m_pendingCmd;
    m_pendingCmd = UdpCommand::None;
    return cmd;
}

// -------------------------------------------------------------------------
void UdpTrackingReceiver::receiverThread()
{
    static constexpr int BUFSIZE = 256;
    char buf[BUFSIZE];

    while (m_running.load())
    {
        sockaddr_in sender{};
        int senderLen = sizeof(sender);

        int n = recvfrom(static_cast<SOCKET>(m_sock), buf, BUFSIZE - 1, 0,
                         reinterpret_cast<sockaddr*>(&sender), &senderLen);

        if (n > 0)
        {
            buf[n] = '\0';
            parsePacket(buf, n);
            m_totalPackets.fetch_add(1);
        }
        else
        {
            // Non-blocking: WSAEWOULDBLOCK is expected when no data; sleep briefly
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK || err == 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            else if (err != WSAECONNRESET)
            {
                // Fatal error or socket closed
                break;
            }
        }
    }
}

// -------------------------------------------------------------------------
void UdpTrackingReceiver::parsePacket(const char* buf, int /*len*/)
{
    // CMD packets
    if (strncmp(buf, "CMD:", 4) == 0)
    {
        UdpCommand cmd = UdpCommand::None;
        if (strncmp(buf + 4, "CONFIRM", 7) == 0)
            cmd = UdpCommand::Confirm;
        else if (strncmp(buf + 4, "CLEAR", 5) == 0)
            cmd = UdpCommand::Clear;
        else if (strncmp(buf + 4, "LOOP", 4) == 0)
            cmd = UdpCommand::Loop;

        if (cmd != UdpCommand::None)
        {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_pendingCmd = cmd;
            m_lastPacketTime = nowSec();
        }
        return;
    }

    // Data packet: "DET,PINCH,X,Y,DIST\n"
    int det = 0, pinch = 0;
    float x = 0.5f, y = 0.5f, dist = 1.0f;
    int parsed = sscanf(buf, "%d,%d,%f,%f,%f", &det, &pinch, &x, &y, &dist);
    if (parsed >= 4)
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_state.detected   = (det != 0);
        m_state.pinching   = (pinch != 0);
        m_state.normX      = x;
        m_state.normY      = y;
        m_state.pinchDist  = (parsed >= 5) ? dist : 1.0f;
        m_lastPacketTime   = nowSec();
    }
}
