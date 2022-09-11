#pragma once

#include <raccoon-ecs/system.h>

class WorldHolder;

/**
 * System that saves gameplay commands to the history
 */
class SaveCommandsToHistorySystem : public RaccoonEcs::System
{
public:
	SaveCommandsToHistorySystem(WorldHolder& worldHolder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
};
