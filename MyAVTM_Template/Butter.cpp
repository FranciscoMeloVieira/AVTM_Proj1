#include "Butter.h"
#include "renderer.h"
#include "mathUtility.h"
#include <iostream>

extern gmu mu;
extern Renderer renderer;

Butter::Butter(int meshID, int texMode, Vector3 pos)
{
	butterNode = new MeshNode(meshID, texMode);
	butterNode->setTexMode(BUTTER_TEXTURE_MODE);

	butterNode->setScale(BUTTER_WIDTH, BUTTER_HEIGHT, BUTTER_DEPTH);
	butterNode->setAbsolutePosition(pos.x, pos.y, pos.z);
	position = pos;

	collider = new Collider();
	collider->position = position;
	collider->max = { BUTTER_WIDTH, BUTTER_HEIGHT, BUTTER_DEPTH };
	collider->min = { 0.0f, 0.0f, 0.0f };
}

void Butter::OnCollision(const GameObject& other, const CollisionInfo& info)
{
	auto otherPhysicsObj = dynamic_cast<const PhysicsObject*>(&other);

    if (!otherPhysicsObj)
		return;

    // 1. Positional correction
    position += info.collisionNormal * info.penetrationDepth;
    collider->position = position;
    butterNode->setAbsolutePosition(position.x, position.y, position.z);

	// Should not be necessary but bugs are a thing, so let's be safe and ensure the normal is pointing in the right direction
    Vector3 normal = info.collisionNormal;
    Vector3 displacement = position - other.position;
    if (Dot(displacement, normal) < 0.0f)
    {
        normal = -normal;
    }

    // 2. Relative velocity (Velocity of Butter relative to Other)
    Vector3 relativeVelocity = velocity - otherPhysicsObj->velocity;

    // Calculate velocity projected onto the normal
    float velAlongNormal = Dot(relativeVelocity, normal);

    // Only solve if objects are moving TOWARD each other along the normal
    if (velAlongNormal < 0.0f)
    {
		// Restitution (0.0 = no bounce/stick, 0.2 = slight bounce), should be a property of the object, but for now, let's hardcode it
        float restitution = 0.1f;

        // Calculate impulse magnitude
        float impulseMagnitude = -(1.0f + restitution) * velAlongNormal;
        impulseMagnitude /= ((1.0f / mass) + (1.0f / otherPhysicsObj->mass));

        // Apply impulse in the direction of the corrected normal
        Vector3 impulse = normal * impulseMagnitude;

        velocity += impulse * (1.0f / mass);
    }
}

void Butter::update(float deltaTime)
{
	float frictionCoefficient = 0.7f; // Adjust this value for more or less friction, should depend on the surface and object properties, but hardcoding for now
	acceleration = -velocity * frictionCoefficient;

	// 3. Physics integration
	position += velocity * deltaTime;
	velocity += acceleration * deltaTime;

	// 4. Update visuals and colliders
	butterNode->setAbsolutePosition(position.x, position.y, position.z);
	updateColliderPosition();
}





