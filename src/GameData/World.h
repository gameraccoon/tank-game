#pragma once

#include "GameData/EcsDefinitions.h"

#include "GameData/Serialization/Json/JsonComponentSerializer.h"

class World
{
public:
	World(const ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator);

	World(const World&) = default;
	World& operator=(const World&) = delete;
	World(World&&) noexcept = default;
	World& operator=(World&&) noexcept = default;
	~World() = default;

	void overrideBy(const World& otherWorld);

	[[nodiscard]] EntityManager& getEntityManager() { return entityManager; }
	[[nodiscard]] const EntityManager& getEntityManager() const { return entityManager; }

	[[nodiscard]] ComponentSetHolder& getWorldComponents() { return worldComponents; }
	[[nodiscard]] const ComponentSetHolder& getWorldComponents() const { return worldComponents; }

	[[nodiscard]] nlohmann::json toJson(const Json::ComponentSerializationHolder& jsonSerializerHolder);
	void fromJson(const nlohmann::json& json, const Json::ComponentSerializationHolder& jsonSerializerHolder);

	void clearCaches();

private:
	EntityManager entityManager;
	ComponentSetHolder worldComponents;
};
