#pragma once

#include <unordered_map>

#include <raccoon-ecs/system.h>

#include "HAL/Network/ConnectionManager.h"

#include "GameLogic/SharedManagers/WorldHolder.h"

/**
 * System that handles network communication on the server
 */
class ServerNetworkSystem : public RaccoonEcs::System
{
public:
	ServerNetworkSystem(WorldHolder& worldHolder, HAL::ConnectionManager& connectionManager, bool& shouldQuitGame) noexcept;
	~ServerNetworkSystem() override = default;

	void update() override;
	static std::string GetSystemId() { return "ServerNetworkSystem"; }

private:
	WorldHolder& mWorldHolder;
	HAL::ConnectionManager& mConnectionManager;
	bool& mShouldQuitGame;
};
