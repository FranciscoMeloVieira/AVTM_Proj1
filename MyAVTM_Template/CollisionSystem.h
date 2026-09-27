#pragma once
#include "Collision.h"
#include "GameObject.h"
#include <vector>
#include "Vector3.h"


class CollisionSystem
{
public:
	void updateCollisions();
	void AddGameObject(GameObject* obj) { gameObjects.push_back(obj); }
private:
	std::vector<GameObject*> gameObjects;
	CollisionInfo CheckCollision(const Collider* a, const Collider* b);

    // Helper structure for Axis-Aligned Bounding Box (AABB) bounds
    struct AABB
    {
        float minX, maxX;
        float minY, maxY;
        float minZ, maxZ;
        GameObject* owner;
    };

    // Helper to calculate world-space AABB for a game object
    AABB GetWorldAABB(GameObject* obj) const
    {
        AABB bounds;
        Collider* c = obj->collider;
        bounds.minX = obj->position.x + c->min.x;
        bounds.maxX = obj->position.x + c->max.x;
        bounds.minY = obj->position.y + c->min.y;
        bounds.maxY = obj->position.y + c->max.y;
        bounds.minZ = obj->position.z + c->min.z;
        bounds.maxZ = obj->position.z + c->max.z;
        bounds.owner = obj;
        return bounds;
    }
};