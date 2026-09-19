#include "CheriosManager.hpp"
#include "renderer.h"
#include <cmath>
#include <stdio.h>

extern Renderer renderer;
extern gmu mu;

CheriosManager::CheriosManager(int meshId, int textureId, float spacing)
    : m_meshId(meshId), m_textureId(textureId)
{
    buildTrackBorders();


    // Setup Instance VBO
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, torusPositions.size() * sizeof(Vector3), torusPositions.data(), GL_DYNAMIC_DRAW);

    glBindVertexArray(renderer.myMeshes[m_meshId].vao);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
    glVertexAttribDivisor(3, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

CheriosManager::~CheriosManager() {
    if (instanceVBO != 0) {
        glDeleteBuffers(1, &instanceVBO);
    }
}

void CheriosManager::addBorderLine(float x1, float z1, float x2, float z2, float spacing) {
    float dx = x2 - x1;
    float dz = z2 - z1;
    float length = std::sqrt(dx * dx + dz * dz);

    // Safety check to prevent division by zero on zero-length lines
    if (length < 0.001f) return;

    int count = static_cast<int>(length / spacing);
    if (count == 0) count = 1;

    for (int i = 0; i <= count; ++i) {
        float t = static_cast<float>(i) / count;
        torusPositions.push_back({ x1 + t * dx, 2.0f, z1 + t * dz });
    }
}

void CheriosManager::buildTrackBorders() {
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

void CheriosManager::draw() {

    glBindVertexArray(renderer.myMeshes[m_meshId].vao);
    glEnableVertexAttribArray(3);

    if (torusPositions.empty()) return;

    mu.pushMatrix(gmu::MODEL);

    mu.loadIdentity(gmu::MODEL);

    mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
    mu.computeNormalMatrix3x3();

    renderer.activateRenderMeshesShaderProg();

    dataMesh data;
    data.meshID = m_meshId;
    data.texMode = m_textureId;
    data.vm = mu.get(gmu::VIEW_MODEL);
    data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
    data.normal = mu.getNormalMatrix();

    renderer.renderMeshInstanced(data, static_cast<int>(torusPositions.size()));

    mu.popMatrix(gmu::MODEL);
}

void CheriosManager::update(float deltaTime) {
    if (torusPositions.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, torusPositions.size() * sizeof(Vector3), torusPositions.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

