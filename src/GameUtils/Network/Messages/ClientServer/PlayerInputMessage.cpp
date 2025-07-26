#include "EngineCommon/precomp.h"

#include "GameUtils/Network/Messages/ClientServer/PlayerInputMessage.h"

#include "EngineCommon/Types/Serialization.h"

#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Input/InputHistory.h"
#include "GameData/LogCategories.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/Network/CompressedInput.h"
#include "GameUtils/Network/GameStateRewinder.h"

namespace Network::ClientServer
{
	HAL::Network::Message CreatePlayerInputMessage(GameStateRewinder& gameStateRewinder)
	{
		// at this point we already has input for the next frame (at least, may be more)
		const u32 lastInputUpdateIdx = gameStateRewinder.getTimeData().lastFixedUpdateIndex + 1;

		const std::vector<GameplayInput::FrameState> inputs = gameStateRewinder.getLastInputs(Input::MAX_INPUT_HISTORY_SEND_SIZE, lastInputUpdateIdx);
		const size_t inputsToSend = std::min(inputs.size(), Input::MAX_INPUT_HISTORY_SEND_SIZE);

		HAL::Network::Message resultMessage(static_cast<u32>(NetworkMessageId::PlayerInput));
		resultMessage.reservePayload(4 + 1 + inputsToSend * ((4 + 4) * 2 + (1 + 8) * 1));
		std::vector<std::byte>& resultMessageData = resultMessage.getDataMutRef();

		Serialization::AppendNumber<u32>(resultMessageData, lastInputUpdateIdx);
		static_assert(Input::MAX_INPUT_HISTORY_SEND_SIZE < std::numeric_limits<u8>::max(), "u8 is too small to fit input history size");
		Serialization::AppendNumberNarrowCast<u8>(resultMessageData, inputsToSend);

		Utils::AppendInputHistory(resultMessageData, inputs, inputsToSend);

		return resultMessage;
	}

	static bool hasNewInput(const u32 oldFrameIndex, const u32 newFrameIndex)
	{
		return oldFrameIndex < newFrameIndex;
	}

	static bool inputIsNotFromFarFuture(const u32 oldFrameIndex, const u32 newFrameIndex)
	{
		return oldFrameIndex + 10 > newFrameIndex;
	}

	void ApplyPlayerInputMessage(WorldLayer& world, GameStateRewinder& gameStateRewinder, const HAL::Network::Message& message, const ConnectionId connectionId)
	{
		size_t streamIndex = 0;
		const std::span<const std::byte> payload = message.getPayloadRef();

		const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
		const u32 lastServerProcessedUpdateIdx = time->getValue()->lastFixedUpdateIndex;

		const u32 lastReceivedInputUpdateIdx = Serialization::ReadNumber<u32>(payload, streamIndex).value_or(0);
		const size_t receivedInputsCount = Serialization::ReadNumber<u8>(payload, streamIndex).value_or(0);

		if (hasNewInput(lastServerProcessedUpdateIdx, lastReceivedInputUpdateIdx) && inputIsNotFromFarFuture(lastServerProcessedUpdateIdx, lastReceivedInputUpdateIdx))
		{
			LogInfo(LOG_NETWORK_MESSAGES, "Processing input message on server frame %u with updateIdx: %u", lastServerProcessedUpdateIdx, lastReceivedInputUpdateIdx);
			// read the input (do it inside the "if", not to waste time on reading the input if it's not needed)
			const std::vector<GameplayInput::FrameState> receivedFrameStates = Utils::ReadInputHistory(payload, receivedInputsCount, streamIndex);

			const u32 firstReceivedUpdateIdx = lastReceivedInputUpdateIdx - static_cast<u32>(receivedInputsCount) + 1;

			// fill missing updates with repeating last input
			if (firstReceivedUpdateIdx > lastServerProcessedUpdateIdx + 1)
			{
				static GameplayInput::FrameState emptyInput;
				const u32 firstStoredUpdateIdx = gameStateRewinder.getFirstStoredUpdateIdx();
				const GameplayInput::FrameState& lastInput = (lastServerProcessedUpdateIdx > firstStoredUpdateIdx) ? gameStateRewinder.getOrPredictPlayerInput(connectionId, lastServerProcessedUpdateIdx) : emptyInput;
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
			LogInfo(LOG_NETWORK_MESSAGES, "Ignoring input message with updateIdx %u on server frame %u", lastReceivedInputUpdateIdx, lastServerProcessedUpdateIdx);
		}

		// we would rather get 2 inputs in advance, so we have one input to lose without it being noticeable
		const u32 idealLastInputUpdateIdx = lastServerProcessedUpdateIdx + 2;

		ServerConnectionsComponent* serverConnections = gameStateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
		serverConnections->getClientDataRef()[connectionId].indexShift = lastReceivedInputUpdateIdx - idealLastInputUpdateIdx;
	}
} // namespace Network::ClientServer
