#include "Base/precomp.h"

#include "GameLogic/Systems/ClientInpuntSendSystem.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/GameplayInputComponent.generated.h"
#include "GameData/Components/InputHistoryComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/Input/GameplayInput.h"
#include "GameData/World.h"

#include "Utils/Network/Messages/PlayerInputMessage.h"

#include "GameLogic/SharedManagers/WorldHolder.h"
#include "GameLogic/SharedManagers/TimeData.h"


ClientInputSendSystem::ClientInputSendSystem(WorldHolder& worldHolder, const TimeData& timeData) noexcept
	: mWorldHolder(worldHolder)
	, mTimeData(timeData)
{
}

void ClientInputSendSystem::update()
{
	SCOPED_PROFILER("ClientInputSendSystem::update");

	World& world = mWorldHolder.getWorld();
	GameData& gameData = mWorldHolder.getGameData();
	EntityManager& entityManager = world.getEntityManager();
	const u32 lastUpdateIndex = mTimeData.lastFixedUpdateIndex;
	const u32 updatesThisFrame = mTimeData.countFixedTimeUpdatesThisFrame;

	auto [connectionManagerCmp] = gameData.getGameComponents().getComponents<ConnectionManagerComponent>();

	ClientGameDataComponent* clientGameData = world.getWorldComponents().getOrAddComponent<ClientGameDataComponent>();
	HAL::ConnectionManager* connectionManager = connectionManagerCmp->getManagerPtr();

	if (connectionManager == nullptr || clientGameData == nullptr)
	{
		return;
	}

	const ConnectionId connectionId = clientGameData->getClientConnectionId();
	if (connectionId == InvalidConnectionId || !connectionManager->isConnectionOpen(connectionId))
	{
		return;
	}


	const OptionalEntity controlledEntity = clientGameData->getControlledPlayer();

	if (!controlledEntity.isValid())
	{
		return;
	}

	auto [gameplayInput] = entityManager.getEntityComponents<GameplayInputComponent>(controlledEntity.getEntity());
	if (gameplayInput == nullptr)
	{
		gameplayInput = entityManager.addComponent<GameplayInputComponent>(controlledEntity.getEntity());
	}
	GameplayInput::FrameState& gameplayInputState = gameplayInput->getCurrentFrameStateRef();

	InputHistoryComponent* inputHistory = world.getNotRewindableWorldComponents().getOrAddComponent<InputHistoryComponent>();
	for (u32 i = 0; i < updatesThisFrame; ++i)
	{
		inputHistory->getInputsRef().push_back(gameplayInputState);
	}
	inputHistory->setLastInputUpdateIdx(lastUpdateIndex + updatesThisFrame - 1);

	connectionManager->sendMessage(
		connectionId,
		Network::CreatePlayerInputMessage(world),
		HAL::ConnectionManager::MessageReliability::Unreliable
	);
}
