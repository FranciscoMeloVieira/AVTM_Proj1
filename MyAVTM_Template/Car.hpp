#pragma once
#include <ScenegraphNode.h>
#include "GameObject.h"
#include "Event.h"


class Car : public PhysicsObject {


public:
    Car(int meshId, int textureId,
        const float* position = nullptr,
        float angle = 0.0f,
        const float* axis = nullptr,
        const float* scale = nullptr);

    Car(int meshId, int textureId,
        const float(&transformMatrix)[16]);
    void update(float deltaTime) override;
    void stopCar();
	void setPosition(Vector3 position);
	void applyControlRequests(float inSteering, float inThrottle, float inBrake);
	Vector3 getFacingDirection() const { return facingDirection; }
	float getThrottleInput() const { return throttleInput; }
	float getBrakeInput() const { return brakeInput; }

    float steering = 0;
    bool isAccelerating = false;
	bool isDecelerating = false;
	float throttleInput = 0.0f;
	float brakeInput = 0.0f;
    EmptyNode* carNode = nullptr; // Node representing the car
    void OnCollision(const GameObject& other, const CollisionInfo& info) override;

	Event<int> onOrangeCollision;
    void reset(const Vector3& targetPosition, const Vector3& targetFacing = { 0.0f, 0.0f, 1.0f });

private:
    /* Variables for driving */
	
	float accelerationForce = 100.0f;
    float brakeForce = 300.0f;
	float deceleration = 1.0f;

    float maxSpeed = 30.0f;
	float maxAcceleration = 10.0f;
    float dragCoefficient = 0.8f;
    float tireGrip = 12.0f;

	float maxTurnRate = 90.0f; 
	Vector3 facingDirection = { 0.0f, 0.0f, 1.0f }; 
	float currentRotationY = 0.0f; 

    void addChildren();

	Vector3 getForwardVector() {
		return Vector3(std::sin(rotation.y), 0.0f, std::cos(rotation.y));
	}
};