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
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();

	TupleVector<Entity, const DeathComponent*> entitiesWithDeathComponent;
	world.getEntityManager().getComponentsWithEntities<const DeathComponent>(entitiesWithDeathComponent);

	if (entitiesWithDeathComponent.empty())
	{
		return;
	}

	// ToDo: this all should be done via a network command(s), so it can be corrected from the server

	NetworkOwnedEntitiesComponent* networkOwnedEntities = world.getWorldComponents().getOrAddComponent<NetworkOwnedEntitiesComponent>();
	std::vector<NetworkEntityId>& ownedEntities = networkOwnedEntities->getOwnedEntitiesRef();

	TupleVector<Entity, const DeathComponent*, const NetworkIdComponent*> networkIdComponents;
	world.getEntityManager().getComponentsWithEntities<const DeathComponent, const NetworkIdComponent>(networkIdComponents);

	for (const auto& [entity, deathComponent, networkIdComponent] : networkIdComponents)
	{
		std::erase(ownedEntities, networkIdComponent->getId());
	}

	// this invalidates all the component pointers stored in this scope
	for (const auto& componentTuple : entitiesWithDeathComponent)
	{
		world.getEntityManager().removeEntity(std::get<0>(componentTuple));
	}
}
