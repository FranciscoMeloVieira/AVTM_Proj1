#pragma once

struct HUDData
{
	int lives;
	int points;
};



class HUD
{
public:
	HUD() = default;
	~HUD() = default;
	void setHUDData(HUDData data) { hudData = data; }
	void renderHUD();

private:
	HUDData hudData;
};

