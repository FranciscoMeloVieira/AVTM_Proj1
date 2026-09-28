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
// Author: Jo�o Madeiras Pereira
//

/*TODO*/
// E suposto ter o culling ligado 24/7 por causa da transparencia
// Chão transparente faz com que a skybox passe, e por baixo dá para ver
// Há problema de na rearMirror não haja objetos transparentes?




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
#include "Camera.hpp"
#include "CameraFixedPerspective.hpp"
#include "CameraFixedOrtho.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include "meshFromAssimp.h"
#include <GameManager.h>
#include "LightningSystem.hpp"
#include "InputManager.hpp"
#include "MirrorRenderer.hpp"
#include "ShadowSystem.hpp"

using namespace std;

#define CAPTION "AVTM 2026 Welcome Demo"
int WindowHandle = 0;
int WinX = 640, WinY = 480;

unsigned int FrameCount = 0;

/*TODO - make better pls*/
GameManager* gameManager = nullptr;
LightingSystem* lightningSystem = nullptr;
InputManager* inputManager = nullptr;
MirrorRenderer* mirrorRenderer = nullptr;

Camera* camera1 = nullptr;
Camera* camera2 = nullptr;
Camera* camera3 = nullptr;

//File with the font
const string fontPathFile = "fonts/arial.ttf";

//Object of class gmu (Graphics Math Utility) to manage math and matrix operations
gmu mu;

//Object of class renderer to manage the rendering of meshes and ttf-based bitmap text
Renderer renderer;

// Frame counting and FPS computation
#define FPS 60

bool fontLoaded = false;

// Global scene graph
static EmptyNode* sceneRoot = nullptr;
static MeshNode* floorNode = nullptr;

Material defaultMaterial = {
	{ 0.6f, 0.6f, 0.6f, 1.0f }, //Diffuse
	{ 0.1f, 0.1f, 0.1f, 1.0f }, //Ambient
	{ 0.15f, 0.15f, 0.15f, 1.0f }, //Specular
	{ 0.0f, 0.0f, 0.0f, 1.0f }, //Emissive
	10.0f  //Shininess
};

Material asphaltMaterial = { //Based on black rubber
	{ 0.01f, 0.01f, 0.01f, 1.0f }, //Diffuse
	{ 0.02f, 0.02f, 0.02f, 1.0f }, //Ambient
	{ 0.04f, 0.04f, 0.04f, 1.0f }, //Specular
	{ 0.0f, 0.0f, 0.0f, 1.0f }, //Emissive
	(0.08f * 128.0f)  //Shininess
};


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
	inputManager->getActiveCamera()->loadProjectionMatrix(mu, ratio);
}


void processKeys(unsigned char key, int x, int y) {
	if (key == 27) { glutLeaveMainLoop(); return; }
	inputManager->onKeyDown(key, x, y);
	if (inputManager->consumeActiveCameraChanged())
		changeSize(WinX, WinY);
}
void processKeysUp(unsigned char key, int x, int y) {
	inputManager->onKeyUp(key, x, y);
}
void processMouseButtons(int button, int state, int x, int y) {
	inputManager->onMouseButton(button, state, x, y);
}
void processMouseMotion(int x, int y) {
	inputManager->onMouseMotion(x, y);
}
void mouseWheel(int wheel, int direction, int x, int y) {
	inputManager->onMouseWheel(wheel, direction, x, y);
}

int lastTime = 0;
// ============================================================================
// Render the scene and UI
// ============================================================================
void renderSim(void) {
	int currentTime = glutGet(GLUT_ELAPSED_TIME);
	float deltaTime = (currentTime - lastTime) / 1000.0f;
	lastTime = currentTime;
	gameManager->update(deltaTime);
	lightningSystem->update(gameManager->getCarMatrix());

	FrameCount++;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);



	renderer.activateRenderMeshesShaderProg(); // use the required GLSL program to draw the meshes with illumination
	mirrorRenderer->renderRearMirror(renderer, lightningSystem, sceneRoot, floorNode, gameManager->getCarMatrix(), WinX, WinY);
	float currentCamY = 0.0f;
	if (inputManager->getActiveCamera() == camera3) {
		float* carPos = gameManager->getCarMatrix();
		inputManager->getActiveCamera()->setCameraPosition(carPos[12] + inputManager->getCamX(), carPos[13] + inputManager->getCamY(), carPos[14] + inputManager->getCamZ());
		inputManager->getActiveCamera()->setCameraTarget(carPos[12], carPos[13], carPos[14]);
		inputManager->getActiveCamera()->setCameraUpVector(0.0f, 1.0f, 0.0f);
		currentCamY = carPos[13] + inputManager->getCamY();
	}
	inputManager->getActiveCamera()->applyViewMatrix(mu);
	inputManager->getActiveCamera()->loadProjectionMatrix(mu, (1.0f * WinX) / WinY);

	lightningSystem->sendToShader(mu.get(gmu::VIEW), renderer.getMeshShaderProg());

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
	lightningSystem->sendToShader(mu.get(gmu::VIEW), renderer.getMeshShaderProg());

	// ===== RENDER SCENEGRAPH =====
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 0x01, 0x01);
	glStencilMask(0x00);
	float viewNoTrans[16];
	MirrorRenderer::stripTranslation(mu.get(gmu::VIEW), viewNoTrans);
	renderer.renderSkybox(mu.get(gmu::PROJECTION), viewNoTrans);
	renderer.activateRenderMeshesShaderProg();
	if (currentCamY > 0)
		mirrorRenderer->renderFloorReflection(renderer, lightningSystem, sceneRoot, floorNode);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	floorNode->draw();

	if (currentCamY > 0 && lightningSystem->isLightActive(6)) {
		float floorPlane[4] = { 0, 1, 0, 0 };
		const float* dirLightDirection = lightningSystem->getLight(6).direction; // w must be 0

		glBlendFunc(GL_DST_COLOR, GL_ZERO);
		ShadowSystem::renderShadows(dirLightDirection, floorPlane, sceneRoot);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glDisable(GL_BLEND);
	GLuint meshProg = renderer.getMeshShaderProg();
	GLint isTransparentLoc = glGetUniformLocation(meshProg, "transparent");
	glUniform1i(isTransparentLoc, GL_FALSE);
	sceneRoot->draw();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUniform1i(isTransparentLoc, GL_TRUE);
	sceneRoot->draw();

	glDepthFunc(GL_LESS);
	glDisable(GL_STENCIL_TEST);
	glUniform1i(isTransparentLoc, GL_FALSE);
	// ===== RENDER TEXT =====
	if (fontLoaded && gameManager && gameManager->hud) {
		gameManager->hud->render(renderer);
	}
	glutSwapBuffers();
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
	MeshNode* floor = new MeshNode(0, 10, defaultMaterial);  // meshID=0, texMode=1
	floor->setAbsolutePosition(150.0f, 0.0f, 0.0f);
	floor->setScale(400.0f, 0.1f, 400.0f);
	floor->translate(-0.5f, -1.0f, -0.5f);  // Center cube at origin
	floorNode = floor;

	gameManager = new GameManager(sceneRoot);


	/*TODO - Maybe change location*/
	camera1 = new CameraFixedOrtho(200.0f, 0.1f, 1000.0f);
	camera2 = new CameraFixedPerspective(90.0f, 0.1f, 1000.0f);
	camera3 = new CameraFixedPerspective(90.0f, 0.1f, 1000.0f);

	EmptyNode* racetrack = new EmptyNode();
	sceneRoot->addChild(racetrack);

	MeshNode* trackSegment = new MeshNode(0, 4, asphaltMaterial); 
	trackSegment->setTransform(
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
		12.0f, 1.0f, 200.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);  // Center cube at origin
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 4, asphaltMaterial);
	trackSegment->setTransform(
		81.0f, 0.0f, 94.0f,
		0.0f, 0.0f, 0.0f,
		150.0f, 1.0f, 12.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 4, asphaltMaterial);
	trackSegment->setTransform(
		150.0f, 0.0f, 63.0f,
		0.0f, 0.0f, 0.0f,
		12.0f, 1.0f, 50.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 4, asphaltMaterial);
	trackSegment->setTransform(
		94.0f, 0.0f, 44.0f,
		0.0f, 0.0f, 0.0f,
		100.0f, 1.0f, 12.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 4, asphaltMaterial);
	trackSegment->setTransform(
		50.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
		12.0f, 1.0f, 75.7f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 4, asphaltMaterial);
	trackSegment->setTransform(
		94.0f, 0.0f, -32.0f,
		0.0f, 0.0f, 0.0f,
		76.0f, 1.0f, 12.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 4, asphaltMaterial);
	trackSegment->setTransform(
		126.0f, 0.0f, -69.0f,
		0.0f, 0.0f, 0.0f,
		12.0f, 1.0f, 62.0f);
	trackSegment->translate(-0.5f, -0.5f, -0.5f);
	racetrack->addChild(trackSegment);

	trackSegment = new MeshNode(0, 4, asphaltMaterial);
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

char model_dir[50];

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
	amesh = createCylinder(1.5f*2.0f, 1.0f*0.5f, 20);
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

	Assimp::Importer carImporter;
	const aiScene* carScene = nullptr;
	float carScaleFactor;

	strcpy(model_dir, "assets/car/"); // Import3DFromFile expects this global set beforehand
	if (!Import3DFromFile("assets/car/car.obj", carImporter, carScene, carScaleFactor)) {
		cerr << "Failed to load car.obj\n";
	}

	createMyMeshFromAssimp(carScene, &renderer);

	buildSceneGraph();

	//The truetypeInit creates a texture object in TexObjArray for storing the fontAtlasTexture
	
	fontLoaded = renderer.truetypeInit(fontPathFile);
	if (!fontLoaded)
		cerr << "Fonts not loaded\n";
	else 
		cerr << "Fonts loaded\n";

	printf("\nNumber of Texture Objects is %d\n\n", renderer.TexObjArray.getNumTextureObjects());
}

unsigned int loadCubemap(vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	ILuint imageID;
	ilGenImages(1, &imageID);
	ilBindImage(imageID);

	for (unsigned int i = 0; i < faces.size(); i++)
	{
		if (ilLoadImage((const ILstring)faces[i].c_str()))
		{
			ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);

			int width = ilGetInteger(IL_IMAGE_WIDTH);
			int height = ilGetInteger(IL_IMAGE_HEIGHT);
			unsigned char* data = ilGetData();

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
		}
		else
		{
			std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
		}
	}

	ilDeleteImages(1, &imageID);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	return textureID;
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

	vector<std::string> skyboxFaces = {
	"assets/skybox/right.jpg",
	"assets/skybox/left.jpg",
	"assets/skybox/top.jpg",
	"assets/skybox/bottom.jpg",
	"assets/skybox/front.jpg",
	"assets/skybox/back.jpg"
	};
	GLuint skyboxTex = loadCubemap(skyboxFaces);
	renderer.setSkyboxCubemap(skyboxTex);
	renderer.initSkyboxGeometry();

	if(!renderer.setRenderMeshesShaderProg("shaders/mesh.vert", "shaders/mesh.frag") || 
		!renderer.setRenderTextShaderProg("shaders/ttf.vert", "shaders/ttf.frag") || 
		!renderer.setRenderSkyboxShaderProg("shaders/skybox.vert", "shaders/skybox.frag"))
	return(1);

	GLuint meshProg = renderer.getMeshShaderProg();
	lightningSystem = new LightingSystem();
	lightningSystem->init();
	lightningSystem->initUniforms(meshProg);
	inputManager = new InputManager();
	inputManager->setLightingSystem(lightningSystem);
	inputManager->setGameManager(gameManager);
	inputManager->setCameras(camera1, camera2, camera3);
	inputManager->init();
	mirrorRenderer = new MirrorRenderer();
	mirrorRenderer->init();

	//  GLUT main loop
	glutMainLoop();

	return(0);
}