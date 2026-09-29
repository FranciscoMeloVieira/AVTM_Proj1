#include "Candles.hpp"
#include "renderer.h"
#include <iostream>


extern Renderer renderer;
extern gmu mu;

Candles::Candles(int meshId, int textureId) : m_meshId(meshId), m_textureId(textureId)
{
	Material candleMaterial = { //TODO - Change
	{ 0.5f, 0.4f, 0.4f, 1.0f }, //Diffuse
	{ 0.05f, 0.05f, 0.05f, 1.0f }, //Ambient
	{ 0.4f, 0.7f, 0.04f, 1.0f }, //Specular
	{ 0.0f, 0.0f, 0.0f, 1.0f }, //Emissive
	(0.08f * 128.0f)  //Shininess
	};

	std::vector<Vector3> positions = {
		{ 50.0f, 1.0f, -150.0f},
		{ 250.0f, 1.0f, -150.0f},
		{ 50.0f, 1.0f, 150.0f},
		{ 250.0f, 1.0f, 150.0f},
		{ 150.0f, 1.0f,  0.0f},
		{ -50.0f, 1.0f, -200.0f} };

	/*for (Candle& candle : candles)
	{

		orange.collider = new Collider();
		orange.collider->position = orange.position;
		orange.collider->max = { 1.0f, 1.0f, 1.0f };
		orange.collider->min = { -1.0f, -1.0f, -1.0f };

		orange.mass = 1.0f;
		*/

	candleNode = new InstancedObjectNode(m_meshId, m_textureId, candleMaterial, positions);
	candleNode->setTransform(
		0.0f, 0.0f, 0.0f,    // position
		90.0f, 0.0f, -90.0f,   // rotation — X axis, not Y
		30.0f, 30.0f, 30.0f);
}


void Candles::update(float deltaTime)
{
}

	/*
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

*/