//
// AVT 2026: Texturing with Phong Shading and Text rendered with TrueType library
// The text rendering was based on https://dev.to/shreyaspranav/how-to-render-truetype-fonts-in-opengl-using-stbtruetypeh-1p5k
// You can also learn an alternative with FreeType text: https://learnopengl.com/In-Practice/Text-Rendering
// This demo was built for learning purposes only.
// Some code could be severely optimised, but I tried to
// keep as simple and clear as possible.
//
// The code comes with no warranties, use it at your own risk.
// You may use it, or parts of it, wherever you want.
// 
// Author: João Madeiras Pereira
//

#include <math.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// include GLEW to access OpenGL 3.3 functions
#include <GL/glew.h>

// GLUT is the toolkit to interface with the OS
#include <GL/freeglut.h>

#include <IL/il.h>

#include "renderer.h"
#include "shader.h"
#include "mathUtility.h"
#include "model.h"
#include "texture.h"
#include <ScenegraphNode.h>

/*TODO*/
#include "Car.hpp"
#include "OrangeManager.hpp"
#include "Camera.hpp"
#include "CameraFixedPerspective.hpp"
#include "CameraFixedOrtho.hpp"
#include "CheriosManager.hpp"
#include "CollisionSystem.h"
#include "Butter.h"

using namespace std;

#define CAPTION "AVTM 2026 Welcome Demo"
int WindowHandle = 0;
int WinX = 640, WinY = 480;

unsigned int FrameCount = 0;

/*TODO - make better pls*/
CollisionSystem* collisionManager = nullptr;
Car* carnode = nullptr;
OrangesManager* orangesManager = nullptr;
CheeriosManager* cheriosManager = nullptr;
Butter* butter = nullptr;
Camera* camera1 = nullptr;
Camera* camera2 = nullptr;
Camera* camera3 = nullptr;
Camera* rearCamera = nullptr;

Camera* activeCamera = nullptr;

//File with the font
const string fontPathFile = "fonts/arial.ttf";

//Object of class gmu (Graphics Math Utility) to manage math and matrix operations
gmu mu;

//Object of class renderer to manage the rendering of meshes and ttf-based bitmap text
Renderer renderer;
	
// Camera Position (initialize to same defaults as Camera)
float camX = 0.0f, camY = 0.0f, camZ = 0.0f;

// Camera Spherical Coordinates
float alpha = 180.0f, _beta = 45.0f;
float r = 45.0f;

// Mouse Tracking Variables
int startX, startY, tracking = 0;

// Frame counting and FPS computation
#define FPS 60
long myTime,timebase = 0,frame = 0;
char s[32];

//float lightPos[4] = {4.0f, 5.0f, 2.0f, 1.0f};
float lightPos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

//Spotlight
bool spotlight_mode = false;
float coneDir[4] = { 0.0f, -0.0f, -1.0f, 0.0f };

bool fontLoaded = false;

// Global scene graph
static EmptyNode* sceneRoot = nullptr;

bool paused = false;


/* --- Lights --- */
const int MAX_LIGHTS = 9;

struct LightCPU {
	float position[4];
	float direction[4];
	float cosCutOff;
	float spotExp;
	int type;      // 0 = directional, 1 = point, 2 = spot
	bool activated;
};

LightCPU lights[MAX_LIGHTS];

void initScene() {
	lights[0] = { {  50, 10, -150 , 1}, {0,0,0,0}, 0,0, 1, 1 };
	lights[1] = { { 250, 10, -150, 1}, {0,0,0,0}, 0,0, 1, 1 };
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

struct LightUniformLocs {
	GLint position, direction, cosCutOff, spotExp, type, enabled;
};
std::vector<LightUniformLocs> lightLocs;

void initLightUniforms(GLuint shaderProgram) {
	lightLocs.resize(MAX_LIGHTS);
	for (int i = 0; i < MAX_LIGHTS; i++) {
		std::string idx = "lights[" + std::to_string(i) + "]";
		lightLocs[i].position = glGetUniformLocation(shaderProgram, (idx + ".position").c_str());
		lightLocs[i].direction = glGetUniformLocation(shaderProgram, (idx + ".direction").c_str());
		lightLocs[i].cosCutOff = glGetUniformLocation(shaderProgram, (idx + ".cosCutOff").c_str());
		lightLocs[i].spotExp = glGetUniformLocation(shaderProgram, (idx + ".spotExp").c_str());
		lightLocs[i].type = glGetUniformLocation(shaderProgram, (idx + ".type").c_str());
		lightLocs[i].enabled = glGetUniformLocation(shaderProgram, (idx + ".activated").c_str());
	}
}

void mat4MulVec4(const float* m, const float* v, float* out) {
	for (int row = 0; row < 4; row++) {
		out[row] = m[row + 0] * v[0] +
			m[row + 4] * v[1] +
			m[row + 8] * v[2] +
			m[row + 12] * v[3];
	}
}

void transformVec4(const float* m, float x, float y, float z, float w, float out[4]) {
	out[0] = m[0] * x + m[4] * y + m[8] * z + m[12] * w;
	out[1] = m[1] * x + m[5] * y + m[9] * z + m[13] * w;
	out[2] = m[2] * x + m[6] * y + m[10] * z + m[14] * w;
	out[3] = w;
}

void sendLightsToShader(const float* viewMatrix) {

	float* m = carnode->carNode->getLocalModelMatrix();

	float leftX = -1.0f;   // Left headlight offset
	float rightX = 1.0f;   // Right headlight offset
	float heightY = 0.8f;  // Lowered to bumper/grille level
	float frontZ = 3.5f;  // Front edge of the car body

	transformVec4(m, leftX, heightY, frontZ, 1.0f, lights[7].position);

	transformVec4(m, rightX, heightY, frontZ, 1.0f, lights[8].position);

	float forwardDirZ = (frontZ < 0.0f) ? -1.0f : 1.0f;
	float localDir[4];
	transformVec4(m, 0.0f, -0.15f, forwardDirZ, 0.0f, localDir);

	for (int i = 7; i <= 8; i++) {
		lights[i].direction[0] = localDir[0];
		lights[i].direction[1] = localDir[1];
		lights[i].direction[2] = localDir[2];
		lights[i].direction[3] = 0.0f;
	}

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


/// ::::::::::::::::::::::::::::::::::::::::::::::::CALLBACK FUNCIONS:::::::::::::::::::::::::::::::::::::::::::::::::://///

void timer(int value)
{
	std::ostringstream oss;
	oss << CAPTION << ": " << FrameCount << " FPS @ (" << WinX << "x" << WinY << ")";
	std::string s = oss.str();
	glutSetWindow(WindowHandle);
	glutSetWindowTitle(s.c_str());
    FrameCount = 0;
    glutTimerFunc(1000, timer, 0);
}

void refresh(int value)
{
	//PUT YOUR CODE HERE
	glutPostRedisplay();
	glutTimerFunc(1000 / FPS, refresh, 0);
}

// ------------------------------------------------------------
//
// Reshape Callback Function
//

void changeSize(int w, int h) {
	WinX = w;
	WinY = h;
	float ratio;
	// Prevent a divide by zero, when window is too short
	if(h == 0)
		h = 1;
	// set the viewport to be the entire window
	glViewport(0, 0, w, h);
	// set the projection matrix
	ratio = (1.0f * w) / h;
	mu.loadIdentity(gmu::PROJECTION);
	activeCamera->loadProjectionMatrix(mu, ratio);
}


// ------------------------------------------------------------
//
// Render stufff
//
int lastTime = 0;


void setupRearCameraView() {
	float* carPos = carnode->carNode->getLocalModelMatrix();

	float eyeX = carPos[12];
	float eyeY = carPos[13] + 8.0f;
	float eyeZ = carPos[14];

	float dirX = carPos[8];
	float dirY = carPos[9];
	float dirZ = carPos[10];

	float targetX = eyeX - dirX * 10.0f;
	float targetY = eyeY - dirY * 10.0f;
	float targetZ = eyeZ - dirZ * 10.0f;

	rearCamera->setCameraPosition(eyeX, eyeY, eyeZ);
	rearCamera->setCameraTarget(targetX, targetY, targetZ);
	rearCamera->setCameraUpVector(0.0f, 1.0f, 0.0f);
	rearCamera->applyViewMatrix(mu);
}

void renderRearViewHUD()
{
	int hudWidth = 220;
	int hudHeight = 140;

	int hudX = (WinX - hudWidth) / 2;
	int hudY = WinY - hudHeight;

	// SETUP STENCIL MASK WRITING
	glEnable(GL_STENCIL_TEST);
	glStencilMask(0xFF);
	glClear(GL_STENCIL_BUFFER_BIT);

	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);

	mu.pushMatrix(gmu::PROJECTION);
	mu.pushMatrix(gmu::VIEW);
	mu.pushMatrix(gmu::MODEL);

	mu.loadIdentity(gmu::PROJECTION);
	mu.ortho(0, WinX, 0, WinY, -1, 1);
	mu.loadIdentity(gmu::VIEW);
	mu.loadIdentity(gmu::MODEL);

	mu.translate(gmu::MODEL, hudX + hudWidth * 0.5f, hudY + hudHeight * 0.5f, 0.0f);
	mu.scale(gmu::MODEL, (float)hudWidth, (float)hudHeight, 1.0f);

	renderer.activateRenderMeshesShaderProg();
	GLuint meshProg = renderer.getMeshShaderProg();
	mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
	glUniformMatrix4fv(glGetUniformLocation(meshProg, "m_pvm"), 1, GL_FALSE, mu.get(gmu::PROJ_VIEW_MODEL));

	static MyMesh unitQuadMesh = createQuad(1.0f, 1.0f);
	glBindVertexArray(unitQuadMesh.vao);
	glDrawElements(unitQuadMesh.type, unitQuadMesh.numIndexes, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	mu.popMatrix(gmu::MODEL);
	mu.popMatrix(gmu::VIEW);
	mu.popMatrix(gmu::PROJECTION);

	// RENDER SCENE INSIDE STENCIL MASK
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	glStencilFunc(GL_EQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilMask(0x00);

	glViewport(hudX, hudY, hudWidth, hudHeight);
	glEnable(GL_SCISSOR_TEST);
	glScissor(hudX, hudY, hudWidth, hudHeight);
	glClear(GL_DEPTH_BUFFER_BIT);

	float ratio = (float)hudWidth / (float)hudHeight;
	mu.loadIdentity(gmu::PROJECTION);
	rearCamera->loadProjectionMatrix(mu, ratio);

	mu.loadIdentity(gmu::MODEL);
	setupRearCameraView();

	sendLightsToShader(mu.get(gmu::VIEW));
	sceneRoot->draw();

	glDisable(GL_SCISSOR_TEST);
	glViewport(0, 0, WinX, WinY);
}
// ============================================================================
// Render the scene and UI
// ============================================================================
void renderSim(void) {
	if (paused == true) {
		glutPostRedisplay();
		lastTime = glutGet(GLUT_ELAPSED_TIME);
		return;
	}
	int currentTime = glutGet(GLUT_ELAPSED_TIME);
	float deltaTime = (currentTime - lastTime) / 1000.0f;
	lastTime = currentTime;
	carnode->update(deltaTime);
	orangesManager->update(deltaTime);
	cheriosManager->update(deltaTime);
	butter->update(deltaTime);
	collisionManager->updateCollisions();

	FrameCount++;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);



	renderer.activateRenderMeshesShaderProg(); // use the required GLSL program to draw the meshes with illumination
	renderRearViewHUD();
	if (activeCamera == camera3) {
		float* carPos = carnode->carNode->getLocalModelMatrix();
		activeCamera->setCameraPosition(carPos[12]+camX, carPos[13]+camY, carPos[14]+camZ);
		activeCamera->setCameraTarget(carPos[12], carPos[13], carPos[14]);
		activeCamera->setCameraUpVector(0.0f, 1.0f, 0.0f);
	}
	activeCamera->applyViewMatrix(mu);

	//Associar os Texture Units aos Objects Texture
	//stone.tga loaded in TU0; checker.tga loaded in TU1;  lightwood.tga loaded in TU2
	renderer.setTexUnit(0, 0);
	renderer.setTexUnit(1, 1);
	renderer.setTexUnit(2, 2);
	renderer.setTexUnit(3, 3);
	renderer.setTexUnit(4, 4);
	renderer.setTexUnit(5, 5);

	// load identity matrices
	mu.loadIdentity(gmu::MODEL);

	/*  new Lights method */
	sendLightsToShader(mu.get(gmu::VIEW));

	// ===== RENDER SCENEGRAPH =====
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0x00);
	sceneRoot->draw();
	glDisable(GL_STENCIL_TEST);
	// ===== RENDER TEXT =====
	if (fontLoaded) {
		glDisable(GL_DEPTH_TEST);
		TextCommand textCmd = { "AVTM 2026 Welcome:\nGood Luck!", {100, 100}, 0.5 };
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		int m_viewport[4];
		glGetIntegerv(GL_VIEWPORT, m_viewport);

		mu.loadIdentity(gmu::MODEL);
		mu.loadIdentity(gmu::VIEW);
		mu.pushMatrix(gmu::PROJECTION);
		mu.loadIdentity(gmu::PROJECTION);
		mu.ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, m_viewport[1], m_viewport[1] + m_viewport[3] - 1, -1, 1);
		mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
		textCmd.pvm = mu.get(gmu::PROJ_VIEW_MODEL);
		renderer.renderText(textCmd);
		mu.popMatrix(gmu::PROJECTION);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
	}
	glutSwapBuffers();
}

// ------------------------------------------------------------
//
// Events from the Keyboard
//

void processKeysUp(unsigned char key, int xx, int yy)
{
	switch (key) {
	case 'a':
	case 'd':
		carnode->steering = 0.0f;
		break;
	case 'w':
		carnode->isAccelerating = false;
		break;
	case 's':
		carnode->isDecelerating = false;
		break;
	}
}

void processKeys(unsigned char key, int xx, int yy)
{
	switch(key) {

		case 27:
			glutLeaveMainLoop();
			break;

		case 'l':   //toggle spotlight mode
			if (!spotlight_mode) {
				spotlight_mode = true;
				printf("Point light disabled. Spot light enabled\n");
			}
			else {
				spotlight_mode = false;
				printf("Spot light disabled. Point light enabled\n");
			}
			break;

		case 'r':    //reset
			alpha = 57.0f; _beta = 18.0f;  // Camera Spherical Coordinates
			r = 45.0f;
			camX = r * sin(alpha * 3.14f / 180.0f) * cos(_beta * 3.14f / 180.0f);
			camZ = r * cos(alpha * 3.14f / 180.0f) * cos(_beta * 3.14f / 180.0f);
			camY = r * sin(_beta * 3.14f / 180.0f);
			break;

		//case 'm': glEnable(GL_MULTISAMPLE); break;
		//case 'n': glDisable(GL_MULTISAMPLE); break;

		case 'w':
			carnode->isAccelerating = true;
			break;
		case 's':
			carnode->isDecelerating = true;
			break;
		case 'a':
			carnode->steering = 1.0f;
			break;
		case 'd':
			carnode->steering = -1.0f;
			break;

		case '1':
			activeCamera = camera1;
			changeSize(WinX, WinY);
			break;
		case '2':
			activeCamera = camera2;
			changeSize(WinX, WinY);
			break;
		case '3':
			activeCamera = camera3;
			changeSize(WinX, WinY);
			break;
		case 'p':
			paused = !paused;
			break;

		case 'n':
			lights[6].activated = !lights[6].activated;
			break;
		case 'h':
			lights[7].activated = !lights[7].activated;
			lights[8].activated = !lights[8].activated;
			break;
		case 'c':
			for (int i = 0; i < 6; i++)
			{
				lights[i].activated = !lights[i].activated;

			}
			break;
	}
}


// ------------------------------------------------------------
//
// Mouse Events
//

void processMouseButtons(int button, int state, int xx, int yy)
{
	// start tracking the mouse
	if (state == GLUT_DOWN)  {
		startX = xx;
		startY = yy;
		if (button == GLUT_LEFT_BUTTON)
			tracking = 1;
		else if (button == GLUT_RIGHT_BUTTON)
			tracking = 2;
	}

	//stop tracking the mouse
	else if (state == GLUT_UP) {
		if (tracking == 1) {
			alpha -= (xx - startX);
			_beta += (yy - startY);
		}
		else if (tracking == 2) {
			r += (yy - startY) * 0.01f;
			if (r < 0.1f)
				r = 0.1f;
		}
		tracking = 0;
	}
}

// Track mouse motion while buttons are pressed

void processMouseMotion(int xx, int yy)
{

	int deltaX, deltaY;
	float alphaAux, betaAux;
	float rAux;

	deltaX =  - xx + startX;
	deltaY =    yy - startY;

	// left mouse button: move camera
	if (tracking == 1) {


		alphaAux = alpha + deltaX;
		betaAux = _beta + deltaY;

		if (betaAux > 85.0f)
			betaAux = 85.0f;
		else if (betaAux < -85.0f)
			betaAux = -85.0f;
		rAux = r;
	}
	// right mouse button: zoom
	else if (tracking == 2) {

		alphaAux = alpha;
		betaAux = _beta;
		rAux = r + (deltaY * 0.01f);
		if (rAux < 0.1f)
			rAux = 0.1f;
	}
	if (activeCamera == camera3)
	{
		camX = rAux * sin(alphaAux * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
		camZ = rAux * cos(alphaAux * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
		camY = rAux * sin(betaAux * 3.14f / 180.0f);

	}
	
//  uncomment this if not using an idle or refresh func
//	glutPostRedisplay();
}


void mouseWheel(int wheel, int direction, int x, int y) {

	r += direction * 0.1f;
	if (r < 0.1f)
		r = 0.1f;

	camX = r * sin(alpha * 3.14f / 180.0f) * cos(_beta * 3.14f / 180.0f);
	camZ = r * cos(alpha * 3.14f / 180.0f) * cos(_beta * 3.14f / 180.0f);
	camY = r *   						     sin(_beta * 3.14f / 180.0f);

//  uncomment this if not using an idle or refresh func
//	glutPostRedisplay();
}


//
// Scene building with basic geometry
//

// ============================================================================
// Build the scene graph
// ============================================================================
void buildSceneGraph() {
	if (sceneRoot != nullptr) return;  // Already built

	// Create root node (empty, just a container)
	sceneRoot = new EmptyNode();

	// Create and add floor
	MeshNode* floor = new MeshNode(0, 6);  // meshID=0, texMode=1
	floor->setAbsolutePosition(150.0f, 0.0f, 0.0f);
	floor->setScale(400.0f, 0.1f, 400.0f);
	floor->translate(-0.5f, -1.0f, -0.5f);  // Center cube at origin
	sceneRoot->addChild(floor);

	orangesManager = new OrangesManager(2, 3);
	cheriosManager = new CheeriosManager(5, 1, 12.0f);
	collisionManager = new CollisionSystem();
	carnode = new Car(0, 2, nullptr, 0.0f, nullptr, nullptr); 
	sceneRoot->addChild(carnode->carNode);
	sceneRoot->addChild(orangesManager->orangesNode);
	sceneRoot->addChild(cheriosManager->cheeriosNode);

	collisionManager->AddGameObject(carnode);
	for (int i = 0; i < orangesManager->numOranges; i++) {
		collisionManager->AddGameObject(&orangesManager->oranges[i]);
	}
	for (int i = 0; i < cheriosManager->cheerios.size(); i++) {
		collisionManager->AddGameObject(&cheriosManager->cheerios[i]);
	}

	/*TODO - Maybe change location*/
	camera1 = new CameraFixedOrtho(200.0f, 0.1f, 1000.0f);
	camera2 = new CameraFixedPerspective(90.0f, 0.1f, 1000.0f);
	camera3 = new CameraFixedPerspective(90.0f, 0.1f, 1000.0f);
	rearCamera = new CameraFixedPerspective(90.0f, 0.1f, 1000.0f);
	activeCamera = camera1;

	EmptyNode* racetrack = new EmptyNode();
	sceneRoot->addChild(racetrack);

	MeshNode* trackSegment = new MeshNode(0, 4); 
	trackSegment->setTransform(
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
		12.0f, 1.0f, 200.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);  // Center cube at origin
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 4);
	trackSegment->setTransform(
		81.0f, 0.0f, 94.0f,
		0.0f, 0.0f, 0.0f,
		150.0f, 1.0f, 12.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 4);
	trackSegment->setTransform(
		150.0f, 0.0f, 63.0f,
		0.0f, 0.0f, 0.0f,
		12.0f, 1.0f, 50.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 4);
	trackSegment->setTransform(
		94.0f, 0.0f, 44.0f,
		0.0f, 0.0f, 0.0f,
		100.0f, 1.0f, 12.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 4);
	trackSegment->setTransform(
		50.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
		12.0f, 1.0f, 75.7f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 4);
	trackSegment->setTransform(
		94.0f, 0.0f, -32.0f,
		0.0f, 0.0f, 0.0f,
		76.0f, 1.0f, 12.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 4);
	trackSegment->setTransform(
		126.0f, 0.0f, -69.0f,
		0.0f, 0.0f, 0.0f,
		12.0f, 1.0f, 62.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 4);
	trackSegment->setTransform(
		63.4f, 0.0f, -94.0f,
		0.0f, 0.0f, 0.0f,
		116.0f, 1.0f, 12.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	racetrack->setScale(2.0f, 0.1f, 2.0f);

	butter = new Butter(0, 1, { 0.0f, 0.0f, 50.0f });
	sceneRoot->addChild(butter->butterNode);
	collisionManager->AddGameObject(butter);
}

//
// Build the scene with meshes and textures
//

void buildScene()
{
	//Texture Object definition
	renderer.TexObjArray.texture2D_Loader("assets/stone.tga");
	renderer.TexObjArray.texture2D_Loader("assets/checker.png");
	renderer.TexObjArray.texture2D_Loader("assets/lightwood.tga");
	renderer.TexObjArray.texture2D_Loader("assets/orange.png");
	renderer.TexObjArray.texture2D_Loader("assets/road.jpg");
	renderer.TexObjArray.texture2D_Loader("assets/Tire.png");

	//Scene geometry with triangle meshes

	MyMesh amesh;

	float amb[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	float diff[] = { 0.6f, 0.6f, 0.6f, 1.0f }; 
	float spec[] = { 0.15f, 0.15f, 0.15f, 1.0f }; 

	float amb1[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	float diff1[] = { 0.6f, 0.6f, 0.6f, 1.0f };
	float spec1[] = { 0.15f, 0.15f, 0.15f, 1.0f }; 

	float emissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float shininess = 10.0f; 
	int texcount = 0;

	// create geometry and VAO of the cube
	amesh = createCube();
	memcpy(amesh.mat.ambient, amb1, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff1, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec1, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO of the pawn
	amesh = createPawn();
	memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO of the sphere
	amesh = createSphere(1.0f, 20);
	memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO of the cylinder
	amesh = createCylinder(1.5f, 0.5f, 20);
	memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO of the cone
	amesh = createCone(2.5f, 1.2f, 20);
	memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	// create geometry and VAO of the torus
	amesh = createTorus(0.5f, 1.5f, 20, 20);
	memcpy(amesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(amesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(amesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(amesh.mat.emissive, emissive, 4 * sizeof(float));
	amesh.mat.shininess = shininess;
	amesh.mat.texCount = texcount;
	renderer.myMeshes.push_back(amesh);

	buildSceneGraph();

	//The truetypeInit creates a texture object in TexObjArray for storing the fontAtlasTexture
	
	fontLoaded = renderer.truetypeInit(fontPathFile);
	if (!fontLoaded)
		cerr << "Fonts not loaded\n";
	else 
		cerr << "Fonts loaded\n";

	printf("\nNumber of Texture Objects is %d\n\n", renderer.TexObjArray.getNumTextureObjects());

	// set the camera position based on its spherical coordinates
	camX = r * sin(alpha * 3.14f / 180.0f) * cos(_beta * 3.14f / 180.0f);
	camZ = r * cos(alpha * 3.14f / 180.0f) * cos(_beta * 3.14f / 180.0f);
	camY = r * sin(_beta * 3.14f / 180.0f);


}

// ------------------------------------------------------------
//
// Main function
//

int main(int argc, char **argv) {

//  GLUT initialization
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH|GLUT_DOUBLE|GLUT_RGBA|GLUT_MULTISAMPLE|GLUT_STENCIL);

	glutInitContextVersion (4, 3);
	glutInitContextProfile (GLUT_CORE_PROFILE );
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);

	glutInitWindowPosition(100,100);
	glutInitWindowSize(WinX, WinY);
	WindowHandle = glutCreateWindow(CAPTION);

//  Callback Registration
	glutDisplayFunc(renderSim);
	glutReshapeFunc(changeSize);

	glutTimerFunc(0, timer, 0);
	glutIdleFunc(renderSim);  // Use it for maximum performance
	//glutTimerFunc(0, refresh, 0);    //use it to to get 60 FPS whatever

//	Mouse and Keyboard Callbacks
	glutKeyboardFunc(processKeys);
	/*For when keys are released to stop */
	glutKeyboardUpFunc(processKeysUp);
	glutMouseFunc(processMouseButtons);
	glutMotionFunc(processMouseMotion);
	glutMouseWheelFunc ( mouseWheel ) ;
	

//	return from main loop
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

//	Init GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	// some GL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	printf ("Vendor: %s\n", glGetString (GL_VENDOR));
	printf ("Renderer: %s\n", glGetString (GL_RENDERER));
	printf ("Version: %s\n", glGetString (GL_VERSION));
	printf ("GLSL: %s\n", glGetString (GL_SHADING_LANGUAGE_VERSION));

	/* Initialization of DevIL */
	if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
	{
		printf("wrong DevIL version \n");
		exit(0);
	}
	ilInit();

	buildScene();

	if(!renderer.setRenderMeshesShaderProg("shaders/mesh.vert", "shaders/mesh.frag") || 
		!renderer.setRenderTextShaderProg("shaders/ttf.vert", "shaders/ttf.frag"))
	return(1);

	GLuint meshProg = renderer.getMeshShaderProg();
	glUseProgram(meshProg);
	glUniform1i(glGetUniformLocation(meshProg, "lightCount"), MAX_LIGHTS);
	initScene();
	initLightUniforms(meshProg);

	//  GLUT main loop
	glutMainLoop();

	return(0);
}





