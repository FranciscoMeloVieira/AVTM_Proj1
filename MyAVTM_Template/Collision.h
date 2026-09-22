#pragma once
#include "Vector3.h"

typedef struct CollisionInfo
{
	bool collisionDetected = false;
	Vector3 collisionNormal;
	float penetrationDepth;
} CollisionInfo;

class Collider
{
public:
	Vector3 min;
	Vector3 max;
	Vector3 position; // Center position of the AABB
	Vector3 rotation; // Rotation of the AABB (if needed)
};