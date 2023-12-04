#pragma once

#ifdef IMGUI_ENABLED
#ifndef DISABLE_SDL

#include <mutex>
#include <raccoon-ecs/system.h>

#include "HAL/EngineFwd.h"

#include "GameLogic/Imgui/ImguiMainMenu.h"

class WorldHolder;
struct ImguiDebugData;
class ImguiComponent;

/**
 * System that handles dear imgui debug tool
 */
class ImguiSystem : public RaccoonEcs::System
{
public:
	ImguiSystem(
		ImguiDebugData& debugData,
		HAL::Engine& engine) noexcept;

	void update() override;
	void init() override;
	void shutdown() override;

private:
	HAL::Engine& mEngine;
	ImguiDebugData& mDebugData;

	ImguiMainMenu mImguiMainMenu;

	std::mutex mRenderDataMutex;
};

#endif // !DISABLE_SDL
#endif // IMGUI_ENABLED
