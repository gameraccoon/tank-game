#pragma once

#include <raccoon-ecs/utils/system.h>

class WorldHolder;
class GameStateRewinder;
class FrameTimeCorrector;

/**
 * System that handles network messages from the server
 */
class ClientNetworkMessageSystem final : public RaccoonEcs::System
{
public:
	ClientNetworkMessageSystem(
		WorldHolder& worldHolder,
		GameStateRewinder& gameStateRewinder,
		FrameTimeCorrector& frameTimeCorrector,
		bool& shouldQuitGame
	) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
	FrameTimeCorrector& mFrameTimeCorrector;
	bool& mShouldQuitGameRef;
};
