#pragma once

#include <raccoon-ecs/utils/system.h>

#include "GameData/Resources/ResourceHandle.h"

#include "HAL/EngineFwd.h"

class GameStateRewinder;
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
		GameStateRewinder& gameStateRewinder,
		ResourceManager& resourceManager) noexcept;

	void update() override;
	void init() override;

private:
	WorldHolder& mWorldHolder;
	GameStateRewinder& mGameStateRewinder;
	ResourceManager& mResourceManager;

	ResourceHandle mCollisionSpriteHandle;
	ResourceHandle mNavmeshSpriteHandle;
	ResourceHandle mFontHandle;
	ResourceHandle mPointTextureHandle;
	ResourceHandle mLineTextureHandle;
	ResourceHandle mArrowUpTextureHandle;
	ResourceHandle mArrowDownTextureHandle;
	ResourceHandle mArrowLeftTextureHandle;
	ResourceHandle mArrowRightTextureHandle;
};
