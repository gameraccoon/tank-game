#pragma once

#include <raccoon-ecs/utils/system.h>

class WorldHolder;

/**
 * System that process characters and objects movement
 */
class MovementSystem : public RaccoonEcs::System
{
public:
	MovementSystem(WorldHolder& worldHolder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
};
