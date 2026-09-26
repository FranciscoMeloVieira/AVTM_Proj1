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

void Car::OnCollision(const GameObject& other, const CollisionInfo& info)
{
	if (const Orange* orange = dynamic_cast<const Orange*>(&other))
	{
		velocity = { 0.0f, 0.0f, 0.0f };
		acceleration = { 0.0f, 0.0f, 0.0f };
		position = { 0.0f, 0.0f, 0.0f };
		collider->position = position;
	}
	// Push the car out of the collision
	position += info.collisionNormal * info.penetrationDepth;

	// Remove velocity going into the collision surface
	float velocityIntoWall = Dot(velocity, info.collisionNormal);

	if (velocityIntoWall > 0.0f)
	{
		velocity -= info.collisionNormal * velocityIntoWall;
	}

	collider->position = position;
	carNode->setAbsolutePosition(position.x, position.y, position.z);
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
