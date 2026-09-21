#pragma once

class CarNode;          
class CheriosManager;

class CollisionManager {
public:
	CollisionManager(CarNode* carNode, CheriosManager* cheriosManager) : carNode(carNode), cheriosManager(cheriosManager) {}

	bool isCarCollidingWithCherios();
	void setCarNode(CarNode* carNode) { this->carNode = carNode; }

private:
	CarNode* carNode;
	CheriosManager* cheriosManager;
};