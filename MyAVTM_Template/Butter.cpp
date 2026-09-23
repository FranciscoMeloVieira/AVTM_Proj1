#include "Butter.h"
#include "renderer.h"
#include "mathUtility.h"
#include <iostream>

extern gmu mu;
extern Renderer renderer;

Butter::Butter(int meshID, int texMode, Vector3 pos)
{
	butterNode = new MeshNode(meshID, texMode);
	butterNode->setTexMode(BUTTER_TEXTURE_MODE);

	butterNode->setScale(BUTTER_WIDTH, BUTTER_HEIGHT, BUTTER_DEPTH);
	butterNode->setAbsolutePosition(pos.x, pos.y, pos.z);
	position = pos;

	collider = new Collider();
	collider->position = position;
	collider->max = { BUTTER_WIDTH / 2.0f, BUTTER_HEIGHT / 2.0f, BUTTER_DEPTH / 2.0f };
	collider->min = { -BUTTER_WIDTH / 2.0f, -BUTTER_HEIGHT / 2.0f, -BUTTER_DEPTH / 2.0f };
}

void Butter::OnCollision(const GameObject& other, const CollisionInfo& info)
{
	std::cout << "Butter collided with another object!" << std::endl;

	// 1. Correct position away from the obstacle
	position -= info.collisionNormal * info.penetrationDepth;

	// 2. Measure speed moving INTO the collision surface
	// Note: Dot is negative when moving toward the surface normal
	float impactSpeed = Dot(velocity, info.collisionNormal);

	if (impactSpeed < 0.0f)
	{
		Vector3 normalVelocity = info.collisionNormal * impactSpeed;
		Vector3 tangentVelocity = velocity - normalVelocity;

		float bounciness = 0.1f;    // Minimal rebound
		float impactFriction = 0.7f; // High speed loss on impact (70% speed dump)

		// Cancel inward velocity + heavily dampen sliding velocity
		velocity = (-normalVelocity * bounciness) + (tangentVelocity * (1.0f - impactFriction));

		// Instantly stop if moving very slowly
		if (Length(velocity) < 0.1f)
		{
			velocity = { 0.0f, 0.0f, 0.0f };
		}
	}

	collider->position = position;
	butterNode->setAbsolutePosition(position.x, position.y, position.z);
}

void Butter::update(float deltaTime)
{
	// 1. Add linear drag to quickly slow down after being hit
	float friction = 4.0f; // Increase value for a faster slowdown (e.g., 2.0 = smooth stop, 8.0 = quick halt)
	velocity -= velocity * friction * deltaTime;

	// 2. Stop completely at low thresholds to prevent indefinite sliding
	if (Length(velocity) < 0.05f)
	{
		velocity = { 0.0f, 0.0f, 0.0f };
	}

	// 3. Physics integration
	position += velocity * deltaTime;
	velocity += acceleration * deltaTime;

	// 4. Update visuals and colliders
	butterNode->setAbsolutePosition(position.x, position.y, position.z);
	updateColliderPosition();
}





