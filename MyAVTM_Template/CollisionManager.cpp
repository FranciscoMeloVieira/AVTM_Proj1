#include "CollisionManager.hpp"

#include "Car.hpp"
#include "CheriosManager.hpp"
#include "OrangeManager.hpp"

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

bool CollisionManager::isCarCollidingWithOranges()
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

	const float orangeRadius = 0.5f;

	for (int i = 0; i < orangeManager->numOranges; ++i)
	{
		const auto& orangePos = orangeManager->oranges[i].pos;
		float orangeMinX = orangePos[0] - orangeRadius;
		float orangeMaxX = orangePos[0] + orangeRadius;
		float orangeMinZ = orangePos[2] - orangeRadius;
		float orangeMaxZ = orangePos[2] + orangeRadius;
		bool overlapX = (carMinX <= orangeMaxX) && (carMaxX >= orangeMinX);
		bool overlapZ = (carMinZ <= orangeMaxZ) && (carMaxZ >= orangeMinZ);
		if (overlapX && overlapZ)
		{
			return true;
		}
	}
	return false;
}