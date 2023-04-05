#include "Base/precomp.h"

#include "GameLogic/Systems/SaveMovementToHistorySystem.h"

#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"

#include "GameData/World.h"

#include "Utils/Network/GameStateRewinder.h"
#include "Utils/SharedManagers/WorldHolder.h"


SaveMovementToHistorySystem::SaveMovementToHistorySystem(
	WorldHolder& worldHolder,
	GameStateRewinder& gameStateRewinder
) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
{
}

void SaveMovementToHistorySystem::update()
{
	SCOPED_PROFILER("SaveMovementToHistorySystem::update");
	World& world = mWorldHolder.getWorld();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();

	const u32 updateIndex = time->getValue()->lastFixedUpdateIndex;

	if (mGameStateRewinder.hasConfirmedCommandsForUpdate(updateIndex))
	{
		// we don't want to rewrite data that was already confirmed by the server
		return;
	}

	const GameplayTimestamp& inputUpdateTimestamp = time->getValue()->lastFixedUpdateTimestamp;

	EntityManager& entityManager = world.getEntityManager();
	MovementUpdateData newUpdateData;
	entityManager.forEachComponentSetWithEntity<const MovementComponent, const TransformComponent>(
		[&newUpdateData, inputUpdateTimestamp](Entity entity, const MovementComponent* movement, const TransformComponent* transform)
	{
		// only if we moved within some agreed (between client and server) period of time
		const GameplayTimestamp updateTimestamp = movement->getUpdateTimestamp();
		if (updateTimestamp.isInitialized() && updateTimestamp.getIncreasedByUpdateCount(15) > inputUpdateTimestamp)
		{
			newUpdateData.addHash(entity, transform->getLocation());
		}
	});

	std::sort(newUpdateData.updateHash.begin(), newUpdateData.updateHash.end());

	mGameStateRewinder.addPredictedMovementDataForUpdate(updateIndex + 1, std::move(newUpdateData));
}
