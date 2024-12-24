#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/SaveMovementToHistorySystem.h"

#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/NetworkOwnedEntitiesComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Network/EntityMoveData.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/Network/GameStateRewinder.h"
#include "GameUtils/SharedManagers/WorldHolder.h"

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
	ComponentSetHolder& worldComponents = world.getWorldComponents();
	std::vector<EntityMoveData> movementData;
	const NetworkOwnedEntitiesComponent* networkOwnedEntities = worldComponents.getOrAddComponent<const NetworkOwnedEntitiesComponent>();
	const std::vector<NetworkEntityId> ownedEntities = networkOwnedEntities->getOwnedEntities();
	entityManager.forEachComponentSet<const TransformComponent, const NetworkIdComponent>(
		[&movementData, &ownedEntities](const TransformComponent* transform, const NetworkIdComponent* networkId) {
			// only ever consider owned entities here
			if (std::ranges::find(ownedEntities, networkId->getId()) != ownedEntities.end())
			{
				movementData.emplace_back(networkId->getId(), transform->getLocation(), transform->getDirection());
			}
		}
	);

	mGameStateRewinder.addPredictedMovementDataForUpdate(nextUpdateIdx, std::move(movementData));
}
