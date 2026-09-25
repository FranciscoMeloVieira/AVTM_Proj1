#include "CollisionSystem.h"
#include "GameObject.h"
#include <iostream>

void CollisionSystem::updateCollisions()
{
	for (size_t i = 0; i < gameObjects.size(); ++i)
	{
		for (size_t j = i + 1; j < gameObjects.size(); ++j)
		{
			Collider* colliderA = gameObjects[i]->collider;
			Collider* colliderB = gameObjects[j]->collider;

			CollisionInfo collisionInfo = CheckCollision(colliderA, colliderB);
			if (collisionInfo.collisionDetected)
			{
				gameObjects[i]->OnCollision(*gameObjects[j], collisionInfo);
				gameObjects[j]->OnCollision(*gameObjects[i], collisionInfo);
			}
		}
	}
}

CollisionInfo CollisionSystem::CheckCollision(const Collider* colliderA, const Collider* colliderB)
{
	CollisionInfo collisionInfo;

	float overlapX = std::min(colliderA->position.x + colliderA->max.x, colliderB->position.x + colliderB->max.x) -
		std::max(colliderA->position.x + colliderA->min.x, colliderB->position.x + colliderB->min.x);
	float overlapY = std::min(colliderA->position.y + colliderA->max.y, colliderB->position.y + colliderB->max.y) -
		std::max(colliderA->position.y + colliderA->min.y, colliderB->position.y + colliderB->min.y);
	float overlapZ = std::min(colliderA->position.z + colliderA->max.z, colliderB->position.z + colliderB->max.z) -
		std::max(colliderA->position.z + colliderA->min.z, colliderB->position.z + colliderB->min.z);

	if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f)
	{
		return collisionInfo;
	}

	collisionInfo.collisionDetected = true;

	if (overlapX < overlapY && overlapX < overlapZ)
	{
		collisionInfo.penetrationDepth = overlapX;
		collisionInfo.collisionNormal = (colliderA->position.x < colliderB->position.x) ? Vector3(-1.0f, 0.0f, 0.0f) : Vector3(1.0f, 0.0f, 0.0f);
	}
	else if (overlapY < overlapZ)
	{
		collisionInfo.penetrationDepth = overlapY;
		collisionInfo.collisionNormal = (colliderA->position.y < colliderB->position.y) ? Vector3(0.0f, -1.0f, 0.0f) : Vector3(0.0f, 1.0f, 0.0f);
	}
	else
	{
		collisionInfo.penetrationDepth = overlapZ;
		collisionInfo.collisionNormal = (colliderA->position.z < colliderB->position.z) ? Vector3(0.0f, 0.0f, -1.0f) : Vector3(0.0f, 0.0f, 1.0f);
	}

	return collisionInfo;
}