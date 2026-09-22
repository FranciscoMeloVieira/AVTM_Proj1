#pragma once
#include <ScenegraphNode.h>
#include <GL/glew.h>
#include <vector>
#include "Vector3.h"

class CheriosManager : public ScenegraphNode {
private:
    int m_meshId;
    int m_textureId;
    GLuint instanceVBO = 0;

    void addBorderLine(float x1, float z1, float x2, float z2, float spacing);
    void buildTrackBorders();

public:
    CheriosManager(int meshId, int textureId, float spacing = 10.0f);
    ~CheriosManager();
    std::vector<Vector3> torusPositions;

    void draw();
    void update(float deltaTime);
};