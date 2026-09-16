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

private:
	orange* oranges;
	int numOranges = 10;

	int m_meshId;
	int m_textureId;
	GLuint instanceVBO = 3;
};
