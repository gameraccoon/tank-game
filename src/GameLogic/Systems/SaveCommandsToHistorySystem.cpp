#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/SaveCommandsToHistorySystem.h"

#include "GameData/Components/GameplayCommandsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/Network/GameStateRewinder.h"
#include "GameUtils/SharedManagers/WorldHolder.h"


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
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	const u32 currentUpdateIndex = time->getValue()->lastFixedUpdateIndex;

	const GameplayCommandsComponent* gameplayCommands = world.getWorldComponents().getOrAddComponent<const GameplayCommandsComponent>();

	mGameStateRewinder.writeSimulatedCommands(currentUpdateIndex + 1, gameplayCommands->getData());
}
