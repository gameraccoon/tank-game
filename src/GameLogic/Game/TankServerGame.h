#pragma once

#include "HAL/Network/ConnectionManager.h"

#include "GameLogic/Game/Game.h"

#ifdef IMGUI_ENABLED
#include "GameLogic/Imgui/ImguiDebugData.h"
#endif

class ArgumentsParser;

class TankServerGame : public Game
{
public:
	TankServerGame(ResourceManager& resourceManager, ThreadPool& threadPool);

	void preStart(const ArgumentsParser& arguments);
	void initResources() final;
	void dynamicTimePreFrameUpdate(float dt, int plannedFixedTimeUpdates) final;
	void dynamicTimePostFrameUpdate(float dt, int processedFixedTimeUpdates) final;

	bool shouldQuitGame() const override { return mShouldQuitGame; }
	void quitGame() override { mShouldQuitGame = true; }

private:
	void initSystems();
	void processInputCorrections();

private:
	HAL::ConnectionManager mConnectionManager;
	std::vector<World> mPreviousFrameWorlds;
	bool mShouldQuitGame = false;
};
