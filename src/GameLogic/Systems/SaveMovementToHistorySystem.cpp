#include "Base/precomp.h"

#include "GameLogic/Systems/SaveMovementToHistorySystem.h"

#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Network/MovementHistory.h"
#include "GameData/WorldLayer.h"

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
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();

	const u32 nextUpdateIdx = time->getValue()->lastFixedUpdateIndex + 1;

	if (mGameStateRewinder.hasConfirmedCommandsForUpdate(nextUpdateIdx))
	{
		// we don't want to rewrite data that was already confirmed by the server
		return;
	}

	EntityManager& entityManager = world.getEntityManager();
	MovementUpdateData newUpdateData;
	entityManager.forEachComponentSet<const TransformComponent, const NetworkIdComponent>(
		[&newUpdateData](const TransformComponent* transform, const NetworkIdComponent* networkId)
	{
		newUpdateData.addHash(networkId->getId(), transform->getLocation(), transform->getDirection());
	});

	std::sort(newUpdateData.updateHash.begin(), newUpdateData.updateHash.end());

	mGameStateRewinder.addPredictedMovementDataForUpdate(nextUpdateIdx, std::move(newUpdateData));
}
