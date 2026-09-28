#pragma once
#include "mathUtility.h"
#include <functional>

class Renderer;
class ScenegraphNode;

namespace ShadowSystem {
    void renderShadows(const float* lightDirection, const float* floorPlane, ScenegraphNode* node);
}