#pragma once
#include <ScenegraphNode.h>
#include "Collision.h"
#include "GameObject.h"

#define BUTTER_WIDTH 5.0f
#define BUTTER_HEIGHT 5.0f
#define BUTTER_DEPTH 5.0f
#define BUTTER_SPEED 5.0f
#define BUTTER_TEXTURE_MODE 4



class Butter : public PhysicsObject
{
public:
	Butter(int meshID, int texMode, Vector3 pos);
	MeshNode* butterNode;
	void update(float deltaTime);
	void OnCollision(const GameObject& other, const CollisionInfo& info) override;
	void setPosition(float x, float y, float z) {
		butterNode->setAbsolutePosition(x, y, z);
		updateColliderPosition();
	}

private:
	void updateColliderPosition() {
		const float* matrix = butterNode->getLocalModelMatrix();
		collider->position = { matrix[12], matrix[13], matrix[14] };
	}
};