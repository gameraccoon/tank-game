#pragma once

#include <raccoon-ecs/system.h>

class GameStateRewinder;
class WorldHolder;

/**
 * System that gets scheduled commands for this frame for command history future records
 */
class FetchScheduledCommandsSystem : public RaccoonEcs::System
{
public:
	FetchScheduledCommandsSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
};
