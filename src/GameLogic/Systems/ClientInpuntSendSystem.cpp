#include "Base/precomp.h"

#include "GameLogic/Systems/ClientInpuntSendSystem.h"

#include "Base/Types/Serialization.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/InputHistoryComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/World.h"

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
	const u32 currentFrameIndex = mTimeData.frameNumber;

	world.getEntityManager().forEachComponentSet<InputHistoryComponent, const MovementComponent>(
		[currentFrameIndex](InputHistoryComponent* inputHistory, const MovementComponent* movement)
	{
		inputHistory->getMovementInputsRef().push_back(movement->getMoveDirection());
		inputHistory->setLastInputFrameIdx(currentFrameIndex);
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

		const size_t inputsSize = inputHistory->getMovementInputs().size();
		const size_t inputsToSend = std::min(inputsSize, static_cast<size_t>(10));

		Serialization::WriteNumber<u8>(inputHistoryMessageData, inputsToSend);

		const size_t offset = inputsSize - inputsToSend;
		for (size_t i = 0; i < inputsToSend; ++i)
		{
			const Vector2D input = inputHistory->getMovementInputs()[offset + i];
			Serialization::WriteNumber<f32>(inputHistoryMessageData, input.x);
			Serialization::WriteNumber<f32>(inputHistoryMessageData, input.y);
		}
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
