#pragma once

#include <memory>
#include <vector>
#include <math.h>
#include "mathUtility.h"
#include "model.h"
#include <Vector3.h>

class Renderer;
class gmu;

class ScenegraphNode {
public:
	ScenegraphNode() = default;
	ScenegraphNode(const float* position, float angle, const float* axis, const float* scale);
	ScenegraphNode(const float(&transformMatrix)[16]);
	virtual ~ScenegraphNode() = default;

	void addChild(ScenegraphNode* child);
	virtual void draw() = 0;

	void setAbsolutePosition(const float x, const float y, const float z);
	void setRotation(float angle, const float x, const float y, const float z);
	void setScale(const float scaleX, const float scaleY, const float scaleZ);
	void setTransform(const float* matrix);
	/*TODO - test*/
	void ScenegraphNode::setTransform(
		float px, float py, float pz,
		float rx, float ry, float rz,
		float sx, float sy, float sz);

	void translate(const float deltaX, const float deltaY, const float deltaZ);

	ScenegraphNode* getParent() const { return parent; }
	void setVisibility(bool _visible) { isVisible = _visible; }
	float* getLocalModelMatrix() { return localTransform; }

protected:
	float localTransform[16] = { 1.0f, 0.0f, 0.0f, 0.0f,
									   0.0f, 1.0f, 0.0f, 0.0f,
									   0.0f, 0.0f, 1.0f, 0.0f,
									   0.0f, 0.0f, 0.0f, 1.0f };

	std::vector<std::unique_ptr<ScenegraphNode>> children;
	ScenegraphNode* parent = nullptr;

	bool isVisible = true;
};

class EmptyNode : public ScenegraphNode {
public:
	EmptyNode(const float* position = nullptr, 
			  float angle = 0.0f, 
			  const float* axis = nullptr,
			  const float* scale = nullptr) : ScenegraphNode(position, angle, axis, scale) {}
	EmptyNode(const float(&transformMatrix)[16]) : ScenegraphNode(transformMatrix) {}

	void draw() override;
};

class MeshNode : public ScenegraphNode {
public:
	MeshNode(int meshID,
			 const float* position = nullptr,
			 float angle = 0.0f,
			 const float* axis = nullptr,
		const float* scale = nullptr) : ScenegraphNode(position, angle, axis, scale), meshID(meshID) {}
	MeshNode(int meshID, const float(&transformMatrix)[16]) : ScenegraphNode(transformMatrix), meshID(meshID) {}
	MeshNode(int meshID, int texMode) : ScenegraphNode(), meshID(meshID), texMode(texMode) {}

	void draw() override;
	void setTexMode(int _texMode) { texMode = _texMode; }
	int getTexMode() const { return texMode; }
	void setMeshID(int _meshID) { meshID = _meshID; }
	int getMeshID() const { return meshID; }

private:
	int meshID = 0;
	int texMode = 1;
};

class InstancedObjectNode : public ScenegraphNode
{
public:
	InstancedObjectNode(int meshId, int textureId, const std::vector<std::pair<Vector3, Vector3>>& positions);
	void draw() override;

	void setPositions(const std::vector<std::pair<Vector3, Vector3>>& newPositions) { positions = newPositions; instanceCount = static_cast<int>(positions.size()); updateInstances(); }

private:
	GLuint instanceVBO;

	std::vector<std::pair<Vector3, Vector3>> positions;
	int instanceCount;

	int meshId;
	int textureId;

	void updateInstances();
};


