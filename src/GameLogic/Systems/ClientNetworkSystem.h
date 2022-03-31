#pragma once

#include <unordered_map>

#include <raccoon-ecs/system.h>

#include "HAL/Network/ConnectionManager.h"

#include "GameLogic/SharedManagers/WorldHolder.h"

/**
 * System that handles network communication on client
 */
class ClientNetworkSystem : public RaccoonEcs::System
{
public:
	ClientNetworkSystem(WorldHolder& worldHolder, HAL::ConnectionManager& connectionManager, const bool& shouldQuitGame) noexcept;
	~ClientNetworkSystem() override = default;

	void update() override;
	static std::string GetSystemId() { return "ClientNetworkSystem"; }

private:
	WorldHolder& mWorldHolder;
	HAL::ConnectionManager& mConnectionManager;
	ConnectionId mConnectionId;
	const bool& mShouldQuitGameRef;
};
