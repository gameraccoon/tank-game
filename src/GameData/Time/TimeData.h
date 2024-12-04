#pragma once

#include "GameData/Time/GameplayTimestamp.h"

struct TimeData
{
	float lastUpdateDt = 0.0f;
	float lastFixedUpdateDt = 0.0f;
	GameplayTimestamp lastFixedUpdateTimestamp{ 0 };
	u32 lastFixedUpdateIndex = 0;
	int countFixedTimeUpdatesThisFrame = 1;

	// at which point between updates we are (0 - just updated, 1 - about to update)
	float frameAlpha = 0.0f;

	void fixedUpdate(float deltaTime, u32 updatesCount = 1);
};
