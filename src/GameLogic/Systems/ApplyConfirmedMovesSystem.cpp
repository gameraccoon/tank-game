#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/ApplyConfirmedMovesSystem.h"

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
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	const u32 currentUpdateIndex = time->getValue()->lastFixedUpdateIndex;

	// get the most recently known moves for non-owned entities
	std::vector<EntityMoveData> newMoves = mGameStateRewinder.getLatestKnownNonOwnedEntityMoves();

	// if we have confirmed moves of the owned entities for this update, append them
	if (mGameStateRewinder.hasConfirmedMovesForUpdate(currentUpdateIndex))
	{
		std::vector<EntityMoveData> authoritativeMovementData = mGameStateRewinder.getMovesForUpdate(currentUpdateIndex);

		for (const auto& move : authoritativeMovementData)
		{
			newMoves.push_back(move);
		}
	}

	// apply all the moves
	world.getEntityManager().forEachComponentSet<TransformComponent, const NetworkIdComponent>(
		[&newMoves](TransformComponent* transform, const NetworkIdComponent* networkId) {
			const auto it = std::ranges::find_if(newMoves, [networkId](const EntityMoveData& moveData) {
				return moveData.networkEntityId == networkId->getId();
			});

			if (it != newMoves.end())
			{
				transform->setLocation(it->location);
				transform->setDirection(it->direction);
			}
		}
	);
}
