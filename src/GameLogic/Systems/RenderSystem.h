#pragma once

#ifndef DISABLE_SDL

#include <raccoon-ecs/utils/system.h>

#include "GameData/Geometry/Vector2D.h"

class ResourceManager;
class ThreadPool;
class WorldLayer;
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
		ResourceManager& resourceManager
	) noexcept;

	void update() override;

private:
	void drawBackground(RenderData& renderData, WorldLayer& worldLayer, Vector2D drawShift, Vector2D windowSize);
	void drawTileGridLayer(RenderData& renderData, WorldLayer& worldLayer, Vector2D drawShift, size_t layerIdx);

private:
	WorldHolder& mWorldHolder;
	ResourceManager& mResourceManager;
};

#endif // !DISABLE_SDL
