#pragma once

#include <string>
#include <vector>
#include <map>
#include "mathUtility.h"

// Forward declaration
class Renderer;

struct HUDData
{
	int lives = 5;
	int points = 0;
};

struct OneShotMessage
{
	std::string text;
	float duration = 0.0f;       // In seconds; <= 0 means manual clear only
	float remainingTime = 0.0f;  // Countdown timer
	float position[2] = { -1.0f, -1.0f }; // -1 indicates auto-centering
	float size = 0.6f;
	float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	bool active = false;
};

class HUD
{
public:
	HUD();
	~HUD();

	// --- Constant values (Points, Lives, custom HUD values) ---
	void setHUDData(const HUDData& data) { hudData = data; }
	HUDData getHUDData() const { return hudData; }
	void setLives(int lives) { hudData.lives = lives; }
	int getLives() const { return hudData.lives; }
	void setPoints(int points) { hudData.points = points; }
	int getPoints() const { return hudData.points; }

	// Constant text styling & position
	void setConstantTextPosition(float x, float y) { constantPos[0] = x; constantPos[1] = y; }
	void setConstantTextSize(float size) { constantSize = size; }
	void setConstantTextColor(float r, float g, float b, float a = 1.0f) {
		constantColor[0] = r; constantColor[1] = g; constantColor[2] = b; constantColor[3] = a;
	}

	// Custom constant entries (e.g. key="Lap", text="Lap: 1/3")
	void setConstantValue(const std::string& key, const std::string& text);
	void removeConstantValue(const std::string& key);
	void clearConstantValues();

	// --- One-shot / State messages ---
	void setGameOver(bool isOver, const std::string& message = "GAME OVER");
	bool isGameOver() const { return gameOver; }

	void setPaused(bool isPaused, const std::string& message = "GAME PAUSED");
	bool isPaused() const { return paused; }

	// Generic one-shot timed message (e.g. "LEVEL CLEAR!", "CHECKPOINT REACHED")
	// If duration <= 0, message stays until clearOneShotMessage() is called.
	// If x < 0 or y < 0, message is centered on that axis.
	void showOneShotMessage(const std::string& message, float duration = 3.0f,
		float x = -1.0f, float y = -1.0f, float size = 0.6f, const float color[4] = nullptr);
	void clearOneShotMessage();
	bool hasActiveOneShotMessage() const { return oneShotMsg.active; }

	// Update timers (decrements active timed message)
	void update(float deltaTime);

	// --- Rendering ---
	// Renders HUD using internally stored data
	void render(Renderer& renderer);

	// Overload allowing direct pass of points & lives
	void render(Renderer& renderer, int points, int lives);

	// Compatibility alias for existing code
	void renderHUD(Renderer& renderer) { render(renderer); }

	// Helper to draw arbitrary 2D text on screen with orthographic projection
	void drawText(Renderer& renderer, const std::string& text, float x, float y,
		float size = 0.5f, const float color[4] = nullptr);

private:
	HUDData hudData;

	// Constant text formatting & styling
	float constantPos[2] = { 100.0f, 100.0f };
	float constantSize = 0.5f;
	float constantColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	// Additional key-value constant fields
	std::map<std::string, std::string> customConstants;

	// State-based message flags and texts
	bool gameOver = false;
	std::string gameOverText = "GAME OVER";
	float gameOverColor[4] = { 1.0f, 0.2f, 0.2f, 1.0f };

	bool paused = false;
	std::string pausedText = "GAME PAUSED";
	float pausedColor[4] = { 1.0f, 1.0f, 0.2f, 1.0f };

	// Timed / one-shot message
	OneShotMessage oneShotMsg;

	// Dedicated math utility for 2D HUD projection
	gmu mu;
};

