#pragma once

#include <raccoon-ecs/system.h>

class GameStateRewinder;
class WorldHolder;

/**
 * If the command history has moves confirmed by the server, this system will apply them to the entities in the world
 */
class ApplyConfirmedMovesSystem : public RaccoonEcs::System
{
public:
	ApplyConfirmedMovesSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
};
