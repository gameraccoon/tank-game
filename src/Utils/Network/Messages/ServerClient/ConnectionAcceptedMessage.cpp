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
		const u32 updateIdx = Serialization::ReadNumber<u32>(message.data, streamIndex);
		const u64 sentTimestamp = Serialization::ReadNumber<u64>(message.data, streamIndex);
		// time in microseconds
		const u64 roundTripTimeUs = timestampNow - sentTimestamp;
		LogInfo("Received connection accepted message on client frame %u with updateIdx: %u RTT: %llums", gameStateRewinder.getTimeData().lastFixedUpdateIndex, updateIdx, roundTripTimeUs / 1000ull);

		// estimate the frame we should be simulating on the client
		const u32 estimatedClientFrameIndex = updateIdx + static_cast<u32>(std::ceil(float(roundTripTimeUs) / float(std::chrono::microseconds(TimeConstants::ONE_FIXED_UPDATE_DURATION).count())));
		LogInfo("Estimated client frame index: %u", estimatedClientFrameIndex);
		gameStateRewinder.setInitialClientFrameIndex(estimatedClientFrameIndex);
	}
} // namespace Network::ServerClient
