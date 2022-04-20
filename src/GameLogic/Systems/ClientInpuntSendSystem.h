#pragma once

#include <raccoon-ecs/system.h>

#include "HAL/Network/ConnectionManager.h"

class WorldHolder;
class TimeData;

/**
 * System that updates and sends input history to server
 */
class ClientInputSendSystem : public RaccoonEcs::System
{
public:
	ClientInputSendSystem(WorldHolder& worldHolder, const TimeData& timeData) noexcept;

	void update() override;
	static std::string GetSystemId() { return "ClientInputSendSystem"; }

private:
	WorldHolder& mWorldHolder;
	const TimeData& mTimeData;
};
