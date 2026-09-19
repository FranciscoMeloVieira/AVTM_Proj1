#include "Camera.hpp"

class CameraFixedPerspective : public Camera
{
public:
    float fov;
    float nearPlane, farPlane;

    CameraFixedPerspective(float fov = 90.0f, float nearPlane = 0.1f, float farPlane = 1000.0f)
        : fov(fov), nearPlane(nearPlane), farPlane(farPlane) {
    }

    void loadProjectionMatrix(gmu& mu, float ratio) const override {
        mu.loadIdentity(gmu::PROJECTION);
        mu.perspective(fov, ratio, nearPlane, farPlane);
    }
};
