#include "Base/precomp.h"

#include "GameData/World.h"

#include <nlohmann/json.hpp>

#include "GameData/Serialization/Json/ComponentSetHolder.h"
#include "GameData/Serialization/Json/EntityManager.h"
#include "GameData/Serialization/Json/JsonComponentSerializer.h"


void World::overrideBy(const World& otherWorld)
{
	entityManager.overrideBy(otherWorld.entityManager);
	worldComponents.overrideBy(otherWorld.worldComponents);
}

World::World(const ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator)
	: entityManager(componentFactory, entityGenerator)
	, worldComponents(componentFactory)
{
}

nlohmann::json World::toJson(const Json::ComponentSerializationHolder& jsonSerializerHolder)
{
	return nlohmann::json{
		{"entity_manager", Json::SerializeEntityManager(entityManager, jsonSerializerHolder)},
		{"world_components", Json::SerializeComponentSetHolder(worldComponents, jsonSerializerHolder)}
	};
}

void World::fromJson(const nlohmann::json& json, const Json::ComponentSerializationHolder& jsonSerializerHolder)
{
	Json::DeserializeEntityManager(entityManager, json.at("entity_manager"), jsonSerializerHolder);
	Json::DeserializeComponentSetHolder(worldComponents, json.at("world_components"), jsonSerializerHolder);
}

void World::clearCaches()
{
	entityManager.clearCaches();
}
