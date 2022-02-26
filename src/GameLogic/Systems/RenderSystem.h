#pragma once

#include <memory>
#include <vector>

#include <raccoon-ecs/system.h>

#include "HAL/Base/ResourceManager.h"

#include "Utils/Multithreading/ThreadPool.h"

#include "GameLogic/SharedManagers/WorldHolder.h"
#include "GameLogic/SharedManagers/TimeData.h"

struct RenderData;

/**
 * System that handles rendering of world objects
 */
class RenderSystem : public RaccoonEcs::System
{
public:
	RenderSystem(
		WorldHolder& worldHolder,
		const TimeData& timeData,
		HAL::ResourceManager& resourceManager,
		RaccoonEcs::ThreadPool& threadPool) noexcept;

	~RenderSystem() override = default;

	void update() override;
	static std::string GetSystemId() { return "RenderSystem"; }

private:
	void drawBackground(RenderData& renderData, World& world, Vector2D drawShift, Vector2D windowSize);

private:
	WorldHolder& mWorldHolder;
	const TimeData& mTime;
	HAL::ResourceManager& mResourceManager;
	RaccoonEcs::ThreadPool& mThreadPool;
};
