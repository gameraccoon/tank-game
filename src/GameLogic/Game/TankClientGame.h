#pragma once

#include "HAL/Network/ConnectionManager.h"

#include "GameData/Render/RenderAccessorGameRef.h"

#include "GameLogic/Game/Game.h"
#ifdef IMGUI_ENABLED
#include "GameLogic/Imgui/ImguiDebugData.h"
#endif

class ArgumentsParser;
class GameplayTimestamp;

class TankClientGame : public Game
{
public:
	using Game::Game;

	void preStart(ArgumentsParser& arguments, RenderAccessorGameRef renderAccessor);
	void initResources() override;
	void dynamicTimePreFrameUpdate(float dt, int plannedFixedTimeUpdates) final;
	void fixedTimeUpdate(float dt) final;
	void dynamicTimePostFrameUpdate(float dt, int processedFixedTimeUpdates) final;

	bool shouldQuitGame() const override { return mShouldQuitGame; }
	void quitGame() override { mShouldQuitGameNextTick = true; }

private:
	void initSystems();
	void correctUpdates(u32 lastUpdateIdxWithAuthoritativeMoves);
	void processMoveCorrections();
	void saveMovesForLastFrame(u32 inputUpdateIndex, const GameplayTimestamp& inputUpdateTimestamp);

private:
	HAL::ConnectionManager mConnectionManager;
	std::vector<World> mPreviousFrameWorlds;
	bool mShouldQuitGameNextTick = false;
	bool mShouldQuitGame = false;

	HAL::ConnectionManager::NetworkAddress mServerAddress{HAL::ConnectionManager::NetworkAddress::Ipv4({127, 0, 0, 1}, 14436)};

#ifdef IMGUI_ENABLED
	ImguiDebugData mImguiDebugData{getWorldHolder(), getComponentFactory(), {}, {}};
#endif // IMGUI_ENABLED
};
