#pragma once
#include "ICollidable.h"
#include "Collision.h"
#include "Vector3.h"

class GameObject : public ICollidable
{
public:
	Vector3 position;
	Vector3 rotation;
	Collider* collider;

	virtual void update(float deltaTime) = 0;
	virtual void OnCollision(const GameObject& other, const CollisionInfo& info) override = 0;

};

class PhysicsObject : public GameObject
{
public:
	Vector3 velocity = { 0.0f, 0.0f, 0.0f };
	Vector3 acceleration = { 0.0f, 0.0f, 0.0f };

	float mass = 1.0f;
	Vector3 force = { 0.0f, 0.0f, 0.0f };


	void update(float deltaTime) {};
};

