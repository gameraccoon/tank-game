#pragma once

#include <raccoon-ecs/system.h>

class GameStateRewinder;
class WorldHolder;

/**
 * System that updates and sends input history to server
 */
class ClientInputSendSystem : public RaccoonEcs::System
{
public:
	ClientInputSendSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
};
