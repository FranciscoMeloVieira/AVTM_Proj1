#include "CollisionManager.hpp"

#include "Car.hpp"
#include "CheriosManager.hpp"

bool CollisionManager::isCarCollidingWithCherios()
{
	const float* matrix = carNode->getLocalModelMatrix();
	float carX = matrix[12];
	float carZ = matrix[14];

	const float carHalfWidth = 1.7f;
	const float carHalfLength = 3.7f;

	float carMinX = carX - carHalfWidth;
	float carMaxX = carX + carHalfWidth;
	float carMinZ = carZ - carHalfLength;
	float carMaxZ = carZ + carHalfLength;

	const float cheerioRadius = 0.5f;

	const auto& torusPositions = cheriosManager->torusPositions;

	for (const auto& torusPos : torusPositions)
	{
		float cheerioMinX = torusPos.x - cheerioRadius;
		float cheerioMaxX = torusPos.x + cheerioRadius;
		float cheerioMinZ = torusPos.z - cheerioRadius;
		float cheerioMaxZ = torusPos.z + cheerioRadius;

		bool overlapX = (carMinX <= cheerioMaxX) && (carMaxX >= cheerioMinX);
		bool overlapZ = (carMinZ <= cheerioMaxZ) && (carMaxZ >= cheerioMinZ);

		if (overlapX && overlapZ)
		{
			return true;
		}
	}

	return false;
}