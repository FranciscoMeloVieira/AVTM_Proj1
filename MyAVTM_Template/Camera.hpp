#ifndef CAMERA_FIXED_PERSPECTIVE_HPP
#define CAMERA_FIXED_PERSPECTIVE_HPP

#include "mathUtility.h"

class Camera
{
public:
	void setCameraPosition(float x, float y, float z) { camX = x; camY = y; camZ = z; }
	void setCameraTarget(float x, float y, float z) { targetX = x; targetY = y; targetZ = z; }
	void setCameraUpVector(float x, float y, float z) { upX = x; upY = y; upZ = z; }

	virtual void applyViewMatrix(gmu& mu) const {
		mu.loadIdentity(gmu::VIEW);
		mu.lookAt(camX, camY, camZ, targetX, targetY, targetZ, upX, upY, upZ);
	}

	virtual void loadProjectionMatrix(gmu& mu, float ratio) const = 0;
private:
	float camX = 0.0f, camY = 50.0f, camZ = 0.0f;
	float targetX = 0.0f, targetY = 0.0f, targetZ = 0.0f; 
	float upX = 0.0f, upY = 0.0f, upZ = -1.0f; 
};

#endif