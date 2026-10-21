#include "GameManager.h"
#include <iostream>
#include <GL/freeglut.h>
#include <cmath>
#include <cstdlib>
#include <vector>

GameManager::GameManager(EmptyNode* sceneRoot)
{
	this->sceneRoot = sceneRoot;
	hud = new HUD();
	this->init();
}

GameManager::~GameManager() = default;

void GameManager::initInputControllers()
{
	m_inputControllers.clear();
	m_inputControllers.push_back(std::make_unique<MouseInputController>());
	m_inputControllers.push_back(std::make_unique<ArduinoInputController>());
	m_inputControllers.push_back(std::make_unique<PS4GyroInputController>());

	m_activeInputMode = PathInputMode::Mouse;
	if (const char* envMode = std::getenv("AVTM_INPUT_MODE"))
	{
		std::string modeStr = envMode;
		for (char& c : modeStr) c = static_cast<char>(std::tolower(c));
		if (modeStr == "arduino" || modeStr == "tilt")
			m_activeInputMode = PathInputMode::Arduino;
		else if (modeStr == "ps4" || modeStr == "ps4gyro" || modeStr == "gyro" || modeStr == "dualshock")
			m_activeInputMode = PathInputMode::PS4Gyro;
		else if (modeStr == "mouse")
			m_activeInputMode = PathInputMode::Mouse;
	}
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

	pathVisualizer = new PathVisualizer();
	sceneRoot->addChild(pathVisualizer);
	loadTestPath(0);

	initInputControllers();
}

void GameManager::update(float deltaTime)
{
	if (hud) {
		hud->setLives(lives);
		hud->setPoints(points);
		hud->setPaused(paused);
		hud->setGameOver(lives <= 0);

		const int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
		const int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);
		if (windowHeight > 0)
		{
			const float halfWidth = 200.0f * static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
			getActiveInputController()->setBounds(150.0f - halfWidth, 150.0f + halfWidth, -200.0f, 200.0f);
		}

		const double now = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
		IPathInputController* activeCtrl = getActiveInputController();
		activeCtrl->update(deltaTime, now, pathEditor);

		if (activeCtrl->consumeEnterEditorRequested())
		{
			const bool wasFollowingPath = controlMode == ControlMode::PathFollowing;
			enterPathEditor();
			if (wasFollowingPath)
			{
				clearEditorPath();
				startActiveInputRecording();
			}
		}
		if (activeCtrl->consumeConfirmRequested())
			confirmEditorPath();
		if (activeCtrl->consumeClearRequested())
			clearEditorPath();
		if (activeCtrl->consumeRecalibrateRequested())
			activeCtrl->beginCalibration();
		if (activeCtrl->consumeResetCursorRequested())
			resetActiveInputCursor();

		if (isEditingPath())
		{
			const std::string modeName = activeCtrl->getModeName();
			const bool isConn = activeCtrl->isConnected();
			const EditorState est = pathEditor.getState();

			if (activeCtrl->hasCursor() && activeCtrl->isCalibrated() && pathVisualizer)
			{
				pathVisualizer->setCursorPosition(activeCtrl->getCursorPosition());
			}
			else if (pathVisualizer && !activeCtrl->hasCursor())
			{
				pathVisualizer->hideCursor();
			}

			if (est == EditorState::Recording)
			{
				hud->setConstantValue("0_Mode", "PATH EDITOR: [RECORDING STROKE...]");
				char buf[128];
				std::snprintf(buf, sizeof(buf), "Samples: %d | Input: %s", static_cast<int>(pathEditor.getRawSamples().size()), modeName.c_str());
				hud->setConstantValue("1_Path", buf);

				if (m_activeInputMode == PathInputMode::PS4Gyro)
				{
					hud->setConstantValue("2_Progress", "PS4 X: Finish & Follow | C: Clear | E: Exit");
					hud->setConstantValue("3_Speed", "G: Calibrate | Tab/U: Switch Input");
				}
				else if (m_activeInputMode == PathInputMode::Arduino)
				{
					hud->setConstantValue("2_Progress", isConn
						? "D4/N: Finish & Follow | C: Clear | E: Exit"
						: "N: Finish & Follow | C: Clear | E: Exit");
					hud->setConstantValue("3_Speed", isConn
						? (showPathEditorHudDetails ? "I: Disconnect | G: Calibrate | Tab/U: Switch" : "G: Calibrate | Tab/U: Switch")
						: "I: Connect Arduino | Tab/U: Switch Input");
				}
				else // Mouse
				{
					hud->setConstantValue("2_Progress", "Drag mouse | N: Finish & Follow | C: Clear | E: Exit");
					hud->setConstantValue("3_Speed", "Tab/U: Switch Input");
				}
			}
			else if (est == EditorState::Preview)
			{
				hud->setConstantValue("0_Mode", "PATH EDITOR: [PREVIEW]");
				const Path& p = pathEditor.getProcessedPath();
				if (p.valid)
				{
					char buf[160];
					std::snprintf(buf, sizeof(buf), "Len: %.1fm | Pts: %d (raw %d) | Loop: %s | Smooth: %s",
						p.totalLength, static_cast<int>(p.points.size()), static_cast<int>(pathEditor.getRawSamples().size()),
						pathEditor.isClosedLoop() ? "ON" : "OFF",
						pathEditor.isSmoothingEnabled() ? "ON" : "OFF");
					if (showPathEditorHudDetails)
						hud->setConstantValue("1_Path", buf);
					else
						hud->removeConstantValue("1_Path");

					if (m_activeInputMode == PathInputMode::PS4Gyro)
					{
						hud->setConstantValue("2_Progress", "PS4 X: Redraw | C: Clear | L: Loop | S: Smooth | E: Exit");
						hud->setConstantValue("3_Speed", isConn ? "G: Calibrate | Tab/U: Switch"
																: "I: Connect PS4 Controller | Tab/U: Switch Input");
					}
					else if (m_activeInputMode == PathInputMode::Arduino)
					{
						hud->setConstantValue("2_Progress", isConn
							? "D4/Enter/Space/N: Follow | C: Clear | L: Loop | S: Smooth | E: Exit"
							: "Enter/Space/N: Follow | C: Clear | L: Loop | S: Smooth | E: Exit");
						hud->setConstantValue("3_Speed", isConn
							? (showPathEditorHudDetails ? "I: Disconnect | G: Calibrate | B: Start | O: Reset cursor" : "B: Start | O: Reset cursor | Tab/U: Switch")
							: "I: Connect Arduino | Tab/U: Switch Input");
					}
					else // Mouse
					{
						hud->setConstantValue("2_Progress", "Enter/Space/N: Follow | C: Clear | L: Loop | S: Smooth | E: Exit");
						hud->setConstantValue("3_Speed", "Drag mouse to redraw | Tab/U: Switch Input");
					}
				}
				else
				{
					hud->setConstantValue("1_Path", "ERROR: " + pathEditor.getLastErrorMessage());
					hud->setConstantValue("2_Progress", (m_activeInputMode == PathInputMode::PS4Gyro)
						? "PS4 X: Redraw | C: Clear | E: Exit"
						: ((m_activeInputMode == PathInputMode::Arduino) ? (isConn ? "D4/B: Start | C: Clear | E: Exit" : "C: Clear | E: Exit")
						: "Drag mouse to draw | C: Clear | E: Exit"));
					hud->setConstantValue("3_Speed", "Tab/U: Switch Input");
				}
			}
			else // Editing (Ready to Draw)
			{
				hud->setConstantValue("0_Mode", "PATH EDITOR: [READY TO DRAW]");
				std::string inputModeStr = "Input: " + modeName + " (Tab/U: Switch)";
				if (showPathEditorHudDetails)
				{
					hud->setConstantValue("1_Path", inputModeStr);
					char buf[128];
					std::snprintf(buf, sizeof(buf), "Loop %s | Smooth %s | View %s",
						pathEditor.isClosedLoop() ? "CLOSED" : "OPEN",
						pathEditor.isSmoothingEnabled() ? "ON" : "OFF",
						pathEditor.isTrackReferenceVisible() ? "TRACK" : "BLANK");
					hud->setConstantValue("2_Progress", buf);
				}
				else
				{
					hud->setConstantValue("1_Path", inputModeStr);
					hud->removeConstantValue("2_Progress");
				}

				if (m_activeInputMode == PathInputMode::PS4Gyro)
				{
					hud->setConstantValue("3_Speed", isConn ? "PS4 X: Start Drawing | G: Calibrate | I: Disconnect"
															: "I: Connect PS4 Controller | E: Exit | Tab/U: Switch");
				}
				else if (m_activeInputMode == PathInputMode::Arduino)
				{
					hud->setConstantValue("3_Speed", isConn
						? (showPathEditorHudDetails ? "I: Disconnect | G: Calibrate | B: Start | O: Reset cursor | D4: Start | E: Exit" : "B: Start | O: Reset cursor | D4: Start | E: Exit")
						: "I: Connect Arduino | Tab/U: Switch Input | E: Exit");
				}
				else // Mouse
				{
					hud->setConstantValue("3_Speed", "Drag mouse to draw | E: Exit | Tab/U: Switch Input");
				}
			}

			if (showPathEditorHudDetails)
			{
				char sensorStatus[256];
				std::snprintf(sensorStatus, sizeof(sensorStatus), "%s: %s",
					modeName.c_str(), activeCtrl->getStatusMessage().c_str());
				hud->setConstantValue("4_Steer", sensorStatus);
			}
			else
			{
				hud->removeConstantValue("4_Steer");
			}

			if (pathVisualizer)
			{
				pathVisualizer->setPath(pathEditor.getProcessedPath());
				pathVisualizer->setRawSamples(pathEditor.getRawSamples());
				pathVisualizer->hideMarkers();
			}

			hud->update(deltaTime);
			return; // While in editor mode, pause world updates
		}

		if (controlMode == ControlMode::WASD)
		{
			std::string modeText = showPathEditorHudDetails
				? "MODE: WASD  [E: Edit Path | M: Engage AI | T: Path | V: Visuals]"
				: "MODE: WASD  [E: Edit Path | M: Engage AI | V: Visuals]";
			if (activeCtrl->isConnected() && m_activeInputMode != PathInputMode::Mouse)
				modeText += (m_activeInputMode == PathInputMode::PS4Gyro) ? " | PS4 X: Edit path" : " | D4: Draw path";
			hud->setConstantValue("0_Mode", modeText);
			hud->removeConstantValue("1_Path");
			hud->removeConstantValue("2_Progress");
			hud->removeConstantValue("3_Speed");
			hud->removeConstantValue("4_Steer");
		}
		else
		{
			hud->setConstantValue("0_Mode", (activeCtrl->isConnected() && m_activeInputMode != PathInputMode::Mouse)
				? ((m_activeInputMode == PathInputMode::PS4Gyro)
					? "MODE: AI PATH FOLLOWER  [E: Edit Path | M: WASD | PS4 X: Redraw path]"
					: "MODE: AI PATH FOLLOWER  [E: Edit Path | M: WASD | D4: New path]")
				: "MODE: AI PATH FOLLOWER  [E: Edit Path | M: WASD]");
			hud->setConstantValue("1_Path", "PATH: " + activePath.name +
				(showPathEditorHudDetails ? "  [T: Switch Path]" : ""));
		}

		hud->removeConstantValue("2_Progress");
		hud->removeConstantValue("3_Speed");
		hud->removeConstantValue("4_Steer");

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

void GameManager::enterPathEditor()
{
	car->stopCar();
	pathEditor.enterEditMode();
	IPathInputController* ctrl = getActiveInputController();
	if (ctrl->hasCursor() && car)
	{
		ctrl->resetCursor(car->position);
	}
	if (pathVisualizer)
	{
		pathVisualizer->setPath(pathEditor.getProcessedPath());
		pathVisualizer->setRawSamples(pathEditor.getRawSamples());
		pathVisualizer->hideMarkers();
	}
	std::cout << "[PathEditor] Active Input: " << ctrl->getModeName() << std::endl;
	std::cout << "[PathEditor] (Press Tab or U to switch input mode: Mouse / Arduino / PS4 Gyro)" << std::endl;
}

void GameManager::exitPathEditor()
{
	stopActiveInputRecording();
	if (pathVisualizer)
		pathVisualizer->hideCursor();
	pathEditor.exitEditMode();
	applyTrackReferenceVisibility();
	if (pathVisualizer)
	{
		pathVisualizer->clearRawSamples();
		pathVisualizer->setPath(activePath);
	}
}

void GameManager::togglePathEditor()
{
	if (isEditingPath())
	{
		exitPathEditor();
	}
	else
	{
		enterPathEditor();
	}
}

void GameManager::confirmEditorPath()
{
	stopActiveInputRecording();

	Path confirmed;
	if (pathEditor.confirmPath(confirmed))
	{
		activePath = confirmed;
		if (pathVisualizer)
			pathVisualizer->hideCursor();
		setControlMode(ControlMode::PathFollowing);
		pathFollower.setPath(activePath, true);

		if (pathVisualizer)
		{
			pathVisualizer->clearRawSamples();
			pathVisualizer->setPath(activePath);
		}

		std::cout << "[GameManager] Custom path confirmed and handed to path follower!" << std::endl;
	}
}

void GameManager::clearEditorPath()
{
	stopActiveInputRecording();
	pathEditor.clear();
	if (pathVisualizer)
	{
		pathVisualizer->clearRawSamples();
		pathVisualizer->setPath(pathEditor.getProcessedPath());
		pathVisualizer->hideMarkers();
	}
}

void GameManager::toggleEditorClosedLoop()
{
	pathEditor.toggleClosedLoop();
	if (pathVisualizer)
	{
		pathVisualizer->setPath(pathEditor.getProcessedPath());
	}
}

void GameManager::toggleEditorSmoothing()
{
	pathEditor.toggleSmoothing();
	if (pathVisualizer)
	{
		pathVisualizer->setPath(pathEditor.getProcessedPath());
	}
}

void GameManager::toggleTrackReference()
{
	pathEditor.toggleTrackReference();
	applyTrackReferenceVisibility();
}

void GameManager::applyTrackReferenceVisibility()
{
	bool visible = pathEditor.isTrackReferenceVisible();
	if (cheeriosManager && cheeriosManager->cheeriosNode)
		cheeriosManager->cheeriosNode->setVisibility(visible);
	if (racetrackNode)
		racetrackNode->setVisibility(visible);
	if (candles && candles->candleNode)
		candles->candleNode->setVisibility(visible);
	if (butter && butter->butterNode)
		butter->butterNode->setVisibility(visible);
	if (orangesManager && orangesManager->orangesNode)
		orangesManager->orangesNode->setVisibility(visible);
}

void GameManager::onEditorMouseDown(int x, int y, int winW, int winH)
{
	const double now = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
	getActiveInputController()->onMouseDown(x, y, winW, winH, now, pathEditor);
}

void GameManager::onEditorMouseDrag(int x, int y, int winW, int winH)
{
	const double now = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
	getActiveInputController()->onMouseDrag(x, y, winW, winH, now, pathEditor);
}

void GameManager::onEditorMouseUp(int x, int y, int winW, int winH)
{
	const double now = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
	getActiveInputController()->onMouseUp(x, y, winW, winH, now, pathEditor);
}

void GameManager::setPathInputMode(PathInputMode mode)
{
	if (m_activeInputMode == mode)
		return;

	IPathInputController* oldCtrl = getActiveInputController();
	if (oldCtrl->isRecording())
		oldCtrl->stopRecording(pathEditor);

	m_activeInputMode = mode;
	IPathInputController* newCtrl = getActiveInputController();
	if (newCtrl->hasCursor() && car)
	{
		newCtrl->resetCursor(car->position);
	}
	if (pathVisualizer)
	{
		if (newCtrl->hasCursor() && newCtrl->isCalibrated() && isEditingPath())
			pathVisualizer->setCursorPosition(newCtrl->getCursorPosition());
		else
			pathVisualizer->hideCursor();
	}

	std::cout << "[PathEditor] Input mode switched to: " << newCtrl->getModeName() << std::endl;
}

void GameManager::cyclePathInputMode()
{
	switch (m_activeInputMode)
	{
	case PathInputMode::Mouse:
		setPathInputMode(PathInputMode::Arduino);
		break;
	case PathInputMode::Arduino:
		setPathInputMode(PathInputMode::PS4Gyro);
		break;
	case PathInputMode::PS4Gyro:
	default:
		setPathInputMode(PathInputMode::Mouse);
		break;
	}
}

IPathInputController* GameManager::getActiveInputController()
{
	for (auto& ctrl : m_inputControllers)
	{
		if (ctrl->getMode() == m_activeInputMode)
			return ctrl.get();
	}
	if (!m_inputControllers.empty())
		return m_inputControllers[0].get();
	return nullptr;
}

const IPathInputController* GameManager::getActiveInputController() const
{
	for (const auto& ctrl : m_inputControllers)
	{
		if (ctrl->getMode() == m_activeInputMode)
			return ctrl.get();
	}
	if (!m_inputControllers.empty())
		return m_inputControllers[0].get();
	return nullptr;
}

void GameManager::toggleActiveInputConnection()
{
	getActiveInputController()->toggleConnection();
}

void GameManager::calibrateActiveInput()
{
	getActiveInputController()->beginCalibration();
}

void GameManager::startActiveInputRecording()
{
	const double now = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
	getActiveInputController()->startRecording(pathEditor, now);
}

void GameManager::stopActiveInputRecording()
{
	getActiveInputController()->stopRecording(pathEditor);
}

void GameManager::resetActiveInputCursor()
{
	getActiveInputController()->resetCursor(car ? car->position : Vector3(150.0f, 0.0f, 0.0f));
}

bool GameManager::isActiveInputRecording() const
{
	return getActiveInputController()->isRecording();
}

void GameManager::toggleArduinoConnection()
{
	if (ArduinoInputController* ac = getArduinoController())
		ac->toggleConnection();
}

void GameManager::calibrateArduinoInput()
{
	if (ArduinoInputController* ac = getArduinoController())
		ac->beginCalibration();
}

void GameManager::startArduinoRecording()
{
	if (ArduinoInputController* ac = getArduinoController())
		ac->startRecording(pathEditor, glutGet(GLUT_ELAPSED_TIME) / 1000.0);
}

void GameManager::stopArduinoRecording()
{
	if (ArduinoInputController* ac = getArduinoController())
		ac->stopRecording(pathEditor);
}

void GameManager::resetArduinoCursor()
{
	if (ArduinoInputController* ac = getArduinoController())
		ac->resetCursor(car ? car->position : Vector3(150.0f, 0.0f, 0.0f));
}

bool GameManager::isArduinoRecording() const
{
	if (const ArduinoInputController* ac = const_cast<GameManager*>(this)->getArduinoController())
		return ac->isRecording();
	return false;
}

ArduinoInputController* GameManager::getArduinoController()
{
	for (auto& ctrl : m_inputControllers)
	{
		if (ctrl->getMode() == PathInputMode::Arduino)
			return static_cast<ArduinoInputController*>(ctrl.get());
	}
	return nullptr;
}

PS4GyroInputController* GameManager::getPS4Controller()
{
	for (auto& ctrl : m_inputControllers)
	{
		if (ctrl->getMode() == PathInputMode::PS4Gyro)
			return static_cast<PS4GyroInputController*>(ctrl.get());
	}
	return nullptr;
}
