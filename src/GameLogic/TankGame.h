#pragma once

#include <thread>

#include "GameLogic/Game.h"

#ifdef IMGUI_ENABLED
#include "GameLogic/Imgui/ImguiDebugData.h"
#endif

class TankGame : public Game
{
public:
	using Game::Game;

	void preStart(ArgumentsParser& arguments);
	void initResources() override;

private:
	void initSystems();

#ifdef IMGUI_ENABLED
	ImguiDebugData mImguiDebugData{getWorldHolder(), getTime(), getComponentFactory(), {}};
#endif // IMGUI_ENABLED
};
