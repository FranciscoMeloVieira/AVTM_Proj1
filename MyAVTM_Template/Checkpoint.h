#pragma once
#include "GameObject.h"
#include <unordered_set>

class CheckpointManager;

class Checkpoint : public GameObject
{
public:
	Checkpoint(CheckpointManager* manager, const Vector3& pos, const Vector3& min, const Vector3& max, int id);
	void update(float deltaTime) override;
	void OnCollision(const GameObject& other, const CollisionInfo& info) override;
private: 
	CheckpointManager* manager;
	std::unordered_set<const GameObject*> collidingObjects;
	int id = 0;
};

