#include "entityreferenceutils.h"

#include "GameData/World.h"

namespace Utils
{
	std::optional<EntityView> GetEntityView(Entity entity, World* world)
	{
		return EntityView(entity, world->getEntityManager());
	}
}
