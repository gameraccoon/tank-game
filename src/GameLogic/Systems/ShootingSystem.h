#pragma once

#include <raccoon-ecs/utils/system.h>

class WorldHolder;

/**
 * System that creates projectiles
 */
class ShootingSystem : public RaccoonEcs::System
{
public:
	ShootingSystem(WorldHolder& worldHolder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
};
