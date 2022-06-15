#pragma once

#include <raccoon-ecs/system.h>

#include "GameData/Geometry/Vector2D.h"

class ResourceManager;
class ThreadPool;
class World;
class WorldHolder;
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

	void update() override;

private:
	void drawBackground(RenderData& renderData, World& world, Vector2D drawShift, Vector2D windowSize);
	void drawTileGridLayer(RenderData& renderData, World& world, Vector2D drawShift, size_t layerIdx);

private:
	WorldHolder& mWorldHolder;
	ResourceManager& mResourceManager;
	ThreadPool& mThreadPool;
};
