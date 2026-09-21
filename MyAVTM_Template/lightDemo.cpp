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
#include "CollisionManager.hpp"

using namespace std;

#define CAPTION "AVTM 2026 Welcome Demo"
int WindowHandle = 0;
int WinX = 640, WinY = 480;

unsigned int FrameCount = 0;

/*TODO - make better pls*/
CollisionManager* collisionManager = nullptr;
CarNode* carnode = nullptr;
OrangesManager* orangesManager = nullptr;
CheriosManager* cheriosManager = nullptr;
Camera* camera1 = nullptr;
Camera* camera2 = nullptr;
Camera* camera3 = nullptr;

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


// ============================================================================
// Render the scene and UI
// ============================================================================
void renderSim(void) {

	int currentTime = glutGet(GLUT_ELAPSED_TIME);
	float deltaTime = (currentTime - lastTime) / 1000.0f;
	lastTime = currentTime;
	bool isColliding = collisionManager->isCarCollidingWithCherios();
	if (isColliding)
	{
		carnode->stopCar();
	}
	carnode->update(deltaTime);
	orangesManager->update(deltaTime);
	cheriosManager->update(deltaTime);

	FrameCount++;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderer.activateRenderMeshesShaderProg(); // use the required GLSL program to draw the meshes with illumination

	if (activeCamera == camera3) {
		float* carPos = carnode->getLocalModelMatrix();
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

	// load identity matrices
	mu.loadIdentity(gmu::MODEL);
	//send the light position in eye coordinates
	renderer.setLightPos(lightPos);

	//Spotlight settings
	renderer.setSpotLightMode(spotlight_mode);
	renderer.setSpotParam(coneDir, 0.93);

	// ===== RENDER SCENEGRAPH =====
	sceneRoot->draw();

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

		case 'c': 
			printf("Camera Spherical Coordinates (%f, %f, %f)\n", alpha, _beta, r);
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

		case 'm': glEnable(GL_MULTISAMPLE); break;
		case 'n': glDisable(GL_MULTISAMPLE); break;

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
	MeshNode* floor = new MeshNode(0, 4);  // meshID=0, texMode=1
	floor->setAbsolutePosition(150.0f, 0.0f, 0.0f);
	floor->setScale(400.0f, 0.1f, 400.0f);
	floor->translate(-0.5f, -1.0f, -0.5f);  // Center cube at origin
	sceneRoot->addChild(floor);

	orangesManager = new OrangesManager(2, 3);
	cheriosManager = new CheriosManager(5, 1, 12.0f);
	collisionManager = new CollisionManager(nullptr, cheriosManager, orangesManager);
	carnode = new CarNode(collisionManager, 0, 2, nullptr, 0.0f, nullptr, nullptr);
	collisionManager->setCarNode(carnode); // Cyclic reference, but we need it to check for collisions
	sceneRoot->addChild(cheriosManager);
	sceneRoot->addChild(carnode);
	sceneRoot->addChild(orangesManager);

	/*TODO - Maybe change location*/
	camera1 = new CameraFixedOrtho(200.0f, 0.1f, 1000.0f);
	camera2 = new CameraFixedPerspective(90.0f, 0.1f, 1000.0f);
	camera3 = new CameraFixedPerspective(90.0f, 0.1f, 1000.0f);
	activeCamera = camera1;

	EmptyNode* racetrack = new EmptyNode();
	sceneRoot->addChild(racetrack);

	MeshNode* trackSegment = new MeshNode(0, 0); 
	trackSegment->setTransform(
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
		12.0f, 1.0f, 200.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);  // Center cube at origin
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 0);
	trackSegment->setTransform(
		81.0f, 0.0f, 94.0f,
		0.0f, 0.0f, 0.0f,
		150.0f, 1.0f, 12.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 0);
	trackSegment->setTransform(
		150.0f, 0.0f, 63.0f,
		0.0f, 0.0f, 0.0f,
		12.0f, 1.0f, 50.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 0);
	trackSegment->setTransform(
		94.0f, 0.0f, 44.0f,
		0.0f, 0.0f, 0.0f,
		100.0f, 1.0f, 12.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 0);
	trackSegment->setTransform(
		50.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
		12.0f, 1.0f, 75.7f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 0);
	trackSegment->setTransform(
		94.0f, 0.0f, -32.0f,
		0.0f, 0.0f, 0.0f,
		76.0f, 1.0f, 12.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 0);
	trackSegment->setTransform(
		126.0f, 0.0f, -69.0f,
		0.0f, 0.0f, 0.0f,
		12.0f, 1.0f, 62.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 0);
	trackSegment->setTransform(
		63.4f, 0.0f, -94.0f,
		0.0f, 0.0f, 0.0f,
		116.0f, 1.0f, 12.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	racetrack->setScale(2.0f, 0.1f, 2.0f);
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

	//Scene geometry with triangle meshes

	MyMesh amesh;

	float amb[] = { 0.2f, 0.15f, 0.1f, 1.0f };
	float diff[] = { 0.8f, 0.6f, 0.4f, 1.0f };
	float spec[] = { 0.8f, 0.8f, 0.8f, 1.0f };

	float amb1[] = { 0.3f, 0.0f, 0.0f, 1.0f };
	float diff1[] = { 0.8f, 0.1f, 0.1f, 1.0f };
	float spec1[] = { 0.3f, 0.3f, 0.3f, 1.0f };

	float emissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float shininess = 100.0f;
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
	glutInitDisplayMode(GLUT_DEPTH|GLUT_DOUBLE|GLUT_RGBA|GLUT_MULTISAMPLE);

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
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

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

	//  GLUT main loop
	glutMainLoop();

	return(0);
}





