#include "ShadowSystem.hpp"
#include "ScenegraphNode.h"
#include <GL/glew.h>
#include <cassert>

extern gmu mu;

namespace ShadowSystem {
    void renderShadows(const float* lightDirection, const float* floorPlane, ScenegraphNode* node)
    {
            float shadowMat[16];
            mu.shadow_matrix(shadowMat, const_cast<float*>(floorPlane), const_cast<float*>(lightDirection));

            glDisable(GL_DEPTH_TEST);

            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

            mu.pushMatrix(gmu::MODEL);
            mu.multMatrix(gmu::MODEL, shadowMat);
           
            node->draw();

            mu.popMatrix(gmu::MODEL);

            glEnable(GL_DEPTH_TEST);
    }
}
