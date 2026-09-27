#include "CollisionSystem.h"
#include "GameObject.h"
#include <iostream>
#include <algorithm>

void CollisionSystem::updateCollisions()
{
    if (gameObjects.size() < 2)
        return;

    // 1. Build world-space bounding boxes for active objects
    std::vector<AABB> aabbs;
    aabbs.reserve(gameObjects.size());

    for (GameObject* obj : gameObjects)
    {
        if (obj && obj->collider)
        {
            aabbs.push_back(GetWorldAABB(obj));
        }
    }

    // 2. Sort AABBs along the chosen axis (X-axis in this case)
    std::stable_sort(aabbs.begin(), aabbs.end(), [](const AABB& a, const AABB& b) {
        return a.minX < b.minX;
        });

    // 3. Sweep along the X-axis
    for (size_t i = 0; i < aabbs.size(); ++i)
    {
        const AABB& a = aabbs[i];

        for (size_t j = i + 1; j < aabbs.size(); ++j)
        {
            const AABB& b = aabbs[j];

            if (b.minX > a.maxX)
            {
                break;
            }

            // Quick check for Y and Z overlap before doing full collision check
            bool overlapY = (a.minY <= b.maxY && a.maxY >= b.minY);
            bool overlapZ = (a.minZ <= b.maxZ && a.maxZ >= b.minZ);

            if (overlapY && overlapZ)
            {
                // Full narrow-phase collision check
                CollisionInfo collisionInfo = CheckCollision(a.owner->collider, b.owner->collider);

                if (collisionInfo.collisionDetected)
                {
                    // Object A receives primary collision
                    a.owner->OnCollision(*b.owner, collisionInfo);

                    // Object B receives inverted collision
                    CollisionInfo invertedInfo = collisionInfo;
                    invertedInfo.collisionNormal = collisionInfo.collisionNormal * -1.0f;
                    b.owner->OnCollision(*a.owner, invertedInfo);
                }
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