#pragma once

#include <chrono>
#include <string>

#include <raccoon-ecs/system.h>

#include "Base/Types/BasicTypes.h"

class WorldHolder;
class GameStateRewinder;

/**
 * System that handles network communication on the server
 */
class ServerNetworkSystem : public RaccoonEcs::System
{
public:
	ServerNetworkSystem(
		WorldHolder& worldHolder,
		GameStateRewinder& gameStateRewinder,
		u16 serverPort,
		bool& shouldQuitGame
	) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
	const u16 mServerPort;
	u32 mLastClientInteractionUpdateIdx = 0;
	bool& mShouldQuitGame;
	// about a minute without any network activity will cause the server to shut down
	static constexpr u32 SERVER_IDLE_TIMEOUT_UPDATES = 60*60;
};
