#pragma once
#include <CheriosManager.hpp>
#include <OrangeManager.hpp>
#include <CollisionSystem.h>
#include <Car.hpp>
#include <HUD.h>	
#include <Butter.h>
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

	float* getCarMatrix() { return car->carNode->getLocalModelMatrix(); }

	bool paused = false;

	int lives = 5;
	int points = 0;

	void decreaseLive() { lives--; }
	void addPoints(int p) { points += p; }
	void resetGame();

	CheckpointManager* checkpointManager = nullptr;

private:
};

