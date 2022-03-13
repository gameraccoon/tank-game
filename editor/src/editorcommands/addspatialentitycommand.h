#pragma once

#include "GameData/Geometry/Vector2D.h"

#include "editorcommand.h"

class World;

class AddSpatialEntityCommand : public EditorCommand
{
public:
	AddSpatialEntityCommand(Entity entity, Vector2D location);

	void doCommand(World* world) override;
	void undoCommand(World* world) override;

private:
	Entity mEntity;
	Vector2D mLocation;
};
