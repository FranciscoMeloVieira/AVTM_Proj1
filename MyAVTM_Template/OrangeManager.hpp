#include <ScenegraphNode.h>
#include <GL/glew.h>
#include "GameObject.h"

class Orange : public PhysicsObject
{
public:
	void OnCollision(const GameObject& other, const CollisionInfo& info) override;
};

class OrangesManager {

public:
	OrangesManager(int meshId, int textureId);
	~OrangesManager();
	void draw();
	void update(float deltaTime);
	void uploadInstanceData();
	int numOranges = 30;
	std::vector<Orange> oranges;

private:
	void spawnOrange(Orange& orange);
	int m_meshId;
	int m_textureId;
	GLuint instanceVBO = 3;


	float leftBorder = -50.0f, rightBorder = 350.0f, topBorder = 200.0f, bottomBorder = -200.0f;
	int minOrangeSpeed = 5, maxOrangeSpeed = 20;
};
