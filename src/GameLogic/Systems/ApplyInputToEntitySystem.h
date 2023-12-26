#pragma once

#include <raccoon-ecs/utils/system.h>

class WorldHolder;

/**
 * System that takes input stored per world and tries to apply it to an entity
 */
class ApplyInputToEntitySystem : public RaccoonEcs::System
{
public:
	ApplyInputToEntitySystem(WorldHolder& worldHolder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
};
