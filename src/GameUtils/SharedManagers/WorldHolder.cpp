#include "EngineCommon/precomp.h"

#include "GameUtils/SharedManagers/WorldHolder.h"

#include "GameData/WorldLayer.h"

WorldHolder::WorldHolder(WorldLayer& staticWorld, WorldLayer& reflectedWorld, GameData& gameData) noexcept
	: mDynamicWorld(nullptr)
	, mStaticWorld(staticWorld)
	, mReflectedWorld(reflectedWorld)
	, mGameData(gameData)
	, mAllEntitiesView({ { staticWorld.getEntityManager(), &staticWorld }, { reflectedWorld.getEntityManager(), &reflectedWorld } })
	, mMutableEntitiesView({ { staticWorld.getEntityManager(), &staticWorld } })
{
}

void WorldHolder::setDynamicWorld(WorldLayer& newDynamicWorld)
{
	mDynamicWorld = &newDynamicWorld;
	mAllEntitiesView = CombinedEntityManagerView{ {
		{ mDynamicWorld->getEntityManager(), mDynamicWorld },
		{ mStaticWorld.getEntityManager(), &mStaticWorld },
		{ mReflectedWorld.getEntityManager(), &mReflectedWorld },
	} };
	mMutableEntitiesView = CombinedEntityManagerView{ {
		{ mDynamicWorld->getEntityManager(), mDynamicWorld },
		{ mReflectedWorld.getEntityManager(), &mReflectedWorld },
	} };
}
