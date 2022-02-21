#include "addspatialentitycommand.h"

#include <QtWidgets/qcombobox.h>

#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/World.h"

AddSpatialEntityCommand::AddSpatialEntityCommand(Entity entity, Vector2D location)
	: EditorCommand(EffectBitset(EffectType::Entities))
	, mEntity(entity)
	, mLocation(location)
{
}

void AddSpatialEntityCommand::doCommand(World* world)
{
	EntityManager& entityManager = world->getEntityManager();
	entityManager.addExistingEntityUnsafe(mEntity);
	TransformComponent* transform = entityManager.addComponent<TransformComponent>(mEntity);
	transform->setLocation(mLocation);
}

void AddSpatialEntityCommand::undoCommand(World* world)
{
	EntityManager& entityManager = world->getEntityManager();
	entityManager.removeEntity(mEntity);
}
