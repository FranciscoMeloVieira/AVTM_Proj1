#pragma once
#include <ScenegraphNode.h>
#include <GL/glew.h>
#include <vector>
#include "Vector3.h"
#include "GameObject.h"


class Cheerio : public PhysicsObject {
public: 
    void OnCollision(const GameObject& other, const CollisionInfo& info) override;
};

class CheeriosManager {
private:
    int m_meshId;
    int m_textureId;
    GLuint instanceVBO = 0;

    void addBorderLine(float x1, float z1, float x2, float z2, float spacing);
    void buildTrackBorders();

public:
    CheeriosManager(int meshId, int textureId, float spacing = 10.0f);
    ~CheeriosManager();
	std::vector<Cheerio> cheerios;
	InstancedObjectNode* cheeriosNode = nullptr;

    void update(float deltaTime);
};