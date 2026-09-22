#pragma once
class GameObject;

struct CollisionInfo;

class ICollidable
{
public:
	virtual ~ICollidable() = default;

	virtual void OnCollision(const GameObject& other, const CollisionInfo& info) = 0;
};
