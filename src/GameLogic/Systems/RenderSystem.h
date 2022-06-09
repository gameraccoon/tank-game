#pragma once

#include <raccoon-ecs/system.h>

#include "Utils/ResourceManagement/ResourceManager.h"
#include "Utils/Multithreading/ThreadPool.h"

#include "GameLogic/SharedManagers/WorldHolder.h"

struct RenderData;

/**
 * System that handles rendering of world objects
 */
class RenderSystem : public RaccoonEcs::System
{
public:
	RenderSystem(
		WorldHolder& worldHolder,
		ResourceManager& resourceManager,
		ThreadPool& threadPool) noexcept;

	~RenderSystem() override = default;

	void update() override;
	static std::string GetSystemId() { return "RenderSystem"; }

private:
	void drawBackground(RenderData& renderData, World& world, Vector2D drawShift, Vector2D windowSize);
	void drawTileGridLayer(RenderData& renderData, World& world, Vector2D drawShift, size_t layerIdx);

private:
	WorldHolder& mWorldHolder;
	ResourceManager& mResourceManager;
	ThreadPool& mThreadPool;
};
