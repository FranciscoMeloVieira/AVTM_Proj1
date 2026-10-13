#pragma once

// CameraPathInput — Stage 3 finger-drawing interface.
//
// Primary mode  : Receives hand-tracking data via UDP from finger_tracker.py
//                 (MediaPipe neural-net tracker — best quality).
// Fallback mode : When no UDP signal is present, falls back to the old
//                 OpenCV skin-segmentation tracker (CameraCapture + FingerTracker).

#include "UdpTrackingReceiver.h"
#include "FingerTracker.h"   // fallback only
#include "CameraCapture.h"   // fallback only
#include "Vector3.h"
#include <string>

class PathEditor;
class PathVisualizer;
class GameManager;

class CameraPathInput
{
public:
    CameraPathInput(GameManager* gameManager, PathEditor* editor, PathVisualizer* visualizer);
    ~CameraPathInput();

    // cameraIndex only used if falling back to the OpenCV tracker
    bool start(int cameraIndex = 0);
    void stop();

    // Call every frame from GameManager::update()
    void update(float deltaTime, int winW, int winH);

    // Is this subsystem alive?
    bool isUdpActive()    const { return m_udpReceiver.hasSignal(); }
    bool isPinching()     const { return m_lastPinching; }
    bool isHandDetected() const { return m_lastDetected; }
    const Vector3& getLastWorldPos() const { return m_lastWorldPos; }

    // Clear the finger trail in fallback mode
    void clearTrail() { m_tracker.clearTrail(); }

private:
    void updateFromUdp    (float deltaTime, int winW, int winH);
    void updateFromOpenCV (float deltaTime, int winW, int winH);
    void applyTracking    (bool detected, bool pinching, float nx, float ny,
                           int winW, int winH, float deltaTime);
    void handleCommand    (UdpCommand cmd);

    GameManager*   m_gameManager = nullptr;
    PathEditor*    m_editor      = nullptr;
    PathVisualizer* m_visualizer = nullptr;

    // -- UDP (primary) ----------------------------------------
    UdpTrackingReceiver m_udpReceiver;
    bool m_udpStarted = false;

    // -- OpenCV (fallback) ------------------------------------
    CameraCapture  m_capture;
    FingerTracker  m_tracker;
    bool           m_fallbackStarted = false;

    // -- Shared state -----------------------------------------
    // Latest tracking result (from UDP primary or OpenCV fallback)
    FingerTrackingResult m_lastResult;

    bool    m_lastDetected  = false;
    bool    m_lastPinching  = false;
    bool    m_activeDrawing = false;
    Vector3 m_lastWorldPos  { 150.0f, 0.0f, 0.0f };

    // -- Diagnostics ------------------------------------------
    int   m_frameCount  = 0;
    float m_fpsTimer    = 0.0f;
    float m_currentFps  = 0.0f;
    float m_noSignalTimer = 0.0f;
};
