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
	void initResources() override;

	bool shouldQuitGame() const override { return mShouldQuitGame; }
	void quitGame() override { mShouldQuitGame = true; }

private:
	void initSystems();

private:
	HAL::ConnectionManager mConnectionManager;
	bool mShouldQuitGame = false;
};
