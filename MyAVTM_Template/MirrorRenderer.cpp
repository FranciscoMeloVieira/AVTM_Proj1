#include "MirrorRenderer.hpp"
#include "Camera.hpp"
#include "CameraFixedPerspective.hpp"
#include "renderer.h"
#include "model.h"
#include "ScenegraphNode.h"
#include <GL/glew.h>
#include "LightningSystem.hpp"
#include <cstring>

extern gmu mu;

void MirrorRenderer::init() {
    rearCamera = new CameraFixedPerspective(90.0f, 0.1f, 1000.0f);
}

void MirrorRenderer::stripTranslation(const float* viewIn, float* viewOut) {
    memcpy(viewOut, viewIn, sizeof(float) * 16);
    viewOut[12] = 0.0f;
    viewOut[13] = 0.0f;
    viewOut[14] = 0.0f;
}

void MirrorRenderer::beginStencilMask(unsigned int maskValue, unsigned int maskBits) {
    glEnable(GL_STENCIL_TEST);
    glStencilMask(maskBits);
    glStencilFunc(GL_ALWAYS, maskValue, maskBits);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
}

void MirrorRenderer::beginMaskedDraw(unsigned int maskValue, unsigned int maskBits) {
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glStencilFunc(GL_EQUAL, maskValue, maskBits);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0x00);
}

void MirrorRenderer::setupRearCameraView(const float* carMatrix) {
    float eyeX = carMatrix[12];
    float eyeY = carMatrix[13] + 8.0f;
    float eyeZ = carMatrix[14];

    float dirX = carMatrix[8];
    float dirY = carMatrix[9];
    float dirZ = carMatrix[10];

    float targetX = eyeX - dirX * 10.0f;
    float targetY = eyeY - dirY * 10.0f;
    float targetZ = eyeZ - dirZ * 10.0f;

    rearCamera->setCameraPosition(eyeX, eyeY, eyeZ);
    rearCamera->setCameraTarget(targetX, targetY, targetZ);
    rearCamera->setCameraUpVector(0.0f, 1.0f, 0.0f);
    rearCamera->applyViewMatrix(mu);
}

void MirrorRenderer::renderRearMirror(Renderer& renderer, LightingSystem* lightingSystem, EmptyNode* sceneRoot, MeshNode* floorNode,
    const float* carMatrix, int winX, int winY) {
    int hudWidth = 220;
    int hudHeight = 140;
    int hudX = (winX - hudWidth) / 2;
    int hudY = winY - hudHeight;

    glClear(GL_STENCIL_BUFFER_BIT);
    beginStencilMask(1, 0xFF);
    glDisable(GL_DEPTH_TEST);

    mu.pushMatrix(gmu::PROJECTION);
    mu.pushMatrix(gmu::VIEW);
    mu.pushMatrix(gmu::MODEL);

    mu.loadIdentity(gmu::PROJECTION);
    mu.ortho(0, winX, 0, winY, -1, 1);
    mu.loadIdentity(gmu::VIEW);
    mu.loadIdentity(gmu::MODEL);

    mu.translate(gmu::MODEL, hudX + hudWidth * 0.5f, hudY + hudHeight * 0.5f, 0.0f);
    mu.scale(gmu::MODEL, (float)hudWidth, (float)hudHeight, 1.0f);

    renderer.activateRenderMeshesShaderProg();
    GLuint meshProg = renderer.getMeshShaderProg();
    mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
    glUniformMatrix4fv(glGetUniformLocation(meshProg, "m_pvm"), 1, GL_FALSE, mu.get(gmu::PROJ_VIEW_MODEL));
    glUniform1i(glGetUniformLocation(meshProg, "texMode"), 0);

    static MyMesh unitQuadMesh = createQuad(1.0f, 1.0f);
    glBindVertexArray(unitQuadMesh.vao);
    glDrawElements(unitQuadMesh.type, unitQuadMesh.numIndexes, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    mu.popMatrix(gmu::MODEL);
    mu.popMatrix(gmu::VIEW);
    mu.popMatrix(gmu::PROJECTION);

    beginMaskedDraw(1, 0xFF);
    glEnable(GL_DEPTH_TEST);

    glViewport(hudX, hudY, hudWidth, hudHeight);
    glEnable(GL_SCISSOR_TEST);
    glScissor(hudX, hudY, hudWidth, hudHeight);
    glClear(GL_DEPTH_BUFFER_BIT);

    float ratio = (float)hudWidth / (float)hudHeight;
    mu.loadIdentity(gmu::PROJECTION);
    rearCamera->loadProjectionMatrix(mu, ratio);

    mu.loadIdentity(gmu::MODEL);
    setupRearCameraView(carMatrix);

    lightingSystem->sendToShader(mu.get(gmu::VIEW), renderer.getMeshShaderProg());

    float viewNoTrans[16];
    stripTranslation(mu.get(gmu::VIEW), viewNoTrans);
    renderer.renderSkybox(mu.get(gmu::PROJECTION), viewNoTrans);
    renderer.activateRenderMeshesShaderProg();
    sceneRoot->draw();
    floorNode->draw();

    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, winX, winY);
}

void MirrorRenderer::renderFloorReflection(Renderer& renderer, LightingSystem* lightingSystem, EmptyNode* sceneRoot, MeshNode* floorNode) {

    beginStencilMask(0x02, 0x02);
    GLuint meshProg = renderer.getMeshShaderProg();
    glUniform1i(glGetUniformLocation(meshProg, "transparent"), GL_FALSE);
    floorNode->draw();

    beginMaskedDraw(0x02, 0x03);

    lightingSystem->sendMirroredToShader(mu.get(gmu::VIEW), meshProg);

    mu.pushMatrix(gmu::MODEL);
    mu.loadIdentity(gmu::MODEL);
    mu.scale(gmu::MODEL, 1.0f, -1.0f, 1.0f);

    glFrontFace(GL_CW);
    sceneRoot->draw();
    glFrontFace(GL_CCW);

    mu.popMatrix(gmu::MODEL);

    lightingSystem->sendToShader(mu.get(gmu::VIEW), meshProg);
}