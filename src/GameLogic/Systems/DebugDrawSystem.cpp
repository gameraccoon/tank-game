#include "EngineCommon/precomp.h"

#ifndef DISABLE_SDL

#include <algorithm>

#include "EngineCommon/Random/Random.h"
#include "EngineCommon/Types/TemplateHelpers.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/CollisionComponent.generated.h"
#include "GameData/Components/DebugDrawComponent.generated.h"
#include "GameData/Components/RenderAccessorComponent.generated.h"
#include "GameData/Components/RenderModeComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/WorldLayer.h"

#include "HAL/Graphics/Font.h"
#include "HAL/Graphics/Sprite.h"

#include "EngineUtils/ResourceManagement/ResourceManager.h"

#include "GameUtils/Network/GameStateRewinder.h"
#include "GameUtils/SharedManagers/WorldHolder.h"

#include "EngineLogic/Render/RenderAccessor.h"

#include "GameLogic/Systems/DebugDrawSystem.h"

DebugDrawSystem::DebugDrawSystem(
	WorldHolder& worldHolder,
	GameStateRewinder& gameStateRewinder,
	ResourceManager& resourceManager
) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
	, mResourceManager(resourceManager)
{
}

template<typename T>
void RemoveOldDrawElement(std::vector<T>& vector, GameplayTimestamp now)
{
	std::erase_if(
		vector,
		[now](const T& val) { return val.isLifeTimeExceeded(now); }
	);
}

void DebugDrawSystem::update()
{
	SCOPED_PROFILER("DebugDrawSystem::update");
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();
	GameData& gameData = mWorldHolder.getGameData();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	const TimeData& timeValue = *time->getValue();

	EntityManager& entityManager = world.getEntityManager();

	auto [renderMode] = gameData.getGameComponents().getComponents<const RenderModeComponent>();

	const Vector2D drawShift = ZERO_VECTOR;

	auto [renderAccessorCmp] = gameData.getGameComponents().getComponents<RenderAccessorComponent>();
	if (renderAccessorCmp == nullptr || !renderAccessorCmp->getAccessor().has_value())
	{
		return;
	}

	RenderAccessorGameRef renderAccessor = *renderAccessorCmp->getAccessor();

	std::unique_ptr<RenderData> renderData = std::make_unique<RenderData>();

	if (renderMode && renderMode->getIsDrawDebugCollisionsEnabled())
	{
		entityManager.forEachComponentSet<const CollisionComponent, const TransformComponent>(
			[&renderData, &collisionSpriteHandle = mCollisionSpriteHandle, drawShift](const CollisionComponent* collision, const TransformComponent* transform) {
				const Vector2D location = transform->getLocation() + drawShift;
				QuadRenderData& quadData = TemplateHelpers::EmplaceVariant<QuadRenderData>(renderData->layers);
				quadData.position = Vector2D(collision->getBoundingBox().minX + location.x, collision->getBoundingBox().minY + location.y);
				quadData.rotation = 0.0f;
				quadData.size = Vector2D(collision->getBoundingBox().maxX - collision->getBoundingBox().minX, collision->getBoundingBox().maxY - collision->getBoundingBox().minY);
				quadData.spriteHandle = collisionSpriteHandle;
				quadData.anchor = ZERO_VECTOR;
			}
		);
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
					textData.color = { 255, 255, 255, 255 };
					textData.fontHandle = mFontHandle;
					textData.pos = screenPoint.screenPos;
					textData.text = screenPoint.name;
				}
			}

			for (const auto& worldPoint : debugDraw->getWorldPoints())
			{
				Vector2D screenPos = worldPoint.pos + drawShift;

				QuadRenderData& quadData = TemplateHelpers::EmplaceVariant<QuadRenderData>(renderData->layers);
				quadData.position = screenPos;
				quadData.rotation = 0.0f;
				quadData.size = pointSize;
				quadData.spriteHandle = mPointTextureHandle;
				if (!worldPoint.name.empty())
				{
					TextRenderData& textData = TemplateHelpers::EmplaceVariant<TextRenderData>(renderData->layers);
					textData.color = { 255, 255, 255, 255 };
					textData.fontHandle = mFontHandle;
					textData.pos = screenPos;
					textData.text = worldPoint.name;
				}
			}

			for (const auto& worldLineSegment : debugDraw->getWorldLineSegments())
			{
				QuadRenderData& quadData = TemplateHelpers::EmplaceVariant<QuadRenderData>(renderData->layers);
				Vector2D screenPosStart = worldLineSegment.startPos + drawShift;
				Vector2D screenPosEnd = worldLineSegment.endPos + drawShift;
				Vector2D diff = screenPosEnd - screenPosStart;
				quadData.position = (screenPosStart + screenPosEnd) * 0.5f;
				quadData.rotation = diff.rotation().getValue();
				quadData.size = { diff.size(), pointSize.y };
				quadData.spriteHandle = mLineTextureHandle;
			}
		}
	}

	if (renderMode && renderMode->getIsDrawDebugCharacterInfoEnabled())
	{
		entityManager.forEachComponentSet<const CharacterStateComponent, const TransformComponent>(
			[&renderData, fontHandle = mFontHandle, drawShift](const CharacterStateComponent* characterState, const TransformComponent* transform) {
				TextRenderData& textData = TemplateHelpers::EmplaceVariant<TextRenderData>(renderData->layers);
				textData.color = { 255, 255, 255, 255 };
				textData.fontHandle = fontHandle;
				textData.pos = transform->getLocation() + drawShift;
				textData.text = ID_TO_STR(enum_to_string(characterState->getState()));
			}
		);
	}

	if (renderMode && renderMode->getIsDrawDebugInputEnabled())
	{
		constexpr Vector2D crossCenterOffset{ 60, 280 };
		constexpr Vector2D crossPieceSize{ 23, 23 };
		constexpr float crossPieceOffset = 25;
		const GameplayInput::FrameState& lastInput = mGameStateRewinder.getInputForUpdate(timeValue.lastFixedUpdateIndex);
		const float horizontalMove = lastInput.getAxisValue(GameplayInput::InputAxis::MoveHorizontal);
		const float verticalMove = lastInput.getAxisValue(GameplayInput::InputAxis::MoveVertical);

		const bool isMoveUpPressed = lastInput.isKeyActive(GameplayInput::InputKey::MoveUp);
		const bool isMoveDownPressed = lastInput.isKeyActive(GameplayInput::InputKey::MoveDown);
		const bool isMoveLeftPressed = lastInput.isKeyActive(GameplayInput::InputKey::MoveLeft);
		const bool isMoveRightPressed = lastInput.isKeyActive(GameplayInput::InputKey::MoveRight);

		if (horizontalMove < 0.0f || isMoveLeftPressed)
		{
			QuadRenderData& quadData = TemplateHelpers::EmplaceVariant<QuadRenderData>(renderData->layers);
			quadData.position = crossCenterOffset + Vector2D(-crossPieceOffset, 0.0f);
			quadData.size = crossPieceSize;
			quadData.spriteHandle = mArrowLeftTextureHandle;
			quadData.alpha = isMoveLeftPressed ? 1.0f : std::clamp(-horizontalMove, 0.0f, 1.0f);
		}

		if (horizontalMove > 0.0f || isMoveRightPressed)
		{
			QuadRenderData& quadData = TemplateHelpers::EmplaceVariant<QuadRenderData>(renderData->layers);
			quadData.position = crossCenterOffset + Vector2D(crossPieceOffset, 0.0f);
			quadData.size = crossPieceSize;
			quadData.spriteHandle = mArrowRightTextureHandle;
			quadData.alpha = isMoveRightPressed ? 1.0f : std::clamp(horizontalMove, 0.0f, 1.0f);
		}

		if (verticalMove > 0.0f || isMoveDownPressed)
		{
			QuadRenderData& quadData = TemplateHelpers::EmplaceVariant<QuadRenderData>(renderData->layers);
			quadData.position = crossCenterOffset + Vector2D(0.0f, crossPieceOffset);
			quadData.size = crossPieceSize;
			quadData.spriteHandle = mArrowDownTextureHandle;
			quadData.alpha = isMoveDownPressed ? 1.0f : std::clamp(verticalMove, 0.0f, 1.0f);
		}

		if (verticalMove < 0.0f || isMoveUpPressed)
		{
			QuadRenderData& quadData = TemplateHelpers::EmplaceVariant<QuadRenderData>(renderData->layers);
			quadData.position = crossCenterOffset + Vector2D(0.0f, -crossPieceOffset);
			quadData.size = crossPieceSize;
			quadData.spriteHandle = mArrowUpTextureHandle;
			quadData.alpha = isMoveUpPressed ? 1.0f : std::clamp(-verticalMove, 0.0f, 1.0f);
		}
	}

	auto [debugDraw] = gameData.getGameComponents().getComponents<DebugDrawComponent>();
	if (debugDraw != nullptr)
	{
		RemoveOldDrawElement(debugDraw->getWorldPointsRef(), timeValue.lastFixedUpdateTimestamp);
		RemoveOldDrawElement(debugDraw->getScreenPointsRef(), timeValue.lastFixedUpdateTimestamp);
		RemoveOldDrawElement(debugDraw->getWorldLineSegmentsRef(), timeValue.lastFixedUpdateTimestamp);
	}

	renderAccessor.submitData(std::move(renderData));
}

void DebugDrawSystem::init()
{
	SCOPED_PROFILER("DebugDrawSystem::initResources");
	mCollisionSpriteHandle = mResourceManager.lockResource<Graphics::Sprite>(RelativeResourcePath("resources/textures/protection-field-2.png"));
	mNavmeshSpriteHandle = mResourceManager.lockResource<Graphics::Sprite>(RelativeResourcePath("resources/textures/explosive-small-2.png"));
	mPointTextureHandle = mResourceManager.lockResource<Graphics::Sprite>(RelativeResourcePath("resources/textures/protection-field-2.png"));
	mLineTextureHandle = mResourceManager.lockResource<Graphics::Sprite>(RelativeResourcePath("resources/textures/explosive-small-2.png"));
	mArrowUpTextureHandle = mResourceManager.lockResource<Graphics::Sprite>(RelativeResourcePath("resources/textures/debug/arrow-up.png"));
	mArrowDownTextureHandle = mResourceManager.lockResource<Graphics::Sprite>(RelativeResourcePath("resources/textures/debug/arrow-down.png"));
	mArrowLeftTextureHandle = mResourceManager.lockResource<Graphics::Sprite>(RelativeResourcePath("resources/textures/debug/arrow-left.png"));
	mArrowRightTextureHandle = mResourceManager.lockResource<Graphics::Sprite>(RelativeResourcePath("resources/textures/debug/arrow-right.png"));
	mFontHandle = mResourceManager.lockResource<Graphics::Font>(mResourceManager.getAbsoluteResourcePath(RelativeResourcePath("resources/fonts/prstart.ttf")), 16);
}

#endif // !DISABLE_SDL
