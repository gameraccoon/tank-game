#include "Base/precomp.h"

#include "Utils/SharedManagers/WorldHolder.h"

WorldHolder::WorldHolder(WorldLayer& staticWorld, GameData& gameData) noexcept
	: mDynamicWorld(nullptr)
	, mStaticWorld(staticWorld)
	, mGameData(gameData)
	, mEntityManagerView( {{ staticWorld.getEntityManager(), &staticWorld }})
{
}

void WorldHolder::setDynamicWorld(WorldLayer& newDynamicWorld)
{
	mDynamicWorld = &newDynamicWorld;
	mEntityManagerView = CombinedEntityManagerView{{{ mDynamicWorld->getEntityManager(), mDynamicWorld }, { mStaticWorld.getEntityManager(), &mStaticWorld }}};
}
