#pragma once

#include <raccoon-ecs/system.h>

#include "HAL/Network/ConnectionManager.h"

class WorldHolder;

/**
 * System that handles network communication on client
 */
class ClientNetworkSystem : public RaccoonEcs::System
{
public:
	ClientNetworkSystem(
		WorldHolder& worldHolder,
		const HAL::ConnectionManager::NetworkAddress& serverAddress,
		const bool& shouldQuitGame
	) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	HAL::ConnectionManager::NetworkAddress mServerAddress;
	const bool& mShouldQuitGameRef;
};
