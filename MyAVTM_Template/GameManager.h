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

	void toggleControlMode();
	void setControlMode(ControlMode mode);
	ControlMode getControlMode() const { return controlMode; }

	void switchTestPath();
	void loadTestPath(int index);
	void toggleDebugVisuals();

private:
};