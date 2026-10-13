#include "GameManager.h"
#include <iostream>

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
	/*for (int i = 0; i < orangesManager->numOranges; i++) {
		collisionSystem->AddGameObject(&orangesManager->oranges[i]);
	}*/
	for (int i = 0; i < cheeriosManager->cheerios.size(); i++) {
		collisionSystem->AddGameObject(&cheeriosManager->cheerios[i]);
	}
	collisionSystem->AddGameObject(butter);
	for (int i = 0; i < checkpointManager->checkpoints.size(); i++) {
		collisionSystem->AddGameObject(checkpointManager->checkpoints[i]);
	}

	HUDData data = { lives, points };
	hud->setHUDData(data);

	pathVisualizer = new PathVisualizer();
	sceneRoot->addChild(pathVisualizer);
	loadTestPath(0);
}

void GameManager::update(float deltaTime)
{
	if (hud) {
		hud->setLives(lives);
		hud->setPoints(points);
		hud->setPaused(paused);
		hud->setGameOver(lives <= 0);

		if (controlMode == ControlMode::WASD)
		{
			hud->setConstantValue("0_Mode", "MODE: WASD  [M: Engage AI | T: Cycle Path | V: Visuals]");
			hud->removeConstantValue("1_Path");
			hud->removeConstantValue("2_Progress");
			hud->removeConstantValue("3_Speed");
			hud->removeConstantValue("4_Steer");
		}
		else
		{
			hud->setConstantValue("0_Mode", "MODE: AI PATH FOLLOWER  [M: Switch to WASD]");
			hud->setConstantValue("1_Path", "PATH: " + activePath.name + "  [T: Switch Path]");

			char buf[128];
			std::snprintf(buf, sizeof(buf), "PROGRESS: %.1f / %.1f m", pathFollower.getCurrentProgress(), activePath.totalLength);
			hud->setConstantValue("2_Progress", buf);

			float curSpeed = Dot(car->velocity, car->getFacingDirection());
			std::snprintf(buf, sizeof(buf), "SPEED: %.1f m/s  (Target: %.1f)", curSpeed, pathFollower.getEffectiveTargetSpeed());
			hud->setConstantValue("3_Speed", buf);

			std::snprintf(buf, sizeof(buf), "STEER: %.2f  | LAT DEV: %.2f m", pathFollower.getLastOutput().steering, pathFollower.getLateralDeviation());
			hud->setConstantValue("4_Steer", buf);
		}

		hud->update(deltaTime);
	}

	if (paused || lives <= 0)
		return;

	// AI Path Following Update (mutually exclusive with WASD)
	if (controlMode == ControlMode::PathFollowing && activePath.valid)
	{
		CarState state;
		state.position = car->position;
		state.velocity = car->velocity;
		state.forward = car->getFacingDirection();
		state.speed = Dot(car->velocity, state.forward);

		PathFollowOutput out = pathFollower.update(state, deltaTime);
		car->applyControlRequests(out.steering, out.throttle, out.brake);

		if (pathVisualizer && debugVisualsEnabled)
		{
			pathVisualizer->setTargetPosition(pathFollower.getTargetPosition());
			pathVisualizer->setProgressPosition(pathFollower.getClosestPosition());
		}
	}

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
	pathFollower.reset();
	if (controlMode == ControlMode::PathFollowing)
	{
		pathFollower.setPath(activePath);
	}
	if (hud) {
		hud->setLives(lives);
		hud->setPoints(points);
		hud->setGameOver(false);
	}
}

void GameManager::loadTestPath(int index)
{
	currentPathIndex = (index % 3 + 3) % 3;
	switch (currentPathIndex)
	{
	case 0:
		activePath = Path::createTrackLoopPath();
		break;
	case 1:
		activePath = Path::createStraightTestPath();
		break;
	case 2:
		activePath = Path::createTurnTestPath();
		break;
	}

	pathFollower.setPath(activePath);
	if (pathVisualizer)
	{
		pathVisualizer->setPath(activePath);
	}
}

void GameManager::switchTestPath()
{
	loadTestPath(currentPathIndex + 1);
	std::cout << "[AI] Switched test path to: " << activePath.name << std::endl;
}

void GameManager::setControlMode(ControlMode mode)
{
	controlMode = mode;
	car->stopCar();

	if (controlMode == ControlMode::PathFollowing)
	{
		pathFollower.reset();
		pathFollower.setPath(activePath);
		std::cout << "[Mode] Switched to AI PATH FOLLOWING (" << activePath.name << ")" << std::endl;
	}
	else
	{
		std::cout << "[Mode] Switched to MANUAL WASD" << std::endl;
	}
}

void GameManager::toggleControlMode()
{
	if (controlMode == ControlMode::WASD)
	{
		setControlMode(ControlMode::PathFollowing);
	}
	else
	{
		setControlMode(ControlMode::WASD);
	}
}

void GameManager::toggleDebugVisuals()
{
	debugVisualsEnabled = !debugVisualsEnabled;
	if (pathVisualizer)
	{
		pathVisualizer->setEnabled(debugVisualsEnabled);
	}
	std::cout << "[Visuals] Path debug visualization " << (debugVisualsEnabled ? "ENABLED" : "DISABLED") << std::endl;
}

void GameManager::toggleFog()
{
	fogActive = !fogActive;
}