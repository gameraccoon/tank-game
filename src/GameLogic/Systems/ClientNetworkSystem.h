#pragma once

#include <raccoon-ecs/system.h>

class WorldHolder;

/**
 * System that handles network communication on client
 */
class ClientNetworkSystem : public RaccoonEcs::System
{
public:
	ClientNetworkSystem(WorldHolder& worldHolder, const bool& shouldQuitGame) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	const bool& mShouldQuitGameRef;
};
