#pragma once

#include <optional>

#include "GameData/EcsDefinitions.h"

#include "GameData/Serialization/Json/JsonComponentSerializer.h"

class World
{
public:
	struct FrameState
	{
		std::unique_ptr<EntityManager> entityManager;
		std::unique_ptr<ComponentSetHolder> worldComponents;
	};

public:
	World(const ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator);

	World(const World&) = delete;
	World operator=(const World&) = delete;
	World(World&&) = delete;
	World operator=(World&&) = delete;
	~World() = default;

	[[nodiscard]] EntityManager& getEntityManager() { return mEntityManager; }
	[[nodiscard]] const EntityManager& getEntityManager() const { return mEntityManager; }

	[[nodiscard]] ComponentSetHolder& getWorldComponents() { return mWorldComponents; }
	[[nodiscard]] const ComponentSetHolder& getWorldComponents() const { return mWorldComponents; }

	[[nodiscard]] EntityManager& getNotRewindableEntityManager() { return mNotRewindableEntityManager; }
	[[nodiscard]] const EntityManager& getNotRewindableEntityManager() const { return mNotRewindableEntityManager; }

	[[nodiscard]] ComponentSetHolder& getNotRewindableWorldComponents() { return mNotRewindableComponents; }
	[[nodiscard]] const ComponentSetHolder& getNotRewindableWorldComponents() const { return mNotRewindableComponents; }

	[[nodiscard]] nlohmann::json toJson(const Json::ComponentSerializationHolder& jsonSerializerHolder);
	void fromJson(const nlohmann::json& json, const Json::ComponentSerializationHolder& jsonSerializerHolder);

	void clearCaches();

	std::vector<FrameState>& getFrameHistoryRef() { return mFrameHistory; }
	void addNewFrameToTheHistory();
	void trimOldFrames(size_t oldFramesLeft);

private:
	EntityManager mEntityManager;
	ComponentSetHolder mWorldComponents;
	EntityManager mNotRewindableEntityManager;
	ComponentSetHolder mNotRewindableComponents;

	std::vector<FrameState> mFrameHistory;
};
