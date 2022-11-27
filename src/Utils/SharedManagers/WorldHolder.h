#pragma once

#include "Base/Debug/Assert.h"

class World;
class GameData;

class WorldHolder
{
public:
	WorldHolder(World* world, GameData& gameData)
		: mWorld(world)
		, mGameData(gameData)
	{
	}

	void setWorld(World& newWorld)
	{
		mWorld = &newWorld;
	}

	World& getWorld()
	{
		AssertFatal(mWorld, "World is not set");
		return *mWorld;
	}

	GameData& getGameData()
	{
		return mGameData;
	}

private:
	World* mWorld;
	GameData& mGameData;
};
