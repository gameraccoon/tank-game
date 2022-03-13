#pragma once

#include "editorcommand.h"

#include "GameData/Geometry/Vector2D.h"

class World;

class ChangeEntityGroupLocationCommand : public EditorCommand
{
public:
	using SetterFunction = void (World::*)(const OptionalEntity&);

public:
	ChangeEntityGroupLocationCommand(const std::vector<Entity>& entities, Vector2D shift);

	void doCommand(World* world) override;
	void undoCommand(World* world) override;

	const std::vector<Entity>& getOriginalEntities() const { return mOriginalEntities; }
	const std::vector<Entity>& getModifiedEntities() const { return mModifiedEntities; }

private:
	const Vector2D mShift;
	const std::vector<Entity> mOriginalEntities;
	std::vector<Vector2D> mOriginalEntitiesPos;

	std::vector<Entity> mModifiedEntities;
	std::vector<Vector2D> mModifiedEntitiesPos;
};
