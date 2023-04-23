#pragma once

#include <raccoon-ecs/system.h>

class GameStateRewinder;
class WorldHolder;

/**
 * System applies input from the history record for this frame
 */
class FetchClientInputFromHistorySystem : public RaccoonEcs::System
{
public:
	FetchClientInputFromHistorySystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
};
