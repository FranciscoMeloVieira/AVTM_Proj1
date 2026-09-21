#include <ScenegraphNode.h>
#include <GL/glew.h>

static struct orange
{
	float pos[3];
	float angle;
	float speed;
};

class OrangesManager : public ScenegraphNode {

public:
	OrangesManager(int meshId, int textureId);
	~OrangesManager();
	void draw() override;
	void update(float deltaTime);

	orange* oranges;
	int numOranges = 30;

private:
	int m_meshId;
	int m_textureId;
	GLuint instanceVBO = 3;

	int leftBorder = -50, rightBorder = 350, topBorder = 200, bottomBorder = -200;
	int minOrangeSpeed = 5, maxOrangeSpeed = 20;
};
