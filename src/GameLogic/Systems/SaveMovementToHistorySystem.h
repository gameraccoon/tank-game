#pragma once

#include <raccoon-ecs/utils/system.h>

class WorldHolder;
class GameStateRewinder;

/**
 * System that saves current entity movements to the history
 */
class SaveMovementToHistorySystem : public RaccoonEcs::System
{
public:
	SaveMovementToHistorySystem(
		WorldHolder& worldHolder,
		GameStateRewinder& gameStateRewinder
	) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
};
