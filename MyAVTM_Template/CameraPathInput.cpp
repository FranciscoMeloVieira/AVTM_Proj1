#include "CameraPathInput.h"
#include "GameManager.h"
#include "PathEditor.h"
#include "PathVisualizer.h"
#include <GL/freeglut.h>
#include <iostream>
#include <iomanip>

CameraPathInput::CameraPathInput(GameManager* gameManager, PathEditor* editor, PathVisualizer* visualizer)
    : m_gameManager(gameManager), m_editor(editor), m_visualizer(visualizer)
{
}

CameraPathInput::~CameraPathInput()
{
    stop();
}

bool CameraPathInput::start(int cameraIndex)
{
    bool success = m_capture.start(cameraIndex, 640, 480);
    if (success)
    {
        cv::namedWindow("Finger Path Drawing", cv::WINDOW_AUTOSIZE);
        std::cout << "[CameraPathInput] OpenCV window 'Finger Path Drawing' created." << std::endl;
        std::cout << "[CameraPathInput] Instructions:" << std::endl;
        std::cout << "  - Pinch index + thumb to DRAW path" << std::endl;
        std::cout << "  - Release pinch to HOVER / PREVIEW" << std::endl;
        std::cout << "  - Press [C] or [Enter] to CONFIRM path and start AI driving" << std::endl;
        std::cout << "  - Press [X] to CLEAR and redraw" << std::endl;
        std::cout << "  - Press [L] to toggle CLOSED LOOP" << std::endl;
    }
    return success;
}

void CameraPathInput::stop()
{
    m_capture.stop();
    try
    {
        cv::destroyWindow("Finger Path Drawing");
    }
    catch (...)
    {
    }
}

void CameraPathInput::update(float deltaTime, int winW, int winH)
{
    if (!m_capture.isOpened())
    {
        return;
    }

    cv::Mat rawFrame;
    bool hasFrame = m_capture.getLatestFrame(rawFrame);

    cv::Mat debugFrame;
    if (hasFrame && !rawFrame.empty())
    {
        m_tracker.process(rawFrame, m_lastResult, debugFrame);
    }

    if (debugFrame.empty())
    {
        return;
    }

    // Camera 1 top-down orthographic mapping:
    // orthoViewSize = 200.0f, cameraX = 150.0f, cameraZ = 0.0f, planeY = 0.0f
    float aspect = (winH > 0) ? (static_cast<float>(winW) / static_cast<float>(winH)) : (4.0f / 3.0f);
    float ndcX = (2.0f * m_lastResult.normalizedPos.x - 1.0f) * aspect;
    float ndcZ = (2.0f * m_lastResult.normalizedPos.y - 1.0f);

    float worldX = 150.0f + ndcX * 200.0f;
    float worldZ = 0.0f + ndcZ * 200.0f;
    m_lastWorldPos = Vector3(worldX, 0.0f, worldZ);

    double timeSec = glutGet(GLUT_ELAPSED_TIME) / 1000.0;

    // Handle finger tracking & drawing
    if (m_lastResult.detected)
    {
        if (m_visualizer)
        {
            m_visualizer->setCursorPosition(m_lastWorldPos, m_lastResult.isPinching);
        }

        if (m_lastResult.isPinching)
        {
            // Auto-enter edit mode if currently idle
            if (m_editor && m_editor->getState() == EditorState::Idle && m_gameManager)
            {
                m_gameManager->enterPathEditor();
            }

            if (!m_activeDrawing)
            {
                if (m_editor)
                {
                    m_editor->beginStroke(m_lastWorldPos, timeSec);
                    m_tracker.clearTrail();
                    m_tracker.addTrailPoint(m_lastResult.pinchPoint);
                }
                m_activeDrawing = true;
                std::cout << "[CameraPathInput] Pinch started at (" << std::fixed << std::setprecision(1)
                          << worldX << ", " << worldZ << "). Recording path samples..." << std::endl;
            }
            else
            {
                if (m_editor)
                {
                    m_editor->addSample(m_lastWorldPos, timeSec);
                    m_tracker.addTrailPoint(m_lastResult.pinchPoint);
                    if (m_visualizer)
                    {
                        m_visualizer->setRawSamples(m_editor->getRawSamples());
                    }
                }
            }
        }
        else // Not pinching
        {
            if (m_activeDrawing)
            {
                if (m_editor)
                {
                    m_editor->endStroke();
                    std::cout << "[CameraPathInput] Pinch released! Stroke complete." << std::endl;
                    std::cout << "  - Raw samples recorded: " << m_editor->getRawSamples().size() << std::endl;
                    std::cout << "  - Processed path points: " << m_editor->getProcessedPath().points.size() << std::endl;
                    std::cout << "  - Total length: " << m_editor->getProcessedPath().totalLength << "m" << std::endl;
                    if (m_visualizer)
                    {
                        m_visualizer->setRawSamples(m_editor->getRawSamples());
                        m_visualizer->setPath(m_editor->getProcessedPath());
                    }
                }
                m_activeDrawing = false;
            }
        }
    }
    else // Hand lost / out of view
    {
        if (m_activeDrawing)
        {
            if (m_editor)
            {
                m_editor->endStroke();
                std::cout << "[CameraPathInput] Hand lost during drawing. Stroke ended safely." << std::endl;
                if (m_visualizer)
                {
                    m_visualizer->setRawSamples(m_editor->getRawSamples());
                    m_visualizer->setPath(m_editor->getProcessedPath());
                }
            }
            m_activeDrawing = false;
        }

        if (m_visualizer)
        {
            m_visualizer->hideCursor();
        }
    }

    // FPS computation & periodic console diagnostics
    m_frameCount++;
    m_fpsTimer += deltaTime;
    if (m_fpsTimer >= 1.0f)
    {
        m_currentFps = static_cast<float>(m_frameCount) / m_fpsTimer;
        m_frameCount = 0;
        m_fpsTimer = 0.0f;

        int rawCount = m_editor ? static_cast<int>(m_editor->getRawSamples().size()) : 0;
        int procCount = m_editor ? static_cast<int>(m_editor->getProcessedPath().points.size()) : 0;

        std::cout << "[Diagnostics] FPS: " << std::fixed << std::setprecision(1) << m_currentFps
                  << " | Hand: " << (m_lastResult.detected ? "DETECTED" : "NONE")
                  << " | State: " << (m_lastResult.isPinching ? "PINCH (DRAW)" : "HOVER (PREVIEW)")
                  << " | Raw samples: " << rawCount
                  << " | Processed pts: " << procCount
                  << " | Cursor: (" << std::setprecision(1) << worldX << ", " << worldZ << ")"
                  << std::endl;
    }

    // Render diagnostic statistics onto debug frame
    int rawCount = m_editor ? static_cast<int>(m_editor->getRawSamples().size()) : 0;
    std::string fpsStr = "FPS: " + std::to_string(static_cast<int>(m_currentFps));
    std::string ptsStr = "Samples: " + std::to_string(rawCount);
    cv::putText(debugFrame, fpsStr, cv::Point(debugFrame.cols - 130, 25),
                cv::FONT_HERSHEY_SIMPLEX, 0.55, cv::Scalar(0, 255, 0), 2);
    cv::putText(debugFrame, ptsStr, cv::Point(debugFrame.cols - 150, 50),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 240, 255), 1);

    try
    {
        // Show auxiliary OpenCV window
        cv::imshow("Finger Path Drawing", debugFrame);

        // Process OpenCV window keys so user can press hotkeys inside either window
        int cvKey = cv::pollKey();
        if (cvKey > 0 && m_gameManager)
        {
            switch (cvKey)
            {
            case 'c':
            case 'C':
            case 13: // Enter
                std::cout << "[CameraPathInput] Confirm key received." << std::endl;
                m_gameManager->confirmEditorPath();
                break;

            case 'x':
            case 'X':
                std::cout << "[CameraPathInput] Clear key received." << std::endl;
                m_gameManager->clearEditorPath();
                m_tracker.clearTrail();
                break;

            case 'l':
            case 'L':
                std::cout << "[CameraPathInput] Toggle closed loop key received." << std::endl;
                m_gameManager->toggleEditorClosedLoop();
                break;

            case 'e':
            case 'E':
                m_gameManager->togglePathEditor();
                break;

            default:
                break;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[CameraPathInput] GUI error: " << e.what() << std::endl;
    }
    catch (...)
    {
    }
}
