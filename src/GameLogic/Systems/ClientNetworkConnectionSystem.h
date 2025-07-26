#pragma once

#include <raccoon-ecs/utils/system.h>

#include "HAL/Network/NetworkStructs.h"

class WorldHolder;

/**
 * System that handles network connection to the server
 */
class ClientNetworkConnectionSystem final : public RaccoonEcs::System
{
public:
	ClientNetworkConnectionSystem(
		WorldHolder& worldHolder,
		const HAL::Network::NetworkAddress& serverAddress,
		const bool& shouldQuitGame
	) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	HAL::Network::NetworkAddress mServerAddress;
	const bool& mShouldQuitGameRef;
};
