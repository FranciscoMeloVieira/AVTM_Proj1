#pragma once
#include <vector>
#include "Checkpoint.h"


class CheckpointManager
{
public:
	CheckpointManager(std::vector<Vector3> checkpointPositions);
	~CheckpointManager() = default;
	void OnCheckpointCrossed(int checkpointId);
	std::vector<Checkpoint*> checkpoints;
	void update(float deltaTime)
	{
		for (Checkpoint* checkpoint : checkpoints)
		{
			checkpoint->update(deltaTime);
		}
	}

private:
	float checkpointWidth = 5.0f; // Width of the checkpoint area
	float checkpointLength = 5.0f; // Length of the checkpoint area

	int nextExpectedCheckpointId = 0;
};
