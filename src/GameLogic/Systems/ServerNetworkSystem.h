#pragma once

#include <string>

#include <raccoon-ecs/system.h>

class WorldHolder;

/**
 * System that handles network communication on the server
 */
class ServerNetworkSystem : public RaccoonEcs::System
{
public:
	ServerNetworkSystem(WorldHolder& worldHolder, bool& shouldQuitGame) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	bool& mShouldQuitGame;
};
