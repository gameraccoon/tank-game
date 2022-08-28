#include "Base/precomp.h"

#include "Utils/Network/Messages/PlayerInputMessage.h"

#include "Base/Types/Serialization.h"
#include "Base/Types/BasicTypes.h"

#include "GameData/Components/InputHistoryComponent.generated.h"
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

	void ApplyPlayerInputMessage(World& world, const HAL::ConnectionManager::Message& message, ConnectionId connectionId)
	{
		ServerConnectionsComponent* serverConnections = world.getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();

		size_t streamIndex = HAL::ConnectionManager::Message::payloadStartPos;
		const std::vector<std::byte>& data = message.data;

		const u32 clientFrameIndex = Serialization::ReadNumber<u32>(data, streamIndex);
		const size_t receivedInputsCount = Serialization::ReadNumber<u8>(data, streamIndex);
		const std::vector<GameplayInput::FrameState> receivedFrameStates = Utils::ReadInputHistory(data, receivedInputsCount, streamIndex);

		Input::InputHistory& inputHistory = serverConnections->getInputsRef()[connectionId];
		const u32 lastStoredFrameIndex = inputHistory.lastInputUpdateIdx;
		if (hasNewInput(lastStoredFrameIndex, clientFrameIndex))
		{
			const size_t newFramesCount = clientFrameIndex - lastStoredFrameIndex;
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

			inputHistory.lastInputUpdateIdx = clientFrameIndex;
		}

		const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
		const s32 currentIndexShift = static_cast<s32>(time->getValue().lastFixedUpdateIndex) - clientFrameIndex + 1;
		if (inputHistory.indexShift == std::numeric_limits<s32>::max())
		{
			inputHistory.indexShift = currentIndexShift;
		}
		else if (currentIndexShift != inputHistory.indexShift)
		{
			constexpr int INDEX_SHIFT_CHANGE_TOLERANCE = 2;
			++inputHistory.indexShiftIncorrectFrames;
			if (inputHistory.indexShiftIncorrectFrames > INDEX_SHIFT_CHANGE_TOLERANCE)
			{
				// correct index shift after we noticed it's incorrect for several frames
				inputHistory.indexShift = currentIndexShift;
				inputHistory.indexShiftIncorrectFrames = 0;
			}
		}
		else
		{
			inputHistory.indexShiftIncorrectFrames = 0;
		}
	}
}
