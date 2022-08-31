#pragma once

#include <raccoon-ecs/system.h>

class WorldHolder;

/**
 * System that adds current gameplay input data to input history
 */
class PopulateInputHistorySystem : public RaccoonEcs::System
{
public:
	PopulateInputHistorySystem(WorldHolder& worldHolder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
};
