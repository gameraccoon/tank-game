#include "Base/precomp.h"

#include "GameLogic/Systems/DeadEntitiesDestructionSystem.h"

#include "Base/Types/TemplateAliases.h"

#include "GameData/World.h"
#include "GameData/GameData.h"
#include "GameData/Components/DeathComponent.generated.h"

#include "GameLogic/SharedManagers/WorldHolder.h"

DeadEntitiesDestructionSystem::DeadEntitiesDestructionSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void DeadEntitiesDestructionSystem::update()
{
	SCOPED_PROFILER("DeadEntitiesDestructionSystem::update");
	World& world = mWorldHolder.getWorld();

	TupleVector<Entity, const DeathComponent*> components;
	world.getEntityManager().getComponentsWithEntities<const DeathComponent>(components);

	for (const auto& componentTuple : components)
	{
		world.getEntityManager().removeEntity(std::get<0>(componentTuple));
	}
}
