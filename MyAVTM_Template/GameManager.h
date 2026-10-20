#pragma once
#include <CheriosManager.hpp>
#include <OrangeManager.hpp>
#include <CollisionSystem.h>
#include <Car.hpp>
#include "HUD.h"
#include <Butter.h>
#include <Candles.hpp>
#include <CheckpointManager.h>
#include "Path.h"
#include "PathFollower.h"
#include "PathVisualizer.h"
#include "PathEditor.h"
#include "IPathInputController.h"
#include "MouseInputController.h"
#include "ArduinoInputController.h"
#include "PS4GyroInputController.h"
#include <memory>
#include <string>
#include <vector>

enum class ControlMode
{
	WASD,
	PathFollowing
};

class GameManager
{
public:
	GameManager(EmptyNode* sceneRoot);
	~GameManager();
	void init();
	void update(float deltaTime);

	EmptyNode* sceneRoot;
	HUD* hud;

	CollisionSystem* collisionSystem;
	OrangesManager* orangesManager;
	CheeriosManager* cheeriosManager;
	Car* car;
	Butter* butter;
	Candles* candles;
	EmptyNode* racetrackNode = nullptr;
	void setRacetrackNode(EmptyNode* node) { racetrackNode = node; }

	float* getCarMatrix() { return car->carNode->getLocalModelMatrix(); }
	void toggleRearMirror() { rearMirrorRender = !rearMirrorRender; }
	bool shouldRearMirrorRender() { return rearMirrorRender; }
	void toggleFog();

	bool paused = false;

	bool rearMirrorRender = true;
	bool fogActive = true;

	int lives = 5;
	int points = 0;

	void takeDamage() {
		lives--;
		if (hud) {
			hud->setLives(lives);
			if (lives <= 0) hud->setGameOver(true);
		}
	}
	void addPoints(int p) {
		points += p;
		if (hud) {
			hud->setPoints(points);
		}
	}
	void resetGame();

	CheckpointManager* checkpointManager = nullptr;

	// Path Following AI (Stage 1)
	ControlMode controlMode = ControlMode::WASD;
	Path activePath;
	PathFollower pathFollower;
	PathVisualizer* pathVisualizer = nullptr;
	int currentPathIndex = 0;
	bool debugVisualsEnabled = true;
	bool showPathEditorHudDetails = false;

	void toggleControlMode();
	void setControlMode(ControlMode mode);
	ControlMode getControlMode() const { return controlMode; }

	void switchTestPath();
	void loadTestPath(int index);
	void toggleDebugVisuals();

	// Path Editor (Stage 2 & Stage 3)
	PathEditor pathEditor;
	bool isEditingPath() const { return pathEditor.getState() != EditorState::Idle; }
	void enterPathEditor();
	void exitPathEditor();
	void togglePathEditor();
	void confirmEditorPath();
	void clearEditorPath();
	void toggleEditorClosedLoop();
	void toggleEditorSmoothing();
	void toggleTrackReference();
	void applyTrackReferenceVisibility();

	// Mouse event forwarding
	void onEditorMouseDown(int x, int y, int winW, int winH);
	void onEditorMouseDrag(int x, int y, int winW, int winH);
	void onEditorMouseUp(int x, int y, int winW, int winH);

	// Unified Input Controller Management (Stage 3)
	void setPathInputMode(PathInputMode mode);
	PathInputMode getPathInputMode() const { return m_activeInputMode; }
	void cyclePathInputMode();
	IPathInputController* getActiveInputController();
	const IPathInputController* getActiveInputController() const;

	void toggleActiveInputConnection();
	void calibrateActiveInput();
	void startActiveInputRecording();
	void stopActiveInputRecording();
	void resetActiveInputCursor();
	bool isActiveInputRecording() const;

	// Backward-compatible delegates for existing Arduino calls
	void toggleArduinoConnection();
	void calibrateArduinoInput();
	void startArduinoRecording();
	void stopArduinoRecording();
	void resetArduinoCursor();
	bool isArduinoRecording() const;

	// Access to specific controllers for tests / configuration
	ArduinoInputController* getArduinoController();
	PS4GyroInputController* getPS4Controller();

private:
	std::vector<std::unique_ptr<IPathInputController>> m_inputControllers;
	PathInputMode m_activeInputMode = PathInputMode::Mouse;

	void initInputControllers();
};
