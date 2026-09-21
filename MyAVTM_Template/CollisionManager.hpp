#pragma once
#include "Car.hpp"
#include "CheriosManager.hpp"

class CollisionManager {
public:
	CollisionManager() = default;

	static bool isCarCollidingWithCherios(CarNode& carNode, CheriosManager& cheriosManager);	
};