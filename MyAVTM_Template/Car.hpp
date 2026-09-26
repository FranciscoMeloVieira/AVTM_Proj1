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

    float steering = 0;
    bool isAccelerating = false;
	bool isDecelerating = false;
    EmptyNode* carNode = nullptr; // Node representing the car
    void OnCollision(const GameObject& other, const CollisionInfo& info) override;

	Event<int> onOrangeCollision;

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