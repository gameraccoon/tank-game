#pragma once

#include <nlohmann/json_fwd.hpp>

#include "GameData/EcsDefinitions.h"
#include "GameData/Serialization/Json/JsonComponentSerializer.h"

namespace Json
{
	nlohmann::json SerializeEntityManager(EntityManager& entityManager, const ComponentSerializationHolder& jsonSerializationHolder);
	void DeserializeEntityManager(EntityManager& outEntityManager, const nlohmann::json& json, const ComponentSerializationHolder& jsonSerializationHolder);

	void GetPrefabFromEntity(const EntityManager& entityManager, nlohmann::json& json, Entity entity, const ComponentSerializationHolder& jsonSerializationHolder);
	Entity CreatePrefabInstance(EntityManager& entityManager, const nlohmann::json& json, const ComponentSerializationHolder& jsonSerializationHolder);
	void ApplyPrefabToExistentEntity(EntityManager& entityManager, const nlohmann::json& json, Entity entity, const ComponentSerializationHolder& jsonSerializationHolder);
}
