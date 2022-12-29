#include "Base/precomp.h"

#include "GameLogic/Systems/SaveMovementToHistorySystem.h"

#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"

#include "GameData/World.h"

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

	const u32 inputUpdateIndex = time->getValue()->lastFixedUpdateIndex;
	const GameplayTimestamp& inputUpdateTimestamp = time->getValue()->lastFixedUpdateTimestamp;

	EntityManager& entityManager = world.getEntityManager();

	MovementHistory& movementHistory = mGameStateRewinder.getMovementHistory();

	std::vector<MovementUpdateData>& updates = movementHistory.updates;
	AssertFatal(inputUpdateIndex == movementHistory.lastUpdateIdx + 1, "We skipped some frames in the movement history. %u %u", inputUpdateIndex, movementHistory.lastUpdateIdx);
	const size_t nextUpdateIndex = updates.size() + inputUpdateIndex - movementHistory.lastUpdateIdx - 1;
	Assert(nextUpdateIndex == updates.size(), "Possibly miscalculated size of the vector. %u %u", nextUpdateIndex, updates.size());
	updates.resize(nextUpdateIndex + 1);
	MovementUpdateData& newUpdateData = updates[nextUpdateIndex];

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

	movementHistory.lastUpdateIdx = inputUpdateIndex;
}
