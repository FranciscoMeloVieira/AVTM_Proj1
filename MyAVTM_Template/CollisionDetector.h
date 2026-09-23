#pragma once
#include "Collision.h"
#include "GameObject.h"
#include <vector>
#include "Vector3.h"


class CollisionDetector
{
public:
	void updateCollisions();
	void AddGameObject(PhysicsObject* obj) { gameObjects.push_back(obj); }
private:
	std::vector<PhysicsObject*> gameObjects;
	CollisionInfo CheckCollision(const Collider* a, const Collider* b);
};