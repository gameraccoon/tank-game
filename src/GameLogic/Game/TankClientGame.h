#pragma once

#include "HAL/Network/ConnectionManager.h"

#include "GameLogic/Game/Game.h"

#ifdef IMGUI_ENABLED
#include "GameLogic/Imgui/ImguiDebugData.h"
#endif

class ArgumentsParser;
class RenderAccessor;

class TankClientGame : public Game
{
public:
	using Game::Game;

	void preStart(ArgumentsParser& arguments, RenderAccessor& renderAccessor);
	void initResources() override;
	void dynamicTimePostFrameUpdate(float dt, int processedFixedUpdates) override;

	bool shouldQuitGame() const override { return mShouldQuitGame; }
	void quitGame() override { mShouldQuitGameNextTick = true; }

private:
	void initSystems();

private:
	HAL::ConnectionManager mConnectionManager;
	std::vector<World> mPreviousFrameWorlds;
	bool mShouldQuitGameNextTick = false;
	bool mShouldQuitGame = false;

#ifdef IMGUI_ENABLED
	ImguiDebugData mImguiDebugData{getWorldHolder(), getTime(), getComponentFactory(), {}};
#endif // IMGUI_ENABLED
};
