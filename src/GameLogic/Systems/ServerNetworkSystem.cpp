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
#include "GameData/Input/GameplayInput.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/Time/GameplayTimestamp.h"
#include "GameData/World.h"

#include "HAL/Network/ConnectionManager.h"

#include "Utils/Network/CompressedInput.h"

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
		case NetworkMessageId::PlayerInput:
		{
			size_t streamIndex = 0;
			Serialization::ByteStream& data = message.data;
			while (streamIndex < data.size())
			{
				const u32 networkId = Serialization::ReadNumber<u32>(data, streamIndex);
				const u32 frameIndex = Serialization::ReadNumber<u32>(data, streamIndex);
				const size_t receivedInputsCount = Serialization::ReadNumber<u8>(data, streamIndex);
				std::vector<GameplayInput::FrameState> receivedFrameStates = Utils::ReadInputHistory(data, receivedInputsCount, streamIndex);

				const auto it = networkIdMapping->getNetworkIdToEntity().find(networkId);
				if (it != networkIdMapping->getNetworkIdToEntity().end())
				{
					auto [inputHistory] = world.getEntityManager().getEntityComponents<InputHistoryComponent>(it->second);
					if (inputHistory)
					{
						const u32 lastStoredFrameIndex = inputHistory->getLastInputFrameIdx();
						if (frameIndex > lastStoredFrameIndex)
						{
							const size_t newFramesCount = frameIndex - lastStoredFrameIndex;
							const size_t newInputsCount = std::min(newFramesCount, receivedInputsCount);
							const size_t resultsOriginalSize = inputHistory->getInputs().size();
							const size_t resultsNewSize = resultsOriginalSize + newFramesCount;

							inputHistory->getInputsRef().resize(resultsNewSize);

							// add new elements to the end of the array
							const size_t firstIndexToWrite = resultsNewSize - newInputsCount;
							const size_t firstIndexToRead = receivedInputsCount - newInputsCount;
							for (size_t writeIdx = firstIndexToWrite, readIdx = firstIndexToRead; writeIdx < resultsNewSize; ++writeIdx, ++readIdx)
							{
								inputHistory->getInputsRef()[writeIdx] = receivedFrameStates[readIdx];
							}

							// if we have a gap in the inputs, fill it with the last input that we had before or the first input after
							const size_t firstMissingIndex = resultsNewSize - newFramesCount;
							const size_t indexToFillFrom = (resultsOriginalSize > 0) ? (resultsOriginalSize - 1) : firstIndexToWrite;
							const GameplayInput::FrameState& inputToFillWith = inputHistory->getInputs()[indexToFillFrom];
							for (size_t idx = firstMissingIndex; idx < firstIndexToWrite; ++idx)
							{
								inputHistory->getInputsRef()[idx] = inputToFillWith;
							}

							inputHistory->setLastInputFrameIdx(frameIndex);

							continue;
						}
					}
				}
			}
			break;
		}
		default:
			ReportError("Unhandled message");
		}
	}
}
