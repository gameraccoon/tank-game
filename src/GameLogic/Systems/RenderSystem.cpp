#include "Base/precomp.h"

#include "GameLogic/Systems/RenderSystem.h"

#include <algorithm>
#include <ranges>

#include "Base/Types/TemplateAliases.h"
#include "Base/Types/TemplateHelpers.h"

#include "GameData/GameData.h"
#include "GameData/World.h"
#include "GameData/Components/SpriteRenderComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Components/RenderModeComponent.generated.h"
#include "GameData/Components/WorldCachedDataComponent.generated.h"
#include "GameData/Components/BackgroundTextureComponent.generated.h"
#include "GameData/Components/RenderAccessorComponent.generated.h"

#include "GameLogic/Render/RenderAccessor.h"

#include <glm/matrix.hpp>
#include <glm/gtc/matrix_transform.hpp>

RenderSystem::RenderSystem(
		WorldHolder& worldHolder,
		const TimeData& timeData,
		HAL::ResourceManager& resourceManager,
		RaccoonEcs::ThreadPool& threadPool
	) noexcept
	: mWorldHolder(worldHolder)
	, mTime(timeData)
	, mResourceManager(resourceManager)
	, mThreadPool(threadPool)
{
}

void RenderSystem::update()
{
	SCOPED_PROFILER("RenderSystem::update");
	World& world = mWorldHolder.getWorld();
	GameData& gameData = mWorldHolder.getGameData();

	const auto [worldCachedData] = world.getWorldComponents().getComponents<WorldCachedDataComponent>();
	Vector2D workingRect = worldCachedData->getScreenSize();
	Vector2D cameraLocation = worldCachedData->getCameraPos();

	const auto [renderMode] = gameData.getGameComponents().getComponents<RenderModeComponent>();

	Vector2D halfWindowSize = workingRect * 0.5f;

	Vector2D drawShift = halfWindowSize - cameraLocation;

	RenderAccessor* renderAccessor = nullptr;
	if (auto [renderAccessorCmp] = gameData.getGameComponents().getComponents<RenderAccessorComponent>(); renderAccessorCmp != nullptr)
	{
		renderAccessor = renderAccessorCmp->getAccessor();
	}

	if (renderAccessor == nullptr)
	{
		return;
	}

	std::unique_ptr<RenderData> renderData = std::make_unique<RenderData>();

	if (!renderMode || renderMode->getIsDrawBackgroundEnabled())
	{
		drawBackground(*renderData, world, drawShift, workingRect);
	}

	if (!renderMode || renderMode->getIsDrawVisibleEntitiesEnabled())
	{
		SCOPED_PROFILER("draw visible entities");
		world.getEntityManager().forEachComponentSet<const SpriteRenderComponent, const TransformComponent>(
			[&drawShift, &renderData](const SpriteRenderComponent* spriteRender, const TransformComponent* transform)
		{
			Vector2D location = transform->getLocation() + drawShift;
			float rotation = transform->getRotation().getValue();
			for (const auto& data : spriteRender->getSpriteDatas())
			{
				QuadRenderData& quadData = TemplateHelpers::EmplaceVariant<QuadRenderData>(renderData->layers);
				quadData.spriteHandle = data.spriteHandle;
				quadData.position = location;
				quadData.size = data.params.size;
				quadData.anchor = data.params.anchor;
				quadData.rotation = rotation;
				quadData.alpha = 1.0f;
			}
		});
	}

	renderAccessor->submitData(std::move(renderData));
}

void RenderSystem::drawBackground(RenderData& renderData, World& world, Vector2D drawShift, Vector2D windowSize)
{
	SCOPED_PROFILER("RenderSystem::drawBackground");
	auto [backgroundTexture] = world.getWorldComponents().getComponents<BackgroundTextureComponent>();
	if (backgroundTexture != nullptr)
	{
		if (!backgroundTexture->getSprite().spriteHandle.isValid())
		{
			backgroundTexture->getSpriteRef().spriteHandle = mResourceManager.lockSprite(backgroundTexture->getSpriteDesc().path);
			backgroundTexture->getSpriteRef().params = backgroundTexture->getSpriteDesc().params;
		}

		const SpriteData& spriteData = backgroundTexture->getSpriteRef();
		const Vector2D spriteSize(spriteData.params.size);
		const Vector2D tiles(windowSize.x / spriteSize.x, windowSize.y / spriteSize.y);
		const Vector2D uvShift(-drawShift.x / spriteSize.x, -drawShift.y / spriteSize.y);

		BackgroundRenderData& bgRenderData = TemplateHelpers::EmplaceVariant<BackgroundRenderData>(renderData.layers);
		bgRenderData.spriteHandle = spriteData.spriteHandle;
		bgRenderData.start = ZERO_VECTOR;
		bgRenderData.size = windowSize;
		bgRenderData.uv = Graphics::QuadUV(uvShift, uvShift + tiles);
	}
}
