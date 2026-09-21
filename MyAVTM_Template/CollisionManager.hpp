#pragma once

class CarNode;          
class CheriosManager;
class OrangesManager;

class CollisionManager {
public:
	CollisionManager(CarNode* carNode, CheriosManager* cheriosManager, OrangesManager* orangeManager) : carNode(carNode), cheriosManager(cheriosManager), orangeManager(orangeManager) {}
		
	bool isCarCollidingWithCherios();
	bool isCarCollidingWithOranges();
	void setCarNode(CarNode* carNode) { this->carNode = carNode; }

private:
	CarNode* carNode;
	CheriosManager* cheriosManager;
	OrangesManager* orangeManager;
};