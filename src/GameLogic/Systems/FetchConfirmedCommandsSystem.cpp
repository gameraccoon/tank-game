#include "Base/precomp.h"

#include "GameLogic/Systems/FetchConfirmedCommandsSystem.h"

#include "GameData/Components/GameplayCommandsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/WorldLayer.h"

#include "Utils/Network/GameStateRewinder.h"
#include "Utils/SharedManagers/WorldHolder.h"

FetchConfirmedCommandsSystem::FetchConfirmedCommandsSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
{
}

void FetchConfirmedCommandsSystem::update()
{
	SCOPED_PROFILER("FetchConfirmedCommandsSystem::update");
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	const u32 currentUpdateIndex = time->getValue()->lastFixedUpdateIndex;

	// if we have confirmed commands for this frame, apply them instead of what we generated last frame
	if (mGameStateRewinder.hasConfirmedCommandsForUpdate(currentUpdateIndex))
	{
		GameplayCommandsComponent* gameplayCommands = world.getWorldComponents().getOrAddComponent<GameplayCommandsComponent>();
		gameplayCommands->setData(mGameStateRewinder.getCommandsForUpdate(currentUpdateIndex).gameplayGeneratedCommands);
	}
}
