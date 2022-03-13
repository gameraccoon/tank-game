#pragma once

#include <vector>

#include <nlohmann/json.hpp>

#include "editorcommand.h"

#include "GameData/Geometry/Vector2D.h"
#include "GameData/Serialization/Json/JsonComponentSerializer.h"

class World;

class AddEntityGroupCommand : public EditorCommand
{
public:
	AddEntityGroupCommand(const std::vector<nlohmann::json>& entities, const Json::ComponentSerializationHolder& jsonSerializerHolder, const Vector2D& shift);

	void doCommand(World* world) override;
	void undoCommand(World* world) override;

private:
	std::vector<nlohmann::json> mEntities;
	std::vector<Entity> mCreatedEntities;
	const Json::ComponentSerializationHolder& mSerializationHolder;
	Vector2D mShift;
};
