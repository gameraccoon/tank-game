#include "changeentitygrouplocationcommand.h"

#include <GameData/World.h>
#include <GameData/Components/TransformComponent.generated.h>

ChangeEntityGroupLocationCommand::ChangeEntityGroupLocationCommand(const std::vector<Entity>& entities, Vector2D shift)
	: EditorCommand(EffectBitset(EffectType::ComponentAttributes, EffectType::EntityLocations))
	, mShift(shift)
	, mOriginalEntities(entities)
{
}

void ChangeEntityGroupLocationCommand::doCommand(World* world)
{
	mOriginalEntitiesPos.reserve(mOriginalEntities.size());
	mModifiedEntities.reserve(mOriginalEntities.size());
	mModifiedEntitiesPos.reserve(mOriginalEntities.size());
	EntityManager& entityManager = world->getEntityManager();

	for (size_t i = 0; i < mOriginalEntities.size(); ++i)
	{
		const Entity entity = mOriginalEntities[i];
		auto [component] = entityManager.getEntityComponents<TransformComponent>(entity);
		if (component)
		{
			const Vector2D originalPos = component->getLocation();
			mOriginalEntitiesPos.push_back(originalPos);
			const Vector2D newPos = originalPos + mShift;
			mModifiedEntitiesPos.push_back(newPos);
			component->setLocation(newPos);
			mModifiedEntities.push_back(entity);
		}
	}
}

void ChangeEntityGroupLocationCommand::undoCommand(World* world)
{
	EntityManager& entityManager = world->getEntityManager();
	for (size_t i = 0; i < mModifiedEntities.size(); ++i)
	{
		const Entity entity = mModifiedEntities[i];
		auto [component] = entityManager.getEntityComponents<TransformComponent>(entity);
		if (component)
		{
			const Vector2D newLocation = mOriginalEntitiesPos[i];
			component->setLocation(newLocation);
		}
	}
}
