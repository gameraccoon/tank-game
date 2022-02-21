#include "Base/precomp.h"

#include "GameData/World.h"

#include <nlohmann/json.hpp>

#include "GameData/Serialization/Json/ComponentSetHolder.h"
#include "GameData/Serialization/Json/EntityManager.h"
#include "GameData/Serialization/Json/JsonComponentSerializer.h"

World::World(const ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator)
	: mEntityManager(componentFactory, entityGenerator)
	, mWorldComponents(componentFactory)
{
}

nlohmann::json World::toJson(const Json::ComponentSerializationHolder& jsonSerializerHolder)
{
	return nlohmann::json{
		{"entity_manager", Json::SerializeEntityManager(mEntityManager, jsonSerializerHolder)},
		{"world_components", Json::SerializeComponentSetHolder(mWorldComponents, jsonSerializerHolder)}
	};
}

void World::fromJson(const nlohmann::json& json, const Json::ComponentSerializationHolder& jsonSerializerHolder)
{
	Json::DeserializeEntityManager(mEntityManager, json.at("entity_manager"), jsonSerializerHolder);
	Json::DeserializeComponentSetHolder(mWorldComponents, json.at("world_components"), jsonSerializerHolder);
}

void World::clearCaches()
{
	mEntityManager.clearCaches();
}
