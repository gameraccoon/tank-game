#pragma once

#include <raccoon-ecs/system.h>

#include "GameData/Resources/ResourceHandle.h"

#include "HAL/EngineFwd.h"

class ResourceManager;
class WorldHolder;

/**
 * System that handles rendering of world objects
 */
class DebugDrawSystem : public RaccoonEcs::System
{
public:
	DebugDrawSystem(
		WorldHolder& worldHolder,
		ResourceManager& resourceManager) noexcept;

	void update() override;
	void init() override;

private:
	WorldHolder& mWorldHolder;
	ResourceManager& mResourceManager;

	ResourceHandle mCollisionSpriteHandle;
	ResourceHandle mNavmeshSpriteHandle;
	ResourceHandle mFontHandle;
	ResourceHandle mPointTextureHandle;
	ResourceHandle mLineTextureHandle;
};
