#include "removeentitiescommand.h"

#include <QtWidgets/qcombobox.h>

#include "GameData/World.h"
#include "GameData/Serialization/Json/EntityManager.h"

RemoveEntitiesCommand::RemoveEntitiesCommand(const std::vector<Entity>& entities, const Json::ComponentSerializationHolder& jsonSerializerHolder)
	: EditorCommand(EffectBitset(EffectType::Entities))
	, mEntities(entities)
	, mComponentSerializerHolder(jsonSerializerHolder)
{
}

void RemoveEntitiesCommand::doCommand(World* world)
{
	EntityManager& entityManager = world->getEntityManager();

	if (mSerializedComponents.empty())
	{
		mSerializedComponents.resize(mEntities.size());

		for (size_t i = 0, iSize = mEntities.size(); i < iSize; ++i)
		{
			Json::GetPrefabFromEntity(entityManager, mSerializedComponents[i], mEntities[i], mComponentSerializerHolder);
		}
	}

	for (const Entity entity : mEntities)
	{
		entityManager.removeEntity(entity);
	}
}

void RemoveEntitiesCommand::undoCommand(World* world)
{
	EntityManager& entityManager = world->getEntityManager();
	for (size_t i = 0, iSize = mEntities.size(); i < iSize; ++i)
	{
		entityManager.addExistingEntityUnsafe(mEntities[i]);
		Json::ApplyPrefabToExistentEntity(entityManager, mSerializedComponents[i], mEntities[i], mComponentSerializerHolder);
	}
}
