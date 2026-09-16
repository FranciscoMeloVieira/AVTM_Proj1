#include "Car.hpp"
#include "mathUtility.h"
#include <algorithm>
#include <cmath>
#include "renderer.h"

#include <iostream>

extern gmu mu;

CarNode::CarNode(int meshId, int textureId,
	const float* position,
	float angle,
	const float* axis,
	const float* scale)
	: ScenegraphNode(position, angle, axis, scale),
	meshID(meshId),
	texMode(textureId)
{
	if (position) {
		setAbsolutePosition(position[0], position[1], position[2]);
	}

	if (angle != 0.0f && axis) {
		setRotation(angle, axis[0], axis[1], axis[2]);
	}

	if (scale) {
		setScale(scale[0], scale[1], scale[2]);
	}
	addChildren();
}

void CarNode::update(float deltaTime)
{
	if (isAccelerating)
		if (speed < 0.0f)
			speed += brakeDeceleration * deltaTime;
		else
			speed += acceleration * deltaTime;
	else if (isDecelerating)
		if (speed > 0.0f)
			speed -= brakeDeceleration * deltaTime;
		else
			speed -= acceleration * deltaTime;
	else
		if (speed > 0.0f)
			speed -= deceleration * deltaTime;
		else 
			speed += deceleration * deltaTime;

	speed = std::clamp(speed, -maxSpeed, maxSpeed);


	if (speed > 0.01f || speed < -0.01f)
	{
		float turnRate = steering * maxTurnRate;
		turnRate *= (( - 5.0f / 6.0f * speed * speed + maxSpeed * speed) * (1.0f / maxTurnRate) * 1.0f / 5.4f ); // Adjust turn rate based on speed (example formula)
		float frameRotation = turnRate * (speed / maxSpeed) * deltaTime;
		setRotation(frameRotation, 0.0f, 1.0f, 0.0f);
	}

	translate(0.0f, 0.0f, speed * deltaTime);
}

void CarNode::draw()
{
	mu.pushMatrix(gmu::MODEL);

	mu.multMatrix(gmu::MODEL, localTransform);

	mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
	mu.computeNormalMatrix3x3();

	for (auto& child : children) {
		child->draw();
	}

	mu.popMatrix(gmu::MODEL);
}

void CarNode::addChildren()
{
	MeshNode* carBody = new MeshNode(0, 2);
	carBody->setTransform(
		-1.7f, 1.5f, -3.7f,
		0.0f, 0.0f, 0.0f,
		3.4f, 2.0f, 7.4f
	);
	this->addChild(carBody);

	MeshNode* carTop = new MeshNode(0, 2);
	carTop->setTransform(
		0.7f - 1.7f, 3.5f, 1.2f - 3.7f,
		0.0f, 0.0f, 0.0f,
		2.0f, 1.5f, 5.0f
	);
	this->addChild(carTop);

	MeshNode* wheel1 = new MeshNode(3, 3);
	wheel1->setTransform(
		0.0f - 1.7f, 1.5f, 1.5f - 3.7f,
		0.0f, 0.0f, 90.0f,
		2.0f, 0.5f, 2.0f
	);
	this->addChild(wheel1);

	MeshNode* wheel2 = new MeshNode(3, 3);
	wheel2->setTransform(
		3.4f - 1.7f, 1.5f, 1.5f - 3.7f,
		0.0f, 0.0f, 90.0f,
		2.0f, 0.5f, 2.0f
	);
	this->addChild(wheel2);

	MeshNode* wheel3 = new MeshNode(3, 3);
	wheel3->setTransform(
		0.0f - 1.7f, 1.5f, 6.5f - 3.7f,
		0.0f, 0.0f, 90.0f,
		2.0f, 0.5f, 2.0f
	);
	this->addChild(wheel3);

	MeshNode* wheel4 = new MeshNode(3, 3);
	wheel4->setTransform(
		3.4f - 1.7f, 1.5f, 6.5f - 3.7f,
		0.0f, 0.0f, 90.0f,
		2.0f, 0.5f, 2.0f
	);
	this->addChild(wheel4);
}
