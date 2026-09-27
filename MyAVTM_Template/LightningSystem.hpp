#pragma once
#include <GL/glew.h>
#include <vector>

struct LightCPU {
    float position[4];
    float direction[4];
    float cosCutOff;
    float spotExp;
    int type;      // 0 = directional, 1 = point, 2 = spot
    bool activated;
};

struct LightUniformLocs {
    GLint position, direction, cosCutOff, spotExp, type, enabled;
};

class LightingSystem {
public:
    void init();
    void initUniforms(GLuint shaderProg);
    void update(const float* carMatrix);
    void sendToShader(const float* viewMatrix, GLuint shaderProg);

    /* Control the lights */
    void toggleLight(int index);
    void togglePointlights();
    void toggleSpotLights();
    void toggleDirectionalLights();
private:
    static constexpr int MAX_LIGHTS = 9;
    LightCPU lights[MAX_LIGHTS];
    std::vector<LightUniformLocs> lightLocs;
};