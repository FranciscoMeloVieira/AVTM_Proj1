#include "InputManager.hpp"
#include "GameManager.h"
#include "LightningSystem.hpp"
#include <GL/freeglut.h>
#include <cmath>
#include <cstdio>

void InputManager::onKeyDown(unsigned char key, int xx, int yy) {
    if (key == 'e' || key == 'E') {
        gameManager->togglePathEditor();
        if (gameManager->isEditingPath()) {
            activeCamera = camera1;
            activeCameraChanged = true;
        }
        return;
    }

    if (gameManager && gameManager->isEditingPath()) {
        switch (key) {
        case 'i':
        case 'I':
            gameManager->toggleArduinoConnection();
            return;
        case 'g':
        case 'G':
            gameManager->calibrateArduinoInput();
            return;
        case 'b':
        case 'B':
            gameManager->startArduinoRecording();
            return;
        case 'n':
        case 'N':
            gameManager->stopArduinoRecording();
            return;
        case 'o':
        case 'O':
            gameManager->resetArduinoCursor();
            return;
        case 13:   // Enter
        case ' ':  // Space
        case 'y':
        case 'Y':
            gameManager->confirmEditorPath();
            return;
        case 'c':
        case 'C':
            gameManager->clearEditorPath();
            return;
        case 'l':
        case 'L':
            gameManager->toggleEditorClosedLoop();
            return;
        case 's':
        case 'S':
            gameManager->toggleEditorSmoothing();
            return;
        case 'k':
        case 'K':
            gameManager->toggleTrackReference();
            return;
        default:
            break;
        }
    }

    switch (key) {

    case 27:
        glutLeaveMainLoop();
        break;

    case 'r':   // reset camera
        alpha = 57.0f; _beta = 18.0f;
        r = 45.0f;
        camX = r * sinf(alpha * 3.14f / 180.0f) * cosf(_beta * 3.14f / 180.0f);
        camZ = r * cosf(alpha * 3.14f / 180.0f) * cosf(_beta * 3.14f / 180.0f);
        camY = r * sinf(_beta * 3.14f / 180.0f);
        gameManager->resetGame();
        break;

    case 'w':
        if (gameManager->getControlMode() == ControlMode::WASD)
            gameManager->car->isAccelerating = true;
        break;
    case 's':
        if (gameManager->getControlMode() == ControlMode::WASD)
            gameManager->car->isDecelerating = true;
        break;
    case 'a':
        if (gameManager->getControlMode() == ControlMode::WASD)
            gameManager->car->steering = 1.0f;
        break;
    case 'd':
        if (gameManager->getControlMode() == ControlMode::WASD)
            gameManager->car->steering = -1.0f;
        break;

    case 'm':
    case 'M':
        gameManager->toggleControlMode();
        break;

    case 't':
    case 'T':
        gameManager->switchTestPath();
        break;

    case 'v':
    case 'V':
        gameManager->toggleDebugVisuals();
        break;

    case '1':
        activeCamera = camera1;
        activeCameraChanged = true;
        break;
    case '2':
        activeCamera = camera2;
        activeCameraChanged = true;
        break;
    case '3':
        activeCamera = camera3;
        activeCameraChanged = true;
        break;
    case 'p':
        gameManager->paused = !gameManager->paused;
        if (gameManager->hud)
            gameManager->hud->setPaused(gameManager->paused);
        break;

    case 'n':
        lightingSystem->toggleDirectionalLights();
        break;
    case 'h':
        lightingSystem->toggleSpotLights();
        break;
    case 'c':
        lightingSystem->togglePointlights();
        break;
    case 'b':
        gameManager->toggleRearMirror();
        break;
    case 'f':
        gameManager->toggleFog();
        break;
    }
}

void InputManager::onKeyUp(unsigned char key, int xx, int yy) {
    switch (key) {
    case 'a':
    case 'd':
        if (gameManager->getControlMode() == ControlMode::WASD)
            gameManager->car->steering = 0.0f;
        break;
    case 'w':
        if (gameManager->getControlMode() == ControlMode::WASD)
            gameManager->car->isAccelerating = false;
        break;
    case 's':
        if (gameManager->getControlMode() == ControlMode::WASD)
            gameManager->car->isDecelerating = false;
        break;
    }
}

void InputManager::onMouseButton(int button, int state, int xx, int yy)
{
    if (gameManager && gameManager->isEditingPath()) {
        if (gameManager->isArduinoRecording())
            return;
        int winW = glutGet(GLUT_WINDOW_WIDTH);
        int winH = glutGet(GLUT_WINDOW_HEIGHT);
        if (button == GLUT_LEFT_BUTTON) {
            if (state == GLUT_DOWN)
                gameManager->onEditorMouseDown(xx, yy, winW, winH);
            else if (state == GLUT_UP)
                gameManager->onEditorMouseUp(xx, yy, winW, winH);
        }
        return;
    }

    // start tracking the mouse
    if (state == GLUT_DOWN) {
        startX = xx;
        startY = yy;
        if (button == GLUT_LEFT_BUTTON)
            tracking = 1;
        else if (button == GLUT_RIGHT_BUTTON)
            tracking = 2;
    }

    //stop tracking the mouse
    else if (state == GLUT_UP) {
        if (tracking == 1) {
            alpha -= (xx - startX);
            _beta += (yy - startY);
        }
        else if (tracking == 2) {
            r += (yy - startY) * 0.01f;
            if (r < 0.1f)
                r = 0.1f;
        }
        tracking = 0;
    }
}

void InputManager::onMouseMotion(int xx, int yy) 
{
    if (gameManager && gameManager->isEditingPath()) {
        if (gameManager->isArduinoRecording())
            return;
        int winW = glutGet(GLUT_WINDOW_WIDTH);
        int winH = glutGet(GLUT_WINDOW_HEIGHT);
        gameManager->onEditorMouseDrag(xx, yy, winW, winH);
        return;
    }
    int deltaX, deltaY;
    float alphaAux, betaAux;
    float rAux;

    deltaX = -xx + startX;
    deltaY = yy - startY;

    // left mouse button: move camera
    if (tracking == 1) {


        alphaAux = alpha + deltaX;
        betaAux = _beta + deltaY;

        if (betaAux > 85.0f)
            betaAux = 85.0f;
        else if (betaAux < -85.0f)
            betaAux = -85.0f;
        rAux = r;
    }
    // right mouse button: zoom
    else if (tracking == 2) {

        alphaAux = alpha;
        betaAux = _beta;
        rAux = r + (deltaY * 0.01f);
        if (rAux < 0.1f)
            rAux = 0.1f;
    }
    if (activeCamera == camera3)
    {
        camX = rAux * sin(alphaAux * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
        camZ = rAux * cos(alphaAux * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
        camY = rAux * sin(betaAux * 3.14f / 180.0f);

    }
}

void InputManager::onMouseWheel(int wheel, int direction, int x, int y) {
    r += direction * 0.1f;
    if (r < 0.1f) r = 0.1f;

    camX = r * sinf(alpha * 3.14f / 180.0f) * cosf(_beta * 3.14f / 180.0f);
    camZ = r * cosf(alpha * 3.14f / 180.0f) * cosf(_beta * 3.14f / 180.0f);
    camY = r * sinf(_beta * 3.14f / 180.0f);
}

void InputManager::init() {
    camX = r * sinf(alpha * 3.14f / 180.0f) * cosf(_beta * 3.14f / 180.0f);
    camZ = r * cosf(alpha * 3.14f / 180.0f) * cosf(_beta * 3.14f / 180.0f);
    camY = r * sinf(_beta * 3.14f / 180.0f);
}
