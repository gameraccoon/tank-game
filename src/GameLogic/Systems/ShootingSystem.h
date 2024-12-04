#pragma once

#include <raccoon-ecs/utils/system.h>

class WorldHolder;

/**
 * System that creates projectiles
 */
class ShootingSystem final : public RaccoonEcs::System
{
public:
	explicit ShootingSystem(WorldHolder& worldHolder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
};
