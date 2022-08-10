#pragma once

#include <string>

#include <raccoon-ecs/system.h>

#include "Base/Types/BasicTypes.h"

class WorldHolder;

/**
 * System that handles network communication on the server
 */
class ServerNetworkSystem : public RaccoonEcs::System
{
public:
	ServerNetworkSystem(WorldHolder& worldHolder, u16 serverPort, bool& shouldQuitGame) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	const u16 mServerPort;
	bool& mShouldQuitGame;
};
