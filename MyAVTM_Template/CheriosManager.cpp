#include "CheriosManager.hpp"
#include "renderer.h"
#include <cmath>
#include <stdio.h>
#include <iostream>

extern Renderer renderer;
extern gmu mu;

CheeriosManager::CheeriosManager(int meshId, int textureId, float spacing)
    : m_meshId(meshId), m_textureId(textureId)
{
    buildTrackBorders();


    // Setup Instance VBO
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

	Vector3* instancePositions = new Vector3[cheerios.size()];
	for (size_t i = 0; i < cheerios.size(); ++i) {
		instancePositions[i] = cheerios[i].position;
	}

    glBufferData(GL_ARRAY_BUFFER, cheerios.size() * sizeof(Vector3), instancePositions, GL_DYNAMIC_DRAW);

    glBindVertexArray(renderer.myMeshes[m_meshId].vao);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
    glVertexAttribDivisor(3, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

CheeriosManager::~CheeriosManager() {
    if (instanceVBO != 0) {
        glDeleteBuffers(1, &instanceVBO);
    }
}

void CheeriosManager::addBorderLine(float x1, float z1, float x2, float z2, float spacing) {
    float dx = x2 - x1;
    float dz = z2 - z1;
    float length = std::sqrt(dx * dx + dz * dz);

    // Safety check to prevent division by zero on zero-length lines
    if (length < 0.001f) return;

    int count = static_cast<int>(length / spacing);
    if (count == 0) count = 1;

    for (int i = 0; i <= count; ++i) {
        float t = static_cast<float>(i) / count;
		Cheerio cheerio;
		cheerio.position.x = x1 + t * dx;
		cheerio.position.y = 3.0f; // Fixed height
		cheerio.position.z = z1 + t * dz;
		cheerio.collider = new Collider();
		cheerio.collider->position = cheerio.position;
		cheerio.collider->max = { 1.0f, 1.0f, 1.0f };
		cheerio.collider->min = { -1.0f, -1.0f, -1.0f };
		cheerio.mass = 1.0f;
		cheerios.push_back(cheerio);
    }
}

void CheeriosManager::buildTrackBorders() {
    float spacing = 6.0f;

    struct Line {
        float x1, z1, x2, z2;
    };

    Line trackLines[] = {
    { -12.0f, -200.0f,  -12.0f,  200.0f },
    { -12.0f,  200.0f,  312.0f,  200.0f },
    { 312.0f,  200.0f,  312.0f,   76.0f },
    { 312.0f,   76.0f,  112.0f,   76.0f },
    { 112.0f,   76.0f,  112.0f,  -52.0f },
    { 112.0f,  -52.0f,  264.0f,  -52.0f },
    { 264.0f,  -52.0f,  264.0f, -200.0f },
    { 264.0f, -200.0f,  -12.0f, -200.0f },

    {  12.0f, -176.0f,   12.0f,  176.0f },
    {  12.0f,  176.0f,  288.0f,  176.0f },
    { 288.0f,  176.0f,  288.0f,  100.0f },
    { 288.0f,  100.0f,   88.0f,  100.0f },
    {  88.0f,  100.0f,   88.0f,  -76.0f },
    {  88.0f,  -76.0f,  240.0f,  -76.0f },
    { 240.0f,  -76.0f,  240.0f, -176.0f },
    { 240.0f, -176.0f,   12.0f, -176.0f }
    };

    for (const auto& line : trackLines) {
        addBorderLine(line.x1, line.z1, line.x2, line.z2, spacing);
    }
}

void CheeriosManager::draw() {

    glBindVertexArray(renderer.myMeshes[m_meshId].vao);
    glEnableVertexAttribArray(3);

    if (cheerios.empty()) return;

    mu.pushMatrix(gmu::MODEL);

    mu.loadIdentity(gmu::MODEL);

    mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
    mu.computeNormalMatrix3x3();

    renderer.activateRenderMeshesShaderProg();

    // TODO - is Default Material, improve
    Material cheerioMaterial = {
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
    data.material = cheerioMaterial;
    data.normal = mu.getNormalMatrix();

    renderer.renderMeshInstanced(data, static_cast<int>(cheerios.size()));

    mu.popMatrix(gmu::MODEL);
}

void CheeriosManager::update(float deltaTime) {
    if (cheerios.empty()) return;

	/*for (Cheerio& cheerio : cheerios) {
		cheerio.position += cheerio.velocity * deltaTime;
		cheerio.collider->position = cheerio.position;
	}*/

   /* glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, cheerios.size() * sizeof(Vector3), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);*/
}

void Cheerio::OnCollision(const GameObject& other, const CollisionInfo& info)
{
	std::cout << "Cheerio collided with another object!" << std::endl;
}
