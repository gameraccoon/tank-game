#include "Base/precomp.h"

#include "GameLogic/Systems/ApplyConfirmedMovesSystem.h"

#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/World.h"

#include "Utils/Network/GameStateRewinder.h"
#include "Utils/SharedManagers/WorldHolder.h"

ApplyConfirmedMovesSystem::ApplyConfirmedMovesSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
{
}

void ApplyConfirmedMovesSystem::update()
{
	SCOPED_PROFILER("FetchConfirmedMovesSystem::update");
	World& world = mWorldHolder.getWorld();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	const u32 currentUpdateIndex = time->getValue()->lastFixedUpdateIndex;

	// if we have confirmed moves for this update, apply them
	if (mGameStateRewinder.hasConfirmedMovesForUpdate(currentUpdateIndex))
	{
		std::unordered_map<Entity, EntityMoveData> entityMoves;
		for (const auto& move : mGameStateRewinder.getMovesForUpdate(currentUpdateIndex).moves)
		{
			entityMoves.emplace(move.entity, move);
		}

		world.getEntityManager().forEachComponentSetWithEntity<MovementComponent, TransformComponent>(
			[&entityMoves](Entity entity, MovementComponent* movement, TransformComponent* transform) {
				const auto it = entityMoves.find(entity);

				if (it == entityMoves.end())
				{
					return;
				}

				const EntityMoveData& move = it->second;

				transform->setLocation(move.location);
				movement->setUpdateTimestamp(move.timestamp);
			}
		);
	}
}
