#pragma once

#include "GameData/EcsDefinitions.h"
#include "GameData/Serialization/Json/JsonComponentSerializer.h"

class WorldLayer
{
public:
	explicit WorldLayer(const ComponentFactory& componentFactory);

	WorldLayer(const WorldLayer&) = default;
	WorldLayer& operator=(const WorldLayer&) = delete;
	WorldLayer(WorldLayer&&) noexcept = default;
	WorldLayer& operator=(WorldLayer&&) noexcept = default;
	~WorldLayer() = default;

	void overrideBy(const WorldLayer& otherWorld);

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
