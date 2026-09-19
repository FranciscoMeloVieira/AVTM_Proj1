#include <ScenegraphNode.h>
#include <GL/glew.h>

struct Vector3 {
    float x, y, z;
};

class CheriosManager : public ScenegraphNode {
private:
    int m_meshId;
    int m_textureId;
    GLuint instanceVBO = 0;
    std::vector<Vector3> torusPositions;

    void addBorderLine(float x1, float z1, float x2, float z2, float spacing);
    void buildTrackBorders();

public:
    CheriosManager(int meshId, int textureId, float spacing = 10.0f);
    ~CheriosManager();

    void draw();
    void update(float deltaTime);
};