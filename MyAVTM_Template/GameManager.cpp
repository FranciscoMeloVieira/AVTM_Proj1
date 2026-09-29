#include "GameManager.h"

GameManager::GameManager(EmptyNode* sceneRoot)
{
	this->sceneRoot = sceneRoot;
	hud = new HUD();
	this->init();
}

void GameManager::init()
{
	collisionSystem = new CollisionSystem();

	orangesManager = new OrangesManager(2, 3);
	cheeriosManager = new CheeriosManager(5, 1, 12.0f);
	car = new Car(0, 2, nullptr, 0.0f, nullptr, nullptr);
	car->onOrangeCollision.addListener([this](int damage) {
		this->takeDamage();
		});
	candles = new Candles(7,0);

	butter = new Butter(0, 1, { 0.0f, 0.0f, 50.0f });

	//Add Objects to the scene graph
	sceneRoot->addChild(car->carNode);
	sceneRoot->addChild(orangesManager->orangesNode);
	sceneRoot->addChild(cheeriosManager->cheeriosNode);
	sceneRoot->addChild(butter->butterNode);
	sceneRoot->addChild(candles->candleNode);

	checkpointManager = new CheckpointManager({
		{ 0.0f, 0.0f, 50.0f },
		{ 0.0f, 0.0f, -30.0f }
		});
	checkpointManager->onCheckpointCrossed.addListener([this](int points) {
		this->addPoints(points);
		});

	//Add Objects to the collision system
	collisionSystem->AddGameObject(car);
	for (int i = 0; i < orangesManager->numOranges; i++) {
		collisionSystem->AddGameObject(&orangesManager->oranges[i]);
	}
	for (int i = 0; i < cheeriosManager->cheerios.size(); i++) {
		collisionSystem->AddGameObject(&cheeriosManager->cheerios[i]);
	}
	collisionSystem->AddGameObject(butter);
	for (int i = 0; i < checkpointManager->checkpoints.size(); i++) {
		collisionSystem->AddGameObject(checkpointManager->checkpoints[i]);
	}

	HUDData data = { lives, points };
	hud->setHUDData(data);
}

void GameManager::update(float deltaTime)
{
	if (hud) {
		hud->setLives(lives);
		hud->setPoints(points);
		hud->setPaused(paused);
		hud->setGameOver(lives <= 0);
		hud->update(deltaTime);
	}

	if (paused || lives <= 0)
		return;

	car->update(deltaTime);
	orangesManager->update(deltaTime);
	cheeriosManager->update(deltaTime);
	butter->update(deltaTime);
	checkpointManager->update(deltaTime);
	collisionSystem->updateCollisions();
}

void GameManager::resetGame()
{
	lives = 5;
	points = 0;
	car->reset({ 0.0f, 0.0f, 0.0f });
	butter->setPosition(0.0f, 0.0f, 50.0f);
	checkpointManager->reset();
	if (hud) {
		hud->setLives(lives);
		hud->setPoints(points);
		hud->setGameOver(false);
	}
}
