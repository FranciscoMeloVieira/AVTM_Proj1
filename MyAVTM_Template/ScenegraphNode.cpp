#include "ScenegraphNode.h"
#include "renderer.h"
#include "mathUtility.h"

extern gmu mu;
extern Renderer renderer;

ScenegraphNode::ScenegraphNode(const float* position, float angle, const float* axis, const float* scale) {
	if (position) {
		setAbsolutePosition(position[0], position[1], position[2]);
	}
	if (angle != 0.0f && axis) {
		setRotation(angle, axis[0], axis[1], axis[2]);
	}
	if (scale) {
		setScale(scale[0], scale[1], scale[2]);
	}
}

ScenegraphNode::ScenegraphNode(const float(&transformMatrix)[16]) {
	if (transformMatrix) {
		setTransform(transformMatrix);
	}
}

void ScenegraphNode::addChild(ScenegraphNode* child) {
	this->children.push_back(std::unique_ptr<ScenegraphNode>(child));
	child->parent = this;
}

void ScenegraphNode::translate(const float deltaX, const float deltaY, const float deltaZ) {
	mu.pushMatrix(gmu::MODEL);
	mu.loadMatrix(gmu::MODEL, localTransform);
	mu.translate(gmu::MODEL, deltaX, deltaY, deltaZ);
	memcpy(localTransform, mu.get(gmu::MODEL), 16 * sizeof(float));
	mu.popMatrix(gmu::MODEL);
}

void ScenegraphNode::setAbsolutePosition(const float x, const float y, const float z) {
	float currentPosition[3] = { localTransform[12], localTransform[13], localTransform[14] };
	float delta[3] = { x - currentPosition[0], y - currentPosition[1], z - currentPosition[2] };
	translate(delta[0], delta[1], delta[2]);
}

void ScenegraphNode::setRotation(float angle, const float x, const float y, const float z) {
	mu.pushMatrix(gmu::MODEL);
	mu.loadMatrix(gmu::MODEL, localTransform);
	mu.rotate(gmu::MODEL, angle, x, y, z);
	memcpy(localTransform, mu.get(gmu::MODEL), 16 * sizeof(float));
	mu.popMatrix(gmu::MODEL);
}

void ScenegraphNode::setScale(const float scaleX, const float scaleY, const float scaleZ) {
	mu.pushMatrix(gmu::MODEL);
	mu.loadMatrix(gmu::MODEL, localTransform);
	mu.scale(gmu::MODEL, scaleX, scaleY, scaleZ);
	memcpy(localTransform, mu.get(gmu::MODEL), 16 * sizeof(float));
	mu.popMatrix(gmu::MODEL);
}

void ScenegraphNode::setTransform(const float* matrix) {
	memcpy(localTransform, matrix, 16 * sizeof(float));
}

void EmptyNode::draw() {
	for (auto& child : children) {
		child->draw();
	}
}

void MeshNode::draw() {
	mu.pushMatrix(gmu::MODEL);
	mu.multMatrix(gmu::MODEL, localTransform);

	mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
	mu.computeNormalMatrix3x3();

	dataMesh data;

	data.meshID = meshID;
	data.texMode = texMode;
	data.vm = mu.get(gmu::VIEW_MODEL);
	data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
	data.normal = mu.getNormalMatrix();

	renderer.renderMesh(data);

	for (auto& child : children) {
		child->draw();
	}

	mu.popMatrix(gmu::MODEL);
}
