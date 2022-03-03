#include "Base/precomp.h"

#include "GameLogic/Systems/DebugDrawSystem.h"

#include <algorithm>

#include "Base/Random/Random.h"
#include "Base/Types/TemplateHelpers.h"

#include "GameData/World.h"
#include "GameData/GameData.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Components/CollisionComponent.generated.h"
#include "GameData/Components/RenderModeComponent.generated.h"
#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/WorldCachedDataComponent.generated.h"
#include "GameData/Components/DebugDrawComponent.generated.h"
#include "GameData/Components/RenderAccessorComponent.generated.h"

#include "HAL/Graphics/Font.h"

#include "GameLogic/Render/RenderAccessor.h"

DebugDrawSystem::DebugDrawSystem(
		WorldHolder& worldHolder,
		const TimeData& timeData,
		HAL::ResourceManager& resourceManager) noexcept
	: mWorldHolder(worldHolder)
	, mTime(timeData)
	, mResourceManager(resourceManager)
{
}

template<typename T>
void RemoveOldDrawElement(std::vector<T>& vector, GameplayTimestamp now)
{
	std::erase_if(
		vector,
		[now](const T& val){ return val.isLifeTimeExceeded(now); }
	);
}

void DebugDrawSystem::update()
{
	SCOPED_PROFILER("DebugDrawSystem::update");
	World& world = mWorldHolder.getWorld();
	GameData& gameData = mWorldHolder.getGameData();

	auto [worldCachedData] = world.getWorldComponents().getComponents<WorldCachedDataComponent>();
	const Vector2D workingRect = worldCachedData->getScreenSize();
	const Vector2D cameraLocation = worldCachedData->getCameraPos();

	EntityManager& entityManager = world.getEntityManager();

	auto [renderMode] = gameData.getGameComponents().getComponents<const RenderModeComponent>();

	const Vector2D screenHalfSize = workingRect * 0.5f;
	const Vector2D drawShift = screenHalfSize - cameraLocation;

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

	if (renderMode && renderMode->getIsDrawDebugCollisionsEnabled())
	{
		entityManager.forEachComponentSet<const CollisionComponent, const TransformComponent>(
			[&renderData, &collisionSpriteHandle = mCollisionSpriteHandle, drawShift](const CollisionComponent* collision, const TransformComponent* transform)
		{
			const Vector2D location = transform->getLocation() + drawShift;
			QuadRenderData& quadData = TemplateHelpers::EmplaceVariant<QuadRenderData>(renderData->layers);
			quadData.position = Vector2D(collision->getBoundingBox().minX + location.x, collision->getBoundingBox().minY + location.y);
			quadData.rotation = 0.0f;
			quadData.size = Vector2D(collision->getBoundingBox().maxX-collision->getBoundingBox().minX,
				collision->getBoundingBox().maxY-collision->getBoundingBox().minY);
			quadData.spriteHandle = collisionSpriteHandle;
			quadData.anchor = ZERO_VECTOR;
		});
	}

	if (renderMode && renderMode->getIsDrawDebugPrimitivesEnabled())
	{
		auto [debugDraw] = gameData.getGameComponents().getComponents<const DebugDrawComponent>();
		if (debugDraw != nullptr)
		{
			Vector2D pointSize(6, 6);
			for (const auto& screenPoint : debugDraw->getScreenPoints())
			{
				QuadRenderData& quadData = TemplateHelpers::EmplaceVariant<QuadRenderData>(renderData->layers);
				quadData.position = screenPoint.screenPos;
				quadData.size = pointSize;
				quadData.spriteHandle = mPointTextureHandle;
				if (!screenPoint.name.empty())
				{
					TextRenderData& textData = TemplateHelpers::EmplaceVariant<TextRenderData>(renderData->layers);
					textData.color = {255, 255, 255, 255};
					textData.fontHandle = mFontHandle;
					textData.pos = screenPoint.screenPos;
					textData.text = screenPoint.name;
				}
			}

			for (const auto& worldPoint : debugDraw->getWorldPoints())
			{
				Vector2D screenPos = worldPoint.pos - cameraLocation + screenHalfSize;

				QuadRenderData& quadData = TemplateHelpers::EmplaceVariant<QuadRenderData>(renderData->layers);
				quadData.position = screenPos;
				quadData.rotation = 0.0f;
				quadData.size = pointSize;
				quadData.spriteHandle = mPointTextureHandle;
				if (!worldPoint.name.empty())
				{
					TextRenderData& textData = TemplateHelpers::EmplaceVariant<TextRenderData>(renderData->layers);
					textData.color = {255, 255, 255, 255};
					textData.fontHandle = mFontHandle;
					textData.pos = screenPos;
					textData.text = worldPoint.name;
				}
			}

			for (const auto& worldLineSegment : debugDraw->getWorldLineSegments())
			{
				QuadRenderData& quadData = TemplateHelpers::EmplaceVariant<QuadRenderData>(renderData->layers);
				Vector2D screenPosStart = worldLineSegment.startPos - cameraLocation + screenHalfSize;
				Vector2D screenPosEnd = worldLineSegment.endPos - cameraLocation + screenHalfSize;
				Vector2D diff = screenPosEnd - screenPosStart;
				quadData.position = (screenPosStart + screenPosEnd) * 0.5f;
				quadData.rotation = diff.rotation().getValue();
				quadData.size = {diff.size(), pointSize.y};
				quadData.spriteHandle = mLineTextureHandle;
			}
		}
	}

	if (renderMode && renderMode->getIsDrawDebugCharacterInfoEnabled())
	{
		entityManager.forEachComponentSet<const CharacterStateComponent, const TransformComponent>(
			[&renderData, fontHandle = mFontHandle, drawShift](const CharacterStateComponent* characterState, const TransformComponent* transform)
		{
			TextRenderData& textData = TemplateHelpers::EmplaceVariant<TextRenderData>(renderData->layers);
			textData.color = {255, 255, 255, 255};
			textData.fontHandle = fontHandle;
			textData.pos = transform->getLocation() + drawShift;
			textData.text = ID_TO_STR(enum_to_string(characterState->getState()));
		});
	}

	auto [debugDraw] = gameData.getGameComponents().getComponents<DebugDrawComponent>();
	if (debugDraw != nullptr)
	{
		RemoveOldDrawElement(debugDraw->getWorldPointsRef(), mTime.currentTimestamp);
		RemoveOldDrawElement(debugDraw->getScreenPointsRef(), mTime.currentTimestamp);
		RemoveOldDrawElement(debugDraw->getWorldLineSegmentsRef(), mTime.currentTimestamp);
	}

	renderAccessor->submitData(std::move(renderData));
}

void DebugDrawSystem::init()
{
	SCOPED_PROFILER("DebugDrawSystem::initResources");
	mCollisionSpriteHandle = mResourceManager.lockSprite("resources/textures/protection-field-2.png");
	mNavmeshSpriteHandle = mResourceManager.lockSprite("resources/textures/explosive-small-2.png");
	mPointTextureHandle = mResourceManager.lockSprite("resources/textures/protection-field-2.png");
	mLineTextureHandle = mResourceManager.lockSprite("resources/textures/explosive-small-2.png");
	mFontHandle = mResourceManager.lockResource<Graphics::Font>("resources/fonts/prstart.ttf", 16);
}
