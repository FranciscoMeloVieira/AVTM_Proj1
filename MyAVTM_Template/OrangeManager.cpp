#include "OrangeManager.hpp"
#include "renderer.h"
#include <iostream>

extern Renderer renderer;
extern gmu mu;

OrangesManager::OrangesManager(int meshId, int textureId) : m_meshId(meshId), m_textureId(textureId)
{
	oranges.resize(numOranges);
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
	}


	/*instance VBO*/
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, numOranges * sizeof(float) * 3, nullptr, GL_DYNAMIC_DRAW);

	glBindVertexArray(renderer.myMeshes[m_meshId].vao);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glVertexAttribDivisor(3, 1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

OrangesManager::~OrangesManager() {
	if (instanceVBO != 0) {
		glDeleteBuffers(1, &instanceVBO);
	}
}

void OrangesManager::draw() {
	mu.pushMatrix(gmu::MODEL);
	mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
	mu.computeNormalMatrix3x3();

	renderer.activateRenderMeshesShaderProg();

	// TODO - is Default Material, improve
	Material orangeMaterial = {
		{ 0.6f, 0.6f, 0.6f, 1.0f }, //Diffuse
		{ 0.1f, 0.1f, 0.1f, 1.0f }, //Ambient
		{ 0.15f, 0.15f, 0.15f, 1.0f }, //Specular
		{ 0.0f, 0.0f, 0.0f, 1.0f }, //Emissive
		10.0f  //Shininess
	};

	dataMesh data;
	data.meshID = m_meshId;
	data.texMode = m_textureId;
	data.vm = mu.get(gmu::VIEW_MODEL);
	data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
	data.normal = mu.getNormalMatrix();
	data.material = orangeMaterial;

	renderer.renderMeshInstanced(data, numOranges);

	mu.popMatrix(gmu::MODEL);
}

void OrangesManager::update(float deltaTime)
{
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
	}

	uploadInstanceData();
}

void OrangesManager::uploadInstanceData()
{
	std::vector<float> instancePositions;
	instancePositions.reserve(oranges.size() * 3);

	for (const Orange& orange : oranges)
	{
		instancePositions.push_back(orange.position.x);
		instancePositions.push_back(orange.position.y);
		instancePositions.push_back(orange.position.z);
	}

	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

	glBufferSubData(
		GL_ARRAY_BUFFER,
		0,
		instancePositions.size() * sizeof(float),
		instancePositions.data()
	);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
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
	std::cout << "Orange Colision" << std::endl;
}
