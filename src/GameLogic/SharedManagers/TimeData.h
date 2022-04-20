#pragma once

#include "GameData/Time/GameplayTimestamp.h"

class TimeData
{
public:
	void update(float dt, u32 framesCount = 1);

public:
	float dt = 0.0f;
	GameplayTimestamp currentTimestamp{0};
	u32 frameNumber = 0;
};
