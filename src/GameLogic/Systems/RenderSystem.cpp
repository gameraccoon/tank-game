#include "EngineCommon/precomp.h"

#include "EngineCommon/TimeConstants.h"

#ifndef DISABLE_SDL

#include <algorithm>
#include <ranges>

#include "EngineCommon/Types/TemplateHelpers.h"

#include "GameData/Components/BackgroundTextureComponent.generated.h"
#include "GameData/Components/MoveInterpolationComponent.generated.h"
#include "GameData/Components/RenderAccessorComponent.generated.h"
#include "GameData/Components/RenderModeComponent.generated.h"
#include "GameData/Components/SpriteRenderComponent.generated.h"
#include "GameData/Components/TileGridComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Components/WorldCachedDataComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/WorldLayer.h"

#include "HAL/Graphics/Sprite.h"

#include "EngineUtils/ResourceManagement/ResourceManager.h"

#include "GameUtils/SharedManagers/WorldHolder.h"

#include "EngineLogic/Render/RenderAccessor.h"

#include "GameLogic/Systems/RenderSystem.h"

RenderSystem::RenderSystem(
	WorldHolder& worldHolder,
	ResourceManager& resourceManager
) noexcept
	: mWorldHolder(worldHolder)
	, mResourceManager(resourceManager)
{
}

void RenderSystem::update()
{
	SCOPED_PROFILER("RenderSystem::update");
	WorldLayer& dynamicWorldLayer = mWorldHolder.getDynamicWorldLayer();
	WorldLayer& staticWorldLayer = mWorldHolder.getStaticWorldLayer();
	GameData& gameData = mWorldHolder.getGameData();

	auto [renderAccessorCmp] = gameData.getGameComponents().getComponents<RenderAccessorComponent>();
	if (renderAccessorCmp == nullptr || !renderAccessorCmp->getAccessor().has_value())
	{
		return;
	}

	const RenderAccessorGameRef renderAccessor = *renderAccessorCmp->getAccessor();

	const auto [worldCachedData] = dynamicWorldLayer.getWorldComponents().getComponents<WorldCachedDataComponent>();
	const Vector2D workingRect = worldCachedData->getScreenSize();

	const auto [renderMode] = gameData.getGameComponents().getComponents<RenderModeComponent>();

	constexpr Vector2D drawShift = ZERO_VECTOR;

	std::unique_ptr<RenderData> renderData = std::make_unique<RenderData>();

	auto [timeComponent] = dynamicWorldLayer.getWorldComponents().getComponents<TimeComponent>();
	const TimeData& time = *timeComponent->getValue();
	const GameplayTimestamp lastFixedUpdateTime = time.lastFixedUpdateTimestamp;

	if (!renderMode || renderMode->getIsDrawBackgroundEnabled())
	{
		drawBackground(*renderData, staticWorldLayer, drawShift, workingRect);
	}

	{
		SCOPED_PROFILER("draw tile grid layer 0");
		drawTileGridLayer(*renderData, staticWorldLayer, drawShift, 0);
	}

	if (!renderMode || renderMode->getIsDrawVisibleEntitiesEnabled())
	{
		SCOPED_PROFILER("draw visible entities");
		const float frameAlpha = time.frameAlpha;
		dynamicWorldLayer.getEntityManager().forEachComponentSet<const SpriteRenderComponent, const TransformComponent, const MoveInterpolationComponent>(
			[&drawShift, &renderData, lastFixedUpdateTime, frameAlpha](const SpriteRenderComponent* spriteRender, const TransformComponent* transform, const MoveInterpolationComponent* moveInterpolation) {
				const Vector2D location = transform->getLocation() + drawShift;
				const Vector2D direction = transform->getDirection();
				const float rotation = direction.rotation().getValue() + PI * 0.5f;

				AssertFatal(moveInterpolation->getTargetTimestamp() != moveInterpolation->getOriginalTimestamp(), "Invalid interpolation timestamps, they should not be equal");
				float alpha = (static_cast<float>((lastFixedUpdateTime - moveInterpolation->getOriginalTimestamp()).getFixedFramesCount()) + frameAlpha) / static_cast<float>((moveInterpolation->getTargetTimestamp() - moveInterpolation->getOriginalTimestamp()).getFixedFramesCount());
				alpha = std::clamp(alpha, 0.0f, 1.0f);
				const Vector2D positionOffset = (moveInterpolation->getOriginalPosition() - transform->getLocation()) * (1.0f - alpha);

				for (const auto& data : spriteRender->getSpriteDatas())
				{
					QuadRenderData& quadData = TemplateHelpers::EmplaceVariant<QuadRenderData>(renderData->layers);
					quadData.spriteHandle = data.spriteHandle;
					quadData.position = location + positionOffset;
					quadData.size = data.params.size;
					quadData.anchor = data.params.anchor;
					quadData.rotation = rotation;
					quadData.alpha = 1.0f;
				}
			}
		);
	}

	{
		SCOPED_PROFILER("draw tile grid layer 1");
		drawTileGridLayer(*renderData, dynamicWorldLayer, drawShift, 1);
	}

	renderAccessor.submitData(std::move(renderData));
}

void RenderSystem::drawBackground(RenderData& renderData, WorldLayer& worldLayer, Vector2D drawShift, Vector2D windowSize) const
{
	SCOPED_PROFILER("RenderSystem::drawBackground");
	auto [backgroundTexture] = worldLayer.getWorldComponents().getComponents<BackgroundTextureComponent>();
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

void RenderSystem::drawTileGridLayer(RenderData& renderData, WorldLayer& worldLayer, const Vector2D drawShift, const size_t layerIdx)
{
	worldLayer.getEntityManager().forEachComponentSet<const TileGridComponent, const TransformComponent>(
		[&drawShift, &renderData, layerIdx](const TileGridComponent* tileGrid, const TransformComponent* transform) {
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
		}
	);
}

#endif // !DISABLE_SDL
