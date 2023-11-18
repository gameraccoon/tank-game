#pragma once

#include <raccoon-ecs/system.h>

class WorldHolder;

/**
 * System that removes projectiles after their lifetime time is over
 */
class ProjectileLifetimeSystem : public RaccoonEcs::System
{
public:
	ProjectileLifetimeSystem(WorldHolder& worldHolder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
};
