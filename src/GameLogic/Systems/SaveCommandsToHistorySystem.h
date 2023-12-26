#pragma once

#include <raccoon-ecs/utils/system.h>

class WorldHolder;
class GameStateRewinder;

/**
 * System that saves gameplay commands to the history
 */
class SaveCommandsToHistorySystem : public RaccoonEcs::System
{
public:
	SaveCommandsToHistorySystem(
		WorldHolder& worldHolder,
		GameStateRewinder& gameStateRewinder
	) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
};
