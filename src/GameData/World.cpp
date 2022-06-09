#include "Base/precomp.h"

#include "GameData/World.h"

#include <algorithm>
#include <nlohmann/json.hpp>

#include "GameData/Serialization/Json/ComponentSetHolder.h"
#include "GameData/Serialization/Json/EntityManager.h"
#include "GameData/Serialization/Json/JsonComponentSerializer.h"


World::FrameState::FrameState(const ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator)
	: entityManager(componentFactory, entityGenerator)
	, worldComponents(componentFactory)
{}

World::FrameState::FrameState(const EntityManager& entityManager, const ComponentSetHolder& worldComponents)
	: entityManager(entityManager)
	, worldComponents(worldComponents)
{}

void World::FrameState::overrideBy(const FrameState& originalFrameState)
{
	entityManager.overrideBy(originalFrameState.entityManager);
	worldComponents.overrideBy(originalFrameState.worldComponents);
}

World::World(const ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator)
	: mNotRewindableEntityManager(componentFactory, entityGenerator)
	, mNotRewindableComponents(componentFactory)
	, mCurrentFrameIdx(0)
{
	mFrameHistory.emplace_back(componentFactory, entityGenerator);
}

nlohmann::json World::toJson(const Json::ComponentSerializationHolder& jsonSerializerHolder)
{
	return nlohmann::json{
		{"entity_manager", Json::SerializeEntityManager(mFrameHistory[mCurrentFrameIdx].entityManager, jsonSerializerHolder)},
		{"world_components", Json::SerializeComponentSetHolder(mFrameHistory[mCurrentFrameIdx].worldComponents, jsonSerializerHolder)}
	};
}

void World::fromJson(const nlohmann::json& json, const Json::ComponentSerializationHolder& jsonSerializerHolder)
{
	Json::DeserializeEntityManager(mFrameHistory[mCurrentFrameIdx].entityManager, json.at("entity_manager"), jsonSerializerHolder);
	Json::DeserializeComponentSetHolder(mFrameHistory[mCurrentFrameIdx].worldComponents, json.at("world_components"), jsonSerializerHolder);
}

void World::clearCaches()
{
	mFrameHistory[mCurrentFrameIdx].entityManager.clearCaches();
	mNotRewindableEntityManager.clearCaches();
}

void World::addNewFrameToTheHistory()
{
	SCOPED_PROFILER("World::addNewFrameToTheHistory");
	AssertFatal(!mFrameHistory.empty(), "Frame history should always contain at least one frame");
	AssertFatal(mCurrentFrameIdx < mFrameHistory.size(), "mCurrentFrameIdx is out of bounds");
	if (mCurrentFrameIdx == mFrameHistory.size() - 1)
	{
		mFrameHistory.emplace_back(mFrameHistory[mCurrentFrameIdx]);
	}
	else
	{
		mFrameHistory[mCurrentFrameIdx + 1].overrideBy(mFrameHistory[mCurrentFrameIdx]);
	}
	++mCurrentFrameIdx;
}

void World::trimOldFrames(size_t oldFramesLeft)
{
	SCOPED_PROFILER("World::trimOldFrames");
	AssertFatal(oldFramesLeft <= mCurrentFrameIdx, "Can't keep more historical frames than we have, have: %u asked to keep: %u", mCurrentFrameIdx, oldFramesLeft);
	const size_t shiftLeft = mCurrentFrameIdx - oldFramesLeft;
	if (shiftLeft > 0)
	{
		std::rotate(mFrameHistory.begin(), mFrameHistory.begin() + shiftLeft, mFrameHistory.end());
		mCurrentFrameIdx -= shiftLeft;
	}
}

void World::unwindBackInHistory(size_t framesBackCount)
{
	if (framesBackCount >= mCurrentFrameIdx)
	{
		ReportFatalError("framesBackCount is too big for the current size of the history. framesBackCount is %u and history size is %u", framesBackCount, mCurrentFrameIdx);
		mCurrentFrameIdx = 0;
		return;
	}

	mCurrentFrameIdx -= framesBackCount;
}
