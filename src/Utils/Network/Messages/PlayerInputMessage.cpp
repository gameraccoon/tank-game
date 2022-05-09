#include "Base/precomp.h"

#include "Utils/Network/Messages/PlayerInputMessage.h"

#include "Base/Types/Serialization.h"
#include "Base/Types/BasicTypes.h"

#include "GameData/Components/InputHistoryComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/World.h"

#include "Utils/Network/CompressedInput.h"

#include "HAL/Network/ConnectionManager.h"

namespace Network
{
	HAL::ConnectionManager::Message CreatePlayerInputMessage(World& world)
	{
		std::vector<std::byte> inputHistoryMessageData;
		InputHistoryComponent* inputHistory = world.getNotRewindableWorldComponents().getOrAddComponent<InputHistoryComponent>();

		const size_t inputsSize = inputHistory->getInputs().size();
		const size_t inputsToSend = std::min(inputsSize, static_cast<size_t>(10));

		inputHistoryMessageData.reserve(4 + 1 + inputsToSend * ((4 + 4) * 2 + (1 + 8) * 1));

		Serialization::WriteNumber<u32>(inputHistoryMessageData, inputHistory->getLastInputFrameIdx());
		Serialization::WriteNumber<u8>(inputHistoryMessageData, inputsToSend);

		Utils::WriteInputHistory(inputHistoryMessageData, inputHistory->getInputs(), inputsToSend);

		return HAL::ConnectionManager::Message{
			static_cast<u32>(NetworkMessageId::PlayerInput),
			std::move(inputHistoryMessageData)
		};
	}

	static bool hasNewInput(u32 oldFrameIndex, u32 newFrameIndex)
	{
		constexpr u32 WRAP_ZONE_SIZE = 1024;
		constexpr u32 WRAP_MIN = std::numeric_limits<u32>::max() - WRAP_ZONE_SIZE;
		constexpr u32 WRAP_MAX = WRAP_ZONE_SIZE;

		const bool oldAboutToWrap = (oldFrameIndex > WRAP_MIN);
		const bool newAboutToWrap = (newFrameIndex > WRAP_MIN);
		const bool oldAfterWrap = (oldFrameIndex < WRAP_MAX);
		const bool newAfterWrap = (newFrameIndex < WRAP_MAX);

		if ALMOST_NEVER(oldAboutToWrap && newAfterWrap)
		{
			return true;
		}
		else if ALMOST_NEVER(newAboutToWrap && oldAfterWrap)
		{
			return false;
		}
		else
		{
			return oldFrameIndex < newFrameIndex;
		}
	}

	void ApplyPlayerInputMessage(World& world, HAL::ConnectionManager::Message&& message, ConnectionId connectionId)
	{
		ServerConnectionsComponent* serverConnections = world.getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();

		size_t streamIndex = 0;
		Serialization::ByteStream& data = message.data;

		const u32 frameIndex = Serialization::ReadNumber<u32>(data, streamIndex);
		const size_t receivedInputsCount = Serialization::ReadNumber<u8>(data, streamIndex);
		std::vector<GameplayInput::FrameState> receivedFrameStates = Utils::ReadInputHistory(data, receivedInputsCount, streamIndex);

		auto it = serverConnections->getInputsRef().find(connectionId);
		if (it != serverConnections->getInputsRef().end())
		{
			Input::InputHistory& inputHistory = it->second;
			const u32 lastStoredFrameIndex = inputHistory.lastInputFrameIdx;
			if (hasNewInput(lastStoredFrameIndex, frameIndex))
			{
				const size_t newFramesCount = frameIndex - lastStoredFrameIndex;
				const size_t newInputsCount = std::min(newFramesCount, receivedInputsCount);
				const size_t resultsOriginalSize = inputHistory.inputs.size();
				const size_t resultsNewSize = resultsOriginalSize + newFramesCount;

				inputHistory.inputs.resize(resultsNewSize);

				// add new elements to the end of the array
				const size_t firstIndexToWrite = resultsNewSize - newInputsCount;
				const size_t firstIndexToRead = receivedInputsCount - newInputsCount;
				for (size_t writeIdx = firstIndexToWrite, readIdx = firstIndexToRead; writeIdx < resultsNewSize; ++writeIdx, ++readIdx)
				{
					inputHistory.inputs[writeIdx] = receivedFrameStates[readIdx];
				}

				// if we have a gap in the inputs, fill it with the last input that we had before or the first input after
				const size_t firstMissingIndex = resultsNewSize - newFramesCount;
				const size_t indexToFillFrom = (resultsOriginalSize > 0) ? (resultsOriginalSize - 1) : firstIndexToWrite;
				const GameplayInput::FrameState& inputToFillWith = inputHistory.inputs[indexToFillFrom];
				for (size_t idx = firstMissingIndex; idx < firstIndexToWrite; ++idx)
				{
					inputHistory.inputs[idx] = inputToFillWith;
				}

				inputHistory.lastInputFrameIdx = frameIndex;
			}
		}
	}
}
