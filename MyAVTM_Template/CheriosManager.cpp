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

	std::vector<Vector3> positions;

	for (const Cheerio& cheerio : cheerios) {
		positions.push_back(cheerio.position);
	}

	cheeriosNode = new InstancedObjectNode(m_meshId, m_textureId, positions);
}

CheeriosManager::~CheeriosManager() {
  
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

void CheeriosManager::update(float deltaTime) {
    if (cheerios.empty()) return;

    /*std::vector<Vector3> positions;

    for (const Cheerio& cheerio : cheerios) {
        positions.push_back(cheerio.position);
    }

	cheeriosNode->setPositions(positions);*/
}

void Cheerio::OnCollision(const GameObject& other, const CollisionInfo& info)
{
}
