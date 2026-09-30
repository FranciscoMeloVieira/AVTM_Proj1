#include <ScenegraphNode.h>
#include <GL/glew.h>
#include "GameObject.h"

class Candle : public PhysicsObject
{
public:
	void OnCollision(const GameObject& other, const CollisionInfo& info) override;
};

class Candles {

public:
	Candles(int meshId, int textureId);
	~Candles();
	void update(float deltaTime);
	void uploadInstanceData();
	int numOranges = 30;
	InstancedObjectNode* candleNode = nullptr;

private:
	int m_meshId;
	int m_textureId;
	GLuint instanceVBO = 4;
};
