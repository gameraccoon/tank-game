#include "Base/precomp.h"

#include "GameLogic/Systems/SaveCommandsToHistorySystem.h"

#include "GameData/Components/GameplayCommandsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/World.h"

#include "Utils/Network/GameStateRewinder.h"
#include "Utils/SharedManagers/WorldHolder.h"


SaveCommandsToHistorySystem::SaveCommandsToHistorySystem(
		WorldHolder& worldHolder,
		GameStateRewinder& gameStateRewinder
	) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
{
}

void SaveCommandsToHistorySystem::update()
{
	SCOPED_PROFILER("SaveCommandsToHistorySystem::update");
	World& world = mWorldHolder.getWorld();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	const u32 currentUpdateIndex = time->getValue()->lastFixedUpdateIndex;

	const GameplayCommandsComponent* gameplayCommands = world.getWorldComponents().getOrAddComponent<const GameplayCommandsComponent>();

	mGameStateRewinder.overrideCommandsOneUpdate(currentUpdateIndex, gameplayCommands->getData());
}
