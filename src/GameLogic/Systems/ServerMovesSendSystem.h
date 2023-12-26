#pragma once

#include <raccoon-ecs/utils/system.h>

class WorldHolder;
class GameStateRewinder;

/**
 * System that sends entity moves to client
 */
class ServerMovesSendSystem : public RaccoonEcs::System
{
public:
	ServerMovesSendSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
};
