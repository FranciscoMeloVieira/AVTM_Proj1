#pragma once

class GameManager;
class Camera;
class LightingSystem;

class InputManager {
public:
    void onKeyDown(unsigned char key, int x, int y);
    void onKeyUp(unsigned char key, int x, int y);
    void onMouseButton(int button, int state, int x, int y);
    void onMouseMotion(int x, int y);
    void onMouseWheel(int wheel, int direction, int x, int y);

    float getCamX() const { return camX; }
    float getCamY() const { return camY; }
    float getCamZ() const { return camZ; }
    bool isPaused() const { return paused; }

    void setGameManager(GameManager* gm) { gameManager = gm; }
    void setLightingSystem(LightingSystem* ls) { lightingSystem = ls; }
    void setCameras(Camera* c1, Camera* c2, Camera* c3) { camera1 = c1; camera2 = c2; camera3 = c3; activeCamera = c1; }

    void init();

    Camera* getActiveCamera() const { return activeCamera; }
    bool consumeActiveCameraChanged() {
        bool changed = activeCameraChanged;
        activeCameraChanged = false;
        return changed;
    }

private:
    GameManager* gameManager = nullptr;
    LightingSystem* lightingSystem = nullptr;
    Camera* camera1 = nullptr;
    Camera* camera2 = nullptr;
    Camera* camera3 = nullptr;
    Camera* activeCamera = nullptr;
    bool activeCameraChanged = false;

    float camX = 0.0f, camY = 0.0f, camZ = 0.0f;
    float alpha = 180.0f, _beta = 45.0f, r = 45.0f;
    int startX = 0, startY = 0, tracking = 0;
    bool paused = false;
};