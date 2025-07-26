#include "EngineCommon/precomp.h"

#include "GameUtils/Network/Messages/ServerClient/ConnectionAcceptedMessage.h"

#include "EngineCommon/TimeConstants.h"
#include "EngineCommon/Types/Serialization.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/LogCategories.h"
#include "GameData/Network/NetworkMessageIds.h"

#include "GameUtils/Network/GameStateRewinder.h"

namespace Network::ServerClient
{
	HAL::Network::Message CreateConnectionAcceptedMessage(const u32 updateIdx, const u64 forwardedTimestamp)
	{
		std::vector<std::byte> connectMessageData;

		Serialization::AppendNumber<u32>(connectMessageData, updateIdx);
		Serialization::AppendNumber<u64>(connectMessageData, forwardedTimestamp);

		return HAL::Network::Message{
			static_cast<u32>(NetworkMessageId::ConnectionAccepted),
			connectMessageData
		};
	}

	void ApplyConnectionAcceptedMessage(GameStateRewinder& gameStateRewinder, const u64 timestampNow, const std::span<const std::byte> messagePayload)
	{
		size_t streamIndex = 0;
		const u32 updateIdx = Serialization::ReadNumber<u32>(messagePayload, streamIndex).value_or(0);
		const u64 sentTimestamp = Serialization::ReadNumber<u64>(messagePayload, streamIndex).value_or(0);
		// time in microseconds
		const u64 roundTripTimeUs = timestampNow - sentTimestamp;
		const u64 oneWayTimeUs = roundTripTimeUs / 2;
		const u32 previousClientUpdateIdx = gameStateRewinder.getTimeData().lastFixedUpdateIndex;
		LogInfo(LOG_NETWORK_MESSAGES, "Received connection accepted message on client frame %u with updateIdx: %u RTT: %llums", previousClientUpdateIdx, updateIdx, roundTripTimeUs / 1000ull);

		// estimate the frame we should be simulating on the client
		const u32 estimatedClientUpdateIndex = updateIdx + static_cast<u32>(std::ceil(float(oneWayTimeUs) / float(std::chrono::microseconds(TimeConstants::ONE_FIXED_UPDATE_DURATION).count())));

		// we can still receive updates from updateIdx - 1, so we need to make sure we have enough frames in the history
		const u32 firstStoredUpdateIdx = gameStateRewinder.getFirstStoredUpdateIdx();
		const u32 storedSimulatedUpdatesCount = gameStateRewinder.getTimeData().lastFixedUpdateIndex - 1 - firstStoredUpdateIdx - 1;
		const u32 resultingClientUpdateIndex = std::min(estimatedClientUpdateIndex, updateIdx + storedSimulatedUpdatesCount);

		LogInfo(LOG_NETWORK_MESSAGES, "Estimated client update index: %u, resulting client update index: %u", estimatedClientUpdateIndex, resultingClientUpdateIndex);

		gameStateRewinder.setInitialClientUpdateIndex(resultingClientUpdateIndex);
	}
} // namespace Network::ServerClient
