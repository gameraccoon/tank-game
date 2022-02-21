#pragma once

#include <optional>

#include "GameData/EcsDefinitions.h"

#include "GameData/Serialization/Json/JsonComponentSerializer.h"

class World
{
public:
	World(const ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator);

	World(const World&) = delete;
	World(World&&) = delete;
	World operator=(const World&) = delete;
	World operator=(World&&) = delete;
	~World() = default;

	[[nodiscard]] EntityManager& getEntityManager() { return mEntityManager; }
	[[nodiscard]] const EntityManager& getEntityManager() const { return mEntityManager; }

	[[nodiscard]] ComponentSetHolder& getWorldComponents() { return mWorldComponents; }
	[[nodiscard]] const ComponentSetHolder& getWorldComponents() const { return mWorldComponents; }

	[[nodiscard]] nlohmann::json toJson(const Json::ComponentSerializationHolder& jsonSerializerHolder);
	void fromJson(const nlohmann::json& json, const Json::ComponentSerializationHolder& jsonSerializerHolder);

	void clearCaches();

private:
	EntityManager mEntityManager;
	ComponentSetHolder mWorldComponents;
};
