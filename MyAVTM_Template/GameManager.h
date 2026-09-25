#pragma once
#include <CheriosManager.hpp>
#include <OrangeManager.hpp>
#include <CollisionSystem.h>
#include <Car.hpp>

class GameManager
{
public:
	GameManager();
	~GameManager();
	void init();
	void update(float deltaTime);

	EmptyNode* sceneRoot;

	CollisionSystem* collisionSystem;
	OrangesManager* orangesManager;
	CheeriosManager* cheeriosManager;
	Car* car;

	bool paused = false;

	int lives = 5;
	int points = 0;

	void decreaseLive() { lives--; }
	void addPoints(int p) { points += p; }
	void resetGame();


private:
};

