#pragma once
#include "mathUtility.h"

class Camera;
class Renderer;
class EmptyNode;
class MeshNode;
class LightingSystem;

class MirrorRenderer {
public:
    void init();

    void renderRearMirror(Renderer& renderer, LightingSystem* lightingSystem, EmptyNode* sceneRoot, MeshNode* floorNode,
        const float* carMatrix, int winX, int winY);
    void renderFloorReflection(Renderer& renderer, LightingSystem* lightingSystem, EmptyNode* sceneRoot, MeshNode* floorNode);

    static void stripTranslation(const float* viewIn, float* viewOut);

private:
    void setupRearCameraView(const float* carMatrix);
    void beginStencilMask(unsigned int maskValue, unsigned int maskBits);
    void beginMaskedDraw(unsigned int maskValue, unsigned int maskBits);

    Camera* rearCamera = nullptr;
};