#include "LightningSystem.hpp"
#include <string>
#include <cmath>


void LightingSystem::init() {
	lights[0] = { {  50, 10, -150 , 1}, {0,0,0,0}, 0,0, 1, 1 };
	lights[1] = { { 300, 10, -150, 1}, {0,0,0,0}, 0,0, 1, 1 };
	lights[2] = { {  50, 10, 150,   1}, {0,0,0,0}, 0,0, 1, 1 };
	lights[3] = { { 250, 10, 150,   1}, {0,0,0,0}, 0,0, 1, 1 };
	lights[4] = { { 150, 10,  0,  1}, {0,0,0,0}, 0,0, 1, 1 };
	lights[5] = { { -50, 10, -200, 1}, {0,0,0,0}, 0,0, 1, 1 };

	lights[6] = { {0,0,0,0}, { 0.3f, 0.5f, 0.8f, 0 }, 0,0, 0, 1 }; // directional

	float cutOffCos = cosf(15.0f * 3.14159f / 180.0f);
	float spotExponent = 20.0f;

	lights[7] = { {0,0,0,1}, {0,0,-1,0}, cutOffCos, spotExponent, 2, 1 };
	lights[8] = { {0,0,0,1}, {0,0,-1,0}, cutOffCos, spotExponent, 2, 1 };
}

void LightingSystem::initUniforms(GLuint shaderProg) {
	lightLocs.resize(MAX_LIGHTS);
	for (int i = 0; i < MAX_LIGHTS; i++) {
		std::string idx = "lights[" + std::to_string(i) + "]";
		lightLocs[i].position = glGetUniformLocation(shaderProg, (idx + ".position").c_str());
		lightLocs[i].direction = glGetUniformLocation(shaderProg, (idx + ".direction").c_str());
		lightLocs[i].cosCutOff = glGetUniformLocation(shaderProg, (idx + ".cosCutOff").c_str());
		lightLocs[i].spotExp = glGetUniformLocation(shaderProg, (idx + ".spotExp").c_str());
		lightLocs[i].type = glGetUniformLocation(shaderProg, (idx + ".type").c_str());
		lightLocs[i].enabled = glGetUniformLocation(shaderProg, (idx + ".activated").c_str());
	}
}

namespace {
	void mat4MulVec4(const float* m, const float* v, float* out) {
		for (int row = 0; row < 4; row++) {
			out[row] = m[row + 0] * v[0] + m[row + 4] * v[1] +
				m[row + 8] * v[2] + m[row + 12] * v[3];
		}
	}

	void transformVec4(const float* m, float x, float y, float z, float w, float out[4]) {
		out[0] = m[0] * x + m[4] * y + m[8] * z + m[12] * w;
		out[1] = m[1] * x + m[5] * y + m[9] * z + m[13] * w;
		out[2] = m[2] * x + m[6] * y + m[10] * z + m[14] * w;
		out[3] = w;
	}
}

void LightingSystem::update(const float* carMatrix) {
	float leftX = -2.0f, rightX = 2.0f, heightY = 1.5f, frontZ = 3.5f;

	transformVec4(carMatrix, leftX, heightY, frontZ, 1.0f, lights[7].position);
	transformVec4(carMatrix, rightX, heightY, frontZ, 1.0f, lights[8].position);

	float forwardDirZ = (frontZ < 0.0f) ? -1.0f : 1.0f;
	float localDir[4];
	transformVec4(carMatrix, 0.0f, -0.15f, forwardDirZ, 0.0f, localDir);

	for (int i = 7; i <= 8; i++) {
		lights[i].direction[0] = localDir[0];
		lights[i].direction[1] = localDir[1];
		lights[i].direction[2] = localDir[2];
		lights[i].direction[3] = 0.0f;
	}
}

void LightingSystem::sendToShader(const float* viewMatrix, GLuint shaderProg) {
	for (int i = 0; i < MAX_LIGHTS; i++) {
		float posIn[4] = { lights[i].position[0], lights[i].position[1],
							lights[i].position[2], lights[i].type == 0 ? 0.0f : 1.0f };
		float dirIn[4] = { lights[i].direction[0], lights[i].direction[1],
							lights[i].direction[2], 0.0f };

		float posEye[4], dirEye[4];
		mat4MulVec4(viewMatrix, posIn, posEye);
		mat4MulVec4(viewMatrix, dirIn, dirEye);

		glUniform4fv(lightLocs[i].position, 1, posEye);
		glUniform4fv(lightLocs[i].direction, 1, dirEye);
		glUniform1f(lightLocs[i].cosCutOff, lights[i].cosCutOff);
		glUniform1f(lightLocs[i].spotExp, lights[i].spotExp);
		glUniform1i(lightLocs[i].type, lights[i].type);
		glUniform1i(lightLocs[i].enabled, lights[i].activated);
	}
}

void LightingSystem::togglePointlights()
{
	for (int i = 0; i < 6; i++)
	{
		lights[i].activated = !lights[i].activated;

	}
}

void LightingSystem::toggleSpotLights()
{
	lights[7].activated = !lights[7].activated;
	lights[8].activated = !lights[8].activated;
}

void LightingSystem::toggleDirectionalLights()
{
	lights[6].activated = !lights[6].activated;
}

bool LightingSystem::isLightActive(int lightIndex)
{
	return lights[lightIndex].activated;
}

const LightCPU& LightingSystem::getLight(int index) const {
	return lights[index];
}

void LightingSystem::sendMirroredToShader(const float* viewMatrix, GLuint shaderProg, float floorY) {
	for (int i = 0; i < MAX_LIGHTS; i++) {
		float mirroredY = floorY - (lights[i].position[1] - floorY); 
		float posIn[4] = { lights[i].position[0], mirroredY,
							lights[i].position[2], lights[i].type == 0 ? 0.0f : 1.0f };

		float mirroredDirY = -lights[i].direction[1]; // direction reflects too (for directional/spot)
		float dirIn[4] = { lights[i].direction[0], mirroredDirY, lights[i].direction[2], 0.0f };

		float posEye[4], dirEye[4];
		mat4MulVec4(viewMatrix, posIn, posEye);
		mat4MulVec4(viewMatrix, dirIn, dirEye);

		glUniform4fv(lightLocs[i].position, 1, posEye);
		glUniform4fv(lightLocs[i].direction, 1, dirEye);
		glUniform1f(lightLocs[i].cosCutOff, lights[i].cosCutOff);
		glUniform1f(lightLocs[i].spotExp, lights[i].spotExp);
		glUniform1i(lightLocs[i].type, lights[i].type);
		glUniform1i(lightLocs[i].enabled, lights[i].activated);
	}
}