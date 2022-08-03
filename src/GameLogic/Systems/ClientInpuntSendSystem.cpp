#include "Base/precomp.h"

#include "GameLogic/Systems/ClientInpuntSendSystem.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/GameplayInputComponent.generated.h"
#include "GameData/Components/InputHistoryComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/Input/GameplayInput.h"
#include "GameData/World.h"

#include "Utils/Network/Messages/PlayerInputMessage.h"

#include "HAL/Network/ConnectionManager.h"

#include "GameLogic/SharedManagers/WorldHolder.h"


ClientInputSendSystem::ClientInputSendSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void ClientInputSendSystem::update()
{
	SCOPED_PROFILER("ClientInputSendSystem::update");

	World& world = mWorldHolder.getWorld();
	GameData& gameData = mWorldHolder.getGameData();
	EntityManager& entityManager = world.getEntityManager();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	const u32 lastUpdateIndex = time->getValue().lastFixedUpdateIndex;
	const u32 updatesThisFrame = time->getValue().countFixedTimeUpdatesThisFrame;

	auto [connectionManagerCmp] = gameData.getGameComponents().getComponents<ConnectionManagerComponent>();

	ClientGameDataComponent* clientGameData = world.getWorldComponents().getOrAddComponent<ClientGameDataComponent>();
	HAL::ConnectionManager* connectionManager = connectionManagerCmp->getManagerPtr();

	if (connectionManager == nullptr || clientGameData == nullptr)
	{
		return;
	}

	const ConnectionId connectionId = clientGameData->getClientConnectionId();
	if (connectionId == InvalidConnectionId || !connectionManager->isServerConnectionOpen(connectionId))
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
	const GameplayInput::FrameState& gameplayInputState = gameplayInput->getCurrentFrameState();

	InputHistoryComponent* inputHistory = world.getNotRewindableWorldComponents().getOrAddComponent<InputHistoryComponent>();
	for (u32 i = 0; i < updatesThisFrame; ++i)
	{
		inputHistory->getInputsRef().push_back(gameplayInputState);
	}
	inputHistory->setLastInputUpdateIdx(lastUpdateIndex + updatesThisFrame);

	connectionManager->sendMessageToServer(
		connectionId,
		Network::CreatePlayerInputMessage(world),
		HAL::ConnectionManager::MessageReliability::Unreliable
	);
}
