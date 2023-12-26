#pragma once

#include <raccoon-ecs/utils/system.h>

#include "HAL/Network/ConnectionManager.h"

class WorldHolder;
class GameStateRewinder;
class FrameTimeCorrector;

/**
 * System that handles network communication on client
 */
class ClientNetworkSystem : public RaccoonEcs::System
{
public:
	ClientNetworkSystem(
		WorldHolder& worldHolder,
		GameStateRewinder& gameStateRewinder,
		const HAL::Network::NetworkAddress& serverAddress,
		FrameTimeCorrector& frameTimeCorrector,
		bool& shouldQuitGame
	) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
	HAL::Network::NetworkAddress mServerAddress;
	FrameTimeCorrector& mFrameTimeCorrector;
	bool& mShouldQuitGameRef;
};
