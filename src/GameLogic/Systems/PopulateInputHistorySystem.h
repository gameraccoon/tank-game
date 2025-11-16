#pragma once

#include <raccoon-ecs/utils/system.h>

class WorldHolder;
class GameStateRewinder;

/**
 * System that adds current gameplay input data to input history
 */
class PopulateInputHistorySystem final : public RaccoonEcs::System
{
public:
	PopulateInputHistorySystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
};
