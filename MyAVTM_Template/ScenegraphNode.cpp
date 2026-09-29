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
	localTransform[12] = x;
	localTransform[13] = y;
	localTransform[14] = z;
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
	mu.pushMatrix(gmu::MODEL);

	mu.multMatrix(gmu::MODEL, localTransform);

	mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
	mu.computeNormalMatrix3x3();

	for (auto& child : children) {
		child->draw();
	}

	mu.popMatrix(gmu::MODEL);
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
	data.material = material;

	renderer.renderMesh(data);

	for (auto& child : children) {
		child->draw();
	}

	mu.popMatrix(gmu::MODEL);
}

void ScenegraphNode::setTransform(
	float px, float py, float pz,
	float rx, float ry, float rz,
	float sx, float sy, float sz)
{
	mu.pushMatrix(gmu::MODEL);

	mu.loadIdentity(gmu::MODEL);

	mu.translate(gmu::MODEL, px, py, pz);

	mu.rotate(gmu::MODEL, rx, 1, 0, 0);
	mu.rotate(gmu::MODEL, ry, 0, 1, 0);
	mu.rotate(gmu::MODEL, rz, 0, 0, 1);

	mu.scale(gmu::MODEL, sx, sy, sz);

	memcpy(localTransform,
		mu.get(gmu::MODEL),
		16 * sizeof(float));

	mu.popMatrix(gmu::MODEL);
}

InstancedObjectNode::InstancedObjectNode(int meshId, int textureId, Material material, const std::vector<Vector3>& positions) : meshId(meshId), textureId(textureId), material(material), positions(positions)
{
	instanceCount = static_cast<int>(positions.size());

	/*instance VBO*/
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, instanceCount * sizeof(float) * 3, positions.data(), GL_DYNAMIC_DRAW);

	glBindVertexArray(renderer.myMeshes[meshId].vao);
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glVertexAttribDivisor(3, 1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void InstancedObjectNode::draw()
{
	mu.pushMatrix(gmu::MODEL);
	mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
	mu.computeNormalMatrix3x3();

	renderer.activateRenderMeshesShaderProg();

	dataMesh data;
	data.meshID = meshId;
	data.texMode = textureId;
	data.vm = mu.get(gmu::VIEW_MODEL);
	data.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
	data.normal = mu.getNormalMatrix();
	data.material = material;
	data.model = localTransform;

	renderer.renderMeshInstanced(data, instanceCount);

	mu.popMatrix(gmu::MODEL);
}

void InstancedObjectNode::updateInstances()
{
	std::vector<float> instancePositions;
	instancePositions.reserve(positions.size() * 3);

	for (const Vector3& pos : positions)
	{
		instancePositions.push_back(pos.x);
		instancePositions.push_back(pos.y);
		instancePositions.push_back(pos.z);
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
