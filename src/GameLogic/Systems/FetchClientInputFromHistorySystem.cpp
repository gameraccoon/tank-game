#include "Base/precomp.h"

#include "GameLogic/Systems/FetchClientInputFromHistorySystem.h"

#include "GameData/Components/GameplayInputComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/WorldLayer.h"

#include "Utils/Network/GameStateRewinder.h"
#include "Utils/SharedManagers/WorldHolder.h"

FetchClientInputFromHistorySystem::FetchClientInputFromHistorySystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
{
}

void FetchClientInputFromHistorySystem::update()
{
	SCOPED_PROFILER("FetchInputFromHistorySystem::update");
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	const u32 currentUpdateIndex = time->getValue()->lastFixedUpdateIndex;

	ComponentSetHolder& thisFrameWorldComponents = world.getWorldComponents();

	// it may be a bit strange that we calculate and then rewrite input with itself, but this is to try
	// to keep InputSystem generic and keep it useful for projects without state rewinding
	GameplayInputComponent* gameplayInput = thisFrameWorldComponents.getOrAddComponent<GameplayInputComponent>();
	gameplayInput->setCurrentFrameState(mGameStateRewinder.getInputForUpdate(currentUpdateIndex));
}
