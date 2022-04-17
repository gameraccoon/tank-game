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


ClientInputSendSystem::ClientInputSendSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void ClientInputSendSystem::update()
{
	SCOPED_PROFILER("ClientInputSendSystem::update");

	World& world = mWorldHolder.getWorld();
	GameData& gameData = mWorldHolder.getGameData();

	world.getEntityManager().forEachComponentSet<InputHistoryComponent, const MovementComponent>(
		[](InputHistoryComponent* inputHistory, const MovementComponent* movement)
	{
		auto& inputs = inputHistory->getMovementInputsRef();
		size_t nextInput = inputHistory->getMovementInputsLastIdx() + 1;
		if (nextInput >= inputs.size())
		{
			nextInput = 0;
		}

		inputs[nextInput] = movement->getMoveDirection();
		inputHistory->setMovementInputsFilledSize(std::min(inputHistory->getMovementInputsFilledSize() + 1, inputs.size()));
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

	std::vector<std::byte> inputHistoryMessageData;
	entityManager.forEachComponentSet<const InputHistoryComponent, const NetworkIdComponent>(
		[&inputHistoryMessageData](const InputHistoryComponent* inputHistory, const NetworkIdComponent* networkId)
	{
		Serialization::WriteNumber<u32>(inputHistoryMessageData, networkId->getId());
		const size_t inputsToSend = std::min(inputHistory->getMovementInputsFilledSize(), static_cast<size_t>(10));

		Serialization::WriteNumber<u8>(inputHistoryMessageData, inputsToSend);
		const size_t lastIndex = inputHistory->getMovementInputsLastIdx();
		const size_t arraySize = inputHistory->getMovementInputs().size();

		for (size_t i = 0; i < inputsToSend; ++i)
		{
			const size_t offset = inputsToSend - i - 1;
			const size_t index = (lastIndex + arraySize - offset) % arraySize;
			const Vector2D input = inputHistory->getMovementInputs()[index];
			Serialization::WriteNumber<f32>(inputHistoryMessageData, input.x);
			Serialization::WriteNumber<f32>(inputHistoryMessageData, input.y);
		}
	});

	if (!inputHistoryMessageData.empty())
	{
		connectionManager->sendMessage(
			connectionId,
			HAL::ConnectionManager::Message{
				static_cast<u32>(NetworkMessageId::PlayerMove),
				std::move(inputHistoryMessageData)
			}
		);
	}
}
