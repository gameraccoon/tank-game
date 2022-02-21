#include "addentitygroupcommand.h"

#include <QtWidgets/qcombobox.h>

#include "GameData/World.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Serialization/Json/EntityManager.h"

AddEntityGroupCommand::AddEntityGroupCommand(const std::vector<nlohmann::json>& entities, const Json::ComponentSerializationHolder& jsonSerializerHolder, const Vector2D& shift)
	: EditorCommand(EffectBitset(EffectType::Entities))
	, mEntities(entities)
	, mSerializationHolder(jsonSerializerHolder)
	, mShift(shift)
{
}

void AddEntityGroupCommand::doCommand(World* world)
{
	mCreatedEntities.clear();
	EntityManager& entityManager = world->getEntityManager();
	for (const auto& serializedObject : mEntities)
	{
		const Entity entity = Json::CreatePrefabInstance(entityManager, serializedObject, mSerializationHolder);
		auto [transform] = entityManager.getEntityComponents<TransformComponent>(entity);
		if (transform)
		{
			const Vector2D newPos = transform->getLocation() + mShift;
			transform->setLocation(newPos);
		}
		mCreatedEntities.push_back(entity);
	}
}

void AddEntityGroupCommand::undoCommand(World* world)
{
	EntityManager& entityManager = world->getEntityManager();
	for (const Entity entity : mCreatedEntities)
	{
		entityManager.removeEntity(entity);
	}
}
