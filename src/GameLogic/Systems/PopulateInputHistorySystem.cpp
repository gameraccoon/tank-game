#include "Base/precomp.h"

#include "GameLogic/Systems/PopulateInputHistorySystem.h"

#include "GameData/Components/GameplayInputComponent.generated.h"
#include "GameData/Components/InputHistoryComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Input/GameplayInput.h"
#include "GameData/World.h"

#include "GameLogic/SharedManagers/WorldHolder.h"


PopulateInputHistorySystem::PopulateInputHistorySystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void PopulateInputHistorySystem::update()
{
	SCOPED_PROFILER("PopulateInputHistorySystem::update");

	World& world = mWorldHolder.getWorld();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	const u32 lastUpdateIndex = time->getValue().lastFixedUpdateIndex;
	const u32 updatesThisFrame = time->getValue().countFixedTimeUpdatesThisFrame;
	AssertFatal(updatesThisFrame > 0, "updatesThisFrame can't be zero");

	const GameplayInputComponent* gameplayInput = world.getWorldComponents().getOrAddComponent<const GameplayInputComponent>();
	const GameplayInput::FrameState& gameplayInputState = gameplayInput->getCurrentFrameState();

	InputHistoryComponent* inputHistory = world.getNotRewindableWorldComponents().getOrAddComponent<InputHistoryComponent>();
	for (u32 i = 0; i < updatesThisFrame; ++i)
	{
		inputHistory->getInputsRef().push_back(gameplayInputState);
	}
	inputHistory->setLastInputUpdateIdx(lastUpdateIndex + updatesThisFrame);
}
