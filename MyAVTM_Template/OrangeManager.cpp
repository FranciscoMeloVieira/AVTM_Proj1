#include "OrangeManager.hpp"
#include "renderer.h"
#include <iostream>

extern Renderer renderer;
extern gmu mu;

OrangesManager::OrangesManager(int meshId, int textureId) : m_meshId(meshId), m_textureId(textureId)
{
	Material orangeMaterial = { //Based on Red Rubber
	{ 0.5f, 0.4f, 0.4f, 1.0f }, //Diffuse
	{ 0.05f, 0.05f, 0.05f, 1.0f }, //Ambient
	{ 0.4f, 0.7f, 0.04f, 1.0f }, //Specular
	{ 0.0f, 0.0f, 0.0f, 1.0f }, //Emissive
	(0.08f * 128.0f)  //Shininess
	};

	oranges.resize(numOranges);
	std::vector<Vector3> positions;
	for (Orange& orange : oranges)
	{
		orange.position = {
			-500.0f,
			3.0f,
			-500.0f
		};

		orange.collider = new Collider();
		orange.collider->position = orange.position;
		orange.collider->max = { 1.0f, 1.0f, 1.0f };
		orange.collider->min = { -1.0f, -1.0f, -1.0f };

		orange.mass = 1.0f;

		spawnOrange(orange);
		positions.push_back(orange.position);
	}

	// Create the InstancedObjectNode for rendering
	orangesNode = new InstancedObjectNode(m_meshId, m_textureId, orangeMaterial, positions);
}


void OrangesManager::update(float deltaTime)
{
	std::vector<Vector3> positions;
	for (Orange& orange : oranges)
	{
		// Physics
		orange.velocity += orange.acceleration * deltaTime;
		orange.position += orange.velocity * deltaTime;

		orange.collider->position = orange.position;

		// Respawn when leaving the map
		if (orange.position.x < leftBorder ||
			orange.position.x > rightBorder ||
			orange.position.z < bottomBorder ||
			orange.position.z > topBorder)
		{
			spawnOrange(orange);
		}
		positions.push_back(orange.position);
	}
	orangesNode->setPositions(positions);
}


void OrangesManager::spawnOrange(Orange& orange)
{
	int borderLine = rand() % 4;

	float randomX =
		static_cast<float>(rand()) / RAND_MAX *
		(rightBorder - leftBorder) + leftBorder;

	float randomZ =
		static_cast<float>(rand()) / RAND_MAX *
		(topBorder - bottomBorder) + bottomBorder;

	switch (borderLine)
	{
	case 0: // North (+Z)
		orange.position = {
			randomX,
			3.0f,
			topBorder
		};
		break;

	case 1: // South (-Z)
		orange.position = {
			randomX,
			3.0f,
			bottomBorder
		};
		break;

	case 2: // East (+X)
		orange.position = {
			rightBorder,
			3.0f,
			randomZ
		};
		break;

	case 3: // West (-X)
		orange.position = {
			leftBorder,
			3.0f,
			randomZ
		};
		break;
	}

	// Give it a random direction
	float angle =
		static_cast<float>(rand()) / RAND_MAX * 6.2831853f;

	float speed =
		static_cast<float>(rand()) / RAND_MAX *
		(maxOrangeSpeed - minOrangeSpeed) +
		minOrangeSpeed;

	orange.velocity = {
		std::cos(angle) * speed,
		0.0f,
		std::sin(angle) * speed
	};

	orange.collider->position = orange.position;
}

void Orange::OnCollision(const GameObject& other, const CollisionInfo& info)
{
	// 1. Positional correction to prevent sticking inside objects
	position += info.collisionNormal * info.penetrationDepth;

	// Should not be necessary but bugs are a thing, so let's be safe and ensure the normal is pointing in the right direction
	Vector3 normal = Normalize(info.collisionNormal);
	Vector3 displacement = position - other.position;
	if (Dot(displacement, normal) < 0.0f)
	{
		normal = -normal;
	}

	// 2. Only reflect if moving TOWARD the collision normal
	if (Dot(velocity, normal) < 0.0f)
	{
		// Standard Reflection: R = V - 2*(V . N)*N
		velocity = velocity - normal * (2.0f * Dot(velocity, normal));
	}

	// 3. Update scene nodes and colliders
	collider->position = position;
}
