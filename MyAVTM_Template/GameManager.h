#pragma once
#include <CheriosManager.hpp>
#include <OrangeManager.hpp>
#include <CollisionSystem.h>
#include <Car.hpp>
#include "HUD.h"
#include <Butter.h>
#include <Candles.hpp>
#include <CheckpointManager.h>

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

private:
};