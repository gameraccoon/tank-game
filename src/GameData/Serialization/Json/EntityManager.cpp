#include "EngineCommon/precomp.h"

#include "GameData/Serialization/Json/EntityManager.h"

#include <nlohmann/json.hpp>
#include <soasort.h>

namespace Json
{
	nlohmann::json SerializeEntityManager(EntityManager& entityManager, const ComponentSerializationHolder& jsonSerializationHolder)
	{
		entityManager.clearCaches();

		const std::vector<Entity> sortedEntities = entityManager.collectAllEntities();
		nlohmann::json entitiesJson;
		for (Entity entity : sortedEntities)
		{
			entitiesJson.push_back(entity.getRawId());
		}

		nlohmann::json outJson{
			{ "entities", entitiesJson }
		};

		auto components = nlohmann::json{};

		for (const auto& [componentId, rawComponents] : entityManager.getComponentsData())
		{
			auto componentArrayObject = nlohmann::json::array();
			if (const ComponentSerializer* jsonSerializer = jsonSerializationHolder.getComponentSerializerFromClassName(componentId))
			{
				for (auto& component : rawComponents)
				{
					auto componentObj = nlohmann::json{};
					if (component != nullptr)
					{
						jsonSerializer->toJson(componentObj, component);
					}
					componentArrayObject.push_back(componentObj);
				}
				components[ID_TO_STR(componentId)] = componentArrayObject;
			}
		}
		outJson["components"] = components;

		return outJson;
	}

	void DeserializeEntityManager(EntityManager& outEntityManager, const nlohmann::json& json, const ComponentSerializationHolder& jsonSerializationHolder)
	{
		// make sure the manager is fully reset
		outEntityManager.clear();

		// entities go in order of component indexes, starting from zero
		const auto& entitiesJson = json.at("entities");

		std::unordered_map<Entity::RawId, Entity> entityMap;
		std::vector<Entity::RawId> rawIds;
		for (const auto& entityData : entitiesJson)
		{
			Entity::RawId rawIdx = entityData.get<Entity::RawId>();
			const Entity newEntity = outEntityManager.addEntity();
			entityMap.emplace(rawIdx, newEntity);
			rawIds.push_back(rawIdx);
		}

		const auto& components = json.at("components");
		for (const auto& [typeStr, vector] : components.items())
		{
			const StringId type = STR_TO_ID(typeStr);
			if (const ComponentSerializer* jsonSerializer = jsonSerializationHolder.getComponentSerializerFromClassName(type))
			{
				size_t entityIndex = 0;
				for (const auto& componentData : vector)
				{
					if (!componentData.is_null())
					{
						void* component = outEntityManager.addComponentByType(entityMap.find(rawIds[entityIndex])->second, type);
						jsonSerializer->fromJson(componentData, component);
					}
					++entityIndex;
				}
			}
			else
			{
				ReportFatalError("Unknown component %s", type);
			}
		}
	}

	void GetPrefabFromEntity(const EntityManager& entityManager, nlohmann::json& json, const Entity entity, const ComponentSerializationHolder& jsonSerializationHolder)
	{
		std::vector<ConstTypedComponent> components;
		entityManager.getAllEntityComponents(entity, components);

		for (const ConstTypedComponent& componentData : components)
		{
			auto componentObj = nlohmann::json{};
			const StringId componentTypeName = componentData.typeId;
			if (const ComponentSerializer* componentSerializer = jsonSerializationHolder.getComponentSerializerFromClassName(componentTypeName))
			{
				componentSerializer->toJson(componentObj, componentData.component);
				json[ID_TO_STR(componentData.typeId)] = componentObj;
			}
		}
	}

	Entity CreatePrefabInstance(EntityManager& entityManager, const nlohmann::json& json, const ComponentSerializationHolder& jsonSerializationHolder)
	{
		const Entity entity = entityManager.addEntity();
		ApplyPrefabToExistentEntity(entityManager, json, entity, jsonSerializationHolder);
		return entity;
	}

	void ApplyPrefabToExistentEntity(EntityManager& entityManager, const nlohmann::json& json, const Entity entity, const ComponentSerializationHolder& jsonSerializationHolder)
	{
		for (const auto& [componentTypeNameStr, componentObj] : json.items())
		{
			const StringId componentTypeName = STR_TO_ID(componentTypeNameStr);

			void* component = entityManager.addComponentByType(entity, componentTypeName);
			if (const ComponentSerializer* componentSerializer = jsonSerializationHolder.getComponentSerializerFromClassName(componentTypeName))
			{
				componentSerializer->fromJson(componentObj, component);
			}
		}
	}
} // namespace Json
