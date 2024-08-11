#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/PopulateInputHistorySystem.h"

#include "GameData/Components/GameplayInputComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Input/GameplayInputFrameState.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/Network/GameStateRewinder.h"
#include "GameUtils/SharedManagers/WorldHolder.h"

PopulateInputHistorySystem::PopulateInputHistorySystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
{
}

void PopulateInputHistorySystem::update()
{
	SCOPED_PROFILER("PopulateInputHistorySystem::update");

	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	const u32 lastUpdateIndex = time->getValue()->lastFixedUpdateIndex;
	const u32 updatesThisFrame = time->getValue()->countFixedTimeUpdatesThisFrame;
	AssertFatal(updatesThisFrame > 0, "updatesThisFrame can't be zero");

	const GameplayInputComponent* gameplayInput = world.getWorldComponents().getOrAddComponent<const GameplayInputComponent>();
	const GameplayInput::FrameState& gameplayInputState = gameplayInput->getCurrentFrameState();

	for (u32 i = 0; i < updatesThisFrame; ++i)
	{
		mGameStateRewinder.setInputForUpdate(lastUpdateIndex + i + 1, gameplayInputState);
	}
}
