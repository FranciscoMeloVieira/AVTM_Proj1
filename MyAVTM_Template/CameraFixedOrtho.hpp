#include "Camera.hpp"

class CameraFixedOrtho : public Camera
{
public:
    float viewSize;
    float nearPlane, farPlane;

    CameraFixedOrtho(float viewSize = 25.0f, float nearPlane = 0.1f, float farPlane = 1000.0f)
        : viewSize(viewSize), nearPlane(nearPlane), farPlane(farPlane) {
    }

    void loadProjectionMatrix(gmu& mu, float ratio) const override {
        float right = viewSize * ratio;
        float top = viewSize;

        mu.loadIdentity(gmu::PROJECTION);
        mu.ortho(-right, right, -top, top, nearPlane, farPlane);
    }
};

