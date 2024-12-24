#pragma once

#include "EngineCommon/Debug/Assert.h"

#include "GameData/EcsDefinitions.h"

class WorldLayer;
class GameData;

class WorldHolder
{
public:
	WorldHolder(WorldLayer& staticWorld, WorldLayer& reflectedWorld, GameData& gameData) noexcept;

	void setDynamicWorld(WorldLayer& newDynamicWorld);

	// get the dynamic world, that is used for storing predictable and rewindable entities and components
	WorldLayer& getDynamicWorldLayer()
	{
		AssertFatal(mDynamicWorld, "Dunamic world is not set");
		return *mDynamicWorld;
	}

	// get a combination of all entity managers that compose the full world
	CombinedEntityManagerView& getAllEntities()
	{
		return mAllEntitiesView;
	}

	// get a combination of only entity managers that hold mutable entities
	CombinedEntityManagerView& getMutableEntities()
	{
		return mMutableEntitiesView;
	}

	// get the unchangeable world layer that is not affected by the gameplay
	WorldLayer& getStaticWorldLayer()
	{
		return mStaticWorld;
	}

	// get the world that stores non-owned entities that we don't predict and never rewind
	WorldLayer& getReflectedWorldLayer()
	{
		return mReflectedWorld;
	}

	GameData& getGameData()
	{
		return mGameData;
	}

private:
	WorldLayer* mDynamicWorld;
	WorldLayer& mStaticWorld;
	WorldLayer& mReflectedWorld;
	GameData& mGameData;
	CombinedEntityManagerView mAllEntitiesView;
	CombinedEntityManagerView mMutableEntitiesView;
};
