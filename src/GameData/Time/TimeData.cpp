#include "Base/precomp.h"

#include "GameData/Time/TimeData.h"

void TimeData::fixedUpdate(float deltaTime, u32 updatesCount)
{
	lastFixedUpdateDt = deltaTime;
	lastUpdateDt = deltaTime;
	lastFixedUpdateTimestamp.increaseByUpdateCount(updatesCount);
	countFixedTimeUpdatesThisFrame += updatesCount;
	++lastFixedUpdateIndex;
}
