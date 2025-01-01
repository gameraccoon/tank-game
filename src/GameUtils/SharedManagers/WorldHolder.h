#pragma once

#include "EngineCommon/Debug/Assert.h"

#include "GameData/EcsDefinitions.h"

class WorldLayer;
class GameData;

class WorldHolder
{
public:
	WorldHolder(WorldLayer& staticWorld, GameData& gameData) noexcept;

	void setDynamicWorld(WorldLayer& newDynamicWorld);

	WorldLayer& getDynamicWorldLayer()
	{
		AssertFatal(mDynamicWorld, "Dunamic world is not set");
		return *mDynamicWorld;
	}

	CombinedEntityManagerView& getFullWorld()
	{
		return mEntityManagerView;
	}

	WorldLayer& getStaticWorldLayer()
	{
		return mStaticWorld;
	}

	GameData& getGameData()
	{
		return mGameData;
	}

private:
	WorldLayer* mDynamicWorld;
	WorldLayer& mStaticWorld;
	GameData& mGameData;
	CombinedEntityManagerView mEntityManagerView;
};
