#pragma once

#include <raccoon-ecs/system.h>

class WorldHolder;

/**
 * System that removes entities after their lifetime time is over
 */
class TimeLimitedLifetimeSystem : public RaccoonEcs::System
{
public:
	TimeLimitedLifetimeSystem(WorldHolder& worldHolder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
};
