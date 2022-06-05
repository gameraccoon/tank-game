#pragma once

#include <optional>
#include <functional>

#include "GameData/EcsDefinitions.h"

#include "GameData/Serialization/Json/JsonComponentSerializer.h"

class World
{
public:
	struct FrameState
	{
		FrameState(const ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator);
		FrameState(const EntityManager& entityManager, const ComponentSetHolder& worldComponents);

		FrameState(const FrameState&) = default;
		FrameState& operator=(const FrameState&) = delete;
		FrameState(FrameState&&) = default;
		FrameState& operator=(FrameState&&) = default;

		void overrideBy(const FrameState& originalFrameState);

		EntityManager entityManager;
		ComponentSetHolder worldComponents;
	};

public:
	World(const ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator);

	World(const World&) = delete;
	World operator=(const World&) = delete;
	World(World&&) = delete;
	World operator=(World&&) = delete;
	~World() = default;

	[[nodiscard]] EntityManager& getEntityManager() { return mFrameHistory[mCurrentFrameIdx].entityManager; }
	[[nodiscard]] const EntityManager& getEntityManager() const { return mFrameHistory[mCurrentFrameIdx].entityManager; }

	[[nodiscard]] ComponentSetHolder& getWorldComponents() { return mFrameHistory[mCurrentFrameIdx].worldComponents; }
	[[nodiscard]] const ComponentSetHolder& getWorldComponents() const { return mFrameHistory[mCurrentFrameIdx].worldComponents; }

	[[nodiscard]] EntityManager& getNotRewindableEntityManager() { return mNotRewindableEntityManager; }
	[[nodiscard]] const EntityManager& getNotRewindableEntityManager() const { return mNotRewindableEntityManager; }

	[[nodiscard]] ComponentSetHolder& getNotRewindableWorldComponents() { return mNotRewindableComponents; }
	[[nodiscard]] const ComponentSetHolder& getNotRewindableWorldComponents() const { return mNotRewindableComponents; }

	[[nodiscard]] nlohmann::json toJson(const Json::ComponentSerializationHolder& jsonSerializerHolder);
	void fromJson(const nlohmann::json& json, const Json::ComponentSerializationHolder& jsonSerializerHolder);

	void clearCaches();

	void addNewFrameToTheHistory();
	void trimOldFrames(size_t oldFramesLeft);

	void unwindBackInHistory(size_t framesBackCount);

private:
	EntityManager mNotRewindableEntityManager;
	ComponentSetHolder mNotRewindableComponents;
	size_t mCurrentFrameIdx = 0;

	std::vector<FrameState> mFrameHistory;
};
