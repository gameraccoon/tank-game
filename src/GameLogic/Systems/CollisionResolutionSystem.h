#pragma once

#include <raccoon-ecs/system.h>

class WorldHolder;

/**
 * System that detects collisions between objects and resolves them
 */
class CollisionResolutionSystem : public RaccoonEcs::System
{
public:
	CollisionResolutionSystem(WorldHolder& worldHolder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
};
