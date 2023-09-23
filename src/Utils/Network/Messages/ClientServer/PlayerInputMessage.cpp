#include "Base/precomp.h"

#include "Utils/Network/Messages/ClientServer/PlayerInputMessage.h"

#include "Base/Types/Serialization.h"

#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Input/InputHistory.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/World.h"

#include "Utils/Network/CompressedInput.h"
#include "Utils/Network/GameStateRewinder.h"

namespace Network::ClientServer
{
	HAL::ConnectionManager::Message CreatePlayerInputMessage(GameStateRewinder& gameStateRewinder)
	{
		HAL::ConnectionManager::Message resultMessage(static_cast<u32>(NetworkMessageId::PlayerInput));

		const std::vector<GameplayInput::FrameState> inputs = gameStateRewinder.getLastInputs(Input::MAX_INPUT_HISTORY_SEND_SIZE);
		const size_t inputsToSend = std::min(inputs.size(), Input::MAX_INPUT_HISTORY_SEND_SIZE);

		resultMessage.reserve(4 + 1 + inputsToSend * ((4 + 4) * 2 + (1 + 8) * 1));

		Serialization::AppendNumber<u32>(resultMessage.data, gameStateRewinder.getTimeData().lastFixedUpdateIndex);
		static_assert(Input::MAX_INPUT_HISTORY_SEND_SIZE < std::numeric_limits<u8>::max(), "u8 is too small to fit input history size");
		Serialization::AppendNumberNarrowCast<u8>(resultMessage.data, inputsToSend);

		Utils::AppendInputHistory(resultMessage.data, inputs, inputsToSend);

		return resultMessage;
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

		if ALMOST_NEVER (oldAboutToWrap && newAfterWrap)
		{
			return true;
		}
		else if ALMOST_NEVER (newAboutToWrap && oldAfterWrap)
		{
			return false;
		}
		else
		{
			return oldFrameIndex < newFrameIndex;
		}
	}

	void ApplyPlayerInputMessage(World& world, GameStateRewinder& gameStateRewinder, const HAL::ConnectionManager::Message& message, ConnectionId connectionId)
	{
		size_t streamIndex = HAL::ConnectionManager::Message::payloadStartPos;
		const std::vector<std::byte>& data = message.data;

		const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
		const u32 lastServerProcessedUpdateIdx = time->getValue()->lastFixedUpdateIndex;

		const u32 lastReceivedInputUpdateIdx = Serialization::ReadNumber<u32>(data, streamIndex).value_or(0);
		const size_t receivedInputsCount = Serialization::ReadNumber<u8>(data, streamIndex).value_or(0);

		LogInfo("Received input message on server frame %u with updateIdx: %u", lastServerProcessedUpdateIdx, lastReceivedInputUpdateIdx);

		if (hasNewInput(lastServerProcessedUpdateIdx, lastReceivedInputUpdateIdx))
		{
			// read the input (do it inside the "if", not to waste time on reading the input if it's not needed)
			const std::vector<GameplayInput::FrameState> receivedFrameStates = Utils::ReadInputHistory(data, receivedInputsCount, streamIndex);

			const u32 firstReceivedUpdateIdx = lastReceivedInputUpdateIdx - static_cast<u32>(receivedInputsCount) + 1;

			// fill missing updates with repeating last input
			if (firstReceivedUpdateIdx > lastServerProcessedUpdateIdx + 1)
			{
				static GameplayInput::FrameState emptyInput;
				const u32 firstStoredUpdateIdx = gameStateRewinder.getFirstStoredUpdateIdx();
				const GameplayInput::FrameState& lastInput = (lastServerProcessedUpdateIdx > firstStoredUpdateIdx) ? gameStateRewinder.getPlayerInput(connectionId, lastServerProcessedUpdateIdx) : emptyInput;
				for (u32 updateIdx = lastServerProcessedUpdateIdx + 1; updateIdx < firstReceivedUpdateIdx; ++updateIdx)
				{
					gameStateRewinder.addPlayerInput(connectionId, updateIdx, lastInput);
				}
			}

			// fill updates with received inputs
			const u32 firstUpdateToFill = std::max(firstReceivedUpdateIdx, lastServerProcessedUpdateIdx + 1);
			for (u32 updateIdx = firstUpdateToFill; updateIdx <= lastReceivedInputUpdateIdx; ++updateIdx)
			{
				const size_t inputIdx = updateIdx - firstReceivedUpdateIdx;
				gameStateRewinder.addPlayerInput(connectionId, updateIdx, receivedFrameStates[inputIdx]);
			}
		}
		else
		{
			// we got no new input, we may want to send a message to the client to notify about that
			// also we may need to prepare some buffer to delay applying player input for the next frames
		}
	}
} // namespace Network::ClientServer
