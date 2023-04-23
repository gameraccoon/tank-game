#include "Base/precomp.h"

#include "Utils/Network/Messages/PlayerInputMessage.h"

#include "Base/Types/Serialization.h"

#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/World.h"

#include "Utils/Network/CompressedInput.h"

namespace Network
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

	void ApplyPlayerInputMessage(World& world, GameStateRewinder& gameStateRewinder, const HAL::ConnectionManager::Message& message, ConnectionId connectionId)
	{
		size_t streamIndex = HAL::ConnectionManager::Message::payloadStartPos;
		const std::vector<std::byte>& data = message.data;

		const u32 clientUpdateIdx = Serialization::ReadNumber<u32>(data, streamIndex);
		const size_t receivedInputsCount = Serialization::ReadNumber<u8>(data, streamIndex);
		const std::vector<GameplayInput::FrameState> receivedFrameStates = Utils::ReadInputHistory(data, receivedInputsCount, streamIndex);

		ServerConnectionsComponent* serverConnections = gameStateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
		OneClientData& clientData = serverConnections->getClientDataRef()[connectionId];

		const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
		const s32 currentIndexShift = static_cast<s32>(time->getValue()->lastFixedUpdateIndex) - static_cast<s32>(clientUpdateIdx) + 1;
		if (clientData.indexShift == std::numeric_limits<s32>::max())
		{
			clientData.indexShift = currentIndexShift;
		}
		else if (currentIndexShift != clientData.indexShift)
		{
			constexpr int INDEX_SHIFT_CHANGE_TOLERANCE = 2;
			++clientData.indexShiftIncorrectFrames;
			if (clientData.indexShiftIncorrectFrames > INDEX_SHIFT_CHANGE_TOLERANCE)
			{
				LogInfo("Correct indexShift from %d to %d", clientData.indexShift, currentIndexShift);
				// correct index shift after we noticed it's incorrect for several frames
				clientData.indexShift = currentIndexShift;
				clientData.indexShiftIncorrectFrames = 0;
			}
		}
		else
		{
			clientData.indexShiftIncorrectFrames = 0;
		}

		const s32 indexShift = clientData.indexShift;

		const u32 lastReceivedInputUpdateIdx = static_cast<u32>(static_cast<s32>(clientUpdateIdx) + indexShift);

		const u32 firstStoredUpdateIdx = gameStateRewinder.getFirstStoredUpdateIdx();
		const u32 lastStoredUpdateIdx = gameStateRewinder.getLastKnownInputUpdateIdxForPlayer(connectionId);
		if (hasNewInput(lastStoredUpdateIdx, lastReceivedInputUpdateIdx))
		{
			const u32 firstReceivedUpdateIdx = lastReceivedInputUpdateIdx - static_cast<u32>(receivedInputsCount) + 1;
			// fill missing updates with repeating last input
			if (lastStoredUpdateIdx < firstReceivedUpdateIdx)
			{
				static GameplayInput::FrameState emptyInput;
				const GameplayInput::FrameState& lastInput = (lastStoredUpdateIdx > firstStoredUpdateIdx) ? gameStateRewinder.getPlayerInput(connectionId, lastStoredUpdateIdx) : emptyInput;
				for (u32 updateIdx = lastStoredUpdateIdx + 1; updateIdx < firstReceivedUpdateIdx; ++updateIdx)
				{
					gameStateRewinder.addPlayerInput(connectionId, updateIdx, lastInput);
				}
			}

			// fill updates with received inputs
			const u32 firstUpdateToFill = std::max(firstReceivedUpdateIdx, lastStoredUpdateIdx + 1);
			for (u32 updateIdx = firstUpdateToFill; updateIdx <= lastReceivedInputUpdateIdx; ++updateIdx)
			{
				const size_t inputIdx = updateIdx - firstReceivedUpdateIdx;
				gameStateRewinder.addPlayerInput(connectionId, updateIdx, receivedFrameStates[inputIdx]);
			}
		}
	}
}
