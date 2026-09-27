#include "HUD.h"
#include "renderer.h"
#include <GL/glew.h>

HUD::HUD()
{
	hudData.lives = 5;
	hudData.points = 0;
}

HUD::~HUD()
{
}

void HUD::setConstantValue(const std::string& key, const std::string& text)
{
	customConstants[key] = text;
}

void HUD::removeConstantValue(const std::string& key)
{
	customConstants.erase(key);
}

void HUD::clearConstantValues()
{
	customConstants.clear();
}

void HUD::setGameOver(bool isOver, const std::string& message)
{
	gameOver = isOver;
	if (!message.empty()) {
		gameOverText = message;
	}
}

void HUD::setPaused(bool isPaused, const std::string& message)
{
	paused = isPaused;
	if (!message.empty()) {
		pausedText = message;
	}
}

void HUD::showOneShotMessage(const std::string& message, float duration,
	float x, float y, float size, const float color[4])
{
	oneShotMsg.text = message;
	oneShotMsg.duration = duration;
	oneShotMsg.remainingTime = duration;
	oneShotMsg.position[0] = x;
	oneShotMsg.position[1] = y;
	oneShotMsg.size = size;
	if (color) {
		for (int i = 0; i < 4; ++i) oneShotMsg.color[i] = color[i];
	} else {
		oneShotMsg.color[0] = 1.0f;
		oneShotMsg.color[1] = 1.0f;
		oneShotMsg.color[2] = 1.0f;
		oneShotMsg.color[3] = 1.0f;
	}
	oneShotMsg.active = true;
}

void HUD::clearOneShotMessage()
{
	oneShotMsg.active = false;
	oneShotMsg.text.clear();
}

void HUD::update(float deltaTime)
{
	if (oneShotMsg.active && oneShotMsg.duration > 0.0f) {
		oneShotMsg.remainingTime -= deltaTime;
		if (oneShotMsg.remainingTime <= 0.0f) {
			oneShotMsg.active = false;
			oneShotMsg.text.clear();
		}
	}
}

void HUD::render(Renderer& renderer, int points, int lives)
{
	hudData.points = points;
	hudData.lives = lives;
	render(renderer);
}

void HUD::render(Renderer& renderer)
{
	int m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);

	float vpX = static_cast<float>(m_viewport[0]);
	float vpY = static_cast<float>(m_viewport[1]);
	float vpWidth = static_cast<float>(m_viewport[2]);
	float vpHeight = static_cast<float>(m_viewport[3]);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	mu.loadIdentity(gmu::MODEL);
	mu.loadIdentity(gmu::VIEW);
	mu.pushMatrix(gmu::PROJECTION);
	mu.loadIdentity(gmu::PROJECTION);
	mu.ortho(vpX, vpX + vpWidth - 1.0f, vpY, vpY + vpHeight - 1.0f, -1.0f, 1.0f);
	mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);
	float* pvm = mu.get(gmu::PROJ_VIEW_MODEL);

	// 1. Render constant stats: Points & Lives
	std::string statsText = "Points: " + std::to_string(hudData.points) + " Lives: " + std::to_string(hudData.lives);
	TextCommand textCmd;
	textCmd.str = statsText;
	textCmd.position[0] = constantPos[0];
	textCmd.position[1] = constantPos[1];
	textCmd.size = constantSize;
	for (int i = 0; i < 4; ++i) textCmd.color[i] = constantColor[i];
	textCmd.pvm = pvm;
	renderer.renderText(textCmd);

	// 2. Render any custom constant lines
	float nextY = constantPos[1] - 40.0f * constantSize;
	for (const auto& pair : customConstants) {
		TextCommand customCmd;
		customCmd.str = pair.second;
		customCmd.position[0] = constantPos[0];
		customCmd.position[1] = nextY;
		customCmd.size = constantSize;
		for (int i = 0; i < 4; ++i) customCmd.color[i] = constantColor[i];
		customCmd.pvm = pvm;
		renderer.renderText(customCmd);
		nextY -= 40.0f * constantSize;
	}

	// 3. Render Game Over message (centered)
	if (gameOver) {
		TextCommand goCmd;
		goCmd.str = gameOverText;
		goCmd.size = 0.7f;
		float approxWidth = static_cast<float>(gameOverText.length()) * (50.0f * goCmd.size);
		goCmd.position[0] = (vpWidth - approxWidth) * 0.5f;
		if (goCmd.position[0] < 10.0f) goCmd.position[0] = 10.0f;
		goCmd.position[1] = vpHeight * 0.55f;
		for (int i = 0; i < 4; ++i) goCmd.color[i] = gameOverColor[i];
		goCmd.pvm = pvm;
		renderer.renderText(goCmd);
	}
	// 4. Render Game Paused message (centered, if not game over)
	else if (paused) {
		TextCommand pauseCmd;
		pauseCmd.str = pausedText;
		pauseCmd.size = 0.7f;
		float approxWidth = static_cast<float>(pausedText.length()) * (50.0f * pauseCmd.size);
		pauseCmd.position[0] = (vpWidth - approxWidth) * 0.5f;
		if (pauseCmd.position[0] < 10.0f) pauseCmd.position[0] = 10.0f;
		pauseCmd.position[1] = vpHeight * 0.55f;
		for (int i = 0; i < 4; ++i) pauseCmd.color[i] = pausedColor[i];
		pauseCmd.pvm = pvm;
		renderer.renderText(pauseCmd);
	}

	// 5. Render one-shot / timed message
	if (oneShotMsg.active && !oneShotMsg.text.empty()) {
		TextCommand osCmd;
		osCmd.str = oneShotMsg.text;
		osCmd.size = oneShotMsg.size;
		for (int i = 0; i < 4; ++i) osCmd.color[i] = oneShotMsg.color[i];

		if (oneShotMsg.position[0] < 0.0f) {
			float approxWidth = static_cast<float>(oneShotMsg.text.length()) * (50.0f * osCmd.size);
			osCmd.position[0] = (vpWidth - approxWidth) * 0.5f;
			if (osCmd.position[0] < 10.0f) osCmd.position[0] = 10.0f;
		} else {
			osCmd.position[0] = oneShotMsg.position[0];
		}

		if (oneShotMsg.position[1] < 0.0f) {
			osCmd.position[1] = (gameOver || paused) ? (vpHeight * 0.40f) : (vpHeight * 0.50f);
		} else {
			osCmd.position[1] = oneShotMsg.position[1];
		}

		osCmd.pvm = pvm;
		renderer.renderText(osCmd);
	}

	mu.popMatrix(gmu::PROJECTION);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void HUD::drawText(Renderer& renderer, const std::string& text, float x, float y, float size, const float color[4])
{
	int m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);

	float vpX = static_cast<float>(m_viewport[0]);
	float vpY = static_cast<float>(m_viewport[1]);
	float vpWidth = static_cast<float>(m_viewport[2]);
	float vpHeight = static_cast<float>(m_viewport[3]);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	mu.loadIdentity(gmu::MODEL);
	mu.loadIdentity(gmu::VIEW);
	mu.pushMatrix(gmu::PROJECTION);
	mu.loadIdentity(gmu::PROJECTION);
	mu.ortho(vpX, vpX + vpWidth - 1.0f, vpY, vpY + vpHeight - 1.0f, -1.0f, 1.0f);
	mu.computeDerivedMatrix(gmu::PROJ_VIEW_MODEL);

	TextCommand textCmd;
	textCmd.str = text;
	textCmd.position[0] = x;
	textCmd.position[1] = y;
	textCmd.size = size;
	if (color) {
		for (int i = 0; i < 4; ++i) textCmd.color[i] = color[i];
	} else {
		textCmd.color[0] = 1.0f; textCmd.color[1] = 1.0f; textCmd.color[2] = 1.0f; textCmd.color[3] = 1.0f;
	}
	textCmd.pvm = mu.get(gmu::PROJ_VIEW_MODEL);

	renderer.renderText(textCmd);

	mu.popMatrix(gmu::PROJECTION);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

