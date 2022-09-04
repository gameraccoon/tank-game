#pragma once

#include <raccoon-ecs/system.h>

class WorldHolder;

/**
 * System that saves current entity movements to the history
 */
class SaveMovementToHistorySystem : public RaccoonEcs::System
{
public:
	SaveMovementToHistorySystem(WorldHolder& worldHolder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
};
