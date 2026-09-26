#include "Checkpoint.h"
#include <iostream>
#include "Car.hpp"
#include "CheckpointManager.h"

Checkpoint::Checkpoint(CheckpointManager* manager, const Vector3& pos, const Vector3& min, const Vector3& max, int id)
{
	this->manager = manager;
	this->position = pos;
	this->id = id;
	collider = new Collider();
	collider->position = pos;
	collider->min = min;
	collider->max = max; 
}



void Checkpoint::OnCollision(const GameObject& other, const CollisionInfo& info)
{
	auto car = dynamic_cast<const Car*>(&other);
	if (!car)
		return;

	currentFrameObjects.insert(&other);

	if (!previousFrameObjects.contains(&other))
	{
		std::cout << "Checkpoint " << id << " crossed!" << std::endl;
		if (manager)
		{
			manager->OnCheckpointCrossed(id);
		}
	}
}

void Checkpoint::update(float deltaTime)
{
	previousFrameObjects = currentFrameObjects;
	currentFrameObjects.clear();
}
