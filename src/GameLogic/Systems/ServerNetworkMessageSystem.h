#pragma once

#include <raccoon-ecs/utils/system.h>

#include "EngineCommon/Types/BasicTypes.h"

class WorldHolder;
class GameStateRewinder;

/**
 * System that handles messages coming from clients
 */
class ServerNetworkMessageSystem final : public RaccoonEcs::System
{
public:
	ServerNetworkMessageSystem(
		WorldHolder& worldHolder,
		GameStateRewinder& gameStateRewinder,
		bool& shouldPauseGame,
		bool& shouldQuitGame
	) noexcept;

	void update() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
	u32 mLastClientInteractionUpdateIdx = 0;
	bool& mShouldPauseGame;
	bool& mShouldQuitGame;
	// after a few frames without updates the server will pause the simulation (in case we stepped on a breakpoint)
	static constexpr u32 SERVER_IDLE_TIMEOUT_UPDATES_TO_PAUSE = 3;
	// about a minute without any network activity will cause the server to shut down
	static constexpr u32 SERVER_IDLE_TIMEOUT_UPDATES_TO_QUIT = 60 * 60;
};
