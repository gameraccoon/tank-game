#pragma once

#include <string>
#include <chrono>

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
		bool& shouldQuitGame) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
	const u16 mServerPort;
	bool& mShouldQuitGame;
	std::chrono::time_point<std::chrono::system_clock> mLastClientInterationTime;
};
