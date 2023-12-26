#pragma once

#include <raccoon-ecs/utils/system.h>

class GameStateRewinder;
class WorldHolder;

/**
 * If the command history has commands that are confirmed by the server, this system will write them to the world
 */
class FetchConfirmedCommandsSystem : public RaccoonEcs::System
{
public:
	FetchConfirmedCommandsSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
};
