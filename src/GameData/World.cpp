#include "Base/precomp.h"

#include "GameData/World.h"

#include <nlohmann/json.hpp>

#include "GameData/Serialization/Json/ComponentSetHolder.h"
#include "GameData/Serialization/Json/EntityManager.h"
#include "GameData/Serialization/Json/JsonComponentSerializer.h"

World::World(const ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator)
	: mEntityManager(componentFactory, entityGenerator)
	, mWorldComponents(componentFactory)
	, mNotRewindableEntityManager(componentFactory, entityGenerator)
	, mNotRewindableComponents(componentFactory)
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
	mNotRewindableEntityManager.clearCaches();
}

void World::addNewFrameToTheHistory()
{
	SCOPED_PROFILER("World::addNewFrameToTheHistory");
	mFrameHistory.emplace_back(mEntityManager.clone(), mWorldComponents.clone());
}

void World::trimOldFrames(size_t oldFramesLeft)
{
	SCOPED_PROFILER("World::trimOldFrames");
	if (oldFramesLeft > mFrameHistory.size())
	{
		ReportError("Can't leave more frames than already exist");
		return;
	}
	mFrameHistory.erase(mFrameHistory.begin(), mFrameHistory.begin() + (mFrameHistory.size() - oldFramesLeft));
}
