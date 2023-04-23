#pragma once

#include <raccoon-ecs/system.h>

class GameStateRewinder;
class WorldHolder;

/**
 * System that applies gameplay commands collected in the previous frame
 */
class ApplyGameplayCommandsSystem : public RaccoonEcs::System
{
public:
	ApplyGameplayCommandsSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
};
