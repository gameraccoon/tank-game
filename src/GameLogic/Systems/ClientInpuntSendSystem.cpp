#include "Base/precomp.h"

#include "GameLogic/Systems/ClientInpuntSendSystem.h"

#include "Base/Types/Serialization.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/GameplayInputComponent.generated.h"
#include "GameData/Components/InputHistoryComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/Input/GameplayInput.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/World.h"

#include "Utils/Network/CompressedInput.h"

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
	const u32 currentFrameIndex = mTimeData.lastFixedUpdateIndex;
	const u32 updatesThisFrame = mTimeData.countFixedTimeUpdatesThisFrame;

	GameplayInputComponent* gameplayInput = world.getWorldComponents().getOrAddComponent<GameplayInputComponent>();
	GameplayInput::FrameState& gameplayInputState = gameplayInput->getCurrentFrameStateRef();

	world.getEntityManager().forEachComponentSet<InputHistoryComponent>(
		[currentFrameIndex, updatesThisFrame, &gameplayInputState](InputHistoryComponent* inputHistory)
	{
		for (u32 i = 0; i < updatesThisFrame; ++i)
		{
			inputHistory->getInputsRef().push_back(gameplayInputState);
		}
		inputHistory->setLastInputFrameIdx(currentFrameIndex + updatesThisFrame - 1);
	});

	auto [connectionManagerCmp] = gameData.getGameComponents().getComponents<ConnectionManagerComponent>();
	auto [clientGameData] = world.getWorldComponents().getComponents<ClientGameDataComponent>();

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

	EntityManager& entityManager = world.getEntityManager();

	size_t entitiesCount = entityManager.getMatchingEntitiesCount<const InputHistoryComponent, const NetworkIdComponent>();
	if (entitiesCount == 0)
	{
		return;
	}

	std::vector<std::byte> inputHistoryMessageData;
	inputHistoryMessageData.reserve(1 + entitiesCount * (4 + 1 + 10 * (4 + 4)));
	entityManager.forEachComponentSet<const InputHistoryComponent, const NetworkIdComponent>(
		[&inputHistoryMessageData](const InputHistoryComponent* inputHistory, const NetworkIdComponent* networkId)
	{
		Serialization::WriteNumber<u32>(inputHistoryMessageData, networkId->getId());
		Serialization::WriteNumber<u32>(inputHistoryMessageData, inputHistory->getLastInputFrameIdx());

		const size_t inputsSize = inputHistory->getInputs().size();
		const size_t inputsToSend = std::min(inputsSize, static_cast<size_t>(10));

		Serialization::WriteNumber<u8>(inputHistoryMessageData, inputsToSend);

		Utils::WriteInputHistory(inputHistoryMessageData, inputHistory->getInputs(), inputsToSend);
	});

	if (!inputHistoryMessageData.empty())
	{
		connectionManager->sendMessage(
			connectionId,
			HAL::ConnectionManager::Message{
				static_cast<u32>(NetworkMessageId::PlayerInput),
				std::move(inputHistoryMessageData)
			}
		);
	}
}
