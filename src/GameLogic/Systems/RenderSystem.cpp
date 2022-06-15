#include "Base/precomp.h"

#include "GameLogic/Systems/RenderSystem.h"

#include <algorithm>
#include <ranges>

#include "Base/Types/TemplateAliases.h"
#include "Base/Types/TemplateHelpers.h"

#include "GameData/Components/BackgroundTextureComponent.generated.h"
#include "GameData/Components/RenderAccessorComponent.generated.h"
#include "GameData/Components/RenderModeComponent.generated.h"
#include "GameData/Components/SpriteRenderComponent.generated.h"
#include "GameData/Components/TileGridComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Components/WorldCachedDataComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/World.h"

#include "Utils/Multithreading/ThreadPool.h"
#include "Utils/ResourceManagement/ResourceManager.h"

#include "HAL/Graphics/Sprite.h"

#include "GameLogic/Render/RenderAccessor.h"
#include "GameLogic/SharedManagers/WorldHolder.h"


RenderSystem::RenderSystem(
		WorldHolder& worldHolder,
		ResourceManager& resourceManager,
		ThreadPool& threadPool
	) noexcept
	: mWorldHolder(worldHolder)
	, mResourceManager(resourceManager)
	, mThreadPool(threadPool)
{
}

void RenderSystem::update()
{
	SCOPED_PROFILER("RenderSystem::update");
	World& world = mWorldHolder.getWorld();
	GameData& gameData = mWorldHolder.getGameData();

	auto [renderAccessorCmp] = gameData.getGameComponents().getComponents<RenderAccessorComponent>();
	if (renderAccessorCmp == nullptr || !renderAccessorCmp->getAccessor().has_value())
	{
		return;
	}

	RenderAccessorGameRef renderAccessor = *renderAccessorCmp->getAccessor();

	const auto [worldCachedData] = world.getWorldComponents().getComponents<WorldCachedDataComponent>();
	const Vector2D workingRect = worldCachedData->getScreenSize();

	const auto [renderMode] = gameData.getGameComponents().getComponents<RenderModeComponent>();

	const Vector2D drawShift = worldCachedData->getDrawShift();

	std::unique_ptr<RenderData> renderData = std::make_unique<RenderData>();

	if (!renderMode || renderMode->getIsDrawBackgroundEnabled())
	{
		drawBackground(*renderData, world, drawShift, workingRect);
	}

	{
		SCOPED_PROFILER("draw tile grid layer 0");
		drawTileGridLayer(*renderData, world, drawShift, 0);
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

	{
		SCOPED_PROFILER("draw tile grid layer 1");
		drawTileGridLayer(*renderData, world, drawShift, 1);
	}

	renderAccessor.submitData(std::move(renderData));
}

void RenderSystem::drawBackground(RenderData& renderData, World& world, Vector2D drawShift, Vector2D windowSize)
{
	SCOPED_PROFILER("RenderSystem::drawBackground");
	auto [backgroundTexture] = world.getWorldComponents().getComponents<BackgroundTextureComponent>();
	if (backgroundTexture != nullptr)
	{
		if (!backgroundTexture->getSprite().spriteHandle.isValid())
		{
			backgroundTexture->getSpriteRef().spriteHandle = mResourceManager.lockResource<Graphics::Sprite>(backgroundTexture->getSpriteDesc().path);
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

void RenderSystem::drawTileGridLayer(RenderData& renderData, World& world, const Vector2D drawShift, const size_t layerIdx)
{
	world.getEntityManager().forEachComponentSet<const TileGridComponent, const TransformComponent>(
		[&drawShift, &renderData, layerIdx](const TileGridComponent* tileGrid, const TransformComponent* transform)
	{
		const TileGridParams& tileGridData = tileGrid->getGridData();
		if (tileGridData.layers.size() <= layerIdx)
		{
			return;
		}
		const std::vector<size_t>& layer = tileGridData.layers[layerIdx];

		const Vector2D location = transform->getLocation() + drawShift;
		const Vector2D tileSize = Vector2D{ static_cast<float>(tileGridData.originalTileSize.x), static_cast<float>(tileGridData.originalTileSize.y) };

		for (size_t i = 0, iSize = layer.size(); i < iSize; ++i)
		{
			const size_t tileIndex = layer[i];
			if (tileIndex == TileSetParams::EmptyTileIndex)
			{
				continue;
			}

			if (tileGridData.tiles.size() <= tileIndex)
			{
				continue;
			}

			const TileSetParams::Tile& tile = tileGridData.tiles[tileIndex];

			QuadRenderData& quadData = TemplateHelpers::EmplaceVariant<QuadRenderData>(renderData.layers);
			quadData.spriteHandle = tile.spriteHandle;
			quadData.position = location + Vector2D::HadamardProduct(tileSize, Vector2D{ static_cast<float>(i % tileGridData.gridSize.x), static_cast<float>(i / tileGridData.gridSize.y) });
			quadData.size = tileSize;
			quadData.anchor = ZERO_VECTOR;
			quadData.rotation = 0;
			quadData.alpha = 1.0f;
		}
	});
}
