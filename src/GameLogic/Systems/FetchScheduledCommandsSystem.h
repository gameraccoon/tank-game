#pragma once

#include <raccoon-ecs/system.h>

class WorldHolder;

/**
 * System that gets scheduled commands for this frame for command history future records
 */
class FetchScheduledCommandsSystem : public RaccoonEcs::System
{
public:
	FetchScheduledCommandsSystem(WorldHolder& worldHolder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
};
