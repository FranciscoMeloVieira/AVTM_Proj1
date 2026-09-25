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
};