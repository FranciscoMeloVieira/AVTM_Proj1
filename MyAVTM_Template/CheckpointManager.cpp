#include "CheckpointManager.h"
#include <iostream>

CheckpointManager::CheckpointManager(std::vector<Vector3> checkpointPositions)
{
	int index = 0;
	for (const Vector3& pos : checkpointPositions)
	{
		Vector3 min = { pos.x - checkpointWidth / 2, pos.y - 1.0f, pos.z - checkpointLength / 2 };
		Vector3 max = { pos.x + checkpointWidth / 2, pos.y + 1.0f, pos.z + checkpointLength / 2 };
		checkpoints.push_back(new Checkpoint(this, pos, min, max, index));
		index++;
	}
}

void CheckpointManager::OnCheckpointCrossed(int checkpointId)
{
	if (checkpointId == nextExpectedCheckpointId)
	{
		nextExpectedCheckpointId++;
		std::cout << "Checkpoint " << checkpointId << " crossed! Next expected checkpoint: " << nextExpectedCheckpointId << std::endl;
		if (nextExpectedCheckpointId >= checkpoints.size())
		{
			nextExpectedCheckpointId = 0; // Loop back to the first checkpoint
			std::cout << "All checkpoints crossed! Looping back to the first checkpoint." << std::endl;
		}
	}
}

