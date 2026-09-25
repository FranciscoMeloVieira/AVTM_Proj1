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
	collider->max = { BUTTER_WIDTH, BUTTER_HEIGHT, BUTTER_DEPTH };
	collider->min = { 0.0f, 0.0f, 0.0f };
}

void Butter::OnCollision(const GameObject& other, const CollisionInfo& info)
{
	// 1. Correct position away from the obstacle (changed -= to +=)
	position += info.collisionNormal * info.penetrationDepth;

	collider->position = position;
	butterNode->setAbsolutePosition(position.x, position.y, position.z);
}

void Butter::update(float deltaTime)
{

	// 3. Physics integration
	position += velocity * deltaTime;
	velocity += acceleration * deltaTime;

	// 4. Update visuals and colliders
	butterNode->setAbsolutePosition(position.x, position.y, position.z);
	updateColliderPosition();
}





