#pragma once

#include <raccoon-ecs/system.h>

class WorldHolder;
class GameStateRewinder;

/**
 * System that sends commands generated during the frame
 */
class ServerCommandsSendSystem : public RaccoonEcs::System
{
public:
	ServerCommandsSendSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
};
