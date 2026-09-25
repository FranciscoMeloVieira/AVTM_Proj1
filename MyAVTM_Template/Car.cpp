#include "Car.hpp"
#include "mathUtility.h"
#include <algorithm>
#include <cmath>
#include "renderer.h"

#include <iostream>
#include <OrangeManager.hpp>

extern gmu mu;

Car::Car(int meshId, int textureId,
	const float* position,
	float angle,
	const float* axis,
	const float* scale)
{
	carNode = new EmptyNode(position, angle, axis, scale);
	addChildren();

	collider = new Collider();
	collider->position = { 0.0f, 0.0f, 0.0f };
	collider->max = { 5.0f, 5.0f, 5.0f };
	collider->min = { -5.0f, -5.0f, -5.0f };
}

void Car::update(float deltaTime)
{
	acceleration = { 0.0f, 0.0f, 0.0f };

	// 1. Determine signed speed (positive = forward, negative = reverse)
	float forwardSpeed = Dot(velocity, facingDirection);
	float absoluteSpeed = std::abs(forwardSpeed);
	bool isReversing = forwardSpeed < -0.1f;

	// 2. Scale turning rate based on current absolute speed
	float speedRatio = std::min(absoluteSpeed / maxSpeed, 1.0f);

	// Minimal turn capability at rest, scaling up with speed
	float turnSpeedMultiplier = 0.10f + (0.90f * speedRatio);

	// 3. Apply direction sign so reversing turns in the correct arc
	float directionSign = isReversing ? -1.0f : 1.0f;
	float steeringAngle = -steering * maxTurnRate * turnSpeedMultiplier * directionSign * deltaTime;

	float radians = steeringAngle * 3.14159265f / 180.0f;

	facingDirection = Normalize(RotateY(facingDirection, radians));

	// Rotation/Visual Node logic
	float targetAngle = std::atan2(facingDirection.x, facingDirection.z);
	float targetDegrees = targetAngle * 180.0f / 3.14159265f;
	float rotationDifference = targetDegrees - currentRotationY;

	while (rotationDifference > 180.0f) rotationDifference -= 360.0f;
	while (rotationDifference < -180.0f) rotationDifference += 360.0f;

	currentRotationY += rotationDifference;
	carNode->setRotation(rotationDifference, 0.0f, 1.0f, 0.0f);

	// 4. Compute Acceleration & Reversing Logic
	if (isAccelerating)
	{
		acceleration += facingDirection * accelerationForce;
	}
	else if (isDecelerating)
	{
		// If moving forward, brake. If stopped or already moving backward, reverse.
		if (forwardSpeed > 0.1f)
		{
			acceleration -= facingDirection * brakeForce;
		}
		else
		{
			// Reverse drive force
			float reverseForce = accelerationForce * 0.5f; // Reverse is usually slower
			acceleration -= facingDirection * reverseForce;
		}
	}

	// 5. Apply Drag
	acceleration -= velocity * dragCoefficient;

	// 6. Physics Integration
	velocity += acceleration * deltaTime;

	// Stop completely if minimal input and speed
	if (!isAccelerating && !isDecelerating && Length(velocity) < 0.1f)
	{
		velocity = { 0.0f, 0.0f, 0.0f };
	}

	// 7. Lateral Friction
	Vector3 rightDirection = { facingDirection.z, 0.0f, -facingDirection.x };

	float currentForwardSpeed = Dot(velocity, facingDirection);
	float currentLateralSpeed = Dot(velocity, rightDirection);

	currentLateralSpeed *= std::exp(-tireGrip * deltaTime);

	velocity = (facingDirection * currentForwardSpeed) + (rightDirection * currentLateralSpeed);

	// 8. Apply Position Updates
	position += velocity * deltaTime;

	collider->position = position;
	carNode->setAbsolutePosition(position.x, position.y, position.z);
}
void Car::stopCar()
{
	isAccelerating = false;
	isDecelerating = false;
	steering = 0.0f;
}

void Car::setPosition(Vector3 position)
{
	this->position = position;
	collider->position = position;
	carNode->setAbsolutePosition(position.x, position.y, position.z);
}

void Car::OnCollision(const GameObject& other, const CollisionInfo& info)
{
	if (const Orange* orange = dynamic_cast<const Orange*>(&other))
	{
		velocity = { 0.0f, 0.0f, 0.0f };
		acceleration = { 0.0f, 0.0f, 0.0f };
		position = { 0.0f, 0.0f, 0.0f };
		collider->position = position;
	}

	// Push the car out of collision
	position += info.collisionNormal * info.penetrationDepth;

	collider->position = position;
	carNode->setAbsolutePosition(position.x, position.y, position.z);
}

void Car::addChildren()
{
	MeshNode* carBody = new MeshNode(0, 0);
	carBody->setTransform(
		-1.7f, 1.5f, -3.7f,
		0.0f, 0.0f, 0.0f,
		3.4f, 2.0f, 7.4f
	);
	this->carNode->addChild(carBody);

	MeshNode* carTop = new MeshNode(0, 0);
	carTop->setTransform(
		0.7f - 1.7f, 3.5f, 1.2f - 3.7f,
		0.0f, 0.0f, 0.0f,
		2.0f, 1.5f, 5.0f
	);
	this->carNode->addChild(carTop);

	MeshNode* wheel1 = new MeshNode(5, 5);
	wheel1->setTransform(
		0.0f - 1.7f, 1.5f, 1.5f - 3.7f,
		0.0f, 0.0f, 90.0f,
		0.7f, 0.7f, 0.7f
	);
	this->carNode->addChild(wheel1);

	MeshNode* wheel2 = new MeshNode(5, 5);
	wheel2->setTransform(
		3.4f - 1.7f, 1.5f, 1.5f - 3.7f,
		0.0f, 0.0f, 90.0f,
		0.7f, 0.7f, 0.7f
	);
	this->carNode->addChild(wheel2);

	MeshNode* wheel3 = new MeshNode(5, 5);
	wheel3->setTransform(
		0.0f - 1.7f, 1.5f, 6.5f - 3.7f,
		0.0f, 0.0f, 90.0f,
		0.7f, 0.7f, 0.7f
	);
	this->carNode->addChild(wheel3);

	MeshNode* wheel4 = new MeshNode(5, 5);
	wheel4->setTransform(
		3.4f - 1.7f, 1.5f, 6.5f - 3.7f,
		0.0f, 0.0f, 90.0f,
		0.7f, 0.7f, 0.7f
	);
	this->carNode->addChild(wheel4);
}
