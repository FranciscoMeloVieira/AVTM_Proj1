#include "OrangeManager.hpp"
#include "renderer.h"

extern Renderer renderer;
extern gmu mu;

OrangesManager::OrangesManager(int meshId, int textureId) : m_meshId(meshId), m_textureId(textureId)
{
	oranges = new orange[numOranges];
	for (int i = 0; i < numOranges; ++i) {
		oranges[i].pos[0] = -500;
		oranges[i].pos[1] = 10.0f;
		oranges[i].pos[2] = -500;
		oranges[i].angle = static_cast<float>(rand() % 360);
		oranges[i].speed = static_cast<float>(rand()) / RAND_MAX * (maxOrangeSpeed - minOrangeSpeed) + minOrangeSpeed;
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
	delete[] oranges;
	if (instanceVBO != 0) {
		glDeleteBuffers(1, &instanceVBO);
	}
}

void OrangesManager::draw() {
	mu.pushMatrix(gmu::MODEL);
	mu.multMatrix(gmu::MODEL, localTransform);
	mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
	mu.computeNormalMatrix3x3();

	renderer.activateRenderMeshesShaderProg();

	dataMesh data;
	data.meshID = m_meshId;
	data.texMode = m_textureId;
	data.vm = mu.get(gmu::VIEW_MODEL);
	data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
	data.normal = mu.getNormalMatrix();

	renderer.renderMeshInstanced(data, numOranges);

	mu.popMatrix(gmu::MODEL);
}

void OrangesManager::update(float deltaTime)
{
	for(int i = 0; i < numOranges; i++)
	{
		oranges[i].pos[0] += oranges[i].speed * cos(oranges[i].angle * 3.14159f / 180.0f) * deltaTime;
		oranges[i].pos[2] += oranges[i].speed * sin(oranges[i].angle * 3.14159f	/ 180.0f) * deltaTime;

		if (oranges[i].pos[0] < leftBorder || oranges[i].pos[0] > rightBorder ||
			oranges[i].pos[2] < bottomBorder || oranges[i].pos[2] > topBorder)
		{
			int borderLine = rand() % 4;

			switch (borderLine)
			{
			case 0: // North Line (+Z border)
				oranges[i].pos[0] = leftBorder + (static_cast<float>(rand()) / RAND_MAX) * (rightBorder - leftBorder);
				oranges[i].pos[2] = topBorder;
				break;

			case 1: // South Line (-Z border)
				oranges[i].pos[0] = leftBorder + (static_cast<float>(rand()) / RAND_MAX) * (rightBorder - leftBorder);
				oranges[i].pos[2] = bottomBorder;
				break;

			case 2: // East Line (+X border)
				oranges[i].pos[0] = rightBorder;
				oranges[i].pos[2] = bottomBorder + (static_cast<float>(rand()) / RAND_MAX) * (topBorder - bottomBorder);
				break;

			case 3: // West Line (-X border)
				oranges[i].pos[0] = leftBorder;
				oranges[i].pos[2] = bottomBorder + (static_cast<float>(rand()) / RAND_MAX) * (topBorder - bottomBorder);
				break;
			}
		}
	}

	std::vector<float> instancePositions;
	instancePositions.reserve(numOranges * 3);
	for (int i = 0; i < numOranges; ++i) {
		instancePositions.push_back(oranges[i].pos[0]);
		instancePositions.push_back(oranges[i].pos[1]);
		instancePositions.push_back(oranges[i].pos[2]);
	}

	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, instancePositions.size() * sizeof(float), instancePositions.data());
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}