#pragma once

#include <raccoon-ecs/system.h>

#include "HAL/Network/ConnectionManager.h"

class WorldHolder;
class GameStateRewinder;

/**
 * System that handles network communication on client
 */
class ClientNetworkSystem : public RaccoonEcs::System
{
public:
	ClientNetworkSystem(
		WorldHolder& worldHolder,
		GameStateRewinder& gameStateRewinder,
		const HAL::ConnectionManager::NetworkAddress& serverAddress,
		bool& shouldQuitGame
	) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
	HAL::ConnectionManager::NetworkAddress mServerAddress;
	bool& mShouldQuitGameRef;
};
