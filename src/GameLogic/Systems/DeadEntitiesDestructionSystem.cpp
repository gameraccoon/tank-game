#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/DeadEntitiesDestructionSystem.h"

#include "EngineCommon/Types/TemplateAliases.h"

#include "GameData/Components/DeathComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/NetworkOwnedEntitiesComponent.generated.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/SharedManagers/WorldHolder.h"

DeadEntitiesDestructionSystem::DeadEntitiesDestructionSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void DeadEntitiesDestructionSystem::update()
{
	SCOPED_PROFILER("DeadEntitiesDestructionSystem::update");

	const size_t deadEntitiesCount = mWorldHolder.getMutableEntities().getMatchingEntitiesCount<DeathComponent>();

	if (deadEntitiesCount == 0)
	{
		return;
	}

	// ToDo: this all should be done via a network command(s), so it can be corrected from the server

	NetworkOwnedEntitiesComponent* networkOwnedEntities = mWorldHolder.getDynamicWorldLayer().getWorldComponents().getOrAddComponent<NetworkOwnedEntitiesComponent>();
	std::vector<NetworkEntityId>& ownedEntities = networkOwnedEntities->getOwnedEntitiesRef();

	TupleVector<const DeathComponent*, const NetworkIdComponent*> networkIdComponents;
	mWorldHolder.getMutableEntities().getComponents<const DeathComponent, const NetworkIdComponent>(networkIdComponents);

	for (const auto& [deathComponent, networkIdComponent] : networkIdComponents)
	{
		std::erase(ownedEntities, networkIdComponent->getId());
	}

	// this invalidates all the component pointers stored in this scope
	mWorldHolder.getMutableEntities().forEachComponentSetWithEntity<DeathComponent>(
		[](EntityView entityView, const DeathComponent* /*deathComponent*/) {
			entityView.scheduleRemoveEntity();
		}
	);
	mWorldHolder.getMutableEntities().executeScheduledActions();
}
