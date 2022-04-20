#include "Base/precomp.h"

#include "GameLogic/SharedManagers/TimeData.h"

#include <cmath>

void TimeData::update(float deltaTime, u32 framesCount)
{
	dt = deltaTime;
	currentTimestamp.increaseByFloatTime(dt);
	frameNumber += framesCount;
}
