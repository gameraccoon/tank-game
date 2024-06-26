#include "EngineCommon/precomp.h"

#include <nlohmann/json.hpp>

#include "GameData/Serialization/Json/ComponentSetHolder.h"
#include "GameData/Serialization/Json/EntityManager.h"
#include "GameData/Serialization/Json/JsonComponentSerializer.h"
#include "GameData/WorldLayer.h"

void WorldLayer::overrideBy(const WorldLayer& otherWorld)
{
	entityManager.overrideBy(otherWorld.entityManager);
	worldComponents.overrideBy(otherWorld.worldComponents);
}

WorldLayer::WorldLayer(const ComponentFactory& componentFactory)
	: entityManager(componentFactory)
	, worldComponents(componentFactory)
{
}

nlohmann::json WorldLayer::toJson(const Json::ComponentSerializationHolder& jsonSerializerHolder)
{
	return nlohmann::json{
		{"entity_manager", Json::SerializeEntityManager(entityManager, jsonSerializerHolder)},
		{"world_components", Json::SerializeComponentSetHolder(worldComponents, jsonSerializerHolder)}
	};
}

void WorldLayer::fromJson(const nlohmann::json& json, const Json::ComponentSerializationHolder& jsonSerializerHolder)
{
	Json::DeserializeEntityManager(entityManager, json.at("entity_manager"), jsonSerializerHolder);
	Json::DeserializeComponentSetHolder(worldComponents, json.at("world_components"), jsonSerializerHolder);
}

void WorldLayer::clearCaches()
{
	entityManager.clearCaches();
}
