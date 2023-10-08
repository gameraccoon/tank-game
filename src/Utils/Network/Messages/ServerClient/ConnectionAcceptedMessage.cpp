#include "Base/precomp.h"

#include "Utils/Network/Messages/ServerClient/ConnectionAcceptedMessage.h"

#include "Base/TimeConstants.h"
#include "Base/Types/Serialization.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/EcsDefinitions.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/Network/NetworkProtocolVersion.h"

#include "Utils/Network/GameStateRewinder.h"

namespace Network::ServerClient
{
	HAL::ConnectionManager::Message CreateConnectionAcceptedMessage(u32 updateIdx, u64 forwardedTimestamp)
	{
		std::vector<std::byte> connectMessageData;

		Serialization::AppendNumber<u32>(connectMessageData, updateIdx);
		Serialization::AppendNumber<u64>(connectMessageData, forwardedTimestamp);

		return HAL::ConnectionManager::Message{
			static_cast<u32>(NetworkMessageId::ConnectionAccepted),
			connectMessageData
		};
	}

	void ApplyConnectionAcceptedMessage(GameStateRewinder& gameStateRewinder, u64 timestampNow, const HAL::ConnectionManager::Message& message)
	{
		size_t streamIndex = HAL::ConnectionManager::Message::payloadStartPos;
		const u32 updateIdx = Serialization::ReadNumber<u32>(message.data, streamIndex).value_or(0);
		const u64 sentTimestamp = Serialization::ReadNumber<u64>(message.data, streamIndex).value_or(0);
		// time in microseconds
		const u64 roundTripTimeUs = timestampNow - sentTimestamp;
		const u64 oneWayTimeUs = roundTripTimeUs / 2;
		const u32 previousClientUpdateIdx = gameStateRewinder.getTimeData().lastFixedUpdateIndex;
		LogInfo("Received connection accepted message on client frame %u with updateIdx: %u RTT: %llums", previousClientUpdateIdx, updateIdx, roundTripTimeUs / 1000ull);

		// estimate the frame we should be simulating on the client
		const u32 estimatedClientUpdateIndex = updateIdx + static_cast<u32>(std::ceil(float(oneWayTimeUs) / float(std::chrono::microseconds(TimeConstants::ONE_FIXED_UPDATE_DURATION).count())));

		// we can still receive updates from updateIdx - 1, so we need to make sure we have enough frames in the history
		const u32 firstStoredUpdateIdx = gameStateRewinder.getFirstStoredUpdateIdx();
		const u32 storedSimulatedUpdatesCount = gameStateRewinder.getTimeData().lastFixedUpdateIndex - 1 - firstStoredUpdateIdx - 1;
		const u32 resultingClientUpdateIndex = std::min(estimatedClientUpdateIndex, updateIdx + storedSimulatedUpdatesCount);

		LogInfo("Estimated client update index: %u, resulting client update index: %u", estimatedClientUpdateIndex, resultingClientUpdateIndex);

		gameStateRewinder.setInitialClientUpdateIndex(resultingClientUpdateIndex);
	}
} // namespace Network::ServerClient
