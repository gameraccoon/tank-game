#pragma once

#include <raccoon-ecs/system.h>

class GameStateRewinder;
class WorldHolder;

/**
 * System that gets scheduled commands for this frame for command history future records
 */
class FetchExternalCommandsSystem : public RaccoonEcs::System
{
public:
	FetchExternalCommandsSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
};
