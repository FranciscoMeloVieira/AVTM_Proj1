#include "Car.hpp"
#include "mathUtility.h"
#include <algorithm>
#include <cmath>
#include "renderer.h"

#include <iostream>
#include <OrangeManager.hpp>
#include <Checkpoint.h>

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
		onOrangeCollision.invoke(1);
		reset({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f });
	}

	if (const Checkpoint* checkpoint = dynamic_cast<const Checkpoint*>(&other))
	{
		return;
	}

	// Push the car out of collision
	position += info.collisionNormal * info.penetrationDepth;

	collider->position = position;
	carNode->setAbsolutePosition(position.x, position.y, position.z);
}

void Car::reset(const Vector3& targetPosition, const Vector3& targetFacing)
{
	// 1. Reset position and motion
	position = targetPosition;
	velocity = { 0.0f, 0.0f, 0.0f };
	acceleration = { 0.0f, 0.0f, 0.0f };
	steering = 0.0f;

	// 2. Reset direction vectors
	facingDirection = Normalize(targetFacing);

	// 3. Calculate target angle relative to Z-axis (0,0,1)
	float targetAngle = std::atan2(facingDirection.x, facingDirection.z);
	float targetDegrees = targetAngle * 180.0f / 3.14159265f;
	float rotationDifference = targetDegrees - currentRotationY;

	while (rotationDifference > 180.0f) rotationDifference -= 360.0f;
	while (rotationDifference < -180.0f) rotationDifference += 360.0f;

	currentRotationY += rotationDifference;
	carNode->setRotation(rotationDifference, 0.0f, 1.0f, 0.0f);


	carNode->setAbsolutePosition(position.x, position.y, position.z);
	
	collider->position = position;
	
}

void Car::addChildren()
{
	MeshNode* carOBJ = new MeshNode(6, 0);
	carOBJ->setTransform(
		0.0f, 0.0f, 0.0f,    // position
		90.0f, 0.0f, -90.0f,   // rotation — X axis, not Y
		3.0f, 3.0f, 3.0f);
	this->carNode->addChild(carOBJ);
}
