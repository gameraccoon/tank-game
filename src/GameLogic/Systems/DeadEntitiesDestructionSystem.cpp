#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/DeadEntitiesDestructionSystem.h"

#include "EngineCommon/Types/TemplateAliases.h"

#include "GameData/Components/DeathComponent.generated.h"
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

	TupleVector<Entity, const DeathComponent*> components;
	world.getEntityManager().getComponentsWithEntities<const DeathComponent>(components);

	for (const auto& componentTuple : components)
	{
		world.getEntityManager().removeEntity(std::get<0>(componentTuple));
	}
}
