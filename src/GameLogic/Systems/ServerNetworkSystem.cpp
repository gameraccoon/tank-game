#include "Base/precomp.h"

#include "GameLogic/Systems/ServerNetworkSystem.h"

#include "Base/Types/Serialization.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/ImguiComponent.generated.h"
#include "GameData/Components/InputHistoryComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/RenderModeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/World.h"

#include "HAL/Network/ConnectionManager.h"

#include "GameLogic/SharedManagers/WorldHolder.h"


ServerNetworkSystem::ServerNetworkSystem(WorldHolder& worldHolder, bool& shouldQuitGame) noexcept
	: mWorldHolder(worldHolder)
	, mShouldQuitGame(shouldQuitGame)
{
}

void ServerNetworkSystem::update()
{
	SCOPED_PROFILER("ServerNetworkSystem::update");

	World& world = mWorldHolder.getWorld();
	GameData& gameData = mWorldHolder.getGameData();

	auto [connectionManagerCmp] = gameData.getGameComponents().getComponents<ConnectionManagerComponent>();

	HAL::ConnectionManager* connectionManager = connectionManagerCmp->getManagerPtr();

	if (connectionManager == nullptr)
	{
		return;
	}

	if (!connectionManager->isPortOpen(12345))
	{
		connectionManager->startListeningToPort(12345);
		return;
	}

	NetworkIdMappingComponent* networkIdMapping = world.getWorldComponents().getOrAddComponent<NetworkIdMappingComponent>();

	auto newMessages = connectionManager->consumeReceivedMessages(12345);

	for (auto& [_, message] : newMessages)
	{
		switch (static_cast<NetworkMessageId>(message.type))
		{
		case NetworkMessageId::Disconnect:
			mShouldQuitGame = true;
			break;
		case NetworkMessageId::PlayerMove:
		{
			size_t streamIndex = 0;
			Serialization::ByteStream& data = message.data;
			while (streamIndex < data.size())
			{
				const u32 networkId = Serialization::ReadNumber<u32>(data, streamIndex);
				const u8 inputsToRead = Serialization::ReadNumber<u8>(data, streamIndex);

				const auto it = networkIdMapping->getNetworkIdToEntity().find(networkId);
				if (it != networkIdMapping->getNetworkIdToEntity().end())
				{
					auto [inputHistory] = world.getEntityManager().getEntityComponents<InputHistoryComponent>(it->second);
					if (inputHistory)
					{
						const size_t arraySize = inputHistory->getMovementInputs().size();
						const size_t lastIndex = inputHistory->getMovementInputsLastIdx();
						inputHistory->setMovementInputsFilledSize(std::min(arraySize, inputHistory->getMovementInputsFilledSize() + inputsToRead));
						inputHistory->setMovementInputsLastIdx(lastIndex);
						for (size_t i = 0; i < inputsToRead; ++i)
						{
							const size_t offset = inputsToRead - i - 1;
							const size_t index = (lastIndex + arraySize - offset) % arraySize;
							Vector2D& input = inputHistory->getMovementInputsRef()[index];
							input.x = Serialization::ReadNumber<f32>(data, streamIndex);
							input.y = Serialization::ReadNumber<f32>(data, streamIndex);
						}
					}
				}
				else
				{
					streamIndex += inputsToRead * 4 * 2;
				}
			}
			break;
		}
		default:
			ReportError("Unhandled message");
		}
	}
}
