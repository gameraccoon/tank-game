#include "Base/precomp.h"

#include "Utils/Network/Messages/PlayerInputMessage.h"

#include "Base/Types/Serialization.h"
#include "Base/Types/BasicTypes.h"

#include "GameData/Components/InputHistoryComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/World.h"

#include "Utils/Network/CompressedInput.h"

namespace Network
{
	HAL::ConnectionManager::Message CreatePlayerInputMessage(World& world)
	{
		HAL::ConnectionManager::Message resultMesssage(static_cast<u32>(NetworkMessageId::PlayerInput));
		InputHistoryComponent* inputHistory = world.getNotRewindableWorldComponents().getOrAddComponent<InputHistoryComponent>();

		const size_t inputsSize = inputHistory->getInputs().size();
		const size_t inputsToSend = std::min(inputsSize, Input::MAX_INPUT_HISTORY_SEND_SIZE);

		resultMesssage.reserve(4 + 1 + inputsToSend * ((4 + 4) * 2 + (1 + 8) * 1));

		Serialization::AppendNumber<u32>(resultMesssage.data, inputHistory->getLastInputUpdateIdx());
		static_assert(Input::MAX_INPUT_HISTORY_SEND_SIZE < 256, "u8 is too small to fit input history size");
		Serialization::AppendNumberNarrowCast<u8>(resultMesssage.data, inputsToSend);

		Utils::AppendInputHistory(resultMesssage.data, inputHistory->getInputs(), inputsToSend);

		return resultMesssage;
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

		size_t streamIndex = HAL::ConnectionManager::Message::payloadStartPos;
		std::vector<std::byte>& data = message.data;

		const u32 frameIndex = Serialization::ReadNumber<u32>(data, streamIndex);
		const size_t receivedInputsCount = Serialization::ReadNumber<u8>(data, streamIndex);
		std::vector<GameplayInput::FrameState> receivedFrameStates = Utils::ReadInputHistory(data, receivedInputsCount, streamIndex);

		Input::InputHistory& inputHistory = serverConnections->getInputsRef()[connectionId];
		const u32 lastStoredFrameIndex = inputHistory.lastInputUpdateIdx;
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

			inputHistory.lastInputUpdateIdx = frameIndex;
		}

		if (inputHistory.indexShift == std::numeric_limits<s32>::max())
		{
			const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
			// calculate the shift in the way to use the last received data to process the next fixed update
			inputHistory.indexShift = static_cast<s32>(time->getValue().lastFixedUpdateIndex) - frameIndex + 1;
		}
	}
}
