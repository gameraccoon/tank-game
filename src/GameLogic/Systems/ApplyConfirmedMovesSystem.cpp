#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/ApplyConfirmedMovesSystem.h"

#include "GameData/Components/MoveInterpolationComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Network/EntityMoveData.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/Network/GameStateRewinder.h"
#include "GameUtils/SharedManagers/WorldHolder.h"

ApplyConfirmedMovesSystem::ApplyConfirmedMovesSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
{
}

void ApplyConfirmedMovesSystem::update()
{
	SCOPED_PROFILER("ApplyConfirmedMovesSystem::update");
	WorldLayer& dynamicWorld = mWorldHolder.getDynamicWorldLayer();

	const auto [time] = dynamicWorld.getWorldComponents().getComponents<const TimeComponent>();
	const u32 currentUpdateIndex = time->getValue()->lastFixedUpdateIndex;
	const GameplayTimestamp lastFixedUpdateTime = time->getValue()->lastFixedUpdateTimestamp;

	// get the most recently known moves for non-owned entities
	{
		WorldLayer& reflectedWorld = mWorldHolder.getReflectedWorldLayer();
		std::vector<EntityMoveData> newMoves = mGameStateRewinder.getLatestKnownNonOwnedEntityMoves();
		reflectedWorld.getEntityManager().forEachComponentSetWithEntity<TransformComponent, const NetworkIdComponent>(
			[&newMoves, &reflectedWorld, &lastFixedUpdateTime](const Entity entity, TransformComponent* transform, const NetworkIdComponent* networkId) {
				const auto it = std::ranges::find_if(newMoves, [networkId](const EntityMoveData& moveData) {
					return moveData.networkEntityId == networkId->getId();
				});

				auto [moveInterpolation] = reflectedWorld.getEntityManager().getEntityComponents<MoveInterpolationComponent>(entity);
				if (moveInterpolation)
				{
					moveInterpolation->setOriginalPosition(transform->getLocation());
					moveInterpolation->setOriginalTimestamp(lastFixedUpdateTime);
					moveInterpolation->setTargetTimestamp(lastFixedUpdateTime.getIncreasedByUpdateCount(1));
				}

				if (it != newMoves.end())
				{
					transform->setLocation(it->location);
					transform->setDirection(it->direction);
				}
			}
		);
	}

	// if we have confirmed moves of the owned entities for this update, apply them
	if (mGameStateRewinder.hasConfirmedMovesForUpdate(currentUpdateIndex))
	{
		std::vector<EntityMoveData> authoritativeMovementData = mGameStateRewinder.getMovesForUpdate(currentUpdateIndex);
		dynamicWorld.getEntityManager().forEachComponentSet<TransformComponent, const NetworkIdComponent>(
			[&authoritativeMovementData](TransformComponent* transform, const NetworkIdComponent* networkId) {
				const auto it = std::ranges::find_if(authoritativeMovementData, [networkId](const EntityMoveData& moveData) {
					return moveData.networkEntityId == networkId->getId();
				});

				if (it != authoritativeMovementData.end())
				{
					transform->setLocation(it->location);
					transform->setDirection(it->direction);
				}
			}
		);
	}
}
